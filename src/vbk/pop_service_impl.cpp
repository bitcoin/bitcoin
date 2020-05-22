// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <vbk/pop_service_impl.hpp>

#include <chrono>
#include <memory>
#include <thread>

#include <amount.h>
#include <chain.h>
#include <consensus/validation.h>
#include <pow.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/interpreter.h>
#include <script/sigcache.h>
#include <shutdown.h>
#include <streams.h>
#include <util/strencodings.h>
#include <validation.h>

#include <vbk/service_locator.hpp>
#include <vbk/util.hpp>

#include <veriblock/alt-util.hpp>
#include <veriblock/altintegration.hpp>
#include <veriblock/finalizer.hpp>
#include <veriblock/stateless_validation.hpp>
#include <veriblock/validation_state.hpp>

namespace {

bool set_error(ScriptError* ret, const ScriptError serror)
{
    if (ret)
        *ret = serror;
    return false;
}
typedef std::vector<unsigned char> valtype;

#define stacktop(i) (stack.at(stack.size() + (i)))
void popstack(std::vector<valtype>& stack)
{
    if (stack.empty())
        throw std::runtime_error("popstack(): stack empty");
    stack.pop_back();
}

} // namespace


namespace VeriBlock {

void PopServiceImpl::addPopPayoutsIntoCoinbaseTx(CMutableTransaction& coinbaseTx, const CBlockIndex& pindexPrev, const Consensus::Params& consensusParams) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    PoPRewards rewards = getPopRewards(pindexPrev, consensusParams);
    assert(coinbaseTx.vout.size() == 1 && "at this place we should have only PoW payout here");
    for (const auto& itr : rewards) {
        CTxOut out;
        out.scriptPubKey = itr.first;
        out.nValue = itr.second;
        coinbaseTx.vout.push_back(out);
    }
}

bool PopServiceImpl::checkCoinbaseTxWithPopRewards(const CTransaction& tx, const CAmount& PoWBlockReward, const CBlockIndex& pindexPrev, const Consensus::Params& consensusParams, BlockValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
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

PoPRewards PopServiceImpl::getPopRewards(const CBlockIndex& pindexPrev, const Consensus::Params& consensusParams) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    auto& config = getService<Config>();
    if ((pindexPrev.nHeight + 1) < (int)config.popconfig.alt->getEndorsementSettlementInterval()) {
        return {};
    }
    auto state = altintegration::ValidationState();
    auto blockHash = pindexPrev.GetBlockHash();
    auto rewards = altTree->getPopPayout(blockHash.asVector(), state);
    if (state.IsError()) {
        throw std::logic_error(state.GetDebugMessage());
    }

    int halvings = (pindexPrev.nHeight + 1) / consensusParams.nSubsidyHalvingInterval;
    PoPRewards btcRewards{};
    //erase rewards, that pay 0 satoshis and halve rewards
    for (const auto& r : rewards) {
        auto rewardValue = r.second;
        rewardValue >>= halvings;
        if ((rewardValue != 0) && (halvings < 64)) {
            CScript key = CScript(r.first.begin(), r.first.end());
            btcRewards[key] = config.POP_REWARD_COEFFICIENT * rewardValue;
        }
    }

    return btcRewards;
}

bool PopServiceImpl::validatePopTx(const CTransaction& tx, TxValidationState& state)
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

    //    // check size
    //    auto& config = getService<Config>();
    //    if (::GetSerializeSize(tx, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS) > config.max_pop_tx_weight) {
    //        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-pop-txns-oversize");
    //    }

    return true;
}

bool PopServiceImpl::validatePopTxInput(const CTxIn& in, TxValidationState& state)
{
    if (!isVBKNoInput(in.prevout)) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-pop-prevout");
    }
    return true;
}

