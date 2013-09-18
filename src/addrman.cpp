// Copyright (c) 2012 Pieter Wuille
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "addrman.h"
#include "hash.h"

using namespace std;

int CAddrInfo::GetTriedBucket(const std::vector<unsigned char> &nKey) const
{
    CDataStream ss1(SER_GETHASH, 0);
    std::vector<unsigned char> vchKey = GetKey();
    ss1 << nKey << vchKey;
    uint64 hash1 = Hash(ss1.begin(), ss1.end()).Get64();

    CDataStream ss2(SER_GETHASH, 0);
    std::vector<unsigned char> vchGroupKey = GetGroup();
    ss2 << nKey << vchGroupKey << (hash1 % ADDRMAN_TRIED_BUCKETS_PER_GROUP);
    uint64 hash2 = Hash(ss2.begin(), ss2.end()).Get64();
    return hash2 % ADDRMAN_TRIED_BUCKET_COUNT;
}

int CAddrInfo::GetNewBucket(const std::vector<unsigned char> &nKey, const CNetAddr& src) const
{
    CDataStream ss1(SER_GETHASH, 0);
    std::vector<unsigned char> vchGroupKey = GetGroup();
    std::vector<unsigned char> vchSourceGroupKey = src.GetGroup();
    ss1 << nKey << vchGroupKey << vchSourceGroupKey;
    uint64 hash1 = Hash(ss1.begin(), ss1.end()).Get64();

    CDataStream ss2(SER_GETHASH, 0);
    ss2 << nKey << vchSourceGroupKey << (hash1 % ADDRMAN_NEW_BUCKETS_PER_SOURCE_GROUP);
    uint64 hash2 = Hash(ss2.begin(), ss2.end()).Get64();
    return hash2 % ADDRMAN_NEW_BUCKET_COUNT;
}

bool CAddrInfo::IsTerrible(int64 nNow) const
{
    if (nLastTry && nLastTry >= nNow-60) // never remove things tried the last minute
        return false;

    if (nTime > nNow + 10*60) // came in a flying DeLorean
        return true;

    if (nTime==0 || nNow-nTime > ADDRMAN_HORIZON_DAYS*86400) // not seen in over a month
        return true;

    if (nLastSuccess==0 && nAttempts>=ADDRMAN_RETRIES) // tried three times and never a success
        return true;

    if (nNow-nLastSuccess > ADDRMAN_MIN_FAIL_DAYS*86400 && nAttempts>=ADDRMAN_MAX_FAILURES) // 10 successive failures in the last week
        return true;

    return false;
}

double CAddrInfo::GetChance(int64 nNow) const
{
    double fChance = 1.0;

    int64 nSinceLastSeen = nNow - nTime;
    int64 nSinceLastTry = nNow - nLastTry;

    if (nSinceLastSeen < 0) nSinceLastSeen = 0;
    if (nSinceLastTry < 0) nSinceLastTry = 0;

    fChance *= 600.0 / (600.0 + nSinceLastSeen);

    // deprioritize very recent attempts away
    if (nSinceLastTry < 60*10)
        fChance *= 0.01;

    // deprioritize 50% after each failed attempt
    for (int n=0; n<nAttempts; n++)
        fChance /= 1.5;

    return fChance;
}

CAddrInfo* CAddrMan::Find(const CNetAddr& addr, int *pnId)
{
    std::map<CNetAddr, int>::iterator it = mapAddr.find(addr);
    if (it == mapAddr.end())
        return NULL;
    if (pnId)
        *pnId = (*it).second;
    std::map<int, CAddrInfo>::iterator it2 = mapInfo.find((*it).second);
    if (it2 != mapInfo.end())
        return &(*it2).second;
    return NULL;
}

CAddrInfo* CAddrMan::Create(const CAddress &addr, const CNetAddr &addrSource, int *pnId)
{
    int nId = nIdCount++;
    mapInfo[nId] = CAddrInfo(addr, addrSource);
    mapAddr[addr] = nId;
    mapInfo[nId].nRandomPos = vRandom.size();
    vRandom.push_back(nId);
    if (pnId)
        *pnId = nId;
    return &mapInfo[nId];
}

void CAddrMan::SwapRandom(unsigned int nRndPos1, unsigned int nRndPos2)
{
    if (nRndPos1 == nRndPos2)
        return;

    assert(nRndPos1 < vRandom.size() && nRndPos2 < vRandom.size());

    int nId1 = vRandom[nRndPos1];
    int nId2 = vRandom[nRndPos2];

    assert(mapInfo.count(nId1) == 1);
    assert(mapInfo.count(nId2) == 1);

    mapInfo[nId1].nRandomPos = nRndPos2;
    mapInfo[nId2].nRandomPos = nRndPos1;

    vRandom[nRndPos1] = nId2;
    vRandom[nRndPos2] = nId1;
}

