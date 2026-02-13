// Copyright (c) 2018-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/node.h>

#include <addrdb.h>
#include <banman.h>
#include <chain.h>
#include <chainlock/chainlock.h>
#include <chainparams.h>
#include <coinjoin/common.h>
#include <deploymentstatus.h>
#include <evo/deterministicmns.h>
#include <governance/classes.h>
#include <governance/exceptions.h>
#include <external_signer.h>
#include <governance/governance.h>
#include <governance/object.h>
#include <governance/vote.h>
#include <init.h>
#include <interfaces/chain.h>
#include <interfaces/coinjoin.h>
#include <interfaces/handler.h>
#include <interfaces/wallet.h>
#include <instantsend/instantsend.h>
#include <llmq/context.h>
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
#include <util/translation.h>
#include <validation.h>
#include <validationinterface.h>
#include <warnings.h>

#include <governance/validators.h>

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <coinjoin/coinjoin.h>
#include <coinjoin/options.h>

#include <univalue.h>

#include <boost/signals2/signal.hpp>

#include <memory>
#include <optional>
#include <utility>
#include <variant>

using interfaces::BlockTip;
using interfaces::Chain;
using interfaces::EVO;
using interfaces::FoundBlock;
using interfaces::GOV;
using interfaces::Handler;
using interfaces::LLMQ;
using interfaces::MakeHandler;
using interfaces::MnEntry;
using interfaces::MnEntryCPtr;
using interfaces::MnList;
using interfaces::MnListPtr;
using interfaces::Node;
using interfaces::WalletLoader;

namespace node {
namespace {
class MnEntryImpl : public MnEntry
{
private:
    CDeterministicMNCPtr m_dmn;

public:
    MnEntryImpl(const CDeterministicMNCPtr& dmn) :
        MnEntry{dmn},
        m_dmn{Assert(dmn)}
    {
    }
    ~MnEntryImpl() = default;

    bool isBanned() const override { return m_dmn->pdmnState->IsBanned(); }

    CService getNetInfoPrimary() const override { return m_dmn->pdmnState->netInfo->GetPrimary(); }
    MnType getType() const override { return m_dmn->nType; }
    UniValue toJson() const override { return m_dmn->ToJson(); }
    const CKeyID& getKeyIdOwner() const override { return m_dmn->pdmnState->keyIDOwner; }
    const CKeyID& getKeyIdVoting() const override { return m_dmn->pdmnState->keyIDVoting; }
    const COutPoint& getCollateralOutpoint() const override { return m_dmn->collateralOutpoint; }
    const CScript& getScriptPayout() const override { return m_dmn->pdmnState->scriptPayout; }
    const CScript& getScriptOperatorPayout() const override { return m_dmn->pdmnState->scriptOperatorPayout; }
    const int32_t& getLastPaidHeight() const override { return m_dmn->pdmnState->nLastPaidHeight; }
    const int32_t& getPoSePenalty() const override { return m_dmn->pdmnState->nPoSePenalty; }
    const int32_t& getRegisteredHeight() const override { return m_dmn->pdmnState->nRegisteredHeight; }
    const uint16_t& getOperatorReward() const override { return m_dmn->nOperatorReward; }
    const uint256& getProTxHash() const override { return m_dmn->proTxHash; }
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

    int32_t getHeight() const override { return m_list.GetHeight(); }
    size_t getAllEvoCount() const override { return m_list.GetAllEvoCount(); }
    size_t getAllMNsCount() const override { return m_list.GetAllMNsCount(); }
    size_t getValidEvoCount() const override { return m_list.GetValidEvoCount(); }
    size_t getValidMNsCount() const override { return m_list.GetValidMNsCount(); }
    size_t getValidWeightedMNsCount() const override { return m_list.GetValidWeightedMNsCount(); }
    uint256 getBlockHash() const override { return m_list.GetBlockHash(); }

