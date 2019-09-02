var ws = undefined;
var files_local = [];
var files_remote = [];
var filter = "";
var outbound = [];
var request_id = 0;
var server = [
    ["message",              message]
];

function connection_status(status)
{
    var cn = document.getElementById("connected");
    if(status) {
        cn.innerHTML = "Connected";
    } else {
        cn.innerHTML = "Disconnected";
    }
}

function delayed_request(func, cmd, payload, delay)
{
    setTimeout(function() {
               func(cmd, payload);
               }, delay);
}

function getdate(ts)
{
    var currentdate = new Date(ts);
    var datetime = currentdate.getDate() + "/"
        + (currentdate.getMonth()+1)  + "/"
        + currentdate.getFullYear() + " "
        + currentdate.getHours() + ":"
        + currentdate.getMinutes() + ":"
        + currentdate.getSeconds();
    return datetime;
}

function menu_files(src)
{
    var filesLocalList  = document.getElementById("filesLocalList");
    var filesRemoteList = document.getElementById("filesRemoteList");
    filesLocalList.style.display = "none";
    filesRemoteList.style.display = "none";

    if (src == "local")       filesLocalList.style.display = "block";
    else if (src == "remote") filesRemoteList.style.display = "block";


    var fileItemBody = document.getElementById("fileItemBody");
    fileItemBody.innerHTML = "none";

    var fileItem = document.getElementById("fileItem");
    fileItem.style.display = "none";
}

function showFile(file)
{
    var filesLocalList = document.getElementById("filesLocalList");
    filesLocalList.style.display = "none";
    var filesRemoteList = document.getElementById("filesRemoteList");
    filesRemoteList.style.display = "none";
    var fileItemBody = document.getElementById("fileItemBody");
    var fileItemTitle = document.getElementById("fileItemTitle");
    fileItemBody.innerHTML = "";
    fileItemTitle.innerHTML = "";
    var desc = document.createElement("div");
    if (file.endsWith("mp3")) {
        desc.innerHTML = "<audio controls><source src=\"assets/media/"+file+"\" type=\"audio/mpeg\"></audio>";
        desc.setAttribute("class", "fileAudio");
    } else if(file.endsWith("mp4")) {
        desc.innerHTML = "<video width=\"800\" height=\"600\" controls><source src=\"assets/media/"+file+"\" type=\"video/mp4\"></video>";
        desc.setAttribute("class", "fileVideo");
    } else {
        desc.innerHTML = "<a href=\"assets/media/"+file+"\">"+file+"</a>";
        desc.setAttribute("class", "fileGeneric");
    }
    fileItemBody.appendChild(desc);
    fileItemTitle.innerHTML = file;
    var fileItem = document.getElementById("fileItem");
    fileItem.style.display = "block";
}

function file_onclick(element, desc, type)
{
    element.onclick = function() {
        if (type == "show_files")
            showFile(desc);
        else if (type == "job_add")
            job_add(desc);
        else if (type == "job_finalize")
            job_finalize(desc);
    }
}

function search_remote()
{
    filter = document.getElementById("searchRemote").value;
    files_show();
}

function format_size(size)
{
    if (size < 1024)                      return size + "b";
    else if (size < (1024 * 1024))        return (size / 1024).toFixed(2) + "kb";
    else if (size < (1024 * 1024 * 1024)) return (size / (1024 * 1024)).toFixed(2) + "mb";
    else                                  return (size / (1024 * 1024 * 1024)).toFixed(2) + "gb";
}

