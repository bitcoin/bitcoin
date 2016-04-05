// Copyright (c) 2016 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "thinblock.h"
#include "utiltime.h"
#include <sstream>
#include <iomanip>

// Start statistics at zero
CStatHistory<uint64_t> CThinBlockStats::nOriginalSize("thin/blockSize",STAT_OP_SUM | STAT_KEEP);
CStatHistory<uint64_t> CThinBlockStats::nThinSize("thin/thinSize",STAT_OP_SUM | STAT_KEEP);
CStatHistory<uint64_t> CThinBlockStats::nBlocks("thin/numBlocks",STAT_OP_SUM | STAT_KEEP);
std::map<int64_t, std::pair<uint64_t, uint64_t> > CThinBlockStats::mapThinBlocksInBound;
std::map<int64_t, std::pair<uint64_t, uint64_t> > CThinBlockStats::mapThinBlocksOutBound;


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

    for (unsigned int i = 1; i < nTx; i++)
    {
        const uint256 hash256 = block.vtx[i].GetHash();
        uint64_t cheapHash = hash256.GetCheapHash();
        vTxHashes.push_back(cheapHash);

        if (setPartialTxHash.count(cheapHash))
                this->collision = true;
        setPartialTxHash.insert(cheapHash);

        // We always add the first tx, the coinbase as it is the one
        // most often missing.
        if (i == 0) vMissingTx.push_back(block.vtx[i]);
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

void CThinBlockStats::UpdateInBound(uint64_t nThinBlockSize, uint64_t nOriginalBlockSize)
{
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
std::string CThinBlockStats::ToString()
{
    static const char *units[] = { "B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
    int i = 0;
    double size = double( CThinBlockStats::nOriginalSize() - CThinBlockStats::nThinSize() );
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
    double nCompressionRate = 0;
    uint64_t nThinSizeTotal = 0;
    uint64_t nOriginalSizeTotal = 0;
    for (std::map<int64_t, std::pair<uint64_t, uint64_t> >::iterator mi = CThinBlockStats::mapThinBlocksInBound.begin(); mi != CThinBlockStats::mapThinBlocksInBound.end(); ++mi) {
        nThinSizeTotal += (*mi).second.first;
        nOriginalSizeTotal += (*mi).second.second;
    }

    if (nOriginalSizeTotal > 0)
        nCompressionRate = 100 - (100 * (double)nThinSizeTotal / nOriginalSizeTotal);

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1);
    ss << "Compression for Inbound thinblocks (last 24hrs): " << nCompressionRate << "%";
    return ss.str();
}

// Calculate the xthin percentage compression over the last 24 hours
std::string CThinBlockStats::OutBoundPercentToString()
{
    double nCompressionRate = 0;
    uint64_t nThinSizeTotal = 0;
    uint64_t nOriginalSizeTotal = 0;
    for (std::map<int64_t, std::pair<uint64_t, uint64_t> >::iterator mi = CThinBlockStats::mapThinBlocksOutBound.begin(); mi != CThinBlockStats::mapThinBlocksOutBound.end(); ++mi) {
        nThinSizeTotal += (*mi).second.first;
        nOriginalSizeTotal += (*mi).second.second;
    }

    if (nOriginalSizeTotal > 0)
        nCompressionRate = 100 - (100 * (double)nThinSizeTotal / nOriginalSizeTotal);

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1);
    ss << "Compression for Outbound thinblocks (last 24hrs): " << nCompressionRate << "%";
    return ss.str();
}
