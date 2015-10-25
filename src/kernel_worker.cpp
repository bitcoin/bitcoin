#include <vector>
#include <inttypes.h>

#include "uint256.h"
#include "bignum.h"
#include "kernel.h"
#include "kernel_worker.h"

using namespace std;

#ifdef USE_ASM

#ifdef _MSC_VER
#include <stdlib.h>
#define __builtin_bswap32 _byteswap_ulong
#endif

#if defined(__i386__) || defined(__x86_64__)
#include <immintrin.h>
#endif

#ifndef __i386__
// kernel padding
static const uint32_t block1_suffix[9] = { 0x80000000, 0, 0, 0, 0, 0, 0, 0, 0x000000e0 };
// hash padding
static const uint32_t block2_suffix[8] = { 0x80000000, 0, 0, 0, 0, 0, 0, 0x00000100 };

// Sha256 initial state
static const uint32_t sha256_initial[8] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };

extern "C" void sha256_transform(uint32_t *state, const uint32_t *block, int swap);
#endif

// 4-way kernel padding
static const uint32_t block1_suffix_4way[4 * 9] = {
    0x80000000, 0x80000000, 0x80000000, 0x80000000,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0x000000e0, 0x000000e0, 0x000000e0, 0x000000e0
};

// 4-way hash padding
static const uint32_t block2_suffix_4way[4 * 8] = {
    0x80000000, 0x80000000, 0x80000000, 0x80000000,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0x00000100, 0x00000100, 0x00000100, 0x00000100
};

extern "C" int sha256_use_4way();
extern "C" void sha256_init_4way(uint32_t *state);
extern "C" void sha256_transform_4way(uint32_t *state, const uint32_t *block, int swap);
bool fUse4Way = sha256_use_4way() != 0;

#ifdef __x86_64__
// 8-way kernel padding
static const uint32_t block1_suffix_8way[8 * 9] = {
    0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0x000000e0, 0x000000e0, 0x000000e0, 0x000000e0, 0x000000e0, 0x000000e0, 0x000000e0, 0x000000e0
};

// 8-way hash padding
static const uint32_t block2_suffix_8way[8 * 8] = {
    0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0x000000e0, 0x000000e0, 0x000000e0, 0x000000e0, 0x000000e0, 0x000000e0, 0x000000e0, 0x000000e0
};

extern "C" int sha256_use_8way();
extern "C" void sha256_init_8way(uint32_t *state);
extern "C" void sha256_transform_8way(uint32_t *state, const uint32_t *block, int swap);
bool fUse8Way = sha256_use_8way() != 0;

inline void copyrow8_swap32(uint32_t *to, uint32_t *from)
{
    // There are no AVX2 CPUs without SSSE3 support, so we don't need any conditions here.
    __m128i mask = _mm_set_epi8(12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3);
    _mm_storeu_si128((__m128i *)&to[0], _mm_shuffle_epi8(_mm_loadu_si128((__m128i *)&from[0]), mask));
    _mm_storeu_si128((__m128i *)&to[4], _mm_shuffle_epi8(_mm_loadu_si128((__m128i *)&from[4]), mask));
}
#endif

#if defined(__i386__) || defined(__x86_64__)
extern "C" int sha256_use_ssse3();
bool fUseSSSE3 = sha256_use_ssse3() != 0;

inline void copyrow4_swap32(uint32_t *to, uint32_t *from)
{
    if (!fUseSSSE3)
    {
        for (int i = 0; i < 4; i++)
            to[i] = __builtin_bswap32(from[i]);
    }
    else
    {
        __m128i mask = _mm_set_epi8(12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3);
        _mm_storeu_si128((__m128i *)&to[0], _mm_shuffle_epi8(_mm_loadu_si128((__m128i *)&from[0]), mask));
    }
}
#else
inline void copyrow4_swap32(uint32_t *to, uint32_t *from)
{
    for (int i = 0; i < 4; i++)
        to[i] = __builtin_bswap32(from[i]);
}
#endif
#endif

