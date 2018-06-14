// Copyright (c) 2017-2018 The Particl Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pos/miner.h>

#include <pos/kernel.h>
#include <miner.h>
#include <chainparams.h>
#include <utilmoneystr.h>

#include <fs.h>
#include <sync.h>
#include <net.h>
#include <validation.h>
#include <base58.h>
#include <crypto/sha256.h>

#include <wallet/hdwallet.h>

#include <stdint.h>

typedef CWallet* CWalletRef;
std::vector<StakeThread*> vStakeThreads;

void StakeThread::condWaitFor(int ms)
{
    std::unique_lock<std::mutex> lock(mtxMinerProc);
    fWakeMinerProc = false;
    condMinerProc.wait_for(lock, std::chrono::milliseconds(ms), [this] { return this->fWakeMinerProc; });
};

std::atomic<bool> fStopMinerProc(false);
std::atomic<bool> fTryToSync(false);
std::atomic<bool> fIsStaking(false);


int nMinStakeInterval = 0;  // min stake interval in seconds
int nMinerSleep = 500;
std::atomic<int64_t> nTimeLastStake(0);

extern double GetDifficulty(const CBlockIndex* blockindex = nullptr);

double GetPoSKernelPS()
{
    LOCK(cs_main);

    CBlockIndex *pindex = chainActive.Tip();
    CBlockIndex *pindexPrevStake = nullptr;

    int nBestHeight = pindex->nHeight;

    int nPoSInterval = 72; // blocks sampled
    double dStakeKernelsTriedAvg = 0;
    int nStakesHandled = 0, nStakesTime = 0;

    while (pindex && nStakesHandled < nPoSInterval)
    {
        if (pindex->IsProofOfStake())
        {
            if (pindexPrevStake)
            {
                dStakeKernelsTriedAvg += GetDifficulty(pindexPrevStake) * 4294967296.0;
                nStakesTime += pindexPrevStake->nTime - pindex->nTime;
                nStakesHandled++;
            }
            pindexPrevStake = pindex;
        }
        pindex = pindex->pprev;
    }

    double result = 0;

    if (nStakesTime)
        result = dStakeKernelsTriedAvg / nStakesTime;

    //if (IsProtocolV2(nBestHeight))
        result *= Params().GetStakeTimestampMask(nBestHeight) + 1;

    return result;
}

bool CheckStake(CBlock *pblock)
{
    uint256 proofHash, hashTarget;
    uint256 hashBlock = pblock->GetHash();

    if (!pblock->IsProofOfStake())
        return error("%s: %s is not a proof-of-stake block.", __func__, hashBlock.GetHex());

    if (!CheckStakeUnique(*pblock, false)) // Check in SignBlock also
        return error("%s: %s CheckStakeUnique failed.", __func__, hashBlock.GetHex());

    BlockMap::const_iterator mi = mapBlockIndex.find(pblock->hashPrevBlock);
    if (mi == mapBlockIndex.end())
        return error("%s: %s prev block not found: %s.", __func__, hashBlock.GetHex(), pblock->hashPrevBlock.GetHex());

    if (!chainActive.Contains(mi->second))
        return error("%s: %s prev block in active chain: %s.", __func__, hashBlock.GetHex(), pblock->hashPrevBlock.GetHex());

    // verify hash target and signature of coinstake tx
    if (!CheckProofOfStake(mi->second, *pblock->vtx[0], pblock->nTime, pblock->nBits, proofHash, hashTarget))
        return error("%s: proof-of-stake checking failed.", __func__);

    // debug print
    LogPrintf("CheckStake(): New proof-of-stake block found  \n  hash: %s \nproofhash: %s  \ntarget: %s\n", hashBlock.GetHex(), proofHash.GetHex(), hashTarget.GetHex());
    if (LogAcceptCategory(BCLog::POS))
    {
        LogPrintf("block %s\n", pblock->ToString());
        LogPrintf("out %s\n", FormatMoney(pblock->vtx[0]->GetValueOut()));
    };

    {
        LOCK(cs_main);
        if (pblock->hashPrevBlock != chainActive.Tip()->GetBlockHash()) // hashbestchain
            return error("%s: Generated block is stale.", __func__);
    }

    std::shared_ptr<const CBlock> shared_pblock = std::make_shared<const CBlock>(*pblock);
    if (!ProcessNewBlock(Params(), shared_pblock, true, nullptr))
        return error("%s: Block not accepted.", __func__);

    return true;
};

