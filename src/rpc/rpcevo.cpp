// Copyright (c) 2018-2021 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <base58.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <index/txindex.h>
#include <init.h>
#include <messagesigner.h>
#include <rpc/server.h>
#include <txmempool.h>
#include <utilmoneystr.h>
#include <validation.h>

#ifdef ENABLE_WALLET
#include <wallet/coincontrol.h>
#include <wallet/wallet.h>
#include <wallet/rpcwallet.h>
#endif//ENABLE_WALLET

#include <netbase.h>

#include <evo/specialtx.h>
#include <evo/providertx.h>
#include <evo/deterministicmns.h>
#include <evo/simplifiedmns.h>

#include <bls/bls.h>

#include <masternode/masternode-meta.h>

#ifdef ENABLE_WALLET
extern UniValue signrawtransaction(const JSONRPCRequest& request);
extern UniValue sendrawtransaction(const JSONRPCRequest& request);
#else
class CWallet;
#endif//ENABLE_WALLET

static std::string GetHelpString(int nParamNum, std::string strParamName)
{
    static const std::map<std::string, std::string> mapParamHelp = {
        {"collateralAddress",
            "%d. \"collateralAddress\"        (string, required) The dash address to send the collateral to.\n"
        },
        {"collateralHash",
            "%d. \"collateralHash\"           (string, required) The collateral transaction hash.\n"
        },
        {"collateralIndex",
            "%d. collateralIndex            (numeric, required) The collateral transaction output index.\n"
        },
        {"feeSourceAddress",
            "%d. \"feeSourceAddress\"         (string, optional) If specified wallet will only use coins from this address to fund ProTx.\n"
            "                              If not specified, payoutAddress is the one that is going to be used.\n"
            "                              The private key belonging to this address must be known in your wallet.\n"
        },
        {"fundAddress",
            "%d. \"fundAddress\"              (string, optional) If specified wallet will only use coins from this address to fund ProTx.\n"
            "                              If not specified, payoutAddress is the one that is going to be used.\n"
            "                              The private key belonging to this address must be known in your wallet.\n"
        },
        {"ipAndPort",
            "%d. \"ipAndPort\"                (string, required) IP and port in the form \"IP:PORT\".\n"
            "                              Must be unique on the network. Can be set to 0, which will require a ProUpServTx afterwards.\n"
        },
        {"operatorKey",
            "%d. \"operatorKey\"              (string, required) The operator BLS private key associated with the\n"
            "                              registered operator public key.\n"
        },
        {"operatorPayoutAddress",
            "%d. \"operatorPayoutAddress\"    (string, optional) The address used for operator reward payments.\n"
            "                              Only allowed when the ProRegTx had a non-zero operatorReward value.\n"
            "                              If set to an empty string, the currently active payout address is reused.\n"
        },
        {"operatorPubKey_register",
            "%d. \"operatorPubKey\"           (string, required) The operator BLS public key. The BLS private key does not have to be known.\n"
            "                              It has to match the BLS private key which is later used when operating the masternode.\n"
        },
        {"operatorPubKey_update",
            "%d. \"operatorPubKey\"           (string, required) The operator BLS public key. The BLS private key does not have to be known.\n"
            "                              It has to match the BLS private key which is later used when operating the masternode.\n"
            "                              If set to an empty string, the currently active operator BLS public key is reused.\n"
        },
        {"operatorReward",
            "%d. \"operatorReward\"           (numeric, required) The fraction in %% to share with the operator. The value must be\n"
            "                              between 0.00 and 100.00.\n"
        },
        {"ownerAddress",
            "%d. \"ownerAddress\"             (string, required) The dash address to use for payee updates and proposal voting.\n"
            "                              The corresponding private key does not have to be known by your wallet.\n"
            "                              The address must be unused and must differ from the collateralAddress.\n"
        },
        {"payoutAddress_register",
            "%d. \"payoutAddress\"            (string, required) The dash address to use for masternode reward payments.\n"
        },
        {"payoutAddress_update",
            "%d. \"payoutAddress\"            (string, required) The dash address to use for masternode reward payments.\n"
            "                              If set to an empty string, the currently active payout address is reused.\n"
        },
        {"proTxHash",
            "%d. \"proTxHash\"                (string, required) The hash of the initial ProRegTx.\n"
        },
        {"reason",
            "%d. reason                     (numeric, optional) The reason for masternode service revocation.\n"
        },
        {"submit",
            "%d. submit                     (bool, optional, default=true) If true, the resulting transaction is sent to the network.\n"
        },
        {"votingAddress_register",
            "%d. \"votingAddress\"            (string, required) The voting key address. The private key does not have to be known by your wallet.\n"
            "                              It has to match the private key which is later used when voting on proposals.\n"
            "                              If set to an empty string, ownerAddress will be used.\n"
        },
        {"votingAddress_update",
            "%d. \"votingAddress\"            (string, required) The voting key address. The private key does not have to be known by your wallet.\n"
            "                              It has to match the private key which is later used when voting on proposals.\n"
            "                              If set to an empty string, the currently active voting key address is reused.\n"
        },
    };

    auto it = mapParamHelp.find(strParamName);
    if (it == mapParamHelp.end())
        throw std::runtime_error(strprintf("FIXME: WRONG PARAM NAME %s!", strParamName));

    return strprintf(it->second, nParamNum);
}

