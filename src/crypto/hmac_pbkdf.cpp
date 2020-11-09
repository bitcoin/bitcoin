#include <sys/types.h>
#include <crypto/hmac_sha512.h>
#include <crypto/sha512.h>
#include <crypto/common.h>

#ifdef __cplusplus
extern "C" {
#endif

void pkcs5_pbkdf2_hmac_sha512(unsigned char *password,
                unsigned int plen,
                unsigned char *salt,
                unsigned int slen,
                unsigned int iteration_count,
                unsigned int key_length,
                unsigned char *output)
{
    uint8_t obuf[64];
    uint8_t d1[64];
    uint8_t asalt[4];
    unsigned int i, j;
    unsigned int count;
    size_t r;

    if (key_length < 1 || output == 0)
        return;

    for (count = 1; key_length > 0; count++) {
        asalt[0] = (count >> 24) & 0xff;
        asalt[1] = (count >> 16) & 0xff;
        asalt[2] = (count >> 8) & 0xff;
        asalt[3] = count & 0xff;

        CHMAC_SHA512(password, plen).Write(salt, slen).Write(asalt, sizeof(asalt)).Finalize(d1);
        memcpy(obuf, d1, sizeof(obuf));

        for (i = 1; i < iteration_count; i++) {
            CHMAC_SHA512(password, plen).Write(d1, sizeof(d1)).Finalize(d1);

            for (j = 0; j < sizeof(obuf); j++)
                obuf[j] ^= d1[j];
        }

        r = key_length < 64 ? key_length : 64;
        memcpy(output, obuf, r);
        output += r;
        key_length -= r;
    };
}

#ifdef __cplusplus
}
#endif
