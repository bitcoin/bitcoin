// Copyright (c) 2018 The Dash Core developers
// Copyright (c) 2019 The BitGreen Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <base58.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <init.h>
#include <key_io.h>
#include <messagesigner.h>
#include <rpc/register.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <special/util.h>
#include <util/moneystr.h>
#include <util/validation.h>
#include <validation.h>
#include <wallet/coincontrol.h>
#include <wallet/rpcwallet.h>

#ifdef ENABLE_WALLET
#include <wallet/wallet.h>
#endif//ENABLE_WALLET

#include <netbase.h>

#include <special/deterministicmns.h>
#include <special/specialtx.h>
#include <special/providertx.h>

#ifdef ENABLE_WALLET
// extern UniValue signrawtransactionwithwallet(const JSONRPCRequest& request);
extern UniValue sendrawtransaction(const JSONRPCRequest& request);
#endif//ENABLE_WALLET

RPCArg GetHelpArg(std::string strParamName)
{
    static const std::map<std::string, RPCArg> mapParamHelp = {
        {"collateralAddress",
            {"collateralAddress", RPCArg::Type::STR, RPCArg::Optional::NO, "The bitgreen address to send the collateral to"},
        },
        {"collateralHash",
            {"collateralHash", RPCArg::Type::STR, RPCArg::Optional::NO, "The collateral transaction hash."}
        },
        {"collateralIndex",
            {"collateralIndex", RPCArg::Type::NUM, RPCArg::Optional::NO, "The collateral transaction output index."}
        },
        {"feeSourceAddress",
            {"feeSourceAddress", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "If specified wallet will only use coins from this address to fund ProTx.\n"
            "                              If not specified, payoutAddress is the one that is going to be used.\n"
            "                              The private key belonging to this address must be known in your wallet."}
        },
        {"fundAddress",
            {"fundAddress", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "If specified wallet will only use coins from this address to fund ProTx.\n"
            "                              If not specified, payoutAddress is the one that is going to be used.\n"
            "                              The private key belonging to this address must be known in your wallet."}
        },
        {"ipAndPort",
            {"ipAndPort", RPCArg::Type::STR, RPCArg::Optional::NO, "IP and port in the form \"IP:PORT\".\n"
            "                              Must be unique on the network. Can be set to 0, which will require a ProUpServTx afterwards."}
        },
        {"operatorKey",
            {"operatorKey", RPCArg::Type::STR, RPCArg::Optional::NO, "The operator private key belonging to the\n"
            "                              registered operator public key."}
        },
        {"operatorPayoutAddress",
            {"operatorPayoutAddress", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "The address used for operator reward payments.\n"
            "                              Only allowed when the ProRegTx had a non-zero operatorReward value.\n"
            "                              If set to an empty string, the currently active payout address is reused."}
        },
        {"operatorPubKey",
            {"operatorPubKey", RPCArg::Type::STR, RPCArg::Optional::NO, "The operator BLS public key. The private key does not have to be known.\n"
            "                              It has to match the private key which is later used when operating the masternode."}
        },
        {"operatorReward",
            {"operatorReward", RPCArg::Type::NUM, RPCArg::Optional::NO, "The fraction in %% to share with the operator. The value must be\n"
            "                              between 0.00 and 100.00."}
        },
        {"ownerAddress",
            {"ownerAddress", RPCArg::Type::STR, RPCArg::Optional::NO, "The bitgreen address to use for payee updates and proposal voting.\n"
            "                              The private key belonging to this address must be known in your wallet. The address must\n"
            "                              be unused and must differ from the collateralAddress."}
        },
        {"payoutAddress",
            {"payoutAddress", RPCArg::Type::STR, RPCArg::Optional::NO, "The bitgreen address to use for masternode reward payments."}
        },
        {"proTxHash",
            {"proTxHash", RPCArg::Type::STR, RPCArg::Optional::NO, "The hash of the initial ProRegTx."}
        },
        {"reason",
            {"reason", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "The reason for masternode service revocation."}
        },
        {"votingAddress",
            {"votingAddress", RPCArg::Type::STR, RPCArg::Optional::NO, "The voting key address. The private key does not have to be known by your wallet.\n"
            "                              It has to match the private key which is later used when voting on proposals.\n"
            "                              If set to an empty string, ownerAddress will be used."}
        },
    };

    auto it = mapParamHelp.find(strParamName);
    if (it == mapParamHelp.end())
        throw std::runtime_error(strprintf("FIXME: WRONG PARAM NAME %s!", strParamName));

    return it->second;
}

// Allows to specify BitGreen address or priv key. In case of BitGreen address, the priv key is taken from the wallet
static CKey ParsePrivKey(const CWallet* pwallet, const std::string &strKeyOrAddress, bool allowAddresses = true) {
    CTxDestination address = DecodeDestination(strKeyOrAddress);
    if (allowAddresses && IsValidDestination(address)) {
#ifdef ENABLE_WALLET
        if (!pwallet) {
            throw std::runtime_error("Addresses not supported when wallet is disabled");
        }
        EnsureWalletIsUnlocked(pwallet);
        CKeyID keyId;
        CKey key;
        keyId = GetKeyForDestination(*pwallet, address);
        if (keyId.IsNull()) {
            throw JSONRPCError(RPC_TYPE_ERROR, "Address does not refer to a key");
        }
        if (!pwallet->GetKey(keyId, key)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Private key for address " + strKeyOrAddress + " is not known");
        }
        return key;
#else//ENABLE_WALLET
        throw std::runtime_error("Addresses not supported in no-wallet builds");
#endif//ENABLE_WALLET
    }

    CKey secret = DecodeSecret(strKeyOrAddress);
    if (!secret.IsValid()) {
        throw std::runtime_error(strprintf("Invalid priv-key/address %s", strKeyOrAddress));
    }
    return secret;
}