static CKeyID ParsePubKeyIDFromAddress(const std::string& strAddress, const std::string& paramName)
{
    CTxDestination dest = DecodeDestination(strAddress);
    const CKeyID *keyID = boost::get<CKeyID>(&dest);
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
    assert(pwallet != nullptr);

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();

    LOCK2(cs_main, mempool.cs);
    LOCK(pwallet->cs_wallet);

    CTxDestination nodest = CNoDestination();
    if (fundDest == nodest) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "No source of funds specified");
    }

    CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
    ds << payload;
    tx.vExtraPayload.assign(ds.begin(), ds.end());

    static CTxOut dummyTxOut(0, CScript() << OP_RETURN);
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
    CReserveKey reservekey(pwallet);
    CAmount nFee;
    int nChangePos = -1;
    std::string strFailReason;

    if (!pwallet->CreateTransaction(vecSend, newTx, reservekey, nFee, nChangePos, strFailReason, coinControl, false, tx.vExtraPayload.size())) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, strFailReason);
    }

    tx.vin = newTx->vin;
    tx.vout = newTx->vout;

    if (dummyTxOutAdded && tx.vout.size() > 1) {
        // CreateTransaction added a change output, so we don't need the dummy txout anymore.
        // Removing it results in slight overpayment of fees, but we ignore this for now (as it's a very low amount).
        auto it = std::find(tx.vout.begin(), tx.vout.end(), dummyTxOut);
        assert(it != tx.vout.end());
        tx.vout.erase(it);
    }
}

template<typename SpecialTxPayload>
static void UpdateSpecialTxInputsHash(const CMutableTransaction& tx, SpecialTxPayload& payload)
{
    payload.inputsHash = CalcTxInputsHash(tx);
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

static std::string SignAndSendSpecialTx(const CMutableTransaction& tx, bool fSubmit = true)
{
    {
    LOCK(cs_main);

    CValidationState state;
    if (!CheckSpecialTx(tx, chainActive.Tip(), state, *pcoinsTip.get())) {
        throw std::runtime_error(FormatStateMessage(state));
    }
    } // cs_main

    CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
    ds << tx;

    JSONRPCRequest signRequest;
    signRequest.params.setArray();
    signRequest.params.push_back(HexStr(ds));
    UniValue signResult = signrawtransactionwithwallet(signRequest);

    if (!fSubmit) {
        return signResult["hex"].get_str();
    }

    JSONRPCRequest sendRequest;
    sendRequest.params.setArray();
    sendRequest.params.push_back(signResult["hex"].get_str());
    return sendrawtransaction(sendRequest).get_str();
}

static void protx_register_fund_help(CWallet* const pwallet)
{
    throw std::runtime_error(
            "protx register_fund \"collateralAddress\" \"ipAndPort\" \"ownerAddress\" \"operatorPubKey\" \"votingAddress\" operatorReward \"payoutAddress\" ( \"fundAddress\" submit )\n"
            "\nCreates, funds and sends a ProTx to the network. The resulting transaction will move 1000 Dash\n"
            "to the address specified by collateralAddress and will then function as the collateral of your\n"
            "masternode.\n"
            "A few of the limitations you see in the arguments are temporary and might be lifted after DIP3\n"
            "is fully deployed.\n"
            + HelpRequiringPassphrase(pwallet) + "\n"
            "\nArguments:\n"
            + GetHelpString(1, "collateralAddress")
            + GetHelpString(2, "ipAndPort")
            + GetHelpString(3, "ownerAddress")
            + GetHelpString(4, "operatorPubKey_register")
            + GetHelpString(5, "votingAddress_register")
            + GetHelpString(6, "operatorReward")
            + GetHelpString(7, "payoutAddress_register")
            + GetHelpString(8, "fundAddress")
            + GetHelpString(9, "submit") +
            "\nResult (if \"submit\" is not set or set to true):\n"
            "\"txid\"                        (string) The transaction id.\n"
            "\nResult (if \"submit\" is set to false):\n"
            "\"hex\"                         (string) The serialized signed ProTx in hex format.\n"
            "\nExamples:\n"
            + HelpExampleCli("protx", "register_fund \"XrVhS9LogauRJGJu2sHuryjhpuex4RNPSb\" \"1.2.3.4:1234\" \"Xt9AMWaYSz7tR7Uo7gzXA3m4QmeWgrR3rr\" \"93746e8731c57f87f79b3620a7982924e2931717d49540a85864bd543de11c43fb868fd63e501a1db37e19ed59ae6db4\" \"Xt9AMWaYSz7tR7Uo7gzXA3m4QmeWgrR3rr\" 0 \"XrVhS9LogauRJGJu2sHuryjhpuex4RNPSb\"")
    );
}

static void protx_register_help(CWallet* const pwallet)
{
    throw std::runtime_error(
            "protx register \"collateralHash\" collateralIndex \"ipAndPort\" \"ownerAddress\" \"operatorPubKey\" \"votingAddress\" operatorReward \"payoutAddress\" ( \"feeSourceAddress\" submit )\n"
            "\nSame as \"protx register_fund\", but with an externally referenced collateral.\n"
            "The collateral is specified through \"collateralHash\" and \"collateralIndex\" and must be an unspent\n"
            "transaction output spendable by this wallet. It must also not be used by any other masternode.\n"
            + HelpRequiringPassphrase(pwallet) + "\n"
            "\nArguments:\n"
            + GetHelpString(1, "collateralHash")
            + GetHelpString(2, "collateralIndex")
            + GetHelpString(3, "ipAndPort")
            + GetHelpString(4, "ownerAddress")
            + GetHelpString(5, "operatorPubKey_register")
            + GetHelpString(6, "votingAddress_register")
            + GetHelpString(7, "operatorReward")
            + GetHelpString(8, "payoutAddress_register")
            + GetHelpString(9, "feeSourceAddress")
            + GetHelpString(10, "submit") +
            "\nResult (if \"submit\" is not set or set to true):\n"
            "\"txid\"                        (string) The transaction id.\n"
            "\nResult (if \"submit\" is set to false):\n"
            "\"hex\"                         (string) The serialized signed ProTx in hex format.\n"
            "\nExamples:\n"
            + HelpExampleCli("protx", "register \"0123456701234567012345670123456701234567012345670123456701234567\" 0 \"1.2.3.4:1234\" \"Xt9AMWaYSz7tR7Uo7gzXA3m4QmeWgrR3rr\" \"93746e8731c57f87f79b3620a7982924e2931717d49540a85864bd543de11c43fb868fd63e501a1db37e19ed59ae6db4\" \"Xt9AMWaYSz7tR7Uo7gzXA3m4QmeWgrR3rr\" 0 \"XrVhS9LogauRJGJu2sHuryjhpuex4RNPSb\"")
    );
}

static void protx_register_prepare_help()
{
    throw std::runtime_error(
            "protx register_prepare \"collateralHash\" collateralIndex \"ipAndPort\" \"ownerAddress\" \"operatorPubKey\" \"votingAddress\" operatorReward \"payoutAddress\" ( \"feeSourceAddress\" )\n"
            "\nCreates an unsigned ProTx and a message that must be signed externally\n"
            "with the private key that corresponds to collateralAddress to prove collateral ownership.\n"
            "The prepared transaction will also contain inputs and outputs to cover fees.\n"
            "\nArguments:\n"
            + GetHelpString(1, "collateralHash")
            + GetHelpString(2, "collateralIndex")
            + GetHelpString(3, "ipAndPort")
            + GetHelpString(4, "ownerAddress")
            + GetHelpString(5, "operatorPubKey_register")
            + GetHelpString(6, "votingAddress_register")
            + GetHelpString(7, "operatorReward")
            + GetHelpString(8, "payoutAddress_register")
            + GetHelpString(9, "feeSourceAddress") +
            "\nResult:\n"
            "{                             (json object)\n"
            "  \"tx\" :                      (string) The serialized unsigned ProTx in hex format.\n"
            "  \"collateralAddress\" :       (string) The collateral address.\n"
            "  \"signMessage\" :             (string) The string message that needs to be signed with\n"
            "                              the collateral key.\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("protx", "register_prepare \"0123456701234567012345670123456701234567012345670123456701234567\" 0 \"1.2.3.4:1234\" \"Xt9AMWaYSz7tR7Uo7gzXA3m4QmeWgrR3rr\" \"93746e8731c57f87f79b3620a7982924e2931717d49540a85864bd543de11c43fb868fd63e501a1db37e19ed59ae6db4\" \"Xt9AMWaYSz7tR7Uo7gzXA3m4QmeWgrR3rr\" 0 \"XrVhS9LogauRJGJu2sHuryjhpuex4RNPSb\"")
    );
}

