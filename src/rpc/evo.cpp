// Copyright (c) 2018-2022 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <base58.h>
#include <bls/bls.h>
#include <chainparams.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <evo/deterministicmns.h>
#include <evo/dmn_types.h>
#include <evo/providertx.h>
#include <evo/simplifiedmns.h>
#include <evo/specialtx.h>
#include <evo/specialtxman.h>
#include <index/txindex.h>
#include <llmq/blockprocessor.h>
#include <llmq/context.h>
#include <llmq/utils.h>
#include <masternode/meta.h>
#include <messagesigner.h>
#include <netbase.h>
#include <rpc/blockchain.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <util/moneystr.h>
#include <util/translation.h>
#include <validation.h>

#ifdef ENABLE_WALLET
#include <wallet/coincontrol.h>
#include <wallet/rpcwallet.h>
#include <wallet/wallet.h>
#endif//ENABLE_WALLET

#ifdef ENABLE_WALLET
extern UniValue signrawtransaction(const JSONRPCRequest& request);
extern UniValue sendrawtransaction(const JSONRPCRequest& request);
#else
class CWallet;
#endif//ENABLE_WALLET

static RPCArg GetRpcArg(const std::string& strParamName)
{
    static const std::map<std::string, RPCArg> mapParamHelp = {
        {"collateralAddress",
            {"collateralAddress", RPCArg::Type::STR, RPCArg::Optional::NO,
                "The dash address to send the collateral to."}
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
            {"feeSourceAddress", RPCArg::Type::STR, /* default */ "",
                "If specified wallet will only use coins from this address to fund ProTx.\n"
                "If not specified, payoutAddress is the one that is going to be used.\n"
                "The private key belonging to this address must be known in your wallet."}
        },
        {"fundAddress",
            {"fundAddress", RPCArg::Type::STR, /* default */ "",
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
            {"operatorPayoutAddress", RPCArg::Type::STR, /* default */ "",
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
                "The dash address to use for payee updates and proposal voting.\n"
                "The corresponding private key does not have to be known by your wallet.\n"
                "The address must be unused and must differ from the collateralAddress."}
        },
        {"payoutAddress_register",
            {"payoutAddress", RPCArg::Type::STR, RPCArg::Optional::NO,
                "The dash address to use for masternode reward payments."}
        },
        {"payoutAddress_update",
            {"payoutAddress", RPCArg::Type::STR, RPCArg::Optional::NO,
                "The dash address to use for masternode reward payments.\n"
                "If set to an empty string, the currently active payout address is reused."}
        },
        {"proTxHash",
            {"proTxHash", RPCArg::Type::STR, RPCArg::Optional::NO,
                "The hash of the initial ProRegTx."}
        },
        {"reason",
            {"reason", RPCArg::Type::NUM, /* default */ "",
                "The reason for masternode service revocation."}
        },
        {"submit",
            {"submit", RPCArg::Type::BOOL, /* default */ "true",
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

static CBLSSecretKey ParseBLSSecretKey(const std::string& hexKey, const std::string& paramName, bool specific_legacy_bls_scheme)
{
    CBLSSecretKey secKey;

    if (!secKey.SetHexStr(hexKey, specific_legacy_bls_scheme)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s must be a valid BLS secret key", paramName));
    }
    return secKey;
}

static bool ValidatePlatformPort(const int32_t port)
{
    return port >= 1 && port <= std::numeric_limits<uint16_t>::max();
}

#ifdef ENABLE_WALLET

template<typename SpecialTxPayload>
static void FundSpecialTx(CWallet* pwallet, CMutableTransaction& tx, const SpecialTxPayload& payload, const CTxDestination& fundDest)
{
    CHECK_NONFATAL(pwallet != nullptr);

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();

    LOCK(pwallet->cs_wallet);

    CTxDestination nodest = CNoDestination();
    if (fundDest == nodest) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "No source of funds specified");
    }

    CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
    ds << payload;
    tx.vExtraPayload.assign(ds.begin(), ds.end());

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
    pwallet->AvailableCoins(vecOutputs);

    for (const auto& out : vecOutputs) {
        CTxDestination txDest;
        if (ExtractDestination(out.tx->tx->vout[out.i].scriptPubKey, txDest) && txDest == fundDest) {
            coinControl.Select(COutPoint(out.tx->tx->GetHash(), out.i));
        }
    }

    if (!coinControl.HasSelected()) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("No funds at specified address %s", EncodeDestination(fundDest)));
    }

    CTransactionRef newTx;
    CAmount nFee;
    int nChangePos = -1;
    bilingual_str strFailReason;

    if (!pwallet->CreateTransaction(vecSend, newTx, nFee, nChangePos, strFailReason, coinControl, false, tx.vExtraPayload.size())) {
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
static void SignSpecialTxPayloadByHash(const CMutableTransaction& tx, SpecialTxPayload& payload, const CKey& key)
{
    UpdateSpecialTxInputsHash(tx, payload);
    payload.vchSig.clear();

    uint256 hash = ::SerializeHash(payload);
    if (!CHashSigner::SignHash(hash, key, payload.vchSig)) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "failed to sign special tx");
    }
}

template<typename SpecialTxPayload>
static void SignSpecialTxPayloadByString(const CMutableTransaction& tx, SpecialTxPayload& payload, const CKey& key)
{
    UpdateSpecialTxInputsHash(tx, payload);
    payload.vchSig.clear();

    std::string m = payload.MakeSignString();
    if (!CMessageSigner::SignMessage(m, payload.vchSig, key)) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "failed to sign special tx");
    }
}

template<typename SpecialTxPayload>
static void SignSpecialTxPayloadByHash(const CMutableTransaction& tx, SpecialTxPayload& payload, const CBLSSecretKey& key)
{
    UpdateSpecialTxInputsHash(tx, payload);

    uint256 hash = ::SerializeHash(payload);
    payload.sig = key.Sign(hash);
}