static CKeyID ParsePubKeyIDFromAddress(const CWallet* pwallet, const std::string& strAddress, const std::string& paramName)
{
    CTxDestination address = DecodeDestination(strAddress);
    if (!IsValidDestination(address))
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s must be a valid P2PKH address, not %s", paramName, strAddress));

    CKeyID keyID = GetKeyForDestination(*pwallet, address);
    if (keyID.IsNull())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("%s does not refer to a key", strAddress));

    return keyID;
}

static CBLSPublicKey ParseBLSPubKey(const std::string& hexKey, const std::string& paramName)
{
    CBLSPublicKey pubKey;
    if (!pubKey.SetHexStr(hexKey))
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s must be a valid BLS public key, not %s", paramName, hexKey));
    return pubKey;
}

static CBLSSecretKey ParseBLSSecretKey(const std::string& hexKey, const std::string& paramName)
{
    CBLSSecretKey secKey;
    if (!secKey.SetHexStr(hexKey))
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s must be a valid BLS secret key", paramName));
    return secKey;
}

#ifdef ENABLE_WALLET

template<typename SpecialTxPayload>
static void FundSpecialTx(CWallet* pwallet, CMutableTransaction& tx, const SpecialTxPayload& payload, const CTxDestination& fundDest)
{
    assert(pwallet != nullptr);
    LOCK2(cs_main, pwallet->cs_wallet);

    CTxDestination nodest = CNoDestination();
    if (fundDest == nodest)
        throw JSONRPCError(RPC_INTERNAL_ERROR, "No source of funds specified");

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

    CCoinControl coin_control;
    coin_control.destChange = fundDest;
    // coin_control.fRequireAllInputs = false;

    std::vector<COutput> vecOutputs;
    auto locked_chain = pwallet->chain().lock();
    LOCK(pwallet->cs_wallet);

    pwallet->AvailableCoins(*locked_chain, vecOutputs);

    for (const auto& out : vecOutputs) {
        CTxDestination txDest;
        if (ExtractDestination(out.tx->tx->vout[out.i].scriptPubKey, txDest) && txDest == fundDest) {
            coin_control.Select(COutPoint(out.tx->tx->GetHash(), out.i));
        }
    }

    if (!coin_control.HasSelected())
        throw JSONRPCError(RPC_INTERNAL_ERROR, "No funds at specified address");

    CTransactionRef wtx;
    CAmount nFeeRequired;
    int nChangePosRet = -1;
    std::string strError;

    if (!pwallet->CreateTransaction(*locked_chain, vecSend, wtx, nFeeRequired, nChangePosRet, strError, coin_control, false))
        throw JSONRPCError(RPC_INTERNAL_ERROR, strError);

    tx.vin = wtx->vin;
    tx.vout = wtx->vout;

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
    const CTransaction& ctx = CTransaction(tx);
    payload.inputsHash = CalcTxInputsHash(ctx);
}

template<typename SpecialTxPayload>
static void SignSpecialTxPayloadByHash(const CMutableTransaction& tx, SpecialTxPayload& payload, const CKey& key)
{
    UpdateSpecialTxInputsHash(tx, payload);
    payload.vchSig.clear();

    uint256 hash = ::SerializeHash(payload);
    if (!CHashSigner::SignHash(hash, key, payload.vchSig))
        throw JSONRPCError(RPC_INTERNAL_ERROR, "failed to sign special tx");
}

template<typename SpecialTxPayload>
static void SignSpecialTxPayloadByString(const CMutableTransaction& tx, SpecialTxPayload& payload, const CKey& key)
{
    UpdateSpecialTxInputsHash(tx, payload);
    payload.vchSig.clear();

    std::string m = payload.MakeSignString();
    if (!CMessageSigner::SignMessage(m, payload.vchSig, key))
        throw JSONRPCError(RPC_INTERNAL_ERROR, "failed to sign special tx");
}

template<typename SpecialTxPayload>
static void SignSpecialTxPayloadByHash(const CMutableTransaction& tx, SpecialTxPayload& payload, const CBLSSecretKey& key)
{
    UpdateSpecialTxInputsHash(tx, payload);

    uint256 hash = ::SerializeHash(payload);
    payload.sig = key.Sign(hash);
}

static std::string SignAndSendSpecialTx(const CMutableTransaction& tx)
{
    UniValue signResult;

    {
        LOCK(cs_main);
        const CTransaction& ctx = CTransaction(tx);

        CValidationState state;
        if (!CheckSpecialTx(ctx, ChainActive().Tip(), state)) {
            throw std::runtime_error(FormatStateMessage(state));
        }

        CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
        ds << tx;

        JSONRPCRequest signRequest;
        signRequest.params.setArray();
        signRequest.params.push_back(HexStr(ds.begin(), ds.end()));

        signResult = signrawtransactionwithwallet(signRequest);
    }

    JSONRPCRequest sendRequest;
    sendRequest.params.setArray();
    sendRequest.params.push_back(signResult["hex"].get_str());
    return sendrawtransaction(sendRequest).get_str();
}