static void protx_register_submit_help(CWallet* const pwallet)
{
    throw std::runtime_error(
            "protx register_submit \"tx\" \"sig\"\n"
            "\nCombines the unsigned ProTx and a signature of the signMessage, signs all inputs\n"
            "which were added to cover fees and submits the resulting transaction to the network.\n"
            "Note: See \"help protx register_prepare\" for more info about creating a ProTx and a message to sign.\n"
            + HelpRequiringPassphrase(pwallet) + "\n"
            "\nArguments:\n"
            "1. \"tx\"                 (string, required) The serialized unsigned ProTx in hex format.\n"
            "2. \"sig\"                (string, required) The signature signed with the collateral key. Must be in base64 format.\n"
            "\nResult:\n"
            "\"txid\"                  (string) The transaction id.\n"
            "\nExamples:\n"
            + HelpExampleCli("protx", "register_submit \"tx\" \"sig\"")
    );
}

// handles register, register_prepare and register_fund in one method
static UniValue protx_register(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    bool isExternalRegister = request.params[0].get_str() == "register";
    bool isFundRegister = request.params[0].get_str() == "register_fund";
    bool isPrepareRegister = request.params[0].get_str() == "register_prepare";

    if (isFundRegister && (request.fHelp || (request.params.size() < 8 || request.params.size() > 10))) {
        protx_register_fund_help(pwallet);
    } else if (isExternalRegister && (request.fHelp || (request.params.size() < 9 || request.params.size() > 11))) {
        protx_register_help(pwallet);
    } else if (isPrepareRegister && (request.fHelp || (request.params.size() != 9 && request.params.size() != 10))) {
        protx_register_prepare_help();
    }

    if (isExternalRegister || isFundRegister) {
        EnsureWalletIsUnlocked(pwallet);
    }

    size_t paramIdx = 1;

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
        LOCK(pwallet->cs_wallet);
        pwallet->LockCoin(ptx.collateralOutpoint);
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

    FundSpecialTx(pwallet, tx, ptx, fundDest);
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
        assert(collateralIndex != (uint32_t) -1);
        ptx.collateralOutpoint.n = collateralIndex;

        SetTxPayload(tx, ptx);
        return SignAndSendSpecialTx(tx, fSubmit);
    } else {
        // referencing external collateral

        Coin coin;
        if (!GetUTXOCoin(ptx.collateralOutpoint, coin)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("collateral not found: %s", ptx.collateralOutpoint.ToStringShort()));
        }
        CTxDestination txDest;
        ExtractDestination(coin.out.scriptPubKey, txDest);
        const CKeyID *keyID = boost::get<CKeyID>(&txDest);
        if (!keyID) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("collateral type not supported: %s", ptx.collateralOutpoint.ToStringShort()));
        }

        if (isPrepareRegister) {
            // external signing with collateral key
            ptx.vchSig.clear();
            SetTxPayload(tx, ptx);

            UniValue ret(UniValue::VOBJ);
            ret.pushKV("tx", EncodeHexTx(tx));
            ret.pushKV("collateralAddress", EncodeDestination(txDest));
            ret.pushKV("signMessage", ptx.MakeSignString());
            return ret;
        } else {
            // lets prove we own the collateral
            CKey key;
            if (!pwallet->GetKey(*keyID, key)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("collateral key not in wallet: %s", EncodeDestination(txDest)));
            }
            SignSpecialTxPayloadByString(tx, ptx, key);
            SetTxPayload(tx, ptx);
            return SignAndSendSpecialTx(tx, fSubmit);
        }
    }
}

