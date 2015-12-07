#include <vector>
#include <inttypes.h>

#include "uint256.h"
#include "bignum.h"
#include "kernel.h"
#include "kernel_worker.h"

using namespace std;

KernelWorker::KernelWorker(unsigned char *kernel, uint32_t nBits, uint32_t nInputTxTime, int64_t nValueIn, uint32_t nIntervalBegin, uint32_t nIntervalEnd) 
        : kernel(kernel), nBits(nBits), nInputTxTime(nInputTxTime), bnValueIn(nValueIn), nIntervalBegin(nIntervalBegin), nIntervalEnd(nIntervalEnd)
    {
        solutions = vector<std::pair<uint256,uint32_t> >();
    }

void KernelWorker::Do_generic()
{
    SetThreadPriority(THREAD_PRIORITY_LOWEST);

    // Compute maximum possible target to filter out majority of obviously insufficient hashes
    CBigNum bnTargetPerCoinDay;
    bnTargetPerCoinDay.SetCompact(nBits);
    uint256 nMaxTarget = (bnTargetPerCoinDay * bnValueIn * nStakeMaxAge / COIN / nOneDay).getuint256();

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
}

void KernelWorker::Do()
{
    Do_generic();
}

vector<pair<uint256,uint32_t> >& KernelWorker::GetSolutions()
{
    return solutions;
}

// Scan given kernel for solutions

bool ScanKernelBackward(unsigned char *kernel, uint32_t nBits, uint32_t nInputTxTime, int64_t nValueIn, std::pair<uint32_t, uint32_t> &SearchInterval, std::pair<uint256, uint32_t> &solution)
{
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
