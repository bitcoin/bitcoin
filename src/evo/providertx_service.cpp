// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <evo/providertx_service.h>

#include <consensus/amount.h>
#include <consensus/tx_verify.h>
#include <consensus/validation.h>
#include <deploymentstatus.h>
#include <evo/chainhelper.h>
#include <evo/deterministicmns.h>
#include <evo/netinfo.h>
#include <evo/providertx.h>
#include <evo/sharedcollateral.h>
#include <evo/specialtx.h>
#include <evo/specialtxman.h>
#include <interfaces/wallet.h>
#include <key_io.h>
#include <node/context.h>
#include <node/transaction.h>
#include <policy/policy.h>
#include <script/standard.h>
#include <sync.h>
#include <util/check.h>
#include <util/message.h>
#include <util/moneystr.h>
#include <util/result.h>
#include <util/strencodings.h>
#include <util/translation.h>
#include <validation.h>

#include <algorithm>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace evo::provider {
namespace {

using interfaces::CoinLockResult;
using interfaces::ExistingProviderCollateral;
using interfaces::FundProviderCollateral;
using interfaces::PreparedProviderRegistration;
using interfaces::PreparedSharedConsent;
using interfaces::PreparedSharedRegistration;
using interfaces::ProviderNetInfo;
using interfaces::ProviderPayout;
using interfaces::ProviderPlatformEndpoints;
using interfaces::ProviderRegistrationRequest;
using interfaces::ProviderRevokeRequest;
using interfaces::ProviderTxError;
using interfaces::ProviderTxErrorCode;
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

ProviderTxError Error(ProviderTxErrorCode code, std::string message, std::string reject_reason = {},
                      std::optional<TransactionError> transaction_error = std::nullopt)
{
    return ProviderTxError{code, Untranslated(std::move(message)), std::move(reject_reason), transaction_error};
}

std::optional<ProviderTxError> CheckDestination(const std::optional<CTxDestination>& destination, std::string_view field_name)
{
    if (destination && !IsValidDestination(*destination)) {
        return Error(ProviderTxErrorCode::INVALID_ADDRESS_OR_KEY, strprintf("invalid %s address", field_name));
    }
    return std::nullopt;
}

std::optional<ProviderTxError> CheckWallet(Wallet& wallet)
{
    AssertLockNotHeld(::cs_main);
    if (wallet.isLocked()) {
        return Error(ProviderTxErrorCode::WALLET_UNLOCK_NEEDED,
                     "Please enter the wallet passphrase with walletpassphrase first.");
    }
    return std::nullopt;
}

uint16_t CurrentProviderTxVersion(node::NodeContext& node, std::optional<bool> basic_override = std::nullopt)
{
    AssertLockNotHeld(::cs_main);
    auto& chainman{*Assert(node.chainman)};
    LOCK(::cs_main);
    return DeploymentToProtxVersion(chainman.ActiveChain().Tip(), chainman, basic_override);
}

bool EvoSharesActive(node::NodeContext& node)
{
    AssertLockNotHeld(::cs_main);
    auto& chainman{*Assert(node.chainman)};
    LOCK(::cs_main);
    return DeploymentActiveAfter(chainman.ActiveChain().Tip(), chainman, Consensus::DEPLOYMENT_EVO_SHARES);
}

struct ChainSnapshot {
    uint16_t provider_tx_version{0};
    CDeterministicMNCPtr dmn;
    std::optional<Coin> collateral_coin;
};

ChainSnapshot SnapshotChain(node::NodeContext& node, std::optional<uint256> pro_tx_hash,
                            std::optional<COutPoint> collateral_outpoint, std::optional<bool> basic_override = std::nullopt)
{
    AssertLockNotHeld(::cs_main);
    auto& chainman{*Assert(node.chainman)};
    LOCK(::cs_main);

    ChainSnapshot snapshot;
    snapshot.provider_tx_version = DeploymentToProtxVersion(chainman.ActiveChain().Tip(), chainman, basic_override);
    if (pro_tx_hash) snapshot.dmn = Assert(node.dmnman)->GetListAtChainTip().GetMN(*pro_tx_hash);
    if (collateral_outpoint) {
        Coin coin;
        if (chainman.ActiveChainstate().CoinsTip().GetCoin(*collateral_outpoint, coin) && !coin.IsSpent()) {
            snapshot.collateral_coin = std::move(coin);
        }
    }
    return snapshot;
}

template <typename ProTx>
std::optional<ProviderTxError> ApplyCoreNetInfo(ProTx& payload, const std::vector<std::string>& entries, bool optional)
{
    CHECK_NONFATAL(payload.netInfo);
    if (!optional && entries.empty()) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER, "Invalid param for coreP2PAddrs, cannot be empty");
    }
    for (size_t index{0}; index < entries.size(); ++index) {
        if (entries[index].empty()) {
            return Error(ProviderTxErrorCode::INVALID_PARAMETER,
                         strprintf("Invalid param for coreP2PAddrs[%zu], cannot be empty", index));
        }
        if (const auto status{payload.netInfo->AddEntry(NetInfoPurpose::CORE_P2P, entries[index])};
            status != NetInfoStatus::Success) {
            return Error(ProviderTxErrorCode::INVALID_PARAMETER,
                         strprintf("Error setting coreP2PAddrs[%zu] to '%s' (%s)", index, entries[index],
                                   NISToString(status)));
        }
    }
    return std::nullopt;
}

bool IsNumeric(std::string_view input)
{
    return !input.empty() && input.find_first_not_of("0123456789") == std::string_view::npos;
}

bool HasPlatformEndpoints(const ProviderPlatformEndpoints& input)
{
    if (std::holds_alternative<uint16_t>(input)) return true;
    const auto* entries{std::get_if<std::vector<std::string>>(&input)};
    return entries && !entries->empty();
}

template <typename ProTx>
std::optional<ProviderTxError> ApplyPlatformField(ProTx& payload, const ProviderPlatformEndpoints& input,
                                                  NetInfoPurpose purpose, std::string_view field_name,
                                                  uint16_t& legacy_port, bool optional)
{
    CHECK_NONFATAL(payload.netInfo);

    const auto add_entry = [&](const std::string& entry, size_t index) -> std::optional<ProviderTxError> {
        if (entry.empty()) {
            return Error(ProviderTxErrorCode::INVALID_PARAMETER,
                         strprintf("Invalid param for %s[%zu], cannot be empty", field_name, index));
        }
        if (const auto status{payload.netInfo->AddEntry(purpose, entry)}; status != NetInfoStatus::Success) {
            return Error(ProviderTxErrorCode::INVALID_PARAMETER,
                         strprintf("Error setting %s[%zu] to '%s' (%s)", field_name, index, entry, NISToString(status)));
        }
        return std::nullopt;
    };

    if (const auto* port{std::get_if<uint16_t>(&input)}) {
        if (*port == 0) {
            return Error(ProviderTxErrorCode::INVALID_PARAMETER,
                         strprintf("Invalid param for %s, must be a valid port [1-65535]", field_name));
        }
        if (!payload.netInfo->CanStorePlatform()) {
            legacy_port = *port;
            return std::nullopt;
        }
        if (!payload.netInfo->HasEntries(NetInfoPurpose::CORE_P2P)) {
            return Error(ProviderTxErrorCode::INVALID_PARAMETER,
                         strprintf("Cannot set param for %s, must specify coreP2PAddrs first", field_name));
        }
        const CService service{CNetAddr{payload.netInfo->GetPrimary()}, *port};
        return add_entry(service.ToStringAddrPort(), 0);
    }

    const auto* entries{std::get_if<std::vector<std::string>>(&input)};
    const bool empty{!entries || entries->empty()};
    if (empty) {
        if (!optional) {
            return Error(ProviderTxErrorCode::INVALID_PARAMETER,
                         strprintf("Invalid param for %s, cannot be empty", field_name));
        }
        if (!payload.netInfo->IsEmpty()) {
            return Error(ProviderTxErrorCode::INVALID_PARAMETER,
                         strprintf("Invalid param for %s, cannot be empty if other fields populated", field_name));
        }
    }

    if (!payload.netInfo->CanStorePlatform()) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER,
                     strprintf("Invalid param for %s, this ProTx version only accepts a bare port number; the "
                               "\"ADDR:PORT\" / address-array form requires upgrading to a version 3 ProTx",
                               field_name));
    }
    if (!entries) return std::nullopt;
    for (size_t index{0}; index < entries->size(); ++index) {
        if (IsNumeric((*entries)[index])) {
            return Error(ProviderTxErrorCode::INVALID_PARAMETER,
                         strprintf("Invalid param for %s[%zu], must be string", field_name, index));
        }
        if (auto error{add_entry((*entries)[index], index)}) return error;
    }
    return std::nullopt;
}

