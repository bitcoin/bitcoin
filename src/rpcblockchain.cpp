// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"
#include "rpcserver.h"
#include "init.h"
#include "txdb.h"
#include "kernel.h"
#include "checkpoints.h"
#include <errno.h>


using namespace json_spirit;
using namespace std;

extern void TxToJSON(const CTransaction& tx, const uint256 hashBlock, json_spirit::Object& entry);

double BitsToDouble(unsigned int nBits)
{
    // Floating point number that is a multiple of the minimum difficulty,
    // minimum difficulty = 1.0.
    int nShift = (nBits >> 24) & 0xff;

    double dDiff = (double)0x0000ffff / (double)(nBits & 0x00ffffff);

    while (nShift < 29)
    {
        dDiff *= 256.0;
        nShift++;
    };
    
    while (nShift > 29)
    {
        dDiff /= 256.0;
        nShift--;
    };

    return dDiff;
};

double GetDifficulty(const CBlockIndex* blockindex)
{
    if (blockindex == NULL)
    {
        if (pindexBest == NULL)
            return 1.0;
        else
            blockindex = GetLastBlockIndex(pindexBest, false);
    };

    return BitsToDouble(blockindex->nBits);
}

double GetHeaderDifficulty(const CBlockThinIndex* blockindex)
{
    if (blockindex == NULL)
    {
        if (pindexBest == NULL)
            return 1.0;
        else
            blockindex = GetLastBlockThinIndex(pindexBestHeader, false);
    };

    return BitsToDouble(blockindex->nBits);
}



double GetPoWMHashPS()
{
    if (pindexBest->nHeight >= Params().LastPOWBlock())
        return 0;

    int nPoWInterval = 72;
    int64_t nTargetSpacingWorkMin = 30, nTargetSpacingWork = 30;

    CBlockIndex* pindex = pindexGenesisBlock;
    CBlockIndex* pindexPrevWork = pindexGenesisBlock;

    while (pindex)
    {
        if (pindex->IsProofOfWork())
        {
            int64_t nActualSpacingWork = pindex->GetBlockTime() - pindexPrevWork->GetBlockTime();
            nTargetSpacingWork = ((nPoWInterval - 1) * nTargetSpacingWork + nActualSpacingWork + nActualSpacingWork) / (nPoWInterval + 1);
            nTargetSpacingWork = max(nTargetSpacingWork, nTargetSpacingWorkMin);
            pindexPrevWork = pindex;
        };

        pindex = pindex->pnext;
    };

    return GetDifficulty() * 4294.967296 / nTargetSpacingWork;
}

double GetPoSKernelPS()
{
    int nPoSInterval = 72;
    double dStakeKernelsTriedAvg = 0;
    int nStakesHandled = 0, nStakesTime = 0;
    
    if (nNodeMode == NT_THIN)
    {
        CBlockThinIndex* pindex = pindexBestHeader;;
        CBlockThinIndex* pindexPrevStake = NULL;
        
        while (pindex && nStakesHandled < nPoSInterval)
        {
            if (pindex->IsProofOfStake())
            {
                if (pindexPrevStake)
                {
                    dStakeKernelsTriedAvg += GetHeaderDifficulty(pindex) * 4294967296.0;
                    nStakesTime += pindexPrevStake ? (pindexPrevStake->nTime - pindex->nTime) : 0;
                    nStakesHandled++;
                };
                pindexPrevStake = pindex;
            };
            
            pindex = pindex->pprev;
        };
    } else
    {
        CBlockIndex* pindex = pindexBest;;
        CBlockIndex* pindexPrevStake = NULL;
        
        while (pindex && nStakesHandled < nPoSInterval)
        {
            if (pindex->IsProofOfStake())
            {
                if (pindexPrevStake)
                {
                    dStakeKernelsTriedAvg += GetDifficulty(pindex) * 4294967296.0;
                    nStakesTime += pindexPrevStake ? (pindexPrevStake->nTime - pindex->nTime) : 0;
                    nStakesHandled++;
                };
                pindexPrevStake = pindex;
            };
            
            pindex = pindex->pprev;
        };
    };
    
    
    double result = 0;
    
    if (nStakesTime)
        result = dStakeKernelsTriedAvg / nStakesTime;
    
    if (Params().IsProtocolV2(nBestHeight))
        result *= STAKE_TIMESTAMP_MASK + 1;
    
    return result;
}

