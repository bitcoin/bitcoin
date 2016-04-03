// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "wallet.h"
#include "bitcoinrpc.h"
#include "init.h"
#include "base58.h"

using namespace json_spirit;
using namespace std;

Value encryptdata(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error(
            "encryptdata <public key> <hex data>\n"
            "Encrypt octet stream with provided public key..\n");

    CPubKey pubKey(ParseHex(params[0].get_str()));

    vector<unsigned char> vchEncrypted;
    pubKey.EncryptData(ParseHex(params[1].get_str()), vchEncrypted);

    return HexStr(vchEncrypted);
}

Value decryptdata(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error(
            "decryptdata <novacoin address or private key> <encrypted stream>\n"
            "Decrypt octet stream.\n");

    EnsureWalletIsUnlocked();
    CKey key;
    CBitcoinAddress addr(params[0].get_str());
    if (addr.IsValid()) {
        CKeyID keyID;
        addr.GetKeyID(keyID);
        if (!pwalletMain->GetKey(keyID, key))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "We have no private key for this address");
    }
    else {
        CBitcoinSecret vchSecret;
        if (!vchSecret.SetString(params[0].get_str()))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Provided private key is inconsistent.");
        bool fCompressed;
        CSecret secret = vchSecret.GetSecret(fCompressed);
        key.SetSecret(secret, fCompressed);
    }

    vector<unsigned char> vchDecrypted;
    key.DecryptData(ParseHex(params[1].get_str()), vchDecrypted);

    return HexStr(vchDecrypted);
}

Value encryptmessage(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error(
            "encryptmessage <public key> <message string>\n"
            "Encrypt message with provided public key.\n");

    CPubKey pubKey(ParseHex(params[0].get_str()));

    vector<unsigned char> vchEncrypted;
    string strData = params[1].get_str();
    pubKey.EncryptData(vector<unsigned char>(strData.begin(), strData.end()), vchEncrypted);

    return EncodeBase58Check(vchEncrypted);
}

Value decryptmessage(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error(
            "decryptmessage <novacoin address or private key> <encrypted message>\n"
            "Decrypt message string.\n");

    EnsureWalletIsUnlocked();

    CKey key;
    CBitcoinAddress addr(params[0].get_str());
    if (addr.IsValid()) {
        CKeyID keyID;
        addr.GetKeyID(keyID);
        if (!pwalletMain->GetKey(keyID, key))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "We have no private key for this address");
    }
    else {
        CBitcoinSecret vchSecret;
        if (!vchSecret.SetString(params[0].get_str()))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Provided private key is inconsistent.");
        bool fCompressed;
        CSecret secret = vchSecret.GetSecret(fCompressed);
        key.SetSecret(secret, fCompressed);
    }

    vector<unsigned char> vchEncrypted;
    if (!DecodeBase58Check(params[1].get_str(), vchEncrypted))
        throw runtime_error("Incorrect string");
    vector<unsigned char> vchDecrypted;
    key.DecryptData(vchEncrypted, vchDecrypted);

    return std::string((const char*)&vchDecrypted[0], vchDecrypted.size());
}
