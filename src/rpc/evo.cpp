// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bls/bls.h>
#include <chainparams.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <deploymentstatus.h>
#include <evo/chainhelper.h>
#include <evo/deterministicmns.h>
#include <evo/dmn_types.h>
#include <evo/providertx.h>
#include <evo/providertx_service.h>
#include <evo/smldiff.h>
#include <evo/specialtx.h>
#include <evo/specialtxman.h>
#include <index/txindex.h>
#include <interfaces/wallet.h>
#include <llmq/context.h>
#include <masternode/meta.h>
#include <node/context.h>
#include <node/transaction.h>
#include <rpc/server.h>
#include <rpc/server_util.h>
#include <rpc/util.h>
#include <util/check.h>
#include <util/strencodings.h>
#include <util/translation.h>
#include <validation.h>
#include <wallet/rpc/util.h>
#include <walletinitinterface.h>

#include <algorithm>
#include <limits>
#include <optional>
#include <set>
#include <string_view>

#ifdef ENABLE_WALLET
#include <wallet/wallet.h>
#endif // ENABLE_WALLET

#ifndef ENABLE_WALLET
namespace wallet {
class CWallet;
} // namespace wallet
#endif // ENABLE_WALLET

using node::GetTransaction;
using node::NodeContext;
using wallet::CWallet;
#ifdef ENABLE_WALLET
using wallet::DEFAULT_DISABLE_WALLET;
using wallet::GetWalletForJSONRPCRequest;
using wallet::HELP_REQUIRING_PASSPHRASE;
using wallet::isminetype;
#endif // ENABLE_WALLET

// Defined here rather than with the other ToJson() in evo/core_write.cpp: dash-tx never prints
// a masternode entry, and the g_txindex lookup below needs libbitcoin_node anyway. Hosting it
// in evo/deterministicmns.cpp instead would create four new circular dependencies.
UniValue CDeterministicMN::ToJson() const
{
    UniValue obj(UniValue::VOBJ);
    obj.pushKV("type", std::string(GetMnType(nType).description));
    obj.pushKV("proTxHash", proTxHash.ToString());
    obj.pushKV("collateralHash", collateralOutpoint.hash.ToString());
    obj.pushKV("collateralIndex", collateralOutpoint.n);

    if (g_txindex) {
        CTransactionRef collateralTx;
        uint256 nBlockHash;
        g_txindex->FindTx(collateralOutpoint.hash, nBlockHash, collateralTx);
        if (collateralTx) {
            CTxDestination dest;
            if (ExtractDestination(collateralTx->vout[collateralOutpoint.n].scriptPubKey, dest)) {
                obj.pushKV("collateralAddress", EncodeDestination(dest));
            }
        }
    }

    obj.pushKV("operatorReward", static_cast<double>(nOperatorReward) / 100);
    obj.pushKV("state", pdmnState->ToJson(nType));
    return obj;
}

static RPCArg GetRpcArg(const std::string& strParamName)
{
    static const std::map<std::string, RPCArg> mapParamHelp = {
        {"collateralAddress",
            {"collateralAddress", RPCArg::Type::STR, RPCArg::Optional::NO,
                "The Dash address to send the collateral to."}
        },
        {"collateralHash",
            {"collateralHash", RPCArg::Type::STR, RPCArg::Optional::NO,
                "The collateral transaction hash."}
        },
        {"collateralIndex",
            {"collateralIndex", RPCArg::Type::NUM, RPCArg::Optional::NO,
                "The collateral transaction output index."}
        },
        {"feeSourceAddress",
            {"feeSourceAddress", RPCArg::Type::STR, RPCArg::Default{""},
                "If specified wallet will only use coins from this address to fund ProTx.\n"
                "If not specified, payoutAddress is the one that is going to be used.\n"
                "The private key belonging to this address must be known in your wallet."}
        },
        {"fundAddress",
            {"fundAddress", RPCArg::Type::STR, RPCArg::Default{""},
                "If specified wallet will only use coins from this address to fund ProTx.\n"
                "If not specified, payoutAddress is the one that is going to be used.\n"
                "The private key belonging to this address must be known in your wallet."}
        },
        {"coreP2PAddrs",
            {"coreP2PAddrs", RPCArg::Type::ARR, RPCArg::Optional::NO,
                "Array of addresses in the form \"ADDR:PORT\". Must be unique on the network.\n"
                "A legacy ProTx can only store a single entry; storing multiple entries\n"
                "requires upgrading to a version 3 ProTx.\n"
                "The first entry must be IPv4 where masternodes require IPv4 (mainnet and, by default, regtest).\n"
                "On mainnet, the first entry must use port 9999.\n"
                "Can be set to an empty string, which will require a ProUpServTx afterwards.",
                {
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, ""},
                },
                RPCArgOptions{.skip_type_check = true}}
        },
        {"coreP2PAddrs_update",
            {"coreP2PAddrs", RPCArg::Type::ARR, RPCArg::Optional::NO,
                "Array of addresses in the form \"ADDR:PORT\". Must be unique on the network.\n"
                "A legacy ProTx can only store a single entry; storing multiple entries\n"
                "requires upgrading to a version 3 ProTx.\n"
                "The first entry must be IPv4 where masternodes require IPv4 (mainnet and, by default, regtest).\n"
                "On mainnet, the first entry must use port 9999.",
                {
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, ""},
                },
                RPCArgOptions{.skip_type_check = true}}
        },
        {"operatorKey",
            {"operatorKey", RPCArg::Type::STR, RPCArg::Optional::NO,
                "The operator BLS private key associated with the\n"
                "registered operator public key."}
        },
        {"operatorPayoutAddress",
            {"operatorPayoutAddress", RPCArg::Type::STR, RPCArg::Default{""},
                "The address used for operator reward payments.\n"
                "Only allowed when the ProRegTx had a non-zero operatorReward value.\n"
                "If set to an empty string, the currently active payout address is reused."}
        },
        {"operatorPubKey_register",
            {"operatorPubKey", RPCArg::Type::STR, RPCArg::Optional::NO,
                "The operator BLS public key. The BLS private key does not have to be known.\n"
                "It has to match the BLS private key which is later used when operating the masternode."}
        },
        {"operatorPubKey_register_legacy",
            {"operatorPubKey", RPCArg::Type::STR, RPCArg::Optional::NO,
                "The operator BLS public key in legacy scheme. The BLS private key does not have to be known.\n"
                "It has to match the BLS private key which is later used when operating the masternode.\n"}
        },
        {"operatorPubKey_update",
            {"operatorPubKey", RPCArg::Type::STR, RPCArg::Optional::NO,
                "The operator BLS public key. The BLS private key does not have to be known.\n"
                "It has to match the BLS private key which is later used when operating the masternode.\n"
                "If set to an empty string, the currently active operator BLS public key is reused."}
        },
        {"operatorPubKey_update_legacy",
            {"operatorPubKey", RPCArg::Type::STR, RPCArg::Optional::NO,
                "The operator BLS public key in legacy scheme. The BLS private key does not have to be known.\n"
                "It has to match the BLS private key which is later used when operating the masternode.\n"
                "If set to an empty string, the currently active operator BLS public key is reused."}
        },
        {"operatorReward",
            {"operatorReward", RPCArg::Type::STR, RPCArg::Optional::NO,
                "The fraction in %% to share with the operator.\n"
                "The value must be between 0 and 10000.",
                RPCArgOptions{.skip_type_check = true}}
        },
        {"ownerAddress",
            {"ownerAddress", RPCArg::Type::STR, RPCArg::Optional::NO,
                "The Dash address to use for payee updates and proposal voting.\n"
                "The corresponding private key does not have to be known by your wallet.\n"
                "The address must be unused and must differ from the collateralAddress."}
        },
        {"payoutAddress_register",
            {"payoutAddress", RPCArg::Type::ARR, RPCArg::Optional::NO,
                "The Dash address to use for masternode reward payments, or after v24 activation, "
                "an array of payout shares. Not compatible with legacy bls operator key.",
                {
                    {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "",
                        {
                            {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The Dash payout address."},
                            {"reward", RPCArg::Type::NUM, RPCArg::Optional::NO, "The payout share in basis points."},
                        }},
                },
                RPCArgOptions{
                    .skip_type_check = true,
                    .oneline_description={"\"payoutAddress\" | [{\"address\",\"reward\"},...] (string or array)"},
                    .type_str={"string or array", "string or array"},
                }}
        },
        {"payoutAddress_update",
            {"payoutAddress", RPCArg::Type::ARR, RPCArg::Optional::NO,
                "The Dash address to use for masternode reward payments, or after v24 activation, "
                "an array of payout shares. Not compatible with legacy bls operator key.\n"
                "If set to an empty string, the currently active payout address is reused.",
                {
                    {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "",
                        {
                            {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The Dash payout address."},
                            {"reward", RPCArg::Type::NUM, RPCArg::Optional::NO, "The payout share in basis points."},
                        }},
                },
                RPCArgOptions{
                    .skip_type_check = true,
                    .oneline_description={"\"payoutAddress\" | [{\"address\",\"reward\"},...] (string or array)"},
                    .type_str={"string or array", "string or array"},
                }}
        },
        {"proTxHash",
            {"proTxHash", RPCArg::Type::STR, RPCArg::Optional::NO,
                "The hash of the initial ProRegTx."}
        },
        {"reason",
            {"reason", RPCArg::Type::NUM, RPCArg::DefaultHint{"Reason is not specified"},
                "The reason for masternode service revocation."}
        },
        {"submit",
            {"submit", RPCArg::Type::BOOL, RPCArg::Default{true},
                "If true, the resulting transaction is sent to the network.",
                RPCArgOptions{.skip_type_check = true}}
        },
        {"votingAddress_register",
            {"votingAddress", RPCArg::Type::STR, RPCArg::Optional::NO,
                "The voting key address. The private key does not have to be known by your wallet.\n"
                "It has to match the private key which is later used when voting on proposals.\n"
                "If set to an empty string, ownerAddress will be used."}
        },
        {"votingAddress_update",
            {"votingAddress", RPCArg::Type::STR, RPCArg::Optional::NO,
                "The voting key address. The private key does not have to be known by your wallet.\n"
                "It has to match the private key which is later used when voting on proposals.\n"
                "If set to an empty string, the currently active voting key address is reused."}
        },
        {"platformNodeID",
            {"platformNodeID", RPCArg::Type::STR, RPCArg::Optional::NO,
                "Platform P2P node ID, derived from P2P public key."}
        },
        {"platformP2PAddrs",
            {"platformP2PAddrs", RPCArg::Type::ARR, RPCArg::Optional::NO,
                "Array of addresses in the form \"ADDR:PORT\" used by Platform for peer-to-peer connection.\n"
                "For a legacy ProTx, pass a bare port number instead (e.g. 26656);\n"
                "the \"ADDR:PORT\" / address-array form requires upgrading to a version 3 ProTx.\n"
                "Where masternodes require IPv4 (mainnet and, by default, regtest), the first entry must use the\n"
                "first coreP2PAddrs address.\n"
                "On mainnet, the first entry must use port 26656.\n"
                "Must be unique on the network. Can be set to an empty string, which will require a ProUpServTx afterwards.",
                {
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, ""},
                },
                RPCArgOptions{.skip_type_check = true}}
        },
        {"platformP2PAddrs_update",
            {"platformP2PAddrs", RPCArg::Type::ARR, RPCArg::Optional::NO,
                "Array of addresses in the form \"ADDR:PORT\" used by Platform for peer-to-peer connection.\n"
                "For a legacy ProTx, pass a bare port number instead (e.g. 26656);\n"
                "the \"ADDR:PORT\" / address-array form requires upgrading to a version 3 ProTx.\n"
                "Where masternodes require IPv4 (mainnet and, by default, regtest), the first entry must use the\n"
                "first coreP2PAddrs address.\n"
                "On mainnet, the first entry must use port 26656.\n"
                "Must be unique on the network.",
                {
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, ""},
                },
                RPCArgOptions{.skip_type_check = true}}
        },
        {"platformHTTPSAddrs",
            {"platformHTTPSAddrs", RPCArg::Type::ARR, RPCArg::Optional::NO,
                "Array of addresses in the form \"ADDR:PORT\" used by Platform for their HTTPS API.\n"
                "For a legacy ProTx, pass a bare port number instead (e.g. 443);\n"
                "the \"ADDR:PORT\" / address-array form requires upgrading to a version 3 ProTx.\n"
                "Where masternodes require IPv4 (mainnet and, by default, regtest), the first entry must use the\n"
                "first coreP2PAddrs address.\n"
                "On mainnet, the first entry must use port 443.\n"
                "Must be unique on the network. Can be set to an empty string, which will require a ProUpServTx afterwards.",
                {
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, ""},
                },
                RPCArgOptions{.skip_type_check = true}}
        },
        {"platformHTTPSAddrs_update",
            {"platformHTTPSAddrs", RPCArg::Type::ARR, RPCArg::Optional::NO,
                "Array of addresses in the form \"ADDR:PORT\" used by Platform for their HTTPS API.\n"
                "For a legacy ProTx, pass a bare port number instead (e.g. 443);\n"
                "the \"ADDR:PORT\" / address-array form requires upgrading to a version 3 ProTx.\n"
                "Where masternodes require IPv4 (mainnet and, by default, regtest), the first entry must use the\n"
                "first coreP2PAddrs address.\n"
                "On mainnet, the first entry must use port 443.\n"
                "Must be unique on the network.",
                {
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, ""},
                },
                RPCArgOptions{.skip_type_check = true}}
        },
    };

    auto it = mapParamHelp.find(strParamName);
    if (it == mapParamHelp.end())
        throw std::runtime_error(strprintf("FIXME: WRONG PARAM NAME %s!", strParamName));

    return it->second;
}

static CBLSSecretKey ParseBLSSecretKey(const std::string& hexKey, const std::string& paramName)
{
    CBLSSecretKey secKey;

    // Actually, bool flag for bls::PrivateKey has other meaning (modOrder)
    if (!secKey.SetHexStr(hexKey, false)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s must be a valid BLS secret key", paramName));
    }
    return secKey;
}

#ifdef ENABLE_WALLET
// The shared-masternode helpers and RPCs below all feed into wallet-gated signing (protx
// shared_sign / shared_dissolve), so they are only compiled with wallet support.

