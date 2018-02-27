// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Liberta Core developers
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
#include "init.h"
#include "main.h"
#include "masternode-sync.h"
#include "net.h"
#include "policy/policy.h"
#include "pos.h"
#include "pow.h"
#include "primitives/transaction.h"
#include "script/standard.h"
#include "timedata.h"
#include "txmempool.h"
#include "util.h"
#include "utilmoneystr.h"
#include "validationinterface.h"
#ifdef ENABLE_WALLET
#include "wallet/db.h"
#include "wallet/wallet.h"
#include "wallet/walletdb.h"
#endif
#include "masternode-payments.h"
#include <boost/thread.hpp>
#include <boost/tuple/tuple.hpp>
#include <queue>

using namespace std;

std::string convertAddress(const char address[], char newVersionByte){
    std::vector<unsigned char> v;
    DecodeBase58Check(address,v);
    v[0]=newVersionByte;
    string result = EncodeBase58Check(v);
    return result;
}
//////////////////////////////////////////////////////////////////////////////
//
// LibertaMiner
//

//
// Unconfirmed transactions in the memory pool often depend on other
// transactions in the memory pool. When we select transactions from the
// pool, we select by highest priority or fee rate, so we might consider
// transactions that depend on transactions that aren't yet in the block.

uint64_t nLastBlockTx = 0;
uint64_t nLastBlockSize = 0;
int64_t nLastCoinStakeSearchInterval = 0;

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
        pblock->nBits = GetNextWorkRequired(pindexPrev, pblock, consensusParams, false);

    return nNewTime - nOldTime;
}

inline CMutableTransaction CreateCoinbaseTransaction(const CScript& scriptPubKeyIn, CAmount nFees, const int nHeight, bool fProofOfStake)
{
    // Create and Compute final coinbase transaction.
    CAmount reward = nFees+ GetProofOfWorkSubsidy(chainActive.Tip()->nHeight+ 1, Params().GetConsensus());
    CAmount devsubsidy = reward *0.1;

    CMutableTransaction txNew;
    txNew.vin.resize(1);
    txNew.vin[0].prevout.SetNull();
    txNew.vin[0].scriptSig = CScript() << nHeight << OP_0;
    txNew.nTime = GetAdjustedTime();

    if (fProofOfStake)
    {
        txNew.vout.resize(1);
        txNew.vout[0].SetEmpty();
        return txNew;
    }
    else
    {
        txNew.vout.resize(2);
        txNew.vout[0].nValue = reward - devsubsidy;
        txNew.vout[0].scriptPubKey = scriptPubKeyIn;
        txNew.vout[1].nValue = devsubsidy;
        txNew.vout[1].scriptPubKey = CScript() << ParseHex("02f391f21dd01129757e2bb37318309c4453ecbbeaed6bb15b97d2f800e888058b") << OP_CHECKSIG;;
    }

    //Masternode and general budget payments
    if (!fProofOfStake)
        FillBlockPayee(txNew, 0, fProofOfStake);

    return txNew;
}

