
// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef ACTIVEMASTERNODE_H
#define ACTIVEMASTERNODE_H

#include "bignum.h"
#include "sync.h"
#include "net.h"
#include "key.h"
#include "core.h"
#include "init.h"
#include "wallet.h"
#include "darksend.h"

// Responsible for activating the masternode and pinging the network
class CActiveMasternode
{
public:
    CTxIn vinMasternode;
    CPubKey pubkeyMasterNode;
    CPubKey pubkeyMasterNode2;

    std::string strMasterNodeSignMessage;
    std::vector<unsigned char> vchMasterNodeSignature;

    std::string masterNodeAddr;
    CService masterNodeSignAddr;

    int isCapableMasterNode;
    int64_t masterNodeSignatureTime;
    int masternodePortOpen;

    CActiveMasternode()
    {
        isCapableMasterNode = MASTERNODE_NOT_PROCESSED;
        masternodePortOpen = 0;
    }

    // get 1000DRK input that can be used for the masternode
    bool GetMasterNodeVin(CTxIn& vin, CPubKey& pubkey, CKey& secretKey);

    // start the masternode and register with the network
    void RegisterAsMasterNode(bool stop);
    // start a remote masternode
    bool RegisterAsMasterNodeRemoteOnly(std::string strMasterNodeAddr, std::string strMasterNodePrivKey);

    // enable hot wallet mode (run a masternode with no funds)
    bool EnableHotColdMasterNode(CTxIn& vin, int64_t sigTime, CService& addr);
};

#endif