void protx_register_fund_help(const CWallet* pwallet)
{
    throw std::runtime_error(
        RPCHelpMan{"protx register_fund",
            "\nCreates, funds and sends a ProTx to the network. The resulting transaction will move 2500 BITG\n"
            "to the address specified by collateralAddress and will then function as the collateral of your\n"
            "masternode.\n",
            {
                GetHelpArg("collateralAddress"),
                GetHelpArg("ipAndPort"),
                GetHelpArg("ownerAddress"),
                GetHelpArg("operatorPubKey"),
                GetHelpArg("votingAddress"),
                GetHelpArg("operatorReward"),
                GetHelpArg("payoutAddress"),
                GetHelpArg("fundAddress")
            },
            RPCResult{
                "\"txid\"                        (string) The transaction id.\n"
            },
            RPCExamples{
                HelpExampleCli("protx", "register_fund \"XrVhS9LogauRJGJu2sHuryjhpuex4RNPSb\" \"1.2.3.4:1234\" \"Xt9AMWaYSz7tR7Uo7gzXA3m4QmeWgrR3rr\" \"93746e8731c57f87f79b3620a7982924e2931717d49540a85864bd543de11c43fb868fd63e501a1db37e19ed59ae6db4\" \"Xt9AMWaYSz7tR7Uo7gzXA3m4QmeWgrR3rr\" 0 \"XrVhS9LogauRJGJu2sHuryjhpuex4RNPSb\"")
            },
        }.ToString()
    );
}

void protx_register_help(CWallet* const pwallet)
{
    throw std::runtime_error(
        RPCHelpMan{"protx register",
            "\nSame as \"protx register_fund\", but with an externally referenced collateral.\n"
            "The collateral is specified through \"collateralHash\" and \"collateralIndex\" and must be an unspent\n"
            "transaction output spendable by this wallet. It must also not be used by any other masternode.\n"
            + HelpRequiringPassphrase(pwallet) + "\n",
            {
                GetHelpArg("collateralHash"),
                GetHelpArg("collateralIndex"),
                GetHelpArg("ipAndPort"),
                GetHelpArg("ownerAddress"),
                GetHelpArg("operatorPubKey"),
                GetHelpArg("votingAddress"),
                GetHelpArg("operatorReward"),
                GetHelpArg("payoutAddress"),
                GetHelpArg("feeSourceAddress")
            },
            RPCResult{
                "\"txid\"                        (string) The transaction id.\n"
            },
            RPCExamples{
                HelpExampleCli("protx", "register \"0123456701234567012345670123456701234567012345670123456701234567\" 0 \"1.2.3.4:1234\" \"Xt9AMWaYSz7tR7Uo7gzXA3m4QmeWgrR3rr\" \"93746e8731c57f87f79b3620a7982924e2931717d49540a85864bd543de11c43fb868fd63e501a1db37e19ed59ae6db4\" \"Xt9AMWaYSz7tR7Uo7gzXA3m4QmeWgrR3rr\" 0 \"XrVhS9LogauRJGJu2sHuryjhpuex4RNPSb\"")
            }
        }.ToString()
    );
}

void protx_register_prepare_help()
{
    throw std::runtime_error(
        RPCHelpMan{"protx register_prepare",
            "\nCreates an unsigned ProTx and returns it. The ProTx must be signed externally with the collateral\n"
            "key and then passed to \"protx register_submit\". The prepared transaction will also contain inputs\n"
            "and outputs to cover fees.\n",
            {
                GetHelpArg("collateralHash"),
                GetHelpArg("collateralIndex"),
                GetHelpArg("ipAndPort"),
                GetHelpArg("ownerAddress"),
                GetHelpArg("operatorPubKey"),
                GetHelpArg("votingAddress"),
                GetHelpArg("operatorReward"),
                GetHelpArg("payoutAddress"),
                GetHelpArg("feeSourceAddress")
            },
            RPCResult{
                "{                             (json object)\n"
                "  \"tx\" :                      (string) The serialized ProTx in hex format.\n"
                "  \"collateralAddress\" :       (string) The collateral address.\n"
                "  \"signMessage\" :             (string) The string message that needs to be signed with\n"
                "                              the collateral key.\n"
                "}\n"
            },
            RPCExamples{
                HelpExampleCli("protx", "register_prepare \"0123456701234567012345670123456701234567012345670123456701234567\" 0 \"1.2.3.4:1234\" \"Xt9AMWaYSz7tR7Uo7gzXA3m4QmeWgrR3rr\" \"93746e8731c57f87f79b3620a7982924e2931717d49540a85864bd543de11c43fb868fd63e501a1db37e19ed59ae6db4\" \"Xt9AMWaYSz7tR7Uo7gzXA3m4QmeWgrR3rr\" 0 \"XrVhS9LogauRJGJu2sHuryjhpuex4RNPSb\"")
            }
        }.ToString()
    );
}

void protx_register_submit_help(CWallet* const pwallet)
{
    throw std::runtime_error(
        RPCHelpMan{"protx register_submit",
            "\nSubmits the specified ProTx to the network. This command will also sign the inputs of the transaction\n"
            "which were previously added by \"protx register_prepare\" to cover transaction fees\n"
            + HelpRequiringPassphrase(pwallet) + "\n",
            {
                RPCArg{"tx", RPCArg::Type::STR, RPCArg::Optional::NO, "The serialized transaction previously returned by \"protx register_prepare\""},
                RPCArg{"sig", RPCArg::Type::STR, RPCArg::Optional::NO, "The signature signed with the collateral key. Must be in base64 format."}
            },
            RPCResult{
                "\"txid\"                  (string) The transaction id.\n"
            },
            RPCExamples{
                HelpExampleCli("protx", "register_submit \"tx\" \"sig\"")
            }
        }.ToString()
    );
}

