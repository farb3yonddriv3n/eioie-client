#include <common.h>
#include <dirent.h>
#include <time.h>

static const char *dirs[] = { "parts", "blocks" };

static int load_json(json_object **obj, char *content, int ncontent)
{
    if (!obj || !content || ncontent < 1) return -1;
    struct json_tokener *tok = json_tokener_new();
    *obj = json_tokener_parse_ex(tok, content, ncontent);
    json_tokener_free(tok);
    if (!*obj) return -1;
    return 0;
}

static int load_json_file(json_object **obj, const char *filename)
{
    if (!obj || !filename) return -1;
    sn_initz(fn, (char *)filename);
    char *content;
    int n = eioie_fread(&content, fn);
    if (n <= 1) return -1;
    if (load_json(obj, content, n) != 0) {
        if (content) free(content);
        return -1;
    }
    if (content) free(content);
    return 0;
}

static int fileexists(const char *filename, bool *exists)
{
    FILE *pfile;
    pfile = fopen(filename, "r");
    *exists = false;
    if (pfile) {
        *exists = true;
        fclose(pfile);
    }
    return 0;
}

static int filepart(const char *filename, size_t offset, size_t maxread,
                    char **dst, uint64_t *ndst)
{
    if (!filename) return -1;
    FILE *pfile;
    pfile = fopen(filename, "rb");
    if (!pfile) return -1;
    fseek(pfile, 0, SEEK_END);
    size_t size = ftell(pfile);
    if (offset >= size) return -1;
    fseek(pfile, offset, SEEK_SET);
    *ndst = (maxread > size - offset) ?
            size - offset :
            maxread;
    *dst = malloc(*ndst);
    if (*dst == NULL) {
        fclose(pfile);
        return -1;
    }
    size_t result = fread(*dst, sizeof(char), *ndst, pfile);
    if (result != *ndst) {
        fclose(pfile);
        free(*dst);
        return -1;
    }
    fclose(pfile);
    return 0;
}

static int fileparts(const char *filename, size_t maxread,
                     unsigned int *parts)
{
    size_t size;
    if (os.filesize(filename, &size) != 0) return -1;
    *parts = size / maxread;
    if (size % maxread != 0) (*parts)++;
    return 0;
}

static int filesize(const char *filename, size_t *size)
{
    if (!filename || !size) return -1;
    FILE *pfile;
    pfile = fopen(filename, "rb");
    if (!pfile) return -1;
    fseek(pfile, 0, SEEK_END);
    *size = ftell(pfile);
    fclose(pfile);
    return 0;
}

static int filewrite(const char *fname, const char *mode,
                     char *content, int ncontent)
{
    FILE *pfile;
    int result;
    pfile = fopen(fname, mode);
    if (pfile == NULL) return -1;
    result = fwrite(content, sizeof(char), ncontent, pfile);
    fclose(pfile);
    return (result == ncontent) ? 0 : -1;
}

static int gettimems(double *result)
{
    if (!result) return -1;
    long   ms;
    time_t s;
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    s  = spec.tv_sec;
    ms = round(spec.tv_nsec / 1.0e6);
    *result = s + ((double)ms / 1000);
    return 0;
}

static int gettime(char *buffer, size_t nbuffer)
{
    if (!buffer) return -1;
    double result;
    if (gettimems(&result) != 0) return -1;
    snprintf(buffer, nbuffer, "%lf", result);
    return 0;
}

static int gettimeiter(char *buffer, size_t nbuffer, int i)
{
    if (!buffer) return -1;
    double result;
    if (gettimems(&result) != 0) return -1;
    snprintf(buffer, nbuffer, "%ld%d", (unsigned long int)result, i);
    return 0;
}

static int gettimestr(char *buffer, size_t nbuffer)
{
    time_t     n = time(NULL);
    struct tm *t = localtime(&n);
    strftime(buffer, nbuffer - 1, "%d/%m/%Y %H:%M:%S", t);
    return 0;
}

static int finalize(struct config_s *cfg, struct list_s *l,
                    char *fullpath, int nfullpath,
                    char *filename, int nfilename)
{
    void **files = NULL;
    int nfiles, i;
    ifr(list.toarray_sort(l, &files, &nfiles, LIST_ARRAY_SORT_STR));
    char timems[128];
    ifr(gettime(timems, sizeof(timems)));
    char dstnamems[512];
    snprintf(dstnamems, sizeof(dstnamems), "%s/%s", cfg->dir.download, timems);
    for (i = 0; i < nfiles; i++) {
        char *fname = ((char **)files)[i];
        char *buffer;
        char fbuffer[1024];
        snprintf(fbuffer, sizeof(fbuffer), "%s/%s/%s",
                                           cfg->dir.download,
                                           dirs[0], fname);
        sn_initz(fn, fbuffer);
        int n = eioie_fread(&buffer, fn);
        if (n <= 0) return -1;
        if (eioie_fwrite(dstnamems, "a", buffer, n) != 0) return -1;
        free(buffer);
        if (remove(fbuffer) != 0) return -1;
    }
    unsigned char dstnamehash[SHA256HEX];
    sn_initz(sdstnamems, dstnamems);
    char *buffer;
    int n = eioie_fread(&buffer, sdstnamems);
    if (n <= 0) return -1;
    sha256hex((unsigned char *)buffer, n, dstnamehash);
    free(buffer);
    snprintf(fullpath, nfullpath, "%s/%.*s", cfg->dir.download, SHA256HEX, dstnamehash);
    snprintf(filename, nfilename, "%.*s", SHA256HEX, dstnamehash);
    ifr(os.filemove(dstnamems, fullpath));
    if (files) free(files);
    return 0;
}

