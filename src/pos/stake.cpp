#include <arith_uint256.h>
#include <cassert>
#include <chain.h>
#include <coins.h>
#include <consensus/amount.h>
#include <hash.h>
#include <pos/stake.h>
#include <serialize.h>
#include <validation.h>

// Minimum value for any staking input
static constexpr CAmount MIN_STAKE_AMOUNT{1 * COIN};

bool CheckStakeKernelHash(const CBlockIndex* pindexPrev, unsigned int nBits,
                          uint256 hashBlockFrom, unsigned int nTimeBlockFrom,
                          CAmount amount, const COutPoint& prevout,
                          unsigned int nTimeTx, uint256& hashProofOfStake,
                          bool fPrintProofOfStake)
{
    assert(pindexPrev);

    // Require timestamp to be masked to 16-second granularity
    if (nTimeTx & STAKE_TIMESTAMP_MASK) {
        return false;
    }

    // Enforce minimum coin age
    if (nTimeTx <= nTimeBlockFrom || nTimeTx - nTimeBlockFrom < MIN_STAKE_AGE) {
        return false;
    }

    // Derive a stake modifier from the previous block
    HashWriter ss_mod;
    ss_mod << pindexPrev->GetBlockHash() << pindexPrev->nHeight << pindexPrev->nTime;
    const uint256 stake_modifier = ss_mod.GetHash();

    // Mask times before hashing to reduce kernel search space
    const unsigned int nTimeTxMasked{nTimeTx & ~STAKE_TIMESTAMP_MASK};
    const unsigned int nTimeBlockFromMasked{nTimeBlockFrom & ~STAKE_TIMESTAMP_MASK};

    // Build the kernel hash
    HashWriter ss_kernel;
    ss_kernel << stake_modifier << hashBlockFrom << nTimeBlockFromMasked
              << prevout.hash << prevout.n << nTimeTxMasked;
    hashProofOfStake = ss_kernel.GetHash();

    // Target is weighted by coin amount (amount / COIN)
    arith_uint256 bnTarget;
    bnTarget.SetCompact(nBits);
    arith_uint256 bnWeight(amount / COIN);
    bnTarget *= bnWeight;

    if (UintToArith256(hashProofOfStake) > bnTarget) {
        return false;
    }

    if (fPrintProofOfStake) {
        // Optional: add detailed logging for kernel evaluation
    }

    return true;
}

bool ContextualCheckProofOfStake(const CBlock& block, const CBlockIndex* pindexPrev,
                                 const CCoinsViewCache& view, const CChain& chain,
                                 const Consensus::Params& params)
{
    assert(pindexPrev);

    // Allow first block after genesis to be mined with proof-of-work
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

    uint256 hashProof;
    for (size_t i = 0; i < tx->vin.size(); ++i) {
        const CTxIn& txin = tx->vin[i];
        const Coin& coin = view.AccessCoin(txin.prevout);
        if (coin.IsSpent()) {
            return false;
        }

        const CBlockIndex* pindexFrom = chain[coin.nHeight];
        if (!pindexFrom) {
            return false;
        }

        // All inputs must satisfy basic age and amount requirements
        if (block.nTime <= pindexFrom->nTime ||
            block.nTime - pindexFrom->nTime < MIN_STAKE_AGE) {
            return false;
        }
        if (coin.out.nValue < MIN_STAKE_AMOUNT) {
            return false;
        }

        // Perform kernel check on the designated (first) input
        if (i == 0) {
            if (!CheckStakeKernelHash(pindexPrev, block.nBits, pindexFrom->GetBlockHash(),
                                      pindexFrom->nTime, coin.out.nValue, txin.prevout,
                                      block.nTime, hashProof, false)) {
                return false;
            }
        }
    }
    return true;
}