CBlockTemplate* CreateNewBlock(const CChainParams& chainparams, const CScript& scriptPubKeyIn, CWallet* pwallet, bool fProofOfStake)
{

    // Create new block
    unique_ptr<CBlockTemplate> pblocktemplate(new CBlockTemplate());
    if(!pblocktemplate.get())
        return NULL;
    CBlock *pblock = &pblocktemplate->block; // pointer for convenience

    // Add dummy coinbase tx as first transaction
    pblock->vtx.push_back(CTransaction());
    pblocktemplate->vTxFees.push_back(-1); // updated at end
    pblocktemplate->vTxSigOps.push_back(-1); // updated at end

    // Largest block you're willing to create:
    unsigned int nBlockMaxSize = GetArg("-blockmaxsize", DEFAULT_BLOCK_MAX_SIZE);
    // Limit to between 1K and MAX_BLOCK_SIZE-1K for sanity:
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
    uint64_t nBlockSize = 1000;
    uint64_t nBlockTx = 0;
    unsigned int nBlockSigOps = 100;
    int lastFewTxs = 0;
    CAmount nFees = 0;

    {
        LOCK2(cs_main, mempool.cs);
        CBlockIndex* pindexPrev = chainActive.Tip();
        const int nHeight = pindexPrev->nHeight + 1;
        pblock->nTime = GetAdjustedTime();
        const int64_t nMedianTimePast = pindexPrev->GetMedianTimePast();

        pblock->nVersion =  1; //ComputeBlockVersion(pindexPrev, chainparams.GetConsensus());
        // -regtest only: allow overriding block.nVersion with
        // -blockversion=N to test forking scenarios
        if (chainparams.MineBlocksOnDemand())
            pblock->nVersion = GetArg("-blockversion", pblock->nVersion);

        int64_t nLockTimeCutoff = (STANDARD_LOCKTIME_VERIFY_FLAGS & LOCKTIME_MEDIAN_TIME_PAST) ? nMedianTimePast : pblock->GetBlockTime();

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

            if (tx.IsCoinStake() || !IsFinalTx(tx, nHeight, nLockTimeCutoff)|| pblock->GetBlockTime() < (int64_t)tx.nTime)
                continue;

            unsigned int nTxSigOps = iter->GetSigOpCount();
            if (nBlockSigOps + nTxSigOps >= MAX_BLOCK_SIGOPS) {
                if (nBlockSigOps > MAX_BLOCK_SIGOPS - 2) {
                    break;
                }
                continue;
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

        if (fDebug) LogPrintf("CreateNewBlock(): total size %u txs: %u fees: %ld sigops %d\n", nBlockSize, nBlockTx, nFees, nBlockSigOps);

        pblock->vtx[0] = CreateCoinbaseTransaction(scriptPubKeyIn, nFees, nHeight, fProofOfStake);

        //Make payee
        if (!fProofOfStake && pblock->vtx[0].vout.size() > 1) {
            int size = pblock->vtx[0].vout.size();
            pblock->payee = pblock->vtx[0].vout[size-1].scriptPubKey;
        }

        pblocktemplate->vTxFees[0] = -nFees;

        // Fill in header
        pblock->hashPrevBlock  = pindexPrev->GetBlockHash();
        if (!fProofOfStake)
            UpdateTime(pblock, chainparams.GetConsensus(), pindexPrev);

        pblock->nBits          = GetNextWorkRequired(pindexPrev, pblock, chainparams.GetConsensus(), fProofOfStake);
        pblock->nNonce         = 0;
        pblocktemplate->vTxSigOps[0] = GetLegacySigOpCount(pblock->vtx[0]);

        CValidationState state;
        if (!fProofOfStake && !TestBlockValidity(state, chainparams, *pblock, pindexPrev, false, false)) {
            throw std::runtime_error(strprintf("%s: TestBlockValidity failed: %s", __func__, FormatStateMessage(state)));
        }
    }

    return pblocktemplate.release();
}

void IncrementExtraNonce(CBlock* pblock, const CBlockIndex* pindexPrev, unsigned int& nExtraNonce)
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
    pblock->hashMerkleRoot = BlockMerkleRoot(*pblock);
}

//////////////////////////////////////////////////////////////////////////////
//
// Internal miner
//
double dHashesPerMin = 0.0;
int64_t nHPSTimerStart = 0;

//
bool ProcessBlockFound(const CBlock* pblock, const CChainParams& chainparams)
{

    CValidationState state;

    // Found a solution
    {
        LOCK(cs_main);
        if (pblock->hashPrevBlock != chainActive.Tip()->GetBlockHash())
            return error("ProcessBlockFound(): generated/staked block is stale");
    }

    if (fDebug)LogPrintf("%s \n ", pblock->ToString());

    // verify hash target and signature of coinstake tx
    if (pblock->IsProofOfStake() && !CheckProofOfStake(mapBlockIndex[pblock->hashPrevBlock], pblock->vtx[1], pblock->nBits, state))
        return false;
   
    LogPrintf("%s %s\n", pblock->IsProofOfStake() ? "Stake " : "Mined " , pblock->IsProofOfStake() ? FormatMoney(pblock->vtx[1].GetValueOut()) : FormatMoney(pblock->vtx[0].GetValueOut()));

    // Inform about the new block
    GetMainSignals().BlockFound(pblock->GetHash());

    // Process this block the same as if we had received it from another node
      if (!ProcessNewBlock(state, chainparams, NULL, pblock, true, NULL))
        return error("LibertaMiner: ProcessNewBlock, block not accepted");

    return true;
}