Object blockToJSON(const CBlock& block, const CBlockIndex* blockindex, bool fPrintTransactionDetail)
{
    Object result;
    result.push_back(Pair("hash", block.GetHash().GetHex()));
    CMerkleTx txGen(block.vtx[0]);
    txGen.SetMerkleBranch(&block);
    result.push_back(Pair("confirmations", (int)txGen.GetDepthInMainChain()));
    result.push_back(Pair("size", (int)::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION)));
    result.push_back(Pair("height", blockindex->nHeight));
    result.push_back(Pair("version", block.nVersion));
    result.push_back(Pair("merkleroot", block.hashMerkleRoot.GetHex()));
    result.push_back(Pair("mint", ValueFromAmount(blockindex->nMint)));
    result.push_back(Pair("time", (int64_t)block.GetBlockTime()));
    result.push_back(Pair("nonce", (uint64_t)block.nNonce));
    result.push_back(Pair("bits", HexBits(block.nBits)));
    result.push_back(Pair("difficulty", GetDifficulty(blockindex)));
    result.push_back(Pair("blocktrust", leftTrim(blockindex->GetBlockTrust().GetHex(), '0')));
    result.push_back(Pair("chaintrust", leftTrim(blockindex->nChainTrust.GetHex(), '0')));
    if (blockindex->pprev)
        result.push_back(Pair("previousblockhash", blockindex->pprev->GetBlockHash().GetHex()));
    if (blockindex->pnext)
        result.push_back(Pair("nextblockhash", blockindex->pnext->GetBlockHash().GetHex()));

    result.push_back(Pair("flags", strprintf("%s%s", blockindex->IsProofOfStake()? "proof-of-stake" : "proof-of-work", blockindex->GeneratedStakeModifier()? " stake-modifier": "")));
    result.push_back(Pair("proofhash", blockindex->hashProof.GetHex()));
    result.push_back(Pair("entropybit", (int)blockindex->GetStakeEntropyBit()));
    result.push_back(Pair("modifier", strprintf("%016x", blockindex->nStakeModifier)));
    
    Array txinfo;
    BOOST_FOREACH (const CTransaction& tx, block.vtx)
    {
        if (fPrintTransactionDetail)
        {
            Object entry;

            entry.push_back(Pair("txid", tx.GetHash().GetHex()));
            TxToJSON(tx, 0, entry);

            txinfo.push_back(entry);
        } else
            txinfo.push_back(tx.GetHash().GetHex());
    };

    result.push_back(Pair("tx", txinfo));

    if (block.IsProofOfStake())
        result.push_back(Pair("signature", HexStr(block.vchBlockSig.begin(), block.vchBlockSig.end())));

    return result;
}

Object blockHeaderToJSON(const CBlockThin& block, const CBlockThinIndex* blockindex)
{
    Object result;
    result.push_back(Pair("hash", block.GetHash().GetHex()));
    //CMerkleTx txGen(block.vtx[0]);
    //txGen.SetMerkleBranch(&block);
    //result.push_back(Pair("confirmations", (int)txGen.GetDepthInMainChain()));
    result.push_back(Pair("size", (int)::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION)));
    result.push_back(Pair("height", blockindex->nHeight));
    result.push_back(Pair("version", block.nVersion));
    result.push_back(Pair("merkleroot", block.hashMerkleRoot.GetHex()));
    //result.push_back(Pair("mint", ValueFromAmount(blockindex->nMint)));
    result.push_back(Pair("time", (int64_t)block.GetBlockTime()));
    result.push_back(Pair("nonce", (uint64_t)block.nNonce));
    result.push_back(Pair("bits", HexBits(block.nBits)));
    result.push_back(Pair("difficulty", GetHeaderDifficulty(blockindex)));
    result.push_back(Pair("blocktrust", leftTrim(blockindex->GetBlockTrust().GetHex(), '0')));
    result.push_back(Pair("chaintrust", leftTrim(blockindex->nChainTrust.GetHex(), '0')));
    
    if (blockindex->pprev)
        result.push_back(Pair("previousblockhash", blockindex->pprev->GetBlockHash().GetHex()));
    if (blockindex->pnext)
        result.push_back(Pair("nextblockhash", blockindex->pnext->GetBlockHash().GetHex()));

    result.push_back(Pair("flags", strprintf("%s%s", blockindex->IsProofOfStake()? "proof-of-stake" : "proof-of-work", blockindex->GeneratedStakeModifier()? " stake-modifier": "")));
    result.push_back(Pair("proofhash", blockindex->hashProof.GetHex()));
    result.push_back(Pair("entropybit", (int)blockindex->GetStakeEntropyBit()));
    result.push_back(Pair("modifier", strprintf("%016x", blockindex->nStakeModifier)));
    
    //if (block.IsProofOfStake())
    //    result.push_back(Pair("signature", HexStr(block.vchBlockSig.begin(), block.vchBlockSig.end())));

    return result;
}


