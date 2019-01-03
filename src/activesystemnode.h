// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef ACTIVESYSTEMNODE_H
#define ACTIVESYSTEMNODE_H

#include "sync.h"
#include "net.h"
#include "key.h"
#include "init.h"
#include "wallet.h"
#include "systemnode-sync.h"
#include "legacysigner.h"
#include "systemnode.h"

#define ACTIVE_SYSTEMNODE_INITIAL                     0 // initial state
#define ACTIVE_SYSTEMNODE_SYNC_IN_PROCESS             1
#define ACTIVE_SYSTEMNODE_INPUT_TOO_NEW               2
#define ACTIVE_SYSTEMNODE_NOT_CAPABLE                 3
#define ACTIVE_SYSTEMNODE_STARTED                     4

class CActiveSystemnode;

extern CActiveSystemnode activeSystemnode;

// Responsible for activating the Systemnode and pinging the network
class CActiveSystemnode
{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;

    /// Ping Systemnode
    bool SendSystemnodePing(std::string& errorMessage);

public:
	// Initialized by init.cpp
	// Keys for the main Systemnode
	CPubKey pubKeySystemnode;

    // Signature signing over staking priviledge
    std::vector<unsigned char> vchSigSignover;

    // Initialized while registering Systemnode
	CTxIn vin;
    CService service;

    int status;
    std::string notCapableReason;

    CActiveSystemnode()
    {        
        status = ACTIVE_SYSTEMNODE_INITIAL;
    }

    /// Manage status of main Systemnode
    void ManageStatus(); 
    std::string GetStatus();

    /// Enable cold wallet mode (run a Systemnode with no funds)
    bool EnableHotColdSystemNode(const CTxIn& vin, const CService& addr);
};

#endif
