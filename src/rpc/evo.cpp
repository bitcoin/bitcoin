// Copyright (c) 2018-2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <base58.h>
#include <bls/bls.h>
#include <chainparams.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <deploymentstatus.h>
#include <evo/chainhelper.h>
#include <evo/deterministicmns.h>
#include <evo/dmn_types.h>
#include <evo/providertx.h>
#include <evo/simplifiedmns.h>
#include <evo/specialtx.h>
#include <evo/specialtxman.h>
#include <index/txindex.h>
#include <llmq/blockprocessor.h>
#include <llmq/context.h>
#include <masternode/meta.h>
#include <messagesigner.h>
#include <netbase.h>
#include <node/context.h>
#include <rpc/blockchain.h>
#include <rpc/server.h>
#include <rpc/server_util.h>
#include <rpc/util.h>
#include <util/check.h>
#include <util/moneystr.h>
#include <util/translation.h>
#include <validation.h>
#include <wallet/rpc/util.h>
#include <walletinitinterface.h>

#ifdef ENABLE_WALLET
#include <wallet/coincontrol.h>
#include <wallet/spend.h>
#include <wallet/wallet.h>
#endif//ENABLE_WALLET

#ifdef ENABLE_WALLET
extern RPCHelpMan signrawtransactionwithwallet();
extern RPCHelpMan sendrawtransaction();
#else
class CWallet;
#endif//ENABLE_WALLET

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
        {"ipAndPort",
            {"ipAndPort", RPCArg::Type::STR, RPCArg::Optional::NO,
                "IP and port in the form \"IP:PORT\". Must be unique on the network.\n"
                "Can be set to an empty string, which will require a ProUpServTx afterwards."}
        },
        {"ipAndPort_update",
            {"ipAndPort", RPCArg::Type::STR, RPCArg::Optional::NO,
                "IP and port in the form \"IP:PORT\". Must be unique on the network."}
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
                "The value must be between 0 and 10000."}
        },
        {"ownerAddress",
            {"ownerAddress", RPCArg::Type::STR, RPCArg::Optional::NO,
                "The Dash address to use for payee updates and proposal voting.\n"
                "The corresponding private key does not have to be known by your wallet.\n"
                "The address must be unused and must differ from the collateralAddress."}
        },
        {"payoutAddress_register",
            {"payoutAddress", RPCArg::Type::STR, RPCArg::Optional::NO,
                "The Dash address to use for masternode reward payments."}
        },
        {"payoutAddress_update",
            {"payoutAddress", RPCArg::Type::STR, RPCArg::Optional::NO,
                "The Dash address to use for masternode reward payments.\n"
                "If set to an empty string, the currently active payout address is reused."}
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
                "If true, the resulting transaction is sent to the network."}
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
        {"platformP2PPort",
            {"platformP2PPort", RPCArg::Type::NUM, RPCArg::Optional::NO,
                "TCP port of Dash Platform peer-to-peer communication between nodes (network byte order)."}
        },
        {"platformHTTPPort",
            {"platformHTTPPort", RPCArg::Type::NUM, RPCArg::Optional::NO,
                "TCP port of Platform HTTP/API interface (network byte order)."}
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

static bool ValidatePlatformPort(const int32_t port)
{
    return port >= 1 && port <= std::numeric_limits<uint16_t>::max();
}

template <typename SpecialTxPayload>
static void FundSpecialTx(CWallet& wallet, CMutableTransaction& tx, const SpecialTxPayload& payload,
                          const CTxDestination& fundDest) EXCLUSIVE_LOCKS_REQUIRED(!wallet.cs_wallet)
{
    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    wallet.BlockUntilSyncedToCurrentChain();

    LOCK(wallet.cs_wallet);

    CTxDestination nodest = CNoDestination();
    if (fundDest == nodest) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "No source of funds specified");
    }

    CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
    ds << payload;
    tx.vExtraPayload.assign(UCharCast(ds.data()), UCharCast(ds.data() + ds.size()));

    static const CTxOut dummyTxOut(0, CScript() << OP_RETURN);
    std::vector<CRecipient> vecSend;
    bool dummyTxOutAdded = false;

    if (tx.vout.empty()) {
        // add dummy txout as CreateTransaction requires at least one recipient
        tx.vout.emplace_back(dummyTxOut);
        dummyTxOutAdded = true;
    }

    for (const auto& txOut : tx.vout) {
        CRecipient recipient = {txOut.scriptPubKey, txOut.nValue, false};
        vecSend.push_back(recipient);
    }

    CCoinControl coinControl;
    coinControl.destChange = fundDest;
    coinControl.fRequireAllInputs = false;

    std::vector<COutput> vecOutputs;
    AvailableCoins(wallet, vecOutputs);

    for (const auto& out : vecOutputs) {
        CTxDestination txDest;
        if (ExtractDestination(out.txout.scriptPubKey, txDest) && txDest == fundDest) {
            coinControl.Select(out.outpoint);
        }
    }

    if (!coinControl.HasSelected()) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("No funds at specified address %s", EncodeDestination(fundDest)));
    }

    CTransactionRef newTx;
    CAmount nFee;
    int nChangePos = -1;
    bilingual_str strFailReason;

    FeeCalculation fee_calc_out;
    if (!CreateTransaction(wallet, vecSend, newTx, nFee, nChangePos, strFailReason, coinControl, fee_calc_out, false,
                           tx.vExtraPayload.size())) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, strFailReason.original);
    }

    tx.vin = newTx->vin;
    tx.vout = newTx->vout;

    if (dummyTxOutAdded && tx.vout.size() > 1) {
        // CreateTransaction added a change output, so we don't need the dummy txout anymore.
        // Removing it results in slight overpayment of fees, but we ignore this for now (as it's a very low amount).
        auto it = std::find(tx.vout.begin(), tx.vout.end(), dummyTxOut);
        CHECK_NONFATAL(it != tx.vout.end());
        tx.vout.erase(it);
    }
}

