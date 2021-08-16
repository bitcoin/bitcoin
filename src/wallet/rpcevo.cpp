// Copyright (c) 2018-2020 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/validation.h>
#include <core_io.h>
#include <init.h>
#include <messagesigner.h>
#include <rpc/server.h>
#include <util/moneystr.h>
#include <validation.h>

#include <wallet/coincontrol.h>
#include <wallet/wallet.h>
#include <wallet/rpcwallet.h>

#include <netbase.h>

#include <evo/specialtx.h>
#include <evo/providertx.h>
#include <evo/deterministicmns.h>
#include <evo/simplifiedmns.h>

#include <bls/bls.h>

#include <masternode/masternodemeta.h>
#include <rpc/util.h>
#include <rpc/blockchain.h>
#include <util/translation.h>
#include <node/context.h>
#include <node/transaction.h>

static CKeyID ParsePubKeyIDFromAddress(const std::string& strAddress, const std::string& paramName)
{
    CTxDestination dest = DecodeDestination(strAddress);
    const WitnessV0KeyHash *keyID = std::get_if<WitnessV0KeyHash>(&dest);
    if (!keyID) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s must be a valid P2PWKH address, not %s", paramName, strAddress));
    }
    return ToKeyID(*keyID);
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

template<typename SpecialTxPayload>
static void FundSpecialTx(CWallet& pwallet, CMutableTransaction& tx, const SpecialTxPayload& payload, const CTxDestination& fundDest)
{

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet.BlockUntilSyncedToCurrentChain();
    {
        LOCK(pwallet.cs_wallet);

        CTxDestination nodest = CNoDestination();
        if (fundDest == nodest) {
            throw JSONRPCError(RPC_INTERNAL_ERROR, "No source of funds specified");
        }

        SetTxPayload(tx, payload);
        std::vector<CRecipient> vecSend;
        for (const auto& txOut : tx.vout) {
            CRecipient recipient = {txOut.scriptPubKey, txOut.nValue, false};
            vecSend.push_back(recipient);
        }

        CCoinControl coinControl;
        coinControl.destChange = fundDest;

        std::vector<COutput> vecOutputs;
        pwallet.AvailableCoins(vecOutputs);

        for (const auto& out : vecOutputs) {
            CTxDestination txDest;
            if (ExtractDestination(out.tx->tx->vout[out.i].scriptPubKey, txDest) && txDest == fundDest) {
                coinControl.Select(COutPoint(out.tx->tx->GetHash(), out.i));
            }
        }

        if (!coinControl.HasSelected()) {
            throw JSONRPCError(RPC_INTERNAL_ERROR, "No funds at specified address");
        }
        CAmount nFeeRequired = 0;
        int nChangePosRet = -1;
        bilingual_str error;
        CTransactionRef wtx;
        FeeCalculation fee_calc_out;
        bool fCreated = pwallet.CreateTransaction(vecSend, wtx, nFeeRequired, nChangePosRet, error, coinControl, fee_calc_out);
        if (!fCreated)
            throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, error.original);

        tx.vin = wtx->vin;
        tx.vout = wtx->vout;
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
static UniValue SignAndSendSpecialTx(const JSONRPCRequest& request, const CMutableTransaction& tx, bool fSubmit = true)
{
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    {
        LOCK(cs_main);

        TxValidationState state;
        if (!CheckSpecialTx(node.chainman->m_blockman, CTransaction(tx), node.chainman->ActiveTip(), state, node.chainman->ActiveChainstate().CoinsTip(), true)) {
            throw std::runtime_error(state.ToString());
        }
    } // cs_main

    CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
    ds << tx;

    JSONRPCRequest signRequest;
    signRequest.context = request.context;
    signRequest.URI = request.URI;
    signRequest.params.setArray();
    signRequest.params.push_back(HexStr(ds));
    UniValue signResult = signrawtransactionwithwallet().HandleRequest(signRequest);
    if (!fSubmit) {
        return signResult["hex"].get_str();
    }
    JSONRPCRequest sendRequest;
    sendRequest.context = request.context;
    sendRequest.URI = request.URI;
    sendRequest.params.setArray();
    sendRequest.params.push_back(signResult["hex"].get_str());
    return sendrawtransaction().HandleRequest(sendRequest);
}


// handles register, register_prepare and register_fund
static RPCHelpMan protx_register()
{
    return RPCHelpMan{"protx_register",
                "\nSame as \"protx_register_fund\", but with an externally referenced collateral.\n"
                "The collateral is specified through \"collateralHash\" and \"collateralIndex\" and must be an unspent\n"
                "transaction output spendable by this wallet. It must also not be used by any other masternode.\n",
                {
                    {"collateralHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The collateral transaction hash."},
                    {"collateralIndex", RPCArg::Type::NUM, RPCArg::Optional::NO, "The collateral transaction output index."},
                    {"ipAndPort", RPCArg::Type::STR, RPCArg::Optional::NO, "IP and port in the form \"IP:PORT\".\n"
                                        "Must be unique on the network. Can be set to 0, which will require a ProUpServTx afterwards."},
                    {"ownerAddress", RPCArg::Type::STR, RPCArg::Optional::NO, "The Syscoin address to use for payee updates and proposal voting.\n"
                                        "The corresponding private key does not have to be known by your wallet.\n"
                                        "The address must be unused and must differ from the collateralAddress."},
                    {"operatorPubKey", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The operator BLS public key. The BLS private key does not have to be known.\n"
                                        "It has to match the BLS private key which is later used when operating the masternode."},
                    {"votingAddress", RPCArg::Type::STR, RPCArg::Optional::NO, "The voting key address. The private key does not have to be known by your wallet.\n"
                                        "It has to match the private key which is later used when voting on proposals.\n"
                                        "If set to an empty string, ownerAddress will be used.\n"},
                    {"operatorReward", RPCArg::Type::NUM, RPCArg::Optional::NO, "The fraction in %% to share with the operator. The value must be\n"
                                        "between 0.00 and 100.00."},
                    {"payoutAddress", RPCArg::Type::STR, RPCArg::Optional::NO, "The Syscoin address to use for masternode reward payments."},
                    {"fundAddress", RPCArg::Type::STR, RPCArg::Default{""}, "If specified wallet will only use coins from this address to fund ProTx.\n"
                                        "If not specified, payoutAddress is the one that is going to be used.\n"
                                        "The private key belonging to this address must be known in your wallet."},
                    {"submit", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED, "If true, the resulting transaction is sent to the network."},
                },
                RPCResult{RPCResult::Type::STR_HEX, "hex", "txid"},
                RPCExamples{
                    HelpExampleCli("protx_register", "1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d 0 173.249.49.9:18369 tsys1q2j57a4rtserh9022a63pvk3jqmg7un55stux0v 003bc97fcd6023996f8703b4da34dedd1641bd45ed12ac7a4d74a529dd533ecb99d4fb8ddb04853bb110f0d747ee8e63 tsys1qxh8am0c9w0q9kv7h7f9q2c4jrfjg63yawrgm0r 5 tsys1qxh8am0c9w0q9kv7h7f9q2c4jrfjg63yawrgm0r")
                + HelpExampleRpc("protx_register", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\", 0, \"173.249.49.9:18369\", \"tsys1q2j57a4rtserh9022a63pvk3jqmg7un55stux0v\", \"003bc97fcd6023996f8703b4da34dedd1641bd45ed12ac7a4d74a529dd533ecb99d4fb8ddb04853bb110f0d747ee8e63\", \"tsys1qxh8am0c9w0q9kv7h7f9q2c4jrfjg63yawrgm0r\", 5, \"tsys1qxh8am0c9w0q9kv7h7f9q2c4jrfjg63yawrgm0r\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();
    EnsureWalletIsUnlocked(*pwallet);
    
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    size_t paramIdx = 0;


    CMutableTransaction tx;
    tx.nVersion = SYSCOIN_TX_VERSION_MN_REGISTER;

    CProRegTx ptx;
    ptx.nVersion = CProRegTx::CURRENT_VERSION;

    uint256 collateralHash = ParseHashV(request.params[paramIdx], "collateralHash");
    int32_t collateralIndex = request.params[paramIdx + 1].get_int();
    if (collateralHash.IsNull() || collateralIndex < 0) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("invalid hash or index: %s-%d", collateralHash.ToString(), collateralIndex));
    }

    ptx.collateralOutpoint = COutPoint(collateralHash, (uint32_t)collateralIndex);
    paramIdx += 2;
    CTxDestination fundDest;
    { 
        // TODO unlock on failure
        LOCK(pwallet->cs_wallet);
        pwallet->LockCoin(ptx.collateralOutpoint);


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

        // make sure fee calculation works
        ptx.vchSig.resize(65);


        fundDest = payoutDest;
        if (!request.params[paramIdx + 6].isNull()) {
            fundDest = DecodeDestination(request.params[paramIdx + 6].get_str());
            if (!IsValidDestination(fundDest))
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Syscoin address: ") + request.params[paramIdx + 6].get_str());
        }
    }
    bool fSubmit{true};
    if (!request.params[paramIdx + 7].isNull()) {
        fSubmit = request.params[paramIdx + 7].get_bool();
    }
    FundSpecialTx(*pwallet, tx, ptx, fundDest);
    UpdateSpecialTxInputsHash(tx, ptx);


    // referencing external collateral

    Coin coin;
    if (!GetUTXOCoin(*node.chainman, ptx.collateralOutpoint, coin)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("collateral not found: %s", ptx.collateralOutpoint.ToStringShort()));
    }
    CTxDestination txDest;
    ExtractDestination(coin.out.scriptPubKey, txDest);
    CKeyID keyID;
    if (auto witness_id = std::get_if<WitnessV0KeyHash>(&txDest)) {	
        keyID = ToKeyID(*witness_id);
    }	
    else if (auto key_id = std::get_if<PKHash>(&txDest)) {	
        keyID = ToKeyID(*key_id);
    }	
    if (keyID.IsNull()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("collateral type not supported: %s", ptx.collateralOutpoint.ToStringShort()));
    }

    CKey key;
    {
        // lets prove we own the collateral
        LegacyScriptPubKeyMan& spk_man = EnsureLegacyScriptPubKeyMan(*pwallet);
        LOCK2(pwallet->cs_wallet, spk_man.cs_KeyStore);
        if (!spk_man.GetKey(keyID, key)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("collateral key not in wallet: %s", EncodeDestination(txDest)));
        }
    }
    SignSpecialTxPayloadByString(tx, ptx, key);
    SetTxPayload(tx, ptx);
    return SignAndSendSpecialTx(request,  tx, fSubmit);
},
    };
}
    