function files_show_sub(src)
{
    var files = document.getElementById("filesRemote");
    for (added = 0, i = 0; i < src.length; i++) {
        var transaction = src[i];
        if (filter.length > 0 &&
            !(transaction["description"].toLowerCase().indexOf(filter) != -1 ||
                transaction["tags"].toLowerCase().indexOf(filter) != -1)) continue;
        added++;
        var transactionDiv = document.createElement("div");
        transactionDiv.setAttribute("class", "filesItem");
        var name;
        var local = (transaction["src"] == "local") ? " (local)" : "";
        if (transaction["decryptable"])
            name = files_item_sub(transactionDiv, transaction["description"] + local);
        else
            name = files_item_sub(transactionDiv, transaction["name"] + local);
        name.classList.add("fileShowName");
        var size = files_item_sub(transactionDiv, format_size(transaction["size"]));
        size.classList.add("fileShowSize");
        var complete = files_item_sub(transactionDiv, transaction["complete"]);
        if (!transaction["complete"]) {
            file_onclick(complete, transaction["name"], "job_add");
            complete.setAttribute("class", "filesItemSubClick");
        }
        complete.classList.add("fileShowComplete");
        var finalize = files_item_sub(transactionDiv, "Finalize");
        file_onclick(finalize, transaction["name"], "job_finalize");
        finalize.setAttribute("class", "filesItemSubClick");
        finalize.classList.add("fileShowSize");
        var finalized;
        if (transaction["finalized"]) {
            finalized = files_item_sub(transactionDiv, "View");
            file_onclick(finalized, transaction["description"], "show_files");
            finalized.setAttribute("class", "filesItemSubClick");
        } else {
            finalized = files_item_sub(transactionDiv, "Encrypted");
        }
        finalized.classList.add("fileShowFinalized");
        var tags = files_item_sub(transactionDiv, transaction["tags"]);
        tags.classList.add("fileShowTags");
        var jobs;
        if ("jobs" in transaction)
            jobs = files_item_sub(transactionDiv, transaction["jobs"]);
        else
            jobs = files_item_sub(transactionDiv, transaction["chunks_done"] + "/" +
                                                  transaction["chunks_total"]);
        jobs.classList.add("fileShowTags");
        var tasks;
        if ("tasks" in transaction)
            tasks = files_item_sub(transactionDiv, transaction["tasks"].length);
        else
            tasks = files_item_sub(transactionDiv, 0);
        tasks.classList.add("fileShowTags");
        files.appendChild(transactionDiv);
    }
    return added;
}

function files_show()
{
    var added = 0;
    var files = document.getElementById("filesRemote");
    files.innerHTML = "";
    added += files_show_sub(files_remote);
    added += files_show_sub(files_local);
    if (!added) files.innerHTML = "No files found.";
}

function message_files_local(payload)
{
    files_local = [];
    for (i = 0; i < payload["blocks"].length; i++)
        for (t = 0; t < payload["blocks"][i]["transactions"].length; t++) {
            payload["blocks"][i]["transactions"][t]["src"] = "local";
            files_remote.push(payload["blocks"][i]["transactions"][t]);
        }
}

function files_item_sub(dst, innerHtml)
{
    var div = document.createElement("div");
    div.setAttribute("class", "filesItemSub");
    div.innerHTML = innerHtml;
    dst.appendChild(div);
    return div;
}

function message_files_remote(parsed)
{
    files_remote = [];
    for (i = 0; i < parsed["payload"]["roots"].length; i++)
        for (b = 0; b < parsed["payload"]["roots"][i]["blocks"].length; b++)
            for (t = 0; t < parsed["payload"]["roots"][i]["blocks"][b]["transactions"].length; t++) {
                parsed["payload"]["roots"][i]["blocks"][b]["transactions"][t]["src"] = "remote";
                files_remote.push(parsed["payload"]["roots"][i]["blocks"][b]["transactions"][t]);
            }
    message_files_local(parsed["payload"]["local"]);
    files_show();
}

function message_file_job_done(parsed)
{
    for (i = 0; i < files_remote.length; i++)
        if (files_remote[i]["name"] == parsed["payload"]["name"]) {
            files_remote[i]["complete"] = true;
            job_finalize(files_remote[i]["name"]);
            break;
        }
    files_show();
}

function message_file_job_finalized(parsed)
{
    for (i = 0; i < files_remote.length; i++)
        if (files_remote[i]["name"] == parsed["request"]["name"]) {
            files_remote[i]["finalized"] = true;
            break;
        }
    files_show();
}

function message_traffic(parsed)
{
    var traffic = document.getElementById("traffic");
    traffic.innerHTML = "Download: " + parsed["payload"]["download"] +
                        " Upload: " + parsed["payload"]["upload"];
}

