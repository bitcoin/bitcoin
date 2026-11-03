// Copyright (c) 2026 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_PROVIDERTX_SERVICE_H
#define BITCOIN_EVO_PROVIDERTX_SERVICE_H

#include <interfaces/providertx.h>

#include <optional>
#include <string>

namespace interfaces {
class Wallet;
} // namespace interfaces

namespace node {
struct NodeContext;
} // namespace node

namespace evo::provider {

interfaces::ProviderTxCapabilities GetCapabilities(node::NodeContext& node,
                                                   std::optional<bool> basic_override = std::nullopt);

std::optional<interfaces::ProviderTxError> ValidateNetInfo(const interfaces::ProviderNetInfo& net_info, MnType type,
                                                           uint16_t version, bool optional);

std::optional<std::string> FindNetInfoInUse(node::NodeContext& node, const interfaces::ProviderNetInfo& net_info,
                                            MnType type, uint16_t version);

interfaces::ProviderTxResult<interfaces::ProviderTxSubmission> Register(
    node::NodeContext& node, interfaces::Wallet& wallet, const interfaces::ProviderRegistrationRequest& request);

interfaces::ProviderTxResult<interfaces::PreparedProviderRegistration> PrepareRegistration(
    node::NodeContext& node, interfaces::Wallet& wallet, const interfaces::ProviderRegistrationRequest& request);

interfaces::ProviderTxResult<interfaces::ProviderTxSubmission> SubmitRegistration(
    node::NodeContext& node, interfaces::Wallet& wallet, const CTransactionRef& tx,
    const std::vector<unsigned char>& collateral_signature);

interfaces::ProviderTxResult<interfaces::ProviderTxSubmission> UpdateService(
    node::NodeContext& node, interfaces::Wallet& wallet, const interfaces::ProviderUpdateServiceRequest& request);

interfaces::ProviderTxResult<interfaces::ProviderTxSubmission> UpdateRegistrar(
    node::NodeContext& node, interfaces::Wallet& wallet, const interfaces::ProviderUpdateRegistrarRequest& request);

interfaces::ProviderTxResult<interfaces::ProviderTxSubmission> Revoke(node::NodeContext& node, interfaces::Wallet& wallet,
                                                                      const interfaces::ProviderRevokeRequest& request);

interfaces::ProviderTxResult<interfaces::PreparedSharedRegistration> PrepareSharedRegistration(
    node::NodeContext& node, const interfaces::SharedRegistrationRequest& request);

interfaces::ProviderTxResult<interfaces::SharedSignResult> SignShared(node::NodeContext& node, interfaces::Wallet& wallet,
                                                                      const interfaces::SharedSignRequest& request);

//! `wallet` may be null unless the transaction is a ProUpSharedRegTx
interfaces::ProviderTxResult<interfaces::ProviderTxSubmission> CombineShared(
    node::NodeContext& node, interfaces::Wallet* wallet, const interfaces::SharedCombineRequest& request);

interfaces::ProviderTxResult<interfaces::ProviderTxSubmission> DissolveShared(
    node::NodeContext& node, interfaces::Wallet& wallet, const interfaces::SharedDissolveRequest& request);

interfaces::ProviderTxResult<interfaces::PreparedSharedConsent> PrepareSharedDissolution(
    node::NodeContext& node, const interfaces::SharedDissolvePrepareRequest& request);

interfaces::ProviderTxResult<interfaces::ProviderTxSubmission> UpdateShare(
    node::NodeContext& node, interfaces::Wallet& wallet, const interfaces::SharedUpdateShareRequest& request);

interfaces::ProviderTxResult<interfaces::PreparedSharedConsent> PrepareSharedRegistrarUpdate(
    node::NodeContext& node, interfaces::Wallet& wallet, const interfaces::SharedRegistrarUpdatePrepareRequest& request);

} // namespace evo::provider

#endif // BITCOIN_EVO_PROVIDERTX_SERVICE_H