    void forEachMN(bool only_valid, std::function<void(const MnEntry&)> cb) const override
    {
        m_list.ForEachMNShared(only_valid, [&cb](const auto& dmn) {
            cb(MnEntryImpl{dmn});
        });
    }
    MnEntryCPtr getMN(const uint256& hash) const override
    {
        const auto dmn{m_list.GetMN(hash)};
        return dmn ? std::make_unique<const MnEntryImpl>(dmn) : nullptr;
    }
    MnEntryCPtr getMNByService(const CService& service) const override
    {
        const auto dmn{m_list.GetMNByService(service)};
        return dmn ? std::make_unique<const MnEntryImpl>(dmn) : nullptr;
    }
    MnEntryCPtr getValidMN(const uint256& hash) const override
    {
        const auto dmn{m_list.GetValidMN(hash)};
        return dmn ? std::make_unique<const MnEntryImpl>(dmn) : nullptr;
    }
    std::vector<MnEntryCPtr> getProjectedMNPayees(const CBlockIndex* pindex) const override
    {
        std::vector<MnEntryCPtr> ret;
        for (const auto& payee : m_list.GetProjectedMNPayees(pindex)) {
            ret.emplace_back(std::make_unique<const MnEntryImpl>(payee));
        }
        return ret;
    }

