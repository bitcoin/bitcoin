// Copyright (c) 2014-2020 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_SPECIALTX_RPC_UTILS_H
#define CROWN_SPECIALTX_RPC_UTILS_H

#include "json/json_spirit_value.h"
#include "key.h"

class CMutableTransaction;

namespace Platform
{
    std::string GetCommand(const json_spirit::Array & params, const std::string & errorMessage);
    std::string SignAndSendSpecialTx(const CMutableTransaction & tx);

    CKey GetPrivKeyFromWallet(const CKeyID & keyId);
    CKey PullPrivKeyFromWallet(const std::string & strAddress, const std::string & paramName);
    CKey ParsePrivKeyOrAddress(const std::string & strKeyOrAddress, const std::string & paramName, bool allowAddresses = true);
    CKeyID ParsePubKeyIDFromAddress(const std::string & strAddress, const std::string & paramName);

    bool GetPayerPrivKeyForNftTx(const CMutableTransaction & tx, CKey & payerKey);
    bool GetPayerPubKeyIdForNftTx(const CMutableTransaction & tx, CKeyID & payerKeyId);
}

#endif // CROWN_SPECIALTX_RPC_UTILS_H