int CAddrMan::SelectTried(int nKBucket)
{
    std::vector<int> &vTried = vvTried[nKBucket];

    // random shuffle the first few elements (using the entire list)
    // find the least recently tried among them
    int64 nOldest = -1;
    int nOldestPos = -1;
    for (unsigned int i = 0; i < ADDRMAN_TRIED_ENTRIES_INSPECT_ON_EVICT && i < vTried.size(); i++)
    {
        int nPos = GetRandInt(vTried.size() - i) + i;
        int nTemp = vTried[nPos];
        vTried[nPos] = vTried[i];
        vTried[i] = nTemp;
        assert(nOldest == -1 || mapInfo.count(nTemp) == 1);
        if (nOldest == -1 || mapInfo[nTemp].nLastSuccess < mapInfo[nOldest].nLastSuccess) {
           nOldest = nTemp;
           nOldestPos = nPos;
        }
    }

    return nOldestPos;
}

int CAddrMan::ShrinkNew(int nUBucket)
{
    assert(nUBucket >= 0 && (unsigned int)nUBucket < vvNew.size());
    std::set<int> &vNew = vvNew[nUBucket];

    // first look for deletable items
    for (std::set<int>::iterator it = vNew.begin(); it != vNew.end(); it++)
    {
        assert(mapInfo.count(*it));
        CAddrInfo &info = mapInfo[*it];
        if (info.IsTerrible())
        {
            if (--info.nRefCount == 0)
            {
                SwapRandom(info.nRandomPos, vRandom.size()-1);
                vRandom.pop_back();
                mapAddr.erase(info);
                mapInfo.erase(*it);
                nNew--;
            }
            vNew.erase(it);
            return 0;
        }
    }

    // otherwise, select four randomly, and pick the oldest of those to replace
    int n[4] = {GetRandInt(vNew.size()), GetRandInt(vNew.size()), GetRandInt(vNew.size()), GetRandInt(vNew.size())};
    int nI = 0;
    int nOldest = -1;
    for (std::set<int>::iterator it = vNew.begin(); it != vNew.end(); it++)
    {
        if (nI == n[0] || nI == n[1] || nI == n[2] || nI == n[3])
        {
            assert(nOldest == -1 || mapInfo.count(*it) == 1);
            if (nOldest == -1 || mapInfo[*it].nTime < mapInfo[nOldest].nTime)
                nOldest = *it;
        }
        nI++;
    }
    assert(mapInfo.count(nOldest) == 1);
    CAddrInfo &info = mapInfo[nOldest];
    if (--info.nRefCount == 0)
    {
        SwapRandom(info.nRandomPos, vRandom.size()-1);
        vRandom.pop_back();
        mapAddr.erase(info);
        mapInfo.erase(nOldest);
        nNew--;
    }
    vNew.erase(nOldest);

    return 1;
}

void CAddrMan::MakeTried(CAddrInfo& info, int nId, int nOrigin)
{
    assert(vvNew[nOrigin].count(nId) == 1);

    // remove the entry from all new buckets
    for (std::vector<std::set<int> >::iterator it = vvNew.begin(); it != vvNew.end(); it++)
    {
        if ((*it).erase(nId))
            info.nRefCount--;
    }
    nNew--;

    assert(info.nRefCount == 0);

    // what tried bucket to move the entry to
    int nKBucket = info.GetTriedBucket(nKey);
    std::vector<int> &vTried = vvTried[nKBucket];

    // first check whether there is place to just add it
    if (vTried.size() < ADDRMAN_TRIED_BUCKET_SIZE)
    {
        vTried.push_back(nId);
        nTried++;
        info.fInTried = true;
        return;
    }

    // otherwise, find an item to evict
    int nPos = SelectTried(nKBucket);

    // find which new bucket it belongs to
    assert(mapInfo.count(vTried[nPos]) == 1);
    int nUBucket = mapInfo[vTried[nPos]].GetNewBucket(nKey);
    std::set<int> &vNew = vvNew[nUBucket];

    // remove the to-be-replaced tried entry from the tried set
    CAddrInfo& infoOld = mapInfo[vTried[nPos]];
    infoOld.fInTried = false;
    infoOld.nRefCount = 1;
    // do not update nTried, as we are going to move something else there immediately

    // check whether there is place in that one,
    if (vNew.size() < ADDRMAN_NEW_BUCKET_SIZE)
    {
        // if so, move it back there
        vNew.insert(vTried[nPos]);
    } else {
        // otherwise, move it to the new bucket nId came from (there is certainly place there)
        vvNew[nOrigin].insert(vTried[nPos]);
    }
    nNew++;

    vTried[nPos] = nId;
    // we just overwrote an entry in vTried; no need to update nTried
    info.fInTried = true;
    return;
}

