// Copyright (c) 2014-2018 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ACTIVEMASTERNODE_H
#define ACTIVEMASTERNODE_H

#include "chainparams.h"
#include "key.h"
#include "net.h"
#include "primitives/transaction.h"
#include "validationinterface.h"

#include "evo/deterministicmns.h"
#include "evo/providertx.h"

struct CActiveMasternodeInfo;
class CActiveLegacyMasternodeManager;
class CActiveDeterministicMasternodeManager;

static const int ACTIVE_MASTERNODE_INITIAL          = 0; // initial state
static const int ACTIVE_MASTERNODE_SYNC_IN_PROCESS  = 1;
static const int ACTIVE_MASTERNODE_INPUT_TOO_NEW    = 2;
static const int ACTIVE_MASTERNODE_NOT_CAPABLE      = 3;
static const int ACTIVE_MASTERNODE_STARTED          = 4;

extern CActiveMasternodeInfo activeMasternodeInfo;
extern CActiveLegacyMasternodeManager legacyActiveMasternodeManager;
extern CActiveDeterministicMasternodeManager* activeMasternodeManager;

struct CActiveMasternodeInfo {
    // Keys for the active Masternode
    CKeyID legacyKeyIDOperator;
    CKey legacyKeyOperator;

    std::unique_ptr<CBLSPublicKey> blsPubKeyOperator;
    std::unique_ptr<CBLSSecretKey> blsKeyOperator;

    // Initialized while registering Masternode
    uint256 proTxHash;
    COutPoint outpoint;
    CService service;
};


class CActiveDeterministicMasternodeManager : public CValidationInterface
{
public:
    enum masternode_state_t {
        MASTERNODE_WAITING_FOR_PROTX,
        MASTERNODE_POSE_BANNED,
        MASTERNODE_REMOVED,
        MASTERNODE_OPERATOR_KEY_CHANGED,
        MASTERNODE_READY,
        MASTERNODE_ERROR,
    };

private:
    CDeterministicMNCPtr mnListEntry;
    masternode_state_t state{MASTERNODE_WAITING_FOR_PROTX};
    std::string strError;

public:
    virtual void UpdatedBlockTip(const CBlockIndex* pindexNew, const CBlockIndex* pindexFork, bool fInitialDownload);

    void Init();

    CDeterministicMNCPtr GetDMN() const { return mnListEntry; }

    std::string GetStateString() const;
    std::string GetStatus() const;

private:
    bool GetLocalAddress(CService& addrRet);
};

// Responsible for activating the Masternode and pinging the network (legacy MN list)
class CActiveLegacyMasternodeManager
{
public:
    enum masternode_type_enum_t {
        MASTERNODE_UNKNOWN = 0,
        MASTERNODE_REMOTE  = 1
    };

private:
    // critical section to protect the inner data structures
    mutable CCriticalSection cs;

    masternode_type_enum_t eType;

    bool fPingerEnabled;

    /// Ping Masternode
    bool SendMasternodePing(CConnman& connman);

    //  sentinel ping data
    int64_t nSentinelPingTime;
    uint32_t nSentinelVersion;

public:
    int nState; // should be one of ACTIVE_MASTERNODE_XXXX
    std::string strNotCapableReason;


    CActiveLegacyMasternodeManager() :
        eType(MASTERNODE_UNKNOWN),
        fPingerEnabled(false),
        nState(ACTIVE_MASTERNODE_INITIAL)
    {
    }

    /// Manage state of active Masternode
    void ManageState(CConnman& connman);

    std::string GetStateString() const;
    std::string GetStatus() const;
    std::string GetTypeString() const;

    bool UpdateSentinelPing(int version);

    void DoMaintenance(CConnman& connman);

private:
    void ManageStateInitial(CConnman& connman);
    void ManageStateRemote();
};

#endif
