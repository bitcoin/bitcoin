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
#include "evo/simplifiedmns.h"

#include "bls/bls.h"

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

static CBLSPublicKey ParseBLSPubKey(const std::string& hexKey, const std::string& paramName)
{
    auto binKey = ParseHex(hexKey);
    CBLSPublicKey pubKey;
    pubKey.SetBuf(binKey);
    if (!pubKey.IsValid()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s must be a valid BLS address, not %s", paramName, hexKey));
    }
    return pubKey;
}

static CBLSSecretKey ParseBLSSecretKey(const std::string& hexKey, const std::string& paramName)
{
    auto binKey = ParseHex(hexKey);
    CBLSSecretKey secKey;
    secKey.SetBuf(binKey);
    if (!secKey.IsValid()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s must be a valid BLS secret key", paramName));
    }
    return secKey;
}

#ifdef ENABLE_WALLET

template<typename SpecialTxPayload>
static void FundSpecialTx(CMutableTransaction& tx, SpecialTxPayload payload)
{
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

template<typename SpecialTxPayload>
static void SignSpecialTxPayload(const CMutableTransaction& tx, SpecialTxPayload& payload, const CBLSSecretKey& key)
{
    payload.inputsHash = CalcTxInputsHash(tx);

    uint256 hash = ::SerializeHash(payload);
    payload.sig = key.Sign(hash);
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
            "protx register \"collateralAddress\" collateralAmount \"ipAndPort\" \"ownerKeyAddr\" \"operatorKeyAddr\" \"votingKeyAddr\" operatorReward \"payoutAddress\"\n"
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
            "4. \"ownerKeyAddr\"        (string, required) The owner key used for payee updates and proposal voting.\n"
            "                         The private key belonging to this address be known in your wallet. The address must\n"
            "                         be unused and must differ from the collateralAddress\n"
            "5. \"operatorKeyAddr\"     (string, required) The operator key address. The private key does not have to be known by your wallet.\n"
            "                         It has to match the private key which is later used when operating the masternode.\n"
            "                         If set to \"0\" or an empty string, ownerAddr will be used.\n"
            "6. \"votingKeyAddr\"       (string, required) The voting key address. The private key does not have to be known by your wallet.\n"
            "                         It has to match the private key which is later used when voting on proposals.\n"
            "                         If set to \"0\" or an empty string, ownerAddr will be used.\n"
            "7. \"operatorReward\"      (numeric, required) The fraction in %% to share with the operator. If non-zero,\n"
            "                         \"ipAndPort\" must be zero as well. The value must be between 0 and 100.\n"
            "8. \"payoutAddress\"       (string, required) The dash address to use for masternode reward payments\n"
            "                         Must match \"collateralAddress\"."
            "\nExamples:\n"
            + HelpExampleCli("protx", "register \"XrVhS9LogauRJGJu2sHuryjhpuex4RNPSb\" 1000 \"1.2.3.4:1234\" \"Xt9AMWaYSz7tR7Uo7gzXA3m4QmeWgrR3rr\" \"Xt9AMWaYSz7tR7Uo7gzXA3m4QmeWgrR3rr\" \"Xt9AMWaYSz7tR7Uo7gzXA3m4QmeWgrR3rr\" 0 \"XrVhS9LogauRJGJu2sHuryjhpuex4RNPSb\"")
    );
}

UniValue protx_register(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 9)
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

    CKey keyOwner = ParsePrivKey(request.params[4].get_str(), true);
    CBLSPublicKey pubKeyOperator = ParseBLSPubKey(request.params[5].get_str(), "operator BLS address");
    CKeyID keyIDVoting = keyOwner.GetPubKey().GetID();
    if (request.params[6].get_str() != "0" && request.params[6].get_str() != "") {
        keyIDVoting = ParsePubKeyIDFromAddress(request.params[6].get_str(), "voting address");
    }

    double operatorReward = ParseDoubleV(request.params[7], "operatorReward");
    if (operatorReward < 0 || operatorReward > 100)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "operatorReward must be between 0 and 100");
    ptx.nOperatorReward = (uint16_t)(operatorReward * 100);

    CBitcoinAddress payoutAddress(request.params[8].get_str());
    if (!payoutAddress.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("invalid payout address: %s", request.params[8].get_str()));

    ptx.keyIDOwner = keyOwner.GetPubKey().GetID();
    ptx.pubKeyOperator = pubKeyOperator;
    ptx.keyIDVoting = keyIDVoting;
    ptx.scriptPayout = GetScriptForDestination(payoutAddress.Get());

    SignSpecialTxPayload(tx, ptx, keyOwner); // make sure sig is set, otherwise fee calculation won't be correct
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

    SignSpecialTxPayload(tx, ptx, keyOwner); // redo signing
    SetTxPayload(tx, ptx);

    return SignAndSendSpecialTx(tx);
}

