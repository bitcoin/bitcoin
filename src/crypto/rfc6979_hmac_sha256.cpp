// Copyright (c) 2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "crypto/rfc6979_hmac_sha256.h"

#include <string.h>

#include <algorithm>

static const unsigned char zero[1] = {0x00};
static const unsigned char one[1] = {0x01};

RFC6979_HMAC_SHA256::RFC6979_HMAC_SHA256(const unsigned char* key, size_t keylen, const unsigned char* msg, size_t msglen) : retry(false)
{
    memset(V, 0x01, sizeof(V));
    memset(K, 0x00, sizeof(K));

    CHMAC_SHA256(K, sizeof(K)).Write(V, sizeof(V)).Write(zero, sizeof(zero)).Write(key, keylen).Write(msg, msglen).Finalize(K);
    CHMAC_SHA256(K, sizeof(K)).Write(V, sizeof(V)).Finalize(V);
    CHMAC_SHA256(K, sizeof(K)).Write(V, sizeof(V)).Write(one, sizeof(one)).Write(key, keylen).Write(msg, msglen).Finalize(K);
    CHMAC_SHA256(K, sizeof(K)).Write(V, sizeof(V)).Finalize(V);
}

RFC6979_HMAC_SHA256::~RFC6979_HMAC_SHA256()
{
    memset(V, 0x01, sizeof(V));
    memset(K, 0x00, sizeof(K));
}

void RFC6979_HMAC_SHA256::Generate(unsigned char* output, size_t outputlen)
{
    if (retry) {
        CHMAC_SHA256(K, sizeof(K)).Write(V, sizeof(V)).Write(zero, sizeof(zero)).Finalize(K);
        CHMAC_SHA256(K, sizeof(K)).Write(V, sizeof(V)).Finalize(V);
    }

    while (outputlen > 0) {
        CHMAC_SHA256(K, sizeof(K)).Write(V, sizeof(V)).Finalize(V);
        size_t len = std::min(outputlen, sizeof(V));
        memcpy(output, V, len);
        output += len;
        outputlen -= len;
    }

    retry = true;
}