static UniValue protx_register_submit(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() != 3) {
        protx_register_submit_help(pwallet);
    }

    EnsureWalletIsUnlocked(pwallet);

    CMutableTransaction tx;
    if (!DecodeHexTx(tx, request.params[1].get_str())) {
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

    ptx.vchSig = DecodeBase64(request.params[2].get_str().c_str());

    SetTxPayload(tx, ptx);
    return SignAndSendSpecialTx(tx);
}

static void protx_update_service_help(CWallet* const pwallet)
{
    throw std::runtime_error(
            "protx update_service \"proTxHash\" \"ipAndPort\" \"operatorKey\" (\"operatorPayoutAddress\" \"feeSourceAddress\" )\n"
            "\nCreates and sends a ProUpServTx to the network. This will update the IP address\n"
            "of a masternode.\n"
            "If this is done for a masternode that got PoSe-banned, the ProUpServTx will also revive this masternode.\n"
            + HelpRequiringPassphrase(pwallet) + "\n"
            "\nArguments:\n"
            + GetHelpString(1, "proTxHash")
            + GetHelpString(2, "ipAndPort")
            + GetHelpString(3, "operatorKey")
            + GetHelpString(4, "operatorPayoutAddress")
            + GetHelpString(5, "feeSourceAddress") +
            "\nResult:\n"
            "\"txid\"                        (string) The transaction id.\n"
            "\nExamples:\n"
            + HelpExampleCli("protx", "update_service \"0123456701234567012345670123456701234567012345670123456701234567\" \"1.2.3.4:1234\" 5a2e15982e62f1e0b7cf9783c64cf7e3af3f90a52d6c40f6f95d624c0b1621cd")
    );
}

static UniValue protx_update_service(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || (request.params.size() < 4 || request.params.size() > 6))
        protx_update_service_help(pwallet);

    EnsureWalletIsUnlocked(pwallet);

    CProUpServTx ptx;
    ptx.nVersion = CProUpServTx::CURRENT_VERSION;
    ptx.proTxHash = ParseHashV(request.params[1], "proTxHash");

    if (!Lookup(request.params[2].get_str().c_str(), ptx.addr, Params().GetDefaultPort(), false)) {
        throw std::runtime_error(strprintf("invalid network address %s", request.params[2].get_str()));
    }

    CBLSSecretKey keyOperator = ParseBLSSecretKey(request.params[3].get_str(), "operatorKey");

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
    if (!request.params[4].isNull()) {
        if (request.params[4].get_str().empty()) {
            ptx.scriptOperatorPayout = dmn->pdmnState->scriptOperatorPayout;
        } else {
            CTxDestination payoutDest = DecodeDestination(request.params[4].get_str());
            if (!IsValidDestination(payoutDest)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("invalid operator payout address: %s", request.params[4].get_str()));
            }
            ptx.scriptOperatorPayout = GetScriptForDestination(payoutDest);
        }
    } else {
        ptx.scriptOperatorPayout = dmn->pdmnState->scriptOperatorPayout;
    }

    CTxDestination feeSource;

    // param feeSourceAddress
    if (!request.params[5].isNull()) {
        feeSource = DecodeDestination(request.params[5].get_str());
        if (!IsValidDestination(feeSource))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Dash address: ") + request.params[5].get_str());
    } else {
        if (ptx.scriptOperatorPayout != CScript()) {
            // use operator reward address as default source for fees
            ExtractDestination(ptx.scriptOperatorPayout, feeSource);
        } else {
            // use payout address as default source for fees
            ExtractDestination(dmn->pdmnState->scriptPayout, feeSource);
        }
    }

    FundSpecialTx(pwallet, tx, ptx, feeSource);

    SignSpecialTxPayloadByHash(tx, ptx, keyOperator);
    SetTxPayload(tx, ptx);

    return SignAndSendSpecialTx(tx);
}