// handles register, register_prepare and register_fund in one method
UniValue protx_register(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    bool isExternalRegister = request.params[0].get_str() == "register";
    bool isFundRegister = request.params[0].get_str() == "register_fund";
    bool isPrepareRegister = request.params[0].get_str() == "register_prepare";

    if (isFundRegister && (request.fHelp || (request.params.size() != 8 && request.params.size() != 9)))
        protx_register_fund_help(pwallet);
    else if (isExternalRegister && (request.fHelp || (request.params.size() != 9 && request.params.size() != 10)))
        protx_register_help(pwallet);
    else if (isPrepareRegister && (request.fHelp || (request.params.size() != 9 && request.params.size() != 10)))
        protx_register_prepare_help();

    if (isExternalRegister || isFundRegister)
        EnsureWalletIsUnlocked(pwallet);

    size_t paramIdx = 1;

    CAmount collateralAmount = 2500 * COIN;

    CMutableTransaction tx;
    tx.nVersion = 2;
    tx.nType = TRANSACTION_PROVIDER_REGISTER;

    CProRegTx ptx;
    ptx.nVersion = CProRegTx::CURRENT_VERSION;

    if (isFundRegister) {
        CTxDestination collateralAddress = DecodeDestination(request.params[paramIdx].get_str());
        if (!IsValidDestination(collateralAddress))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("Invalid collateral address: %s", request.params[paramIdx].get_str()));

        CScript collateralScript = GetScriptForDestination(collateralAddress);

        CTxOut collateralTxOut(collateralAmount, collateralScript);
        tx.vout.emplace_back(collateralTxOut);

        paramIdx++;
    } else {
        uint256 collateralHash = ParseHashV(request.params[paramIdx], "collateralHash");
        int collateralIndex = UniValue(UniValue::VNUM, request.params[paramIdx + 1].getValStr()).get_int();
        if (collateralHash.IsNull() || collateralIndex < 0)
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("Invalid hash or index: %s-%d", collateralHash.ToString(), collateralIndex));

        ptx.collateralOutpoint = COutPoint(collateralHash, collateralIndex);
        paramIdx += 2;

        // TODO: unlock on failure
        LOCK(pwallet->cs_wallet);
        pwallet->LockCoin(ptx.collateralOutpoint);
    }

    if (request.params[paramIdx].get_str() != "") {
        if (!Lookup(request.params[paramIdx].get_str().c_str(), ptx.addr, Params().GetDefaultPort(), false)) {
            throw std::runtime_error(strprintf("Invalid network address %s", request.params[paramIdx].get_str()));
        }
    }

    CKey keyOwner = ParsePrivKey(pwallet, request.params[paramIdx + 1].get_str(), true);
    CBLSPublicKey pubKeyOperator = ParseBLSPubKey(request.params[paramIdx + 2].get_str(), "operator BLS address");
    CKeyID keyIDVoting = keyOwner.GetPubKey().GetID();
    if (request.params[paramIdx + 3].get_str() != "") {
        keyIDVoting = ParsePubKeyIDFromAddress(pwallet, request.params[paramIdx + 3].get_str(), "voting address");
    }

    int64_t operatorReward;
    if (!ParseFixedPoint(request.params[paramIdx + 4].getValStr(), 2, &operatorReward)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "OperatorReward must be a number");
    }
    if (operatorReward < 0 || operatorReward > 10000) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "OperatorReward must be between 0.00 and 100.00");
    }
    ptx.nOperatorReward = operatorReward;

    CTxDestination payoutAddress = DecodeDestination(request.params[paramIdx + 5].get_str());
    if (!IsValidDestination(payoutAddress))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("Invalid payout address: %s", request.params[paramIdx + 5].get_str()));

    ptx.keyIDOwner = keyOwner.GetPubKey().GetID();
    ptx.pubKeyOperator = pubKeyOperator;
    ptx.keyIDVoting = keyIDVoting;
    ptx.scriptPayout = GetScriptForDestination(payoutAddress);

    if (!isFundRegister) {
        // make sure fee calculation works
        ptx.vchSig.resize(65);
    }

    CTxDestination fundAddress = payoutAddress;
    if (request.params.size() > paramIdx + 6) {
        fundAddress = DecodeDestination(request.params[paramIdx + 6].get_str());
        if (!IsValidDestination(fundAddress))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid BitGreen address: ") + request.params[paramIdx + 6].get_str());
    }

    FundSpecialTx(pwallet, tx, ptx, fundAddress);
    UpdateSpecialTxInputsHash(tx, ptx);

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
        return SignAndSendSpecialTx(tx);
    } else {
        // referencing external collateral
        Coin coin;
        if (!GetUTXOCoin(ptx.collateralOutpoint, coin))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("collateral not found: %s", ptx.collateralOutpoint.ToString()));

        CTxDestination txDest;
        CKeyID keyID;
        if (!ExtractDestination(coin.out.scriptPubKey, txDest))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("collateral type not supported: %s", ptx.collateralOutpoint.ToString()));

        keyID = GetKeyForDestination(*pwallet, txDest);
        if (keyID.IsNull())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("collateral type not supported: %s", ptx.collateralOutpoint.ToString()));

        if (isPrepareRegister) {
            // external signing with collateral key
            ptx.vchSig.clear();
            SetTxPayload(tx, ptx);

            const CTransaction& ctx = CTransaction(tx);

            UniValue ret(UniValue::VOBJ);
            ret.pushKV("tx", EncodeHexTx(ctx));
            ret.pushKV("collateralAddress", EncodeDestination(txDest));
            ret.pushKV("signMessage", ptx.MakeSignString());
            return ret;
        } else {
            // lets prove we own the collateral
            CKey key;
            if (!pwallet->GetKey(keyID, key))
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("collateral key not in wallet: %s", EncodeDestination(txDest)));
            SignSpecialTxPayloadByString(tx, ptx, key);
            SetTxPayload(tx, ptx);
            return SignAndSendSpecialTx(tx);
        }
    }
}

