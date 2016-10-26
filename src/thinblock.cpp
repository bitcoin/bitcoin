// Copyright (c) 2016 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "thinblock.h"
#include "util.h"
#include "utiltime.h"
#include "net.h"
#include "chainparams.h"
#include "pow.h"
#include "timedata.h"
#include "main.h"
#include "txmempool.h"
#include "unlimited.h"
#include <sstream>
#include <iomanip>
#include <map>
#include <string>
#include <vector>

extern CCriticalSection cs_thinblockstats;
extern CCriticalSection cs_orphancache;
extern std::map<uint256, COrphanTx> mapOrphanTransactions GUARDED_BY(cs_orphancache);

CThinBlock::CThinBlock(const CBlock& block, CBloomFilter& filter)
{
    header = block.GetBlockHeader();

    unsigned int nTx = block.vtx.size();
    vTxHashes.reserve(nTx);
    for (unsigned int i = 0; i < nTx; i++)
    {
        const uint256& hash = block.vtx[i].GetHash();
        vTxHashes.push_back(hash);

        // Find the transactions that do not match the filter.
        // These are the ones we need to relay back to the requesting peer.
        // NOTE: We always add the first tx, the coinbase as it is the one
        //       most often missing.
        if (!filter.contains(hash) || i == 0)
            vMissingTx.push_back(block.vtx[i]);
    }
}

CXThinBlock::CXThinBlock(const CBlock& block, CBloomFilter* filter)
{
    header = block.GetBlockHeader();
    this->collision = false;

    unsigned int nTx = block.vtx.size();
    vTxHashes.reserve(nTx);
    std::set<uint64_t> setPartialTxHash;
    for (unsigned int i = 0; i < nTx; i++)
    {
        const uint256 hash256 = block.vtx[i].GetHash();
        uint64_t cheapHash = hash256.GetCheapHash();
        vTxHashes.push_back(cheapHash);

        if (setPartialTxHash.count(cheapHash))
                this->collision = true;
        setPartialTxHash.insert(cheapHash);

        // Find the transactions that do not match the filter.
        // These are the ones we need to relay back to the requesting peer.
        // NOTE: We always add the first tx, the coinbase as it is the one
        //       most often missing.
        if ((filter && !filter->contains(hash256)) || i == 0)
            vMissingTx.push_back(block.vtx[i]);
    }
}

CXThinBlock::CXThinBlock(const CBlock& block)
{
    header = block.GetBlockHeader();
    this->collision = false;

    unsigned int nTx = block.vtx.size();
    vTxHashes.reserve(nTx);
    std::set<uint64_t> setPartialTxHash;

    for (unsigned int i = 0; i < nTx; i++)
    {
        const uint256 hash256 = block.vtx[i].GetHash();
        uint64_t cheapHash = hash256.GetCheapHash();
        vTxHashes.push_back(cheapHash);

        if (setPartialTxHash.count(cheapHash))
                this->collision = true;
        setPartialTxHash.insert(cheapHash);

        // if it is missing from this node, then add it to the thin block
        if (!((mempool.exists(hash256))||(mapOrphanTransactions.find(hash256) != mapOrphanTransactions.end())))
	  {
          vMissingTx.push_back(block.vtx[i]);
	  }
        // We always add the first tx, the coinbase as it is the one
        // most often missing.
        else if (i == 0) vMissingTx.push_back(block.vtx[i]);
    }
}

CXThinBlockTx::CXThinBlockTx(uint256 blockHash, std::vector<CTransaction>& vTx)
{
    blockhash = blockHash;
    vMissingTx = vTx;
}

CXRequestThinBlockTx::CXRequestThinBlockTx(uint256 blockHash, std::set<uint64_t>& setHashesToRequest)
{
    blockhash = blockHash;
    setCheapHashesToRequest = setHashesToRequest;
}