static std::vector<interfaces::SharedShareSpec> ParseShares(const UniValue& value, const std::string& paramName)
{
    if (!value.isArray()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s must be an array", paramName));
    }
    const auto& arr = value.get_array();
    if (arr.size() < CProRegTx::MIN_SHARES || arr.size() > CProRegTx::MAX_SHARES) {
        throw JSONRPCError(RPC_INVALID_PARAMETER,
                           strprintf("%s must contain between %d and %d entries", paramName, CProRegTx::MIN_SHARES,
                                     CProRegTx::MAX_SHARES));
    }
    std::vector<interfaces::SharedShareSpec> shares;
    for (size_t i = 0; i < arr.size(); ++i) {
        const UniValue& entry = arr[i];
        if (!entry.isObject()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s entries must be objects", paramName));
        }
        const UniValue& amount_value = entry.find_value("amount");
        const UniValue& refund_value = entry.find_value("refundAddress");
        const UniValue& reward_value = entry.find_value("rewardAddress");
        const UniValue& owner_value = entry.find_value("ownerAddress");
        if (!amount_value.isNum() || !refund_value.isStr() || !owner_value.isStr()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER,
                               strprintf("%s entries must include numeric amount, refundAddress and ownerAddress", paramName));
        }
        interfaces::SharedShareSpec share;
        share.amount = amount_value.getInt<int64_t>();
        share.refund = DecodeDestination(refund_value.get_str());
        if (!IsValidDestination(share.refund)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("invalid refund address: %s", refund_value.get_str()));
        }
        if (reward_value.isStr() && !reward_value.get_str().empty()) {
            share.reward = DecodeDestination(reward_value.get_str());
            if (!IsValidDestination(*share.reward)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("invalid reward address: %s", reward_value.get_str()));
            }
        }
        CTxDestination owner_dest = DecodeDestination(owner_value.get_str());
        const PKHash* owner_pkhash = std::get_if<PKHash>(&owner_dest);
        if (!owner_pkhash) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("invalid share owner address: %s", owner_value.get_str()));
        }
        share.owner = ToKeyID(*owner_pkhash);
        shares.push_back(std::move(share));
    }
    return shares;
}

//! Parse a required index parameter that must fit a uint16_t, as every share and actor index does
static uint16_t ParseShareIndex(const UniValue& value, const std::string& paramName)
{
    const int index{value.getInt<int>()};
    if (index < 0 || index > std::numeric_limits<uint16_t>::max()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s out of range", paramName));
    }
    return static_cast<uint16_t>(index);
}

//! Parse an address a fee is paid from, which the shared masternode RPCs require
static CTxDestination ParseRequiredFeeSource(const UniValue& value)
{
    CTxDestination fee_source{DecodeDestination(value.get_str())};
    if (!IsValidDestination(fee_source)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Dash address: ") + value.get_str());
    }
    return fee_source;
}

static CKeyID ParsePubKeyIDFromAddress(const std::string& strAddress, const std::string& paramName)
{
    CTxDestination dest = DecodeDestination(strAddress);
    const PKHash *pkhash = std::get_if<PKHash>(&dest);
    if (!pkhash) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s must be a valid P2PKH address, not %s", paramName, strAddress));
    }
    return ToKeyID(*pkhash);
}

static CBLSPublicKey ParseBLSPubKey(const std::string& hexKey, const std::string& paramName, bool specific_legacy_bls_scheme)
{
    CBLSPublicKey pubKey;
    if (!pubKey.SetHexStr(hexKey, specific_legacy_bls_scheme)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s must be a valid BLS public key, not %s", paramName, hexKey));
    }
    return pubKey;
}

static std::vector<interfaces::ProviderPayout> ParsePayouts(const UniValue& value, const std::string& paramName)
{
    std::vector<interfaces::ProviderPayout> payouts;
    if (value.isArray()) {
        const auto& arr = value.get_array();
        if (arr.empty()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s must contain at least one entry", paramName));
        }
        if (arr.size() > 8) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s must not contain more than 8 entries", paramName));
        }
        for (size_t i = 0; i < arr.size(); ++i) {
            const UniValue& entry = arr[i];
            if (!entry.isObject()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s entries must be objects", paramName));
            }
            const UniValue& address_value = entry.find_value("address");
            const UniValue& reward_value = entry.find_value("reward");
            if (!address_value.isStr() || !reward_value.isNum()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s entries must include address and numeric reward", paramName));
            }
            CTxDestination dest = DecodeDestination(address_value.get_str());
            if (!IsValidDestination(dest)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("invalid payout address: %s", address_value.get_str()));
            }
            const int64_t reward = reward_value.getInt<int64_t>();
            if (reward < 0 || reward > std::numeric_limits<uint16_t>::max()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "payout reward out of range");
            }
            payouts.push_back({dest, static_cast<uint16_t>(reward)});
        }
    } else {
        CTxDestination destination{DecodeDestination(value.get_str())};
        if (!IsValidDestination(destination)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("invalid payout address: %s", value.get_str()));
        }
        payouts.push_back({destination, MasternodePayoutShare::MAX_REWARD});
    }
    return payouts;
}

static std::unique_ptr<interfaces::Wallet> MakeWalletInterface(const NodeContext& node,
                                                               const std::shared_ptr<CWallet>& wallet)
{
    auto* wallet_loader{CHECK_NONFATAL(node.wallet_loader)};
    return interfaces::MakeWallet(*CHECK_NONFATAL(wallet_loader->context()), wallet);
}

[[noreturn]] static void ThrowProviderTxError(const interfaces::ProviderTxError& provider_error)
{
    using interfaces::ProviderTxErrorCode;
    switch (provider_error.code) {
    case ProviderTxErrorCode::INVALID_PARAMETER:
        throw JSONRPCError(RPC_INVALID_PARAMETER, provider_error.message.original);
    case ProviderTxErrorCode::INVALID_ADDRESS_OR_KEY:
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, provider_error.message.original);
    case ProviderTxErrorCode::FUNDING_ERROR:
        // Preserve the historical protx RPC error code while exposing a useful typed category.
        throw JSONRPCError(RPC_INTERNAL_ERROR, provider_error.message.original);
    case ProviderTxErrorCode::WALLET_ERROR:
        throw JSONRPCError(RPC_WALLET_ERROR, provider_error.message.original);
    case ProviderTxErrorCode::WALLET_UNLOCK_NEEDED:
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, provider_error.message.original);
    case ProviderTxErrorCode::INTERNAL_ERROR:
        throw JSONRPCError(RPC_INTERNAL_ERROR, provider_error.message.original);
    case ProviderTxErrorCode::VERIFY_ERROR:
        throw JSONRPCError(RPC_VERIFY_ERROR, provider_error.message.original);
    case ProviderTxErrorCode::CONSENSUS_REJECTED:
        throw std::runtime_error(provider_error.message.original);
    case ProviderTxErrorCode::BROADCAST_ERROR:
        throw JSONRPCTransactionError(*CHECK_NONFATAL(provider_error.transaction_error), provider_error.message.original);
    }
    NONFATAL_UNREACHABLE();
}

static std::string SubmissionToString(const interfaces::ProviderTxSubmission& submission)
{
    return submission.submitted ? submission.tx->GetHash().GetHex() : EncodeHexTx(*submission.tx);
}

template <typename T>
static T UnwrapOrThrow(interfaces::ProviderTxResult<T> result)
{
    if (const auto* error{std::get_if<interfaces::ProviderTxError>(&result)}) ThrowProviderTxError(*error);
    return std::get<T>(std::move(result));
}

static std::optional<CTxDestination> ParseFeeSource(const UniValue& param)
{
    if (param.isNull()) return std::nullopt;
    CTxDestination fee_source{DecodeDestination(param.get_str())};
    if (!IsValidDestination(fee_source)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Dash address: ") + param.get_str());
    }
    return fee_source;
}

static std::vector<std::string> ParseCoreNetInfo(const UniValue& input, bool optional)
{
    if (input.isStr()) {
        if (input.get_str().empty()) {
            if (!optional) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid param for coreP2PAddrs[0], cannot be empty");
            }
            return {};
        }
        return {input.get_str()};
    }
    if (!input.isArray()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid param for coreP2PAddrs, must be string or array");
    }
    const auto& entries{input.get_array()};
    if (!optional && entries.empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid param for coreP2PAddrs, cannot be empty");
    }
    std::vector<std::string> result;
    result.reserve(entries.size());
    for (size_t index{0}; index < entries.size(); ++index) {
        if (!entries[index].isStr()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER,
                               strprintf("Invalid param for coreP2PAddrs[%zu], must be string", index));
        }
        result.push_back(entries[index].get_str());
    }
    return result;
}

static interfaces::ProviderPlatformEndpoints ParsePlatformNetInfo(const UniValue& input, std::string_view field_name,
                                                                  bool extended_addresses)
{
    if (!input.isArray() && !input.isNum() && !input.isStr()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER,
                           strprintf("Invalid param for %s, must be array, number or string", field_name));
    }
    if (input.isArray()) {
        std::vector<std::string> result;
        const auto& entries{input.get_array()};
        if (!entries.empty() && !extended_addresses) {
            throw JSONRPCError(RPC_INVALID_PARAMETER,
                               strprintf("Invalid param for %s, this ProTx version only accepts a bare port number; "
                                         "the "
                                         "\"ADDR:PORT\" / address-array form requires upgrading to a version 3 ProTx",
                                         field_name));
        }
        result.reserve(entries.size());
        for (size_t index{0}; index < entries.size(); ++index) {
            if (!entries[index].isStr() || entries[index].get_str().find_first_not_of("0123456789") == std::string::npos) {
                throw JSONRPCError(RPC_INVALID_PARAMETER,
                                   strprintf("Invalid param for %s[%zu], must be string", field_name, index));
            }
            result.push_back(entries[index].get_str());
        }
        return result;
    }
    const std::string& value{input.getValStr()};
    if (value.empty()) return std::monostate{};
    int32_t port{0};
    if (ParseInt32(value, &port) && port >= 1 && port <= std::numeric_limits<uint16_t>::max()) {
        return static_cast<uint16_t>(port);
    }
    if (input.isStr() && value.find_first_not_of("0123456789") != std::string::npos)
        return std::vector<std::string>{value};
    throw JSONRPCError(RPC_INVALID_PARAMETER,
                       strprintf("Invalid param for %s, must be a valid port [1-65535]", field_name));
}

static uint160 ParsePlatformNodeID(const UniValue& value)
{
    if (!IsHex(value.get_str())) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "platformNodeID must be hexadecimal string");
    }
    uint160 platform_node_id;
    platform_node_id.SetHex(value.get_str());
    return platform_node_id;
}

// forward declaration
namespace {
enum class ProTxRegisterAction
{
    External,
    Fund,
    Prepare,
};
} // anonymous namespace

static UniValue protx_register_common_wrapper(const JSONRPCRequest& request,
                                              const bool specific_legacy_bls_scheme,
                                              ProTxRegisterAction action,
                                              const MnType mnType);

static UniValue protx_update_service_common_wrapper(const JSONRPCRequest& request, const MnType mnType);


static RPCHelpMan protx_register_fund_wrapper(const bool legacy)
{
    std::string rpc_name = legacy ? "register_fund_legacy" : "register_fund";
    std::string rpc_full_name = std::string("protx ").append(rpc_name);
    std::string pubkey_operator = legacy ? "\"0532646990082f4fd639f90387b1551f2c7c39d37392cb9055a06a7e85c1d23692db8f87f827886310bccc1e29db9aee\"" : "\"8532646990082f4fd639f90387b1551f2c7c39d37392cb9055a06a7e85c1d23692db8f87f827886310bccc1e29db9aee\"";
    std::string rpc_example = rpc_name.append(" \"" + EXAMPLE_ADDRESS[0] + "\" \"1.2.3.4:1234\" \"" + EXAMPLE_ADDRESS[1] + "\" ").append(pubkey_operator).append(" \"" + EXAMPLE_ADDRESS[1] + "\" 0 \"" + EXAMPLE_ADDRESS[0] + "\"");
    return RPCHelpMan{rpc_full_name,
        "\nCreates, funds and sends a ProTx to the network. The resulting transaction will move 1000 Dash\n"
        "to the address specified by collateralAddress and will then function as the collateral of your\n"
        "masternode.\n"
        "A few of the limitations you see in the arguments are temporary and might be lifted after DIP3\n"
        "is fully deployed.\n"
        + std::string(legacy ? "\nDEPRECATED: May be removed in a future version, pass config option -deprecatedrpc=legacy_mn to use RPC\n" : "")
        + HELP_REQUIRING_PASSPHRASE,
        {
            GetRpcArg("collateralAddress"),
            GetRpcArg("coreP2PAddrs"),
            GetRpcArg("ownerAddress"),
            legacy ? GetRpcArg("operatorPubKey_register_legacy") : GetRpcArg("operatorPubKey_register"),
            GetRpcArg("votingAddress_register"),
            GetRpcArg("operatorReward"),
            GetRpcArg("payoutAddress_register"),
            GetRpcArg("fundAddress"),
            GetRpcArg("submit"),
        },
        {
            RPCResult{"if \"submit\" is not set or set to true",
                RPCResult::Type::STR_HEX, "txid", "The transaction id"},
            RPCResult{"if \"submit\" is set to false",
                RPCResult::Type::STR_HEX, "hex", "The serialized signed ProTx in hex format"},
        },
        RPCExamples{
            HelpExampleCli("protx",  rpc_example)
        },
        [legacy](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    if (legacy && !IsDeprecatedRPCEnabled("legacy_mn")) {
        throw std::runtime_error("DEPRECATED: Pass config option -deprecatedrpc=legacy_mn to enable this RPC");
    }
    return protx_register_common_wrapper(request, self.m_name == "protx register_fund_legacy", ProTxRegisterAction::Fund, MnType::Regular);
},
    };
}

static RPCHelpMan protx_register_fund() {
    return protx_register_fund_wrapper(false);
}

static RPCHelpMan protx_register_fund_legacy() {
    return protx_register_fund_wrapper(true);
}

static RPCHelpMan protx_register_wrapper(bool legacy)
{
    std::string rpc_name = legacy ? "register_legacy" : "register";
    std::string rpc_full_name = std::string("protx ").append(rpc_name);
    std::string pubkey_operator = legacy ? "\"0532646990082f4fd639f90387b1551f2c7c39d37392cb9055a06a7e85c1d23692db8f87f827886310bccc1e29db9aee\"" : "\"8532646990082f4fd639f90387b1551f2c7c39d37392cb9055a06a7e85c1d23692db8f87f827886310bccc1e29db9aee\"";
    std::string rpc_example = rpc_name.append(" \"0123456701234567012345670123456701234567012345670123456701234567\" 0 \"1.2.3.4:1234\" \"" + EXAMPLE_ADDRESS[1] + "\" ").append(pubkey_operator).append(" \"" + EXAMPLE_ADDRESS[1] + "\" 0 \"" + EXAMPLE_ADDRESS[0] + "\"");
    return RPCHelpMan{rpc_full_name,
        "\nSame as \"protx register_fund\", but with an externally referenced collateral.\n"
        "The collateral is specified through \"collateralHash\" and \"collateralIndex\" and must be an unspent\n"
        "transaction output spendable by this wallet. It must also not be used by any other masternode.\n"
        + std::string(legacy ? "\nDEPRECATED: May be removed in a future version, pass config option -deprecatedrpc=legacy_mn to use RPC\n" : "")
        + HELP_REQUIRING_PASSPHRASE,
        {
            GetRpcArg("collateralHash"),
            GetRpcArg("collateralIndex"),
            GetRpcArg("coreP2PAddrs"),
            GetRpcArg("ownerAddress"),
            legacy ? GetRpcArg("operatorPubKey_register_legacy") : GetRpcArg("operatorPubKey_register"),
            GetRpcArg("votingAddress_register"),
            GetRpcArg("operatorReward"),
            GetRpcArg("payoutAddress_register"),
            GetRpcArg("feeSourceAddress"),
            GetRpcArg("submit"),
        },
        {
            RPCResult{"if \"submit\" is not set or set to true",
                RPCResult::Type::STR_HEX, "txid", "The transaction id"},
            RPCResult{"if \"submit\" is set to false",
                RPCResult::Type::STR_HEX, "hex", "The serialized signed ProTx in hex format"},
        },
        RPCExamples{
            HelpExampleCli("protx", rpc_example),
        },
        [legacy](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    if (legacy && !IsDeprecatedRPCEnabled("legacy_mn")) {
        throw std::runtime_error("DEPRECATED: Pass config option -deprecatedrpc=legacy_mn to enable this RPC");
    }
    return protx_register_common_wrapper(request, self.m_name == "protx register_legacy", ProTxRegisterAction::External, MnType::Regular);
},
    };
}

