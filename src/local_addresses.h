// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LOCAL_ADDRESSES_H
#define BITCOIN_LOCAL_ADDRESSES_H

#include <netaddress.h>

#include <optional>
#include <map>

class CAddress;

enum
{
    LOCAL_NONE,   // unknown
    LOCAL_IF,     // address a local interface listens on
    LOCAL_BIND,   // address explicit bound to
    LOCAL_MAPPED, // address reported by PCP
    LOCAL_MANUAL, // address explicitly specified (-externalip=)

    LOCAL_MAX
};

struct LocalServiceInfo {
    int nScore;
    uint16_t nPort;
};

void ClearLocal();
bool AddLocal(const CService& addr, int nScore = LOCAL_NONE);
void RemoveLocal(const CService& addr);
bool SeenLocal(const CService& addr);
bool IsLocal(const CService& addr);
std::optional<CService> GetLocalAddress(const CAddress& addr, const Network& connected_through);
int GetnScore(const CService& addr);


#endif // BITCOIN_LOCAL_ADDRESSES_H