static RPCHelpMan protx_register_fund()
{
        return RPCHelpMan{"protx_register_fund",
                "\nCreates, funds and sends a ProTx to the network. The resulting transaction will move 100000 Syscoin\n"
                "to the address specified by collateralAddress and will then function as the collateral of your\n"
                "masternode.\n",
                {
                    {"collateralAddress", RPCArg::Type::STR, RPCArg::Optional::NO, "The Syscoin address to send the collateral to."},
                    {"ipAndPort", RPCArg::Type::STR, RPCArg::Optional::NO, "IP and port in the form \"IP:PORT\".\n"
                                        "Must be unique on the network. Can be set to 0, which will require a ProUpServTx afterwards."},
                    {"ownerAddress", RPCArg::Type::STR, RPCArg::Optional::NO, "The Syscoin address to use for payee updates and proposal voting.\n"
                                        "The corresponding private key does not have to be known by your wallet.\n"
                                        "The address must be unused and must differ from the collateralAddress."},
                    {"operatorPubKey", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The operator BLS public key. The BLS private key does not have to be known.\n"
                                        "It has to match the BLS private key which is later used when operating the masternode."},
                    {"votingAddress", RPCArg::Type::STR, RPCArg::Optional::NO, "The voting key address. The private key does not have to be known by your wallet.\n"
                                        "It has to match the private key which is later used when voting on proposals.\n"
                                        "If set to an empty string, ownerAddress will be used.\n"},
                    {"operatorReward", RPCArg::Type::NUM, RPCArg::Optional::NO, "The fraction in %% to share with the operator. The value must be\n"
                                        "between 0.00 and 100.00."},
                    {"payoutAddress", RPCArg::Type::STR, RPCArg::Optional::NO, "The Syscoin address to use for masternode reward payments."},
                    {"fundAddress", RPCArg::Type::STR, RPCArg::Default{""}, "If specified wallet will only use coins from this address to fund ProTx.\n"
                                        "If not specified, payoutAddress is the one that is going to be used.\n"
                                        "The private key belonging to this address must be known in your wallet."},
                    {"submit", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED, "If true, the resulting transaction is sent to the network."},
                },
                RPCResult{RPCResult::Type::ANY, "hash", "txid"},
                RPCExamples{
                    HelpExampleCli("protx_register_fund", "\"tsys1qxh8am0c9w0q9kv7h7f9q2c4jrfjg63yawrgm0r\" 173.249.49.9:18369 tsys1q2j57a4rtserh9022a63pvk3jqmg7un55stux0v 003bc97fcd6023996f8703b4da34dedd1641bd45ed12ac7a4d74a529dd533ecb99d4fb8ddb04853bb110f0d747ee8e63 tsys1qxh8am0c9w0q9kv7h7f9q2c4jrfjg63yawrgm0r 5 tsys1qxh8am0c9w0q9kv7h7f9q2c4jrfjg63yawrgm0r")
            + HelpExampleRpc("protx_register_fund", "\"tsys1qxh8am0c9w0q9kv7h7f9q2c4jrfjg63yawrgm0r\", \"173.249.49.9:18369\", \"tsys1q2j57a4rtserh9022a63pvk3jqmg7un55stux0v\", \"003bc97fcd6023996f8703b4da34dedd1641bd45ed12ac7a4d74a529dd533ecb99d4fb8ddb04853bb110f0d747ee8e63\", \"tsys1qxh8am0c9w0q9kv7h7f9q2c4jrfjg63yawrgm0r\", 5, \"tsys1qxh8am0c9w0q9kv7h7f9q2c4jrfjg63yawrgm0r\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();
    EnsureWalletIsUnlocked(*pwallet);
    

    size_t paramIdx = 0;


    CMutableTransaction tx;
    tx.nVersion = SYSCOIN_TX_VERSION_MN_REGISTER;

    CProRegTx ptx;
    ptx.nVersion = CProRegTx::CURRENT_VERSION;


    CTxDestination collateralDest = DecodeDestination(request.params[paramIdx].get_str());
    if (!IsValidDestination(collateralDest)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("invalid collaterall address: %s", request.params[paramIdx].get_str()));
    }
    CScript collateralScript = GetScriptForDestination(collateralDest);

    CTxOut collateralTxOut(nMNCollateralRequired, collateralScript);
    tx.vout.emplace_back(collateralTxOut);

    paramIdx++;


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


    CTxDestination fundDest = payoutDest;
    if (!request.params[paramIdx + 6].isNull()) {
        fundDest = DecodeDestination(request.params[paramIdx + 6].get_str());
        if (!IsValidDestination(fundDest))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Syscoin address: ") + request.params[paramIdx + 6].get_str());
    }

    FundSpecialTx(*pwallet, tx, ptx, fundDest);
    UpdateSpecialTxInputsHash(tx, ptx);

    bool fSubmit{true};
    if (!request.params[paramIdx + 7].isNull()) {
        fSubmit = request.params[paramIdx + 7].get_bool();
    }

    uint32_t collateralIndex = (uint32_t) -1;
    for (uint32_t i = 0; i < tx.vout.size(); i++) {
        if (tx.vout[i].nValue == nMNCollateralRequired) {
            collateralIndex = i;
            break;
        }
    }
    CHECK_NONFATAL(collateralIndex != (uint32_t) -1);
    ptx.collateralOutpoint.n = collateralIndex;

    SetTxPayload(tx, ptx);
    return SignAndSendSpecialTx(request, tx, fSubmit);
},
    };
}  
static RPCHelpMan protx_register_prepare()
{
    return RPCHelpMan{"protx_register_prepare",
            "\nCreates an unsigned ProTx and returns it. The ProTx must be signed externally with the collateral\n"
            "key and then passed to \"protx_register_submit\". The prepared transaction will also contain inputs\n"
            "and outputs to cover fees.\n",
            {
                {"collateralHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The collateral transaction hash."},
                {"collateralIndex", RPCArg::Type::NUM, RPCArg::Optional::NO, "The collateral transaction output index."},
                {"ipAndPort", RPCArg::Type::STR, RPCArg::Optional::NO, "IP and port in the form \"IP:PORT\".\n"
                                    "Must be unique on the network. Can be set to 0, which will require a ProUpServTx afterwards."},
                {"ownerAddress", RPCArg::Type::STR, RPCArg::Optional::NO, "The Syscoin address to use for payee updates and proposal voting.\n"
                                    "The corresponding private key does not have to be known by your wallet.\n"
                                    "The address must be unused and must differ from the collateralAddress."},
                {"operatorPubKey", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The operator BLS public key. The BLS private key does not have to be known.\n"
                                    "It has to match the BLS private key which is later used when operating the masternode."},
                {"votingAddress", RPCArg::Type::STR, RPCArg::Optional::NO, "The voting key address. The private key does not have to be known by your wallet.\n"
                                    "It has to match the private key which is later used when voting on proposals.\n"
                                    "If set to an empty string, ownerAddress will be used.\n"},
                {"operatorReward", RPCArg::Type::NUM, RPCArg::Optional::NO, "The fraction in %% to share with the operator. The value must be\n"
                                    "between 0.00 and 100.00."},
                {"payoutAddress", RPCArg::Type::STR, RPCArg::Optional::NO, "The Syscoin address to use for masternode reward payments."},
                {"fundAddress", RPCArg::Type::STR, RPCArg::Default{""}, "If specified wallet will only use coins from this address to fund ProTx.\n"
                                    "If not specified, payoutAddress is the one that is going to be used.\n"
                                    "The private key belonging to this address must be known in your wallet."},
            },
            RPCResult{RPCResult::Type::ANY, "", "Unsigned ProTX transaction object"},
            RPCExamples{
                HelpExampleCli("protx_register_prepare", "1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d 0 173.249.49.9:18369 tsys1q2j57a4rtserh9022a63pvk3jqmg7un55stux0v 003bc97fcd6023996f8703b4da34dedd1641bd45ed12ac7a4d74a529dd533ecb99d4fb8ddb04853bb110f0d747ee8e63 tsys1qxh8am0c9w0q9kv7h7f9q2c4jrfjg63yawrgm0r 5 tsys1qxh8am0c9w0q9kv7h7f9q2c4jrfjg63yawrgm0r")
            + HelpExampleRpc("protx_register_prepare", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\", 0, \"173.249.49.9:18369\", \"tsys1q2j57a4rtserh9022a63pvk3jqmg7un55stux0v\", \"003bc97fcd6023996f8703b4da34dedd1641bd45ed12ac7a4d74a529dd533ecb99d4fb8ddb04853bb110f0d747ee8e63\", \"tsys1qxh8am0c9w0q9kv7h7f9q2c4jrfjg63yawrgm0r\", 5, \"tsys1qxh8am0c9w0q9kv7h7f9q2c4jrfjg63yawrgm0r\"")
            },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();

    size_t paramIdx = 0;


    CMutableTransaction tx;
    tx.nVersion = SYSCOIN_TX_VERSION_MN_REGISTER;

    CProRegTx ptx;
    ptx.nVersion = CProRegTx::CURRENT_VERSION;

    uint256 collateralHash = ParseHashV(request.params[paramIdx], "collateralHash");
    int32_t collateralIndex = request.params[paramIdx + 1].get_int();
    if (collateralHash.IsNull() || collateralIndex < 0) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("invalid hash or index: %s-%d", collateralHash.ToString(), collateralIndex));
    }

    ptx.collateralOutpoint = COutPoint(collateralHash, (uint32_t)collateralIndex);
    paramIdx += 2;
    CTxDestination fundDest;
    {
        // TODO unlock on failure
        LOCK(pwallet->cs_wallet);
        pwallet->LockCoin(ptx.collateralOutpoint);
        

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


        // make sure fee calculation works
        ptx.vchSig.resize(65);
        

        fundDest = payoutDest;
        if (!request.params[paramIdx + 6].isNull()) {
            fundDest = DecodeDestination(request.params[paramIdx + 6].get_str());
            if (!IsValidDestination(fundDest))
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Syscoin address: ") + request.params[paramIdx + 6].get_str());
        }
    }
    FundSpecialTx(*pwallet, tx, ptx, fundDest);
    UpdateSpecialTxInputsHash(tx, ptx);

    // referencing external collateral
    Coin coin;
    if (!GetUTXOCoin(*node.chainman, ptx.collateralOutpoint, coin)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("collateral not found: %s", ptx.collateralOutpoint.ToStringShort()));
    }
    CTxDestination txDest;
    ExtractDestination(coin.out.scriptPubKey, txDest);
    CKeyID keyID;
    if (auto witness_id = std::get_if<WitnessV0KeyHash>(&txDest)) {	
        keyID = ToKeyID(*witness_id);
    }	
    else if (auto key_id = std::get_if<PKHash>(&txDest)) {	
        keyID = ToKeyID(*key_id);
    }	
    if (keyID.IsNull()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("collateral type not supported: %s", ptx.collateralOutpoint.ToStringShort()));
    }
    // external signing with collateral key
    ptx.vchSig.clear();
    SetTxPayload(tx, ptx);
    

    UniValue ret(UniValue::VOBJ);
    ret.pushKV("tx", EncodeHexTx(CTransaction(tx)));
    ret.pushKV("collateralAddress", EncodeDestination(txDest));
    ret.pushKV("signMessage", ptx.MakeSignString());
    return ret;
},
    };
}  