static RPCHelpMan protx_register()
{
    return protx_register_wrapper(false);
}

static RPCHelpMan protx_register_legacy()
{
    return protx_register_wrapper(true);
}

static RPCHelpMan protx_register_prepare_wrapper(const bool legacy)
{
    std::string rpc_name = legacy ? "register_prepare_legacy" : "register_prepare";
    std::string rpc_full_name = std::string("protx ").append(rpc_name);
    std::string pubkey_operator = legacy ? "\"0532646990082f4fd639f90387b1551f2c7c39d37392cb9055a06a7e85c1d23692db8f87f827886310bccc1e29db9aee\"" : "\"8532646990082f4fd639f90387b1551f2c7c39d37392cb9055a06a7e85c1d23692db8f87f827886310bccc1e29db9aee\"";
    std::string rpc_example = rpc_name.append(" \"0123456701234567012345670123456701234567012345670123456701234567\" 0 \"1.2.3.4:1234\" \"" + EXAMPLE_ADDRESS[1] + "\" ").append(pubkey_operator).append(" \"" + EXAMPLE_ADDRESS[1] + "\" 0 \"" + EXAMPLE_ADDRESS[0] + "\"");
    return RPCHelpMan{rpc_full_name,
        "\nCreates an unsigned ProTx and a message that must be signed externally\n"
        "with the private key that corresponds to collateralAddress to prove collateral ownership.\n"
        "The prepared transaction will also contain inputs and outputs to cover fees.\n"
        + std::string(legacy ? "\nDEPRECATED: May be removed in a future version, pass config option -deprecatedrpc=legacy_mn to use RPC\n" : ""),
        {
            GetRpcArg("collateralHash"),
            GetRpcArg("collateralIndex"),
            GetRpcArg("coreP2PAddrs"),
            GetRpcArg("ownerAddress"),
            legacy ? GetRpcArg("operatorPubKey_register_legacy") : GetRpcArg("operatorPubKey_register"),
            GetRpcArg("votingAddress_register"),
            GetRpcArg("operatorReward"),
            GetRpcArg("payoutAddress_register"),
            GetRpcArg("feeSourceAddress"),
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "tx", "The serialized unsigned ProTx in hex format"},
                {RPCResult::Type::STR_HEX, "collateralAddress", "The collateral address"},
                {RPCResult::Type::STR_HEX, "signMessage", "The string message that needs to be signed with the collateral key"},
            }},
        RPCExamples{
            HelpExampleCli("protx", rpc_example)
        },
        [legacy](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    if (legacy && !IsDeprecatedRPCEnabled("legacy_mn")) {
        throw std::runtime_error("DEPRECATED: Pass config option -deprecatedrpc=legacy_mn to enable this RPC");
    }
    return protx_register_common_wrapper(request, self.m_name == "protx register_prepare_legacy", ProTxRegisterAction::Prepare, MnType::Regular);
},
    };
}

static RPCHelpMan protx_register_prepare()
{
    return protx_register_prepare_wrapper(false);
}

static RPCHelpMan protx_register_prepare_legacy()
{
    return protx_register_prepare_wrapper(true);
}

static RPCHelpMan protx_register_fund_evo()
{
    const std::string command_name{"protx register_fund_evo"};
    return RPCHelpMan{
        command_name,
        "\nCreates, funds and sends a ProTx to the network. The resulting transaction will move 4000 Dash\n"
        "to the address specified by collateralAddress and will then function as the collateral of your\n"
        "EvoNode.\n"
        "A few of the limitations you see in the arguments are temporary and might be lifted after DIP3\n"
        "is fully deployed.\n" +
            HELP_REQUIRING_PASSPHRASE,
        {
            GetRpcArg("collateralAddress"),
            GetRpcArg("coreP2PAddrs"),
            GetRpcArg("ownerAddress"),
            GetRpcArg("operatorPubKey_register"),
            GetRpcArg("votingAddress_register"),
            GetRpcArg("operatorReward"),
            GetRpcArg("payoutAddress_register"),
            GetRpcArg("platformNodeID"),
            GetRpcArg("platformP2PAddrs"),
            GetRpcArg("platformHTTPSAddrs"),
            GetRpcArg("fundAddress"),
            GetRpcArg("submit"),
        },
        {
            RPCResult{"if \"submit\" is not set or set to true",
                RPCResult::Type::STR_HEX, "txid", "The transaction id"},
            RPCResult{"if \"submit\" is set to false",
                RPCResult::Type::STR_HEX, "hex", "The serialized signed ProTx in hex format"},
        },
        RPCExamples{
            HelpExampleCli("protx", "register_fund_evo \"" + EXAMPLE_ADDRESS[0] + "\" \"1.2.3.4:1234\" \"" + EXAMPLE_ADDRESS[1] + "\" \"93746e8731c57f87f79b3620a7982924e2931717d49540a85864bd543de11c43fb868fd63e501a1db37e19ed59ae6db4\" \"" + EXAMPLE_ADDRESS[1] + "\" 0 \"" + EXAMPLE_ADDRESS[0] + "\" \"f2dbd9b0a1f541a7c44d34a58674d0262f5feca5\" 22821 22822")},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    return protx_register_common_wrapper(request, false, ProTxRegisterAction::Fund, MnType::Evo);
},
    };
}

static RPCHelpMan protx_register_evo()
{
    const std::string command_name{"protx register_evo"};
    return RPCHelpMan{
        command_name,
        "\nSame as \"protx register_fund_evo\", but with an externally referenced collateral.\n"
        "The collateral is specified through \"collateralHash\" and \"collateralIndex\" and must be an unspent\n"
        "transaction output spendable by this wallet. It must also not be used by any other masternode.\n" +
            HELP_REQUIRING_PASSPHRASE,
        {
            GetRpcArg("collateralHash"),
            GetRpcArg("collateralIndex"),
            GetRpcArg("coreP2PAddrs"),
            GetRpcArg("ownerAddress"),
            GetRpcArg("operatorPubKey_register"),
            GetRpcArg("votingAddress_register"),
            GetRpcArg("operatorReward"),
            GetRpcArg("payoutAddress_register"),
            GetRpcArg("platformNodeID"),
            GetRpcArg("platformP2PAddrs"),
            GetRpcArg("platformHTTPSAddrs"),
            GetRpcArg("feeSourceAddress"),
            GetRpcArg("submit"),
        },
        {
            RPCResult{"if \"submit\" is not set or set to true",
                RPCResult::Type::STR_HEX, "txid", "The transaction id"},
            RPCResult{"if \"submit\" is set to false",
                RPCResult::Type::STR_HEX, "hex", "The serialized signed ProTx in hex format"},
        },
        RPCExamples{
            HelpExampleCli("protx", "register_evo \"0123456701234567012345670123456701234567012345670123456701234567\" 0 \"1.2.3.4:1234\" \"" + EXAMPLE_ADDRESS[1] + "\" \"93746e8731c57f87f79b3620a7982924e2931717d49540a85864bd543de11c43fb868fd63e501a1db37e19ed59ae6db4\" \"" + EXAMPLE_ADDRESS[1] + "\" 0 \"" + EXAMPLE_ADDRESS[0] + "\" \"f2dbd9b0a1f541a7c44d34a58674d0262f5feca5\" 22821 22822")},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    return protx_register_common_wrapper(request, false, ProTxRegisterAction::External, MnType::Evo);
},
    };
}

static RPCHelpMan protx_register_prepare_evo()
{
    const std::string command_name{"protx register_prepare_evo"};
    return RPCHelpMan{
        command_name,
        "\nCreates an unsigned ProTx and a message that must be signed externally\n"
        "with the private key that corresponds to collateralAddress to prove collateral ownership.\n"
        "The prepared transaction will also contain inputs and outputs to cover fees.\n",
        {
            GetRpcArg("collateralHash"),
            GetRpcArg("collateralIndex"),
            GetRpcArg("coreP2PAddrs"),
            GetRpcArg("ownerAddress"),
            GetRpcArg("operatorPubKey_register"),
            GetRpcArg("votingAddress_register"),
            GetRpcArg("operatorReward"),
            GetRpcArg("payoutAddress_register"),
            GetRpcArg("platformNodeID"),
            GetRpcArg("platformP2PAddrs"),
            GetRpcArg("platformHTTPSAddrs"),
            GetRpcArg("feeSourceAddress"),
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "", {
                                              {RPCResult::Type::STR_HEX, "tx", "The serialized unsigned ProTx in hex format"},
                                              {RPCResult::Type::STR_HEX, "collateralAddress", "The collateral address"},
                                              {RPCResult::Type::STR_HEX, "signMessage", "The string message that needs to be signed with the collateral key"},
                                          }},
        RPCExamples{HelpExampleCli("protx", "register_prepare_evo \"0123456701234567012345670123456701234567012345670123456701234567\" 0 \"1.2.3.4:1234\" \"" + EXAMPLE_ADDRESS[1] + "\" \"93746e8731c57f87f79b3620a7982924e2931717d49540a85864bd543de11c43fb868fd63e501a1db37e19ed59ae6db4\" \"" + EXAMPLE_ADDRESS[1] + "\" 0 \"" + EXAMPLE_ADDRESS[0] + "\" \"f2dbd9b0a1f541a7c44d34a58674d0262f5feca5\" 22821 22822")},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    return protx_register_common_wrapper(request, false, ProTxRegisterAction::Prepare, MnType::Evo);
},
    };
}

static UniValue protx_register_common_wrapper(const JSONRPCRequest& request,
                                              const bool specific_legacy_bls_scheme,
                                              const ProTxRegisterAction action,
                                              const MnType mnType)
{
    NodeContext& node{EnsureAnyNodeContext(request.context)};
    std::shared_ptr<CWallet> const pwallet{GetWalletForJSONRPCRequest(request)};
    if (!pwallet) return UniValue::VNULL;
    EnsureWalletIsUnlocked(*pwallet);

    interfaces::ProviderRegistrationRequest typed_request;
    typed_request.type = mnType;
    typed_request.use_legacy_bls_scheme = specific_legacy_bls_scheme;
    size_t paramIdx{0};
    if (action == ProTxRegisterAction::Fund) {
        CTxDestination collateral_destination{DecodeDestination(request.params[paramIdx].get_str())};
        if (!IsValidDestination(collateral_destination)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                               strprintf("invalid collateral address: %s", request.params[paramIdx].get_str()));
        }
        typed_request.collateral = interfaces::FundProviderCollateral{collateral_destination};
        paramIdx++;
    } else {
        const uint256 collateral_hash{ParseHashV(request.params[paramIdx], "collateralHash")};
        const int32_t collateral_index{request.params[paramIdx + 1].getInt<int>()};
        if (collateral_hash.IsNull() || collateral_index < 0) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                               strprintf("invalid hash or index: %s-%d", collateral_hash.ToString(), collateral_index));
        }
        typed_request.collateral = interfaces::ExistingProviderCollateral{
            COutPoint{collateral_hash, static_cast<uint32_t>(collateral_index)}};
        paramIdx += 2;
    }

    typed_request.net_info.core_p2p = ParseCoreNetInfo(request.params[paramIdx], /*optional=*/true);
    typed_request.owner_key = ParsePubKeyIDFromAddress(request.params[paramIdx + 1].get_str(), "owner address");
    typed_request.operator_key = ParseBLSPubKey(request.params[paramIdx + 2].get_str(), "operator BLS address",
                                                specific_legacy_bls_scheme);
    typed_request.voting_key = typed_request.owner_key;
    if (!request.params[paramIdx + 3].get_str().empty()) {
        typed_request.voting_key = ParsePubKeyIDFromAddress(request.params[paramIdx + 3].get_str(), "voting address");
    }

    int64_t operatorReward;
    if (!ParseFixedPoint(request.params[paramIdx + 4].getValStr(), 2, &operatorReward)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "operatorReward must be a number");
    }
    if (operatorReward < 0 || operatorReward > 10000) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "operatorReward must be between 0 and 10000");
    }
    typed_request.operator_reward = static_cast<uint16_t>(operatorReward);
    const auto capabilities{evo::provider::GetCapabilities(node, /*basic_override=*/!specific_legacy_bls_scheme)};
    typed_request.uses_extended_payouts = request.params[paramIdx + 5].isArray();
    if (typed_request.uses_extended_payouts && !capabilities.extended_addresses) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "payouts array requires provider transaction version 3");
    }
    typed_request.payouts = ParsePayouts(request.params[paramIdx + 5], "payouts");

    if (mnType == MnType::Evo) {
        typed_request.platform_node_id = ParsePlatformNodeID(request.params[paramIdx + 6]);
        typed_request.net_info.platform_p2p = ParsePlatformNetInfo(request.params[paramIdx + 7], "platformP2PAddrs",
                                                                   capabilities.extended_addresses);
        typed_request.net_info.platform_https = ParsePlatformNetInfo(request.params[paramIdx + 8], "platformHTTPSAddrs",
                                                                     capabilities.extended_addresses);
        paramIdx += 3;
    }

    typed_request.fee_source = ParseFeeSource(request.params[paramIdx + 6]);
    if ((action == ProTxRegisterAction::External || action == ProTxRegisterAction::Fund) &&
        !request.params[paramIdx + 7].isNull()) {
        typed_request.submit = ParseBoolV(request.params[paramIdx + 7], "submit");
    }

    auto wallet_interface{MakeWalletInterface(node, pwallet)};
    if (action == ProTxRegisterAction::Prepare) {
        const auto prepared{UnwrapOrThrow(evo::provider::PrepareRegistration(node, *wallet_interface, typed_request))};
        UniValue response{UniValue::VOBJ};
        response.pushKV("tx", EncodeHexTx(*prepared.tx));
        response.pushKV("collateralAddress", EncodeDestination(prepared.collateral_address));
        response.pushKV("signMessage", prepared.sign_message);
        return response;
    }
    return SubmissionToString(UnwrapOrThrow(evo::provider::Register(node, *wallet_interface, typed_request)));
}

