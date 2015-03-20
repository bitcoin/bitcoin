// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_FORTUNA_H
#define BITCOIN_CRYPTO_FORTUNA_H

#include <string.h>
#include <stdint.h>

#include "crypto/sha256.h"

/** Fortuna's internal generator class. Do not use directly. */
class FortunaGenerator
{
private:
    bool seeded;
    unsigned char K[32];
    unsigned char C[16];

    void IncrementCounter();

public:
    FortunaGenerator();

    void Reseed(const unsigned char *seed, size_t seedlen);
    void Generate(unsigned char *output, size_t len);
};

/** The Fortuna PRNG.
 *
 * See Practical Cryptography by Ferguson and Schneier.
 * https://www.schneier.com/fortuna.pdf
 */
class Fortuna
{
public:
    static const int NUM_POOLS = 32;

private:
    uint64_t reseed_timestamp;
    uint64_t reseed_counter;
    CSHA256d P[NUM_POOLS];
    unsigned char source_pos[256];
    FortunaGenerator G;
    size_t P0_size;

public:
    Fortuna();
    void Seed(const unsigned char *data, size_t size);
    void AddEvent(unsigned char source, const unsigned char *data, size_t size);
    void Generate(uint64_t ts, unsigned char *output, size_t len);
};

#endif // BITCOIN_CRYPTO_FORTUNA_H