Object diskBlockThinIndexToJSON(CDiskBlockThinIndex& diskBlock)
{
    CBlock block = diskBlock.GetBlock();
    
    Object result;
    result.push_back(Pair("hash", block.GetHash().GetHex()));
    //CMerkleTx txGen(block.vtx[0]);
    //txGen.SetMerkleBranch(&block);
    //result.push_back(Pair("confirmations", (int)txGen.GetDepthInMainChain()));
    //result.push_back(Pair("size", (int)::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION)));
    result.push_back(Pair("height", diskBlock.nHeight));
    result.push_back(Pair("version", block.nVersion));
    result.push_back(Pair("merkleroot", block.hashMerkleRoot.GetHex()));
    //result.push_back(Pair("mint", ValueFromAmount(blockindex->nMint)));
    result.push_back(Pair("time", (int64_t)block.GetBlockTime()));
    result.push_back(Pair("nonce", (uint64_t)block.nNonce));
    result.push_back(Pair("bits", HexBits(block.nBits)));
    result.push_back(Pair("difficulty", BitsToDouble(diskBlock.nBits)));
    result.push_back(Pair("blocktrust", leftTrim(diskBlock.GetBlockTrust().GetHex(), '0')));
    result.push_back(Pair("chaintrust", leftTrim(diskBlock.nChainTrust.GetHex(), '0')));
    
    result.push_back(Pair("previousblockhash", diskBlock.hashPrev.GetHex()));
    result.push_back(Pair("nextblockhash", diskBlock.hashNext.GetHex()));
    

    result.push_back(Pair("flags", strprintf("%s%s", diskBlock.IsProofOfStake()? "proof-of-stake" : "proof-of-work", diskBlock.GeneratedStakeModifier()? " stake-modifier": "")));
    result.push_back(Pair("proofhash", diskBlock.hashProof.GetHex()));
    result.push_back(Pair("entropybit", (int)diskBlock.GetStakeEntropyBit()));
    result.push_back(Pair("modifier", strprintf("%016x", diskBlock.nStakeModifier)));
    //result.push_back(Pair("modifierchecksum", strprintf("%08x", diskBlock.nStakeModifierChecksum)));
    
    //if (block.IsProofOfStake())
    //    result.push_back(Pair("signature", HexStr(block.vchBlockSig.begin(), block.vchBlockSig.end())));

    return result;
}

Value getbestblockhash(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getbestblockhash\n"
            "Returns the hash of the best block in the longest block chain.");

    return hashBestChain.GetHex();
}

Value getblockcount(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getblockcount\n"
            "Returns the number of blocks in the longest block chain.");

    return nBestHeight;
}


Value getdifficulty(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getdifficulty\n"
            "Returns the difficulty as a multiple of the minimum difficulty.");

    Object obj;
    obj.push_back(Pair("proof-of-work",        GetDifficulty()));
    obj.push_back(Pair("proof-of-stake",       GetDifficulty(GetLastBlockIndex(pindexBest, true))));
    obj.push_back(Pair("search-interval",      (int)nLastCoinStakeSearchInterval));
    return obj;
}


