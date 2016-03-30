// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Copyright (c) 2013 The NovaCoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txdb.h"
#include "miner.h"
#include "kernel.h"
#include "kernel_worker.h"

using namespace std;

//////////////////////////////////////////////////////////////////////////////
//
// BitcoinMiner
//

int64_t nReserveBalance = 0;
static unsigned int nMaxStakeSearchInterval = 60;
uint64_t nStakeInputsMapSize = 0;

int static FormatHashBlocks(void* pbuffer, unsigned int len)
{
    unsigned char* pdata = (unsigned char*)pbuffer;
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

    COrphan(CTransaction* ptxIn)
    {
        ptx = ptxIn;
        dPriority = dFeePerKb = 0;
    }
};


uint64_t nLastBlockTx = 0;
uint64_t nLastBlockSize = 0;
uint32_t nLastCoinStakeSearchInterval = 0;
 
// We want to sort transactions by priority and fee, so:
typedef boost::tuple<double, double, CTransaction*> TxPriority;
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
        }
        else
        {
            if (a.get<0>() == b.get<0>())
                return a.get<1>() < b.get<1>();
            return a.get<0>() < b.get<0>();
        }
    }
};

// CreateNewBlock: create new block (without proof-of-work/with provided coinstake)
CBlock* CreateNewBlock(CWallet* pwallet, CTransaction *txCoinStake)
{
    bool fProofOfStake = txCoinStake != NULL;

    // Create new block
    auto_ptr<CBlock> pblock(new CBlock());
    if (!pblock.get())
        return NULL;

    // Create coinbase tx
    CTransaction txCoinBase;
    txCoinBase.vin.resize(1);
    txCoinBase.vin[0].prevout.SetNull();
    txCoinBase.vout.resize(1);

    if (!fProofOfStake)
    {
        CReserveKey reservekey(pwallet);
        txCoinBase.vout[0].scriptPubKey.SetDestination(reservekey.GetReservedKey().GetID());

        // Add our coinbase tx as first transaction
        pblock->vtx.push_back(txCoinBase);
    }
    else
    {
        // Coinbase output must be empty for Proof-of-Stake block
        txCoinBase.vout[0].SetEmpty();

        // Syncronize timestamps
        pblock->nTime = txCoinBase.nTime = txCoinStake->nTime;

        // Add coinbase and coinstake transactions
        pblock->vtx.push_back(txCoinBase);
        pblock->vtx.push_back(*txCoinStake);
    }

    // Largest block you're willing to create:
    unsigned int nBlockMaxSize = GetArgUInt("-blockmaxsize", MAX_BLOCK_SIZE_GEN/2);
    // Limit to betweeen 1K and MAX_BLOCK_SIZE-1K for sanity:
    nBlockMaxSize = std::max(1000u, std::min(MAX_BLOCK_SIZE-1000u, nBlockMaxSize));

    // How much of the block should be dedicated to high-priority transactions,
    // included regardless of the fees they pay
    unsigned int nBlockPrioritySize = GetArgUInt("-blockprioritysize", 27000);
    nBlockPrioritySize = std::min(nBlockMaxSize, nBlockPrioritySize);

    // Minimum block size you want to create; block will be filled with free transactions
    // until there are no more or the block reaches this size:
    unsigned int nBlockMinSize = GetArgUInt("-blockminsize", 0);
    nBlockMinSize = std::min(nBlockMaxSize, nBlockMinSize);

    // Fee-per-kilobyte amount considered the same as "free"
    // Be careful setting this: if you set it to zero then
    // a transaction spammer can cheaply fill blocks using
    // 1-satoshi-fee transactions. It should be set above the real
    // cost to you of processing a transaction.
    int64_t nMinTxFee = MIN_TX_FEE;
    if (mapArgs.count("-mintxfee"))
        ParseMoney(mapArgs["-mintxfee"], nMinTxFee);

    CBlockIndex* pindexPrev = pindexBest;

    pblock->nBits = GetNextTargetRequired(pindexPrev, fProofOfStake);

    // Collect memory pool transactions into the block
    int64_t nFees = 0;
    {
        LOCK2(cs_main, mempool.cs);
        CBlockIndex* pindexPrev = pindexBest;
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
                        printf("ERROR: mempool transaction missing input\n");
                        if (fDebug) assert("mempool transaction missing input" == 0);
                        fMissingInputs = true;
                        if (porphan)
                            vOrphan.pop_back();
                        break;
                    }

                    // Has to wait for dependencies
                    if (!porphan)
                    {
                        // Use list for automatic deletion
                        vOrphan.push_back(COrphan(&tx));
                        porphan = &vOrphan.back();
                    }
                    mapDependers[txin.prevout.hash].push_back(porphan);
                    porphan->setDependsOn.insert(txin.prevout.hash);
                    nTotalIn += mempool.mapTx[txin.prevout.hash].vout[txin.prevout.n].nValue;
                    continue;
                }
                int64_t nValueIn = txPrev.vout[txin.prevout.n].nValue;
                nTotalIn += nValueIn;

                int nConf = txindex.GetDepthInMainChain();
                dPriority += (double)nValueIn * nConf;
            }
            if (fMissingInputs) continue;

            // Priority is sum(valuein * age) / txsize
            unsigned int nTxSize = ::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION);
            dPriority /= nTxSize;

            // This is a more accurate fee-per-kilobyte than is used by the client code, because the
            // client code rounds up the size to the nearest 1K. That's good, because it gives an
            // incentive to create smaller transactions.
            double dFeePerKb =  double(nTotalIn-tx.GetValueOut()) / (double(nTxSize)/1000.0);

            if (porphan)
            {
                porphan->dPriority = dPriority;
                porphan->dFeePerKb = dFeePerKb;
            }
            else
                vecPriority.push_back(TxPriority(dPriority, dFeePerKb, &(*mi).second));
        }

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
            CTransaction& tx = *(vecPriority.front().get<2>());

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
            if (tx.nTime > GetAdjustedTime() || (fProofOfStake && tx.nTime > txCoinStake->nTime))
                continue;

            // Skip free transactions if we're past the minimum block size:
            if (fSortedByFee && (dFeePerKb < nMinTxFee) && (nBlockSize + nTxSize >= nBlockMinSize))
                continue;

            // Prioritize by fee once past the priority size or we run out of high-priority
            // transactions:
            if (!fSortedByFee &&
                ((nBlockSize + nTxSize >= nBlockPrioritySize) || (dPriority < COIN * 144 / 250)))
            {
                fSortedByFee = true;
                comparer = TxPriorityCompare(fSortedByFee);
                std::make_heap(vecPriority.begin(), vecPriority.end(), comparer);
            }

            // Connecting shouldn't fail due to dependency on other memory pool transactions
            // because we're already processing them in order of dependency
            map<uint256, CTxIndex> mapTestPoolTmp(mapTestPool);
            MapPrevTx mapInputs;
            bool fInvalid;
            if (!tx.FetchInputs(txdb, mapTestPoolTmp, false, true, mapInputs, fInvalid))
                continue;

            // Transaction fee
            int64_t nTxFees = tx.GetValueIn(mapInputs)-tx.GetValueOut();
            int64_t nMinFee = tx.GetMinFee(nBlockSize, true, GMF_BLOCK, nTxSize);
            if (nTxFees < nMinFee)
                continue;

            // Sigops accumulation
            nTxSigOps += tx.GetP2SHSigOpCount(mapInputs);
            if (nBlockSigOps + nTxSigOps >= MAX_BLOCK_SIGOPS)
                continue;

            if (!tx.ConnectInputs(txdb, mapInputs, mapTestPoolTmp, CDiskTxPos(1,1,1), pindexPrev, false, true, true, MANDATORY_SCRIPT_VERIFY_FLAGS))
                continue;
            mapTestPoolTmp[tx.GetHash()] = CTxIndex(CDiskTxPos(1,1,1), tx.vout.size());
            swap(mapTestPool, mapTestPoolTmp);

            // Added
            pblock->vtx.push_back(tx);
            nBlockSize += nTxSize;
            ++nBlockTx;
            nBlockSigOps += nTxSigOps;
            nFees += nTxFees;

            if (fDebug && GetBoolArg("-printpriority"))
            {
                printf("priority %.1f feeperkb %.1f txid %s\n",
                       dPriority, dFeePerKb, tx.GetHash().ToString().c_str());
            }

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
                            vecPriority.push_back(TxPriority(porphan->dPriority, porphan->dFeePerKb, porphan->ptx));
                            std::push_heap(vecPriority.begin(), vecPriority.end(), comparer);
                        }
                    }
                }
            }
        }

        nLastBlockTx = nBlockTx;
        nLastBlockSize = nBlockSize;

        if (!fProofOfStake)
        {
            pblock->vtx[0].vout[0].nValue = GetProofOfWorkReward(pblock->nBits, nFees);

            if (fDebug)
                printf("CreateNewBlock(): PoW reward %" PRIu64 "\n", pblock->vtx[0].vout[0].nValue);
        }

        if (fDebug && GetBoolArg("-printpriority"))
            printf("CreateNewBlock(): total size %" PRIu64 "\n", nBlockSize);

        // Fill in header
        pblock->hashPrevBlock  = pindexPrev->GetBlockHash();
        if (!fProofOfStake)
        {
            pblock->nTime          = max(pindexPrev->GetMedianTimePast()+1, pblock->GetMaxTransactionTime());
            pblock->nTime          = max(pblock->GetBlockTime(), PastDrift(pindexPrev->GetBlockTime()));
            pblock->UpdateTime(pindexPrev);
        }
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
    }
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
    printf("CheckWork() : new proof-of-work block found  \n  hash: %s  \ntarget: %s\n", hashBlock.GetHex().c_str(), hashTarget.GetHex().c_str());
    pblock->print();
    printf("generated %s\n", FormatMoney(pblock->vtx[0].vout[0].nValue).c_str());

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
        if (!ProcessBlock(NULL, pblock))
            return error("CheckWork() : ProcessBlock, block not accepted");
    }

    return true;
}