    void copyContextTo(MnList& mn_list) const override
    {
        if (!m_context) return;
        mn_list.setContext(m_context);
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
        const CBlockIndex *tip = WITH_LOCK(::cs_main, return chainman().ActiveChain().Tip());
        MnListImpl mnList{CDeterministicMNList{}};
        if (tip != nullptr && context().dmnman != nullptr) {
            mnList = context().dmnman->GetListForBlock(tip);
        }
        mnList.setContext(m_context);
        return {std::make_shared<MnListImpl>(mnList), tip};
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
    bool existsObj(const uint256& hash) override
    {
        if (context().govman != nullptr) {
            return context().govman->HaveObjectForHash(hash);
        }
        return false;
    }
    bool getObjLocalValidity(const CGovernanceObject& obj, std::string& error, bool check_collateral) override
    {
        if (context().govman != nullptr && context().chainman != nullptr && context().dmnman != nullptr) {
            LOCK(cs_main);
            return obj.IsValidLocally(context().dmnman->GetListAtChainTip(), *(context().chainman), error, check_collateral);
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
        if (context().govman != nullptr && context().connman != nullptr) {
            CGovernanceException exception;
            bool result = context().govman->ProcessVoteAndRelay(vote, exception, *context().connman);
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
        const Consensus::Params& consensusParams = Params().GetConsensus();
        if (context().chainman) {
            LOCK(::cs_main);
            CSuperblock::GetNearestSuperblocksHeights(context().chainman->ActiveHeight(), info.lastsuperblock, info.nextsuperblock);
            info.governancebudget = CSuperblock::GetPaymentsLimit(context().chainman->ActiveChain(), info.nextsuperblock);
            if (context().dmnman) {
                info.fundingthreshold = context().dmnman->GetListAtChainTip().GetValidWeightedMNsCount() / 10;
            }
        }
        info.proposalfee = GOVERNANCE_PROPOSAL_FEE_TX;
        info.superblockcycle = consensusParams.nSuperblockCycle;
        info.superblockmaturitywindow = consensusParams.nSuperblockMaturityWindow;
        info.targetSpacing = consensusParams.nPowTargetSpacing;
        info.relayRequiredConfs = GOVERNANCE_MIN_RELAY_FEE_CONFIRMATIONS;
        info.requiredConfs = GOVERNANCE_FEE_CONFIRMATIONS;
        return info;
    }
    std::optional<int32_t> getProposalFundedHeight(const uint256& proposal_hash) override
    {
        if (context().govman != nullptr && context().chainman != nullptr) {
            const int32_t nTipHeight = context().chainman->ActiveHeight();
            for (const auto& trigger : context().govman->GetActiveTriggers()) {
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
                CSuperblock::GetNearestSuperblocksHeights(context().chainman->ActiveHeight(), last_sb, next_sb);
                const CAmount budget{CSuperblock::GetPaymentsLimit(context().chainman->ActiveChain(), next_sb)};
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
        CProposalValidator validator(data_hex);
        if (!validator.Validate()) {
            error = "Invalid proposal data: " + validator.GetErrorMessages();
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
            CProposalValidator validator(data_hex);
            if (!validator.Validate()) { error = "Invalid proposal data: " + validator.GetErrorMessages(); return false; }
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
            context().govman->AddGovernanceObject(govobj);
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
    size_t getInstantSentLockCount() override
    {
        if (context().llmq_ctx && context().llmq_ctx->isman != nullptr) {
            return context().llmq_ctx->isman->GetInstantSendLockCount();
        }
        return 0;
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
private:
    ::ExternalSigner m_signer;
};
#endif

class NodeImpl : public Node
{
private:
    ChainstateManager& chainman() { return *Assert(m_context->chainman); }
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
    uint64_t getLogCategories() override { return LogInstance().GetCategoryMask(); }
    bool baseInitialize() override
    {
        return AppInitBasicSetup(gArgs) && AppInitParameterInteraction(gArgs) && AppInitSanityChecks() &&
               AppInitLockDataDirectory() && AppInitInterfaces(*m_context);
    }
    bool appInitMain(interfaces::BlockAndHeaderTipInfo* tip_info) override
    {
        return AppInitMain(*m_context, tip_info);
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
    size_t getMempoolMaxUsage() override { return gArgs.GetIntArg("-maxmempool", DEFAULT_MAX_MEMPOOL_SIZE) * 1000000; }
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
        return tip ? tip->GetBlockHash() : Params().GenesisBlock().GetHash();
    }
    int64_t getLastBlockTime() override
    {
        LOCK(::cs_main);
        if (chainman().ActiveChain().Tip()) {
            return chainman().ActiveChain().Tip()->GetBlockTime();
        }
        return Params().GenesisBlock().GetBlockTime(); // Genesis block's time of current network
    }
    std::string getLastBlockHash() override
    {
        LOCK(::cs_main);
        if (m_context->chainman->ActiveChain().Tip()) {
            return m_context->chainman->ActiveChain().Tip()->GetBlockHash().ToString();
        }
        return Params().GenesisBlock().GetHash().ToString(); // Genesis block's hash of current network
    }
    double getVerificationProgress() override
    {
        const CBlockIndex* tip;
        {
            LOCK(::cs_main);
            tip = chainman().ActiveChain().Tip();
        }
        return GuessVerificationProgress(Params().TxData(), tip);
    }
    bool isInitialBlockDownload() override {
        return chainman().ActiveChainstate().IsInitialBlockDownload();
    }
    bool isMasternode() override
    {
        return m_context->active_ctx != nullptr;
    }
    bool isLoadingBlocks() override { return node::fReindex || node::fImporting; }
    void setNetworkActive(bool active) override
    {
        if (m_context->connman) {
            m_context->connman->SetNetworkActive(active, m_context->mn_sync.get());
        }
    }
    bool getNetworkActive() override { return m_context->connman && m_context->connman->GetNetworkActive(); }
    CFeeRate getDustRelayFee() override { return ::dustRelayFee; }
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
        return MakeHandler(::uiInterface.InitMessage_connect(fn));
    }
    std::unique_ptr<Handler> handleMessageBox(MessageBoxFn fn) override
    {
        return MakeHandler(::uiInterface.ThreadSafeMessageBox_connect(fn));
    }
    std::unique_ptr<Handler> handleQuestion(QuestionFn fn) override
    {
        return MakeHandler(::uiInterface.ThreadSafeQuestion_connect(fn));
    }
    std::unique_ptr<Handler> handleShowProgress(ShowProgressFn fn) override
    {
        return MakeHandler(::uiInterface.ShowProgress_connect(fn));
    }
    std::unique_ptr<Handler> handleInitWallet(InitWalletFn fn) override
    {
        return MakeHandler(::uiInterface.InitWallet_connect(fn));
    }
    std::unique_ptr<Handler> handleNotifyNumConnectionsChanged(NotifyNumConnectionsChangedFn fn) override
    {
        return MakeHandler(::uiInterface.NotifyNumConnectionsChanged_connect(fn));
    }
    std::unique_ptr<Handler> handleNotifyNetworkActiveChanged(NotifyNetworkActiveChangedFn fn) override
    {
        return MakeHandler(::uiInterface.NotifyNetworkActiveChanged_connect(fn));
    }
    std::unique_ptr<Handler> handleNotifyAlertChanged(NotifyAlertChangedFn fn) override
    {
        return MakeHandler(::uiInterface.NotifyAlertChanged_connect(fn));
    }
    std::unique_ptr<Handler> handleBannedListChanged(BannedListChangedFn fn) override
    {
        return MakeHandler(::uiInterface.BannedListChanged_connect(fn));
    }
    std::unique_ptr<Handler> handleNotifyBlockTip(NotifyBlockTipFn fn) override
    {
        return MakeHandler(::uiInterface.NotifyBlockTip_connect([fn](SynchronizationState sync_state, const CBlockIndex* block) {
            fn(sync_state, BlockTip{block->nHeight, block->GetBlockTime(), block->GetBlockHash()},
                GuessVerificationProgress(Params().TxData(), block));
        }));
    }
    std::unique_ptr<Handler> handleNotifyChainLock(NotifyChainLockFn fn) override
    {
        return MakeHandler(::uiInterface.NotifyChainLock_connect([fn](const std::string& bestChainLockHash, int bestChainLockHeight) {
            fn(bestChainLockHash, bestChainLockHeight);
        }));
    }
    std::unique_ptr<Handler> handleNotifyHeaderTip(NotifyHeaderTipFn fn) override
    {
        return MakeHandler(
            ::uiInterface.NotifyHeaderTip_connect([fn](SynchronizationState sync_state, const CBlockIndex* block) {
                fn(sync_state, BlockTip{block->nHeight, block->GetBlockTime(), block->GetBlockHash()},
                    /* verification progress is unused when a header was received */ 0);
            }));
    }
    std::unique_ptr<Handler> handleNotifyGovernanceChanged(NotifyGovernanceChangedFn fn) override
    {
        return MakeHandler(::uiInterface.NotifyGovernanceChanged_connect(fn));
    }
    std::unique_ptr<Handler> handleNotifyMasternodeListChanged(NotifyMasternodeListChangedFn fn) override
    {
        return MakeHandler(
            ::uiInterface.NotifyMasternodeListChanged_connect([fn](const CDeterministicMNList& newList, const CBlockIndex* pindex) {
                fn(newList, pindex);
            }));
    }
    std::unique_ptr<Handler> handleNotifyAdditionalDataSyncProgressChanged(NotifyAdditionalDataSyncProgressChangedFn fn) override
    {
        return MakeHandler(
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
        m_notifications->blockConnected(*block, index->nHeight);
    }
    void BlockDisconnected(const std::shared_ptr<const CBlock>& block, const CBlockIndex* index) override
    {
        m_notifications->blockDisconnected(*block, index->nHeight);
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
private:
    ChainstateManager& chainman() { return *Assert(m_node.chainman); }
public:
    explicit ChainImpl(NodeContext& node) : m_node(node) {}
    std::optional<int> getHeight() override
    {
        LOCK(::cs_main);
        const CChain& active = chainman().ActiveChain();
        int height = active.Height();
        if (height >= 0) {
            return height;
        }
        return std::nullopt;
    }
    uint256 getBlockHash(int height) override
    {
        LOCK(::cs_main);
        const CChain& active = chainman().ActiveChain();
        CBlockIndex* block = active[height];
        assert(block != nullptr);
        return block->GetBlockHash();
    }
    bool haveBlockOnDisk(int height) override
    {
        LOCK(::cs_main);
        const CChain& active = chainman().ActiveChain();
        CBlockIndex* block = active[height];
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
        if (!index) return {};
        return chainman().ActiveChain().GetLocator(index);
    }
    std::optional<int> findLocatorFork(const CBlockLocator& locator) override
    {
        LOCK(::cs_main);
        const CChainState& active = chainman().ActiveChainstate();
        if (const CBlockIndex* fork = active.FindForkInGlobalIndex(locator)) {
            return fork->nHeight;
        }
        return std::nullopt;
    }
    bool isInstantSendLockedTx(const uint256& hash) override
    {
        if (m_node.llmq_ctx == nullptr || m_node.llmq_ctx->isman == nullptr) return false;
        return m_node.llmq_ctx->isman->IsLocked(hash);
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
        const CChain& active = chainman().ActiveChain();
        return FillBlock(chainman().m_blockman.LookupBlockIndex(hash), block, lock, active);
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
        const CChain& active = chainman().ActiveChain();
        const CBlockIndex* block = chainman().m_blockman.LookupBlockIndex(block_hash);
        const CBlockIndex* ancestor = chainman().m_blockman.LookupBlockIndex(ancestor_hash);
        if (block && ancestor && block->GetAncestor(ancestor->nHeight) != ancestor) ancestor = nullptr;
        return FillBlock(ancestor, ancestor_out, lock, active);
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
        return GuessVerificationProgress(Params().TxData(), chainman().m_blockman.LookupBlockIndex(block_hash));
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
        limit_ancestor_count = gArgs.GetIntArg("-limitancestorcount", DEFAULT_ANCESTOR_LIMIT);
        limit_descendant_count = gArgs.GetIntArg("-limitdescendantcount", DEFAULT_DESCENDANT_LIMIT);
    }
    bool checkChainLimits(const CTransactionRef& tx) override
    {
        if (!m_node.mempool) return true;
        LockPoints lp;
        CTxMemPoolEntry entry(tx, 0, 0, 0, false, 0, lp);
        CTxMemPool::setEntries ancestors;
        auto limit_ancestor_count = gArgs.GetIntArg("-limitancestorcount", DEFAULT_ANCESTOR_LIMIT);
        auto limit_ancestor_size = gArgs.GetIntArg("-limitancestorsize", DEFAULT_ANCESTOR_SIZE_LIMIT) * 1000;
        auto limit_descendant_count = gArgs.GetIntArg("-limitdescendantcount", DEFAULT_DESCENDANT_LIMIT);
        auto limit_descendant_size = gArgs.GetIntArg("-limitdescendantsize", DEFAULT_DESCENDANT_SIZE_LIMIT) * 1000;
        std::string unused_error_string;
        LOCK(m_node.mempool->cs);
        return m_node.mempool->CalculateMemPoolAncestors(
            entry, ancestors, limit_ancestor_count, limit_ancestor_size,
            limit_descendant_count, limit_descendant_size, unused_error_string);
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
        return m_node.mempool->GetMinFee(gArgs.GetIntArg("-maxmempool", DEFAULT_MAX_MEMPOOL_SIZE) * 1000000);
    }
    CFeeRate relayMinFee() override { return ::minRelayTxFee; }
    CFeeRate relayIncrementalFee() override { return ::incrementalRelayFee; }
    CFeeRate relayDustFee() override { return ::dustRelayFee; }
    bool havePruned() override
    {
        LOCK(::cs_main);
        return chainman().m_blockman.m_have_pruned;
    }
    bool isReadyToBroadcast() override { return !node::fImporting && !node::fReindex && !isInitialBlockDownload(); }
    bool isInitialBlockDownload() override {
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
        if (!old_tip.IsNull()) {
            LOCK(::cs_main);
            const CChain& active = chainman().ActiveChain();
            if (old_tip == active.Tip()->GetBlockHash()) return;
        }
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

    NodeContext& m_node;
};
} // namespace
} // namespace node

namespace interfaces {
std::unique_ptr<Node> MakeNode(node::NodeContext& context) { return std::make_unique<node::NodeImpl>(context); }
std::unique_ptr<Chain> MakeChain(node::NodeContext& node) { return std::make_unique<node::ChainImpl>(node); }
MnListPtr MakeMNList(const CDeterministicMNList& mn_list) { return std::make_shared<node::MnListImpl>(mn_list); }
} // namespace interfaces
