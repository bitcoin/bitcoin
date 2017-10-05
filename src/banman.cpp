// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <banman.h>

#include <netaddress.h>
#include <ui_interface.h>
#include <util/system.h>
#include <util/time.h>


BanMan::BanMan(fs::path ban_file, CClientUIInterface* client_interface, int64_t default_ban_time)
    : clientInterface(client_interface), m_ban_db(std::move(ban_file)), m_default_ban_time(default_ban_time)
{
    if (clientInterface) clientInterface->InitMessage(_("Loading banlist..."));

    int64_t nStart = GetTimeMillis();
    setBannedIsDirty = false;
    banmap_t banmap;
    if (m_ban_db.Read(banmap)) {
        SetBanned(banmap);        // thread save setter
        SetBannedSetDirty(false); // no need to write down, just read data
        SweepBanned();            // sweep out unused entries

        LogPrint(BCLog::NET, "Loaded %d banned node ips/subnets from banlist.dat  %dms\n",
            banmap.size(), GetTimeMillis() - nStart);
    } else {
        LogPrintf("Invalid or missing banlist.dat; recreating\n");
        SetBannedSetDirty(true); // force write
        DumpBanlist();
    }
}

BanMan::~BanMan()
{
    DumpBanlist();
}

void BanMan::DumpBanlist()
{
    SweepBanned(); // clean unused entries (if bantime has expired)

    if (!BannedSetIsDirty()) return;

    int64_t nStart = GetTimeMillis();

    banmap_t banmap;
    GetBanned(banmap);
    if (m_ban_db.Write(banmap)) {
        SetBannedSetDirty(false);
    }

    LogPrint(BCLog::NET, "Flushed %d banned node ips/subnets to banlist.dat  %dms\n",
        banmap.size(), GetTimeMillis() - nStart);
}

void BanMan::ClearBanned()
{
    {
        LOCK(cs_setBanned);
        setBanned.clear();
        setBannedIsDirty = true;
    }
    DumpBanlist(); //store banlist to disk
    if (clientInterface) clientInterface->BannedListChanged();
}

bool BanMan::IsBanned(CNetAddr netAddr)
{
    LOCK(cs_setBanned);
    for (const auto& it : setBanned) {
        CSubNet subNet = it.first;
        CBanEntry banEntry = it.second;

        if (subNet.Match(netAddr) && GetTime() < banEntry.nBanUntil) {
            return true;
        }
    }
    return false;
}

bool BanMan::IsBanned(CSubNet subNet)
{
    LOCK(cs_setBanned);
    banmap_t::iterator i = setBanned.find(subNet);
    if (i != setBanned.end()) {
        CBanEntry banEntry = (*i).second;
        if (GetTime() < banEntry.nBanUntil) {
            return true;
        }
    }
    return false;
}

void BanMan::Ban(const CNetAddr& netAddr, const BanReason& banReason, int64_t bantimeoffset, bool sinceUnixEpoch)
{
    CSubNet subNet(netAddr);
    Ban(subNet, banReason, bantimeoffset, sinceUnixEpoch);
}

void BanMan::Ban(const CSubNet& subNet, const BanReason& banReason, int64_t bantimeoffset, bool sinceUnixEpoch)
{
    CBanEntry banEntry(GetTime());
    banEntry.banReason = banReason;
    if (bantimeoffset <= 0) {
        bantimeoffset = m_default_ban_time;
        sinceUnixEpoch = false;
    }
    banEntry.nBanUntil = (sinceUnixEpoch ? 0 : GetTime()) + bantimeoffset;

    {
        LOCK(cs_setBanned);
        if (setBanned[subNet].nBanUntil < banEntry.nBanUntil) {
            setBanned[subNet] = banEntry;
            setBannedIsDirty = true;
        } else
            return;
    }
    if (clientInterface) clientInterface->BannedListChanged();

    //store banlist to disk immediately if user requested ban
    if (banReason == BanReasonManuallyAdded) DumpBanlist();
}

bool BanMan::Unban(const CNetAddr& netAddr)
{
    CSubNet subNet(netAddr);
    return Unban(subNet);
}

bool BanMan::Unban(const CSubNet& subNet)
{
    {
        LOCK(cs_setBanned);
        if (setBanned.erase(subNet) == 0) return false;
        setBannedIsDirty = true;
    }
    if (clientInterface) clientInterface->BannedListChanged();
    DumpBanlist(); //store banlist to disk immediately
    return true;
}

void BanMan::GetBanned(banmap_t& banMap)
{
    LOCK(cs_setBanned);
    // Sweep the banlist so expired bans are not returned
    SweepBanned();
    banMap = setBanned; //create a thread safe copy
}

void BanMan::SetBanned(const banmap_t& banMap)
{
    LOCK(cs_setBanned);
    setBanned = banMap;
    setBannedIsDirty = true;
}

void BanMan::SweepBanned()
{
    int64_t now = GetTime();
    bool notifyUI = false;
    {
        LOCK(cs_setBanned);
        banmap_t::iterator it = setBanned.begin();
        while (it != setBanned.end()) {
            CSubNet subNet = (*it).first;
            CBanEntry banEntry = (*it).second;
            if (now > banEntry.nBanUntil) {
                setBanned.erase(it++);
                setBannedIsDirty = true;
                notifyUI = true;
                LogPrint(BCLog::NET, "%s: Removed banned node ip/subnet from banlist.dat: %s\n", __func__, subNet.ToString());
            } else
                ++it;
        }
    }
    // update UI
    if (notifyUI && clientInterface) {
        clientInterface->BannedListChanged();
    }
}

bool BanMan::BannedSetIsDirty()
{
    LOCK(cs_setBanned);
    return setBannedIsDirty;
}

void BanMan::SetBannedSetDirty(bool dirty)
{
    LOCK(cs_setBanned); //reuse setBanned lock for the setBannedIsDirty flag
    setBannedIsDirty = dirty;
}
