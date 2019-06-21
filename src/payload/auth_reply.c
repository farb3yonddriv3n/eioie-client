#include <common.h>

int authrpl_write(struct data_s *d, void *userdata)
{
    struct peer_s *p = (struct peer_s *)userdata;
    if (!p || !d) return -1;
    if (p->send_buffer.type != BUFFER_AUTH_REPLY) return -1;
    if (data.write.raw(d, (char *)p->send_buffer.u.auth.str.s,
                       p->send_buffer.u.auth.str.n) != 0) return -1;
    return 0;
}

int authrpl_read(struct peer_s *p)
{
    if (!p) return -1;
    if (p->user.cb.authrpl)
        ifr(p->user.cb.authrpl(p,
                               ADDR_IP(p->net.remote.addr),
                               ADDR_PORT(p->net.remote.addr),
                               p->recv_buffer.available->data.s,
                               p->recv_buffer.available->data.n));
    return 0;
}

int authrpl_size(int *sz, void *userdata)
{
    struct peer_s *p = (struct peer_s *)userdata;
    if (!p || !sz) return -1;
    if (p->send_buffer.type != BUFFER_AUTH_REPLY) return -1;
    *sz = p->send_buffer.u.auth.str.n;
    return 0;
}