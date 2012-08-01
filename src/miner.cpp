// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "miner.h"

#include "amount.h"
#include "primitives/block.h"
#include "primitives/transaction.h"
#include "hash.h"
#include "main.h"
#include "net.h"
#include "pow.h"
#include "timedata.h"
#include "util.h"
#include "utilmoneystr.h"
#ifdef ENABLE_WALLET
#include "wallet.h"
#endif

#include <boost/thread.hpp>
#include <boost/tuple/tuple.hpp>

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
// CTxInfo represents a logical transaction to potentially be included in blocks
// It stores extra metadata such as the subjective priority of a transaction at
// the time of building the block. When there are unconfirmed transactions that
// depend on other unconfirmed transactions, these "child" transactions' CTxInfo
// object factors in its "parents" to its priority and effective size; this way,
// the "child" can cover the "cost" of its "parents", and the "parents" are
// included into the block as part of the "child".
//
class CTxInfo;
typedef std::map<uint256, CTxInfo> mapInfo_t;

class CTxInfo
{
public:
    mapInfo_t *pmapInfoById;
    const CTransaction* ptx;
    uint256 hash;
private:
    set<uint256> setDependsOn;
public:
    set<uint256> setDependents;
    double dPriority;
    CAmount nTxFee;
    int nTxSigOpsP2SHOnly;
    bool fInvalid;
    unsigned int nSize;
    unsigned int nEffectiveSizeCached;

    CTxInfo() : pmapInfoById(NULL), ptx(NULL), hash(0), dPriority(0), nTxFee(0), fInvalid(false), nSize(0), nEffectiveSizeCached(0)
    {
    }

    void addDependsOn(const uint256& hashPrev)
    {
        setDependsOn.insert(hashPrev);
        nEffectiveSizeCached = 0;
    }

    void rmDependsOn(const uint256& hashPrev)
    {
        setDependsOn.erase(hashPrev);
        nEffectiveSizeCached = 0;
    }

    // effectiveSize handles inheriting the fInvalid flag as a side effect
    unsigned int effectiveSize()
    {
        if (fInvalid)
            return -1;

        if (nEffectiveSizeCached)
            return nEffectiveSizeCached;

        assert(pmapInfoById);

        if (!nSize)
            nSize = ::GetSerializeSize(*ptx, SER_NETWORK, PROTOCOL_VERSION);
        unsigned int nEffectiveSize = nSize;
        BOOST_FOREACH(const uint256& dephash, setDependsOn)
        {
            CTxInfo& depinfo = (*pmapInfoById)[dephash];
            nEffectiveSize += depinfo.effectiveSize();

            if (depinfo.fInvalid)
            {
                fInvalid = true;
                return -1;
            }
        }
        nEffectiveSizeCached = nEffectiveSize;
        return nEffectiveSize;
    }

    unsigned int effectiveSizeMod()
    {
        unsigned int nTxSizeMod = effectiveSize();
        const CTransaction &tx = *ptx;
        // In order to avoid disincentivizing cleaning up the UTXO set we don't count
        // the constant overhead for each txin and up to 110 bytes of scriptSig (which
        // is enough to cover a compressed pubkey p2sh redemption) for priority.
        // Providing any more cleanup incentive than making additional inputs free would
        // risk encouraging people to create junk outputs to redeem later.
        BOOST_FOREACH(const CTxIn& txin, tx.vin)
        {
            unsigned int offset = 41U + min(110U, (unsigned int)txin.scriptSig.size());
            if (nTxSizeMod > offset)
                nTxSizeMod -= offset;
        }
        return nTxSizeMod;
    }

    double getPriority()
    {
        // Priority is sum(valuein * age) / modified_txsize
        return dPriority / effectiveSizeMod();
    }

    CFeeRate getFeeRate()
    {
        return CFeeRate(nTxFee, effectiveSize());
    }

    unsigned int GetLegacySigOpCount()
    {
        assert(pmapInfoById);

        unsigned int n = ::GetLegacySigOpCount(*ptx);
        BOOST_FOREACH(const uint256& dephash, setDependsOn)
        {
            CTxInfo& depinfo = (*pmapInfoById)[dephash];
            n += depinfo.GetLegacySigOpCount();
        }
        return n;
    }

