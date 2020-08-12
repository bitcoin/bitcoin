// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <vbk/pop_service_impl.hpp>

#include <chrono>
#include <memory>

#include <amount.h>
#include <chain.h>
#include <consensus/validation.h>
#include <pow.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <util/strencodings.h>
#include <validation.h>

#ifdef WIN32
#include <boost/thread/interruption.hpp>
#endif //WIN32

#include <vbk/adaptors/batch_adapter.hpp>
#include <vbk/p2p_sync.hpp>
#include <vbk/service_locator.hpp>
#include <vbk/util.hpp>

#include <veriblock/alt-util.hpp>
#include <veriblock/altintegration.hpp>
#include <veriblock/finalizer.hpp>
#include <veriblock/stateless_validation.hpp>
#include <veriblock/storage/util.hpp>
#include <veriblock/validation_state.hpp>

#include <shutdown.h>
#include <veriblock/storage/inmem/storage_manager_inmem.hpp>

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
    altintegration::ValidationState state;
    bool ret = this->altTree->setState(pindexPrev.GetBlockHash().asVector(), state);
    (void)ret;
    assert(ret);

    auto& config = getService<Config>();
    if ((pindexPrev.nHeight + 1) < (int)config.popconfig.alt->getEndorsementSettlementInterval()) {
        return {};
    }
    auto blockHash = pindexPrev.GetBlockHash();
    auto rewards = altTree->getPopPayout(blockHash.asVector());
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

bool PopServiceImpl::acceptBlock(const CBlockIndex& indexNew, BlockValidationState& state)
{
    AssertLockHeld(cs_main);
    auto containing = VeriBlock::blockToAltBlock(indexNew);
    altintegration::ValidationState instate;
    if (!altTree->acceptBlockHeader(containing, instate)) {
        LogPrintf("ERROR: alt tree cannot accept block %s\n", instate.toString());
        return state.Invalid(BlockValidationResult::BLOCK_CACHED_INVALID, instate.GetPath());
    }

    return true;
}

bool PopServiceImpl::addAllBlockPayloads(const CBlock& connecting, BlockValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);

    // add payloads only from blocks with POP data
    if (connecting.nVersion & POP_BLOCK_VERSION_BIT) {
        return addAllPayloadsToBlockImpl(*altTree, connecting, state);
    }

    return true;
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

    if (!altTree->setState(left.hash, state)) {
        if (!altTree->setState(right.hash, state)) {
            throw std::logic_error("both chains are invalid");
        }
        return -1;
    }

    return altTree->comparePopScore(left.hash, right.hash);
}

PopServiceImpl::PopServiceImpl(const altintegration::Config& config, CDBWrapper& db)
{
    config.validate();

    repo = std::make_shared<Repository>(db);
    store = std::make_shared<altintegration::PayloadsStorage>(repo);
    altTree = altintegration::Altintegration::create(config, *store);
    mempool = std::make_shared<altintegration::MemPool>(*altTree);
    //
    mempool->onAccepted<altintegration::ATV>(VeriBlock::p2p::offerPopDataToAllNodes<altintegration::ATV>);
    mempool->onAccepted<altintegration::VTB>(VeriBlock::p2p::offerPopDataToAllNodes<altintegration::VTB>);
    mempool->onAccepted<altintegration::VbkBlock>(VeriBlock::p2p::offerPopDataToAllNodes<altintegration::VbkBlock>);
}

bool PopServiceImpl::setState(const uint256& block, altintegration::ValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    return altTree->setState(block.asVector(), state);
}

altintegration::PopData PopServiceImpl::getPopData() EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    return mempool->getPop();
}

void PopServiceImpl::removePayloadsFromMempool(const altintegration::PopData& popData) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    mempool->removePayloads(popData);
}

bool PopServiceImpl::hasPopData(CBlockTreeDB& db)
{
    return db.Exists(BatchAdapter::btctip()) && db.Exists(BatchAdapter::vbktip()) && db.Exists(BatchAdapter::alttip());
}


void PopServiceImpl::saveTrees(altintegration::BatchAdaptor& batch)
{
    AssertLockHeld(cs_main);
    altintegration::SaveAllTrees(*this->altTree, batch);
}

