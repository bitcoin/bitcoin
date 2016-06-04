// Copyright (c) 2016 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "thinblock.h"
#include "util.h"
#include "net.h"
#include "chainparams.h"
#include "pow.h"
#include "timedata.h"
#include "main.h"
#include "txmempool.h"
#include <sstream>
#include <iomanip>
#include <map>
#include <string>
#include <vector>

// Start statistics at zero
CStatHistory<uint64_t> CThinBlockStats::nOriginalSize("thin/blockSize",STAT_OP_SUM | STAT_KEEP);
CStatHistory<uint64_t> CThinBlockStats::nThinSize("thin/thinSize",STAT_OP_SUM | STAT_KEEP);
CStatHistory<uint64_t> CThinBlockStats::nBlocks("thin/numBlocks",STAT_OP_SUM | STAT_KEEP);


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
    

// Assumes cs_main is locked
bool CXThinBlock::process(CNode* pfrom)  // TODO: request from the "best" txn source not necessarily from the block source 
{
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
  bool collision = false;
  std::map<uint64_t, uint256> mapPartialTxHash;
  std::vector<uint256> memPoolHashes;
  mempool.queryHashes(memPoolHashes);
  for (uint64_t i = 0; i < memPoolHashes.size(); i++) {
    uint64_t cheapHash = memPoolHashes[i].GetCheapHash();
    if(mapPartialTxHash.count(cheapHash)) //Check for collisions
      collision = true;
    mapPartialTxHash[cheapHash] = memPoolHashes[i];
  }
  for (std::map<uint256, COrphanTx>::iterator mi = mapOrphanTransactions.begin(); mi != mapOrphanTransactions.end(); ++mi) {
    uint64_t cheapHash = (*mi).first.GetCheapHash();
    if(mapPartialTxHash.count(cheapHash)) //Check for collisions
      collision = true;
    mapPartialTxHash[cheapHash] = (*mi).first;
  }
  for (std::map<uint256, CTransaction>::iterator mi = mapMissingTx.begin(); mi != mapMissingTx.end(); ++mi) {
    uint64_t cheapHash = (*mi).first.GetCheapHash();
    // Check for cheap hash collision. Only mark as collision if the full hash is not the same,
    // because the same tx could have been received into the mempool during the request of the xthinblock.
    // In that case we would have the same transaction twice, so it is not a real cheap hash collision and we continue normally.
    const uint256 existingHash = mapPartialTxHash[cheapHash];
    if( (!existingHash.IsNull()) ) { // Check if we already have the cheap hash
      if ((existingHash != (*mi).first)) { // Check if it really is a cheap hash collision and not just the same transaction
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
    LogPrintf("(exp) TX HASH COLLISION for xthinblock: re-requesting a thinblock\n");
    return false;
  }

  int missingCount = 0;
  int unnecessaryCount = 0;
  // Xpress Validation - only perform xval if the chaintip matches the last blockhash in the thinblock
  bool fXVal = (header.hashPrevBlock == chainActive.Tip()->GetBlockHash()) ? true : false;

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
  pfrom->thinBlockWaitingForTxns = missingCount;
  LogPrint("thin", "(exp) thinblock waiting for: %d, unnecessary: %d, txs: %d full: %d\n", pfrom->thinBlockWaitingForTxns, unnecessaryCount, pfrom->thinBlock.vtx.size(), mapMissingTx.size());

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
    CThinBlockStats::Update(pfrom->nSizeThinBlock, blockSize);
    std::string ss = CThinBlockStats::ToString();
    LogPrint("thin", "thin block stats: %s\n", ss.c_str());

    HandleBlockMessage(pfrom, "", pfrom->thinBlock, GetInv());  // clears the thin block
    BOOST_FOREACH(uint64_t &cheapHash, vTxHashes)
      EraseOrphanTx(mapPartialTxHash[cheapHash]);
  }
  else if (pfrom->thinBlockWaitingForTxns > 0) {
    // This marks the end of the transactions we've received. If we get this and we have NOT been able to
    // finish reassembling the block, we need to re-request the transactions we're missing:
    std::set<uint64_t> setHashesToRequest;
    size_t count=0;
    for (size_t i = 0; i < pfrom->thinBlock.vtx.size(); i++) {
      if (pfrom->thinBlock.vtx[i].IsNull()) {
	setHashesToRequest.insert(pfrom->xThinBlockHashes[i]);
        count++;
      }
    }
    //LogPrint("thin", "(exp) Re-requesting %d tx\n", count);
    // Re-request transactions that we are still missing
    CXRequestThinBlockTx thinBlockTx(header.GetHash(), setHashesToRequest);
    pfrom->mapThinBlocksInFlight[header.GetHash()] = GetTime(); // make a note that we are requesting for this block
    pfrom->PushMessage(NetMsgType::GET_XBLOCKTX, thinBlockTx);
    LogPrint("thin", "(exp) Missing %d transactions for xthinblock, re-requesting\n", 
	     pfrom->thinBlockWaitingForTxns);
  }

  return true;
}


void CThinBlockStats::Update(uint64_t nThinBlockSize, uint64_t nOriginalBlockSize)
{
	CThinBlockStats::nOriginalSize += nOriginalBlockSize;
	CThinBlockStats::nThinSize += nThinBlockSize;
	CThinBlockStats::nBlocks+=1;
}


std::string CThinBlockStats::ToString()
{
	static const char *units[] = { "B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
	int i = 0;
	double size = double( CThinBlockStats::nOriginalSize() - CThinBlockStats::nThinSize() );
	while (size > 1024) {
		size /= 1024;
		i++;
	}

	std::ostringstream ss;
	ss << std::fixed << std::setprecision(2);
	ss << CThinBlockStats::nBlocks() << " thin " << ((CThinBlockStats::nBlocks()>1) ? "blocks have" : "block has") << " saved " << size << units[i] << " of bandwidth";
	std::string s = ss.str();
	return s;
}