static std::string SignAndSendSpecialTx(const JSONRPCRequest& request, const ChainstateManager& chainman, const CMutableTransaction& tx, bool fSubmit = true)
{
    {
    LOCK(cs_main);

    TxValidationState state;
    if (!CheckSpecialTx(CTransaction(tx), chainman.ActiveChain().Tip(), chainman.ActiveChainstate().CoinsTip(), true, state)) {
        throw std::runtime_error(state.ToString());
    }
    } // cs_main

    CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
    ds << tx;

    JSONRPCRequest signRequest(request);
    signRequest.params.setArray();
    signRequest.params.push_back(HexStr(ds));
    UniValue signResult = signrawtransactionwithwallet(signRequest);

    if (!fSubmit) {
        return signResult["hex"].get_str();
    }

    JSONRPCRequest sendRequest(request);
    sendRequest.params.setArray();
    sendRequest.params.push_back(signResult["hex"].get_str());
    return sendrawtransaction(sendRequest).get_str();
}

static void protx_register_fund_help(const JSONRPCRequest& request, bool legacy)
{
    std::string rpc_name = legacy ? "register_fund_legacy" : "register_fund";
    std::string rpc_full_name = std::string("protx ").append(rpc_name);
    std::string pubkey_operator = legacy ? "\"0532646990082f4fd639f90387b1551f2c7c39d37392cb9055a06a7e85c1d23692db8f87f827886310bccc1e29db9aee\"" : "\"8532646990082f4fd639f90387b1551f2c7c39d37392cb9055a06a7e85c1d23692db8f87f827886310bccc1e29db9aee\"";
    std::string rpc_example = rpc_name.append(" \"" + EXAMPLE_ADDRESS[0] + "\" \"1.2.3.4:1234\" \"" + EXAMPLE_ADDRESS[1] + "\" ").append(pubkey_operator).append(" \"" + EXAMPLE_ADDRESS[1] + "\" 0 \"" + EXAMPLE_ADDRESS[0] + "\"");
    RPCHelpMan{rpc_full_name,
        "\nCreates, funds and sends a ProTx to the network. The resulting transaction will move 1000 Dash\n"
        "to the address specified by collateralAddress and will then function as the collateral of your\n"
        "masternode.\n"
        "A few of the limitations you see in the arguments are temporary and might be lifted after DIP3\n"
        "is fully deployed.\n"
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
    }.Check(request);
}

static void protx_register_help(const JSONRPCRequest& request, bool legacy)
{
    std::string rpc_name = legacy ? "register_legacy" : "register";
    std::string rpc_full_name = std::string("protx ").append(rpc_name);
    std::string pubkey_operator = legacy ? "\"0532646990082f4fd639f90387b1551f2c7c39d37392cb9055a06a7e85c1d23692db8f87f827886310bccc1e29db9aee\"" : "\"8532646990082f4fd639f90387b1551f2c7c39d37392cb9055a06a7e85c1d23692db8f87f827886310bccc1e29db9aee\"";
    std::string rpc_example = rpc_name.append(" \"0123456701234567012345670123456701234567012345670123456701234567\" 0 \"1.2.3.4:1234\" \"" + EXAMPLE_ADDRESS[1] + "\" ").append(pubkey_operator).append(" \"" + EXAMPLE_ADDRESS[1] + "\" 0 \"" + EXAMPLE_ADDRESS[0] + "\"");
    RPCHelpMan{rpc_full_name,
        "\nSame as \"protx register_fund\", but with an externally referenced collateral.\n"
        "The collateral is specified through \"collateralHash\" and \"collateralIndex\" and must be an unspent\n"
        "transaction output spendable by this wallet. It must also not be used by any other masternode.\n"
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
            HelpExampleCli("protx", rpc_example)
        },
    }.Check(request);
}

static void protx_register_prepare_help(const JSONRPCRequest& request, bool legacy)
{
    std::string rpc_name = legacy ? "register_prepare_legacy" : "register_prepare";
    std::string rpc_full_name = std::string("protx ").append(rpc_name);
    std::string pubkey_operator = legacy ? "\"0532646990082f4fd639f90387b1551f2c7c39d37392cb9055a06a7e85c1d23692db8f87f827886310bccc1e29db9aee\"" : "\"8532646990082f4fd639f90387b1551f2c7c39d37392cb9055a06a7e85c1d23692db8f87f827886310bccc1e29db9aee\"";
    std::string rpc_example = rpc_name.append(" \"0123456701234567012345670123456701234567012345670123456701234567\" 0 \"1.2.3.4:1234\" \"" + EXAMPLE_ADDRESS[1] + "\" ").append(pubkey_operator).append(" \"" + EXAMPLE_ADDRESS[1] + "\" 0 \"" + EXAMPLE_ADDRESS[0] + "\"");
    RPCHelpMan{rpc_full_name,
        "\nCreates an unsigned ProTx and a message that must be signed externally\n"
        "with the private key that corresponds to collateralAddress to prove collateral ownership.\n"
        "The prepared transaction will also contain inputs and outputs to cover fees.\n",
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
    }.Check(request);
}

static void protx_register_submit_help(const JSONRPCRequest& request)
{
    RPCHelpMan{"protx register_submit",
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
    }.Check(request);
}

static void protx_register_fund_evo_help(const JSONRPCRequest& request)
{
    RPCHelpMan{
        "protx register_fund_evo",
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
    }.Check(request);
}

static void protx_register_evo_help(const JSONRPCRequest& request)
{
    RPCHelpMan{
        "protx register_evo",
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
    }.Check(request);
}

static void protx_register_prepare_evo_help(const JSONRPCRequest& request)
{
    RPCHelpMan{
        "protx register_prepare_evo",
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
    }.Check(request);
}

