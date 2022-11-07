// Copyright (c) 2018-2022 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <base58.h>
#include <bls/bls.h>
#include <chainparams.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <evo/deterministicmns.h>
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
#include <rpc/blockchain.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <util/moneystr.h>
#include <util/validation.h>
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
                "IP and port in the form \"IP:PORT\".\n"
                "Must be unique on the network. Can be set to 0, which will require a ProUpServTx afterwards."}
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
            {"operatorPubKey_register", RPCArg::Type::STR, RPCArg::Optional::NO,
                "The operator BLS public key. The BLS private key does not have to be known.\n"
                "It has to match the BLS private key which is later used when operating the masternode."}
        },
        {"operatorPubKey_update",
            {"operatorPubKey_update", RPCArg::Type::STR, RPCArg::Optional::NO,
                "The operator BLS public key. The BLS private key does not have to be known.\n"
                "It has to match the BLS private key which is later used when operating the masternode.\n"
                "If set to an empty string, the currently active operator BLS public key is reused."}
        },
        {"operatorReward",
            {"operatorReward", RPCArg::Type::STR, RPCArg::Optional::NO,
                "The fraction in %% to share with the operator. The value must be\n"
                "between 0.00 and 100.00."}
        },
        {"ownerAddress",
            {"ownerAddress", RPCArg::Type::STR, RPCArg::Optional::NO,
                "The dash address to use for payee updates and proposal voting.\n"
                "The corresponding private key does not have to be known by your wallet.\n"
                "The address must be unused and must differ from the collateralAddress."}
        },
        {"payoutAddress_register",
            {"payoutAddress_register", RPCArg::Type::STR, RPCArg::Optional::NO,
                "The dash address to use for masternode reward payments."}
        },
        {"payoutAddress_update",
            {"payoutAddress_update", RPCArg::Type::STR, RPCArg::Optional::NO,
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
            {"votingAddress_register", RPCArg::Type::STR, RPCArg::Optional::NO,
                "The voting key address. The private key does not have to be known by your wallet.\n"
                "It has to match the private key which is later used when voting on proposals.\n"
                "If set to an empty string, ownerAddress will be used."}
        },
        {"votingAddress_update",
            {"votingAddress_update", RPCArg::Type::STR, RPCArg::Optional::NO,
                "The voting key address. The private key does not have to be known by your wallet.\n"
                "It has to match the private key which is later used when voting on proposals.\n"
                "If set to an empty string, the currently active voting key address is reused."}
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
    const CKeyID *keyID = std::get_if<CKeyID>(&dest);
    if (!keyID) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s must be a valid P2PKH address, not %s", paramName, strAddress));
    }
    return *keyID;
}

static CBLSPublicKey ParseBLSPubKey(const std::string& hexKey, const std::string& paramName)
{
    CBLSPublicKey pubKey;
    if (!pubKey.SetHexStr(hexKey)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s must be a valid BLS public key, not %s", paramName, hexKey));
    }
    return pubKey;
}

