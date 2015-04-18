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

// Responsible for activating the Masternode and pinging the network
class CActiveMasternode
{
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
        status = MASTERNODE_NOT_PROCESSED;
    }

    /// Manage status of main Masternode
    void ManageStatus(); 

    /// Ping for main Masternode
    bool Mnping(std::string& errorMessage); 
    /// Ping for any Masternode
    bool Mnping(CTxIn vin, CService service, CKey key, CPubKey pubKey, std::string &retErrorMessage); 

    /// Register remote Masternode
    bool Register(std::string strService, std::string strKey, std::string txHash, std::string strOutputIndex, std::string strDonationAddress, std::string strDonationPercentage, std::string& errorMessage); 
    /// Register any Masternode
    bool Register(CTxIn vin, CService service, CKey key, CPubKey pubKey, CKey keyMasternode, CPubKey pubKeyMasternode, CScript donationAddress, int donationPercentage, std::string &retErrorMessage); 

    /// Get 1000DRK input that can be used for the Masternode
    bool GetMasterNodeVin(CTxIn& vin, CPubKey& pubkey, CKey& secretKey);
    bool GetMasterNodeVin(CTxIn& vin, CPubKey& pubkey, CKey& secretKey, std::string strTxHash, std::string strOutputIndex);
    vector<COutput> SelectCoinsMasternode();
    bool GetVinFromOutput(COutput out, CTxIn& vin, CPubKey& pubkey, CKey& secretKey);

    /// Enable hot wallet mode (run a Masternode with no funds)
    bool EnableHotColdMasterNode(CTxIn& vin, CService& addr);
};

#endif
