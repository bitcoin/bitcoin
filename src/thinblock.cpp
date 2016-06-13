// Copyright (c) 2016 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "thinblock.h"
#include "utiltime.h"
#include "unlimited.h"
#include <sstream>
#include <iomanip>

// Start statistics at zero
CStatHistory<uint64_t> CThinBlockStats::nOriginalSize("thin/blockSize", STAT_OP_SUM | STAT_KEEP);
CStatHistory<uint64_t> CThinBlockStats::nThinSize("thin/thinSize", STAT_OP_SUM | STAT_KEEP);
CStatHistory<uint64_t> CThinBlockStats::nBlocks("thin/numBlocks", STAT_OP_SUM | STAT_KEEP);
CStatHistory<uint64_t> CThinBlockStats::nMempoolLimiterBytesSaved("nSize", STAT_OP_SUM | STAT_KEEP);
CStatHistory<uint64_t> CThinBlockStats::nTotalBloomFilterBytes("nSizeBloom", STAT_OP_SUM | STAT_KEEP);
std::map<int64_t, std::pair<uint64_t, uint64_t> > CThinBlockStats::mapThinBlocksInBound;
std::map<int64_t, int> CThinBlockStats::mapThinBlocksInBoundReRequestedTx;
std::map<int64_t, std::pair<uint64_t, uint64_t> > CThinBlockStats::mapThinBlocksOutBound;
std::map<int64_t, uint64_t> CThinBlockStats::mapBloomFiltersInBound;
std::map<int64_t, uint64_t> CThinBlockStats::mapBloomFiltersOutBound;
std::map<int64_t, double> CThinBlockStats::mapThinBlockResponseTime;
std::map<int64_t, double> CThinBlockStats::mapThinBlockValidationTime;

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
    CThinBlockStats::nMempoolLimiterBytesSaved += nBytesSaved;
}

std::string CThinBlockStats::ToString()
{
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