UniValue protx_register_submit(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    if (request.fHelp || request.params.size() != 3)
        protx_register_submit_help(pwallet);

    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    EnsureWalletIsUnlocked(pwallet);

    CMutableTransaction tx;
    if (!DecodeHexTx(tx, request.params[1].get_str()))
        throw JSONRPCError(RPC_INVALID_PARAMETER, "transaction not deserializable");
    if (tx.nType != TRANSACTION_PROVIDER_REGISTER)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "transaction not a ProRegTx");

    CProRegTx ptx;
    if (!GetTxPayload(tx, ptx))
        throw JSONRPCError(RPC_INVALID_PARAMETER, "transaction payload not deserializable");
    if (!ptx.vchSig.empty())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "payload signature not empty");

    ptx.vchSig = DecodeBase64(request.params[2].get_str().c_str());

    SetTxPayload(tx, ptx);
    return SignAndSendSpecialTx(tx);
}

void protx_update_service_help(CWallet* const pwallet)
{
    throw std::runtime_error(
        RPCHelpMan{"protx update_service",
            "\nCreates and sends a ProUpServTx to the network. This will update the IP address\n"
            "of a masternode.\n"
            "If this is done for a masternode that got PoSe-banned, the ProUpServTx will also revive this masternode.\n"
            + HelpRequiringPassphrase(pwallet) + "\n",
            {
                GetHelpArg("proTxHash"),
                GetHelpArg("ipAndPort"),
                GetHelpArg("operatorKey"),
                GetHelpArg("operatorPayoutAddress"),
                GetHelpArg("feeSourceAddress")
            },
            RPCResult{
                "\"txid\"                  (string) The transaction id.\n"
            },
            RPCExamples{
                HelpExampleCli("protx", "update_service \"0123456701234567012345670123456701234567012345670123456701234567\" \"1.2.3.4:1234\" 5a2e15982e62f1e0b7cf9783c64cf7e3af3f90a52d6c40f6f95d624c0b1621cd")
            }
        }.ToString()
    );
}

UniValue protx_update_service(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    if (request.fHelp || (request.params.size() < 4 || request.params.size() > 6))
        protx_update_service_help(pwallet);

    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    EnsureWalletIsUnlocked(pwallet);

    CProUpServTx ptx;
    ptx.nVersion = CProRegTx::CURRENT_VERSION;
    ptx.proTxHash = ParseHashV(request.params[1], "proTxHash");

    if (!Lookup(request.params[2].get_str().c_str(), ptx.addr, Params().GetDefaultPort(), false))
        throw std::runtime_error(strprintf("Invalid network address %s", request.params[2].get_str()));

    CBLSSecretKey keyOperator = ParseBLSSecretKey(request.params[3].get_str(), "operatorKey");

    // TODO:
    // auto dmn = deterministicMNManager->GetListAtChainTip().GetMN(ptx.proTxHash);
    // if (!dmn)
    //     throw std::runtime_error(strprintf("Masternode with proTxHash %s not found", ptx.proTxHash.ToString()));

    // if (keyOperator.GetPublicKey() != dmn->pdmnState->pubKeyOperator.Get())
    //     throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("The operator key does not belong to the registered public key"));

    CMutableTransaction tx;
    tx.nVersion = 2;
    tx.nType = TRANSACTION_PROVIDER_UPDATE_SERVICE;

    // param operatorPayoutAddress
    if (request.params.size() >= 5) {
        if (request.params[4].get_str().empty()) {
            // TODO:
            // ptx.scriptOperatorPayout = dmn->pdmnState->scriptOperatorPayout;
        } else {
            CTxDestination payoutAddress = DecodeDestination(request.params[4].get_str());
            if (!IsValidDestination(payoutAddress))
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("Invalid operator payout address: %s", request.params[4].get_str()));
            ptx.scriptOperatorPayout = GetScriptForDestination(payoutAddress);
        }
    } else {
        // TODO:
        // ptx.scriptOperatorPayout = dmn->pdmnState->scriptOperatorPayout;
    }

    CTxDestination feeSource;

    // param feeSourceAddress
    if (request.params.size() >= 6) {
        CTxDestination feeSourceAddress = DecodeDestination(request.params[5].get_str());
        if (!IsValidDestination(feeSourceAddress))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid BitGreen address: ") + request.params[5].get_str());
        feeSource = feeSourceAddress;
    } else {
        if (ptx.scriptOperatorPayout != CScript()) {
            // use operator reward address as default source for fees
            ExtractDestination(ptx.scriptOperatorPayout, feeSource);
        } else {
            // use payout address as default source for fees
            // TODO:
            // ExtractDestination(dmn->pdmnState->scriptPayout, feeSource);
        }
    }

    FundSpecialTx(pwallet, tx, ptx, feeSource);

    SignSpecialTxPayloadByHash(tx, ptx, keyOperator);
    SetTxPayload(tx, ptx);

    return SignAndSendSpecialTx(tx);
}

void protx_update_registrar_help(CWallet* const pwallet)
{
    throw std::runtime_error(
        RPCHelpMan{"protx update_registrar",
            "\nCreates and sends a ProUpRegTx to the network. This will update the operator key, voting key and payout\n"
            "address of the masternode specified by \"proTxHash\".\n"
            "The owner key of the masternode must be known to your wallet.\n"
            + HelpRequiringPassphrase(pwallet) + "\n",
            {
                GetHelpArg("proTxHash"),
                GetHelpArg("operatorPubKey"),
                GetHelpArg("votingAddress"),
                GetHelpArg("payoutAddress"),
                GetHelpArg("feeSourceAddress")
            },
            RPCResult{
                "\"txid\"                  (string) The transaction id.\n"
            },
            RPCExamples{
                HelpExampleCli("protx", "update_registrar \"0123456701234567012345670123456701234567012345670123456701234567\" \"982eb34b7c7f614f29e5c665bc3605f1beeef85e3395ca12d3be49d2868ecfea5566f11cedfad30c51b2403f2ad95b67\" \"XwnLY9Tf7Zsef8gMGL2fhWA9ZmMjt4KPwG\"")
            }
        }.ToString()
    );
}

