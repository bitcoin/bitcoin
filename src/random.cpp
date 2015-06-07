// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "random.h"

#include "support/cleanse.h"
#ifdef WIN32
#include "compat.h" // for Windows API
#endif
#include "serialize.h"        // for begin_ptr(vec)
#include "util.h"             // for LogPrint()
#include "utilstrencodings.h" // for GetTime()

#include <limits>

#ifndef WIN32
#include <sys/time.h>
#endif

#include <openssl/err.h>
#include <openssl/rand.h>

static inline int64_t GetPerformanceCounter()
{
    int64_t nCounter = 0;
#ifdef WIN32
    QueryPerformanceCounter((LARGE_INTEGER*)&nCounter);
#else
    timeval t;
    gettimeofday(&t, NULL);
    nCounter = (int64_t)(t.tv_sec * 1000000 + t.tv_usec);
#endif
    return nCounter;
}

void RandAddSeed()
{
    // Seed with CPU performance counter
    int64_t nCounter = GetPerformanceCounter();
    RAND_add(&nCounter, sizeof(nCounter), 1.5);
    memory_cleanse((void*)&nCounter, sizeof(nCounter));
}

void RandAddSeedPerfmon()
{
    RandAddSeed();

#ifdef WIN32
    // Don't need this on Linux, OpenSSL automatically uses /dev/urandom
    // Seed with the entire set of perfmon data

    // This can take up to 2 seconds, so only do it every 10 minutes
    static int64_t nLastPerfmon;
    if (GetTime() < nLastPerfmon + 10 * 60)
        return;
    nLastPerfmon = GetTime();

    std::vector<unsigned char> vData(250000, 0);
    long ret = 0;
    unsigned long nSize = 0;
    const size_t nMaxSize = 10000000; // Bail out at more than 10MB of performance data
    while (true) {
        nSize = vData.size();
        ret = RegQueryValueExA(HKEY_PERFORMANCE_DATA, "Global", NULL, NULL, begin_ptr(vData), &nSize);
        if (ret != ERROR_MORE_DATA || vData.size() >= nMaxSize)
            break;
        vData.resize(std::max((vData.size() * 3) / 2, nMaxSize)); // Grow size of buffer exponentially
    }
    RegCloseKey(HKEY_PERFORMANCE_DATA);
    if (ret == ERROR_SUCCESS) {
        RAND_add(begin_ptr(vData), nSize, nSize / 100.0);
        memory_cleanse(begin_ptr(vData), nSize);
        LogPrint("rand", "%s: %lu bytes\n", __func__, nSize);
    } else {
        static bool warned = false; // Warn only once
        if (!warned) {
            LogPrintf("%s: Warning: RegQueryValueExA(HKEY_PERFORMANCE_DATA) failed with code %i\n", __func__, ret);
            warned = true;
        }
    }
#endif
}

void GetRandBytes(unsigned char* buf, int num)
{
    if (RAND_bytes(buf, num) != 1) {
        LogPrintf("%s: OpenSSL RAND_bytes() failed with error: %s\n", __func__, ERR_error_string(ERR_get_error(), NULL));
        assert(false);
    }
}

uint64_t GetRand(uint64_t nMax)
{
    if (nMax == 0)
        return 0;

    // The range of the random source must be a multiple of the modulus
    // to give every possible output value an equal possibility
    uint64_t nRange = (std::numeric_limits<uint64_t>::max() / nMax) * nMax;
    uint64_t nRand = 0;
    do {
        GetRandBytes((unsigned char*)&nRand, sizeof(nRand));
    } while (nRand >= nRange);
    return (nRand % nMax);
}

int GetRandInt(int nMax)
{
    return GetRand(nMax);
}

uint256 GetRandHash()
{
    uint256 hash;
    GetRandBytes((unsigned char*)&hash, sizeof(hash));
    return hash;
}

uint32_t insecure_rand_Rz = 11;
uint32_t insecure_rand_Rw = 11;
void seed_insecure_rand(bool fDeterministic)
{
    // The seed values have some unlikely fixed points which we avoid.
    if (fDeterministic) {
        insecure_rand_Rz = insecure_rand_Rw = 11;
    } else {
        uint32_t tmp;
        do {
            GetRandBytes((unsigned char*)&tmp, 4);
        } while (tmp == 0 || tmp == 0x9068ffffU);
        insecure_rand_Rz = tmp;
        do {
            GetRandBytes((unsigned char*)&tmp, 4);
        } while (tmp == 0 || tmp == 0x464fffffU);
        insecure_rand_Rw = tmp;
    }
}