template<typename SpecialTxPayload>
static void UpdateSpecialTxInputsHash(const CMutableTransaction& tx, SpecialTxPayload& payload)
{
    payload.inputsHash = CalcTxInputsHash(CTransaction(tx));
}

template<typename SpecialTxPayload>
static void SignSpecialTxPayloadByHash(const CMutableTransaction& tx, SpecialTxPayload& payload, const CKeyID& keyID, const CWallet& wallet)
{
    UpdateSpecialTxInputsHash(tx, payload);
    payload.vchSig.clear();

    const uint256 hash = ::SerializeHash(payload);
    if (!wallet.SignSpecialTxPayload(hash, keyID, payload.vchSig)) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "failed to sign special tx");
    }
}

template <typename SpecialTxPayload>
static void SignSpecialTxPayloadByHash(const CMutableTransaction& tx, SpecialTxPayload& payload,
                                       const CBLSSecretKey& key, bool use_legacy)
{
    UpdateSpecialTxInputsHash(tx, payload);

    uint256 hash = ::SerializeHash(payload);
    payload.sig = key.Sign(hash, use_legacy);
}

static std::string SignAndSendSpecialTx(const JSONRPCRequest& request, CChainstateHelper& chain_helper, const ChainstateManager& chainman, const CMutableTransaction& tx, bool fSubmit = true)
{
    {
    LOCK(cs_main);

    TxValidationState state;
    if (!chain_helper.special_tx->CheckSpecialTx(CTransaction(tx), chainman.ActiveChain().Tip(), chainman.ActiveChainstate().CoinsTip(), true, state)) {
        throw std::runtime_error(state.ToString());
    }
    } // cs_main

    CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
    ds << tx;

    JSONRPCRequest signRequest(request);
    signRequest.params.setArray();
    signRequest.params.push_back(HexStr(ds));
    UniValue signResult = signrawtransactionwithwallet().HandleRequest(signRequest);

    if (!fSubmit) {
        return signResult["hex"].get_str();
    }

    JSONRPCRequest sendRequest(request);
    sendRequest.params.setArray();
    sendRequest.params.push_back(signResult["hex"].get_str());
    return sendrawtransaction().HandleRequest(sendRequest).get_str();
}

