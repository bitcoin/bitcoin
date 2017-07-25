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
#include "unlimited.h"
#include "util.h"
#include "utilmoneystr.h"
#include "validationinterface.h"

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
        return CompareTxMemPoolEntryByScore()(*b, *a); // Convert to less than
    }
};

int64_t UpdateTime(CBlockHeader *pblock, const Consensus::Params &consensusParams, const CBlockIndex *pindexPrev)
{
    int64_t nOldTime = pblock->nTime;
    int64_t nNewTime = std::max(pindexPrev->GetMedianTimePast() + 1, GetAdjustedTime());

    if (nOldTime < nNewTime)
        pblock->nTime = nNewTime;

    // Updating time can change work required on testnet:
    if (consensusParams.fPowAllowMinDifficultyBlocks)
        pblock->nBits = GetNextWorkRequired(pindexPrev, pblock, consensusParams);

    return nNewTime - nOldTime;
}

BlockAssembler::BlockAssembler(const CChainParams &_chainparams)
    : chainparams(_chainparams), nBlockSize(0), nBlockTx(0), nBlockSigOps(0), nFees(0), nHeight(0), nLockTimeCutoff(0),
      lastFewTxs(0), blockFinished(false), buip055ChainBlock(false)
{
    // Largest block you're willing to create:
    nBlockMaxSize = maxGeneratedBlock;
    // Core:
    // nBlockMaxSize = GetArg("-blockmaxsize", DEFAULT_BLOCK_MAX_SIZE);
    // Limit to between 1K and MAX_BLOCK_SIZE-1K for sanity:
    // nBlockMaxSize = std::max((unsigned int)1000, std::min((unsigned int)(MAX_BLOCK_SIZE-1000), nBlockMaxSize));

    // Minimum block size you want to create; block will be filled with free transactions
    // until there are no more or the block reaches this size:
    nBlockMinSize = GetArg("-blockminsize", DEFAULT_BLOCK_MIN_SIZE);
    nBlockMinSize = std::min(nBlockMaxSize, nBlockMinSize);
}

void BlockAssembler::resetBlock(const CScript &scriptPubKeyIn)
{
    inBlock.clear();

    nBlockSize = reserveBlockSize(scriptPubKeyIn); // Core: 1000
    nBlockSigOps = 100; // Reserve 100 sigops for miners to use in their coinbase transaction

    // These counters do not include coinbase tx
    nBlockTx = 0;
    nFees = 0;

    lastFewTxs = 0;
    blockFinished = false;
}

uint64_t BlockAssembler::reserveBlockSize(const CScript &scriptPubKeyIn)
{
    CBlockHeader h;
    uint64_t nHeaderSize, nCoinbaseSize;

    // BU add the proper block size quantity to the actual size
    nHeaderSize = h.GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION);
    assert(nHeaderSize == 80); // BU always 80 bytes
    nHeaderSize += 5; // tx count varint - 5 bytes is enough for 4 billion txs; 3 bytes for 65535 txs

    // This serializes with output value, a fixed-length 8 byte field, of zero and height, a serialized CScript
    // signed integer taking up 4 bytes for heights 32768-8388607 (around the year 2167) after which it will use 5.
    nCoinbaseSize = coinbaseTx(scriptPubKeyIn, 400000, 0).GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION);

    // BU Miners take the block we give them, wipe away our coinbase and add their own.
    // So if their reserve choice is bigger then our coinbase then use that.
    return nHeaderSize + std::max(nCoinbaseSize, coinbaseReserve.value);
}