static RPCHelpMan protx_register_submit()
{
    return RPCHelpMan{
        "protx register_submit",
        "\nCombines the unsigned ProTx and a signature of the signMessage, signs all inputs\n"
        "which were added to cover fees and submits the resulting transaction to the network.\n"
        "Note: See \"help protx register_prepare\" for more info about creating a ProTx and a message to sign.\n" +
            HELP_REQUIRING_PASSPHRASE,
        {
            {"tx", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The serialized unsigned ProTx in hex format."},
            {"sig", RPCArg::Type::STR, RPCArg::Optional::NO,
             "The signature signed with the collateral key. Must be in base64 format."},
        },
        RPCResult{RPCResult::Type::STR_HEX, "txid", "The transaction id"},
        RPCExamples{HelpExampleCli("protx", "register_submit \"tx\" \"sig\"")},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            NodeContext& node{EnsureAnyNodeContext(request.context)};
            const std::shared_ptr<CWallet> wallet{GetWalletForJSONRPCRequest(request)};
            if (!wallet) return UniValue::VNULL;
            EnsureWalletIsUnlocked(*wallet);

            CMutableTransaction tx;
            if (!DecodeHexTx(tx, request.params[0].get_str())) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "transaction not deserializable");
            }
            auto opt_vchSig{DecodeBase64(request.params[1].get_str())};
            if (!opt_vchSig.has_value()) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "malformed base64 encoding");
            }

            auto wallet_interface{MakeWalletInterface(node, wallet)};
            return SubmissionToString(UnwrapOrThrow(evo::provider::SubmitRegistration(
                node, *wallet_interface, MakeTransactionRef(std::move(tx)), *opt_vchSig)));
        },
    };
}

static RPCHelpMan protx_update_service()
{
    return RPCHelpMan{"protx update_service",
        "\nCreates and sends a ProUpServTx to the network. This will update the IP address\n"
        "of a masternode.\n"
        "If this is done for a masternode that got PoSe-banned, the ProUpServTx will also revive this masternode.\n"
        + HELP_REQUIRING_PASSPHRASE,
        {
            GetRpcArg("proTxHash"),
            GetRpcArg("coreP2PAddrs_update"),
            GetRpcArg("operatorKey"),
            GetRpcArg("operatorPayoutAddress"),
            GetRpcArg("feeSourceAddress"),
            GetRpcArg("submit"),
        },
        {
            RPCResult{"if \"submit\" is not set or set to true",
                RPCResult::Type::STR_HEX, "txid", "The transaction id"},
            RPCResult{"if \"submit\" is set to false",
                RPCResult::Type::STR_HEX, "hex", "The serialized signed ProTx in hex format"},
        },
        RPCExamples{
            HelpExampleCli("protx", "update_service \"0123456701234567012345670123456701234567012345670123456701234567\" \"1.2.3.4:1234\" 5a2e15982e62f1e0b7cf9783c64cf7e3af3f90a52d6c40f6f95d624c0b1621cd")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    return protx_update_service_common_wrapper(request, MnType::Regular);
},
    };
}

static RPCHelpMan protx_update_service_evo()
{
    const std::string command_name{"protx update_service_evo"};
    return RPCHelpMan{
        command_name,
        "\nCreates and sends a ProUpServTx to the network. This will update the IP address and the Platform fields\n"
        "of an EvoNode.\n"
        "If this is done for an EvoNode that got PoSe-banned, the ProUpServTx will also revive this EvoNode.\n" +
            HELP_REQUIRING_PASSPHRASE,
        {
            GetRpcArg("proTxHash"),
            GetRpcArg("coreP2PAddrs_update"),
            GetRpcArg("operatorKey"),
            GetRpcArg("platformNodeID"),
            GetRpcArg("platformP2PAddrs_update"),
            GetRpcArg("platformHTTPSAddrs_update"),
            GetRpcArg("operatorPayoutAddress"),
            GetRpcArg("feeSourceAddress"),
            GetRpcArg("submit"),
        },
        {
            RPCResult{"if \"submit\" is not set or set to true",
                RPCResult::Type::STR_HEX, "txid", "The transaction id"},
            RPCResult{"if \"submit\" is set to false",
                RPCResult::Type::STR_HEX, "hex", "The serialized signed ProTx in hex format"},
        },
        RPCExamples{
            HelpExampleCli("protx", "update_service_evo \"0123456701234567012345670123456701234567012345670123456701234567\" \"1.2.3.4:1234\" \"5a2e15982e62f1e0b7cf9783c64cf7e3af3f90a52d6c40f6f95d624c0b1621cd\" \"f2dbd9b0a1f541a7c44d34a58674d0262f5feca5\" 22821 22822")},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    return protx_update_service_common_wrapper(request, MnType::Evo);
},
    };
}

static UniValue protx_update_service_common_wrapper(const JSONRPCRequest& request, const MnType mnType)
{
    NodeContext& node{EnsureAnyNodeContext(request.context)};
    std::shared_ptr<CWallet> const wallet{GetWalletForJSONRPCRequest(request)};
    if (!wallet) return UniValue::VNULL;
    EnsureWalletIsUnlocked(*wallet);

    interfaces::ProviderUpdateServiceRequest typed_request;
    typed_request.type = mnType;
    typed_request.pro_tx_hash = ParseHashV(request.params[0], "proTxHash");
    typed_request.net_info.core_p2p = ParseCoreNetInfo(request.params[1], /*optional=*/false);
    typed_request.operator_key = ParseBLSSecretKey(request.params[2].get_str(), "operatorKey");

    size_t paramIdx{3};
    if (mnType == MnType::Evo) {
        const auto capabilities{evo::provider::GetCapabilities(node)};
        typed_request.platform_node_id = ParsePlatformNodeID(request.params[paramIdx]);
        typed_request.net_info.platform_p2p = ParsePlatformNetInfo(request.params[paramIdx + 1], "platformP2PAddrs",
                                                                   capabilities.extended_addresses);
        typed_request.net_info.platform_https = ParsePlatformNetInfo(request.params[paramIdx + 2], "platformHTTPSAddrs",
                                                                     capabilities.extended_addresses);
        paramIdx += 3;
    }

    if (!request.params[paramIdx].isNull() && !request.params[paramIdx].get_str().empty()) {
        CTxDestination payout_destination{DecodeDestination(request.params[paramIdx].get_str())};
        if (!IsValidDestination(payout_destination)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                               strprintf("invalid operator payout address: %s", request.params[paramIdx].get_str()));
        }
        typed_request.operator_payout = payout_destination;
    }
    typed_request.fee_source = ParseFeeSource(request.params[paramIdx + 1]);
    if (!request.params[paramIdx + 2].isNull()) {
        typed_request.submit = ParseBoolV(request.params[paramIdx + 2], "submit");
    }

    auto wallet_interface{MakeWalletInterface(node, wallet)};
    return SubmissionToString(UnwrapOrThrow(evo::provider::UpdateService(node, *wallet_interface, typed_request)));
}

static RPCHelpMan protx_update_registrar_wrapper(const bool specific_legacy_bls_scheme)
{
    std::string rpc_name = specific_legacy_bls_scheme ? "update_registrar_legacy" : "update_registrar";
    std::string rpc_full_name = std::string("protx ").append(rpc_name);
    std::string pubkey_operator = specific_legacy_bls_scheme ? "\"0532646990082f4fd639f90387b1551f2c7c39d37392cb9055a06a7e85c1d23692db8f87f827886310bccc1e29db9aee\"" : "\"8532646990082f4fd639f90387b1551f2c7c39d37392cb9055a06a7e85c1d23692db8f87f827886310bccc1e29db9aee\"";
    std::string rpc_example = rpc_name.append(" \"0123456701234567012345670123456701234567012345670123456701234567\" ").append(pubkey_operator).append(" \"" + EXAMPLE_ADDRESS[1] + "\"");
    return RPCHelpMan{
        rpc_full_name,
        "\nCreates and sends a ProUpRegTx to the network. This will update the operator key, voting key and payout\n"
        "address of the masternode specified by \"proTxHash\".\n"
        "The owner key of the masternode must be known to your wallet.\n" +
            std::string(specific_legacy_bls_scheme ? "\nDEPRECATED: May be removed in a future version, pass config "
                                                     "option -deprecatedrpc=legacy_mn to use RPC\n"
                                                   : "") +
            HELP_REQUIRING_PASSPHRASE,
        {
            GetRpcArg("proTxHash"),
            specific_legacy_bls_scheme ? GetRpcArg("operatorPubKey_update_legacy") : GetRpcArg("operatorPubKey_update"),
            GetRpcArg("votingAddress_update"),
            GetRpcArg("payoutAddress_update"),
            GetRpcArg("feeSourceAddress"),
            GetRpcArg("submit"),
        },
        {
            RPCResult{"if \"submit\" is not set or set to true", RPCResult::Type::STR_HEX, "txid", "The transaction id"},
            RPCResult{"if \"submit\" is set to false", RPCResult::Type::STR_HEX, "hex",
                      "The serialized signed ProTx in hex format"},
        },
        RPCExamples{HelpExampleCli("protx", rpc_example)},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            const bool use_legacy{self.m_name == "protx update_registrar_legacy"};
            if (use_legacy && !IsDeprecatedRPCEnabled("legacy_mn")) {
                throw std::runtime_error("DEPRECATED: Pass config option -deprecatedrpc=legacy_mn to enable this RPC");
            }

            NodeContext& node{EnsureAnyNodeContext(request.context)};
            std::shared_ptr<CWallet> const wallet{GetWalletForJSONRPCRequest(request)};
            if (!wallet) return UniValue::VNULL;
            EnsureWalletIsUnlocked(*wallet);

            interfaces::ProviderUpdateRegistrarRequest typed_request;
            typed_request.pro_tx_hash = ParseHashV(request.params[0], "proTxHash");
            typed_request.use_legacy_bls_scheme = use_legacy;
            if (!request.params[1].get_str().empty()) {
                typed_request.operator_key = ParseBLSPubKey(request.params[1].get_str(), "operator BLS address",
                                                            use_legacy);
            }
            if (!request.params[2].get_str().empty()) {
                typed_request.voting_key = ParsePubKeyIDFromAddress(request.params[2].get_str(), "voting address");
            }
            if (request.params[3].isArray() || !request.params[3].get_str().empty()) {
                typed_request.uses_extended_payouts = request.params[3].isArray();
                if (typed_request.uses_extended_payouts &&
                    !evo::provider::GetCapabilities(node, /*basic_override=*/!use_legacy).extended_addresses) {
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "payouts array requires provider transaction version 3");
                }
                typed_request.payouts = ParsePayouts(request.params[3], "payouts");
            }
            typed_request.fee_source = ParseFeeSource(request.params[4]);
            if (!request.params[5].isNull()) {
                typed_request.submit = ParseBoolV(request.params[5], "submit");
            }

            auto wallet_interface{MakeWalletInterface(node, wallet)};
            return SubmissionToString(
                UnwrapOrThrow(evo::provider::UpdateRegistrar(node, *wallet_interface, typed_request)));
        },
    };
}

static RPCHelpMan protx_update_registrar()
{
    return protx_update_registrar_wrapper(false);
}

static RPCHelpMan protx_update_registrar_legacy()
{
    return protx_update_registrar_wrapper(true);
}

static RPCHelpMan protx_revoke()
{
    return RPCHelpMan{
        "protx revoke",
        "\nCreates and sends a ProUpRevTx to the network. This will revoke the operator key of the masternode and\n"
        "put it into the PoSe-banned state. It will also set the service field of the masternode\n"
        "to zero. Use this in case your operator key got compromised or you want to stop providing your service\n"
        "to the masternode owner.\n" +
            HELP_REQUIRING_PASSPHRASE,
        {
            GetRpcArg("proTxHash"),
            GetRpcArg("operatorKey"),
            GetRpcArg("reason"),
            GetRpcArg("feeSourceAddress"),
            GetRpcArg("submit"),
        },
        {
            RPCResult{"if \"submit\" is not set or set to true", RPCResult::Type::STR_HEX, "txid", "The transaction id"},
            RPCResult{"if \"submit\" is set to false", RPCResult::Type::STR_HEX, "hex",
                      "The serialized signed ProTx in hex format"},
        },
        RPCExamples{HelpExampleCli("protx",
                                   "revoke \"0123456701234567012345670123456701234567012345670123456701234567\" "
                                   "\"072f36a77261cdd5d64c32d97bac417540eddca1d5612f416feb07ff75a8e240\"")},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            NodeContext& node{EnsureAnyNodeContext(request.context)};
            std::shared_ptr<CWallet> const pwallet{GetWalletForJSONRPCRequest(request)};
            if (!pwallet) return UniValue::VNULL;
            EnsureWalletIsUnlocked(*pwallet);

            interfaces::ProviderRevokeRequest typed_request;
            typed_request.pro_tx_hash = ParseHashV(request.params[0], "proTxHash");
            typed_request.operator_key = ParseBLSSecretKey(request.params[1].get_str(), "operatorKey");
            if (!request.params[2].isNull()) {
                const int32_t nReason{request.params[2].getInt<int>()};
                if (nReason < 0 || nReason > CProUpRevTx::REASON_LAST) {
                    throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("invalid reason %d, must be between 0 and %d",
                                                                        nReason, CProUpRevTx::REASON_LAST));
                }
                typed_request.reason = static_cast<uint16_t>(nReason);
            }
            typed_request.fee_source = ParseFeeSource(request.params[3]);
            if (!request.params[4].isNull()) {
                typed_request.submit = ParseBoolV(request.params[4], "submit");
            }

            auto wallet_interface{MakeWalletInterface(node, pwallet)};
            return SubmissionToString(UnwrapOrThrow(evo::provider::Revoke(node, *wallet_interface, typed_request)));
        },
    };
}

