// VeriBlock Blockchain Project
// Copyright 2017-2018 VeriBlock, Inc
// Copyright 2018-2019 Xenios SEZC
// All rights reserved.
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
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

void PopServiceImpl::addPopPayoutsIntoCoinbaseTx(CMutableTransaction& coinbaseTx, const CBlockIndex& pindexPrev, const Consensus::Params& consensusParams)
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

bool PopServiceImpl::checkCoinbaseTxWithPopRewards(const CTransaction& tx, const CAmount& PoWBlockReward, const CBlockIndex& pindexPrev, const Consensus::Params& consensusParams, BlockValidationState& state)
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

PoPRewards PopServiceImpl::getPopRewards(const CBlockIndex& pindexPrev, const Consensus::Params& consensusParams)
{
    // TODO: implement
    return {};
    //    int halvings = (pindexPrev.nHeight + 1) / consensusParams.nSubsidyHalvingInterval;
    //
    //    PoPRewards rewards;
    //    auto& config = getService<Config>();
    //    auto& pop_service = VeriBlock::getService<VeriBlock::PopService>();
    //
    //    int nextBlockHeight = pindexPrev.nHeight + 1;
    //    if (nextBlockHeight > config.POP_REWARD_PAYMENT_DELAY) {
    //        int32_t checkHeight = nextBlockHeight - config.POP_REWARD_PAYMENT_DELAY;
    //        const CBlockIndex* endorsedBlock = pindexPrev.GetAncestor(checkHeight);
    //        const CBlockIndex* contaningBlocksTip = pindexPrev.GetAncestor(checkHeight + config.POP_REWARD_SETTLEMENT_INTERVAL);
    //        const CBlockIndex* difficultyBlockStart = pindexPrev.GetAncestor(checkHeight - config.POP_DIFFICULTY_AVERAGING_INTERVAL);
    //        const CBlockIndex* difficultyBlockEnd = pindexPrev.GetAncestor(checkHeight + config.POP_REWARD_SETTLEMENT_INTERVAL - 1);
    //
    //        if (!endorsedBlock) {
    //            return rewards;
    //        }
    //
    //        pop_service.rewardsCalculateOutputs(checkHeight, *endorsedBlock, *contaningBlocksTip, difficultyBlockStart, difficultyBlockEnd, rewards);
    //    }
    //
    //    // erase rewards, that pay 0 satoshis and halve rewards
    //    for (auto it = rewards.begin(), end = rewards.end(); it != end;) {
    //        it->second >>= halvings;
    //        if (it->second == 0 || halvings >= 64) {
    //            it = rewards.erase(it);
    //        } else {
    //            ++it;
    //        }
    //    }
    //
    //    return rewards;
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

    // check size
    auto& config = getService<Config>();
    if (::GetSerializeSize(tx, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS) > config.max_pop_tx_weight) {
        return state.Invalid(TxValidationResult::TX_CONSENSUS, "bad-pop-txns-oversize");
    }

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

bool PopServiceImpl::acceptBlock(const CBlockIndex& indexNew, BlockValidationState& state)
{
    std::lock_guard<std::mutex> lock(mutex);
    auto containing = VeriBlock::blockToAltBlock(indexNew);
    altintegration::ValidationState instate;
    if (!altTree->acceptBlock(containing, instate)) {
        return state.Error(instate.GetDebugMessage());
    }

    return true;
}

bool PopServiceImpl::checkPopPayloads(const CBlockIndex& indexPrev, const CBlock& fullBlock, BlockValidationState& state)
{
    // does not modify internal state, so no locking required
    altintegration::AltTree copy = *altTree;
    return addAllPayloadsToBlockImpl(copy, indexPrev, fullBlock, state, true, true);
}

bool PopServiceImpl::addAllBlockPayloads(const CBlockIndex& indexPrev, const CBlock& connecting, BlockValidationState& state)
{
    std::lock_guard<std::mutex> lock(mutex);
    return addAllPayloadsToBlockImpl(*altTree, indexPrev, connecting, state, true, false);
}

bool PopServiceImpl::evalScript(const CScript& script, std::vector<std::vector<unsigned char>>& stack, ScriptError* serror, altintegration::AltPayloads* pub, altintegration::ValidationState& state, bool with_checks)
{
    return evalScriptImpl(script, stack, serror, pub, state, with_checks);
}

std::vector<BlockBytes> PopServiceImpl::getLastKnownVBKBlocks(size_t blocks)
{
    std::lock_guard<std::mutex> lock(mutex);
    return altintegration::getLastKnownBlocks(altTree->vbk(), blocks);
}

std::vector<BlockBytes> PopServiceImpl::getLastKnownBTCBlocks(size_t blocks)
{
    std::lock_guard<std::mutex> lock(mutex);
    return altintegration::getLastKnownBlocks(altTree->btc(), blocks);
}

// Forkresolution
int PopServiceImpl::compareForks(const CBlockIndex& leftForkTip, const CBlockIndex& rightForkTip)
{
    if (&leftForkTip == &rightForkTip) {
        return 0;
    }

    std::lock_guard<std::mutex> lock(mutex);

    auto left = blockToAltBlock(leftForkTip);
    auto right = blockToAltBlock(leftForkTip);
    auto state = altintegration::ValidationState();
    if (!altTree->acceptBlock(left, state)) {
        throw std::logic_error("compareForks: left fork tip can not be connected to altTree");
    }
    if (!altTree->acceptBlock(right, state)) {
        throw std::logic_error("compareForks: right fork tip can not be connected to altTree");
    }

    return altTree->compareTwoBranches(left.hash, right.hash);
}

// Pop rewards
void PopServiceImpl::rewardsCalculateOutputs(const int& blockHeight, const CBlockIndex& endorsedBlock, const CBlockIndex& contaningBlocksTip, const CBlockIndex* difficulty_start_interval, const CBlockIndex* difficulty_end_interval, std::map<CScript, int64_t>& outputs)
{
    std::lock_guard<std::mutex> lock(mutex);
    // TODO: implement
}

PopServiceImpl::PopServiceImpl(const altintegration::Config& config)
{
    config.validate();

    auto tree = altintegration::Altintegration::create(config);
    altTree = std::make_shared<altintegration::AltTree>(std::move(tree));
}

void PopServiceImpl::invalidateBlockByHash(const uint256& block)
{
    std::lock_guard<std::mutex> lock(mutex);
    auto v = block.asVector();
    altTree->invalidateBlockByHash(v);
}

bool PopServiceImpl::setState(const uint256& block)
{
    std::lock_guard<std::mutex> lock(mutex);
    altintegration::ValidationState state;
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
    altintegration::AltPayloads publication;

    if (script.size() > config.max_pop_script_size) {
        return set_error(serror, SCRIPT_ERR_SCRIPT_SIZE);
    }

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
                if (publication.hasAtv) {
                    return set_error(serror, SCRIPT_ERR_INVALID_STACK_OPERATION);
                }

                // validate ATV size
                const auto& el = stacktop(-1);
                if (el.size() > config.max_atv_size || el.size() < config.min_atv_size) {
                    return set_error(serror, SCRIPT_ERR_PUSH_SIZE);
                }

                // validate ATV content
                try {
                    publication.atv = altintegration::ATV::fromVbkEncoding(el);
                    publication.hasAtv = true;
                    if (with_checks && !altintegration::checkATV(publication.atv, state, altp, vbkp)) {
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
                    publication.vtbs.push_back(std::move(vtb));
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
        *pub = publication;
    }

    return true;
}

bool parseBlockPopPayloadsImpl(const CBlock& block, const CBlockIndex& pindexPrev, const Consensus::Params& params, BlockValidationState& state, std::vector<altintegration::AltPayloads>* payloads) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
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

    const altintegration::PublicationData& publicationData = payloads.atv.transaction.publicationData;

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

    payloads.containingTx = altintegration::uint256(txhash.asVector());
    payloads.endorsed = blockToAltBlock(*endorsedIndex);

    return true;
}

bool addAllPayloadsToBlockImpl(altintegration::AltTree& tree, const CBlockIndex& indexPrev, const CBlock& block, BlockValidationState& state, bool atomic, bool cleanupAfter) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    auto containing = VeriBlock::blockToAltBlock(indexPrev.nHeight + 1, block.GetBlockHeader());

    altintegration::ValidationState instate;
    std::vector<altintegration::AltPayloads> payloads;
    if (!parseBlockPopPayloadsImpl(block, indexPrev, Params().GetConsensus(), state, &payloads)) {
        return error("[%s] block %s failed stateless validation: %s", __func__, block.GetHash().ToString(), instate.toString());
    }

    if (!tree.acceptBlock(containing, instate)) {
        return error("[%s] block %s is not accepted by altTree: %s", __func__, block.GetHash().ToString(), instate.toString());
    }

    if (!payloads.empty() && !tree.addPayloads(containing, payloads, instate, atomic)) {
        return error("[%s] block %s failed stateful pop validation: %s", __func__, block.GetHash().ToString(), instate.toString());
    }

    if (cleanupAfter) {
        tree.removePayloads(containing, payloads);
    }

    return true;
}

} // namespace VeriBlock