UniValue protx_update_registrar(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    if (request.fHelp || (request.params.size() != 5 && request.params.size() != 6))
        protx_update_registrar_help(pwallet);

    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    EnsureWalletIsUnlocked(pwallet);

    CProUpRegTx ptx;
    ptx.nVersion = CProRegTx::CURRENT_VERSION;
    ptx.proTxHash = ParseHashV(request.params[1], "proTxHash");

    // TODO:
    // auto dmn = deterministicMNManager->GetListAtChainTip().GetMN(ptx.proTxHash);
    // if (!dmn)
    //     throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("masternode %s not found", ptx.proTxHash.ToString()));

    // ptx.pubKeyOperator = dmn->pdmnState->pubKeyOperator.Get();
    // ptx.keyIDVoting = dmn->pdmnState->keyIDVoting;
    // ptx.scriptPayout = dmn->pdmnState->scriptPayout;

    if (request.params[2].get_str() != "")
        ptx.pubKeyOperator = ParseBLSPubKey(request.params[2].get_str(), "operator BLS address");
    if (request.params[3].get_str() != "")
        ptx.keyIDVoting = ParsePubKeyIDFromAddress(pwallet, request.params[3].get_str(), "voting address");

    CTxDestination payoutAddress = DecodeDestination(request.params[4].get_str());
    if (!IsValidDestination(payoutAddress))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("Invalid payout address: %s", request.params[4].get_str()));
    ptx.scriptPayout = GetScriptForDestination(payoutAddress);

    CKey keyOwner;
    // if (!pwallet->GetKey(dmn->pdmnState->keyIDOwner, keyOwner))
    //     throw std::runtime_error(strprintf("Private key for owner address %s not found in your wallet", CBitcoinAddress(dmn->pdmnState->keyIDOwner).ToString()));

    CMutableTransaction tx;
    tx.nVersion = 2;
    tx.nType = TRANSACTION_PROVIDER_UPDATE_REGISTRAR;

    // make sure we get anough fees added
    ptx.vchSig.resize(65);

    CTxDestination feeSourceAddress = payoutAddress;
    if (request.params.size() > 5) {
        feeSourceAddress = DecodeDestination(request.params[5].get_str());
        if (!IsValidDestination(feeSourceAddress))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid BitGreen address: ") + request.params[5].get_str());
    }

    FundSpecialTx(pwallet, tx, ptx, feeSourceAddress);
    SignSpecialTxPayloadByHash(tx, ptx, keyOwner);
    SetTxPayload(tx, ptx);

    return SignAndSendSpecialTx(tx);
}

void protx_revoke_help(CWallet* const pwallet)
{
    throw std::runtime_error(
        RPCHelpMan{"protx revoke",
            "\nCreates and sends a ProUpRevTx to the network. This will revoke the operator key of the masternode and\n"
            "put it into the PoSe-banned state. It will also set the service field of the masternode\n"
            "to zero. Use this in case your operator key got compromised or you want to stop providing your service\n"
            "to the masternode owner.\n"
            + HelpRequiringPassphrase(pwallet) + "\n",
            {
                GetHelpArg("proTxHash"),
                GetHelpArg("operatorKey"),
                GetHelpArg("reason"),
                GetHelpArg("feeSourceAddress")
            },
            RPCResult{
                "\"txid\"                  (string) The transaction id.\n"
            },
            RPCExamples{
                HelpExampleCli("protx", "revoke \"0123456701234567012345670123456701234567012345670123456701234567\" \"072f36a77261cdd5d64c32d97bac417540eddca1d5612f416feb07ff75a8e240\"")
            }
        }.ToString()
    );
}

UniValue protx_revoke(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    if (request.fHelp || (request.params.size() < 3 || request.params.size() > 5))
        protx_revoke_help(pwallet);

    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    EnsureWalletIsUnlocked(pwallet);

    CProUpRevTx ptx;
    ptx.nVersion = CProRegTx::CURRENT_VERSION;
    ptx.proTxHash = ParseHashV(request.params[1], "proTxHash");

    CBLSSecretKey keyOperator = ParseBLSSecretKey(request.params[2].get_str(), "operatorKey");

    if (request.params.size() > 3) {
        int nReason = UniValue(UniValue::VNUM, request.params[3].getValStr()).get_int();
        if (nReason < 0 || nReason > CProUpRevTx::REASON_LAST)
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Invalid reason %d, must be between 0 and %d", nReason, CProUpRevTx::REASON_LAST));
        ptx.nReason = (uint16_t)nReason;
    }

    // TODO:
    // auto dmn = deterministicMNManager->GetListAtChainTip().GetMN(ptx.proTxHash);
    // if (!dmn) {
    //     throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("masternode %s not found", ptx.proTxHash.ToString()));
    // }

    // if (keyOperator.GetPublicKey() != dmn->pdmnState->pubKeyOperator.Get()) {
    //     throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("the operator key does not belong to the registered public key"));
    // }

    CMutableTransaction tx;
    tx.nVersion = 2;
    tx.nType = TRANSACTION_PROVIDER_UPDATE_REVOKE;

    if (request.params.size() > 4) {
        CTxDestination feeSourceAddress = DecodeDestination(request.params[4].get_str());
        if (!IsValidDestination(feeSourceAddress))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid BitGreen address: ") + request.params[4].get_str());
        FundSpecialTx(pwallet, tx, ptx, feeSourceAddress);
    // TODO:
    // } else if (dmn->pdmnState->scriptOperatorPayout != CScript()) {
        // Using funds from previousely specified operator payout address
        // CTxDestination txDest;
        // ExtractDestination(dmn->pdmnState->scriptOperatorPayout, txDest);
        // FundSpecialTx(pwallet, tx, ptx, txDest);
    // } else if (dmn->pdmnState->scriptPayout != CScript()) {
        // Using funds from previousely specified masternode payout address
        // CTxDestination txDest;
        // ExtractDestination(dmn->pdmnState->scriptPayout, txDest);
        // FundSpecialTx(pwallet, tx, ptx, txDest);
    } else {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "No payout or fee source addresses found, can't revoke");
    }

    SignSpecialTxPayloadByHash(tx, ptx, keyOperator);
    SetTxPayload(tx, ptx);

    return SignAndSendSpecialTx(tx);
}
#endif//ENABLE_WALLET