static CBLSSecretKey ParseBLSSecretKey(const std::string& hexKey, const std::string& paramName)
{
    CBLSSecretKey secKey;
    if (!secKey.SetHexStr(hexKey)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s must be a valid BLS secret key", paramName));
    }
    return secKey;
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

static std::string SignAndSendSpecialTx(const JSONRPCRequest& request, const CMutableTransaction& tx, bool fSubmit = true)
{
    {
    LOCK(cs_main);

    CValidationState state;
    if (!CheckSpecialTx(CTransaction(tx), ::ChainActive().Tip(), state, ::ChainstateActive().CoinsTip(), true)) {
        throw std::runtime_error(FormatStateMessage(state));
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

static void protx_register_fund_help(const JSONRPCRequest& request)
{
    RPCHelpMan{"protx register_fund",
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
            GetRpcArg("operatorPubKey_register"),
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
            HelpExampleCli("protx", "register_fund \"XrVhS9LogauRJGJu2sHuryjhpuex4RNPSb\" \"1.2.3.4:1234\" \"Xt9AMWaYSz7tR7Uo7gzXA3m4QmeWgrR3rr\" \"93746e8731c57f87f79b3620a7982924e2931717d49540a85864bd543de11c43fb868fd63e501a1db37e19ed59ae6db4\" \"Xt9AMWaYSz7tR7Uo7gzXA3m4QmeWgrR3rr\" 0 \"XrVhS9LogauRJGJu2sHuryjhpuex4RNPSb\"")
        },
    }.Check(request);
}

static void protx_register_help(const JSONRPCRequest& request)
{
    RPCHelpMan{"protx register",
        "\nSame as \"protx register_fund\", but with an externally referenced collateral.\n"
        "The collateral is specified through \"collateralHash\" and \"collateralIndex\" and must be an unspent\n"
        "transaction output spendable by this wallet. It must also not be used by any other masternode.\n"
        + HELP_REQUIRING_PASSPHRASE,
        {
            GetRpcArg("collateralHash"),
            GetRpcArg("collateralIndex"),
            GetRpcArg("ipAndPort"),
            GetRpcArg("ownerAddress"),
            GetRpcArg("operatorPubKey_register"),
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
            HelpExampleCli("protx", "register \"0123456701234567012345670123456701234567012345670123456701234567\" 0 \"1.2.3.4:1234\" \"Xt9AMWaYSz7tR7Uo7gzXA3m4QmeWgrR3rr\" \"93746e8731c57f87f79b3620a7982924e2931717d49540a85864bd543de11c43fb868fd63e501a1db37e19ed59ae6db4\" \"Xt9AMWaYSz7tR7Uo7gzXA3m4QmeWgrR3rr\" 0 \"XrVhS9LogauRJGJu2sHuryjhpuex4RNPSb\"")
        },
    }.Check(request);
}

static void protx_register_prepare_help(const JSONRPCRequest& request)
{
    RPCHelpMan{"protx register_prepare",
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
            HelpExampleCli("protx", "register_prepare \"0123456701234567012345670123456701234567012345670123456701234567\" 0 \"1.2.3.4:1234\" \"Xt9AMWaYSz7tR7Uo7gzXA3m4QmeWgrR3rr\" \"93746e8731c57f87f79b3620a7982924e2931717d49540a85864bd543de11c43fb868fd63e501a1db37e19ed59ae6db4\" \"Xt9AMWaYSz7tR7Uo7gzXA3m4QmeWgrR3rr\" 0 \"XrVhS9LogauRJGJu2sHuryjhpuex4RNPSb\"")
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

// handles register, register_prepare and register_fund in one method
static UniValue protx_register(const JSONRPCRequest& request)
{
    bool isExternalRegister = request.strMethod == "protxregister";
    bool isFundRegister = request.strMethod == "protxregister_fund";
    bool isPrepareRegister = request.strMethod == "protxregister_prepare";

    if (isFundRegister && (request.fHelp || (request.params.size() < 7 || request.params.size() > 9))) {
        protx_register_fund_help(request);
    } else if (isExternalRegister && (request.fHelp || (request.params.size() < 8 || request.params.size() > 10))) {
        protx_register_help(request);
    } else if (isPrepareRegister && (request.fHelp || (request.params.size() != 8 && request.params.size() != 9))) {
        protx_register_prepare_help(request);
    }

    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;

    if (isExternalRegister || isFundRegister) {
        EnsureWalletIsUnlocked(wallet.get());
    }

    size_t paramIdx = 0;

    CAmount collateralAmount = 1000 * COIN;

    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_PROVIDER_REGISTER;

    CProRegTx ptx;
    ptx.nVersion = CProRegTx::CURRENT_VERSION;

    if (isFundRegister) {
        CTxDestination collateralDest = DecodeDestination(request.params[paramIdx].get_str());
        if (!IsValidDestination(collateralDest)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("invalid collaterall address: %s", request.params[paramIdx].get_str()));
        }
        CScript collateralScript = GetScriptForDestination(collateralDest);

        CTxOut collateralTxOut(collateralAmount, collateralScript);
        tx.vout.emplace_back(collateralTxOut);

        paramIdx++;
    } else {
        uint256 collateralHash = ParseHashV(request.params[paramIdx], "collateralHash");
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
    CBLSPublicKey pubKeyOperator = ParseBLSPubKey(request.params[paramIdx + 2].get_str(), "operator BLS address");
    CKeyID keyIDVoting = ptx.keyIDOwner;

    if (request.params[paramIdx + 3].get_str() != "") {
        keyIDVoting = ParsePubKeyIDFromAddress(request.params[paramIdx + 3].get_str(), "voting address");
    }

    int64_t operatorReward;
    if (!ParseFixedPoint(request.params[paramIdx + 4].getValStr(), 2, &operatorReward)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "operatorReward must be a number");
    }
    if (operatorReward < 0 || operatorReward > 10000) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "operatorReward must be between 0.00 and 100.00");
    }
    ptx.nOperatorReward = operatorReward;

    CTxDestination payoutDest = DecodeDestination(request.params[paramIdx + 5].get_str());
    if (!IsValidDestination(payoutDest)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("invalid payout address: %s", request.params[paramIdx + 5].get_str()));
    }

    ptx.pubKeyOperator = pubKeyOperator;
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
        uint32_t collateralIndex = (uint32_t) -1;
        for (uint32_t i = 0; i < tx.vout.size(); i++) {
            if (tx.vout[i].nValue == collateralAmount) {
                collateralIndex = i;
                break;
            }
        }
        CHECK_NONFATAL(collateralIndex != (uint32_t) -1);
        ptx.collateralOutpoint.n = collateralIndex;

        SetTxPayload(tx, ptx);
        return SignAndSendSpecialTx(request, tx, fSubmit);
    } else {
        // referencing external collateral

        Coin coin;
        if (!GetUTXOCoin(ptx.collateralOutpoint, coin)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("collateral not found: %s", ptx.collateralOutpoint.ToStringShort()));
        }
        CTxDestination txDest;
        ExtractDestination(coin.out.scriptPubKey, txDest);
        const CKeyID *keyID = std::get_if<CKeyID>(&txDest);
        if (!keyID) {
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
            if (!spk_man->GetKey(*keyID, key)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("collateral key not in wallet: %s", EncodeDestination(txDest)));
            }
            SignSpecialTxPayloadByString(tx, ptx, key);
            SetTxPayload(tx, ptx);
            return SignAndSendSpecialTx(request, tx, fSubmit);
        }
    }
}