void ThreadStakeMiner(CWallet* pwallet)
{

    LogPrintf("StakeMiner started\n");
    SetThreadPriority(THREAD_PRIORITY_LOWEST);
    RenameThread("stake-miner");

    const CChainParams& chainparams = Params();
    boost::shared_ptr<CReserveScript> coinstakeScript;
    GetMainSignals().ScriptForMining(coinstakeScript);

    if (!coinstakeScript || coinstakeScript->reserveScript.empty())
        throw std::runtime_error("No coinstake script available (staking requires a wallet)");

    bool fTryToSync = true;

    while (true)
    {
    
        while (pwallet->IsLocked())
        {
          //  nLastCoinStakeSearchInterval = 0;
            MilliSleep(1000);
        }

        while (vNodes.empty() || IsInitialBlockDownload())
        {
			fTryToSync = true;
           // nLastCoinStakeSearchInterval = 0;
            MilliSleep(1000);
        }

		if (fTryToSync)
		{
			fTryToSync = false;
			if (vNodes.size() < 6 || pindexBestHeader->GetBlockTime() < GetTime() - 10 * 60)
			{
				MilliSleep(60000);
				continue;
			}
		}

        //
        // Create new block
        //
        unique_ptr<CBlockTemplate> pblocktemplate(CreateNewBlock(chainparams, coinstakeScript->reserveScript, pwallet, true));
        if (!pblocktemplate.get())
             return;

        CBlock *pblock = &pblocktemplate->block;
        if(SignBlock(pwallet, pblock))
        {
            SetThreadPriority(THREAD_PRIORITY_NORMAL);
            ProcessBlockFound(pblock, chainparams);
            SetThreadPriority(THREAD_PRIORITY_LOWEST);
            MilliSleep(500);
        }
        else
            MilliSleep(500);
    }
}

// attempt to generate suitable proof-of-stake
bool SignBlock(CWallet* pwallet, CBlock* pblock)
{
    
    // if we are trying to sign
    //    something except proof-of-stake block template
    if (!pblock->vtx[0].vout[0].IsEmpty()){
    	LogPrintf("something except proof-of-stake block\n");
    	return false;
    }

    // if we are trying to sign
    //    a complete proof-of-stake block
    if (pblock->IsProofOfStake()){
    	LogPrintf("trying to sign a complete proof-of-stake block\n");
    	return true;
    }

    static int64_t nLastCoinStakeSearchTime = GetAdjustedTime(); // startup timestamp

    CKey key;
    CMutableTransaction txCoinStake;
    txCoinStake.nTime = GetAdjustedTime();
    txCoinStake.nTime &= ~STAKE_TIMESTAMP_MASK;
    CAmount nFees = 0;

    int64_t nSearchTime = txCoinStake.nTime; // search to current time

    //LogPrintf("SearchTime = %d \n", nSearchTime);
    //LogPrintf("nLastCoinStakeSearchTime = %d \n", nLastCoinStakeSearchTime);

    if (nSearchTime >= nLastCoinStakeSearchTime)
    {
        int64_t nSearchInterval =  1 ;

        if (pwallet->CreateCoinStake(*pwallet, pblock->nBits, nSearchInterval, nFees, txCoinStake, key))
        {
            if (txCoinStake.nTime >= pindexBestHeader->GetMedianTimePast()+1)
            {
                // make sure coinstake would meet timestamp protocol
                //    as it would be the same as the block timestamp
                pblock->nTime = txCoinStake.nTime = pblock->vtx[0].nTime;

                // we have to make sure that we have no future timestamps in
                //    our transactions set
                for (vector<CTransaction>::iterator it = pblock->vtx.begin(); it != pblock->vtx.end();)
                    if (it->nTime > pblock->nTime) { it = pblock->vtx.erase(it); } else { ++it; }

                pblock->vtx.insert(pblock->vtx.begin() + 1, txCoinStake);
                pblock->hashMerkleRoot = BlockMerkleRoot(*pblock);

                // append a signature to our block
                return key.Sign(pblock->GetHash(), pblock->vchBlockSig);
            }
        }
        nLastCoinStakeSearchInterval = nSearchTime - nLastCoinStakeSearchTime;
        nLastCoinStakeSearchTime = nSearchTime;
    }
    return false;
}

