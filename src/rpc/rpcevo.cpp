// Copyright (c) 2018 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "base58.h"
#include "consensus/validation.h"
#include "core_io.h"
#include "init.h"
#include "messagesigner.h"
#include "rpc/server.h"
#include "utilmoneystr.h"
#include "validation.h"

#ifdef ENABLE_WALLET
#include "wallet/wallet.h"
#endif//ENABLE_WALLET

#include "netbase.h"

#include "evo/specialtx.h"
#include "evo/providertx.h"
#include "evo/deterministicmns.h"

#ifdef ENABLE_WALLET
extern UniValue signrawtransaction(const JSONRPCRequest& request);
extern UniValue sendrawtransaction(const JSONRPCRequest& request);
#endif//ENABLE_WALLET

// Allows to specify Dash address or priv key. In case of Dash address, the priv key is taken from the wallet
static CKey ParsePrivKey(const std::string &strKeyOrAddress, bool allowAddresses = true) {
    CBitcoinAddress address;
    if (allowAddresses && address.SetString(strKeyOrAddress) && address.IsValid()) {
#ifdef ENABLE_WALLET
        CKeyID keyId;
        CKey key;
        if (!address.GetKeyID(keyId) || !pwalletMain->GetKey(keyId, key))
            throw std::runtime_error(strprintf("non-wallet or invalid address %s", strKeyOrAddress));
        return key;
#else//ENABLE_WALLET
        throw std::runtime_error("addresses not supported in no-wallet builds");
#endif//ENABLE_WALLET
    }

    CBitcoinSecret secret;
    if (!secret.SetString(strKeyOrAddress) || !secret.IsValid())
        throw std::runtime_error(strprintf("invalid priv-key/address %s", strKeyOrAddress));
    return secret.GetKey();
}

static CKeyID ParsePubKeyIDFromAddress(const std::string& strAddress, const std::string& paramName)
{
    CBitcoinAddress address(strAddress);
    CKeyID keyID;
    if (!address.IsValid() || !address.GetKeyID(keyID))
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s must be a valid P2PKH address, not %s", paramName, strAddress));
    return keyID;
}


#ifdef ENABLE_WALLET

template<typename SpecialTxPayload>
static void FundSpecialTx(CMutableTransaction& tx, SpecialTxPayload payload)
{
    // resize so that fee calculation is correct
    payload.vchSig.resize(65);

    CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
    ds << payload;
    tx.vExtraPayload.assign(ds.begin(), ds.end());

    static CTxOut dummyTxOut(0, CScript() << OP_RETURN);
    bool dummyTxOutAdded = false;
    if (tx.vout.empty()) {
        // add dummy txout as FundTransaction requires at least one output
        tx.vout.emplace_back(dummyTxOut);
        dummyTxOutAdded = true;
    }

    CAmount nFee;
    CFeeRate feeRate = CFeeRate(0);
    int nChangePos = -1;
    std::string strFailReason;
    std::set<int> setSubtractFeeFromOutputs;
    if (!pwalletMain->FundTransaction(tx, nFee, false, feeRate, nChangePos, strFailReason, false, false, setSubtractFeeFromOutputs, true, CNoDestination()))
        throw JSONRPCError(RPC_INTERNAL_ERROR, strFailReason);

    if (dummyTxOutAdded && tx.vout.size() > 1) {
        // FundTransaction added a change output, so we don't need the dummy txout anymore
        // Removing it results in slight overpayment of fees, but we ignore this for now (as it's a very low amount)
        auto it = std::find(tx.vout.begin(), tx.vout.end(), dummyTxOut);
        assert(it != tx.vout.end());
        tx.vout.erase(it);
    }
}

template<typename SpecialTxPayload>
static void SignSpecialTxPayload(const CMutableTransaction& tx, SpecialTxPayload& payload, const CKey& key)
{
    payload.inputsHash = CalcTxInputsHash(tx);
    payload.vchSig.clear();

    uint256 hash = ::SerializeHash(payload);
    if (!CHashSigner::SignHash(hash, key, payload.vchSig)) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "failed to sign special tx");
    }
}

