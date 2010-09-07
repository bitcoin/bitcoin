// Copyright (c) 2009-2010 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#include "headers.h"
#include "cryptopp/sha.h"





//
// Global state
//

CCriticalSection cs_main;

map<uint256, CTransaction> mapTransactions;
CCriticalSection cs_mapTransactions;
unsigned int nTransactionsUpdated = 0;
map<COutPoint, CInPoint> mapNextTx;

map<uint256, CBlockIndex*> mapBlockIndex;
const uint256 hashGenesisBlock("0x000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f");
CBlockIndex* pindexGenesisBlock = NULL;
int nBestHeight = -1;
CBigNum bnBestChainWork = 0;
CBigNum bnBestInvalidWork = 0;
uint256 hashBestChain = 0;
CBlockIndex* pindexBest = NULL;
int64 nTimeBestReceived = 0;

map<uint256, CBlock*> mapOrphanBlocks;
multimap<uint256, CBlock*> mapOrphanBlocksByPrev;

map<uint256, CDataStream*> mapOrphanTransactions;
multimap<uint256, CDataStream*> mapOrphanTransactionsByPrev;

map<uint256, CWalletTx> mapWallet;
vector<uint256> vWalletUpdated;
CCriticalSection cs_mapWallet;

map<vector<unsigned char>, CPrivKey> mapKeys;
map<uint160, vector<unsigned char> > mapPubKeys;
CCriticalSection cs_mapKeys;
CKey keyUser;

map<uint256, int> mapRequestCount;
CCriticalSection cs_mapRequestCount;

map<string, string> mapAddressBook;
CCriticalSection cs_mapAddressBook;

vector<unsigned char> vchDefaultKey;

double dHashesPerSec;
int64 nHPSTimerStart;

// Settings
int fGenerateBitcoins = false;
int64 nTransactionFee = 0;
CAddress addrIncoming;
int fLimitProcessors = false;
int nLimitProcessors = 1;
int fMinimizeToTray = true;
int fMinimizeOnClose = true;






//////////////////////////////////////////////////////////////////////////////
//
// mapKeys
//

bool AddKey(const CKey& key)
{
    CRITICAL_BLOCK(cs_mapKeys)
    {
        mapKeys[key.GetPubKey()] = key.GetPrivKey();
        mapPubKeys[Hash160(key.GetPubKey())] = key.GetPubKey();
    }
    return CWalletDB().WriteKey(key.GetPubKey(), key.GetPrivKey());
}

vector<unsigned char> GenerateNewKey()
{
    RandAddSeedPerfmon();
    CKey key;
    key.MakeNewKey();
    if (!AddKey(key))
        throw runtime_error("GenerateNewKey() : AddKey failed");
    return key.GetPubKey();
}




//////////////////////////////////////////////////////////////////////////////
//
// mapWallet
//

bool AddToWallet(const CWalletTx& wtxIn)
{
    uint256 hash = wtxIn.GetHash();
    CRITICAL_BLOCK(cs_mapWallet)
    {
        // Inserts only if not already there, returns tx inserted or tx found
        pair<map<uint256, CWalletTx>::iterator, bool> ret = mapWallet.insert(make_pair(hash, wtxIn));
        CWalletTx& wtx = (*ret.first).second;
        bool fInsertedNew = ret.second;
        if (fInsertedNew)
            wtx.nTimeReceived = GetAdjustedTime();

        bool fUpdated = false;
        if (!fInsertedNew)
        {
            // Merge
            if (wtxIn.hashBlock != 0 && wtxIn.hashBlock != wtx.hashBlock)
            {
                wtx.hashBlock = wtxIn.hashBlock;
                fUpdated = true;
            }
            if (wtxIn.nIndex != -1 && (wtxIn.vMerkleBranch != wtx.vMerkleBranch || wtxIn.nIndex != wtx.nIndex))
            {
                wtx.vMerkleBranch = wtxIn.vMerkleBranch;
                wtx.nIndex = wtxIn.nIndex;
                fUpdated = true;
            }
            if (wtxIn.fFromMe && wtxIn.fFromMe != wtx.fFromMe)
            {
                wtx.fFromMe = wtxIn.fFromMe;
                fUpdated = true;
            }
            if (wtxIn.fSpent && wtxIn.fSpent != wtx.fSpent)
            {
                wtx.fSpent = wtxIn.fSpent;
                fUpdated = true;
            }
        }

        //// debug print
        printf("AddToWallet %s  %s%s\n", wtxIn.GetHash().ToString().substr(0,6).c_str(), (fInsertedNew ? "new" : ""), (fUpdated ? "update" : ""));

        // Write to disk
        if (fInsertedNew || fUpdated)
            if (!wtx.WriteToDisk())
                return false;

        // If default receiving address gets used, replace it with a new one
        CScript scriptDefaultKey;
        scriptDefaultKey.SetBitcoinAddress(vchDefaultKey);
        foreach(const CTxOut& txout, wtx.vout)
        {
            if (txout.scriptPubKey == scriptDefaultKey)
            {
                CWalletDB walletdb;
                walletdb.WriteDefaultKey(GenerateNewKey());
                walletdb.WriteName(PubKeyToAddress(vchDefaultKey), "");
            }
        }

        // Notify UI
        vWalletUpdated.push_back(hash);
    }

    // Refresh UI
    MainFrameRepaint();
    return true;
}

bool AddToWalletIfMine(const CTransaction& tx, const CBlock* pblock)
{
    if (tx.IsMine() || mapWallet.count(tx.GetHash()))
    {
        CWalletTx wtx(tx);
        // Get merkle branch if transaction was found in a block
        if (pblock)
            wtx.SetMerkleBranch(pblock);
        return AddToWallet(wtx);
    }
    return true;
}

bool EraseFromWallet(uint256 hash)
{
    CRITICAL_BLOCK(cs_mapWallet)
    {
        if (mapWallet.erase(hash))
            CWalletDB().EraseTx(hash);
    }
    return true;
}

void WalletUpdateSpent(const COutPoint& prevout)
{
    // Anytime a signature is successfully verified, it's proof the outpoint is spent.
    // Update the wallet spent flag if it doesn't know due to wallet.dat being
    // restored from backup or the user making copies of wallet.dat.
    CRITICAL_BLOCK(cs_mapWallet)
    {
        map<uint256, CWalletTx>::iterator mi = mapWallet.find(prevout.hash);
        if (mi != mapWallet.end())
        {
            CWalletTx& wtx = (*mi).second;
            if (!wtx.fSpent && wtx.vout[prevout.n].IsMine())
            {
                printf("WalletUpdateSpent found spent coin %sbc %s\n", FormatMoney(wtx.GetCredit()).c_str(), wtx.GetHash().ToString().c_str());
                wtx.fSpent = true;
                wtx.WriteToDisk();
                vWalletUpdated.push_back(prevout.hash);
            }
        }
    }
}








//////////////////////////////////////////////////////////////////////////////
//
// mapOrphanTransactions
//

void AddOrphanTx(const CDataStream& vMsg)
{
    CTransaction tx;
    CDataStream(vMsg) >> tx;
    uint256 hash = tx.GetHash();
    if (mapOrphanTransactions.count(hash))
        return;
    CDataStream* pvMsg = mapOrphanTransactions[hash] = new CDataStream(vMsg);
    foreach(const CTxIn& txin, tx.vin)
        mapOrphanTransactionsByPrev.insert(make_pair(txin.prevout.hash, pvMsg));
}

void EraseOrphanTx(uint256 hash)
{
    if (!mapOrphanTransactions.count(hash))
        return;
    const CDataStream* pvMsg = mapOrphanTransactions[hash];
    CTransaction tx;
    CDataStream(*pvMsg) >> tx;
    foreach(const CTxIn& txin, tx.vin)
    {
        for (multimap<uint256, CDataStream*>::iterator mi = mapOrphanTransactionsByPrev.lower_bound(txin.prevout.hash);
             mi != mapOrphanTransactionsByPrev.upper_bound(txin.prevout.hash);)
        {
            if ((*mi).second == pvMsg)
                mapOrphanTransactionsByPrev.erase(mi++);
            else
                mi++;
        }
    }
    delete pvMsg;
    mapOrphanTransactions.erase(hash);
}








//////////////////////////////////////////////////////////////////////////////
//
// CTransaction
//

bool CTxIn::IsMine() const
{
    CRITICAL_BLOCK(cs_mapWallet)
    {
        map<uint256, CWalletTx>::iterator mi = mapWallet.find(prevout.hash);
        if (mi != mapWallet.end())
        {
            const CWalletTx& prev = (*mi).second;
            if (prevout.n < prev.vout.size())
                if (prev.vout[prevout.n].IsMine())
                    return true;
        }
    }
    return false;
}

int64 CTxIn::GetDebit() const
{
    CRITICAL_BLOCK(cs_mapWallet)
    {
        map<uint256, CWalletTx>::iterator mi = mapWallet.find(prevout.hash);
        if (mi != mapWallet.end())
        {
            const CWalletTx& prev = (*mi).second;
            if (prevout.n < prev.vout.size())
                if (prev.vout[prevout.n].IsMine())
                    return prev.vout[prevout.n].nValue;
        }
    }
    return 0;
}

int64 CWalletTx::GetTxTime() const
{
    if (!fTimeReceivedIsTxTime && hashBlock != 0)
    {
        // If we did not receive the transaction directly, we rely on the block's
        // time to figure out when it happened.  We use the median over a range
        // of blocks to try to filter out inaccurate block times.
        map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hashBlock);
        if (mi != mapBlockIndex.end())
        {
            CBlockIndex* pindex = (*mi).second;
            if (pindex)
                return pindex->GetMedianTime();
        }
    }
    return nTimeReceived;
}

int CWalletTx::GetRequestCount() const
{
    // Returns -1 if it wasn't being tracked
    int nRequests = -1;
    CRITICAL_BLOCK(cs_mapRequestCount)
    {
        if (IsCoinBase())
        {
            // Generated block
            if (hashBlock != 0)
            {
                map<uint256, int>::iterator mi = mapRequestCount.find(hashBlock);
                if (mi != mapRequestCount.end())
                    nRequests = (*mi).second;
            }
        }
        else
        {
            // Did anyone request this transaction?
            map<uint256, int>::iterator mi = mapRequestCount.find(GetHash());
            if (mi != mapRequestCount.end())
            {
                nRequests = (*mi).second;

                // How about the block it's in?
                if (nRequests == 0 && hashBlock != 0)
                {
                    map<uint256, int>::iterator mi = mapRequestCount.find(hashBlock);
                    if (mi != mapRequestCount.end())
                        nRequests = (*mi).second;
                    else
                        nRequests = 1; // If it's in someone else's block it must have got out
                }
            }
        }
    }
    return nRequests;
}




int CMerkleTx::SetMerkleBranch(const CBlock* pblock)
{
    if (fClient)
    {
        if (hashBlock == 0)
            return 0;
    }
    else
    {
        CBlock blockTmp;
        if (pblock == NULL)
        {
            // Load the block this tx is in
            CTxIndex txindex;
            if (!CTxDB("r").ReadTxIndex(GetHash(), txindex))
                return 0;
            if (!blockTmp.ReadFromDisk(txindex.pos.nFile, txindex.pos.nBlockPos))
                return 0;
            pblock = &blockTmp;
        }

        // Update the tx's hashBlock
        hashBlock = pblock->GetHash();

        // Locate the transaction
        for (nIndex = 0; nIndex < pblock->vtx.size(); nIndex++)
            if (pblock->vtx[nIndex] == *(CTransaction*)this)
                break;
        if (nIndex == pblock->vtx.size())
        {
            vMerkleBranch.clear();
            nIndex = -1;
            printf("ERROR: SetMerkleBranch() : couldn't find tx in block\n");
            return 0;
        }

        // Fill in merkle branch
        vMerkleBranch = pblock->GetMerkleBranch(nIndex);
    }

    // Is the tx in a block that's in the main chain
    map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hashBlock);
    if (mi == mapBlockIndex.end())
        return 0;
    CBlockIndex* pindex = (*mi).second;
    if (!pindex || !pindex->IsInMainChain())
        return 0;

    return pindexBest->nHeight - pindex->nHeight + 1;
}



void CWalletTx::AddSupportingTransactions(CTxDB& txdb)
{
    vtxPrev.clear();

    const int COPY_DEPTH = 3;
    if (SetMerkleBranch() < COPY_DEPTH)
    {
        vector<uint256> vWorkQueue;
        foreach(const CTxIn& txin, vin)
            vWorkQueue.push_back(txin.prevout.hash);

        // This critsect is OK because txdb is already open
        CRITICAL_BLOCK(cs_mapWallet)
        {
            map<uint256, const CMerkleTx*> mapWalletPrev;
            set<uint256> setAlreadyDone;
            for (int i = 0; i < vWorkQueue.size(); i++)
            {
                uint256 hash = vWorkQueue[i];
                if (setAlreadyDone.count(hash))
                    continue;
                setAlreadyDone.insert(hash);

                CMerkleTx tx;
                if (mapWallet.count(hash))
                {
                    tx = mapWallet[hash];
                    foreach(const CMerkleTx& txWalletPrev, mapWallet[hash].vtxPrev)
                        mapWalletPrev[txWalletPrev.GetHash()] = &txWalletPrev;
                }
                else if (mapWalletPrev.count(hash))
                {
                    tx = *mapWalletPrev[hash];
                }
                else if (!fClient && txdb.ReadDiskTx(hash, tx))
                {
                    ;
                }
                else
                {
                    printf("ERROR: AddSupportingTransactions() : unsupported transaction\n");
                    continue;
                }

                int nDepth = tx.SetMerkleBranch();
                vtxPrev.push_back(tx);

                if (nDepth < COPY_DEPTH)
                    foreach(const CTxIn& txin, tx.vin)
                        vWorkQueue.push_back(txin.prevout.hash);
            }
        }
    }

    reverse(vtxPrev.begin(), vtxPrev.end());
}











