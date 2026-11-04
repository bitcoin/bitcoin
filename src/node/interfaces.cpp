// Copyright (c) 2018-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/node.h>

#include <addrdb.h>
#include <banman.h>
#include <blockfilter.h>
#include <chain.h>
#include <chainlock/chainlock.h>
#include <chainparams.h>
#include <coinjoin/common.h>
#include <deploymentstatus.h>
#include <evo/chainhelper.h>
#include <evo/creditpool.h>
#include <evo/deterministicmns.h>
#include <evo/providertx_service.h>
#include <evo/specialtxman.h>
#include <external_signer.h>
#include <governance/governance.h>
#include <governance/object.h>
#include <governance/superblock.h>
#include <governance/vote.h>
#include <index/blockfilterindex.h>
#include <init.h>
#include <instantsend/instantsend.h>
#include <interfaces/chain.h>
#include <interfaces/coinjoin.h>
#include <interfaces/handler.h>
#include <interfaces/wallet.h>
#include <kernel/chain.h>
#include <kernel/mempool_entry.h>
#include <llmq/blockprocessor.h>
#include <llmq/commitment.h>
#include <llmq/context.h>
#include <llmq/options.h>
#include <llmq/quorums.h>
#include <llmq/quorumsman.h>
#include <mapport.h>
#include <masternode/sync.h>
#include <net.h>
#include <net_processing.h>
#include <netaddress.h>
#include <netbase.h>
#include <node/blockstorage.h>
#include <node/coin.h>
#include <node/context.h>
#include <node/interface_ui.h>
#include <node/transaction.h>
#include <policy/feerate.h>
#include <policy/fees.h>
#include <policy/policy.h>
#include <policy/settings.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <rpc/protocol.h>
#include <rpc/server.h>
#include <rpc/server_util.h>
#include <shutdown.h>
#include <support/allocators/secure.h>
#include <sync.h>
#include <txmempool.h>
#include <uint256.h>
#include <util/check.h>
#include <util/system.h>
#include <util/result.h>
#include <util/translation.h>
#include <validation.h>
#include <validationinterface.h>
#include <warnings.h>

#include <governance/common.h>

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <coinjoin/coinjoin.h>
#include <coinjoin/options.h>

#include <univalue.h>

#include <boost/signals2/signal.hpp>

#include <algorithm>
#include <memory>
#include <optional>
#include <ranges>
#include <utility>
#include <variant>

using interfaces::BlockTip;
using interfaces::Chain;
using interfaces::EVO;
using interfaces::FoundBlock;
using interfaces::GOV;
using interfaces::Handler;
using interfaces::LLMQ;
using interfaces::MakeSignalHandler;
using interfaces::MnEntry;
using interfaces::MnEntryCPtr;
using interfaces::MnList;
using interfaces::MnListPtr;
using interfaces::Node;
using interfaces::PreparedProviderRegistration;
using interfaces::PreparedSharedConsent;
using interfaces::PreparedSharedRegistration;
using interfaces::ProviderNetInfo;
using interfaces::ProviderRegistrationRequest;
using interfaces::ProviderRevokeRequest;
using interfaces::ProviderTxCapabilities;
using interfaces::ProviderTxError;
using interfaces::ProviderTxResult;
using interfaces::ProviderTxSubmission;
using interfaces::ProviderUpdateRegistrarRequest;
using interfaces::ProviderUpdateServiceRequest;
using interfaces::SharedCombineRequest;
using interfaces::SharedDissolvePrepareRequest;
using interfaces::SharedDissolveRequest;
using interfaces::SharedRegistrarUpdatePrepareRequest;
using interfaces::SharedRegistrationRequest;
using interfaces::SharedSignRequest;
using interfaces::SharedSignResult;
using interfaces::SharedUpdateShareRequest;
using interfaces::Wallet;
using interfaces::WalletLoader;

namespace node {
// All members of the classes in this namespace are intentionally public, as the
// classes themselves are private.
namespace {
class MnEntryImpl : public MnEntry
{
private:
    CDeterministicMNCPtr m_dmn;
    const std::vector<CScript> m_script_payouts;
    const CScript m_script_payout;

public:
    MnEntryImpl(const CDeterministicMNCPtr& dmn) :
        MnEntry{dmn},
        m_dmn{Assert(dmn)},
        m_script_payouts{m_dmn->pdmnState->GetOwnerRewardScripts()},
        m_script_payout{m_script_payouts.empty() ? CScript() : m_script_payouts.front()}
    {
    }
    ~MnEntryImpl() = default;

    bool isBanned() const override { return m_dmn->pdmnState->IsBanned(); }

    CService getNetInfoPrimary() const override { return m_dmn->pdmnState->netInfo->GetPrimary(); }
    std::vector<CService> getPlatformHTTPSAddrs() const override
    {
        std::vector<CService> ret;
        if (m_dmn->pdmnState->nVersion < ProTxVersion::ExtAddr) {
            // Before ExtAddr the Platform ports are scalar fields paired with
            // the primary address instead of netInfo entries, so an evonode
            // that has not submitted an extended-address update would
            // otherwise contribute no gateway at all.
            if (m_dmn->nType == MnType::Evo && m_dmn->pdmnState->platformHTTPPort != 0) {
                ret.emplace_back(m_dmn->pdmnState->netInfo->GetPrimary(),
                                 m_dmn->pdmnState->platformHTTPPort);
            }
            return ret;
        }
        for (const auto& entry : m_dmn->pdmnState->netInfo->GetEntries(NetInfoPurpose::PLATFORM_HTTPS)) {
            if (const auto service_opt{entry.GetAddrPort()}) {
                ret.push_back(*service_opt);
            }
        }
        return ret;
    }
    MnType getType() const override { return m_dmn->nType; }
    UniValue toJson() const override { return m_dmn->ToJson(); }
    const CKeyID& getKeyIdOwner() const override { return m_dmn->pdmnState->keyIDOwner; }
    const CKeyID& getKeyIdVoting() const override { return m_dmn->pdmnState->keyIDVoting; }
    const COutPoint& getCollateralOutpoint() const override { return m_dmn->collateralOutpoint; }
    const CScript& getScriptPayout() const override { return m_script_payout; }
    std::vector<CScript> getScriptPayouts() const override { return m_script_payouts; }
    const CScript& getScriptOperatorPayout() const override { return m_dmn->pdmnState->scriptOperatorPayout; }
    const int32_t& getLastPaidHeight() const override { return m_dmn->pdmnState->nLastPaidHeight; }
    const int32_t& getPoSePenalty() const override { return m_dmn->pdmnState->nPoSePenalty; }
    const int32_t& getRegisteredHeight() const override { return m_dmn->pdmnState->nRegisteredHeight; }
    const uint16_t& getOperatorReward() const override { return m_dmn->nOperatorReward; }
    const uint256& getProTxHash() const override { return m_dmn->proTxHash; }
    bool isShared() const override { return m_dmn->pdmnState->IsShared(); }
    std::vector<interfaces::MnShare> getShares() const override
    {
        std::vector<interfaces::MnShare> ret;
        ret.reserve(m_dmn->pdmnState->shares.size());
        for (const auto& share : m_dmn->pdmnState->shares) {
            ret.push_back({share.amount, share.scriptRefund, share.scriptReward, share.keyIDOwner});
        }
        return ret;
    }
    const uint32_t& getEarlyPeriodBlocks() const override { return m_dmn->pdmnState->nEarlyPeriodBlocks; }
    const CAmount& getEarlyPenalty() const override { return m_dmn->pdmnState->nEarlyPenalty; }
};

class MnListImpl : public MnList
{
private:
    CDeterministicMNList m_list;

public:
    MnListImpl(const CDeterministicMNList& mn_list) :
        MnList{mn_list},
        m_list{mn_list}
    {
    }
    ~MnListImpl() = default;

    Counts getCounts() const override
    {
        const auto counts{m_list.GetCounts()};
        return {
            .m_total_evo = counts.m_total_evo,
            .m_total_mn = counts.m_total_mn,
            .m_total_weighted = counts.m_total_weighted,
            .m_valid_evo = counts.m_valid_evo,
            .m_valid_mn = counts.m_valid_mn,
            .m_valid_weighted = counts.m_valid_weighted,
        };
    }
    int32_t getHeight() const override { return m_list.GetHeight(); }
    uint256 getBlockHash() const override { return m_list.GetBlockHash(); }

    void forEachMN(bool only_valid, std::function<void(const MnEntryCPtr&)> cb) const override
    {
        m_list.ForEachMNShared(only_valid, [&cb](const auto& dmn) {
            cb(std::make_shared<const MnEntryImpl>(dmn));
        });
    }
    std::vector<MnEntryCPtr> getProjectedMNPayees(const CBlockIndex* pindex) const override
    {
        std::vector<MnEntryCPtr> ret;
        for (const auto& payee : m_list.GetProjectedMNPayees(pindex)) {
            ret.emplace_back(std::make_shared<const MnEntryImpl>(payee));
        }
        return ret;
    }

