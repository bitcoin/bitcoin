// Copyright (c) 2014-2023 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NETFULFILLEDMAN_H
#define BITCOIN_NETFULFILLEDMAN_H

#include <netaddress.h>
#include <serialize.h>
#include <sync.h>

#include <memory>

template<typename T>
class CFlatDB;
class CNetFulfilledRequestManager;

class NetFulfilledRequestStore
{
protected:
    typedef std::map<std::string, int64_t> fulfilledreqmapentry_t;
    typedef std::map<CService, fulfilledreqmapentry_t> fulfilledreqmap_t;

protected:
    //keep track of what node has/was asked for and when
    fulfilledreqmap_t mapFulfilledRequests;
    mutable RecursiveMutex cs_mapFulfilledRequests;

public:
    SERIALIZE_METHODS(NetFulfilledRequestStore, obj)
    {
        LOCK(obj.cs_mapFulfilledRequests);
        READWRITE(obj.mapFulfilledRequests);
    }

    void Clear();

    std::string ToString() const;
};

// Fulfilled requests are used to prevent nodes from asking for the same data on sync
// and from being banned for doing so too often.
class CNetFulfilledRequestManager : public NetFulfilledRequestStore
{
private:
    using db_type = CFlatDB<NetFulfilledRequestStore>;

private:
    const std::unique_ptr<db_type> m_db;
    const bool is_valid{false};

public:
    explicit CNetFulfilledRequestManager(bool load_cache);
    ~CNetFulfilledRequestManager();

    bool IsValid() const { return is_valid; }
    void CheckAndRemove();

    void AddFulfilledRequest(const CService& addr, const std::string& strRequest);
    bool HasFulfilledRequest(const CService& addr, const std::string& strRequest);

    void RemoveAllFulfilledRequests(const CService& addr);

    void DoMaintenance();
};

extern std::unique_ptr<CNetFulfilledRequestManager> netfulfilledman;

#endif // BITCOIN_NETFULFILLEDMAN_H