static RPCHelpMan protx_register_submit()
{
   return RPCHelpMan{"protx_register_submit",
            "\nSubmits the specified ProTx to the network. This command will also sign the inputs of the transaction\n"
            "which were previously added by \"protx_register_prepare\" to cover transaction fees\n"
            "and outputs to cover fees.\n",
            {
                {"tx", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The serialized transaction previously returned by \"protx_register_prepare\"."},
                {"sig", RPCArg::Type::STR, RPCArg::Optional::NO, "The signature signed with the collateral key. Must be in base64 format."},
            },
            RPCResult{RPCResult::Type::STR_HEX, "hex", "txid"},
            RPCExamples{
                HelpExampleCli("protx_register_submit", "")
            + HelpExampleRpc("protx_register_submit", "")
            },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{

    CMutableTransaction tx;
    if (!DecodeHexTx(tx, request.params[0].get_str())) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "transaction not deserializable");
    }
    if (tx.nVersion != SYSCOIN_TX_VERSION_MN_REGISTER) {
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
},
    };
}  

static RPCHelpMan protx_update_service()
{

   return RPCHelpMan{"protx_update_service",
            "\nCreates and sends a ProUpServTx to the network. This will update the IP address\n"
            "of a masternode.\n"
            "If this is done for a masternode that got PoSe-banned, the ProUpServTx will also revive this masternode.\n",
            {
                {"proTxHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The hash of the initial ProRegTx."},
                {"ipAndPort", RPCArg::Type::STR, RPCArg::Optional::NO, "IP and port in the form \"IP:PORT\".\n"
                                    "Must be unique on the network. Can be set to 0, which will require a ProUpServTx afterwards."},
                {"operatorKey", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The operator BLS private key associated with the\n"
                                    "registered operator public key."},                   
                {"operatorPayoutAddress", RPCArg::Type::STR_HEX, RPCArg::Default{""}, "The address used for operator reward payments.\n"
                                    "Only allowed when the ProRegTx had a non-zero operatorReward value.\n"
                                    "If set to an empty string, the currently active payout address is reused."}, 
                {"feeSourceAddress", RPCArg::Type::STR, RPCArg::Default{""}, "If specified wallet will only use coins from this address to fund ProTx.\n"
                                    "If not specified, payoutAddress is the one that is going to be used.\n"
                                    "The private key belonging to this address must be known in your wallet."},
            },
            RPCResult{RPCResult::Type::STR_HEX, "hex", "txid"},
            RPCExamples{
                    HelpExampleCli("protx_update_service", "1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d 173.249.49.9:18369 003bc97fcd6023996f8703b4da34dedd1641bd45ed12ac7a4d74a529dd533ecb99d4fb8ddb04853bb110f0d747ee8e63 tsys1qxh8am0c9w0q9kv7h7f9q2c4jrfjg63yawrgm0r tsys1qxh8am0c9w0q9kv7h7f9q2c4jrfjg63yawrgm0r")
                + HelpExampleRpc("protx_update_service", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\", \"173.249.49.9:18369\", \"003bc97fcd6023996f8703b4da34dedd1641bd45ed12ac7a4d74a529dd533ecb99d4fb8ddb04853bb110f0d747ee8e63\", \"tsys1qxh8am0c9w0q9kv7h7f9q2c4jrfjg63yawrgm0r\", \"tsys1qxh8am0c9w0q9kv7h7f9q2c4jrfjg63yawrgm0r\"")
            },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    EnsureWalletIsUnlocked(*pwallet);
    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();

    CProUpServTx ptx;
    ptx.nVersion = CProUpServTx::CURRENT_VERSION;
    ptx.proTxHash = ParseHashV(request.params[0], "proTxHash");

    if (!Lookup(request.params[1].get_str().c_str(), ptx.addr, Params().GetDefaultPort(), false)) {
        throw std::runtime_error(strprintf("invalid network address %s", request.params[1].get_str()));
    }

    CBLSSecretKey keyOperator = ParseBLSSecretKey(request.params[2].get_str(), "operatorKey");
    CDeterministicMNList mnList;
    if(deterministicMNManager)
        deterministicMNManager->GetListAtChainTip(mnList);
    auto dmn = mnList.GetMN(ptx.proTxHash);
    if (!dmn) {
        throw std::runtime_error(strprintf("masternode with proTxHash %s not found", ptx.proTxHash.ToString()));
    }

    if (keyOperator.GetPublicKey() != dmn->pdmnState->pubKeyOperator.Get()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("the operator key does not belong to the registered public key"));
    }

    CMutableTransaction tx;
    tx.nVersion = SYSCOIN_TX_VERSION_MN_UPDATE_SERVICE;

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
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Syscoin address: ") + request.params[4].get_str());
    } else {
        if (ptx.scriptOperatorPayout != CScript()) {
            // use operator reward address as default source for fees
            ExtractDestination(ptx.scriptOperatorPayout, feeSource);
        } else {
            // use payout address as default source for fees
            ExtractDestination(dmn->pdmnState->scriptPayout, feeSource);
        }
    }

    FundSpecialTx(*pwallet, tx, ptx, feeSource);

    SignSpecialTxPayloadByHash(tx, ptx, keyOperator);
    SetTxPayload(tx, ptx);

    return SignAndSendSpecialTx(request, tx);
},
    };
}  