    void setContext(NodeContext* context) override
    {
        m_context = context;
    }

private:
    // Note: Currently we do nothing with m_context but in the future, if we have a hard fork
    //       that requires checking for deployment information in deterministic masternode logic,
    //       we will need NodeContext::chainman. This has been kept around to retain those code
    //       paths.
    [[maybe_unused]] NodeContext* m_context{nullptr};
};

class EVOImpl : public EVO
{
private:
    ChainstateManager& chainman() { return *Assert(m_context->chainman); }
    NodeContext& context() { return *Assert(m_context); }

public:
    std::pair<MnListPtr, const CBlockIndex*> getListAtChainTip() override
    {
        const auto *tip = WITH_LOCK(::cs_main, return chainman().ActiveChain().Tip());
        if (tip && context().dmnman) {
            MnListImpl mnList = context().dmnman->GetListForBlock(tip);
            if (!mnList.getBlockHash().IsNull()) {
                mnList.setContext(m_context);
                return {std::make_shared<MnListImpl>(mnList), tip};
            }
        }
        return {nullptr, nullptr};
    }
    ProviderTxCapabilities getProviderTxCapabilities() override { return evo::provider::GetCapabilities(context()); }
    std::optional<ProviderTxError> validateProviderNetInfo(const ProviderNetInfo& net_info, MnType type,
                                                           uint16_t version, bool ipv4_optional, bool optional) override
    {
        return evo::provider::ValidateNetInfo(net_info, type, version, ipv4_optional, optional);
    }
    std::optional<std::string> findProviderNetInfoInUse(const ProviderNetInfo& net_info, MnType type, uint16_t version,
                                                        bool ipv4_optional) override
    {
        return evo::provider::FindNetInfoInUse(context(), net_info, type, version, ipv4_optional);
    }
    ProviderTxResult<ProviderTxSubmission> registerMasternode(Wallet& wallet, const ProviderRegistrationRequest& request) override
    {
        return evo::provider::Register(context(), wallet, request);
    }
    ProviderTxResult<PreparedProviderRegistration> prepareMasternodeRegistration(
        Wallet& wallet, const ProviderRegistrationRequest& request) override
    {
        return evo::provider::PrepareRegistration(context(), wallet, request);
    }
    ProviderTxResult<ProviderTxSubmission> submitMasternodeRegistration(
        Wallet& wallet, const CTransactionRef& tx, const std::vector<unsigned char>& collateral_signature) override
    {
        return evo::provider::SubmitRegistration(context(), wallet, tx, collateral_signature);
    }
    ProviderTxResult<ProviderTxSubmission> updateMasternodeService(Wallet& wallet,
                                                                   const ProviderUpdateServiceRequest& request) override
    {
        return evo::provider::UpdateService(context(), wallet, request);
    }
    ProviderTxResult<ProviderTxSubmission> updateMasternodeRegistrar(Wallet& wallet,
                                                                     const ProviderUpdateRegistrarRequest& request) override
    {
        return evo::provider::UpdateRegistrar(context(), wallet, request);
    }
    ProviderTxResult<ProviderTxSubmission> revokeMasternode(Wallet& wallet, const ProviderRevokeRequest& request) override
    {
        return evo::provider::Revoke(context(), wallet, request);
    }
    ProviderTxResult<PreparedSharedRegistration> prepareSharedRegistration(const SharedRegistrationRequest& request) override
    {
        return evo::provider::PrepareSharedRegistration(context(), request);
    }
    ProviderTxResult<SharedSignResult> signShared(Wallet& wallet, const SharedSignRequest& request) override
    {
        return evo::provider::SignShared(context(), wallet, request);
    }
    ProviderTxResult<ProviderTxSubmission> combineShared(Wallet& wallet, const SharedCombineRequest& request) override
    {
        return evo::provider::CombineShared(context(), &wallet, request);
    }
    ProviderTxResult<ProviderTxSubmission> dissolveShared(Wallet& wallet, const SharedDissolveRequest& request) override
    {
        return evo::provider::DissolveShared(context(), wallet, request);
    }
    ProviderTxResult<PreparedSharedConsent> prepareSharedDissolution(const SharedDissolvePrepareRequest& request) override
    {
        return evo::provider::PrepareSharedDissolution(context(), request);
    }
    ProviderTxResult<ProviderTxSubmission> updateShare(Wallet& wallet, const SharedUpdateShareRequest& request) override
    {
        return evo::provider::UpdateShare(context(), wallet, request);
    }
    ProviderTxResult<PreparedSharedConsent> prepareSharedRegistrarUpdate(
        Wallet& wallet, const SharedRegistrarUpdatePrepareRequest& request) override
    {
        return evo::provider::PrepareSharedRegistrarUpdate(context(), wallet, request);
    }
    void setContext(NodeContext* context) override
    {
        m_context = context;
    }

private:
    NodeContext* m_context{nullptr};
};

class GOVImpl : public GOV
{
private:
    NodeContext& context() { return *Assert(m_context); }

public:
    void getAllNewerThan(std::vector<CGovernanceObject> &objs, int64_t nMoreThanTime,
                         bool include_postponed) override
    {
        if (context().govman != nullptr) {
            context().govman->GetAllNewerThan(objs, nMoreThanTime, include_postponed);
        }
    }
    Votes getObjVotes(const CGovernanceObject& obj, vote_signal_enum_t vote_signal) override
    {
        Votes ret;
        if (context().govman != nullptr && context().dmnman != nullptr) {
            const auto& tip_mn_list{context().dmnman->GetListAtChainTip()};
            if (auto govobj{context().govman->FindGovernanceObject(obj.GetHash())}) {
                ret.m_abs = govobj->GetAbstainCount(tip_mn_list, vote_signal);
                ret.m_no = govobj->GetNoCount(tip_mn_list, vote_signal);
                ret.m_yes = govobj->GetYesCount(tip_mn_list, vote_signal);
            } else {
                ret.m_abs = obj.GetAbstainCount(tip_mn_list, vote_signal);
                ret.m_no = obj.GetNoCount(tip_mn_list, vote_signal);
                ret.m_yes = obj.GetYesCount(tip_mn_list, vote_signal);
            }
        }
        return ret;
    }
    UniqueVoters getObjUniqueVoters(const CGovernanceObject& obj, vote_signal_enum_t vote_signal) override
    {
        if (context().govman != nullptr && context().dmnman != nullptr) {
            const auto& tip_mn_list{context().dmnman->GetListAtChainTip()};
            if (auto govobj{context().govman->FindGovernanceObject(obj.GetHash())}) {
                const auto count = govobj->GetUniqueVoterCount(tip_mn_list, vote_signal);
                return {.m_regular = count.m_regular, .m_evo = count.m_evo};
            } else {
                const auto count = obj.GetUniqueVoterCount(tip_mn_list, vote_signal);
                return {.m_regular = count.m_regular, .m_evo = count.m_evo};
            }
        }
        return {0, 0};
    }
    std::vector<CGovernanceVote> getCurrentVotes(const uint256& hash) override
    {
        if (context().govman != nullptr) {
            return context().govman->GetCurrentVotes(hash, COutPoint{});
        }
        return {};
    }
    bool existsObj(const uint256& hash) override
    {
        if (context().govman != nullptr) {
            return context().govman->HaveObjectForHash(hash);
        }
        return false;
    }
    bool isEnabled() override
    {
        if (context().govman != nullptr) {
            return context().govman->IsValid();
        }
        return false;
    }
    bool processVoteAndRelay(const CGovernanceVote& vote, std::string& error) override
    {
        if (context().govman != nullptr) {
            CGovernanceException exception;
            bool result = context().govman->ProcessVoteAndRelay(vote, exception);
            if (!result) {
                error = exception.GetMessage();
            }
            return result;
        }
        error = "Governance manager not available";
        return false;
    }
    GovernanceInfo getGovernanceInfo() override
    {
        GovernanceInfo info;
        if (context().chainman) {
            const Consensus::Params& consensusParams = context().chainman->GetConsensus();
            LOCK(::cs_main);
            CSuperblock::GetNearestSuperblocksHeights(context().chainman->ActiveHeight(), info.lastsuperblock, info.nextsuperblock);
            info.governancebudget = CSuperblock::GetPaymentsLimit(context().chainman->ActiveChain(), info.nextsuperblock);
            if (context().dmnman) {
                info.fundingthreshold = static_cast<int>(context().dmnman->GetListAtChainTip().GetCounts().m_valid_weighted / 10);
            }
            info.superblockcycle = consensusParams.nSuperblockCycle;
            info.superblockmaturitywindow = consensusParams.nSuperblockMaturityWindow;
            info.targetSpacing = consensusParams.nPowTargetSpacing;
        }
        info.proposalfee = GOVERNANCE_PROPOSAL_FEE_TX;
        info.relayRequiredConfs = GOVERNANCE_MIN_RELAY_FEE_CONFIRMATIONS;
        info.requiredConfs = GOVERNANCE_FEE_CONFIRMATIONS;
        return info;
    }
    std::optional<int32_t> getProposalFundedHeight(const uint256& proposal_hash) override
    {
        if (context().chain_helper != nullptr && context().chainman != nullptr) {
            const int32_t nTipHeight = WITH_LOCK(::cs_main, return context().chainman->ActiveHeight());
            for (const auto& trigger : context().chain_helper->superblocks->GetActiveTriggers()) {
                if (!trigger || trigger->GetBlockHeight() > nTipHeight) continue;
                for (const auto& hash : trigger->GetProposalHashes()) {
                    if (hash == proposal_hash) {
                        return trigger->GetBlockHeight();
                    }
                }
            }
        }
        return std::nullopt;
    }
    FundableResult getFundableProposalHashes() override
    {
        FundableResult result;
        if (context().govman != nullptr && context().chainman != nullptr && context().dmnman != nullptr) {
            const auto tip_mn_list{context().dmnman->GetListAtChainTip()};
            if (const auto proposals{context().govman->GetApprovedProposals(tip_mn_list)}; !proposals.empty()) {
                int32_t last_sb{0}, next_sb{0};
                CAmount budget{0};
                {
                    LOCK(::cs_main);
                    CSuperblock::GetNearestSuperblocksHeights(context().chainman->ActiveHeight(), last_sb, next_sb);
                    budget = CSuperblock::GetPaymentsLimit(context().chainman->ActiveChain(), next_sb);
                }
                for (const auto& proposal : proposals) {
                    UniValue json = proposal->GetJSONObject();
                    CAmount payment_amount{0};
                    try {
                        payment_amount = ParsePaymentAmount(json["payment_amount"].getValStr());
                    } catch (...) {
                        continue;
                    }
                    if (result.allocated + payment_amount > budget) {
                        // Budget is saturated, cannot fulfill proposal
                        continue;
                    }
                    result.allocated += payment_amount;
                    result.hashes.insert(proposal->GetHash());
                }
                return result;
            }
        }
        return result;
    }
    std::optional<CGovernanceObject> createProposal(int32_t revision, int64_t created_time,
                        const std::string& data_hex, std::string& error) override
    {
        CGovernanceObject govobj(uint256{}, revision, created_time, uint256{}, data_hex);
        if (govobj.GetObjectType() != GovernanceObject::PROPOSAL) {
            error = "Invalid object type, only proposals can be validated";
            return std::nullopt;
        }
        std::string strValidationError;
        if (!governance::ValidateProposal(data_hex, strValidationError)) {
            error = "Invalid proposal data: " + strValidationError;
            return std::nullopt;
        }
        const ChainstateManager& chainman = *Assert(context().chainman);
        {
            LOCK(::cs_main);
            std::string strError;
            if (!govobj.IsValidLocally(Assert(context().dmnman)->GetListAtChainTip(), chainman, strError, false)) {
                error = "Governance object is not valid - " + govobj.GetHash().ToString() + " - " + strError;
                return std::nullopt;
            }
        }
        return govobj;
    }