static UniValue protx_register_common_wrapper(const JSONRPCRequest& request,
                                              const ChainstateManager& chainman,
                                              const bool specific_legacy_bls_scheme,
                                              const bool isExternalRegister,
                                              const bool isFundRegister,
                                              const bool isPrepareRegister,
                                              const MnType mnType)
{
    const bool isEvoRequested = mnType == MnType::Evo;
    if (isEvoRequested) {
        if (isFundRegister && (request.fHelp || (request.params.size() < 10 || request.params.size() > 12))) {
            protx_register_fund_evo_help(request);
        } else if (isExternalRegister && (request.fHelp || (request.params.size() < 11 || request.params.size() > 13))) {
            protx_register_evo_help(request);
        } else if (isPrepareRegister && (request.fHelp || (request.params.size() != 11 && request.params.size() != 12))) {
            protx_register_prepare_evo_help(request);
        }
    } else {
        if (isFundRegister && (request.fHelp || (request.params.size() < 7 || request.params.size() > 9))) {
            protx_register_fund_help(request, specific_legacy_bls_scheme);
        } else if (isExternalRegister && (request.fHelp || (request.params.size() < 8 || request.params.size() > 10))) {
            protx_register_help(request, specific_legacy_bls_scheme);
        } else if (isPrepareRegister && (request.fHelp || (request.params.size() != 8 && request.params.size() != 9))) {
            protx_register_prepare_help(request, specific_legacy_bls_scheme);
        }
    }

    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;

    if (isExternalRegister || isFundRegister) {
        EnsureWalletIsUnlocked(wallet.get());
    }

    bool isV19active = llmq::utils::IsV19Active(WITH_LOCK(cs_main, return chainman.ActiveChain().Tip();));
    if (isEvoRequested && !isV19active) {
        throw JSONRPCError(RPC_INVALID_REQUEST, "EvoNodes aren't allowed yet");
    }

    size_t paramIdx = 0;

    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_PROVIDER_REGISTER;

    const bool use_legacy = isV19active ? specific_legacy_bls_scheme : true;

    CProRegTx ptx;
    ptx.nType = mnType;

    if (isFundRegister) {
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

        // TODO unlock on failure
        LOCK(wallet->cs_wallet);
        wallet->LockCoin(ptx.collateralOutpoint);
    }

    if (request.params[paramIdx].get_str() != "") {
        if (!Lookup(request.params[paramIdx].get_str().c_str(), ptx.addr, Params().GetDefaultPort(), false)) {
            throw std::runtime_error(strprintf("invalid network address %s", request.params[paramIdx].get_str()));
        }
    }

    ptx.keyIDOwner = ParsePubKeyIDFromAddress(request.params[paramIdx + 1].get_str(), "owner address");
    ptx.pubKeyOperator.Set(ParseBLSPubKey(request.params[paramIdx + 2].get_str(), "operator BLS address", use_legacy), use_legacy);
    ptx.nVersion = use_legacy ? CProRegTx::LEGACY_BLS_VERSION : CProRegTx::BASIC_BLS_VERSION;
    CHECK_NONFATAL(ptx.pubKeyOperator.IsLegacy() == (ptx.nVersion == CProRegTx::LEGACY_BLS_VERSION));

    CKeyID keyIDVoting = ptx.keyIDOwner;

    if (request.params[paramIdx + 3].get_str() != "") {
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

    if (!isFundRegister) {
        // make sure fee calculation works
        ptx.vchSig.resize(65);
    }

    CTxDestination fundDest = payoutDest;
    if (!request.params[paramIdx + 6].isNull()) {
        fundDest = DecodeDestination(request.params[paramIdx + 6].get_str());
        if (!IsValidDestination(fundDest))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Dash address: ") + request.params[paramIdx + 6].get_str());
    }

    FundSpecialTx(wallet.get(), tx, ptx, fundDest);
    UpdateSpecialTxInputsHash(tx, ptx);

    bool fSubmit{true};
    if ((isExternalRegister || isFundRegister) && !request.params[paramIdx + 7].isNull()) {
        fSubmit = ParseBoolV(request.params[paramIdx + 7], "submit");
    }

    if (isFundRegister) {
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
        return SignAndSendSpecialTx(request, chainman, tx, fSubmit);
    } else {
        // referencing external collateral

        Coin coin;
        if (!GetUTXOCoin(ptx.collateralOutpoint, coin)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("collateral not found: %s", ptx.collateralOutpoint.ToStringShort()));
        }
        CTxDestination txDest;
        ExtractDestination(coin.out.scriptPubKey, txDest);
        const PKHash *pkhash = std::get_if<PKHash>(&txDest);
        if (!pkhash) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("collateral type not supported: %s", ptx.collateralOutpoint.ToStringShort()));
        }

        if (isPrepareRegister) {
            // external signing with collateral key
            ptx.vchSig.clear();
            SetTxPayload(tx, ptx);

            UniValue ret(UniValue::VOBJ);
            ret.pushKV("tx", EncodeHexTx(CTransaction(tx)));
            ret.pushKV("collateralAddress", EncodeDestination(txDest));
            ret.pushKV("signMessage", ptx.MakeSignString());
            return ret;
        } else {
            // lets prove we own the collateral
            LegacyScriptPubKeyMan* spk_man = wallet->GetLegacyScriptPubKeyMan();
            if (!spk_man) {
                throw JSONRPCError(RPC_WALLET_ERROR, "This type of wallet does not support this command");
            }

            CKey key;
            if (!spk_man->GetKey(ToKeyID(*pkhash), key)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("collateral key not in wallet: %s", EncodeDestination(txDest)));
            }
            SignSpecialTxPayloadByString(tx, ptx, key);
            SetTxPayload(tx, ptx);
            return SignAndSendSpecialTx(request, chainman, tx, fSubmit);
        }
    }
}

