// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <consensus/validation.h>
#include <dbwrapper.h>
#include <shutdown.h>
#include <validation.h>
#include <vbk/adaptors/payloads_provider.hpp>
#include <veriblock/pop.hpp>


#ifdef WIN32
#include <boost/thread/interruption.hpp>
#endif //WIN32

#include "pop_service.hpp"
#include <utility>
#include <vbk/adaptors/block_provider.hpp>
#include <vbk/p2p_sync.hpp>
#include <vbk/pop_common.hpp>
#include <veriblock/pop.hpp>


namespace VeriBlock {

void InitPopContext(CDBWrapper& db)
{
    auto payloads_provider = std::make_shared<PayloadsProvider>(db);
    SetPop(payloads_provider);

    auto& app = GetPop();
    app.getMemPool().onAccepted<altintegration::ATV>(VeriBlock::p2p::offerPopDataToAllNodes<altintegration::ATV>);
    app.getMemPool().onAccepted<altintegration::VTB>(VeriBlock::p2p::offerPopDataToAllNodes<altintegration::VTB>);
    app.getMemPool().onAccepted<altintegration::VbkBlock>(VeriBlock::p2p::offerPopDataToAllNodes<altintegration::VbkBlock>);
}

CBlockIndex* compareTipToBlock(CBlockIndex* candidate)
{
    AssertLockHeld(cs_main);
    assert(candidate != nullptr && "block has no according header in block tree");

    auto blockHash = candidate->GetBlockHash();
    auto* tip = ChainActive().Tip();
    if (!tip) {
        // if tip is not set, candidate wins
        return nullptr;
    }

    auto tipHash = tip->GetBlockHash();
    if (tipHash == blockHash) {
        // we compare tip with itself
        return tip;
    }

    int result = 0;
    if (Params().isPopActive(tip->nHeight)) {
        result = compareForks(*tip, *candidate);
    } else {
        result = CBlockIndexWorkComparator()(tip, candidate) ? -1 : 1;
    }

    if (result < 0) {
        // candidate has higher POP score
        return candidate;
    }

    if (result == 0 && tip->nChainWork < candidate->nChainWork) {
        // candidate is POP equal to current tip;
        // candidate has higher chainwork
        return candidate;
    }

    // otherwise, current chain wins
    return tip;
}

bool acceptBlock(const CBlockIndex& indexNew, BlockValidationState& state)
{
    AssertLockHeld(cs_main);
    auto containing = VeriBlock::blockToAltBlock(indexNew);
    altintegration::ValidationState instate;
    if (!GetPop().getAltBlockTree().acceptBlockHeader(containing, instate)) {
        LogPrintf("ERROR: alt tree cannot accept block %s\n", instate.toString());
        return state.Invalid(BlockValidationResult::BLOCK_CACHED_INVALID, instate.GetPath());
    }

    return true;
}

bool addAllBlockPayloads(const CBlock& block, BlockValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    auto bootstrapBlockHeight = GetPop().getConfig().getAltParams().getBootstrapBlock().height;
    auto hash = block.GetHash();
    auto* index = LookupBlockIndex(hash);

    if (index->nHeight == bootstrapBlockHeight) {
        // skip bootstrap block block
        return true;
    }

    altintegration::ValidationState instate;

    if (!GetPop().check(block.popData, instate)) {
        return error("[%s] block %s is not accepted because popData is invalid: %s", __func__, block.GetHash().ToString(),
            instate.toString());
    }

    GetPop().getAltBlockTree().acceptBlock(block.GetHash().asVector(), block.popData);

    return true;
}

bool setState(const uint256& block, altintegration::ValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    return GetPop().getAltBlockTree().setState(block.asVector(), state);
}

altintegration::PopData getPopData(const CBlockIndex& pindexPrev) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);

    auto prevHash = pindexPrev.GetBlockHash().asVector();
    return GetPop().generatePopData(prevHash);
}

// PoP rewards are calculated for the current tip but are paid in the next block
PoPRewards getPopRewards(const CBlockIndex& pindexPrev, const CChainParams& params) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    auto& pop = GetPop();

    if (!params.isPopActive(pindexPrev.nHeight)) {
        return {};
    }

    auto& cfg = pop.getConfig();
    if (pindexPrev.nHeight < (int)cfg.alt->getEndorsementSettlementInterval()) {
        return {};
    }
    if (pindexPrev.nHeight < (int)cfg.alt->getPayoutParams().getPopPayoutDelay()) {
        return {};
    }

    altintegration::ValidationState state;
    auto prevHash = pindexPrev.GetBlockHash().asVector();
    bool ret = pop.getAltBlockTree().setState(prevHash, state);
    (void)ret;
    assert(ret);

    auto rewards = pop.getPopPayout(prevHash);
    int halvings = (pindexPrev.nHeight + 1) / params.GetConsensus().nSubsidyHalvingInterval;
    PoPRewards result{};
    // erase rewards, that pay 0 satoshis, then halve rewards
    for (const auto& r : rewards) {
        auto rewardValue = r.second;
        rewardValue >>= halvings;
        if ((rewardValue != 0) && (halvings < 64)) {
            CScript key = CScript(r.first.begin(), r.first.end());
            result[key] = params.PopRewardCoefficient() * rewardValue;
        }
    }

    return result;
}

