#include <common.h>

#define KB_TO_BYTES(m_dst) (m_dst * 1024)

static int update(ssize_t bytes, bool *suspend, double retry,
                  double *start, size_t *total_bytes, int limit)
{
    if (!suspend || !start || !total_bytes || bytes < 0) return -1;
    *suspend = false;
    double timestampms;
    if (os.gettimems(&timestampms) != 0) return -1;
    if (*start == .0f || (timestampms - 1.0f) >= *start) {
        *total_bytes = bytes;
        return os.gettimems(start);
    }
    if (*total_bytes + bytes >= KB_TO_BYTES(limit)) {
        *suspend = true;
        return 0;
    }
    *total_bytes += bytes;
    return 0;
}

static int update_send(struct peer_s *p, ssize_t bytes, bool *suspend)
{
    return update(bytes, suspend, p->cfg.net.interval.retry,
                  &p->traffic.send.start, &p->traffic.send.bytes,
                  p->cfg.net.max.upload);
}

static int update_recv(struct peer_s *p, ssize_t bytes, bool *suspend)
{
    ifr(update(bytes, suspend, p->cfg.net.interval.retry,
               &p->traffic.recv.start, &p->traffic.recv.bytes,
               p->cfg.net.max.download));
    if (*suspend) return net.suspend(&p->ev);
    return 0;
}

static int dump(struct peer_s *p, json_object **obj)
{
    if (!p || !obj) return -1;
    *obj = json_object_new_object();
    if (!(*obj)) return -1;
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%4ldkB", p->traffic.recv.bytes / 1024);
    json_object *recvbytes = json_object_new_string(buffer);
    snprintf(buffer, sizeof(buffer), "%4ldkB", p->traffic.send.bytes / 1024);
    json_object *sendbytes = json_object_new_string(buffer);
    json_object_object_add(*obj, "download", recvbytes);
    json_object_object_add(*obj, "upload", sendbytes);
    return 0;
}

const struct module_traffic_s traffic = {
    .update.send = update_send,
    .update.recv = update_recv,
    .dump        = dump,
};
