#include <common.h>

int queryrpl_write(struct data_s *d, void *userdata)
{
    struct peer_s *p = (struct peer_s *)userdata;
    if (p->send_buffer.type != BUFFER_QUERY_REPLY) return -1;
    if (!p || !d) return -1;
    if (data.write.integer(d, p->send_buffer.u.query_reply.host) != 0) return -1;
    if (data.write.shortint(d, p->send_buffer.u.query_reply.port) != 0) return -1;
    if (data.write.byte(d, p->send_buffer.u.query_reply.reachable) != 0) return -1;
    return 0;
}

int queryrpl_read(struct peer_s *p)
{
    if (!p) return -1;
    sn_initr(bf, p->recv_buffer.available->data.s,
             p->recv_buffer.available->data.n);
    int host;
    unsigned short port;
    char reachable;
    printf("query reply read\n");
    if (sn_read((void *)&host, sizeof(host), &bf) != 0) return -1;
    if (sn_read((void *)&port, sizeof(port), &bf) != 0) return -1;
    if (sn_read((void *)&reachable, sizeof(reachable), &bf) != 0) return -1;
    if (p->user.cb.query_reply) {
        printf("query reply read1\n");
        ifr(p->user.cb.query_reply(p, ADDR_IP(p->net.remote.addr),
                                   ADDR_PORT(p->net.remote.addr),
                                   host, port, reachable));
    }
    return 0;
}

int queryrpl_size(int *sz, void *userdata)
{
    if (!sz) return -1;
    *sz = DATA_SIZE_INT + \
          DATA_SIZE_SHORT + \
          DATA_SIZE_BYTE;
    return 0;
}