static RPCHelpMan protx_shared_sign()
{
    return RPCHelpMan{"protx shared_sign",
        "\nSigns a shared masternode transaction (shared ProRegTx, unanimous ProDisTx or ProUpSharedRegTx)\n"
        "with every share owner key this wallet holds and returns the produced signatures. The transaction\n"
        "itself is not modified; pass the signatures to \"protx shared_combine\".\n"
        "Only the multi-party flows pass through here: a unilateral dissolution is built, signed and\n"
        "submitted in one step by \"protx shared_dissolve\" and never needs shared_sign.\n"
        "A dissolution is signed only when it returns every share's principal to its refund address, less\n"
        "the fee from the actor's share, as \"protx shared_dissolve_prepare\" builds it.\n"
        + HELP_REQUIRING_PASSPHRASE,
        {
            {"tx", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The serialized transaction in hex format."},
            {"allowTimeLocks", RPCArg::Type::BOOL, RPCArg::Default{false}, "Sign a registration or dissolution carrying an unsatisfied lock time or a relative (BIP68) input lock. The signed digest commits to these fields, so a lock a co-signer failed to notice delays when the transaction can confirm.",
             RPCArgOptions{.skip_type_check = true}},
        },
        RPCResult{RPCResult::Type::OBJ, "", "",
        {
            {RPCResult::Type::STR, "type", "What is being signed: \"registration\", \"dissolution\" or \"registrarUpdate\""},
            {RPCResult::Type::OBJ, "terms", "The decoded payload being consented to (see the matching special transaction JSON)", {}, /*skip_type_check=*/true},
            {RPCResult::Type::STR, "warning", /*optional=*/true, "Present for a registration whose earlyPenalty is zero: any participant can force an early exit at no cost beyond the transaction fee"},
            {RPCResult::Type::ARR, "signatures", "One entry per share owner key this wallet holds",
            {
                {RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::NUM, "shareIndex", "Index into the share table"},
                    {RPCResult::Type::STR, "signature", "Base64-encoded signature by this share's owner key"},
                }},
            }},
        }},
        RPCExamples{HelpExampleCli("protx", "shared_sign \"tx\"")},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    NodeContext& node = EnsureAnyNodeContext(request.context);

    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;
    EnsureWalletIsUnlocked(*pwallet);

    CMutableTransaction tx;
    if (!DecodeHexTx(tx, request.params[0].get_str())) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "transaction not deserializable");
    }
    interfaces::SharedSignRequest typed_request;
    typed_request.tx = MakeTransactionRef(std::move(tx));
    typed_request.allow_time_locks = request.params[1].isNull() ? false
                                                                : ParseBoolV(request.params[1], "allowTimeLocks");

    auto wallet_interface{MakeWalletInterface(node, pwallet)};
    const auto signed_result{UnwrapOrThrow(evo::provider::SignShared(node, *wallet_interface, typed_request))};

    // A readable summary of the terms, so a co-signer can review what the signature commits to
    const CTransaction& signed_tx{*typed_request.tx};
    UniValue ret(UniValue::VOBJ);
    switch (signed_result.kind) {
    case interfaces::SharedSignResult::Kind::Registration:
        ret.pushKV("type", "registration");
        ret.pushKV("terms", CHECK_NONFATAL(GetTxPayload<CProRegTx>(signed_tx))->ToJson());
        break;
    case interfaces::SharedSignResult::Kind::Dissolution: {
        ret.pushKV("type", "dissolution");
        // The digest covers the outputs directly, so show them with the payload
        UniValue terms{CHECK_NONFATAL(GetTxPayload<CProDisTx>(signed_tx))->ToJson()};
        UniValue outputs(UniValue::VARR);
        for (const auto& out : signed_tx.vout) {
            UniValue o(UniValue::VOBJ);
            CTxDestination dest;
            o.pushKV("address", ExtractDestination(out.scriptPubKey, dest) ? EncodeDestination(dest) : HexStr(out.scriptPubKey));
            o.pushKV("amount", ValueFromAmount(out.nValue));
            outputs.push_back(o);
        }
        terms.pushKV("outputs", outputs);
        ret.pushKV("terms", terms);
        break;
    }
    case interfaces::SharedSignResult::Kind::RegistrarUpdate:
        ret.pushKV("type", "registrarUpdate");
        ret.pushKV("terms", CHECK_NONFATAL(GetTxPayload<CProUpSharedRegTx>(signed_tx))->ToJson());
        break;
    }
    if (signed_result.warning) {
        ret.pushKV("warning", *signed_result.warning);
    }

    UniValue signatures(UniValue::VARR);
    for (const auto& signature : signed_result.signatures) {
        UniValue entry(UniValue::VOBJ);
        entry.pushKV("shareIndex", static_cast<uint64_t>(signature.share_index));
        entry.pushKV("signature", EncodeBase64(signature.signature));
        signatures.push_back(entry);
    }
    ret.pushKV("signatures", signatures);
    return ret;
},
    };
}

static RPCHelpMan protx_shared_dissolve()
{
    return RPCHelpMan{"protx shared_dissolve",
        "\nCreates, signs and optionally submits a unilateral ProDisTx that dissolves a shared masternode,\n"
        "refunding every participant's principal to its immutable refund script. During the early period a\n"
        "unilateral dissolution pays the configured penalty, redistributed pro-rata to the other shares.\n"
        "The wallet must hold the actor share's owner key. With submit=false the signed transaction hex is\n"
        "returned instead, which can be stored offline as a standby dissolution: ProDisTx validity is\n"
        "monotone, so a transaction that is valid now stays valid forever.\n"
        + HELP_REQUIRING_PASSPHRASE,
        {
            {"proTxHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The hash of the initial ProRegTx."},
            {"actorIndex", RPCArg::Type::NUM, RPCArg::Optional::NO, "Index into the share table of the dissolving participant."},
            {"fee", RPCArg::Type::NUM, RPCArg::Default{100000}, "Transaction fee in duffs, paid from the actor's share. At most 1000000 duffs (consensus ceiling)."},
            {"submit", RPCArg::Type::BOOL, RPCArg::Default{true}, "Submit the transaction to the network.",
             RPCArgOptions{.skip_type_check = true}},
            {"payPenalty", RPCArg::Type::BOOL, RPCArg::DefaultHint{"determined by the current height"}, "Pay the early-period penalty. Pass true to build a standby valid at any height, false for one valid only after the early period ends.",
             RPCArgOptions{.skip_type_check = true}},
        },
        RPCResult{RPCResult::Type::STR_HEX, "result", "The transaction id if submitted, otherwise the signed transaction hex"},
        RPCExamples{HelpExampleCli("protx", "shared_dissolve \"proTxHash\" 0")},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    NodeContext& node = EnsureAnyNodeContext(request.context);

    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;
    EnsureWalletIsUnlocked(*pwallet);

    interfaces::SharedDissolveRequest typed_request;
    typed_request.pro_tx_hash = ParseHashV(request.params[0], "proTxHash");
    typed_request.actor_index = ParseShareIndex(request.params[1], "actorIndex");
    typed_request.fee = request.params[2].isNull() ? 100000 : request.params[2].getInt<int64_t>();
    typed_request.submit = request.params[3].isNull() ? true : ParseBoolV(request.params[3], "submit");
    if (!request.params[4].isNull()) {
        typed_request.pay_penalty = ParseBoolV(request.params[4], "payPenalty");
    }

    auto wallet_interface{MakeWalletInterface(node, pwallet)};
    return SubmissionToString(UnwrapOrThrow(evo::provider::DissolveShared(node, *wallet_interface, typed_request)));
},
    };
}

static RPCHelpMan protx_shared_update_share()
{
    return RPCHelpMan{"protx shared_update_share",
        "\nCreates and sends a ProUpShareTx updating one collateral share's reward script. Only the share's\n"
        "owner key (which this wallet must hold) can update it; all other share fields are immutable.\n"
        + HELP_REQUIRING_PASSPHRASE,
        {
            {"proTxHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The hash of the initial ProRegTx."},
            {"shareIndex", RPCArg::Type::NUM, RPCArg::Optional::NO, "Index into the share table of the share to update."},
            {"rewardAddress", RPCArg::Type::STR, RPCArg::Optional::NO, "The new reward address. To reset rewards, provide the share's refund address."},
            {"feeSourceAddress", RPCArg::Type::STR, RPCArg::Optional::NO, "Wallet address to pay the transaction fee from."},
            {"submit", RPCArg::Type::BOOL, RPCArg::Default{true}, "Submit the transaction to the network.",
             RPCArgOptions{.skip_type_check = true}},
        },
        {
            RPCResult{"if \"submit\" is not set or set to true",
                RPCResult::Type::STR_HEX, "txid", "The transaction id"},
            RPCResult{"if \"submit\" is set to false",
                RPCResult::Type::STR_HEX, "hex", "The serialized signed ProUpShareTx in hex format"},
        },
        RPCExamples{HelpExampleCli("protx", "shared_update_share \"proTxHash\" 0 \"" + EXAMPLE_ADDRESS[1] + "\" \"" + EXAMPLE_ADDRESS[0] + "\"")},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    NodeContext& node = EnsureAnyNodeContext(request.context);

    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;
    EnsureWalletIsUnlocked(*pwallet);

    interfaces::SharedUpdateShareRequest typed_request;
    typed_request.pro_tx_hash = ParseHashV(request.params[0], "proTxHash");
    typed_request.share_index = ParseShareIndex(request.params[1], "shareIndex");
    typed_request.reward = DecodeDestination(request.params[2].get_str());
    if (!IsValidDestination(typed_request.reward)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("invalid reward address: %s", request.params[2].get_str()));
    }
    typed_request.fee_source = ParseRequiredFeeSource(request.params[3]);
    if (!request.params[4].isNull()) {
        typed_request.submit = ParseBoolV(request.params[4], "submit");
    }

    auto wallet_interface{MakeWalletInterface(node, pwallet)};
    return SubmissionToString(UnwrapOrThrow(evo::provider::UpdateShare(node, *wallet_interface, typed_request)));
},
    };
}

static RPCHelpMan protx_shared_update_registrar_prepare()
{
    return RPCHelpMan{"protx shared_update_registrar_prepare",
        "\nCreates an unsigned ProUpSharedRegTx updating a shared masternode's operator key and/or voting\n"
        "key. Fee inputs from this wallet are added and signed. Every share owner must then sign the\n"
        "returned transaction with \"protx shared_sign\"; combine the signatures with \"protx shared_combine\"\n"
        "on this same wallet (combining invalidates the fee-input signatures, which only this wallet can\n"
        "re-sign).\n"
        + HELP_REQUIRING_PASSPHRASE,
        {
            {"proTxHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The hash of the initial ProRegTx."},
            {"operatorPubKey", RPCArg::Type::STR, RPCArg::Optional::NO, "The new operator BLS public key, or an empty string to keep the current key."},
            {"votingAddress", RPCArg::Type::STR, RPCArg::Optional::NO, "The new voting address, or an empty string to keep the current address."},
            {"feeSourceAddress", RPCArg::Type::STR, RPCArg::Optional::NO, "Wallet address to pay the transaction fee from."},
        },
        RPCResult{RPCResult::Type::OBJ, "", "",
        {
            {RPCResult::Type::STR_HEX, "tx", "The serialized unsigned ProUpSharedRegTx"},
            {RPCResult::Type::STR_HEX, "signHash", "The payload hash every share owner must sign"},
        }},
        RPCExamples{HelpExampleCli("protx", "shared_update_registrar_prepare \"proTxHash\" \"operatorPubKey\" \"\" \"" + EXAMPLE_ADDRESS[0] + "\"")},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    NodeContext& node = EnsureAnyNodeContext(request.context);

    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;
    EnsureWalletIsUnlocked(*pwallet);

    interfaces::SharedRegistrarUpdatePrepareRequest typed_request;
    typed_request.pro_tx_hash = ParseHashV(request.params[0], "proTxHash");
    if (!request.params[1].get_str().empty()) {
        typed_request.operator_key = ParseBLSPubKey(request.params[1].get_str(), "operator BLS address",
                                                    /*specific_legacy_bls_scheme=*/false);
    }
    if (!request.params[2].get_str().empty()) {
        typed_request.voting_key = ParsePubKeyIDFromAddress(request.params[2].get_str(), "voting address");
    }
    typed_request.fee_source = ParseRequiredFeeSource(request.params[3]);

    auto wallet_interface{MakeWalletInterface(node, pwallet)};
    const auto prepared{
        UnwrapOrThrow(evo::provider::PrepareSharedRegistrarUpdate(node, *wallet_interface, typed_request))};
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("tx", EncodeHexTx(*prepared.tx));
    ret.pushKV("signHash", prepared.sign_hash.ToString());
    return ret;
},
    };
}

static RPCHelpMan protx_shared_combine()
{
    return RPCHelpMan{"protx shared_combine",
        "\nCombines share owner signatures produced by \"protx shared_sign\" into a shared masternode\n"
        "transaction. For a shared ProRegTx the completed transaction hex is returned and the funding\n"
        "inputs still have to be signed (e.g. by passing the result around signrawtransactionwithwallet).\n"
        "For a ProDisTx or ProUpSharedRegTx the transaction can be submitted directly. Submit a\n"
        "ProUpSharedRegTx from the wallet that ran \"protx shared_update_registrar_prepare\": combining\n"
        "re-signs its fee inputs, and is refused unless they are unlocked coins of a single address of\n"
        "this wallet, any change returns to that address, and the fee is within -maxtxfee.\n",
        {
            {"tx", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The serialized transaction in hex format."},
            {"signatures", RPCArg::Type::ARR, RPCArg::Optional::NO, "Signature entries collected from \"protx shared_sign\".",
            {
                {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "",
                {
                    {"shareIndex", RPCArg::Type::NUM, RPCArg::Optional::NO, "Index into the share table"},
                    {"signature", RPCArg::Type::STR, RPCArg::Optional::NO, "Base64-encoded signature"},
                }},
            }},
            {"submit", RPCArg::Type::BOOL, RPCArg::Default{false}, "Submit the transaction to the network (not available for registrations, whose funding inputs still need signing).",
             RPCArgOptions{.skip_type_check = true}},
        },
        RPCResult{RPCResult::Type::STR_HEX, "result", "The transaction id if submitted, otherwise the combined transaction hex"},
        RPCExamples{HelpExampleCli("protx", "shared_combine \"tx\" \"[{\\\"shareIndex\\\":0,\\\"signature\\\":\\\"...\\\"}]\"")},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    NodeContext& node = EnsureAnyNodeContext(request.context);

    CMutableTransaction tx;
    if (!DecodeHexTx(tx, request.params[0].get_str())) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "transaction not deserializable");
    }

    // Collect (shareIndex, signature) pairs
    interfaces::SharedCombineRequest typed_request;
    std::set<size_t> share_indexes;
    for (const auto& entry : request.params[1].get_array().getValues()) {
        const int64_t index{entry.find_value("shareIndex").getInt<int64_t>()};
        auto opt_sig = DecodeBase64(entry.find_value("signature").get_str());
        if (index < 0 || !opt_sig.has_value() || opt_sig->size() != CPubKey::COMPACT_SIGNATURE_SIZE) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid signature entry");
        }
        if (!share_indexes.insert(static_cast<size_t>(index)).second) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "duplicate shareIndex");
        }
        typed_request.signatures.push_back({static_cast<size_t>(index), std::move(*opt_sig)});
    }

    typed_request.submit = request.params[2].isNull() ? false : ParseBoolV(request.params[2], "submit");

    std::unique_ptr<interfaces::Wallet> wallet_interface;
    if (tx.nType == TRANSACTION_PROVIDER_UPDATE_SHARED_REGISTRAR) {
        std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
        if (!pwallet) return UniValue::VNULL;
        EnsureWalletIsUnlocked(*pwallet);
        wallet_interface = MakeWalletInterface(node, pwallet);
    }
    typed_request.tx = MakeTransactionRef(std::move(tx));
    return SubmissionToString(
        UnwrapOrThrow(evo::provider::CombineShared(node, wallet_interface.get(), typed_request)));
},
    };
}

#endif//ENABLE_WALLET

#ifdef ENABLE_WALLET
static bool CheckWalletOwnsScript(const CWallet* const pwallet, const CScript& script) {
    if (!pwallet) {
        return false;
    }
    return WITH_LOCK(pwallet->cs_wallet, return pwallet->IsMine(script)) == isminetype::ISMINE_SPENDABLE;
}

static bool CheckWalletOwnsAnyPayout(const CWallet* const pwallet, const CDeterministicMNState& state)
{
    for (const auto& script : state.GetOwnerRewardScripts()) {
        if (CheckWalletOwnsScript(pwallet, script)) return true;
    }
    return false;
}

static bool CheckWalletOwnsKey(const CWallet* const pwallet, const CKeyID& keyID) {
    return CheckWalletOwnsScript(pwallet, GetScriptForDestination(PKHash(keyID)));
}

