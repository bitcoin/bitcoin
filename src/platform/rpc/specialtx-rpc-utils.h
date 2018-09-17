// Copyright (c) 2014-2018 Crown Core developers
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

    CKey ParsePrivKeyOrAddress(const std::string & strKeyOrAddress, bool allowAddresses = true);
    CKeyID ParsePubKeyIDFromAddress(const std::string & strAddress, const std::string & paramName);
}

#endif // CROWN_SPECIALTX_RPC_UTILS_H
