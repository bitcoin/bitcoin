// Copyright (c) 2013 NovaCoin Developers

#ifndef PBKDF2_H
#define PBKDF2_H

#include <openssl/sha.h>
#include <stdint.h>

typedef struct HMAC_SHA256Context {
    SHA256_CTX ictx;
    SHA256_CTX octx;
} HMAC_SHA256_CTX;

void
HMAC_SHA256_Init(HMAC_SHA256_CTX * ctx, const void * _K, size_t Klen);

void
HMAC_SHA256_Update(HMAC_SHA256_CTX * ctx, const void *in, size_t len);

void
HMAC_SHA256_Final(unsigned char digest[32], HMAC_SHA256_CTX * ctx);

void
PBKDF2_SHA256(const uint8_t * passwd, size_t passwdlen, const uint8_t * salt,
    size_t saltlen, uint64_t c, uint8_t * buf, size_t dkLen);

#endif // PBKDF2_H