KernelWorker::KernelWorker(unsigned char *kernel, uint32_t nBits, uint32_t nInputTxTime, int64_t nValueIn, uint32_t nIntervalBegin, uint32_t nIntervalEnd) 
        : kernel(kernel), nBits(nBits), nInputTxTime(nInputTxTime), bnValueIn(nValueIn), nIntervalBegin(nIntervalBegin), nIntervalEnd(nIntervalEnd)
    {
        solutions = vector<std::pair<uint256,uint32_t> >();
    }

#ifdef USE_ASM
#ifdef __x86_64__
void KernelWorker::Do_8way()
{
    SetThreadPriority(THREAD_PRIORITY_LOWEST);

    // Compute maximum possible target to filter out majority of obviously insufficient hashes
    CBigNum bnTargetPerCoinDay;
    bnTargetPerCoinDay.SetCompact(nBits);
    uint256 nMaxTarget = (bnTargetPerCoinDay * bnValueIn * nStakeMaxAge / COIN / nOneDay).getuint256();

#ifdef _MSC_VER
    __declspec(align(16)) uint32_t blocks1[8 * 16];
    __declspec(align(16)) uint32_t blocks2[8 * 16];
    __declspec(align(16)) uint32_t candidates[8 * 8];
#else
    uint32_t blocks1[8 * 16] __attribute__((aligned(16)));
    uint32_t blocks2[8 * 16] __attribute__((aligned(16)));
    uint32_t candidates[8 * 8] __attribute__((aligned(16)));
#endif

    vector<uint32_t> vRow = vector<uint32_t>(8);
    uint32_t *pnKernel = (uint32_t *) kernel;

    for(int i = 0; i < 7; i++)
    {
        fill(vRow.begin(), vRow.end(), pnKernel[i]);
        copyrow8_swap32(&blocks1[i*8], &vRow[0]);
    }

    memcpy(&blocks1[56], &block1_suffix_8way[0], 36*8);   // sha256 padding
    memcpy(&blocks2[64], &block2_suffix_8way[0], 32*8);

    uint32_t nHashes[8];
    uint32_t nTimeStamps[8];

    // Search forward in time from the given timestamp
    // Stopping search in case of shutting down
    for (uint32_t nTimeTx=nIntervalBegin, nMaxTarget32 = nMaxTarget.Get32(7); nTimeTx<nIntervalEnd && !fShutdown; nTimeTx +=8)
    {
        sha256_init_8way(blocks2);
        sha256_init_8way(candidates);

        nTimeStamps[0] = nTimeTx;
        nTimeStamps[1] = nTimeTx+1;
        nTimeStamps[2] = nTimeTx+2;
        nTimeStamps[3] = nTimeTx+3;
        nTimeStamps[4] = nTimeTx+4;
        nTimeStamps[5] = nTimeTx+5;
        nTimeStamps[6] = nTimeTx+6;
        nTimeStamps[7] = nTimeTx+7;

        copyrow8_swap32(&blocks1[24], &nTimeStamps[0]); // Kernel timestamps
        sha256_transform_8way(&blocks2[0], &blocks1[0], 0); // first hashing
        sha256_transform_8way(&candidates[0], &blocks2[0], 0); // second hashing
        copyrow8_swap32(&nHashes[0], &candidates[56]);

        for(int nResult = 0; nResult < 8; nResult++)
        {
            if (nHashes[nResult] <= nMaxTarget32) // Possible hit
            {
                uint256 nHashProofOfStake = 0;
                uint32_t *pnHashProofOfStake = (uint32_t *) &nHashProofOfStake;

                for (int i = 0; i < 7; i++)
                    pnHashProofOfStake[i] = __builtin_bswap32(candidates[(i*8) + nResult]);
                pnHashProofOfStake[7] = nHashes[nResult];

                CBigNum bnCoinDayWeight = bnValueIn * GetWeight((int64_t)nInputTxTime, (int64_t)nTimeStamps[nResult]) / COIN / nOneDay;
                CBigNum bnTargetProofOfStake = bnCoinDayWeight * bnTargetPerCoinDay;

                if (bnTargetProofOfStake >= CBigNum(nHashProofOfStake))
                    solutions.push_back(std::pair<uint256,uint32_t>(nHashProofOfStake, nTimeStamps[nResult]));
            }
        }
    }
}
#endif

