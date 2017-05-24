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

void CDoSManager::AddWhitelistedRange(const CSubNet &subnet)
{
    LOCK(cs_vWhitelistedRange);
    vWhitelistedRange.push_back(subnet);
}

void CDoSManager::ClearBanned()
{
    LOCK(cs_setBanned);
    setBanned.clear();
    setBannedIsDirty = true;
    uiInterface.BannedListChanged();
}

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

void CDoSManager::Ban(const CNetAddr &addr, const BanReason &banReason, int64_t bantimeoffset, bool sinceUnixEpoch)
{
    CSubNet subNet(addr);
    Ban(subNet, banReason, bantimeoffset, sinceUnixEpoch);
}

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

bool CDoSManager::Unban(const CNetAddr &addr)
{
    CSubNet subNet(addr);
    return Unban(subNet);
}

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

void CDoSManager::GetBanned(banmap_t &banMap)
{
    LOCK(cs_setBanned);
    SweepBanned();
    banMap = setBanned; // create a thread safe copy
}

void CDoSManager::SetBanned(const banmap_t &banMap)
{
    LOCK(cs_setBanned);
    setBanned = banMap;
    setBannedIsDirty = true;
}

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

bool CDoSManager::BannedSetIsDirty()
{
    LOCK(cs_setBanned);
    return setBannedIsDirty;
}

void CDoSManager::SetBannedSetDirty(bool dirty)
{
    LOCK(cs_setBanned); // reuse setBanned lock for the isDirty flag
    setBannedIsDirty = dirty;
}

// Requires cs_main.
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