static UniValue protx_register_evo(const JSONRPCRequest& request, const ChainstateManager& chainman)
{
    bool isExternalRegister = request.strMethod == "protxregister_evo";
    bool isFundRegister = request.strMethod == "protxregister_fund_evo";
    bool isPrepareRegister = request.strMethod == "protxregister_prepare_evo";
    if (request.strMethod.find("_hpmn") != std::string::npos) {
        if (!IsDeprecatedRPCEnabled("hpmn")) {
            throw JSONRPCError(RPC_METHOD_DEPRECATED, "*_hpmn methods are deprecated. Use the related *_evo methods or set -deprecatedrpc=hpmn to enable them");
        }
        isExternalRegister = request.strMethod == "protxregister_hpmn";
        isFundRegister = request.strMethod == "protxregister_fund_hpmn";
        isPrepareRegister = request.strMethod == "protxregister_prepare_hpmn";
    }
    return protx_register_common_wrapper(request, chainman, false, isExternalRegister, isFundRegister, isPrepareRegister, MnType::Evo);
}

static UniValue protx_register(const JSONRPCRequest& request, const ChainstateManager& chainman)
{
    bool isExternalRegister = request.strMethod == "protxregister";
    bool isFundRegister = request.strMethod == "protxregister_fund";
    bool isPrepareRegister = request.strMethod == "protxregister_prepare";
    return protx_register_common_wrapper(request, chainman, false, isExternalRegister, isFundRegister, isPrepareRegister, MnType::Regular);
}

static UniValue protx_register_legacy(const JSONRPCRequest& request, const ChainstateManager& chainman)
{
    bool isExternalRegister = request.strMethod == "protxregister_legacy";
    bool isFundRegister = request.strMethod == "protxregister_fund_legacy";
    bool isPrepareRegister = request.strMethod == "protxregister_prepare_legacy";
    return protx_register_common_wrapper(request, chainman, true, isExternalRegister, isFundRegister, isPrepareRegister, MnType::Regular);
}

static UniValue protx_register_submit(const JSONRPCRequest& request, const ChainstateManager& chainman)
{
    protx_register_submit_help(request);

    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;

    EnsureWalletIsUnlocked(wallet.get());

    CMutableTransaction tx;
    if (!DecodeHexTx(tx, request.params[0].get_str())) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "transaction not deserializable");
    }
    if (tx.nType != TRANSACTION_PROVIDER_REGISTER) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "transaction not a ProRegTx");
    }
    CProRegTx ptx;
    if (!GetTxPayload(tx, ptx)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "transaction payload not deserializable");
    }
    if (!ptx.vchSig.empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "payload signature not empty");
    }

    ptx.vchSig = DecodeBase64(request.params[1].get_str().c_str());

    SetTxPayload(tx, ptx);
    return SignAndSendSpecialTx(request, chainman, tx);
}

static void protx_update_service_help(const JSONRPCRequest& request)
{
    RPCHelpMan{"protx update_service",
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
    }.Check(request);
}

static void protx_update_service_evo_help(const JSONRPCRequest& request)
{
    RPCHelpMan{
        "protx update_service_evo",
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
    }.Check(request);
}

static UniValue protx_update_service_common_wrapper(const JSONRPCRequest& request, const ChainstateManager& chainman, const MnType mnType)
{
    if (request.strMethod.find("_hpmn") != std::string::npos) {
        if (!IsDeprecatedRPCEnabled("hpmn")) {
            throw JSONRPCError(RPC_METHOD_DEPRECATED, "*_hpmn methods are deprecated. Use the related *_evo methods or set -deprecatedrpc=hpmn to enable them");
        }
    }

    const bool isEvoRequested = mnType == MnType::Evo;
    if (isEvoRequested) {
        protx_update_service_evo_help(request);
    } else {
        protx_update_service_help(request);
    }

    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;

    EnsureWalletIsUnlocked(wallet.get());

    const bool isV19active = llmq::utils::IsV19Active(WITH_LOCK(cs_main, return chainman.ActiveChain().Tip();));
    const bool is_bls_legacy = !isV19active;
    if (isEvoRequested && !isV19active) {
        throw JSONRPCError(RPC_INVALID_REQUEST, "EvoNodes aren't allowed yet");
    }

    CProUpServTx ptx;
    ptx.nType = mnType;
    ptx.proTxHash = ParseHashV(request.params[0], "proTxHash");

    if (!Lookup(request.params[1].get_str().c_str(), ptx.addr, Params().GetDefaultPort(), false)) {
        throw std::runtime_error(strprintf("invalid network address %s", request.params[1].get_str()));
    }

    CBLSSecretKey keyOperator = ParseBLSSecretKey(request.params[2].get_str(), "operatorKey", is_bls_legacy);

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

    auto dmn = deterministicMNManager->GetListAtChainTip().GetMN(ptx.proTxHash);
    if (!dmn) {
        throw std::runtime_error(strprintf("masternode with proTxHash %s not found", ptx.proTxHash.ToString()));
    }
    if (dmn->nType != mnType) {
        throw std::runtime_error(strprintf("masternode with proTxHash %s is not a %s", ptx.proTxHash.ToString(), GetMnType(mnType).description));
    }
    ptx.nVersion = dmn->pdmnState->nVersion;

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

    FundSpecialTx(wallet.get(), tx, ptx, feeSource);

    SignSpecialTxPayloadByHash(tx, ptx, keyOperator);
    SetTxPayload(tx, ptx);

    return SignAndSendSpecialTx(request, chainman, tx);
}

