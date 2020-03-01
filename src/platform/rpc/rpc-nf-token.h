// Copyright (c) 2014-2020 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWN_PLATFORM_RPC_NF_TOKEN_H
#define CROWN_PLATFORM_RPC_NF_TOKEN_H

#include "json/json_spirit_value.h"

json_spirit::Value nftoken(const json_spirit::Array& params, bool fHelp);

namespace Platform
{
    json_spirit::Value RegisterNfToken(const json_spirit::Array& params, bool fHelp);
    void RegisterNfTokenHelp();
    json_spirit::Value ListNfTokenTxs(const json_spirit::Array& params, bool fHelp);
    void ListNfTokenTxsHelp();
    json_spirit::Value GetNfToken(const json_spirit::Array& params, bool fHelp);
    void GetNfTokenHelp();
    json_spirit::Value GetNfTokenByTxId(const json_spirit::Array& params, bool fHelp);
    void GetNfTokenByTxIdHelp();
    json_spirit::Value NfTokenTotalSupply(const json_spirit::Array& params, bool fHelp);
    void NfTokenTotalSupplyHelp();
    json_spirit::Value NfTokenBalanceOf(const json_spirit::Array& params, bool fHelp);
    void NfTokenBalanceOfHelp();
    json_spirit::Value NfTokenOwnerOf(const json_spirit::Array& params, bool fHelp);
    void NfTokenOwnerOfHelp();
}

#endif // CROWN_PLATFORM_RPC_NF_TOKEN_H