// forward declaration
namespace {
enum class ProTxRegisterAction
{
    External,
    Fund,
    Prepare,
};
} // anonumous namespace

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
            GetRpcArg("ipAndPort"),
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
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
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
            GetRpcArg("ipAndPort"),
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
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
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
            GetRpcArg("ipAndPort"),
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
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
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
            GetRpcArg("ipAndPort"),
            GetRpcArg("ownerAddress"),
            GetRpcArg("operatorPubKey_register"),
            GetRpcArg("votingAddress_register"),
            GetRpcArg("operatorReward"),
            GetRpcArg("payoutAddress_register"),
            GetRpcArg("platformNodeID"),
            GetRpcArg("platformP2PPort"),
            GetRpcArg("platformHTTPPort"),
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
            GetRpcArg("ipAndPort"),
            GetRpcArg("ownerAddress"),
            GetRpcArg("operatorPubKey_register"),
            GetRpcArg("votingAddress_register"),
            GetRpcArg("operatorReward"),
            GetRpcArg("payoutAddress_register"),
            GetRpcArg("platformNodeID"),
            GetRpcArg("platformP2PPort"),
            GetRpcArg("platformHTTPPort"),
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
            GetRpcArg("ipAndPort"),
            GetRpcArg("ownerAddress"),
            GetRpcArg("operatorPubKey_register"),
            GetRpcArg("votingAddress_register"),
            GetRpcArg("operatorReward"),
            GetRpcArg("payoutAddress_register"),
            GetRpcArg("platformNodeID"),
            GetRpcArg("platformP2PPort"),
            GetRpcArg("platformHTTPPort"),
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
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureChainman(node);

    CChainstateHelper& chain_helper = *CHECK_NONFATAL(node.chain_helper);

    const bool isEvoRequested = mnType == MnType::Evo;

    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    if (action == ProTxRegisterAction::External || action == ProTxRegisterAction::Fund) {
        EnsureWalletIsUnlocked(*pwallet);
    }

    size_t paramIdx = 0;

    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_PROVIDER_REGISTER;

    const bool use_legacy = specific_legacy_bls_scheme;

    CProRegTx ptx;
    ptx.nType = mnType;
    ptx.nVersion = CProRegTx::GetMaxVersion(/*is_basic_scheme_active=*/!use_legacy);

    if (action == ProTxRegisterAction::Fund) {
        CTxDestination collateralDest = DecodeDestination(request.params[paramIdx].get_str());
        if (!IsValidDestination(collateralDest)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("invalid collaterall address: %s", request.params[paramIdx].get_str()));
        }
        CScript collateralScript = GetScriptForDestination(collateralDest);

        CAmount fundCollateral = GetMnType(mnType).collat_amount;
        CTxOut collateralTxOut(fundCollateral, collateralScript);
        tx.vout.emplace_back(collateralTxOut);

        paramIdx++;
    } else {
        uint256 collateralHash(ParseHashV(request.params[paramIdx], "collateralHash"));
        int32_t collateralIndex = ParseInt32V(request.params[paramIdx + 1], "collateralIndex");
        if (collateralHash.IsNull() || collateralIndex < 0) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("invalid hash or index: %s-%d", collateralHash.ToString(), collateralIndex));
        }

        ptx.collateralOutpoint = COutPoint(collateralHash, (uint32_t)collateralIndex);
        paramIdx += 2;
    }

    if (!request.params[paramIdx].get_str().empty()) {
        if (auto entryRet = ptx.netInfo.AddEntry(request.params[paramIdx].get_str()); entryRet != NetInfoStatus::Success) {
            throw std::runtime_error(strprintf("%s (%s)", NISToString(entryRet), request.params[paramIdx].get_str()));
        }
    }

    ptx.keyIDOwner = ParsePubKeyIDFromAddress(request.params[paramIdx + 1].get_str(), "owner address");
    ptx.pubKeyOperator.Set(ParseBLSPubKey(request.params[paramIdx + 2].get_str(), "operator BLS address", use_legacy), use_legacy);
    CHECK_NONFATAL(ptx.pubKeyOperator.IsLegacy() == (ptx.nVersion == ProTxVersion::LegacyBLS));

    CKeyID keyIDVoting = ptx.keyIDOwner;

    if (!request.params[paramIdx + 3].get_str().empty()) {
        keyIDVoting = ParsePubKeyIDFromAddress(request.params[paramIdx + 3].get_str(), "voting address");
    }

    int64_t operatorReward;
    if (!ParseFixedPoint(request.params[paramIdx + 4].getValStr(), 2, &operatorReward)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "operatorReward must be a number");
    }
    if (operatorReward < 0 || operatorReward > 10000) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "operatorReward must be between 0 and 10000");
    }
    ptx.nOperatorReward = operatorReward;

    CTxDestination payoutDest = DecodeDestination(request.params[paramIdx + 5].get_str());
    if (!IsValidDestination(payoutDest)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("invalid payout address: %s", request.params[paramIdx + 5].get_str()));
    }

    if (isEvoRequested) {
        if (!IsHex(request.params[paramIdx + 6].get_str())) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "platformNodeID must be hexadecimal string");
        }
        ptx.platformNodeID.SetHex(request.params[paramIdx + 6].get_str());

        int32_t requestedPlatformP2PPort = ParseInt32V(request.params[paramIdx + 7], "platformP2PPort");
        if (!ValidatePlatformPort(requestedPlatformP2PPort)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "platformP2PPort must be a valid port [1-65535]");
        }
        ptx.platformP2PPort = static_cast<uint16_t>(requestedPlatformP2PPort);

        int32_t requestedPlatformHTTPPort = ParseInt32V(request.params[paramIdx + 8], "platformHTTPPort");
        if (!ValidatePlatformPort(requestedPlatformHTTPPort)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "platformHTTPPort must be a valid port [1-65535]");
        }
        ptx.platformHTTPPort = static_cast<uint16_t>(requestedPlatformHTTPPort);

        paramIdx += 3;
    }

    ptx.keyIDVoting = keyIDVoting;
    ptx.scriptPayout = GetScriptForDestination(payoutDest);

    if (action != ProTxRegisterAction::Fund) {
        // make sure fee calculation works
        ptx.vchSig.resize(65);
    }

    CTxDestination fundDest = payoutDest;
    if (!request.params[paramIdx + 6].isNull()) {
        fundDest = DecodeDestination(request.params[paramIdx + 6].get_str());
        if (!IsValidDestination(fundDest))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Dash address: ") + request.params[paramIdx + 6].get_str());
    }

    bool fSubmit{true};
    if ((action == ProTxRegisterAction::External || action == ProTxRegisterAction::Fund) && !request.params[paramIdx + 7].isNull()) {
        fSubmit = ParseBoolV(request.params[paramIdx + 7], "submit");
    }

    if (action == ProTxRegisterAction::Fund) {
        FundSpecialTx(*pwallet, tx, ptx, fundDest);
        UpdateSpecialTxInputsHash(tx, ptx);
        CAmount fundCollateral = GetMnType(mnType).collat_amount;
        uint32_t collateralIndex = (uint32_t) -1;
        for (uint32_t i = 0; i < tx.vout.size(); i++) {
            if (tx.vout[i].nValue == fundCollateral) {
                collateralIndex = i;
                break;
            }
        }
        CHECK_NONFATAL(collateralIndex != (uint32_t) -1);
        ptx.collateralOutpoint.n = collateralIndex;

        SetTxPayload(tx, ptx);
        return SignAndSendSpecialTx(request, chain_helper, chainman, tx, fSubmit);
    } else {
        // referencing external collateral

        const bool unlockOnError = [&]() {
            if (LOCK(pwallet->cs_wallet); !pwallet->IsLockedCoin(ptx.collateralOutpoint.hash, ptx.collateralOutpoint.n)) {
                pwallet->LockCoin(ptx.collateralOutpoint);
                return true;
            }
            return false;
        }();
        try {
            FundSpecialTx(*pwallet, tx, ptx, fundDest);
            UpdateSpecialTxInputsHash(tx, ptx);
            Coin coin;
            if (!GetUTXOCoin(chainman.ActiveChainstate(), ptx.collateralOutpoint, coin)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("collateral not found: %s", ptx.collateralOutpoint.ToStringShort()));
            }
            CTxDestination txDest;
            ExtractDestination(coin.out.scriptPubKey, txDest);
            const PKHash* pkhash = std::get_if<PKHash>(&txDest);
            if (!pkhash) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("collateral type not supported: %s", ptx.collateralOutpoint.ToStringShort()));
            }

            if (action == ProTxRegisterAction::Prepare) {
                // external signing with collateral key
                ptx.vchSig.clear();
                SetTxPayload(tx, ptx);

                UniValue ret(UniValue::VOBJ);
                ret.pushKV("tx", EncodeHexTx(CTransaction(tx)));
                ret.pushKV("collateralAddress", EncodeDestination(txDest));
                ret.pushKV("signMessage", ptx.MakeSignString());
                return ret;
            } else {
                {
                    LOCK(pwallet->cs_wallet);
                    // lets prove we own the collateral
                    CScript scriptPubKey = GetScriptForDestination(txDest);
                    std::unique_ptr<SigningProvider> provider = pwallet->GetSolvingProvider(scriptPubKey);

                    std::string signed_payload;
                    SigningResult err = pwallet->SignMessage(ptx.MakeSignString(), *pkhash, signed_payload);
                    if (err == SigningResult::SIGNING_FAILED) {
                        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, SigningResultString(err));
                    } else if (err != SigningResult::OK){
                        throw JSONRPCError(RPC_WALLET_ERROR, SigningResultString(err));
                    }
                    auto opt_vchSig = DecodeBase64(signed_payload);
                    if (!opt_vchSig.has_value()) throw JSONRPCError(RPC_INTERNAL_ERROR, "failed to decode base64 ready signature for protx");
                    ptx.vchSig = opt_vchSig.value();
                } // cs_wallet
                SetTxPayload(tx, ptx);
                return SignAndSendSpecialTx(request, chain_helper, chainman, tx, fSubmit);
            }
        } catch (...) {
            if (unlockOnError) {
                WITH_LOCK(pwallet->cs_wallet, pwallet->UnlockCoin(ptx.collateralOutpoint));
            }
            throw;
        }
    }
}