static void protx_update_registrar_help(const JSONRPCRequest& request, bool legacy)
{
    std::string rpc_name = legacy ? "update_registrar_legacy" : "update_registrar";
    std::string rpc_full_name = std::string("protx ").append(rpc_name);
    std::string pubkey_operator = legacy ? "\"0532646990082f4fd639f90387b1551f2c7c39d37392cb9055a06a7e85c1d23692db8f87f827886310bccc1e29db9aee\"" : "\"8532646990082f4fd639f90387b1551f2c7c39d37392cb9055a06a7e85c1d23692db8f87f827886310bccc1e29db9aee\"";
    std::string rpc_example = rpc_name.append(" \"0123456701234567012345670123456701234567012345670123456701234567\" ").append(pubkey_operator).append(" \"" + EXAMPLE_ADDRESS[1] + "\"");
    RPCHelpMan{rpc_full_name,
        "\nCreates and sends a ProUpRegTx to the network. This will update the operator key, voting key and payout\n"
        "address of the masternode specified by \"proTxHash\".\n"
        "The owner key of the masternode must be known to your wallet.\n"
        + HELP_REQUIRING_PASSPHRASE,
        {
            GetRpcArg("proTxHash"),
            legacy ? GetRpcArg("operatorPubKey_update_legacy") : GetRpcArg("operatorPubKey_update"),
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
    }.Check(request);
}

static UniValue protx_update_registrar_wrapper(const JSONRPCRequest& request, const ChainstateManager& chainman, const bool specific_legacy_bls_scheme)
{
    protx_update_registrar_help(request, specific_legacy_bls_scheme);

    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;

    EnsureWalletIsUnlocked(wallet.get());

    CProUpRegTx ptx;
    ptx.proTxHash = ParseHashV(request.params[0], "proTxHash");

    auto dmn = deterministicMNManager->GetListAtChainTip().GetMN(ptx.proTxHash);
    if (!dmn) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("masternode %s not found", ptx.proTxHash.ToString()));
    }
    ptx.keyIDVoting = dmn->pdmnState->keyIDVoting;
    ptx.scriptPayout = dmn->pdmnState->scriptPayout;

    const bool use_legacy = llmq::utils::IsV19Active(chainman.ActiveChain().Tip()) ? specific_legacy_bls_scheme : true;

    if (request.params[1].get_str() != "") {
        // new pubkey
        ptx.pubKeyOperator.Set(ParseBLSPubKey(request.params[1].get_str(), "operator BLS address", use_legacy), use_legacy);
    } else {
        // same pubkey, reuse as is
        ptx.pubKeyOperator = dmn->pdmnState->pubKeyOperator;
    }

    ptx.nVersion = use_legacy ? CProUpRegTx::LEGACY_BLS_VERSION : CProUpRegTx::BASIC_BLS_VERSION;
    CHECK_NONFATAL(ptx.pubKeyOperator.IsLegacy() == (ptx.nVersion == CProUpRegTx::LEGACY_BLS_VERSION));

    if (request.params[2].get_str() != "") {
        ptx.keyIDVoting = ParsePubKeyIDFromAddress(request.params[2].get_str(), "voting address");
    }

    CTxDestination payoutDest;
    ExtractDestination(ptx.scriptPayout, payoutDest);
    if (request.params[3].get_str() != "") {
        payoutDest = DecodeDestination(request.params[3].get_str());
        if (!IsValidDestination(payoutDest)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("invalid payout address: %s", request.params[3].get_str()));
        }
        ptx.scriptPayout = GetScriptForDestination(payoutDest);
    }

    LegacyScriptPubKeyMan* spk_man = wallet->GetLegacyScriptPubKeyMan();
    if (!spk_man) {
        throw JSONRPCError(RPC_WALLET_ERROR, "This type of wallet does not support this command");
    }

    CKey keyOwner;
    if (!spk_man->GetKey(dmn->pdmnState->keyIDOwner, keyOwner)) {
        throw std::runtime_error(strprintf("Private key for owner address %s not found in your wallet", EncodeDestination(PKHash(dmn->pdmnState->keyIDOwner))));
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

    FundSpecialTx(wallet.get(), tx, ptx, feeSourceDest);
    SignSpecialTxPayloadByHash(tx, ptx, keyOwner);
    SetTxPayload(tx, ptx);

    return SignAndSendSpecialTx(request, chainman, tx);
}

static UniValue protx_update_registrar(const JSONRPCRequest& request, const ChainstateManager& chainman)
{
    return protx_update_registrar_wrapper(request, chainman, false);
}

static UniValue protx_update_registrar_legacy(const JSONRPCRequest& request, const ChainstateManager& chainman)
{
    return protx_update_registrar_wrapper(request, chainman, true);
}

static void protx_revoke_help(const JSONRPCRequest& request)
{
    RPCHelpMan{"protx revoke",
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
    }.Check(request);
}

static UniValue protx_revoke(const JSONRPCRequest& request, const ChainstateManager& chainman)
{
    protx_revoke_help(request);

    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;

    EnsureWalletIsUnlocked(wallet.get());

    const bool isV19active = llmq::utils::IsV19Active(WITH_LOCK(cs_main, return chainman.ActiveChain().Tip();));
    const bool is_bls_legacy = !isV19active;
    CProUpRevTx ptx;
    ptx.nVersion = CProUpRevTx::GetVersion(isV19active);
    ptx.proTxHash = ParseHashV(request.params[0], "proTxHash");

    CBLSSecretKey keyOperator = ParseBLSSecretKey(request.params[1].get_str(), "operatorKey", is_bls_legacy);

    if (!request.params[2].isNull()) {
        int32_t nReason = ParseInt32V(request.params[2], "reason");
        if (nReason < 0 || nReason > CProUpRevTx::REASON_LAST) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("invalid reason %d, must be between 0 and %d", nReason, CProUpRevTx::REASON_LAST));
        }
        ptx.nReason = (uint16_t)nReason;
    }

    auto dmn = deterministicMNManager->GetListAtChainTip().GetMN(ptx.proTxHash);
    if (!dmn) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("masternode %s not found", ptx.proTxHash.ToString()));
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
        FundSpecialTx(wallet.get(), tx, ptx, feeSourceDest);
    } else if (dmn->pdmnState->scriptOperatorPayout != CScript()) {
        // Using funds from previousely specified operator payout address
        CTxDestination txDest;
        ExtractDestination(dmn->pdmnState->scriptOperatorPayout, txDest);
        FundSpecialTx(wallet.get(), tx, ptx, txDest);
    } else if (dmn->pdmnState->scriptPayout != CScript()) {
        // Using funds from previousely specified masternode payout address
        CTxDestination txDest;
        ExtractDestination(dmn->pdmnState->scriptPayout, txDest);
        FundSpecialTx(wallet.get(), tx, ptx, txDest);
    } else {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "No payout or fee source addresses found, can't revoke");
    }

    SignSpecialTxPayloadByHash(tx, ptx, keyOperator);
    SetTxPayload(tx, ptx);

    return SignAndSendSpecialTx(request, chainman, tx);
}