template <typename ProTx>
std::optional<ProviderTxError> ApplyNetInfo(ProTx& payload, const ProviderNetInfo& net_info, bool platform, bool optional)
{
    if (auto error{ApplyCoreNetInfo(payload, net_info.core_p2p, optional)}) return error;
    if (!platform) {
        if (HasPlatformEndpoints(net_info.platform_p2p) || HasPlatformEndpoints(net_info.platform_https)) {
            return Error(ProviderTxErrorCode::INVALID_PARAMETER, "Platform endpoints are only valid for an EvoNode");
        }
        return std::nullopt;
    }
    if (auto error{ApplyPlatformField(payload, net_info.platform_p2p, NetInfoPurpose::PLATFORM_P2P, "platformP2PAddrs",
                                      payload.platformP2PPort, optional)}) {
        return error;
    }
    return ApplyPlatformField(payload, net_info.platform_https, NetInfoPurpose::PLATFORM_HTTPS, "platformHTTPSAddrs",
                              payload.platformHTTPPort, optional);
}

template <typename ProTx>
std::optional<ProviderTxError> ValidateNetworkFields(const ProTx& payload, bool allow_empty, bool check_platform_node_id)
{
    TxValidationState state;
    const uint160* platform_node_id{check_platform_node_id && payload.nType == MnType::Evo ? &payload.platformNodeID
                                                                                           : nullptr};
    if (!CheckProviderNetworkFields(payload.netInfo, payload.nType, payload.nVersion, platform_node_id,
                                    payload.platformP2PPort, payload.platformHTTPPort, allow_empty, state)) {
        return Error(ProviderTxErrorCode::CONSENSUS_REJECTED, state.ToString(), state.GetRejectReason());
    }
    return std::nullopt;
}

template <typename ProTx>
std::optional<ProviderTxError> ApplyPlatformNodeId(ProTx& payload, const std::optional<uint160>& platform_node_id)
{
    if (payload.nType != MnType::Evo) return std::nullopt;
    if (!platform_node_id) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER, "platformNodeID must be specified for an EvoNode");
    }
    payload.platformNodeID = *platform_node_id;
    return std::nullopt;
}

using PayoutResult = std::variant<MasternodePayoutShares, ProviderTxError>;

PayoutResult BuildPayouts(const std::vector<ProviderPayout>& payouts)
{
    if (payouts.empty()) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER, "payouts must contain at least one entry");
    }
    if (payouts.size() > 8) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER, "payouts must not contain more than 8 entries");
    }

    MasternodePayoutShares result;
    result.reserve(payouts.size());
    for (const auto& payout : payouts) {
        if (!IsValidDestination(payout.destination)) {
            return Error(ProviderTxErrorCode::INVALID_ADDRESS_OR_KEY,
                         strprintf("invalid payout address: %s", EncodeDestination(payout.destination)));
        }
        result.emplace_back(GetScriptForDestination(payout.destination), payout.reward);
    }
    return result;
}

std::optional<ProviderTxError> ResolveFeeSource(const std::optional<CTxDestination>& fee_source,
                                                const CScript& operator_payout, const CDeterministicMNState& dmn_state,
                                                std::string missing_error, CTxDestination& fund_destination)
{
    if (fee_source) {
        fund_destination = *fee_source;
    } else if (!operator_payout.empty()) {
        ExtractDestination(operator_payout, fund_destination);
    } else {
        const auto reward_scripts{dmn_state.GetOwnerRewardScripts()};
        if (reward_scripts.empty() || !ExtractDestination(reward_scripts.front(), fund_destination)) {
            return Error(ProviderTxErrorCode::INVALID_PARAMETER, std::move(missing_error));
        }
    }
    return std::nullopt;
}

template <typename Payload>
void UpdateInputsHash(const CMutableTransaction& tx, Payload& payload)
{
    payload.inputsHash = CalcTxInputsHash(CTransaction(tx));
}

template <typename Payload>
std::optional<ProviderTxError> SignPayload(const CMutableTransaction& tx, Payload& payload, const CKeyID& key_id,
                                           Wallet& wallet)
{
    UpdateInputsHash(tx, payload);
    payload.vchSig.clear();
    if (!wallet.signSpecialTxPayload(::SerializeHash(payload), key_id, payload.vchSig)) {
        return Error(ProviderTxErrorCode::INTERNAL_ERROR, "failed to sign special tx");
    }
    return std::nullopt;
}

template <typename Payload>
void SignPayload(const CMutableTransaction& tx, Payload& payload, const CBLSSecretKey& key, bool legacy)
{
    UpdateInputsHash(tx, payload);
    payload.sig = key.Sign(::SerializeHash(payload), legacy);
}

using MutableTxResult = std::variant<CMutableTransaction, ProviderTxError>;

template <typename Payload>
MutableTxResult Fund(Wallet& wallet, CMutableTransaction tx, const Payload& payload, const CTxDestination& fund_destination)
{
    AssertLockNotHeld(::cs_main);
    if (!IsValidDestination(fund_destination)) {
        return Error(ProviderTxErrorCode::FUNDING_ERROR, "No source of funds specified");
    }
    SetTxPayload(tx, payload);
    auto funded{wallet.fundTransaction(tx, fund_destination)};
    if (!funded) {
        return Error(ProviderTxErrorCode::FUNDING_ERROR, util::ErrorString(funded).original);
    }
    return CMutableTransaction{**funded};
}

std::optional<ProviderTxError> Preflight(node::NodeContext& node, const CTransaction& tx)
{
    AssertLockNotHeld(::cs_main);
    auto& chainman{*Assert(node.chainman)};
    auto& chain_helper{*Assert(node.chain_helper)};
    LOCK(::cs_main);

    const CBlockIndex* tip{chainman.ActiveChain().Tip()};
    const auto& consensus{chainman.GetConsensus()};
    if (!DeploymentActiveAfter(tip, consensus, Consensus::DEPLOYMENT_DIP0003)) {
        const int current_height{tip ? tip->nHeight : -1};
        const int next_block_height{current_height + 1};
        const int activation_height{consensus.DIP0003Height};
        const int blocks_to_mine{activation_height > next_block_height ? activation_height - next_block_height : 0};
        return Error(ProviderTxErrorCode::VERIFY_ERROR,
                     strprintf("DIP0003 is not active yet; ProTx transactions are valid starting at block height %d "
                               "(current chain height %d, next block height %d). Mine %d more block%s or restart "
                               "this regtest/devnet chain with DIP3 activation parameters that are already active.",
                               activation_height, current_height, next_block_height, blocks_to_mine,
                               blocks_to_mine == 1 ? "" : "s"));
    }

    TxValidationState state;
    if (!chain_helper.special_tx->CheckSpecialTx(tx, tip, GetSpecialTxRules(tip, chainman),
                                                 chainman.ActiveChainstate().CoinsTip(), true, state)) {
        return Error(ProviderTxErrorCode::CONSENSUS_REJECTED, state.ToString(), state.GetRejectReason());
    }
    return std::nullopt;
}

ProviderTxResult<ProviderTxSubmission> Broadcast(node::NodeContext& node, const CTransactionRef& tx)
{
    AssertLockNotHeld(::cs_main);
    const CAmount max_fee{node::DEFAULT_MAX_RAW_TX_FEE_RATE.GetFee(GetVirtualTransactionSize(*tx))};
    bilingual_str message;
    const TransactionError transaction_error{
        node::BroadcastTransaction(node, tx, message, max_fee, /*relay=*/true, /*wait_callback=*/true)};
    if (transaction_error != TransactionError::OK) {
        return Error(ProviderTxErrorCode::BROADCAST_ERROR, message.original, {}, transaction_error);
    }
    return ProviderTxSubmission{tx, true};
}

//! Sign every input this wallet can sign and optionally submit. With `allow_partial`, an
//! unsubmitted transaction is returned signed as far as this wallet can sign it, for another
//! wallet to finish.
ProviderTxResult<ProviderTxSubmission> Finish(node::NodeContext& node, Wallet& wallet, const CMutableTransaction& tx,
                                              bool submit, bool allow_partial = false)
{
    AssertLockNotHeld(::cs_main);
    if (auto error{Preflight(node, CTransaction{tx})}) return *error;

    auto signed_result{wallet.signTransaction(tx)};
    if (!signed_result) {
        return Error(wallet.isLocked() ? ProviderTxErrorCode::WALLET_UNLOCK_NEEDED : ProviderTxErrorCode::WALLET_ERROR,
                     util::ErrorString(signed_result).original);
    }
    if (!submit && allow_partial) return ProviderTxSubmission{signed_result->tx, false};
    if (!signed_result->complete) {
        std::string errors;
        for (const auto& error : signed_result->errors) {
            if (!errors.empty()) errors += ", ";
            errors += error.original;
        }
        return Error(ProviderTxErrorCode::WALLET_ERROR,
                     strprintf("transaction inputs could not be fully signed by this wallet; run this command on "
                               "the wallet that funded the transaction, or finish signing there with "
                               "signrawtransactionwithwallet (%s)",
                               errors));
    }
    if (!submit) return ProviderTxSubmission{signed_result->tx, false};
    return Broadcast(node, signed_result->tx);
}