bool PopServiceImpl::validatePopTxOutput(const CTxOut& out, TxValidationState& state)
{
    if (out.scriptPubKey.size() != 1) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-pop-out-scriptpubkey-not-single");
    }
    if (out.scriptPubKey[0] != OP_RETURN) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-pop-out-scriptpubkey-unexpected");
    }
    return true;
}

bool PopServiceImpl::checkPopInputs(const CTransaction& tx, TxValidationState& state, unsigned int flags, bool cacheSigStore, PrecomputedTransactionData& txdata)
{
    ScriptError error(SCRIPT_ERR_UNKNOWN_ERROR);

    auto checker = CachingTransactionSignatureChecker(&tx, 0, 0, cacheSigStore, txdata);
    if (!VerifyScript(tx.vin[0].scriptSig, tx.vout[0].scriptPubKey, &tx.vin[0].scriptWitness, flags | SCRIPT_VERIFY_POP, checker, &error))
        return state.Invalid(TxValidationResult::TX_NOT_STANDARD, strprintf("invalid pop tx script (%s)", ScriptErrorString(error)));

    return true;
}

bool PopServiceImpl::acceptBlock(const CBlockIndex& indexNew, BlockValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    auto containing = VeriBlock::blockToAltBlock(indexNew);
    altintegration::ValidationState instate;
    if (!altTree->acceptBlock(containing, instate)) {
        return state.Error(instate.GetDebugMessage());
    }

    return true;
}

bool PopServiceImpl::addAllBlockPayloads(const CBlockIndex& indexPrev, const CBlock& connecting, BlockValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    return addAllPayloadsToBlockImpl(*altTree, indexPrev, connecting, state);
}

bool PopServiceImpl::evalScript(const CScript& script, std::vector<std::vector<unsigned char>>& stack, ScriptError* serror, altintegration::AltPayloads* pub, altintegration::ValidationState& state, bool with_checks)
{
    return evalScriptImpl(script, stack, serror, pub, state, with_checks);
}

std::vector<BlockBytes> PopServiceImpl::getLastKnownVBKBlocks(size_t blocks)
{
    LOCK(cs_main);
    return altintegration::getLastKnownBlocks(altTree->vbk(), blocks);
}

std::vector<BlockBytes> PopServiceImpl::getLastKnownBTCBlocks(size_t blocks)
{
    LOCK(cs_main);
    return altintegration::getLastKnownBlocks(altTree->btc(), blocks);
}

// Forkresolution
int PopServiceImpl::compareForks(const CBlockIndex& leftForkTip, const CBlockIndex& rightForkTip) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    if (&leftForkTip == &rightForkTip) {
        return 0;
    }

    auto left = blockToAltBlock(leftForkTip);
    auto right = blockToAltBlock(rightForkTip);
    auto state = altintegration::ValidationState();
    return altTree->comparePopScore(left.hash, right.hash);
}

PopServiceImpl::PopServiceImpl(const altintegration::Config& config)
{
    config.validate();
    altTree = altintegration::Altintegration::create(config);
}

void PopServiceImpl::invalidateBlockByHash(const uint256& block) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    auto v = block.asVector();
    altTree->removeSubtree(v);
}

bool PopServiceImpl::setState(const uint256& block, altintegration::ValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    return altTree->setState(block.asVector(), state);
}