    bool submitProposal(const uint256& parent, int32_t revision, int64_t created_time, const std::string& data_hex,
                        const uint256& fee_txid, std::string& out_object_hash, std::string& error) override
    {
        if (!context().govman || !context().dmnman || !context().chainman) { error = "Governance not available"; return false; }
        if(!Assert(context().mn_sync)->IsBlockchainSynced()) { error = "Client not synced"; return false; }
        const auto mnList = Assert(context().dmnman)->GetListAtChainTip();
        CGovernanceObject govobj(parent, revision, created_time, fee_txid, data_hex);
        if (govobj.GetObjectType() == GovernanceObject::TRIGGER) { error = "Submission of triggers is not available"; return false; }
        if (govobj.GetObjectType() == GovernanceObject::PROPOSAL) {
            std::string strValidationError;
            if (!governance::ValidateProposal(data_hex, strValidationError)) { error = "Invalid proposal data: " + strValidationError; return false; }
        }
        const CTxMemPool& mempool = *Assert(context().mempool);
        bool fMissingConfirmations{false};
        {
            LOCK2(cs_main, mempool.cs);
            std::string strError;
            if (!govobj.IsValidLocally(mnList, *Assert(context().chainman), strError, fMissingConfirmations, true) && !fMissingConfirmations) {
                error = "Governance object is not valid - " + govobj.GetHash().ToString() + " - " + strError;
                return false;
            }
        }
        if (!Assert(context().govman)->MasternodeRateCheck(govobj)) { error = "Object creation rate limit exceeded"; return false; }
        if (fMissingConfirmations) {
            context().govman->AddPostponedObject(govobj);
            context().govman->RelayObject(govobj);
        } else {
            context().govman->AddGovernanceObject(govobj, "<local>");
        }
        out_object_hash = govobj.GetHash().ToString();
        return true;
    }
    void setContext(NodeContext* context) override
    {
        m_context = context;
    }

private:
    NodeContext* m_context{nullptr};
};

class LLMQImpl : public LLMQ
{
private:
    NodeContext& context() { return *Assert(m_context); }

public:
    CreditPoolCounts getCreditPoolCounts() override
    {
        CreditPoolCounts ret{};
        if (!context().chainman) {
            return ret;
        }
        const auto* pindex{WITH_LOCK(::cs_main, return context().chainman->ActiveChain().Tip())};
        if (!pindex || !pindex->pprev) {
            return ret;
        }
        auto& chain_helper{context().chainman->ActiveChainstate().ChainHelper()};
        const auto pool{chain_helper.GetCreditPool(pindex)};
        ret.m_locked = pool.locked;
        ret.m_limit = pool.currentLimit;
        ret.m_diff = pool.locked - chain_helper.GetCreditPool(pindex->pprev).locked;
        return ret;
    }
    ChainLockInfo getBestChainLock() override
    {
        if (!context().chainlocks) {
            return {};
        }
        const auto [clsig, pindex] = context().chainlocks->GetBestChainlockWithPindex();
        if (!pindex) {
            return {};
        }
        return {
            .m_height = clsig.getHeight(),
            .m_block_time = pindex->GetBlockTime(),
            .m_hash = clsig.getBlockHash(),
        };
    }
    InstantSendCounts getInstantSendCounts() override
    {
        if (!context().isman) {
            return {};
        }
        const auto counts{context().isman->GetCounts()};
        return {
            .m_verified = counts.m_verified,
            .m_unverified = counts.m_unverified,
            .m_awaiting_tx = counts.m_awaiting_tx,
            .m_unprotected_tx = counts.m_unprotected_tx,
        };
    }
    size_t getPendingAssetUnlocks() override
    {
        if (!context().mempool) {
            return 0;
        }
        LOCK(context().mempool->cs);
        return static_cast<size_t>(std::ranges::count_if(context().mempool->mapTx, [](const auto& entry) {
            return entry.GetTx().IsPlatformTransfer();
        }));
    }
    std::vector<QuorumInfo> getQuorumStats() override
    {
        std::vector<QuorumInfo> stats{};
        if (!context().llmq_ctx || !context().llmq_ctx->qman || !context().chainman) {
            return stats;
        }
        const auto* pindex{WITH_LOCK(::cs_main, return context().chainman->ActiveChain().Tip())};
        if (!pindex) {
            return stats;
        }
        for (const auto& type : llmq::GetEnabledQuorumTypes(*context().chainman, pindex)) {
            const auto llmq_params{Params().GetLLMQ(type)};
            if (!llmq_params.has_value()) {
                continue;
            }
            const auto quorums{context().llmq_ctx->qman->ScanQuorums(type, pindex, llmq_params->signingActiveQuorumCount)};
            double health{0.0};
            for (const auto& q : quorums) {
                size_t numMembers = q->members.size();
                size_t numValidMembers = q->qc->CountValidMembers();
                health += (numMembers > 0) ? (double(numValidMembers) / double(numMembers)) : 0.0;
            }
            health = quorums.empty() ? 0.0 : (health / quorums.size());
            const int32_t newest_height{(!quorums.empty() && quorums[0]->m_quorum_base_block_index)
                ? quorums[0]->m_quorum_base_block_index->nHeight : 0};
            const int32_t expiry_height{(newest_height > 0)
                ? newest_height + llmq_params->signingActiveQuorumCount * llmq_params->dkgInterval
                : 0};
            stats.emplace_back(QuorumInfo{
                .m_name = std::string(llmq_params->name),
                .m_count = quorums.size(),
                .m_health = health,
                .m_rotates = llmq_params->useRotation,
                .m_data_retention_blocks = llmq_params->max_store_depth(),
                .m_newest_height = newest_height,
                .m_expiry_height = expiry_height,
            });
        }
        return stats;
    }
    std::vector<PlatformQuorum> getPlatformQuorums(uint8_t llmq_type) override
    {
        std::vector<PlatformQuorum> ret;
        if (!context().llmq_ctx || !context().llmq_ctx->quorum_block_processor || !context().chainman) {
            return ret;
        }
        const auto* pindex{WITH_LOCK(::cs_main, return context().chainman->ActiveChain().Tip())};
        if (!pindex) {
            return ret;
        }
        const auto type{static_cast<Consensus::LLMQType>(llmq_type)};
        const auto llmq_params{Params().GetLLMQ(type)};
        if (!llmq_params.has_value()) {
            return ret;
        }
        // Drive proofs may be signed by an older Platform quorum while they
        // are still consensus-valid and retained locally. Export the full
        // retained-key window, not only the current signing-active set.
        const auto quorum_count{static_cast<size_t>(std::max(llmq_params->signingActiveQuorumCount,
                                                             llmq_params->keepOldKeys))};
        // Read mined final commitments directly: they already carry the quorum
        // hash and public key, so there is no need to materialize full CQuorum
        // objects (member lists, vvec/contribution reads, quorum cache inserts)
        // via ScanQuorums. Newest-first, matching ScanQuorums' ordering.
        const auto& qbp{*context().llmq_ctx->quorum_block_processor};
        const auto quorum_base_block_indexes{llmq_params->useRotation
            ? qbp.GetMinedCommitmentsIndexedUntilBlock(type, pindex, quorum_count)
            : qbp.GetMinedCommitmentsUntilBlock(type, pindex, quorum_count)};
        for (const auto* pQuorumBaseBlockIndex : quorum_base_block_indexes) {
            const auto qc{qbp.GetMinedCommitment(type, pQuorumBaseBlockIndex->GetBlockHash()).first};
            if (!qc.quorumPublicKey.IsValid()) continue;
            ret.emplace_back(PlatformQuorum{
                .m_quorum_hash = qc.quorumHash,
                .m_pubkey = qc.quorumPublicKey.ToByteVector(/*specificLegacyScheme=*/false),
                .m_height = pQuorumBaseBlockIndex->nHeight,
            });
        }
        return ret;
    }
    std::vector<uint8_t> getInstantSendLock(const uint256& txid) override
    {
        if (!context().isman) {
            return {};
        }
        const auto islock{context().isman->GetInstantSendLockByTxid(txid)};
        if (!islock) {
            return {};
        }
        CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
        ds << *islock;
        return {UCharCast(ds.data()), UCharCast(ds.data()) + ds.size()};
    }
    void setContext(NodeContext* context) override
    {
        m_context = context;
    }

private:
    NodeContext* m_context{nullptr};
};

namespace Masternode = interfaces::Masternode;
class MasternodeSyncImpl : public Masternode::Sync
{
private:
    NodeContext& context() { return *Assert(m_context); }

public:
    bool isSynced() override
    {
        if (context().mn_sync != nullptr) {
            return context().mn_sync->IsSynced();
        }
        return false;
    }
    bool isBlockchainSynced() override
    {
        if (context().mn_sync != nullptr) {
            return context().mn_sync->IsBlockchainSynced();
        }
        return false;
    }
    bool isGovernanceSynced() override
    {
        if (context().mn_sync != nullptr) {
            return context().mn_sync->GetAssetID() > MASTERNODE_SYNC_GOVERNANCE;
        }
        return false;
    }
    std::string getSyncStatus() override
    {
        if (context().mn_sync != nullptr) {
            return context().mn_sync->GetSyncStatus();
        }
        return "";
    }
    void setContext(NodeContext* context) override
    {
        m_context = context;
    }

private:
    NodeContext* m_context{nullptr};
};

namespace CoinJoin = interfaces::CoinJoin;
class CoinJoinOptionsImpl : public CoinJoin::Options
{
public:
    int getSessions() override
    {
        return CCoinJoinClientOptions::GetSessions();
    }
    int getRounds() override
    {
        return CCoinJoinClientOptions::GetRounds();
    }
    int getAmount() override
    {
        return CCoinJoinClientOptions::GetAmount();
    }
    int getDenomsGoal() override
    {
        return CCoinJoinClientOptions::GetDenomsGoal();
    }
    int getDenomsHardCap() override
    {
        return CCoinJoinClientOptions::GetDenomsHardCap();
    }
    void setEnabled(bool fEnabled) override
    {
        return CCoinJoinClientOptions::SetEnabled(fEnabled);
    }
    void setMultiSessionEnabled(bool fEnabled) override
    {
        CCoinJoinClientOptions::SetMultiSessionEnabled(fEnabled);
    }
    void setSessions(int sessions) override
    {
        CCoinJoinClientOptions::SetSessions(sessions);
    }
    void setRounds(int nRounds) override
    {
        CCoinJoinClientOptions::SetRounds(nRounds);
    }
    void setAmount(CAmount amount) override
    {
        CCoinJoinClientOptions::SetAmount(amount);
    }
    void setDenomsGoal(int denoms_goal) override
    {
        CCoinJoinClientOptions::SetDenomsGoal(denoms_goal);
    }
    void setDenomsHardCap(int denoms_hardcap) override
    {
        CCoinJoinClientOptions::SetDenomsHardCap(denoms_hardcap);
    }
    bool isEnabled() override
    {
        return CCoinJoinClientOptions::IsEnabled();
    }
    bool isMultiSessionEnabled() override
    {
        return CCoinJoinClientOptions::IsMultiSessionEnabled();
    }
    bool isCollateralAmount(CAmount nAmount) override
    {
        return ::CoinJoin::IsCollateralAmount(nAmount);
    }
    CAmount getMinCollateralAmount() override
    {
        return ::CoinJoin::GetCollateralAmount();
    }
    CAmount getMaxCollateralAmount() override
    {
        return ::CoinJoin::GetMaxCollateralAmount();
    }
    CAmount getSmallestDenomination() override
    {
        return ::CoinJoin::GetSmallestDenomination();
    }
    bool isDenominated(CAmount nAmount) override
    {
        return ::CoinJoin::IsDenominatedAmount(nAmount);
    }
    std::array<CAmount, 5> getStandardDenominations() override
    {
        return ::CoinJoin::GetStandardDenominations();
    }
};

#ifdef ENABLE_EXTERNAL_SIGNER
class ExternalSignerImpl : public interfaces::ExternalSigner
{
public:
    ExternalSignerImpl(::ExternalSigner signer) : m_signer(std::move(signer)) {}
    std::string getName() override { return m_signer.m_name; }
    ::ExternalSigner m_signer;
};
#endif

class NodeImpl : public Node
{
public:
    EVOImpl m_evo;
    GOVImpl m_gov;
    LLMQImpl m_llmq;
    MasternodeSyncImpl m_masternodeSync;
    CoinJoinOptionsImpl m_coinjoin;

