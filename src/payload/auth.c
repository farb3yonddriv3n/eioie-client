#include <common.h>

int auth_write(struct data_s *d, void *userdata)
{
    struct peer_s *p = (struct peer_s *)userdata;
    if (!p || !d) return -1;
    if (p->send_buffer.type != BUFFER_AUTH) return -1;
    ifr(data.write.raw(d, (char *)p->send_buffer.u.auth.str.s,
                       p->send_buffer.u.auth.str.n));
    return 0;
}

int auth_read(struct peer_s *p)
{
    if (!p) return -1;
    if (p->user.cb.auth)
        ifr(p->user.cb.auth(p,
                            p->received.header.src.host,
                            p->received.header.src.port,
                            p->recv_buffer.available->data.s,
                            p->recv_buffer.available->data.n));
    return 0;
}

int auth_size(int *sz, void *userdata)
{
    struct peer_s *p = (struct peer_s *)userdata;
    if (!p || !sz) return -1;
    if (p->send_buffer.type != BUFFER_AUTH) return -1;
    *sz = p->send_buffer.u.auth.str.n;
    return 0;
}
