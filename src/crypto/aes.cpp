// Copyright (c) 2016-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/aes.h>

#include <string.h>

extern "C" {
#include <crypto/ctaes/ctaes.c>
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


template <typename T>
static int CBCEncrypt(const T& enc, const unsigned char iv[AES_BLOCKSIZE], const unsigned char* data, int size, bool pad, unsigned char* out)
{
    int written = 0;
    int padsize = size % AES_BLOCKSIZE;
    unsigned char mixed[AES_BLOCKSIZE];

    if (!data || !size || !out)
        return 0;

    if (!pad && padsize != 0)
        return 0;

    memcpy(mixed, iv, AES_BLOCKSIZE);

    // Write all but the last block
    while (written + AES_BLOCKSIZE <= size) {
        for (int i = 0; i != AES_BLOCKSIZE; i++)
            mixed[i] ^= *data++;
        enc.Encrypt(out + written, mixed);
        memcpy(mixed, out + written, AES_BLOCKSIZE);
        written += AES_BLOCKSIZE;
    }
    if (pad) {
        // For all that remains, pad each byte with the value of the remaining
        // space. If there is none, pad by a full block.
        for (int i = 0; i != padsize; i++)
            mixed[i] ^= *data++;
        for (int i = padsize; i != AES_BLOCKSIZE; i++)
            mixed[i] ^= AES_BLOCKSIZE - padsize;
        enc.Encrypt(out + written, mixed);
        written += AES_BLOCKSIZE;
    }
    return written;
}

template <typename T>
static int CBCDecrypt(const T& dec, const unsigned char iv[AES_BLOCKSIZE], const unsigned char* data, int size, bool pad, unsigned char* out)
{
    int written = 0;
    bool fail = false;
    const unsigned char* prev = iv;

    if (!data || !size || !out)
        return 0;

    if (size % AES_BLOCKSIZE != 0)
        return 0;

    // Decrypt all data. Padding will be checked in the output.
    while (written != size) {
        dec.Decrypt(out, data + written);
        for (int i = 0; i != AES_BLOCKSIZE; i++)
            *out++ ^= prev[i];
        prev = data + written;
        written += AES_BLOCKSIZE;
    }

    // When decrypting padding, attempt to run in constant-time
    if (pad) {
        // If used, padding size is the value of the last decrypted byte. For
        // it to be valid, It must be between 1 and AES_BLOCKSIZE.
        unsigned char padsize = *--out;
        fail = !padsize | (padsize > AES_BLOCKSIZE);

        // If not well-formed, treat it as though there's no padding.
        padsize *= !fail;

        // All padding must equal the last byte otherwise it's not well-formed
        for (int i = AES_BLOCKSIZE; i != 0; i--)
            fail |= ((i > AES_BLOCKSIZE - padsize) & (*out-- != padsize));

        written -= padsize;
    }
    return written * !fail;
}

AES256CBCEncrypt::AES256CBCEncrypt(const unsigned char key[AES256_KEYSIZE], const unsigned char ivIn[AES_BLOCKSIZE], bool padIn)
    : enc(key), pad(padIn)
{
    memcpy(iv, ivIn, AES_BLOCKSIZE);
}

int AES256CBCEncrypt::Encrypt(const unsigned char* data, int size, unsigned char* out) const
{
    return CBCEncrypt(enc, iv, data, size, pad, out);
}

AES256CBCEncrypt::~AES256CBCEncrypt()
{
    memset(iv, 0, sizeof(iv));
}

AES256CBCDecrypt::AES256CBCDecrypt(const unsigned char key[AES256_KEYSIZE], const unsigned char ivIn[AES_BLOCKSIZE], bool padIn)
    : dec(key), pad(padIn)
{
    memcpy(iv, ivIn, AES_BLOCKSIZE);
}


int AES256CBCDecrypt::Decrypt(const unsigned char* data, int size, unsigned char* out) const
{
    return CBCDecrypt(dec, iv, data, size, pad, out);
}

AES256CBCDecrypt::~AES256CBCDecrypt()
{
    memset(iv, 0, sizeof(iv));
}
