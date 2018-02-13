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
                "  register    - Create and send ProTx to network\n"
        );
    }

    std::string command = request.params[0].get_str();

    if (command == "register") {
        return protx_register(request);
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