void KernelWorker::Do_4way()
{
    SetThreadPriority(THREAD_PRIORITY_LOWEST);

    // Compute maximum possible target to filter out majority of obviously insufficient hashes
    CBigNum bnTargetPerCoinDay;
    bnTargetPerCoinDay.SetCompact(nBits);
    uint256 nMaxTarget = (bnTargetPerCoinDay * bnValueIn * nStakeMaxAge / COIN / nOneDay).getuint256();

#ifdef _MSC_VER
    __declspec(align(16)) uint32_t blocks1[4 * 16];
    __declspec(align(16)) uint32_t blocks2[4 * 16];
    __declspec(align(16)) uint32_t candidates[4 * 8];
#else
    uint32_t blocks1[4 * 16] __attribute__((aligned(16)));
    uint32_t blocks2[4 * 16] __attribute__((aligned(16)));
    uint32_t candidates[4 * 8] __attribute__((aligned(16)));
#endif

    vector<uint32_t> vRow = vector<uint32_t>(4);
    uint32_t *pnKernel = (uint32_t *) kernel;

    for(int i = 0; i < 7; i++)
    {
        fill(vRow.begin(), vRow.end(), pnKernel[i]);
        copyrow4_swap32(&blocks1[i*4], &vRow[0]);
    }

    memcpy(&blocks1[28], &block1_suffix_4way[0], 36*4);   // sha256 padding
    memcpy(&blocks2[32], &block2_suffix_4way[0], 32*4);

    uint32_t nHashes[4];
    uint32_t nTimeStamps[4];

    // Search forward in time from the given timestamp
    // Stopping search in case of shutting down
    for (uint32_t nTimeTx=nIntervalBegin, nMaxTarget32 = nMaxTarget.Get32(7); nTimeTx<nIntervalEnd && !fShutdown; nTimeTx +=4)
    {
        sha256_init_4way(blocks2);
        sha256_init_4way(candidates);

        nTimeStamps[0] = nTimeTx;
        nTimeStamps[1] = nTimeTx+1;
        nTimeStamps[2] = nTimeTx+2;
        nTimeStamps[3] = nTimeTx+3;

        copyrow4_swap32(&blocks1[24], &nTimeStamps[0]); // Kernel timestamps
        sha256_transform_4way(&blocks2[0], &blocks1[0], 0); // first hashing
        sha256_transform_4way(&candidates[0], &blocks2[0], 0); // second hashing
        copyrow4_swap32(&nHashes[0], &candidates[28]);

        for(int nResult = 0; nResult < 4; nResult++)
        {
            if (nHashes[nResult] <= nMaxTarget32) // Possible hit
            {
                uint256 nHashProofOfStake = 0;
                uint32_t *pnHashProofOfStake = (uint32_t *) &nHashProofOfStake;

                for (int i = 0; i < 7; i++)
                    pnHashProofOfStake[i] = __builtin_bswap32(candidates[(i*4) + nResult]);
                pnHashProofOfStake[7] = nHashes[nResult];

                CBigNum bnCoinDayWeight = bnValueIn * GetWeight((int64_t)nInputTxTime, (int64_t)nTimeStamps[nResult]) / COIN / nOneDay;
                CBigNum bnTargetProofOfStake = bnCoinDayWeight * bnTargetPerCoinDay;

                if (bnTargetProofOfStake >= CBigNum(nHashProofOfStake))
                    solutions.push_back(std::pair<uint256,uint32_t>(nHashProofOfStake, nTimeStamps[nResult]));
            }
        }
    }
}
#endif