void CAddrMan::Good_(const CService &addr, int64 nTime)
{
    int nId;
    CAddrInfo *pinfo = Find(addr, &nId);

    // if not found, bail out
    if (!pinfo)
        return;

    CAddrInfo &info = *pinfo;

    // check whether we are talking about the exact same CService (including same port)
    if (info != addr)
        return;

    // update info
    info.nLastSuccess = nTime;
    info.nLastTry = nTime;
    info.nTime = nTime;
    info.nAttempts = 0;

    // if it is already in the tried set, don't do anything else
    if (info.fInTried)
        return;

    // find a bucket it is in now
    int nRnd = GetRandInt(vvNew.size());
    int nUBucket = -1;
    for (unsigned int n = 0; n < vvNew.size(); n++)
    {
        int nB = (n+nRnd) % vvNew.size();
        std::set<int> &vNew = vvNew[nB];
        if (vNew.count(nId))
        {
            nUBucket = nB;
            break;
        }
    }

    // if no bucket is found, something bad happened;
    // TODO: maybe re-add the node, but for now, just bail out
    if (nUBucket == -1) return;

    LogPrint("addrman", "Moving %s to tried\n", addr.ToString().c_str());

    // move nId to the tried tables
    MakeTried(info, nId, nUBucket);
}

bool CAddrMan::Add_(const CAddress &addr, const CNetAddr& source, int64 nTimePenalty)
{
    if (!addr.IsRoutable())
        return false;

    bool fNew = false;
    int nId;
    CAddrInfo *pinfo = Find(addr, &nId);

    if (pinfo)
    {
        // periodically update nTime
        bool fCurrentlyOnline = (GetAdjustedTime() - addr.nTime < 24 * 60 * 60);
        int64 nUpdateInterval = (fCurrentlyOnline ? 60 * 60 : 24 * 60 * 60);
        if (addr.nTime && (!pinfo->nTime || pinfo->nTime < addr.nTime - nUpdateInterval - nTimePenalty))
            pinfo->nTime = max((int64)0, addr.nTime - nTimePenalty);

        // add services
        pinfo->nServices |= addr.nServices;

        // do not update if no new information is present
        if (!addr.nTime || (pinfo->nTime && addr.nTime <= pinfo->nTime))
            return false;

        // do not update if the entry was already in the "tried" table
        if (pinfo->fInTried)
            return false;

        // do not update if the max reference count is reached
        if (pinfo->nRefCount == ADDRMAN_NEW_BUCKETS_PER_ADDRESS)
            return false;

        // stochastic test: previous nRefCount == N: 2^N times harder to increase it
        int nFactor = 1;
        for (int n=0; n<pinfo->nRefCount; n++)
            nFactor *= 2;
        if (nFactor > 1 && (GetRandInt(nFactor) != 0))
            return false;
    } else {
        pinfo = Create(addr, source, &nId);
        pinfo->nTime = max((int64)0, (int64)pinfo->nTime - nTimePenalty);
        nNew++;
        fNew = true;
    }

    int nUBucket = pinfo->GetNewBucket(nKey, source);
    std::set<int> &vNew = vvNew[nUBucket];
    if (!vNew.count(nId))
    {
        pinfo->nRefCount++;
        if (vNew.size() == ADDRMAN_NEW_BUCKET_SIZE)
            ShrinkNew(nUBucket);
        vvNew[nUBucket].insert(nId);
    }
    return fNew;
}

void CAddrMan::Attempt_(const CService &addr, int64 nTime)
{
    CAddrInfo *pinfo = Find(addr);

    // if not found, bail out
    if (!pinfo)
        return;

    CAddrInfo &info = *pinfo;

    // check whether we are talking about the exact same CService (including same port)
    if (info != addr)
        return;

    // update info
    info.nLastTry = nTime;
    info.nAttempts++;
}

