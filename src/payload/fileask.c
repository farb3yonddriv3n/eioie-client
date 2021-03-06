#include <common.h>

int fileask_write(struct data_s *d, void *userdata)
{
    struct peer_s *p = (struct peer_s *)userdata;
    if (!p || !d) return -1;
    if (p->send_buffer.type != BUFFER_FILEASK) return -1;
    ifr(data.write.raw(d, (char *)p->send_buffer.u.fileask.file,
                       SHA256HEX));
    ifr(data.write.raw(d, (char *)p->send_buffer.u.fileask.chunk,
                       SHA256HEX))
    return 0;
}

int fileask_read(struct peer_s *p)
{
    if (!p) return -1;
    if (p->user.cb.fileask)
        ifr(p->user.cb.fileask(p,
                               p->received.header.src.host,
                               p->received.header.src.port,
                               p->recv_buffer.available->data.s,
                               p->recv_buffer.available->data.n));
    return 0;
}

int fileask_size(int *sz, void *userdata)
{
    struct peer_s *p = (struct peer_s *)userdata;
    if (!p || !sz) return -1;
    if (p->send_buffer.type != BUFFER_FILEASK) return -1;
    *sz = SHA256HEX * 2;
    return 0;
}