void KernelWorker::Do_generic()
{
    SetThreadPriority(THREAD_PRIORITY_LOWEST);

    // Compute maximum possible target to filter out majority of obviously insufficient hashes
    CBigNum bnTargetPerCoinDay;
    bnTargetPerCoinDay.SetCompact(nBits);
    uint256 nMaxTarget = (bnTargetPerCoinDay * bnValueIn * nStakeMaxAge / COIN / nOneDay).getuint256();

#if !defined(USE_ASM) || defined(__i386__)
    SHA256_CTX ctx, workerCtx;
    // Init new sha256 context and update it
    //   with first 24 bytes of kernel
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, kernel, 8 + 16);
    workerCtx = ctx; // save context

    // Sha256 result buffer
    uint32_t hashProofOfStake[8];
    uint256 *pnHashProofOfStake = (uint256 *)&hashProofOfStake;

    // Search forward in time from the given timestamp
    // Stopping search in case of shutting down
    for (uint32_t nTimeTx=nIntervalBegin, nMaxTarget32 = nMaxTarget.Get32(7); nTimeTx<nIntervalEnd && !fShutdown; nTimeTx++)
    {
        // Complete first hashing iteration
        uint256 hash1;
        SHA256_Update(&ctx, (unsigned char*)&nTimeTx, 4);
        SHA256_Final((unsigned char*)&hash1, &ctx);

        // Restore context
        ctx = workerCtx;

        // Finally, calculate kernel hash
        SHA256((unsigned char*)&hash1, sizeof(hashProofOfStake), (unsigned char*)&hashProofOfStake);

        // Skip if hash doesn't satisfy the maximum target
        if (hashProofOfStake[7] > nMaxTarget32)
            continue;

        CBigNum bnCoinDayWeight = bnValueIn * GetWeight((int64_t)nInputTxTime, (int64_t)nTimeTx) / COIN / nOneDay;
        CBigNum bnTargetProofOfStake = bnCoinDayWeight * bnTargetPerCoinDay;

        if (bnTargetProofOfStake >= CBigNum(*pnHashProofOfStake))
            solutions.push_back(std::pair<uint256,uint32_t>(*pnHashProofOfStake, nTimeTx));
    }
#else

#ifdef _MSC_VER
    __declspec(align(16)) uint32_t block1[16];
    __declspec(align(16)) uint32_t block2[16];
    __declspec(align(16)) uint32_t candidate[8];
#else
    uint32_t block1[16] __attribute__((aligned(16)));
    uint32_t block2[16] __attribute__((aligned(16)));
    uint32_t candidate[8] __attribute__((aligned(16)));
#endif

    memcpy(&block1[7], &block1_suffix[0], 36);   // sha256 padding
    memcpy(&block2[8], &block2_suffix[0], 32);

    uint32_t *pnKernel = (uint32_t *) kernel;

    for (int i = 0; i < 6; i++)
        block1[i] = __builtin_bswap32(pnKernel[i]);

    // Search forward in time from the given timestamp
    // Stopping search in case of shutting down
    for (uint32_t nTimeTx=nIntervalBegin, nMaxTarget32 = nMaxTarget.Get32(7); nTimeTx<nIntervalEnd && !fShutdown; nTimeTx++)
    {
        memcpy(&block2[0], &sha256_initial[0], 32);
        memcpy(&candidate[0], &sha256_initial[0], 32);

        block1[6] = __builtin_bswap32(nTimeTx);

        sha256_transform(&block2[0], &block1[0], 0); // first hashing
        sha256_transform(&candidate[0], &block2[0], 0); // second hashing

        uint32_t nHash7 = __builtin_bswap32(candidate[7]);

        // Skip if hash doesn't satisfy the maximum target
        if (nHash7 > nMaxTarget32)
            continue;

        uint256 nHashProofOfStake;
        uint32_t *pnHashProofOfStake = (uint32_t *) &nHashProofOfStake;

        for (int i = 0; i < 7; i++)
            pnHashProofOfStake[i] = __builtin_bswap32(candidate[i]);
        pnHashProofOfStake[7] = nHash7;

        CBigNum bnCoinDayWeight = bnValueIn * GetWeight((int64_t)nInputTxTime, (int64_t)nTimeTx) / COIN / nOneDay;
        CBigNum bnTargetProofOfStake = bnCoinDayWeight * bnTargetPerCoinDay;

        if (bnTargetProofOfStake >= CBigNum(nHashProofOfStake))
            solutions.push_back(std::pair<uint256,uint32_t>(nHashProofOfStake, nTimeTx));
    }
