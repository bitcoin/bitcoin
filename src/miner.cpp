// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2015-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "miner.h"

#include "amount.h"
#include "chain.h"
#include "chainparams.h"
#include "coins.h"
#include "consensus/consensus.h"
#include "consensus/merkle.h"
#include "consensus/validation.h"
#include "hash.h"
#include "main.h"
#include "net.h"
#include "parallel.h"
#include "policy/policy.h"
#include "pow.h"
#include "primitives/transaction.h"
#include "script/standard.h"
#include "timedata.h"
#include "txmempool.h"
#include "util.h"
#include "utilmoneystr.h"
#include "validationinterface.h"
#include "unlimited.h"

#include <boost/thread.hpp>
#include <boost/tuple/tuple.hpp>
#include <queue>

using namespace std;

//////////////////////////////////////////////////////////////////////////////
//
// BitcoinMiner
//

//
// Unconfirmed transactions in the memory pool often depend on other
// transactions in the memory pool. When we select transactions from the
// pool, we select by highest priority or fee rate, so we might consider
// transactions that depend on transactions that aren't yet in the block.

uint64_t nLastBlockTx = 0;
uint64_t nLastBlockSize = 0;

class ScoreCompare
{
public:
    ScoreCompare() {}

    bool operator()(const CTxMemPool::txiter a, const CTxMemPool::txiter b)
    {
        return CompareTxMemPoolEntryByScore()(*b,*a); // Convert to less than
    }
};

int64_t UpdateTime(CBlockHeader* pblock, const Consensus::Params& consensusParams, const CBlockIndex* pindexPrev)
{
    int64_t nOldTime = pblock->nTime;
    int64_t nNewTime = std::max(pindexPrev->GetMedianTimePast()+1, GetAdjustedTime());

    if (nOldTime < nNewTime)
        pblock->nTime = nNewTime;

    // Updating time can change work required on testnet:
    if (consensusParams.fPowAllowMinDifficultyBlocks)
        pblock->nBits = GetNextWorkRequired(pindexPrev, pblock, consensusParams);

    return nNewTime - nOldTime;
}

CBlockTemplate* CreateNewBlock(const CChainParams& chainparams, const CScript& scriptPubKeyIn,bool blockstreamCoreCompatible);

CBlockTemplate* CreateNewBlock(const CChainParams& chainparams, const CScript& scriptPubKeyIn)
{
  CBlockTemplate* tmpl = NULL;
  if (maxGeneratedBlock >  BLOCKSTREAM_CORE_MAX_BLOCK_SIZE)
    tmpl = CreateNewBlock(chainparams, scriptPubKeyIn, false);
  
  if ((!tmpl) || (tmpl->block.nBlockSize <= BLOCKSTREAM_CORE_MAX_BLOCK_SIZE))  // If the block is too small we need to drop back to the 1MB ruleset
    {
      tmpl = CreateNewBlock(chainparams, scriptPubKeyIn, true);
    }

  return tmpl;
}

