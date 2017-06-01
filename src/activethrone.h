// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef ACTIVETHRONE_H
#define ACTIVETHRONE_H

#include "sync.h"
#include "net.h"
#include "key.h"
#include "init.h"
#include "wallet.h"
#include "darksend.h"
#include "throne.h"

#define ACTIVE_THRONE_INITIAL                     0 // initial state
#define ACTIVE_THRONE_SYNC_IN_PROCESS             1
#define ACTIVE_THRONE_INPUT_TOO_NEW               2
#define ACTIVE_THRONE_NOT_CAPABLE                 3
#define ACTIVE_THRONE_STARTED                     4

// Responsible for activating the Throne and pinging the network
class CActiveThrone
{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;

    /// Ping Throne
    bool SendThronePing(std::string& errorMessage);

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
        status = ACTIVE_THRONE_INITIAL;
    }

    /// Manage status of main Throne
    void ManageStatus(); 
    std::string GetStatus();

    /// Enable cold wallet mode (run a Throne with no funds)
    bool EnableHotColdThroNe(CTxIn& vin, CService& addr);
};

#endif