bool CXThinBlock::CheckBlockHeader(const CBlockHeader& block, CValidationState& state)
{
  // Check proof of work matches claimed amount
  if (!CheckProofOfWork(header.GetHash(), header.nBits, Params().GetConsensus()))
    return state.DoS(50, error("CheckBlockHeader(): proof of work failed"), REJECT_INVALID, "high-hash");

  // Check timestamp
  if (header.GetBlockTime() > GetAdjustedTime() + 2 * 60 * 60)
    return state.Invalid(error("CheckBlockHeader(): block timestamp too far in the future"), REJECT_INVALID, "time-too-new");

  return true;
}
    
bool CXThinBlock::process(CNode* pfrom, int nSizeThinBlock, std::string strCommand)  // TODO: request from the "best" txn source not necessarily from the block source 
{
    // Xpress Validation - only perform xval if the chaintip matches the last blockhash in the thinblock
    bool fXVal;
    {
        LOCK(cs_main);
	fXVal = (header.hashPrevBlock == chainActive.Tip()->GetBlockHash()) ? true : false;
    }

    pfrom->nSizeThinBlock = nSizeThinBlock;
    pfrom->thinBlock.SetNull();
    pfrom->thinBlock.nVersion = header.nVersion;
    pfrom->thinBlock.nBits = header.nBits;
    pfrom->thinBlock.nNonce = header.nNonce;
    pfrom->thinBlock.nTime = header.nTime;
    pfrom->thinBlock.hashMerkleRoot = header.hashMerkleRoot;
    pfrom->thinBlock.hashPrevBlock = header.hashPrevBlock;
    pfrom->xThinBlockHashes = vTxHashes;

    // Create the mapMissingTx from all the supplied tx's in the xthinblock
    std::map<uint256, CTransaction> mapMissingTx;
    BOOST_FOREACH(CTransaction tx, vMissingTx)
      mapMissingTx[tx.GetHash()] = tx;

    // Create a map of all 8 bytes tx hashes pointing to their full tx hash counterpart 
    // We need to check all transaction sources (orphan list, mempool, and new (incoming) transactions in this block) for a collision.
    int missingCount = 0;
    int unnecessaryCount = 0;
    bool collision = false;
    std::map<uint64_t, uint256> mapPartialTxHash;
    std::vector<uint256> memPoolHashes;

    // Do the orphans first before taking the mempool.cs lock, so that we maintain correct locking order.
    {
    LOCK(cs_orphancache);
    for (std::map<uint256, COrphanTx>::iterator mi = mapOrphanTransactions.begin(); mi != mapOrphanTransactions.end(); ++mi) {
        uint64_t cheapHash = (*mi).first.GetCheapHash();
        if (mapPartialTxHash.count(cheapHash)) //Check for collisions
            collision = true;
        mapPartialTxHash[cheapHash] = (*mi).first;
    }
    }
    {
    // We don't have to keep the lock on mempool.cs here to do mempool.queryHashes 
    // but we take the lock anyway so we don't have to re-lock again later.
    LOCK2(mempool.cs, cs_xval);
    mempool.queryHashes(memPoolHashes);

    for (uint64_t i = 0; i < memPoolHashes.size(); i++) {
        uint64_t cheapHash = memPoolHashes[i].GetCheapHash();
        if (mapPartialTxHash.count(cheapHash)) //Check for collisions
            collision = true;
        mapPartialTxHash[cheapHash] = memPoolHashes[i];
    }
    for (std::map<uint256, CTransaction>::iterator mi = mapMissingTx.begin(); mi != mapMissingTx.end(); ++mi) {
	uint64_t cheapHash = (*mi).first.GetCheapHash();
        // Check for cheap hash collision. Only mark as collision if the full hash is not the same,
        // because the same tx could have been received into the mempool during the request of the xthinblock.
        // In that case we would have the same transaction twice, so it is not a real cheap hash collision and we continue normally.
        const uint256 existingHash = mapPartialTxHash[cheapHash];
        if (!existingHash.IsNull()) { // Check if we already have the cheap hash
            if (existingHash != (*mi).first) { // Check if it really is a cheap hash collision and not just the same transaction
                collision = true;
            }
	}
	mapPartialTxHash[cheapHash] = (*mi).first;
    }

    // There is a remote possiblity of a Tx hash collision therefore if it occurs we re-request a normal
    // thinblock which has the full Tx hash data rather than just the truncated hash.
    if (collision) {
        std::vector<CInv> vGetData;
        vGetData.push_back(CInv(MSG_THINBLOCK, header.GetHash())); 
        pfrom->PushMessage("getdata", vGetData);
        LogPrintf("TX HASH COLLISION for xthinblock: re-requesting a thinblock\n");
        return true;
    }

    // Look for each transaction in our various pools and buffers.
    // With xThinBlocks the vTxHashes contains only the first 8 bytes of the tx hash.
    BOOST_FOREACH(uint64_t &cheapHash, vTxHashes) 
    {
	// Replace the truncated hash with the full hash value if it exists
	const uint256 hash = mapPartialTxHash[cheapHash];
	CTransaction tx;
	if (!hash.IsNull())
        {
	    bool inMemPool = mempool.lookup(hash, tx);
	    bool inMissingTx = mapMissingTx.count(hash) > 0;
	    bool inOrphanCache = mapOrphanTransactions.count(hash) > 0;

	    if ((inMemPool && inMissingTx) || (inOrphanCache && inMissingTx))
                unnecessaryCount++;

	    if (inOrphanCache) {
                tx = mapOrphanTransactions[hash].tx;
                setUnVerifiedOrphanTxHash.insert(hash);
	    }
	    else if (inMemPool && fXVal)
                setPreVerifiedTxHash.insert(hash);
	    else if (inMissingTx)
                tx = mapMissingTx[hash];
        }
	if (tx.IsNull())
            missingCount++;
	// This will push an empty/invalid transaction if we don't have it yet
	pfrom->thinBlock.vtx.push_back(tx);
    }
    }

    pfrom->thinBlockWaitingForTxns = missingCount;
    LogPrint("thin", "thinblock waiting for: %d, unnecessary: %d, txs: %d full: %d\n", pfrom->thinBlockWaitingForTxns, unnecessaryCount, pfrom->thinBlock.vtx.size(), mapMissingTx.size());

    if (pfrom->thinBlockWaitingForTxns == 0) {
        // We have all the transactions now that are in this block: try to reassemble and process.
        pfrom->thinBlockWaitingForTxns = -1;
        pfrom->AddInventoryKnown(GetInv());
        int blockSize = pfrom->thinBlock.GetSerializeSize(SER_NETWORK, CBlock::CURRENT_VERSION);
        LogPrint("thin", "Reassembled thin block for %s (%d bytes). Message was %d bytes, compression ratio %3.2f\n",
	       pfrom->thinBlock.GetHash().ToString(),
	       blockSize,
	       pfrom->nSizeThinBlock,
	       ((float) blockSize) / ((float) pfrom->nSizeThinBlock)
	       );

        // Update run-time statistics of thin block bandwidth savings
        CThinBlockStats::UpdateInBound(pfrom->nSizeThinBlock, blockSize);
        std::string ss = CThinBlockStats::ToString();
        LogPrint("thin", "thin block stats: %s\n", ss.c_str());
        requester.Received(GetInv(), pfrom, pfrom->nSizeThinBlock);
        HandleBlockMessage(pfrom, strCommand, pfrom->thinBlock,  GetInv());  // clears the thin block
        LOCK(cs_orphancache);
        BOOST_FOREACH(uint64_t &cheapHash, vTxHashes)
            EraseOrphanTx(mapPartialTxHash[cheapHash]);
    }
    else if (pfrom->thinBlockWaitingForTxns > 0) {
        // This marks the end of the transactions we've received. If we get this and we have NOT been able to
        // finish reassembling the block, we need to re-request the transactions we're missing:
        std::set<uint64_t> setHashesToRequest;
        for (size_t i = 0; i < pfrom->thinBlock.vtx.size(); i++) {
	    if (pfrom->thinBlock.vtx[i].IsNull())
	        setHashesToRequest.insert(pfrom->xThinBlockHashes[i]);
        }

        // Re-request transactions that we are still missing
        CXRequestThinBlockTx thinBlockTx(header.GetHash(), setHashesToRequest);
        pfrom->PushMessage(NetMsgType::GET_XBLOCKTX, thinBlockTx);
        LogPrint("thin", "Missing %d transactions for xthinblock, re-requesting\n", pfrom->thinBlockWaitingForTxns);
        CThinBlockStats::UpdateInBoundReRequestedTx(pfrom->thinBlockWaitingForTxns);
    }

    return true;
}