static RPCHelpMan protx_register_submit()
{
    return RPCHelpMan{"protx register_submit",
        "\nCombines the unsigned ProTx and a signature of the signMessage, signs all inputs\n"
        "which were added to cover fees and submits the resulting transaction to the network.\n"
        "Note: See \"help protx register_prepare\" for more info about creating a ProTx and a message to sign.\n"
        + HELP_REQUIRING_PASSPHRASE,
        {
            {"tx", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The serialized unsigned ProTx in hex format."},
            {"sig", RPCArg::Type::STR, RPCArg::Optional::NO, "The signature signed with the collateral key. Must be in base64 format."},
        },
        RPCResult{
            RPCResult::Type::STR_HEX, "txid", "The transaction id"
        },
        RPCExamples{
            HelpExampleCli("protx", "register_submit \"tx\" \"sig\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureChainman(node);

    CChainstateHelper& chain_helper = *CHECK_NONFATAL(node.chain_helper);

    const std::shared_ptr<const CWallet> wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;

    EnsureWalletIsUnlocked(*wallet);

    CMutableTransaction tx;
    if (!DecodeHexTx(tx, request.params[0].get_str())) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "transaction not deserializable");
    }
    if (tx.nType != TRANSACTION_PROVIDER_REGISTER) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "transaction not a ProRegTx");
    }
    auto ptx = [&tx]() {
        if (const auto opt_ptx = GetTxPayload<CProRegTx>(tx); opt_ptx.has_value()) {
            return *opt_ptx;
        }
        throw JSONRPCError(RPC_INVALID_PARAMETER, "transaction payload not deserializable");
    }();
    if (!ptx.vchSig.empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "payload signature not empty");
    }

    auto opt_vchSig= DecodeBase64(request.params[1].get_str());
    if (!opt_vchSig.has_value()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "malformed base64 encoding");
    }
    ptx.vchSig = opt_vchSig.value();

    SetTxPayload(tx, ptx);
    return SignAndSendSpecialTx(request, chain_helper, chainman, tx);
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
            GetRpcArg("ipAndPort_update"),
            GetRpcArg("operatorKey"),
            GetRpcArg("operatorPayoutAddress"),
            GetRpcArg("feeSourceAddress"),
        },
        RPCResult{
            RPCResult::Type::STR_HEX, "txid", "The transaction id"
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
            GetRpcArg("ipAndPort_update"),
            GetRpcArg("operatorKey"),
            GetRpcArg("platformNodeID"),
            GetRpcArg("platformP2PPort"),
            GetRpcArg("platformHTTPPort"),
            GetRpcArg("operatorPayoutAddress"),
            GetRpcArg("feeSourceAddress"),
        },
        RPCResult{
            RPCResult::Type::STR_HEX, "txid", "The transaction id"},
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
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureChainman(node);

    CDeterministicMNManager& dmnman = *CHECK_NONFATAL(node.dmnman);
    CChainstateHelper& chain_helper = *CHECK_NONFATAL(node.chain_helper);

    const bool isEvoRequested = mnType == MnType::Evo;
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;

    EnsureWalletIsUnlocked(*wallet);

    CProUpServTx ptx;
    ptx.proTxHash = ParseHashV(request.params[0], "proTxHash");
    auto dmn = dmnman.GetListAtChainTip().GetMN(ptx.proTxHash);
    if (!dmn) {
        throw std::runtime_error(strprintf("masternode with proTxHash %s not found", ptx.proTxHash.ToString()));
    }

    ptx.nType = mnType;
    if (dmn->nType != mnType) {
        throw std::runtime_error(strprintf("masternode with proTxHash %s is not a %s", ptx.proTxHash.ToString(), GetMnType(mnType).description));
    }

    ptx.nVersion = dmn->pdmnState->nVersion;

    if (auto entryRet = ptx.netInfo.AddEntry(request.params[1].get_str()); entryRet != NetInfoStatus::Success) {
        throw std::runtime_error(strprintf("%s (%s)", NISToString(entryRet), request.params[1].get_str()));
    }

    CBLSSecretKey keyOperator = ParseBLSSecretKey(request.params[2].get_str(), "operatorKey");

    size_t paramIdx = 3;
    if (isEvoRequested) {
        if (!IsHex(request.params[paramIdx].get_str())) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "platformNodeID must be hexadecimal string");
        }
        ptx.platformNodeID.SetHex(request.params[paramIdx].get_str());

        int32_t requestedPlatformP2PPort = ParseInt32V(request.params[paramIdx + 1], "platformP2PPort");
        if (!ValidatePlatformPort(requestedPlatformP2PPort)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "platformP2PPort must be a valid port [1-65535]");
        }
        ptx.platformP2PPort = static_cast<uint16_t>(requestedPlatformP2PPort);

        int32_t requestedPlatformHTTPPort = ParseInt32V(request.params[paramIdx + 2], "platformHTTPPort");
        if (!ValidatePlatformPort(requestedPlatformHTTPPort)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "platformHTTPPort must be a valid port [1-65535]");
        }
        ptx.platformHTTPPort = static_cast<uint16_t>(requestedPlatformHTTPPort);

        paramIdx += 3;
    }

    if (keyOperator.GetPublicKey() != dmn->pdmnState->pubKeyOperator.Get()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("the operator key does not belong to the registered public key"));
    }

    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_PROVIDER_UPDATE_SERVICE;

    // param operatorPayoutAddress
    if (!request.params[paramIdx].isNull()) {
        if (request.params[paramIdx].get_str().empty()) {
            ptx.scriptOperatorPayout = dmn->pdmnState->scriptOperatorPayout;
        } else {
            CTxDestination payoutDest = DecodeDestination(request.params[paramIdx].get_str());
            if (!IsValidDestination(payoutDest)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("invalid operator payout address: %s", request.params[paramIdx].get_str()));
            }
            ptx.scriptOperatorPayout = GetScriptForDestination(payoutDest);
        }
    } else {
        ptx.scriptOperatorPayout = dmn->pdmnState->scriptOperatorPayout;
    }

    CTxDestination feeSource;

    // param feeSourceAddress
    if (!request.params[paramIdx + 1].isNull()) {
        feeSource = DecodeDestination(request.params[paramIdx + 1].get_str());
        if (!IsValidDestination(feeSource))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Dash address: ") + request.params[paramIdx + 1].get_str());
    } else {
        if (ptx.scriptOperatorPayout != CScript()) {
            // use operator reward address as default source for fees
            ExtractDestination(ptx.scriptOperatorPayout, feeSource);
        } else {
            // use payout address as default source for fees
            ExtractDestination(dmn->pdmnState->scriptPayout, feeSource);
        }
    }

    FundSpecialTx(*wallet, tx, ptx, feeSource);

    const bool isV19active = DeploymentActiveAfter(WITH_LOCK(cs_main, return chainman.ActiveChain().Tip();),
                                                   Params().GetConsensus(), Consensus::DEPLOYMENT_V19);
    SignSpecialTxPayloadByHash(tx, ptx, keyOperator, !isV19active);
    SetTxPayload(tx, ptx);

    return SignAndSendSpecialTx(request, chain_helper, chainman, tx);
}

