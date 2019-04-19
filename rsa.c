#include <common.h>

#define KEY_LENGTH  2048
#define PUB_EXP     3
#define PRINT_KEYS
#define WRITE_TO_FILE

#define KEY_PRIV ".priv.key"
#define KEY_PUB  ".pub.key"

int rsa_decrypt(RSA *privkey, const unsigned char *src, const int srclen,
                unsigned char **dst, int *dstlen)
{
    char err[128];
    *dst = malloc(srclen);
    *dstlen = RSA_private_decrypt(srclen, src, *dst,
                                  privkey, RSA_PKCS1_OAEP_PADDING);
    if (*dstlen == -1) {
        ERR_load_crypto_strings();
        ERR_error_string(ERR_get_error(), err);
        fprintf(stderr, "Error decrypting message: %s\n", err);
        return -1;
    }

    return 0;
}

int rsa_encrypt(RSA *pubkey, const unsigned char *src, const int srclen,
                unsigned char **dst, int *dstlen)
{
    char err[128];
    *dst = malloc(RSA_size(pubkey));
    if ((*dstlen = RSA_public_encrypt(srclen, src, *dst,
                                      pubkey, RSA_PKCS1_OAEP_PADDING)) == -1) {
        ERR_load_crypto_strings();
        ERR_error_string(ERR_get_error(), err);
        fprintf(stderr, "Error encrypting message: %s\n", err);
        return -1;
    }

    return 0;
}

static int read_key(RSA *func(FILE *pfile, RSA **x, pem_password_cb *cb, void *u),
                    RSA **rsakey, sn fkey, sn *key)
{
    FILE *pfile;
    *rsakey = NULL;
    sn_to_char(bkey, fkey, 1024);
    pfile = fopen(bkey, "r");
    if (pfile == NULL) return -1;
    key->n = eioie_fread(&key->s, fkey);
    if (key->n == -1) return -1;
    *rsakey = func(pfile, rsakey, NULL, NULL);
    fclose(pfile);
    return 0;
}

int rsa_load(struct config_s *cfg)
{
    sn_initz(fpub, KEY_PUB);
    sn_initz(fpriv, KEY_PRIV);

    int ret = read_key(PEM_read_RSAPublicKey, &cfg->rsakey.public,
                       fpub, &cfg->key.public);
    if (ret != 0) return ret;

    ret = read_key(PEM_read_RSAPrivateKey, &cfg->rsakey.private,
                   fpriv, &cfg->key.private);
    if (ret != 0) return ret;

    return 0;
}

int rsa_generate()
{
    int  pri_len;
    int  pub_len;
    char *pri_key;
    char *pub_key;

    RSA *keypair = RSA_generate_key(KEY_LENGTH, PUB_EXP, NULL, NULL);

    BIO *pri = BIO_new(BIO_s_mem());
    BIO *pub = BIO_new(BIO_s_mem());

    PEM_write_bio_RSAPrivateKey(pri, keypair, NULL, NULL, 0, NULL, NULL);
    PEM_write_bio_RSAPublicKey(pub, keypair);

    pri_len = BIO_pending(pri);
    pub_len = BIO_pending(pub);

    pri_key = malloc(pri_len + 1);
    pub_key = malloc(pub_len + 1);

    BIO_read(pri, pri_key, pri_len);
    BIO_read(pub, pub_key, pub_len);

    pri_key[pri_len] = '\0';
    pub_key[pub_len] = '\0';

    int ret = eioie_fwrite(KEY_PRIV, "w", pri_key, pri_len);
    if (ret != 0) return -1;

    ret = eioie_fwrite(KEY_PUB, "w", pub_key, pub_len);
    if (ret != 0) return -1;

    RSA_free(keypair);
    BIO_free_all(pub);
    BIO_free_all(pri);
    free(pri_key);
    free(pub_key);

    return 0;
}