CMutableTransaction BlockAssembler::coinbaseTx(const CScript &scriptPubKeyIn, int nHeight, CAmount nValue)
{
    CMutableTransaction tx;

    tx.vin.resize(1);
    tx.vin[0].prevout.SetNull();
    tx.vout.resize(1);
    tx.vout[0].scriptPubKey = scriptPubKeyIn;
    tx.vout[0].nValue = nValue;
    tx.vin[0].scriptSig = CScript() << nHeight << OP_0;

    // BU005 add block size settings to the coinbase
    std::string cbmsg = FormatCoinbaseMessage(BUComments, minerComment);
    const char *cbcstr = cbmsg.c_str();
    vector<unsigned char> vec(cbcstr, cbcstr + cbmsg.size());
    COINBASE_FLAGS = CScript() << vec;
    // Chop off any extra data in the COINBASE_FLAGS so the sig does not exceed the max.
    // we can do this because the coinbase is not a "real" script...
    if (tx.vin[0].scriptSig.size() + COINBASE_FLAGS.size() > MAX_COINBASE_SCRIPTSIG_SIZE)
    {
        COINBASE_FLAGS.resize(MAX_COINBASE_SCRIPTSIG_SIZE - tx.vin[0].scriptSig.size());
    }
    tx.vin[0].scriptSig = tx.vin[0].scriptSig + COINBASE_FLAGS;

    return tx;
}

CBlockTemplate *BlockAssembler::CreateNewBlock(const CScript &scriptPubKeyIn)
{
    CBlockTemplate *tmpl = nullptr;

    if (nBlockMaxSize > BLOCKSTREAM_CORE_MAX_BLOCK_SIZE)
        tmpl = CreateNewBlock(scriptPubKeyIn, false);

    // If the block is too small we need to drop back to the 1MB ruleset
    if ((!tmpl) || (tmpl->block.nBlockSize <= BLOCKSTREAM_CORE_MAX_BLOCK_SIZE))
    {
        tmpl = CreateNewBlock(scriptPubKeyIn, true);
    }

    return tmpl;
}

CBlockTemplate *BlockAssembler::CreateNewBlock(const CScript &scriptPubKeyIn, bool blockstreamCoreCompatible)
{
    resetBlock(scriptPubKeyIn);

    // The constructed block template
    std::unique_ptr<CBlockTemplate> pblocktemplate(new CBlockTemplate());

    CBlock *pblock = &pblocktemplate->block;

    // Add dummy coinbase tx as first transaction
    pblock->vtx.push_back(CTransaction());
    pblocktemplate->vTxFees.push_back(-1); // updated at end
    pblocktemplate->vTxSigOps.push_back(-1); // updated at end

    LOCK2(cs_main, mempool.cs);
    CBlockIndex *pindexPrev = chainActive.Tip();
    nHeight = pindexPrev->nHeight + 1;

    buip055ChainBlock = pindexPrev->IsforkActiveOnNextBlock(miningForkTime.value);

    pblock->nTime = GetAdjustedTime();
    pblock->nVersion = UnlimitedComputeBlockVersion(pindexPrev, chainparams.GetConsensus(), pblock->nTime);
    // -regtest only: allow overriding block.nVersion with
    // -blockversion=N to test forking scenarios
    if (chainparams.MineBlocksOnDemand())
        pblock->nVersion = GetArg("-blockversion", pblock->nVersion);

    const int64_t nMedianTimePast = pindexPrev->GetMedianTimePast();

    nLockTimeCutoff =
        (STANDARD_LOCKTIME_VERIFY_FLAGS & LOCKTIME_MEDIAN_TIME_PAST) ? nMedianTimePast : pblock->GetBlockTime();

    addPriorityTxs(pblocktemplate.get());
    addScoreTxs(pblocktemplate.get());

    nLastBlockTx = nBlockTx;
    nLastBlockSize = nBlockSize;
    LogPrintf("CreateNewBlock(): total size %llu txs: %llu fees: %lld sigops %u\n", nBlockSize, nBlockTx, nFees,
        nBlockSigOps);

    // Create coinbase transaction.
    pblock->vtx[0] = coinbaseTx(scriptPubKeyIn, nHeight, nFees + GetBlockSubsidy(nHeight, chainparams.GetConsensus()));
    pblocktemplate->vTxFees[0] = -nFees;

    // Fill in header
    pblock->hashPrevBlock = pindexPrev->GetBlockHash();
    UpdateTime(pblock, chainparams.GetConsensus(), pindexPrev);
    pblock->nBits = GetNextWorkRequired(pindexPrev, pblock, chainparams.GetConsensus());
    pblock->nNonce = 0;
    pblocktemplate->vTxSigOps[0] = GetLegacySigOpCount(pblock->vtx[0]);

    CValidationState state;
    if (blockstreamCoreCompatible)
    {
        if (!TestConservativeBlockValidity(state, chainparams, *pblock, pindexPrev, false, false))
        {
            throw std::runtime_error(
                strprintf("%s: TestConservativeBlockValidity failed: %s", __func__, FormatStateMessage(state)));
        }
    }
    else
    {
        if (!TestBlockValidity(state, chainparams, *pblock, pindexPrev, false, false))
        {
            throw std::runtime_error(
                strprintf("%s: TestBlockValidity failed: %s", __func__, FormatStateMessage(state)));
        }
    }
    if (pblock->fExcessive)
    {
        throw std::runtime_error(strprintf("%s: Excessive block generated: %s", __func__, FormatStateMessage(state)));
    }

    return pblocktemplate.release();
}