//! Submit a transaction that needs no further signing
ProviderTxResult<ProviderTxSubmission> Submit(node::NodeContext& node, CMutableTransaction tx)
{
    if (auto error{Preflight(node, CTransaction{tx})}) return *error;
    return Broadcast(node, MakeTransactionRef(std::move(tx)));
}

class CollateralLockGuard
{
public:
    CollateralLockGuard(Wallet& wallet, const COutPoint& outpoint) :
        m_wallet(wallet),
        m_outpoint(outpoint)
    {
        m_result = m_wallet.acquireCoinLock(m_outpoint, /*write_to_db=*/false);
    }

    ~CollateralLockGuard()
    {
        if (m_result == CoinLockResult::ACQUIRED && !m_keep) m_wallet.unlockCoin(m_outpoint);
    }

    bool Succeeded() const { return m_result != CoinLockResult::FAILED; }
    bool Acquired() const { return m_result == CoinLockResult::ACQUIRED; }
    void Keep() { m_keep = true; }

private:
    Wallet& m_wallet;
    COutPoint m_outpoint;
    CoinLockResult m_result{CoinLockResult::FAILED};
    bool m_keep{false};
};

using RegistrationOutcome = std::variant<ProviderTxSubmission, PreparedProviderRegistration>;
using RegistrationResult = ProviderTxResult<RegistrationOutcome>;

RegistrationResult BuildRegistration(node::NodeContext& node, Wallet& wallet,
                                     const ProviderRegistrationRequest& request, bool prepare)
{
    if (!IsValidMnType(request.type)) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER, "invalid masternode type");
    }
    if (prepare && !std::holds_alternative<ExistingProviderCollateral>(request.collateral)) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER,
                     "prepared registration requires an existing collateral outpoint");
    }
    if (!request.operator_key.IsValid()) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER, "operator BLS address must be a valid BLS public key");
    }
    if (request.operator_reward > 10000) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER, "operatorReward must be between 0 and 10000");
    }
    if (auto error{CheckDestination(request.fee_source, "fee source")}) return *error;

    const auto* existing_collateral{std::get_if<ExistingProviderCollateral>(&request.collateral)};
    const ChainSnapshot chain_snapshot{
        SnapshotChain(node, std::nullopt, existing_collateral ? std::optional{existing_collateral->outpoint} : std::nullopt,
                      /*basic_override=*/!request.use_legacy_bls_scheme)};
    if (auto error{CheckWallet(wallet)}) return *error;

    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_PROVIDER_REGISTER;

    CProRegTx payload;
    payload.nType = request.type;
    payload.nVersion = chain_snapshot.provider_tx_version;
    payload.netInfo = NetInfoInterface::MakeNetInfo(payload.nVersion);

    if (const auto* collateral{std::get_if<FundProviderCollateral>(&request.collateral)}) {
        if (!IsValidDestination(collateral->destination)) {
            return Error(ProviderTxErrorCode::INVALID_ADDRESS_OR_KEY,
                         strprintf("invalid collateral address: %s", EncodeDestination(collateral->destination)));
        }
        tx.vout.emplace_back(GetMnType(request.type).collat_amount, GetScriptForDestination(collateral->destination));
    } else {
        payload.collateralOutpoint = std::get<ExistingProviderCollateral>(request.collateral).outpoint;
        if (payload.collateralOutpoint.hash.IsNull()) {
            return Error(ProviderTxErrorCode::INVALID_ADDRESS_OR_KEY,
                         strprintf("invalid hash or index: %s-%d", payload.collateralOutpoint.hash.ToString(),
                                   payload.collateralOutpoint.n));
        }
    }

    if (auto error{ApplyNetInfo(payload, request.net_info, request.type == MnType::Evo, /*optional=*/true)}) {
        return *error;
    }
    payload.keyIDOwner = request.owner_key;
    payload.keyIDVoting = request.voting_key.IsNull() ? request.owner_key : request.voting_key;
    payload.pubKeyOperator.Set(request.operator_key, request.use_legacy_bls_scheme);
    if (payload.pubKeyOperator.IsLegacy() != (payload.nVersion == ProTxVersion::LegacyBLS)) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER, "operator key encoding does not match ProTx version");
    }
    payload.nOperatorReward = request.operator_reward;

    auto payouts_result{BuildPayouts(request.payouts)};
    if (const auto* error{std::get_if<ProviderTxError>(&payouts_result)}) return *error;
    payload.payouts = std::get<MasternodePayoutShares>(std::move(payouts_result));
    payload.scriptPayout = payload.payouts.front().scriptPayout;
    if (payload.nVersion < ProTxVersion::ExtAddr && (request.uses_extended_payouts || payload.payouts.size() > 1)) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER, "payouts array requires provider transaction version 3");
    }

    if (auto error{ApplyPlatformNodeId(payload, request.platform_node_id)}) return *error;
    if (auto error{ValidateNetworkFields(payload, /*allow_empty=*/true,
                                         /*check_platform_node_id=*/true)}) {
        return *error;
    }

    const CTxDestination fund_destination{request.fee_source.value_or(request.payouts.front().destination)};
    if (std::holds_alternative<FundProviderCollateral>(request.collateral)) {
        auto funded_result{Fund(wallet, std::move(tx), payload, fund_destination)};
        if (const auto* error{std::get_if<ProviderTxError>(&funded_result)}) return *error;
        tx = std::get<CMutableTransaction>(std::move(funded_result));
        UpdateInputsHash(tx, payload);

        const CAmount collateral_amount{GetMnType(request.type).collat_amount};
        // Funding may reorder outputs and can produce a change output of the
        // exact collateral amount, so match the destination script too.
        const CScript collateral_script{
            GetScriptForDestination(std::get<FundProviderCollateral>(request.collateral).destination)};
        const auto it{std::find_if(tx.vout.begin(), tx.vout.end(), [&](const CTxOut& output) {
            return output.nValue == collateral_amount && output.scriptPubKey == collateral_script;
        })};
        if (it == tx.vout.end()) {
            return Error(ProviderTxErrorCode::INTERNAL_ERROR, "funded transaction lost its collateral output");
        }
        payload.collateralOutpoint.n = static_cast<uint32_t>(std::distance(tx.vout.begin(), it));
        SetTxPayload(tx, payload);
        auto result{Finish(node, wallet, tx, request.submit)};
        if (const auto* error{std::get_if<ProviderTxError>(&result)}) return *error;
        return RegistrationOutcome{std::get<ProviderTxSubmission>(std::move(result))};
    }

    CollateralLockGuard collateral_lock{wallet, payload.collateralOutpoint};
    if (!collateral_lock.Succeeded()) {
        return Error(ProviderTxErrorCode::WALLET_ERROR,
                     strprintf("failed to lock collateral: %s", payload.collateralOutpoint.ToStringShort()));
    }

    payload.vchSig.resize(CPubKey::COMPACT_SIGNATURE_SIZE);
    auto funded_result{Fund(wallet, std::move(tx), payload, fund_destination)};
    if (const auto* error{std::get_if<ProviderTxError>(&funded_result)}) return *error;
    tx = std::get<CMutableTransaction>(std::move(funded_result));
    UpdateInputsHash(tx, payload);

    if (!chain_snapshot.collateral_coin) {
        return Error(ProviderTxErrorCode::INVALID_ADDRESS_OR_KEY,
                     strprintf("collateral not found: %s", payload.collateralOutpoint.ToStringShort()));
    }
    CTxDestination collateral_destination;
    ExtractDestination(chain_snapshot.collateral_coin->out.scriptPubKey, collateral_destination);
    const auto* collateral_key{std::get_if<PKHash>(&collateral_destination)};
    if (!collateral_key) {
        return Error(ProviderTxErrorCode::INVALID_ADDRESS_OR_KEY,
                     strprintf("collateral type not supported: %s", payload.collateralOutpoint.ToStringShort()));
    }

    if (prepare) {
        payload.vchSig.clear();
        SetTxPayload(tx, payload);
        collateral_lock.Keep();
        return RegistrationOutcome{PreparedProviderRegistration{MakeTransactionRef(std::move(tx)), collateral_destination,
                                                                payload.MakeSignString(), collateral_lock.Acquired()}};
    }

    std::string signature;
    const SigningResult signing_result{wallet.signMessage(payload.MakeSignString(), *collateral_key, signature)};
    if (signing_result != SigningResult::OK) {
        return Error(signing_result == SigningResult::SIGNING_FAILED ? ProviderTxErrorCode::INVALID_ADDRESS_OR_KEY
                                                                     : ProviderTxErrorCode::WALLET_ERROR,
                     SigningResultString(signing_result));
    }
    auto decoded_signature{DecodeBase64(signature)};
    if (!decoded_signature) {
        return Error(ProviderTxErrorCode::INTERNAL_ERROR, "failed to decode base64 ready signature for protx");
    }
    payload.vchSig = std::move(*decoded_signature);
    SetTxPayload(tx, payload);
    auto result{Finish(node, wallet, tx, request.submit)};
    if (const auto* error{std::get_if<ProviderTxError>(&result)}) return *error;
    collateral_lock.Keep();
    return RegistrationOutcome{std::get<ProviderTxSubmission>(std::move(result))};
}

