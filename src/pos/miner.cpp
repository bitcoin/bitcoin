// Copyright (c) 2017 The Particl Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pos/miner.h"
#include "pos/kernel.h"
#include "../miner.h"
#include "chainparams.h"
#include "utilmoneystr.h"

#include "sync.h"
#include "net.h"
#include "validation.h"
#include "base58.h"
#include "crypto/sha256.h"

#include <boost/filesystem/operations.hpp>
#include <atomic>
#include <stdint.h>
#include <thread>
#include <condition_variable>

std::atomic<bool> fStopMinerProc;

std::thread threadStakeMiner;

bool fWakeMinerProc = false;
std::condition_variable condMinerProc;
std::mutex mtxMinerProc;

bool fIsStaking = false;
bool fTryToSync = false;

int nMinStakeInterval = 0;  // min stake interval in seconds
int nMinerSleep = 500;
int64_t nLastCoinStakeSearchInterval = 0; // per node
int64_t nLastCoinStakeSearchTime = 0;

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
    
    //// debug print
    LogPrintf("CheckStake(): New proof-of-stake block found  \n  hash: %s \nproofhash: %s  \ntarget: %s\n", hashBlock.GetHex(), proofHash.GetHex(), hashTarget.GetHex());
    LogPrintf(pblock->ToString());
    LogPrintf("out %s\n", FormatMoney(pblock->vtx[0]->GetValueOut()));
    
    
    {
        LOCK(cs_main);
        if (pblock->hashPrevBlock != chainActive.Tip()->GetBlockHash()) // hashbestchain
            return error("%s: Generated block is stale.", __func__);
        
        /* TODO: necessary?
        // Track how many getdata requests this block gets
        {
            LOCK(wallet.cs_wallet);
            wallet.mapRequestCount[hashBlock] = 0;
        }
        */
    }
    
    std::shared_ptr<const CBlock> shared_pblock = std::make_shared<const CBlock>(*pblock);
    if (!ProcessNewBlock(Params(), shared_pblock, true, NULL))
        return error("%s: Block not accepted.", __func__);
    
    return true;
};

bool ImportOutputs(CBlockTemplate *pblocktemplate, int nHeight)
{
    LogPrint("pos", "%s, nHeight %d\n", __func__, nHeight);
    
    CBlock *pblock = &pblocktemplate->block;
    if (pblock->vtx.size() < 1)
        return error("%s: Malformed block.", __func__);
    
    boost::filesystem::path fPath = GetDataDir() / "genesisOutputs.txt";
    
    if (!boost::filesystem::exists(fPath))
        return error("%s: File not found 'genesisOutputs.txt'.", __func__);
    
    
    const int nMaxOutputsPerTxn = 200;
    FILE *fp;
    errno = 0;
    if (!(fp = fopen(fPath.string().c_str(), "r")))
        return error("%s - Can't open file, strerror: %s.", __func__, strerror(errno));
    
    CSHA256 hasher;
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
            || !(pAmount = strtok(NULL, ",")))
            continue;
        
        nOutput++;
        if (nOutput <= nMaxOutputsPerTxn * (nHeight-1))
            continue;
        
        long amount = strtol(pAmount, NULL, 10);
        if (!MoneyRange(amount))
        {
            LogPrintf("Warning: %s - Skipping invalid amount: %s\n", __func__, pAmount);
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
        
        hasher.Write((unsigned char*)&txout->nValue, sizeof(txout->nValue));
        hasher.Write(txout->scriptPubKey.data(), txout->scriptPubKey.size());
        nAdded++;
        if (nAdded >= nMaxOutputsPerTxn)
            break;
    };
    
    fclose(fp);
    
    uint256 hash;
    hasher.Finalize((unsigned char*) &hash);
    
    if (!Params().CheckImportCoinbase(nHeight, hash))
        return error("%s - Incorrect outputs hash.", __func__);
    
    pblock->vtx.push_back(MakeTransactionRef(txn));
    
    return true;
};

void ShutdownThreadStakeMiner()
{
    if (threadStakeMiner.get_id() == std::thread::id()) // no thread created
        return;
    
    LogPrint("pos", "ShutdownThreadStakeMiner\n");
    fStopMinerProc = true;
    
    {
        std::lock_guard<std::mutex> lock(mtxMinerProc);
        fWakeMinerProc = true;
    }
    condMinerProc.notify_one();
    
    threadStakeMiner.join();
};

void WakeThreadStakeMiner()
{
    // call once chain is synced, or wallet unlocked
    LogPrint("pos", "WakeThreadStakeMiner\n");
    
    {
        std::lock_guard<std::mutex> lock(mtxMinerProc);
        fWakeMinerProc = true;
    }
    
    condMinerProc.notify_one();
};

bool ThreadStakeMinerStopped()
{
    return fStopMinerProc;
}

