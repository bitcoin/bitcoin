// Copyright (c) 2014-2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_NETFULFILLEDMAN_H
#define SYSCOIN_NETFULFILLEDMAN_H

#include <netaddress.h>
#include <serialize.h>
#include <sync.h>

class CNetFulfilledRequestManager;
extern CNetFulfilledRequestManager netfulfilledman;

// Fulfilled requests are used to prevent nodes from asking for the same data on sync
// and from being banned for doing so too often.
class CNetFulfilledRequestManager
{
private:
    typedef std::map<std::string, int64_t> fulfilledreqmapentry_t;
    typedef std::map<CService, fulfilledreqmapentry_t> fulfilledreqmap_t;

    //keep track of what node has/was asked for and when
    fulfilledreqmap_t mapFulfilledRequests;
    mutable RecursiveMutex cs_mapFulfilledRequests;

public:
    CNetFulfilledRequestManager() {}

    SERIALIZE_METHODS(CNetFulfilledRequestManager, obj)
    {
         LOCK(obj.cs_mapFulfilledRequests);
         READWRITE(obj.mapFulfilledRequests);
    }

    void AddFulfilledRequest(const CService& addr, const std::string& strRequest);
    bool HasFulfilledRequest(const CService& addr, const std::string& strRequest);

    void RemoveAllFulfilledRequests(const CService& addr);

    void CheckAndRemove();
    void Clear();

    std::string ToString() const;

    void DoMaintenance();
};

#endif // SYSCOIN_NETFULFILLEDMAN_H
