// Copyright (c) 2016-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_THINBLOCK_H
#define BITCOIN_THINBLOCK_H

#include "serialize.h"
#include "uint256.h"
#include "primitives/block.h"
#include "bloom.h"
#include "stat.h"
#include "sync.h"
#include "consensus/validation.h"
#include "protocol.h"
#include <vector>

class CNode;

class CThinBlock
{
public:
    CBlockHeader header;
    std::vector<uint256> vTxHashes; // List of all transactions id's in the block
    std::vector<CTransaction> vMissingTx; // vector of transactions that did not match the bloom filter

public:
    CThinBlock(const CBlock& block, CBloomFilter& filter);
    CThinBlock() {}

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(header);
        READWRITE(vTxHashes);
        READWRITE(vMissingTx);
    }

    CInv GetInv() { return CInv(MSG_BLOCK, header.GetHash()); }
    bool process(CNode* pfrom, int nSizeThinbBlock, std::string strCommand);
    bool CheckBlockHeader(const CBlockHeader& block, CValidationState& state);
};

class CXThinBlock
{
public:
    CBlockHeader header;
    std::vector<uint64_t> vTxHashes; // List of all transactions id's in the block
    std::vector<CTransaction> vMissingTx; // vector of transactions that did not match the bloom filter
    bool collision;

public:
    CXThinBlock(const CBlock& block, CBloomFilter* filter); // Use the filter to determine which txns the client has
    CXThinBlock(const CBlock& block);  // Assume client has all of the transactions (except coinbase)
    CXThinBlock() {}

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(header);
        READWRITE(vTxHashes);
        READWRITE(vMissingTx);
    }
    CInv GetInv() { return CInv(MSG_BLOCK, header.GetHash()); }
    bool process(CNode* pfrom, int nSizeThinbBlock, std::string strCommand);
    bool CheckBlockHeader(const CBlockHeader& block, CValidationState& state);
};

// This class is used for retrieving a list of still missing transactions after receiving a "thinblock" message.
// The CXThinBlockTx when recieved can be used to fill in the missing transactions after which it is sent
// back to the requestor.  This class uses a 64bit hash as opposed to the normal 256bit hash.
class CXThinBlockTx
{
public:
    /** Public only for unit testing */
    uint256 blockhash;
    std::vector<CTransaction> vMissingTx; // map of missing transactions

public:
    CXThinBlockTx(uint256 blockHash, std::vector<CTransaction>& vTx);
    CXThinBlockTx() {}

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(blockhash);
        READWRITE(vMissingTx);
    }
};
// This class is used for retrieving a list of still missing transactions after receiving a "thinblock" message.
// The CXThinBlockTx when recieved can be used to fill in the missing transactions after which it is sent
// back to the requestor.  This class uses a 64bit hash as opposed to the normal 256bit hash.
class CXRequestThinBlockTx
{
public:
    /** Public only for unit testing */
    uint256 blockhash;
    std::set<uint64_t> setCheapHashesToRequest; // map of missing transactions

public:
    CXRequestThinBlockTx(uint256 blockHash, std::set<uint64_t>& setHashesToRequest);
    CXRequestThinBlockTx() {}

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(blockhash);
        READWRITE(setCheapHashesToRequest);
    }
};

// This class stores statistics for thin block derived protocols.
class CThinBlockData
{
private:
    CCriticalSection cs_mapThinBlockTimer;
    std::map<uint256, uint64_t> mapThinBlockTimer;

    CCriticalSection cs_thinblockstats;
    CStatHistory<uint64_t> nOriginalSize;
    CStatHistory<uint64_t> nThinSize;
    CStatHistory<uint64_t> nBlocks;
    CStatHistory<uint64_t> nMempoolLimiterBytesSaved;
    CStatHistory<uint64_t> nTotalBloomFilterBytes;
    std::map<int64_t, std::pair<uint64_t, uint64_t> > mapThinBlocksInBound;
    std::map<int64_t, std::pair<uint64_t, uint64_t> > mapThinBlocksOutBound;
    std::map<int64_t, uint64_t> mapBloomFiltersOutBound;
    std::map<int64_t, uint64_t> mapBloomFiltersInBound;
    std::map<int64_t, double> mapThinBlockResponseTime;
    std::map<int64_t, double> mapThinBlockValidationTime;
    std::map<int64_t, int> mapThinBlocksInBoundReRequestedTx;
 
public:
    void UpdateInBound(uint64_t nThinBlockSize, uint64_t nOriginalBlockSize);
    void UpdateOutBound(uint64_t nThinBlockSize, uint64_t nOriginalBlockSize);
    void UpdateOutBoundBloomFilter(uint64_t nBloomFilterSize);
    void UpdateInBoundBloomFilter(uint64_t nBloomFilterSize);
    void UpdateResponseTime(double nResponseTime);
    void UpdateValidationTime(double nValidationTime);
    void UpdateInBoundReRequestedTx(int nReRequestedTx);
    void UpdateMempoolLimiterBytesSaved(unsigned int nBytesSaved);
    std::string ToString();
    std::string InBoundPercentToString();
    std::string OutBoundPercentToString();
    std::string InBoundBloomFiltersToString();
    std::string OutBoundBloomFiltersToString();
    std::string ResponseTimeToString();
    std::string ValidationTimeToString();
    std::string ReRequestedTxToString();
    std::string MempoolLimiterBytesSavedToString();

    bool CheckThinblockTimer(uint256 hash);
    void ClearThinBlockTimer(uint256 hash);
};
extern CThinBlockData thindata; // Singleton class


bool HaveConnectThinblockNodes();
bool HaveThinblockNodes();
bool IsThinBlocksEnabled();
bool CanThinBlockBeDownloaded(CNode* pto);
void ConnectToThinBlockNodes();
void CheckNodeSupportForThinBlocks();
void SendXThinBlock(CBlock &block, CNode* pfrom, const CInv &inv);
bool IsThinBlockValid(const CNode *pfrom, const std::vector<CTransaction> &vMissingTx, const CBlockHeader &header);
void BuildSeededBloomFilter(CBloomFilter& memPoolFilter, std::vector<uint256>& vOrphanHashes, uint256 hash, bool fDeterministic = false);

// Xpress Validation: begin
// Transactions that have already been accepted into the memory pool do not need to be
// re-verified and can avoid having to do a second and expensive CheckInputs() when 
// processing a new block.  (Protected by cs_xval)
extern std::set<uint256> setPreVerifiedTxHash;

// Orphans that are added to the thinblock must be verifed since they have never been
// accepted into the memory pool.  (Protected by cs_xval)
extern std::set<uint256> setUnVerifiedOrphanTxHash;

extern CCriticalSection cs_xval;
// Xpress Validation: end

#endif // BITCOIN_THINBLOCK_H
