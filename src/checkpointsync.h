// Copyright (c) 2012-2020 The Peercoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef PEERCOIN_CHECKPOINTSYNC_H
#define PEERCOIN_CHECKPOINTSYNC_H

#define CHECKPOINT_MAX_SPAN (60 * 60 * 4) // max 4 hours before latest block

#include <netmessagemaker.h>

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
void SetCheckpointEnforce(bool fEnforce);
bool AcceptPendingSyncCheckpoint();
uint256 AutoSelectSyncCheckpoint();
bool CheckSyncCheckpoint(const uint256& hashBlock, const CBlockIndex* pindexPrev);
bool ResetSyncCheckpoint();
void AskForPendingSyncCheckpoint(CNode* pfrom);
bool CheckCheckpointPubKey();
bool SetCheckpointPrivKey(std::string strPrivKey);
bool SendSyncCheckpoint(uint256 hashCheckpoint);
bool IsSyncCheckpointTooOld(unsigned int nSeconds);

// Synchronized checkpoint (introduced first in ppcoin)
class CUnsignedSyncCheckpoint
{
public:
    int nVersion;
    uint256 hashCheckpoint;      // checkpoint block

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(this->nVersion);
        READWRITE(hashCheckpoint);
    }

    void SetNull()
    {
        nVersion = 1;
        hashCheckpoint = uint256();
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

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(vchMsg);
        READWRITE(vchSig);
    }

    void SetNull()
    {
        CUnsignedSyncCheckpoint::SetNull();
        vchMsg.clear();
        vchSig.clear();
    }

    bool IsNull() const
    {
        return (hashCheckpoint == uint256());
    }

    uint256 GetHash() const
    {
        return Hash(this->vchMsg.begin(), this->vchMsg.end());
    }

    bool RelayTo(CNode* pnode) const
    {
        // returns true if wasn't already sent
        if (g_connman && pnode->hashCheckpointKnown != hashCheckpoint)
        {
            pnode->hashCheckpointKnown = hashCheckpoint;
            g_connman->PushMessage(pnode, CNetMsgMaker(pnode->GetSendVersion()).Make("checkpoint", *this));
            return true;
        }
        return false;
    }

    bool CheckSignature();
    bool ProcessSyncCheckpoint(CNode* pfrom);
};

#endif // PEERCOIN_CHECKPOINTSYNC_H
