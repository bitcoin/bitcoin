// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Copyright (c) 2013 The NovaCoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txdb.h"
#include "miner.h"
#include "kernel.h"
#include "core.h"


using namespace std;

//////////////////////////////////////////////////////////////////////////////
//
// BitcoinMiner
//

int static FormatHashBlocks(void* pbuffer, unsigned int len)
{
    unsigned char* pdata = (unsigned char*) pbuffer;
    unsigned int blocks = 1 + ((len + 8) / 64);
    unsigned char* pend = pdata + 64 * blocks;
    memset(pdata + len, 0, 64 * blocks - len);
    pdata[len] = 0x80;
    unsigned int bits = len * 8;
    pend[-1] = (bits >> 0) & 0xff;
    pend[-2] = (bits >> 8) & 0xff;
    pend[-3] = (bits >> 16) & 0xff;
    pend[-4] = (bits >> 24) & 0xff;
    return blocks;
}

static const unsigned int pSHA256InitState[8] =
{0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};

void SHA256Transform(void* pstate, void* pinput, const void* pinit)
{
    SHA256_CTX ctx;
    unsigned char data[64];

    SHA256_Init(&ctx);

    for (int i = 0; i < 16; i++)
        ((uint32_t*)data)[i] = ByteReverse(((uint32_t*)pinput)[i]);

    for (int i = 0; i < 8; i++)
        ctx.h[i] = ((uint32_t*)pinit)[i];

    SHA256_Update(&ctx, data, sizeof(data));
    for (int i = 0; i < 8; i++)
        ((uint32_t*)pstate)[i] = ctx.h[i];
}

// Some explaining would be appreciated
class COrphan
{
public:
    CTransaction* ptx;
    set<uint256> setDependsOn;
    double dPriority;
    double dFeePerKb;
    int64_t nFee;

    COrphan(CTransaction* ptxIn)
    {
        ptx = ptxIn;
        dPriority = dFeePerKb = 0;
        nFee = 0;
    }

    COrphan(double dPriority_, double dFeePerKb_, int64_t nFee_, CTransaction* ptxIn)
    {
        dPriority = dPriority_;
        dFeePerKb = dFeePerKb_;
        nFee = nFee_;
        ptx = ptxIn;
    }

    void print() const
    {
        LogPrintf("COrphan(hash=%s, dPriority=%.1f, dFeePerKb=%.1f)\n",
            ptx->GetHash().ToString().substr(0,10).c_str(), dPriority, dFeePerKb);
        BOOST_FOREACH(uint256 hash, setDependsOn)
            LogPrintf("   setDependsOn %s\n", hash.ToString().substr(0,10).c_str());
    }
};


uint64_t nLastBlockTx = 0;
uint64_t nLastBlockSize = 0;
int64_t nLastCoinStakeSearchInterval = 0;

// We want to sort transactions by priority and fee, so:
typedef boost::tuple<double, double, int64_t, CTransaction*> TxPriority;
class TxPriorityCompare
{
    bool byFee;
public:
    TxPriorityCompare(bool _byFee) : byFee(_byFee) { }
    bool operator()(const TxPriority& a, const TxPriority& b)
    {
        if (byFee)
        {
            if (a.get<1>() == b.get<1>())
                return a.get<0>() < b.get<0>();
            return a.get<1>() < b.get<1>();
        } else
        {
            if (a.get<0>() == b.get<0>())
                return a.get<1>() < b.get<1>();
            return a.get<0>() < b.get<0>();
        }
    }
};