static RPCHelpMan protx_update_registrar_wrapper(const bool specific_legacy_bls_scheme)
{
    std::string rpc_name = specific_legacy_bls_scheme ? "update_registrar_legacy" : "update_registrar";
    std::string rpc_full_name = std::string("protx ").append(rpc_name);
    std::string pubkey_operator = specific_legacy_bls_scheme ? "\"0532646990082f4fd639f90387b1551f2c7c39d37392cb9055a06a7e85c1d23692db8f87f827886310bccc1e29db9aee\"" : "\"8532646990082f4fd639f90387b1551f2c7c39d37392cb9055a06a7e85c1d23692db8f87f827886310bccc1e29db9aee\"";
    std::string rpc_example = rpc_name.append(" \"0123456701234567012345670123456701234567012345670123456701234567\" ").append(pubkey_operator).append(" \"" + EXAMPLE_ADDRESS[1] + "\"");
    return RPCHelpMan{rpc_full_name,
        "\nCreates and sends a ProUpRegTx to the network. This will update the operator key, voting key and payout\n"
        "address of the masternode specified by \"proTxHash\".\n"
        "The owner key of the masternode must be known to your wallet.\n"
        + std::string(specific_legacy_bls_scheme ? "\nDEPRECATED: May be removed in a future version, pass config option -deprecatedrpc=legacy_mn to use RPC\n" : "")
        + HELP_REQUIRING_PASSPHRASE,
        {
            GetRpcArg("proTxHash"),
            specific_legacy_bls_scheme ? GetRpcArg("operatorPubKey_update_legacy") : GetRpcArg("operatorPubKey_update"),
            GetRpcArg("votingAddress_update"),
            GetRpcArg("payoutAddress_update"),
            GetRpcArg("feeSourceAddress"),
        },
        RPCResult{
            RPCResult::Type::STR_HEX, "txid", "The transaction id"
        },
        RPCExamples{
            HelpExampleCli("protx", rpc_example)
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const bool use_legacy{self.m_name == "protx update_registrar_legacy"};
    if (use_legacy && !IsDeprecatedRPCEnabled("legacy_mn")) {
        throw std::runtime_error("DEPRECATED: Pass config option -deprecatedrpc=legacy_mn to enable this RPC");
    }

    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureChainman(node);

    CDeterministicMNManager& dmnman = *CHECK_NONFATAL(node.dmnman);
    CChainstateHelper& chain_helper = *CHECK_NONFATAL(node.chain_helper);

    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;

    EnsureWalletIsUnlocked(*wallet);

    CProUpRegTx ptx;
    ptx.nVersion = CProUpRegTx::GetMaxVersion(/*is_basic_scheme_active=*/!use_legacy);

    ptx.proTxHash = ParseHashV(request.params[0], "proTxHash");
    auto dmn = dmnman.GetListAtChainTip().GetMN(ptx.proTxHash);
    if (!dmn) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("masternode %s not found", ptx.proTxHash.ToString()));
    }

    ptx.keyIDVoting = dmn->pdmnState->keyIDVoting;
    ptx.scriptPayout = dmn->pdmnState->scriptPayout;

    if (!request.params[1].get_str().empty()) {
        // new pubkey
        ptx.pubKeyOperator.Set(ParseBLSPubKey(request.params[1].get_str(), "operator BLS address", use_legacy), use_legacy);
    } else {
        // same pubkey, reuse as is
        ptx.pubKeyOperator = dmn->pdmnState->pubKeyOperator;
    }

    CHECK_NONFATAL(ptx.pubKeyOperator.IsLegacy() == (ptx.nVersion == ProTxVersion::LegacyBLS));

    if (!request.params[2].get_str().empty()) {
        ptx.keyIDVoting = ParsePubKeyIDFromAddress(request.params[2].get_str(), "voting address");
    }

    CTxDestination payoutDest;
    ExtractDestination(ptx.scriptPayout, payoutDest);
    if (!request.params[3].get_str().empty()) {
        payoutDest = DecodeDestination(request.params[3].get_str());
        if (!IsValidDestination(payoutDest)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("invalid payout address: %s", request.params[3].get_str()));
        }
        ptx.scriptPayout = GetScriptForDestination(payoutDest);
    }

    {
        const auto pkhash{PKHash(dmn->pdmnState->keyIDOwner)};
        LOCK(wallet->cs_wallet);
        if (wallet->IsMine(GetScriptForDestination(pkhash)) != isminetype::ISMINE_SPENDABLE) {
            throw std::runtime_error(strprintf("Private key for owner address %s not found in your wallet", EncodeDestination(pkhash)));
        }
    }

    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_PROVIDER_UPDATE_REGISTRAR;

    // make sure we get anough fees added
    ptx.vchSig.resize(65);

    CTxDestination feeSourceDest = payoutDest;
    if (!request.params[4].isNull()) {
        feeSourceDest = DecodeDestination(request.params[4].get_str());
        if (!IsValidDestination(feeSourceDest))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Dash address: ") + request.params[4].get_str());
    }

    FundSpecialTx(*wallet, tx, ptx, feeSourceDest);
    SignSpecialTxPayloadByHash(tx, ptx, dmn->pdmnState->keyIDOwner, *wallet);
    SetTxPayload(tx, ptx);

    return SignAndSendSpecialTx(request, chain_helper, chainman, tx);
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
    return RPCHelpMan{"protx revoke",
        "\nCreates and sends a ProUpRevTx to the network. This will revoke the operator key of the masternode and\n"
        "put it into the PoSe-banned state. It will also set the service field of the masternode\n"
        "to zero. Use this in case your operator key got compromised or you want to stop providing your service\n"
        "to the masternode owner.\n"
        + HELP_REQUIRING_PASSPHRASE,
        {
            GetRpcArg("proTxHash"),
            GetRpcArg("operatorKey"),
            GetRpcArg("reason"),
            GetRpcArg("feeSourceAddress"),
        },
        RPCResult{
            RPCResult::Type::STR_HEX, "txid", "The transaction id"
        },
        RPCExamples{
            HelpExampleCli("protx", "revoke \"0123456701234567012345670123456701234567012345670123456701234567\" \"072f36a77261cdd5d64c32d97bac417540eddca1d5612f416feb07ff75a8e240\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureChainman(node);

    CDeterministicMNManager& dmnman = *CHECK_NONFATAL(node.dmnman);
    CChainstateHelper& chain_helper = *CHECK_NONFATAL(node.chain_helper);

    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    EnsureWalletIsUnlocked(*pwallet);

    const bool isV19active{DeploymentActiveAfter(WITH_LOCK(cs_main, return chainman.ActiveChain().Tip();), Params().GetConsensus(), Consensus::DEPLOYMENT_V19)};
    CProUpRevTx ptx;
    ptx.nVersion = CProUpRevTx::GetMaxVersion(/*is_basic_scheme_active=*/isV19active);

    ptx.proTxHash = ParseHashV(request.params[0], "proTxHash");
    auto dmn = dmnman.GetListAtChainTip().GetMN(ptx.proTxHash);
    if (!dmn) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("masternode %s not found", ptx.proTxHash.ToString()));
    }

    CBLSSecretKey keyOperator = ParseBLSSecretKey(request.params[1].get_str(), "operatorKey");

    if (!request.params[2].isNull()) {
        int32_t nReason = ParseInt32V(request.params[2], "reason");
        if (nReason < 0 || nReason > CProUpRevTx::REASON_LAST) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("invalid reason %d, must be between 0 and %d", nReason, CProUpRevTx::REASON_LAST));
        }
        ptx.nReason = (uint16_t)nReason;
    }

    if (keyOperator.GetPublicKey() != dmn->pdmnState->pubKeyOperator.Get()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("the operator key does not belong to the registered public key"));
    }

    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_PROVIDER_UPDATE_REVOKE;

    if (!request.params[3].isNull()) {
        CTxDestination feeSourceDest = DecodeDestination(request.params[3].get_str());
        if (!IsValidDestination(feeSourceDest))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Dash address: ") + request.params[3].get_str());
        FundSpecialTx(*pwallet, tx, ptx, feeSourceDest);
    } else if (dmn->pdmnState->scriptOperatorPayout != CScript()) {
        // Using funds from previousely specified operator payout address
        CTxDestination txDest;
        ExtractDestination(dmn->pdmnState->scriptOperatorPayout, txDest);
        FundSpecialTx(*pwallet, tx, ptx, txDest);
    } else if (dmn->pdmnState->scriptPayout != CScript()) {
        // Using funds from previousely specified masternode payout address
        CTxDestination txDest;
        ExtractDestination(dmn->pdmnState->scriptPayout, txDest);
        FundSpecialTx(*pwallet, tx, ptx, txDest);
    } else {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "No payout or fee source addresses found, can't revoke");
    }

    SignSpecialTxPayloadByHash(tx, ptx, keyOperator, !isV19active);
    SetTxPayload(tx, ptx);

    return SignAndSendSpecialTx(request, chain_helper, chainman, tx);
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

