// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_BANMAN_H
#define BITCOIN_BANMAN_H

#include <addrdb.h>
#include <bloom.h>
#include <fs.h>
#include <net_types.h> // For banmap_t
#include <sync.h>

#include <chrono>
#include <cstdint>
#include <memory>

// NOTE: When adjusting this, update rpcnet:setban's help ("24h")
static constexpr unsigned int DEFAULT_MISBEHAVING_BANTIME = 60 * 60 * 24; // Default 24-hour ban
// How often to dump addresses to banlist.dat
static constexpr std::chrono::minutes DUMP_BANS_INTERVAL{15};

class CClientUIInterface;
class CNetAddr;
class CSubNet;

// Banman manages two related but distinct concepts:
//
// 1. Banning. This is configured manually by the user, through the setban RPC.
// If an address or subnet is banned, we never accept incoming connections from
// it and never create outgoing connections to it. We won't gossip its address
// to other peers in addr messages. Banned addresses and subnets are stored to
// banlist.dat on shutdown and reloaded on startup. Banning can be used to
// prevent connections with spy nodes or other griefers.
//
// 2. Discouragement. If a peer misbehaves enough (see Misbehaving() in
// net_processing.cpp), we'll mark that address as discouraged. We still allow
// incoming connections from them, but they're preferred for eviction when
// we receive new incoming connections. We never make outgoing connections to
// them, and do not gossip their address to other peers. This is implemented as
// a bloom filter. We can (probabilistically) test for membership, but can't
// list all discouraged addresses or unmark them as discouraged. Discouragement
// can prevent our limited connection slots being used up by incompatible
// or broken peers.
//
// Neither banning nor discouragement are protections against denial-of-service
// attacks, since if an attacker has a way to waste our resources and we
// disconnect from them and ban that address, it's trivial for them to
// reconnect from another IP address.
//
// Attempting to automatically disconnect or ban any class of peer carries the
// risk of splitting the network. For example, if we banned/disconnected for a
// transaction that fails a policy check and a future version changes the
// policy check so the transaction is accepted, then that transaction could
// cause the network to split between old nodes and new nodes.

class BanMan
{
public:
    ~BanMan();
    BanMan(fs::path ban_file, CClientUIInterface* client_interface, int64_t default_ban_time);

    //! Returns true if a new ban is added
    // Updates existing ban entries with longer ones
    bool Ban(const CSubNet& sub_net, int64_t ban_time_offset = 0, bool since_unix_epoch = false);

    void Discourage(const CNetAddr& net_addr);
    void ClearBanned();

    //! Return whether net_addr is banned
    bool IsBanned(const CNetAddr& net_addr);

    //! Return whether sub_net is exactly banned
    bool IsBanned(const CSubNet& sub_net);

    //! Return whether net_addr is discouraged.
    bool IsDiscouraged(const CNetAddr& net_addr);

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

    RecursiveMutex m_cs_banned;
    banmap_t m_banned GUARDED_BY(m_cs_banned);
    bool m_is_dirty GUARDED_BY(m_cs_banned);
    CClientUIInterface* m_client_interface = nullptr;
    CBanDB m_ban_db;
    const int64_t m_default_ban_time;
    CRollingBloomFilter m_discouraged GUARDED_BY(m_cs_banned) {50000, 0.000001};
};

#endif
