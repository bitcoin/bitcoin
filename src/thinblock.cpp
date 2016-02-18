// Copyright (c) 2016 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "thinblock.h"

CThinBlock::CThinBlock(const CBlock& block, CBloomFilter& filter)
{
    header = block.GetBlockHeader();

    vTxHashes.reserve(block.vtx.size());
    for (unsigned int i = 0; i < block.vtx.size(); i++)
    {
        const uint256& hash = block.vtx[i].GetHash();
        vTxHashes.push_back(hash);

        // Find the transactions that do not match the filter.
        // These are the ones we need to relay back to the requesting peer.
        // NOTE: We always add the first tx, the coinbase as it is the one
        //       most often missing.
        if (!filter.contains(hash) || i == 0)
            mapMissingTx[hash] = block.vtx[i];
    }
}

CXThinBlock::CXThinBlock(const CBlock& block, CBloomFilter* filter)
{
    header = block.GetBlockHeader();
    this->collision = false;

    vTxHashes.reserve(block.vtx.size());
    std::set<uint64_t> setPartialTxHash;
    for (unsigned int i = 0; i < block.vtx.size(); i++)
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
            mapMissingTx[hash256] = block.vtx[i];
    }
}

CXThinBlock::CXThinBlock(const CBlock& block)
{
    header = block.GetBlockHeader();
    this->collision = false;

    vTxHashes.reserve(block.vtx.size());
    std::set<uint64_t> setPartialTxHash;

    for (unsigned int i = 1; i < block.vtx.size(); i++)
    {
        const uint256 hash256 = block.vtx[i].GetHash();
        uint64_t cheapHash = hash256.GetCheapHash();
        vTxHashes.push_back(cheapHash);

        if (setPartialTxHash.count(cheapHash))
                this->collision = true;
        setPartialTxHash.insert(cheapHash);

        // We always add the first tx, the coinbase as it is the one
        // most often missing.
        if (i == 0) mapMissingTx[hash256] = block.vtx[i];
    }
}



CXThinBlockTx::CXThinBlockTx(uint256 blockHash, std::vector<uint64_t>& vHashesToRequest)
{
    blockhash = blockHash;

    CTransaction tx;
    for (unsigned int i = 0; i < vHashesToRequest.size(); i++)
        mapTx[vHashesToRequest[i]] = tx;
}