void CThinBlockStats::UpdateInBound(uint64_t nThinBlockSize, uint64_t nOriginalBlockSize)
{
    LOCK(cs_thinblockstats);

    // Update InBound thinblock tracking information
    CThinBlockStats::nOriginalSize += nOriginalBlockSize;
    CThinBlockStats::nThinSize += nThinBlockSize;
    CThinBlockStats::nBlocks += 1;
    CThinBlockStats::mapThinBlocksInBound[GetTimeMillis()] = std::pair<uint64_t, uint64_t>(nThinBlockSize, nOriginalBlockSize);

    // Delete any entries that are more than 24 hours old
    int64_t nTimeCutoff = GetTimeMillis() - 60*60*24*1000;
    for (std::map<int64_t, std::pair<uint64_t, uint64_t> >::iterator mi = CThinBlockStats::mapThinBlocksInBound.begin(); mi != CThinBlockStats::mapThinBlocksInBound.end(); ++mi) {
        if ((*mi).first < nTimeCutoff)
            CThinBlockStats::mapThinBlocksInBound.erase(mi);
    }
}

void CThinBlockStats::UpdateOutBound(uint64_t nThinBlockSize, uint64_t nOriginalBlockSize)
{
    LOCK(cs_thinblockstats);

    CThinBlockStats::nOriginalSize += nOriginalBlockSize;
    CThinBlockStats::nThinSize += nThinBlockSize;
    CThinBlockStats::nBlocks += 1;
    CThinBlockStats::mapThinBlocksOutBound[GetTimeMillis()] = std::pair<uint64_t, uint64_t>(nThinBlockSize, nOriginalBlockSize);

    // Delete any entries that are more than 24 hours old
    int64_t nTimeCutoff = GetTimeMillis() - 60*60*24*1000;
    for (std::map<int64_t, std::pair<uint64_t, uint64_t> >::iterator mi = CThinBlockStats::mapThinBlocksOutBound.begin(); mi != CThinBlockStats::mapThinBlocksOutBound.end(); ++mi) {
        if ((*mi).first < nTimeCutoff)
            CThinBlockStats::mapThinBlocksOutBound.erase(mi);
    }
}

