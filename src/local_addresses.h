// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LOCAL_ADDRESSES_H
#define BITCOIN_LOCAL_ADDRESSES_H

#include <netaddress.h>
#include <sync.h>

#include <optional>
#include <map>

enum
{
    LOCAL_NONE,   // unknown
    LOCAL_IF,     // address a local interface listens on
    LOCAL_BIND,   // address explicit bound to
    LOCAL_MAPPED, // address reported by PCP
    LOCAL_MANUAL, // address explicitly specified (-externalip=)

    LOCAL_MAX
};

struct LocalServiceInfo {
    int nScore;
    uint16_t nPort;
};

class LocalAddressManager
{
public:
    using map_type = std::map<CNetAddr, LocalServiceInfo>;

    // learn a new local address
    bool Add(const CService& addr_, int nScore = LOCAL_NONE) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    void Remove(const CNetAddr& addr) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** vote for a local address */
    bool Seen(const CNetAddr& addr) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** check whether a given address is potentially local */
    bool Contains(const CNetAddr& addr) const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    // Determine the "best" local address for a particular peer.
    [[nodiscard]] std::optional<CService> Get(const CNetAddr& addr, const Network& connected_through) const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    void Clear() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    int GetnScore(const CNetAddr& addr) const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    map_type GetAll() const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** Calculates a metric for how reachable (*this) is from a given partner */
    static int GetReachability(const CNetAddr& addr, const CNetAddr& paddrPartner);

private:
    mutable Mutex m_mutex;
    map_type m_addresses GUARDED_BY(m_mutex);
};

extern std::unique_ptr<LocalAddressManager> g_localaddressman;

#endif // BITCOIN_LOCAL_ADDRESSES_H