static void protx_update_registrar_help(CWallet* const pwallet)
{
    throw std::runtime_error(
            "protx update_registrar \"proTxHash\" \"operatorPubKey\" \"votingAddress\" \"payoutAddress\" ( \"feeSourceAddress\" )\n"
            "\nCreates and sends a ProUpRegTx to the network. This will update the operator key, voting key and payout\n"
            "address of the masternode specified by \"proTxHash\".\n"
            "The owner key of the masternode must be known to your wallet.\n"
            + HelpRequiringPassphrase(pwallet) + "\n"
            "\nArguments:\n"
            + GetHelpString(1, "proTxHash")
            + GetHelpString(2, "operatorPubKey_update")
            + GetHelpString(3, "votingAddress_update")
            + GetHelpString(4, "payoutAddress_update")
            + GetHelpString(5, "feeSourceAddress") +
            "\nResult:\n"
            "\"txid\"                        (string) The transaction id.\n"
            "\nExamples:\n"
            + HelpExampleCli("protx", "update_registrar \"0123456701234567012345670123456701234567012345670123456701234567\" \"982eb34b7c7f614f29e5c665bc3605f1beeef85e3395ca12d3be49d2868ecfea5566f11cedfad30c51b2403f2ad95b67\" \"XwnLY9Tf7Zsef8gMGL2fhWA9ZmMjt4KPwG\"")
    );
}

static UniValue protx_update_registrar(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || (request.params.size() != 5 && request.params.size() != 6)) {
        protx_update_registrar_help(pwallet);
    }

    EnsureWalletIsUnlocked(pwallet);

    CProUpRegTx ptx;
    ptx.nVersion = CProUpRegTx::CURRENT_VERSION;
    ptx.proTxHash = ParseHashV(request.params[1], "proTxHash");

    auto dmn = deterministicMNManager->GetListAtChainTip().GetMN(ptx.proTxHash);
    if (!dmn) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("masternode %s not found", ptx.proTxHash.ToString()));
    }
    ptx.pubKeyOperator = dmn->pdmnState->pubKeyOperator.Get();
    ptx.keyIDVoting = dmn->pdmnState->keyIDVoting;
    ptx.scriptPayout = dmn->pdmnState->scriptPayout;

    if (request.params[2].get_str() != "") {
        ptx.pubKeyOperator = ParseBLSPubKey(request.params[2].get_str(), "operator BLS address");
    }
    if (request.params[3].get_str() != "") {
        ptx.keyIDVoting = ParsePubKeyIDFromAddress(request.params[3].get_str(), "voting address");
    }

    CTxDestination payoutDest;
    ExtractDestination(ptx.scriptPayout, payoutDest);
    if (request.params[4].get_str() != "") {
        payoutDest = DecodeDestination(request.params[4].get_str());
        if (!IsValidDestination(payoutDest)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("invalid payout address: %s", request.params[4].get_str()));
        }
        ptx.scriptPayout = GetScriptForDestination(payoutDest);
    }

    CKey keyOwner;
    if (!pwallet->GetKey(dmn->pdmnState->keyIDOwner, keyOwner)) {
        throw std::runtime_error(strprintf("Private key for owner address %s not found in your wallet", EncodeDestination(dmn->pdmnState->keyIDOwner)));
    }

    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_PROVIDER_UPDATE_REGISTRAR;

    // make sure we get anough fees added
    ptx.vchSig.resize(65);

    CTxDestination feeSourceDest = payoutDest;
    if (!request.params[5].isNull()) {
        feeSourceDest = DecodeDestination(request.params[5].get_str());
        if (!IsValidDestination(feeSourceDest))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Dash address: ") + request.params[5].get_str());
    }

    FundSpecialTx(pwallet, tx, ptx, feeSourceDest);
    SignSpecialTxPayloadByHash(tx, ptx, keyOwner);
    SetTxPayload(tx, ptx);

    return SignAndSendSpecialTx(tx);
}

static void protx_revoke_help(CWallet* const pwallet)
{
    throw std::runtime_error(
            "protx revoke \"proTxHash\" \"operatorKey\" ( reason \"feeSourceAddress\")\n"
            "\nCreates and sends a ProUpRevTx to the network. This will revoke the operator key of the masternode and\n"
            "put it into the PoSe-banned state. It will also set the service field of the masternode\n"
            "to zero. Use this in case your operator key got compromised or you want to stop providing your service\n"
            "to the masternode owner.\n"
            + HelpRequiringPassphrase(pwallet) + "\n"
            "\nArguments:\n"
            + GetHelpString(1, "proTxHash")
            + GetHelpString(2, "operatorKey")
            + GetHelpString(3, "reason")
            + GetHelpString(4, "feeSourceAddress") +
            "\nResult:\n"
            "\"txid\"                        (string) The transaction id.\n"
            "\nExamples:\n"
            + HelpExampleCli("protx", "revoke \"0123456701234567012345670123456701234567012345670123456701234567\" \"072f36a77261cdd5d64c32d97bac417540eddca1d5612f416feb07ff75a8e240\"")
    );
}