CAddress CAddrMan::Select_(int nUnkBias)
{
    if (size() == 0)
        return CAddress();

    double nCorTried = sqrt(nTried) * (100.0 - nUnkBias);
    double nCorNew = sqrt(nNew) * nUnkBias;
    if ((nCorTried + nCorNew)*GetRandInt(1<<30)/(1<<30) < nCorTried)
    {
        // use a tried node
        double fChanceFactor = 1.0;
        while(1)
        {
            int nKBucket = GetRandInt(vvTried.size());
            std::vector<int> &vTried = vvTried[nKBucket];
            if (vTried.size() == 0) continue;
            int nPos = GetRandInt(vTried.size());
            assert(mapInfo.count(vTried[nPos]) == 1);
            CAddrInfo &info = mapInfo[vTried[nPos]];
            if (GetRandInt(1<<30) < fChanceFactor*info.GetChance()*(1<<30))
                return info;
            fChanceFactor *= 1.2;
        }
    } else {
        // use a new node
        double fChanceFactor = 1.0;
        while(1)
        {
            int nUBucket = GetRandInt(vvNew.size());
            std::set<int> &vNew = vvNew[nUBucket];
            if (vNew.size() == 0) continue;
            int nPos = GetRandInt(vNew.size());
            std::set<int>::iterator it = vNew.begin();
            while (nPos--)
                it++;
            assert(mapInfo.count(*it) == 1);
            CAddrInfo &info = mapInfo[*it];
            if (GetRandInt(1<<30) < fChanceFactor*info.GetChance()*(1<<30))
                return info;
            fChanceFactor *= 1.2;
        }
    }
}

#ifdef DEBUG_ADDRMAN
int CAddrMan::Check_()
{
    std::set<int> setTried;
    std::map<int, int> mapNew;

    if (vRandom.size() != nTried + nNew) return -7;

    for (std::map<int, CAddrInfo>::iterator it = mapInfo.begin(); it != mapInfo.end(); it++)
    {
        int n = (*it).first;
        CAddrInfo &info = (*it).second;
        if (info.fInTried)
        {

            if (!info.nLastSuccess) return -1;
            if (info.nRefCount) return -2;
            setTried.insert(n);
        } else {
            if (info.nRefCount < 0 || info.nRefCount > ADDRMAN_NEW_BUCKETS_PER_ADDRESS) return -3;
            if (!info.nRefCount) return -4;
            mapNew[n] = info.nRefCount;
        }
        if (mapAddr[info] != n) return -5;
        if (info.nRandomPos<0 || info.nRandomPos>=vRandom.size() || vRandom[info.nRandomPos] != n) return -14;
        if (info.nLastTry < 0) return -6;
        if (info.nLastSuccess < 0) return -8;
    }

    if (setTried.size() != nTried) return -9;
    if (mapNew.size() != nNew) return -10;

    for (int n=0; n<vvTried.size(); n++)
    {
        std::vector<int> &vTried = vvTried[n];
        for (std::vector<int>::iterator it = vTried.begin(); it != vTried.end(); it++)
        {
            if (!setTried.count(*it)) return -11;
            setTried.erase(*it);
        }
    }

    for (int n=0; n<vvNew.size(); n++)
    {
        std::set<int> &vNew = vvNew[n];
        for (std::set<int>::iterator it = vNew.begin(); it != vNew.end(); it++)
        {
            if (!mapNew.count(*it)) return -12;
            if (--mapNew[*it] == 0)
                mapNew.erase(*it);
        }
    }

    if (setTried.size()) return -13;
    if (mapNew.size()) return -15;

    return 0;
}
#endif

void CAddrMan::GetAddr_(std::vector<CAddress> &vAddr)
{
    int nNodes = ADDRMAN_GETADDR_MAX_PCT*vRandom.size()/100;
    if (nNodes > ADDRMAN_GETADDR_MAX)
        nNodes = ADDRMAN_GETADDR_MAX;

    // perform a random shuffle over the first nNodes elements of vRandom (selecting from all)
    for (int n = 0; n<nNodes; n++)
    {
        int nRndPos = GetRandInt(vRandom.size() - n) + n;
        SwapRandom(n, nRndPos);
        assert(mapInfo.count(vRandom[n]) == 1);
        vAddr.push_back(mapInfo[vRandom[n]]);
    }
}

void CAddrMan::Connected_(const CService &addr, int64 nTime)
{
    CAddrInfo *pinfo = Find(addr);

    // if not found, bail out
    if (!pinfo)
        return;

    CAddrInfo &info = *pinfo;

    // check whether we are talking about the exact same CService (including same port)
    if (info != addr)
        return;

    // update info
    int64 nUpdateInterval = 20 * 60;
    if (nTime - info.nTime > nUpdateInterval)
        info.nTime = nTime;
}