    explicit NodeImpl(NodeContext& context) { setContext(&context); }
    void initLogging() override { InitLogging(*Assert(m_context->args)); }
    void initParameterInteraction() override { InitParameterInteraction(*Assert(m_context->args)); }
    bilingual_str getWarnings() override { return GetWarnings(true); }
    int getExitStatus() override { return Assert(m_context)->exit_status.load(); }
    uint64_t getLogCategories() override { return LogInstance().GetCategoryMask(); }
    bool baseInitialize() override
    {
        if (!AppInitBasicSetup(gArgs, Assert(m_context)->exit_status)) return false;
        if (!AppInitParameterInteraction(gArgs)) return false;

        m_context->kernel = std::make_unique<kernel::Context>();
        if (!AppInitSanityChecks(*m_context->kernel)) return false;

        if (!AppInitLockDataDirectory()) return false;
        if (!AppInitInterfaces(*m_context)) return false;

        return true;
    }
    bool appInitMain(interfaces::BlockAndHeaderTipInfo* tip_info) override
    {
        if (AppInitMain(*m_context, tip_info)) return true;
        // Error during initialization, set exit status before continue
        m_context->exit_status.store(EXIT_FAILURE);
        return false;
    }
    void appShutdown() override
    {
        Interrupt(*m_context);
        Shutdown(*m_context);
    }
    void appPrepareShutdown() override
    {
        Interrupt(*m_context);
        StartRestart();
        PrepareShutdown(*m_context);
    }
    void startShutdown() override
    {
        StartShutdown();
        // Stop RPC for clean shutdown if any of waitfor* commands is executed.
        if (gArgs.GetBoolArg("-server", false)) {
            InterruptRPC();
            StopRPC();
        }
    }
    bool shutdownRequested() override { return ShutdownRequested(); }
    bool isSettingIgnored(const std::string& name) override
    {
        bool ignored = false;
        gArgs.LockSettings([&](util::Settings& settings) {
            if (auto* options = util::FindKey(settings.command_line_options, name)) {
                ignored = !options->empty();
            }
        });
        return ignored;
    }
    util::SettingsValue getPersistentSetting(const std::string& name) override { return gArgs.GetPersistentSetting(name); }
    void updateRwSetting(const std::string& name, const util::SettingsValue& value) override
    {
        gArgs.LockSettings([&](util::Settings& settings) {
            if (value.isNull()) {
                settings.rw_settings.erase(name);
            } else {
                settings.rw_settings[name] = value;
            }
        });
        gArgs.WriteSettingsFile();
    }
    void forceSetting(const std::string& name, const util::SettingsValue& value) override
    {
        gArgs.LockSettings([&](util::Settings& settings) {
            if (value.isNull()) {
                settings.forced_settings.erase(name);
            } else {
                settings.forced_settings[name] = value;
            }
        });
    }
    void resetSettings() override
    {
        gArgs.WriteSettingsFile(/*errors=*/nullptr, /*backup=*/true);
        gArgs.LockSettings([&](util::Settings& settings) {
            settings.rw_settings.clear();
        });
        gArgs.WriteSettingsFile();
    }
    void mapPort(bool use_upnp, bool use_natpmp) override { StartMapPort(use_upnp, use_natpmp); }
    bool getProxy(Network net, Proxy& proxy_info) override { return GetProxy(net, proxy_info); }
    size_t getNodeCount(ConnectionDirection flags) override
    {
        return m_context->connman ? m_context->connman->GetNodeCount(flags) : 0;
    }
    bool getNodesStats(NodesStats& stats) override
    {
        stats.clear();

        if (m_context->connman) {
            std::vector<CNodeStats> stats_temp;
            m_context->connman->GetNodeStats(stats_temp);

            stats.reserve(stats_temp.size());
            for (auto& node_stats_temp : stats_temp) {
                stats.emplace_back(std::move(node_stats_temp), false, CNodeStateStats());
            }

            // Try to retrieve the CNodeStateStats for each node.
            if (m_context->peerman) {
                TRY_LOCK(::cs_main, lockMain);
                if (lockMain) {
                    for (auto& node_stats : stats) {
                        std::get<1>(node_stats) =
                            m_context->peerman->GetNodeStateStats(std::get<0>(node_stats).nodeid, std::get<2>(node_stats));
                    }
                }
            }
            return true;
        }
        return false;
    }
    bool getBanned(banmap_t& banmap) override
    {
        if (m_context->banman) {
            m_context->banman->GetBanned(banmap);
            return true;
        }
        return false;
    }
    bool ban(const CNetAddr& net_addr, int64_t ban_time_offset) override
    {
        if (m_context->banman) {
            m_context->banman->Ban(net_addr, ban_time_offset);
            return true;
        }
        return false;
    }
    bool unban(const CSubNet& ip) override
    {
        if (m_context->banman) {
            m_context->banman->Unban(ip);
            return true;
        }
        return false;
    }
    bool disconnectByAddress(const CNetAddr& net_addr) override
    {
        if (m_context->connman) {
            return m_context->connman->DisconnectNode(net_addr);
        }
        return false;
    }
    bool disconnectById(NodeId id) override
    {
        if (m_context->connman) {
            return m_context->connman->DisconnectNode(id);
        }
        return false;
    }
    std::vector<std::unique_ptr<interfaces::ExternalSigner>> listExternalSigners() override
    {
#ifdef ENABLE_EXTERNAL_SIGNER
        std::vector<ExternalSigner> signers = {};
        const std::string command = gArgs.GetArg("-signer", "");
        if (command == "") return {};
        ExternalSigner::Enumerate(command, signers, Params().NetworkIDString());
        std::vector<std::unique_ptr<interfaces::ExternalSigner>> result;
        result.reserve(signers.size());
        for (auto& signer : signers) {
            result.emplace_back(std::make_unique<ExternalSignerImpl>(std::move(signer)));
        }
        return result;
#else
        // This result is indistinguishable from a successful call that returns
        // no signers. For the current GUI this doesn't matter, because the wallet
        // creation dialog disables the external signer checkbox in both
        // cases. The return type could be changed to std::optional<std::vector>
        // (or something that also includes error messages) if this distinction
        // becomes important.
        return {};
#endif // ENABLE_EXTERNAL_SIGNER
    }
    int64_t getTotalBytesRecv() override { return m_context->connman ? m_context->connman->GetTotalBytesRecv() : 0; }
    int64_t getTotalBytesSent() override { return m_context->connman ? m_context->connman->GetTotalBytesSent() : 0; }
    size_t getMempoolSize() override { return m_context->mempool ? m_context->mempool->size() : 0; }
    size_t getMempoolDynamicUsage() override { return m_context->mempool ? m_context->mempool->DynamicMemoryUsage() : 0; }
    size_t getMempoolMaxUsage() override { return gArgs.GetIntArg("-maxmempool", DEFAULT_MAX_MEMPOOL_SIZE_MB) * 1000000; }
    bool getHeaderTip(int& height, int64_t& block_time) override
    {
        LOCK(::cs_main);
        auto best_header = chainman().m_best_header;
        if (best_header) {
            height = best_header->nHeight;
            block_time = best_header->GetBlockTime();
            return true;
        }
        return false;
    }
    std::map<CNetAddr, LocalServiceInfo> getNetLocalAddresses() override
    {
        if (m_context->connman)
            return m_context->connman->getNetLocalAddresses();
        else
            return {};
    }
    int getNumBlocks() override
    {
        LOCK(::cs_main);
        return chainman().ActiveChain().Height();
    }
    uint256 getBestBlockHash() override
    {
        const CBlockIndex* tip = WITH_LOCK(::cs_main, return chainman().ActiveChain().Tip());
        return tip ? tip->GetBlockHash() : chainman().GetParams().GenesisBlock().GetHash();
    }
    int64_t getLastBlockTime() override
    {
        LOCK(::cs_main);
        if (chainman().ActiveChain().Tip()) {
            return chainman().ActiveChain().Tip()->GetBlockTime();
        }
        return chainman().GetParams().GenesisBlock().GetBlockTime(); // Genesis block's time of current network
    }
    std::string getLastBlockHash() override
    {
        LOCK(::cs_main);
        if (m_context->chainman->ActiveChain().Tip()) {
            return m_context->chainman->ActiveChain().Tip()->GetBlockHash().ToString();
        }
        return chainman().GetParams().GenesisBlock().GetHash().ToString(); // Genesis block's hash of current network
    }
    double getVerificationProgress() override
    {
        return GuessVerificationProgress(chainman().GetParams().TxData(), WITH_LOCK(::cs_main, return chainman().ActiveChain().Tip()));
    }
    bool isInitialBlockDownload() override {
        return chainman().ActiveChainstate().IsInitialBlockDownload();
    }
    bool isMasternode() override
    {
        return m_context->active_ctx != nullptr;
    }
    bool isLoadingBlocks() override { return node::fReindex || node::fImporting; }
    bool isV24Active() override
    {
        LOCK(::cs_main);
        return DeploymentActiveAfter(chainman().ActiveChain().Tip(), chainman(), Consensus::DEPLOYMENT_V24);
    }
    void setNetworkActive(bool active) override
    {
        if (m_context->connman) {
            m_context->connman->SetNetworkActive(active, m_context->mn_sync.get());
        }
    }
    bool getNetworkActive() override { return m_context->connman && m_context->connman->GetNetworkActive(); }
    CFeeRate getDustRelayFee() override
    {
        if (!m_context->mempool) return CFeeRate{DUST_RELAY_TX_FEE};
        return m_context->mempool->m_dust_relay_feerate;
    }
    UniValue executeRpc(const std::string& command, const UniValue& params, const std::string& uri) override
    {
        JSONRPCRequest req;
        req.context = *m_context;
        req.params = params;
        req.strMethod = command;
        req.URI = uri;
        return ::tableRPC.execute(req);
    }
    std::vector<std::string> listRpcCommands() override { return ::tableRPC.listCommands(); }
    void rpcSetTimerInterfaceIfUnset(RPCTimerInterface* iface) override { RPCSetTimerInterfaceIfUnset(iface); }
    void rpcUnsetTimerInterface(RPCTimerInterface* iface) override { RPCUnsetTimerInterface(iface); }
    bool getUnspentOutput(const COutPoint& output, Coin& coin) override
    {
        LOCK(::cs_main);
        return chainman().ActiveChainstate().CoinsTip().GetCoin(output, coin);
    }
    TransactionError broadcastTransaction(CTransactionRef tx, CAmount max_tx_fee, bilingual_str& err_string) override
    {
        return BroadcastTransaction(*m_context, std::move(tx), err_string, max_tx_fee, /*relay=*/ true, /*wait_callback=*/ false);
    }
    WalletLoader& walletLoader() override
    {
        return *Assert(m_context->wallet_loader);
    }