#endif//ENABLE_WALLET

static void protx_list_help(const JSONRPCRequest& request)
{
    RPCHelpMan{"protx list",
        "\nLists all ProTxs in your wallet or on-chain, depending on the given type.\n",
        {
            {"type", RPCArg::Type::STR, /* default */ "registered",
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
            {"detailed", RPCArg::Type::BOOL, /* default */ "false", "If not specified, only the hashes of the ProTx will be returned."},
            {"height", RPCArg::Type::NUM, /* default */ "current chain-tip", ""},
        },
        RPCResults{},
        RPCExamples{""},
    }.Check(request);
}

#ifdef ENABLE_WALLET
static bool CheckWalletOwnsKey(CWallet* pwallet, const CKeyID& keyID) {
    if (!pwallet) {
        return false;
    }
    LegacyScriptPubKeyMan* spk_man = pwallet->GetLegacyScriptPubKeyMan();
    if (!spk_man) {
        return false;
    }
    return spk_man->HaveKey(keyID);
}

static bool CheckWalletOwnsScript(CWallet* pwallet, const CScript& script) {
    if (!pwallet) {
        return false;
    }
    LegacyScriptPubKeyMan* spk_man = pwallet->GetLegacyScriptPubKeyMan();
    if (!spk_man) {
        return false;
    }

    CTxDestination dest;
    if (ExtractDestination(script, dest)) {
        if ((std::get_if<PKHash>(&dest) && spk_man->HaveKey(ToKeyID(*std::get_if<PKHash>(&dest)))) || (std::get_if<ScriptHash>(&dest) && spk_man->HaveCScript(CScriptID{ScriptHash(*std::get_if<ScriptHash>(&dest))}))) {
            return true;
        }
    }
    return false;
}
#endif

static UniValue BuildDMNListEntry(CWallet* pwallet, const CDeterministicMN& dmn, bool detailed)
{
    if (!detailed) {
        return dmn.proTxHash.ToString();
    }

    UniValue o(UniValue::VOBJ);

    dmn.ToJson(o);

    int confirmations = GetUTXOConfirmations(dmn.collateralOutpoint);
    o.pushKV("confirmations", confirmations);

#ifdef ENABLE_WALLET
    bool hasOwnerKey = CheckWalletOwnsKey(pwallet, dmn.pdmnState->keyIDOwner);
    bool hasVotingKey = CheckWalletOwnsKey(pwallet, dmn.pdmnState->keyIDVoting);

    bool ownsCollateral = false;
    uint256 tmpHashBlock;
    CTransactionRef collateralTx = GetTransaction(/* block_index */ nullptr,  /* mempool */ nullptr, dmn.collateralOutpoint.hash, Params().GetConsensus(), tmpHashBlock);
    if (collateralTx) {
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

    auto metaInfo = mmetaman.GetMetaInfo(dmn.proTxHash);
    o.pushKV("metaInfo", metaInfo->ToJson());

    return o;
}

static UniValue protx_list(const JSONRPCRequest& request, const ChainstateManager& chainman)
{
    protx_list_help(request);

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
        LOCK2(wallet->cs_wallet, cs_main);

        if (request.params.size() > 4) {
            protx_list_help(request);
        }

        bool detailed = !request.params[1].isNull() ? ParseBoolV(request.params[1], "detailed") : false;

        int height = !request.params[2].isNull() ? ParseInt32V(request.params[2], "height") : chainman.ActiveChain().Height();
        if (height < 1 || height > chainman.ActiveChain().Height()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid height specified");
        }

        std::vector<COutPoint> vOutpts;
        wallet->ListProTxCoins(vOutpts);
        std::set<COutPoint> setOutpts;
        for (const auto& outpt : vOutpts) {
            setOutpts.emplace(outpt);
        }

        CDeterministicMNList mnList = deterministicMNManager->GetListForBlock(chainman.ActiveChain()[height]);
        mnList.ForEachMN(false, [&](const auto& dmn) {
            if (setOutpts.count(dmn.collateralOutpoint) ||
                CheckWalletOwnsKey(wallet.get(), dmn.pdmnState->keyIDOwner) ||
                CheckWalletOwnsKey(wallet.get(), dmn.pdmnState->keyIDVoting) ||
                CheckWalletOwnsScript(wallet.get(), dmn.pdmnState->scriptPayout) ||
                CheckWalletOwnsScript(wallet.get(), dmn.pdmnState->scriptOperatorPayout)) {
                ret.push_back(BuildDMNListEntry(wallet.get(), dmn, detailed));
            }
        });
#endif
    } else if (type == "valid" || type == "registered" || type == "evo") {
        if (request.params.size() > 3) {
            protx_list_help(request);
        }

        LOCK(cs_main);

        bool detailed = !request.params[1].isNull() ? ParseBoolV(request.params[1], "detailed") : false;

        int height = !request.params[2].isNull() ? ParseInt32V(request.params[2], "height") : chainman.ActiveChain().Height();
        if (height < 1 || height > chainman.ActiveChain().Height()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid height specified");
        }

        CDeterministicMNList mnList = deterministicMNManager->GetListForBlock(chainman.ActiveChain()[height]);
        bool onlyValid = type == "valid";
        bool onlyEvoNodes = type == "evo";
        mnList.ForEachMN(onlyValid, [&](const auto& dmn) {
            if (onlyEvoNodes && dmn.nType != MnType::Evo) return;
            ret.push_back(BuildDMNListEntry(wallet.get(), dmn, detailed));
        });
    } else {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid type specified");
    }

    return ret;
}

static void protx_info_help(const JSONRPCRequest& request)
{
    RPCHelpMan{"protx info",
        "\nReturns detailed information about a deterministic masternode.\n",
        {
            GetRpcArg("proTxHash"),
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
    }.Check(request);
}

static UniValue protx_info(const JSONRPCRequest& request)
{
    protx_info_help(request);

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

    uint256 proTxHash(ParseHashV(request.params[0], "proTxHash"));
    auto mnList = deterministicMNManager->GetListAtChainTip();
    auto dmn = mnList.GetMN(proTxHash);
    if (!dmn) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s not found", proTxHash.ToString()));
    }
    return BuildDMNListEntry(wallet.get(), *dmn, true);
}

static void protx_diff_help(const JSONRPCRequest& request)
{
    RPCHelpMan{"protx diff",
        "\nCalculates a diff between two deterministic masternode lists. The result also contains proof data.\n",
        {
            {"baseBlock", RPCArg::Type::NUM, RPCArg::Optional::NO, "The starting block height."},
            {"block", RPCArg::Type::NUM, RPCArg::Optional::NO, "The ending block height."},
            {"extended", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED, "Show additional fields."},
        },
        RPCResults{},
        RPCExamples{""},
    }.Check(request);
}

static uint256 ParseBlock(const UniValue& v, const ChainstateManager& chainman, std::string strName) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
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

static UniValue protx_diff(const JSONRPCRequest& request, const ChainstateManager& chainman)
{
    protx_diff_help(request);

    LOCK(cs_main);
    uint256 baseBlockHash = ParseBlock(request.params[0], chainman, "baseBlock");
    uint256 blockHash = ParseBlock(request.params[1], chainman, "block");
    bool extended = false;
    if (!request.params[2].isNull()) {
        extended = ParseBoolV(request.params[2], "extended");
    }

    CSimplifiedMNListDiff mnListDiff;
    std::string strError;
    const LLMQContext& llmq_ctx = EnsureAnyLLMQContext(request.context);

    if (!BuildSimplifiedMNListDiff(baseBlockHash, blockHash, mnListDiff, *llmq_ctx.quorum_block_processor, strError, extended)) {
        throw std::runtime_error(strError);
    }

    UniValue ret;
    mnListDiff.ToJson(ret, extended);
    return ret;
}

static void protx_listdiff_help(const JSONRPCRequest& request)
{
    RPCHelpMan{"protx listdiff",
               "\nCalculate a full MN list diff between two masternode lists.\n",
               {
                       {"baseBlock", RPCArg::Type::NUM, RPCArg::Optional::NO, "The starting block height."},
                       {"block", RPCArg::Type::NUM, RPCArg::Optional::NO, "The ending block height."},
               },
               RPCResults{},
               RPCExamples{""},
    }.Check(request);
}

static const CBlockIndex* ParseBlockIndex(const UniValue& v, const ChainstateManager& chainman, std::string strName) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
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

static UniValue protx_listdiff(const JSONRPCRequest& request, const ChainstateManager& chainman)
{
    protx_listdiff_help(request);

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

    auto baseBlockMNList = deterministicMNManager->GetListForBlock(pBaseBlockIndex);
    auto blockMNList = deterministicMNManager->GetListForBlock(pTargetBlockIndex);

    auto mnDiff = baseBlockMNList.BuildDiff(blockMNList);

    UniValue jaddedMNs(UniValue::VARR);
    for(const auto& mn : mnDiff.addedMNs) {
        UniValue obj;
        mn->ToJson(obj);
        jaddedMNs.push_back(obj);
    }
    ret.pushKV("addedMNs", jaddedMNs);

    UniValue jremovedMNs(UniValue::VARR);
    for(const auto& internal_id : mnDiff.removedMns) {
        auto dmn = baseBlockMNList.GetMNByInternalId(internal_id);
        jremovedMNs.push_back(dmn->proTxHash.ToString());
    }
    ret.pushKV("removedMNs", jremovedMNs);

    UniValue jupdatedMNs(UniValue::VARR);
    for(const auto& [internal_id, stateDiff] : mnDiff.updatedMNs) {
        auto dmn = baseBlockMNList.GetMNByInternalId(internal_id);
        UniValue s(UniValue::VOBJ);
        stateDiff.ToJson(s, dmn->nType);
        UniValue obj(UniValue::VOBJ);
        obj.pushKV(dmn->proTxHash.ToString(), s);
        jupdatedMNs.push_back(obj);
    }
    ret.pushKV("updatedMNs", jupdatedMNs);

    return ret;
}

[[ noreturn ]] static void protx_help()
{
    RPCHelpMan{
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
        "  register_legacy          - Create a ProTx by parsing BLS using the legacy scheme and send it to network\n"
        "  register_fund_legacy     - Fund and create a ProTx by parsing BLS using the legacy scheme, then send it to network\n"
        "  register_prepare_legacy  - Create an unsigned ProTx by parsing BLS using the legacy scheme\n"
        "  register_submit          - Sign and submit a ProTx\n"
#endif
        "  list                     - List ProTxs\n"
        "  info                     - Return information about a ProTx\n"
#ifdef ENABLE_WALLET
        "  update_service           - Create and send ProUpServTx to network\n"
        "  update_service_evo       - Create and send ProUpServTx to network for an EvoNode\n"
        "  update_registrar         - Create and send ProUpRegTx to network\n"
        "  update_registrar_legacy  - Create ProUpRegTx by parsing BLS using the legacy scheme, then send it to network\n"
        "  revoke                   - Create and send ProUpRevTx to network\n"
#endif
        "  diff                     - Calculate a diff and a proof between two masternode lists\n"
        "  listdiff                 - Calculate a full MN list diff between two masternode lists\n",
        {
            {"command", RPCArg::Type::STR, RPCArg::Optional::NO, "The command to execute"},
        },
        RPCResults{},
        RPCExamples{""},
    }.Throw();
}

static UniValue protx(const JSONRPCRequest& request)
{
    const JSONRPCRequest new_request{request.strMethod == "protx" ? request.squashed() : request};
    const std::string command{new_request.strMethod};

    const ChainstateManager& chainman = EnsureAnyChainman(request.context);

#ifdef ENABLE_WALLET
    if (command == "protxregister" || command == "protxregister_fund" || command == "protxregister_prepare") {
        return protx_register(new_request, chainman);
    } else if (command == "protxregister_evo" || command == "protxregister_fund_evo" || command == "protxregister_prepare_evo" || command == "protxregister_hpmn" || command == "protxregister_fund_hpmn" || command == "protxregister_prepare_hpmn") {
        return protx_register_evo(new_request, chainman);
    } else if (command == "protxregister_legacy" || command == "protxregister_fund_legacy" || command == "protxregister_prepare_legacy") {
        return protx_register_legacy(new_request, chainman);
    } else if (command == "protxregister_submit") {
        return protx_register_submit(new_request, chainman);
    } else if (command == "protxupdate_service") {
        return protx_update_service_common_wrapper(new_request, chainman, MnType::Regular);
    } else if (command == "protxupdate_service_evo" || command == "protxupdate_service_hpmn") {
        return protx_update_service_common_wrapper(new_request, chainman, MnType::Evo);
    } else if (command == "protxupdate_registrar") {
        return protx_update_registrar(new_request, chainman);
    } else if (command == "protxupdate_registrar_legacy") {
        return protx_update_registrar_legacy(new_request, chainman);
    } else if (command == "protxrevoke") {
        return protx_revoke(new_request, chainman);
    } else
#endif
    if (command == "protxlist") {
        return protx_list(new_request, chainman);
    } else if (command == "protxinfo") {
        return protx_info(new_request);
    } else if (command == "protxdiff") {
        return protx_diff(new_request, chainman);
    } else if (command == "protxlistdiff") {
        return protx_listdiff(new_request, chainman);
    }
    else {
        protx_help();
    }
}

static void bls_generate_help(const JSONRPCRequest& request)
{
    RPCHelpMan{"bls generate",
        "\nReturns a BLS secret/public key pair.\n",
        {
            {"legacy", RPCArg::Type::BOOL, /* default */ "true until the v19 fork is activated, otherwise false", "Use legacy BLS scheme"},
            },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "secret", "BLS secret key"},
                {RPCResult::Type::STR_HEX, "public", "BLS public key"},
                {RPCResult::Type::STR_HEX, "scheme", "BLS scheme (valid schemes: legacy, basic)"}
            }},
        RPCExamples{
            HelpExampleCli("bls generate", "")
        },
    }.Check(request);
}

