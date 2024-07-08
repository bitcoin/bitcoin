// Copyright (c) 2014-2023 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <flatdatabase.h>
#include <netfulfilledman.h>
#include <shutdown.h>
std::unique_ptr<CNetFulfilledRequestManager> netfulfilledman;
CNetFulfilledRequestManager::CNetFulfilledRequestManager() :
    m_db{std::make_unique<db_type>("netfulfilled.dat", "magicFulfilledCache")}
{
}

bool CNetFulfilledRequestManager::LoadCache(bool load_cache)
{
    assert(m_db != nullptr);
    is_valid = load_cache ? m_db->Load(*this) : m_db->Store(*this);
    if (is_valid && load_cache) {
        CheckAndRemove();
    }
    return is_valid;
}

CNetFulfilledRequestManager::~CNetFulfilledRequestManager()
{
    if (!is_valid) return;
    m_db->Store(*this);
}

void CNetFulfilledRequestManager::AddFulfilledRequest(const CService& addr, const std::string& strRequest)
{
    LOCK(cs_mapFulfilledRequests);
    mapFulfilledRequests[addr][strRequest] = GetTime() + Params().FulfilledRequestExpireTime();
}

bool CNetFulfilledRequestManager::HasFulfilledRequest(const CService& addr, const std::string& strRequest)
{
    LOCK(cs_mapFulfilledRequests);
    fulfilledreqmap_t::iterator it = mapFulfilledRequests.find(addr);

    return  it != mapFulfilledRequests.end() &&
            it->second.find(strRequest) != it->second.end() &&
            it->second[strRequest] > GetTime();
}

void CNetFulfilledRequestManager::RemoveAllFulfilledRequests(const CService& addr)
{
    LOCK(cs_mapFulfilledRequests);
    fulfilledreqmap_t::iterator it = mapFulfilledRequests.find(addr);

    if (it != mapFulfilledRequests.end()) {
        mapFulfilledRequests.erase(it++);
    }
}

void CNetFulfilledRequestManager::CheckAndRemove()
{
    LOCK(cs_mapFulfilledRequests);

    int64_t now = GetTime();
    fulfilledreqmap_t::iterator it = mapFulfilledRequests.begin();

    while(it != mapFulfilledRequests.end()) {
        fulfilledreqmapentry_t::iterator it_entry = it->second.begin();
        while(it_entry != it->second.end()) {
            if(now > it_entry->second) {
                it->second.erase(it_entry++);
            } else {
                ++it_entry;
            }
        }
        if(it->second.size() == 0) {
            mapFulfilledRequests.erase(it++);
        } else {
            ++it;
        }
    }
}

void NetFulfilledRequestStore::Clear()
{
    LOCK(cs_mapFulfilledRequests);
    mapFulfilledRequests.clear();
}

std::string NetFulfilledRequestStore::ToString() const
{
    std::ostringstream info;
    info << "Nodes with fulfilled requests: " << (int)mapFulfilledRequests.size();
    return info.str();
}

void CNetFulfilledRequestManager::DoMaintenance()
{
    if (ShutdownRequested()) return;

    CheckAndRemove();
}
