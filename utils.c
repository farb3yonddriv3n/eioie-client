#include <common.h>

int eioie_fwrite(const char *fname, const char *mode, char *content, int ncontent)
{
    FILE *pfile;
    int result;

    pfile = fopen(fname, mode);
    if (pfile == NULL) {
        return -1;
    }

    result = fwrite(content, sizeof(char), ncontent, pfile);

    fclose(pfile);
    return !(result == ncontent);
}

int eioie_fread(char **dst, sn fname)
{
    FILE *pfile;
    int   lsize;
    char *buffer;
    int   result;

    char fnbuffer[1024];
    snprintf(fnbuffer, sizeof(fnbuffer), "%.*s", sn_p(fname));
    pfile = fopen(fnbuffer, "rb");
    if (pfile == NULL) {
        return -1;
    }

    fseek(pfile , 0 , SEEK_END);
    lsize = ftell(pfile);
    rewind(pfile);

    if (lsize > MAX_FILE_SIZE) {
        fclose(pfile);
        return -1;
    }

    buffer = malloc(sizeof(char) * lsize);
    if (buffer == NULL) {
        fclose(pfile);
        return -1;
    }

    result = fread(buffer, sizeof(char), lsize, pfile);
    if (result != lsize) {
        fclose(pfile);
        free(buffer);
        return -1;
    }

    *dst = buffer;

    fclose(pfile);
    return result;
}

void bin2hexstr(char *dst, size_t dstlen,
                char *src, size_t srclen)
{
    char tmp[8];
    char *step = src;
    int i, n;
    for (i = 0; i < srclen; i++) {
        snprintf(tmp, sizeof(tmp), "%02x", (unsigned char)(*step));
        n = strlen(tmp);
        if ((step - src + n) > dstlen) break;
        memcpy(dst, tmp, n);
        dst += n;
        step++;
    }
}