void CThinBlockStats::UpdateOutBoundBloomFilter(uint64_t nBloomFilterSize)
{
    LOCK(cs_thinblockstats);

    CThinBlockStats::mapBloomFiltersOutBound[GetTimeMillis()] = nBloomFilterSize;
    CThinBlockStats::nTotalBloomFilterBytes += nBloomFilterSize;

    // Delete any entries that are more than 24 hours old
    int64_t nTimeCutoff = GetTimeMillis() - 60*60*24*1000;
    for (std::map<int64_t, uint64_t>::iterator mi = CThinBlockStats::mapBloomFiltersOutBound.begin(); mi != CThinBlockStats::mapBloomFiltersOutBound.end(); ++mi) {
        if ((*mi).first < nTimeCutoff)
            CThinBlockStats::mapBloomFiltersOutBound.erase(mi);
    }
}

void CThinBlockStats::UpdateInBoundBloomFilter(uint64_t nBloomFilterSize)
{
    LOCK(cs_thinblockstats);

    CThinBlockStats::mapBloomFiltersInBound[GetTimeMillis()] = nBloomFilterSize;
    CThinBlockStats::nTotalBloomFilterBytes += nBloomFilterSize;

    // Delete any entries that are more than 24 hours old
    int64_t nTimeCutoff = GetTimeMillis() - 60*60*24*1000;
    for (std::map<int64_t, uint64_t>::iterator mi = CThinBlockStats::mapBloomFiltersInBound.begin(); mi != CThinBlockStats::mapBloomFiltersInBound.end(); ++mi) {
        if ((*mi).first < nTimeCutoff)
            CThinBlockStats::mapBloomFiltersInBound.erase(mi);
    }
}