static int filejoin_clean(void *fname)
{
    if (!fname) return -1;
    free(fname);
    return 0;
}

static int blocksremote(struct config_s *cfg, void *dfs,
                        int (*cb)(void *dfs, const char *filename,
                                  const char *fullpath))
{
    if (!cfg || !cb) return -1;
    char blocksdir[256];
    snprintf(blocksdir, sizeof(blocksdir), "%s/%s", cfg->dir.download, dirs[1]);
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(blocksdir)) == NULL) return -1;
    while ((ent = readdir(dir)) != NULL) {
        if (strlen(ent->d_name) != SHA256HEX) continue;
        char fullpath[512];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", blocksdir, ent->d_name);
        if (cb(dfs, ent->d_name, fullpath) != 0) return -1;
    }
    closedir(dir);
    return 0;
}

static int blockfile(struct config_s *cfg, unsigned char *bfile,
                     int nbfile, bool *found, char *blockpath,
                     int nblockpath)
{
    if (!cfg || !bfile || !found || !blockpath) return -1;
    DIR *dir;
    struct dirent *ent;
    *found = false;
    if ((dir = opendir(cfg->dir.block)) == NULL) return -1;
    while ((ent = readdir(dir)) != NULL) {
        if (strlen(ent->d_name) == nbfile) {
            memcpy(bfile, ent->d_name, nbfile);
            snprintf(blockpath, nblockpath, "%s/%.*s", cfg->dir.block,
                                                       nbfile, bfile);
            *found = true;
            break;
        }
    }
    closedir(dir);
    return 0;
}

static int readkeys(struct config_s *cfg,
                    int (*cb)(struct config_s *cfg, const char *fullpath,
                              const char *filename))
{
    if (!cfg || !cb) return -1;
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(cfg->dir.keys)) == NULL) return -1;
    while ((ent = readdir(dir)) != NULL) {
        char fname[128];
        int found = sscanf(ent->d_name, "%s.priv", fname);
        if (found != 1 || strlen(ent->d_name) < SHA256HEX) continue;
        char fullpath[512];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", cfg->dir.keys, ent->d_name);
        ifr(cb(cfg, fullpath, fname));
    }
    closedir(dir);
    return 0;
}

static int filereadable(const char *fname, bool *hr)
{
    if (!fname) return -1;
    sn_initz(fn, (char *)fname);
    char *content;
    int n = eioie_fread(&content, fn);
    if (n <= 1) return -1;
    *hr = true;
    int i;
    for (i = 0; i < n; i++) {
        if (content[i] < 32 || content[i] > 126) {
            *hr = false;
            break;
        }
    }
    free(content);
    return 0;
}

static int filejoin(struct config_s *cfg, const char *fname,
                    char *fullpath, int nfullpath,
                    char *filename, int nfilename,
                    bool *finalized)
{
    struct list_s files;
    ifr(list.init(&files));
    int host, port, tidx, gidx, pidx, parts;
    int found = sscanf(fname, "%x_%d_%d_%d_%d_%d.part",
                               &host, &port, &tidx,
                               &gidx, &pidx, &parts);
    if (found != 6) return -1;
    DIR *dir;
    struct dirent *ent;
    char partsdir[256];
    snprintf(partsdir, sizeof(partsdir), "%s/%s/", cfg->dir.download, dirs[0]);
    if ((dir = opendir(partsdir)) == NULL) return -1;
    while ((ent = readdir(dir)) != NULL) {
        int shost, sport, stidx;
        found = sscanf(ent->d_name, "%x_%d_%d", &shost, &sport, &stidx);
        if (found != 3) continue;
        if (host != shost || port != sport || tidx != stidx) continue;
        int len = strlen(ent->d_name) + 1;
        char *name = malloc(len);
        if (!name) return -1;
        memcpy(name, ent->d_name, len - 1);
        name[len - 1] = 0;
        ifr(list.add(&files, (void *)name, filejoin_clean));
    }
    closedir(dir);
    *finalized = false;
    int size;
    ifr(list.size(&files, &size));
    if (size == parts) {
        ifr(finalize(cfg, &files, fullpath, nfullpath,
                     filename, nfilename));
        *finalized = true;
    }
    ifr(list.clean(&files));
    return 0;
}