static RPCHelpMan protx_update_registrar()
{
        return RPCHelpMan{"protx_update_registrar",
            "\nCreates and sends a ProUpRegTx to the network. This will update the operator key, voting key and payout\n"
            "address of the masternode specified by \"proTxHash\".\n"
            "The owner key of the masternode must be known to your wallet.\n",
            {
                {"proTxHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The hash of the initial ProRegTx."},
                {"operatorPubKey", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The operator BLS public key. The BLS private key does not have to be known.\n"
                                "It has to match the BLS private key which is later used when operating the masternode.\n"
                                "If set to an empty string, the currently active operator BLS public key is reused."},                   
                {"votingAddress", RPCArg::Type::STR, RPCArg::Optional::NO, "The voting key address. The private key does not have to be known by your wallet.\n"
                                "It has to match the private key which is later used when voting on proposals.\n"
                                "If set to an empty string, the currently active voting key address is reused."}, 
                {"payoutAddress", RPCArg::Type::STR, RPCArg::Optional::NO, "The Syscoin address to use for masternode reward payments.\n"
                                 "If set to an empty string, the currently active payout address is reused."}, 
                {"feeSourceAddress", RPCArg::Type::STR, RPCArg::Default{""}, "If specified wallet will only use coins from this address to fund ProTx.\n"
                                    "If not specified, payoutAddress is the one that is going to be used.\n"
                                    "The private key belonging to this address must be known in your wallet."},
            },
            RPCResult{RPCResult::Type::STR_HEX, "hex", "txid"},
            RPCExamples{
                    HelpExampleCli("protx_update_registrar", "1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d 003bc97fcd6023996f8703b4da34dedd1641bd45ed12ac7a4d74a529dd533ecb99d4fb8ddb04853bb110f0d747ee8e63 tsys1qxh8am0c9w0q9kv7h7f9q2c4jrfjg63yawrgm0r tsys1qxh8am0c9w0q9kv7h7f9q2c4jrfjg63yawrgm0r")
                + HelpExampleRpc("protx_update_registrar", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\", \"003bc97fcd6023996f8703b4da34dedd1641bd45ed12ac7a4d74a529dd533ecb99d4fb8ddb04853bb110f0d747ee8e63\", \"tsys1qxh8am0c9w0q9kv7h7f9q2c4jrfjg63yawrgm0r\", \"tsys1qxh8am0c9w0q9kv7h7f9q2c4jrfjg63yawrgm0r\"")
            },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();
    EnsureWalletIsUnlocked(*pwallet);

    CProUpRegTx ptx;
    ptx.nVersion = CProUpRegTx::CURRENT_VERSION;
    ptx.proTxHash = ParseHashV(request.params[0], "proTxHash");
    CDeterministicMNList mnList;
    if(deterministicMNManager)
        deterministicMNManager->GetListAtChainTip(mnList);
    auto dmn = mnList.GetMN(ptx.proTxHash);
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
    
    

    CKey keyOwner;
    {
        LegacyScriptPubKeyMan& spk_man = EnsureLegacyScriptPubKeyMan(*pwallet);
        LOCK2(pwallet->cs_wallet, spk_man.cs_KeyStore);
        if (!spk_man.GetKey(dmn->pdmnState->keyIDOwner, keyOwner)) {
            throw std::runtime_error(strprintf("Private key for owner address %s not found in your wallet", EncodeDestination(WitnessV0KeyHash(dmn->pdmnState->keyIDOwner))));
        }
    }

    CMutableTransaction tx;
    tx.nVersion = SYSCOIN_TX_VERSION_MN_UPDATE_REGISTRAR;

    // make sure we get anough fees added
    ptx.vchSig.resize(65);

    CTxDestination feeSourceDest = payoutDest;
    if (!request.params[4].isNull()) {
        feeSourceDest = DecodeDestination(request.params[4].get_str());
        if (!IsValidDestination(feeSourceDest))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Syscoin address: ") + request.params[5].get_str());
    }

    FundSpecialTx(*pwallet, tx, ptx, feeSourceDest);
    SignSpecialTxPayloadByHash(tx, ptx, keyOwner);
    SetTxPayload(tx, ptx);

    return SignAndSendSpecialTx(request, tx);
},
    };
}  


