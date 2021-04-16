// Copyright (c) 2014-2019 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_MASTERNODE_MASTERNODEUTILS_H
#define SYSCOIN_MASTERNODE_MASTERNODEUTILS_H


class CConnman;

class CMasternodeUtils
{
public:
    static void ProcessMasternodeConnections(CConnman& connman);
    static void DoMaintenance(CConnman &connman);
};

#endif//SYSCOIN_MASTERNODE_MASTERNODEUTILS_H