void addPopPayoutsIntoCoinbaseTx(CMutableTransaction& coinbaseTx, const CBlockIndex& pindexPrev, const CChainParams& params) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    PoPRewards rewards = getPopRewards(pindexPrev, params);
    assert(coinbaseTx.vout.size() == 1 && "at this place we should have only PoW payout here");
    for (const auto& itr : rewards) {
        CTxOut out;
        out.scriptPubKey = itr.first;
        out.nValue = itr.second;
        coinbaseTx.vout.push_back(out);
    }
}

bool checkCoinbaseTxWithPopRewards(const CTransaction& tx, const CAmount& nFees, const CBlockIndex& pindex, const CChainParams& params, BlockValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    PoPRewards expectedRewards = getPopRewards(*pindex.pprev, params);
    CAmount nTotalPopReward = 0;

    if (tx.vout.size() < expectedRewards.size()) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "bad-pop-vouts-size",
            strprintf("checkCoinbaseTxWithPopRewards(): coinbase has incorrect size of pop vouts (actual vouts size=%d vs expected vouts=%d)", tx.vout.size(), expectedRewards.size()));
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
    for (const auto& payout : expectedRewards) {
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

    CAmount PoWBlockReward =
        GetBlockSubsidy(pindex.nHeight, params);

    if (tx.GetValueOut() > nTotalPopReward + PoWBlockReward + nFees) {
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS,
            "bad-cb-pop-amount",
            strprintf("ConnectBlock(): coinbase pays too much (actual=%d vs limit=%d)", tx.GetValueOut(), PoWBlockReward + nTotalPopReward));
    }

    return true;
}

std::vector<BlockBytes> getLastKnownVBKBlocks(size_t blocks)
{
    LOCK(cs_main);
    return altintegration::getLastKnownBlocks(GetPop().getVbkBlockTree(), blocks);
}

std::vector<BlockBytes> getLastKnownBTCBlocks(size_t blocks)
{
    LOCK(cs_main);
    return altintegration::getLastKnownBlocks(GetPop().getBtcBlockTree(), blocks);
}

bool hasPopData(CBlockTreeDB& db)
{
    return db.Exists(tip_key<altintegration::BtcBlock>()) &&
           db.Exists(tip_key<altintegration::VbkBlock>()) &&
           db.Exists(tip_key<altintegration::AltBlock>());
}

void saveTrees(CDBBatch* batch)
{
    AssertLockHeld(cs_main);
    VeriBlock::BlockBatch b(*batch);
    GetPop().saveAllTrees(b);
}
bool loadTrees(CDBWrapper& db)
{
    altintegration::ValidationState state;

    BlockReader reader(db);
    if (!GetPop().loadAllTrees(reader, state)) {
        return error("%s: failed to load trees %s", __func__, state.toString());
    }

    return true;
}

void removePayloadsFromMempool(const altintegration::PopData& popData) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    GetPop().getMemPool().removeAll(popData);
}

int compareForks(const CBlockIndex& leftForkTip, const CBlockIndex& rightForkTip) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    auto& pop = GetPop();
    AssertLockHeld(cs_main);
    if (&leftForkTip == &rightForkTip) {
        return 0;
    }

    auto left = blockToAltBlock(leftForkTip);
    auto right = blockToAltBlock(rightForkTip);
    auto state = altintegration::ValidationState();

    if (!pop.getAltBlockTree().setState(left.hash, state)) {
        assert(false && "current tip is invalid");
    }

    return pop.getAltBlockTree().comparePopScore(left.hash, right.hash);
}

CAmount getCoinbaseSubsidy(const CAmount& subsidy, int32_t height, const CChainParams& params)
{
    if (!params.isPopActive(height)) {
        return subsidy;
    }

    int64_t powRewardPercentage = 100 - params.PopRewardPercentage();
    CAmount newSubsidy = powRewardPercentage * subsidy;
    return newSubsidy / 100;
}

void addDisconnectedPopdata(const altintegration::PopData& popData) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    altintegration::ValidationState state;
    auto& popmp = VeriBlock::GetPop().getMemPool();
    for (const auto& i : popData.context) {
        popmp.submit(i, state);
    }
    for (const auto& i : popData.vtbs) {
        popmp.submit(i, state);
    }
    for (const auto& i : popData.atvs) {
        popmp.submit(i, state);
    }
}

bool isPopEnabled()
{
    auto block = VeriBlock::GetPop().getConfig().getAltParams().getBootstrapBlock();
    CBlockIndex* index;
    {
        LOCK(cs_main);
        index = LookupBlockIndex(uint256(block.hash));
    }
    if (index == nullptr) {
        return false;
    }
    if (index->nHeight == 0) {
        return true;
    }
    return ChainActive().Contains(index);
}

bool isPopEnabled(int32_t height)
{
    auto block = VeriBlock::GetPop().getConfig().getAltParams().getBootstrapBlock();
    return height >= block.getHeight();
}

} // namespace VeriBlock