static RPCHelpMan protx_revoke()
{
        return RPCHelpMan{"protx_revoke",
            "\nCreates and sends a ProUpRevTx to the network. This will revoke the operator key of the masternode and\n"
            "put it into the PoSe-banned state. It will also set the service field of the masternode\n"
            "to zero. Use this in case your operator key got compromised or you want to stop providing your service\n"
            "to the masternode owner.\n",
            {
                {"proTxHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The hash of the initial ProRegTx."},
                {"operatorKey", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The operator BLS private key associated with the\n"
                                    "registered operator public key."},                   
                {"reason", RPCArg::Type::NUM, RPCArg::Default{0}, "The reason for masternode service revocation."},   
                {"feeSourceAddress", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "If specified wallet will only use coins from this address to fund ProTx.\n"
                                    "If not specified, payoutAddress is the one that is going to be used.\n"
                                    "The private key belonging to this address must be known in your wallet."},
            },
            RPCResult{RPCResult::Type::STR_HEX, "hex", "txid"},
            RPCExamples{
                    HelpExampleCli("protx_update_registrar", "1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d 003bc97fcd6023996f8703b4da34dedd1641bd45ed12ac7a4d74a529dd533ecb99d4fb8ddb04853bb110f0d747ee8e63 0 tsys1qxh8am0c9w0q9kv7h7f9q2c4jrfjg63yawrgm0r")
                + HelpExampleRpc("protx_update_registrar", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\", \"003bc97fcd6023996f8703b4da34dedd1641bd45ed12ac7a4d74a529dd533ecb99d4fb8ddb04853bb110f0d747ee8e63\", 0, \"tsys1qxh8am0c9w0q9kv7h7f9q2c4jrfjg63yawrgm0r\"")
            },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    EnsureWalletIsUnlocked(*pwallet);

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();

    CProUpRevTx ptx;
    ptx.nVersion = CProUpRevTx::CURRENT_VERSION;
    ptx.proTxHash = ParseHashV(request.params[0], "proTxHash");

    CBLSSecretKey keyOperator = ParseBLSSecretKey(request.params[1].get_str(), "operatorKey");

    if (!request.params[2].isNull()) {
        int32_t nReason = request.params[2].get_int();
        if (nReason < 0 || nReason > CProUpRevTx::REASON_LAST) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("invalid reason %d, must be between 0 and %d", nReason, CProUpRevTx::REASON_LAST));
        }
        ptx.nReason = (uint16_t)nReason;
    }
    CDeterministicMNList mnList;
    if(deterministicMNManager)
        deterministicMNManager->GetListAtChainTip(mnList);
    auto dmn = mnList.GetMN(ptx.proTxHash);
    if (!dmn) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("masternode %s not found", ptx.proTxHash.ToString()));
    }

    if (keyOperator.GetPublicKey() != dmn->pdmnState->pubKeyOperator.Get()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("the operator key does not belong to the registered public key"));
    }

    CMutableTransaction tx;
    tx.nVersion = SYSCOIN_TX_VERSION_MN_UPDATE_REVOKE;

    if (!request.params[3].isNull()) {
        CTxDestination feeSourceDest = DecodeDestination(request.params[3].get_str());
        if (!IsValidDestination(feeSourceDest))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Syscoin address: ") + request.params[3].get_str());
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

    SignSpecialTxPayloadByHash(tx, ptx, keyOperator);
    SetTxPayload(tx, ptx);

    return SignAndSendSpecialTx(request, tx);
},
    };
} 


