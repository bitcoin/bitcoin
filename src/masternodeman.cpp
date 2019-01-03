// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "activemasternode.h"
#include "addrman.h"
#include "alert.h"
#include "clientversion.h"
#include "init.h"
#include "governance.h"
#include "masternode-payments.h"
#include "masternode-sync.h"
#include "masternodeman.h"
#include "messagesigner.h"
#include "netfulfilledman.h"
#include "netmessagemaker.h"
#ifdef ENABLE_WALLET
#include "privatesend-client.h"
#endif // ENABLE_WALLET
#include "script/standard.h"
#include "ui_interface.h"
#include "util.h"
#include "warnings.h"

#include "evo/deterministicmns.h"
#include "evo/providertx.h"

/** Masternode manager */
CMasternodeMan mnodeman;

const std::string CMasternodeMan::SERIALIZATION_VERSION_STRING = "CMasternodeMan-Version-13";

struct CompareScoreMN
{
    bool operator()(const std::pair<arith_uint256, const CDeterministicMNCPtr&>& t1,
                    const std::pair<arith_uint256, const CDeterministicMNCPtr&>& t2) const
    {
        return (t1.first != t2.first) ? (t1.first < t2.first) : (t1.second->collateralOutpoint < t2.second->collateralOutpoint);
    }
};

CMasternodeMan::CMasternodeMan():
    cs(),
    mapMasternodes(),
    vecDirtyGovernanceObjectHashes(),
    nDsqCount(0)
{}

bool CMasternodeMan::IsValidForMixingTxes(const COutPoint& outpoint)
{
    LOCK(cs);
    CMasternode* pmn = Find(outpoint);
    if (!pmn) {
        return false;
    }
    return pmn->IsValidForMixingTxes();
}

bool CMasternodeMan::AllowMixing(const COutPoint &outpoint)
{
    LOCK(cs);
    CMasternode* pmn = Find(outpoint);
    if (!pmn) {
        return false;
    }
    nDsqCount++;
    pmn->nLastDsq = nDsqCount;
    pmn->nMixingTxCount = 0;

    return true;
}

bool CMasternodeMan::DisallowMixing(const COutPoint &outpoint)
{
    LOCK(cs);
    CMasternode* pmn = Find(outpoint);
    if (!pmn) {
        return false;
    }
    pmn->nMixingTxCount++;

    return true;
}

int64_t CMasternodeMan::GetLastDsq(const COutPoint& outpoint)
{
    LOCK(cs);
    CMasternode* pmn = Find(outpoint);
    if (!pmn) {
        return 0;
    }
    return pmn->nLastDsq;
}

void CMasternodeMan::Clear()
{
    LOCK(cs);
    mapMasternodes.clear();
    nDsqCount = 0;
}

CMasternode* CMasternodeMan::Find(const COutPoint &outpoint)
{
    LOCK(cs);

    // This code keeps compatibility to old code depending on the non-deterministic MN lists
    // When deterministic MN lists get activated, we stop relying on the MNs we encountered due to MNBs and start
    // using the MNs found in the deterministic MN manager. To keep compatibility, we create CMasternode entries
    // for these and return them here. This is needed because we also need to track some data per MN that is not
    // on-chain, like vote counts

    auto mnList = deterministicMNManager->GetListAtChainTip();
    auto dmn = mnList.GetMNByCollateral(outpoint);
    if (!dmn || !mnList.IsMNValid(dmn)) {
        return nullptr;
    }

    auto it = mapMasternodes.find(outpoint);
    if (it != mapMasternodes.end()) {
        return &(it->second);
    } else {
        // MN is not in mapMasternodes but in the deterministic list. Create an entry in mapMasternodes for compatibility with legacy code
        CMasternode mn(outpoint.hash, dmn);
        it = mapMasternodes.emplace(outpoint, mn).first;
        return &(it->second);
    }
}

bool CMasternodeMan::GetMasternodeScores(const uint256& nBlockHash, CMasternodeMan::score_pair_vec_t& vecMasternodeScoresRet)
{
    AssertLockHeld(cs);

    vecMasternodeScoresRet.clear();

    auto mnList = deterministicMNManager->GetListAtChainTip();
    auto scores = mnList.CalculateScores(nBlockHash);
    for (const auto& p : scores) {
        vecMasternodeScoresRet.emplace_back(p.first, p.second);
    }

    sort(vecMasternodeScoresRet.rbegin(), vecMasternodeScoresRet.rend(), CompareScoreMN());
    return !vecMasternodeScoresRet.empty();
}

