// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2015-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_DOSMANAGER_H
#define BITCOIN_DOSMANAGER_H

#include "netbase.h" // for CSubNet
#include "sync.h" // for CCritalSection

#ifdef DEBUG
#include <univalue.h>
#endif


class CDoSManager
{
protected:
#ifdef DEBUG
    friend UniValue getstructuresizes(const UniValue &params, bool fHelp);
#endif

    // Whitelisted ranges. Any node connecting from these is automatically
    // whitelisted (as well as those connecting to whitelisted binds).
    std::vector<CSubNet> vWhitelistedRange;
    mutable CCriticalSection cs_vWhitelistedRange;

public:
    bool IsWhitelistedRange(const CNetAddr &ip);
    void AddWhitelistedRange(const CSubNet &subnet);
};

// actual definition should be in globals.cpp for ordered construction/destruction
extern CDoSManager dosMan;

#endif // BITCOIN_DOSMANAGER_H
