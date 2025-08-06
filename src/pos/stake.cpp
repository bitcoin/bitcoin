#include <pos/stake.h>
#include <arith_uint256.h>
#include <chain.h>
#include <hash.h>
#include <serialize.h>
#include <cassert>

bool CheckStakeKernelHash(const CBlockIndex* pindexPrev, unsigned int nBits,
                          const CBlock& blockFrom, unsigned int nTxPrevOffset,
                          const CTransactionRef& txPrev, const COutPoint& prevout,
                          unsigned int nTimeTx, uint256& hashProofOfStake,
                          bool fPrintProofOfStake)
{
    assert(pindexPrev);

    // Basic consensus checks on the staking input
    if (prevout.n >= txPrev->vout.size()) {
        return false; // invalid outpoint
    }
    if (blockFrom.GetBlockTime() >= nTimeTx) {
        return false; // coin not mature enough
    }

    // Derive a simple stake modifier from the previous block hash and height
    HashWriter ss_mod;
    ss_mod << pindexPrev->GetBlockHash() << pindexPrev->nHeight << pindexPrev->nTime;
    const uint256 stake_modifier = ss_mod.GetHash();

    // Time weight is the age of the coin being staked
    const unsigned int nTimeWeight = nTimeTx - blockFrom.GetBlockTime();

    // Build the kernel hash using the stake modifier and prevout details
    HashWriter ss_kernel;
    ss_kernel << stake_modifier << blockFrom.GetHash() << nTxPrevOffset
              << txPrev->GetHash() << prevout.hash << prevout.n << nTimeTx;
    hashProofOfStake = ss_kernel.GetHash();

    // Target is weighted by coin amount and time weight
    arith_uint256 bnTarget;
    bnTarget.SetCompact(nBits);
    const arith_uint256 bnWeight = arith_uint256(txPrev->vout[prevout.n].nValue) * nTimeWeight;
    bnTarget *= bnWeight;

    if (UintToArith256(hashProofOfStake) > bnTarget) {
        return false;
    }

    if (fPrintProofOfStake) {
        // Logging could be added here for detailed kernel evaluation
    }

    return true;
}