    EVO& evo() override { return m_evo; }
    GOV& gov() override { return m_gov; }
    LLMQ& llmq() override { return m_llmq; }
    Masternode::Sync& masternodeSync() override { return m_masternodeSync; }
    CoinJoin::Options& coinJoinOptions() override { return m_coinjoin; }
    std::unique_ptr<interfaces::CoinJoin::Loader>& coinJoinLoader() override { return m_context->coinjoin_loader; }

    std::unique_ptr<Handler> handleInitMessage(InitMessageFn fn) override
    {
        return MakeSignalHandler(::uiInterface.InitMessage_connect(fn));
    }
    std::unique_ptr<Handler> handleMessageBox(MessageBoxFn fn) override
    {
        return MakeSignalHandler(::uiInterface.ThreadSafeMessageBox_connect(fn));
    }
    std::unique_ptr<Handler> handleQuestion(QuestionFn fn) override
    {
        return MakeSignalHandler(::uiInterface.ThreadSafeQuestion_connect(fn));
    }
    std::unique_ptr<Handler> handleShowProgress(ShowProgressFn fn) override
    {
        return MakeSignalHandler(::uiInterface.ShowProgress_connect(fn));
    }
    std::unique_ptr<Handler> handleInitWallet(InitWalletFn fn) override
    {
        return MakeSignalHandler(::uiInterface.InitWallet_connect(fn));
    }
    std::unique_ptr<Handler> handleNotifyNumConnectionsChanged(NotifyNumConnectionsChangedFn fn) override
    {
        return MakeSignalHandler(::uiInterface.NotifyNumConnectionsChanged_connect(fn));
    }
    std::unique_ptr<Handler> handleNotifyNetworkActiveChanged(NotifyNetworkActiveChangedFn fn) override
    {
        return MakeSignalHandler(::uiInterface.NotifyNetworkActiveChanged_connect(fn));
    }
    std::unique_ptr<Handler> handleNotifyAlertChanged(NotifyAlertChangedFn fn) override
    {
        return MakeSignalHandler(::uiInterface.NotifyAlertChanged_connect(fn));
    }
    std::unique_ptr<Handler> handleBannedListChanged(BannedListChangedFn fn) override
    {
        return MakeSignalHandler(::uiInterface.BannedListChanged_connect(fn));
    }
    std::unique_ptr<Handler> handleNotifyBlockTip(NotifyBlockTipFn fn) override
    {
        return MakeSignalHandler(::uiInterface.NotifyBlockTip_connect([fn](SynchronizationState sync_state, const CBlockIndex* block) {
            fn(sync_state, BlockTip{block->nHeight, block->GetBlockTime(), block->GetBlockHash()},
                GuessVerificationProgress(Params().TxData(), block));
        }));
    }
    std::unique_ptr<Handler> handleNotifyChainLock(NotifyChainLockFn fn) override
    {
        return MakeSignalHandler(::uiInterface.NotifyChainLock_connect([fn](const std::string& bestChainLockHash, int bestChainLockHeight) {
            fn(bestChainLockHash, bestChainLockHeight);
        }));
    }
    std::unique_ptr<Handler> handleNotifyHeaderTip(NotifyHeaderTipFn fn) override
    {
        return MakeSignalHandler(
            ::uiInterface.NotifyHeaderTip_connect([fn](SynchronizationState sync_state, int64_t height, int64_t timestamp) {
                fn(sync_state, BlockTip{(int)height, timestamp, uint256{}});
            }));
    }
    std::unique_ptr<Handler> handleNotifyInstantSendChanged(NotifyInstantSendChangedFn fn) override
    {
        return MakeSignalHandler(::uiInterface.NotifyInstantSendChanged_connect(fn));
    }
    std::unique_ptr<Handler> handleNotifyGovernanceChanged(NotifyGovernanceChangedFn fn) override
    {
        return MakeSignalHandler(::uiInterface.NotifyGovernanceChanged_connect(fn));
    }
    std::unique_ptr<Handler> handleNotifyMasternodeListChanged(NotifyMasternodeListChangedFn fn) override
    {
        return MakeSignalHandler(
            ::uiInterface.NotifyMasternodeListChanged_connect([fn](const CDeterministicMNList& newList, const CBlockIndex* pindex) {
                fn(newList, pindex);
            }));
    }
    std::unique_ptr<Handler> handleNotifyAdditionalDataSyncProgressChanged(NotifyAdditionalDataSyncProgressChangedFn fn) override
    {
        return MakeSignalHandler(
            ::uiInterface.NotifyAdditionalDataSyncProgressChanged_connect([fn](double nSyncProgress) {
                fn(nSyncProgress);
            }));
    }
    NodeContext* context() override { return m_context; }
    void setContext(NodeContext* context) override
    {
        m_context = context;
        m_evo.setContext(context);
        m_gov.setContext(context);
        m_llmq.setContext(context);
        m_masternodeSync.setContext(context);
    }
    ChainstateManager& chainman() { return *Assert(m_context->chainman); }
    NodeContext* m_context{nullptr};
};

bool FillBlock(const CBlockIndex* index, const FoundBlock& block, UniqueLock<RecursiveMutex>& lock, const CChain& active)
{
    if (!index) return false;
    if (block.m_hash) *block.m_hash = index->GetBlockHash();
    if (block.m_height) *block.m_height = index->nHeight;
    if (block.m_time) *block.m_time = index->GetBlockTime();
    if (block.m_max_time) *block.m_max_time = index->GetBlockTimeMax();
    if (block.m_mtp_time) *block.m_mtp_time = index->GetMedianTimePast();
    if (block.m_in_active_chain) *block.m_in_active_chain = active[index->nHeight] == index;
    if (block.m_locator) { *block.m_locator = GetLocator(index); }
    if (block.m_next_block) FillBlock(active[index->nHeight] == index ? active[index->nHeight + 1] : nullptr, *block.m_next_block, lock, active);
    if (block.m_data) {
        REVERSE_LOCK(lock);
        if (!ReadBlockFromDisk(*block.m_data, index, Params().GetConsensus())) block.m_data->SetNull();
    }
    block.found = true;
    return true;
}

class NotificationsProxy : public CValidationInterface
{
public:
    explicit NotificationsProxy(std::shared_ptr<Chain::Notifications> notifications)
        : m_notifications(std::move(notifications)) {}
    virtual ~NotificationsProxy() = default;
    void TransactionAddedToMempool(const CTransactionRef& tx, int64_t nAcceptTime, uint64_t mempool_sequence) override
    {
        m_notifications->transactionAddedToMempool(tx, nAcceptTime);
    }
    void TransactionRemovedFromMempool(const CTransactionRef& tx, MemPoolRemovalReason reason, uint64_t mempool_sequence) override
    {
        m_notifications->transactionRemovedFromMempool(tx, reason);
    }
    void BlockConnected(const std::shared_ptr<const CBlock>& block, const CBlockIndex* index) override
    {
        m_notifications->blockConnected(kernel::MakeBlockInfo(index, block.get()));
    }
    void BlockDisconnected(const std::shared_ptr<const CBlock>& block, const CBlockIndex* index) override
    {
        m_notifications->blockDisconnected(kernel::MakeBlockInfo(index, block.get()));
    }
    void UpdatedBlockTip(const CBlockIndex* index, const CBlockIndex* fork_index, bool is_ibd) override
    {
        m_notifications->updatedBlockTip();
    }
    void ChainStateFlushed(const CBlockLocator& locator) override { m_notifications->chainStateFlushed(locator); }
    void NotifyChainLock(const CBlockIndex* pindexChainLock, const std::shared_ptr<const chainlock::ChainLockSig>& clsig) override
    {
        m_notifications->notifyChainLock(pindexChainLock, clsig);
    }
    void NotifyTransactionLock(const CTransactionRef &tx, const std::shared_ptr<const instantsend::InstantSendLock>& islock) override
    {
        m_notifications->notifyTransactionLock(tx, islock);
    }
    std::shared_ptr<Chain::Notifications> m_notifications;
};

class NotificationsHandlerImpl : public Handler
{
public:
    explicit NotificationsHandlerImpl(std::shared_ptr<Chain::Notifications> notifications)
        : m_proxy(std::make_shared<NotificationsProxy>(std::move(notifications)))
    {
        RegisterSharedValidationInterface(m_proxy);
    }
    ~NotificationsHandlerImpl() override { disconnect(); }
    void disconnect() override
    {
        if (m_proxy) {
            UnregisterSharedValidationInterface(m_proxy);
            m_proxy.reset();
        }
    }
    std::shared_ptr<NotificationsProxy> m_proxy;
};

class RpcHandlerImpl : public Handler
{
public:
    explicit RpcHandlerImpl(const CRPCCommand& command) : m_command(command), m_wrapped_command(&command)
    {
        m_command.actor = [this](const JSONRPCRequest& request, UniValue& result, bool last_handler) {
            if (!m_wrapped_command) return false;
            try {
                return m_wrapped_command->actor(request, result, last_handler);
            } catch (const UniValue& e) {
                // If this is not the last handler and a wallet not found
                // exception was thrown, return false so the next handler can
                // try to handle the request. Otherwise, reraise the exception.
                if (!last_handler) {
                    const UniValue& code = e["code"];
                    if (code.isNum() && code.getInt<int>() == RPC_WALLET_NOT_FOUND) {
                        return false;
                    }
                }
                throw;
            }
        };
        ::tableRPC.appendCommand(m_command.name, &m_command);
    }

