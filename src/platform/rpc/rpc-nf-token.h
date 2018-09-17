// Copyright (c) 2014-2018 Crown Core developers
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
}

#endif // CROWN_PLATFORM_RPC_NF_TOKEN_H