constexpr std::string_view ZERO_PENALTY_WARNING{
    "earlyPenalty is zero: any participant can force an early exit at no cost beyond the transaction fee"};

ProviderTxError SharedMnNotFound()
{
    return Error(ProviderTxErrorCode::INVALID_PARAMETER, "shared masternode not found");
}

//! The shared masternode registered as `pro_tx_hash` at the chain tip, or null
CDeterministicMNCPtr GetSharedMn(node::NodeContext& node, const uint256& pro_tx_hash)
{
    auto dmn{Assert(node.dmnman)->GetListAtChainTip().GetMN(pro_tx_hash)};
    if (!dmn || !dmn->pdmnState->IsShared()) return nullptr;
    return dmn;
}

struct Dissolution {
    CMutableTransaction tx;
    CProDisTx payload;
};

//! Builds an unsigned ProDisTx transaction paying each non-actor share its principal plus its
//! pro-rata slice of the penalty (sequential floor, remainder to the last non-actor entry, which
//! always satisfies the consensus minimums), with the actor absorbing penalty and fee
std::variant<Dissolution, ProviderTxError> BuildDissolution(const CDeterministicMN& dmn, uint16_t actor_index,
                                                            CAmount penalty, CAmount fee)
{
    const auto& shares = dmn.pdmnState->shares;
    if (actor_index >= shares.size()) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER, "actorIndex out of range");
    }
    if (fee <= 0) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER, "fee must be positive");
    }
    if (fee > CProDisTx::MAX_FEE) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER,
                     strprintf("fee exceeds the consensus ceiling of %d duffs", CProDisTx::MAX_FEE));
    }
    const CAmount actor_output = shares[actor_index].amount - penalty - fee;
    if (actor_output < 0) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER, "penalty and fee exceed the actor's share");
    }

    Dissolution ret;
    ret.tx.nVersion = 3;
    ret.tx.nType = TRANSACTION_PROVIDER_DISSOLVE;
    ret.tx.vin.emplace_back(dmn.collateralOutpoint);

    CollateralShares non_actors;
    for (size_t i = 0; i < shares.size(); i++) {
        if (i != actor_index) non_actors.push_back(shares[i]);
    }
    const auto bonuses{SplitAmountByShares(penalty, non_actors)};
    for (size_t i = 0; i < non_actors.size(); i++) {
        ret.tx.vout.emplace_back(non_actors[i].amount + bonuses[i], non_actors[i].scriptRefund);
    }
    if (actor_output > 0) {
        ret.tx.vout.emplace_back(actor_output, shares[actor_index].scriptRefund);
    }

    ret.payload.proTxHash = dmn.proTxHash;
    ret.payload.actorIndex = actor_index;
    return ret;
}

//! True when `tx` pays what BuildDissolution() builds without a penalty: every share's principal
//! back to its refund script, less a fee from the actor's share. Consensus also accepts other
//! payouts every share owner signs, but no RPC or GUI flow builds one, so none is signed here.
bool ReturnsPrincipal(const CDeterministicMN& dmn, uint16_t actor_index, const CTransaction& tx)
{
    CAmount fee{0};
    for (const auto& share : dmn.pdmnState->shares) fee += share.amount;
    CAmount out_total{0};
    for (const auto& out : tx.vout) {
        if (!MoneyRange(out.nValue) || !MoneyRange(out_total + out.nValue)) return false;
        out_total += out.nValue;
    }
    fee -= out_total;
    const auto built{BuildDissolution(dmn, actor_index, /*penalty=*/0, fee)};
    const auto* expected{std::get_if<Dissolution>(&built)};
    return expected && tx.vout == expected->tx.vout;
}

//! True when `tx` carries an absolute lock not yet satisfied at the next block, or any BIP68
//! relative lock (special transactions are version 3, so sequences carry relative-lock
//! semantics). Wallet-funded inputs use a non-final sequence purely for fee sniping
//! discouragement with an already-satisfied nLockTime, which is not a lock.
bool HasTimeLock(node::NodeContext& node, const CTransaction& tx)
{
    AssertLockNotHeld(::cs_main);
    auto& chainman{*Assert(node.chainman)};
    bool has_time_lock{false};
    {
        LOCK(::cs_main);
        const CBlockIndex* tip{chainman.ActiveChain().Tip()};
        has_time_lock = !IsFinalTx(tx, tip->nHeight + 1, tip->GetMedianTimePast());
    }
    for (const auto& txin : tx.vin) {
        has_time_lock |= !(txin.nSequence & CTxIn::SEQUENCE_LOCKTIME_DISABLE_FLAG);
    }
    return has_time_lock;
}

//! Put one signature per share, in share order, into a unanimous ProDisTx or ProUpSharedRegTx
template <typename Payload>
std::optional<ProviderTxError> InsertUnanimousSignatures(node::NodeContext& node, CMutableTransaction& tx,
                                                         const std::map<size_t, CompactSignature>& signatures,
                                                         std::string count_error)
{
    auto payload{GetTxPayload<Payload>(tx)};
    if (!payload) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER, "transaction payload not deserializable");
    }
    const auto dmn{GetSharedMn(node, payload->proTxHash)};
    if (!dmn) return SharedMnNotFound();
    const size_t share_count{dmn->pdmnState->shares.size()};
    if (signatures.size() != share_count) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER, std::move(count_error));
    }
    payload->vchSigs.clear();
    for (size_t i = 0; i < share_count; i++) {
        const auto it = signatures.find(i);
        if (it == signatures.end()) {
            return Error(ProviderTxErrorCode::INVALID_PARAMETER, strprintf("missing signature for share %d", i));
        }
        payload->vchSigs.push_back(it->second);
    }
    SetTxPayload(tx, *payload);
    return std::nullopt;
}

//! The share signatures on a registrar update commit to its payload and input outpoints but
//! not to its outputs. Before re-signing the fee inputs, make sure the transaction has the shape
//! the wallet gave it when funding it: unlocked coins of one address of this wallet, change (if
//! any) back to that address, and a fee no higher than the wallet's maximum.
std::optional<ProviderTxError> CheckRegistrarUpdateFunding(Wallet& wallet, const CMutableTransaction& tx)
{
    const auto single_address_error{[] {
        return Error(ProviderTxErrorCode::WALLET_ERROR,
                     "a shared registrar update must spend from and return change to a single address");
    }};
    std::vector<COutPoint> outpoints;
    outpoints.reserve(tx.vin.size());
    for (const CTxIn& input : tx.vin) {
        outpoints.push_back(input.prevout);
    }
    const auto coins{wallet.getCoins(outpoints)};
    std::optional<CTxDestination> fee_source;
    CAmount input_total{0};
    for (size_t i = 0; i < coins.size(); ++i) {
        const CScript& script{coins[i].txout.scriptPubKey};
        CTxDestination source;
        if (coins[i].depth_in_main_chain < 0 || !wallet.isSpendable(script) || !ExtractDestination(script, source)) {
            return Error(ProviderTxErrorCode::WALLET_ERROR,
                         "a shared registrar update must be combined by the wallet that funded it");
        }
        // Masternode collaterals are locked, and funding never selects a locked coin
        if (wallet.isLockedCoin(outpoints[i])) {
            return Error(ProviderTxErrorCode::WALLET_ERROR, "a shared registrar update must not spend a locked coin");
        }
        if (fee_source && source != *fee_source) return single_address_error();
        fee_source = source;
        input_total += coins[i].txout.nValue;
        if (!MoneyRange(input_total)) {
            return Error(ProviderTxErrorCode::INVALID_PARAMETER, "transaction input value out of range");
        }
    }
    if (!fee_source) {
        return Error(ProviderTxErrorCode::WALLET_ERROR,
                     "a shared registrar update must be combined by the wallet that funded it");
    }
    CAmount output_total{0};
    for (const CTxOut& output : tx.vout) {
        if (!MoneyRange(output.nValue)) {
            return Error(ProviderTxErrorCode::INVALID_PARAMETER, "transaction output value out of range");
        }
        output_total += output.nValue;
        if (!MoneyRange(output_total)) {
            return Error(ProviderTxErrorCode::INVALID_PARAMETER, "transaction output value out of range");
        }
        // Funding without change leaves a zero-valued marker output
        if (output.nValue == 0) continue;
        if (CTxDestination change; !ExtractDestination(output.scriptPubKey, change) || change != *fee_source) {
            return single_address_error();
        }
    }
    if (const CAmount fee{input_total - output_total}; fee > wallet.getDefaultMaxTxFee()) {
        return Error(ProviderTxErrorCode::WALLET_ERROR,
                     strprintf("a shared registrar update fee of %s exceeds this wallet's maximum fee (%s)",
                               FormatMoney(fee), FormatMoney(wallet.getDefaultMaxTxFee())));
    }
    return std::nullopt;
}

} // namespace

