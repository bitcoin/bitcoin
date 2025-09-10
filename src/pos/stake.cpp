#include <pos/stake.h>
#include <pos/difficulty.h>

#include <arith_uint256.h>
#include <hash.h>
#include <primitives/transaction.h>
#include <script/standard.h>
#include <util/overflow.h>
#include <logging.h>

#include <cassert>

/**
 * Very early / minimal PoSv3.1 implementation.
 * NOTE: This is an initial kernel + validation path and will be extended in the full
 * implementation pass (stake modifier, block signature, reward clipping, advanced
 * fakestake mitigations). It is added now to begin wiring the consensus surface the
 * rest of the code already references (validation.cpp includes these headers).
 */

static bool IsCoinStakeTx(const CTransaction& tx)
{
    // A coinstake must: not be coinbase; have at least one input; have at least two outputs
    // and the first output must be empty (scriptPubKey.size()==0) per typical PoS v3 pattern.
    if (tx.IsCoinBase()) return false;
    if (tx.vin.empty()) return false;
    if (tx.vout.size() < 2) return false;
    if (!tx.vout[0].scriptPubKey.empty()) return false;
    return true;
}

bool IsProofOfStake(const CBlock& block)
{
    if (block.vtx.size() < 2) return false;
    return IsCoinStakeTx(*block.vtx[1]);
}

// Basic stake kernel hash: H( prevout.hash || prevout.n || nTimeBlockFrom || nTimeTx )
// Later iterations will introduce a proper stake modifier and possibly richer entropy.
static uint256 ComputeKernelHash(const COutPoint& prevout,
                                 unsigned int nTimeBlockFrom,
                                 unsigned int nTimeTx)
{
    CHashWriter ss(SER_GETHASH, 0);
    ss << prevout.hash;
    ss << prevout.n;
    ss << nTimeBlockFrom;
    ss << nTimeTx;
    return ss.GetHash();
}

bool CheckStakeKernelHash(const CBlockIndex* pindexPrev,
                          unsigned int nBits,
                          uint256 hashBlockFrom,
                          unsigned int nTimeBlockFrom,
                          CAmount amount,
                          const COutPoint& prevout,
                          unsigned int nTimeTx,
                          uint256& hashProofOfStake,
                          bool fPrintProofOfStake,
                          const Consensus::Params& params)
{
    if (!pindexPrev) return false;
    if (nTimeTx <= nTimeBlockFrom) return false; // must move forward in time
    if ((nTimeTx & params.nStakeTimestampMask) != 0) return false; // enforce mask alignment

    // Derive target from nBits
    bool fNegative = false;
    bool fOverflow = false;
    arith_uint256 bnTarget;
    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);
    if (fNegative || fOverflow || bnTarget == 0) return false;

    // Amount scaling: target * amount (bounded to prevent overflow)
    arith_uint256 bnWeight = arith_uint256(amount);
    // Avoid overflow by shifting if necessary
    // (Simplistic; in future incorporate 64-bit safe multiply or cap amount)
    arith_uint256 bnTargetWeight = bnTarget;
    bnTargetWeight *= bnWeight;

    hashProofOfStake = ComputeKernelHash(prevout, nTimeBlockFrom, nTimeTx);
    arith_uint256 bnHash = UintToArith256(hashProofOfStake);

    if (fPrintProofOfStake) {
        LogDebug(BCLog::STAKE, "CheckStakeKernelHash: hash=%s target=%s amt=%lld\n",
                 hashProofOfStake.ToString(), bnTargetWeight.ToString(), amount);
    }

    if (bnHash > bnTargetWeight) return false;
    return true;
}

bool ContextualCheckProofOfStake(const CBlock& block,
                                 const CBlockIndex* pindexPrev,
                                 const CCoinsViewCache& view,
                                 const CChain& chain,
                                 const Consensus::Params& params)
{
    if (!pindexPrev) return false;
    if (!IsProofOfStake(block)) return false;

    const CTransaction& coinstake = *block.vtx[1];

    // Enforce block time == coinstake tx time (v3 convention)
    if (block.nTime != coinstake.nTime) return false;
    if ((block.nTime & params.nStakeTimestampMask) != 0) return false;

    // Check single coinstake only
    for (size_t i = 2; i < block.vtx.size(); ++i) {
        if (IsCoinStakeTx(*block.vtx[i])) return false; // multiple coinstakes
    }

    // Use first input as kernel (common approach)
    if (coinstake.vin.empty()) return false;
    const CTxIn& txin = coinstake.vin[0];
    const Coin& coin = view.AccessCoin(txin.prevout);
    if (coin.IsSpent()) return false;

    // Depth / confirmations
    int spend_height = pindexPrev->nHeight + 1; // height of the coinstake block
    int coin_height = coin.nHeight;
    if (coin_height <= 0 || coin_height > spend_height) return false;
    int depth = spend_height - coin_height;

    int minConf = params.nStakeMinConfirmations > 0 ? params.nStakeMinConfirmations : 80;
    if (depth < minConf) return false;

    // Minimum age (time based) – approximate using ancestor median time past difference.
    const CBlockIndex* pindexFrom = chain[coin_height];
    if (!pindexFrom) return false;
    if (block.GetBlockTime() - pindexFrom->GetBlockTime() < MIN_STAKE_AGE) return false;

    // Reconstruct previous stake source block time for kernel (using pindexFrom)
    unsigned int nTimeBlockFrom = pindexFrom->GetBlockTime();

    // Determine nBits / target for this PoS block
    unsigned int nBits = GetPoSNextTargetRequired(pindexPrev, block.GetBlockTime(), params);

    uint256 hashProofOfStake;
    if (!CheckStakeKernelHash(pindexPrev, nBits,
                              pindexFrom->GetBlockHash(), nTimeBlockFrom,
                              coin.out.nValue, txin.prevout,
                              block.nTime, hashProofOfStake, false, params)) {
        return false;
    }

    // Basic sanity on coinstake value: must not create more than inputs + reward (reward logic TBD)
    CAmount input_value = coin.out.nValue;
    CAmount output_value = 0;
    for (const auto& o : coinstake.vout) output_value += o.nValue;
    if (output_value < input_value) return false; // must not burn value in kernel input
    // (Excess is assumed to be stake reward + fees; capped later once reward code lands.)

    return true;
}