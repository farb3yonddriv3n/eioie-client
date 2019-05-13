#ifndef GROUP_H_
#define GROUP_H_

struct group_s {
    struct {
        struct root_s **array;
        size_t          size;
    } roots;
};

struct module_group_s {
    int (*init)(struct group_s **g);
    int (*compare)(struct group_s *local, struct group_s *remote,
                   bool *equal);
    int (*receive)(struct group_s *g, struct transaction_s *t);
    int (*validate)(struct group_s *g, bool *valid);
    int (*clean)(struct group_s *g);
    struct {
        int (*add)(struct group_s *g, struct root_s *r);
    } roots;
    struct {
        int (*save)(struct group_s *g);
        int (*load)(struct group_s *g);
    } db;
};

const struct module_group_s group;

#endif