static std::string SignAndSendSpecialTx(const CMutableTransaction& tx)
{
    LOCK(cs_main);
    CValidationState state;
    if (!CheckSpecialTx(tx, NULL, state))
        throw std::runtime_error(FormatStateMessage(state));

    CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
    ds << tx;

    JSONRPCRequest signReqeust;
    signReqeust.params.setArray();
    signReqeust.params.push_back(HexStr(ds.begin(), ds.end()));
    UniValue signResult = signrawtransaction(signReqeust);

    JSONRPCRequest sendRequest;
    sendRequest.params.setArray();
    sendRequest.params.push_back(signResult["hex"].get_str());
    return sendrawtransaction(sendRequest).get_str();
}

void protx_register_help()
{
    throw std::runtime_error(
            "protx register \"collateralAddress\" collateralAmount \"ipAndPort\" protocolVersion \"ownerKeyAddr\" \"operatorKeyAddr\" \"votingKeyAddr\" operatorReward \"payoutAddress\"\n"
            "\nCreates and sends a ProTx to the network. The resulting transaction will move the specified amount\n"
            "to the address specified by collateralAddress and will then function as the collateral of your\n"
            "masternode.\n"
            "A few of the limitations you see in the arguments are temporary and might be lifted after DIP3\n"
            "is fully deployed.\n"
            "\nArguments:\n"
            "1. \"collateralAddress\"   (string, required) The dash address to send the collateral to.\n"
            "                         Must be a P2PKH address.\n"
            "2. \"collateralAmount\"    (numeric or string, required) The collateral amount.\n"
            "                         Must be exactly 1000 Dash.\n"
            "3. \"ipAndPort\"           (string, required) IP and port in the form \"IP:PORT\".\n"
            "                         Must be unique on the network. Can be set to 0, which will require a ProUpServTx afterwards.\n"
            "4. \"protocolVersion\"     (numeric, required) The protocol version of your masternode.\n"
            "                         Can be 0 to default to the clients protocol version.\n"
            "5. \"ownerKeyAddr\"        (string, required) The owner key used for payee updates and proposal voting.\n"
            "                         The private key belonging to this address be known in your wallet. The address must\n"
            "                         be unused and must differ from the collateralAddress\n"
            "6. \"operatorKeyAddr\"     (string, required) The operator key address. The private key does not have to be known by your wallet.\n"
            "                         It has to match the private key which is later used when operating the masternode.\n"
            "                         If set to \"0\" or an empty string, ownerAddr will be used.\n"
            "7. \"votingKeyAddr\"       (string, required) The voting key address. The private key does not have to be known by your wallet.\n"
            "                         It has to match the private key which is later used when voting on proposals.\n"
            "                         If set to \"0\" or an empty string, ownerAddr will be used.\n"
            "8. \"operatorReward\"      (numeric, required) The fraction in %% to share with the operator. If non-zero,\n"
            "                         \"ipAndPort\" and \"protocolVersion\" must be zero as well. The value must be between 0 and 100.\n"
            "9. \"payoutAddress\"       (string, required) The dash address to use for masternode reward payments\n"
            "                         Must match \"collateralAddress\"."
            "\nExamples:\n"
            + HelpExampleCli("protx", "register \"XwnLY9Tf7Zsef8gMGL2fhWA9ZmMjt4KPwG\" 1000 \"1.2.3.4:1234\" 0 \"93Fd7XY2zF4q9YKTZUSFxLgp4Xs7MuaMnvY9kpvH7V8oXWqsCC1\" XwnLY9Tf7Zsef8gMGL2fhWA9ZmMjt4KPwG")
    );
}

