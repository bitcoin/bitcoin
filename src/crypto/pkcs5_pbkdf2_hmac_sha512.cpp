// Copyright (c) 2022 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/pkcs5_pbkdf2_hmac_sha512.h>
#include <crypto/hmac_sha512.h>

#include <cstring> // memcpy

PKCS5_PBKDF2_HMAC_SHA512::PKCS5_PBKDF2_HMAC_SHA512(
    const char* pass, size_t passlen,
    const unsigned char* salt, int saltlen,
    int iter,
    size_t keylen, unsigned char* out)
{
    unsigned char digtmp[CHMAC_SHA512::OUTPUT_SIZE], *p, itmp[4];
    int cplen, j, k, tkeylen, mdlen{CHMAC_SHA512::OUTPUT_SIZE};
    unsigned long i = 1;

    const unsigned char *upass = reinterpret_cast<const unsigned char *>(pass);
    p = out;
    tkeylen = keylen;

    while (tkeylen) {
        cplen = (tkeylen > mdlen) ? mdlen : tkeylen;
        /* We are unlikely to ever use more than 256 blocks (5120 bits!)
        * but just in case...
        */
        itmp[0] = (unsigned char)((i >> 24) & 0xff);
        itmp[1] = (unsigned char)((i >> 16) & 0xff);
        itmp[2] = (unsigned char)((i >> 8) & 0xff);
        itmp[3] = (unsigned char)(i & 0xff);
        CHMAC_SHA512(upass, passlen).Write(salt, saltlen).Write(itmp, 4).Finalize(digtmp);
        memcpy(p, digtmp, cplen);
        for (j = 1; j < iter; ++j) {
            CHMAC_SHA512(upass, passlen).Write(digtmp, mdlen).Finalize(digtmp);
            for (k = 0; k < cplen; ++k) {
              p[k] ^= digtmp[k];
            }
        }
        tkeylen-= cplen;
        ++i;
        p+= cplen;
    }
}