static UniValue protx_revoke(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || (request.params.size() < 3 || request.params.size() > 5)) {
        protx_revoke_help(pwallet);
    }

    EnsureWalletIsUnlocked(pwallet);

    CProUpRevTx ptx;
    ptx.nVersion = CProUpRevTx::CURRENT_VERSION;
    ptx.proTxHash = ParseHashV(request.params[1], "proTxHash");

    CBLSSecretKey keyOperator = ParseBLSSecretKey(request.params[2].get_str(), "operatorKey");

    if (!request.params[3].isNull()) {
        int32_t nReason = ParseInt32V(request.params[3], "reason");
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

    if (!request.params[4].isNull()) {
        CTxDestination feeSourceDest = DecodeDestination(request.params[4].get_str());
        if (!IsValidDestination(feeSourceDest))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Dash address: ") + request.params[4].get_str());
        FundSpecialTx(pwallet, tx, ptx, feeSourceDest);
    } else if (dmn->pdmnState->scriptOperatorPayout != CScript()) {
        // Using funds from previousely specified operator payout address
        CTxDestination txDest;
        ExtractDestination(dmn->pdmnState->scriptOperatorPayout, txDest);
        FundSpecialTx(pwallet, tx, ptx, txDest);
    } else if (dmn->pdmnState->scriptPayout != CScript()) {
        // Using funds from previousely specified masternode payout address
        CTxDestination txDest;
        ExtractDestination(dmn->pdmnState->scriptPayout, txDest);
        FundSpecialTx(pwallet, tx, ptx, txDest);
    } else {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "No payout or fee source addresses found, can't revoke");
    }

    SignSpecialTxPayloadByHash(tx, ptx, keyOperator);
    SetTxPayload(tx, ptx);

    return SignAndSendSpecialTx(tx);
}
#endif//ENABLE_WALLET

static void protx_list_help()
{
    throw std::runtime_error(
            "protx list (\"type\" \"detailed\" \"height\")\n"
            "\nLists all ProTxs in your wallet or on-chain, depending on the given type.\n"
            "If \"type\" is not specified, it defaults to \"registered\".\n"
            "If \"detailed\" is not specified, it defaults to \"false\" and only the hashes of the ProTx will be returned.\n"
            "If \"height\" is not specified, it defaults to the current chain-tip.\n"
            "\nAvailable types:\n"
            "  registered   - List all ProTx which are registered at the given chain height.\n"
            "                 This will also include ProTx which failed PoSe verfication.\n"
            "  valid        - List only ProTx which are active/valid at the given chain height.\n"
#ifdef ENABLE_WALLET
            "  wallet       - List only ProTx which are found in your wallet at the given chain height.\n"
            "                 This will also include ProTx which failed PoSe verfication.\n"
#endif
    );
}

#ifdef ENABLE_WALLET
static bool CheckWalletOwnsKey(CWallet* pwallet, const CKeyID& keyID) {
    if (!pwallet) {
        return false;
    }
    return pwallet->HaveKey(keyID);
}

static bool CheckWalletOwnsScript(CWallet* pwallet, const CScript& script) {
    if (!pwallet) {
        return false;
    }

    CTxDestination dest;
    if (ExtractDestination(script, dest)) {
        if ((boost::get<CKeyID>(&dest) && pwallet->HaveKey(*boost::get<CKeyID>(&dest))) || (boost::get<CScriptID>(&dest) && pwallet->HaveCScript(*boost::get<CScriptID>(&dest)))) {
            return true;
        }
    }
    return false;
}
#endif

static UniValue BuildDMNListEntry(CWallet* pwallet, const CDeterministicMNCPtr& dmn, bool detailed)
{
    if (!detailed) {
        return dmn->proTxHash.ToString();
    }

    UniValue o(UniValue::VOBJ);

    dmn->ToJson(o);

    int confirmations = GetUTXOConfirmations(dmn->collateralOutpoint);
    o.pushKV("confirmations", confirmations);

#ifdef ENABLE_WALLET
    bool hasOwnerKey = CheckWalletOwnsKey(pwallet, dmn->pdmnState->keyIDOwner);
    bool hasOperatorKey = false; //CheckWalletOwnsKey(dmn->pdmnState->keyIDOperator);
    bool hasVotingKey = CheckWalletOwnsKey(pwallet, dmn->pdmnState->keyIDVoting);

    bool ownsCollateral = false;
    CTransactionRef collateralTx;
    uint256 tmpHashBlock;
    if (GetTransaction(dmn->collateralOutpoint.hash, collateralTx, Params().GetConsensus(), tmpHashBlock)) {
        ownsCollateral = CheckWalletOwnsScript(pwallet, collateralTx->vout[dmn->collateralOutpoint.n].scriptPubKey);
    }

    if (pwallet) {
        UniValue walletObj(UniValue::VOBJ);
        walletObj.pushKV("hasOwnerKey", hasOwnerKey);
        walletObj.pushKV("hasOperatorKey", hasOperatorKey);
        walletObj.pushKV("hasVotingKey", hasVotingKey);
        walletObj.pushKV("ownsCollateral", ownsCollateral);
        walletObj.pushKV("ownsPayeeScript", CheckWalletOwnsScript(pwallet, dmn->pdmnState->scriptPayout));
        walletObj.pushKV("ownsOperatorRewardScript", CheckWalletOwnsScript(pwallet, dmn->pdmnState->scriptOperatorPayout));
        o.pushKV("wallet", walletObj);
    }
#endif

    auto metaInfo = mmetaman.GetMetaInfo(dmn->proTxHash);
    o.pushKV("metaInfo", metaInfo->ToJson());

    return o;
}

