// Copyright (c) 2017-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/tx_verify.h>

#include <consensus/consensus.h>
#include <primitives/transaction.h>
#include <script/interpreter.h>
#include <consensus/validation.h>

// TODO remove the following dependencies
#include <chain.h>
#include <coins.h>
#include <utilmoneystr.h>
#include <utilmemory.h>
#include <random.h>

#include <functional>

bool IsFinalTx(const CTransaction &tx, int nBlockHeight, int64_t nBlockTime)
{
    if (tx.nLockTime == 0)
        return true;
    if ((int64_t)tx.nLockTime < ((int64_t)tx.nLockTime < LOCKTIME_THRESHOLD ? (int64_t)nBlockHeight : nBlockTime))
        return true;
    for (const auto& txin : tx.vin) {
        if (!(txin.nSequence == CTxIn::SEQUENCE_FINAL))
            return false;
    }
    return true;
}

std::pair<int, int64_t> CalculateSequenceLocks(const CTransaction &tx, int flags, std::vector<int>* prevHeights, const CBlockIndex& block)
{
    assert(prevHeights->size() == tx.vin.size());

    // Will be set to the equivalent height- and time-based nLockTime
    // values that would be necessary to satisfy all relative lock-
    // time constraints given our view of block chain history.
    // The semantics of nLockTime are the last invalid height/time, so
    // use -1 to have the effect of any height or time being valid.
    int nMinHeight = -1;
    int64_t nMinTime = -1;

    // tx.nVersion is signed integer so requires cast to unsigned otherwise
    // we would be doing a signed comparison and half the range of nVersion
    // wouldn't support BIP 68.
    bool fEnforceBIP68 = static_cast<uint32_t>(tx.nVersion) >= 2
                      && flags & LOCKTIME_VERIFY_SEQUENCE;

    // Do not enforce sequence numbers as a relative lock time
    // unless we have been instructed to
    if (!fEnforceBIP68) {
        return std::make_pair(nMinHeight, nMinTime);
    }

    for (size_t txinIndex = 0; txinIndex < tx.vin.size(); txinIndex++) {
        const CTxIn& txin = tx.vin[txinIndex];

        // Sequence numbers with the most significant bit set are not
        // treated as relative lock-times, nor are they given any
        // consensus-enforced meaning at this point.
        if (txin.nSequence & CTxIn::SEQUENCE_LOCKTIME_DISABLE_FLAG) {
            // The height of this input is not relevant for sequence locks
            (*prevHeights)[txinIndex] = 0;
            continue;
        }

        int nCoinHeight = (*prevHeights)[txinIndex];

        if (txin.nSequence & CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG) {
            int64_t nCoinTime = block.GetAncestor(std::max(nCoinHeight-1, 0))->GetMedianTimePast();
            // NOTE: Subtract 1 to maintain nLockTime semantics
            // BIP 68 relative lock times have the semantics of calculating
            // the first block or time at which the transaction would be
            // valid. When calculating the effective block time or height
            // for the entire transaction, we switch to using the
            // semantics of nLockTime which is the last invalid block
            // time or height.  Thus we subtract 1 from the calculated
            // time or height.

            // Time-based relative lock-times are measured from the
            // smallest allowed timestamp of the block containing the
            // txout being spent, which is the median time past of the
            // block prior.
            nMinTime = std::max(nMinTime, nCoinTime + (int64_t)((txin.nSequence & CTxIn::SEQUENCE_LOCKTIME_MASK) << CTxIn::SEQUENCE_LOCKTIME_GRANULARITY) - 1);
        } else {
            nMinHeight = std::max(nMinHeight, nCoinHeight + (int)(txin.nSequence & CTxIn::SEQUENCE_LOCKTIME_MASK) - 1);
        }
    }

    return std::make_pair(nMinHeight, nMinTime);
}

bool EvaluateSequenceLocks(const CBlockIndex& block, std::pair<int, int64_t> lockPair)
{
    assert(block.pprev);
    int64_t nBlockTime = block.pprev->GetMedianTimePast();
    if (lockPair.first >= block.nHeight || lockPair.second >= nBlockTime)
        return false;

    return true;
}

