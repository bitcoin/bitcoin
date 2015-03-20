// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_AES_H
#define BITCOIN_CRYPTO_AES_H

#include <stdint.h>

/** An encryption class for AES-128. */
class AES128Encrypt
{
private:
    uint32_t rk[44];

public:
    AES128Encrypt(const unsigned char key[16]);
    ~AES128Encrypt();
    void Encrypt(unsigned char ciphertext[16], const unsigned char plaintext[16]);
};

/** A decryption class for AES-128. */
class AES128Decrypt
{
private:
    uint32_t rk[44];

public:
    AES128Decrypt(const unsigned char key[16]);
    ~AES128Decrypt();
    void Decrypt(unsigned char plaintext[16], const unsigned char ciphertext[16]);
};

/** An encryption class for AES-256. */
class AES256Encrypt
{
private:
    uint32_t rk[60];

public:
    AES256Encrypt(const unsigned char key[32]);
    ~AES256Encrypt();
    void Encrypt(unsigned char ciphertext[16], const unsigned char plaintext[16]);
};

/** A decryption class for AES-256. */
class AES256Decrypt
{
private:
    uint32_t rk[60];

public:
    AES256Decrypt(const unsigned char key[32]);
    ~AES256Decrypt();
    void Decrypt(unsigned char plaintext[16], const unsigned char ciphertext[16]);
};

#endif // BITCOIN_CRYPTO_AES_H
