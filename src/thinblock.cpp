// Copyright (c) 2016 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "thinblock.h"
#include <sstream>
#include <iomanip>

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