bool CTransaction::AcceptToMemoryPool(CTxDB& txdb, bool fCheckInputs, bool* pfMissingInputs)
{
    if (pfMissingInputs)
        *pfMissingInputs = false;

    // Coinbase is only valid in a block, not as a loose transaction
    if (IsCoinBase())
        return error("AcceptToMemoryPool() : coinbase as individual tx");

    if (!CheckTransaction())
        return error("AcceptToMemoryPool() : CheckTransaction failed");

    // To help v0.1.5 clients who would see it as a negative number
    if ((int64)nLockTime > INT_MAX)
        return error("AcceptToMemoryPool() : not accepting nLockTime beyond 2038 yet");

    // Rather not work on nonstandard transactions
    if (GetSigOpCount() > 2 || ::GetSerializeSize(*this, SER_NETWORK) < 100)
        return error("AcceptToMemoryPool() : nonstandard transaction");

    // Do we already have it?
    uint256 hash = GetHash();
    CRITICAL_BLOCK(cs_mapTransactions)
        if (mapTransactions.count(hash))
            return false;
    if (fCheckInputs)
        if (txdb.ContainsTx(hash))
            return false;

    // Check for conflicts with in-memory transactions
    CTransaction* ptxOld = NULL;
    for (int i = 0; i < vin.size(); i++)
    {
        COutPoint outpoint = vin[i].prevout;
        if (mapNextTx.count(outpoint))
        {
            // Disable replacement feature for now
            return false;

            // Allow replacing with a newer version of the same transaction
            if (i != 0)
                return false;
            ptxOld = mapNextTx[outpoint].ptx;
            if (!IsNewerThan(*ptxOld))
                return false;
            for (int i = 0; i < vin.size(); i++)
            {
                COutPoint outpoint = vin[i].prevout;
                if (!mapNextTx.count(outpoint) || mapNextTx[outpoint].ptx != ptxOld)
                    return false;
            }
            break;
        }
    }

    // Check against previous transactions
    map<uint256, CTxIndex> mapUnused;
    int64 nFees = 0;
    if (fCheckInputs && !ConnectInputs(txdb, mapUnused, CDiskTxPos(1,1,1), pindexBest, nFees, false, false))
    {
        if (pfMissingInputs)
            *pfMissingInputs = true;
        return error("AcceptToMemoryPool() : ConnectInputs failed %s", hash.ToString().substr(0,6).c_str());
    }

    // Store transaction in memory
    CRITICAL_BLOCK(cs_mapTransactions)
    {
        if (ptxOld)
        {
            printf("AcceptToMemoryPool() : replacing tx %s with new version\n", ptxOld->GetHash().ToString().c_str());
            ptxOld->RemoveFromMemoryPool();
        }
        AddToMemoryPoolUnchecked();
    }

    ///// are we sure this is ok when loading transactions or restoring block txes
    // If updated, erase old tx from wallet
    if (ptxOld)
        EraseFromWallet(ptxOld->GetHash());

    printf("AcceptToMemoryPool(): accepted %s\n", hash.ToString().substr(0,6).c_str());
    return true;
}


bool CTransaction::AddToMemoryPoolUnchecked()
{
    // Add to memory pool without checking anything.  Don't call this directly,
    // call AcceptToMemoryPool to properly check the transaction first.
    CRITICAL_BLOCK(cs_mapTransactions)
    {
        uint256 hash = GetHash();
        mapTransactions[hash] = *this;
        for (int i = 0; i < vin.size(); i++)
            mapNextTx[vin[i].prevout] = CInPoint(&mapTransactions[hash], i);
        nTransactionsUpdated++;
    }
    return true;
}


bool CTransaction::RemoveFromMemoryPool()
{
    // Remove transaction from memory pool
    CRITICAL_BLOCK(cs_mapTransactions)
    {
        foreach(const CTxIn& txin, vin)
            mapNextTx.erase(txin.prevout);
        mapTransactions.erase(GetHash());
        nTransactionsUpdated++;
    }
    return true;
}






int CMerkleTx::GetDepthInMainChain(int& nHeightRet) const
{
    if (hashBlock == 0 || nIndex == -1)
        return 0;

    // Find the block it claims to be in
    map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hashBlock);
    if (mi == mapBlockIndex.end())
        return 0;
    CBlockIndex* pindex = (*mi).second;
    if (!pindex || !pindex->IsInMainChain())
        return 0;

    // Make sure the merkle branch connects to this block
    if (!fMerkleVerified)
    {
        if (CBlock::CheckMerkleBranch(GetHash(), vMerkleBranch, nIndex) != pindex->hashMerkleRoot)
            return 0;
        fMerkleVerified = true;
    }

    nHeightRet = pindex->nHeight;
    return pindexBest->nHeight - pindex->nHeight + 1;
}


int CMerkleTx::GetBlocksToMaturity() const
{
    if (!IsCoinBase())
        return 0;
    return max(0, (COINBASE_MATURITY+20) - GetDepthInMainChain());
}


bool CMerkleTx::AcceptToMemoryPool(CTxDB& txdb, bool fCheckInputs)
{
    if (fClient)
    {
        if (!IsInMainChain() && !ClientConnectInputs())
            return false;
        return CTransaction::AcceptToMemoryPool(txdb, false);
    }
    else
    {
        return CTransaction::AcceptToMemoryPool(txdb, fCheckInputs);
    }
}



bool CWalletTx::AcceptWalletTransaction(CTxDB& txdb, bool fCheckInputs)
{
    CRITICAL_BLOCK(cs_mapTransactions)
    {
        // Add previous supporting transactions first
        foreach(CMerkleTx& tx, vtxPrev)
        {
            if (!tx.IsCoinBase())
            {
                uint256 hash = tx.GetHash();
                if (!mapTransactions.count(hash) && !txdb.ContainsTx(hash))
                    tx.AcceptToMemoryPool(txdb, fCheckInputs);
            }
        }
        return AcceptToMemoryPool(txdb, fCheckInputs);
    }
    return false;
}

void ReacceptWalletTransactions()
{
    CTxDB txdb("r");
    CRITICAL_BLOCK(cs_mapWallet)
    {
        foreach(PAIRTYPE(const uint256, CWalletTx)& item, mapWallet)
        {
            CWalletTx& wtx = item.second;
            if (wtx.fSpent && wtx.IsCoinBase())
                continue;

            CTxIndex txindex;
            if (txdb.ReadTxIndex(wtx.GetHash(), txindex))
            {
                // Update fSpent if a tx got spent somewhere else by a copy of wallet.dat
                if (!wtx.fSpent)
                {
                    if (txindex.vSpent.size() != wtx.vout.size())
                    {
                        printf("ERROR: ReacceptWalletTransactions() : txindex.vSpent.size() %d != wtx.vout.size() %d\n", txindex.vSpent.size(), wtx.vout.size());
                        continue;
                    }
                    for (int i = 0; i < txindex.vSpent.size(); i++)
                    {
                        if (!txindex.vSpent[i].IsNull() && wtx.vout[i].IsMine())
                        {
                            printf("ReacceptWalletTransactions found spent coin %sbc %s\n", FormatMoney(wtx.GetCredit()).c_str(), wtx.GetHash().ToString().c_str());
                            wtx.fSpent = true;
                            wtx.WriteToDisk();
                            break;
                        }
                    }
                }
            }
            else
            {
                // Reaccept any txes of ours that aren't already in a block
                if (!wtx.IsCoinBase())
                    wtx.AcceptWalletTransaction(txdb, false);
            }
        }
    }
}


void CWalletTx::RelayWalletTransaction(CTxDB& txdb)
{
    foreach(const CMerkleTx& tx, vtxPrev)
    {
        if (!tx.IsCoinBase())
        {
            uint256 hash = tx.GetHash();
            if (!txdb.ContainsTx(hash))
                RelayMessage(CInv(MSG_TX, hash), (CTransaction)tx);
        }
    }
    if (!IsCoinBase())
    {
        uint256 hash = GetHash();
        if (!txdb.ContainsTx(hash))
        {
            printf("Relaying wtx %s\n", hash.ToString().substr(0,6).c_str());
            RelayMessage(CInv(MSG_TX, hash), (CTransaction)*this);
        }
    }
}

void ResendWalletTransactions()
{
    // Do this infrequently and randomly to avoid giving away
    // that these are our transactions.
    static int64 nNextTime;
    if (GetTime() < nNextTime)
        return;
    bool fFirst = (nNextTime == 0);
    nNextTime = GetTime() + GetRand(30 * 60);
    if (fFirst)
        return;

    // Only do it if there's been a new block since last time
    static int64 nLastTime;
    if (nTimeBestReceived < nLastTime)
        return;
    nLastTime = GetTime();

    // Rebroadcast any of our txes that aren't in a block yet
    printf("ResendWalletTransactions()\n");
    CTxDB txdb("r");
    CRITICAL_BLOCK(cs_mapWallet)
    {
        // Sort them in chronological order
        multimap<unsigned int, CWalletTx*> mapSorted;
        foreach(PAIRTYPE(const uint256, CWalletTx)& item, mapWallet)
        {
            CWalletTx& wtx = item.second;
            // Don't rebroadcast until it's had plenty of time that
            // it should have gotten in already by now.
            if (nTimeBestReceived - (int64)wtx.nTimeReceived > 5 * 60)
                mapSorted.insert(make_pair(wtx.nTimeReceived, &wtx));
        }
        foreach(PAIRTYPE(const unsigned int, CWalletTx*)& item, mapSorted)
        {
            CWalletTx& wtx = *item.second;
            wtx.RelayWalletTransaction(txdb);
        }
    }
}










//////////////////////////////////////////////////////////////////////////////
//
// CBlock and CBlockIndex
//

bool CBlock::ReadFromDisk(const CBlockIndex* pindex, bool fReadTransactions)
{
    if (!ReadFromDisk(pindex->nFile, pindex->nBlockPos, fReadTransactions))
        return false;
    if (GetHash() != pindex->GetBlockHash())
        return error("CBlock::ReadFromDisk() : GetHash() doesn't match index");
    return true;
}

uint256 GetOrphanRoot(const CBlock* pblock)
{
    // Work back to the first block in the orphan chain
    while (mapOrphanBlocks.count(pblock->hashPrevBlock))
        pblock = mapOrphanBlocks[pblock->hashPrevBlock];
    return pblock->GetHash();
}

int64 CBlock::GetBlockValue(int nHeight, int64 nFees) const
{
    int64 nSubsidy = 50 * COIN;

    // Subsidy is cut in half every 4 years
    nSubsidy >>= (nHeight / 210000);

    return nSubsidy + nFees;
}

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast)
{
    const int64 nTargetTimespan = 14 * 24 * 60 * 60; // two weeks
    const int64 nTargetSpacing = 10 * 60;
    const int64 nInterval = nTargetTimespan / nTargetSpacing;

    // Genesis block
    if (pindexLast == NULL)
        return bnProofOfWorkLimit.GetCompact();

    // Only change once per interval
    if ((pindexLast->nHeight+1) % nInterval != 0)
        return pindexLast->nBits;

    // Go back by what we want to be 14 days worth of blocks
    const CBlockIndex* pindexFirst = pindexLast;
    for (int i = 0; pindexFirst && i < nInterval-1; i++)
        pindexFirst = pindexFirst->pprev;
    assert(pindexFirst);

    // Limit adjustment step
    int64 nActualTimespan = pindexLast->GetBlockTime() - pindexFirst->GetBlockTime();
    printf("  nActualTimespan = %"PRI64d"  before bounds\n", nActualTimespan);
    if (nActualTimespan < nTargetTimespan/4)
        nActualTimespan = nTargetTimespan/4;
    if (nActualTimespan > nTargetTimespan*4)
        nActualTimespan = nTargetTimespan*4;

    // Retarget
    CBigNum bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew *= nActualTimespan;
    bnNew /= nTargetTimespan;

    if (bnNew > bnProofOfWorkLimit)
        bnNew = bnProofOfWorkLimit;

    /// debug print
    printf("GetNextWorkRequired RETARGET\n");
    printf("nTargetTimespan = %"PRI64d"    nActualTimespan = %"PRI64d"\n", nTargetTimespan, nActualTimespan);
    printf("Before: %08x  %s\n", pindexLast->nBits, CBigNum().SetCompact(pindexLast->nBits).getuint256().ToString().c_str());
    printf("After:  %08x  %s\n", bnNew.GetCompact(), bnNew.getuint256().ToString().c_str());

    return bnNew.GetCompact();
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits)
{
    CBigNum bnTarget;
    bnTarget.SetCompact(nBits);

    // Check range
    if (bnTarget <= 0 || bnTarget > bnProofOfWorkLimit)
        return error("CheckProofOfWork() : nBits below minimum work");

    // Check proof of work matches claimed amount
    if (hash > bnTarget.getuint256())
        return error("CheckProofOfWork() : hash doesn't match nBits");

    return true;
}

bool IsInitialBlockDownload()
{
    if (pindexBest == NULL)
        return true;
    static int64 nLastUpdate;
    static CBlockIndex* pindexLastBest;
    if (pindexBest != pindexLastBest)
    {
        pindexLastBest = pindexBest;
        nLastUpdate = GetTime();
    }
    return (GetTime() - nLastUpdate < 10 &&
            pindexBest->GetBlockTime() < GetTime() - 24 * 60 * 60);
}

