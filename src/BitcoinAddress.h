// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once

#include "base58.h"

/** base58-encoded Bitcoin addresses.
* Public-key-hash-addresses have version 0 (or 111 testnet).
* The data vector contains RIPEMD160(SHA256(pubkey)), where pubkey is the serialized public key.
* Script-hash-addresses have version 5 (or 196 testnet).
* The data vector contains RIPEMD160(SHA256(cscript)), where cscript is the serialized redemption script.
*/
class CBitcoinAddress : public CBase58Data {
public:
    bool Set(const CKeyID &id);
    bool Set(const CScriptID &id);
    bool Set(const CTxDestination &dest);
    bool IsValid() const;
    bool IsValid(const CChainParams &params) const;

    CBitcoinAddress() {}
    CBitcoinAddress(const CTxDestination &dest) { Set(dest); }
    CBitcoinAddress(const std::string& strAddress) { _SetString(strAddress); }
    CBitcoinAddress(const char* pszAddress) { _SetString(pszAddress); }

    CTxDestination Get() const;
    bool GetKeyID(CKeyID &keyID) const;
    bool IsScript() const;
};