UniValue protx_register(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 10)
        protx_register_help();

    CBitcoinAddress collateralAddress(request.params[1].get_str());
    if (!collateralAddress.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("invalid collaterall address: %s", request.params[1].get_str()));
    CScript collateralScript = GetScriptForDestination(collateralAddress.Get());

    CAmount collateralAmount;
    if (!ParseMoney(request.params[2].get_str(), collateralAmount))
        throw std::runtime_error(strprintf("invalid collateral amount %s", request.params[2].get_str()));
    if (collateralAmount != 1000 * COIN)
        throw std::runtime_error(strprintf("invalid collateral amount %d. only 1000 DASH is supported at the moment", collateralAmount));

    CTxOut collateralTxOut(collateralAmount, collateralScript);

    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_PROVIDER_REGISTER;
    tx.vout.emplace_back(collateralTxOut);

    CProRegTx ptx;
    ptx.nVersion = CProRegTx::CURRENT_VERSION;

    if (request.params[3].get_str() != "0" && request.params[3].get_str() != "") {
        if (!Lookup(request.params[3].get_str().c_str(), ptx.addr, Params().GetDefaultPort(), false))
            throw std::runtime_error(strprintf("invalid network address %s", request.params[3].get_str()));
    }

    ptx.nProtocolVersion = ParseInt32V(request.params[4], "protocolVersion");
    if (ptx.nProtocolVersion == 0 && ptx.addr != CService())
        ptx.nProtocolVersion = PROTOCOL_VERSION;

    CKey keyOwner = ParsePrivKey(request.params[5].get_str(), true);
    CKeyID keyIDOperator = keyOwner.GetPubKey().GetID();
    CKeyID keyIDVoting = keyOwner.GetPubKey().GetID();
    if (request.params[6].get_str() != "0" && request.params[6].get_str() != "") {
        keyIDOperator = ParsePubKeyIDFromAddress(request.params[6].get_str(), "operator address");
    }
    if (request.params[7].get_str() != "0" && request.params[7].get_str() != "") {
        keyIDVoting = ParsePubKeyIDFromAddress(request.params[7].get_str(), "voting address");
    }

    double operatorReward = ParseDoubleV(request.params[8], "operatorReward");
    if (operatorReward < 0 || operatorReward > 100)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "operatorReward must be between 0 and 100");
    ptx.nOperatorReward = (uint16_t)(operatorReward * 100);

    CBitcoinAddress payoutAddress(request.params[9].get_str());
    if (!payoutAddress.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("invalid payout address: %s", request.params[9].get_str()));

    ptx.keyIDOwner = keyOwner.GetPubKey().GetID();
    ptx.keyIDOperator = keyIDOperator;
    ptx.keyIDVoting = keyIDVoting;
    ptx.scriptPayout = GetScriptForDestination(payoutAddress.Get());

    FundSpecialTx(tx, ptx);

    uint32_t collateralIndex = (uint32_t) - 1;
    for (uint32_t i = 0; i < tx.vout.size(); i++) {
        if (tx.vout[i] == collateralTxOut) {
            collateralIndex = i;
            break;
        }
    }
    assert(collateralIndex != (uint32_t) - 1);
    ptx.nCollateralIndex = collateralIndex;

    SignSpecialTxPayload(tx, ptx, keyOwner);
    SetTxPayload(tx, ptx);

    return SignAndSendSpecialTx(tx);
}

void protx_update_service_help()
{
    throw std::runtime_error(
            "protx update_service \"proTxHash\" \"ipAndPort\" protocolVersion (\"operatorPayoutAddress\")\n"
            "\nCreates and sends a ProUpServTx to the network. This will update the address and protocol version\n"
            "of a masternode. The operator key of the masternode must be known to your wallet.\n"
            "If this is done for a masternode that got PoSe-banned, the ProUpServTx will also revive this masternode.\n"
            "\nArguments:\n"
            "1. \"proTxHash\"                (string, required) The hash of the initial ProRegTx.\n"
            "2. \"ipAndPort\"                (string, required) IP and port in the form \"IP:PORT\".\n"
            "                              Must be unique on the network.\n"
            "3. \"protocolVersion\"          (numeric, required) The protocol version of your masternode.\n"
            "                              Can be 0 to default to the clients protocol version\n"
            "4. \"operatorPayoutAddress\"    (string, optional) The address used for operator reward payments.\n"
            "                              Only allowed when the ProRegTx had a non-zero operatorReward value.\n"
            "\nExamples:\n"
            + HelpExampleCli("protx", "update_service \"0123456701234567012345670123456701234567012345670123456701234567\" \"1.2.3.4:1234\" 0")
    );
}

