#ifndef API_H_
#define API_H_

struct module_api_s {
    int (*write)(struct peer_s *p, const char *json, int len);
    int (*read)();
    //int (*clean)();
    struct {
        void (*read)(EV_P_ ev_io *w, int revents);
        void (*write)(EV_P_ ev_io *w, int revents);
    } ev;
};

int api_message_write(struct peer_s *p, int host, unsigned short port,
                      char *msg, int len);

extern const struct module_api_s api;

#endif