void InvalidChainFound(CBlockIndex* pindexNew)
{
    if (pindexNew->bnChainWork > bnBestInvalidWork)
    {
        bnBestInvalidWork = pindexNew->bnChainWork;
        CTxDB().WriteBestInvalidWork(bnBestInvalidWork);
        MainFrameRepaint();
    }
    printf("InvalidChainFound: invalid block=%s  height=%d  work=%s\n", pindexNew->GetBlockHash().ToString().substr(0,20).c_str(), pindexNew->nHeight, pindexNew->bnChainWork.ToString().c_str());
    printf("InvalidChainFound:  current best=%s  height=%d  work=%s\n", hashBestChain.ToString().substr(0,20).c_str(), nBestHeight, bnBestChainWork.ToString().c_str());
    if (pindexBest && bnBestInvalidWork > bnBestChainWork + pindexBest->GetBlockWork() * 6)
        printf("InvalidChainFound: WARNING: Displayed transactions may not be correct!  You may need to upgrade, or other nodes may need to upgrade.\n");
}











bool CTransaction::DisconnectInputs(CTxDB& txdb)
{
    // Relinquish previous transactions' spent pointers
    if (!IsCoinBase())
    {
        foreach(const CTxIn& txin, vin)
        {
            COutPoint prevout = txin.prevout;

            // Get prev txindex from disk
            CTxIndex txindex;
            if (!txdb.ReadTxIndex(prevout.hash, txindex))
                return error("DisconnectInputs() : ReadTxIndex failed");

            if (prevout.n >= txindex.vSpent.size())
                return error("DisconnectInputs() : prevout.n out of range");

            // Mark outpoint as not spent
            txindex.vSpent[prevout.n].SetNull();

            // Write back
            txdb.UpdateTxIndex(prevout.hash, txindex);
        }
    }

    // Remove transaction from index
    if (!txdb.EraseTxIndex(*this))
        return error("DisconnectInputs() : EraseTxPos failed");

    return true;
}


bool CTransaction::ConnectInputs(CTxDB& txdb, map<uint256, CTxIndex>& mapTestPool, CDiskTxPos posThisTx,
                                 CBlockIndex* pindexBlock, int64& nFees, bool fBlock, bool fMiner, int64 nMinFee)
{
    // Take over previous transactions' spent pointers
    if (!IsCoinBase())
    {
        int64 nValueIn = 0;
        for (int i = 0; i < vin.size(); i++)
        {
            COutPoint prevout = vin[i].prevout;

            // Read txindex
            CTxIndex txindex;
            bool fFound = true;
            if (fMiner && mapTestPool.count(prevout.hash))
            {
                // Get txindex from current proposed changes
                txindex = mapTestPool[prevout.hash];
            }
            else
            {
                // Read txindex from txdb
                fFound = txdb.ReadTxIndex(prevout.hash, txindex);
            }
            if (!fFound && (fBlock || fMiner))
                return fMiner ? false : error("ConnectInputs() : %s prev tx %s index entry not found", GetHash().ToString().substr(0,6).c_str(),  prevout.hash.ToString().substr(0,6).c_str());

            // Read txPrev
            CTransaction txPrev;
            if (!fFound || txindex.pos == CDiskTxPos(1,1,1))
            {
                // Get prev tx from single transactions in memory
                CRITICAL_BLOCK(cs_mapTransactions)
                {
                    if (!mapTransactions.count(prevout.hash))
                        return error("ConnectInputs() : %s mapTransactions prev not found %s", GetHash().ToString().substr(0,6).c_str(),  prevout.hash.ToString().substr(0,6).c_str());
                    txPrev = mapTransactions[prevout.hash];
                }
                if (!fFound)
                    txindex.vSpent.resize(txPrev.vout.size());
            }
            else
            {
                // Get prev tx from disk
                if (!txPrev.ReadFromDisk(txindex.pos))
                    return error("ConnectInputs() : %s ReadFromDisk prev tx %s failed", GetHash().ToString().substr(0,6).c_str(),  prevout.hash.ToString().substr(0,6).c_str());
            }

            if (prevout.n >= txPrev.vout.size() || prevout.n >= txindex.vSpent.size())
                return error("ConnectInputs() : %s prevout.n out of range %d %d %d prev tx %s\n%s", GetHash().ToString().substr(0,6).c_str(), prevout.n, txPrev.vout.size(), txindex.vSpent.size(), prevout.hash.ToString().substr(0,6).c_str(), txPrev.ToString().c_str());

            // If prev is coinbase, check that it's matured
            if (txPrev.IsCoinBase())
                for (CBlockIndex* pindex = pindexBlock; pindex && pindexBlock->nHeight - pindex->nHeight < COINBASE_MATURITY; pindex = pindex->pprev)
                    if (pindex->nBlockPos == txindex.pos.nBlockPos && pindex->nFile == txindex.pos.nFile)
                        return error("ConnectInputs() : tried to spend coinbase at depth %d", pindexBlock->nHeight - pindex->nHeight);

            // Verify signature
            if (!VerifySignature(txPrev, *this, i))
                return error("ConnectInputs() : %s VerifySignature failed", GetHash().ToString().substr(0,6).c_str());

            // Check for conflicts
            if (!txindex.vSpent[prevout.n].IsNull())
                return fMiner ? false : error("ConnectInputs() : %s prev tx already used at %s", GetHash().ToString().substr(0,6).c_str(), txindex.vSpent[prevout.n].ToString().c_str());

            // Mark outpoints as spent
            txindex.vSpent[prevout.n] = posThisTx;

            // Write back
            if (fBlock)
                txdb.UpdateTxIndex(prevout.hash, txindex);
            else if (fMiner)
                mapTestPool[prevout.hash] = txindex;

            nValueIn += txPrev.vout[prevout.n].nValue;

            // Check for negative or overflow input values
            if (!MoneyRange(txPrev.vout[prevout.n].nValue) || !MoneyRange(nValueIn))
                return error("ConnectInputs() : txin values out of range");
        }

        if (nValueIn < GetValueOut())
            return error("ConnectInputs() : %s value in < value out", GetHash().ToString().substr(0,6).c_str());

        // Tally transaction fees
        int64 nTxFee = nValueIn - GetValueOut();
        if (nTxFee < 0)
            return error("ConnectInputs() : %s nTxFee < 0", GetHash().ToString().substr(0,6).c_str());
        if (nTxFee < nMinFee)
            return false;
        nFees += nTxFee;
        if (!MoneyRange(nFees))
            return error("ConnectInputs() : nFees out of range");
    }

    if (fBlock)
    {
        // Add transaction to disk index
        if (!txdb.AddTxIndex(*this, posThisTx, pindexBlock->nHeight))
            return error("ConnectInputs() : AddTxPos failed");
    }
    else if (fMiner)
    {
        // Add transaction to test pool
        mapTestPool[GetHash()] = CTxIndex(CDiskTxPos(1,1,1), vout.size());
    }

    return true;
}


bool CTransaction::ClientConnectInputs()
{
    if (IsCoinBase())
        return false;

    // Take over previous transactions' spent pointers
    CRITICAL_BLOCK(cs_mapTransactions)
    {
        int64 nValueIn = 0;
        for (int i = 0; i < vin.size(); i++)
        {
            // Get prev tx from single transactions in memory
            COutPoint prevout = vin[i].prevout;
            if (!mapTransactions.count(prevout.hash))
                return false;
            CTransaction& txPrev = mapTransactions[prevout.hash];

            if (prevout.n >= txPrev.vout.size())
                return false;

            // Verify signature
            if (!VerifySignature(txPrev, *this, i))
                return error("ConnectInputs() : VerifySignature failed");

            ///// this is redundant with the mapNextTx stuff, not sure which I want to get rid of
            ///// this has to go away now that posNext is gone
            // // Check for conflicts
            // if (!txPrev.vout[prevout.n].posNext.IsNull())
            //     return error("ConnectInputs() : prev tx already used");
            //
            // // Flag outpoints as used
            // txPrev.vout[prevout.n].posNext = posThisTx;

            nValueIn += txPrev.vout[prevout.n].nValue;

            if (!MoneyRange(txPrev.vout[prevout.n].nValue) || !MoneyRange(nValueIn))
                return error("ClientConnectInputs() : txin values out of range");
        }
        if (GetValueOut() > nValueIn)
            return false;
    }

    return true;
}




bool CBlock::DisconnectBlock(CTxDB& txdb, CBlockIndex* pindex)
{
    // Disconnect in reverse order
    for (int i = vtx.size()-1; i >= 0; i--)
        if (!vtx[i].DisconnectInputs(txdb))
            return false;

    // Update block index on disk without changing it in memory.
    // The memory index structure will be changed after the db commits.
    if (pindex->pprev)
    {
        CDiskBlockIndex blockindexPrev(pindex->pprev);
        blockindexPrev.hashNext = 0;
        txdb.WriteBlockIndex(blockindexPrev);
    }

    return true;
}

bool CBlock::ConnectBlock(CTxDB& txdb, CBlockIndex* pindex)
{
    // Check it again in case a previous version let a bad block in
    if (!CheckBlock())
        return false;

    //// issue here: it doesn't know the version
    unsigned int nTxPos = pindex->nBlockPos + ::GetSerializeSize(CBlock(), SER_DISK) - 1 + GetSizeOfCompactSize(vtx.size());

    map<uint256, CTxIndex> mapUnused;
    int64 nFees = 0;
    foreach(CTransaction& tx, vtx)
    {
        CDiskTxPos posThisTx(pindex->nFile, pindex->nBlockPos, nTxPos);
        nTxPos += ::GetSerializeSize(tx, SER_DISK);

        if (!tx.ConnectInputs(txdb, mapUnused, posThisTx, pindex, nFees, true, false))
            return false;
    }

    if (vtx[0].GetValueOut() > GetBlockValue(pindex->nHeight, nFees))
        return false;

    // Update block index on disk without changing it in memory.
    // The memory index structure will be changed after the db commits.
    if (pindex->pprev)
    {
        CDiskBlockIndex blockindexPrev(pindex->pprev);
        blockindexPrev.hashNext = pindex->GetBlockHash();
        txdb.WriteBlockIndex(blockindexPrev);
    }

    // Watch for transactions paying to me
    foreach(CTransaction& tx, vtx)
        AddToWalletIfMine(tx, this);

    return true;
}



bool Reorganize(CTxDB& txdb, CBlockIndex* pindexNew)
{
    printf("REORGANIZE\n");

    // Find the fork
    CBlockIndex* pfork = pindexBest;
    CBlockIndex* plonger = pindexNew;
    while (pfork != plonger)
    {
        while (plonger->nHeight > pfork->nHeight)
            if (!(plonger = plonger->pprev))
                return error("Reorganize() : plonger->pprev is null");
        if (pfork == plonger)
            break;
        if (!(pfork = pfork->pprev))
            return error("Reorganize() : pfork->pprev is null");
    }

    // List of what to disconnect
    vector<CBlockIndex*> vDisconnect;
    for (CBlockIndex* pindex = pindexBest; pindex != pfork; pindex = pindex->pprev)
        vDisconnect.push_back(pindex);

    // List of what to connect
    vector<CBlockIndex*> vConnect;
    for (CBlockIndex* pindex = pindexNew; pindex != pfork; pindex = pindex->pprev)
        vConnect.push_back(pindex);
    reverse(vConnect.begin(), vConnect.end());

    // Disconnect shorter branch
    vector<CTransaction> vResurrect;
    foreach(CBlockIndex* pindex, vDisconnect)
    {
        CBlock block;
        if (!block.ReadFromDisk(pindex))
            return error("Reorganize() : ReadFromDisk for disconnect failed");
        if (!block.DisconnectBlock(txdb, pindex))
            return error("Reorganize() : DisconnectBlock failed");

        // Queue memory transactions to resurrect
        foreach(const CTransaction& tx, block.vtx)
            if (!tx.IsCoinBase())
                vResurrect.push_back(tx);
    }

    // Connect longer branch
    vector<CTransaction> vDelete;
    for (int i = 0; i < vConnect.size(); i++)
    {
        CBlockIndex* pindex = vConnect[i];
        CBlock block;
        if (!block.ReadFromDisk(pindex))
            return error("Reorganize() : ReadFromDisk for connect failed");
        if (!block.ConnectBlock(txdb, pindex))
        {
            // Invalid block
            txdb.TxnAbort();
            return error("Reorganize() : ConnectBlock failed");
        }

        // Queue memory transactions to delete
        foreach(const CTransaction& tx, block.vtx)
            vDelete.push_back(tx);
    }
    if (!txdb.WriteHashBestChain(pindexNew->GetBlockHash()))
        return error("Reorganize() : WriteHashBestChain failed");

    // Commit now because resurrecting could take some time
    txdb.TxnCommit();

    // Disconnect shorter branch
    foreach(CBlockIndex* pindex, vDisconnect)
        if (pindex->pprev)
            pindex->pprev->pnext = NULL;

    // Connect longer branch
    foreach(CBlockIndex* pindex, vConnect)
        if (pindex->pprev)
            pindex->pprev->pnext = pindex;

    // Resurrect memory transactions that were in the disconnected branch
    foreach(CTransaction& tx, vResurrect)
        tx.AcceptToMemoryPool(txdb, false);

    // Delete redundant memory transactions that are in the connected branch
    foreach(CTransaction& tx, vDelete)
        tx.RemoveFromMemoryPool();

    return true;
}