static bool CheckWalletOwnsKey(const CWallet* const pwallet, const CKeyID& keyID) {
    return CheckWalletOwnsScript(pwallet, GetScriptForDestination(PKHash(keyID)));
}
#endif

static UniValue BuildDMNListEntry(const CWallet* const pwallet, const CDeterministicMN& dmn, CMasternodeMetaMan& mn_metaman, bool detailed, const ChainstateManager& chainman, const CBlockIndex* pindex = nullptr)
{
    if (!detailed) {
        return dmn.proTxHash.ToString();
    }

    UniValue o = dmn.ToJson();

    CTransactionRef collateralTx{nullptr};
    int confirmations = GetUTXOConfirmations(chainman.ActiveChainstate(), dmn.collateralOutpoint);

    if (pindex != nullptr) {
        if (confirmations > -1) {
            confirmations -= WITH_LOCK(cs_main, return chainman.ActiveChain().Height()) - pindex->nHeight;
        } else {
            uint256 minedBlockHash;
            collateralTx = GetTransaction(/* pindex */ nullptr, /* mempool */ nullptr, dmn.collateralOutpoint.hash, Params().GetConsensus(), minedBlockHash);
            const CBlockIndex* const pindexMined = WITH_LOCK(cs_main, return chainman.m_blockman.LookupBlockIndex(minedBlockHash));
            CHECK_NONFATAL(pindexMined != nullptr);
            CHECK_NONFATAL(pindex->GetAncestor(pindexMined->nHeight) == pindexMined);
            confirmations = pindex->nHeight - pindexMined->nHeight + 1;
        }
    }
    o.pushKV("confirmations", confirmations);

#ifdef ENABLE_WALLET
    bool hasOwnerKey = CheckWalletOwnsKey(pwallet, dmn.pdmnState->keyIDOwner);
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
        walletObj.pushKV("ownsPayeeScript", CheckWalletOwnsScript(pwallet, dmn.pdmnState->scriptPayout));
        walletObj.pushKV("ownsOperatorRewardScript", CheckWalletOwnsScript(pwallet, dmn.pdmnState->scriptOperatorPayout));
        o.pushKV("wallet", walletObj);
    }