void CThinBlockStats::UpdateResponseTime(double nResponseTime)
{
    LOCK(cs_thinblockstats);

    // only update stats if IBD is complete
    if (IsChainNearlySyncd() && IsThinBlocksEnabled()) {
        CThinBlockStats::mapThinBlockResponseTime[GetTimeMillis()] = nResponseTime;

        // Delete any entries that are more than 24 hours old
        int64_t nTimeCutoff = GetTimeMillis() - 60*60*24*1000;
        for (std::map<int64_t, double>::iterator mi = CThinBlockStats::mapThinBlockResponseTime.begin(); mi != CThinBlockStats::mapThinBlockResponseTime.end(); ++mi) {
            if ((*mi).first < nTimeCutoff)
                CThinBlockStats::mapThinBlockResponseTime.erase(mi);
        }
    }
}

void CThinBlockStats::UpdateValidationTime(double nValidationTime)
{
    LOCK(cs_thinblockstats);

    // only update stats if IBD is complete
    if (IsChainNearlySyncd() && IsThinBlocksEnabled()) {
        CThinBlockStats::mapThinBlockValidationTime[GetTimeMillis()] = nValidationTime;

        // Delete any entries that are more than 24 hours old
        int64_t nTimeCutoff = GetTimeMillis() - 60*60*24*1000;
        for (std::map<int64_t, double>::iterator mi = CThinBlockStats::mapThinBlockValidationTime.begin(); mi != CThinBlockStats::mapThinBlockValidationTime.end(); ++mi) {
            if ((*mi).first < nTimeCutoff)
                CThinBlockStats::mapThinBlockValidationTime.erase(mi);
        }
    }
}

void CThinBlockStats::UpdateInBoundReRequestedTx(int nReRequestedTx)
{
    LOCK(cs_thinblockstats);

    // Update InBound thinblock tracking information
    CThinBlockStats::mapThinBlocksInBoundReRequestedTx[GetTimeMillis()] = nReRequestedTx;

    // Delete any entries that are more than 24 hours old
    int64_t nTimeCutoff = GetTimeMillis() - 60*60*24*1000;
    for (std::map<int64_t, int>::iterator mi = CThinBlockStats::mapThinBlocksInBoundReRequestedTx.begin(); mi != CThinBlockStats::mapThinBlocksInBoundReRequestedTx.end(); ++mi) {
        if ((*mi).first < nTimeCutoff)
            CThinBlockStats::mapThinBlocksInBoundReRequestedTx.erase(mi);
    }
}

void CThinBlockStats::UpdateMempoolLimiterBytesSaved(unsigned int nBytesSaved)
{
    LOCK(cs_thinblockstats);
    CThinBlockStats::nMempoolLimiterBytesSaved += nBytesSaved;
}