bool CheckStake(CBlock* pblock, CWallet& wallet)
{
    uint256 proofHash = 0, hashTarget = 0;
    uint256 hashBlock = pblock->GetHash();

    if(!pblock->IsProofOfStake())
        return error("CheckStake() : %s is not a proof-of-stake block", hashBlock.GetHex().c_str());

    // verify hash target and signature of coinstake tx
    if (!CheckProofOfStake(pblock->vtx[1], pblock->nBits, proofHash, hashTarget))
        return error("CheckStake() : proof-of-stake checking failed");

    //// debug print
    printf("CheckStake() : new proof-of-stake block found  \n  hash: %s \nproofhash: %s  \ntarget: %s\n", hashBlock.GetHex().c_str(), proofHash.GetHex().c_str(), hashTarget.GetHex().c_str());
    pblock->print();
    printf("out %s\n", FormatMoney(pblock->vtx[1].GetValueOut()).c_str());

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
        if (!ProcessBlock(NULL, pblock))
            return error("CheckStake() : ProcessBlock, block not accepted");
    }

    return true;
}

// Precalculated SHA256 contexts and metadata
// (txid, vout.n) => (kernel, (tx.nTime, nAmount))
typedef std::map<std::pair<uint256, unsigned int>, std::pair<std::vector<unsigned char>, std::pair<uint32_t, uint64_t> > > MidstateMap;

