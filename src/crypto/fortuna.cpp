// Copyright (c) 2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "crypto/fortuna.h"

#include "crypto/aes.h"
#include "crypto/sha256.h"

#include <assert.h>

#include <algorithm>

static const int MIN_POOL_SIZE = 64;
static const int MIN_RESEED_INTERVAL = 100000; // 100 ms

FortunaGenerator::FortunaGenerator() : seeded(false)
{
    memset(K, 0, sizeof(K));
    memset(C, 0, sizeof(C));
}

void inline FortunaGenerator::IncrementCounter()
{
    C[0]++;
    for (int i=1; i < 16; i++) {
        C[i] += C[i - 1] == 0;
    }
}

void FortunaGenerator::Reseed(const unsigned char *seed, size_t seedlen)
{
    CSHA256d().Write(K, sizeof(K)).Write(seed, seedlen).Finalize(K);
    IncrementCounter();
    seeded = true;
}

void FortunaGenerator::Generate(unsigned char *output, size_t len)
{
    assert(seeded);
    do {
        AES256Encrypt crypter(K);
        size_t now = std::min<size_t>(len, 1024576);
        len -= now;
        while (now >= 16) {
            crypter.Encrypt(output, C);
            output += 16;
            now -= 16;
            IncrementCounter();
        }
        if (now > 0) {
            unsigned char buf[16];
            crypter.Encrypt(buf, C);
            memcpy(output, buf, now);
            output += now;
            now = 0;
            IncrementCounter();
            memset(buf, 0, sizeof(buf));
        }
        crypter.Encrypt(K, C);
        IncrementCounter();
        crypter.Encrypt(K + 16, C);
        IncrementCounter();
    } while(len > 0);
}

Fortuna::Fortuna() : reseed_timestamp(0), reseed_counter(0), P0_size(0)
{
    memset(source_pos, 0, sizeof(source_pos));
}

void Fortuna::Seed(const unsigned char *data, size_t size)
{
    G.Reseed(data, size);
}

void Fortuna::Generate(uint64_t ts, unsigned char *output, size_t len)
{
    if (P0_size >= MIN_POOL_SIZE && reseed_timestamp + MIN_RESEED_INTERVAL < ts) {
        reseed_counter++;
        P0_size = 0;
        unsigned char s[NUM_POOLS * CSHA256d::OUTPUT_SIZE];
        size_t slen = 0;
        for (int i = 0; i < NUM_POOLS; i++) {
            if ((reseed_counter & ((1 << i) - 1)) == 0) {
                P[i].Finalize(s + slen);
                slen += P[i].OUTPUT_SIZE;
                P[i].Reset();
            }
        }
        G.Reseed(s, slen);
        reseed_timestamp = ts;
    }
    return G.Generate(output, len);
}

void Fortuna::AddEvent(unsigned char source, const unsigned char *data, size_t size)
{
    assert(size >= 1);
    assert(size <= 32);
    // The Fortuna specification warns against having the accumulator do its own
    // round-robin routing of per-source inputs to pools. However, if an attacker
    // is able to create arbitrary function calls inside the binary, we probably
    // have bigger problems anyway.
    int i = source_pos[source];
    source_pos[source] = (source_pos[source] + 1) % NUM_POOLS;
    P[i].Write(&source, 1);
    unsigned char sizechar = size;
    P[i].Write(&sizechar, 1);
    P[1].Write(data, size);
    if (i == 0) {
        P0_size += 2 + size;
    }
}