#endif
}

void KernelWorker::Do()
{
#ifdef USE_ASM
#ifdef __x86_64__
    if (false && fUse8Way) // disable for now
    {
        Do_8way();
        return;
    }
#endif
    if (fUse4Way)
    {
        Do_4way();
        return;
    }
#endif

    Do_generic();
}

vector<pair<uint256,uint32_t> >& KernelWorker::GetSolutions()
{
    return solutions;
}

// Scan given kernel for solutions
#ifdef USE_ASM

#ifdef __x86_64__
bool ScanKernelBackward_8Way(unsigned char *kernel, uint32_t nBits, uint32_t nInputTxTime, int64_t nValueIn, std::pair<uint32_t, uint32_t> &SearchInterval, std::pair<uint256, uint32_t> &solution)
{
    CBigNum bnTargetPerCoinDay;
    bnTargetPerCoinDay.SetCompact(nBits);

    CBigNum bnValueIn(nValueIn);

    // Get maximum possible target to filter out the majority of obviously insufficient hashes
    uint256 nMaxTarget = (bnTargetPerCoinDay * bnValueIn * nStakeMaxAge / COIN / nOneDay).getuint256();

#ifdef _MSC_VER
    __declspec(align(16)) uint32_t blocks1[8 * 16];
    __declspec(align(16)) uint32_t blocks2[8 * 16];
    __declspec(align(16)) uint32_t candidates[8 * 8];
#else
    uint32_t blocks1[8 * 16] __attribute__((aligned(16)));
    uint32_t blocks2[8 * 16] __attribute__((aligned(16)));
    uint32_t candidates[8 * 8] __attribute__((aligned(16)));
#endif

    vector<uint32_t> vRow = vector<uint32_t>(8);
    uint32_t *pnKernel = (uint32_t *) kernel;

    for(int i = 0; i < 7; i++)
    {
        fill(vRow.begin(), vRow.end(), pnKernel[i]);
        copyrow8_swap32(&blocks1[i*8], &vRow[0]);
    }

    memcpy(&blocks1[56], &block1_suffix_8way[0], 36*8);   // sha256 padding
    memcpy(&blocks2[64], &block2_suffix_8way[0], 32*8);

    uint32_t nHashes[8];
    uint32_t nTimeStamps[8];

    // Search forward in time from the given timestamp
    // Stopping search in case of shutting down
    for (uint32_t nTimeTx=SearchInterval.first, nMaxTarget32 = nMaxTarget.Get32(7); nTimeTx>SearchInterval.second && !fShutdown; nTimeTx -=8)
    {
        sha256_init_8way(blocks2);
        sha256_init_8way(candidates);

        nTimeStamps[0] = nTimeTx;
        nTimeStamps[1] = nTimeTx-1;
        nTimeStamps[2] = nTimeTx-2;
        nTimeStamps[3] = nTimeTx-3;
        nTimeStamps[4] = nTimeTx-4;
        nTimeStamps[5] = nTimeTx-5;
        nTimeStamps[6] = nTimeTx-6;
        nTimeStamps[7] = nTimeTx-7;

        copyrow8_swap32(&blocks1[24], &nTimeStamps[0]); // Kernel timestamps
        sha256_transform_8way(&blocks2[0], &blocks1[0], 0); // first hashing
        sha256_transform_8way(&candidates[0], &blocks2[0], 0); // second hashing
        copyrow8_swap32(&nHashes[0], &candidates[56]);

        for(int nResult = 0; nResult < 8; nResult++)
        {
            if (nHashes[nResult] <= nMaxTarget32) // Possible hit
            {
                uint256 nHashProofOfStake = 0;
                uint32_t *pnHashProofOfStake = (uint32_t *) &nHashProofOfStake;

                for (int i = 0; i < 7; i++)
                    pnHashProofOfStake[i] = __builtin_bswap32(candidates[(i*8) + nResult]);
                pnHashProofOfStake[7] = nHashes[nResult];

                CBigNum bnCoinDayWeight = bnValueIn * GetWeight((int64_t)nInputTxTime, (int64_t)nTimeStamps[nResult]) / COIN / nOneDay;
                CBigNum bnTargetProofOfStake = bnCoinDayWeight * bnTargetPerCoinDay;

                if (bnTargetProofOfStake >= CBigNum(nHashProofOfStake))
                {
                    solution.first = nHashProofOfStake;
                    solution.second = nTimeStamps[nResult];

                    return true;
                }
            }
        }
    }

    return false;
}
#endif