// Fill the inputs map with precalculated contexts and metadata
bool FillMap(CWallet *pwallet, uint32_t nUpperTime, MidstateMap &inputsMap)
{
    // Choose coins to use
    int64_t nBalance = pwallet->GetBalance();

    if (nBalance <= nReserveBalance)
        return false;

    uint32_t nTime = GetAdjustedTime();

    CTxDB txdb("r");
    {
        LOCK2(cs_main, pwallet->cs_wallet);

        CoinsSet setCoins;
        int64_t nValueIn = 0;
        if (!pwallet->SelectCoinsSimple(nBalance - nReserveBalance, MIN_TX_FEE, MAX_MONEY, nUpperTime, nCoinbaseMaturity * 10, setCoins, nValueIn))
            return error("FillMap() : SelectCoinsSimple failed");

        if (setCoins.empty())
            return false;

        CBlock block;
        CTxIndex txindex;

        for(CoinsSet::const_iterator pcoin = setCoins.begin(); pcoin != setCoins.end(); pcoin++)
        {
            pair<uint256, uint32_t> key = make_pair(pcoin->first->GetHash(), pcoin->second);

            // Skip existent inputs
            if (inputsMap.find(key) != inputsMap.end())
                continue;

            // Trying to parse scriptPubKey
            txnouttype whichType;
            vector<valtype> vSolutions;
            if (!Solver(pcoin->first->vout[pcoin->second].scriptPubKey, whichType, vSolutions))
                continue;

            // Only support pay to public key and pay to address
            if (whichType != TX_PUBKEY && whichType != TX_PUBKEYHASH)
                continue;

            // Load transaction index item
            if (!txdb.ReadTxIndex(pcoin->first->GetHash(), txindex))
                continue;

            // Read block header
            if (!block.ReadFromDisk(txindex.pos.nFile, txindex.pos.nBlockPos, false))
                continue;

            // Only load coins meeting min age requirement
            if (nStakeMinAge + block.nTime > nTime - nMaxStakeSearchInterval)
                continue;

            // Get stake modifier
            uint64_t nStakeModifier = 0;
            if (!GetKernelStakeModifier(block.GetHash(), nStakeModifier))
                continue;

            // Build static part of kernel
            CDataStream ssKernel(SER_GETHASH, 0);
            ssKernel << nStakeModifier;
            ssKernel << block.nTime << (txindex.pos.nTxPos - txindex.pos.nBlockPos) << pcoin->first->nTime << pcoin->second;

            // (txid, vout.n) => (kernel, (tx.nTime, nAmount))
            inputsMap[key] = make_pair(std::vector<unsigned char>(ssKernel.begin(), ssKernel.end()), make_pair(pcoin->first->nTime, pcoin->first->vout[pcoin->second].nValue));
        }

        nStakeInputsMapSize = inputsMap.size();

        if (fDebug)
            printf("FillMap() : Map of %" PRIu64 " precalculated contexts has been created by stake miner\n", nStakeInputsMapSize);
    }

    return true;
}