Value settxfee(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 1 || AmountFromValue(params[0]) < MIN_TX_FEE)
        throw runtime_error(
            "settxfee <amount>\n"
            "<amount> is a real and is rounded to the nearest 0.01");

    nTransactionFee = AmountFromValue(params[0]);
    nTransactionFee = (nTransactionFee / CENT) * CENT;  // round to cent

    return true;
}

Value getrawmempool(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getrawmempool\n"
            "Returns all transaction ids in memory pool.");

    vector<uint256> vtxid;
    mempool.queryHashes(vtxid);

    Array a;
    BOOST_FOREACH(const uint256& hash, vtxid)
        a.push_back(hash.ToString());

    return a;
}

Value getblockhash(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getblockhash <index>\n"
            "Returns hash of block in best-block-chain at <index>.");

    int nHeight = params[0].get_int();
    if (nHeight < 0 || nHeight > nBestHeight)
        throw runtime_error("Block number out of range.");

    CBlockIndex* pblockindex = FindBlockByHeight(nHeight);
    return pblockindex->phashBlock->GetHex();
}

Value getblock(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "getblock <hash> [txinfo]\n"
            "txinfo optional to print more detailed tx info\n"
            "Returns details of a block with given block-hash.");

    std::string strHash = params[0].get_str();
    uint256 hash(strHash);
    
    if (nNodeMode == NT_THIN)
    {
        CDiskBlockThinIndex diskindex;
        CTxDB txdb("r");
        if (txdb.ReadBlockThinIndex(hash, diskindex))
            return diskBlockThinIndexToJSON(diskindex);
        
        throw runtime_error("Read header from db failed.\n");
    };

    if (mapBlockIndex.count(hash) == 0)
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");

    CBlock block;
    CBlockIndex* pblockindex = mapBlockIndex[hash];
    block.ReadFromDisk(pblockindex, true);

    return blockToJSON(block, pblockindex, params.size() > 1 ? params[1].get_bool() : false);
}

Value getblockbynumber(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "getblockbynumber <number> [txinfo]\n"
            "txinfo optional to print more detailed tx info\n"
            "Returns details of a block with given block-number.");

    int nHeight = params[0].get_int();
    if (nHeight < 0 || nHeight > nBestHeight)
        throw runtime_error("Block number out of range.");
    
    if (nNodeMode == NT_THIN)
    {
        if (!fThinFullIndex
            && pindexRear
            && nHeight < pindexRear->nHeight)
        {
            CDiskBlockThinIndex diskindex;
            uint256 hashPrev = pindexRear->GetBlockHash();
            
            // -- find closest checkpoint
            Checkpoints::MapCheckpoints& checkpoints = (fTestNet ? Checkpoints::mapCheckpointsTestnet : Checkpoints::mapCheckpoints);
            Checkpoints::MapCheckpoints::reverse_iterator rit;

            for (rit = checkpoints.rbegin(); rit != checkpoints.rend(); ++rit)
            {
                if (rit->first < nHeight)
                    break;
                hashPrev = rit->second;
            };
            
            CTxDB txdb("r");
            while (hashPrev != 0)
            {
                if (!txdb.ReadBlockThinIndex(hashPrev, diskindex))
                    throw runtime_error("Read header from db failed.\n");
                
                if (diskindex.nHeight == nHeightFilteredNeeded)
                    return diskBlockThinIndexToJSON(diskindex);
                
                hashPrev = diskindex.hashPrev;
            };
            
            throw runtime_error("block not found.");
        };
        
        
        CBlockThin block;
        std::map<uint256, CBlockThinIndex*>::iterator mi = mapBlockThinIndex.find(hashBestChain);
        if (mi != mapBlockThinIndex.end())
        {
            CBlockThinIndex* pblockindex = mi->second;
            while (pblockindex->pprev && pblockindex->nHeight > nHeight)
                pblockindex = pblockindex->pprev;
            
            if (nHeight != pblockindex->nHeight)
            {
                throw runtime_error("block not in chain index.");
            }
            return blockHeaderToJSON(block, pblockindex);
        } else
        {
            throw runtime_error("hashBestChain not in chain index.");
        }
        
        
    };
    
    CBlock block;
    CBlockIndex* pblockindex = mapBlockIndex[hashBestChain];
    while (pblockindex->nHeight > nHeight)
        pblockindex = pblockindex->pprev;

    uint256 hash = *pblockindex->phashBlock;

    pblockindex = mapBlockIndex[hash];
    block.ReadFromDisk(pblockindex, true);

    return blockToJSON(block, pblockindex, params.size() > 1 ? params[1].get_bool() : false);
}