bool CBlock::SetBestChain(CTxDB& txdb, CBlockIndex* pindexNew)
{
    uint256 hash = GetHash();

    txdb.TxnBegin();
    if (pindexGenesisBlock == NULL && hash == hashGenesisBlock)
    {
        pindexGenesisBlock = pindexNew;
        txdb.WriteHashBestChain(hash);
    }
    else if (hashPrevBlock == hashBestChain)
    {
        // Adding to current best branch
        if (!ConnectBlock(txdb, pindexNew) || !txdb.WriteHashBestChain(hash))
        {
            txdb.TxnAbort();
            InvalidChainFound(pindexNew);
            return error("SetBestChain() : ConnectBlock failed");
        }
        txdb.TxnCommit();
        pindexNew->pprev->pnext = pindexNew;

        // Delete redundant memory transactions
        foreach(CTransaction& tx, vtx)
            tx.RemoveFromMemoryPool();
    }
    else
    {
        // New best branch
        if (!Reorganize(txdb, pindexNew))
        {
            txdb.TxnAbort();
            InvalidChainFound(pindexNew);
            return error("SetBestChain() : Reorganize failed");
        }
    }
    txdb.TxnCommit();

    // New best block
    hashBestChain = hash;
    pindexBest = pindexNew;
    nBestHeight = pindexBest->nHeight;
    bnBestChainWork = pindexNew->bnChainWork;
    nTimeBestReceived = GetTime();
    nTransactionsUpdated++;
    printf("SetBestChain: new best=%s  height=%d  work=%s\n", hashBestChain.ToString().substr(0,20).c_str(), nBestHeight, bnBestChainWork.ToString().c_str());

    return true;
}


bool CBlock::AddToBlockIndex(unsigned int nFile, unsigned int nBlockPos)
{
    // Check for duplicate
    uint256 hash = GetHash();
    if (mapBlockIndex.count(hash))
        return error("AddToBlockIndex() : %s already exists", hash.ToString().substr(0,20).c_str());

    // Construct new block index object
    CBlockIndex* pindexNew = new CBlockIndex(nFile, nBlockPos, *this);
    if (!pindexNew)
        return error("AddToBlockIndex() : new CBlockIndex failed");
    map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.insert(make_pair(hash, pindexNew)).first;
    pindexNew->phashBlock = &((*mi).first);
    map<uint256, CBlockIndex*>::iterator miPrev = mapBlockIndex.find(hashPrevBlock);
    if (miPrev != mapBlockIndex.end())
    {
        pindexNew->pprev = (*miPrev).second;
        pindexNew->nHeight = pindexNew->pprev->nHeight + 1;
    }
    pindexNew->bnChainWork = (pindexNew->pprev ? pindexNew->pprev->bnChainWork : 0) + pindexNew->GetBlockWork();

    CTxDB txdb;
    txdb.WriteBlockIndex(CDiskBlockIndex(pindexNew));

    // New best
    if (pindexNew->bnChainWork > bnBestChainWork)
        if (!SetBestChain(txdb, pindexNew))
            return false;

    txdb.Close();

    if (pindexNew == pindexBest)
    {
        // Notify UI to display prev block's coinbase if it was ours
        static uint256 hashPrevBestCoinBase;
        CRITICAL_BLOCK(cs_mapWallet)
            vWalletUpdated.push_back(hashPrevBestCoinBase);
        hashPrevBestCoinBase = vtx[0].GetHash();
    }

    MainFrameRepaint();
    return true;
}




bool CBlock::CheckBlock() const
{
    // These are checks that are independent of context
    // that can be verified before saving an orphan block.

    // Size limits
    if (vtx.empty() || vtx.size() > MAX_SIZE || ::GetSerializeSize(*this, SER_NETWORK) > MAX_SIZE)
        return error("CheckBlock() : size limits failed");

    // Check timestamp
    if (GetBlockTime() > GetAdjustedTime() + 2 * 60 * 60)
        return error("CheckBlock() : block timestamp too far in the future");

    // First transaction must be coinbase, the rest must not be
    if (vtx.empty() || !vtx[0].IsCoinBase())
        return error("CheckBlock() : first tx is not coinbase");
    for (int i = 1; i < vtx.size(); i++)
        if (vtx[i].IsCoinBase())
            return error("CheckBlock() : more than one coinbase");

    // Check transactions
    foreach(const CTransaction& tx, vtx)
        if (!tx.CheckTransaction())
            return error("CheckBlock() : CheckTransaction failed");

    // Check proof of work matches claimed amount
    if (!CheckProofOfWork(GetHash(), nBits))
        return error("CheckBlock() : proof of work failed");

    // Check merkleroot
    if (hashMerkleRoot != BuildMerkleTree())
        return error("CheckBlock() : hashMerkleRoot mismatch");

    return true;
}

bool CBlock::AcceptBlock()
{
    // Check for duplicate
    uint256 hash = GetHash();
    if (mapBlockIndex.count(hash))
        return error("AcceptBlock() : block already in mapBlockIndex");

    // Get prev block index
    map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hashPrevBlock);
    if (mi == mapBlockIndex.end())
        return error("AcceptBlock() : prev block not found");
    CBlockIndex* pindexPrev = (*mi).second;
    int nHeight = pindexPrev->nHeight+1;

    // Check size
    if (nHeight > 79400 && ::GetSerializeSize(*this, SER_NETWORK) > MAX_BLOCK_SIZE)
        return error("AcceptBlock() : over size limit");

    // Check that it's not full of nonstandard transactions
    if (nHeight > 79400 && GetSigOpCount() > MAX_BLOCK_SIGOPS)
        return error("AcceptBlock() : too many nonstandard transactions");

    // Check timestamp against prev
    if (GetBlockTime() <= pindexPrev->GetMedianTimePast())
        return error("AcceptBlock() : block's timestamp is too early");

    // Check that all transactions are finalized
    foreach(const CTransaction& tx, vtx)
        if (!tx.IsFinal(nHeight, GetBlockTime()))
            return error("AcceptBlock() : contains a non-final transaction");

    // Check proof of work
    if (nBits != GetNextWorkRequired(pindexPrev))
        return error("AcceptBlock() : incorrect proof of work");

    // Check that the block chain matches the known block chain up to a checkpoint
    if ((nHeight == 11111 && hash != uint256("0x0000000069e244f73d78e8fd29ba2fd2ed618bd6fa2ee92559f542fdb26e7c1d")) ||
        (nHeight == 33333 && hash != uint256("0x000000002dd5588a74784eaa7ab0507a18ad16a236e7b1ce69f00d7ddfb5d0a6")) ||
        (nHeight == 68555 && hash != uint256("0x00000000001e1b4903550a0b96e9a9405c8a95f387162e4944e8d9fbe501cd6a")) ||
        (nHeight == 70567 && hash != uint256("0x00000000006a49b14bcf27462068f1264c961f11fa2e0eddd2be0791e1d4124a")) ||
        (nHeight == 74000 && hash != uint256("0x0000000000573993a3c9e41ce34471c079dcf5f52a0e824a81e7f953b8661a20")))
        return error("AcceptBlock() : rejected by checkpoint lockin at %d", nHeight);

    // Scanback checkpoint lockin
    for (CBlockIndex* pindex = pindexPrev; pindex->nHeight >= 74000; pindex = pindex->pprev)
    {
        if (pindex->nHeight == 74000 && pindex->GetBlockHash() != uint256("0x0000000000573993a3c9e41ce34471c079dcf5f52a0e824a81e7f953b8661a20"))
            return error("AcceptBlock() : rejected by scanback lockin at %d", pindex->nHeight);
        if (pindex->nHeight == 74638 && pindex->GetBlockHash() == uint256("0x0000000000790ab3f22ec756ad43b6ab569abf0bddeb97c67a6f7b1470a7ec1c"))
            return error("AcceptBlock() : rejected by scanback lockin at %d", pindex->nHeight);
    }

    // Write block to history file
    if (!CheckDiskSpace(::GetSerializeSize(*this, SER_DISK)))
        return error("AcceptBlock() : out of disk space");
    unsigned int nFile;
    unsigned int nBlockPos;
    if (!WriteToDisk(!fClient, nFile, nBlockPos))
        return error("AcceptBlock() : WriteToDisk failed");
    if (!AddToBlockIndex(nFile, nBlockPos))
        return error("AcceptBlock() : AddToBlockIndex failed");

    // Relay inventory, but don't relay old inventory during initial block download
    if (hashBestChain == hash)
        CRITICAL_BLOCK(cs_vNodes)
            foreach(CNode* pnode, vNodes)
                if (nBestHeight > (pnode->nStartingHeight != -1 ? pnode->nStartingHeight - 2000 : 55000))
                    pnode->PushInventory(CInv(MSG_BLOCK, hash));

    return true;
}

bool ProcessBlock(CNode* pfrom, CBlock* pblock)
{
    // Check for duplicate
    uint256 hash = pblock->GetHash();
    if (mapBlockIndex.count(hash))
        return error("ProcessBlock() : already have block %d %s", mapBlockIndex[hash]->nHeight, hash.ToString().substr(0,20).c_str());
    if (mapOrphanBlocks.count(hash))
        return error("ProcessBlock() : already have block (orphan) %s", hash.ToString().substr(0,20).c_str());

    // Preliminary checks
    if (!pblock->CheckBlock())
    {
        delete pblock;
        return error("ProcessBlock() : CheckBlock FAILED");
    }

    // If don't already have its previous block, shunt it off to holding area until we get it
    if (!mapBlockIndex.count(pblock->hashPrevBlock))
    {
        printf("ProcessBlock: ORPHAN BLOCK, prev=%s\n", pblock->hashPrevBlock.ToString().substr(0,20).c_str());
        mapOrphanBlocks.insert(make_pair(hash, pblock));
        mapOrphanBlocksByPrev.insert(make_pair(pblock->hashPrevBlock, pblock));

        // Ask this guy to fill in what we're missing
        if (pfrom)
            pfrom->PushGetBlocks(pindexBest, GetOrphanRoot(pblock));
        return true;
    }

    // Store to disk
    if (!pblock->AcceptBlock())
    {
        delete pblock;
        return error("ProcessBlock() : AcceptBlock FAILED");
    }
    delete pblock;

    // Recursively process any orphan blocks that depended on this one
    vector<uint256> vWorkQueue;
    vWorkQueue.push_back(hash);
    for (int i = 0; i < vWorkQueue.size(); i++)
    {
        uint256 hashPrev = vWorkQueue[i];
        for (multimap<uint256, CBlock*>::iterator mi = mapOrphanBlocksByPrev.lower_bound(hashPrev);
             mi != mapOrphanBlocksByPrev.upper_bound(hashPrev);
             ++mi)
        {
            CBlock* pblockOrphan = (*mi).second;
            if (pblockOrphan->AcceptBlock())
                vWorkQueue.push_back(pblockOrphan->GetHash());
            mapOrphanBlocks.erase(pblockOrphan->GetHash());
            delete pblockOrphan;
        }
        mapOrphanBlocksByPrev.erase(hashPrev);
    }

    printf("ProcessBlock: ACCEPTED\n");
    return true;
}








template<typename Stream>
bool ScanMessageStart(Stream& s)
{
    // Scan ahead to the next pchMessageStart, which should normally be immediately
    // at the file pointer.  Leaves file pointer at end of pchMessageStart.
    s.clear(0);
    short prevmask = s.exceptions(0);
    const char* p = BEGIN(pchMessageStart);
    try
    {
        loop
        {
            char c;
            s.read(&c, 1);
            if (s.fail())
            {
                s.clear(0);
                s.exceptions(prevmask);
                return false;
            }
            if (*p != c)
                p = BEGIN(pchMessageStart);
            if (*p == c)
            {
                if (++p == END(pchMessageStart))
                {
                    s.clear(0);
                    s.exceptions(prevmask);
                    return true;
                }
            }
        }
    }
    catch (...)
    {
        s.clear(0);
        s.exceptions(prevmask);
        return false;
    }
}

bool CheckDiskSpace(uint64 nAdditionalBytes)
{
    uint64 nFreeBytesAvailable = filesystem::space(GetDataDir()).available;

    // Check for 15MB because database could create another 10MB log file at any time
    if (nFreeBytesAvailable < (uint64)15000000 + nAdditionalBytes)
    {
        fShutdown = true;
        string strMessage = _("Warning: Disk space is low  ");
        strMiscWarning = strMessage;
        printf("*** %s\n", strMessage.c_str());
        ThreadSafeMessageBox(strMessage, "Bitcoin", wxOK | wxICON_EXCLAMATION);
        CreateThread(Shutdown, NULL);
        return false;
    }
    return true;
}

FILE* OpenBlockFile(unsigned int nFile, unsigned int nBlockPos, const char* pszMode)
{
    if (nFile == -1)
        return NULL;
    FILE* file = fopen(strprintf("%s/blk%04d.dat", GetDataDir().c_str(), nFile).c_str(), pszMode);
    if (!file)
        return NULL;
    if (nBlockPos != 0 && !strchr(pszMode, 'a') && !strchr(pszMode, 'w'))
    {
        if (fseek(file, nBlockPos, SEEK_SET) != 0)
        {
            fclose(file);
            return NULL;
        }
    }
    return file;
}

static unsigned int nCurrentBlockFile = 1;

FILE* AppendBlockFile(unsigned int& nFileRet)
{
    nFileRet = 0;
    loop
    {
        FILE* file = OpenBlockFile(nCurrentBlockFile, 0, "ab");
        if (!file)
            return NULL;
        if (fseek(file, 0, SEEK_END) != 0)
            return NULL;
        // FAT32 filesize max 4GB, fseek and ftell max 2GB, so we must stay under 2GB
        if (ftell(file) < 0x7F000000 - MAX_SIZE)
        {
            nFileRet = nCurrentBlockFile;
            return file;
        }
        fclose(file);
        nCurrentBlockFile++;
    }
}