bool BlockAssembler::isStillDependent(CTxMemPool::txiter iter)
{
    BOOST_FOREACH (CTxMemPool::txiter parent, mempool.GetMemPoolParents(iter))
    {
        if (!inBlock.count(parent))
        {
            return true;
        }
    }
    return false;
}

// Return true if incremental tx or txs in the block with the given size and sigop count would be
// valid, and false otherwise.  If false, blockFinished and lastFewTxs are updated if appropriate.
bool BlockAssembler::IsIncrementallyGood(uint64_t nExtraSize, unsigned int nExtraSigOps)
{
    if (nBlockSize + nExtraSize > nBlockMaxSize)
    {
        // If the block is so close to full that no more txs will fit
        // or if we've tried more than 50 times to fill remaining space
        // then flag that the block is finished
        if (nBlockSize > nBlockMaxSize - 100 || lastFewTxs > 50)
        {
            blockFinished = true;
            return false;
        }
        // Once we're within 1000 bytes of a full block, only look at 50 more txs
        // to try to fill the remaining space.
        if (nBlockSize > nBlockMaxSize - 1000)
        {
            lastFewTxs++;
        }
        return false;
    }

    // Enforce the "old" sigops for <= 1MB blocks
    if (nBlockSize + nExtraSize <= BLOCKSTREAM_CORE_MAX_BLOCK_SIZE)
    {
        // BU: be conservative about what is generated
        if (nBlockSigOps + nExtraSigOps >= BLOCKSTREAM_CORE_MAX_BLOCK_SIGOPS)
        {
            // BU: so a block that is near the sigops limit might be shorter than it could be if
            // the high sigops tx was backed out and other tx added.
            if (nBlockSigOps > BLOCKSTREAM_CORE_MAX_BLOCK_SIGOPS - 2)
                blockFinished = true;
            return false;
        }
    }
    else
    {
        uint64_t blockMbSize = 1 + (nBlockSize + nExtraSize - 1) / 1000000;
        if (nBlockSigOps + nExtraSigOps > blockMiningSigopsPerMb.value * blockMbSize)
        {
            if (nBlockSigOps > blockMiningSigopsPerMb.value * blockMbSize - 2)
                // very close to the limit, so the block is finished.  So a block that is near the sigops limit
                // might be shorter than it could be if the high sigops tx was backed out and other tx added.
                blockFinished = true;
            return false;
        }
    }

    return true;
}

bool BlockAssembler::TestForBlock(CTxMemPool::txiter iter)
{
    if (!IsIncrementallyGood(iter->GetTxSize(), iter->GetSigOpCount()))
        return false;

    // Must check that lock times are still valid
    // This can be removed once MTP is always enforced
    // as long as reorgs keep the mempool consistent.
    if (!IsFinalTx(iter->GetTx(), nHeight, nLockTimeCutoff))
        return false;

    return true;
}