std::string CThinBlockStats::ToString()
{
    LOCK(cs_thinblockstats);

    static const char *units[] = { "B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
    int i = 0;
    double size = double( CThinBlockStats::nOriginalSize() - CThinBlockStats::nThinSize() - CThinBlockStats::nTotalBloomFilterBytes());
    while (size > 1000) {
	size /= 1000;
	i++;
    }

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << CThinBlockStats::nBlocks() << " thin " << ((CThinBlockStats::nBlocks() > 1) ? "blocks have" : "block has") << " saved " << size << units[i] << " of bandwidth";
    return ss.str();
}

// Calculate the xthin percentage compression over the last 24 hours
std::string CThinBlockStats::InBoundPercentToString()
{
    LOCK(cs_thinblockstats);

    // Delete any entries that are more than 24 hours old
    int64_t nTimeCutoff = GetTimeMillis() - 60*60*24*1000;
    for (std::map<int64_t, std::pair<uint64_t, uint64_t> >::iterator mi = CThinBlockStats::mapThinBlocksInBound.begin(); mi != CThinBlockStats::mapThinBlocksInBound.end(); ++mi) {
        if ((*mi).first < nTimeCutoff)
            CThinBlockStats::mapThinBlocksInBound.erase(mi);
    }

    double nCompressionRate = 0;
    uint64_t nThinSizeTotal = 0;
    uint64_t nOriginalSizeTotal = 0;
    for (std::map<int64_t, std::pair<uint64_t, uint64_t> >::iterator mi = CThinBlockStats::mapThinBlocksInBound.begin(); mi != CThinBlockStats::mapThinBlocksInBound.end(); ++mi) {
        nThinSizeTotal += (*mi).second.first;
        nOriginalSizeTotal += (*mi).second.second;
    }
    // We count up the outbound bloom filters. Outbound bloom filters go with Inbound xthins.
    uint64_t nOutBoundBloomFilterSize = 0;
    for (std::map<int64_t, uint64_t>::iterator mi = CThinBlockStats::mapBloomFiltersOutBound.begin(); mi != CThinBlockStats::mapBloomFiltersOutBound.end(); ++mi) {
        nOutBoundBloomFilterSize += (*mi).second;
    }


    if (nOriginalSizeTotal > 0)
        nCompressionRate = 100 - (100 * (double)(nThinSizeTotal + nOutBoundBloomFilterSize) / nOriginalSizeTotal);

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1);
    ss << "Compression for " << CThinBlockStats::mapThinBlocksInBound.size() << " Inbound  thinblocks (last 24hrs): " << nCompressionRate << "%";
    return ss.str();
}

// Calculate the xthin percentage compression over the last 24 hours
std::string CThinBlockStats::OutBoundPercentToString()
{
    LOCK(cs_thinblockstats);

    // Delete any entries that are more than 24 hours old
    int64_t nTimeCutoff = GetTimeMillis() - 60*60*24*1000;
    for (std::map<int64_t, std::pair<uint64_t, uint64_t> >::iterator mi = CThinBlockStats::mapThinBlocksOutBound.begin(); mi != CThinBlockStats::mapThinBlocksOutBound.end(); ++mi) {
        if ((*mi).first < nTimeCutoff)
            CThinBlockStats::mapThinBlocksOutBound.erase(mi);
    }

    double nCompressionRate = 0;
    uint64_t nThinSizeTotal = 0;
    uint64_t nOriginalSizeTotal = 0;
    for (std::map<int64_t, std::pair<uint64_t, uint64_t> >::iterator mi = CThinBlockStats::mapThinBlocksOutBound.begin(); mi != CThinBlockStats::mapThinBlocksOutBound.end(); ++mi) {
        nThinSizeTotal += (*mi).second.first;
        nOriginalSizeTotal += (*mi).second.second;
    }
    // We count up the inbound bloom filters. Inbound bloom filters go with Outbound xthins.
    uint64_t nInBoundBloomFilterSize = 0;
    for (std::map<int64_t, uint64_t>::iterator mi = CThinBlockStats::mapBloomFiltersInBound.begin(); mi != CThinBlockStats::mapBloomFiltersInBound.end(); ++mi) {
        nInBoundBloomFilterSize += (*mi).second;
    }

    if (nOriginalSizeTotal > 0)
        nCompressionRate = 100 - (100 * (double)(nThinSizeTotal + nInBoundBloomFilterSize) / nOriginalSizeTotal);

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1);
    ss << "Compression for " << CThinBlockStats::mapThinBlocksOutBound.size() << " Outbound thinblocks (last 24hrs): " << nCompressionRate << "%";
    return ss.str();
}