UniValue protx_update_service(const JSONRPCRequest& request)
{
    if (request.fHelp || (request.params.size() != 4 && request.params.size() != 5))
        protx_update_service_help();

    CProUpServTx ptx;
    ptx.nVersion = CProRegTx::CURRENT_VERSION;
    ptx.proTxHash = ParseHashV(request.params[1], "proTxHash");

    if (!Lookup(request.params[2].get_str().c_str(), ptx.addr, Params().GetDefaultPort(), false)) {
        throw std::runtime_error(strprintf("invalid network address %s", request.params[3].get_str()));
    }

    ptx.nProtocolVersion = ParseInt32V(request.params[3], "protocolVersion");
    if (ptx.nProtocolVersion == 0) {
        ptx.nProtocolVersion = PROTOCOL_VERSION;
    }

    if (request.params.size() > 4) {
        CBitcoinAddress payoutAddress(request.params[4].get_str());
        if (!payoutAddress.IsValid())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("invalid operator payout address: %s", request.params[4].get_str()));
        ptx.scriptOperatorPayout = GetScriptForDestination(payoutAddress.Get());
    }

    auto dmn = deterministicMNManager->GetListAtChainTip().GetMN(ptx.proTxHash);
    if (!dmn) {
        throw std::runtime_error(strprintf("masternode with proTxHash %s not found", ptx.proTxHash.ToString()));
    }

    CKey keyOperator;
    if (!pwalletMain->GetKey(dmn->pdmnState->keyIDOperator, keyOperator)) {
        throw std::runtime_error(strprintf("operator key %s not found in your wallet", dmn->pdmnState->keyIDOperator.ToString()));
    }

    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_PROVIDER_UPDATE_SERVICE;

    FundSpecialTx(tx, ptx);
    SignSpecialTxPayload(tx, ptx, keyOperator);
    SetTxPayload(tx, ptx);

    return SignAndSendSpecialTx(tx);
}

void protx_update_registrar_help()
{
    throw std::runtime_error(
            "protx update_registrar \"proTxHash\" \"operatorKeyAddr\" \"votingKeyAddr\" operatorReward \"payoutAddress\"\n"
            "\nCreates and sends a ProUpRegTx to the network. This will update the operator key, voting key and payout\n"
            "address of the masternode specified by \"proTxHash\".\n"
            "The owner key of the masternode must be known to your wallet.\n"
            "\nArguments:\n"
            "1. \"proTxHash\"           (string, required) The hash of the initial ProRegTx.\n"
            "2. \"operatorKeyAddr\"     (string, required) The operator key address. The private key does not have to be known by your wallet.\n"
            "                         It has to match the private key which is later used when operating the masternode.\n"
            "                         If set to \"0\" or an empty string, the last on-chain operator key of the masternode will be used.\n"
            "3. \"votingKeyAddr\"       (string, required) The voting key address. The private key does not have to be known by your wallet.\n"
            "                         It has to match the private key which is later used when voting on proposals.\n"
            "                         If set to \"0\" or an empty string, the last on-chain voting key of the masternode will be used.\n"
            "5. \"payoutAddress\"       (string, required) The dash address to use for masternode reward payments\n"
            "                         Must match \"collateralAddress\" of initial ProRegTx.\n"
            "                         If set to \"0\" or an empty string, the last on-chain payout address of the masternode will be used.\n"
            "\nExamples:\n"
            + HelpExampleCli("protx", "update_registrar \"0123456701234567012345670123456701234567012345670123456701234567\" \"<operatorKeyAddr>\" \"0\" \"XwnLY9Tf7Zsef8gMGL2fhWA9ZmMjt4KPwG\"")
    );
}