bool SequenceLocks(const CTransaction &tx, int flags, std::vector<int>* prevHeights, const CBlockIndex& block)
{
    return EvaluateSequenceLocks(block, CalculateSequenceLocks(tx, flags, prevHeights, block));
}

unsigned int GetLegacySigOpCount(const CTransaction& tx)
{
    unsigned int nSigOps = 0;
    for (const auto& txin : tx.vin)
    {
        nSigOps += txin.scriptSig.GetSigOpCount(false);
    }
    for (const auto& txout : tx.vout)
    {
        nSigOps += txout.scriptPubKey.GetSigOpCount(false);
    }
    return nSigOps;
}

unsigned int GetP2SHSigOpCount(const CTransaction& tx, const CCoinsViewCache& inputs)
{
    if (tx.IsCoinBase())
        return 0;

    unsigned int nSigOps = 0;
    for (unsigned int i = 0; i < tx.vin.size(); i++)
    {
        const Coin& coin = inputs.AccessCoin(tx.vin[i].prevout);
        assert(!coin.IsSpent());
        const CTxOut &prevout = coin.out;
        if (prevout.scriptPubKey.IsPayToScriptHash())
            nSigOps += prevout.scriptPubKey.GetSigOpCount(tx.vin[i].scriptSig);
    }
    return nSigOps;
}

int64_t GetTransactionSigOpCost(const CTransaction& tx, const CCoinsViewCache& inputs, int flags)
{
    int64_t nSigOps = GetLegacySigOpCount(tx) * WITNESS_SCALE_FACTOR;

    if (tx.IsCoinBase())
        return nSigOps;

    if (flags & SCRIPT_VERIFY_P2SH) {
        nSigOps += GetP2SHSigOpCount(tx, inputs) * WITNESS_SCALE_FACTOR;
    }

    for (unsigned int i = 0; i < tx.vin.size(); i++)
    {
        const Coin& coin = inputs.AccessCoin(tx.vin[i].prevout);
        assert(!coin.IsSpent());
        const CTxOut &prevout = coin.out;
        nSigOps += CountWitnessSigOps(tx.vin[i].scriptSig, prevout.scriptPubKey, &tx.vin[i].scriptWitness, flags);
    }
    return nSigOps;
}