bool CMasternodeMan::GetMasternodeRank(const COutPoint& outpoint, int& nRankRet, int nBlockHeight, int nMinProtocol)
{
    uint256 tmp;
    return GetMasternodeRank(outpoint, nRankRet, tmp, nBlockHeight, nMinProtocol);
}

bool CMasternodeMan::GetMasternodeRank(const COutPoint& outpoint, int& nRankRet, uint256& blockHashRet, int nBlockHeight, int nMinProtocol)
{
    nRankRet = -1;

    if (!masternodeSync.IsBlockchainSynced())
        return false;

    // make sure we know about this block
    blockHashRet = uint256();
    if (!GetBlockHash(blockHashRet, nBlockHeight)) {
        LogPrintf("CMasternodeMan::%s -- ERROR: GetBlockHash() failed at nBlockHeight %d\n", __func__, nBlockHeight);
        return false;
    }

    LOCK(cs);

    score_pair_vec_t vecMasternodeScores;
    if (!GetMasternodeScores(blockHashRet, vecMasternodeScores))
        return false;

    int nRank = 0;
    for (const auto& scorePair : vecMasternodeScores) {
        nRank++;
        if(scorePair.second->collateralOutpoint == outpoint) {
            nRankRet = nRank;
            return true;
        }
    }

    return false;
}

bool CMasternodeMan::GetMasternodeRanks(CMasternodeMan::rank_pair_vec_t& vecMasternodeRanksRet, int nBlockHeight, int nMinProtocol)
{
    vecMasternodeRanksRet.clear();

    if (!masternodeSync.IsBlockchainSynced())
        return false;

    // make sure we know about this block
    uint256 nBlockHash = uint256();
    if (!GetBlockHash(nBlockHash, nBlockHeight)) {
        LogPrintf("CMasternodeMan::%s -- ERROR: GetBlockHash() failed at nBlockHeight %d\n", __func__, nBlockHeight);
        return false;
    }

    LOCK(cs);

    score_pair_vec_t vecMasternodeScores;
    if (!GetMasternodeScores(nBlockHash, vecMasternodeScores))
        return false;

    int nRank = 0;
    for (const auto& scorePair : vecMasternodeScores) {
        nRank++;
        vecMasternodeRanksRet.push_back(std::make_pair(nRank, scorePair.second));
    }

    return true;
}

void CMasternodeMan::ProcessMasternodeConnections(CConnman& connman)
{
    std::vector<CDeterministicMNCPtr> vecDmns; // will be empty when no wallet
#ifdef ENABLE_WALLET
    privateSendClient.GetMixingMasternodesInfo(vecDmns);
#endif // ENABLE_WALLET

    connman.ForEachNode(CConnman::AllNodes, [&](CNode* pnode) {
        if (pnode->fMasternode) {
#ifdef ENABLE_WALLET
            bool fFound = false;
            for (const auto& dmn : vecDmns) {
                if (pnode->addr == dmn->pdmnState->addr) {
                    fFound = true;
                    break;
                }
            }
            if (fFound) return; // do NOT disconnect mixing masternodes
#endif // ENABLE_WALLET
            LogPrintf("Closing Masternode connection: peer=%d, addr=%s\n", pnode->id, pnode->addr.ToString());
            pnode->fDisconnect = true;
        }
    });
}

std::string CMasternodeMan::ToString() const
{
    std::ostringstream info;

    info << "Masternodes: masternode object count: " << (int)mapMasternodes.size() <<
            ", deterministic masternode count: " << deterministicMNManager->GetListAtChainTip().GetAllMNsCount() <<
            ", nDsqCount: " << (int)nDsqCount;
    return info.str();
}

bool CMasternodeMan::AddGovernanceVote(const COutPoint& outpoint, uint256 nGovernanceObjectHash)
{
    LOCK(cs);
    CMasternode* pmn = Find(outpoint);
    if(!pmn) {
        return false;
    }
    pmn->AddGovernanceVote(nGovernanceObjectHash);
    return true;
}

void CMasternodeMan::RemoveGovernanceObject(uint256 nGovernanceObjectHash)
{
    LOCK(cs);
    for(auto& mnpair : mapMasternodes) {
        mnpair.second.RemoveGovernanceObject(nGovernanceObjectHash);
    }
}

void CMasternodeMan::DoMaintenance(CConnman& connman)
{
    if(fLiteMode) return; // disable all Dash specific functionality

    if(!masternodeSync.IsBlockchainSynced() || ShutdownRequested())
        return;

    static unsigned int nTick = 0;

    nTick++;

    if(nTick % 60 == 0) {
        mnodeman.ProcessMasternodeConnections(connman);
    }
}