// Calculate the average inbound xthin bloom filter size
std::string CThinBlockStats::InBoundBloomFiltersToString()
{
    LOCK(cs_thinblockstats);

    // Delete any entries that are more than 24 hours old
    int64_t nTimeCutoff = GetTimeMillis() - 60*60*24*1000;
    for (std::map<int64_t, uint64_t>::iterator mi = CThinBlockStats::mapBloomFiltersInBound.begin(); mi != CThinBlockStats::mapBloomFiltersInBound.end(); ++mi) {
        if ((*mi).first < nTimeCutoff)
            CThinBlockStats::mapBloomFiltersInBound.erase(mi);
    }

    uint64_t nInBoundBloomFilters = 0;
    uint64_t nInBoundBloomFilterSize = 0;
    double avgBloomSize = 0;
    for (std::map<int64_t, uint64_t>::iterator mi = CThinBlockStats::mapBloomFiltersInBound.begin(); mi != CThinBlockStats::mapBloomFiltersInBound.end(); ++mi) {
        nInBoundBloomFilterSize += (*mi).second;
        nInBoundBloomFilters += 1;
    }
    if (nInBoundBloomFilters > 0)
        avgBloomSize = (double)nInBoundBloomFilterSize / nInBoundBloomFilters;

    static const char *units[] = { "B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
    int i = 0;
    while (avgBloomSize > 1000) {
	avgBloomSize /= 1000;
	i++;
    }

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1);
    ss << "Inbound bloom filter size (last 24hrs) AVG: " << avgBloomSize << units[i];
    return ss.str();
}

// Calculate the average inbound xthin bloom filter size
std::string CThinBlockStats::OutBoundBloomFiltersToString()
{
    LOCK(cs_thinblockstats);

    // Delete any entries that are more than 24 hours old
    int64_t nTimeCutoff = GetTimeMillis() - 60*60*24*1000;
    for (std::map<int64_t, uint64_t>::iterator mi = CThinBlockStats::mapBloomFiltersOutBound.begin(); mi != CThinBlockStats::mapBloomFiltersOutBound.end(); ++mi) {
        if ((*mi).first < nTimeCutoff)
            CThinBlockStats::mapBloomFiltersOutBound.erase(mi);
    }

    uint64_t nOutBoundBloomFilters = 0;
    uint64_t nOutBoundBloomFilterSize = 0;
    double avgBloomSize = 0;
    for (std::map<int64_t, uint64_t>::iterator mi = CThinBlockStats::mapBloomFiltersOutBound.begin(); mi != CThinBlockStats::mapBloomFiltersOutBound.end(); ++mi) {
        nOutBoundBloomFilterSize += (*mi).second;
        nOutBoundBloomFilters += 1;
    }
    if (nOutBoundBloomFilters > 0)
        avgBloomSize = (double)nOutBoundBloomFilterSize / nOutBoundBloomFilters;

    static const char *units[] = { "B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
    int i = 0;
    while (avgBloomSize > 1000) {
	avgBloomSize /= 1000;
	i++;
    }

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1);
    ss << "Outbound bloom filter size (last 24hrs) AVG: " << avgBloomSize << units[i];
    return ss.str();
}
// Calculate the xthin percentage compression over the last 24 hours
std::string CThinBlockStats::ResponseTimeToString()
{
    LOCK(cs_thinblockstats);

    std::vector<double> vResponseTime;

    double nResponseTimeAverage = 0;
    double nPercentile = 0;
    double nTotalResponseTime = 0;
    double nTotalEntries = 0;
    for (std::map<int64_t, double>::iterator mi = CThinBlockStats::mapThinBlockResponseTime.begin(); mi != CThinBlockStats::mapThinBlockResponseTime.end(); ++mi) {
        nTotalEntries += 1;
        nTotalResponseTime += (*mi).second;
        vResponseTime.push_back((*mi).second);
    }

    if (nTotalEntries > 0) {
        nResponseTimeAverage = (double)nTotalResponseTime / nTotalEntries;

        // Calculate the 95th percentile
        uint64_t nPercentileElement = static_cast<int>((nTotalEntries * 0.95) + 0.5) - 1;
        std::sort(vResponseTime.begin(), vResponseTime.end());
        nPercentile = vResponseTime[nPercentileElement];
    }

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "Response time   (last 24hrs) AVG:" << nResponseTimeAverage << ", 95th pcntl:" << nPercentile;
    return ss.str();
}