bool CheckTransaction(const CTransaction& tx, CValidationState &state, bool fCheckDuplicateInputs, uint64_t table[])
{
    // Basic checks that don't depend on any context
    if (tx.vin.empty())
        return state.DoS(10, false, REJECT_INVALID, "bad-txns-vin-empty");
    if (tx.vout.empty())
        return state.DoS(10, false, REJECT_INVALID, "bad-txns-vout-empty");
    // Size limits (this doesn't take the witness into account, as that hasn't been checked for malleability)
    if (::GetSerializeSize(tx, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS) * WITNESS_SCALE_FACTOR > MAX_BLOCK_WEIGHT)
        return state.DoS(100, false, REJECT_INVALID, "bad-txns-oversize");

    // Check for negative or overflow output values
    CAmount nValueOut = 0;
    for (const auto& txout : tx.vout)
    {
        if (txout.nValue < 0)
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-vout-negative");
        if (txout.nValue > MAX_MONEY)
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-vout-toolarge");
        nValueOut += txout.nValue;
        if (!MoneyRange(nValueOut))
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-txouttotal-toolarge");
    }

    // Check for duplicate inputs - note that this check is slow so we skip it in CheckBlock
    if (tx.IsCoinBase() ) {
        if (tx.vin[0].scriptSig.size() < 2 || tx.vin[0].scriptSig.size() > 100)
            return state.DoS(100, false, REJECT_INVALID, "bad-cb-length");
    } else if (!fCheckDuplicateInputs || tx.vin.size() == 1){
        for (const auto& txin : tx.vin) {
            if (txin.prevout.IsNull()) {
                return state.DoS(10, false, REJECT_INVALID, "bad-txns-prevout-null");
            }
        }
    } else{
        // This duplication checking algorithm uses a probabilistic filter
        // to check for collisions efficiently.
        //
        // This is faster than the naive construction, using a set, which
        // requires more allocation and comparison of uint256s.
        //
        // First we create a bitset table with 1<<21 elements. This
        // is around 300 KB, so we construct it on the heap.
        //
        // We also allow reusing a 'dirty' table because zeroing 300 KB
        // can be expensive, and the table will operate acceptably for all of the
        // transactions in a given block.
        //
        // Then, we iterate through the elements one by one, generated 8 salted
        // 21-bit hashes (which can be quickly generated using siphash) based on
        // each prevout.
        //
        // We then check if all 8 hashes are set in the table yet. If they are,
        // we do a linear scan through the inputs to see if it was a true
        // collision, and reject the txn.
        //
        // Otherwise, we set the 8 bits corresponding to the hashes and
        // continue.
        //
        // From the perspective of the N+1st prevout, assuming the transaction
        // does not double spend:
        //
        // Up to N*8 hashes have been set in the table already (potentially
        // fewer if collisions)
        //
        // For each of the 8 hashes h_1...h_8, P(bit set in table for h_i) =
        // (N*8)/1<<21
        //
        // Each of these probabilities is independent
        //
        // Therefore the total probability of a false collision on all bits is:
        // ((N*8)/2**21)**8
        //
        // The cost of a false collision is to do N comparisons.
        //
        // Therefore, the expression for the expected number of comparisons is:
        //
        // Sum from i = 0 to M [ i*( i*8 / 2**21)**8 ]
        //
        // Based on an input being at least 41 bytes, and a block being 1M bytes
        // max, there are a maximum of 24390 inputs, so M = 24390
        //
        // The total expected number of direct comparisons for M=24930 is
        // therefore 0.33 with this algorithm.
        //
        // As a bonus, we also get "free" null checking by manually inserting
        // the null element into the table so it always generates a conflict
        // check. We remove this null-check before terminating so that we avoid
        // doubling the bloat on the table.
        //
        // If a dirty table is used, the algorithms worst-case
        // runtime is still better because of three key reasons:
        //
        // 1) the linear searches complexity is limited to each transaction's
        // subset of inputs
        // 2) The total number of inputs in the block still does not exceed
        // 24930
        // 3) less initialization of the table
        //
        //
        // The worst case for this algorithm from a denial of service
        // perspective with an invalid transaction would be to do a transaction
        // where the last two elements are a collision. In this case, the scan
        // would require to scan all N elements.
        //
        //
        //
        // N.B. When the table is dirty, the bits set in the table
        // are meaningless because the hash was salted separately.
        //
        uint64_t k1 = GetRand(std::numeric_limits<uint64_t>::max());
        uint64_t k2 = GetRand(std::numeric_limits<uint64_t>::max());
        auto hasher = [k1, k2](const COutPoint& out){return SipHashUint256Extra192(k1, k2, out.hash, out.n);};
        // If we haven't been given a table, make one now.
        std::unique_ptr<uint64_t[]> upTable = table ? std::unique_ptr<uint64_t[]>(nullptr) :
                                                      std::unique_ptr<uint64_t[]>(new uint64_t[1<<15]());
        table = table ? table : upTable.get();
#define HASH(h, a) \
uint64_t bit1 = 1<<(std::get<0>(h) & 63);\
uint64_t bit2 = 1<<(std::get<0>(h)>>6 & 63);\
uint64_t bit3 = 1<<(std::get<0>(h)>>12 & 63);\
uint64_t bit4 = 1<<(std::get<0>(h)>>18 & 63);\
uint64_t bit5 = 1<<(std::get<0>(h)>>24 & 63);\
uint64_t bit6 = 1<<(std::get<0>(h)>>24 & 63);\
uint64_t bit7 = 1<<(std::get<0>(h)>>24 & 63);\
uint64_t bit8 = 1<<(std::get<0>(h)>>24 & 63);\
uint64_t pos1 = (std::get<1>(h)     & 0x07FFF);\
uint64_t pos2 = (std::get<1>(h)>>15 & 0x07FFF);\
uint64_t pos3 = (std::get<1>(h)>>30 & 0x07FFF);\
uint64_t pos4 = (std::get<1>(h)>>45 & 0x07FFF);\
uint64_t pos5 = (std::get<2>(h)     & 0x07FFF);\
uint64_t pos6 = (std::get<2>(h)>>15 & 0x07FFF);\
uint64_t pos7 = (std::get<2>(h)>>30 & 0x07FFF);\
uint64_t pos8 = (std::get<2>(h)>>45 & 0x07FFF);\
        a;
#define SET_BITS \
        table[pos1] |= bit1;\
        table[pos2] |= bit2;\
        table[pos3] |= bit3;\
        table[pos4] |= bit4;\
        table[pos5] |= bit5;\
        table[pos6] |= bit6;\
        table[pos7] |= bit7;\
        table[pos8] |= bit8;

#define TOGGLE_BITS \
        table[pos1] ^= bit1;\
        table[pos2] ^= bit2;\
        table[pos3] ^= bit3;\
        table[pos4] ^= bit4;\
        table[pos5] ^= bit5;\
        table[pos6] ^= bit6;\
        table[pos7] ^= bit7;\
        table[pos8] ^= bit8;
#define DONT_SET_BITS


        auto nil_hash = hasher(COutPoint{});
        HASH(nil_hash, SET_BITS);
        std::unique_ptr<void, std::function<void(void*)>>
            cleanupNilEntryGuard((void*)1, [&](void*) { TOGGLE_BITS; });
        for (auto txinit =  tx.vin.cbegin(); txinit != tx.vin.cend(); ++txinit) {
            auto hash = hasher(txinit->prevout);
            HASH(hash, DONT_SET_BITS)
            if (
            (table[pos1] & bit1) &&
            (table[pos2] & bit2) &&
            (table[pos3] & bit3) &&
            (table[pos4] & bit4) &&
            (table[pos5] & bit5) &&
            (table[pos6] & bit6) &&
            (table[pos7] & bit7) &&
            (table[pos8] & bit8)) {
                if (txinit->prevout.IsNull()) {
                    return state.DoS(10, false, REJECT_INVALID, "bad-txns-prevout-null");
                }
                // If we have a potential collision, then scan through the set up to here for the colliding element
                auto elem = std::find_if(tx.vin.begin(), txinit, [txinit](CTxIn x){return x.prevout == txinit->prevout;});
                // If the iterator outputs anything except for txinit, then we have found a conflict
                if  (elem != txinit) {
                    return state.DoS(100, false, REJECT_INVALID, "bad-txns-inputs-duplicate");
                }
            } else {
                SET_BITS;
            }
        }
    }

    return true;
}