// CreateNewBlock: create new block (without proof-of-work/proof-of-stake)
CBlock* CreateNewBlock(CWallet* pwallet, bool fProofOfStake, int64_t* pFees)
{
    // Create new block
    auto_ptr<CBlock> pblock(new CBlock());
    if (!pblock.get())
        return NULL;
    
    CBlockIndex* pindexPrev = pindexBest;

    // Create coinbase tx
    CTransaction txNew;
    txNew.vin.resize(1);
    txNew.vin[0].prevout.SetNull();
    txNew.vout.resize(1);


    int nHeight = pindexPrev->nHeight+1; // height of new block
    
    if (!Params().IsProtocolV2(nHeight)) // generate old version until protocolV2
        pblock->nVersion = 6;
    

    if (!fProofOfStake)
    {
        CReserveKey reservekey(pwallet);
        CPubKey pubkey;
        if (!reservekey.GetReservedKey(pubkey))
            return NULL;
        txNew.vout[0].scriptPubKey.SetDestination(pubkey.GetID());
    } else
    {
        // Height first in coinbase required for block.version=2
        txNew.vin[0].scriptSig = (CScript() << nHeight) + COINBASE_FLAGS;
        assert(txNew.vin[0].scriptSig.size() <= 100);

        txNew.vout[0].SetEmpty();
    };

    // Add our coinbase tx as first transaction
    pblock->vtx.push_back(txNew);
    pblock->nBits = GetNextTargetRequired(pindexPrev, fProofOfStake);

    // Collect memory pool transactions into the block
    int64_t nFees = 0;
    {
        LOCK2(cs_main, mempool.cs);
        CTxDB txdb("r");

        // Priority order to process transactions
        list<COrphan> vOrphan; // list memory doesn't move
        map<uint256, vector<COrphan*> > mapDependers;

        // This vector will be sorted into a priority queue:
        vector<TxPriority> vecPriority;
        vecPriority.reserve(mempool.mapTx.size());
        for (map<uint256, CTransaction>::iterator mi = mempool.mapTx.begin(); mi != mempool.mapTx.end(); ++mi)
        {
            CTransaction& tx = (*mi).second;
            if (tx.IsCoinBase() || tx.IsCoinStake() || !tx.IsFinal())
                continue;

            COrphan* porphan = NULL;
            double dPriority = 0;
            int64_t nTotalIn = 0;
            bool fMissingInputs = false;
            BOOST_FOREACH(const CTxIn& txin, tx.vin)
            {
                if (tx.nVersion == ANON_TXN_VERSION
                    && txin.IsAnonInput()) // anon inputs are verified later in CheckAnonInputs()
                    continue;

                // Read prev transaction
                CTransaction txPrev;
                CTxIndex txindex;
                if (!txPrev.ReadFromDisk(txdb, txin.prevout, txindex))
                {
                    // This should never happen; all transactions in the memory
                    // pool should connect to either transactions in the chain
                    // or other transactions in the memory pool.
                    if (!mempool.mapTx.count(txin.prevout.hash))
                    {
                        LogPrintf("ERROR: mempool transaction missing input\n");
                        if (fDebug)
                            assert("mempool transaction missing input" == 0);
                        fMissingInputs = true;
                        if (porphan)
                            vOrphan.pop_back();
                        break;
                    };

                    // Has to wait for dependencies
                    if (!porphan)
                    {
                        // Use list for automatic deletion
                        vOrphan.push_back(COrphan(&tx));
                        porphan = &vOrphan.back();
                    };

                    mapDependers[txin.prevout.hash].push_back(porphan);
                    porphan->setDependsOn.insert(txin.prevout.hash);
                    nTotalIn += mempool.mapTx[txin.prevout.hash].vout[txin.prevout.n].nValue;
                    continue;
                };

                int64_t nValueIn = txPrev.vout[txin.prevout.n].nValue;
                nTotalIn += nValueIn;

                int nConf = txindex.GetDepthInMainChainFromIndex();
                dPriority += (double)nValueIn * nConf;
            };


            if (tx.nVersion == ANON_TXN_VERSION)
            {
                int64_t nSumAnon;
                bool fInvalid;
                if (!tx.CheckAnonInputs(txdb, nSumAnon, fInvalid, false))
                {
                    if (fInvalid)
                        LogPrintf("CreateNewBlock() : CheckAnonInputs found invalid tx %s\n", tx.GetHash().ToString().substr(0,10).c_str());
                    fMissingInputs = true;
                    continue;
                };

                nTotalIn += nSumAnon;
            };

            if (fMissingInputs)
                continue;

            // Priority is sum(valuein * age) / txsize
            unsigned int nTxSize = ::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION);
            dPriority /= nTxSize;

            // This is a more accurate fee-per-kilobyte than is used by the client code, because the
            // client code rounds up the size to the nearest 1K. That's good, because it gives an
            // incentive to create smaller transactions.
            int64_t nFee = nTotalIn-tx.GetValueOut();
            double dFeePerKb =  double(nFee) / (double(nTxSize)/1000.0);

            if (porphan)
            {
                porphan->dPriority = dPriority;
                porphan->dFeePerKb = dFeePerKb;
            } else
            {
                vecPriority.push_back(TxPriority(dPriority, dFeePerKb, nFee, &(*mi).second));
            };
        };

        // Collect transactions into block
        map<uint256, CTxIndex> mapTestPool;
        uint64_t nBlockSize = 1000;
        uint64_t nBlockTx = 0;
        int nBlockSigOps = 100;
        bool fSortedByFee = (nBlockPrioritySize <= 0);

        TxPriorityCompare comparer(fSortedByFee);
        std::make_heap(vecPriority.begin(), vecPriority.end(), comparer);

        while (!vecPriority.empty())
        {
            // Take highest priority transaction off the priority queue:
            double dPriority = vecPriority.front().get<0>();
            double dFeePerKb = vecPriority.front().get<1>();
            int64_t nFee = vecPriority.front().get<2>();
            CTransaction& tx = *(vecPriority.front().get<3>());

            std::pop_heap(vecPriority.begin(), vecPriority.end(), comparer);
            vecPriority.pop_back();

            // Size limits
            unsigned int nTxSize = ::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION);
            if (nBlockSize + nTxSize >= nBlockMaxSize)
                continue;

            // Legacy limits on sigOps:
            unsigned int nTxSigOps = tx.GetLegacySigOpCount();
            if (nBlockSigOps + nTxSigOps >= MAX_BLOCK_SIGOPS)
                continue;

            // Timestamp limit
            if (tx.nTime > GetAdjustedTime() || (fProofOfStake && tx.nTime > pblock->vtx[0].nTime))
                continue;

            // Transaction fee
            int64_t nMinFee = tx.GetMinFee(nBlockSize, GMF_BLOCK); // will get GMF_ANON if tx.nVersion == ANON_TXN_VERSION

            // Skip free transactions if we're past the minimum block size:
            if (fSortedByFee && (dFeePerKb < nMinTxFee) && (nBlockSize + nTxSize >= nBlockMinSize))
                continue;

            // Prioritize by fee once past the priority size or we run out of high-priority
            // transactions:
            if (!fSortedByFee
                && ((nBlockSize + nTxSize >= nBlockPrioritySize) || (dPriority < COIN * 144 / 250)))
            {
                fSortedByFee = true;
                comparer = TxPriorityCompare(fSortedByFee);
                std::make_heap(vecPriority.begin(), vecPriority.end(), comparer);
            };

            // Connecting shouldn't fail due to dependency on other memory pool transactions
            // because we're already processing them in order of dependency
            map<uint256, CTxIndex> mapTestPoolTmp(mapTestPool);
            MapPrevTx mapInputs;
            bool fInvalid;
            if (!tx.FetchInputs(txdb, mapTestPoolTmp, false, true, mapInputs, fInvalid))
                continue;

            //int64_t nTxFees = tx.GetValueIn(mapInputs)-tx.GetValueOut();

            // -- Avoid calling CheckAnonInputs twice, use nFee from vecPriority
            if (nFee == 0) // tx came from COrphan
            {
                int64_t nTxFees = tx.GetValueIn(mapInputs)-tx.GetValueOut();

                if (tx.nVersion == ANON_TXN_VERSION)
                {
                    int64_t nSumAnon;
                    bool fInvalid;
                    if (!tx.CheckAnonInputs(txdb, nSumAnon, fInvalid, false))
                    {
                        if (fInvalid)
                            LogPrintf("CreateNewBlock() : CheckAnonInputs found invalid tx %s\n", tx.GetHash().ToString().substr(0,10).c_str());
                        continue;
                    };

                    nTxFees += nSumAnon;
                };
                nFee = nTxFees;
            };


            // TODO: must this be done twice!?
            // Need to look at COrphan


            if (nFee < nMinFee)
                continue;

            nTxSigOps += tx.GetP2SHSigOpCount(mapInputs);
            if (nBlockSigOps + nTxSigOps >= MAX_BLOCK_SIGOPS)
                continue;

            if (!tx.ConnectInputs(txdb, mapInputs, mapTestPoolTmp, CDiskTxPos(1,1,1), pindexPrev, false, true, MANDATORY_SCRIPT_VERIFY_FLAGS))
                continue;

            mapTestPoolTmp[tx.GetHash()] = CTxIndex(CDiskTxPos(1,1,1), tx.vout.size());
            swap(mapTestPool, mapTestPoolTmp);

            // Added
            pblock->vtx.push_back(tx);
            nBlockSize += nTxSize;
            ++nBlockTx;
            nBlockSigOps += nTxSigOps;
            nFees += nFee;

            if (fDebug && GetBoolArg("-printpriority"))
            {
                LogPrintf("priority %.1f feeperkb %.1f txid %s\n",
                    dPriority, dFeePerKb, tx.GetHash().ToString().c_str());
            };

            // Add transactions that depend on this one to the priority queue
            uint256 hash = tx.GetHash();
            if (mapDependers.count(hash))
            {
                BOOST_FOREACH(COrphan* porphan, mapDependers[hash])
                {
                    if (!porphan->setDependsOn.empty())
                    {
                        porphan->setDependsOn.erase(hash);
                        if (porphan->setDependsOn.empty())
                        {
                            vecPriority.push_back(TxPriority(porphan->dPriority, porphan->dFeePerKb, porphan->nFee, porphan->ptx));
                            std::push_heap(vecPriority.begin(), vecPriority.end(), comparer);
                        };
                    };
                };
            };
        };

        nLastBlockTx = nBlockTx;
        nLastBlockSize = nBlockSize;

        if (fDebug && GetBoolArg("-printpriority"))
            LogPrintf("CreateNewBlock(): total size %u\n", nBlockSize);

        if (!fProofOfStake)
            pblock->vtx[0].vout[0].nValue = Params().GetProofOfWorkReward(nHeight, nFees);

        if (pFees)
            *pFees = nFees;

        // Fill in header
        pblock->hashPrevBlock  = pindexPrev->GetBlockHash();
        pblock->nTime          = max(pindexPrev->GetPastTimeLimit()+1, pblock->GetMaxTransactionTime());
        if (!fProofOfStake)
            pblock->UpdateTime(pindexPrev);
        pblock->nNonce         = 0;
    }

    return pblock.release();
}


