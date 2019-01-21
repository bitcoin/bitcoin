// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_BANMAN_H
#define BITCOIN_BANMAN_H

#include <cstdint>
#include <memory>

#include <addrdb.h>
#include <fs.h>
#include <sync.h>

// NOTE: When adjusting this, update rpcnet:setban's help ("24h")
static constexpr unsigned int DEFAULT_MISBEHAVING_BANTIME = 60 * 60 * 24; // Default 24-hour ban

class CClientUIInterface;
class CNetAddr;
class CSubNet;

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

class BanMan
{
public:
    ~BanMan();
    BanMan(fs::path ban_file, CClientUIInterface* client_interface, int64_t default_ban_time);
    void Ban(const CNetAddr& net_addr, const BanReason& ban_reason, int64_t ban_time_offset = 0, bool since_unix_epoch = false);
    void Ban(const CSubNet& sub_net, const BanReason& ban_reason, int64_t ban_time_offset = 0, bool since_unix_epoch = false);
    void ClearBanned();
    bool IsBanned(CNetAddr net_addr);
    bool IsBanned(CSubNet sub_net);
    bool Unban(const CNetAddr& net_addr);
    bool Unban(const CSubNet& sub_net);
    void GetBanned(banmap_t& banmap);
    void DumpBanlist();

private:
    void SetBanned(const banmap_t& banmap);
    bool BannedSetIsDirty();
    //!set the "dirty" flag for the banlist
    void SetBannedSetDirty(bool dirty = true);
    //!clean unused entries (if bantime has expired)
    void SweepBanned();

    CCriticalSection m_cs_banned;
    banmap_t m_banned GUARDED_BY(m_cs_banned);
    bool m_is_dirty GUARDED_BY(m_cs_banned);
    CClientUIInterface* m_client_interface = nullptr;
    CBanDB m_ban_db;
    const int64_t m_default_ban_time;
};

extern std::unique_ptr<BanMan> g_banman;
#endif
