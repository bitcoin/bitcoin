// Copyright (c) 2011-2013 PPCoin developers
// Copyright (c) 2013 Primecoin developers
// Distributed under conditional MIT/X11 open source software license
// see the accompanying file COPYING
#ifndef PRIMECOIN_CHECKPOINTSYNC_H
#define  PRIMECOIN_CHECKPOINTSYNC_H

#include "net.h"
#include "util.h"

#define CHECKPOINT_MAX_SPAN (60 * 60 * 4) // max 4 hours before latest block

class uint256;
class CBlock;
class CBlockIndex;
class CSyncCheckpoint;

extern uint256 hashSyncCheckpoint;
extern CSyncCheckpoint checkpointMessage;
extern uint256 hashInvalidCheckpoint;
extern CCriticalSection cs_hashSyncCheckpoint;
extern std::string strCheckpointWarning;

CBlockIndex* GetLastSyncCheckpoint();
bool WriteSyncCheckpoint(const uint256& hashCheckpoint);
bool IsSyncCheckpointEnforced();
bool AcceptPendingSyncCheckpoint();
uint256 AutoSelectSyncCheckpoint();
bool CheckSyncCheckpoint(const uint256& hashBlock, const CBlockIndex* pindexPrev);
bool WantedByPendingSyncCheckpoint(uint256 hashBlock);
bool ResetSyncCheckpoint();
void AskForPendingSyncCheckpoint(CNode* pfrom);
bool CheckCheckpointPubKey();
bool SetCheckpointPrivKey(std::string strPrivKey);
bool SendSyncCheckpoint(uint256 hashCheckpoint);
bool IsMatureSyncCheckpoint();
bool IsSyncCheckpointTooOld(unsigned int nSeconds);
uint256 WantedByOrphan(const CBlock* pblockOrphan);

// Synchronized checkpoint (introduced first in ppcoin)
class CUnsignedSyncCheckpoint
{
public:
    int nVersion;
    uint256 hashCheckpoint;      // checkpoint block

    IMPLEMENT_SERIALIZE
    (
        READWRITE(this->nVersion);
        nVersion = this->nVersion;
        READWRITE(hashCheckpoint);
    )

    void SetNull()
    {
        nVersion = 1;
        hashCheckpoint = 0;
    }

    std::string ToString() const
    {
        return strprintf(
                "CSyncCheckpoint(\n"
                "    nVersion       = %d\n"
                "    hashCheckpoint = %s\n"
                ")\n",
            nVersion,
            hashCheckpoint.ToString().c_str());
    }

    void print() const
    {
        printf("%s", ToString().c_str());
    }
};

class CSyncCheckpoint : public CUnsignedSyncCheckpoint
{
public:
    static const std::string strMainPubKey;
    static const std::string strTestPubKey;
    static std::string strMasterPrivKey;

    std::vector<unsigned char> vchMsg;
    std::vector<unsigned char> vchSig;

    CSyncCheckpoint()
    {
        SetNull();
    }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(vchMsg);
        READWRITE(vchSig);
    )

    void SetNull()
    {
        CUnsignedSyncCheckpoint::SetNull();
        vchMsg.clear();
        vchSig.clear();
    }

    bool IsNull() const
    {
        return (hashCheckpoint == 0);
    }

    uint256 GetHash() const
    {
        return Hash(this->vchMsg.begin(), this->vchMsg.end());
    }

    bool RelayTo(CNode* pnode) const
    {
        // returns true if wasn't already sent
        if (pnode->hashCheckpointKnown != hashCheckpoint)
        {
            pnode->hashCheckpointKnown = hashCheckpoint;
            pnode->PushMessage("checkpoint", *this);
            return true;
        }
        return false;
    }

    bool CheckSignature();
    bool ProcessSyncCheckpoint(CNode* pfrom);
};

#endif