Value setbestblockbyheight(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "setbestblockbyheight <height>\n"
            "Sets the tip of the chain th block at <height>.");

    int nHeight = params[0].get_int();
    if (nHeight < 0 || nHeight > nBestHeight)
        throw runtime_error("Block height out of range.");
    
    if (nNodeMode == NT_THIN)
    {
        throw runtime_error("Must be in full mode.");
    };
    
    CBlock block;
    CBlockIndex* pblockindex = mapBlockIndex[hashBestChain];
    while (pblockindex->nHeight > nHeight)
        pblockindex = pblockindex->pprev;

    uint256 hash = *pblockindex->phashBlock;

    pblockindex = mapBlockIndex[hash];
    block.ReadFromDisk(pblockindex, true);
    
    
    Object result;
    
    CTxDB txdb;
    {
        LOCK(cs_main);
        
        if (!block.SetBestChain(txdb, pblockindex))
            result.push_back(Pair("result", "failure"));
        else
            result.push_back(Pair("result", "success"));
    
    };
    
    return result;
}

Value rewindchain(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "rewindchain <number>\n"
            "Remove <number> blocks from the chain.");

    int nNumber = params[0].get_int();
    if (nNumber < 0 || nNumber > nBestHeight)
        throw runtime_error("Block number out of range.");
    
    if (nNodeMode == NT_THIN)
    {
        throw runtime_error("Must be in full mode.");
    };
    
    Object result;
    int nRemoved = 0;
    
    {
    LOCK2(cs_main, pwalletMain->cs_wallet);
    
    
    uint32_t nFileRet = 0;
    
    uint8_t buffer[512];
    
    LogPrintf("rewindchain %d\n", nNumber);
    
    void* nFind;
    
    for (int i = 0; i < nNumber; ++i)
    {
        memset(buffer, 0, sizeof(buffer));
        
        FILE* fp = AppendBlockFile(false, nFileRet, "r+b");
        if (!fp)
        {
            LogPrintf("AppendBlockFile failed.\n");
            break;
        };
        
        errno = 0;
        if (fseek(fp, 0, SEEK_END) != 0)
        {
            LogPrintf("fseek failed: %s\n", strerror(errno));
            break;
        };
        
        long int fpos = ftell(fp);
        
        if (fpos == -1)
        {
            LogPrintf("ftell failed: %s\n", strerror(errno));
            break;
        };
        
        long int foundPos = -1;
        long int readSize = sizeof(buffer) / 2;
        while (fpos > 0)
        {
            if (fpos < (long int)sizeof(buffer) / 2)
                readSize = fpos;
            
            memcpy(buffer+readSize, buffer, readSize); // move last read data (incase token crosses a boundary) 
            fpos -= readSize;
            
            if (fseek(fp, fpos, SEEK_SET) != 0)
            {
                LogPrintf("fseek failed: %s\n", strerror(errno));
                break;
            };
            
            errno = 0;
            if (fread(buffer, sizeof(uint8_t), readSize, fp) != (size_t)readSize)
            {
                if (errno != 0)
                    LogPrintf("fread failed: %s\n", strerror(errno));
                else
                    LogPrintf("End of file.\n");
                break;
            };
            
            uint32_t findPos = sizeof(buffer);
            while (findPos > MESSAGE_START_SIZE)
            {
                if ((nFind = gameunits::memrchr(buffer, Params().MessageStart()[0], findPos-MESSAGE_START_SIZE)))
                {
                    if (memcmp(nFind, Params().MessageStart(), MESSAGE_START_SIZE) == 0)
                    {
                        foundPos = ((uint8_t*)nFind - buffer) + MESSAGE_START_SIZE;
                        break;
                    } else
                    {
                        findPos = ((uint8_t*)nFind - buffer);
                        // -- step over matched char that wasn't pchMessageStart
                        if (findPos > 0) // prevent findPos < 0 (unsigned)
                            findPos--;
                    };
                } else
                {
                    break; // pchMessageStart[0] not found in buffer 
                };
            };
            
            if (foundPos > -1)
                break;
        };
        
        LogPrintf("fpos %d, foundPos %d.\n", fpos, foundPos);
        
        if (foundPos < 0)
        {
            LogPrintf("block start not found.\n");
            fclose(fp);
            break;
        };
        
        CAutoFile blkdat(fp, SER_DISK, CLIENT_VERSION);
        
        if (fseek(blkdat, fpos+foundPos, SEEK_SET) != 0)
        {
            LogPrintf("fseek blkdat failed: %s\n", strerror(errno));
            break;
        };
        
        unsigned int nSize;
        blkdat >> nSize;
        LogPrintf("nSize %u .\n", nSize);
        
        if (nSize < 1 || nSize > MAX_BLOCK_SIZE)
        {
            LogPrintf("block size error %u\n", nSize);
            
        };
    
        CBlock block;
        blkdat >> block;
        uint256 hashblock = block.GetHash();
        LogPrintf("hashblock %s .\n", hashblock.ToString().c_str());
        
        std::map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hashblock);
        if (mi != mapBlockIndex.end() && (*mi).second)
        {
            LogPrintf("block is in main chain.\n");
            
            if (!mi->second->pprev)
            {
                LogPrintf("! mi->second.pprev\n");
            } else
            {
                {
                    CBlock blockPrev; // strange way SetBestChain works, TODO: does it need the full block?
                    if (!blockPrev.ReadFromDisk(mi->second->pprev))
                    {
                        LogPrintf("blockPrev.ReadFromDisk failed %s.\n", mi->second->pprev->GetBlockHash().ToString().c_str());
                        break;
                    };
                    
                    CTxDB txdb;
                    if (!blockPrev.SetBestChain(txdb, mi->second->pprev))
                    {
                        LogPrintf("SetBestChain failed.\n");
                    };
                }
                mi->second->pprev->pnext = NULL;
            };
            
            delete mi->second;
            mapBlockIndex.erase(mi);
        };
        
        std::map<uint256, CBlock*>::iterator miOph = mapOrphanBlocks.find(hashblock);
        if (miOph != mapOrphanBlocks.end())
        {
            LogPrintf("block is an orphan.\n");
            mapOrphanBlocks.erase(miOph);
        };
        
        CTxDB txdb;
        for (vector<CTransaction>::iterator it = block.vtx.begin(); it != block.vtx.end(); ++it)
        {
            LogPrintf("EraseTxIndex().\n");
            txdb.EraseTxIndex(*it);
        };
        
        LogPrintf("EraseBlockIndex().\n");
        txdb.EraseBlockIndex(hashblock);
        
        errno = 0;
        if (ftruncate(fileno(fp), fpos+foundPos-MESSAGE_START_SIZE) != 0)
        {
            LogPrintf("ftruncate failed: %s\n", strerror(errno));
        };
        
        LogPrintf("hashBestChain %s, nBestHeight %d\n", hashBestChain.ToString().c_str(), nBestHeight);
        
        //fclose(fp); // ~CAutoFile() will close the file
        nRemoved++;
    };
    }
    
    
    result.push_back(Pair("no. blocks removed", itostr(nRemoved)));
    
    result.push_back(Pair("hashBestChain", hashBestChain.ToString()));
    result.push_back(Pair("nBestHeight", itostr(nBestHeight)));
    
    // -- need restart, setStakeSeen etc
    if (nRemoved > 0)
        result.push_back(Pair("Please restart gameunits", ""));
    
    if (nRemoved == nNumber)
        result.push_back(Pair("result", "success"));
    else
        result.push_back(Pair("result", "failure"));
    return result;
}