static int makedirs(struct config_s *cfg)
{
    if (!cfg) return -1;
    struct stat st = {0};
    if (stat(cfg->dir.block, &st) == -1) {
        if (mkdir(cfg->dir.block, 0700) != 0) return -1;
    }
    memset(&st, 0, sizeof(st));
    if (stat(cfg->dir.download, &st) == -1) {
        if (mkdir(cfg->dir.download, 0700) != 0) return -1;
    }
    memset(&st, 0, sizeof(st));
    if (stat(cfg->dir.keys, &st) == -1) {
        if (mkdir(cfg->dir.keys, 0700) != 0) return -1;
    }
    memset(&st, 0, sizeof(st));
    if (stat(cfg->dir.finalized, &st) == -1) {
        if (mkdir(cfg->dir.finalized, 0700) != 0) return -1;
    }
    memset(&st, 0, sizeof(st));
    if (stat(cfg->dir.tcp, &st) == -1) {
        if (mkdir(cfg->dir.tcp, 0700) != 0) return -1;
    }
    char partsdir[256];
    int i;
    for (i = 0; i < COUNTOF(dirs); i++) {
        snprintf(partsdir, sizeof(partsdir), "%s/%s/", cfg->dir.download, dirs[i]);
        if (stat(partsdir, &st) == -1) {
            if (mkdir(partsdir, 0700) != 0) return -1;
        }
    }
    return 0;
}

static const char *getpartsdir()
{
    return dirs[0];
}

static int filemove(const char *src, const char *dst)
{
    if (!dst || !src) return -1;
    if (rename(src, dst) != 0) return -1;
    return 0;
}

static int blockname(struct config_s *cfg, char *blockname, int nblockname,
                     const char *received, unsigned char *keyhash)
{
    if (!blockname || !received || !keyhash) return -1;
    snprintf(blockname, nblockname, "%s/%s/%.*s",
                                    cfg->dir.download,
                                    dirs[1],
                                    SHA256HEX, keyhash);
    return 0;
}

static int partexists(struct config_s *cfg, const char *startswith,
                      bool *exists)
{
    if (!startswith || !exists) return -1;
    *exists = false;
    DIR *dir;
    struct dirent *ent;
    char partsdir[256];
    snprintf(partsdir, sizeof(partsdir), "%s/%s/", cfg->dir.download, dirs[0]);
    if ((dir = opendir(partsdir)) == NULL) return -1;
    while ((ent = readdir(dir)) != NULL) {
        if (strlen(ent->d_name) < strlen(startswith)) continue;
        if (dmemcmp(startswith, strlen(startswith),
                    ent->d_name, strlen(startswith))) {
            *exists = true;
            break;
        }
    }
    closedir(dir);
    return 0;
}

static int pipe_open(struct peer_s *p, const char *fullpath, unsigned int flags,
                     int *fd, ev_io *ev, unsigned int evflags,
                     void (*cb)(EV_P_ ev_io *w, int revents))
{
    if (!p || !fullpath || !fd || !ev || !cb) return -1;
    if (*fd > STDERR_FILENO) close(*fd);
    *fd = open(fullpath, flags);
    if (*fd == -1) return 0;
    ev_io_init(ev, cb, *fd, evflags);
    ev->data = p;
    if (evflags == EV_READ)
        ev_io_start(p->ev.loop, ev);
    return 0;
}

static int pipes(struct peer_s *p)
{
    if (!p) return -1;
    struct stat st;
    memset(&st, 0, sizeof(st));
    if (stat(p->cfg.api.pipes.write, &st) != 0)
        mkfifo(p->cfg.api.pipes.write, 0666);
    memset(&st, 0, sizeof(st));
    if (stat(p->cfg.api.pipes.read, &st) != 0)
        mkfifo(p->cfg.api.pipes.read, 0666);
    ifr(pipe_open(p, p->cfg.api.pipes.read, O_RDONLY|O_NONBLOCK,
                  &p->api.pipes.read, &p->api.ev.read, EV_READ,
                  api.ev.read));
    return 0;
}

const struct module_os_s os = {
    .filepart      = filepart,
    .fileparts     = fileparts,
    .filesize      = filesize,
    .filewrite     = filewrite,
    .filejoin      = filejoin,
    .fileexists    = fileexists,
    .filemove      = filemove,
    .filereadable  = filereadable,
    .partexists    = partexists,
    .blockname     = blockname,
    .blockfile     = blockfile,
    .blocksremote  = blocksremote,
    .makedirs      = makedirs,
    .loadjsonfile  = load_json_file,
    .loadjson      = load_json,
    .gettimems     = gettimems,
    .gettimestr    = gettimestr,
    .gettimeiter   = gettimeiter,
    .getpartsdir   = getpartsdir,
    .readkeys      = readkeys,
    .pipes         = pipes,
    .pipe_open     = pipe_open,
};