static bool CheckWalletOwnsKey(CWallet* pwallet, const CKeyID& keyID) {
    if (!pwallet) {
        return false;
    }
    LegacyScriptPubKeyMan& spk_man = EnsureLegacyScriptPubKeyMan(*pwallet);
    LOCK2(pwallet->cs_wallet, spk_man.cs_KeyStore);
    return spk_man.IsMine(GetScriptForDestination(CTxDestination(WitnessV0KeyHash(keyID))));
}

static bool CheckWalletOwnsScript(CWallet* pwallet, const CScript& script) {
    if (!pwallet) {
        return false;
    }
    LegacyScriptPubKeyMan& spk_man = EnsureLegacyScriptPubKeyMan(*pwallet);
    LOCK2(pwallet->cs_wallet, spk_man.cs_KeyStore);
    return spk_man.IsMine(script);
}
UniValue BuildDMNListEntry(const NodeContext& node, CWallet* pwallet, const CDeterministicMNCPtr& dmn, int detailed)
{
    if (!detailed) {
        return dmn->proTxHash.ToString();
    }
    UniValue o(UniValue::VOBJ);
    if(detailed == 1) {
        const CTxDestination &voteDest = WitnessV0KeyHash(dmn->pdmnState->keyIDVoting);
        o.pushKV("collateralHash", dmn->collateralOutpoint.hash.ToString());
        o.pushKV("collateralIndex", (int)dmn->collateralOutpoint.n);
        o.pushKV("collateralHeight", dmn->pdmnState->nCollateralHeight);
        o.pushKV("votingAddress", EncodeDestination(voteDest));
        if(pwallet) {
            LegacyScriptPubKeyMan& spk_man = EnsureLegacyScriptPubKeyMan(*pwallet);
            LOCK2(pwallet->cs_wallet, spk_man.cs_KeyStore);
            CKey keyVoting;
            spk_man.GetKey(dmn->pdmnState->keyIDVoting, keyVoting);
            o.pushKV("votingKey", EncodeSecret(keyVoting));
            const auto* address_book_entry = pwallet->FindAddressBookEntry(voteDest);
            if (address_book_entry) {
                o.pushKV("label", address_book_entry->GetLabel());
            }
        }
        return o;
    } else if(detailed >= 2) {
        dmn->ToJson(*node.chainman, o);
        int confirmations = GetUTXOConfirmations(*node.chainman,dmn->collateralOutpoint);
        o.pushKV("confirmations", confirmations);
        if (pwallet) {
            LOCK2(pwallet->cs_wallet, cs_main);
            bool hasOwnerKey = CheckWalletOwnsKey(pwallet, dmn->pdmnState->keyIDOwner);
            bool hasVotingKey = CheckWalletOwnsKey(pwallet, dmn->pdmnState->keyIDVoting);

            bool ownsCollateral = false;
            CTransactionRef collateralTx;
            uint256 tmpHashBlock;
            uint32_t nBlockHeight;
            CBlockIndex* blockindex = nullptr;
            if(pblockindexdb->ReadBlockHeight(dmn->collateralOutpoint.hash, nBlockHeight)){	    
                blockindex = node.chainman->ActiveChain()[nBlockHeight];
            } 
            collateralTx = GetTransaction(blockindex, node.mempool.get(), dmn->collateralOutpoint.hash, Params().GetConsensus(), tmpHashBlock);
            if(collateralTx)
                ownsCollateral = CheckWalletOwnsScript(pwallet, collateralTx->vout[dmn->collateralOutpoint.n].scriptPubKey);
        
            UniValue walletObj(UniValue::VOBJ);
            walletObj.pushKV("hasOwnerKey", hasOwnerKey);
            walletObj.pushKV("hasOperatorKey", false);
            walletObj.pushKV("hasVotingKey", hasVotingKey);
            walletObj.pushKV("ownsCollateral", ownsCollateral);
            walletObj.pushKV("ownsPayeeScript", CheckWalletOwnsScript(pwallet, dmn->pdmnState->scriptPayout));
            walletObj.pushKV("ownsOperatorRewardScript", CheckWalletOwnsScript(pwallet, dmn->pdmnState->scriptOperatorPayout));
            o.pushKV("wallet", walletObj);
        }

        auto metaInfo = mmetaman.GetMetaInfo(dmn->proTxHash);
        o.pushKV("metaInfo", metaInfo->ToJson());
    }

    return o;
}