void BlockAssembler::AddToBlock(CBlockTemplate *pblocktemplate, CTxMemPool::txiter iter)
{
    pblocktemplate->block.vtx.push_back(iter->GetTx());
    pblocktemplate->vTxFees.push_back(iter->GetFee());
    pblocktemplate->vTxSigOps.push_back(iter->GetSigOpCount());
    nBlockSize += iter->GetTxSize();
    ++nBlockTx;
    nBlockSigOps += iter->GetSigOpCount();
    nFees += iter->GetFee();
    inBlock.insert(iter);

    bool fPrintPriority = GetBoolArg("-printpriority", DEFAULT_PRINTPRIORITY);
    if (fPrintPriority)
    {
        double dPriority = iter->GetPriority(nHeight);
        CAmount dummy;
        mempool.ApplyDeltas(iter->GetTx().GetHash(), dPriority, dummy);
        LogPrintf("priority %.1f fee %s txid %s\n", dPriority,
            CFeeRate(iter->GetModifiedFee(), iter->GetTxSize()).ToString().c_str(),
            iter->GetTx().GetHash().ToString().c_str());
    }
}

void BlockAssembler::addScoreTxs(CBlockTemplate *pblocktemplate)
{
    std::priority_queue<CTxMemPool::txiter, std::vector<CTxMemPool::txiter>, ScoreCompare> clearedTxs;
    CTxMemPool::setEntries waitSet;
    CTxMemPool::indexed_transaction_set::index<mining_score>::type::iterator mi =
        mempool.mapTx.get<mining_score>().begin();
    CTxMemPool::txiter iter;
    while (!blockFinished && (mi != mempool.mapTx.get<mining_score>().end() || !clearedTxs.empty()))
    {
        // If no txs that were previously postponed are available to try
        // again, then try the next highest score tx
        if (clearedTxs.empty())
        {
            iter = mempool.mapTx.project<0>(mi);
            mi++;
        }
        // If a previously postponed tx is available to try again, then it
        // has higher score than all untried so far txs
        else
        {
            iter = clearedTxs.top();
            clearedTxs.pop();
        }

        // If tx already in block, skip  (added by addPriorityTxs)
        if (inBlock.count(iter))
        {
            continue;
        }

        // If tx is not applicable to this (forked) chain, skip it
        if (buip055ChainBlock && IsTxOpReturnInvalid(iter->GetTx()))
        {
            continue;
        }
        // Reject the tx if we are on the fork, but the tx is not fork-signed
        if (buip055ChainBlock && onlyAcceptForkSig.value && !IsTxBUIP055Only(*iter))
        {
            continue;
        }
        // if tx is not applicable to this (unforked) chain, skip it
        if (!buip055ChainBlock && IsTxBUIP055Only(*iter))
        {
            continue;
        }

        // If tx is dependent on other mempool txs which haven't yet been included
        // then put it in the waitSet
        if (isStillDependent(iter))
        {
            waitSet.insert(iter);
            continue;
        }

        // If the fee rate is below the min fee rate for mining, then we're done
        // adding txs based on score (fee rate)
        if (iter->GetModifiedFee() < ::minRelayTxFee.GetFee(iter->GetTxSize()) && nBlockSize >= nBlockMinSize)
        {
            return;
        }

        // If this tx fits in the block add it, otherwise keep looping
        if (TestForBlock(iter))
        {
            AddToBlock(pblocktemplate, iter);

            // This tx was successfully added, so
            // add transactions that depend on this one to the priority queue to try again
            BOOST_FOREACH (CTxMemPool::txiter child, mempool.GetMemPoolChildren(iter))
            {
                if (waitSet.count(child))
                {
                    clearedTxs.push(child);
                    waitSet.erase(child);
                }
            }
        }
    }
}