interfaces::ProviderTxCapabilities GetCapabilities(node::NodeContext& node, std::optional<bool> basic_override)
{
    const uint16_t version{CurrentProviderTxVersion(node, basic_override)};
    return interfaces::ProviderTxCapabilities{version, version >= ProTxVersion::ExtAddr};
}

std::optional<ProviderTxError> ValidateNetInfo(const ProviderNetInfo& net_info, MnType type, uint16_t version, bool optional)
{
    if (!IsValidMnType(type)) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER, "invalid masternode type");
    }
    if (version < ProTxVersion::LegacyBLS || version > ProTxVersion::ExtAddr) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER, "invalid provider transaction version");
    }

    CProRegTx payload;
    payload.nType = type;
    payload.nVersion = version;
    payload.netInfo = NetInfoInterface::MakeNetInfo(version);
    if (auto error{ApplyNetInfo(payload, net_info, type == MnType::Evo, optional)}) return error;
    return ValidateNetworkFields(payload, optional, /*check_platform_node_id=*/false);
}

std::optional<std::string> FindNetInfoInUse(node::NodeContext& node, const ProviderNetInfo& net_info, MnType type,
                                            uint16_t version)
{
    if (ValidateNetInfo(net_info, type, version, /*optional=*/true) || !node.dmnman) return std::nullopt;
    CProRegTx payload;
    payload.nType = type;
    payload.nVersion = version;
    payload.netInfo = NetInfoInterface::MakeNetInfo(version);
    if (ApplyNetInfo(payload, net_info, type == MnType::Evo, /*optional=*/true)) return std::nullopt;

    const auto mn_list{node.dmnman->GetListAtChainTip()};
    for (const auto& entry : payload.netInfo->GetEntries()) {
        const auto service{entry.GetAddrPort()};
        const auto domain{entry.GetDomainPort()};
        if ((service && mn_list.HasUniqueProperty(*service)) || (domain && mn_list.HasUniqueProperty(*domain))) {
            return entry.ToStringAddrPort();
        }
    }
    return std::nullopt;
}

ProviderTxResult<ProviderTxSubmission> Register(node::NodeContext& node, Wallet& wallet,
                                                const ProviderRegistrationRequest& request)
{
    auto result{BuildRegistration(node, wallet, request, /*prepare=*/false)};
    if (const auto* error{std::get_if<ProviderTxError>(&result)}) return *error;
    auto& outcome{std::get<RegistrationOutcome>(result)};
    if (auto* submission{std::get_if<ProviderTxSubmission>(&outcome)}) return std::move(*submission);
    return Error(ProviderTxErrorCode::INTERNAL_ERROR, "registration returned an unexpected prepared transaction");
}

ProviderTxResult<PreparedProviderRegistration> PrepareRegistration(node::NodeContext& node, Wallet& wallet,
                                                                   const ProviderRegistrationRequest& request)
{
    auto result{BuildRegistration(node, wallet, request, /*prepare=*/true)};
    if (const auto* error{std::get_if<ProviderTxError>(&result)}) return *error;
    auto& outcome{std::get<RegistrationOutcome>(result)};
    if (auto* prepared{std::get_if<PreparedProviderRegistration>(&outcome)}) return std::move(*prepared);
    return Error(ProviderTxErrorCode::INTERNAL_ERROR, "registration returned an unexpected submitted transaction");
}

ProviderTxResult<ProviderTxSubmission> SubmitRegistration(node::NodeContext& node, Wallet& wallet,
                                                          const CTransactionRef& transaction,
                                                          const std::vector<unsigned char>& collateral_signature)
{
    if (auto error{CheckWallet(wallet)}) return *error;
    if (!transaction) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER, "transaction not deserializable");
    }

    CMutableTransaction tx{*transaction};
    if (tx.nType != TRANSACTION_PROVIDER_REGISTER) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER, "transaction not a ProRegTx");
    }
    auto payload{GetTxPayload<CProRegTx>(tx)};
    if (!payload) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER, "transaction payload not deserializable");
    }
    if (!payload->vchSig.empty()) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER, "payload signature not empty");
    }
    payload->vchSig = collateral_signature;
    SetTxPayload(tx, *payload);
    return Finish(node, wallet, tx, /*submit=*/true);
}

ProviderTxResult<ProviderTxSubmission> UpdateService(node::NodeContext& node, Wallet& wallet,
                                                     const ProviderUpdateServiceRequest& request)
{
    if (!IsValidMnType(request.type)) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER, "invalid masternode type");
    }
    if (!request.operator_key.IsValid()) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER, "operatorKey must be a valid BLS secret key");
    }
    if (auto error{CheckDestination(request.operator_payout, "operator payout")}) return *error;
    if (auto error{CheckDestination(request.fee_source, "fee source")}) return *error;

    const ChainSnapshot chain_snapshot{SnapshotChain(node, request.pro_tx_hash, std::nullopt)};
    if (auto error{CheckWallet(wallet)}) return *error;
    const auto& dmn{chain_snapshot.dmn};
    if (!dmn) {
        return Error(ProviderTxErrorCode::CONSENSUS_REJECTED,
                     strprintf("masternode with proTxHash %s not found", request.pro_tx_hash.ToString()));
    }
    if (dmn->nType != request.type) {
        return Error(ProviderTxErrorCode::CONSENSUS_REJECTED,
                     strprintf("masternode with proTxHash %s is not a %s", request.pro_tx_hash.ToString(),
                               GetMnType(request.type).description));
    }
    if (request.operator_key.GetPublicKey() != dmn->pdmnState->pubKeyOperator.Get()) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER,
                     "the operator key does not belong to the registered public key");
    }

    CProUpServTx payload;
    payload.proTxHash = request.pro_tx_hash;
    payload.nType = request.type;
    payload.nVersion = chain_snapshot.provider_tx_version;
    payload.netInfo = NetInfoInterface::MakeNetInfo(payload.nVersion);
    if (auto error{ApplyNetInfo(payload, request.net_info, request.type == MnType::Evo, /*optional=*/false)}) {
        return *error;
    }
    if (auto error{ApplyPlatformNodeId(payload, request.platform_node_id)}) return *error;
    if (auto error{ValidateNetworkFields(payload, /*allow_empty=*/false,
                                         /*check_platform_node_id=*/true)}) {
        return *error;
    }
    payload.scriptOperatorPayout = request.operator_payout ? GetScriptForDestination(*request.operator_payout)
                                                           : dmn->pdmnState->scriptOperatorPayout;

    CTxDestination fund_destination;
    if (auto error{ResolveFeeSource(request.fee_source, payload.scriptOperatorPayout, *dmn->pdmnState,
                                    "masternode has no default fee source; specify feeSourceAddress",
                                    fund_destination)}) {
        return *error;
    }

    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_PROVIDER_UPDATE_SERVICE;
    auto funded_result{Fund(wallet, std::move(tx), payload, fund_destination)};
    if (const auto* error{std::get_if<ProviderTxError>(&funded_result)}) return *error;
    tx = std::get<CMutableTransaction>(std::move(funded_result));
    SignPayload(tx, payload, request.operator_key, payload.nVersion == ProTxVersion::LegacyBLS);
    SetTxPayload(tx, payload);
    return Finish(node, wallet, tx, request.submit);
}

