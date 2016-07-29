// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef ACTIVEMASTERNODE_H
#define ACTIVEMASTERNODE_H

#include "sync.h"
#include "net.h"
#include "key.h"
#include "init.h"
#include "wallet.h"
#include "darksend.h"
#include "masternode.h"

#define ACTIVE_MASTERNODE_INITIAL                     0 // initial state
#define ACTIVE_MASTERNODE_SYNC_IN_PROCESS             1
#define ACTIVE_MASTERNODE_INPUT_TOO_NEW               2
#define ACTIVE_MASTERNODE_NOT_CAPABLE                 3
#define ACTIVE_MASTERNODE_STARTED                     4

// Responsible for activating the Masternode and pinging the network
class CActiveMasternode
{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;

    /// Ping Masternode
    bool SendMasternodePing(std::string& errorMessage);

    /// Create Masternode broadcast, needs to be relayed manually after that
    bool CreateBroadcast(CTxIn vin, CService service, CKey key, CPubKey pubKey, CKey keyMasternode, CPubKey pubKeyMasternode, std::string &errorMessage, CMasternodeBroadcast &mnb);

    /// Get 1000DRK input that can be used for the Masternode
    bool GetMasterNodeVin(CTxIn& vin, CPubKey& pubkey, CKey& secretKey, std::string strTxHash, std::string strOutputIndex);
    bool GetVinFromOutput(COutput out, CTxIn& vin, CPubKey& pubkey, CKey& secretKey);

public:
	// Initialized by init.cpp
	// Keys for the main Masternode
	CPubKey pubKeyMasternode;

	// Initialized while registering Masternode
	CTxIn vin;
    CService service;

    int status;
    std::string notCapableReason;

    CActiveMasternode()
    {        
        status = ACTIVE_MASTERNODE_INITIAL;
    }

    /// Manage status of main Masternode
    void ManageStatus(); 
    std::string GetStatus();

    /// Create Masternode broadcast, needs to be relayed manually after that
    bool CreateBroadcast(std::string strService, std::string strKey, std::string strTxHash, std::string strOutputIndex, std::string& errorMessage, CMasternodeBroadcast &mnb, bool fOffline = false);

    /// Get 1000DRK input that can be used for the Masternode
    bool GetMasterNodeVin(CTxIn& vin, CPubKey& pubkey, CKey& secretKey);
    vector<COutput> SelectCoinsMasternode();

    /// Enable cold wallet mode (run a Masternode with no funds)
    bool EnableHotColdMasterNode(CTxIn& vin, CService& addr);
};

#endif