void IncrementExtraNonce(CBlock* pblock, CBlockIndex* pindexPrev, unsigned int& nExtraNonce)
{
    // Update nExtraNonce
    static uint256 hashPrevBlock;
    if (hashPrevBlock != pblock->hashPrevBlock)
    {
        nExtraNonce = 0;
        hashPrevBlock = pblock->hashPrevBlock;
    };

    ++nExtraNonce;

    unsigned int nHeight = pindexPrev->nHeight+1; // Height first in coinbase required for block.version=2
    pblock->vtx[0].vin[0].scriptSig = (CScript() << nHeight << CBigNum(nExtraNonce)) + COINBASE_FLAGS;
    assert(pblock->vtx[0].vin[0].scriptSig.size() <= 100);

    pblock->hashMerkleRoot = pblock->BuildMerkleTree();
}


void FormatHashBuffers(CBlock* pblock, char* pmidstate, char* pdata, char* phash1)
{
    //
    // Pre-build hash buffers
    //
    struct
    {
        struct unnamed2
        {
            int nVersion;
            uint256 hashPrevBlock;
            uint256 hashMerkleRoot;
            unsigned int nTime;
            unsigned int nBits;
            unsigned int nNonce;
        }
        block;
        unsigned char pchPadding0[64];
        uint256 hash1;
        unsigned char pchPadding1[64];
    }
    tmp;
    memset(&tmp, 0, sizeof(tmp));

    tmp.block.nVersion       = pblock->nVersion;
    tmp.block.hashPrevBlock  = pblock->hashPrevBlock;
    tmp.block.hashMerkleRoot = pblock->hashMerkleRoot;
    tmp.block.nTime          = pblock->nTime;
    tmp.block.nBits          = pblock->nBits;
    tmp.block.nNonce         = pblock->nNonce;

    FormatHashBlocks(&tmp.block, sizeof(tmp.block));
    FormatHashBlocks(&tmp.hash1, sizeof(tmp.hash1));

    // Byte swap all the input buffer
    for (unsigned int i = 0; i < sizeof(tmp)/4; i++)
        ((unsigned int*)&tmp)[i] = ByteReverse(((unsigned int*)&tmp)[i]);

    // Precalc the first half of the first hash, which stays constant
    SHA256Transform(pmidstate, &tmp.block, pSHA256InitState);

    memcpy(pdata, &tmp.block, 128);
    memcpy(phash1, &tmp.hash1, 64);
}