bool LoadBlockIndex(bool fAllowNew)
{
    //
    // Load block index
    //
    CTxDB txdb("cr");
    if (!txdb.LoadBlockIndex())
        return false;
    txdb.Close();

    //
    // Init with genesis block
    //
    if (mapBlockIndex.empty())
    {
        if (!fAllowNew)
            return false;

        // Genesis Block:
        // CBlock(hash=000000000019d6, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=4a5e1e, nTime=1231006505, nBits=1d00ffff, nNonce=2083236893, vtx=1)
        //   CTransaction(hash=4a5e1e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
        //     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73)
        //     CTxOut(nValue=50.00000000, scriptPubKey=0x5F1DF16B2B704C8A578D0B)
        //   vMerkleTree: 4a5e1e

        // Genesis block
        const char* pszTimestamp = "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks";
        CTransaction txNew;
        txNew.vin.resize(1);
        txNew.vout.resize(1);
        txNew.vin[0].scriptSig = CScript() << 486604799 << CBigNum(4) << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
        txNew.vout[0].nValue = 50 * COIN;
        txNew.vout[0].scriptPubKey = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
        CBlock block;
        block.vtx.push_back(txNew);
        block.hashPrevBlock = 0;
        block.hashMerkleRoot = block.BuildMerkleTree();
        block.nVersion = 1;
        block.nTime    = 1231006505;
        block.nBits    = 0x1d00ffff;
        block.nNonce   = 2083236893;

            //// debug print
            printf("%s\n", block.GetHash().ToString().c_str());
            printf("%s\n", hashGenesisBlock.ToString().c_str());
            printf("%s\n", block.hashMerkleRoot.ToString().c_str());
            assert(block.hashMerkleRoot == uint256("0x4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b"));
            block.print();

        assert(block.GetHash() == hashGenesisBlock);

        // Start new block file
        unsigned int nFile;
        unsigned int nBlockPos;
        if (!block.WriteToDisk(!fClient, nFile, nBlockPos))
            return error("LoadBlockIndex() : writing genesis block to disk failed");
        if (!block.AddToBlockIndex(nFile, nBlockPos))
            return error("LoadBlockIndex() : genesis block not accepted");
    }

    return true;
}



void PrintBlockTree()
{
    // precompute tree structure
    map<CBlockIndex*, vector<CBlockIndex*> > mapNext;
    for (map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.begin(); mi != mapBlockIndex.end(); ++mi)
    {
        CBlockIndex* pindex = (*mi).second;
        mapNext[pindex->pprev].push_back(pindex);
        // test
        //while (rand() % 3 == 0)
        //    mapNext[pindex->pprev].push_back(pindex);
    }

    vector<pair<int, CBlockIndex*> > vStack;
    vStack.push_back(make_pair(0, pindexGenesisBlock));

    int nPrevCol = 0;
    while (!vStack.empty())
    {
        int nCol = vStack.back().first;
        CBlockIndex* pindex = vStack.back().second;
        vStack.pop_back();

        // print split or gap
        if (nCol > nPrevCol)
        {
            for (int i = 0; i < nCol-1; i++)
                printf("| ");
            printf("|\\\n");
        }
        else if (nCol < nPrevCol)
        {
            for (int i = 0; i < nCol; i++)
                printf("| ");
            printf("|\n");
        }
        nPrevCol = nCol;

        // print columns
        for (int i = 0; i < nCol; i++)
            printf("| ");

        // print item
        CBlock block;
        block.ReadFromDisk(pindex);
        printf("%d (%u,%u) %s  %s  tx %d",
            pindex->nHeight,
            pindex->nFile,
            pindex->nBlockPos,
            block.GetHash().ToString().substr(0,20).c_str(),
            DateTimeStrFormat("%x %H:%M:%S", block.GetBlockTime()).c_str(),
            block.vtx.size());

        CRITICAL_BLOCK(cs_mapWallet)
        {
            if (mapWallet.count(block.vtx[0].GetHash()))
            {
                CWalletTx& wtx = mapWallet[block.vtx[0].GetHash()];
                printf("    mine:  %d  %d  %d", wtx.GetDepthInMainChain(), wtx.GetBlocksToMaturity(), wtx.GetCredit());
            }
        }
        printf("\n");


        // put the main timechain first
        vector<CBlockIndex*>& vNext = mapNext[pindex];
        for (int i = 0; i < vNext.size(); i++)
        {
            if (vNext[i]->pnext)
            {
                swap(vNext[0], vNext[i]);
                break;
            }
        }

        // iterate children
        for (int i = 0; i < vNext.size(); i++)
            vStack.push_back(make_pair(nCol+i, vNext[i]));
    }
}










//////////////////////////////////////////////////////////////////////////////
//
// CAlert
//

map<uint256, CAlert> mapAlerts;
CCriticalSection cs_mapAlerts;

string GetWarnings(string strFor)
{
    int nPriority = 0;
    string strStatusBar;
    string strRPC;
    if (mapArgs.count("-testsafemode"))
        strRPC = "test";

    // Misc warnings like out of disk space and clock is wrong
    if (strMiscWarning != "")
    {
        nPriority = 1000;
        strStatusBar = strMiscWarning;
    }

    // Longer invalid proof-of-work chain
    if (pindexBest && bnBestInvalidWork > bnBestChainWork + pindexBest->GetBlockWork() * 6)
    {
        nPriority = 2000;
        strStatusBar = strRPC = "WARNING: Displayed transactions may not be correct!  You may need to upgrade, or other nodes may need to upgrade.";
    }

    // Alerts
    CRITICAL_BLOCK(cs_mapAlerts)
    {
        foreach(PAIRTYPE(const uint256, CAlert)& item, mapAlerts)
        {
            const CAlert& alert = item.second;
            if (alert.AppliesToMe() && alert.nPriority > nPriority)
            {
                nPriority = alert.nPriority;
                strStatusBar = alert.strStatusBar;
                strRPC = alert.strRPCError;
            }
        }
    }

    if (strFor == "statusbar")
        return strStatusBar;
    else if (strFor == "rpc")
        return strRPC;
    assert(("GetWarnings() : invalid parameter", false));
    return "error";
}

bool CAlert::ProcessAlert()
{
    if (!CheckSignature())
        return false;
    if (!IsInEffect())
        return false;

    CRITICAL_BLOCK(cs_mapAlerts)
    {
        // Cancel previous alerts
        for (map<uint256, CAlert>::iterator mi = mapAlerts.begin(); mi != mapAlerts.end();)
        {
            const CAlert& alert = (*mi).second;
            if (Cancels(alert))
            {
                printf("cancelling alert %d\n", alert.nID);
                mapAlerts.erase(mi++);
            }
            else if (!alert.IsInEffect())
            {
                printf("expiring alert %d\n", alert.nID);
                mapAlerts.erase(mi++);
            }
            else
                mi++;
        }

        // Check if this alert has been cancelled
        foreach(PAIRTYPE(const uint256, CAlert)& item, mapAlerts)
        {
            const CAlert& alert = item.second;
            if (alert.Cancels(*this))
            {
                printf("alert already cancelled by %d\n", alert.nID);
                return false;
            }
        }

        // Add to mapAlerts
        mapAlerts.insert(make_pair(GetHash(), *this));
    }

    printf("accepted alert %d, AppliesToMe()=%d\n", nID, AppliesToMe());
    MainFrameRepaint();
    return true;
}








//////////////////////////////////////////////////////////////////////////////
//
// Messages
//


bool AlreadyHave(CTxDB& txdb, const CInv& inv)
{
    switch (inv.type)
    {
    case MSG_TX:    return mapTransactions.count(inv.hash) || txdb.ContainsTx(inv.hash);
    case MSG_BLOCK: return mapBlockIndex.count(inv.hash) || mapOrphanBlocks.count(inv.hash);
    }
    // Don't know what it is, just say we already got one
    return true;
}







bool ProcessMessages(CNode* pfrom)
{
    CDataStream& vRecv = pfrom->vRecv;
    if (vRecv.empty())
        return true;
    //if (fDebug)
    //    printf("ProcessMessages(%u bytes)\n", vRecv.size());

    //
    // Message format
    //  (4) message start
    //  (12) command
    //  (4) size
    //  (4) checksum
    //  (x) data
    //

    loop
    {
        // Scan for message start
        CDataStream::iterator pstart = search(vRecv.begin(), vRecv.end(), BEGIN(pchMessageStart), END(pchMessageStart));
        int nHeaderSize = vRecv.GetSerializeSize(CMessageHeader());
        if (vRecv.end() - pstart < nHeaderSize)
        {
            if (vRecv.size() > nHeaderSize)
            {
                printf("\n\nPROCESSMESSAGE MESSAGESTART NOT FOUND\n\n");
                vRecv.erase(vRecv.begin(), vRecv.end() - nHeaderSize);
            }
            break;
        }
        if (pstart - vRecv.begin() > 0)
            printf("\n\nPROCESSMESSAGE SKIPPED %d BYTES\n\n", pstart - vRecv.begin());
        vRecv.erase(vRecv.begin(), pstart);

        // Read header
        vector<char> vHeaderSave(vRecv.begin(), vRecv.begin() + nHeaderSize);
        CMessageHeader hdr;
        vRecv >> hdr;
        if (!hdr.IsValid())
        {
            printf("\n\nPROCESSMESSAGE: ERRORS IN HEADER %s\n\n\n", hdr.GetCommand().c_str());
            continue;
        }
        string strCommand = hdr.GetCommand();

        // Message size
        unsigned int nMessageSize = hdr.nMessageSize;
        if (nMessageSize > MAX_SIZE)
        {
            printf("ProcessMessage(%s, %u bytes) : nMessageSize > MAX_SIZE\n", strCommand.c_str(), nMessageSize);
            continue;
        }
        if (nMessageSize > vRecv.size())
        {
            // Rewind and wait for rest of message
            ///// need a mechanism to give up waiting for overlong message size error
            vRecv.insert(vRecv.begin(), vHeaderSave.begin(), vHeaderSave.end());
            break;
        }

        // Copy message to its own buffer
        CDataStream vMsg(vRecv.begin(), vRecv.begin() + nMessageSize, vRecv.nType, vRecv.nVersion);
        vRecv.ignore(nMessageSize);

        // Checksum
        if (vRecv.GetVersion() >= 209)
        {
            uint256 hash = Hash(vMsg.begin(), vMsg.end());
            unsigned int nChecksum = 0;
            memcpy(&nChecksum, &hash, sizeof(nChecksum));
            if (nChecksum != hdr.nChecksum)
            {
                printf("ProcessMessage(%s, %u bytes) : CHECKSUM ERROR nChecksum=%08x hdr.nChecksum=%08x\n",
                       strCommand.c_str(), nMessageSize, nChecksum, hdr.nChecksum);
                continue;
            }
        }

        // Process message
        bool fRet = false;
        try
        {
            CRITICAL_BLOCK(cs_main)
                fRet = ProcessMessage(pfrom, strCommand, vMsg);
            if (fShutdown)
                return true;
        }
        catch (std::ios_base::failure& e)
        {
            if (strstr(e.what(), "end of data"))
            {
                // Allow exceptions from underlength message on vRecv
                printf("ProcessMessage(%s, %u bytes) : Exception '%s' caught, normally caused by a message being shorter than its stated length\n", strCommand.c_str(), nMessageSize, e.what());
            }
            else if (strstr(e.what(), "size too large"))
            {
                // Allow exceptions from overlong size
                printf("ProcessMessage(%s, %u bytes) : Exception '%s' caught\n", strCommand.c_str(), nMessageSize, e.what());
            }
            else
            {
                PrintExceptionContinue(&e, "ProcessMessage()");
            }
        }
        catch (std::exception& e) {
            PrintExceptionContinue(&e, "ProcessMessage()");
        } catch (...) {
            PrintExceptionContinue(NULL, "ProcessMessage()");
        }

        if (!fRet)
            printf("ProcessMessage(%s, %u bytes) FAILED\n", strCommand.c_str(), nMessageSize);
    }

    vRecv.Compact();
    return true;
}




