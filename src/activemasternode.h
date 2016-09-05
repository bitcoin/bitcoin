// Copyright (c) 2014-2016 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ACTIVEMASTERNODE_H
#define ACTIVEMASTERNODE_H

#include "net.h"
#include "key.h"
#include "wallet/wallet.h"

class CActiveMasternode;

static const int ACTIVE_MASTERNODE_INITIAL          = 0; // initial state
static const int ACTIVE_MASTERNODE_SYNC_IN_PROCESS  = 1;
static const int ACTIVE_MASTERNODE_INPUT_TOO_NEW    = 2;
static const int ACTIVE_MASTERNODE_NOT_CAPABLE      = 3;
static const int ACTIVE_MASTERNODE_STARTED          = 4;

extern CActiveMasternode activeMasternode;

// Responsible for activating the Masternode and pinging the network
class CActiveMasternode
{
private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;

    /// Ping Masternode
    bool SendMasternodePing(std::string& strErrorRet);

public:
    // Initialized by init.cpp
    std::string strMasterNodeAddr;
    // Keys for the active Masternode
    CPubKey pubKeyMasternode;
    CKey keyMasternode;

    // Initialized while registering Masternode
    CTxIn vin;
    CService service;

    int nState; // should be one of ACTIVE_MASTERNODE_XXXX
    std::string strNotCapableReason;

    CActiveMasternode() : nState(ACTIVE_MASTERNODE_INITIAL) {}

    /// Manage state of active Masternode
    void ManageState();

    std::string GetStatus();

    /// Enable cold wallet mode (run a Masternode with no funds)
    bool EnableRemoteMasterNode(CTxIn& vinNew, CService& serviceNew);
};

#endif
