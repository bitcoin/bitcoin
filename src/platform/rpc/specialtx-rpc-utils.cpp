// Copyright (c) 2014-2020 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "base58.h"
#include "rpcprotocol.h"
#include "rpcserver.h"
#include "platform/specialtx.h"
#include "specialtx-rpc-utils.h"

#ifdef ENABLE_WALLET
#include "wallet.h"
#endif//ENABLE_WALLET

namespace Platform
{

    // Allows to specify Crown address or priv key. In case of Crown address, the priv key is taken from the wallet
    CKey ParsePrivKeyOrAddress(const std::string & strKeyOrAddress, const std::string & paramName, bool allowAddresses /* = true */)
    {
        if (allowAddresses)
        {
            CKey key = PullPrivKeyFromWallet(strKeyOrAddress, paramName);
            if (key.IsValid())
                return key;
        }

        CBitcoinSecret secret;
        if (!secret.SetString(strKeyOrAddress) || !secret.IsValid())
            throw std::runtime_error(strprintf("invalid priv-key/address %s", strKeyOrAddress));
        return secret.GetKey();
    }

    CKey GetPrivKeyFromWallet(const CKeyID & keyId)
    {
#ifdef ENABLE_WALLET
        CKey key;
        if (keyId.IsNull() || !pwalletMain->GetKey(keyId, key))
            throw std::runtime_error(strprintf("non-wallet or invalid address %s", keyId.ToString()));
        return key;
#else//ENABLE_WALLET
        throw std::runtime_error("addresses not supported in no-wallet builds");
#endif//ENABLE_WALLET
    }

    CKey PullPrivKeyFromWallet(const std::string & strAddress, const std::string & paramName)
    {
        CBitcoinAddress address;
        if (!address.SetString(strAddress) || !address.IsValid())
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s must be a valid P2PKH address, not %s", paramName, strAddress));

#ifdef ENABLE_WALLET
        CKeyID keyId;
        CKey key;
        if (!address.GetKeyID(keyId) || !pwalletMain->GetKey(keyId, key))
            throw std::runtime_error(strprintf("non-wallet or invalid address %s", strAddress));
        return key;
#else//ENABLE_WALLET
        throw std::runtime_error("addresses not supported in no-wallet builds");
#endif//ENABLE_WALLET
    }

    CKeyID ParsePubKeyIDFromAddress(const std::string & strAddress, const std::string & paramName)
    {
        CBitcoinAddress address(strAddress);
        CKeyID keyID;
        if (!address.IsValid() || !address.GetKeyID(keyID))
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s must be a valid P2PKH address, not %s", paramName, strAddress));
        return keyID;
    }

    std::string GetCommand(const json_spirit::Array & params, const std::string & errorMessage)
    {
        if (params.empty())
            throw std::runtime_error(errorMessage);

        return params[0].get_str();
    }

    std::string SignAndSendSpecialTx(const CMutableTransaction & tx)
    {
        LOCK(cs_main);
        CValidationState state;
        if (!CheckSpecialTx(tx, NULL, state))
            throw std::runtime_error(FormatStateMessage(state));

        CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
        ds << tx;

        json_spirit::Array signReqeust;
        signReqeust.push_back(HexStr(ds.begin(), ds.end()));
        json_spirit::Value signResult = signrawtransaction(signReqeust, false);

        json_spirit::Array sendRequest;
        sendRequest.push_back(json_spirit::find_value(signResult.get_obj(), "hex").get_str());
        return sendrawtransaction(sendRequest, false).get_str();
    }

    bool GetPayerPrivKeyForNftTx(const CMutableTransaction & tx, CKey & payerKey)
    {
        CKeyID payerKeyId;
        if (GetPayerPubKeyIdForNftTx(tx, payerKeyId))
        {
            payerKey = GetPrivKeyFromWallet(payerKeyId);
            return true;
        }
        return false;
    }

    bool GetPayerPubKeyIdForNftTx(const CMutableTransaction & tx, CKeyID & payerKeyId)
    {
        if (tx.vin.empty())
            return false;

        uint256 hashBlockFrom;
        CTransaction txFrom;
        if (!GetTransaction(tx.vin[0].prevout.hash, txFrom, hashBlockFrom, true))
            return false;

        auto txFromOutIdx = tx.vin[0].prevout.n;
        assert(txFromOutIdx < txFrom.vout.size());

        CTxDestination payer;
        if (ExtractDestination(txFrom.vout[txFromOutIdx].scriptPubKey, payer))
        {
            CBitcoinAddress payerAddress(payer);
            return payerAddress.GetKeyID(payerKeyId);
        }

        return false;
    }
}