#endif

    const auto metaInfo = mn_metaman.GetMetaInfo(dmn.proTxHash);
    o.pushKV("metaInfo", metaInfo->ToJson());

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
            {"detailed", RPCArg::Type::BOOL, RPCArg::Default{false}, "If not specified, only the hashes of the ProTx will be returned."},
            {"height", RPCArg::Type::NUM, RPCArg::DefaultHint{"current chain-tip"}, ""},
        },
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureChainman(node);

    CDeterministicMNManager& dmnman = *CHECK_NONFATAL(node.dmnman);
    CMasternodeMetaMan& mn_metaman = *CHECK_NONFATAL(node.mn_metaman);

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

        LOCK2(wallet->cs_wallet, cs_main);
        int height = !request.params[2].isNull() ? ParseInt32V(request.params[2], "height") : chainman.ActiveChain().Height();
        if (height < 1 || height > chainman.ActiveChain().Height()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid height specified");
        }

        std::set<COutPoint> setOutpts;
        for (const auto& outpt : wallet->ListProTxCoins()) {
            setOutpts.emplace(outpt);
        }

        CDeterministicMNList mnList = dmnman.GetListForBlock(chainman.ActiveChain()[height]);
        mnList.ForEachMN(false, [&](const auto& dmn) {
            if (setOutpts.count(dmn.collateralOutpoint) ||
                CheckWalletOwnsKey(wallet.get(), dmn.pdmnState->keyIDOwner) ||
                CheckWalletOwnsKey(wallet.get(), dmn.pdmnState->keyIDVoting) ||
                CheckWalletOwnsScript(wallet.get(), dmn.pdmnState->scriptPayout) ||
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
        LOCK2(wallet ? wallet->cs_wallet : cs_main, cs_main);
#else
        LOCK(cs_main);
#endif
        int height = !request.params[2].isNull() ? ParseInt32V(request.params[2], "height") : chainman.ActiveChain().Height();
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
    CMasternodeMetaMan& mn_metaman = *CHECK_NONFATAL(node.mn_metaman);

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
        LOCK(cs_main);
        pindex = chainman.ActiveChain().Tip();
    } else {
        LOCK(cs_main);
        uint256 blockHash(ParseHashV(request.params[1], "blockHash"));
        pindex = chainman.m_blockman.LookupBlockIndex(blockHash);
        if (pindex == nullptr) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
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

static uint256 ParseBlock(const UniValue& v, const ChainstateManager& chainman, const std::string& strName) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);

    try {
        return ParseHashV(v, strName);
    } catch (...) {
        int h = ParseInt32V(v, strName);
        if (h < 1 || h > chainman.ActiveChain().Height())
            throw std::runtime_error(strprintf("%s must be a block hash or chain height and not %s", strName, v.getValStr()));
        return *chainman.ActiveChain()[h]->phashBlock;
    }
}

static RPCHelpMan protx_diff()
{
    return RPCHelpMan{"protx diff",
        "\nCalculates a diff between two deterministic masternode lists. The result also contains proof data.\n",
        {
            {"baseBlock", RPCArg::Type::NUM, RPCArg::Optional::NO, "The starting block height."},
            {"block", RPCArg::Type::NUM, RPCArg::Optional::NO, "The ending block height."},
            {"extended", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED, "Show additional fields."},
        },
        RPCResults{},
        RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureChainman(node);

    CDeterministicMNManager& dmnman = *CHECK_NONFATAL(node.dmnman);
    const LLMQContext& llmq_ctx = *CHECK_NONFATAL(node.llmq_ctx);

    LOCK(cs_main);
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

static const CBlockIndex* ParseBlockIndex(const UniValue& v, const ChainstateManager& chainman, const std::string& strName) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);

    try {
        uint256 hash = ParseHashV(v, strName);
        const CBlockIndex* pblockindex = chainman.m_blockman.LookupBlockIndex(hash);
        if (!pblockindex)
            throw std::runtime_error(strprintf("Block %s with hash %s not found", strName, v.getValStr()));
        return pblockindex;
    } catch (...) {
        int h = ParseInt32V(v, strName);
        if (h < 1 || h > chainman.ActiveChain().Height())
            throw std::runtime_error(strprintf("%s must be a chain height and not %s", strName, v.getValStr()));
        return chainman.ActiveChain()[h];
    }
}

static RPCHelpMan protx_listdiff()
{
    return RPCHelpMan{"protx listdiff",
               "\nCalculate a full MN list diff between two masternode lists.\n",
               {
                       {"baseBlock", RPCArg::Type::NUM, RPCArg::Optional::NO, "The starting block height."},
                       {"block", RPCArg::Type::NUM, RPCArg::Optional::NO, "The ending block height."},
               },
               RPCResults{},
               RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    const ChainstateManager& chainman = EnsureChainman(node);

    CDeterministicMNManager& dmnman = *CHECK_NONFATAL(node.dmnman);

    LOCK(cs_main);
    UniValue ret(UniValue::VOBJ);

    const CBlockIndex* pBaseBlockIndex = ParseBlockIndex(request.params[0], chainman, "baseBlock");
    const CBlockIndex* pTargetBlockIndex = ParseBlockIndex(request.params[1], chainman, "block");

    if (pBaseBlockIndex == nullptr) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Base block not found");
    }

    if (pTargetBlockIndex == nullptr) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");
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
        auto dmn = baseBlockMNList.GetMNByInternalId(internal_id);
        // BuildDiff will construct itself with MNs that we already have knowledge
        // of, meaning that fetch operations should never fail.
        CHECK_NONFATAL(dmn);
        UniValue obj(UniValue::VOBJ);
        obj.pushKV(dmn->proTxHash.ToString(), stateDiff.ToJson(dmn->nType));
        jupdatedMNs.push_back(obj);
    }
    ret.pushKV("updatedMNs", jupdatedMNs);

    return ret;
},
    };
}

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
#endif
        "  diff                     - Calculate a diff and a proof between two masternode lists\n"
        "  listdiff                 - Calculate a full MN list diff between two masternode lists\n",
        {
            {"command", RPCArg::Type::STR, RPCArg::Optional::NO, "The command to execute"},
        },
        RPCResults{},
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
            {"legacy", RPCArg::Type::BOOL, RPCArg::Default{false}, "(DEPRECATED, can be set if -deprecatedrpc=legacy_mn is passed) Set true to use legacy BLS scheme"},
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
            {"legacy", RPCArg::Type::BOOL, RPCArg::Default{false}, "Pass true if you need in legacy scheme"},
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
        RPCResults{},
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
// clang-format off
static const CRPCCommand commands[] =
{ //  category              actor (function)
  //  --------------------- -----------------------
    { "evo",                &protx_list,                       },
    { "evo",                &protx_info,                       },
    { "evo",                &protx_register,                   },
    { "evo",                &protx_register_evo,               },
    { "evo",                &protx_register_fund,              },
    { "evo",                &protx_register_fund_evo,          },
    { "evo",                &protx_register_prepare,           },
    { "evo",                &protx_register_prepare_evo,       },
    { "evo",                &protx_update_service,             },
    { "evo",                &protx_update_service_evo,         },
    { "evo",                &protx_register_submit,            },
    { "evo",                &protx_update_registrar,           },
    { "evo",                &protx_revoke,                     },
    { "hidden",             &protx_register_legacy,            },
    { "hidden",             &protx_register_fund_legacy,       },
    { "hidden",             &protx_register_prepare_legacy,    },
    { "hidden",             &protx_update_registrar_legacy,    },
};
// clang-format on
    return commands;
}
#endif // ENABLE_WALLET

void RegisterEvoRPCCommands(CRPCTable& tableRPC)
{
// clang-format off
static const CRPCCommand commands[] =
{ //  category              actor (function)
  //  --------------------- -----------------------
    { "evo",                &bls_help,                         },
    { "evo",                &bls_generate,                     },
    { "evo",                &bls_fromsecret,                   },
    { "evo",                &protx_help,                       },
    { "evo",                &protx_diff,                       },
    { "evo",                &protx_listdiff,                   },
};
static const CRPCCommand commands_wallet[] =
{
    { "evo",                &protx_list,                       },
    { "evo",                &protx_info,                       },
};
// clang-format on
    for (const auto& command : commands) {
        tableRPC.appendCommand(command.name, &command);
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
            tableRPC.appendCommand(command.name, &command);
        }
    }
}
