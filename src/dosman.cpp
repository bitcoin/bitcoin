// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2015-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dosman.h"
#include "bandb.h"
#include "nodestate.h"
#include "ui_interface.h"

#include <boost/foreach.hpp>


/**
* Checks if this CNetAddr is in the whitelist
*
* @param[in] addr  The address to check
* @return true if address is in the whitelist, otherwise false
*/
bool CDoSManager::IsWhitelistedRange(const CNetAddr &addr)
{
    LOCK(cs_vWhitelistedRange);
    BOOST_FOREACH (const CSubNet &subnet, vWhitelistedRange)
    {
        if (subnet.Match(addr))
            return true;
    }
    return false;
}

/**
* Add this CSubNet to the whitelist
*
* @param[in] subnet  The subnet to add to the whitelist
*/
void CDoSManager::AddWhitelistedRange(const CSubNet &subnet)
{
    LOCK(cs_vWhitelistedRange);
    vWhitelistedRange.push_back(subnet);
}

/**
* Remove all in-memory ban entries
* Marks the in-memory banlist as dirty
*/
void CDoSManager::ClearBanned()
{
    LOCK(cs_setBanned);
    setBanned.clear();
    setBannedIsDirty = true;
    uiInterface.BannedListChanged();
}

/**
* Check to see if this CNetAddr is currently banned
*
* @param[in] ip  The address to check
* @return true if address currently banned, otherwise false
*/
bool CDoSManager::IsBanned(CNetAddr ip)
{
    bool fResult = false;
    {
        LOCK(cs_setBanned);
        for (banmap_t::iterator it = setBanned.begin(); it != setBanned.end(); it++)
        {
            CSubNet subNet = (*it).first;
            CBanEntry banEntry = (*it).second;

            if (subNet.Match(ip) && GetTime() < banEntry.nBanUntil)
                fResult = true;
        }
    }
    return fResult;
}

/**
* Check if this CSubNet is currently banned
*
* @param[in] subnet  The subnet to check
* @return true if subnet currently banned, otherwise false
*/
bool CDoSManager::IsBanned(CSubNet subnet)
{
    bool fResult = false;
    {
        LOCK(cs_setBanned);
        banmap_t::iterator i = setBanned.find(subnet);
        if (i != setBanned.end())
        {
            CBanEntry banEntry = (*i).second;
            if (GetTime() < banEntry.nBanUntil)
                fResult = true;
        }
    }
    return fResult;
}

/**
* Add this CNetAddr to the banlist for the specified duration
* Marks the in-memory banlist as dirty
*
* @param[in] addr            The address to ban
* @param[in] banReason       The reason for banning this address
* @param[in] bantimeoffset   The duration of the ban in seconds, either a duration or an absolute time
* @param[in] sinceUnixEpoch  Whether or not the bantimeoffset is a relative duration, or absolute time
*/
void CDoSManager::Ban(const CNetAddr &addr, const BanReason &banReason, int64_t bantimeoffset, bool sinceUnixEpoch)
{
    CSubNet subNet(addr);
    Ban(subNet, banReason, bantimeoffset, sinceUnixEpoch);
}

/**
* Add this CSubNet to the banlist for the specified duration
* Marks the in-memory banlist as dirty
*
* @param[in] addr            The subnet to ban
* @param[in] banReason       The reason for banning this subnet
* @param[in] bantimeoffset   The duration of the ban in seconds, either a duration or an absolute time
* @param[in] sinceUnixEpoch  Whether or not the bantimeoffset is a relative duration, or absolute time
*/
void CDoSManager::Ban(const CSubNet &subNet, const BanReason &banReason, int64_t bantimeoffset, bool sinceUnixEpoch)
{
    CBanEntry banEntry(GetTime());
    banEntry.banReason = banReason;
    if (bantimeoffset <= 0)
    {
        bantimeoffset = GetArg("-bantime", DEFAULT_MISBEHAVING_BANTIME);
        sinceUnixEpoch = false;
    }
    banEntry.nBanUntil = (sinceUnixEpoch ? 0 : GetTime()) + bantimeoffset;

    LOCK(cs_setBanned);
    if (setBanned[subNet].nBanUntil < banEntry.nBanUntil)
        setBanned[subNet] = banEntry;

    setBannedIsDirty = true;
    uiInterface.BannedListChanged();
}