static inline void condWaitFor(int ms)
{
    std::unique_lock<std::mutex> lock(mtxMinerProc);
    fWakeMinerProc = false;
    condMinerProc.wait_for(lock, std::chrono::milliseconds(ms), [] { return fWakeMinerProc; });
};

void ThreadStakeMiner(CHDWallet *pwallet)
{
    fStopMinerProc = false;
    
    int nBestHeight; // TODO: set from new block signal?
    int64_t nBestTime, nTimeLastStake = 0;
    
    nLastCoinStakeSearchTime = GetAdjustedTime(); // startup timestamp
    
    int nLastImportHeight = Params().GetLastImportHeight();
    
    assert(pwallet);
    while (!fStopMinerProc)
    {
        if (pwallet->IsLocked())
        {
            fIsStaking = false;
            condWaitFor(2000);
            continue;
        };
        
        if (g_connman->vNodes.empty() || IsInitialBlockDownload())
        {
            fIsStaking = false;
            fTryToSync = true;
            LogPrint("pos", "%s: IsInitialBlockDownload\n", __func__);
            condWaitFor(2000);
            continue;
        };
        
        if (fTryToSync)
        {
            fTryToSync = false;

            if (g_connman->vNodes.size() < 3 || nBestHeight < GetNumBlocksOfPeers())
            {
                fIsStaking = false;
                LogPrint("pos", "%s: TryToSync\n", __func__);
                condWaitFor(30000);
                continue;
            };
        };
        
        {
            LOCK(cs_main);
            nBestHeight = chainActive.Height();
            nBestTime = chainActive.Tip()->nTime;
        }

        if (nBestHeight < GetNumBlocksOfPeers()-1)
        {
            fIsStaking = false;
            LogPrint("pos", "%s: nBestHeight < GetNumBlocksOfPeers(), %d, %d\n", __func__, nBestHeight, GetNumBlocksOfPeers());
            condWaitFor(nMinerSleep * 4);
            continue;
        };
        
        if (nMinStakeInterval > 0 && nTimeLastStake + (int64_t)nMinStakeInterval > GetTime())
        {
            LogPrint("pos", "%s: Rate limited to 1 / %d seconds.\n", __func__, nMinStakeInterval);
            condWaitFor(nMinStakeInterval * 500); // nMinStakeInterval / 2 seconds
            continue;
        };
        
        
        int64_t nSearchTime = GetAdjustedTime() & ~Params().GetStakeTimestampMask(nBestHeight+1);
        
        if (nSearchTime <= nBestTime)
        {
            fIsStaking = false;
            LogPrint("pos", "%s: Can't stake before last block time.\n", __func__);
            condWaitFor(10000);
            continue;
        };
        
        if (nSearchTime <= pwallet->nLastCoinStakeSearchTime)
        {
            condWaitFor(nMinerSleep);
            continue;
        };
        
        if (pwallet->GetBalance() <= pwallet->nReserveBalance)
        {
            fIsStaking = false;
            LogPrint("pos", "%s: Low balance.\n", __func__);
            condWaitFor(60000);
            continue;
        };
        
        boost::shared_ptr<CReserveScript> coinbaseScript;
        boost::shared_ptr<CReserveKey> rKey(new CReserveKey(pwallet));
        coinbaseScript = rKey;
        
        std::unique_ptr<CBlockTemplate> pblocktemplate(BlockAssembler(Params()).CreateNewBlock(coinbaseScript->reserveScript));
        if (!pblocktemplate.get())
        {
            LogPrint("pos", "%s: Couldn't create new block.\n", __func__);
            condWaitFor(nMinerSleep);
            continue;
        };
        
        if (nBestHeight+1 <= nLastImportHeight
            && !ImportOutputs(pblocktemplate.get(), nBestHeight+1))
        {
            LogPrint("pos", "%s: ImportOutputs failed.\n", __func__);
            condWaitFor(30000);
            continue;
        };
        
        if (pwallet->SignBlock(pblocktemplate.get(), nBestHeight+1, nSearchTime))
        {
            CBlock *pblock = &pblocktemplate->block;
            if (CheckStake(pblock))
            {
                 nTimeLastStake = GetTime();
            };
        } else
        {
            int nRequiredDepth = std::min((int)(Params().GetStakeMinConfirmations()-1), (int)(nBestHeight / 2));
            if (pwallet->deepestTxnDepth < nRequiredDepth-4)
            {
                int nSleep = (nRequiredDepth - pwallet->deepestTxnDepth) / 4;
                LogPrint("pos", "%s: Wallet has no outputs with required depth, sleeping for %ds.\n", __func__, nSleep);
                fIsStaking = false;
                condWaitFor(nSleep * 1000);
                continue;
            }
        }
        
        fIsStaking = true;
        condWaitFor(nMinerSleep);
    };
};

