// Copyright (c) 2014-2024 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MASTERNODE_UTILS_H
#define BITCOIN_MASTERNODE_UTILS_H

class CConnman;
class CDeterministicMNManager;
class CMasternodeSync;
struct CJContext;

class CMasternodeUtils
{
public:
    static void DoMaintenance(CConnman &connman, CDeterministicMNManager& dmnman,
                              const CMasternodeSync& mn_sync, const CJContext& cj_ctx);
};

#endif // BITCOIN_MASTERNODE_UTILS_H