static RPCHelpMan protx_list_wallet()
{
    return RPCHelpMan{"protx_list_wallet",
        "\nList only ProTx which are found in your wallet at the given chain height.\n"
        "This will also include ProTx which failed PoSe verification.\n",
        {
            {"detailed", RPCArg::Type::NUM, RPCArg::Default{0}, "If 0, only the hashes of the ProTx will be returned. If 1 returns voting details for each DMN and keys and if 2 returns full details of each DMN"},
            {"height", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "Height to look for ProTx transactions, if not specified defaults to current chain-tip"},                   
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
                HelpExampleCli("protx_list_wallet", "true")
            + HelpExampleRpc("protx_list_wallet", "true")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    CWallet* pwallet = nullptr;
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (wallet)
        pwallet = wallet.get();

    const NodeContext& node = EnsureAnyNodeContext(request.context);
    UniValue ret(UniValue::VARR);

    if (!pwallet) {
        throw std::runtime_error("\"protx_list_wallet\" not supported when wallet is disabled");
    }
    LOCK2(pwallet->cs_wallet, cs_main);

    int detailed = !request.params[0].isNull() ? request.params[0].get_int() : 0;

    int height = !request.params[1].isNull() ? request.params[1].get_int() : node.chainman->ActiveHeight();
    if (height < 1 || height > node.chainman->ActiveHeight()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid height specified");
    }

    std::vector<COutPoint> vOutpts;
    pwallet->ListProTxCoins(vOutpts);
    std::set<COutPoint> setOutpts;
    for (const auto& outpt : vOutpts) {
        setOutpts.emplace(outpt);
    }
    CDeterministicMNList mnList;
    if(deterministicMNManager)
        deterministicMNManager->GetListForBlock(node.chainman->ActiveChain()[height], mnList);
    mnList.ForEachMN(false, [&](const CDeterministicMNCPtr& dmn) {
        if (setOutpts.count(dmn->collateralOutpoint) ||
            CheckWalletOwnsKey(pwallet, dmn->pdmnState->keyIDOwner) ||
            CheckWalletOwnsKey(pwallet, dmn->pdmnState->keyIDVoting) ||
            CheckWalletOwnsScript(pwallet, dmn->pdmnState->scriptPayout) ||
            CheckWalletOwnsScript(pwallet, dmn->pdmnState->scriptOperatorPayout)) {
            ret.push_back(BuildDMNListEntry(node, pwallet, dmn, detailed));
        }
    });
    return ret;
},
    };
} 