// Scan inputs map in order to find a solution
bool ScanMap(const MidstateMap &inputsMap, uint32_t nBits, MidstateMap::key_type &LuckyInput, std::pair<uint256, uint32_t> &solution)
{
    static uint32_t nLastCoinStakeSearchTime = GetAdjustedTime(); // startup timestamp
    uint32_t nSearchTime = GetAdjustedTime();

    if (inputsMap.size() > 0 && nSearchTime > nLastCoinStakeSearchTime)
    {
        // Scanning interval (begintime, endtime)
        std::pair<uint32_t, uint32_t> interval;

        interval.first = nSearchTime;
        interval.second = nSearchTime - min(nSearchTime-nLastCoinStakeSearchTime, nMaxStakeSearchInterval);

        // (txid, nout) => (kernel, (tx.nTime, nAmount))
        for(MidstateMap::const_iterator input = inputsMap.begin(); input != inputsMap.end(); input++)
        {
            unsigned char *kernel = (unsigned char *) &input->second.first[0];

            // scan(State, Bits, Time, Amount, ...)
            if (ScanKernelBackward(kernel, nBits, input->second.second.first, input->second.second.second, interval, solution))
            {
                // Solution found
                LuckyInput = input->first; // (txid, nout)

                return true;
            }
        }

        // Inputs map iteration can be big enough to consume few seconds while scanning.
        // We're using dynamical calculation of scanning interval in order to compensate this delay.
        nLastCoinStakeSearchInterval = nSearchTime - nLastCoinStakeSearchTime;
        nLastCoinStakeSearchTime = nSearchTime;
    }

    // No solutions were found
    return false;
}