ProviderTxResult<ProviderTxSubmission> UpdateRegistrar(node::NodeContext& node, Wallet& wallet,
                                                       const ProviderUpdateRegistrarRequest& request)
{
    if (auto error{CheckDestination(request.fee_source, "fee source")}) return *error;
    const ChainSnapshot chain_snapshot{
        SnapshotChain(node, request.pro_tx_hash, std::nullopt, /*basic_override=*/!request.use_legacy_bls_scheme)};
    if (auto error{CheckWallet(wallet)}) return *error;
    const auto& dmn{chain_snapshot.dmn};
    if (!dmn) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER,
                     strprintf("masternode %s not found", request.pro_tx_hash.ToString()));
    }
    if (dmn->pdmnState->IsShared()) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER,
                     "masternode is shared; use protx shared_update_share or protx shared_update_registrar_prepare");
    }

    CProUpRegTx payload;
    payload.nVersion = chain_snapshot.provider_tx_version;
    payload.proTxHash = request.pro_tx_hash;
    payload.keyIDVoting = request.voting_key.value_or(dmn->pdmnState->keyIDVoting);

    // Only an ExtAddr service update, which spells out the Platform entries, may raise a pre-ExtAddr
    // EvoNode that has addresses. Keep its own version unless this update rotates the operator key,
    // which clears the addresses.
    const bool keeps_operator_key{!request.operator_key || *request.operator_key == dmn->pdmnState->pubKeyOperator.Get()};
    const bool keep_evo_version{dmn->nType == MnType::Evo && keeps_operator_key &&
                                !dmn->pdmnState->netInfo->IsEmpty() && dmn->pdmnState->nVersion < payload.nVersion &&
                                dmn->pdmnState->nVersion < ProTxVersion::ExtAddr};
    if (keep_evo_version) {
        payload.nVersion = dmn->pdmnState->nVersion;
    }

    if (request.operator_key) {
        if (!request.operator_key->IsValid()) {
            return Error(ProviderTxErrorCode::INVALID_PARAMETER, "operator BLS address must be a valid BLS public key");
        }
        payload.pubKeyOperator.Set(*request.operator_key, request.use_legacy_bls_scheme);
    } else {
        payload.pubKeyOperator = dmn->pdmnState->pubKeyOperator;
        if (!request.use_legacy_bls_scheme && payload.pubKeyOperator != CBLSLazyPublicKey() &&
            payload.pubKeyOperator.IsLegacy()) {
            payload.pubKeyOperator.Set(payload.pubKeyOperator.Get(), /*specificLegacyScheme=*/false);
        }
    }
    if (payload.pubKeyOperator.IsLegacy() != (payload.nVersion == ProTxVersion::LegacyBLS)) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER, "operator key encoding does not match ProTx version");
    }

    CTxDestination payout_destination;
    if (request.payouts) {
        auto payouts_result{BuildPayouts(*request.payouts)};
        if (const auto* error{std::get_if<ProviderTxError>(&payouts_result)}) return *error;
        payload.payouts = std::get<MasternodePayoutShares>(std::move(payouts_result));
        // A kept-version EvoNode carries a single full payout as its legacy payout address
        if (payload.nVersion < ProTxVersion::ExtAddr &&
            (payload.payouts.size() > 1 || payload.payouts.front().reward != MasternodePayoutShare::MAX_REWARD ||
             (request.uses_extended_payouts && !keep_evo_version))) {
            return Error(ProviderTxErrorCode::INVALID_PARAMETER,
                         keep_evo_version ? "payout shares require the EvoNode to be upgraded first with protx "
                                            "update_service_evo listing its Platform addresses"
                                          : "payouts array requires provider transaction version 3");
        }
    } else {
        payload.payouts = GetOwnerPayouts(*dmn->pdmnState);
    }
    if (payload.payouts.empty() || !ExtractDestination(payload.payouts.front().scriptPayout, payout_destination)) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER, "masternode has no payout address");
    }
    payload.scriptPayout = payload.payouts.front().scriptPayout;

    if (!wallet.isSpendable(GetScriptForDestination(PKHash{dmn->pdmnState->keyIDOwner}))) {
        // Wallet-side condition, but CONSENSUS_REJECTED is used deliberately so the
        // RPC layer keeps throwing the legacy error code -1 for this failure.
        return Error(ProviderTxErrorCode::CONSENSUS_REJECTED,
                     strprintf("Private key for owner address %s not found in your wallet",
                               EncodeDestination(PKHash{dmn->pdmnState->keyIDOwner})));
    }

    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_PROVIDER_UPDATE_REGISTRAR;
    payload.vchSig.resize(CPubKey::COMPACT_SIGNATURE_SIZE);
    auto funded_result{Fund(wallet, std::move(tx), payload, request.fee_source.value_or(payout_destination))};
    if (const auto* error{std::get_if<ProviderTxError>(&funded_result)}) return *error;
    tx = std::get<CMutableTransaction>(std::move(funded_result));
    if (auto error{SignPayload(tx, payload, dmn->pdmnState->keyIDOwner, wallet)}) return *error;
    SetTxPayload(tx, payload);
    return Finish(node, wallet, tx, request.submit);
}

ProviderTxResult<ProviderTxSubmission> Revoke(node::NodeContext& node, Wallet& wallet, const ProviderRevokeRequest& request)
{
    if (!request.operator_key.IsValid()) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER, "operatorKey must be a valid BLS secret key");
    }
    if (request.reason > CProUpRevTx::REASON_LAST) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER,
                     strprintf("invalid reason %d, must be between 0 and %d", request.reason, CProUpRevTx::REASON_LAST));
    }
    if (auto error{CheckDestination(request.fee_source, "fee source")}) return *error;

    const ChainSnapshot chain_snapshot{SnapshotChain(node, request.pro_tx_hash, std::nullopt)};
    if (auto error{CheckWallet(wallet)}) return *error;
    const auto& dmn{chain_snapshot.dmn};
    if (!dmn) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER,
                     strprintf("masternode %s not found", request.pro_tx_hash.ToString()));
    }
    if (request.operator_key.GetPublicKey() != dmn->pdmnState->pubKeyOperator.Get()) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER,
                     "the operator key does not belong to the registered public key");
    }

    CProUpRevTx payload;
    payload.proTxHash = request.pro_tx_hash;
    payload.nVersion = chain_snapshot.provider_tx_version;
    payload.nReason = request.reason;

    CTxDestination fund_destination;
    if (auto error{ResolveFeeSource(request.fee_source, dmn->pdmnState->scriptOperatorPayout, *dmn->pdmnState,
                                    "No payout or fee source addresses found, can't revoke", fund_destination)}) {
        return *error;
    }

    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_PROVIDER_UPDATE_REVOKE;
    auto funded_result{Fund(wallet, std::move(tx), payload, fund_destination)};
    if (const auto* error{std::get_if<ProviderTxError>(&funded_result)}) return *error;
    tx = std::get<CMutableTransaction>(std::move(funded_result));
    SignPayload(tx, payload, request.operator_key, payload.nVersion == ProTxVersion::LegacyBLS);
    SetTxPayload(tx, payload);
    return Finish(node, wallet, tx, request.submit);
}