bool evalScriptImpl(const CScript& script, std::vector<std::vector<unsigned char>>& stack, ScriptError* serror, altintegration::AltPayloads* pub, altintegration::ValidationState& state, bool with_checks)
{
    CScript::const_iterator pc = script.begin();
    CScript::const_iterator pend = script.end();
    static const valtype vchFalse(0);
    static const valtype vchTrue(1, 1);
    opcodetype opcode;
    std::vector<unsigned char> vchPushValue;
    auto& config = getService<Config>();
    auto& altp = *config.popconfig.alt;
    auto& vbkp = *config.popconfig.vbk.params;
    auto& btcp = *config.popconfig.btc.params;
    altintegration::PopData popdata;

    try {
        while (pc < pend) {
            if (!script.GetOp(pc, opcode, vchPushValue)) {
                return set_error(serror, SCRIPT_ERR_BAD_OPCODE);
            }

            if (!vchPushValue.empty()) {
                stack.push_back(vchPushValue);
                continue;
            }

            switch (opcode) {
            case OP_CHECKATV: {
                // tx has zero or one ATV
                if (popdata.hasAtv) {
                    return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                }

                // validate ATV size
                const auto& el = stacktop(-1);
                if (el.size() > config.max_atv_size || el.size() < config.min_atv_size) {
                    return set_error(serror, SCRIPT_ERR_PUSH_SIZE);
                }

                // validate ATV content
                try {
                    popdata.atv = altintegration::ATV::fromVbkEncoding(el);
                    popdata.hasAtv = true;
                    if (with_checks && !altintegration::checkATV(popdata.atv, state, altp, vbkp)) {
                        return set_error(serror, SCRIPT_ERR_VBK_ATVFAIL);
                    }
                } catch (...) {
                    return set_error(serror, SCRIPT_ERR_VBK_ATVFAIL);
                }

                popstack(stack);
                break;
            }
            case OP_CHECKVTB: {
                // validate VTB size
                const auto& el = stacktop(-1);
                if (el.size() > config.max_vtb_size || el.size() < config.min_vtb_size) {
                    return set_error(serror, SCRIPT_ERR_PUSH_SIZE);
                }

                try {
                    auto vtb = altintegration::VTB::fromVbkEncoding(el);
                    if (with_checks && !altintegration::checkVTB(vtb, state, vbkp, btcp)) {
                        return set_error(serror, SCRIPT_ERR_VBK_VTBFAIL);
                    }
                    popdata.vtbs.push_back(std::move(vtb));
                } catch (...) {
                    return set_error(serror, SCRIPT_ERR_VBK_VTBFAIL);
                }

                popstack(stack);
                break;
            }
            case OP_CHECKPOP: {
                // OP_CHECKPOP should be last opcode
                if (script.GetOp(pc, opcode, vchPushValue)) {
                    // we could read next opcode. extra opcodes is an error
                    return set_error(serror, SCRIPT_ERR_VBK_EXTRA_OPCODE);
                }

                // stack should be empty at this point
                if (!stack.empty()) {
                    return set_error(serror, SCRIPT_ERR_EVAL_FALSE);
                }

                // we did all checks, put true on stack to signal successful execution
                stack.push_back(vchTrue);
                break;
            }
            default:
                // forbid any other opcodes in pop transactions
                return set_error(serror, SCRIPT_ERR_BAD_OPCODE);
            }
        }
    } catch (...) {
        return set_error(serror, SCRIPT_ERR_UNKNOWN_ERROR);
    }

    // set return value of publications
    if (pub != nullptr) {
        pub->popData = popdata;
    }

    return true;
}

bool parseBlockPopPayloadsImpl(const CBlock& block, const CBlockIndex& pindexPrev, const Consensus::Params& params, BlockValidationState& state, std::vector<altintegration::AltPayloads>* payloads)
{
    AssertLockHeld(cs_main);
    const auto& config = getService<Config>();
    size_t numOfPopTxes = 0;

    for (const auto& tx : block.vtx) {
        if (!isPopTx(*tx)) {
            // do not even consider regular txes here
            continue;
        }

        if (++numOfPopTxes > config.max_pop_tx_amount) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "pop-block-num-pop-tx", "too many pop transactions in a block");
        }

        TxValidationState txstate;
        altintegration::AltPayloads p;
        if (!parseTxPopPayloadsImpl(*tx, params, txstate, p)) {
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, txstate.GetRejectReason(), txstate.GetDebugMessage());
        }
        p.containingBlock = blockToAltBlock(pindexPrev.nHeight + 1, block.GetBlockHeader());

        if (payloads) {
            payloads->push_back(std::move(p));
        }
    }

    return true;
}