bool ImportOutputs(CBlockTemplate *pblocktemplate, int nHeight)
{
    LogPrint(BCLog::POS, "%s, nHeight %d\n", __func__, nHeight);

    CBlock *pblock = &pblocktemplate->block;
    if (pblock->vtx.size() < 1)
        return error("%s: Malformed block.", __func__);

    fs::path fPath = GetDataDir() / "genesisOutputs.txt";

    if (!fs::exists(fPath))
        return error("%s: File not found 'genesisOutputs.txt'.", __func__);

    const int nMaxOutputsPerTxn = 80;
    FILE *fp;
    errno = 0;
    if (!(fp = fopen(fPath.string().c_str(), "rb")))
        return error("%s - Can't open file, strerror: %s.", __func__, strerror(errno));

    CMutableTransaction txn;
    txn.nVersion = PARTICL_TXN_VERSION;
    txn.SetType(TXN_COINBASE);
    txn.nLockTime = 0;
    txn.vin.push_back(CTxIn()); // null prevout

    // scriptsig len must be > 2
    const char *s = "import";
    txn.vin[0].scriptSig = CScript() << std::vector<unsigned char>((const unsigned char*)s, (const unsigned char*)s + strlen(s));

    int nOutput = 0, nAdded = 0;
    char cLine[512];
    char *pAddress, *pAmount;

    while (fgets(cLine, 512, fp))
    {
        cLine[511] = '\0'; // safety
        size_t len = strlen(cLine);
        while (isspace(cLine[len-1]) && len>0)
            cLine[len-1] = '\0', len--;

        if (!(pAddress = strtok(cLine, ","))
            || !(pAmount = strtok(nullptr, ",")))
            continue;

        nOutput++;
        if (nOutput <= nMaxOutputsPerTxn * (nHeight-1))
            continue;

        uint64_t amount;
        if (!ParseUInt64(std::string(pAmount), &amount) || !MoneyRange(amount))
        {
            LogPrintf("Warning: %s - Skipping invalid amount: %s, %s\n", __func__, pAmount, strerror(errno));
            continue;
        };

        std::string addrStr(pAddress);
        CBitcoinAddress addr(addrStr);

        CKeyID id;
        if (!addr.IsValid()
            || !addr.GetKeyID(id))
        {
            LogPrintf("Warning: %s - Skipping invalid address: %s\n", __func__, pAddress);
            continue;
        };

        CScript script = CScript() << OP_DUP << OP_HASH160 << ToByteVector(id) << OP_EQUALVERIFY << OP_CHECKSIG;
        OUTPUT_PTR<CTxOutStandard> txout = MAKE_OUTPUT<CTxOutStandard>();
        txout->nValue = amount;
        txout->scriptPubKey = script;
        txn.vpout.push_back(txout);

        nAdded++;
        if (nAdded >= nMaxOutputsPerTxn)
            break;
    };

    fclose(fp);

    uint256 hash = txn.GetHash();
    if (!Params().CheckImportCoinbase(nHeight, hash))
        return error("%s - Incorrect outputs hash.", __func__);

    pblock->vtx.insert(pblock->vtx.begin()+1, MakeTransactionRef(txn));

    return true;
};

void StopThreadStakeMiner()
{
    if (vStakeThreads.size() < 1 // no thread created
        || fStopMinerProc)
        return;
    LogPrint(BCLog::POS, "StopThreadStakeMiner\n");
    fStopMinerProc = true;

    for (auto t : vStakeThreads)
    {
        {
            std::lock_guard<std::mutex> lock(t->mtxMinerProc);
            t->fWakeMinerProc = true;
        }
        t->condMinerProc.notify_all();

        t->thread.join();
        delete t;
    };
    vStakeThreads.clear();
};