bool CheckWork(CBlock* pblock, CWallet& wallet, CReserveKey& reservekey)
{
    uint256 hashBlock = pblock->GetHash();
    uint256 hashTarget = CBigNum().SetCompact(pblock->nBits).getuint256();

    if(!pblock->IsProofOfWork())
        return error("CheckWork() : %s is not a proof-of-work block", hashBlock.GetHex().c_str());

    if (hashBlock > hashTarget)
        return error("CheckWork() : proof-of-work not meeting target");

    //// debug print
    LogPrintf("CheckWork() : new proof-of-work block found  \n  hash: %s  \ntarget: %s\n", hashBlock.GetHex().c_str(), hashTarget.GetHex().c_str());
    pblock->print();
    LogPrintf("generated %s\n", FormatMoney(pblock->vtx[0].vout[0].nValue).c_str());

    // Found a solution
    {
        LOCK(cs_main);
        if (pblock->hashPrevBlock != hashBestChain)
            return error("CheckWork() : generated block is stale");

        // Remove key from key pool
        reservekey.KeepKey();

        // Track how many getdata requests this block gets
        {
            LOCK(wallet.cs_wallet);
            wallet.mapRequestCount[hashBlock] = 0;
        }

        // Process this block the same as if we had received it from another node
        if (!ProcessBlock(NULL, pblock, hashBlock))
            return error("CheckWork() : ProcessBlock, block not accepted");
    }

    return true;
}

