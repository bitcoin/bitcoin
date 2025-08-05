#include <pos/stake.h>
#include <chain.h>
#include <hash.h>
#include <serialize.h>
#include <arith_uint256.h>
#include <cassert>

bool CheckStakeKernelHash(const CBlockIndex* pindexPrev, unsigned int nBits,
                          const CBlock& blockFrom, unsigned int nTxPrevOffset,
                          const CTransactionRef& txPrev, const COutPoint& prevout,
                          unsigned int nTimeTx, uint256& hashProofOfStake,
                          bool fPrintProofOfStake)
{
    assert(pindexPrev);

    // TODO: Implement the full kernel hash algorithm with stake modifier and
    //       additional consensus checks.
    CHashWriter ss(SER_GETHASH, 0);
    ss << pindexPrev->GetBlockHash() << pindexPrev->nHeight << nBits
       << blockFrom.GetHash() << nTxPrevOffset << txPrev->GetHash()
       << prevout.hash << prevout.n << nTimeTx;
    hashProofOfStake = ss.GetHash();

    arith_uint256 bnTarget;
    bnTarget.SetCompact(nBits);
    if (UintToArith256(hashProofOfStake) > bnTarget) {
        return false;
    }

    if (fPrintProofOfStake) {
        // TODO: add verbose logging for stake kernel evaluations
    }

    return true;
}

