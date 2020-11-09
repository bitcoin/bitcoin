// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ADDRDB_H
#define BITCOIN_ADDRDB_H

#include <fs.h>
#include <net_types.h> // For banmap_t
#include <serialize.h>

#include <string>
#include <vector>

class CAddress;
class CAddrMan;
class CDataStream;

class CBanEntry
{
public:
    static const int CURRENT_VERSION=1;
    int nVersion;
    int64_t nCreateTime;
    int64_t nBanUntil;

    CBanEntry()
    {
        SetNull();
    }

    explicit CBanEntry(int64_t nCreateTimeIn)
    {
        SetNull();
        nCreateTime = nCreateTimeIn;
    }

    SERIALIZE_METHODS(CBanEntry, obj)
    {
        uint8_t ban_reason = 2; //! For backward compatibility
        READWRITE(obj.nVersion, obj.nCreateTime, obj.nBanUntil, ban_reason);
    }

    void SetNull()
    {
        nVersion = CBanEntry::CURRENT_VERSION;
        nCreateTime = 0;
        nBanUntil = 0;
    }
};

/** Access to the (IP) address database (peers.dat) */
class CAddrDB
{
private:
    fs::path pathAddr;
public:
    CAddrDB();
    bool Write(const CAddrMan& addr);
    bool Read(CAddrMan& addr);
    static bool Read(CAddrMan& addr, CDataStream& ssPeers);
};

/** Access to the banlist database (banlist.dat) */
class CBanDB
{
private:
    const fs::path m_ban_list_path;
public:
    explicit CBanDB(fs::path ban_list_path);
    bool Write(const banmap_t& banSet);
    bool Read(banmap_t& banSet);
};

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