UniValue protx_update_registrar(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 5)
        protx_update_registrar_help();

    CProUpRegTx ptx;
    ptx.nVersion = CProRegTx::CURRENT_VERSION;
    ptx.proTxHash = ParseHashV(request.params[1], "proTxHash");

    auto dmn = deterministicMNManager->GetListAtChainTip().GetMN(ptx.proTxHash);
    if (!dmn) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("masternode %s not found", ptx.proTxHash.ToString()));
    }
    ptx.keyIDOperator = dmn->pdmnState->keyIDOperator;
    ptx.keyIDVoting = dmn->pdmnState->keyIDVoting;
    ptx.scriptPayout = dmn->pdmnState->scriptPayout;

    if (request.params[2].get_str() != "0" && request.params[2].get_str() != "") {
        ptx.keyIDOperator = ParsePubKeyIDFromAddress(request.params[2].get_str(), "operator address");
    }
    if (request.params[3].get_str() != "0" && request.params[3].get_str() != "") {
        ptx.keyIDVoting = ParsePubKeyIDFromAddress(request.params[3].get_str(), "operator address");
    }

    CBitcoinAddress payoutAddress(request.params[4].get_str());
    if (!payoutAddress.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("invalid payout address: %s", request.params[4].get_str()));
    ptx.scriptPayout = GetScriptForDestination(payoutAddress.Get());

    CKey keyOwner;
    if (!pwalletMain->GetKey(dmn->pdmnState->keyIDOwner, keyOwner)) {
        throw std::runtime_error(strprintf("owner key %s not found in your wallet", dmn->pdmnState->keyIDOwner.ToString()));
    }

    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_PROVIDER_UPDATE_REGISTRAR;

    FundSpecialTx(tx, ptx);
    SignSpecialTxPayload(tx, ptx, keyOwner);
    SetTxPayload(tx, ptx);

    return SignAndSendSpecialTx(tx);
}

void protx_revoke_help()
{
    throw std::runtime_error(
            "protx revoke \"proTxHash\"\n"
            "\nCreates and sends a ProUpRevTx to the network. This will revoke the operator key of the masternode and\n"
            "put it into the PoSe-banned state. It will also set the service and protocol version fields of the masternode\n"
            "to zero. Use this in case your operator key got compromised or you want to stop providing your service\n"
            "to the masternode owner.\n"
            "The operator key of the masternode must be known to your wallet.\n"
            "\nArguments:\n"
            "1. \"proTxHash\"           (string, required) The hash of the initial ProRegTx.\n"
            "2. reason                  (numeric, optional) The reason for revocation.\n"
            "\nExamples:\n"
            + HelpExampleCli("protx", "revoke \"0123456701234567012345670123456701234567012345670123456701234567\" \"<operatorKeyAddr>\"")
    );
}

UniValue protx_revoke(const JSONRPCRequest& request)
{
    if (request.fHelp || (request.params.size() != 2 && request.params.size() != 3))
        protx_revoke_help();

    CProUpRevTx ptx;
    ptx.nVersion = CProRegTx::CURRENT_VERSION;
    ptx.proTxHash = ParseHashV(request.params[1], "proTxHash");

    if (request.params.size() > 2) {
        int32_t nReason = ParseInt32V(request.params[2], "reason");
        if (nReason < 0 || nReason >= CProUpRevTx::REASON_LAST)
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("invalid reason %d, must be between 0 and %d", nReason, CProUpRevTx::REASON_LAST));
        ptx.nReason = (uint16_t)nReason;
    }

    auto dmn = deterministicMNManager->GetListAtChainTip().GetMN(ptx.proTxHash);
    if (!dmn) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("masternode %s not found", ptx.proTxHash.ToString()));
    }

    CKey keyOperator;
    if (!pwalletMain->GetKey(dmn->pdmnState->keyIDOperator, keyOperator)) {
        throw std::runtime_error(strprintf("operator key %s not found in your wallet", dmn->pdmnState->keyIDOwner.ToString()));
    }

    CMutableTransaction tx;
    tx.nVersion = 3;
    tx.nType = TRANSACTION_PROVIDER_UPDATE_REVOKE;

    FundSpecialTx(tx, ptx);
    SignSpecialTxPayload(tx, ptx, keyOperator);
    SetTxPayload(tx, ptx);

    return SignAndSendSpecialTx(tx);
}