static bool CheckWalletOwnsAnyOwnerKey(const CWallet* const pwallet, const CDeterministicMNState& state)
{
    // A shared masternode has a null keyIDOwner; the share owner keys take its place
    if (state.IsShared()) {
        for (const auto& share : state.shares) {
            if (CheckWalletOwnsKey(pwallet, share.keyIDOwner)) return true;
        }
        return false;
    }
    return CheckWalletOwnsKey(pwallet, state.keyIDOwner);
}

static bool CheckWalletOwnsAnyShareRefund(const CWallet* const pwallet, const CDeterministicMNState& state)
{
    // A dissolution returns each share's collateral to its refund script. The non-shared
    // equivalent (the collateral UTXO) is matched via ListProTxCoins, which cannot see the
    // shared covenant output, so match the refund destinations directly.
    for (const auto& share : state.shares) {
        if (CheckWalletOwnsScript(pwallet, share.scriptRefund)) return true;
    }
    return false;
}
#endif

static UniValue BuildDMNListEntry(const CWallet* const pwallet, const CDeterministicMN& dmn, const CMasternodeMetaMan& mn_metaman, bool detailed, const ChainstateManager& chainman, const CBlockIndex* pindex = nullptr)
{
    if (!detailed) {
        return dmn.proTxHash.ToString();
    }

    UniValue o = dmn.ToJson();

    CTransactionRef collateralTx{nullptr};
    int confirmations = GetUTXOConfirmations(chainman.ActiveChainstate(), dmn.collateralOutpoint);

    if (pindex != nullptr) {
        if (confirmations > -1) {
            confirmations -= WITH_LOCK(::cs_main, return chainman.ActiveChain().Height()) - pindex->nHeight;
        } else {
            uint256 minedBlockHash;
            collateralTx = GetTransaction(/* pindex */ nullptr, /* mempool */ nullptr, dmn.collateralOutpoint.hash, Params().GetConsensus(), minedBlockHash);
            const CBlockIndex* const pindexMined = WITH_LOCK(::cs_main, return chainman.m_blockman.LookupBlockIndex(minedBlockHash));
            CHECK_NONFATAL(pindexMined != nullptr);
            CHECK_NONFATAL(pindex->GetAncestor(pindexMined->nHeight) == pindexMined);
            confirmations = pindex->nHeight - pindexMined->nHeight + 1;
        }
    }
    o.pushKV("confirmations", confirmations);

#ifdef ENABLE_WALLET
    bool hasOwnerKey = CheckWalletOwnsAnyOwnerKey(pwallet, *dmn.pdmnState);
    bool hasVotingKey = CheckWalletOwnsKey(pwallet, dmn.pdmnState->keyIDVoting);

    bool ownsCollateral = false;
    if (Coin coin; GetUTXOCoin(chainman.ActiveChainstate(), dmn.collateralOutpoint, coin)) {
        ownsCollateral = CheckWalletOwnsScript(pwallet, coin.out.scriptPubKey);
    } else if (collateralTx != nullptr) {
        ownsCollateral = CheckWalletOwnsScript(pwallet, collateralTx->vout[dmn.collateralOutpoint.n].scriptPubKey);
    }

    if (pwallet) {
        UniValue walletObj(UniValue::VOBJ);
        walletObj.pushKV("hasOwnerKey", hasOwnerKey);
        walletObj.pushKV("hasOperatorKey", false);
        walletObj.pushKV("hasVotingKey", hasVotingKey);
        walletObj.pushKV("ownsCollateral", ownsCollateral);
        walletObj.pushKV("ownsPayeeScript", CheckWalletOwnsAnyPayout(pwallet, *dmn.pdmnState));
        walletObj.pushKV("ownsOperatorRewardScript", CheckWalletOwnsScript(pwallet, dmn.pdmnState->scriptOperatorPayout));
        o.pushKV("wallet", walletObj);
    }
#endif

    o.pushKV("metaInfo", mn_metaman.GetInfo(dmn.proTxHash).ToJson());

    return o;
}

static RPCHelpMan protx_list()
{
    return RPCHelpMan{"protx list",
        "\nLists all ProTxs in your wallet or on-chain, depending on the given type.\n",
        {
            {"type", RPCArg::Type::STR, RPCArg::Default{"registered"},
                "\nAvailable types:\n"
                "  registered   - List all ProTx which are registered at the given chain height.\n"
                "                 This will also include ProTx which failed PoSe verification.\n"
                "  valid        - List only ProTx which are active/valid at the given chain height.\n"
                "  evo          - List only ProTx corresponding to EvoNodes at the given chain height.\n"
#ifdef ENABLE_WALLET
                "  wallet       - List only ProTx which are found in your wallet at the given chain height.\n"
                "                 This will also include ProTx which failed PoSe verification.\n"
#endif
            },
            {"detailed", RPCArg::Type::BOOL, RPCArg::Default{false}, "If not specified, only the hashes of the ProTx will be returned.",
             RPCArgOptions{.skip_type_check = true}},
            {"height", RPCArg::Type::NUM, RPCArg::DefaultHint{"current chain-tip"}, ""},
        },
        RPCResult{
            RPCResult::Type::ARR, "", "List of masternodes",
            {
                // Array elements are matched against this doc by index, so the two shapes
                // cannot be listed as alternatives here - the first entry would be applied
                // to element 0 only and the second to every element after it.
                // TODO: document fields of the detailed entry
                {RPCResult::Type::ANY, "", "The ProTx hash when detailed=false, otherwise an object describing the masternode"},
            }},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureChainman(node);

    CDeterministicMNManager& dmnman = *CHECK_NONFATAL(node.dmnman);
    const CMasternodeMetaMan& mn_metaman = *CHECK_NONFATAL(node.mn_metaman);

    std::shared_ptr<CWallet> wallet{nullptr};
#ifdef ENABLE_WALLET
    try {
        wallet = GetWalletForJSONRPCRequest(request);
    } catch (...) {
    }
#endif

    std::string type = "registered";
    if (!request.params[0].isNull()) {
        type = request.params[0].get_str();
    }

    UniValue ret(UniValue::VARR);

    if (g_txindex) {
        g_txindex->BlockUntilSyncedToCurrentChain();
    }

    if (type == "wallet") {
        if (!wallet) {
            throw std::runtime_error("\"protx list wallet\" not supported when wallet is disabled");
        }
#ifdef ENABLE_WALLET

        if (request.params.size() > 4) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Too many arguments");
        }

        bool detailed = !request.params[1].isNull() ? ParseBoolV(request.params[1], "detailed") : false;

        LOCK2(wallet->cs_wallet, ::cs_main);
        int height = !request.params[2].isNull() ? request.params[2].getInt<int>() : chainman.ActiveChain().Height();
        if (height < 1 || height > chainman.ActiveChain().Height()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid height specified");
        }

        std::set<COutPoint> setOutpts;
        for (const auto& outpt : wallet->ListProTxCoins()) {
            setOutpts.emplace(outpt);
        }

        CDeterministicMNList mnList = dmnman.GetListForBlock(chainman.ActiveChain()[height]);
        mnList.ForEachMN(/*onlyValid=*/false, [&](const auto& dmn) {
            if (setOutpts.count(dmn.collateralOutpoint) ||
                CheckWalletOwnsAnyOwnerKey(wallet.get(), *dmn.pdmnState) ||
                CheckWalletOwnsKey(wallet.get(), dmn.pdmnState->keyIDVoting) ||
                CheckWalletOwnsAnyPayout(wallet.get(), *dmn.pdmnState) ||
                CheckWalletOwnsAnyShareRefund(wallet.get(), *dmn.pdmnState) ||
                CheckWalletOwnsScript(wallet.get(), dmn.pdmnState->scriptOperatorPayout)) {
                ret.push_back(BuildDMNListEntry(wallet.get(), dmn, mn_metaman, detailed, chainman));
            }
        });
#endif
    } else if (type == "valid" || type == "registered" || type == "evo") {
        if (request.params.size() > 3) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Too many arguments");
        }

        bool detailed = !request.params[1].isNull() ? ParseBoolV(request.params[1], "detailed") : false;

#ifdef ENABLE_WALLET
        LOCK2(wallet ? wallet->cs_wallet : ::cs_main, ::cs_main);
#else
        LOCK(::cs_main);
#endif
        int height = !request.params[2].isNull() ? request.params[2].getInt<int>() : chainman.ActiveChain().Height();
        if (height < 1 || height > chainman.ActiveChain().Height()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid height specified");
        }

        CDeterministicMNList mnList = dmnman.GetListForBlock(chainman.ActiveChain()[height]);
        bool onlyValid = type == "valid";
        bool onlyEvoNodes = type == "evo";
        mnList.ForEachMN(onlyValid, [&](const auto& dmn) {
            if (onlyEvoNodes && dmn.nType != MnType::Evo) return;
            ret.push_back(BuildDMNListEntry(wallet.get(), dmn, mn_metaman, detailed, chainman));
        });
    } else {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid type specified");
    }

    return ret;
},
    };
}

static RPCHelpMan protx_info()
{
    return RPCHelpMan{"protx info",
        "\nReturns detailed information about a deterministic masternode.\n",
        {
            GetRpcArg("proTxHash"),
            {"blockHash", RPCArg::Type::STR_HEX, RPCArg::DefaultHint{"(chain tip)"}, "The hash of the block to get deterministic masternode state at"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "Details about a specific deterministic masternode",
            {
                // TODO: implement proper doc for protx info
                {RPCResult::Type::ELISION, "", ""}
            }
        },
        RPCExamples{
            HelpExampleCli("protx", "info \"0123456701234567012345670123456701234567012345670123456701234567\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureChainman(node);

    CDeterministicMNManager& dmnman = *CHECK_NONFATAL(node.dmnman);
    const CMasternodeMetaMan& mn_metaman = *CHECK_NONFATAL(node.mn_metaman);

    std::shared_ptr<CWallet> wallet{nullptr};
#ifdef ENABLE_WALLET
    try {
        wallet = GetWalletForJSONRPCRequest(request);
    } catch (...) {
    }
#endif

    if (g_txindex) {
        g_txindex->BlockUntilSyncedToCurrentChain();
    }

    const CBlockIndex* pindex{nullptr};

    uint256 proTxHash(ParseHashV(request.params[0], "proTxHash"));

    if (request.params[1].isNull()) {
        LOCK(::cs_main);
        pindex = chainman.ActiveChain().Tip();
    } else {
        LOCK(::cs_main);
        uint256 blockHash(ParseHashV(request.params[1], "blockHash"));
        pindex = chainman.m_blockman.LookupBlockIndex(blockHash);
        if (pindex == nullptr) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
        }
        // A block that was never connected has no list to read yet
        if (!chainman.ActiveChain().Contains(pindex)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Block is not in main chain");
        }
    }

    auto mnList = dmnman.GetListForBlock(pindex);
    auto dmn = mnList.GetMN(proTxHash);
    if (!dmn) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s not found", proTxHash.ToString()));
    }
    return BuildDMNListEntry(wallet.get(), *dmn, mn_metaman, true, chainman, pindex);
},
    };
}

static uint256 ParseBlock(const UniValue& v, const ChainstateManager& chainman, const std::string& strName) EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
{
    AssertLockHeld(::cs_main);

    try {
        return ParseHashV(v, strName);
    } catch (...) {
        bool fail{false}; int32_t h{0};
        if (v.isNum()) {
            h = v.getInt<int>();
        } else if (!ParseInt32(v.get_str(), &h)) {
            fail = true;
        }
        if (fail || h < 1 || h > chainman.ActiveChain().Height()) {
            throw std::runtime_error(strprintf("%s must be a block hash or chain height and not %s", strName, v.getValStr()));
        }
        return *chainman.ActiveChain()[h]->phashBlock;
    }
}

static const CBlockIndex* ParseBlockIndex(const UniValue& v, const ChainstateManager& chainman, const std::string& strName) EXCLUSIVE_LOCKS_REQUIRED(::cs_main)
{
    AssertLockHeld(::cs_main);

    try {
        const auto hash{ParseBlock(v, chainman, strName)};
        const CBlockIndex* pindex = chainman.m_blockman.LookupBlockIndex(hash);
        if (!pindex) {
            throw std::runtime_error(strprintf("Block %s with hash %s not found", strName, v.getValStr()));
        }
        return pindex;
    } catch (...) {
        // Same phrasing as ParseBlock() as it can parse heights
        throw std::runtime_error(strprintf("%s must be a block hash or chain height and not %s", strName, v.getValStr()));
    }
}

static RPCHelpMan protx_diff()
{
    return RPCHelpMan{"protx diff",
        "\nCalculates a diff between two deterministic masternode lists. The result also contains proof data.\n",
        {
            {"baseBlock", RPCArg::Type::STR, RPCArg::Optional::NO, "The starting block hash or height.",
             RPCArgOptions{.skip_type_check = true}},
            {"block", RPCArg::Type::STR, RPCArg::Optional::NO, "The ending block hash or height.",
             RPCArgOptions{.skip_type_check = true}},
            {"extended", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED, "Show additional fields.",
             RPCArgOptions{.skip_type_check = true}},
        },
        CSimplifiedMNListDiff::GetJsonHelp(/*key=*/"", /*optional=*/false),
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureChainman(node);

    CDeterministicMNManager& dmnman = *CHECK_NONFATAL(node.dmnman);
    const LLMQContext& llmq_ctx = *CHECK_NONFATAL(node.llmq_ctx);

    LOCK(::cs_main);
    uint256 baseBlockHash = ParseBlock(request.params[0], chainman, "baseBlock");
    uint256 blockHash = ParseBlock(request.params[1], chainman, "block");
    bool extended = false;
    if (!request.params[2].isNull()) {
        extended = ParseBoolV(request.params[2], "extended");
    }

    CSimplifiedMNListDiff mnListDiff;
    std::string strError;

    if (!BuildSimplifiedMNListDiff(dmnman, chainman, *llmq_ctx.quorum_block_processor, *llmq_ctx.qman, baseBlockHash,
                                   blockHash, mnListDiff, strError, extended))
    {
        throw std::runtime_error(strError);
    }

    return mnListDiff.ToJson(extended);
},
    };
}