/**
* Remove this CNetAddr from the banlist
* Marks the in-memory banlist as dirty if address was found and removed
*
* @param[in] addr  The address to unban
* @return true if address was in the ban list and was removed, otherwise false
*/
bool CDoSManager::Unban(const CNetAddr &addr)
{
    CSubNet subNet(addr);
    return Unban(subNet);
}

/**
* Remove this CSubNet from the banlist
* Marks the in-memory banlist as dirty if subnet was found and removed
*
* @param[in] subNet  The subnet to unban
* @return true if subnet was in the ban list and was removed, otherwise false
*/
bool CDoSManager::Unban(const CSubNet &subNet)
{
    LOCK(cs_setBanned);
    if (setBanned.erase(subNet))
    {
        setBannedIsDirty = true;

        SweepBanned();
        uiInterface.BannedListChanged();
        return true;
    }
    return false;
}

/**
* Copies current in-memory banlist to the passed in banmap_t
* Intended to allow read-only actions on the banlist without holding the lock
*
* @param[in,out] banMap  The banlist copy
*/
void CDoSManager::GetBanned(banmap_t &banMap)
{
    LOCK(cs_setBanned);
    SweepBanned();
    banMap = setBanned; // create a thread safe copy
}

/**
* Overwrite the current in-memory banlist with the passed in banmap_t
* Marks the in-memory banlist as dirty
*
* @param[in] banMap  The new banlist to copy over in-memory banmap_t
*/
void CDoSManager::SetBanned(const banmap_t &banMap)
{
    LOCK(cs_setBanned);
    setBanned = banMap;
    setBannedIsDirty = true;
}

/**
* Iterates the in-memory banlist and removes any ban entries where the ban has expired
* Marks the in-memory banlist as dirty if any entries were removed
*/
void CDoSManager::SweepBanned()
{
    int64_t now = GetTime();

    LOCK(cs_setBanned);
    banmap_t::iterator it = setBanned.begin();
    while (it != setBanned.end())
    {
        CSubNet subNet = (*it).first;
        CBanEntry banEntry = (*it).second;
        if (now > banEntry.nBanUntil)
        {
            setBanned.erase(it++);
            setBannedIsDirty = true;
            LogPrint("net", "%s: Removed banned node ip/subnet from banlist.dat: %s\n", __func__, subNet.ToString());
        }
        else
            ++it;
    }
}

/**
* Check if the current banlist has changes not written to disk
*
* @return true if the in-memory banlist has changes not written to disk, otherwise false
*/
bool CDoSManager::BannedSetIsDirty()
{
    LOCK(cs_setBanned);
    return setBannedIsDirty;
}

/**
* Set flag indicating the in-memory banlist has changes not written to disk
*
* @param[in] dirty  Flag indicating if in-memory banlist has changes not written to disk
*/
void CDoSManager::SetBannedSetDirty(bool dirty)
{
    LOCK(cs_setBanned); // reuse setBanned lock for the isDirty flag
    setBannedIsDirty = dirty;
}

/**
* Set the ban score for this node and if the threshold is exceeded request the node be banned.
* NOTE: This requires an externally taken lock on cs_main to protect the CNodeState returned by
*       the internal call to State()
*
* REVISIT: There are numerous new calls to Misbehaving which have been recently added.
*          A subsequent PR will be added to ensure the lock on cs_main is properly taken,
*          adding unit tests for full coverage of all cases, and adding static analysis
*          tags and an AsserLockHeld to help catch/prevent future developer mistakes.
*
* @param[in] pnode    Id of the node which is misbehaving, used to look up the CNodeState
* @param[in] howmuch  Ban score for the latest infraction against this node
*/
void CDoSManager::Misbehaving(NodeId pnode, int howmuch)
{
    if (howmuch == 0)
        return;

    CNodeState *state = State(pnode);
    if (state == NULL)
        return;

    state->nMisbehavior += howmuch;
    int banscore = GetArg("-banscore", DEFAULT_BANSCORE_THRESHOLD);
    if (state->nMisbehavior >= banscore && state->nMisbehavior - howmuch < banscore)
    {
        LogPrintf("%s: %s (%d -> %d) BAN THRESHOLD EXCEEDED\n", __func__, state->name, state->nMisbehavior - howmuch,
            state->nMisbehavior);
        state->fShouldBan = true;
    }
    else
        LogPrintf("%s: %s (%d -> %d)\n", __func__, state->name, state->nMisbehavior - howmuch, state->nMisbehavior);
}