static UniValue bls_generate(const JSONRPCRequest& request, const ChainstateManager& chainman)
{
    bls_generate_help(request);

    CBLSSecretKey sk;
    sk.MakeNewKey();
    bool bls_legacy_scheme = !llmq::utils::IsV19Active(chainman.ActiveChain().Tip());
    if (!request.params[0].isNull()) {
        bls_legacy_scheme = ParseBoolV(request.params[0], "bls_legacy_scheme");
    }
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("secret", sk.ToString());
    ret.pushKV("public", sk.GetPublicKey().ToString(bls_legacy_scheme));
    std::string bls_scheme_str = bls_legacy_scheme ? "legacy" : "basic";
    ret.pushKV("scheme", bls_scheme_str);
    return ret;
}

static void bls_fromsecret_help(const JSONRPCRequest& request)
{
    RPCHelpMan{"bls fromsecret",
        "\nParses a BLS secret key and returns the secret/public key pair.\n",
        {
            {"secret", RPCArg::Type::STR, RPCArg::Optional::NO, "The BLS secret key"},
            {"legacy", RPCArg::Type::BOOL, /* default */ "true until the v19 fork is activated, otherwise false", "Use legacy BLS scheme"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "secret", "BLS secret key"},
                {RPCResult::Type::STR_HEX, "public", "BLS public key"},
                {RPCResult::Type::STR_HEX, "scheme", "BLS scheme (valid schemes: legacy, basic)"},
            }},
        RPCExamples{
            HelpExampleCli("bls fromsecret", "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f")
        },
    }.Check(request);
}