static UniValue protx_register_submit(const JSONRPCRequest& request)
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
    return SignAndSendSpecialTx(request, tx);
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
            GetRpcArg("ipAndPort"),
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

static UniValue protx_update_service(const JSONRPCRequest& request)
{
    protx_update_service_help(request);

    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;

    EnsureWalletIsUnlocked(wallet.get());

    CProUpServTx ptx;
    ptx.nVersion = CProUpServTx::CURRENT_VERSION;
    ptx.proTxHash = ParseHashV(request.params[0], "proTxHash");

    if (!Lookup(request.params[1].get_str().c_str(), ptx.addr, Params().GetDefaultPort(), false)) {
        throw std::runtime_error(strprintf("invalid network address %s", request.params[1].get_str()));
    }

    CBLSSecretKey keyOperator = ParseBLSSecretKey(request.params[2].get_str(), "operatorKey");

    auto dmn = deterministicMNManager->GetListAtChainTip().GetMN(ptx.proTxHash);
    if (!dmn) {
        throw std::runtime_error(strprintf("masternode with proTxHash %s not found", ptx.proTxHash.ToString()));
    }

    if (keyOperator.GetPublicKey() != dmn->pdmnState->pubKeyOperator.Get()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("the operator key does not belong to the registered public key"));
    }

    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_PROVIDER_UPDATE_SERVICE;

    // param operatorPayoutAddress
    if (!request.params[3].isNull()) {
        if (request.params[3].get_str().empty()) {
            ptx.scriptOperatorPayout = dmn->pdmnState->scriptOperatorPayout;
        } else {
            CTxDestination payoutDest = DecodeDestination(request.params[3].get_str());
            if (!IsValidDestination(payoutDest)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("invalid operator payout address: %s", request.params[3].get_str()));
            }
            ptx.scriptOperatorPayout = GetScriptForDestination(payoutDest);
        }
    } else {
        ptx.scriptOperatorPayout = dmn->pdmnState->scriptOperatorPayout;
    }

    CTxDestination feeSource;

    // param feeSourceAddress
    if (!request.params[4].isNull()) {
        feeSource = DecodeDestination(request.params[4].get_str());
        if (!IsValidDestination(feeSource))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Dash address: ") + request.params[4].get_str());
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

    return SignAndSendSpecialTx(request, tx);
}

static void protx_update_registrar_help(const JSONRPCRequest& request)
{
    RPCHelpMan{"protx update_registrar",
        "\nCreates and sends a ProUpRegTx to the network. This will update the operator key, voting key and payout\n"
        "address of the masternode specified by \"proTxHash\".\n"
        "The owner key of the masternode must be known to your wallet.\n"
        + HELP_REQUIRING_PASSPHRASE,
        {
            GetRpcArg("proTxHash"),
            GetRpcArg("operatorPubKey_update"),
            GetRpcArg("votingAddress_update"),
            GetRpcArg("payoutAddress_update"),
            GetRpcArg("feeSourceAddress"),
        },
        RPCResult{
            RPCResult::Type::STR_HEX, "txid", "The transaction id"
        },
        RPCExamples{
            HelpExampleCli("protx", "update_registrar \"0123456701234567012345670123456701234567012345670123456701234567\" \"982eb34b7c7f614f29e5c665bc3605f1beeef85e3395ca12d3be49d2868ecfea5566f11cedfad30c51b2403f2ad95b67\" \"XwnLY9Tf7Zsef8gMGL2fhWA9ZmMjt4KPwG\"")
        },
    }.Check(request);
}