bool ProcessMessage(CNode* pfrom, string strCommand, CDataStream& vRecv)
{
    static map<unsigned int, vector<unsigned char> > mapReuseKey;
    RandAddSeedPerfmon();
    if (fDebug)
        printf("%s ", DateTimeStrFormat("%x %H:%M:%S", GetTime()).c_str());
    printf("received: %s (%d bytes)\n", strCommand.c_str(), vRecv.size());
    if (mapArgs.count("-dropmessagestest") && GetRand(atoi(mapArgs["-dropmessagestest"])) == 0)
    {
        printf("dropmessagestest DROPPING RECV MESSAGE\n");
        return true;
    }





    if (strCommand == "version")
    {
        // Each connection can only send one version message
        if (pfrom->nVersion != 0)
            return false;

        int64 nTime;
        CAddress addrMe;
        CAddress addrFrom;
        uint64 nNonce = 1;
        vRecv >> pfrom->nVersion >> pfrom->nServices >> nTime >> addrMe;
        if (pfrom->nVersion == 10300)
            pfrom->nVersion = 300;
        if (pfrom->nVersion >= 106 && !vRecv.empty())
            vRecv >> addrFrom >> nNonce;
        if (pfrom->nVersion >= 106 && !vRecv.empty())
            vRecv >> pfrom->strSubVer;
        if (pfrom->nVersion >= 209 && !vRecv.empty())
            vRecv >> pfrom->nStartingHeight;

        if (pfrom->nVersion == 0)
            return false;

        // Disconnect if we connected to ourself
        if (nNonce == nLocalHostNonce && nNonce > 1)
        {
            pfrom->fDisconnect = true;
            return true;
        }

        pfrom->fClient = !(pfrom->nServices & NODE_NETWORK);
        if (pfrom->fClient)
        {
            pfrom->vSend.nType |= SER_BLOCKHEADERONLY;
            pfrom->vRecv.nType |= SER_BLOCKHEADERONLY;
        }

        AddTimeData(pfrom->addr.ip, nTime);

        // Change version
        if (pfrom->nVersion >= 209)
            pfrom->PushMessage("verack");
        pfrom->vSend.SetVersion(min(pfrom->nVersion, VERSION));
        if (pfrom->nVersion < 209)
            pfrom->vRecv.SetVersion(min(pfrom->nVersion, VERSION));

        // Ask the first connected node for block updates
        static int nAskedForBlocks;
        if (!pfrom->fClient && (nAskedForBlocks < 1 || vNodes.size() <= 1))
        {
            nAskedForBlocks++;
            pfrom->PushGetBlocks(pindexBest, uint256(0));
        }

        // Relay alerts
        CRITICAL_BLOCK(cs_mapAlerts)
            foreach(PAIRTYPE(const uint256, CAlert)& item, mapAlerts)
                item.second.RelayTo(pfrom);

        pfrom->fSuccessfullyConnected = true;

        printf("version message: version %d, blocks=%d\n", pfrom->nVersion, pfrom->nStartingHeight);
    }


    else if (pfrom->nVersion == 0)
    {
        // Must have a version message before anything else
        return false;
    }


    else if (strCommand == "verack")
    {
        pfrom->vRecv.SetVersion(min(pfrom->nVersion, VERSION));
    }


    else if (strCommand == "addr")
    {
        vector<CAddress> vAddr;
        vRecv >> vAddr;
        if (pfrom->nVersion < 200) // don't want addresses from 0.1.5
            return true;
        if (pfrom->nVersion < 209 && mapAddresses.size() > 1000) // don't want addr from 0.2.0 unless seeding
            return true;
        if (vAddr.size() > 1000)
            return error("message addr size() = %d", vAddr.size());

        // Store the new addresses
        foreach(CAddress& addr, vAddr)
        {
            if (fShutdown)
                return true;
            // ignore IPv6 for now, since it isn't implemented anyway
            if (!addr.IsIPv4())
                continue;
            addr.nTime = GetAdjustedTime() - 2 * 60 * 60;
            if (pfrom->fGetAddr || vAddr.size() > 10)
                addr.nTime -= 5 * 24 * 60 * 60;
            AddAddress(addr);
            pfrom->AddAddressKnown(addr);
            if (!pfrom->fGetAddr && addr.IsRoutable())
            {
                // Relay to a limited number of other nodes
                CRITICAL_BLOCK(cs_vNodes)
                {
                    // Use deterministic randomness to send to
                    // the same places for 12 hours at a time
                    static uint256 hashSalt;
                    if (hashSalt == 0)
                        RAND_bytes((unsigned char*)&hashSalt, sizeof(hashSalt));
                    uint256 hashRand = addr.ip ^ ((GetTime()+addr.ip)/(12*60*60)) ^ hashSalt;
                    multimap<uint256, CNode*> mapMix;
                    foreach(CNode* pnode, vNodes)
                        mapMix.insert(make_pair(hashRand = Hash(BEGIN(hashRand), END(hashRand)), pnode));
                    int nRelayNodes = 2;
                    for (multimap<uint256, CNode*>::iterator mi = mapMix.begin(); mi != mapMix.end() && nRelayNodes-- > 0; ++mi)
                        ((*mi).second)->PushAddress(addr);
                }
            }
        }
        if (vAddr.size() < 1000)
            pfrom->fGetAddr = false;
    }


    else if (strCommand == "inv")
    {
        vector<CInv> vInv;
        vRecv >> vInv;
        if (vInv.size() > 50000)
            return error("message inv size() = %d", vInv.size());

        CTxDB txdb("r");
        foreach(const CInv& inv, vInv)
        {
            if (fShutdown)
                return true;
            pfrom->AddInventoryKnown(inv);

            bool fAlreadyHave = AlreadyHave(txdb, inv);
            printf("  got inventory: %s  %s\n", inv.ToString().c_str(), fAlreadyHave ? "have" : "new");

            if (!fAlreadyHave)
                pfrom->AskFor(inv);
            else if (inv.type == MSG_BLOCK && mapOrphanBlocks.count(inv.hash))
                pfrom->PushGetBlocks(pindexBest, GetOrphanRoot(mapOrphanBlocks[inv.hash]));

            // Track requests for our stuff
            CRITICAL_BLOCK(cs_mapRequestCount)
            {
                map<uint256, int>::iterator mi = mapRequestCount.find(inv.hash);
                if (mi != mapRequestCount.end())
                    (*mi).second++;
            }
        }
    }


    else if (strCommand == "getdata")
    {
        vector<CInv> vInv;
        vRecv >> vInv;
        if (vInv.size() > 50000)
            return error("message getdata size() = %d", vInv.size());

        foreach(const CInv& inv, vInv)
        {
            if (fShutdown)
                return true;
            printf("received getdata for: %s\n", inv.ToString().c_str());

            if (inv.type == MSG_BLOCK)
            {
                // Send block from disk
                map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(inv.hash);
                if (mi != mapBlockIndex.end())
                {
                    //// could optimize this to send header straight from blockindex for client
                    CBlock block;
                    block.ReadFromDisk((*mi).second, !pfrom->fClient);
                    pfrom->PushMessage("block", block);

                    // Trigger them to send a getblocks request for the next batch of inventory
                    if (inv.hash == pfrom->hashContinue)
                    {
                        // Bypass PushInventory, this must send even if redundant,
                        // and we want it right after the last block so they don't
                        // wait for other stuff first.
                        vector<CInv> vInv;
                        vInv.push_back(CInv(MSG_BLOCK, hashBestChain));
                        pfrom->PushMessage("inv", vInv);
                        pfrom->hashContinue = 0;
                    }
                }
            }
            else if (inv.IsKnownType())
            {
                // Send stream from relay memory
                CRITICAL_BLOCK(cs_mapRelay)
                {
                    map<CInv, CDataStream>::iterator mi = mapRelay.find(inv);
                    if (mi != mapRelay.end())
                        pfrom->PushMessage(inv.GetCommand(), (*mi).second);
                }
            }

            // Track requests for our stuff
            CRITICAL_BLOCK(cs_mapRequestCount)
            {
                map<uint256, int>::iterator mi = mapRequestCount.find(inv.hash);
                if (mi != mapRequestCount.end())
                    (*mi).second++;
            }
        }
    }


    else if (strCommand == "getblocks")
    {
        CBlockLocator locator;
        uint256 hashStop;
        vRecv >> locator >> hashStop;

        // Find the first block the caller has in the main chain
        CBlockIndex* pindex = locator.GetBlockIndex();

        // Send the rest of the chain
        if (pindex)
            pindex = pindex->pnext;
        int nLimit = 500 + locator.GetDistanceBack();
        printf("getblocks %d to %s limit %d\n", (pindex ? pindex->nHeight : -1), hashStop.ToString().substr(0,20).c_str(), nLimit);
        for (; pindex; pindex = pindex->pnext)
        {
            if (pindex->GetBlockHash() == hashStop)
            {
                printf("  getblocks stopping at %d %s\n", pindex->nHeight, pindex->GetBlockHash().ToString().substr(0,20).c_str());
                break;
            }
            pfrom->PushInventory(CInv(MSG_BLOCK, pindex->GetBlockHash()));
            if (--nLimit <= 0)
            {
                // When this block is requested, we'll send an inv that'll make them
                // getblocks the next batch of inventory.
                printf("  getblocks stopping at limit %d %s\n", pindex->nHeight, pindex->GetBlockHash().ToString().substr(0,20).c_str());
                pfrom->hashContinue = pindex->GetBlockHash();
                break;
            }
        }
    }


    else if (strCommand == "tx")
    {
        vector<uint256> vWorkQueue;
        CDataStream vMsg(vRecv);
        CTransaction tx;
        vRecv >> tx;

        CInv inv(MSG_TX, tx.GetHash());
        pfrom->AddInventoryKnown(inv);

        bool fMissingInputs = false;
        if (tx.AcceptToMemoryPool(true, &fMissingInputs))
        {
            AddToWalletIfMine(tx, NULL);
            RelayMessage(inv, vMsg);
            mapAlreadyAskedFor.erase(inv);
            vWorkQueue.push_back(inv.hash);

            // Recursively process any orphan transactions that depended on this one
            for (int i = 0; i < vWorkQueue.size(); i++)
            {
                uint256 hashPrev = vWorkQueue[i];
                for (multimap<uint256, CDataStream*>::iterator mi = mapOrphanTransactionsByPrev.lower_bound(hashPrev);
                     mi != mapOrphanTransactionsByPrev.upper_bound(hashPrev);
                     ++mi)
                {
                    const CDataStream& vMsg = *((*mi).second);
                    CTransaction tx;
                    CDataStream(vMsg) >> tx;
                    CInv inv(MSG_TX, tx.GetHash());

                    if (tx.AcceptToMemoryPool(true))
                    {
                        printf("   accepted orphan tx %s\n", inv.hash.ToString().substr(0,6).c_str());
                        AddToWalletIfMine(tx, NULL);
                        RelayMessage(inv, vMsg);
                        mapAlreadyAskedFor.erase(inv);
                        vWorkQueue.push_back(inv.hash);
                    }
                }
            }

            foreach(uint256 hash, vWorkQueue)
                EraseOrphanTx(hash);
        }
        else if (fMissingInputs)
        {
            printf("storing orphan tx %s\n", inv.hash.ToString().substr(0,6).c_str());
            AddOrphanTx(vMsg);
        }
    }


    else if (strCommand == "block")
    {
        auto_ptr<CBlock> pblock(new CBlock);
        vRecv >> *pblock;

        //// debug print
        printf("received block %s\n", pblock->GetHash().ToString().substr(0,20).c_str());
        // pblock->print();

        CInv inv(MSG_BLOCK, pblock->GetHash());
        pfrom->AddInventoryKnown(inv);

        if (ProcessBlock(pfrom, pblock.release()))
            mapAlreadyAskedFor.erase(inv);
    }


    else if (strCommand == "getaddr")
    {
        // This includes all nodes that are currently online,
        // since they rebroadcast an addr every 24 hours
        pfrom->vAddrToSend.clear();
        int64 nSince = GetAdjustedTime() - 12 * 60 * 60; // in the last 12 hours
        CRITICAL_BLOCK(cs_mapAddresses)
        {
            unsigned int nSize = mapAddresses.size();
            foreach(const PAIRTYPE(vector<unsigned char>, CAddress)& item, mapAddresses)
            {
                if (fShutdown)
                    return true;
                const CAddress& addr = item.second;
                if (addr.nTime > nSince)
                    pfrom->PushAddress(addr);
            }
        }
    }


    else if (strCommand == "checkorder")
    {
        uint256 hashReply;
        CWalletTx order;
        vRecv >> hashReply >> order;

        /// we have a chance to check the order here

        // Keep giving the same key to the same ip until they use it
        if (!mapReuseKey.count(pfrom->addr.ip))
            mapReuseKey[pfrom->addr.ip] = GenerateNewKey();

        // Send back approval of order and pubkey to use
        CScript scriptPubKey;
        scriptPubKey << mapReuseKey[pfrom->addr.ip] << OP_CHECKSIG;
        pfrom->PushMessage("reply", hashReply, (int)0, scriptPubKey);
    }


    else if (strCommand == "submitorder")
    {
        uint256 hashReply;
        CWalletTx wtxNew;
        vRecv >> hashReply >> wtxNew;
        wtxNew.fFromMe = false;

        // Broadcast
        if (!wtxNew.AcceptWalletTransaction())
        {
            pfrom->PushMessage("reply", hashReply, (int)1);
            return error("submitorder AcceptWalletTransaction() failed, returning error 1");
        }
        wtxNew.fTimeReceivedIsTxTime = true;
        AddToWallet(wtxNew);
        wtxNew.RelayWalletTransaction();
        mapReuseKey.erase(pfrom->addr.ip);

        // Send back confirmation
        pfrom->PushMessage("reply", hashReply, (int)0);
    }


    else if (strCommand == "reply")
    {
        uint256 hashReply;
        vRecv >> hashReply;

        CRequestTracker tracker;
        CRITICAL_BLOCK(pfrom->cs_mapRequests)
        {
            map<uint256, CRequestTracker>::iterator mi = pfrom->mapRequests.find(hashReply);
            if (mi != pfrom->mapRequests.end())
            {
                tracker = (*mi).second;
                pfrom->mapRequests.erase(mi);
            }
        }
        if (!tracker.IsNull())
            tracker.fn(tracker.param1, vRecv);
    }


    else if (strCommand == "ping")
    {
    }


    else if (strCommand == "alert")
    {
        CAlert alert;
        vRecv >> alert;

        if (alert.ProcessAlert())
        {
            // Relay
            pfrom->setKnown.insert(alert.GetHash());
            CRITICAL_BLOCK(cs_vNodes)
                foreach(CNode* pnode, vNodes)
                    alert.RelayTo(pnode);
        }
    }


    else
    {
        // Ignore unknown commands for extensibility
    }


    // Update the last seen time for this node's address
    if (pfrom->fNetworkNode)
        if (strCommand == "version" || strCommand == "addr" || strCommand == "inv" || strCommand == "getdata" || strCommand == "ping")
            AddressCurrentlyConnected(pfrom->addr);


    return true;
}