static UniValue protx_list(const JSONRPCRequest& request)
{
    if (request.fHelp) {
        protx_list_help();
    }

#ifdef ENABLE_WALLET
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
#else
    CWallet* const pwallet = nullptr;
#endif

    std::string type = "registered";
    if (!request.params[1].isNull()) {
        type = request.params[1].get_str();
    }

    UniValue ret(UniValue::VARR);

    if (g_txindex) {
        g_txindex->BlockUntilSyncedToCurrentChain();
    }

    LOCK(cs_main);

    if (type == "wallet") {
        if (!pwallet) {
            throw std::runtime_error("\"protx list wallet\" not supported when wallet is disabled");
        }
#ifdef ENABLE_WALLET
        LOCK2(cs_main, pwallet->cs_wallet);

        if (request.params.size() > 4) {
            protx_list_help();
        }

        bool detailed = !request.params[2].isNull() ? ParseBoolV(request.params[2], "detailed") : false;

        int height = !request.params[3].isNull() ? ParseInt32V(request.params[3], "height") : chainActive.Height();
        if (height < 1 || height > chainActive.Height()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid height specified");
        }

        std::vector<COutPoint> vOutpts;
        pwallet->ListProTxCoins(vOutpts);
        std::set<COutPoint> setOutpts;
        for (const auto& outpt : vOutpts) {
            setOutpts.emplace(outpt);
        }

        CDeterministicMNList mnList = deterministicMNManager->GetListForBlock(chainActive[height]);
        mnList.ForEachMN(false, [&](const CDeterministicMNCPtr& dmn) {
            if (setOutpts.count(dmn->collateralOutpoint) ||
                CheckWalletOwnsKey(pwallet, dmn->pdmnState->keyIDOwner) ||
                CheckWalletOwnsKey(pwallet, dmn->pdmnState->keyIDVoting) ||
                CheckWalletOwnsScript(pwallet, dmn->pdmnState->scriptPayout) ||
                CheckWalletOwnsScript(pwallet, dmn->pdmnState->scriptOperatorPayout)) {
                ret.push_back(BuildDMNListEntry(pwallet, dmn, detailed));
            }
        });
#endif
    } else if (type == "valid" || type == "registered") {
        if (request.params.size() > 4) {
            protx_list_help();
        }

        LOCK(cs_main);

        bool detailed = !request.params[2].isNull() ? ParseBoolV(request.params[2], "detailed") : false;

        int height = !request.params[3].isNull() ? ParseInt32V(request.params[3], "height") : chainActive.Height();
        if (height < 1 || height > chainActive.Height()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid height specified");
        }

        CDeterministicMNList mnList = deterministicMNManager->GetListForBlock(chainActive[height]);
        bool onlyValid = type == "valid";
        mnList.ForEachMN(onlyValid, [&](const CDeterministicMNCPtr& dmn) {
            ret.push_back(BuildDMNListEntry(pwallet, dmn, detailed));
        });
    } else {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid type specified");
    }

    return ret;
}

static void protx_info_help()
{
    throw std::runtime_error(
            "protx info \"proTxHash\"\n"
            "\nReturns detailed information about a deterministic masternode.\n"
            "\nArguments:\n"
            + GetHelpString(1, "proTxHash") +
            "\nResult:\n"
            "{                             (json object) Details about a specific deterministic masternode\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("protx", "info \"0123456701234567012345670123456701234567012345670123456701234567\"")
    );
}

static UniValue protx_info(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 2) {
        protx_info_help();
    }

#ifdef ENABLE_WALLET
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
#else
    CWallet* const pwallet = nullptr;
#endif

    if (g_txindex) {
        g_txindex->BlockUntilSyncedToCurrentChain();
    }

    uint256 proTxHash = ParseHashV(request.params[1], "proTxHash");
    auto mnList = deterministicMNManager->GetListAtChainTip();
    auto dmn = mnList.GetMN(proTxHash);
    if (!dmn) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s not found", proTxHash.ToString()));
    }
    return BuildDMNListEntry(pwallet, dmn, true);
}

static void protx_diff_help()
{
    throw std::runtime_error(
            "protx diff \"baseBlock\" \"block\"\n"
            "\nCalculates a diff between two deterministic masternode lists. The result also contains proof data.\n"
            "\nArguments:\n"
            "1. \"baseBlock\"           (numeric, required) The starting block height.\n"
            "2. \"block\"               (numeric, required) The ending block height.\n"
    );
}