bool CheckStake(CBlock* pblock, CWallet& wallet)
{
    uint256 proofHash = 0, hashTarget = 0;
    uint256 hashBlock = pblock->GetHash();

    if (!pblock->IsProofOfStake())
        return error("CheckStake() : %s is not a proof-of-stake block", hashBlock.GetHex().c_str());
    
    std::map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(pblock->hashPrevBlock);
    if (mi == mapBlockIndex.end())
        return error("CheckStake() : %s prev block not found: %s.", hashBlock.GetHex().c_str(), pblock->hashPrevBlock.GetHex().c_str());
    // verify hash target and signature of coinstake tx
    if (!CheckProofOfStake(mi->second, pblock->vtx[1], pblock->nBits, proofHash, hashTarget))
        return error("CheckStake() : proof-of-stake checking failed");

    //// debug print
    LogPrintf("CheckStake() : new proof-of-stake block found  \n  hash: %s \nproofhash: %s  \ntarget: %s\n", hashBlock.GetHex().c_str(), proofHash.GetHex().c_str(), hashTarget.GetHex().c_str());
    pblock->print();
    LogPrintf("out %s\n", FormatMoney(pblock->vtx[1].GetValueOut()).c_str());

    // Found a solution
    {
        LOCK(cs_main);
        if (pblock->hashPrevBlock != hashBestChain)
            return error("CheckStake() : generated block is stale");

        // Track how many getdata requests this block gets
        {
            LOCK(wallet.cs_wallet);
            wallet.mapRequestCount[hashBlock] = 0;
        }

        // Process this block the same as if we had received it from another node
        if (!ProcessBlock(NULL, pblock, hashBlock))
            return error("CheckStake() : ProcessBlock, block not accepted");
    }

    return true;
}

void ThreadStakeMiner(CWallet *pwallet)
{
    SetThreadPriority(THREAD_PRIORITY_LOWEST);

    bool fTryToSync = true;
    int64_t nTimeLastStake = 0;

    while (true)
    {
        boost::this_thread::interruption_point();
        
        while (pwallet->IsLocked())
        {
            fIsStaking = false;
            MilliSleep(2000);
            boost::this_thread::interruption_point();
        };

        while (vNodes.empty() || IsInitialBlockDownload())
        {
            fIsStaking = false;
            fTryToSync = true;
            if (fDebugPoS)
                LogPrintf("StakeMiner() IsInitialBlockDownload\n");
            MilliSleep(2000);
            boost::this_thread::interruption_point();
        };

        if (fTryToSync)
        {
            fTryToSync = false;

            if (vNodes.size() < 3 || nBestHeight < GetNumBlocksOfPeers())
            {
                fIsStaking = false;
                if (fDebugPoS)
                    LogPrintf("StakeMiner() TryToSync\n");
                MilliSleep(60000);
                continue;
            };
        };

        if (nBestHeight < GetNumBlocksOfPeers()-1)
        {
            fIsStaking = false;
            if (fDebugPoS)
                LogPrintf("StakeMiner() nBestHeight < GetNumBlocksOfPeers()\n");
            MilliSleep(nMinerSleep * 4);
            continue;
        };

        if (nMinStakeInterval > 0 && nTimeLastStake + (int64_t)nMinStakeInterval > GetTime())
        {
            if (fDebug)
                LogPrintf("StakeMiner() Rate limited to 1 / %d seconds.\n", nMinStakeInterval);
            MilliSleep(nMinStakeInterval * 500); // nMinStakeInterval / 2 seconds
            continue;
        };

        //
        // Create new block
        //
        int64_t nFees;
        auto_ptr<CBlock> pblock(CreateNewBlock(pwallet, true, &nFees));
        if (!pblock.get())
            return;
        
        fIsStaking = true;
        
        // Trying to sign a block
        if (pblock->SignBlock(*pwallet, nFees))
        {
            SetThreadPriority(THREAD_PRIORITY_NORMAL);
            if (CheckStake(pblock.get(), *pwallet))
                nTimeLastStake = GetTime();
            SetThreadPriority(THREAD_PRIORITY_LOWEST);
        };
        
        MilliSleep(nMinerSleep);
    };
}