bool SendMessages(CNode* pto, bool fSendTrickle)
{
    CRITICAL_BLOCK(cs_main)
    {
        // Don't send anything until we get their version message
        if (pto->nVersion == 0)
            return true;

        // Keep-alive ping
        if (pto->nLastSend && GetTime() - pto->nLastSend > 30 * 60 && pto->vSend.empty())
            pto->PushMessage("ping");

        // Address refresh broadcast
        static int64 nLastRebroadcast;
        if (GetTime() - nLastRebroadcast > 24 * 60 * 60) // every 24 hours
        {
            nLastRebroadcast = GetTime();
            CRITICAL_BLOCK(cs_vNodes)
            {
                foreach(CNode* pnode, vNodes)
                {
                    // Periodically clear setAddrKnown to allow refresh broadcasts
                    pnode->setAddrKnown.clear();

                    // Rebroadcast our address
                    if (addrLocalHost.IsRoutable() && !fUseProxy)
                        pnode->PushAddress(addrLocalHost);
                }
            }
        }

        // Resend wallet transactions that haven't gotten in a block yet
        ResendWalletTransactions();


        //
        // Message: addr
        //
        if (fSendTrickle)
        {
            vector<CAddress> vAddr;
            vAddr.reserve(pto->vAddrToSend.size());
            foreach(const CAddress& addr, pto->vAddrToSend)
            {
                // returns true if wasn't already contained in the set
                if (pto->setAddrKnown.insert(addr).second)
                {
                    vAddr.push_back(addr);
                    // receiver rejects addr messages larger than 1000
                    if (vAddr.size() >= 1000)
                    {
                        pto->PushMessage("addr", vAddr);
                        vAddr.clear();
                    }
                }
            }
            pto->vAddrToSend.clear();
            if (!vAddr.empty())
                pto->PushMessage("addr", vAddr);
        }


        //
        // Message: inventory
        //
        vector<CInv> vInv;
        vector<CInv> vInvWait;
        CRITICAL_BLOCK(pto->cs_inventory)
        {
            vInv.reserve(pto->vInventoryToSend.size());
            vInvWait.reserve(pto->vInventoryToSend.size());
            foreach(const CInv& inv, pto->vInventoryToSend)
            {
                if (pto->setInventoryKnown.count(inv))
                    continue;

                // trickle out tx inv to protect privacy
                if (inv.type == MSG_TX && !fSendTrickle)
                {
                    // 1/4 of tx invs blast to all immediately
                    static uint256 hashSalt;
                    if (hashSalt == 0)
                        RAND_bytes((unsigned char*)&hashSalt, sizeof(hashSalt));
                    uint256 hashRand = inv.hash ^ hashSalt;
                    hashRand = Hash(BEGIN(hashRand), END(hashRand));
                    bool fTrickleWait = ((hashRand & 3) != 0);

                    // always trickle our own transactions
                    if (!fTrickleWait)
                    {
                        TRY_CRITICAL_BLOCK(cs_mapWallet)
                        {
                            map<uint256, CWalletTx>::iterator mi = mapWallet.find(inv.hash);
                            if (mi != mapWallet.end())
                            {
                                CWalletTx& wtx = (*mi).second;
                                if (wtx.fFromMe)
                                    fTrickleWait = true;
                            }
                        }
                    }

                    if (fTrickleWait)
                    {
                        vInvWait.push_back(inv);
                        continue;
                    }
                }

                // returns true if wasn't already contained in the set
                if (pto->setInventoryKnown.insert(inv).second)
                {
                    vInv.push_back(inv);
                    if (vInv.size() >= 1000)
                    {
                        pto->PushMessage("inv", vInv);
                        vInv.clear();
                    }
                }
            }
            pto->vInventoryToSend = vInvWait;
        }
        if (!vInv.empty())
            pto->PushMessage("inv", vInv);


        //
        // Message: getdata
        //
        vector<CInv> vGetData;
        int64 nNow = GetTime() * 1000000;
        CTxDB txdb("r");
        while (!pto->mapAskFor.empty() && (*pto->mapAskFor.begin()).first <= nNow)
        {
            const CInv& inv = (*pto->mapAskFor.begin()).second;
            if (!AlreadyHave(txdb, inv))
            {
                printf("sending getdata: %s\n", inv.ToString().c_str());
                vGetData.push_back(inv);
                if (vGetData.size() >= 1000)
                {
                    pto->PushMessage("getdata", vGetData);
                    vGetData.clear();
                }
            }
            pto->mapAskFor.erase(pto->mapAskFor.begin());
        }
        if (!vGetData.empty())
            pto->PushMessage("getdata", vGetData);

    }
    return true;
}














//////////////////////////////////////////////////////////////////////////////
//
// BitcoinMiner
//

void GenerateBitcoins(bool fGenerate)
{
    if (fGenerateBitcoins != fGenerate)
    {
        fGenerateBitcoins = fGenerate;
        CWalletDB().WriteSetting("fGenerateBitcoins", fGenerateBitcoins);
        MainFrameRepaint();
    }
    if (fGenerateBitcoins)
    {
        int nProcessors = boost::thread::hardware_concurrency();
        printf("%d processors\n", nProcessors);
        if (nProcessors < 1)
            nProcessors = 1;
        if (fLimitProcessors && nProcessors > nLimitProcessors)
            nProcessors = nLimitProcessors;
        int nAddThreads = nProcessors - vnThreadsRunning[3];
        printf("Starting %d BitcoinMiner threads\n", nAddThreads);
        for (int i = 0; i < nAddThreads; i++)
        {
            if (!CreateThread(ThreadBitcoinMiner, NULL))
                printf("Error: CreateThread(ThreadBitcoinMiner) failed\n");
            Sleep(10);
        }
    }
}

void ThreadBitcoinMiner(void* parg)
{
    try
    {
        vnThreadsRunning[3]++;
        BitcoinMiner();
        vnThreadsRunning[3]--;
    }
    catch (std::exception& e) {
        vnThreadsRunning[3]--;
        PrintException(&e, "ThreadBitcoinMiner()");
    } catch (...) {
        vnThreadsRunning[3]--;
        PrintException(NULL, "ThreadBitcoinMiner()");
    }
    UIThreadCall(bind(CalledSetStatusBar, "", 0));
    nHPSTimerStart = 0;
    if (vnThreadsRunning[3] == 0)
        dHashesPerSec = 0;
    printf("ThreadBitcoinMiner exiting, %d threads remaining\n", vnThreadsRunning[3]);
}

int FormatHashBlocks(void* pbuffer, unsigned int len)
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

using CryptoPP::ByteReverse;

static const unsigned int pSHA256InitState[8] =
{0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};

inline void SHA256Transform(void* pstate, void* pinput, const void* pinit)
{
    memcpy(pstate, pinit, 32);
    CryptoPP::SHA256::Transform((CryptoPP::word32*)pstate, (CryptoPP::word32*)pinput);
}

static const int NPAR = 32;
extern void Double_BlockSHA256(const void* pin, void* pout, const void* pinit, unsigned int hash[8][NPAR], const void* init2);




void BitcoinMiner()
{
    printf("BitcoinMiner started\n");
    SetThreadPriority(THREAD_PRIORITY_LOWEST);

    CKey key;
    key.MakeNewKey();
    CBigNum bnExtraNonce = 0;
    while (fGenerateBitcoins)
    {
        Sleep(50);
        if (fShutdown)
            return;
        while (vNodes.empty() || IsInitialBlockDownload())
        {
            Sleep(1000);
            if (fShutdown)
                return;
            if (!fGenerateBitcoins)
                return;
        }

        unsigned int nTransactionsUpdatedLast = nTransactionsUpdated;
        CBlockIndex* pindexPrev = pindexBest;
        unsigned int nBits = GetNextWorkRequired(pindexPrev);


        //
        // Create coinbase tx
        //
        CTransaction txNew;
        txNew.vin.resize(1);
        txNew.vin[0].prevout.SetNull();
        txNew.vin[0].scriptSig << nBits << ++bnExtraNonce;
        txNew.vout.resize(1);
        txNew.vout[0].scriptPubKey << key.GetPubKey() << OP_CHECKSIG;


        //
        // Create new block
        //
        auto_ptr<CBlock> pblock(new CBlock());
        if (!pblock.get())
            return;

        // Add our coinbase tx as first transaction
        pblock->vtx.push_back(txNew);

        // Collect the latest transactions into the block
        int64 nFees = 0;
        CRITICAL_BLOCK(cs_main)
        CRITICAL_BLOCK(cs_mapTransactions)
        {
            CTxDB txdb("r");
            map<uint256, CTxIndex> mapTestPool;
            vector<char> vfAlreadyAdded(mapTransactions.size());
            bool fFoundSomething = true;
            uint64 nBlockSize = 10000;
            int nBlockSigOps = 100;
            while (fFoundSomething)
            {
                fFoundSomething = false;
                unsigned int n = 0;
                for (map<uint256, CTransaction>::iterator mi = mapTransactions.begin(); mi != mapTransactions.end(); ++mi, ++n)
                {
                    if (vfAlreadyAdded[n])
                        continue;
                    CTransaction& tx = (*mi).second;
                    if (tx.IsCoinBase() || !tx.IsFinal())
                        continue;
                    unsigned int nTxSize = ::GetSerializeSize(tx, SER_NETWORK);
                    if (nBlockSize + nTxSize >= MAX_BLOCK_SIZE)
                        continue;
                    int nTxSigOps = tx.GetSigOpCount();
                    if (nBlockSigOps + nTxSigOps >= MAX_BLOCK_SIGOPS)
                        continue;

                    // Transaction fee based on block size
                    int64 nMinFee = tx.GetMinFee(nBlockSize);

                    map<uint256, CTxIndex> mapTestPoolTmp(mapTestPool);
                    if (!tx.ConnectInputs(txdb, mapTestPoolTmp, CDiskTxPos(1,1,1), pindexPrev, nFees, false, true, nMinFee))
                        continue;
                    swap(mapTestPool, mapTestPoolTmp);

                    pblock->vtx.push_back(tx);
                    nBlockSize += nTxSize;
                    nBlockSigOps += nTxSigOps;
                    vfAlreadyAdded[n] = true;
                    fFoundSomething = true;
                }
            }
        }
        pblock->nBits = nBits;
        pblock->vtx[0].vout[0].nValue = pblock->GetBlockValue(pindexPrev->nHeight+1, nFees);
        printf("Running BitcoinMiner with %d transactions in block\n", pblock->vtx.size());


        //
        // Prebuild hash buffer
        //
        struct tmpworkspace
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
        };
        char tmpbuf[sizeof(tmpworkspace)+16];
        tmpworkspace& tmp = *(tmpworkspace*)alignup<16>(tmpbuf);

        tmp.block.nVersion       = pblock->nVersion;
        tmp.block.hashPrevBlock  = pblock->hashPrevBlock  = (pindexPrev ? pindexPrev->GetBlockHash() : 0);
        tmp.block.hashMerkleRoot = pblock->hashMerkleRoot = pblock->BuildMerkleTree();
        tmp.block.nTime          = pblock->nTime          = max((pindexPrev ? pindexPrev->GetMedianTimePast()+1 : 0), GetAdjustedTime());
        tmp.block.nBits          = pblock->nBits          = nBits;
        tmp.block.nNonce         = pblock->nNonce         = 0;

        unsigned int nBlocks0 = FormatHashBlocks(&tmp.block, sizeof(tmp.block));
        unsigned int nBlocks1 = FormatHashBlocks(&tmp.hash1, sizeof(tmp.hash1));

        // Byte swap all the input buffer
        for (int i = 0; i < sizeof(tmp)/4; i++)
            ((unsigned int*)&tmp)[i] = ByteReverse(((unsigned int*)&tmp)[i]);

        // Precalc the first half of the first hash, which stays constant
        uint256 midstatebuf[2];
        uint256& midstate = *alignup<16>(midstatebuf);
        SHA256Transform(&midstate, &tmp.block, pSHA256InitState);


        //
        // Search
        //
        bool f4WaySSE2 = mapArgs.count("-4way");
        int64 nStart = GetTime();
        uint256 hashTarget = CBigNum().SetCompact(pblock->nBits).getuint256();
        uint256 hashbuf[2];
        uint256& hash = *alignup<16>(hashbuf);
        loop
        {
#ifdef FOURWAYSSE2
            if (f4WaySSE2)
            {
                // tcatm's 4-way SSE2 SHA-256
                tmp.block.nNonce += NPAR;
                unsigned int thashbuf[9][NPAR];
                unsigned int (&thash)[9][NPAR] = *alignup<16>(&thashbuf);
                Double_BlockSHA256((char*)&tmp.block + 64, &tmp.hash1, &midstate, thash, pSHA256InitState);
                ((unsigned short*)&hash)[14] = 0xffff;
                for (int j = 0; j < NPAR; j++)
                {
                    if (thash[7][j] == 0)
                    {
                        for (int i = 0; i < sizeof(hash)/4; i++)
                            ((unsigned int*)&hash)[i] = thash[i][j];
                        pblock->nNonce = ByteReverse(tmp.block.nNonce + j);
                    }
                }
            }
            else
#endif
            {
                // Crypto++ SHA-256
                tmp.block.nNonce++;
                SHA256Transform(&tmp.hash1, (char*)&tmp.block + 64, &midstate);
                SHA256Transform(&hash, &tmp.hash1, pSHA256InitState);
            }

            if (((unsigned short*)&hash)[14] == 0)
            {
                // Byte swap the result after preliminary check
                for (int i = 0; i < sizeof(hash)/4; i++)
                    ((unsigned int*)&hash)[i] = ByteReverse(((unsigned int*)&hash)[i]);

                if (hash <= hashTarget)
                {
#ifdef FOURWAYSSE2
                    if (!f4WaySSE2)
#endif
                        pblock->nNonce = ByteReverse(tmp.block.nNonce);
                    assert(hash == pblock->GetHash());

                        //// debug print
                        printf("BitcoinMiner:\n");
                        printf("proof-of-work found  \n  hash: %s  \ntarget: %s\n", hash.GetHex().c_str(), hashTarget.GetHex().c_str());
                        pblock->print();
                        printf("%s ", DateTimeStrFormat("%x %H:%M", GetTime()).c_str());
                        printf("generated %s\n", FormatMoney(pblock->vtx[0].vout[0].nValue).c_str());

                    SetThreadPriority(THREAD_PRIORITY_NORMAL);
                    CRITICAL_BLOCK(cs_main)
                    {
                        if (pindexPrev == pindexBest)
                        {
                            // Save key
                            if (!AddKey(key))
                                return;
                            key.MakeNewKey();

                            // Track how many getdata requests this block gets
                            CRITICAL_BLOCK(cs_mapRequestCount)
                                mapRequestCount[pblock->GetHash()] = 0;

                            // Process this block the same as if we had received it from another node
                            if (!ProcessBlock(NULL, pblock.release()))
                                printf("ERROR in BitcoinMiner, ProcessBlock, block not accepted\n");
                        }
                    }
                    SetThreadPriority(THREAD_PRIORITY_LOWEST);

                    Sleep(500);
                    break;
                }
            }

            // Update nTime every few seconds
            const unsigned int nMask = 0xffff;
            const int nHashesPerCycle = (nMask+1);
            if ((tmp.block.nNonce & nMask) == 0)
            {
                // Meter hashes/sec
                static int nCycleCounter;
                if (nHPSTimerStart == 0)
                {
                    nHPSTimerStart = GetTimeMillis();
                    nCycleCounter = 0;
                }
                else
                    nCycleCounter++;
                if (GetTimeMillis() - nHPSTimerStart > 4000)
                {
                    static CCriticalSection cs;
                    CRITICAL_BLOCK(cs)
                    {
                        if (GetTimeMillis() - nHPSTimerStart > 4000)
                        {
                            dHashesPerSec = 1000.0 * nHashesPerCycle * nCycleCounter / (GetTimeMillis() - nHPSTimerStart);
                            nHPSTimerStart = GetTimeMillis();
                            nCycleCounter = 0;
                            string strStatus = strprintf("    %.0f khash/s", dHashesPerSec/1000.0);
                            UIThreadCall(bind(CalledSetStatusBar, strStatus, 0));
                            static int64 nLogTime;
                            if (GetTime() - nLogTime > 30 * 60)
                            {
                                nLogTime = GetTime();
                                printf("%s ", DateTimeStrFormat("%x %H:%M", GetTime()).c_str());
                                printf("hashmeter %3d CPUs %6.0f khash/s\n", vnThreadsRunning[3], dHashesPerSec/1000.0);
                            }
                        }
                    }
                }

                // Check for stop or if block needs to be rebuilt
                if (fShutdown)
                    return;
                if (!fGenerateBitcoins)
                    return;
                if (fLimitProcessors && vnThreadsRunning[3] > nLimitProcessors)
                    return;
                if (vNodes.empty())
                    break;
                if (tmp.block.nNonce == 0)
                    break;
                if (nTransactionsUpdated != nTransactionsUpdatedLast && GetTime() - nStart > 60)
                    break;
                if (pindexPrev != pindexBest)
                    break;

                pblock->nTime = max(pindexPrev->GetMedianTimePast()+1, GetAdjustedTime());
                tmp.block.nTime = ByteReverse(pblock->nTime);
            }
        }
    }
}


