static RPCHelpMan protx_listdiff()
{
    return RPCHelpMan{"protx listdiff",
               "\nCalculate a full MN list diff between two masternode lists.\n",
               {
                       {"baseBlock", RPCArg::Type::STR, RPCArg::Optional::NO, "The starting block hash or height.",
                        RPCArgOptions{.skip_type_check = true}},
                       {"block", RPCArg::Type::STR, RPCArg::Optional::NO, "The ending block hash or height.",
                        RPCArgOptions{.skip_type_check = true}},
               },
                RPCResult {
                    RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::NUM, "baseHeight", "Height of base (starting) block"},
                        {RPCResult::Type::NUM, "blockHeight", "Height of target (ending) block"},
                        {RPCResult::Type::ARR, "addedMNs", "Added masternodes",
                            {CDeterministicMN::GetJsonHelp(/*key=*/"", /*optional=*/false)}},
                        {RPCResult::Type::ARR, "removedMNs", "Removed masternodes",
                            {{RPCResult::Type::STR_HEX, "protx", "ProTx of removed masternode"}}},
                        {RPCResult::Type::ARR, "updatedMNs", "Updated masternodes",
                            {{RPCResult::Type::OBJ_DYN, "", "json object with ProTx hash as keys",
                                {CDeterministicMNStateDiff::GetJsonHelp(/*key=*/"<protx_hash>", /*optional=*/false)}}}},
                    },
                },
                RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureChainman(node);

    CDeterministicMNManager& dmnman = *CHECK_NONFATAL(node.dmnman);

    LOCK(::cs_main);
    UniValue ret(UniValue::VOBJ);

    const CBlockIndex* pBaseBlockIndex = ParseBlockIndex(request.params[0], chainman, "baseBlock");
    const CBlockIndex* pTargetBlockIndex = ParseBlockIndex(request.params[1], chainman, "block");
    if (!chainman.ActiveChain().Contains(pBaseBlockIndex) || !chainman.ActiveChain().Contains(pTargetBlockIndex)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Block is not in main chain");
    }

    ret.pushKV("baseHeight", pBaseBlockIndex->nHeight);
    ret.pushKV("blockHeight", pTargetBlockIndex->nHeight);

    auto baseBlockMNList = dmnman.GetListForBlock(pBaseBlockIndex);
    auto blockMNList = dmnman.GetListForBlock(pTargetBlockIndex);

    auto mnDiff = baseBlockMNList.BuildDiff(blockMNList);

    UniValue jaddedMNs(UniValue::VARR);
    for(const auto& mn : mnDiff.addedMNs) {
        jaddedMNs.push_back(mn->ToJson());
    }
    ret.pushKV("addedMNs", jaddedMNs);

    UniValue jremovedMNs(UniValue::VARR);
    for(const auto& internal_id : mnDiff.removedMns) {
        auto dmn = baseBlockMNList.GetMNByInternalId(internal_id);
        // BuildDiff will construct itself with MNs that we already have knowledge
        // of, meaning that fetch operations should never fail.
        CHECK_NONFATAL(dmn);
        jremovedMNs.push_back(dmn->proTxHash.ToString());
    }
    ret.pushKV("removedMNs", jremovedMNs);

    UniValue jupdatedMNs(UniValue::VARR);
    for(const auto& [internal_id, stateDiff] : mnDiff.updatedMNs) {
        auto dmn = blockMNList.GetMNByInternalId(internal_id);
        // BuildDiff will construct itself with MNs that we already have knowledge
        // of, meaning that fetch operations should never fail.
        CHECK_NONFATAL(dmn);
        UniValue diff_json{stateDiff.ToJson(dmn->nType)};
        if ((stateDiff.fields & CDeterministicMNStateDiff::Field_netInfo) && !dmn->pdmnState->netInfo->IsEmpty()) {
            // A legacy EvoNode's Platform addresses pair its core P2P address with the Platform ports,
            // which the diff doesn't carry unless they changed too
            diff_json.pushKV("addresses", GetNetInfoWithLegacyFields(*dmn->pdmnState, dmn->nType));
        }
        UniValue obj(UniValue::VOBJ);
        obj.pushKV(dmn->proTxHash.ToString(), diff_json);
        jupdatedMNs.push_back(obj);
    }
    ret.pushKV("updatedMNs", jupdatedMNs);

    return ret;
},
    };
}

// Helper function for evodb verify/repair commands
static UniValue evodb_verify_or_repair_impl(const JSONRPCRequest& request, bool repair)
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    ChainstateManager& chainman = EnsureChainman(node);
    CDeterministicMNManager& dmnman = *CHECK_NONFATAL(node.dmnman);
    CChainstateHelper& chain_helper = *CHECK_NONFATAL(node.chain_helper);

    const CBlockIndex* start_index;
    const CBlockIndex* stop_index;

    {
        LOCK(::cs_main);
        // Default to DIP0003 activation height if startBlock not specified
        if (request.params[0].isNull()) {
            const auto& consensus_params = Params().GetConsensus();
            start_index = chainman.ActiveChain()[consensus_params.DIP0003Height];
            if (!start_index) {
                throw JSONRPCError(RPC_INTERNAL_ERROR, "Cannot find DIP0003 activation block");
            }
        } else {
            uint256 start_block_hash = ParseBlock(request.params[0], chainman, "startBlock");
            start_index = chainman.m_blockman.LookupBlockIndex(start_block_hash);
            if (!start_index) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Start block not found");
            }
        }

        // Default to chain tip if stopBlock not specified
        if (request.params[1].isNull()) {
            stop_index = chainman.ActiveChain().Tip();
            if (!stop_index) {
                throw JSONRPCError(RPC_INTERNAL_ERROR, "Cannot find chain tip");
            }
        } else {
            uint256 stop_block_hash = ParseBlock(request.params[1], chainman, "stopBlock");
            stop_index = chainman.m_blockman.LookupBlockIndex(stop_block_hash);
            if (!stop_index) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Stop block not found");
            }
        }
    }

    int start_height = start_index->nHeight;
    int stop_height = stop_index->nHeight;

    // Validation
    if (stop_height < start_height) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "stopBlock must be >= startBlock");
    }

    // Create a callback that wraps CSpecialTxProcessor::RebuildListFromBlock
    auto build_list_func = [&chain_helper, &chainman](const CBlock& block, const CBlockIndex* const pindexPrev,
                                                      const CDeterministicMNList& prevList, const CCoinsViewCache& view,
                                                      bool debugLogs, BlockValidationState& state,
                                                      CDeterministicMNList& mnListRet) -> bool {
        const bool is_v24_active{DeploymentActiveAfter(pindexPrev, chainman, Consensus::DEPLOYMENT_V24)};
        return chain_helper.special_tx->RebuildListFromBlock(block, pindexPrev, is_v24_active, prevList, view,
                                                             debugLogs, state, mnListRet);
    };

    // Call the dmnman method to do the work
    auto recalc_result = dmnman.RecalculateAndRepairDiffs(start_index, stop_index, build_list_func, repair);

    // Convert result to UniValue
    UniValue result(UniValue::VOBJ);
    UniValue verification_errors(UniValue::VARR);

    for (const auto& verification_error : recalc_result.verification_errors) {
        verification_errors.push_back(verification_error);
    }

    result.pushKV("startHeight", recalc_result.start_height);
    result.pushKV("stopHeight", recalc_result.stop_height);
    result.pushKV("diffsRecalculated", recalc_result.diffs_recalculated);
    result.pushKV("snapshotsVerified", recalc_result.snapshots_verified);
    result.pushKV("verificationErrors", verification_errors);

    // Only include repair errors if we're in repair mode
    if (repair) {
        UniValue repair_errors(UniValue::VARR);
        for (const auto& repair_error : recalc_result.repair_errors) {
            repair_errors.push_back(repair_error);
        }
        result.pushKV("repairErrors", repair_errors);
    }

    return result;
}

static RPCHelpMan evodb_verify()
{
    return RPCHelpMan{"evodb verify",
        "\nVerifies evodb diff records between specified block heights.\n"
        "Checks that all diffs applied between snapshots in the range match the saved snapshots in evodb.\n"
        "This is a read-only operation that does not modify the database.\n"
        "If no heights are specified, defaults to the full range from DIP0003 activation to chain tip.\n",
        {
            {"startBlock", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "The starting block hash or height (defaults to DIP0003 activation height).",
             RPCArgOptions{.skip_type_check = true}},
            {"stopBlock", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "The ending block hash or height (defaults to current chain tip).",
             RPCArgOptions{.skip_type_check = true}},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::NUM, "startHeight", "Actual starting block height (may differ from input if clamped to DIP0003 activation)"},
                {RPCResult::Type::NUM, "stopHeight", "Ending block height"},
                {RPCResult::Type::NUM, "diffsRecalculated", "Number of diffs recalculated (always 0 for verify-only mode)"},
                {RPCResult::Type::NUM, "snapshotsVerified", "Number of snapshot pairs that passed verification"},
                {RPCResult::Type::ARR, "verificationErrors", "List of verification errors (empty if verification passed)",
                    {
                        {RPCResult::Type::STR, "", "Error message"},
                    }
                },
            }
        },
        RPCExamples{
            HelpExampleCli("evodb verify", "")
            + HelpExampleCli("evodb verify", "1000 2000")
            + HelpExampleRpc("evodb", "\"verify\"")
            + HelpExampleRpc("evodb", "\"verify\", 1000, 2000")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    return evodb_verify_or_repair_impl(request, false);
},
    };
}

static RPCHelpMan evodb_repair()
{
    return RPCHelpMan{"evodb repair",
        "\nRepairs corrupted evodb diff records between specified block heights.\n"
        "First verifies all diffs applied between snapshots in the range.\n"
        "If verification fails, recalculates diffs from blockchain data and replaces corrupted records.\n"
        "If no heights are specified, defaults to the full range from DIP0003 activation to chain tip.\n",
        {
            {"startBlock", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "The starting block hash or height (defaults to DIP0003 activation height).",
             RPCArgOptions{.skip_type_check = true}},
            {"stopBlock", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "The ending block hash or height (defaults to current chain tip).",
             RPCArgOptions{.skip_type_check = true}},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::NUM, "startHeight", "Actual starting block height (may differ from input if clamped to DIP0003 activation)"},
                {RPCResult::Type::NUM, "stopHeight", "Ending block height"},
                {RPCResult::Type::NUM, "diffsRecalculated", "Number of diffs successfully recalculated and written to database"},
                {RPCResult::Type::NUM, "snapshotsVerified", "Number of snapshot pairs that passed verification"},
                {RPCResult::Type::ARR, "verificationErrors", "Errors encountered during verification phase (empty if verification passed)",
                    {
                        {RPCResult::Type::STR, "", "Error message"},
                    }
                },
                {RPCResult::Type::ARR, "repairErrors", "Critical errors encountered during repair phase (non-empty means full reindex required)",
                    {
                        {RPCResult::Type::STR, "", "Error message"},
                    }
                },
            }
        },
        RPCExamples{
            HelpExampleCli("evodb repair", "")
            + HelpExampleCli("evodb repair", "1000 2000")
            + HelpExampleRpc("evodb", "\"repair\"")
            + HelpExampleRpc("evodb", "\"repair\", 1000, 2000")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    return evodb_verify_or_repair_impl(request, true);
},
    };
}

#ifdef ENABLE_WALLET
static RPCHelpMan protx_shared_register_prepare_wrapper(const MnType mn_type)
{
    const bool evo{mn_type == MnType::Evo};
    std::vector<RPCArg> args{
            {"fundingTx", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The serialized funding transaction with all participants' inputs and change outputs."},
            {"shares", RPCArg::Type::ARR, RPCArg::Optional::NO, "The collateral share table, in consensus-significant order. Amounts must sum to the collateral.",
            {
                {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "",
                {
                    {"amount", RPCArg::Type::NUM, RPCArg::Optional::NO, "Collateral contribution in duffs (at least 100 DASH)"},
                    {"refundAddress", RPCArg::Type::STR, RPCArg::Optional::NO, "Immutable address the principal is refunded to at dissolution"},
                    {"rewardAddress", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Address this share's owner rewards are paid to (defaults to the refund address)"},
                    {"ownerAddress", RPCArg::Type::STR, RPCArg::Optional::NO, "P2PKH address of the immutable share owner key"},
                }},
            }},
            {"coreP2PAddrs", RPCArg::Type::STR, RPCArg::Optional::NO, "IP address and port of the masternode, leave empty to bank on a later ProUpServTx.",
             RPCArgOptions{.skip_type_check = true}},
            {"operatorPubKey", RPCArg::Type::STR, RPCArg::Optional::NO, "The operator BLS public key."},
            {"votingAddress", RPCArg::Type::STR, RPCArg::Optional::NO, "The voting key address."},
            {"operatorReward", RPCArg::Type::STR, RPCArg::Optional::NO, "The fraction in %% to share with the operator (0.00 to 100.00).",
             RPCArgOptions{.skip_type_check = true}},
            {"earlyPeriodBlocks", RPCArg::Type::NUM, RPCArg::Optional::NO, "Length in blocks of the early period during which unilateral dissolution is penalized (up to 420480)."},
            {"earlyPenalty", RPCArg::Type::NUM, RPCArg::Optional::NO, "Penalty in duffs for unilateral dissolution during the early period (must be below the smallest share, and zero when earlyPeriodBlocks is zero)."},
    };
    if (evo) {
        args.push_back(GetRpcArg("platformNodeID"));
        args.push_back({"platformP2PAddrs", RPCArg::Type::ARR, RPCArg::Optional::NO,
            "Array of addresses in the form \"ADDR:PORT\" used by Platform for peer-to-peer connection.\n"
            "Must be unique on the network. Can be set to an empty string, which will require a ProUpServTx afterwards.",
            {
                {"address", RPCArg::Type::STR, RPCArg::Optional::NO, ""},
            },
            RPCArgOptions{.skip_type_check = true}});
        args.push_back({"platformHTTPSAddrs", RPCArg::Type::ARR, RPCArg::Optional::NO,
            "Array of addresses in the form \"ADDR:PORT\" used by Platform for their HTTPS API.\n"
            "Must be unique on the network. Can be set to an empty string, which will require a ProUpServTx afterwards.",
            {
                {"address", RPCArg::Type::STR, RPCArg::Optional::NO, ""},
            },
            RPCArgOptions{.skip_type_check = true}});
    }
    const std::string example_args{"\"fundingTx\" \"[...]\" \"1.2.3.4:1234\" \"operatorPubKey\" \"" + EXAMPLE_ADDRESS[1] +
                                   "\" 0 10000 5000000000"};
    return RPCHelpMan{evo ? "protx shared_register_prepare_evo" : "protx shared_register_prepare",
        std::string{evo ? "\nSame as \"protx shared_register_prepare\", but for an EvoNode. EvoNodes can only register with\n"
                          "shared collateral once the evo_shares deployment is active.\n" : ""} +
        "\nCreates an unsigned shared masternode registration (a version 3 ProRegTx with a collateral share\n"
        "table) by appending the shared-collateral output and payload to a caller-supplied funding\n"
        "transaction. The funding transaction must already contain every participant's contribution inputs\n"
        "and any change outputs; the consent digest binds all of them. Every share owner must sign the\n"
        "returned consent hash via \"protx shared_sign\"; combine with \"protx shared_combine\", then have the\n"
        "funding inputs signed (signrawtransactionwithwallet) and broadcast with sendrawtransaction.\n"
        "\nIMPORTANT: consent is required from each share in the table, not from each funding input; an input\n"
        "is not tied to any share. Before signing its funding inputs, each participant should check the\n"
        "share table (\"proRegTx\" in decoderawtransaction, or \"terms\" from shared_sign) for its own share,\n"
        "with its own amount and addresses. Signing inputs into a table without that share gives the\n"
        "contribution to the other shares.\n"
        "\nIMPORTANT: once the registration confirms, every participant should create a zero-penalty\n"
        "standby dissolution (\"protx shared_dissolve <proTxHash> <shareIndex> <fee> false false\") and store\n"
        "the returned hex with their refund-key backup, separately from the share owner key. It becomes\n"
        "valid when the early period ends (or immediately when no early period applies), then never expires;\n"
        "broadcasting it after that point recovers the participant's principal without cooperation.\n",
        args,
        RPCResult{RPCResult::Type::OBJ, "", "",
        {
            {RPCResult::Type::STR_HEX, "tx", "The serialized unsigned shared ProRegTx"},
            {RPCResult::Type::NUM, "collateralIndex", "Output index of the shared collateral"},
            {RPCResult::Type::STR_HEX, "consentHash", "The consent digest every share owner must sign"},
            CProRegTx::GetJsonHelp("terms", /*optional=*/false),
            {RPCResult::Type::STR, "warning", /*optional=*/true, "Present when earlyPenalty is zero: any participant can force an early exit at no cost beyond the transaction fee"},
        }},
        RPCExamples{evo ? HelpExampleCli("protx", "shared_register_prepare_evo " + example_args + " \"f2dbd9b0a1f541a7c44d34a58674d0262f5feca5\" \"[\\\"1.2.3.4:22821\\\"]\" \"[\\\"1.2.3.4:22822\\\"]\"")
                        : HelpExampleCli("protx", "shared_register_prepare " + example_args)},
        [mn_type](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    NodeContext& node = EnsureAnyNodeContext(request.context);

    interfaces::SharedRegistrationRequest typed_request;
    typed_request.type = mn_type;
    if (!DecodeHexTx(typed_request.funding_tx, request.params[0].get_str())) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "funding transaction not deserializable");
    }
    if (!evo::provider::GetCapabilities(node).extended_addresses) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "shared masternode registration requires provider transaction version 3");
    }

    typed_request.shares = ParseShares(request.params[1], "shares");
    typed_request.net_info.core_p2p = ParseCoreNetInfo(request.params[2], /*optional=*/true);
    typed_request.operator_key = ParseBLSPubKey(request.params[3].get_str(), "operator BLS address",
                                                /*specific_legacy_bls_scheme=*/false);
    typed_request.voting_key = ParsePubKeyIDFromAddress(request.params[4].get_str(), "voting address");

    int64_t operatorReward;
    if (!ParseFixedPoint(request.params[5].getValStr(), 2, &operatorReward)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "operatorReward must be a number");
    }
    if (operatorReward < 0 || operatorReward > 10000) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "operatorReward must be between 0 and 10000");
    }
    typed_request.operator_reward = static_cast<uint16_t>(operatorReward);

    const int64_t earlyPeriodBlocks{request.params[6].getInt<int64_t>()};
    if (earlyPeriodBlocks < 0 || earlyPeriodBlocks > CProRegTx::MAX_EARLY_PERIOD_BLOCKS) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "earlyPeriodBlocks out of range");
    }
    typed_request.early_period_blocks = static_cast<uint32_t>(earlyPeriodBlocks);
    typed_request.early_penalty = request.params[7].getInt<int64_t>();

    if (mn_type == MnType::Evo) {
        typed_request.platform_node_id = ParsePlatformNodeID(request.params[8]);
        typed_request.net_info.platform_p2p = ParsePlatformNetInfo(request.params[9], "platformP2PAddrs",
                                                                   /*extended_addresses=*/true);
        typed_request.net_info.platform_https = ParsePlatformNetInfo(request.params[10], "platformHTTPSAddrs",
                                                                     /*extended_addresses=*/true);
    }

    const auto prepared{UnwrapOrThrow(evo::provider::PrepareSharedRegistration(node, typed_request))};
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("tx", EncodeHexTx(*prepared.tx));
    ret.pushKV("collateralIndex", static_cast<uint64_t>(prepared.collateral_index));
    ret.pushKV("consentHash", prepared.consent_hash.ToString());
    ret.pushKV("terms", CHECK_NONFATAL(GetTxPayload<CProRegTx>(*prepared.tx))->ToJson());
    if (prepared.warning) {
        ret.pushKV("warning", *prepared.warning);
    }
    return ret;
},
    };
}