static UniValue protx_update_registrar(const JSONRPCRequest& request)
{
    protx_update_registrar_help(request);

    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;

    EnsureWalletIsUnlocked(wallet.get());

    CProUpRegTx ptx;
    ptx.nVersion = CProUpRegTx::CURRENT_VERSION;
    ptx.proTxHash = ParseHashV(request.params[0], "proTxHash");

    auto dmn = deterministicMNManager->GetListAtChainTip().GetMN(ptx.proTxHash);
    if (!dmn) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("masternode %s not found", ptx.proTxHash.ToString()));
    }
    ptx.pubKeyOperator = dmn->pdmnState->pubKeyOperator.Get();
    ptx.keyIDVoting = dmn->pdmnState->keyIDVoting;
    ptx.scriptPayout = dmn->pdmnState->scriptPayout;

    if (request.params[1].get_str() != "") {
        ptx.pubKeyOperator = ParseBLSPubKey(request.params[1].get_str(), "operator BLS address");
    }
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
        throw std::runtime_error(strprintf("Private key for owner address %s not found in your wallet", EncodeDestination(dmn->pdmnState->keyIDOwner)));
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

    return SignAndSendSpecialTx(request, tx);
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

static UniValue protx_revoke(const JSONRPCRequest& request)
{
    protx_revoke_help(request);

    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;

    EnsureWalletIsUnlocked(wallet.get());

    CProUpRevTx ptx;
    ptx.nVersion = CProUpRevTx::CURRENT_VERSION;
    ptx.proTxHash = ParseHashV(request.params[0], "proTxHash");

    CBLSSecretKey keyOperator = ParseBLSSecretKey(request.params[1].get_str(), "operatorKey");

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

    return SignAndSendSpecialTx(request, tx);
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
        if ((std::get_if<CKeyID>(&dest) && spk_man->HaveKey(*std::get_if<CKeyID>(&dest))) || (std::get_if<CScriptID>(&dest) && spk_man->HaveCScript(*std::get_if<CScriptID>(&dest)))) {
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

static UniValue protx_list(const JSONRPCRequest& request)
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

        int height = !request.params[2].isNull() ? ParseInt32V(request.params[2], "height") : ::ChainActive().Height();
        if (height < 1 || height > ::ChainActive().Height()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid height specified");
        }

        std::vector<COutPoint> vOutpts;
        wallet->ListProTxCoins(vOutpts);
        std::set<COutPoint> setOutpts;
        for (const auto& outpt : vOutpts) {
            setOutpts.emplace(outpt);
        }

        CDeterministicMNList mnList = deterministicMNManager->GetListForBlock(::ChainActive()[height]);
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
    } else if (type == "valid" || type == "registered") {
        if (request.params.size() > 3) {
            protx_list_help(request);
        }

        LOCK(cs_main);

        bool detailed = !request.params[1].isNull() ? ParseBoolV(request.params[1], "detailed") : false;

        int height = !request.params[2].isNull() ? ParseInt32V(request.params[2], "height") : ::ChainActive().Height();
        if (height < 1 || height > ::ChainActive().Height()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid height specified");
        }

        CDeterministicMNList mnList = deterministicMNManager->GetListForBlock(::ChainActive()[height]);
        bool onlyValid = type == "valid";
        mnList.ForEachMN(onlyValid, [&](const auto& dmn) {
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

    uint256 proTxHash = ParseHashV(request.params[0], "proTxHash");
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

static uint256 ParseBlock(const UniValue& v, std::string strName) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);

    try {
        return ParseHashV(v, strName);
    } catch (...) {
        int h = ParseInt32V(v, strName);
        if (h < 1 || h > ::ChainActive().Height())
            throw std::runtime_error(strprintf("%s must be a block hash or chain height and not %s", strName, v.getValStr()));
        return *::ChainActive()[h]->phashBlock;
    }
}

static UniValue protx_diff(const JSONRPCRequest& request)
{
    protx_diff_help(request);

    LOCK(cs_main);
    uint256 baseBlockHash = ParseBlock(request.params[0], "baseBlock");
    uint256 blockHash = ParseBlock(request.params[1], "block");
    bool extended = false;
    if (!request.params[2].isNull()) {
        extended = ParseBoolV(request.params[2], "extended");
    }

    CSimplifiedMNListDiff mnListDiff;
    std::string strError;
    LLMQContext& llmq_ctx = EnsureLLMQContext(request.context);

    if (!BuildSimplifiedMNListDiff(baseBlockHash, blockHash, mnListDiff, *llmq_ctx.quorum_block_processor, strError, extended)) {
        throw std::runtime_error(strError);
    }

    UniValue ret;
    mnListDiff.ToJson(ret, extended);
    return ret;
}

[[ noreturn ]] static void protx_help()
{
    RPCHelpMan{"protx",
        "Set of commands to execute ProTx related actions.\n"
        "To get help on individual commands, use \"help protx command\".\n"
        "\nAvailable commands:\n"
#ifdef ENABLE_WALLET
        "  register          - Create and send ProTx to network\n"
        "  register_fund     - Fund, create and send ProTx to network\n"
        "  register_prepare  - Create an unsigned ProTx\n"
        "  register_submit   - Sign and submit a ProTx\n"
#endif
        "  list              - List ProTxs\n"
        "  info              - Return information about a ProTx\n"
#ifdef ENABLE_WALLET
        "  update_service    - Create and send ProUpServTx to network\n"
        "  update_registrar  - Create and send ProUpRegTx to network\n"
        "  revoke            - Create and send ProUpRevTx to network\n"
#endif
        "  diff              - Calculate a diff and a proof between two masternode lists\n",
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

#ifdef ENABLE_WALLET
    if (command == "protxregister" || command == "protxregister_fund" || command == "protxregister_prepare") {
        return protx_register(new_request);
    } else if (command == "protxregister_submit") {
        return protx_register_submit(new_request);
    } else if (command == "protxupdate_service") {
        return protx_update_service(new_request);
    } else if (command == "protxupdate_registrar") {
        return protx_update_registrar(new_request);
    } else if (command == "protxrevoke") {
        return protx_revoke(new_request);
    } else
#endif
    if (command == "protxlist") {
        return protx_list(new_request);
    } else if (command == "protxinfo") {
        return protx_info(new_request);
    } else if (command == "protxdiff") {
        return protx_diff(new_request);
    } else {
        protx_help();
    }
}

static void bls_generate_help(const JSONRPCRequest& request)
{
    RPCHelpMan{"bls generate",
        "\nReturns a BLS secret/public key pair.\n",
        {},
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "secret", "BLS secret key"},
                {RPCResult::Type::STR_HEX, "public", "BLS public key"},
            }},
        RPCExamples{
            HelpExampleCli("bls generate", "")
        },
    }.Check(request);
}