function message_tasks_sub(dst, parsed)
{
    for (i = 0; i < dst.length; i++) {
        dst[i]["tasks"] = [];
        for (t = 0; t < parsed["payload"]["tasks"].length; t++) {
            if (parsed["payload"]["tasks"][t]["name"] == dst[i]["name"]) {
                dst[i]["tasks"].push(parsed["payload"]["tasks"][t]["size"]);
            }
        }
    }
}

function message_tasks(parsed)
{
    message_tasks_sub(files_local, parsed);
    message_tasks_sub(files_remote, parsed);
    files_show();
}

function message_jobs_dump(parsed)
{
    for (i = 0; i < files_remote.length; i++) {
        for (j = 0; j < parsed["payload"]["jobs"].length; j++) {
            if (files_remote[i]["name"] == parsed["payload"]["jobs"][j]["name"]) {
                files_remote[i]["jobs"] = parsed["payload"]["jobs"][j]["counter_done"] + "/" +
                                          parsed["payload"]["jobs"][j]["chunks_size"];
                break;
            }
        }
    }
    files_show();
}

function message(payload)
{
    var parsed = JSON.parse(payload);
    if (parsed["command"] == 2 && "blocks" in parsed["payload"]) {
        message_files_local(parsed["payload"]);
    } else if (parsed["command"] == 3 && "roots" in parsed["payload"]) {
        message_files_remote(parsed);
    } else if (parsed["command"] == 1) {
    } else if (parsed["command"] == 6) {
        message_jobs_dump(parsed);
    } else if (parsed["command"] == 7) {
        message_file_job_done(parsed);
    } else if (parsed["command"] == 9) {
        message_file_job_finalized(parsed);
    } else if (parsed["command"] == 16) {
        message_traffic(parsed);
    } else if (parsed["command"] == 17) {
        message_tasks(parsed);
    } else {
    }
}

function files_get(src)
{
    var payload = new Object();
    payload.src = src;
    send("files_get", payload);
}

function job_add(name)
{
    var payload = new Object();
    payload.name = name;
    send("job_add", payload);
}

function job_finalize(name)
{
    var payload = new Object();
    payload.name = name;
    send("job_finalize", payload);
}

function send(cmd, payload)
{
    var json = new Object();
    json.command = cmd;
    json.payload = payload;
    json.version = 1;
    json.request_id = request_id;
    var text = JSON.stringify(json);
    var rq = new Object();
    rq.id = request_id++;
    rq.time = Date.now();
    outbound.push(rq);
    ws.send(text);
}

function status(cmd, payload)
{
    if (ws == undefined) return "disconnected";
    else                 return "connected";
}

function confirm_message(payload)
{
    var json = JSON.parse(payload);
    if (!("request" in json) || ("request" in json && json["request"] == null))
        return;
    for (i = 0; i < outbound.length; i++) {
        if (json["request"]["request_id"] == outbound[i].id) {
            outbound.splice(i, 1);
            break;
        }
    }
}

function confirmed_check(p1, p2)
{
    for (i = 0; i < outbound.length; i++) {
        if (outbound[i].time < (Date.now() - 5000)) {
            var todiv = document.getElementById("timedout");
            todiv.innerHTML = "Request id " + outbound[i].id + " timed out.";
            $("#timedout").fadeIn(2000);
            $("#timedout").fadeOut(2000);
            outbound.splice(i, 1);
        }
    }
    delayed_request(confirmed_check, "", "", 1000);
}

function connect(cmd, payload)
{
    if (!("WebSocket" in window))
        return "this browser doesn't support websocket";

    if (ws != undefined)
        return "already connected";

    ws = new WebSocket("ws://localhost:8080/websocket");

    ws.onopen = function() {
        connection_status(true);
        menu_files("remote");
        files_get("remote");
        //files_get("local");
        delayed_request(confirmed_check, "", "", 1000);
    };

    ws.onmessage = function (evt) {
        var json = JSON.parse(evt.data);
        for (var i = 0; i < server.length; i++) {
            if (server[i][0] == json.command) {
                if ("payload" in json)
                    confirm_message(json["payload"]);
                server[i][1](json["payload"]);
                return;
            }
        }
    };

    ws.onclose = function() {
        ws = undefined;
        connection_status(false);
        delayed_request(connect, cmd, payload, 1000);
    };
}