void protx_update_service_help()
{
    throw std::runtime_error(
            "protx update_service \"proTxHash\" \"ipAndPort\" \"operatorKey\" (\"operatorPayoutAddress\")\n"
            "\nCreates and sends a ProUpServTx to the network. This will update the IP address\n"
            "of a masternode. The operator key of the masternode must be known to your wallet.\n"
            "If this is done for a masternode that got PoSe-banned, the ProUpServTx will also revive this masternode.\n"
            "\nArguments:\n"
            "1. \"proTxHash\"                (string, required) The hash of the initial ProRegTx.\n"
            "2. \"ipAndPort\"                (string, required) IP and port in the form \"IP:PORT\".\n"
            "                              Must be unique on the network.\n"
            "3. \"operatorKey\"              (string, required) The operator private key belonging to the\n"
            "                              registered operator public key.\n"
            "4. \"operatorPayoutAddress\"    (string, optional) The address used for operator reward payments.\n"
            "                              Only allowed when the ProRegTx had a non-zero operatorReward value.\n"
            "\nExamples:\n"
            + HelpExampleCli("protx", "update_service \"0123456701234567012345670123456701234567012345670123456701234567\" \"1.2.3.4:1234\" 5a2e15982e62f1e0b7cf9783c64cf7e3af3f90a52d6c40f6f95d624c0b1621cd")
    );
}

UniValue protx_update_service(const JSONRPCRequest& request)
{
    if (request.fHelp || (request.params.size() != 3 && request.params.size() != 4))
        protx_update_service_help();

    CProUpServTx ptx;
    ptx.nVersion = CProRegTx::CURRENT_VERSION;
    ptx.proTxHash = ParseHashV(request.params[1], "proTxHash");

    if (!Lookup(request.params[2].get_str().c_str(), ptx.addr, Params().GetDefaultPort(), false)) {
        throw std::runtime_error(strprintf("invalid network address %s", request.params[3].get_str()));
    }

    CBLSSecretKey keyOperator = ParseBLSSecretKey(request.params[3].get_str(), "operatorKey");

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

    if (keyOperator.GetPublicKey() != dmn->pdmnState->pubKeyOperator) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("the operator key does not belong to the registered public key"));
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
            "2. \"operatorPubKey\"      (string, required) The operator public key. The private key does not have to be known by you.\n"
            "                         It has to match the private key which is later used when operating the masternode.\n"
            "                         If set to \"0\" or an empty string, the last on-chain operator key of the masternode will be used.\n"
            "3. \"votingKeyAddr\"       (string, required) The voting key address. The private key does not have to be known by your wallet.\n"
            "                         It has to match the private key which is later used when voting on proposals.\n"
            "                         If set to \"0\" or an empty string, the last on-chain voting key of the masternode will be used.\n"
            "5. \"payoutAddress\"       (string, required) The dash address to use for masternode reward payments\n"
            "                         Must match \"collateralAddress\" of initial ProRegTx.\n"
            "                         If set to \"0\" or an empty string, the last on-chain payout address of the masternode will be used.\n"
            "\nExamples:\n"
            + HelpExampleCli("protx", "update_registrar \"0123456701234567012345670123456701234567012345670123456701234567\" \"982eb34b7c7f614f29e5c665bc3605f1beeef85e3395ca12d3be49d2868ecfea5566f11cedfad30c51b2403f2ad95b67\" \"0\" \"XwnLY9Tf7Zsef8gMGL2fhWA9ZmMjt4KPwG\"")
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
    ptx.pubKeyOperator = dmn->pdmnState->pubKeyOperator;
    ptx.keyIDVoting = dmn->pdmnState->keyIDVoting;
    ptx.scriptPayout = dmn->pdmnState->scriptPayout;

    if (request.params[2].get_str() != "0" && request.params[2].get_str() != "") {
        ptx.pubKeyOperator = ParseBLSPubKey(request.params[2].get_str(), "operator BLS address");
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
            "put it into the PoSe-banned state. It will also set the service field of the masternode\n"
            "to zero. Use this in case your operator key got compromised or you want to stop providing your service\n"
            "to the masternode owner.\n"
            "The operator key of the masternode must be known to your wallet.\n"
            "\nArguments:\n"
            "1. \"proTxHash\"           (string, required) The hash of the initial ProRegTx.\n"
            "2. \"operatorKey\"         (string, required) The operator private key belonging to the\n"
            "                         registered operator public key.\n"
            "3. reason                  (numeric, optional) The reason for revocation.\n"
            "\nExamples:\n"
            + HelpExampleCli("protx", "revoke \"0123456701234567012345670123456701234567012345670123456701234567\" \"072f36a77261cdd5d64c32d97bac417540eddca1d5612f416feb07ff75a8e240\"")
    );
}

