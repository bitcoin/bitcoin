// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Crowncoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef ACTIVETHRONE_H
#define ACTIVETHRONE_H

#include "bignum.h"
#include "sync.h"
#include "net.h"
#include "key.h"
#include "core.h"
#include "init.h"
#include "wallet.h"
#include "darksend.h"

// Responsible for activating the Throne and pinging the network
class CActiveThrone
{
public:
	// Initialized by init.cpp
	// Keys for the main Throne
	CPubKey pubKeyThrone;

	// Initialized while registering Throne
	CTxIn vin;
    CService service;

    int status;
    std::string notCapableReason;

    CActiveThrone()
    {        
        status = THRONE_NOT_PROCESSED;
    }

    /// Manage status of main Throne
    void ManageStatus(); 

    /// Ping for main Throne
    bool Dseep(std::string& errorMessage); 
    /// Ping for any Throne
    bool Dseep(CTxIn vin, CService service, CKey key, CPubKey pubKey, std::string &retErrorMessage, bool stop); 

    /// Stop main Throne
    bool StopThroNe(std::string& errorMessage); 
    /// Stop remote Throne
    bool StopThroNe(std::string strService, std::string strKeyThrone, std::string& errorMessage); 
    /// Stop any Throne
    bool StopThroNe(CTxIn vin, CService service, CKey key, CPubKey pubKey, std::string& errorMessage); 

    /// Register remote Throne
    bool Register(std::string strService, std::string strKey, std::string txHash, std::string strOutputIndex, std::string strDonationAddress, std::string strDonationPercentage, std::string& errorMessage); 
    /// Register any Throne
    bool Register(CTxIn vin, CService service, CKey key, CPubKey pubKey, CKey keyThrone, CPubKey pubKeyThrone, CScript donationAddress, int donationPercentage, std::string &retErrorMessage); 

    /// Get 1000DRK input that can be used for the Throne
    bool GetThroNeVin(CTxIn& vin, CPubKey& pubkey, CKey& secretKey);
    bool GetThroNeVin(CTxIn& vin, CPubKey& pubkey, CKey& secretKey, std::string strTxHash, std::string strOutputIndex);
    vector<COutput> SelectCoinsThrone();
    bool GetVinFromOutput(COutput out, CTxIn& vin, CPubKey& pubkey, CKey& secretKey);

    /// Enable hot wallet mode (run a Throne with no funds)
    bool EnableHotColdThroNe(CTxIn& vin, CService& addr);
};

#endif