ProviderTxResult<PreparedSharedRegistration> PrepareSharedRegistration(node::NodeContext& node,
                                                                       const SharedRegistrationRequest& request)
{
    if (!IsValidMnType(request.type)) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER, "invalid masternode type");
    }
    if (request.type == MnType::Evo && !EvoSharesActive(node)) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER,
                     "EvoNode shared registration requires the evo_shares deployment to be active");
    }

    CMutableTransaction tx{request.funding_tx};
    tx.nVersion = 3;
    tx.nType = TRANSACTION_PROVIDER_REGISTER;

    CProRegTx payload;
    payload.nType = request.type;
    payload.nVersion = CurrentProviderTxVersion(node);
    if (payload.nVersion < ProTxVersion::ExtAddr) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER,
                     "shared masternode registration requires provider transaction version 3");
    }
    payload.netInfo = NetInfoInterface::MakeNetInfo(payload.nVersion);

    if (request.shares.size() < CProRegTx::MIN_SHARES || request.shares.size() > CProRegTx::MAX_SHARES) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER,
                     strprintf("shares must contain between %d and %d entries", CProRegTx::MIN_SHARES,
                               CProRegTx::MAX_SHARES));
    }
    for (const auto& share : request.shares) {
        if (!IsValidDestination(share.refund)) {
            return Error(ProviderTxErrorCode::INVALID_ADDRESS_OR_KEY,
                         strprintf("invalid refund address: %s", EncodeDestination(share.refund)));
        }
        if (share.reward && !IsValidDestination(*share.reward)) {
            return Error(ProviderTxErrorCode::INVALID_ADDRESS_OR_KEY,
                         strprintf("invalid reward address: %s", EncodeDestination(*share.reward)));
        }
        payload.shares.emplace_back(share.amount, GetScriptForDestination(share.refund),
                                    share.reward ? GetScriptForDestination(*share.reward) : CScript{}, share.owner);
    }

    if (auto error{ApplyNetInfo(payload, request.net_info, /*platform=*/payload.nType == MnType::Evo, /*optional=*/true)}) {
        return *error;
    }
    if (auto error{ApplyPlatformNodeId(payload, request.platform_node_id)}) return *error;
    if (auto error{ValidateNetworkFields(payload, /*allow_empty=*/true, /*check_platform_node_id=*/true)}) {
        return *error;
    }

    if (!request.operator_key.IsValid()) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER, "operator BLS address must be a valid BLS public key");
    }
    payload.pubKeyOperator.Set(request.operator_key, /*specificLegacyScheme=*/false);
    payload.keyIDVoting = request.voting_key;
    if (request.operator_reward > 10000) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER, "operatorReward must be between 0 and 10000");
    }
    payload.nOperatorReward = request.operator_reward;
    if (request.early_period_blocks > CProRegTx::MAX_EARLY_PERIOD_BLOCKS) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER, "earlyPeriodBlocks out of range");
    }
    payload.nEarlyPeriodBlocks = request.early_period_blocks;
    payload.nEarlyPenalty = request.early_penalty;

    // Append the shared collateral output; the collateral is always internal
    tx.vout.emplace_back(GetMnType(payload.nType).collat_amount, SharedCollateralScript());
    payload.collateralOutpoint = COutPoint(uint256(), static_cast<uint32_t>(tx.vout.size() - 1));

    // Placeholder consent signatures; filled in by CombineShared()
    payload.vchJoinSigs.assign(payload.shares.size(), CompactSignature{});

    UpdateInputsHash(tx, payload);

    // Preflight the payload with the same stateless rules consensus applies, so consensus-invalid
    // terms (bad share sums, penalty bounds, payee reuse, ...) fail here with a clear error
    // instead of after every participant has signed and the funding inputs are finalized
    if (TxValidationState state; !payload.IsTriviallyValid(state)) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER,
                     strprintf("invalid shared registration terms: %s", state.GetRejectReason()),
                     state.GetRejectReason());
    }

    SetTxPayload(tx, payload);
    PreparedSharedRegistration prepared;
    prepared.tx = MakeTransactionRef(std::move(tx));
    prepared.collateral_index = payload.collateralOutpoint.n;
    prepared.consent_hash = payload.MakeSharedRegConsentHash(*prepared.tx);
    if (payload.nEarlyPenalty == 0) prepared.warning = std::string{ZERO_PENALTY_WARNING};
    return prepared;
}

ProviderTxResult<SharedSignResult> SignShared(node::NodeContext& node, Wallet& wallet, const SharedSignRequest& request)
{
    if (auto error{CheckWallet(wallet)}) return *error;
    if (!request.tx) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER, "transaction not deserializable");
    }
    const CTransaction& tx{*request.tx};

    // The registration consent digest and the dissolution digest both commit to nLockTime and
    // every input sequence, so a lock the signer failed to notice would be silently baked into
    // their signature and delay when the transaction can confirm. Require an explicit opt-in.
    const auto check_time_lock = [&](std::string_view what) -> std::optional<ProviderTxError> {
        if (request.allow_time_locks || !HasTimeLock(node, tx)) return std::nullopt;
        return Error(ProviderTxErrorCode::INVALID_PARAMETER,
                     strprintf("%s carries a lock time or relative lock, which delays when it can confirm; pass "
                               "allowTimeLocks=true to sign it anyway",
                               what));
    };

    // Resolve the share table and the digest to sign from the transaction type
    SharedSignResult result;
    CollateralShares shares;
    uint256 sign_hash;
    if (tx.nType == TRANSACTION_PROVIDER_REGISTER) {
        const auto payload{GetTxPayload<CProRegTx>(tx)};
        if (!payload || !payload->IsShared()) {
            return Error(ProviderTxErrorCode::INVALID_PARAMETER, "transaction is not a shared masternode registration");
        }
        if (auto error{check_time_lock("registration")}) return *error;
        shares = payload->shares;
        sign_hash = payload->MakeSharedRegConsentHash(tx);
        result.kind = SharedSignResult::Kind::Registration;
        if (payload->nEarlyPenalty == 0) result.warning = std::string{ZERO_PENALTY_WARNING};
    } else if (tx.nType == TRANSACTION_PROVIDER_DISSOLVE) {
        const auto payload{GetTxPayload<CProDisTx>(tx)};
        if (!payload) {
            return Error(ProviderTxErrorCode::INVALID_PARAMETER, "transaction payload not deserializable");
        }
        // Only unanimous dissolutions (built unsigned by PrepareSharedDissolution()) are signed
        // here; the signing digest commits to the signature count, so signatures produced here
        // would never verify on a transaction carrying a different count. A unilateral
        // dissolution is built, signed and submitted in one step by DissolveShared().
        if (!payload->vchSigs.empty()) {
            return Error(ProviderTxErrorCode::INVALID_PARAMETER,
                         "transaction already carries dissolution signatures; a unilateral dissolution is created "
                         "fully signed by \"protx shared_dissolve\" and needs no shared_sign step");
        }
        const auto dmn{GetSharedMn(node, payload->proTxHash)};
        if (!dmn) return SharedMnNotFound();
        if (!ReturnsPrincipal(*dmn, payload->actorIndex, tx)) {
            return Error(ProviderTxErrorCode::INVALID_PARAMETER,
                         strprintf("dissolution does not return every share's principal to its refund address, "
                                   "less a fee of at most %d duffs from the actor's share; only dissolutions as "
                                   "built by \"protx shared_dissolve_prepare\" are signed",
                                   CProDisTx::MAX_FEE));
        }
        if (auto error{check_time_lock("dissolution")}) return *error;
        shares = dmn->pdmnState->shares;
        sign_hash = payload->MakeSignHash(tx, static_cast<uint8_t>(shares.size()));
        result.kind = SharedSignResult::Kind::Dissolution;
    } else if (tx.nType == TRANSACTION_PROVIDER_UPDATE_SHARED_REGISTRAR) {
        const auto payload{GetTxPayload<CProUpSharedRegTx>(tx)};
        if (!payload) {
            return Error(ProviderTxErrorCode::INVALID_PARAMETER, "transaction payload not deserializable");
        }
        const auto dmn{GetSharedMn(node, payload->proTxHash)};
        if (!dmn) return SharedMnNotFound();
        shares = dmn->pdmnState->shares;
        sign_hash = ::SerializeHash(*payload);
        result.kind = SharedSignResult::Kind::RegistrarUpdate;
    } else {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER, "transaction is not a shared masternode transaction");
    }

    for (size_t i = 0; i < shares.size(); i++) {
        std::vector<unsigned char> signature;
        // A share whose owner key this wallet does not hold is skipped
        if (!wallet.signSpecialTxPayload(sign_hash, shares[i].keyIDOwner, signature)) continue;
        result.signatures.push_back({i, std::move(signature)});
    }
    if (result.signatures.empty()) {
        return Error(ProviderTxErrorCode::INVALID_ADDRESS_OR_KEY, "none of the share owner keys were found in this wallet");
    }
    return result;
}

