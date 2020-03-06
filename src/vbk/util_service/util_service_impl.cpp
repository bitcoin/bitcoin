#include "util_service_impl.hpp"

#include <chain.h>
#include <consensus/params.h>
#include <consensus/validation.h>
#include <primitives/block.h>
#include <script/interpreter.h>
#include <script/sigcache.h>
#include <validation.h>

#include <vbk/interpreter.hpp>
#include <vbk/pop_service.hpp>
#include <vbk/service_locator.hpp>
#include <vbk/util.hpp>

namespace VeriBlock {


bool UtilServiceImpl::CheckPopInputs(const CTransaction& tx, TxValidationState& state, unsigned int flags, bool cacheSigStore, PrecomputedTransactionData& txdata)
{
    ScriptError error(SCRIPT_ERR_UNKNOWN_ERROR);

    auto checker = CachingTransactionSignatureChecker(&tx, 0, 0, cacheSigStore, txdata);
    if (!VerifyScript(tx.vin[0].scriptSig, tx.vout[0].scriptPubKey, &tx.vin[0].scriptWitness, flags | SCRIPT_VERIFY_POP, checker, &error))
        return state.Invalid(TxValidationResult::TX_NOT_STANDARD, strprintf("invalid pop tx script (%s)", ScriptErrorString(error)));

    return true;
}

bool UtilServiceImpl::isKeystone(const CBlockIndex& block)
{
    return (block.nHeight % getService<Config>().keystone_interval) == 0;
}

const CBlockIndex* UtilServiceImpl::getPreviousKeystone(const CBlockIndex& block)
{
    const CBlockIndex* pblockWalk = &block;
    do {
        pblockWalk = pblockWalk->pprev;
    } while (pblockWalk != nullptr && !this->isKeystone(*pblockWalk));

    return pblockWalk;
}

int UtilServiceImpl::compareForks(const CBlockIndex& left, const CBlockIndex& right)
{
    const CBlockIndex* commonKeystone = FindCommonKeystone(&left, &right);

    assert(commonKeystone != nullptr);

    if (IsCrossedKeystoneBoundary(*commonKeystone, left) && IsCrossedKeystoneBoundary(*commonKeystone, right)) {
        auto& pop_service = VeriBlock::getService<VeriBlock::PopService>();
        return pop_service.compareTwoBranches(commonKeystone, &left, &right);
    }

    return 0;
}

KeystoneArray UtilServiceImpl::getKeystoneHashesForTheNextBlock(const CBlockIndex* pindexPrev)
{
    const CBlockIndex* pwalk = pindexPrev;

    KeystoneArray keystones;
    auto it = keystones.begin();
    auto end = keystones.end();
    while (it != end) {
        if (pwalk == nullptr) {
            break;
        }

        if (isKeystone(*pwalk)) {
            *it = pwalk->GetBlockHash();
            ++it;
        }

        pwalk = getPreviousKeystone(*pwalk);
    }
    return keystones;
}

const CBlockIndex* UtilServiceImpl::FindCommonKeystone(const CBlockIndex* leftFork, const CBlockIndex* rightFork)
{
    const CBlockIndex* workingLeft = leftFork;
    const CBlockIndex* workingRight = rightFork;

    while (workingLeft && workingRight) {
        if (workingLeft == workingRight && isKeystone(*workingLeft)) {
            break;
        } else if (workingLeft->nHeight > workingRight->nHeight) {
            workingLeft = workingLeft->pprev;
        } else if (workingRight->nHeight > workingLeft->nHeight) {
            workingRight = workingRight->pprev;
        } else if (workingRight->nHeight == workingLeft->nHeight) {
            workingRight = getPreviousKeystone(*workingRight);
            workingLeft = getPreviousKeystone(*workingLeft);
        }
    }

    if (workingLeft == nullptr || workingRight == nullptr) {
        return nullptr;
    }

    return workingRight;
}

bool UtilServiceImpl::IsCrossedKeystoneBoundary(const CBlockIndex& bottom, const CBlockIndex& tip)
{
    uint32_t keystone_interval = VeriBlock::getService<VeriBlock::Config>().keystone_interval;

    int32_t keystoneIntervalAmount = bottom.nHeight / keystone_interval;
    int32_t tipIntervalAmount = tip.nHeight / keystone_interval;

    return keystoneIntervalAmount < tipIntervalAmount;
}

uint256 UtilServiceImpl::makeTopLevelRoot(int height, const KeystoneArray& keystones, const uint256& txRoot)
{
    ContextInfoContainer context(height, keystones, txRoot);
    auto contextHash = context.getUnauthenticatedHash();
    return Hash(txRoot.begin(), txRoot.end(), contextHash.begin(), contextHash.end());
}

void UtilServiceImpl::addPopPayoutsIntoCoinbaseTx(CMutableTransaction& coinbaseTx, const CBlockIndex& pindexPrev, const Consensus::Params& consensusParams)
{
    PoPRewards rewards = getPopRewards(pindexPrev, consensusParams);
    assert(coinbaseTx.vout.size() == 1 && "at this place we should have only PoW payout here");
    for (const auto& itr : rewards) {
        CTxOut out;
        out.scriptPubKey = itr.first;
        out.nValue = itr.second;
        coinbaseTx.vout.push_back(out);
    }
}

bool UtilServiceImpl::checkCoinbaseTxWithPopRewards(const CTransaction& tx, const CAmount& PoWBlockReward, const CBlockIndex& pindexPrev, const Consensus::Params& consensusParams, BlockValidationState& state)
{
    PoPRewards rewards = getPopRewards(pindexPrev, consensusParams);
    CAmount nTotalPopReward = 0;

    if (tx.vout.size() < rewards.size()) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-pop-vouts-size",
            strprintf("checkCoinbaseTxWithPopRewards(): coinbase has incorrect size of pop vouts (actual vouts size=%d vs expected vouts=%d)", tx.vout.size(), rewards.size()));
    }

    std::map<CScript, CAmount> cbpayouts;
    // skip first reward, as it is always PoW payout
    for (auto out = tx.vout.begin() + 1, end = tx.vout.end(); out != end; ++out) {
        // pop payouts can not be null
        if (out->IsNull()) {
            continue;
        }
        cbpayouts[out->scriptPubKey] += out->nValue;
    }

    // skip first (regular pow) payout, and last 2 0-value payouts
    for (const auto& payout : rewards) {
        auto& script = payout.first;
        auto& expectedAmount = payout.second;

        auto p = cbpayouts.find(script);
        // coinbase pays correct reward?
        if (p == cbpayouts.end()) {
            // we expected payout for that address
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-pop-missing-payout",
                strprintf("[tx: %s] missing payout for scriptPubKey: '%s' with amount: '%d'",
                    tx.GetHash().ToString(),
                    HexStr(script),
                    expectedAmount));
        }

        // payout found
        auto& actualAmount = p->second;
        // does it have correct amount?
        if (actualAmount != expectedAmount) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-pop-wrong-payout",
                strprintf("[tx: %s] wrong payout for scriptPubKey: '%s'. Expected %d, got %d.",
                    tx.GetHash().ToString(),
                    HexStr(script),
                    expectedAmount, actualAmount));
        }

        nTotalPopReward += expectedAmount;
    }

    if (tx.GetValueOut() > nTotalPopReward + PoWBlockReward) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS,
            "bad-cb-pop-amount",
            strprintf("ConnectBlock(): coinbase pays too much (actual=%d vs limit=%d)", tx.GetValueOut(), PoWBlockReward + nTotalPopReward));
    }

    return true;
}

