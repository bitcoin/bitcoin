// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RANDOM_H
#define BITCOIN_RANDOM_H

#include <crypto/chacha20.h>
#include <crypto/common.h>
#include <uint256.h>

#include <chrono> // For std::chrono::microseconds
#include <cstdint>
#include <limits>

/* Seed OpenSSL PRNG with additional entropy data */
void RandAddSeed();

/**
 * Functions to gather random data via the OpenSSL PRNG
 */
void GetRandBytes(unsigned char* buf, int num);
uint64_t GetRand(uint64_t nMax);
std::chrono::microseconds GetRandMicros(std::chrono::microseconds duration_max) noexcept;
int GetRandInt(int nMax);
uint256 GetRandHash();

bool GetRandBool(double rate);

/**
 * Add a little bit of randomness to the output of GetStrongRangBytes.
 * This sleeps for a millisecond, so should only be called when there is
 * no other work to be done.
 */
void RandAddSeedSleep();

/**
 * Function to gather random data from multiple sources, failing whenever any
 * of those sources fail to provide a result.
 */
void GetStrongRandBytes(unsigned char* buf, int num);

/**
 * Fast randomness source. This is seeded once with secure random data, but
 * is completely deterministic and insecure after that.
 * This class is not thread-safe.
 */
class FastRandomContext {
private:
    bool requires_seed;
    ChaCha20 rng;

    unsigned char bytebuf[64];
    int bytebuf_size;

    uint64_t bitbuf;
    int bitbuf_size;

    void RandomSeed();

    void FillByteBuffer()
    {
        if (requires_seed) {
            RandomSeed();
        }
        rng.Keystream(bytebuf, sizeof(bytebuf));
        bytebuf_size = sizeof(bytebuf);
    }

    void FillBitBuffer()
    {
        bitbuf = rand64();
        bitbuf_size = 64;
    }

public:
    explicit FastRandomContext(bool fDeterministic = false);

    /** Initialize with explicit seed (only for testing) */
    explicit FastRandomContext(const uint256& seed);

    /** Generate a random 64-bit integer. */
    uint64_t rand64()
    {
        if (bytebuf_size < 8) FillByteBuffer();
        uint64_t ret = ReadLE64(bytebuf + 64 - bytebuf_size);
        bytebuf_size -= 8;
        return ret;
    }

    /** Generate a random (bits)-bit integer. */
    uint64_t randbits(int bits) {
        if (bits == 0) {
            return 0;
        } else if (bits > 32) {
            return rand64() >> (64 - bits);
        } else {
            if (bitbuf_size < bits) FillBitBuffer();
            uint64_t ret = bitbuf & (~(uint64_t)0 >> (64 - bits));
            bitbuf >>= bits;
            bitbuf_size -= bits;
            return ret;
        }
    }

    /** Generate a random integer in the range [0..range). */
    uint64_t randrange(uint64_t range)
    {
        --range;
        int bits = CountBits(range);
        while (true) {
            uint64_t ret = randbits(bits);
            if (ret <= range) return ret;
        }
    }

    uint32_t rand32(uint32_t nMax) {
        return rand32() % nMax;
    }

    uint32_t operator()(uint32_t nMax) {
        return rand32(nMax);
    }

    /** Generate random bytes. */
    std::vector<unsigned char> randbytes(size_t len);

    /** Generate a random 32-bit integer. */
    uint32_t rand32() { return randbits(32); }

    /** generate a random uint256. */
    uint256 rand256();

    /** Generate a random boolean. */
    bool randbool() { return randbits(1); }

    // Compatibility with the C++11 UniformRandomBitGenerator concept
    typedef uint64_t result_type;
    static constexpr uint64_t min() { return 0; }
    static constexpr uint64_t max() { return std::numeric_limits<uint64_t>::max(); }
    inline uint64_t operator()() { return rand64(); }
};

/** More efficient than using std::shuffle on a FastRandomContext.
 *
 * This is more efficient as std::shuffle will consume entropy in groups of
 * 64 bits at the time and throw away most.
 *
 * This also works around a bug in libstdc++ std::shuffle that may cause
 * type::operator=(type&&) to be invoked on itself, which the library's
 * debug mode detects and panics on. This is a known issue, see
 * https://stackoverflow.com/questions/22915325/avoiding-self-assignment-in-stdshuffle
 */
template<typename I, typename R>
void Shuffle(I first, I last, R&& rng)
{
    while (first != last) {
        size_t j = rng.randrange(last - first);
        if (j) {
            using std::swap;
            swap(*first, *(first + j));
        }
        ++first;
    }
}

/* Number of random bytes returned by GetOSRand.
 * When changing this constant make sure to change all call sites, and make
 * sure that the underlying OS APIs for all platforms support the number.
 * (many cap out at 256 bytes).
 */
static const int NUM_OS_RANDOM_BYTES = 32;

/** Get 32 bytes of system entropy. Do not use this in application code: use
 * GetStrongRandBytes instead.
 */
void GetOSRand(unsigned char *ent32);

/** Check that OS randomness is available and returning the requested number
 * of bytes.
 */
bool Random_SanityCheck();

/** Initialize the RNG. */
void RandomInit();

#endif // BITCOIN_RANDOM_H