    bool DoInputs(CCoinsViewCache& view, const int nHeight, std::vector<CTxInfo*>& vAdded, unsigned int& nSigOpCounter)
    {
        const CTransaction& tx = *ptx;

        if (view.HaveCoins(hash))
            // Already included in block template
            return true;

        assert(pmapInfoById);

        BOOST_FOREACH(const uint256& dephash, setDependsOn)
        {
            CTxInfo& depinfo = (*pmapInfoById)[dephash];
            if (!depinfo.DoInputs(view, nHeight, vAdded, nSigOpCounter))
                return false;
        }

        if (!view.HaveInputs(tx))
            return false;

        nTxSigOpsP2SHOnly = GetP2SHSigOpCount(tx, view);
        nSigOpCounter += nTxSigOpsP2SHOnly;

        // Note that flags: we don't want to set mempool/IsStandard()
        // policy here, but we still have to ensure that the block we
        // create only contains transactions that are valid in new blocks.
        CValidationState state;
        if (!CheckInputs(tx, state, view, true, MANDATORY_SCRIPT_VERIFY_FLAGS, true))
            return false;

        CTxUndo txundo;
        UpdateCoins(tx, state, view, txundo, nHeight);

        vAdded.push_back(this);

        return true;
    }
};

uint64_t nLastBlockTx = 0;
uint64_t nLastBlockSize = 0;

// We want to sort transactions by priority and fee, so:
typedef CTxInfo* TxPriority;
class TxPriorityCompare
{
    bool byFee;

public:
    TxPriorityCompare(bool _byFee) : byFee(_byFee) { }

    bool operator()(const TxPriority& a, const TxPriority& b)
    {
        if (byFee)
        {
            if (a->getFeeRate() == b->getFeeRate())
                return a->getPriority() < b->getPriority();
            return a->getFeeRate() < b->getFeeRate();
        }
        else
        {
            if (a->getPriority() == b->getPriority())
                return a->getFeeRate() < b->getFeeRate();
            return a->getPriority() < b->getPriority();
        }
    }
};

void UpdateTime(CBlockHeader* pblock, const CBlockIndex* pindexPrev)
{
    pblock->nTime = std::max(pindexPrev->GetMedianTimePast()+1, GetAdjustedTime());

    // Updating time can change work required on testnet:
    if (Params().AllowMinDifficultyBlocks())
        pblock->nBits = GetNextWorkRequired(pindexPrev, pblock);
}

