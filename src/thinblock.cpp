// Copyright (c) 2016-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "thinblock.h"
#include "util.h"
#include "utiltime.h"
#include "chainparams.h"
#include "consensus/merkle.h"
#include "main.h"
#include "net.h"
#include "chainparams.h"
#include "policy/policy.h"
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

using namespace std;

extern CCriticalSection cs_thinblockstats;
extern CCriticalSection cs_orphancache;
extern map<uint256, COrphanTx> mapOrphanTransactions GUARDED_BY(cs_orphancache);

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

bool CThinBlock::process(CNode *pfrom, int nSizeThinBlock, string strCommand)
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
    pfrom->thinBlockHashes = vTxHashes;

    bool mutated;
    uint256 merkleroot = ComputeMerkleRoot(vTxHashes, &mutated);
    if (header.hashMerkleRoot != merkleroot)
    {
        LOCK(cs_main);
        Misbehaving(pfrom->GetId(), 100);
        return error("Thinblock merkle root does not match computed merkle root, peer=%d", pfrom->GetId());
    }

    // Create the mapMissingTx from all the supplied tx's in the xthinblock
    std::map<uint256, CTransaction> mapMissingTx;
    BOOST_FOREACH(const CTransaction tx, vMissingTx)
        mapMissingTx[tx.GetHash()] = tx;

    {
        LOCK(cs_orphancache);
        // We don't have to keep the lock on mempool.cs here to do mempool.queryHashes
        // but we take the lock anyway so we don't have to re-lock again later.
        LOCK2(mempool.cs, cs_xval);
        int missingCount = 0;
        int unnecessaryCount = 0;

        // Look for each transaction in our various pools and buffers.
        BOOST_FOREACH(const uint256 &hash, vTxHashes)
        {
            CTransaction tx;
            if (!hash.IsNull())
            {
                bool inMemPool = mempool.lookup(hash, tx);
                bool inMissingTx = mapMissingTx.count(hash) > 0;
                bool inOrphanCache = mapOrphanTransactions.count(hash) > 0;

                if ((inMemPool && inMissingTx) || (inOrphanCache && inMissingTx))
                    unnecessaryCount++;

                if (inOrphanCache)
                {
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
        pfrom->thinBlockWaitingForTxns = missingCount;
        LogPrint("thin", "Thinblock %s waiting for: %d, unnecessary: %d, txs: %d full: %d\n",
            pfrom->thinBlock.GetHash().ToString(), pfrom->thinBlockWaitingForTxns, unnecessaryCount,
            pfrom->thinBlock.vtx.size(), mapMissingTx.size());
    } // end lock cs_orphancache, mempool.cs, cs_xval

    if (pfrom->thinBlockWaitingForTxns == 0)
    {
        // We have all the transactions now that are in this block: try to reassemble and process.
        requester.Received(GetInv(), pfrom, nSizeThinBlock);
        pfrom->thinBlockWaitingForTxns = -1;
        int blockSize = pfrom->thinBlock.GetSerializeSize(SER_NETWORK, CBlock::CURRENT_VERSION);
        LogPrint("thin", "Reassembled thin block for %s (%d bytes). Message was %d bytes, compression ratio %3.2f\n",
            pfrom->thinBlock.GetHash().ToString(), blockSize, nSizeThinBlock,
            ((float)blockSize) / ((float)nSizeThinBlock));

        // Update run-time statistics of thin block bandwidth savings
        thindata.UpdateInBound(nSizeThinBlock, blockSize);
        LogPrint("thin", "thin block stats: %s\n", thindata.ToString());

        HandleBlockMessage(pfrom, strCommand, pfrom->thinBlock, GetInv());
        LOCK(cs_orphancache);
        BOOST_FOREACH(const uint256 &hash, vTxHashes)
            EraseOrphanTx(hash);
    }
    else if (pfrom->thinBlockWaitingForTxns > 0)
    {
        // This marks the end of the transactions we've received. If we get this and we have NOT been able to
        // finish reassembling the block, we need to re-request the full regular block:
        vector<CInv> vGetData;
        vGetData.push_back(CInv(MSG_BLOCK, header.GetHash()));
        pfrom->PushMessage("getdata", vGetData);
        setPreVerifiedTxHash.clear(); // Xpress Validation - clear the set since we do not do XVal on regular blocks
        LogPrint("thin", "Missing %d Thinblock transactions, re-requesting a regular block\n",
            pfrom->thinBlockWaitingForTxns);
        thindata.UpdateInBoundReRequestedTx(pfrom->thinBlockWaitingForTxns);
    }

    return true;
}


CXThinBlock::CXThinBlock(const CBlock &block, CBloomFilter *filter)
{
    header = block.GetBlockHeader();
    this->collision = false;

    unsigned int nTx = block.vtx.size();
    vTxHashes.reserve(nTx);
    set<uint64_t> setPartialTxHash;
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
    set<uint64_t> setPartialTxHash;

    LOCK(cs_orphancache);
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

CXThinBlockTx::CXThinBlockTx(uint256 blockHash, vector<CTransaction>& vTx)
{
    blockhash = blockHash;
    vMissingTx = vTx;
}

CXRequestThinBlockTx::CXRequestThinBlockTx(uint256 blockHash, set<uint64_t>& setHashesToRequest)
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
    
bool CXThinBlock::process(CNode* pfrom, int nSizeThinBlock, string strCommand)  // TODO: request from the "best" txn source not necessarily from the block source 
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
    map<uint256, CTransaction> mapMissingTx;
    BOOST_FOREACH(CTransaction tx, vMissingTx)
      mapMissingTx[tx.GetHash()] = tx;

    // Create a map of all 8 bytes tx hashes pointing to their full tx hash counterpart 
    // We need to check all transaction sources (orphan list, mempool, and new (incoming) transactions in this block) for a collision.
    int missingCount = 0;
    int unnecessaryCount = 0;
    bool collision = false;
    map<uint64_t, uint256> mapPartialTxHash;
    vector<uint256> memPoolHashes;

    // Do the orphans first before taking the mempool.cs lock, so that we maintain correct locking order.
    {
    LOCK(cs_orphancache);
    for (map<uint256, COrphanTx>::iterator mi = mapOrphanTransactions.begin(); mi != mapOrphanTransactions.end(); ++mi) {
        uint64_t cheapHash = (*mi).first.GetCheapHash();
        if (mapPartialTxHash.count(cheapHash)) //Check for collisions
            collision = true;
        mapPartialTxHash[cheapHash] = (*mi).first;
    }
    }

    bool fMerkleRootCorrect = true;
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
    for (map<uint256, CTransaction>::iterator mi = mapMissingTx.begin(); mi != mapMissingTx.end(); ++mi) {
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
    if (!collision)
    {
        std::vector<uint256> fullTxHashes;
        BOOST_FOREACH(const uint64_t &cheapHash, vTxHashes)
            fullTxHashes.push_back(mapPartialTxHash[cheapHash]);

        bool mutated;
        uint256 merkleroot = ComputeMerkleRoot(fullTxHashes, &mutated);
        if (header.hashMerkleRoot != merkleroot)
        {
            fMerkleRootCorrect = false;
        }
        else
        {

            // Look for each transaction in our various pools and buffers.
            // With xThinBlocks the vTxHashes contains only the first 8 bytes of the tx hash.
            BOOST_FOREACH (const uint256 hash, fullTxHashes)
            {
                // Replace the truncated hash with the full hash value if it exists
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
    }
    }  // End locking mempool.cs and cs_xval

    // This must be done outside of the above section or a deadlock may occur.
    if (!fMerkleRootCorrect)
    {
        LOCK(cs_main);
        Misbehaving(pfrom->GetId(), 100);
        return error("xthinblock merkelroot does not match computed merkleroot, peer=%d", pfrom->GetId());
    }
 
    // There is a remote possiblity of a Tx hash collision therefore if it occurs we re-request a normal
    // thinblock which has the full Tx hash data rather than just the truncated hash.
    if (collision) {
        vector<CInv> vGetData;
        vGetData.push_back(CInv(MSG_THINBLOCK, header.GetHash())); 
        pfrom->PushMessage("getdata", vGetData);  // This must be done outside of the mempool.cs lock or the deadlock detection with pfrom->cs_vSend will be triggered.
        LogPrintf("TX HASH COLLISION for xthinblock: re-requesting a thinblock\n");
        return true;
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
        thindata.UpdateInBound(pfrom->nSizeThinBlock, blockSize);
        string ss = thindata.ToString();
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
        set<uint64_t> setHashesToRequest;
        for (size_t i = 0; i < pfrom->thinBlock.vtx.size(); i++) {
	    if (pfrom->thinBlock.vtx[i].IsNull())
	        setHashesToRequest.insert(pfrom->xThinBlockHashes[i]);
        }

        // Re-request transactions that we are still missing
        CXRequestThinBlockTx thinBlockTx(header.GetHash(), setHashesToRequest);
        pfrom->PushMessage(NetMsgType::GET_XBLOCKTX, thinBlockTx);
        LogPrint("thin", "Missing %d transactions for xthinblock, re-requesting\n", pfrom->thinBlockWaitingForTxns);
        thindata.UpdateInBoundReRequestedTx(pfrom->thinBlockWaitingForTxns);
    }

    return true;
}

void CThinBlockData::UpdateInBound(uint64_t nThinBlockSize, uint64_t nOriginalBlockSize)
{
    LOCK(cs_thinblockstats);

    // Update InBound thinblock tracking information
    nOriginalSize += nOriginalBlockSize;
    nThinSize += nThinBlockSize;
    nBlocks += 1;
    mapThinBlocksInBound[GetTimeMillis()] = pair<uint64_t, uint64_t>(nThinBlockSize, nOriginalBlockSize);

    // Delete any entries that are more than 24 hours old
    int64_t nTimeCutoff = GetTimeMillis() - 60*60*24*1000;
    map<int64_t, pair<uint64_t, uint64_t> >::iterator iter = mapThinBlocksInBound.begin();
    while (iter != mapThinBlocksInBound.end())
    {
        map<int64_t, pair<uint64_t, uint64_t> >::iterator mi = iter++; // increment to avoid iterator becoming invalid
        if ((*mi).first < nTimeCutoff)
            mapThinBlocksInBound.erase(mi);
    }
}

void CThinBlockData::UpdateOutBound(uint64_t nThinBlockSize, uint64_t nOriginalBlockSize)
{
    LOCK(cs_thinblockstats);

    nOriginalSize += nOriginalBlockSize;
    nThinSize += nThinBlockSize;
    nBlocks += 1;
    mapThinBlocksOutBound[GetTimeMillis()] = pair<uint64_t, uint64_t>(nThinBlockSize, nOriginalBlockSize);

    // Delete any entries that are more than 24 hours old
    int64_t nTimeCutoff = GetTimeMillis() - 60*60*24*1000;
    map<int64_t, pair<uint64_t, uint64_t> >::iterator iter = mapThinBlocksOutBound.begin();
    while (iter != mapThinBlocksOutBound.end())
    {
        map<int64_t, pair<uint64_t, uint64_t> >::iterator mi = iter++; // increment to avoid iterator becoming invalid
        if ((*mi).first < nTimeCutoff)
            mapThinBlocksOutBound.erase(mi);
    }
}

void CThinBlockData::UpdateOutBoundBloomFilter(uint64_t nBloomFilterSize)
{
    LOCK(cs_thinblockstats);

    mapBloomFiltersOutBound[GetTimeMillis()] = nBloomFilterSize;
    nTotalBloomFilterBytes += nBloomFilterSize;

    // Delete any entries that are more than 24 hours old
    int64_t nTimeCutoff = GetTimeMillis() - 60*60*24*1000;
    map<int64_t, uint64_t>::iterator iter = mapBloomFiltersOutBound.begin();
    while (iter != mapBloomFiltersOutBound.end())
    {
        map<int64_t, uint64_t>::iterator mi = iter++; // increment to avoid iterator becoming invalid
        if ((*mi).first < nTimeCutoff)
            mapBloomFiltersOutBound.erase(mi);
    }
}

void CThinBlockData::UpdateInBoundBloomFilter(uint64_t nBloomFilterSize)
{
    LOCK(cs_thinblockstats);

    mapBloomFiltersInBound[GetTimeMillis()] = nBloomFilterSize;
    nTotalBloomFilterBytes += nBloomFilterSize;

    // Delete any entries that are more than 24 hours old
    int64_t nTimeCutoff = GetTimeMillis() - 60*60*24*1000;
    map<int64_t, uint64_t>::iterator iter = mapBloomFiltersInBound.begin();
    while (iter != mapBloomFiltersInBound.end())
    {
        map<int64_t, uint64_t>::iterator mi = iter++; // increment to avoid iterator becoming invalid
        if ((*mi).first < nTimeCutoff)
            mapBloomFiltersInBound.erase(mi);
    }
}

void CThinBlockData::UpdateResponseTime(double nResponseTime)
{
    LOCK(cs_thinblockstats);

    // only update stats if IBD is complete
    if (IsChainNearlySyncd() && IsThinBlocksEnabled()) {
        mapThinBlockResponseTime[GetTimeMillis()] = nResponseTime;

        // Delete any entries that are more than 24 hours old
        int64_t nTimeCutoff = GetTimeMillis() - 60*60*24*1000;
        map<int64_t, double>::iterator iter = mapThinBlockResponseTime.begin();
        while (iter != mapThinBlockResponseTime.end())
        {
            map<int64_t, double>::iterator mi = iter++; // increment to avoid iterator becoming invalid
            if ((*mi).first < nTimeCutoff)
                mapThinBlockResponseTime.erase(mi);
        }
    }
}

void CThinBlockData::UpdateValidationTime(double nValidationTime)
{
    LOCK(cs_thinblockstats);

    // only update stats if IBD is complete
    if (IsChainNearlySyncd() && IsThinBlocksEnabled()) {
        mapThinBlockValidationTime[GetTimeMillis()] = nValidationTime;

        // Delete any entries that are more than 24 hours old
        int64_t nTimeCutoff = GetTimeMillis() - 60*60*24*1000;
        map<int64_t, double>::iterator iter = mapThinBlockValidationTime.begin();
        while (iter != mapThinBlockValidationTime.end())
        {
            map<int64_t, double>::iterator mi = iter++; // increment to avoid iterator becoming invalid
            if ((*mi).first < nTimeCutoff)
                mapThinBlockValidationTime.erase(mi);
        }
    }
}

void CThinBlockData::UpdateInBoundReRequestedTx(int nReRequestedTx)
{
    LOCK(cs_thinblockstats);

    // Update InBound thinblock tracking information
    mapThinBlocksInBoundReRequestedTx[GetTimeMillis()] = nReRequestedTx;

    // Delete any entries that are more than 24 hours old
    int64_t nTimeCutoff = GetTimeMillis() - 60*60*24*1000;
    map<int64_t, int>::iterator iter = mapThinBlocksInBoundReRequestedTx.begin();
    while (iter != mapThinBlocksInBoundReRequestedTx.end())
    {
        map<int64_t, int>::iterator mi = iter++; // increment to avoid iterator becoming invalid
        if ((*mi).first < nTimeCutoff)
            mapThinBlocksInBoundReRequestedTx.erase(mi);
    }
}

void CThinBlockData::UpdateMempoolLimiterBytesSaved(unsigned int nBytesSaved)
{
    LOCK(cs_thinblockstats);
    nMempoolLimiterBytesSaved += nBytesSaved;
}

string CThinBlockData::ToString()
{
    LOCK(cs_thinblockstats);

    static const char *units[] = { "B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
    int i = 0;
    double size = double( nOriginalSize() - nThinSize() - nTotalBloomFilterBytes());
    while (size > 1000) {
	size /= 1000;
	i++;
    }

    ostringstream ss;
    ss << fixed << setprecision(2);
    ss << nBlocks() << " thin " << ((nBlocks() > 1) ? "blocks have" : "block has") << " saved " << size << units[i] << " of bandwidth";
    return ss.str();
}

// Calculate the xthin percentage compression over the last 24 hours
string CThinBlockData::InBoundPercentToString()
{
    LOCK(cs_thinblockstats);

    // Delete any entries that are more than 24 hours old
    int64_t nTimeCutoff = GetTimeMillis() - 60*60*24*1000;
    map<int64_t, pair<uint64_t, uint64_t> >::iterator iter = mapThinBlocksInBound.begin();
    while (iter != mapThinBlocksInBound.end())
    {
        map<int64_t, pair<uint64_t, uint64_t> >::iterator mi = iter++; // increment to avoid iterator becoming invalid
        if ((*mi).first < nTimeCutoff)
            mapThinBlocksInBound.erase(mi);
    }

    double nCompressionRate = 0;
    uint64_t nThinSizeTotal = 0;
    uint64_t nOriginalSizeTotal = 0;
    for (map<int64_t, pair<uint64_t, uint64_t> >::iterator mi = mapThinBlocksInBound.begin(); mi != mapThinBlocksInBound.end(); ++mi) {
        nThinSizeTotal += (*mi).second.first;
        nOriginalSizeTotal += (*mi).second.second;
    }
    // We count up the outbound bloom filters. Outbound bloom filters go with Inbound xthins.
    uint64_t nOutBoundBloomFilterSize = 0;
    for (map<int64_t, uint64_t>::iterator mi = mapBloomFiltersOutBound.begin(); mi != mapBloomFiltersOutBound.end(); ++mi) {
        nOutBoundBloomFilterSize += (*mi).second;
    }


    if (nOriginalSizeTotal > 0)
        nCompressionRate = 100 - (100 * (double)(nThinSizeTotal + nOutBoundBloomFilterSize) / nOriginalSizeTotal);

    ostringstream ss;
    ss << fixed << setprecision(1);
    ss << "Compression for " << mapThinBlocksInBound.size() << " Inbound  thinblocks (last 24hrs): " << nCompressionRate << "%";
    return ss.str();
}

// Calculate the xthin percentage compression over the last 24 hours
string CThinBlockData::OutBoundPercentToString()
{
    LOCK(cs_thinblockstats);

    // Delete any entries that are more than 24 hours old
    int64_t nTimeCutoff = GetTimeMillis() - 60*60*24*1000;
    map<int64_t, pair<uint64_t, uint64_t> >::iterator iter = mapThinBlocksOutBound.begin();
    while (iter != mapThinBlocksOutBound.end())
    {
        map<int64_t, pair<uint64_t, uint64_t> >::iterator mi = iter++; // increment to avoid iterator becoming invalid
        if ((*mi).first < nTimeCutoff)
            mapThinBlocksOutBound.erase(mi);
    }

    double nCompressionRate = 0;
    uint64_t nThinSizeTotal = 0;
    uint64_t nOriginalSizeTotal = 0;
    for (map<int64_t, pair<uint64_t, uint64_t> >::iterator mi = mapThinBlocksOutBound.begin(); mi != mapThinBlocksOutBound.end(); ++mi) {
        nThinSizeTotal += (*mi).second.first;
        nOriginalSizeTotal += (*mi).second.second;
    }
    // We count up the inbound bloom filters. Inbound bloom filters go with Outbound xthins.
    uint64_t nInBoundBloomFilterSize = 0;
    for (map<int64_t, uint64_t>::iterator mi = mapBloomFiltersInBound.begin(); mi != mapBloomFiltersInBound.end(); ++mi) {
        nInBoundBloomFilterSize += (*mi).second;
    }

    if (nOriginalSizeTotal > 0)
        nCompressionRate = 100 - (100 * (double)(nThinSizeTotal + nInBoundBloomFilterSize) / nOriginalSizeTotal);

    ostringstream ss;
    ss << fixed << setprecision(1);
    ss << "Compression for " << mapThinBlocksOutBound.size() << " Outbound thinblocks (last 24hrs): " << nCompressionRate << "%";
    return ss.str();
}

// Calculate the average inbound xthin bloom filter size
string CThinBlockData::InBoundBloomFiltersToString()
{
    LOCK(cs_thinblockstats);

    // Delete any entries that are more than 24 hours old
    int64_t nTimeCutoff = GetTimeMillis() - 60*60*24*1000;
    map<int64_t, uint64_t>::iterator iter = mapBloomFiltersInBound.begin();
    while (iter != mapBloomFiltersInBound.end())
    {
        map<int64_t, uint64_t>::iterator mi = iter++; // increment to avoid iterator becoming invalid
        if ((*mi).first < nTimeCutoff)
            mapBloomFiltersInBound.erase(mi);
    }

    uint64_t nInBoundBloomFilters = 0;
    uint64_t nInBoundBloomFilterSize = 0;
    double avgBloomSize = 0;
    for (map<int64_t, uint64_t>::iterator mi = mapBloomFiltersInBound.begin(); mi != mapBloomFiltersInBound.end(); ++mi) {
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

    ostringstream ss;
    ss << fixed << setprecision(1);
    ss << "Inbound bloom filter size (last 24hrs) AVG: " << avgBloomSize << units[i];
    return ss.str();
}

// Calculate the average inbound xthin bloom filter size
string CThinBlockData::OutBoundBloomFiltersToString()
{
    LOCK(cs_thinblockstats);

    // Delete any entries that are more than 24 hours old
    int64_t nTimeCutoff = GetTimeMillis() - 60*60*24*1000;
    map<int64_t, uint64_t>::iterator iter = mapBloomFiltersOutBound.begin();
    while (iter != mapBloomFiltersOutBound.end())
    {
        map<int64_t, uint64_t>::iterator mi = iter++; // increment to avoid iterator becoming invalid
        if ((*mi).first < nTimeCutoff)
            mapBloomFiltersOutBound.erase(mi);
    }

    uint64_t nOutBoundBloomFilters = 0;
    uint64_t nOutBoundBloomFilterSize = 0;
    double avgBloomSize = 0;
    for (map<int64_t, uint64_t>::iterator mi = mapBloomFiltersOutBound.begin(); mi != mapBloomFiltersOutBound.end(); ++mi) {
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

    ostringstream ss;
    ss << fixed << setprecision(1);
    ss << "Outbound bloom filter size (last 24hrs) AVG: " << avgBloomSize << units[i];
    return ss.str();
}
// Calculate the xthin percentage compression over the last 24 hours
string CThinBlockData::ResponseTimeToString()
{
    LOCK(cs_thinblockstats);

    vector<double> vResponseTime;

    double nResponseTimeAverage = 0;
    double nPercentile = 0;
    double nTotalResponseTime = 0;
    double nTotalEntries = 0;
    for (map<int64_t, double>::iterator mi = mapThinBlockResponseTime.begin(); mi != mapThinBlockResponseTime.end(); ++mi) {
        nTotalEntries += 1;
        nTotalResponseTime += (*mi).second;
        vResponseTime.push_back((*mi).second);
    }

    if (nTotalEntries > 0) {
        nResponseTimeAverage = (double)nTotalResponseTime / nTotalEntries;

        // Calculate the 95th percentile
        uint64_t nPercentileElement = static_cast<int>((nTotalEntries * 0.95) + 0.5) - 1;
        sort(vResponseTime.begin(), vResponseTime.end());
        nPercentile = vResponseTime[nPercentileElement];
    }

    ostringstream ss;
    ss << fixed << setprecision(2);
    ss << "Response time   (last 24hrs) AVG:" << nResponseTimeAverage << ", 95th pcntl:" << nPercentile;
    return ss.str();
}

// Calculate the xthin percentage compression over the last 24 hours
string CThinBlockData::ValidationTimeToString()
{
    LOCK(cs_thinblockstats);

    vector<double> vValidationTime;

    double nValidationTimeAverage = 0;
    double nPercentile = 0;
    double nTotalValidationTime = 0;
    double nTotalEntries = 0;
    for (map<int64_t, double>::iterator mi = mapThinBlockValidationTime.begin(); mi != mapThinBlockValidationTime.end(); ++mi) {
        nTotalEntries += 1;
        nTotalValidationTime += (*mi).second;
        vValidationTime.push_back((*mi).second);
    }

    if (nTotalEntries > 0) {
        nValidationTimeAverage = (double)nTotalValidationTime / nTotalEntries;

        // Calculate the 95th percentile
        uint64_t nPercentileElement = static_cast<int>((nTotalEntries * 0.95) + 0.5) - 1;
        sort(vValidationTime.begin(), vValidationTime.end());
        nPercentile = vValidationTime[nPercentileElement];
    }

    ostringstream ss;
    ss << fixed << setprecision(2);
    ss << "Validation time (last 24hrs) AVG:" << nValidationTimeAverage << ", 95th pcntl:" << nPercentile;
    return ss.str();
}

// Calculate the xthin percentage compression over the last 24 hours
string CThinBlockData::ReRequestedTxToString()
{
    LOCK(cs_thinblockstats);

    // Delete any entries that are more than 24 hours old
    int64_t nTimeCutoff = GetTimeMillis() - 60*60*24*1000;
    map<int64_t, int>::iterator iter = mapThinBlocksInBoundReRequestedTx.begin();
    while (iter != mapThinBlocksInBoundReRequestedTx.end())
    {
        map<int64_t, int>::iterator mi = iter++; // increment to avoid iterator becoming invalid
        if ((*mi).first < nTimeCutoff)
            mapThinBlocksInBoundReRequestedTx.erase(mi);
    }

    double nReRequestRate = 0;
    uint64_t nTotalReRequests = 0;
    uint64_t nTotalReRequestedTxs = 0;
    for (map<int64_t, int>::iterator mi = mapThinBlocksInBoundReRequestedTx.begin(); mi != mapThinBlocksInBoundReRequestedTx.end(); ++mi) {
        nTotalReRequests += 1;
        nTotalReRequestedTxs += (*mi).second;
    }

    if ( mapThinBlocksInBound.size() > 0)
        nReRequestRate = 100 * (double)nTotalReRequests / mapThinBlocksInBound.size();

    ostringstream ss;
    ss << fixed << setprecision(1);
    ss << "Tx re-request rate (last 24hrs): " << nReRequestRate << "% Total re-requests:" << nTotalReRequests;
    return ss.str();
}

string CThinBlockData::MempoolLimiterBytesSavedToString()
{
    LOCK(cs_thinblockstats);

    static const char *units[] = { "B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
    int i = 0;
    double size = (double)nMempoolLimiterBytesSaved();
    while (size > 1000) {
	size /= 1000;
	i++;
    }

    ostringstream ss;
    ss << fixed << setprecision(2);
    ss << "Thinblock mempool limiting has saved " << size << units[i] << " of bandwidth";
    return ss.str();
}

bool CThinBlockData::CheckThinblockTimer(uint256 hash)
{
    LOCK(cs_mapThinBlockTimer);
    if (!mapThinBlockTimer.count(hash)) {
        mapThinBlockTimer[hash] = GetTimeMillis();
        LogPrint("thin", "Starting Preferential Thinblock timer\n");
    }
    else {
        // Check that we have not exceeded the 10 second limit.
        // If we have then we want to return false so that we can
        // proceed to download a regular block instead.
        uint64_t elapsed = GetTimeMillis() - mapThinBlockTimer[hash];
        if (elapsed > 10000) {
            LogPrint("thin", "Preferential Thinblock timer exceeded - downloading regular block instead\n");
            return false;
        }
    }
    return true;
}

void CThinBlockData::ClearThinBlockTimer(uint256 hash)
{
    LOCK(cs_mapThinBlockTimer);
    if (mapThinBlockTimer.count(hash)) {
        mapThinBlockTimer.erase(hash);
        LogPrint("thin", "Clearing Preferential Thinblock timer\n");
    }
}


bool HaveConnectThinblockNodes()
{
    // Strip the port from then list of all the current in and outbound ip addresses
    vector<string> vNodesIP;
    {
        LOCK(cs_vNodes);
        BOOST_FOREACH (CNode* pnode, vNodes) {
           int pos = pnode->addrName.rfind(":");
           if (pos <= 0 )
               vNodesIP.push_back(pnode->addrName);
           else
               vNodesIP.push_back(pnode->addrName.substr(0, pos));
        }
    }

    // Create a set used to check for cross connected nodes.
    // A cross connected node is one where we have a connect-thinblock connection to
    // but we also have another inbound connection which is also using
    // connect-thinblock. In those cases we have created a dead-lock where no blocks 
    // can be downloaded unless we also have at least one additional connect-thinblock 
    // connection to a different node.
    set<string> nNotCrossConnected;
 
    int nConnectionsOpen = 0;
    BOOST_FOREACH(const string& strAddrNode, mapMultiArgs["-connect-thinblock"]) {
        string strThinblockNode;
        int pos = strAddrNode.rfind(":");
        if (pos <= 0 )
            strThinblockNode = strAddrNode;
        else
            strThinblockNode = strAddrNode.substr(0, pos);
        BOOST_FOREACH(string strAddr, vNodesIP) {
            if (strAddr == strThinblockNode) {
                nConnectionsOpen++;
                if (!nNotCrossConnected.count(strAddr))
                    nNotCrossConnected.insert(strAddr);
                else
                    nNotCrossConnected.erase(strAddr);
            }
        }
    }
    if (nNotCrossConnected.size() > 0)
        return true;
    else if (nConnectionsOpen > 0)
        LogPrint("thin", "You have a cross connected thinblock node - we may download regular blocks until you resolve the issue\n");
    return false; // Connections are either not open or they are cross connected.
} 


bool HaveThinblockNodes()
{
    {
        LOCK(cs_vNodes);
        BOOST_FOREACH (CNode* pnode, vNodes)
            if (pnode->ThinBlockCapable())
                return true;
    }
    return false;
}

bool IsThinBlocksEnabled() 
{
    return GetBoolArg("-use-thinblocks", true);
}

bool CanThinBlockBeDownloaded(CNode* pto)
{
    if (pto->ThinBlockCapable() && !GetBoolArg("-connect-thinblock-force", false))
        return true;
    else if (pto->ThinBlockCapable() && GetBoolArg("-connect-thinblock-force", false)) {
        // If connect-thinblock-force is true then we have to check that this node is in fact a connect-thinblock node.

        // When -connect-thinblock-force is true we will only download thinblocks from a peer or peers that
        // are using -connect-thinblock=<ip>.  This is an undocumented setting used for setting up performance testing
        // of thinblocks, such as, going over the GFC and needing to have thinblocks always come from the same peer or 
        // group of peers.  Also, this is a one way street.  Thinblocks will flow ONLY from the remote peer to the peer
        // that has invoked -connect-thinblock.

        // Check if this node is also a connect-thinblock node
        BOOST_FOREACH(const string& strAddrNode, mapMultiArgs["-connect-thinblock"])
            if (pto->addrName == strAddrNode)
                return true;
    }
    return false;
}

void ConnectToThinBlockNodes()
{
    // Connect to specific addresses
    if (mapArgs.count("-connect-thinblock") && mapMultiArgs["-connect-thinblock"].size() > 0)
    {
        BOOST_FOREACH(const string& strAddr, mapMultiArgs["-connect-thinblock"])
        {
            CAddress addr;
            //NOTE: Because the only nodes we are connecting to here are the ones the user put in their
            //      bitcoin.conf/commandline args as "-connect-thinblock", we don't use the semaphore to limit outbound connections
            OpenNetworkConnection(addr, NULL, strAddr.c_str());
            MilliSleep(500);
        }
    }
}

void CheckNodeSupportForThinBlocks()
{
    if(IsThinBlocksEnabled()) {
        //BU: Enforce cs_vNodes lock held external to FindNode function calls to prevent use-after-free errors
        LOCK(cs_vNodes);
        // Check that a nodes pointed to with connect-thinblock actually supports thinblocks
        BOOST_FOREACH(string& strAddr, mapMultiArgs["-connect-thinblock"]) {
            if(CNode* pnode = FindNode(strAddr)) {
                if(!pnode->ThinBlockCapable()) {
                    LogPrintf("ERROR: You are trying to use connect-thinblocks but to a node that does not support it - Protocol Version: %d peer=%d\n", 
                               pnode->nVersion, pnode->id);
                }
            }
        }
    }
}

void SendXThinBlock(CBlock &block, CNode* pfrom, const CInv &inv)
{
    if (inv.type == MSG_XTHINBLOCK)
    {
        CXThinBlock xThinBlock(block, pfrom->pThinBlockFilter);
        int nSizeBlock = ::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION);
        if (xThinBlock.collision == true) // If there is a cheapHash collision in this block then send a normal thinblock
        {
            CThinBlock thinBlock(block, *pfrom->pThinBlockFilter);
            int nSizeThinBlock = ::GetSerializeSize(xThinBlock, SER_NETWORK, PROTOCOL_VERSION);
            if (nSizeThinBlock < nSizeBlock) {
                pfrom->PushMessage(NetMsgType::THINBLOCK, thinBlock);
                thindata.UpdateOutBound(nSizeThinBlock, nSizeBlock);
                LogPrint("thin", "TX HASH COLLISION: Sent thinblock - size: %d vs block size: %d => tx hashes: %d transactions: %d  peer: %s (%d)\n", nSizeThinBlock, nSizeBlock, xThinBlock.vTxHashes.size(), xThinBlock.vMissingTx.size(), pfrom->addrName.c_str(), pfrom->id);
            }
            else {
                pfrom->PushMessage(NetMsgType::BLOCK, block);
                LogPrint("thin", "Sent regular block instead - xthinblock size: %d vs block size: %d => tx hashes: %d transactions: %d  peer: %s (%d)\n", nSizeThinBlock, nSizeBlock, xThinBlock.vTxHashes.size(), xThinBlock.vMissingTx.size(), pfrom->addrName.c_str(), pfrom->id);
            }
        }
        else // Send an xThinblock
        {
            // Only send a thinblock if smaller than a regular block
            int nSizeThinBlock = ::GetSerializeSize(xThinBlock, SER_NETWORK, PROTOCOL_VERSION);
            if (nSizeThinBlock < nSizeBlock) {
                thindata.UpdateOutBound(nSizeThinBlock, nSizeBlock);
                pfrom->PushMessage(NetMsgType::XTHINBLOCK, xThinBlock);
                LogPrint("thin", "Sent xthinblock - size: %d vs block size: %d => tx hashes: %d transactions: %d peer: %s (%d)\n", nSizeThinBlock, nSizeBlock, xThinBlock.vTxHashes.size(), xThinBlock.vMissingTx.size(), pfrom->addrName.c_str(), pfrom->id);
            }
            else {
                pfrom->PushMessage(NetMsgType::BLOCK, block);
                LogPrint("thin", "Sent regular block instead - xthinblock size: %d vs block size: %d => tx hashes: %d transactions: %d  peer: %s (%d)\n", nSizeThinBlock, nSizeBlock, xThinBlock.vTxHashes.size(), xThinBlock.vMissingTx.size(), pfrom->addrName.c_str(), pfrom->id);
            }
        }
    }
    else if (inv.type == MSG_THINBLOCK)
    {
        CThinBlock thinBlock(block, *pfrom->pThinBlockFilter);
        int nSizeBlock = ::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION);
        int nSizeThinBlock = ::GetSerializeSize(thinBlock, SER_NETWORK, PROTOCOL_VERSION);
        if (nSizeThinBlock < nSizeBlock) { // Only send a thinblock if smaller than a regular block
            thindata.UpdateOutBound(nSizeThinBlock, nSizeBlock);
            pfrom->PushMessage(NetMsgType::THINBLOCK, thinBlock);
            LogPrint("thin", "Sent thinblock - size: %d vs block size: %d => tx hashes: %d transactions: %d  peer: %s (%d)\n", nSizeThinBlock, nSizeBlock, thinBlock.vTxHashes.size(), thinBlock.vMissingTx.size(), pfrom->addrName.c_str(), pfrom->id);
        }
        else {
            pfrom->PushMessage(NetMsgType::BLOCK, block);
            LogPrint("thin", "Sent regular block instead - thinblock size: %d vs block size: %d => tx hashes: %d transactions: %d  peer: %s (%d)\n", nSizeThinBlock, nSizeBlock, thinBlock.vTxHashes.size(), thinBlock.vMissingTx.size(), pfrom->addrName.c_str(), pfrom->id);
        }
    }
    else
    {
        Misbehaving(pfrom->GetId(), 100);
        return;
    }
    pfrom->blocksSent += 1;
}

bool IsThinBlockValid(const CNode *pfrom, const std::vector<CTransaction> &vMissingTx, const CBlockHeader &header)
{
    // Check that that there is at least one txn in the xthin and that the first txn is the coinbase
    if (vMissingTx.empty())
    {
        return error("No Transactions found in thinblock or xthinblock %s from peer %s (id=%d)",
            header.GetHash().ToString(), pfrom->addrName.c_str(), pfrom->id);
    }
    if (!vMissingTx[0].IsCoinBase())
    {
        return error("First txn is not coinbase for thinblock or xthinblock %s from peer %s (id=%d)",
            header.GetHash().ToString(), pfrom->addrName.c_str(), pfrom->id);
    }

    // check block header
    CValidationState state;
    if (!CheckBlockHeader(header, state, true))
    {
        return error("Received invalid header for thinblock or xthinblock %s from peer %s (id=%d)",
            header.GetHash().ToString(), pfrom->addrName.c_str(), pfrom->id);
    }
    if (state.Invalid())
    {
        return error("Received invalid header for thinblock or xthinblock %s from peer %s (id=%d)",
            header.GetHash().ToString(), pfrom->addrName.c_str(), pfrom->id);
    }

    return true;
}

void BuildSeededBloomFilter(CBloomFilter& filterMemPool, vector<uint256>& vOrphanHashes, uint256 hash, bool fDeterministic)
{
    int64_t nStartTimer = GetTimeMillis();
    seed_insecure_rand(fDeterministic);
    set<uint256> setHighScoreMemPoolHashes;
    set<uint256> setPriorityMemPoolHashes;

    // How much of the block should be dedicated to high-priority transactions.
    // Logically this should be the same size as the DEFAULT_BLOCK_PRIORITY_SIZE however,
    // we can't be sure that a miner won't decide to mine more high priority txs and therefore
    // by including a full blocks worth of high priority tx's we cover every scenario.  And when we
    // go on to add the high fee tx's there will be an intersection between the two which then makes 
    // the total number of tx's that go into the bloom filter smaller than just the sum of the two.  
    uint64_t nBlockPrioritySize = LargestBlockSeen() * 1.5;

    // Largest projected block size used to add the high fee transactions.  We multiply it by an
    // additional factor to take into account that miners may have slighty different policies when selecting
    // high fee tx's from the pool.
    uint64_t nBlockMaxProjectedSize = LargestBlockSeen() * 1.5;

    vector<TxCoinAgePriority> vPriority;
    TxCoinAgePriorityCompare pricomparer;
    {
        LOCK2(cs_main, mempool.cs);
        if (mempool.mapTx.size() > 0) 
        {
            CBlockIndex* pindexPrev = chainActive.Tip();
            const int nHeight = pindexPrev->nHeight + 1;
            const int64_t nMedianTimePast = pindexPrev->GetMedianTimePast();

            int64_t nLockTimeCutoff = (STANDARD_LOCKTIME_VERIFY_FLAGS & LOCKTIME_MEDIAN_TIME_PAST)
                                    ? nMedianTimePast
                                    : GetAdjustedTime();

            // Create a sorted list of transactions and their updated priorities.  This will be used to fill
            // the mempoolhashes with the expected priority area of the next block.  We will multiply this by
            // a factor of ? to account for any differences between the "Miners".
            vPriority.reserve(mempool.mapTx.size());
            for (CTxMemPool::indexed_transaction_set::iterator mi = mempool.mapTx.begin();
                 mi != mempool.mapTx.end(); mi++)
            {
                double dPriority = mi->GetPriority(nHeight);
                CAmount dummy;
                mempool.ApplyDeltas(mi->GetTx().GetHash(), dPriority, dummy);
                vPriority.push_back(TxCoinAgePriority(dPriority, mi));
            }
            make_heap(vPriority.begin(), vPriority.end(), pricomparer);

            uint64_t nPrioritySize = 0;
            CTxMemPool::txiter iter;
            for (uint64_t i = 0; i < vPriority.size(); i++)
            {
                nPrioritySize += vPriority[i].second->GetTxSize();
                if (nPrioritySize > nBlockPrioritySize)
                    break;
                setPriorityMemPoolHashes.insert(vPriority[i].second->GetTx().GetHash());

                // Add children.  We don't need to look for parents here since they will all be parents.
                iter = mempool.mapTx.project<0>(vPriority[i].second);
                BOOST_FOREACH(CTxMemPool::txiter child, mempool.GetMemPoolChildren(iter))
                {
                    uint256 childHash = child->GetTx().GetHash();
                    if (!setPriorityMemPoolHashes.count(childHash)) {
                        setPriorityMemPoolHashes.insert(childHash);
                        nPrioritySize += child->GetTxSize();
                        LogPrint("bloom", "add priority child %s with fee %d modified fee %d size %d clearatentry %d priority %f\n", 
                                       child->GetTx().GetHash().ToString(), child->GetFee(), child->GetModifiedFee(), 
                                       child->GetTxSize(), child->WasClearAtEntry(), child->GetPriority(nHeight));
                    }
                }            
            }

            // Create a list of high score transactions. We will multiply this by
            // a factor of ? to account for any differences between the way Miners include tx's
            CTxMemPool::indexed_transaction_set::nth_index<3>::type::iterator mi = mempool.mapTx.get<3>().begin();
            uint64_t nBlockSize = 0;
            while (mi != mempool.mapTx.get<3>().end())
            {
                CTransaction tx = mi->GetTx();

                if (!IsFinalTx(tx, nHeight, nLockTimeCutoff)) {
                    LogPrint("bloom", "tx %s is not final\n", tx.GetHash().ToString());
                    mi++;
                    continue;
                }

                // If this tx is not accounted for already in the priority set then continue and add
                // it to the high score set if it can be and also add any parents or children.  Also add
                // children and parents to the priority set tx's if they have any.
                iter = mempool.mapTx.project<0>(mi);
                if (!setHighScoreMemPoolHashes.count(tx.GetHash()))
                {
                    LogPrint("bloom", "next tx is %s blocksize %d fee %d modified fee %d size %d clearatentry %d priority %f\n", 
                                   mi->GetTx().GetHash().ToString(), nBlockSize, mi->GetFee(), mi->GetModifiedFee(), mi->GetTxSize(), 
                                   mi->WasClearAtEntry(), mi->GetPriority(nHeight));

                    // add tx to the set: we don't know if this is a parent or child yet.
                    setHighScoreMemPoolHashes.insert(tx.GetHash());

                    // Add any parent tx's
                    bool fChild = false;
                    BOOST_FOREACH(CTxMemPool::txiter parent, mempool.GetMemPoolParents(iter))
                    {
                        fChild = true;
                        uint256 parentHash = parent->GetTx().GetHash();
                        if (!setHighScoreMemPoolHashes.count(parentHash)) {
                            setHighScoreMemPoolHashes.insert(parentHash);
                            LogPrint("bloom", "add high score parent %s with blocksize %d fee %d modified fee %d size %d clearatentry %d priority %f\n", 
                                           parent->GetTx().GetHash().ToString(), nBlockSize, parent->GetFee(), parent->GetModifiedFee(), 
                                           parent->GetTxSize(), parent->WasClearAtEntry(), parent->GetPriority(nHeight));
                        }
                    }

                    // Now add any children tx's.
                    bool fHasChildren = false;
                    BOOST_FOREACH(CTxMemPool::txiter child, mempool.GetMemPoolChildren(iter))
                    {
                        fHasChildren = true;
                        uint256 childHash = child->GetTx().GetHash();
                        if (!setHighScoreMemPoolHashes.count(childHash)) {
                            setHighScoreMemPoolHashes.insert(childHash);
                            LogPrint("bloom", "add high score child %s with blocksize %d fee %d modified fee %d size %d clearatentry %d priority %f\n", 
                                           child->GetTx().GetHash().ToString(), nBlockSize, child->GetFee(), child->GetModifiedFee(), 
                                           child->GetTxSize(), child->WasClearAtEntry(), child->GetPriority(nHeight));
                        }
                    }

                    // If a tx with no parents and no children, then we increment this block size.  
                    // We don't want to add parents and children to the size because for tx's with many children, miners may not mine them
                    // as they are not as profitable but we still have to add their hash to the bloom filter in case they do.
                    if (!fChild && !fHasChildren)
                        nBlockSize += mi->GetTxSize();
                }

                if (nBlockSize >  nBlockMaxProjectedSize)
                    break;

                mi++;
            }
        }
    }
    LogPrint("thin", "Bloom Filter Targeting completed in:%d (ms)\n", GetTimeMillis() - nStartTimer);
    nStartTimer= GetTimeMillis(); // reset the timer

    // We set the beginning of our growth algortithm to the time we request our first xthin.  We do this here
    // rather than setting up a global variable in init.cpp.  This has more to do with potential merge conflicts
    // with BU than any other technical reason.
    static int64_t nStartGrowth = GetTime();

    // Tuning knobs for the false positive growth algorithm
    static uint8_t nHoursToGrow = 72; // number of hours until maximum growth for false positive rate
    //static double nGrowthCoefficient = 0.7676; // use for nMinFalsePositive = 0.0001 and nMaxFalsePositive = 0.01 for 6 hour growth period
    //static double nGrowthCoefficient = 0.8831; // use for nMinFalsePositive = 0.0001 and nMaxFalsePositive = 0.02 for 6 hour growth period
    //static double nGrowthCoefficient = 0.1921; // use for nMinFalsePositive = 0.0001 and nMaxFalsePositive = 0.01 for 24 hour growth period
    static double nGrowthCoefficient = 0.0544; // use for nMinFalsePositive = 0.0001 and nMaxFalsePositive = 0.005 for 72 hour growth period
    static double nMinFalsePositive = 0.0001; // starting value for false positive 
    static double nMaxFalsePositive = 0.005; // maximum false positive rate at end of decay
    // TODO: automatically calculate the nGrowthCoefficient from nHoursToGrow, nMinFalsePositve and nMaxFalsePositive

    // Count up all the transactions that we'll be putting into the filter, removing any duplicates
    BOOST_FOREACH(uint256 txHash, setHighScoreMemPoolHashes)
        if (setPriorityMemPoolHashes.count(txHash))
            setPriorityMemPoolHashes.erase(txHash);

    unsigned int nSelectedTxHashes = setHighScoreMemPoolHashes.size() + vOrphanHashes.size() + setPriorityMemPoolHashes.size();
    unsigned int nElements = max(nSelectedTxHashes, (unsigned int)1); // Must make sure nElements is greater than zero or will assert

    // Calculate the new False Positive rate.  
    // We increase the false positive rate as time increases, starting at nMinFalsePositive and with growth governed by nGrowthCoefficient,
    // using the simple exponential growth function as follows:
    // y = (starting or minimum fprate: nMinFalsePositive) * e ^ (time in hours from start * nGrowthCoefficient)
    int64_t nTimePassed = GetTime() - nStartGrowth;
    double nFPRate = nMinFalsePositive * exp (((double)(nTimePassed) / 3600) * nGrowthCoefficient);
    if (nTimePassed > nHoursToGrow * 3600)
        nFPRate = nMaxFalsePositive;

    filterMemPool = CBloomFilter(nElements, nFPRate, insecure_rand(), BLOOM_UPDATE_ALL);
    LogPrint("thin", "FPrate: %f Num elements in bloom filter:%d high priority txs:%d high fee txs:%d orphans:%d total txs in mempool:%d\n", 
              nFPRate, nElements, setPriorityMemPoolHashes.size(), 
              setHighScoreMemPoolHashes.size(), vOrphanHashes.size(), mempool.mapTx.size());

    // Add the selected tx hashes to the bloom filter
    BOOST_FOREACH(uint256 txHash, setPriorityMemPoolHashes)
        filterMemPool.insert(txHash);
    BOOST_FOREACH(uint256 txHash, setHighScoreMemPoolHashes)
        filterMemPool.insert(txHash);
    BOOST_FOREACH(uint256 txHash, vOrphanHashes)
        filterMemPool.insert(txHash);
    uint64_t nSizeFilter = ::GetSerializeSize(filterMemPool, SER_NETWORK, PROTOCOL_VERSION);
    LogPrint("thin", "Created bloom filter: %d bytes for block: %s in:%d (ms)\n", nSizeFilter, hash.ToString(), GetTimeMillis() - nStartTimer);
    thindata.UpdateOutBoundBloomFilter(nSizeFilter);
}