// Stake miner thread
void ThreadStakeMiner(void* parg)
{
    SetThreadPriority(THREAD_PRIORITY_LOWEST);

    // Make this thread recognisable as the mining thread
    RenameThread("novacoin-miner");
    CWallet* pwallet = (CWallet*)parg;

    MidstateMap inputsMap;
    if (!FillMap(pwallet, GetAdjustedTime(), inputsMap))
        return;

    bool fTrySync = true;

    CBlockIndex* pindexPrev = pindexBest;
    uint32_t nBits = GetNextTargetRequired(pindexPrev, true);

    printf("ThreadStakeMinter started\n");

    try
    {
        vnThreadsRunning[THREAD_MINTER]++;

        MidstateMap::key_type LuckyInput;
        std::pair<uint256, uint32_t> solution;

        // Main miner loop
        do
        {
            if (fShutdown)
                goto _endloop;

            while (pwallet->IsLocked())
            {
                Sleep(1000);
                if (fShutdown)
                    goto _endloop; // Don't be afraid to use a goto if that's the best option.
            }

            while (vNodes.empty() || IsInitialBlockDownload())
            {
                fTrySync = true;

                Sleep(1000);
                if (fShutdown)
                    goto _endloop;
            }

            if (fTrySync)
            {
                // Don't try mine blocks unless we're at the top of chain and have at least three p2p connections.
                fTrySync = false;
                if (vNodes.size() < 3 || nBestHeight < GetNumBlocksOfPeers())
                {
                    Sleep(1000);
                    continue;
                }
            }

            if (ScanMap(inputsMap, nBits, LuckyInput, solution))
            {
                SetThreadPriority(THREAD_PRIORITY_NORMAL);

                // Remove lucky input from the map
                inputsMap.erase(inputsMap.find(LuckyInput));

                CKey key;
                CTransaction txCoinStake;

                // Create new coinstake transaction
                if (!pwallet->CreateCoinStake(LuckyInput.first, LuckyInput.second, solution.second, nBits, txCoinStake, key))
                {
                    string strMessage = _("Warning: Unable to create coinstake transaction, see debug.log for the details. Mining thread has been stopped.");
                    strMiscWarning = strMessage;
                    printf("*** %s\n", strMessage.c_str());

                    break;
                }

                // Now we have new coinstake, it's time to create the block ...
                CBlock* pblock;
                pblock = CreateNewBlock(pwallet, &txCoinStake);
                if (!pblock)
                {
                    string strMessage = _("Warning: Unable to allocate memory for the new block object. Mining thread has been stopped.");
                    strMiscWarning = strMessage;
                    printf("*** %s\n", strMessage.c_str());

                    break;
                }

                unsigned int nExtraNonce = 0;
                IncrementExtraNonce(pblock, pindexPrev, nExtraNonce);

                // ... and sign it
                if (!key.Sign(pblock->GetHash(), pblock->vchBlockSig))
                {
                    string strMessage = _("Warning: Proof-of-Stake miner is unable to sign the block (locked wallet?). Mining thread has been stopped.");
                    strMiscWarning = strMessage;
                    printf("*** %s\n", strMessage.c_str());

                    break;
                }

                CheckStake(pblock, *pwallet);
                SetThreadPriority(THREAD_PRIORITY_LOWEST);
                Sleep(500);
            }

            if (pindexPrev != pindexBest)
            {
                // The best block has been changed, we need to refill the map
                if (FillMap(pwallet, GetAdjustedTime(), inputsMap))
                {
                    pindexPrev = pindexBest;
                    nBits = GetNextTargetRequired(pindexPrev, true);
                }
                else
                {
                    // Clear existent data if FillMap failed
                    inputsMap.clear();
                }
            }

            Sleep(500);

            _endloop:
                (void)0; // do nothing
        }
        while(!fShutdown);

        vnThreadsRunning[THREAD_MINTER]--;
    }
    catch (std::exception& e) {
        vnThreadsRunning[THREAD_MINTER]--;
        PrintException(&e, "ThreadStakeMinter()");
    } catch (...) {
        vnThreadsRunning[THREAD_MINTER]--;
        PrintException(NULL, "ThreadStakeMinter()");
    }
    printf("ThreadStakeMinter exiting, %d threads remaining\n", vnThreadsRunning[THREAD_MINTER]);
}
