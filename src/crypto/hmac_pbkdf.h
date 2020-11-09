#ifndef BITCOIN_CRYPTO_HMAC_PBKDF_H
#define BITCOIN_CRYPTO_HMAC_PBKDF_H

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void pkcs5_pbkdf2_hmac_sha512(unsigned char *password,
                unsigned int plen,
                unsigned char *salt,
                unsigned int slen,
                unsigned int iteration_count,
                unsigned int key_length,
                unsigned char *output);

#ifdef __cplusplus
}
#endif

#endif // BITCOIN_CRYPTO_HMAC_PBKDF_H
