// Copyright (c) 2011 The Bitcoin developers
// Copyright (c) 2011-2012 The PPCoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_CHECKPOINT_H
#define  BITCOIN_CHECKPOINT_H

#include <map>
#include "util.h"

// ppcoin: auto checkpoint min at 8 hours; max at 16 hours
#define AUTO_CHECKPOINT_MIN_SPAN   (60 * 60 * 8)
#define AUTO_CHECKPOINT_MAX_SPAN   (60 * 60 * 16)
#define AUTO_CHECKPOINT_TRUST_SPAN (60 * 60 * 24)

class uint256;
class CBlockIndex;

//
// Block-chain checkpoints are compiled-in sanity checks.
// They are updated every release or three.
//
namespace Checkpoints
{
    // Returns true if block passes checkpoint checks
    bool CheckHardened(int nHeight, const uint256& hash);

    // Return conservative estimate of total number of blocks, 0 if unknown
    int GetTotalBlocksEstimate();

    // Returns last CBlockIndex* in mapBlockIndex that is a checkpoint
    CBlockIndex* GetLastCheckpoint(const std::map<uint256, CBlockIndex*>& mapBlockIndex);

    // ppcoin: synchronized checkpoint
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
            return SerializeHash(*this);
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

        bool CheckSignature()
        {
            CKey key;
            if (!key.SetPubKey(ParseHex("0487ca85b6ae9d311f996c7616d20d0c88a5b4f07d25e78f419019f35cce6522acf978b2d99f0e7a58db1f120439e5c1889266927854aa57c93956c2569188a539")))
                return error("CSyncCheckpoint::CheckSignature() : SetPubKey failed");
            if (!key.Verify(Hash(vchMsg.begin(), vchMsg.end()), vchSig))
                return error("CSyncCheckpoint::CheckSignature() : verify signature failed");

            // Now unserialize the data
            CDataStream sMsg(vchMsg);
            sMsg >> *(CUnsignedSyncCheckpoint*)this;
            return true;
        }

        bool ProcessSyncCheckpoint();
    };

    extern uint256 hashSyncCheckpoint;
    extern CSyncCheckpoint checkpointMessage;
    extern CCriticalSection cs_hashSyncCheckpoint;

    // ppcoin: automatic checkpoint
    extern int nAutoCheckpoint;
    extern int nBranchPoint;

    bool CheckAuto(const CBlockIndex *pindex);
    int  GetNextChainCheckpoint(const CBlockIndex *pindex);
    int  GetNextAutoCheckpoint(int nCheckpoint);
    void AdvanceAutoCheckpoint(int nCheckpoint);
    bool ResetAutoCheckpoint(int nCheckpoint);
}

#endif