static UniValue bls_generate(const JSONRPCRequest& request)
{
    bls_generate_help(request);

    CBLSSecretKey sk;
    sk.MakeNewKey();

    UniValue ret(UniValue::VOBJ);
    ret.pushKV("secret", sk.ToString());
    ret.pushKV("public", sk.GetPublicKey().ToString());
    return ret;
}

static void bls_fromsecret_help(const JSONRPCRequest& request)
{
    RPCHelpMan{"bls fromsecret",
        "\nParses a BLS secret key and returns the secret/public key pair.\n",
        {
            {"secret", RPCArg::Type::STR, RPCArg::Optional::NO, "The BLS secret key"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "secret", "BLS secret key"},
                {RPCResult::Type::STR_HEX, "public", "BLS public key"},
            }},
        RPCExamples{
            HelpExampleCli("bls fromsecret", "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f")
        },
    }.Check(request);
}

static UniValue bls_fromsecret(const JSONRPCRequest& request)
{
    bls_fromsecret_help(request);

    CBLSSecretKey sk;
    if (!sk.SetHexStr(request.params[0].get_str())) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Secret key must be a valid hex string of length %d", sk.SerSize*2));
    }

    UniValue ret(UniValue::VOBJ);
    ret.pushKV("secret", sk.ToString());
    ret.pushKV("public", sk.GetPublicKey().ToString());
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

    if (command == "blsgenerate") {
        return bls_generate(new_request);
    } else if (command == "blsfromsecret") {
        return bls_fromsecret(new_request);
    } else {
        bls_help();
    }
}
// clang-format off
static const CRPCCommand commands[] =
{ //  category              name                      actor (function)
  //  --------------------- ------------------------  -----------------------
    { "evo",                "bls",                    &_bls,                   {}  },
    { "evo",                "protx",                  &protx,                  {}  },
};
// clang-format on
void RegisterEvoRPCCommands(CRPCTable &tableRPC)
{
    for (const auto& command : commands) {
        tableRPC.appendCommand(command.name, &command);
    }
}
