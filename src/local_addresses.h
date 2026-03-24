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

class CAddress;

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

    void Remove(const CService& addr) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** vote for a local address */
    bool Seen(const CService& addr) EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    /** check whether a given address is potentially local */
    bool Contains(const CService& addr) const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    // Determine the "best" local address for a particular peer.
    [[nodiscard]] std::optional<CService> Get(const CAddress& addr, const Network& connected_through) const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    void Clear() EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    int GetnScore(const CService& addr) const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

    map_type GetAll() const EXCLUSIVE_LOCKS_REQUIRED(!m_mutex);

private:
    mutable Mutex m_mutex;
    map_type m_addresses GUARDED_BY(m_mutex);
};

extern std::unique_ptr<LocalAddressManager> g_localaddressman;


// Temporary forwarding functions

inline void ClearLocal() { return g_localaddressman->Clear(); }

inline bool AddLocal(const CService& addr, int nScore = LOCAL_NONE) { return g_localaddressman->Add(addr, nScore); }

inline void RemoveLocal(const CService& addr) { return g_localaddressman->Remove(addr); }

inline bool SeenLocal(const CService& addr) { return g_localaddressman->Seen(addr); }

inline bool IsLocal(const CService& addr) { return g_localaddressman->Contains(addr); }

inline std::optional<CService> GetLocalAddress(const CAddress& addr, const Network& network) { return g_localaddressman->Get(addr, network); }

inline int GetnScore(const CService& addr) { return g_localaddressman->GetnScore(addr); }

inline LocalAddressManager::map_type getNetLocalAddresses() { return g_localaddressman->GetAll(); }

#endif // BITCOIN_LOCAL_ADDRESSES_H