void static LibertaMiner(const CChainParams& chainparams)
{
    LogPrintf("LibertaMiner started\n");
    SetThreadPriority(THREAD_PRIORITY_LOWEST);
    RenameThread("liberta-miner");

    unsigned int nExtraNonce = 0;

    boost::shared_ptr<CReserveScript> coinbaseScript;
    GetMainSignals().ScriptForMining(coinbaseScript);

    try {
        // Throw an error if no script was provided.  This can happen
        // due to some internal error but also if the keypool is empty.
        // In the latter case, already the pointer is NULL.
        if (!coinbaseScript || coinbaseScript->reserveScript.empty())
            throw std::runtime_error("No coinbase script available (mining requires a wallet)");

        while (true) {
            if (chainparams.MiningRequiresPeers()) {
                // Busy-wait for the network to come online so we don't waste time mining
                // on an obsolete chain. In regtest mode we expect to fly solo.
                do {
                    bool fvNodesEmpty;
                    {
                        LOCK(cs_vNodes);
                        fvNodesEmpty = vNodes.empty();
                    }
                    if (!fvNodesEmpty && !IsInitialBlockDownload())
                        break;
                    MilliSleep(1000);
                } while (true);
            }

            //
            // Create new block
            //
            unsigned int nTransactionsUpdatedLast = mempool.GetTransactionsUpdated();
            CBlockIndex* pindexPrev = chainActive.Tip();

            unique_ptr<CBlockTemplate> pblocktemplate(CreateNewBlock(chainparams, coinbaseScript->reserveScript, NULL, false));
            if (!pblocktemplate.get())
            {
                LogPrintf("Error in LibertaMiner: Keypool ran out, please call keypoolrefill before restarting the mining thread\n");
                return;
            }
            CBlock *pblock = &pblocktemplate->block;
            IncrementExtraNonce(pblock, pindexPrev, nExtraNonce);

            LogPrintf("Running LibertaMiner with %u transactions in block (%u bytes)\n", pblock->vtx.size(),
                ::GetSerializeSize(*pblock, SER_NETWORK, PROTOCOL_VERSION));

            //
            // Search
            //
            int64_t nStart = GetTime();
            arith_uint256 hashTarget = arith_uint256().SetCompact(pblock->nBits);
            uint256 testHash;
            for (;;)
            {
                unsigned int nHashesDone = 0;
                unsigned int nNonceFound = (unsigned int) -1;

                for(int i=0;i<1;i++){
                    pblock->nNonce=pblock->nNonce+1;
		    nHashesDone++;
                    if (fDebug) LogPrintf("testHash %s\n", testHash.ToString().c_str());
                    if (fDebug) LogPrintf("Hash Target %s\n", hashTarget.ToString().c_str());

                    if(UintToArith256(testHash)<hashTarget){
                        // Found a solution
                        nNonceFound=pblock->nNonce;
                        LogPrintf("Found Hash %s\n", testHash.ToString().c_str());
                        LogPrintf("hash2 %s\n", pblock->GetHash().ToString().c_str());
                        // Found a solution
                        assert(testHash == pblock->GetHash());

                        SetThreadPriority(THREAD_PRIORITY_NORMAL);
                        ProcessBlockFound(pblock, chainparams);
                        SetThreadPriority(THREAD_PRIORITY_LOWEST);
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
				if (GetTimeMillis() - nHPSTimerStart > 4000*60)
				{
					static CCriticalSection cs;
					{
						LOCK(cs);
						if (GetTimeMillis() - nHPSTimerStart > 4000*60)
						{
							dHashesPerMin = 1000.0 * nHashCounter *60 / (GetTimeMillis() - nHPSTimerStart);
							nHPSTimerStart = GetTimeMillis();
							nHashCounter = 0;
							static int64_t nLogTime;
							if (GetTime() - nLogTime > 30 * 60)
							{
								nLogTime = GetTime();
								LogPrintf("hashmeter %6.0f khash/s\n", dHashesPerMin/1000.0);
							}
						}
					}
				}

				// Check for stop or if block needs to be rebuilt
				boost::this_thread::interruption_point();
				// Regtest mode doesn't require peers
				if (vNodes.empty() && Params().MiningRequiresPeers())
					break;
				if (nNonceFound >= 0xffff0000)
					break;
				if (mempool.GetTransactionsUpdated() != nTransactionsUpdatedLast && GetTime() - nStart > 60)
					break;
				if (pindexPrev != chainActive.Tip())
					break;

				// Update nTime every few seconds
				UpdateTime(pblock, chainparams.GetConsensus(), pindexPrev);
            }
        }
    }
    catch (const boost::thread_interrupted&)
    {
        LogPrintf("LibertaMiner terminated\n");
        throw;
    }
    catch (const std::runtime_error &e)
    {
        LogPrintf("LibertaMiner runtime error: %s\n", e.what());
        return;
    }
}

void GenerateLibertas(bool fGenerate, int nThreads, const CChainParams& chainparams)
{
    static boost::thread_group* minerThreads = NULL;

    if (nThreads < 0)
        nThreads = GetNumCores();

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
        minerThreads->create_thread(boost::bind(&LibertaMiner, boost::cref(chainparams)));
}