static RPCHelpMan protx_info_wallet()
{
    return RPCHelpMan{"protx_info_wallet",
        "\nReturns detailed information about a deterministic masternode in current wallet.\n",
        {
            {"proTxHash", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The hash of the initial ProRegTx."},                 
        },
        RPCResult{RPCResult::Type::ANY, "", ""},
        RPCExamples{
                HelpExampleCli("protx_info_wallet", "1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d")
            + HelpExampleRpc("protx_info_wallet", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
        },
    [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    CWallet* pwallet = nullptr;
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (wallet)
        pwallet = wallet.get();
    const NodeContext& node = EnsureAnyNodeContext(request.context);
    uint256 proTxHash = ParseHashV(request.params[0], "proTxHash");
    CDeterministicMNList mnList;
    if(deterministicMNManager)
        deterministicMNManager->GetListAtChainTip(mnList);
    auto dmn = mnList.GetMN(proTxHash);
    if (!dmn) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s not found", proTxHash.ToString()));
    }
    return BuildDMNListEntry(node, pwallet, dmn, 2);
},
    };
} 

Span<const CRPCCommand> GetEvoWalletRPCCommands()
{
// clang-format off
static const CRPCCommand commands[] =
{ //  category              name                      actor (function)
  //  --------------------- ------------------------  -----------------------
    { "evowallet",              &protx_list_wallet,                 },
    { "evowallet",              &protx_info_wallet,                 },
    { "evowallet",              &protx_register,                    },
    { "evowallet",              &protx_register_fund,               },
    { "evowallet",              &protx_register_prepare,            },
    { "evowallet",              &protx_register_submit,             },
    { "evowallet",              &protx_update_service,              },
    { "evowallet",              &protx_update_registrar,            },
    { "evowallet",              &protx_revoke,                      },
};
// clang-format on
    return MakeSpan(commands);
}