void protx_list_help()
{
    throw std::runtime_error(
            "protx list (\"type\")\n"
            "\nLists all ProTxs in your wallet or on-chain, depending on the given type. If \"type\" is not\n"
            "specified, it defaults to \"wallet\". All types have the optional argument \"detailed\" which if set to\n"
            "\"true\" will result in a detailed list to be returned. If set to \"false\", only the hashes of the ProTx\n"
            "will be returned.\n"
            "\nAvailable types:\n"
            "  wallet (detailed)              - List only ProTx which are found in your wallet. This will also include ProTx which\n"
            "                                   failed PoSe verfication\n"
            "  valid (height) (detailed)      - List only ProTx which are active/valid at the given chain height. If height is not\n"
            "                                   specified, it defaults to the current chain-tip\n"
            "  registered (height) (detaileD) - List all ProTx which are registered at the given chain height. If height is not\n"
            "                                   specified, it defaults to the current chain-tip. This will also include ProTx\n"
            "                                   which failed PoSe verification at that height\n"
    );
}

static bool CheckWalletOwnsScript(const CScript& script) {
    CTxDestination dest;
    if (ExtractDestination(script, dest)) {
        if ((boost::get<CKeyID>(&dest) && pwalletMain->HaveKey(*boost::get<CKeyID>(&dest))) || (boost::get<CScriptID>(&dest) && pwalletMain->HaveCScript(*boost::get<CScriptID>(&dest)))) {
            return true;
        }
    }
    return false;
}

UniValue BuildDMNListEntry(const CDeterministicMNCPtr& dmn, bool detailed)
{
    if (!detailed)
        return dmn->proTxHash.ToString();

    UniValue o(UniValue::VOBJ);

    dmn->ToJson(o);

    int confirmations = GetUTXOConfirmations(COutPoint(dmn->proTxHash, dmn->nCollateralIndex));
    o.push_back(Pair("confirmations", confirmations));

    bool hasOwnerKey = pwalletMain->HaveKey(dmn->pdmnState->keyIDOwner);
    bool hasOperatorKey = pwalletMain->HaveKey(dmn->pdmnState->keyIDOperator);
    bool hasVotingKey = pwalletMain->HaveKey(dmn->pdmnState->keyIDVoting);

    bool ownsCollateral = false;
    CTransactionRef collateralTx;
    uint256 tmpHashBlock;
    if (GetTransaction(dmn->proTxHash, collateralTx, Params().GetConsensus(), tmpHashBlock)) {
        ownsCollateral = CheckWalletOwnsScript(collateralTx->vout[dmn->nCollateralIndex].scriptPubKey);
    }

    UniValue walletObj(UniValue::VOBJ);
    walletObj.push_back(Pair("hasOwnerKey", hasOwnerKey));
    walletObj.push_back(Pair("hasOperatorKey", hasOperatorKey));
    walletObj.push_back(Pair("hasVotingKey", hasVotingKey));
    walletObj.push_back(Pair("ownsCollateral", ownsCollateral));
    walletObj.push_back(Pair("ownsPayeeScript", CheckWalletOwnsScript(dmn->pdmnState->scriptPayout)));
    walletObj.push_back(Pair("ownsOperatorRewardScript", CheckWalletOwnsScript(dmn->pdmnState->scriptOperatorPayout)));
    o.push_back(Pair("wallet", walletObj));

    return o;
}

