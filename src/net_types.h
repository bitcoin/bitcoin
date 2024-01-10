// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NET_TYPES_H
#define BITCOIN_NET_TYPES_H

#include <util/expected.h>

#include <map>
#include <string>

class CBanEntry;
class CSubNet;

using banmap_t = std::map<CSubNet, CBanEntry>;

struct MisbehavingError
{
    int score;
    std::string message;

    MisbehavingError(int s) : score{s} {}

     // Constructor does a perfect forwarding reference
    template <typename T>
    MisbehavingError(int s, T&& msg) :
        score{s},
        message{std::forward<T>(msg)}
    {}
};

using PeerMsgRet = tl::expected<void, MisbehavingError>;

#endif // BITCOIN_NET_TYPES_H