CBlockTemplate* CreateNewBlock(const CChainParams& chainparams, const CScript& scriptPubKeyIn,bool blockstreamCoreCompatible)
{
    // Create new block
    std::unique_ptr<CBlockTemplate> pblocktemplate(new CBlockTemplate());
    if(!pblocktemplate.get())
        return NULL;
    CBlock *pblock = &pblocktemplate->block; // pointer for convenience

    // Create coinbase tx
    CMutableTransaction txNew;
    txNew.vin.resize(1);
    txNew.vin[0].prevout.SetNull();
    txNew.vout.resize(1);
    txNew.vout[0].scriptPubKey = scriptPubKeyIn;

    // Add dummy coinbase tx as first transaction
    pblock->vtx.push_back(CTransaction());
    pblocktemplate->vTxFees.push_back(-1); // updated at end
    pblocktemplate->vTxSigOps.push_back(-1); // updated at end

    // Largest block you're willing to create:
    //unsigned int nBlockMaxSize = GetArg("-blockmaxsize", DEFAULT_BLOCK_MAX_SIZE);
    // Limit to betweeen 1K and MAX_BLOCK_SIZE-1K for sanity:
    uint64_t nBlockMaxSize = maxGeneratedBlock; // std::max((unsigned int)1000, std::min((unsigned int)(maxGeneratedBlock-1000), nBlockMaxSize));

    // How much of the block should be dedicated to high-priority transactions,
    // included regardless of the fees they pay
    uint64_t nBlockPrioritySize = GetArg("-blockprioritysize", DEFAULT_BLOCK_PRIORITY_SIZE);
    nBlockPrioritySize = std::min(nBlockMaxSize, nBlockPrioritySize);

    // Minimum block size you want to create; block will be filled with free transactions
    // until there are no more or the block reaches this size:
    uint64_t nBlockMinSize = GetArg("-blockminsize", DEFAULT_BLOCK_MIN_SIZE);
    nBlockMinSize = std::min(nBlockMaxSize, nBlockMinSize);

    // Collect memory pool transactions into the block
    CTxMemPool::setEntries inBlock;
    CTxMemPool::setEntries waitSet;

    // This vector will be sorted into a priority queue:
    vector<TxCoinAgePriority> vecPriority;
    TxCoinAgePriorityCompare pricomparer;
    std::map<CTxMemPool::txiter, double, CTxMemPool::CompareIteratorByHash> waitPriMap;
    typedef std::map<CTxMemPool::txiter, double, CTxMemPool::CompareIteratorByHash>::iterator waitPriIter;
    double actualPriority = -1;

    std::priority_queue<CTxMemPool::txiter, std::vector<CTxMemPool::txiter>, ScoreCompare> clearedTxs;
    bool fPrintPriority = GetBoolArg("-printpriority", DEFAULT_PRINTPRIORITY);
    uint64_t nBlockSize = 0;  // BU add the proper block size quantity to the actual size
    {
      CBlockHeader h;
      nBlockSize += h.GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION);
    }
    assert(nBlockSize == 80);  // BU block header is always 80 bytes


    unsigned int nCoinbaseSize=0;
    // Compute coinbase transaction WITHOUT FEES just to get its size.  We will recompute this at the end when we know the fees.
    {
      txNew.vout[0].nValue = 0;  // Will be fixed below, but we need to adjust the size for a possible 9 byte varint
      txNew.vin[0].scriptSig = CScript() << ((int) 0) << CScriptNum(0);  // block height will be fixed below, but we need to adjust the size for a possible 9 byte varint

      // BU005 add block size settings to the coinbase
      std::string cbmsg = FormatCoinbaseMessage(BUComments, minerComment);
      const char* cbcstr = cbmsg.c_str();
      vector<unsigned char> vec(cbcstr, cbcstr+cbmsg.size());
      COINBASE_FLAGS = CScript() << vec;
      // Chop off any extra data in the COINBASE_FLAGS so the sig does not exceed the max.  
      // we can do this because the coinbase is not a "real" script...
      if (txNew.vin[0].scriptSig.size() + COINBASE_FLAGS.size() > MAX_COINBASE_SCRIPTSIG_SIZE)
        {
          COINBASE_FLAGS.resize(MAX_COINBASE_SCRIPTSIG_SIZE - txNew.vin[0].scriptSig.size());
        }
      txNew.vin[0].scriptSig = txNew.vin[0].scriptSig + COINBASE_FLAGS;
      nCoinbaseSize = txNew.GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION) + 16;  // This code serialized the transaction but the 2 zeros above (nValue and block height) got encoded into 2 bytes, yet these are varints so there is a possible 16 more bytes 
      // BU005 END
    }
    
    nBlockSize += std::max(nCoinbaseSize,(unsigned int) coinbaseReserve.value);  // BU Miners take the block we give them, wipe away our coinbase and add their own.  So if their reserve choice is bigger then our coinbase then use that.
    
    uint64_t nBlockTx = 0;
    unsigned int nBlockSigOps = 100;  // Reserve 100 sigops for miners to use in their coinbase transaction
    int lastFewTxs = 0;
    CAmount nFees = 0;

    {
        LOCK2(cs_main, mempool.cs);
        CBlockIndex* pindexPrev = chainActive.Tip();
        const int nHeight = pindexPrev->nHeight + 1;
        // Fill in header
        pblock->hashPrevBlock  = pindexPrev->GetBlockHash();
        pblock->nBits          = GetNextWorkRequired(pindexPrev, pblock, chainparams.GetConsensus());        
        pblock->nTime          = GetAdjustedTime();
        const int64_t nMedianTimePast = pindexPrev->GetMedianTimePast();

        pblock->nVersion = UnlimitedComputeBlockVersion(pindexPrev, chainparams.GetConsensus(),pblock->nTime);
        // -regtest only: allow overriding block.nVersion with
        // -blockversion=N to test forking scenarios
        if (chainparams.MineBlocksOnDemand())
            pblock->nVersion = GetArg("-blockversion", pblock->nVersion);

        int64_t nLockTimeCutoff = (STANDARD_LOCKTIME_VERIFY_FLAGS & LOCKTIME_MEDIAN_TIME_PAST)
                                ? nMedianTimePast
                                : pblock->GetBlockTime();

        bool fPriorityBlock = nBlockPrioritySize > 0;
        if (fPriorityBlock) {
            vecPriority.reserve(mempool.mapTx.size());
            for (CTxMemPool::indexed_transaction_set::iterator mi = mempool.mapTx.begin();
                 mi != mempool.mapTx.end(); ++mi)
            {
                double dPriority = mi->GetPriority(nHeight);
                CAmount dummy;
                mempool.ApplyDeltas(mi->GetTx().GetHash(), dPriority, dummy);
                vecPriority.push_back(TxCoinAgePriority(dPriority, mi));
            }
            std::make_heap(vecPriority.begin(), vecPriority.end(), pricomparer);
        }

        CTxMemPool::indexed_transaction_set::nth_index<3>::type::iterator mi = mempool.mapTx.get<3>().begin();
        CTxMemPool::txiter iter;

        while (mi != mempool.mapTx.get<3>().end() || !clearedTxs.empty())
        {
            bool priorityTx = false;
            if (fPriorityBlock && !vecPriority.empty()) { // add a tx from priority queue to fill the blockprioritysize
                priorityTx = true;
                iter = vecPriority.front().second;
                actualPriority = vecPriority.front().first;
                std::pop_heap(vecPriority.begin(), vecPriority.end(), pricomparer);
                vecPriority.pop_back();
            }
            else if (clearedTxs.empty()) { // add tx with next highest score
                iter = mempool.mapTx.project<0>(mi);
                mi++;
            }
            else {  // try to add a previously postponed child tx
                iter = clearedTxs.top();
                clearedTxs.pop();
            }

            if (inBlock.count(iter))
                continue; // could have been added to the priorityBlock

            const CTransaction& tx = iter->GetTx();

            bool fOrphan = false;
            BOOST_FOREACH(CTxMemPool::txiter parent, mempool.GetMemPoolParents(iter))
            {
                if (!inBlock.count(parent)) {
                    fOrphan = true;
                    break;
                }
            }
            if (fOrphan) {
                if (priorityTx)
                    waitPriMap.insert(std::make_pair(iter,actualPriority));
                else
                    waitSet.insert(iter);
                continue;
            }

            unsigned int nTxSize = iter->GetTxSize();            
            if (fPriorityBlock &&
                (nBlockSize + nTxSize >= nBlockPrioritySize || !AllowFree(actualPriority))) {
                fPriorityBlock = false;
                waitPriMap.clear();
            }
            if (!priorityTx &&
                (iter->GetModifiedFee() < ::minRelayTxFee.GetFee(nTxSize) && nBlockSize >= nBlockMinSize)) {
                break;
            }
            if (nBlockSize + nTxSize >= nBlockMaxSize) {
                if (nBlockSize >  nBlockMaxSize - 100 || lastFewTxs > 50) {
                    break;
                }
                // Once we're within 1000 bytes of a full block, only look at 50 more txs
                // to try to fill the remaining space.
                if (nBlockSize > nBlockMaxSize - 1000) {
                    lastFewTxs++;
                }
                continue;
            }

            if (!IsFinalTx(tx, nHeight, nLockTimeCutoff))
                continue;

            unsigned int nTxSigOps = iter->GetSigOpCount();
            if (nBlockSize + nTxSize <= BLOCKSTREAM_CORE_MAX_BLOCK_SIZE) // Enforce the "old" sigops for <= 1MB blocks
              {      
                if (nBlockSigOps + nTxSigOps >= BLOCKSTREAM_CORE_MAX_BLOCK_SIGOPS) {  // BU: be conservative about what is generated
                  if (nBlockSigOps > BLOCKSTREAM_CORE_MAX_BLOCK_SIGOPS - 2) {  // BU: so a block that is near the sigops limit might be shorter than it could be if the high sigops tx was backed out and other tx added.
                      break;
                  }
                  continue;
                }
              }
            else if (nBlockSize + nTxSize > BLOCKSTREAM_CORE_MAX_BLOCK_SIZE)
              {
                uint64_t blockMbSize = 1+((nBlockSize + nTxSize - 1)/1000000);
                if (nBlockSigOps + nTxSigOps > blockMiningSigopsPerMb.value*blockMbSize)
                  {
                  if (nBlockSigOps >  blockMiningSigopsPerMb.value*blockMbSize - 2)
                    {
                    break;  // very close to the limit, so the block is finished.  So a block that is near the sigops limit might be shorter than it could be if the high sigops tx was backed out and other tx added.
                    }
                  continue;  // find another TX
                  }
              }

            CAmount nTxFees = iter->GetFee();
            // Added
            pblock->vtx.push_back(tx);
            pblocktemplate->vTxFees.push_back(nTxFees);
            pblocktemplate->vTxSigOps.push_back(nTxSigOps);
            nBlockSize += nTxSize;
            ++nBlockTx;
            nBlockSigOps += nTxSigOps;
            nFees += nTxFees;

            if (fPrintPriority)
            {
                double dPriority = iter->GetPriority(nHeight);
                CAmount dummy;
                mempool.ApplyDeltas(tx.GetHash(), dPriority, dummy);
                LogPrintf("priority %.1f fee %s txid %s\n",
                          dPriority , CFeeRate(iter->GetModifiedFee(), nTxSize).ToString(), tx.GetHash().ToString());
            }

            inBlock.insert(iter);

            // Add transactions that depend on this one to the priority queue
            BOOST_FOREACH(CTxMemPool::txiter child, mempool.GetMemPoolChildren(iter))
            {
                if (fPriorityBlock) {
                    waitPriIter wpiter = waitPriMap.find(child);
                    if (wpiter != waitPriMap.end()) {
                        vecPriority.push_back(TxCoinAgePriority(wpiter->second,child));
                        std::push_heap(vecPriority.begin(), vecPriority.end(), pricomparer);
                        waitPriMap.erase(wpiter);
                    }
                }
                else {
                    if (waitSet.count(child)) {
                        clearedTxs.push(child);
                        waitSet.erase(child);
                    }
                }
            }
        }
        nLastBlockTx = nBlockTx;
        nLastBlockSize = nBlockSize;
        LogPrintf("CreateNewBlock(): total size %u txs: %u fees: %ld sigops %d\n", nBlockSize, nBlockTx, nFees, nBlockSigOps);

        // Compute final coinbase transaction.
        txNew.vout[0].nValue = nFees + GetBlockSubsidy(nHeight, chainparams.GetConsensus());
        txNew.vin[0].scriptSig = CScript() << nHeight << CScriptNum(0);

        // BU005 add block size settings to the coinbase
        std::string cbmsg = FormatCoinbaseMessage(BUComments, minerComment);
        const char* cbcstr = cbmsg.c_str();
        vector<unsigned char> vec(cbcstr, cbcstr+cbmsg.size());
        COINBASE_FLAGS = CScript() << vec;
        // Chop off any extra data in the COINBASE_FLAGS so the sig does not exceed the max.  
        // we can do this because the coinbase is not a "real" script...
        if (txNew.vin[0].scriptSig.size() + COINBASE_FLAGS.size() > MAX_COINBASE_SCRIPTSIG_SIZE)
          {
          COINBASE_FLAGS.resize(MAX_COINBASE_SCRIPTSIG_SIZE - txNew.vin[0].scriptSig.size());
          }
        txNew.vin[0].scriptSig = txNew.vin[0].scriptSig + COINBASE_FLAGS;
        // BU005 END
        
        pblock->vtx[0] = txNew;
        pblocktemplate->vTxFees[0] = -nFees;
        UpdateTime(pblock, chainparams.GetConsensus(), pindexPrev);
        pblock->nNonce         = 0;
        pblocktemplate->vTxSigOps[0] = GetLegacySigOpCount(pblock->vtx[0]);

        CValidationState state;
        if (blockstreamCoreCompatible)
          {
            if (!TestConservativeBlockValidity(state, chainparams, *pblock, pindexPrev, false, false)) {
              throw std::runtime_error(strprintf("%s: TestBlockValidity failed: %s", __func__, FormatStateMessage(state)));
            }
          }
        else
          {
            if (!TestBlockValidity(state, chainparams, *pblock, pindexPrev, false, false))
              {
                throw std::runtime_error(strprintf("%s: TestBlockValidity failed: %s", __func__, FormatStateMessage(state)));
              }
          }
        if (pblock->fExcessive)
          {
            throw std::runtime_error(strprintf("%s: Excessive block generated: %s", __func__, FormatStateMessage(state)));
          }           
    }

    return pblocktemplate.release();
}

void IncrementExtraNonce(CBlock* pblock, unsigned int& nExtraNonce)
{
    // Update nExtraNonce
    static uint256 hashPrevBlock;
    if (hashPrevBlock != pblock->hashPrevBlock)
    {
        nExtraNonce = 0;
        hashPrevBlock = pblock->hashPrevBlock;
    }
    ++nExtraNonce;
    unsigned int nHeight = pblock->GetHeight(); // Height first in coinbase required for block.version=2
    CMutableTransaction txCoinbase(pblock->vtx[0]);
 
    CScript script = (CScript() << nHeight << CScriptNum(nExtraNonce));
    if (script.size() + COINBASE_FLAGS.size() > MAX_COINBASE_SCRIPTSIG_SIZE)
      {
	COINBASE_FLAGS.resize(MAX_COINBASE_SCRIPTSIG_SIZE - script.size());
      }
    txCoinbase.vin[0].scriptSig = script + COINBASE_FLAGS;
    assert(txCoinbase.vin[0].scriptSig.size() <= MAX_COINBASE_SCRIPTSIG_SIZE);

    pblock->vtx[0] = txCoinbase;
    pblock->hashMerkleRoot = BlockMerkleRoot(*pblock);
}