UniValue protx_list(const JSONRPCRequest& request)
{
    if (request.fHelp)
        protx_list_help();

    std::string type = "wallet";
    if (request.params.size() > 1)
        type = request.params[1].get_str();

    UniValue ret(UniValue::VARR);

    LOCK2(cs_main, pwalletMain->cs_wallet);

    if (type == "wallet") {
        if (request.params.size() > 3)
            protx_list_help();

        bool detailed = request.params.size() > 2 ? ParseBoolV(request.params[2], "detailed") : false;

        std::vector<COutPoint> vOutpts;
        pwalletMain->ListProTxCoins(vOutpts);
        std::set<uint256> setOutpts;
        for (const auto& outpt : vOutpts) {
            setOutpts.emplace(outpt.hash);
        }

        for (const auto& dmn : deterministicMNManager->GetListAtChainTip().all_range()) {
            if (setOutpts.count(dmn->proTxHash) ||
                    pwalletMain->HaveKey(dmn->pdmnState->keyIDOwner) ||
                    pwalletMain->HaveKey(dmn->pdmnState->keyIDOperator) ||
                    pwalletMain->HaveKey(dmn->pdmnState->keyIDVoting) ||
                    CheckWalletOwnsScript(dmn->pdmnState->scriptPayout) ||
                    CheckWalletOwnsScript(dmn->pdmnState->scriptOperatorPayout)) {
                ret.push_back(BuildDMNListEntry(dmn, detailed));
            }
        }
    } else if (type == "valid" || type == "registered") {
        if (request.params.size() > 4)
            protx_list_help();

        LOCK(cs_main);

        int height = request.params.size() > 2 ? ParseInt32V(request.params[2], "height") : chainActive.Height();
        if (height < 1 || height > chainActive.Height())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid height specified");

        bool detailed = request.params.size() > 3 ? ParseBoolV(request.params[3], "detailed") : false;

        CDeterministicMNList mnList = deterministicMNManager->GetListForBlock(chainActive[height]->GetBlockHash());
        CDeterministicMNList::range_type range;

        if (type == "valid") {
            range = mnList.valid_range();
        } else if (type == "registered") {
            range = mnList.all_range();
        }
        for (const auto& dmn : range) {
            ret.push_back(BuildDMNListEntry(dmn, detailed));
        }
    } else {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid type specified");
    }

    return ret;
}

UniValue protx(const JSONRPCRequest& request)
{
    if (request.params.empty()) {
        throw std::runtime_error(
                "protx \"command\" ...\n"
                "Set of commands to execute ProTx related actions.\n"
                "To get help on individual commands, use \"help protx command\".\n"
                "\nArguments:\n"
                "1. \"command\"        (string, required) The command to execute\n"
                "\nAvailable commands:\n"
                "  register          - Create and send ProTx to network\n"
                "  list              - List ProTxs\n"
                "  update_service    - Create and send ProUpServTx to network\n"
                "  update_registrar  - Create and send ProUpRegTx to network\n"
                "  revoke            - Create and send ProUpRevTx to network\n"
        );
    }

    std::string command = request.params[0].get_str();

    if (command == "register") {
        return protx_register(request);
    } else if (command == "list") {
        return protx_list(request);
    } else if (command == "update_service") {
        return protx_update_service(request);
    } else if (command == "update_registrar") {
        return protx_update_registrar(request);
    } else if (command == "revoke") {
        return protx_revoke(request);
    } else {
        throw std::runtime_error("invalid command: " + command);
    }
}
#endif//ENABLE_WALLET

static const CRPCCommand commands[] =
{ //  category              name                      actor (function)         okSafeMode
  //  --------------------- ------------------------  -----------------------  ----------
#ifdef ENABLE_WALLET
    // these require the wallet to be enabled to fund the transactions
    { "evo",                "protx",                  &protx,                  false, {}  },
#endif//ENABLE_WALLET
};

void RegisterEvoRPCCommands(CRPCTable &tableRPC)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        tableRPC.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