static RPCHelpMan protx_shared_register_prepare()
{
    return protx_shared_register_prepare_wrapper(MnType::Regular);
}

static RPCHelpMan protx_shared_register_prepare_evo()
{
    return protx_shared_register_prepare_wrapper(MnType::Evo);
}

static RPCHelpMan protx_shared_dissolve_prepare()
{
    return RPCHelpMan{"protx shared_dissolve_prepare",
        "\nCreates an unsigned unanimous ProDisTx dissolving a shared masternode without penalty at any\n"
        "height. Every share owner must sign it via \"protx shared_sign\"; combine and submit the result\n"
        "with \"protx shared_combine\". For a unilateral dissolution use \"protx shared_dissolve\" instead.\n",
        {
            {"proTxHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The hash of the initial ProRegTx."},
            {"actorIndex", RPCArg::Type::NUM, RPCArg::Optional::NO, "Index into the share table of the participant paying the transaction fee."},
            {"fee", RPCArg::Type::NUM, RPCArg::Default{100000}, "Transaction fee in duffs, paid from the actor's share. At most 1000000 duffs (consensus ceiling)."},
        },
        RPCResult{RPCResult::Type::OBJ, "", "",
        {
            {RPCResult::Type::STR_HEX, "tx", "The serialized unsigned ProDisTx"},
            {RPCResult::Type::STR_HEX, "signHash", "The digest every share owner must sign"},
        }},
        RPCExamples{HelpExampleCli("protx", "shared_dissolve_prepare \"proTxHash\" 0")},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    NodeContext& node = EnsureAnyNodeContext(request.context);

    interfaces::SharedDissolvePrepareRequest typed_request;
    typed_request.pro_tx_hash = ParseHashV(request.params[0], "proTxHash");
    typed_request.actor_index = ParseShareIndex(request.params[1], "actorIndex");
    typed_request.fee = request.params[2].isNull() ? 100000 : request.params[2].getInt<int64_t>();

    const auto prepared{UnwrapOrThrow(evo::provider::PrepareSharedDissolution(node, typed_request))};
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("tx", EncodeHexTx(*prepared.tx));
    ret.pushKV("signHash", prepared.sign_hash.ToString());
    return ret;
},
    };
}
#endif // ENABLE_WALLET

static RPCHelpMan protx_help()
{
    return RPCHelpMan{
        "protx",
        "Set of commands to execute ProTx related actions.\n"
        "To get help on individual commands, use \"help protx command\".\n"
        "\nAvailable commands:\n"
#ifdef ENABLE_WALLET
        "  register                 - Create and send ProTx to network\n"
        "  register_fund            - Fund, create and send ProTx to network\n"
        "  register_prepare         - Create an unsigned ProTx\n"
        "  register_evo             - Create and send ProTx to network for an EvoNode\n"
        "  register_fund_evo        - Fund, create and send ProTx to network for an EvoNode\n"
        "  register_prepare_evo     - Create an unsigned ProTx for an EvoNode\n"
        "  register_legacy          - (DEPRECATED) Create a ProTx by parsing BLS using the legacy scheme and send it to network\n"
        "  register_fund_legacy     - (DEPRECATED) Fund and create a ProTx by parsing BLS using the legacy scheme, then send it to network\n"
        "  register_prepare_legacy  - (DEPRECATED) Create an unsigned ProTx by parsing BLS using the legacy scheme\n"
        "  register_submit          - Sign and submit a ProTx\n"
#endif
        "  list                     - List ProTxs\n"
        "  info                     - Return information about a ProTx\n"
#ifdef ENABLE_WALLET
        "  update_service           - Create and send ProUpServTx to network\n"
        "  update_service_evo       - Create and send ProUpServTx to network for an EvoNode\n"
        "  update_registrar         - Create and send ProUpRegTx to network\n"
        "  update_registrar_legacy  - (DEPRECATED) Create ProUpRegTx by parsing BLS using the legacy scheme, then send it to network\n"
        "  revoke                   - Create and send ProUpRevTx to network\n"
        "  shared_register_prepare         - Create an unsigned shared masternode ProTx\n"
        "  shared_register_prepare_evo     - Create an unsigned shared masternode ProTx for an EvoNode\n"
        "  shared_sign                     - Sign a shared masternode transaction with this wallet's share owner keys\n"
        "  shared_combine                  - Combine share owner signatures into a shared masternode transaction\n"
        "  shared_dissolve                 - Create, sign and send a unilateral ProDisTx\n"
        "  shared_dissolve_prepare         - Create an unsigned unanimous ProDisTx\n"
        "  shared_update_share             - Create and send a ProUpShareTx updating one share's reward address\n"
        "  shared_update_registrar_prepare - Create an unsigned ProUpSharedRegTx\n"
#endif
        "  diff                     - Calculate a diff and a proof between two masternode lists\n"
        "  listdiff                 - Calculate a full MN list diff between two masternode lists\n",
        {
            {"command", RPCArg::Type::STR, RPCArg::Optional::NO, "The command to execute"},
        },
        RPCResult{RPCResult::Type::NONE, "", ""},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    throw JSONRPCError(RPC_INVALID_PARAMETER, "Must be a valid command");
},
    };
}

static RPCHelpMan bls_generate()
{
    return RPCHelpMan{
        "bls generate",
        "\nReturns a BLS secret/public key pair.\n",
        {
            {"legacy", RPCArg::Type::BOOL, RPCArg::Default{false}, "(DEPRECATED, can be set if -deprecatedrpc=legacy_mn is passed) Set true to use legacy BLS scheme",
             RPCArgOptions{.skip_type_check = true}},
        },
        RPCResult{RPCResult::Type::OBJ,
                  "",
                  "",
                  {{RPCResult::Type::STR_HEX, "secret", "BLS secret key"},
                   {RPCResult::Type::STR_HEX, "public", "BLS public key"},
                   {RPCResult::Type::STR_HEX, "scheme", "BLS scheme (valid schemes: legacy, basic)"}}},
        RPCExamples{HelpExampleCli("bls generate", "")},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            CBLSSecretKey sk;
            sk.MakeNewKey();
            bool bls_legacy_scheme{false};
            if (!request.params[0].isNull()) {
                if (!IsDeprecatedRPCEnabled("legacy_mn")) {
                    throw std::runtime_error("DEPRECATED: Pass config option -deprecatedrpc=legacy_mn to set this argument");
                }
                bls_legacy_scheme = ParseBoolV(request.params[0], "bls_legacy_scheme");
            }
            UniValue ret(UniValue::VOBJ);
            ret.pushKV("secret", sk.ToString());
            ret.pushKV("public", sk.GetPublicKey().ToString(bls_legacy_scheme));
            std::string bls_scheme_str = bls_legacy_scheme ? "legacy" : "basic";
            ret.pushKV("scheme", bls_scheme_str);
            return ret;
        },
    };
}

static RPCHelpMan bls_fromsecret()
{
    return RPCHelpMan{
        "bls fromsecret",
        "\nParses a BLS secret key and returns the secret/public key pair.\n",
        {
            {"secret", RPCArg::Type::STR, RPCArg::Optional::NO, "The BLS secret key"},
            {"legacy", RPCArg::Type::BOOL, RPCArg::Default{false}, "Pass true if you need in legacy scheme",
             RPCArgOptions{.skip_type_check = true}},
        },
        RPCResult{RPCResult::Type::OBJ,
                  "",
                  "",
                  {
                      {RPCResult::Type::STR_HEX, "secret", "BLS secret key"},
                      {RPCResult::Type::STR_HEX, "public", "BLS public key"},
                      {RPCResult::Type::STR_HEX, "scheme", "BLS scheme (valid schemes: legacy, basic)"},
                  }},
        RPCExamples{
            HelpExampleCli("bls fromsecret", "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f")},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
            bool bls_legacy_scheme{false};
            if (!request.params[1].isNull()) {
                bls_legacy_scheme = ParseBoolV(request.params[1], "bls_legacy_scheme");
            }
            CBLSSecretKey sk = ParseBLSSecretKey(request.params[0].get_str(), "secretKey");
            UniValue ret(UniValue::VOBJ);
            ret.pushKV("secret", sk.ToString());
            ret.pushKV("public", sk.GetPublicKey().ToString(bls_legacy_scheme));
            std::string bls_scheme_str = bls_legacy_scheme ? "legacy" : "basic";
            ret.pushKV("scheme", bls_scheme_str);
            return ret;
        },
    };
}

static RPCHelpMan bls_help()
{
    return RPCHelpMan{"bls",
        "Set of commands to execute BLS related actions.\n"
        "To get help on individual commands, use \"help bls command\".\n"
        "\nAvailable commands:\n"
        "  generate          - Create a BLS secret/public key pair\n"
        "  fromsecret        - Parse a BLS secret key and return the secret/public key pair\n",
        {
            {"command", RPCArg::Type::STR, RPCArg::Optional::NO, "The command to execute"},
        },
        RPCResult{RPCResult::Type::NONE, "", ""},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    throw JSONRPCError(RPC_INVALID_PARAMETER, "Must be a valid command");
},
    };
}

#ifdef ENABLE_WALLET
Span<const CRPCCommand> GetWalletEvoRPCCommands()
{
    static const CRPCCommand commands[]{
        {"evo", &protx_list},
        {"evo", &protx_info},
        {"evo", &protx_register},
        {"evo", &protx_register_evo},
        {"evo", &protx_register_fund},
        {"evo", &protx_register_fund_evo},
        {"evo", &protx_register_prepare},
        {"evo", &protx_register_prepare_evo},
        {"evo", &protx_update_service},
        {"evo", &protx_update_service_evo},
        {"evo", &protx_register_submit},
        {"evo", &protx_update_registrar},
        {"evo", &protx_revoke},
        {"evo", &protx_shared_register_prepare},
        {"evo", &protx_shared_register_prepare_evo},
        {"evo", &protx_shared_sign},
        {"evo", &protx_shared_combine},
        {"evo", &protx_shared_dissolve},
        {"evo", &protx_shared_dissolve_prepare},
        {"evo", &protx_shared_update_share},
        {"evo", &protx_shared_update_registrar_prepare},
        {"hidden", &protx_register_legacy},
        {"hidden", &protx_register_fund_legacy},
        {"hidden", &protx_register_prepare_legacy},
        {"hidden", &protx_update_registrar_legacy},
    };
    return commands;
}
#endif // ENABLE_WALLET

void RegisterEvoRPCCommands(CRPCTable& t)
{
    static const CRPCCommand commands[]{
        {"evo", &bls_help},
        {"evo", &bls_generate},
        {"evo", &bls_fromsecret},
        {"evo", &protx_help},
        {"evo", &protx_diff},
        {"evo", &protx_listdiff},
        {"hidden", &evodb_verify},
        {"hidden", &evodb_repair},
    };
    static const CRPCCommand commands_wallet[]{
        {"evo", &protx_list},
        {"evo", &protx_info},
    };
    for (const auto& command : commands) {
        t.appendCommand(command.name, &command);
    }
    // If we aren't compiling with wallet support, we still need to register RPCs that are
    // capable of working without wallet support. We have to do this even if wallet support
    // is compiled in but is disabled at runtime because runtime disablement prohibits
    // registering wallet RPCs. We still want the reduced functionality RPC to be registered.
    // TODO: Spin off these hybrid RPCs into dedicated wallet-only and/or wallet-free RPCs
    //       and get rid of this workaround.
    if (!g_wallet_init_interface.HasWalletSupport()
#ifdef ENABLE_WALLET
        || gArgs.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET)
#endif // ENABLE_WALLET
    ) {
        for (const auto& command : commands_wallet) {
            t.appendCommand(command.name, &command);
        }
    }
}
