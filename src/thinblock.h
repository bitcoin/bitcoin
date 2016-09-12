// Copyright (c) 2016 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_THINBLOCK_H
#define BITCOIN_THINBLOCK_H

#include "serialize.h"
#include "uint256.h"
#include "primitives/block.h"
#include "bloom.h"
#include "stat.h"
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
class CThinBlockStats
{
private:
	static CStatHistory<uint64_t> nOriginalSize;
	static CStatHistory<uint64_t> nThinSize;
	static CStatHistory<uint64_t> nBlocks;
	static CStatHistory<uint64_t> nMempoolLimiterBytesSaved;
	static CStatHistory<uint64_t> nTotalBloomFilterBytes;
        static std::map<int64_t, std::pair<uint64_t, uint64_t> > mapThinBlocksInBound;
        static std::map<int64_t, std::pair<uint64_t, uint64_t> > mapThinBlocksOutBound;
        static std::map<int64_t, uint64_t> mapBloomFiltersOutBound;
        static std::map<int64_t, uint64_t> mapBloomFiltersInBound;
        static std::map<int64_t, double> mapThinBlockResponseTime;
        static std::map<int64_t, double> mapThinBlockValidationTime;
        static std::map<int64_t, int> mapThinBlocksInBoundReRequestedTx;
 
public:
	static void UpdateInBound(uint64_t nThinBlockSize, uint64_t nOriginalBlockSize);
	static void UpdateOutBound(uint64_t nThinBlockSize, uint64_t nOriginalBlockSize);
        static void UpdateOutBoundBloomFilter(uint64_t nBloomFilterSize);
        static void UpdateInBoundBloomFilter(uint64_t nBloomFilterSize);
	static void UpdateResponseTime(double nResponseTime);
	static void UpdateValidationTime(double nValidationTime);
	static void UpdateInBoundReRequestedTx(int nReRequestedTx);
        static void UpdateMempoolLimiterBytesSaved(unsigned int nBytesSaved);
	static std::string ToString();
        static std::string InBoundPercentToString();
        static std::string OutBoundPercentToString();
        static std::string InBoundBloomFiltersToString();
        static std::string OutBoundBloomFiltersToString();
        static std::string ResponseTimeToString();
        static std::string ValidationTimeToString();
        static std::string ReRequestedTxToString();
        static std::string MempoolLimiterBytesSavedToString();
};





#endif // BITCOIN_THINBLOCK_H