bool parseTxPopPayloadsImpl(const CTransaction& tx, const Consensus::Params& params, TxValidationState& state, altintegration::AltPayloads& payloads) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    // do only validation of endorsed block, as all other stateful validations will be performed inside addPayloads
    altintegration::ValidationState instate;
    auto txhash = tx.GetHash();
    ScriptError serror = ScriptError::SCRIPT_ERR_UNKNOWN_ERROR;
    std::vector<std::vector<uint8_t>> stack;

    // parse transaction
    if (!evalScriptImpl(tx.vin[0].scriptSig, stack, &serror, &payloads, instate, true)) {
        return state.Invalid(
            TxValidationResult::TX_BAD_POP_DATA,
            "pop-tx-invalid-script",
            "[" + txhash.ToString() + "] scriptSig of POP tx is invalid: " + ScriptErrorString(serror) + ", " + instate.GetPath() + ", " + instate.GetDebugMessage());
    }

    const altintegration::PublicationData& publicationData = payloads.popData.atv.transaction.publicationData;

    CBlockHeader endorsedHeader;

    // parse endorsed header
    try {
        endorsedHeader = headerFromBytes(publicationData.header);
    } catch (const std::exception& e) {
        return state.Invalid(TxValidationResult::TX_BAD_POP_DATA, "pop-tx-alt-block-invalid", "[" + txhash.ToString() + "] can't deserialize endorsed block header: " + e.what());
    }

    // set endorsed header
    AssertLockHeld(cs_main);
    CBlockIndex* endorsedIndex = LookupBlockIndex(endorsedHeader.GetHash());
    if (!endorsedIndex) {
        return state.Invalid(
            TxValidationResult::TX_BAD_POP_DATA,
            "pop-tx-endorsed-block-missing",
            "[ " + txhash.ToString() + "]: endorsed block " + endorsedHeader.GetHash().ToString() + " is missing");
    }

    payloads.endorsed = blockToAltBlock(*endorsedIndex);

    // HACK: fill vbk_context here. Remove when mempool is added.
    {
        auto& p = payloads;
        // extract VBK context
        auto cmp = [](const altintegration::VbkBlock& a, const altintegration::VbkBlock& b) {
            return a.height < b.height;
        };
        auto& v = p.popData.vbk_context;
        for (auto& vtb : p.popData.vtbs) {
            for (auto& ctx : vtb.context) {
                v.push_back(ctx);
            }
            v.push_back(vtb.containingBlock);
            vtb.context.clear();
        }
        if (p.popData.hasAtv) {
            for (auto& ctx : p.popData.atv.context) {
                v.push_back(ctx);
            }
            v.push_back(p.popData.atv.containingBlock);
            p.popData.atv.context.clear();
        }
        std::sort(v.begin(), v.end(), cmp);
        v.erase(std::unique(v.begin(), v.end()), v.end());
    }

    return true;
}

bool addAllPayloadsToBlockImpl(altintegration::AltTree& tree, const CBlockIndex& indexPrev, const CBlock& block, BlockValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    auto containing = VeriBlock::blockToAltBlock(indexPrev.nHeight + 1, block.GetBlockHeader());

    altintegration::ValidationState instate;
    std::vector<altintegration::AltPayloads> payloads;
    if (!parseBlockPopPayloadsImpl(block, indexPrev, Params().GetConsensus(), state, &payloads)) {
        return error("[%s] block %s failed stateless validation: %s", __func__, block.GetHash().ToString(), instate.toString());
    }

    if (!tree.acceptBlock(containing, instate)) {
        return error("[%s] block %s is not accepted by altTree: %s", __func__, block.GetHash().ToString(), instate.toString());
    }

    if (!payloads.empty() && !tree.addPayloads(containing, payloads, instate)) {
        return error("[%s] block %s failed stateful pop validation: %s", __func__, block.GetHash().ToString(), instate.toString());
    }

    return true;
}

} // namespace VeriBlock