    void disconnect() override final
    {
        if (m_wrapped_command) {
            m_wrapped_command = nullptr;
            ::tableRPC.removeCommand(m_command.name, &m_command);
        }
    }

    ~RpcHandlerImpl() override { disconnect(); }

    CRPCCommand m_command;
    const CRPCCommand* m_wrapped_command;
};

class ChainImpl : public Chain
{
public:
    explicit ChainImpl(NodeContext& node) : m_node(node) {}
    std::optional<int> getHeight() override
    {
        const int height{WITH_LOCK(::cs_main, return chainman().ActiveChain().Height())};
        return height >= 0 ? std::optional{height} : std::nullopt;
    }
    uint256 getBlockHash(int height) override
    {
        LOCK(::cs_main);
        return Assert(chainman().ActiveChain()[height])->GetBlockHash();
    }
    bool haveBlockOnDisk(int height) override
    {
        LOCK(::cs_main);
        const CBlockIndex* block{chainman().ActiveChain()[height]};
        return block && ((block->nStatus & BLOCK_HAVE_DATA) != 0) && block->nTx > 0;
    }
    std::optional<int> findFork(const uint256& hash, std::optional<int>* height) override
    {
        LOCK(cs_main);
        const CChain& active = chainman().ActiveChain();
        const CBlockIndex* block = chainman().m_blockman.LookupBlockIndex(hash);
        const CBlockIndex* fork = block ? active.FindFork(block) : nullptr;
        if (height) {
            if (block) {
                *height = block->nHeight;
            } else {
                height->reset();
            }
        }
        if (fork) {
            return fork->nHeight;
        }
        return std::nullopt;
    }
    CBlockLocator getTipLocator() override
    {
        LOCK(::cs_main);
        return chainman().ActiveChain().GetLocator();
    }
    CBlockLocator getActiveChainLocator(const uint256& block_hash) override
    {
        LOCK(::cs_main);
        const CBlockIndex* index = chainman().m_blockman.LookupBlockIndex(block_hash);
        return GetLocator(index);
    }
    std::optional<int> findLocatorFork(const CBlockLocator& locator) override
    {
        LOCK(::cs_main);
        if (const CBlockIndex* fork = chainman().ActiveChainstate().FindForkInGlobalIndex(locator)) {
            return fork->nHeight;
        }
        return std::nullopt;
    }
    bool hasBlockFilterIndex(BlockFilterType filter_type) override
    {
        return GetBlockFilterIndex(filter_type) != nullptr;
    }
    std::optional<bool> blockFilterMatchesAny(BlockFilterType filter_type, const uint256& block_hash, const GCSFilter::ElementSet& filter_set) override
    {
        const BlockFilterIndex* block_filter_index{GetBlockFilterIndex(filter_type)};
        if (!block_filter_index) return std::nullopt;

        BlockFilter filter;
        const CBlockIndex* index{WITH_LOCK(::cs_main, return chainman().m_blockman.LookupBlockIndex(block_hash))};
        if (index == nullptr || !block_filter_index->LookupFilter(index, filter)) return std::nullopt;
        return filter.GetFilter().MatchAny(filter_set);
    }
    bool isInstantSendLockedTx(const uint256& hash) override
    {
        if (m_node.isman == nullptr) return false;
        return m_node.isman->IsLocked(hash);
    }
    bool hasChainLock(int height, const uint256& hash) override
    {
        if (m_node.chainlocks == nullptr) return false;
        return m_node.chainlocks->HasChainLock(height, hash);
    }
    std::vector<COutPoint> listMNCollaterials(const std::vector<std::pair<const CTransactionRef&, uint32_t>>& outputs) override
    {
        const CBlockIndex *tip = WITH_LOCK(::cs_main, return chainman().ActiveChain().Tip());
        CDeterministicMNList mnList{};
        if  (tip != nullptr && m_node.dmnman != nullptr) {
            mnList = m_node.dmnman->GetListForBlock(tip);
        }
        std::vector<COutPoint> listRet;
        for (const auto& [tx, index]: outputs) {
            COutPoint nextOut{tx->GetHash(), index};
            if (CDeterministicMNManager::IsProTxWithCollateral(tx, index) || mnList.HasMNByCollateral(nextOut)) {
                listRet.emplace_back(nextOut);
            }
        }
        return listRet;
    }
    bool findBlock(const uint256& hash, const FoundBlock& block) override
    {
        WAIT_LOCK(cs_main, lock);
        return FillBlock(chainman().m_blockman.LookupBlockIndex(hash), block, lock, chainman().ActiveChain());
    }
    bool findFirstBlockWithTimeAndHeight(int64_t min_time, int min_height, const FoundBlock& block) override
    {
        WAIT_LOCK(cs_main, lock);
        const CChain& active = chainman().ActiveChain();
        return FillBlock(active.FindEarliestAtLeast(min_time, min_height), block, lock, active);
    }
    bool findAncestorByHeight(const uint256& block_hash, int ancestor_height, const FoundBlock& ancestor_out) override
    {
        WAIT_LOCK(cs_main, lock);
        const CChain& active = chainman().ActiveChain();
        if (const CBlockIndex* block = chainman().m_blockman.LookupBlockIndex(block_hash)) {
            if (const CBlockIndex* ancestor = block->GetAncestor(ancestor_height)) {
                return FillBlock(ancestor, ancestor_out, lock, active);
            }
        }
        return FillBlock(nullptr, ancestor_out, lock, active);
    }
    bool findAncestorByHash(const uint256& block_hash, const uint256& ancestor_hash, const FoundBlock& ancestor_out) override
    {
        WAIT_LOCK(cs_main, lock);
        const CBlockIndex* block = chainman().m_blockman.LookupBlockIndex(block_hash);
        const CBlockIndex* ancestor = chainman().m_blockman.LookupBlockIndex(ancestor_hash);
        if (block && ancestor && block->GetAncestor(ancestor->nHeight) != ancestor) ancestor = nullptr;
        return FillBlock(ancestor, ancestor_out, lock, chainman().ActiveChain());
    }
    bool findCommonAncestor(const uint256& block_hash1, const uint256& block_hash2, const FoundBlock& ancestor_out, const FoundBlock& block1_out, const FoundBlock& block2_out) override
    {
        WAIT_LOCK(cs_main, lock);
        const CChain& active = chainman().ActiveChain();
        const CBlockIndex* block1 = chainman().m_blockman.LookupBlockIndex(block_hash1);
        const CBlockIndex* block2 = chainman().m_blockman.LookupBlockIndex(block_hash2);
        const CBlockIndex* ancestor = block1 && block2 ? LastCommonAncestor(block1, block2) : nullptr;
        // Using & instead of && below to avoid short circuiting and leaving
        // output uninitialized. Cast bool to int to avoid -Wbitwise-instead-of-logical
        // compiler warnings.
        return int{FillBlock(ancestor, ancestor_out, lock, active)} &
               int{FillBlock(block1, block1_out, lock, active)} &
               int{FillBlock(block2, block2_out, lock, active)};
    }
    void findCoins(std::map<COutPoint, Coin>& coins) override { return FindCoins(m_node, coins); }
    double guessVerificationProgress(const uint256& block_hash) override
    {
        LOCK(::cs_main);
        return GuessVerificationProgress(chainman().GetParams().TxData(), chainman().m_blockman.LookupBlockIndex(block_hash));
    }
    bool hasBlocks(const uint256& block_hash, int min_height, std::optional<int> max_height) override
    {
        // hasBlocks returns true if all ancestors of block_hash in specified
        // range have block data (are not pruned), false if any ancestors in
        // specified range are missing data.
        //
        // For simplicity and robustness, min_height and max_height are only
        // used to limit the range, and passing min_height that's too low or
        // max_height that's too high will not crash or change the result.
        LOCK(::cs_main);
        if (const CBlockIndex* block = chainman().m_blockman.LookupBlockIndex(block_hash)) {
            if (max_height && block->nHeight >= *max_height) block = block->GetAncestor(*max_height);
            for (; block->nStatus & BLOCK_HAVE_DATA; block = block->pprev) {
                // Check pprev to not segfault if min_height is too low
                if (block->nHeight <= min_height || !block->pprev) return true;
            }
        }
        return false;
    }
    bool isInMempool(const uint256& txid) override
    {
        if (!m_node.mempool) return false;
        LOCK(m_node.mempool->cs);
        return m_node.mempool->exists(txid);
    }
    bool hasDescendantsInMempool(const uint256& txid) override
    {
        if (!m_node.mempool) return false;
        LOCK(m_node.mempool->cs);
        auto it = m_node.mempool->GetIter(txid);
        return it && (*it)->GetCountWithDescendants() > 1;
    }
    bool broadcastTransaction(const CTransactionRef& tx, const CAmount& max_tx_fee, bool relay, bilingual_str& err_string) override
    {
        const TransactionError err = BroadcastTransaction(m_node, tx, err_string, max_tx_fee, relay, /*wait_callback=*/false);
        // Chain clients only care about failures to accept the tx to the mempool. Disregard non-mempool related failures.
        // Note: this will need to be updated if BroadcastTransactions() is updated to return other non-mempool failures
        // that Chain clients do not need to know about.
        return TransactionError::OK == err;
    }
    void getTransactionAncestry(const uint256& txid, size_t& ancestors, size_t& descendants, size_t* ancestorsize, CAmount* ancestorfees) override
    {
        ancestors = descendants = 0;
        if (!m_node.mempool) return;
        m_node.mempool->GetTransactionAncestry(txid, ancestors, descendants, ancestorsize, ancestorfees);
    }
    void getPackageLimits(unsigned int& limit_ancestor_count, unsigned int& limit_descendant_count) override
    {
        const CTxMemPool::Limits default_limits{};

        const CTxMemPool::Limits& limits{m_node.mempool ? m_node.mempool->m_limits : default_limits};

        limit_ancestor_count = limits.ancestor_count;
        limit_descendant_count = limits.descendant_count;
    }
    util::Result<void> checkChainLimits(const CTransactionRef& tx) override
    {
        if (!m_node.mempool) return {};
        LockPoints lp;
        CTxMemPoolEntry entry(tx, 0, 0, 0, false, 0, lp);
        const CTxMemPool::Limits& limits{m_node.mempool->m_limits};
        LOCK(m_node.mempool->cs);
        auto ancestors{m_node.mempool->CalculateMemPoolAncestors(entry, limits)};
        if (!ancestors) return util::Error{util::ErrorString(ancestors)};
        return {};
    }
    CFeeRate estimateSmartFee(int num_blocks, bool conservative, FeeCalculation* calc) override
    {
        if (!m_node.fee_estimator) return {};
        return m_node.fee_estimator->estimateSmartFee(num_blocks, calc, conservative);
    }
    unsigned int estimateMaxBlocks() override
    {
        if (!m_node.fee_estimator) return 0;
        return m_node.fee_estimator->HighestTargetTracked(FeeEstimateHorizon::LONG_HALFLIFE);
    }
    CFeeRate mempoolMinFee() override
    {
        if (!m_node.mempool) return {};
        return m_node.mempool->GetMinFee();
    }
    CFeeRate relayMinFee() override
    {
        if (!m_node.mempool) return CFeeRate{DEFAULT_MIN_RELAY_TX_FEE};
        return m_node.mempool->m_min_relay_feerate;
    }
    CFeeRate relayIncrementalFee() override
    {
        if (!m_node.mempool) return CFeeRate{DEFAULT_INCREMENTAL_RELAY_FEE};
        return m_node.mempool->m_incremental_relay_feerate;
    }
    CFeeRate relayDustFee() override
    {
        if (!m_node.mempool) return CFeeRate{DUST_RELAY_TX_FEE};
        return m_node.mempool->m_dust_relay_feerate;
    }
    bool isNonStandardSpecialTx(const CTransactionRef& tx, std::string& reason) override
    {
        if (!m_node.mempool || !m_node.mempool->m_require_standard) return false;
        return !IsStandardSpecialTx(*tx, reason);
    }
    bool isV24Active() override
    {
        LOCK(::cs_main);
        return DeploymentActiveAfter(chainman().ActiveChain().Tip(), chainman(), Consensus::DEPLOYMENT_V24);
    }
    bool havePruned() override
    {
        LOCK(::cs_main);
        return chainman().m_blockman.m_have_pruned;
    }
    bool isReadyToBroadcast() override { return !chainman().m_blockman.LoadingBlocks() && !isInitialBlockDownload(); }
    bool isInitialBlockDownload() override
    {
        return chainman().ActiveChainstate().IsInitialBlockDownload();
    }
    bool shutdownRequested() override { return ShutdownRequested(); }
    void initMessage(const std::string& message) override { ::uiInterface.InitMessage(message); }
    void initWarning(const bilingual_str& message) override { InitWarning(message); }
    void initError(const bilingual_str& message) override { InitError(message); }
    void showProgress(const std::string& title, int progress, bool resume_possible) override
    {
        ::uiInterface.ShowProgress(title, progress, resume_possible);
    }
    std::unique_ptr<Handler> handleNotifications(std::shared_ptr<Notifications> notifications) override
    {
        return std::make_unique<NotificationsHandlerImpl>(std::move(notifications));
    }
    void waitForNotificationsIfTipChanged(const uint256& old_tip) override
    {
        if (!old_tip.IsNull() && old_tip == WITH_LOCK(::cs_main, return chainman().ActiveChain().Tip()->GetBlockHash())) return;
        SyncWithValidationInterfaceQueue();
    }
    std::unique_ptr<Handler> handleRpc(const CRPCCommand& command) override
    {
        return std::make_unique<RpcHandlerImpl>(command);
    }
    bool rpcEnableDeprecated(const std::string& method) override { return IsDeprecatedRPCEnabled(method); }
    void rpcRunLater(const std::string& name, std::function<void()> fn, int64_t seconds) override
    {
        RPCRunLater(name, std::move(fn), seconds);
    }
    util::SettingsValue getSetting(const std::string& name) override
    {
        return gArgs.GetSetting(name);
    }
    std::vector<util::SettingsValue> getSettingsList(const std::string& name) override
    {
        return gArgs.GetSettingsList(name);
    }
    util::SettingsValue getRwSetting(const std::string& name) override
    {
        util::SettingsValue result;
        gArgs.LockSettings([&](const util::Settings& settings) {
            if (const util::SettingsValue* value = util::FindKey(settings.rw_settings, name)) {
                result = *value;
            }
        });
        return result;
    }
    bool updateRwSetting(const std::string& name, const util::SettingsValue& value, bool write) override
    {
        gArgs.LockSettings([&](util::Settings& settings) {
            if (value.isNull()) {
                settings.rw_settings.erase(name);
            } else {
                settings.rw_settings[name] = value;
            }
        });
        return !write || gArgs.WriteSettingsFile();
    }
    void requestMempoolTransactions(Notifications& notifications) override
    {
        if (!m_node.mempool) return;
        LOCK2(::cs_main, m_node.mempool->cs);
        for (const CTxMemPoolEntry& entry : m_node.mempool->mapTx) {
            notifications.transactionAddedToMempool(entry.GetSharedTx(), /*nAcceptTime=*/0);
        }
    }
    bool hasAssumedValidChain() override
    {
        return chainman().IsSnapshotActive();
    }

    NodeContext* context() override { return &m_node; }
    ChainstateManager& chainman() { return *Assert(m_node.chainman); }
    NodeContext& m_node;
};
} // namespace
} // namespace node

namespace interfaces {
std::unique_ptr<Node> MakeNode(node::NodeContext& context) { return std::make_unique<node::NodeImpl>(context); }
std::unique_ptr<Chain> MakeChain(node::NodeContext& node) { return std::make_unique<node::ChainImpl>(node); }
} // namespace interfaces