static UniValue bls_fromsecret(const JSONRPCRequest& request, const ChainstateManager& chainman)
{
    bls_fromsecret_help(request);

    bool bls_legacy_scheme = !llmq::utils::IsV19Active(chainman.ActiveChain().Tip());
    if (!request.params[1].isNull()) {
        bls_legacy_scheme = ParseBoolV(request.params[1], "bls_legacy_scheme");
    }
    CBLSSecretKey sk = ParseBLSSecretKey(request.params[0].get_str(), "secretKey", bls_legacy_scheme);
    UniValue ret(UniValue::VOBJ);
    ret.pushKV("secret", sk.ToString());
    ret.pushKV("public", sk.GetPublicKey().ToString(bls_legacy_scheme));
    std::string bls_scheme_str = bls_legacy_scheme ? "legacy" : "basic";
    ret.pushKV("scheme", bls_scheme_str);
    return ret;
}

[[ noreturn ]] static void bls_help()
{
    RPCHelpMan{"bls",
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
    }.Throw();
}

static UniValue _bls(const JSONRPCRequest& request)
{
    const JSONRPCRequest new_request{request.strMethod == "bls" ? request.squashed() : request};
    const std::string command{new_request.strMethod};

    const ChainstateManager& chainman = EnsureAnyChainman(request.context);

    if (command == "blsgenerate") {
        return bls_generate(new_request, chainman);
    } else if (command == "blsfromsecret") {
        return bls_fromsecret(new_request, chainman);
    } else {
        bls_help();
    }
}
void RegisterEvoRPCCommands(CRPCTable &tableRPC)
{
// clang-format off
static const CRPCCommand commands[] =
{ //  category              name                      actor (function)
  //  --------------------- ------------------------  -----------------------
    { "evo",                "bls",                    &_bls,                   {}  },
    { "evo",                "protx",                  &protx,                  {}  },
};
// clang-format on
    for (const auto& command : commands) {
        tableRPC.appendCommand(command.name, &command);
    }
}