CBlockTemplate* CreateNewBlock(const CScript& scriptPubKeyIn)
{
    // Create new block
    auto_ptr<CBlockTemplate> pblocktemplate(new CBlockTemplate());
    if(!pblocktemplate.get())
        return NULL;
    CBlock *pblock = &pblocktemplate->block; // pointer for convenience

    // -regtest only: allow overriding block.nVersion with
    // -blockversion=N to test forking scenarios
    if (Params().MineBlocksOnDemand())
        pblock->nVersion = GetArg("-blockversion", pblock->nVersion);

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
    unsigned int nBlockMaxSize = GetArg("-blockmaxsize", DEFAULT_BLOCK_MAX_SIZE);
    // Limit to betweeen 1K and MAX_BLOCK_SIZE-1K for sanity:
    nBlockMaxSize = std::max((unsigned int)1000, std::min((unsigned int)(MAX_BLOCK_SIZE-1000), nBlockMaxSize));

    // How much of the block should be dedicated to high-priority transactions,
    // included regardless of the fees they pay
    unsigned int nBlockPrioritySize = GetArg("-blockprioritysize", DEFAULT_BLOCK_PRIORITY_SIZE);
    nBlockPrioritySize = std::min(nBlockMaxSize, nBlockPrioritySize);

    // Minimum block size you want to create; block will be filled with free transactions
    // until there are no more or the block reaches this size:
    unsigned int nBlockMinSize = GetArg("-blockminsize", DEFAULT_BLOCK_MIN_SIZE);
    nBlockMinSize = std::min(nBlockMaxSize, nBlockMinSize);

    // Collect memory pool transactions into the block
    CAmount nFees = 0;

    {
        LOCK2(cs_main, mempool.cs);
        CBlockIndex* pindexPrev = chainActive.Tip();
        const int nHeight = pindexPrev->nHeight + 1;
        CCoinsViewCache view(pcoinsTip);

        // Priority order to process transactions
        mapInfo_t mapInfoById;
        bool fPrintPriority = GetBoolArg("-printpriority", false);

        // This vector will be sorted into a priority queue:
        vector<TxPriority> vecPriority;
        vecPriority.reserve(mempool.mapTx.size());

        for (map<uint256, CTxMemPoolEntry>::iterator mi = mempool.mapTx.begin();
             mi != mempool.mapTx.end(); ++mi)
        {
            const CTransaction& tx = mi->second.GetTx();

            const uint256& hash = tx.GetHash();
            CTxInfo& txinfo = mapInfoById[hash];
            txinfo.hash = hash;
            txinfo.pmapInfoById = &mapInfoById;
            txinfo.ptx = &tx;

            if (tx.IsCoinBase() || !IsFinalTx(tx, nHeight))
            {
                txinfo.fInvalid = true;
                continue;
            }

            double& dPriority = txinfo.dPriority;
            CAmount& nTxFee = txinfo.nTxFee;
            CAmount nTotalIn = 0;
            BOOST_FOREACH(const CTxIn& txin, tx.vin)
            {
                // Read prev transaction
                CAmount nValueIn;
                int nConf;
                if (view.HaveCoins(txin.prevout.hash))
                {
                    const CCoins* coins = view.AccessCoins(txin.prevout.hash);
                    assert(coins);
                    // Input is confirmed
                    nConf = nHeight - coins->nHeight;
                    nValueIn = coins->vout[txin.prevout.n].nValue;
                    dPriority += (double)nValueIn * nConf;
                }
                else
                if (mempool.mapTx.count(txin.prevout.hash))
                {
                    // Input is still unconfirmed
                    const uint256& hashPrev = txin.prevout.hash;
                    nValueIn = mempool.mapTx[hashPrev].GetTx().vout[txin.prevout.n].nValue;
                    txinfo.addDependsOn(hashPrev);
                    mapInfoById[hashPrev].setDependents.insert(hash);
                    nConf = 0;
                }
                else
                {
                    // We don't know where the input is
                    // In this case, it's impossible to include this transaction in a block, so mark it invalid and move on
                    txinfo.fInvalid = true;
                    LogPrintf("priority %s invalid input %s\n", txinfo.hash.ToString().substr(0,10).c_str(), txin.prevout.hash.ToString().substr(0,10).c_str());
                    goto nexttxn;
                }

                nTotalIn += nValueIn;
            }

            nTxFee = nTotalIn - tx.GetValueOut();

            mempool.ApplyDeltas(hash, dPriority, nTotalIn);

            vecPriority.push_back(&txinfo);

nexttxn:    (void)1;
        }

        // Collect transactions into block
        uint64_t nBlockSize = 1000;
        uint64_t nBlockTx = 0;
        int nBlockSigOps = 100;
        bool fSortedByFee = (nBlockPrioritySize <= 0);

        TxPriorityCompare comparer(fSortedByFee);
        std::make_heap(vecPriority.begin(), vecPriority.end(), comparer);

        while (!vecPriority.empty())
        {
            // Take highest priority transaction off the priority queue:
            CTxInfo& txinfo = *(vecPriority.front());
            std::pop_heap(vecPriority.begin(), vecPriority.end(), comparer);
            vecPriority.pop_back();

            if (txinfo.fInvalid)
                continue;

            const CTransaction& tx = *txinfo.ptx;
            double dPriority = txinfo.getPriority();
            CFeeRate feeRate = txinfo.getFeeRate();

            // Size limits
            unsigned int nTxSize = txinfo.effectiveSize();
            if (nBlockSize + nTxSize >= nBlockMaxSize)
                continue;

            // Legacy limits on sigOps:
            unsigned int nTxSigOps = txinfo.GetLegacySigOpCount();
            if (nBlockSigOps + nTxSigOps >= MAX_BLOCK_SIGOPS)
                continue;

            // Skip free transactions if we're past the minimum block size:
            const uint256& hash = tx.GetHash();
            double dPriorityDelta = 0;
            CAmount nFeeDelta = 0;
            mempool.ApplyDeltas(hash, dPriorityDelta, nFeeDelta);
            if (fSortedByFee && (dPriorityDelta <= 0) && (nFeeDelta <= 0) && (feeRate < ::minRelayTxFee) && (nBlockSize + nTxSize >= nBlockMinSize))
                continue;

            // Prioritise by fee once past the priority size or we run out of high-priority
            // transactions:
            if (!fSortedByFee &&
                ((nBlockSize + nTxSize >= nBlockPrioritySize) || !AllowFree(dPriority)))
            {
                fSortedByFee = true;
                comparer = TxPriorityCompare(fSortedByFee);
                std::make_heap(vecPriority.begin(), vecPriority.end(), comparer);
            }

            // second layer cached modifications just for this transaction
            CCoinsViewCache viewTemp(&view);

            std::vector<CTxInfo*> vAdded;
            if (!txinfo.DoInputs(viewTemp, nHeight, vAdded, nTxSigOps))
                continue;

            if (nBlockSigOps + nTxSigOps >= MAX_BLOCK_SIGOPS)
                continue;

            // push changes from the second layer cache to the first one
            viewTemp.Flush();

            // Added
            nBlockSize += nTxSize;
            nBlockTx += vAdded.size();
            nBlockSigOps += nTxSigOps;

            if (fPrintPriority)
            {
                LogPrintf("priority %.1f fee %s txid %s\n",
                    dPriority, feeRate.ToString(), tx.GetHash().ToString());
            }

            bool fResort = false;
            BOOST_FOREACH(CTxInfo* ptxinfo, vAdded)
            {
                pblock->vtx.push_back(*ptxinfo->ptx);
                pblocktemplate->vTxFees.push_back(ptxinfo->nTxFee);
                pblocktemplate->vTxSigOps.push_back(GetLegacySigOpCount(*ptxinfo->ptx) + ptxinfo->nTxSigOpsP2SHOnly);
                nFees += ptxinfo->nTxFee;

                ptxinfo->fInvalid = true;
                if (!ptxinfo->setDependents.empty())
                {
                    fResort = true;
                    BOOST_FOREACH(const uint256& dhash, ptxinfo->setDependents)
                    {
                        CTxInfo& dtxinfo = mapInfoById[dhash];
                        dtxinfo.rmDependsOn(ptxinfo->hash);
                    }
                }
            }
            if (fResort)
                // Re-sort the priority queue to pick up on improved standing
                std::make_heap(vecPriority.begin(), vecPriority.end(), comparer);
        }

        nLastBlockTx = nBlockTx;
        nLastBlockSize = nBlockSize;
        LogPrintf("CreateNewBlock(): total size %u\n", nBlockSize);

        // Compute final coinbase transaction.
        txNew.vout[0].nValue = GetBlockValue(nHeight, nFees);
        txNew.vin[0].scriptSig = CScript() << nHeight << OP_0;
        pblock->vtx[0] = txNew;
        pblocktemplate->vTxFees[0] = -nFees;

        // Fill in header
        pblock->hashPrevBlock  = pindexPrev->GetBlockHash();
        UpdateTime(pblock, pindexPrev);
        pblock->nBits          = GetNextWorkRequired(pindexPrev, pblock);
        pblock->nNonce         = 0;
        pblocktemplate->vTxSigOps[0] = GetLegacySigOpCount(pblock->vtx[0]);

        CValidationState state;
        if (!TestBlockValidity(state, *pblock, pindexPrev, false, false))
            throw std::runtime_error("CreateNewBlock() : TestBlockValidity failed");
    }

    return pblocktemplate.release();
}

