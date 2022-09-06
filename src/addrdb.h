// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ADDRDB_H
#define BITCOIN_ADDRDB_H

#include <net_types.h>
#include <util/fs.h>
#include <util/result.h>

#include <memory>
#include <vector>

class ArgsManager;
class AddrMan;
class CAddress;
class DataStream;
class NetGroupManager;

/** Only used by tests. */
void ReadFromStream(AddrMan& addr, DataStream& ssPeers);

bool DumpPeerAddresses(const ArgsManager& args, const AddrMan& addr);

/** Access to the banlist database (banlist.json) */
class CBanDB
{
private:
    /**
     * JSON key under which the data is stored in the json database.
     */
    static constexpr const char* JSON_KEY = "banned_nets";

    const fs::path m_banlist_dat;
    const fs::path m_banlist_json;
public:
    explicit CBanDB(fs::path ban_list_path);
    bool Write(const banmap_t& banSet);

    /**
     * Read the banlist from disk.
     * @param[out] banSet The loaded list. Set if `true` is returned, otherwise it is left
     * in an undefined state.
     * @return true on success
     */
    bool Read(banmap_t& banSet);
};

/** Returns an error string on failure */
util::ResultPtr<std::unique_ptr<AddrMan>> LoadAddrman(const NetGroupManager& netgroupman, const ArgsManager& args);

/**
 * Dump the anchor IP address database (anchors.dat)
 *
 * Anchors are last known outgoing block-relay-only peers that are
 * tried to re-connect to on startup.
 */
void DumpAnchors(const fs::path& anchors_db_path, const std::vector<CAddress>& anchors);

/**
 * Read the anchor IP address database (anchors.dat)
 *
 * Deleting anchors.dat is intentional as it avoids renewed peering to anchors after
 * an unclean shutdown and thus potential exploitation of the anchor peer policy.
 */
std::vector<CAddress> ReadAnchors(const fs::path& anchors_db_path);

#endif // BITCOIN_ADDRDB_H
