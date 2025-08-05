#include <pos/stake.h>

#include <chain.h>
#include <consensus/consensus.h> 
#include <primitives/block.h>
#include <uint256.h>

// Stake kernel hash modifier
static const unsigned int STAKE_KERNEL_HASH_MODIFIER = 2;

// Check the stake kernel hash
bool CheckStakeKernelHash(const CBlockIndex* pindexPrev, unsigned int nBits, const CBlock& blockFrom, unsigned int nTxPrevOffset, const CTransactionRef& txPrev, const COutPoint& prevout, unsigned int nTimeTx, uint256& hashProofOfStake, bool fPrintProofOfStake)
{
    if (nTimeTx < txPrev->nTime)
        return false; // Transaction timestamp violation

    // Check that the difficulty matches the block
    if (blockFrom.nBits != nBits)
        return false;

    // Check that the transaction is in the block
    if (nTxPrevOffset + 1 > blockFrom.vtx.size() || blockFrom.vtx[nTxPrevOffset] != txPrev)
        return false;

    // Check that the output is in the transaction
    if (prevout.n >= txPrev->vout.size())
        return false;

    // Check that the transaction is a coinstake
    if (!txPrev->IsCoinStake())
        return false;

    // Check that the output is not spent
    if (txPrev->vout[prevout.n].IsNull())
        return false;

    // Calculate the proof-of-stake hash
    CDataStream ss(SER_GETHASH, 0);
    ss << pindexPrev->nStakeModifier << txPrev->nTime << prevout.hash << prevout.n << nTimeTx;
    hashProofOfStake = Hash(ss.begin(), ss.end());

    if (fPrintProofOfStake)
    {
        LogPrintf("CheckStakeKernelHash() : nStakeModifier=%s, txPrev.nTime=%u, prevout.hash=%s, prevout.n=%u, nTimeTx=%u, hashProofOfStake=%s\n",
            pindexPrev->nStakeModifier.GetHex(),
            txPrev->nTime, prevout.hash.ToString(), prevout.n, nTimeTx,
            hashProofOfStake.ToString());
    }

    // Check that the hash meets the difficulty target
    return (UintToArith256(hashProofOfStake) <= arith_uint256().SetCompact(nBits));
}