// Calculate the xthin percentage compression over the last 24 hours
std::string CThinBlockStats::ValidationTimeToString()
{
    LOCK(cs_thinblockstats);

    std::vector<double> vValidationTime;

    double nValidationTimeAverage = 0;
    double nPercentile = 0;
    double nTotalValidationTime = 0;
    double nTotalEntries = 0;
    for (std::map<int64_t, double>::iterator mi = CThinBlockStats::mapThinBlockValidationTime.begin(); mi != CThinBlockStats::mapThinBlockValidationTime.end(); ++mi) {
        nTotalEntries += 1;
        nTotalValidationTime += (*mi).second;
        vValidationTime.push_back((*mi).second);
    }

    if (nTotalEntries > 0) {
        nValidationTimeAverage = (double)nTotalValidationTime / nTotalEntries;

        // Calculate the 95th percentile
        uint64_t nPercentileElement = static_cast<int>((nTotalEntries * 0.95) + 0.5) - 1;
        std::sort(vValidationTime.begin(), vValidationTime.end());
        nPercentile = vValidationTime[nPercentileElement];
    }

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "Validation time (last 24hrs) AVG:" << nValidationTimeAverage << ", 95th pcntl:" << nPercentile;
    return ss.str();
}

// Calculate the xthin percentage compression over the last 24 hours
std::string CThinBlockStats::ReRequestedTxToString()
{
    LOCK(cs_thinblockstats);

    // Delete any entries that are more than 24 hours old
    int64_t nTimeCutoff = GetTimeMillis() - 60*60*24*1000;
    for (std::map<int64_t, int>::iterator mi = CThinBlockStats::mapThinBlocksInBoundReRequestedTx.begin(); mi != CThinBlockStats::mapThinBlocksInBoundReRequestedTx.end(); ++mi) {
        if ((*mi).first < nTimeCutoff)
            CThinBlockStats::mapThinBlocksInBoundReRequestedTx.erase(mi);
    }

    double nReRequestRate = 0;
    uint64_t nTotalReRequests = 0;
    uint64_t nTotalReRequestedTxs = 0;
    for (std::map<int64_t, int>::iterator mi = CThinBlockStats::mapThinBlocksInBoundReRequestedTx.begin(); mi != CThinBlockStats::mapThinBlocksInBoundReRequestedTx.end(); ++mi) {
        nTotalReRequests += 1;
        nTotalReRequestedTxs += (*mi).second;
    }

    if ( CThinBlockStats::mapThinBlocksInBound.size() > 0)
        nReRequestRate = 100 * (double)nTotalReRequests / CThinBlockStats::mapThinBlocksInBound.size();

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1);
    ss << "Tx re-request rate (last 24hrs): " << nReRequestRate << "% Total re-requests:" << nTotalReRequests;
    return ss.str();
}

std::string CThinBlockStats::MempoolLimiterBytesSavedToString()
{
    LOCK(cs_thinblockstats);

    static const char *units[] = { "B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
    int i = 0;
    double size = (double)CThinBlockStats::nMempoolLimiterBytesSaved();
    while (size > 1000) {
	size /= 1000;
	i++;
    }

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "Thinblock mempool limiting has saved " << size << units[i] << " of bandwidth";
    return ss.str();
}
