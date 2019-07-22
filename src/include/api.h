#ifndef API_H_
#define API_H_

enum api_e {
    API_LISTPEERS,
    API_MESSAGE,
    API_LISTFILES_LOCAL,
    API_LISTFILES_REMOTE,
    API_PEER_ONLINE,
    API_PEER_OFFLINE,
    API_JOBSDUMP,
    API_JOBDONE,
    API_JOBADD,
    API_TSHARE,
    API_BMINE,
    API_BADVERTISE,
    API_BMINING,
    API_ROGUEDUMP,
    API_VERSIONDUMP,
};

struct module_api_s {
    int (*write)(struct peer_s *p, enum api_e cmd, json_object *payload,
                 json_object *request, int dfserr);
    int (*read)();
    //int (*clean)();
    struct {
        void (*read)(EV_P_ ev_io *w, int revents);
        void (*write)(EV_P_ ev_io *w, int revents);
    } ev;
};

int api_message_write(struct peer_s *p, int host, unsigned short port,
                      char *msg, int len);
int api_peer_online(struct peer_s *p, struct world_peer_s *wp);
int api_peer_offline(struct peer_s *p, struct world_peer_s *wp);
int api_job_done(struct peer_s *p, unsigned char *filename,
                 int nfilename);

extern const struct module_api_s api;

#endif