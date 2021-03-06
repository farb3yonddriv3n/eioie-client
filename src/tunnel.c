#include <common.h>

static void tunnel_data(struct gc_gen_client_s *client, char *buf, int len)
{
    struct tunnel_s *t = (struct tunnel_s *)client->base.tunnel;
    struct tcp_s tcp = { .port.src = t->tcp.src,
                         .port.dst = t->tcp.dst,
                         .cidx     = client->base.fd,
                         .reqtype  = TCP_REQUEST };
    unsigned char filename[SHA256HEX];
    sha256hex((unsigned char *)buf, len, filename);
    char filenamestr[256];
    snprintf(filenamestr, sizeof(filenamestr), "%s/%.*s", t->peer->cfg.dir.tcp,
                                                          SHA256HEX, filename);
    os.filewrite(filenamestr, "wb", buf, len);
    task.add(t->peer, t->peer->cfg.dir.tcp, filename, sizeof(filename),
             t->remote.host, t->remote.port, NULL,
             TASK_FILE_DELETE, &tcp);
}

static int find(struct list_s *l, void *ex, void *ud)
{
    if (!l || !ex) return -1;
    struct tunnel_s *et = (struct tunnel_s *)ex;
    struct tunnel_s *nt = (struct tunnel_s *)ud;
    if (et->remote.host == nt->remote.host &&
        et->remote.port == nt->remote.port &&
        et->tcp.dst     == nt->tcp.dst) {
        nt->found = et;
        return 1;
    }
    return 0;
}

static int response(struct peer_s *p, struct header_s *h, char *buf, int len)
{
    struct client_find_s {
        unsigned short         src;
        int                    cidx;
        struct gc_gen_client_s *client;
    };
    int client_find(struct list_s *l, void *ex, void *ud) {
        struct tunnel_s *t = (struct tunnel_s *)ex;
        struct client_find_s *cf = (struct client_find_s *)ud;
        if (t->tcp.src != cf->src) return 0;
        char key[16];
        snprintf(key, sizeof(key), "%d", cf->cidx);
        struct ht_item_s *kv = ht_get(t->server->clients, key, strlen(key));
        if (!kv) return 0;
        cf->client = (struct gc_gen_client_s *)kv->v;
        return 1;
    }
    struct client_find_s cf = { .src    = h->tcp.port.src,
                                .cidx   = h->tcp.cidx,
                                .client = NULL };
    ifr(list.map(&p->tcp.tunnels, client_find, &cf));
    if (!cf.client) return 0;
    bool sent;
    ifr(packet.tcpsent(cf.client, h->tidx, &sent));
    if (sent) return 0;
    return packet.tcpsend(cf.client, buf, len, h->tidx);
}

static int clean(void *ud)
{
    if (!ud) return -1;
    struct tunnel_s *t = (struct tunnel_s *)ud;
    async_server_shutdown(t->server);
    free(t);
    return 0;
}

static int tunnel_open(struct peer_s *p, unsigned char *pubkeyhash,
                       unsigned short *port_local,
                       unsigned short dstport, bool *success)
{
    if (!p || !pubkeyhash || !port_local || !success) return -1;
    *success = false;
    *port_local = 0;
    struct world_peer_s wp = { .found = NULL };
    memcpy(wp.pubkeyhash, pubkeyhash, sizeof(wp.pubkeyhash));
    ifr(list.map(&p->peers, world.peer.findpubkeyhash, &wp));
    if (!wp.found) return -1;
    struct tunnel_s ft = { .remote.host = wp.found->host,
                           .remote.port = wp.found->port,
                           .tcp.dst     = dstport,
                           .found       = NULL };
    wp.host = wp.found->host;
    wp.port = wp.found->port;
    ifr(list.map(&p->tcp.tunnels, find, &ft));
    if (ft.found) return 0; // tunnel already opened
    struct tunnel_s *t;
    t = malloc(sizeof(*t));
    if (!t) return -1;
    memset(t, 0, sizeof(*t));
    struct gc_gen_server_s *server = malloc(sizeof(*server));
    memset(server, 0, sizeof(*server));
    t->peer   = p;
    t->server = server;
    server->loop = p->ev.loop;
    server->log  = &p->log;
    server->pool = NULL;
    server->callback.data = tunnel_data;
    server->host = "0.0.0.0";
    server->port = "0";
    t->remote.host = wp.host;
    t->remote.port = wp.port;
    t->tcp.dst     = dstport;
    memcpy(t->remote.pubkeyhash, pubkeyhash, sizeof(t->remote.pubkeyhash));
    int ret;
    ret = async_server(server, t);
    if (ret != GC_OK) {
        free(t);
        return ret;
    }
    *port_local = t->tcp.src = server->port_local;
    ifr(list.add(&p->tcp.tunnels, t, clean));
    *success = true;
    return 0;
}

static int tunnel_close(struct peer_s *p, unsigned char *pubkeyhash,
                        unsigned short dstport, bool *closed)
{
    if (!p || !pubkeyhash || !closed) return -1;
    *closed = false;
    struct world_peer_s wp = { .found = NULL };
    memcpy(wp.pubkeyhash, pubkeyhash, sizeof(wp.pubkeyhash));
    ifr(list.map(&p->peers, world.peer.findpubkeyhash, &wp));
    if (!wp.found) return -1;
    struct tunnel_s ft = { .remote.host = wp.found->host,
                           .remote.port = wp.found->port,
                           .tcp.dst     = dstport,
                           .found       = NULL };
    ifr(list.map(&p->tcp.tunnels, find, &ft));
    if (!ft.found) return 0;
    ifr(list.del(&p->tcp.tunnels, ft.found));
    *closed = true;
    return 0;
}

static int dump(struct peer_s *p, json_object **obj)
{
    int cb(struct list_s *l, void *ut, void *ud) {
        struct tunnel_s *t     = (struct tunnel_s *)ut;
        json_object   *tunnels = (json_object *)ud;
        json_object *port = json_object_new_int(t->remote.port);
        json_object *pkh  = json_object_new_string_len((const char *)t->remote.pubkeyhash,
                                                       sizeof(t->remote.pubkeyhash));
        json_object *src_port = json_object_new_int(t->tcp.src);
        json_object *dst_port = json_object_new_int(t->tcp.dst);
        int clients;
        ifr(ht_items(t->server->clients, &clients));
        json_object *jclients = json_object_new_int(clients);
        json_object *jt = json_object_new_object();
        char hostbuf[32];
        snprintf(hostbuf, sizeof(hostbuf), "%x", t->remote.host);
        json_object *jhost = json_object_new_string_len(hostbuf, strlen(hostbuf));
        json_object_object_add(jt, "host", jhost);
        json_object_object_add(jt, "port", port);
        json_object_object_add(jt, "pubkeyhash", pkh);
        json_object_object_add(jt, "src_port", src_port);
        json_object_object_add(jt, "dst_port", dst_port);
        json_object_object_add(jt, "clients", jclients);
        json_object_array_add(tunnels, jt);
        return 0;
    }
    if (!p) return -1;
    *obj = json_object_new_object();
    json_object *jtunnels = json_object_new_array();
    json_object_object_add(*obj, "tunnels", jtunnels);
    int count;
    ifr(list.size(&p->tcp.tunnels, &count));
    json_object *jcount = json_object_new_int(count);
    json_object_object_add(*obj, "count", jcount);
    return list.map(&p->tcp.tunnels, cb, jtunnels);
}

const struct module_tunnel_s tunnel = {
    .open     = tunnel_open,
    .close    = tunnel_close,
    .response = response,
    .dump     = dump,
};