void protx_list_help()
{
    throw std::runtime_error(
        RPCHelpMan{"protx list",
            "\nLists all ProTxs in your wallet or on-chain, depending on the given type.\n"
            "\nAvailable types:\n"
            "  registered   - List all ProTx which are registered at the given chain height.\n"
            "                 This will also include ProTx which failed PoSe verfication.\n"
            "  valid        - List only ProTx which are active/valid at the given chain height.\n"
#ifdef ENABLE_WALLET
            "  wallet       - List only ProTx which are found in your wallet at the given chain height.\n"
            "                 This will also include ProTx which failed PoSe verfication.\n"
#endif
            ,{
                {"type", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "If not specified, defaults to \"registered\"."},
                {"detailed", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED, "If not specified, defaults to \"false\" and only the hashes of the ProTx will be returned."},
                {"height", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "If not specified, defaults to the current chain-tip."},
            },
            RPCResult{""},
            RPCExamples{""}
        }.ToString()
    );
}

static bool CheckWalletOwnsKey(CWallet* pwallet, const CKeyID& keyID) {
#ifndef ENABLE_WALLET
    return false;
#else
    if (!pwallet) {
        return false;
    }
    return pwallet->HaveKey(keyID);
#endif
}

static bool CheckWalletOwnsScript(CWallet* pwallet, const CScript& script) {
#ifndef ENABLE_WALLET
    return false;
#else
    if (!pwallet) {
        return false;
    }

    CTxDestination dest;
    if (ExtractDestination(script, dest))
        if (IsMine(*pwallet, dest))
            return true;

    return false;
#endif
}

UniValue BuildDMNListEntry(CWallet* pwallet, const CDeterministicMNCPtr& dmn, bool detailed)
{
    if (!detailed)
        return dmn->proTxHash.ToString();

    UniValue o(UniValue::VOBJ);

    dmn->ToJson(o);

    int confirmations = GetUTXOConfirmations(dmn->collateralOutpoint);
    o.pushKV("confirmations", confirmations);

    bool hasOwnerKey = CheckWalletOwnsKey(pwallet, dmn->pdmnState->keyIDOwner);
    bool hasOperatorKey = false; //CheckWalletOwnsKey(dmn->pdmnState->keyIDOperator);
    bool hasVotingKey = CheckWalletOwnsKey(pwallet, dmn->pdmnState->keyIDVoting);

    bool ownsCollateral = false;
    CTransactionRef collateralTx;
    uint256 tmpHashBlock;
    if (GetTransaction(dmn->collateralOutpoint.hash, collateralTx, Params().GetConsensus(), tmpHashBlock))
        ownsCollateral = CheckWalletOwnsScript(pwallet, collateralTx->vout[dmn->collateralOutpoint.n].scriptPubKey);

    UniValue walletObj(UniValue::VOBJ);
    walletObj.pushKV("hasOwnerKey", hasOwnerKey);
    walletObj.pushKV("hasOperatorKey", hasOperatorKey);
    walletObj.pushKV("hasVotingKey", hasVotingKey);
    walletObj.pushKV("ownsCollateral", ownsCollateral);
    walletObj.pushKV("ownsPayeeScript", CheckWalletOwnsScript(pwallet, dmn->pdmnState->scriptPayout));
    walletObj.pushKV("ownsOperatorRewardScript", CheckWalletOwnsScript(pwallet, dmn->pdmnState->scriptOperatorPayout));
    o.pushKV("wallet", walletObj);

    return o;
}

UniValue protx_list(const JSONRPCRequest& request)
{
    if (request.fHelp)
        protx_list_help();

#ifdef ENABLE_WALLET
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
#else
    CWallet* const pwallet = nullptr;
#endif

    std::string type = "registered";
    if (request.params.size() > 1)
        type = request.params[1].get_str();

    UniValue ret(UniValue::VARR);

    LOCK(cs_main);

    if (type == "wallet") {
        if (!pwallet)
            throw std::runtime_error("\"protx list wallet\" not supported when wallet is disabled");

#ifdef ENABLE_WALLET
        LOCK2(cs_main, pwallet->cs_wallet);

        if (request.params.size() > 3)
            protx_list_help();

        bool detailed = request.params.size() > 2 ? request.params[2].get_bool() : false;

        int height = request.params.size() > 3 ? UniValue(UniValue::VNUM, request.params[3].getValStr()).get_int() : ChainActive().Height();
        if (height < 1 || height > ChainActive().Height())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid height specified");

        std::vector<COutPoint> vOutpts;
        pwallet->ListProTxCoins(vOutpts);
        std::set<COutPoint> setOutpts;
        for (const auto& outpt : vOutpts)
            setOutpts.emplace(outpt);

        CDeterministicMNList mnList = deterministicMNManager->GetListForBlock(ChainActive()[height]);
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
        if (request.params.size() > 4)
            protx_list_help();

        LOCK(cs_main);

        bool detailed = request.params.size() > 2 ? request.params[2].get_bool() : false;

        int height = request.params.size() > 3 ? UniValue(UniValue::VNUM, request.params[3].getValStr()).get_int() : ChainActive().Height();
        if (height < 1 || height > ChainActive().Height()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid height specified");
        }

        CDeterministicMNList mnList = deterministicMNManager->GetListForBlock(ChainActive()[height]);
        bool onlyValid = type == "valid";
        mnList.ForEachMN(onlyValid, [&](const CDeterministicMNCPtr& dmn) {
            ret.push_back(BuildDMNListEntry(pwallet, dmn, detailed));
        });
    } else {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "invalid type specified");
    }

    return ret;
}

