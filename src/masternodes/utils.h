// Copyright (c) 2014-2019 The Dash Core developers
// Copyright (c) 2019 The BitGreen Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef  BITGREEN_MASTERNODES_MASTERNODE_UTILS_H
#define  BITGREEN_MASTERNODES_MASTERNODE_UTILS_H

#include <special/deterministicmns.h>

class CConnman;

class CMasternodeUtils
{
public:
    typedef std::pair<arith_uint256, CDeterministicMNCPtr> score_pair_t;
    typedef std::vector<score_pair_t> score_pair_vec_t;
    typedef std::pair<int, const CDeterministicMNCPtr> rank_pair_t;
    typedef std::vector<rank_pair_t> rank_pair_vec_t;

public:
    static bool GetMasternodeScores(const uint256& nBlockHash, score_pair_vec_t& vecMasternodeScoresRet);
    static bool GetMasternodeRank(const COutPoint &outpoint, int& nRankRet, uint256& blockHashRet, int nBlockHeight = -1);

    static void ProcessMasternodeConnections(CConnman& connman);
    static void DoMaintenance(CConnman &connman);
};

#endif // BITGREEN_MASTERNODES_MASTERNODE_UTILS_H