void IncrementExtraNonce(CBlock* pblock, CBlockIndex* pindexPrev, unsigned int& nExtraNonce)
{
    // Update nExtraNonce
    static uint256 hashPrevBlock;
    if (hashPrevBlock != pblock->hashPrevBlock)
    {
        nExtraNonce = 0;
        hashPrevBlock = pblock->hashPrevBlock;
    }
    ++nExtraNonce;
    unsigned int nHeight = pindexPrev->nHeight+1; // Height first in coinbase required for block.version=2
    CMutableTransaction txCoinbase(pblock->vtx[0]);
    txCoinbase.vin[0].scriptSig = (CScript() << nHeight << CScriptNum(nExtraNonce)) + COINBASE_FLAGS;
    assert(txCoinbase.vin[0].scriptSig.size() <= 100);

    pblock->vtx[0] = txCoinbase;
    pblock->hashMerkleRoot = pblock->BuildMerkleTree();
}

#ifdef ENABLE_WALLET
//////////////////////////////////////////////////////////////////////////////
//
// Internal miner
//
double dHashesPerSec = 0.0;
int64_t nHPSTimerStart = 0;

//
// ScanHash scans nonces looking for a hash with at least some zero bits.
// The nonce is usually preserved between calls, but periodically or if the
// nonce is 0xffff0000 or above, the block is rebuilt and nNonce starts over at
// zero.
//
bool static ScanHash(const CBlockHeader *pblock, uint32_t& nNonce, uint256 *phash)
{
    // Write the first 76 bytes of the block header to a double-SHA256 state.
    CHash256 hasher;
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << *pblock;
    assert(ss.size() == 80);
    hasher.Write((unsigned char*)&ss[0], 76);

    while (true) {
        nNonce++;

        // Write the last 4 bytes of the block header (the nonce) to a copy of
        // the double-SHA256 state, and compute the result.
        CHash256(hasher).Write((unsigned char*)&nNonce, 4).Finalize((unsigned char*)phash);

        // Return the nonce if the hash has at least some zero bits,
        // caller will check if it has enough to reach the target
        if (((uint16_t*)phash)[15] == 0)
            return true;

        // If nothing found after trying for a while, return -1
        if ((nNonce & 0xffff) == 0)
            return false;
        if ((nNonce & 0xfff) == 0)
            boost::this_thread::interruption_point();
    }
}