bool ScanKernelBackward_4Way(unsigned char *kernel, uint32_t nBits, uint32_t nInputTxTime, int64_t nValueIn, std::pair<uint32_t, uint32_t> &SearchInterval, std::pair<uint256, uint32_t> &solution)
{
    CBigNum bnTargetPerCoinDay;
    bnTargetPerCoinDay.SetCompact(nBits);

    CBigNum bnValueIn(nValueIn);

    // Get maximum possible target to filter out the majority of obviously insufficient hashes
    uint256 nMaxTarget = (bnTargetPerCoinDay * bnValueIn * nStakeMaxAge / COIN / nOneDay).getuint256();

#ifdef _MSC_VER
    __declspec(align(16)) uint32_t blocks1[4 * 16];
    __declspec(align(16)) uint32_t blocks2[4 * 16];
    __declspec(align(16)) uint32_t candidates[4 * 8];
#else
    uint32_t blocks1[4 * 16] __attribute__((aligned(16)));
    uint32_t blocks2[4 * 16] __attribute__((aligned(16)));
    uint32_t candidates[4 * 8] __attribute__((aligned(16)));
#endif

    vector<uint32_t> vRow = vector<uint32_t>(4);
    uint32_t *pnKernel = (uint32_t *) kernel;

    for(int i = 0; i < 7; i++)
    {
        fill(vRow.begin(), vRow.end(), pnKernel[i]);
        copyrow4_swap32(&blocks1[i*4], &vRow[0]);
    }

    memcpy(&blocks1[28], &block1_suffix_4way[0], 36*4);   // sha256 padding
    memcpy(&blocks2[32], &block2_suffix_4way[0], 32*4);

    uint32_t nHashes[4];
    uint32_t nTimeStamps[4];

    // Search forward in time from the given timestamp
    // Stopping search in case of shutting down
    for (uint32_t nTimeTx=SearchInterval.first, nMaxTarget32 = nMaxTarget.Get32(7); nTimeTx>SearchInterval.second && !fShutdown; nTimeTx -=4)
    {
        sha256_init_4way(blocks2);
        sha256_init_4way(candidates);

        nTimeStamps[0] = nTimeTx;
        nTimeStamps[1] = nTimeTx-1;
        nTimeStamps[2] = nTimeTx-2;
        nTimeStamps[3] = nTimeTx-3;

        copyrow4_swap32(&blocks1[24], &nTimeStamps[0]); // Kernel timestamps
        sha256_transform_4way(&blocks2[0], &blocks1[0], 0); // first hashing
        sha256_transform_4way(&candidates[0], &blocks2[0], 0); // second hashing
        copyrow4_swap32(&nHashes[0], &candidates[28]);

        for(int nResult = 0; nResult < 4; nResult++)
        {
            if (nHashes[nResult] <= nMaxTarget32) // Possible hit
            {
                uint256 nHashProofOfStake = 0;
                uint32_t *pnHashProofOfStake = (uint32_t *) &nHashProofOfStake;

                for (int i = 0; i < 7; i++)
                    pnHashProofOfStake[i] = __builtin_bswap32(candidates[(i*4) + nResult]);
                pnHashProofOfStake[7] = nHashes[nResult];

                CBigNum bnCoinDayWeight = bnValueIn * GetWeight((int64_t)nInputTxTime, (int64_t)nTimeStamps[nResult]) / COIN / nOneDay;
                CBigNum bnTargetProofOfStake = bnCoinDayWeight * bnTargetPerCoinDay;

                if (bnTargetProofOfStake >= CBigNum(nHashProofOfStake))
                {
                    solution.first = nHashProofOfStake;
                    solution.second = nTimeStamps[nResult];

                    return true;
                }
            }
        }
    }

    return false;
}
#endif