UniValue protx_revoke(const JSONRPCRequest& request)
{
    if (request.fHelp || (request.params.size() != 2 && request.params.size() != 3))
        protx_revoke_help();

    CProUpRevTx ptx;
    ptx.nVersion = CProRegTx::CURRENT_VERSION;
    ptx.proTxHash = ParseHashV(request.params[1], "proTxHash");

    CBLSSecretKey keyOperator = ParseBLSSecretKey(request.params[2].get_str(), "operatorKey");

    if (request.params.size() > 3) {
        int32_t nReason = ParseInt32V(request.params[3], "reason");
        if (nReason < 0 || nReason >= CProUpRevTx::REASON_LAST)
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("invalid reason %d, must be between 0 and %d", nReason, CProUpRevTx::REASON_LAST));
        ptx.nReason = (uint16_t)nReason;
    }

    auto dmn = deterministicMNManager->GetListAtChainTip().GetMN(ptx.proTxHash);
    if (!dmn) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("masternode %s not found", ptx.proTxHash.ToString()));
    }

    if (keyOperator.GetPublicKey() != dmn->pdmnState->pubKeyOperator) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("the operator key does not belong to the registered public key"));
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
    bool hasOperatorKey = false; //pwalletMain->HaveKey(dmn->pdmnState->keyIDOperator);
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

        deterministicMNManager->GetListAtChainTip().ForEachMN(false, [&](const CDeterministicMNCPtr& dmn) {
            if (setOutpts.count(dmn->proTxHash) ||
                pwalletMain->HaveKey(dmn->pdmnState->keyIDOwner) ||
                pwalletMain->HaveKey(dmn->pdmnState->keyIDVoting) ||
                CheckWalletOwnsScript(dmn->pdmnState->scriptPayout) ||
                CheckWalletOwnsScript(dmn->pdmnState->scriptOperatorPayout)) {
                ret.push_back(BuildDMNListEntry(dmn, detailed));
            }
        });
    } else if (type == "valid" || type == "registered") {
        if (request.params.size() > 4)
            protx_list_help();

        LOCK(cs_main);

        int height = request.params.size() > 2 ? ParseInt32V(request.params[2], "height") : chainActive.Height();
        if (height < 1 || height > chainActive.Height())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid height specified");

        bool detailed = request.params.size() > 3 ? ParseBoolV(request.params[3], "detailed") : false;

        CDeterministicMNList mnList = deterministicMNManager->GetListForBlock(chainActive[height]->GetBlockHash());
        bool onlyValid = type == "valid";
        mnList.ForEachMN(onlyValid, [&](const CDeterministicMNCPtr& dmn) {
            ret.push_back(BuildDMNListEntry(dmn, detailed));
        });
    } else {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid type specified");
    }

    return ret;
}

void protx_diff_help()
{
    throw std::runtime_error(
            "protx diff \"baseBlock\" \"block\"\n"
            "\nCalculates a diff between two deterministic masternode lists. The result also contains proof data.\n"
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

UniValue protx_diff(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 3)
        protx_diff_help();

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
                "  diff              - Calculate a diff and a proof between two masternode lists\n"
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
    } else if (command == "diff") {
        return protx_diff(request);
    } else {
        throw std::runtime_error("invalid command: " + command);
    }
}
#endif//ENABLE_WALLET

UniValue bls_generate(const JSONRPCRequest& request)
{
    CBLSSecretKey sk;
    sk.MakeNewKey();

    UniValue ret(UniValue::VOBJ);
    ret.push_back(Pair("secret", sk.ToString()));
    ret.push_back(Pair("public", sk.GetPublicKey().ToString()));
    return ret;
}

UniValue _bls(const JSONRPCRequest& request)
{
    if (request.params.empty()) {
        throw std::runtime_error(
                "bls \"command\" ...\n"
        );
    }

    std::string command = request.params[0].get_str();

    if (command == "generate") {
        return bls_generate(request);
    } else {
        throw std::runtime_error("invalid command: " + command);
    }
}

static const CRPCCommand commands[] =
{ //  category              name                      actor (function)         okSafeMode
  //  --------------------- ------------------------  -----------------------  ----------
    { "evo",                "bls",                    &_bls,                   false, {}  },
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