template <typename BlockTree>
bool LoadTree(CDBIterator& iter, char blocktype, std::pair<char, std::string> tiptype, BlockTree& out, altintegration::ValidationState& state)
{
    using index_t = typename BlockTree::index_t;
    using block_t = typename index_t::block_t;
    using hash_t = typename BlockTree::hash_t;

    // Load tip
    hash_t tiphash;
    std::pair<char, std::string> ckey;

    iter.Seek(tiptype);
    if (!iter.Valid()) {
        // no valid tip is stored = no need to load anything
        return error("%s: failed to load %s tip", block_t::name());
    }
    if (!iter.GetKey(ckey)) {
        return error("%s: failed to find key %c:%s in %s", __func__, tiptype.first, tiptype.second, block_t::name());
    }
    if (ckey != tiptype) {
        return error("%s: bad key for tip %c:%s in %s", __func__, tiptype.first, tiptype.second, block_t::name());
    }
    if (!iter.GetValue(tiphash)) {
        return error("%s: failed to read tip value in %s", __func__, block_t::name());
    }

    std::vector<index_t> blocks;

    // Load blocks
    iter.Seek(std::make_pair(blocktype, hash_t()));
    while (iter.Valid()) {
#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
        boost::this_thread::interruption_point();
#endif
        if (ShutdownRequested()) return false;
        std::pair<char, hash_t> key;
        if (iter.GetKey(key) && key.first == blocktype) {
            index_t diskindex;
            if (iter.GetValue(diskindex)) {
                blocks.push_back(diskindex);
                iter.Next();
            } else {
                return error("%s: failed to read %s block", __func__, block_t::name());
            }
        } else {
            break;
        }
    }

    // sort blocks by height
    std::sort(blocks.begin(), blocks.end(), [](const index_t& a, const index_t& b) {
        return a.getHeight() < b.getHeight();
    });
    if (!altintegration::LoadTree(out, blocks, tiphash, state)) {
        return error("%s: failed to load tree %s", __func__, block_t::name());
    }

    auto* tip = out.getBestChain().tip();
    assert(tip);
    LogPrintf("Loaded %d blocks in %s tree with tip %s\n", out.getBlocks().size(), block_t::name(), tip->toShortPrettyString());

    return true;
}

bool PopServiceImpl::loadTrees(CDBIterator& iter)
{
    altintegration::ValidationState state;
    if (!LoadTree(iter, DB_BTC_BLOCK, BatchAdapter::btctip(), altTree->btc(), state)) {
        return error("%s: failed to load BTC tree %s", __func__, state.toString());
    }
    if (!LoadTree(iter, DB_VBK_BLOCK, BatchAdapter::vbktip(), altTree->vbk(), state)) {
        return error("%s: failed to load VBK tree %s", __func__, state.toString());
    }
    if (!LoadTree(iter, DB_ALT_BLOCK, BatchAdapter::alttip(), *altTree, state)) {
        return error("%s: failed to load ALT tree %s", __func__, state.toString());
    }
    return true;
}

void PopServiceImpl::addDisconnectedPopdata(const altintegration::PopData& popData) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    this->disconnected_popdata.push_back(popData);
}

void PopServiceImpl::updatePopMempoolForReorg() EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    for (const auto& popData : disconnected_popdata) {
        mempool->submitAll(popData);
    }
    disconnected_popdata.clear();
}

bool checkPopDataSize(const altintegration::PopData& popData, altintegration::ValidationState& state)
{
    uint32_t nPopDataSize = ::GetSerializeSize(popData, CLIENT_VERSION);
    auto& config = getService<Config>();

    if (nPopDataSize >= config.popconfig.alt->getMaxPopDataSize()) {
        return state.Invalid("popdata-overisize", "popData raw size more than allowed");
    }

    return true;
}

bool popdataStatelessValidation(const altintegration::PopData& popData, altintegration::ValidationState& state)
{
    auto& config = getService<Config>();

    for (const auto& b : popData.context) {
        if (!altintegration::checkBlock(b, state, *config.popconfig.vbk.params)) {
            return state.Invalid("pop-vbkblock-statelessly-invalid");
        }
    }

    for (const auto& vtb : popData.vtbs) {
        if (!altintegration::checkVTB(vtb, state, *config.popconfig.vbk.params, *config.popconfig.btc.params)) {
            return state.Invalid("pop-vtb-statelessly-invalid");
        }
    }

    for (const auto& atv : popData.atvs) {
        if (!altintegration::checkATV(atv, state, *config.popconfig.alt, *config.popconfig.vbk.params)) {
            return state.Invalid("pop-atv-statelessly-invalid");
        }
    }

    return true;
}

bool addAllPayloadsToBlockImpl(altintegration::AltTree& tree, const CBlock& block, BlockValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);

    altintegration::ValidationState instate;

    if (!checkPopDataSize(block.popData, instate) || !popdataStatelessValidation(block.popData, instate)) {
        return error("[%s] block %s is not accepted by popData: %s", __func__, block.GetHash().ToString(),
            instate.toString());
    }

    if (!tree.addPayloads(block.GetHash().asVector(), block.popData, instate)) {
        state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, instate.toString(), "");
        return error("[%s] block %s failed stateful pop validation: %s", __func__, block.GetHash().ToString(),
            instate.toString());
    }

    return true;
}

} // namespace VeriBlock