void BlockAssembler::addPriorityTxs(CBlockTemplate *pblocktemplate)
{
    // How much of the block should be dedicated to high-priority transactions,
    // included regardless of the fees they pay
    uint64_t nBlockPrioritySize = GetArg("-blockprioritysize", DEFAULT_BLOCK_PRIORITY_SIZE);
    nBlockPrioritySize = std::min(nBlockMaxSize, nBlockPrioritySize);

    if (nBlockPrioritySize == 0)
    {
        return;
    }

    // This vector will be sorted into a priority queue:
    vector<TxCoinAgePriority> vecPriority;
    TxCoinAgePriorityCompare pricomparer;
    std::map<CTxMemPool::txiter, double, CTxMemPool::CompareIteratorByHash> waitPriMap;
    typedef std::map<CTxMemPool::txiter, double, CTxMemPool::CompareIteratorByHash>::iterator waitPriIter;
    double actualPriority = -1;

    vecPriority.reserve(mempool.mapTx.size());
    for (CTxMemPool::indexed_transaction_set::iterator mi = mempool.mapTx.begin(); mi != mempool.mapTx.end(); ++mi)
    {
        double dPriority = mi->GetPriority(nHeight);
        CAmount dummy;
        mempool.ApplyDeltas(mi->GetTx().GetHash(), dPriority, dummy);
        vecPriority.push_back(TxCoinAgePriority(dPriority, mi));
    }
    std::make_heap(vecPriority.begin(), vecPriority.end(), pricomparer);

    CTxMemPool::txiter iter;
    while (!vecPriority.empty() && !blockFinished)
    { // add a tx from priority queue to fill the blockprioritysize
        iter = vecPriority.front().second;
        actualPriority = vecPriority.front().first;
        std::pop_heap(vecPriority.begin(), vecPriority.end(), pricomparer);
        vecPriority.pop_back();

        // If tx already in block, skip
        if (inBlock.count(iter))
        {
            DbgAssert(false, ); // shouldn't happen for priority txs
            continue;
        }

        // If tx is dependent on other mempool txs which haven't yet been included
        // then put it in the waitSet
        if (isStillDependent(iter))
        {
            waitPriMap.insert(std::make_pair(iter, actualPriority));
            continue;
        }

        // If tx is not applicable to this (forked) chain, skip it
        if (buip055ChainBlock && IsTxOpReturnInvalid(iter->GetTx()))
        {
            continue;
        }
        // Reject the tx if we are on the fork, but the tx is not fork-signed
        if (buip055ChainBlock && onlyAcceptForkSig.value && !IsTxBUIP055Only(*iter))
        {
            continue;
        }
        // if tx is not applicable to this (unforked) chain, skip it
        if (!buip055ChainBlock && IsTxBUIP055Only(*iter))
        {
            continue;
        }

        // If this tx fits in the block add it, otherwise keep looping
        if (TestForBlock(iter))
        {
            AddToBlock(pblocktemplate, iter);

            // If now that this txs is added we've surpassed our desired priority size
            // or have dropped below the AllowFreeThreshold, then we're done adding priority txs
            if (nBlockSize >= nBlockPrioritySize || !AllowFree(actualPriority))
            {
                return;
            }

            // This tx was successfully added, so
            // add transactions that depend on this one to the priority queue to try again
            BOOST_FOREACH (CTxMemPool::txiter child, mempool.GetMemPoolChildren(iter))
            {
                waitPriIter wpiter = waitPriMap.find(child);
                if (wpiter != waitPriMap.end())
                {
                    vecPriority.push_back(TxCoinAgePriority(wpiter->second, child));
                    std::push_heap(vecPriority.begin(), vecPriority.end(), pricomparer);
                    waitPriMap.erase(wpiter);
                }
            }
        }
    }
}

void IncrementExtraNonce(CBlock *pblock, unsigned int &nExtraNonce)
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