bool ScanKernelBackward(unsigned char *kernel, uint32_t nBits, uint32_t nInputTxTime, int64_t nValueIn, std::pair<uint32_t, uint32_t> &SearchInterval, std::pair<uint256, uint32_t> &solution)
{
#ifdef USE_ASM
#ifdef __x86_64__
    if (false && fUse8Way) // disable for now
    {
        return ScanKernelBackward_8Way(kernel, nBits, nInputTxTime, nValueIn, SearchInterval, solution);
    }
#endif
    if (fUse4Way)
    {
        return ScanKernelBackward_4Way(kernel, nBits, nInputTxTime, nValueIn, SearchInterval, solution);
    }
#endif

    CBigNum bnTargetPerCoinDay;
    bnTargetPerCoinDay.SetCompact(nBits);

    CBigNum bnValueIn(nValueIn);

    // Get maximum possible target to filter out the majority of obviously insufficient hashes
    uint256 nMaxTarget = (bnTargetPerCoinDay * bnValueIn * nStakeMaxAge / COIN / nOneDay).getuint256();

    SHA256_CTX ctx, workerCtx;
    // Init new sha256 context and update it
    //   with first 24 bytes of kernel
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, kernel, 8 + 16);
    workerCtx = ctx; // save context

    // Search backward in time from the given timestamp
    // Stopping search in case of shutting down
    for (uint32_t nTimeTx=SearchInterval.first; nTimeTx>SearchInterval.second && !fShutdown; nTimeTx--)
    {
        // Complete first hashing iteration
        uint256 hash1;
        SHA256_Update(&ctx, (unsigned char*)&nTimeTx, 4);
        SHA256_Final((unsigned char*)&hash1, &ctx);

        // Restore context
        ctx = workerCtx;

        // Finally, calculate kernel hash
        uint256 hashProofOfStake;
        SHA256((unsigned char*)&hash1, sizeof(hashProofOfStake), (unsigned char*)&hashProofOfStake);

        // Skip if hash doesn't satisfy the maximum target
        if (hashProofOfStake > nMaxTarget)
            continue;

        CBigNum bnCoinDayWeight = bnValueIn * GetWeight((int64_t)nInputTxTime, (int64_t)nTimeTx) / COIN / nOneDay;
        CBigNum bnTargetProofOfStake = bnCoinDayWeight * bnTargetPerCoinDay;

        if (bnTargetProofOfStake >= CBigNum(hashProofOfStake))
        {
            solution.first = hashProofOfStake;
            solution.second = nTimeTx;

            return true;
        }
    }

    return false;
}
