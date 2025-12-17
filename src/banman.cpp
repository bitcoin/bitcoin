// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <banman.h>

#include <common/system.h>
#include <logging.h>
#include <netaddress.h>
#include <node/interface_ui.h>
#include <sync.h>
#include <util/time.h>
#include <util/translation.h>


BanMan::BanMan(fs::path ban_file, CClientUIInterface* client_interface, int64_t default_ban_time)
    : m_client_interface(client_interface), m_ban_db(std::move(ban_file)), m_default_ban_time(default_ban_time)
{
    LoadBanlist();
    DumpBanlist();
}

BanMan::~BanMan()
{
    DumpBanlist();
}

void BanMan::LoadBanlist()
{
    LOCK(m_banned_mutex);

    if (m_client_interface) m_client_interface->InitMessage(_("Loading banlist…"));

    const auto start{SteadyClock::now()};
    if (m_ban_db.Read(m_banned)) {
        SweepBanned(); // sweep out unused entries

        LogDebug(BCLog::NET, "Loaded %d banned node addresses/subnets  %dms\n", m_banned.size(),
                 Ticks<std::chrono::milliseconds>(SteadyClock::now() - start));
    } else {
        LogInfo("Recreating the banlist database");
        m_banned = {};
        m_is_dirty = true;
    }
}

void BanMan::DumpBanlist()
{
    static Mutex dump_mutex;
    LOCK(dump_mutex);

    banmap_t banmap;
    {
        LOCK(m_banned_mutex);
        SweepBanned();
        if (!m_is_dirty) return;
        banmap = m_banned;
        m_is_dirty = false;
    }

    const auto start{SteadyClock::now()};
    if (!m_ban_db.Write(banmap)) {
        LOCK(m_banned_mutex);
        m_is_dirty = true;
    }

    LogDebug(BCLog::NET, "Flushed %d banned node addresses/subnets to disk  %dms\n", banmap.size(),
             Ticks<std::chrono::milliseconds>(SteadyClock::now() - start));
}

void BanMan::ClearBanned()
{
    {
        LOCK(m_banned_mutex);
        m_banned.clear();
        m_is_dirty = true;
    }
    DumpBanlist(); //store banlist to disk
    if (m_client_interface) m_client_interface->BannedListChanged();
}

bool BanMan::IsDiscouraged(const CNetAddr& net_addr)
{
    LOCK(m_banned_mutex);
    return m_discouraged.contains(net_addr.GetAddrBytes());
}

bool BanMan::IsBanned(const CNetAddr& net_addr)
{
    auto current_time = GetTime();
    LOCK(m_banned_mutex);
    for (const auto& it : m_banned) {
        CSubNet sub_net = it.first;
        CBanEntry ban_entry = it.second;

        if (current_time < ban_entry.nBanUntil && sub_net.Match(net_addr)) {
            return true;
        }
    }
    return false;
}

bool BanMan::IsBanned(const CSubNet& sub_net)
{
    auto current_time = GetTime();
    LOCK(m_banned_mutex);
    banmap_t::iterator i = m_banned.find(sub_net);
    if (i != m_banned.end()) {
        CBanEntry ban_entry = (*i).second;
        if (current_time < ban_entry.nBanUntil) {
            return true;
        }
    }
    return false;
}

void BanMan::Ban(const CNetAddr& net_addr, int64_t ban_time_offset, bool since_unix_epoch)
{
    CSubNet sub_net(net_addr);
    Ban(sub_net, ban_time_offset, since_unix_epoch);
}

void BanMan::Discourage(const CNetAddr& net_addr)
{
    LOCK(m_banned_mutex);
    m_discouraged.insert(net_addr.GetAddrBytes());
}

void BanMan::Ban(const CSubNet& sub_net, int64_t ban_time_offset, bool since_unix_epoch)
{
    CBanEntry ban_entry(GetTime());

    int64_t normalized_ban_time_offset = ban_time_offset;
    bool normalized_since_unix_epoch = since_unix_epoch;
    if (ban_time_offset <= 0) {
        normalized_ban_time_offset = m_default_ban_time;
        normalized_since_unix_epoch = false;
    }
    ban_entry.nBanUntil = (normalized_since_unix_epoch ? 0 : GetTime()) + normalized_ban_time_offset;

    {
        LOCK(m_banned_mutex);
        if (m_banned[sub_net].nBanUntil < ban_entry.nBanUntil) {
            m_banned[sub_net] = ban_entry;
            m_is_dirty = true;
        } else
            return;
    }
    if (m_client_interface) m_client_interface->BannedListChanged();

    //store banlist to disk immediately
    DumpBanlist();
}

bool BanMan::Unban(const CNetAddr& net_addr)
{
    CSubNet sub_net(net_addr);
    return Unban(sub_net);
}

bool BanMan::Unban(const CSubNet& sub_net)
{
    {
        LOCK(m_banned_mutex);
        if (m_banned.erase(sub_net) == 0) return false;
        m_is_dirty = true;
    }
    if (m_client_interface) m_client_interface->BannedListChanged();
    DumpBanlist(); //store banlist to disk immediately
    return true;
}

void BanMan::GetBanned(banmap_t& banmap)
{
    LOCK(m_banned_mutex);
    // Sweep the banlist so expired bans are not returned
    SweepBanned();
    banmap = m_banned; //create a thread safe copy
}

void BanMan::SweepBanned()
{
    AssertLockHeld(m_banned_mutex);

    int64_t now = GetTime();
    bool notify_ui = false;
    banmap_t::iterator it = m_banned.begin();
    while (it != m_banned.end()) {
        CSubNet sub_net = (*it).first;
        CBanEntry ban_entry = (*it).second;
        if (!sub_net.IsValid() || now > ban_entry.nBanUntil) {
            m_banned.erase(it++);
            m_is_dirty = true;
            notify_ui = true;
            LogDebug(BCLog::NET, "Removed banned node address/subnet: %s\n", sub_net.ToString());
        } else {
            ++it;
        }
    }

    // update UI
    if (notify_ui && m_client_interface) {
        m_client_interface->BannedListChanged();
    }
}
