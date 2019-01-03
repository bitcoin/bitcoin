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
    : m_client_interface(client_interface), m_ban_db(std::move(ban_file)), m_default_ban_time(default_ban_time)
{
    if (m_client_interface) m_client_interface->InitMessage(_("Loading banlist..."));

    int64_t nStart = GetTimeMillis();
    m_is_dirty = false;
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
        LOCK(m_cs_banned);
        m_banned.clear();
        m_is_dirty = true;
    }
    DumpBanlist(); //store banlist to disk
    if (m_client_interface) m_client_interface->BannedListChanged();
}

bool BanMan::IsBanned(CNetAddr netAddr)
{
    LOCK(m_cs_banned);
    for (const auto& it : m_banned) {
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
    LOCK(m_cs_banned);
    banmap_t::iterator i = m_banned.find(subNet);
    if (i != m_banned.end()) {
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
    CBanEntry banEntry(GetTime(), banReason);

    int64_t normalized_bantimeoffset = bantimeoffset;
    bool normalized_sinceUnixEpoch = sinceUnixEpoch;
    if (bantimeoffset <= 0) {
        normalized_bantimeoffset = m_default_ban_time;
        normalized_sinceUnixEpoch = false;
    }
    banEntry.nBanUntil = (normalized_sinceUnixEpoch ? 0 : GetTime()) + normalized_bantimeoffset;

    {
        LOCK(m_cs_banned);
        if (m_banned[subNet].nBanUntil < banEntry.nBanUntil) {
            m_banned[subNet] = banEntry;
            m_is_dirty = true;
        } else
            return;
    }
    if (m_client_interface) m_client_interface->BannedListChanged();

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
        LOCK(m_cs_banned);
        if (m_banned.erase(subNet) == 0) return false;
        m_is_dirty = true;
    }
    if (m_client_interface) m_client_interface->BannedListChanged();
    DumpBanlist(); //store banlist to disk immediately
    return true;
}

void BanMan::GetBanned(banmap_t& banMap)
{
    LOCK(m_cs_banned);
    // Sweep the banlist so expired bans are not returned
    SweepBanned();
    banMap = m_banned; //create a thread safe copy
}

void BanMan::SetBanned(const banmap_t& banMap)
{
    LOCK(m_cs_banned);
    m_banned = banMap;
    m_is_dirty = true;
}

void BanMan::SweepBanned()
{
    int64_t now = GetTime();
    bool notifyUI = false;
    {
        LOCK(m_cs_banned);
        banmap_t::iterator it = m_banned.begin();
        while (it != m_banned.end()) {
            CSubNet subNet = (*it).first;
            CBanEntry banEntry = (*it).second;
            if (now > banEntry.nBanUntil) {
                m_banned.erase(it++);
                m_is_dirty = true;
                notifyUI = true;
                LogPrint(BCLog::NET, "%s: Removed banned node ip/subnet from banlist.dat: %s\n", __func__, subNet.ToString());
            } else
                ++it;
        }
    }
    // update UI
    if (notifyUI && m_client_interface) {
        m_client_interface->BannedListChanged();
    }
}

bool BanMan::BannedSetIsDirty()
{
    LOCK(m_cs_banned);
    return m_is_dirty;
}

void BanMan::SetBannedSetDirty(bool dirty)
{
    LOCK(m_cs_banned); //reuse m_banned lock for the m_is_dirty flag
    m_is_dirty = dirty;
}