bool Consensus::CheckTxInputs(const CTransaction& tx, CValidationState& state, const CCoinsViewCache& inputs, int nSpendHeight, CAmount& txfee)
{
    // are the actual inputs available?
    if (!inputs.HaveInputs(tx)) {
        return state.DoS(100, false, REJECT_INVALID, "bad-txns-inputs-missingorspent", false,
                         strprintf("%s: inputs missing/spent", __func__));
    }

    CAmount nValueIn = 0;
    for (unsigned int i = 0; i < tx.vin.size(); ++i) {
        const COutPoint &prevout = tx.vin[i].prevout;
        const Coin& coin = inputs.AccessCoin(prevout);
        assert(!coin.IsSpent());

        // If prev is coinbase, check that it's matured
        if (coin.IsCoinBase() && nSpendHeight - coin.nHeight < COINBASE_MATURITY) {
            return state.Invalid(false,
                REJECT_INVALID, "bad-txns-premature-spend-of-coinbase",
                strprintf("tried to spend coinbase at depth %d", nSpendHeight - coin.nHeight));
        }

        // Check for negative or overflow input values
        nValueIn += coin.out.nValue;
        if (!MoneyRange(coin.out.nValue) || !MoneyRange(nValueIn)) {
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-inputvalues-outofrange");
        }
    }

    const CAmount value_out = tx.GetValueOut();
    if (nValueIn < value_out) {
        return state.DoS(100, false, REJECT_INVALID, "bad-txns-in-belowout", false,
            strprintf("value in (%s) < value out (%s)", FormatMoney(nValueIn), FormatMoney(value_out)));
    }

    // Tally transaction fees
    const CAmount txfee_aux = nValueIn - value_out;
    if (!MoneyRange(txfee_aux)) {
        return state.DoS(100, false, REJECT_INVALID, "bad-txns-fee-outofrange");
    }

    txfee = txfee_aux;
    return true;
}
