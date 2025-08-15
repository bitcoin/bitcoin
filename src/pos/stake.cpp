#include <arith_uint256.h>
#include <cassert>
#include <chain.h>
#include <coins.h>
#include <consensus/amount.h>
#include <hash.h>
#include <logging.h>
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

    LogTrace(BCLog::STAKING,
             "CheckStakeKernelHash: height=%d amount=%d nTimeTx=%u nTimeBlockFrom=%u",
             pindexPrev->nHeight, amount, nTimeTx, nTimeBlockFrom);

    // Require timestamp to be masked to 16-second granularity
    if (nTimeTx & STAKE_TIMESTAMP_MASK) {
        LogDebug(BCLog::STAKING,
                 "CheckStakeKernelHash: timestamp %u not masked", nTimeTx);
        return false;
    }

    // Enforce minimum coin age
    if (nTimeTx <= nTimeBlockFrom || nTimeTx - nTimeBlockFrom < MIN_STAKE_AGE) {
        LogDebug(BCLog::STAKING,
                 "CheckStakeKernelHash: min age violation nTimeTx=%u nTimeBlockFrom=%u",
                 nTimeTx, nTimeBlockFrom);
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

    LogTrace(BCLog::STAKING,
             "CheckStakeKernelHash: hash=%s target=%s",
             hashProofOfStake.ToString(), bnTarget.ToString());

    if (UintToArith256(hashProofOfStake) > bnTarget) {
        LogDebug(BCLog::STAKING,
                 "CheckStakeKernelHash: kernel hash %s exceeds target %s",
                 hashProofOfStake.ToString(), bnTarget.ToString());
        return false;
    }

    if (fPrintProofOfStake) {
        LogInfo("CheckStakeKernelHash: hash=%s target=%s",
                hashProofOfStake.ToString(), bnTarget.ToString());
    }

    LogDebug(BCLog::STAKING,
             "CheckStakeKernelHash: kernel meets target hash=%s",
             hashProofOfStake.ToString());

    return true;
}

bool ContextualCheckProofOfStake(const CBlock& block, const CBlockIndex* pindexPrev,
                                 const CCoinsViewCache& view, const CChain& chain,
                                 const Consensus::Params& params)
{
    assert(pindexPrev);

    LogTrace(BCLog::STAKING,
             "ContextualCheckProofOfStake: height=%d time=%u txs=%u",
             pindexPrev->nHeight, block.nTime, block.vtx.size());

    // Allow first block after genesis to be mined with proof-of-work
    if (pindexPrev->nHeight < 1) {
        return true;
    }

    if (block.vtx.size() < 2) {
        LogDebug(BCLog::STAKING,
                 "ContextualCheckProofOfStake: block missing coinstake tx");
        return false; // Needs coinbase and coinstake
    }
    const CTransactionRef& tx = block.vtx[1];
    if (tx->vin.empty() || tx->vout.empty() || !tx->vout[0].IsNull()) {
        LogDebug(BCLog::STAKING,
                 "ContextualCheckProofOfStake: invalid coinstake structure");
        return false;
    }

    uint256 hashProof;
    for (size_t i = 0; i < tx->vin.size(); ++i) {
        const CTxIn& txin = tx->vin[i];
        const Coin& coin = view.AccessCoin(txin.prevout);
        if (coin.IsSpent()) {
            LogDebug(BCLog::STAKING,
                     "ContextualCheckProofOfStake: spent stake input %s",
                     txin.prevout.ToString());
            return false;
        }

        const CBlockIndex* pindexFrom = chain[coin.nHeight];
        if (!pindexFrom) {
            LogDebug(BCLog::STAKING,
                     "ContextualCheckProofOfStake: missing block for input %s",
                     txin.prevout.ToString());
            return false;
        }

        // All inputs must satisfy basic age and amount requirements
        if (block.nTime <= pindexFrom->nTime ||
            block.nTime - pindexFrom->nTime < MIN_STAKE_AGE) {
            LogDebug(BCLog::STAKING,
                     "ContextualCheckProofOfStake: input %s too young", txin.prevout.ToString());
            return false;
        }
        if (coin.out.nValue < MIN_STAKE_AMOUNT) {
            LogDebug(BCLog::STAKING,
                     "ContextualCheckProofOfStake: input %s value too low %d",
                     txin.prevout.ToString(), coin.out.nValue);
            return false;
        }

        // Perform kernel check on the designated (first) input
        if (i == 0) {
            if (!CheckStakeKernelHash(pindexPrev, block.nBits, pindexFrom->GetBlockHash(),
                                      pindexFrom->nTime, coin.out.nValue, txin.prevout,
                                      block.nTime, hashProof, true)) {
                LogDebug(BCLog::STAKING,
                         "ContextualCheckProofOfStake: kernel check failed for %s",
                         txin.prevout.ToString());
                return false;
            }
        }
    }
    return true;
}

bool IsProofOfStake(const CBlock& block)
{
    if (block.vtx.size() < 2) return false;
    const CTransactionRef& tx = block.vtx[1];
    if (tx->vin.empty() || tx->vout.size() < 2) return false;
    if (tx->vin[0].prevout.IsNull()) return false;
    if (!tx->vout[0].IsNull()) return false;
    return true;
}