//////////////////////////////////////////////////////////////////////////////
//
// Actions
//


int64 GetBalance()
{
    int64 nStart = GetTimeMillis();

    int64 nTotal = 0;
    CRITICAL_BLOCK(cs_mapWallet)
    {
        for (map<uint256, CWalletTx>::iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            CWalletTx* pcoin = &(*it).second;
            if (!pcoin->IsFinal() || pcoin->fSpent)
                continue;
            nTotal += pcoin->GetCredit(true);
        }
    }

    //printf("GetBalance() %"PRI64d"ms\n", GetTimeMillis() - nStart);
    return nTotal;
}


int GetRandInt(int nMax)
{
    return GetRand(nMax);
}

bool SelectCoins(int64 nTargetValue, set<CWalletTx*>& setCoinsRet)
{
    setCoinsRet.clear();

    // List of values less than target
    int64 nLowestLarger = INT64_MAX;
    CWalletTx* pcoinLowestLarger = NULL;
    vector<pair<int64, CWalletTx*> > vValue;
    int64 nTotalLower = 0;

    CRITICAL_BLOCK(cs_mapWallet)
    {
       vector<CWalletTx*> vCoins;
       vCoins.reserve(mapWallet.size());
       for (map<uint256, CWalletTx>::iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
           vCoins.push_back(&(*it).second);
       random_shuffle(vCoins.begin(), vCoins.end(), GetRandInt);

       foreach(CWalletTx* pcoin, vCoins)
       {
            if (!pcoin->IsFinal() || pcoin->fSpent)
                continue;
            int64 n = pcoin->GetCredit();
            if (n <= 0)
                continue;
            if (n < nTargetValue)
            {
                vValue.push_back(make_pair(n, pcoin));
                nTotalLower += n;
            }
            else if (n == nTargetValue)
            {
                setCoinsRet.insert(pcoin);
                return true;
            }
            else if (n < nLowestLarger)
            {
                nLowestLarger = n;
                pcoinLowestLarger = pcoin;
            }
        }
    }

    if (nTotalLower < nTargetValue)
    {
        if (pcoinLowestLarger == NULL)
            return false;
        setCoinsRet.insert(pcoinLowestLarger);
        return true;
    }

    // Solve subset sum by stochastic approximation
    sort(vValue.rbegin(), vValue.rend());
    vector<char> vfIncluded;
    vector<char> vfBest(vValue.size(), true);
    int64 nBest = nTotalLower;

    for (int nRep = 0; nRep < 1000 && nBest != nTargetValue; nRep++)
    {
        vfIncluded.assign(vValue.size(), false);
        int64 nTotal = 0;
        bool fReachedTarget = false;
        for (int nPass = 0; nPass < 2 && !fReachedTarget; nPass++)
        {
            for (int i = 0; i < vValue.size(); i++)
            {
                if (nPass == 0 ? rand() % 2 : !vfIncluded[i])
                {
                    nTotal += vValue[i].first;
                    vfIncluded[i] = true;
                    if (nTotal >= nTargetValue)
                    {
                        fReachedTarget = true;
                        if (nTotal < nBest)
                        {
                            nBest = nTotal;
                            vfBest = vfIncluded;
                        }
                        nTotal -= vValue[i].first;
                        vfIncluded[i] = false;
                    }
                }
            }
        }
    }

    // If the next larger is still closer, return it
    if (pcoinLowestLarger && nLowestLarger - nTargetValue <= nBest - nTargetValue)
        setCoinsRet.insert(pcoinLowestLarger);
    else
    {
        for (int i = 0; i < vValue.size(); i++)
            if (vfBest[i])
                setCoinsRet.insert(vValue[i].second);

        //// debug print
        printf("SelectCoins() best subset: ");
        for (int i = 0; i < vValue.size(); i++)
            if (vfBest[i])
                printf("%s ", FormatMoney(vValue[i].first).c_str());
        printf("total %s\n", FormatMoney(nBest).c_str());
    }

    return true;
}




bool CreateTransaction(CScript scriptPubKey, int64 nValue, CWalletTx& wtxNew, CKey& keyRet, int64& nFeeRequiredRet)
{
    nFeeRequiredRet = 0;
    CRITICAL_BLOCK(cs_main)
    {
        // txdb must be opened before the mapWallet lock
        CTxDB txdb("r");
        CRITICAL_BLOCK(cs_mapWallet)
        {
            int64 nFee = nTransactionFee;
            loop
            {
                wtxNew.vin.clear();
                wtxNew.vout.clear();
                wtxNew.fFromMe = true;
                if (nValue < 0)
                    return false;
                int64 nValueOut = nValue;
                int64 nTotalValue = nValue + nFee;

                // Choose coins to use
                set<CWalletTx*> setCoins;
                if (!SelectCoins(nTotalValue, setCoins))
                    return false;
                int64 nValueIn = 0;
                foreach(CWalletTx* pcoin, setCoins)
                    nValueIn += pcoin->GetCredit();

                // Fill a vout to the payee
                bool fChangeFirst = GetRand(2);
                if (!fChangeFirst)
                    wtxNew.vout.push_back(CTxOut(nValueOut, scriptPubKey));

                // Fill a vout back to self with any change
                if (nValueIn > nTotalValue)
                {
                    // Note: We use a new key here to keep it from being obvious which side is the change.
                    //  The drawback is that by not reusing a previous key, the change may be lost if a
                    //  backup is restored, if the backup doesn't have the new private key for the change.
                    //  If we reused the old key, it would be possible to add code to look for and
                    //  rediscover unknown transactions that were written with keys of ours to recover
                    //  post-backup change.

                    // New private key
                    if (keyRet.IsNull())
                        keyRet.MakeNewKey();

                    // Fill a vout to ourself, using same address type as the payment
                    CScript scriptChange;
                    if (scriptPubKey.GetBitcoinAddressHash160() != 0)
                        scriptChange.SetBitcoinAddress(keyRet.GetPubKey());
                    else
                        scriptChange << keyRet.GetPubKey() << OP_CHECKSIG;
                    wtxNew.vout.push_back(CTxOut(nValueIn - nTotalValue, scriptChange));
                }

                // Fill a vout to the payee
                if (fChangeFirst)
                    wtxNew.vout.push_back(CTxOut(nValueOut, scriptPubKey));

                // Fill vin
                foreach(CWalletTx* pcoin, setCoins)
                    for (int nOut = 0; nOut < pcoin->vout.size(); nOut++)
                        if (pcoin->vout[nOut].IsMine())
                            wtxNew.vin.push_back(CTxIn(pcoin->GetHash(), nOut));

                // Sign
                int nIn = 0;
                foreach(CWalletTx* pcoin, setCoins)
                    for (int nOut = 0; nOut < pcoin->vout.size(); nOut++)
                        if (pcoin->vout[nOut].IsMine())
                            if (!SignSignature(*pcoin, wtxNew, nIn++))
                                return false;

                // Check that enough fee is included
                if (nFee < wtxNew.GetMinFee())
                {
                    nFee = nFeeRequiredRet = wtxNew.GetMinFee();
                    continue;
                }

                // Fill vtxPrev by copying from previous transactions vtxPrev
                wtxNew.AddSupportingTransactions(txdb);
                wtxNew.fTimeReceivedIsTxTime = true;

                break;
            }
        }
    }
    return true;
}

// Call after CreateTransaction unless you want to abort
bool CommitTransaction(CWalletTx& wtxNew, const CKey& key)
{
    CRITICAL_BLOCK(cs_main)
    {
        printf("CommitTransaction:\n%s", wtxNew.ToString().c_str());
        CRITICAL_BLOCK(cs_mapWallet)
        {
            // This is only to keep the database open to defeat the auto-flush for the
            // duration of this scope.  This is the only place where this optimization
            // maybe makes sense; please don't do it anywhere else.
            CWalletDB walletdb("r");

            // Add the change's private key to wallet
            if (!key.IsNull() && !AddKey(key))
                throw runtime_error("CommitTransaction() : AddKey failed");

            // Add tx to wallet, because if it has change it's also ours,
            // otherwise just for transaction history.
            AddToWallet(wtxNew);

            // Mark old coins as spent
            set<CWalletTx*> setCoins;
            foreach(const CTxIn& txin, wtxNew.vin)
                setCoins.insert(&mapWallet[txin.prevout.hash]);
            foreach(CWalletTx* pcoin, setCoins)
            {
                pcoin->fSpent = true;
                pcoin->WriteToDisk();
                vWalletUpdated.push_back(pcoin->GetHash());
            }
        }

        // Track how many getdata requests our transaction gets
        CRITICAL_BLOCK(cs_mapRequestCount)
            mapRequestCount[wtxNew.GetHash()] = 0;

        // Broadcast
        if (!wtxNew.AcceptToMemoryPool())
        {
            // This must not fail. The transaction has already been signed and recorded.
            printf("CommitTransaction() : Error: Transaction not valid");
            return false;
        }
        wtxNew.RelayWalletTransaction();
    }
    MainFrameRepaint();
    return true;
}




string SendMoney(CScript scriptPubKey, int64 nValue, CWalletTx& wtxNew, bool fAskFee)
{
    CRITICAL_BLOCK(cs_main)
    {
        CKey key;
        int64 nFeeRequired;
        if (!CreateTransaction(scriptPubKey, nValue, wtxNew, key, nFeeRequired))
        {
            string strError;
            if (nValue + nFeeRequired > GetBalance())
                strError = strprintf(_("Error: This is an oversized transaction that requires a transaction fee of %s  "), FormatMoney(nFeeRequired).c_str());
            else
                strError = _("Error: Transaction creation failed  ");
            printf("SendMoney() : %s", strError.c_str());
            return strError;
        }

        if (fAskFee && !ThreadSafeAskFee(nFeeRequired, _("Sending..."), NULL))
            return "ABORTED";

        if (!CommitTransaction(wtxNew, key))
            return _("Error: The transaction was rejected.  This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.");
    }
    MainFrameRepaint();
    return "";
}



string SendMoneyToBitcoinAddress(string strAddress, int64 nValue, CWalletTx& wtxNew, bool fAskFee)
{
    // Check amount
    if (nValue <= 0)
        return _("Invalid amount");
    if (nValue + nTransactionFee > GetBalance())
        return _("Insufficient funds");

    // Parse bitcoin address
    CScript scriptPubKey;
    if (!scriptPubKey.SetBitcoinAddress(strAddress))
        return _("Invalid bitcoin address");

    return SendMoney(scriptPubKey, nValue, wtxNew, fAskFee);
}