ProviderTxResult<ProviderTxSubmission> CombineShared(node::NodeContext& node, Wallet* wallet,
                                                     const SharedCombineRequest& request)
{
    if (!request.tx) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER, "transaction not deserializable");
    }
    std::map<size_t, CompactSignature> signatures;
    for (const auto& entry : request.signatures) {
        if (entry.signature.size() != CPubKey::COMPACT_SIGNATURE_SIZE) {
            return Error(ProviderTxErrorCode::INVALID_PARAMETER, "invalid signature entry");
        }
        CompactSignature signature;
        std::copy(entry.signature.begin(), entry.signature.end(), signature.begin());
        if (!signatures.emplace(entry.share_index, signature).second) {
            return Error(ProviderTxErrorCode::INVALID_PARAMETER, "duplicate shareIndex");
        }
    }

    CMutableTransaction tx{*request.tx};
    if (tx.nType == TRANSACTION_PROVIDER_REGISTER) {
        auto payload{GetTxPayload<CProRegTx>(tx)};
        if (!payload || !payload->IsShared()) {
            return Error(ProviderTxErrorCode::INVALID_PARAMETER, "transaction is not a shared masternode registration");
        }
        if (request.submit) {
            return Error(ProviderTxErrorCode::INVALID_PARAMETER,
                         "a combined registration still needs its funding inputs signed; submit it with "
                         "sendrawtransaction afterwards");
        }
        if (signatures.size() != payload->shares.size()) {
            return Error(ProviderTxErrorCode::INVALID_PARAMETER, "a registration requires a signature from every share");
        }
        for (const auto& [index, signature] : signatures) {
            if (index >= payload->shares.size()) {
                return Error(ProviderTxErrorCode::INVALID_PARAMETER, "shareIndex out of range");
            }
            payload->vchJoinSigs[index] = signature;
        }
        SetTxPayload(tx, *payload);
        return ProviderTxSubmission{MakeTransactionRef(std::move(tx)), false};
    }
    if (tx.nType == TRANSACTION_PROVIDER_DISSOLVE) {
        // SignShared() only produces signatures over the unanimous digest (which commits to the
        // signature count), so a one-signature transaction built here could never verify; the
        // only valid producer of a unilateral dissolution is DissolveShared()
        if (auto error{InsertUnanimousSignatures<CProDisTx>(
                node, tx, signatures,
                "a dissolution combined through shared_combine requires a signature from every share; a "
                "unilateral dissolution is created fully signed by \"protx shared_dissolve\"")}) {
            return *error;
        }
        if (!request.submit) return ProviderTxSubmission{MakeTransactionRef(std::move(tx)), false};
        return Submit(node, std::move(tx));
    }
    if (tx.nType == TRANSACTION_PROVIDER_UPDATE_SHARED_REGISTRAR) {
        if (auto error{InsertUnanimousSignatures<CProUpSharedRegTx>(
                node, tx, signatures, "a shared registrar update requires a signature from every share")}) {
            return *error;
        }
        if (wallet == nullptr) {
            return Error(ProviderTxErrorCode::WALLET_ERROR,
                         "a shared registrar update must be combined by the wallet that funded it");
        }
        if (auto error{CheckWallet(*wallet)}) return *error;
        if (auto error{CheckRegistrarUpdateFunding(*wallet, tx)}) return *error;
        // Inserting the signatures changed the payload, so the fee inputs signed at prepare time
        // are stale; re-sign them with this wallet
        return Finish(node, *wallet, tx, request.submit);
    }
    return Error(ProviderTxErrorCode::INVALID_PARAMETER, "transaction is not a shared masternode transaction");
}

ProviderTxResult<ProviderTxSubmission> DissolveShared(node::NodeContext& node, Wallet& wallet,
                                                      const SharedDissolveRequest& request)
{
    if (auto error{CheckWallet(wallet)}) return *error;
    const auto dmn{GetSharedMn(node, request.pro_tx_hash)};
    if (!dmn) return SharedMnNotFound();

    const int next_height{WITH_LOCK(::cs_main, return Assert(node.chainman)->ActiveChain().Height()) + 1};
    const bool early{next_height - dmn->pdmnState->nRegisteredHeight <
                     static_cast<int64_t>(dmn->pdmnState->nEarlyPeriodBlocks)};
    const bool pay_penalty{request.pay_penalty.value_or(early)};
    // Deliberate: a zero-penalty standby signed during the early period becomes valid at the
    // boundary. Refuse only when it would also be submitted now.
    if (early && !pay_penalty && request.submit) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER,
                     "a unilateral dissolution during the early period must pay the penalty; pass submit=false to "
                     "build a standby for later");
    }

    auto built{BuildDissolution(*dmn, request.actor_index, pay_penalty ? dmn->pdmnState->nEarlyPenalty : 0,
                                request.fee)};
    if (const auto* error{std::get_if<ProviderTxError>(&built)}) return *error;
    auto& [tx, payload]{std::get<Dissolution>(built)};

    std::vector<unsigned char> signature;
    if (!wallet.signSpecialTxPayload(payload.MakeSignHash(CTransaction(tx), /*sig_count=*/1),
                                     dmn->pdmnState->shares[request.actor_index].keyIDOwner, signature)) {
        return Error(ProviderTxErrorCode::INVALID_ADDRESS_OR_KEY,
                     "private key for the actor share's owner address not found in this wallet");
    }
    CompactSignature compact;
    std::copy(signature.begin(), signature.end(), compact.begin());
    payload.vchSigs = {compact};
    SetTxPayload(tx, payload);

    if (!request.submit) return ProviderTxSubmission{MakeTransactionRef(std::move(tx)), false};
    return Submit(node, std::move(tx));
}

ProviderTxResult<PreparedSharedConsent> PrepareSharedDissolution(node::NodeContext& node,
                                                                 const SharedDissolvePrepareRequest& request)
{
    const auto dmn{GetSharedMn(node, request.pro_tx_hash)};
    if (!dmn) return SharedMnNotFound();

    auto built{BuildDissolution(*dmn, request.actor_index, /*penalty=*/0, request.fee)};
    if (const auto* error{std::get_if<ProviderTxError>(&built)}) return *error;
    auto& [tx, payload]{std::get<Dissolution>(built)};
    SetTxPayload(tx, payload);

    auto prepared{MakeTransactionRef(std::move(tx))};
    // Unanimous dissolution: every share owner signs, so the count is the share count
    const uint256 sign_hash{payload.MakeSignHash(*prepared, static_cast<uint8_t>(dmn->pdmnState->shares.size()))};
    return PreparedSharedConsent{std::move(prepared), sign_hash};
}

ProviderTxResult<ProviderTxSubmission> UpdateShare(node::NodeContext& node, Wallet& wallet,
                                                   const SharedUpdateShareRequest& request)
{
    if (auto error{CheckWallet(wallet)}) return *error;
    const auto dmn{GetSharedMn(node, request.pro_tx_hash)};
    if (!dmn) return SharedMnNotFound();
    if (request.share_index >= dmn->pdmnState->shares.size()) {
        return Error(ProviderTxErrorCode::INVALID_PARAMETER, "shareIndex out of range");
    }
    if (!IsValidDestination(request.reward)) {
        return Error(ProviderTxErrorCode::INVALID_ADDRESS_OR_KEY,
                     strprintf("invalid reward address: %s", EncodeDestination(request.reward)));
    }
    if (!IsValidDestination(request.fee_source)) {
        return Error(ProviderTxErrorCode::INVALID_ADDRESS_OR_KEY,
                     "Invalid Dash address: " + EncodeDestination(request.fee_source));
    }

    CProUpShareTx payload;
    payload.proTxHash = request.pro_tx_hash;
    payload.shareIndex = request.share_index;
    payload.scriptReward = GetScriptForDestination(request.reward);
    // make sure we get enough fees added
    payload.vchSig.resize(CPubKey::COMPACT_SIGNATURE_SIZE);

    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_PROVIDER_UPDATE_SHARE;
    auto funded_result{Fund(wallet, std::move(tx), payload, request.fee_source)};
    if (const auto* error{std::get_if<ProviderTxError>(&funded_result)}) return *error;
    tx = std::get<CMutableTransaction>(std::move(funded_result));
    if (auto error{SignPayload(tx, payload, dmn->pdmnState->shares[request.share_index].keyIDOwner, wallet)}) {
        return *error;
    }
    SetTxPayload(tx, payload);
    return Finish(node, wallet, tx, request.submit, /*allow_partial=*/true);
}

ProviderTxResult<PreparedSharedConsent> PrepareSharedRegistrarUpdate(node::NodeContext& node, Wallet& wallet,
                                                                     const SharedRegistrarUpdatePrepareRequest& request)
{
    if (auto error{CheckWallet(wallet)}) return *error;
    const auto dmn{GetSharedMn(node, request.pro_tx_hash)};
    if (!dmn) return SharedMnNotFound();

    CProUpSharedRegTx payload;
    payload.proTxHash = request.pro_tx_hash;
    if (request.operator_key) {
        if (!request.operator_key->IsValid()) {
            return Error(ProviderTxErrorCode::INVALID_PARAMETER, "operator BLS address must be a valid BLS public key");
        }
        payload.pubKeyOperator.Set(*request.operator_key, /*specificLegacyScheme=*/false);
    } else {
        payload.pubKeyOperator = dmn->pdmnState->pubKeyOperator;
    }
    payload.keyIDVoting = request.voting_key.value_or(dmn->pdmnState->keyIDVoting);
    if (!IsValidDestination(request.fee_source)) {
        return Error(ProviderTxErrorCode::INVALID_ADDRESS_OR_KEY,
                     "Invalid Dash address: " + EncodeDestination(request.fee_source));
    }

    // make sure we get enough fees added: one signature per share
    payload.vchSigs.assign(dmn->pdmnState->shares.size(), CompactSignature{});

    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_PROVIDER_UPDATE_SHARED_REGISTRAR;
    auto funded_result{Fund(wallet, std::move(tx), payload, request.fee_source)};
    if (const auto* error{std::get_if<ProviderTxError>(&funded_result)}) return *error;
    tx = std::get<CMutableTransaction>(std::move(funded_result));
    UpdateInputsHash(tx, payload);
    SetTxPayload(tx, payload);
    return PreparedSharedConsent{MakeTransactionRef(std::move(tx)), ::SerializeHash(payload)};
}

} // namespace evo::provider