Value nextorphan(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "nextorphan [connecthash]\n"
            "displays orphan blocks that connect to current best block.\n"
            "if [connecthash] is set try to connect that block (must be nextorphan).\n");
    
    if (nNodeMode == NT_THIN)
    {
        throw runtime_error("Must be in full mode.");
    };
    
    if (!pindexBest)
        throw runtime_error("No best index.");
    
    throw runtime_error("Not working."); // too few blocks in mapOrphan!?
    
    Object result;
    
    std::map<uint256, CBlock*> mapNextOrphanBlocks;
    
    LOCK(cs_main);
    
    //mapOrphanBlocks.clear();
    uint256 besthash = *pindexBest->phashBlock;
    std::map<uint256, CBlock*>::iterator it;
    for (it = mapOrphanBlocks.begin(); it != mapOrphanBlocks.end(); ++it)
    {
        if (it->second->hashPrevBlock == besthash)
        {
            mapNextOrphanBlocks[it->first] = it->second;
        };
    };
    
    if (params.size() > 0)
    {
        
    } else
    {
        std::map<uint256, CBlock*>::iterator it;
        for (it = mapNextOrphanBlocks.begin(); it != mapNextOrphanBlocks.end(); ++it)
        {
            result.push_back(Pair("block", it->first.ToString()));
        };
    };
    
    result.push_back(Pair("result", "done"));
    return result;
}

