// Copyright (c) 2014-2019 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PROJECT_RPC_NFT_PROTO_H
#define PROJECT_RPC_NFT_PROTO_H

#include "json/json_spirit_value.h"

json_spirit::Value nftproto(const json_spirit::Array& params, bool fHelp);

namespace Platform
{
    json_spirit::Value RegisterNftProtocol(const json_spirit::Array& params, bool fHelp);
    void RegisterNftProtocolHelp();
    json_spirit::Value ListNftProtocols(const json_spirit::Array& params, bool fHelp);
    void ListNftProtocolsHelp();
    json_spirit::Value GetNftProtocol(const json_spirit::Array& params, bool fHelp);
    void GetNftProtocolHelp();
}

#endif // PROJECT_RPC_NFT_PROTO_H
