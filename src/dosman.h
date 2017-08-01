// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2015-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_DOSMANAGER_H
#define BITCOIN_DOSMANAGER_H

#include "banentry.h" // for banmap_t
#include "net.h" // for NodeId
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

    // If a node's misbehaving count reaches this value, it is flagged for banning.
    int nBanThreshold;

public:
    CDoSManager();

    /**
     * Call once the command line is parsed so dosman configures itself appropriately.
     */
    void HandleCommandLine();

    /**
     * Increment the misbehaving score for this node.  If the ban threshold is reached, flag the node to be
     * banned.  No locks are needed to call this function.
     *
     * @param[in] pNode    The node which is misbehaving.  No effect if nullptr.
     * @param[in] howmuch  Incremental misbehaving score for the latest infraction by this node.
     */
    void Misbehaving(CNode *pNode, int howmuch);

    /**
     * Increment the misbehaving score for this node.  If the ban threshold is reached, flag the node to be
     * banned.  No locks are needed to call this function.
     *
     * @param[in] nodeid   The ID of the misbehaving node.  No effect if the CNode is no longer present.
     * @param[in] howmuch  Incremental misbehaving score for the latest infraction by this node.
     */
    void Misbehaving(NodeId nodeid, int howmuch);

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

    //! check if the banlist has unwritten changes
    bool BannedSetIsDirty();
    //! clean unused entries (if bantime has expired)
    void SweepBanned();

    //! save banlist to disk
    void DumpBanlist();
    //! load banlist from disk
    void LoadBanlist();

protected:
    void SweepBannedInternal() EXCLUSIVE_LOCKS_REQUIRED(cs_setBanned);
    void GetBannedInternal(banmap_t &banmap) EXCLUSIVE_LOCKS_REQUIRED(cs_setBanned);
};

// actual definition should be in globals.cpp for ordered construction/destruction
extern CDoSManager dosMan;

#endif // BITCOIN_DOSMANAGER_H
