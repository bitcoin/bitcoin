#ifndef BITCOIN_POS_STAKE_H
#define BITCOIN_POS_STAKE_H

#include <consensus/params.h>
#include <primitives/block.h>
#include <uint256.h>

class CBlockIndex;

/** Check that the kernel for a stake meets the required target */
bool CheckStakeKernelHash(const CBlockIndex* pindexPrev, unsigned int nBits,
                          const CBlock& blockFrom, unsigned int nTxPrevOffset,
                          const CTransactionRef& txPrev, const COutPoint& prevout,
                          unsigned int nTimeTx, uint256& hashProofOfStake,
                          bool fPrintProofOfStake);

/**
 * Validate the proof-of-stake for a block. The block must include a coinstake
 * transaction whose inputs have matured enough time, and the stake kernel hash
 * must satisfy the network difficulty.
 */
inline bool CheckProofOfStake(const CBlock& block, const CBlockIndex* pindexPrev,
                              const Consensus::Params& params)
{
    assert(pindexPrev);
    // Allow the first block after genesis (height 1) to be mined with
    // proof-of-work for initial supply before proof-of-stake activates.
    if (pindexPrev->nHeight < 1) {
        return true;
    }

    if (block.vtx.size() < 2) {
        return false; // Needs coinbase and coinstake
    }

    const CTransactionRef& tx = block.vtx[1];
    if (tx->vin.empty() || tx->vout.empty() || !tx->vout[0].IsNull()) {
        return false;
    }

    uint256 hashProofOfStake;
    const COutPoint& prevout = tx->vin[0].prevout;

    // Enforce difficulty by verifying the stake kernel hash.
    if (!CheckStakeKernelHash(pindexPrev, block.nBits, block, 1, tx,
                              prevout, block.nTime, hashProofOfStake, false)) {
        return false;
    }

    return true;
}

#endif // BITCOIN_POS_STAKE_H
