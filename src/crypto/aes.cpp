// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "aes.h"
#include "crypto/common.h"

#include <assert.h>
#include <string.h>

extern "C" {
#include "crypto/ctaes/ctaes.c"
}

AES128Encrypt::AES128Encrypt(const unsigned char key[16])
{
    AES128_init(&ctx, key);
}

AES128Encrypt::~AES128Encrypt()
{
    memset(&ctx, 0, sizeof(ctx));
}

void AES128Encrypt::Encrypt(unsigned char ciphertext[16], const unsigned char plaintext[16]) const
{
    AES128_encrypt(&ctx, 1, ciphertext, plaintext);
}

AES128Decrypt::AES128Decrypt(const unsigned char key[16])
{
    AES128_init(&ctx, key);
}

AES128Decrypt::~AES128Decrypt()
{
    memset(&ctx, 0, sizeof(ctx));
}

void AES128Decrypt::Decrypt(unsigned char plaintext[16], const unsigned char ciphertext[16]) const
{
    AES128_decrypt(&ctx, 1, plaintext, ciphertext);
}

AES256Encrypt::AES256Encrypt(const unsigned char key[32])
{
    AES256_init(&ctx, key);
}

AES256Encrypt::~AES256Encrypt()
{
    memset(&ctx, 0, sizeof(ctx));
}

void AES256Encrypt::Encrypt(unsigned char ciphertext[16], const unsigned char plaintext[16]) const
{
    AES256_encrypt(&ctx, 1, ciphertext, plaintext);
}

AES256Decrypt::AES256Decrypt(const unsigned char key[32])
{
    AES256_init(&ctx, key);
}

AES256Decrypt::~AES256Decrypt()
{
    memset(&ctx, 0, sizeof(ctx));
}

void AES256Decrypt::Decrypt(unsigned char plaintext[16], const unsigned char ciphertext[16]) const
{
    AES256_decrypt(&ctx, 1, plaintext, ciphertext);
}
