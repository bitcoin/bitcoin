// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2015-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_DOSMANAGER_H
#define BITCOIN_DOSMANAGER_H

#include "banentry.h" // for banmap_t
#include "net.h" // for NodeId
#include "netbase.h" // for CSubNet
#include "sync.h" // for CCritalSection

#ifdef DEBUG
#include <univalue.h>
#endif

// NOTE: When adjusting this, update rpcnet:setban's help ("24h")
static const unsigned int DEFAULT_MISBEHAVING_BANTIME = 60 * 60 * 24; // Default 24-hour ban
static const unsigned int DEFAULT_BANSCORE_THRESHOLD = 100;


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

    // Denial-of-service detection/prevention
    // Key is IP address, value is banned-until-time
    banmap_t setBanned;
    mutable CCriticalSection cs_setBanned;
    bool setBannedIsDirty;

public:
    bool IsWhitelistedRange(const CNetAddr &ip);
    void AddWhitelistedRange(const CSubNet &subnet);

    // Denial-of-service detection/prevention
    // The idea is to detect peers that are behaving
    // badly and disconnect/ban them, but do it in a
    // one-coding-mistake-won't-shatter-the-entire-network
    // way.
    // IMPORTANT:  There should be nothing I can give a
    // node that it will forward on that will make that
    // node's peers drop it. If there is, an attacker
    // can isolate a node and/or try to split the network.
    // Dropping a node for sending stuff that is invalid
    // now but might be valid in a later version is also
    // dangerous, because it can cause a network split
    // between nodes running old code and nodes running
    // new code.
    void ClearBanned(); // needed for unit testing
    bool IsBanned(CNetAddr ip);
    bool IsBanned(CSubNet subnet);
    void Ban(const CNetAddr &ip, const BanReason &banReason, int64_t bantimeoffset = 0, bool sinceUnixEpoch = false);
    void Ban(const CSubNet &subNet, const BanReason &banReason, int64_t bantimeoffset = 0, bool sinceUnixEpoch = false);
    bool Unban(const CNetAddr &ip);
    bool Unban(const CSubNet &ip);
    void GetBanned(banmap_t &banmap);
    void SetBanned(const banmap_t &banmap);

    //! check is the banlist has unwritten changes
    bool BannedSetIsDirty();
    //! set the "dirty" flag for the banlist
    void SetBannedSetDirty(bool dirty = true);
    //! clean unused entries (if bantime has expired)
    void SweepBanned();

    /** Increase a node's misbehavior score. */
    void Misbehaving(NodeId nodeid, int howmuch);
};

// actual definition should be in globals.cpp for ordered construction/destruction
extern CDoSManager dosMan;

#endif // BITCOIN_DOSMANAGER_H