// ppcoin: get information of sync-checkpoint
Value getcheckpoint(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getcheckpoint\n"
            "Show info of synchronized checkpoint.\n");

    Object result;
    CBlockIndex* pindexCheckpoint;

    result.push_back(Pair("synccheckpoint", Checkpoints::hashSyncCheckpoint.ToString().c_str()));
    pindexCheckpoint = mapBlockIndex[Checkpoints::hashSyncCheckpoint];
    result.push_back(Pair("height", pindexCheckpoint->nHeight));
    result.push_back(Pair("timestamp", DateTimeStrFormat(pindexCheckpoint->GetBlockTime()).c_str()));

    // Check that the block satisfies synchronized checkpoint
    if (CheckpointsMode == Checkpoints::STRICT)
        result.push_back(Pair("policy", "strict"));

    if (CheckpointsMode == Checkpoints::ADVISORY)
        result.push_back(Pair("policy", "advisory"));

    if (CheckpointsMode == Checkpoints::PERMISSIVE)
        result.push_back(Pair("policy", "permissive"));

    if (mapArgs.count("-checkpointkey")) // TODO: posv2 remove
        result.push_back(Pair("checkpointmaster", true));

    return result;
}



Value thinscanmerkleblocks(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "thinscanmerkleblocks <height>\n"
            "Request and rescan merkle blocks from peers starting from <height>.");

    int nHeight = params[0].get_int();
    if (nHeight < 0 || nHeight > nBestHeight)
        throw runtime_error("Block height out of range.");
    
    if (nNodeMode != NT_THIN)
        throw runtime_error("Must be in thin mode.");
    
    if (nNodeState == NS_GET_FILTERED_BLOCKS)
        throw runtime_error("Wait for current merkle block scan to complete.");
    
    {
        LOCK2(cs_main, pwalletMain->cs_wallet);
        
        pwalletMain->nLastFilteredHeight = nHeight;
        nHeightFilteredNeeded = nHeight;
        CWalletDB walletdb(pwalletMain->strWalletFile);
        walletdb.WriteLastFilteredHeight(nHeight);
        
        ChangeNodeState(NS_GET_FILTERED_BLOCKS, false);
    }
    
    Object result;
    result.push_back(Pair("result", "Success."));
    result.push_back(Pair("startheight", nHeight));
    return result;
}

Value thinforcestate(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "thinforcestate <state>\n"
            "force into state <state>.\n"
            "2 get headers, 3 get filtered blocks, 4 ready");
    
    if (nNodeMode != NT_THIN)
        throw runtime_error("Must be in thin mode.");
    
    int nState = params[0].get_int();
    if (nState <= NS_STARTUP || nState >= NS_UNKNOWN)
        throw runtime_error("unknown state.");
    
    
    
    Object result;
    if (ChangeNodeState(nState))
        result.push_back(Pair("result", "Success."));
    else
        result.push_back(Pair("result", "Failed."));
    
    return result;
}