static uint256 ParseBlock(const UniValue& v, std::string strName)
{
    AssertLockHeld(cs_main);

    try {
        return ParseHashV(v, strName);
    } catch (...) {
        int h = ParseInt32V(v, strName);
        if (h < 1 || h > chainActive.Height())
            throw std::runtime_error(strprintf("%s must be a block hash or chain height and not %s", strName, v.getValStr()));
        return *chainActive[h]->phashBlock;
    }
}

static UniValue protx_diff(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 3) {
        protx_diff_help();
    }

    LOCK(cs_main);
    uint256 baseBlockHash = ParseBlock(request.params[1], "baseBlock");
    uint256 blockHash = ParseBlock(request.params[2], "block");

    CSimplifiedMNListDiff mnListDiff;
    std::string strError;
    if (!BuildSimplifiedMNListDiff(baseBlockHash, blockHash, mnListDiff, strError)) {
        throw std::runtime_error(strError);
    }

    UniValue ret;
    mnListDiff.ToJson(ret);
    return ret;
}

[[ noreturn ]] static void protx_help()
{
    throw std::runtime_error(
            "protx \"command\" ...\n"
            "Set of commands to execute ProTx related actions.\n"
            "To get help on individual commands, use \"help protx command\".\n"
            "\nArguments:\n"
            "1. \"command\"        (string, required) The command to execute\n"
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
            "  diff              - Calculate a diff and a proof between two masternode lists\n"
    );
}

static UniValue protx(const JSONRPCRequest& request)
{
    if (request.fHelp && request.params.empty()) {
        protx_help();
    }

    std::string command;
    if (!request.params[0].isNull()) {
        command = request.params[0].get_str();
    }

#ifdef ENABLE_WALLET
    if (command == "register" || command == "register_fund" || command == "register_prepare") {
        return protx_register(request);
    } else if (command == "register_submit") {
        return protx_register_submit(request);
    } else if (command == "update_service") {
        return protx_update_service(request);
    } else if (command == "update_registrar") {
        return protx_update_registrar(request);
    } else if (command == "revoke") {
        return protx_revoke(request);
    } else
#endif
    if (command == "list") {
        return protx_list(request);
    } else if (command == "info") {
        return protx_info(request);
    } else if (command == "diff") {
        return protx_diff(request);
    } else {
        protx_help();
    }
}

static void bls_generate_help()
{
    throw std::runtime_error(
            "bls generate\n"
            "\nReturns a BLS secret/public key pair.\n"
            "\nResult:\n"
            "{\n"
            "  \"secret\": \"xxxx\",        (string) BLS secret key\n"
            "  \"public\": \"xxxx\",        (string) BLS public key\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("bls generate", "")
    );
}

static UniValue bls_generate(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1) {
        bls_generate_help();
    }

    CBLSSecretKey sk;
    sk.MakeNewKey();

    UniValue ret(UniValue::VOBJ);
    ret.pushKV("secret", sk.ToString());
    ret.pushKV("public", sk.GetPublicKey().ToString());
    return ret;
}

static void bls_fromsecret_help()
{
    throw std::runtime_error(
            "bls fromsecret \"secret\"\n"
            "\nParses a BLS secret key and returns the secret/public key pair.\n"
            "\nArguments:\n"
            "1. \"secret\"                (string, required) The BLS secret key\n"
            "\nResult:\n"
            "{\n"
            "  \"secret\": \"xxxx\",        (string) BLS secret key\n"
            "  \"public\": \"xxxx\",        (string) BLS public key\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("bls fromsecret", "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f")
    );
}

static UniValue bls_fromsecret(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 2) {
        bls_fromsecret_help();
    }

    CBLSSecretKey sk;
    if (!sk.SetHexStr(request.params[1].get_str())) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Secret key must be a valid hex string of length %d", sk.SerSize*2));
    }

    UniValue ret(UniValue::VOBJ);
    ret.pushKV("secret", sk.ToString());
    ret.pushKV("public", sk.GetPublicKey().ToString());
    return ret;
}

[[ noreturn ]] static void bls_help()
{
    throw std::runtime_error(
            "bls \"command\" ...\n"
            "Set of commands to execute BLS related actions.\n"
            "To get help on individual commands, use \"help bls command\".\n"
            "\nArguments:\n"
            "1. \"command\"        (string, required) The command to execute\n"
            "\nAvailable commands:\n"
            "  generate          - Create a BLS secret/public key pair\n"
            "  fromsecret        - Parse a BLS secret key and return the secret/public key pair\n"
            );
}

static UniValue _bls(const JSONRPCRequest& request)
{
    if (request.fHelp && request.params.empty()) {
        bls_help();
    }

    std::string command;
    if (!request.params[0].isNull()) {
        command = request.params[0].get_str();
    }

    if (command == "generate") {
        return bls_generate(request);
    } else if (command == "fromsecret") {
        return bls_fromsecret(request);
    } else {
        bls_help();
    }
}

static const CRPCCommand commands[] =
{ //  category              name                      actor (function)
  //  --------------------- ------------------------  -----------------------
    { "evo",                "bls",                    &_bls,                   {}  },
    { "evo",                "protx",                  &protx,                  {}  },
};

void RegisterEvoRPCCommands(CRPCTable &tableRPC)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++) {
        tableRPC.appendCommand(commands[vcidx].name, &commands[vcidx]);
    }
}
