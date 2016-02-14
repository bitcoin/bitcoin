// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "wallet.h"
#include "walletdb.h"
#include "bitcoinrpc.h"
#include "init.h"
#include "util.h"
#include "ntp.h"
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
            "decryptdata <novacoin address> <encrypted stream>\n"
            "Decrypt octet stream.\n");

    EnsureWalletIsUnlocked();
    CBitcoinAddress addr(params[0].get_str());

    CKeyID keyID;
    addr.GetKeyID(keyID);

    CKey key;
    pwalletMain->GetKey(keyID, key);

    vector<unsigned char> vchDecrypted;
    key.DecryptData(ParseHex(params[1].get_str()), vchDecrypted);

    return HexStr(vchDecrypted);
}