void protx_info_help()
{
    throw std::runtime_error(
        RPCHelpMan{"protx info",
            "\nReturns detailed information about a deterministic masternode.\n",
            {
                GetHelpArg("proTxHash")
            },
            RPCResult{
                "{\n"
                "    (json object) Details about a specific deterministic masternode\n"
                "}\n"
            },
            RPCExamples{
                HelpExampleCli("protx", "info \"0123456701234567012345670123456701234567012345670123456701234567\"")
            }
        }.ToString()
    );
}

UniValue protx_info(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 2)
        protx_info_help();

#ifdef ENABLE_WALLET
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
#else
    CWallet* const pwallet = nullptr;
#endif

    uint256 proTxHash = ParseHashV(request.params[1], "proTxHash");
    auto mnList = deterministicMNManager->GetListAtChainTip();
    auto dmn = mnList.GetMN(proTxHash);
    if (!dmn)
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s not found", proTxHash.ToString()));

    return BuildDMNListEntry(pwallet, dmn, true);
}

void protx_help()
{
    throw std::runtime_error(
        RPCHelpMan{"protx",
            "\nSet of commands to execute ProTx related actions.\n",
            {
                {"command", RPCArg::Type::STR, /* default */ "all commands", "The command to get help on"},
            },
            RPCResult{
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
            },
            RPCExamples{""},
        }.ToString()
    );
}

UniValue protx(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.empty())
        protx_help();

    std::string command;
    if (request.params.size() >= 1)
        command = request.params[0].get_str();

#ifdef ENABLE_WALLET
    if (command == "register" || command == "register_fund" || command == "register_prepare")
        return protx_register(request);
    else if (command == "register_submit")
        return protx_register_submit(request);
    else if (command == "update_service")
        return protx_update_service(request);
    else if (command == "update_registrar")
        return protx_update_registrar(request);
    else if (command == "revoke")
        return protx_revoke(request);
    else
#endif
    if (command == "list")
        return protx_list(request);
    if (command == "info")
        return protx_info(request);
    else
        protx_help();

    return NullUniValue;
}

void bls_generate_help()
{
    throw std::runtime_error(
        RPCHelpMan{"bls generate",
            "\nReturns a BLS secret/public key pair.\n",
            {},
            RPCResult{
                "{\n"
                "  \"secret\": \"xxxx\",        (string) BLS secret key\n"
                "  \"public\": \"xxxx\",        (string) BLS public key\n"
                "}\n"
            },
            RPCExamples{
                HelpExampleCli("bls generate", "")
            },
        }.ToString()
    );
}

UniValue bls_generate(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        bls_generate_help();

    CBLSSecretKey sk;
    sk.MakeNewKey();

    UniValue ret(UniValue::VOBJ);
    ret.pushKV("secret", sk.ToString());
    ret.pushKV("public", sk.GetPublicKey().ToString());
    return ret;
}

void bls_fromsecret_help()
{
    throw std::runtime_error(
        RPCHelpMan{"bls fromsecret",
            "\nParses a BLS secret key and returns the secret/public key pair.\n",
            {
                {"secret", RPCArg::Type::STR, RPCArg::Optional::NO, "The BLS secret key"},
            },
            RPCResult{
                "{\n"
                "  \"secret\": \"xxxx\",        (string) BLS secret key\n"
                "  \"public\": \"xxxx\",        (string) BLS public key\n"
                "}\n"
            },
            RPCExamples{
                HelpExampleCli("bls fromsecret", "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f")
            },
        }.ToString()
    );
}

UniValue bls_fromsecret(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 2)
        bls_fromsecret_help();

    CBLSSecretKey sk;
    if (!sk.SetHexStr(request.params[1].get_str()))
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Secret key must be a valid hex string of length %d", sk.SerSize*2));

    UniValue ret(UniValue::VOBJ);
    ret.pushKV("secret", sk.ToString());
    ret.pushKV("public", sk.GetPublicKey().ToString());
    return ret;
}

void bls_help()
{
    throw std::runtime_error(
        RPCHelpMan{"bls",
            "\nSet of commands to execute BLS related actions.\n"
            "To get help on individual commands, use \"help bls command\".\n",
            {
                {"command", RPCArg::Type::STR, RPCArg::Optional::NO, "The command to execute"},
            },
            RPCResult{
                "  generate          - Create a BLS secret/public key pair\n"
                "  fromsecret        - Parse a BLS secret key and return the secret/public key pair\n"
            },
            RPCExamples{""},
        }.ToString()
    );
}

UniValue _bls(const JSONRPCRequest& request)
{
    if (request.fHelp && request.params.empty())
        bls_help();

    std::string command;
    if (request.params.size() >= 1)
        command = request.params[0].get_str();

    if (command == "generate")
        return bls_generate(request);
    else if (command == "fromsecret")
        return bls_fromsecret(request);
    else
        bls_help();

    return NullUniValue;
}

// clang-format off
static const CRPCCommand commands[] =
{ //  category              name                      actor (function)         argNames
  //  --------------------- ------------------------  -----------------------  ----------
#ifdef ENABLE_WALLET
    // these require the wallet to be enabled to fund the transactions
    { "special",            "bls",                    &_bls,                   {} },
    { "special",            "protx",                  &protx,                  {} },
#endif//ENABLE_WALLET
};
// clang-format on

void RegisterSpecialRPCCommands(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