PoPRewards UtilServiceImpl::getPopRewards(const CBlockIndex& pindexPrev, const Consensus::Params& consensusParams)
{
    int halvings = (pindexPrev.nHeight + 1) / consensusParams.nSubsidyHalvingInterval;

    PoPRewards rewards;
    auto& config = getService<Config>();
    auto& pop_service = VeriBlock::getService<VeriBlock::PopService>();

    int nextBlockHeight = pindexPrev.nHeight + 1;
    if (nextBlockHeight > config.POP_REWARD_PAYMENT_DELAY) {
        int32_t checkHeight = nextBlockHeight - config.POP_REWARD_PAYMENT_DELAY;
        const CBlockIndex* endorsedBlock = pindexPrev.GetAncestor(checkHeight);
        const CBlockIndex* contaningBlocksTip = pindexPrev.GetAncestor(checkHeight + config.POP_REWARD_SETTLEMENT_INTERVAL);
        const CBlockIndex* difficultyBlockStart = pindexPrev.GetAncestor(checkHeight - config.POP_DIFFICULTY_AVERAGING_INTERVAL);
        const CBlockIndex* difficultyBlockEnd = pindexPrev.GetAncestor(checkHeight + config.POP_REWARD_SETTLEMENT_INTERVAL - 1);

        if (!endorsedBlock) {
            return rewards;
        }

        pop_service.rewardsCalculateOutputs(checkHeight, *endorsedBlock, *contaningBlocksTip, difficultyBlockStart, difficultyBlockEnd, rewards);
    }

    // erase rewards, that pay 0 satoshis and halve rewards
    for (auto it = rewards.begin(), end = rewards.end(); it != end;) {
        it->second >>= halvings;
        if (it->second == 0 || halvings >= 64) {
            it = rewards.erase(it);
        } else {
            ++it;
        }
    }

    return rewards;
}

bool UtilServiceImpl::validatePopTx(const CTransaction& tx, TxValidationState& state)
{
    // validate input
    {
        if (tx.vin.size() != 1) {
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-pop-vin-not-single", strprintf("Expected 1 input, got %d", tx.vin.size()));
        }
        if (!validatePopTxInput(tx.vin[0], state)) {
            return false; // reason already set by ValidatePopTxInput
        }
    }

    // validate output
    {
        if (tx.vout.size() != 1) {
            return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-pop-vout-not-single", strprintf("Expected 1 output, got %d", tx.vout.size()));
        }
        if (!validatePopTxOutput(tx.vout[0], state)) {
            return false; // reason already set by ValidatePopTxOutput
        }
    }

    // check size
    auto& config = getService<Config>();
    if (::GetSerializeSize(tx, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS) > config.max_pop_tx_weight) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-pop-txns-oversize");
    }

    return true;
}

bool UtilServiceImpl::validatePopTxInput(const CTxIn& in, TxValidationState& state)
{
    if (!isVBKNoInput(in.prevout)) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-pop-prevout");
    }
    return true;
}

bool UtilServiceImpl::validatePopTxOutput(const CTxOut& out, TxValidationState& state)
{
    if (out.scriptPubKey.size() != 1) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-pop-out-scriptpubkey-not-single");
    }
    if (out.scriptPubKey[0] != OP_RETURN) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-pop-out-scriptpubkey-unexpected");
    }
    return true;
}


bool UtilServiceImpl::EvalScript(const CScript& script, std::vector<std::vector<unsigned char>>& stack, ScriptError* serror, VeriBlock::Publications* publications, VeriBlock::Context* context, PopTxType* type, bool with_checks)
{
    return EvalScriptImpl(script, stack, serror, publications, context, type, with_checks);
}

} // namespace VeriBlock