CBlockTemplate* CreateNewBlockWithKey(CReserveKey& reservekey)
{
    CPubKey pubkey;
    if (!reservekey.GetReservedKey(pubkey))
        return NULL;

    CScript scriptPubKey = CScript() << ToByteVector(pubkey) << OP_CHECKSIG;
    return CreateNewBlock(scriptPubKey);
}

bool ProcessBlockFound(CBlock* pblock, CWallet& wallet, CReserveKey& reservekey)
{
    LogPrintf("%s\n", pblock->ToString());
    LogPrintf("generated %s\n", FormatMoney(pblock->vtx[0].vout[0].nValue));

    // Found a solution
    {
        LOCK(cs_main);
        if (pblock->hashPrevBlock != chainActive.Tip()->GetBlockHash())
            return error("BitcoinMiner : generated block is stale");
    }

    // Remove key from key pool
    reservekey.KeepKey();

    // Track how many getdata requests this block gets
    {
        LOCK(wallet.cs_wallet);
        wallet.mapRequestCount[pblock->GetHash()] = 0;
    }

    // Process this block the same as if we had received it from another node
    CValidationState state;
    if (!ProcessNewBlock(state, NULL, pblock))
        return error("BitcoinMiner : ProcessNewBlock, block not accepted");

    return true;
}