void WakeThreadStakeMiner(CHDWallet *pwallet)
{
    // Call when chain is synced, wallet unlocked or balance changed
    LogPrint(BCLog::POS, "WakeThreadStakeMiner thread %d\n", pwallet->nStakeThread);

    if (pwallet->nStakeThread >= vStakeThreads.size())
        return; // stake unit test
    StakeThread *t = vStakeThreads[pwallet->nStakeThread];
    pwallet->nLastCoinStakeSearchTime = 0;
    {
        std::lock_guard<std::mutex> lock(t->mtxMinerProc);
        t->fWakeMinerProc = true;
    }

    t->condMinerProc.notify_all();
};

bool ThreadStakeMinerStopped()
{
    return fStopMinerProc;
}

static inline void condWaitFor(size_t nThreadID, int ms)
{
    assert(vStakeThreads.size() > nThreadID);
    StakeThread *t = vStakeThreads[nThreadID];
    t->condWaitFor(ms);
};

void ThreadStakeMiner(size_t nThreadID, std::vector<std::shared_ptr<CWallet>> &vpwallets, size_t nStart, size_t nEnd)
{
    LogPrintf("Starting staking thread %d, %d wallet%s.\n", nThreadID, nEnd - nStart, (nEnd - nStart) > 1 ? "s" : "");

    int nBestHeight; // TODO: set from new block signal?
    int64_t nBestTime;

    int nLastImportHeight = Params().GetLastImportHeight();

    if (!gArgs.GetBoolArg("-staking", true))
    {
        LogPrint(BCLog::POS, "%s: -staking is false.\n", __func__);
        return;
    };

    CScript coinbaseScript;
    while (!fStopMinerProc)
    {
        if (fReindex || fImporting || fBusyImporting)
        {
            fIsStaking = false;
            LogPrint(BCLog::POS, "%s: Block import/reindex.\n", __func__);
            condWaitFor(nThreadID, 30000);
            continue;
        };

        if (fTryToSync)
        {
            fTryToSync = false;

            if (g_connman->vNodes.size() < 3 || nBestHeight < GetNumBlocksOfPeers())
            {
                fIsStaking = false;
                LogPrint(BCLog::POS, "%s: TryToSync\n", __func__);
                condWaitFor(nThreadID, 30000);
                continue;
            };
        };

        if (g_connman->vNodes.empty() || IsInitialBlockDownload())
        {
            fIsStaking = false;
            fTryToSync = true;
            LogPrint(BCLog::POS, "%s: IsInitialBlockDownload\n", __func__);
            condWaitFor(nThreadID, 2000);
            continue;
        };


        {
            LOCK(cs_main);
            nBestHeight = chainActive.Height();
            nBestTime = chainActive.Tip()->nTime;
        }

        if (nBestHeight < GetNumBlocksOfPeers()-1)
        {
            fIsStaking = false;
            LogPrint(BCLog::POS, "%s: nBestHeight < GetNumBlocksOfPeers(), %d, %d\n", __func__, nBestHeight, GetNumBlocksOfPeers());
            condWaitFor(nThreadID, nMinerSleep * 4);
            continue;
        };

        if (nMinStakeInterval > 0 && nTimeLastStake + (int64_t)nMinStakeInterval > GetTime())
        {
            LogPrint(BCLog::POS, "%s: Rate limited to 1 / %d seconds.\n", __func__, nMinStakeInterval);
            condWaitFor(nThreadID, nMinStakeInterval * 500); // nMinStakeInterval / 2 seconds
            continue;
        };

        int64_t nTime = GetAdjustedTime();
        int64_t nMask = Params().GetStakeTimestampMask(nBestHeight+1);
        int64_t nSearchTime = nTime & ~nMask;
        if (nSearchTime <= nBestTime)
        {
            if (nTime < nBestTime)
            {
                LogPrint(BCLog::POS, "%s: Can't stake before last block time.\n", __func__);
                condWaitFor(nThreadID, std::min(1000 + (nBestTime - nTime) * 1000, (int64_t)30000));
                continue;
            };

            int64_t nNextSearch = nSearchTime + nMask;
            condWaitFor(nThreadID, std::min(nMinerSleep + (nNextSearch - nTime) * 1000, (int64_t)10000));
            continue;
        };

        std::unique_ptr<CBlockTemplate> pblocktemplate;

        size_t nWaitFor = 60000;
        for (size_t i = nStart; i < nEnd; ++i)
        {
            auto pwallet = GetParticlWallet(vpwallets[i].get());

            if (!pwallet->fStakingEnabled)
            {
                pwallet->nIsStaking = CHDWallet::NOT_STAKING_DISABLED;
                continue;
            };

            if (nSearchTime <= pwallet->nLastCoinStakeSearchTime)
            {
                nWaitFor = std::min(nWaitFor, (size_t)nMinerSleep);
                continue;
            };

            if (pwallet->nStakeLimitHeight && nBestHeight >= pwallet->nStakeLimitHeight)
            {
                pwallet->nIsStaking = CHDWallet::NOT_STAKING_LIMITED;
                nWaitFor = std::min(nWaitFor, (size_t)30000);
                continue;
            };

            if (pwallet->IsLocked())
            {
                pwallet->nIsStaking = CHDWallet::NOT_STAKING_LOCKED;
                nWaitFor = std::min(nWaitFor, (size_t)30000);
                continue;
            };

            if (pwallet->GetStakeableBalance() <= pwallet->nReserveBalance)
            {
                pwallet->nIsStaking = CHDWallet::NOT_STAKING_BALANCE;
                nWaitFor = std::min(nWaitFor, (size_t)60000);
                pwallet->nLastCoinStakeSearchTime = nSearchTime + 60;
                LogPrint(BCLog::POS, "%s: Wallet %d, low balance.\n", __func__, i);
                continue;
            };

            if (!pblocktemplate.get())
            {
                pblocktemplate = BlockAssembler(Params()).CreateNewBlock(coinbaseScript, true, false);
                if (!pblocktemplate.get())
                {
                    fIsStaking = false;
                    nWaitFor = std::min(nWaitFor, (size_t)nMinerSleep);
                    LogPrint(BCLog::POS, "%s: Couldn't create new block.\n", __func__);
                    continue;
                };

                if (nBestHeight+1 <= nLastImportHeight
                    && !ImportOutputs(pblocktemplate.get(), nBestHeight+1))
                {
                    fIsStaking = false;
                    nWaitFor = std::min(nWaitFor, (size_t)30000);
                    LogPrint(BCLog::POS, "%s: ImportOutputs failed.\n", __func__);
                    continue;
                };
            };

            pwallet->nIsStaking = CHDWallet::IS_STAKING;
            nWaitFor = nMinerSleep;
            fIsStaking = true;
            if (pwallet->SignBlock(pblocktemplate.get(), nBestHeight+1, nSearchTime))
            {
                CBlock *pblock = &pblocktemplate->block;
                if (CheckStake(pblock))
                {
                     nTimeLastStake = GetTime();
                     break;
                };
            } else
            {
                int nRequiredDepth = std::min((int)(Params().GetStakeMinConfirmations()-1), (int)(nBestHeight / 2));
                if (pwallet->deepestTxnDepth < nRequiredDepth-4)
                {
                    pwallet->nIsStaking = CHDWallet::NOT_STAKING_DEPTH;
                    size_t nSleep = (nRequiredDepth - pwallet->deepestTxnDepth) / 4;
                    nWaitFor = std::min(nWaitFor, (size_t)(nSleep * 1000));
                    pwallet->nLastCoinStakeSearchTime = nSearchTime + nSleep;
                    LogPrint(BCLog::POS, "%s: Wallet %d, no outputs with required depth, sleeping for %ds.\n", __func__, i, nSleep);
                    continue;
                };
            };
        };

        condWaitFor(nThreadID, nWaitFor);
    };
};