void static BitcoinMiner(CWallet *pwallet)
{
    LogPrintf("BitcoinMiner started\n");
    SetThreadPriority(THREAD_PRIORITY_LOWEST);
    RenameThread("bitcoin-miner");

    // Each thread has its own key and counter
    CReserveKey reservekey(pwallet);
    unsigned int nExtraNonce = 0;

    try {
        while (true) {
            if (Params().MiningRequiresPeers()) {
                // Busy-wait for the network to come online so we don't waste time mining
                // on an obsolete chain. In regtest mode we expect to fly solo.
                while (vNodes.empty())
                    MilliSleep(1000);
            }

            //
            // Create new block
            //
            unsigned int nTransactionsUpdatedLast = mempool.GetTransactionsUpdated();
            CBlockIndex* pindexPrev = chainActive.Tip();

            auto_ptr<CBlockTemplate> pblocktemplate(CreateNewBlockWithKey(reservekey));
            if (!pblocktemplate.get())
            {
                LogPrintf("Error in BitcoinMiner: Keypool ran out, please call keypoolrefill before restarting the mining thread\n");
                return;
            }
            CBlock *pblock = &pblocktemplate->block;
            IncrementExtraNonce(pblock, pindexPrev, nExtraNonce);

            LogPrintf("Running BitcoinMiner with %u transactions in block (%u bytes)\n", pblock->vtx.size(),
                ::GetSerializeSize(*pblock, SER_NETWORK, PROTOCOL_VERSION));

            //
            // Search
            //
            int64_t nStart = GetTime();
            uint256 hashTarget = uint256().SetCompact(pblock->nBits);
            uint256 hash;
            uint32_t nNonce = 0;
            uint32_t nOldNonce = 0;
            while (true) {
                bool fFound = ScanHash(pblock, nNonce, &hash);
                uint32_t nHashesDone = nNonce - nOldNonce;
                nOldNonce = nNonce;

                // Check if something found
                if (fFound)
                {
                    if (hash <= hashTarget)
                    {
                        // Found a solution
                        pblock->nNonce = nNonce;
                        assert(hash == pblock->GetHash());

                        SetThreadPriority(THREAD_PRIORITY_NORMAL);
                        LogPrintf("BitcoinMiner:\n");
                        LogPrintf("proof-of-work found  \n  hash: %s  \ntarget: %s\n", hash.GetHex(), hashTarget.GetHex());
                        ProcessBlockFound(pblock, *pwallet, reservekey);
                        SetThreadPriority(THREAD_PRIORITY_LOWEST);

                        // In regression test mode, stop mining after a block is found.
                        if (Params().MineBlocksOnDemand())
                            throw boost::thread_interrupted();

                        break;
                    }
                }

                // Meter hashes/sec
                static int64_t nHashCounter;
                if (nHPSTimerStart == 0)
                {
                    nHPSTimerStart = GetTimeMillis();
                    nHashCounter = 0;
                }
                else
                    nHashCounter += nHashesDone;
                if (GetTimeMillis() - nHPSTimerStart > 4000)
                {
                    static CCriticalSection cs;
                    {
                        LOCK(cs);
                        if (GetTimeMillis() - nHPSTimerStart > 4000)
                        {
                            dHashesPerSec = 1000.0 * nHashCounter / (GetTimeMillis() - nHPSTimerStart);
                            nHPSTimerStart = GetTimeMillis();
                            nHashCounter = 0;
                            static int64_t nLogTime;
                            if (GetTime() - nLogTime > 30 * 60)
                            {
                                nLogTime = GetTime();
                                LogPrintf("hashmeter %6.0f khash/s\n", dHashesPerSec/1000.0);
                            }
                        }
                    }
                }

                // Check for stop or if block needs to be rebuilt
                boost::this_thread::interruption_point();
                // Regtest mode doesn't require peers
                if (vNodes.empty() && Params().MiningRequiresPeers())
                    break;
                if (nNonce >= 0xffff0000)
                    break;
                if (mempool.GetTransactionsUpdated() != nTransactionsUpdatedLast && GetTime() - nStart > 60)
                    break;
                if (pindexPrev != chainActive.Tip())
                    break;

                // Update nTime every few seconds
                UpdateTime(pblock, pindexPrev);
                if (Params().AllowMinDifficultyBlocks())
                {
                    // Changing pblock->nTime can change work required on testnet:
                    hashTarget.SetCompact(pblock->nBits);
                }
            }
        }
    }
    catch (boost::thread_interrupted)
    {
        LogPrintf("BitcoinMiner terminated\n");
        throw;
    }
}

void GenerateBitcoins(bool fGenerate, CWallet* pwallet, int nThreads)
{
    static boost::thread_group* minerThreads = NULL;

    if (nThreads < 0) {
        // In regtest threads defaults to 1
        if (Params().DefaultMinerThreads())
            nThreads = Params().DefaultMinerThreads();
        else
            nThreads = boost::thread::hardware_concurrency();
    }

    if (minerThreads != NULL)
    {
        minerThreads->interrupt_all();
        delete minerThreads;
        minerThreads = NULL;
    }

    if (nThreads == 0 || !fGenerate)
        return;

    minerThreads = new boost::thread_group();
    for (int i = 0; i < nThreads; i++)
        minerThreads->create_thread(boost::bind(&BitcoinMiner, pwallet));
}

#endif // ENABLE_WALLET
