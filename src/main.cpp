// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include "alert.h"
#include "checkpoints.h"
#include "db.h"
#include "txdb.h"
#include "net.h"
#include "init.h"
#include "ui_interface.h"
#include "kernel.h"
#include "smessage.h"


using namespace std;

//
// Global state
//

CCriticalSection cs_setpwalletRegistered;
std::set<CWallet*> setpwalletRegistered;

CCriticalSection cs_main;

CTxMemPool mempool;

std::map<uint256, CBlockIndex*> mapBlockIndex;
std::map<uint256, CBlockThinIndex*> mapBlockThinIndex;

std::set<std::pair<COutPoint, unsigned int> > setStakeSeen;


unsigned int nStakeMinAge       = 8 * 60 * 60;      // 8 hours 
unsigned int nModifierInterval  = 10 * 60;          // time to elapse before new modifier is computed

int nCoinbaseMaturity = 50;
CBlockIndex* pindexGenesisBlock = NULL;

CBlockThinIndex* pindexGenesisBlockThin = NULL;
CBlockThinIndex* pindexRear = NULL;

int nBestHeight = -1;
int nHeightFilteredNeeded = -1;

uint256 nBestChainTrust = 0;
uint256 nBestInvalidTrust = 0;

uint256 hashBestChain = 0;
CBlockIndex* pindexBest = NULL;
CBlockThinIndex* pindexBestHeader;

int64_t nTimeBestReceived = 0;

CMedianFilter<int> cPeerBlockCounts(5, 0); // Amount of blocks that other nodes claim to have

std::map<uint256, CBlock*> mapOrphanBlocks;
std::map<uint256, CBlockThin*> mapOrphanBlockThins;

std::map<int64_t, CAnonOutputCount> mapAnonOutputStats; // display only, not 100% accurate, height could become inaccurate due to undos

std::multimap<uint256, CBlock*> mapOrphanBlocksByPrev;
std::multimap<uint256, CBlockThin*> mapOrphanBlockThinsByPrev;

std::set<std::pair<COutPoint, unsigned int> > setStakeSeenOrphan;

std::map<uint256, CTransaction> mapOrphanTransactions;
std::map<uint256, std::set<uint256> > mapOrphanTransactionsByPrev;


std::vector<CPendingFilteredChunk> vPendingFilteredChunks;
std::vector<CMerkleBlockIncoming> vIncomingMerkleBlocks; // blocks with txns attached get stored here until all following tx messages have been processed

// Constant stuff for coinbase transactions we create:
CScript COINBASE_FLAGS;

const string strMessageMagic = "Gameunits Signed Message:\n";

// Settings
int64_t nTransactionFee = MIN_TX_FEE;
int64_t nReserveBalance = 0;
int64_t nMinimumInputValue = 0;

//////////////////////////////////////////////////////////////////////////////
//
// dispatching functions
//

// These functions dispatch to one or all registered wallets


void RegisterWallet(CWallet* pwalletIn)
{
    {
        LOCK(cs_setpwalletRegistered);
        setpwalletRegistered.insert(pwalletIn);
    }
}

void UnregisterWallet(CWallet* pwalletIn)
{
    {
        LOCK(cs_setpwalletRegistered);
        setpwalletRegistered.erase(pwalletIn);
    }
}

// check whether the passed transaction is from us
bool static IsFromMe(CTransaction& tx)
{
    BOOST_FOREACH(CWallet* pwallet, setpwalletRegistered)
        if (pwallet->IsFromMe(tx))
            return true;
    return false;
}

// get the wallet transaction with the given hash (if it exists)
bool static GetTransaction(const uint256& hashTx, CWalletTx& wtx)
{
    BOOST_FOREACH(CWallet* pwallet, setpwalletRegistered)
        if (pwallet->GetTransaction(hashTx,wtx))
            return true;
    return false;
}

// erases transaction with the given hash from all wallets
void static EraseFromWallets(uint256 hash)
{
    BOOST_FOREACH(CWallet* pwallet, setpwalletRegistered)
        pwallet->EraseFromWallet(hash);
}

// make sure all wallets know about the given transaction, in the given block
void SyncWithWallets(const CTransaction& tx, const CBlock* pblock, bool fUpdate, bool fConnect)
{
    if (!fConnect)
    {
        // ppcoin: wallets need to refund inputs when disconnecting coinstake
        if (tx.IsCoinStake())
        {
            BOOST_FOREACH(CWallet* pwallet, setpwalletRegistered)
            {
                if (pwallet->IsFromMe(tx))
                    pwallet->DisableTransaction(tx);
            };
        };

        if (tx.nVersion == ANON_TXN_VERSION)
        {
            BOOST_FOREACH(CWallet* pwallet, setpwalletRegistered)
                pwallet->UndoAnonTransaction(tx);
        };
        return;
    };

    uint256 hash = tx.GetHash();
    BOOST_FOREACH(CWallet* pwallet, setpwalletRegistered)
        pwallet->AddToWalletIfInvolvingMe(tx, hash, pblock, fUpdate);
}

void SyncWithWalletsThin(const CTransaction& tx, const uint256& blockhash, bool fUpdate, bool fConnect)
{
    if (!fConnect)
    {
        // ppcoin: wallets need to refund inputs when disconnecting coinstake
        if (tx.IsCoinStake())
        {
            BOOST_FOREACH(CWallet* pwallet, setpwalletRegistered)
            {
                if (pwallet->IsFromMe(tx))
                    pwallet->DisableTransaction(tx);
            };
        };
        return;
    };

    uint256 hash = tx.GetHash();
    BOOST_FOREACH(CWallet* pwallet, setpwalletRegistered)
        pwallet->AddToWalletIfInvolvingMe(tx, hash, &blockhash, fUpdate);
}

// notify wallets about a new best chain
void static SetBestChain(const CBlockLocator& loc)
{
    BOOST_FOREACH(CWallet* pwallet, setpwalletRegistered)
        pwallet->SetBestChain(loc);
}

// notify wallets about a new best chain
void static SetBestThinChain(const CBlockThinLocator& loc)
{
    BOOST_FOREACH(CWallet* pwallet, setpwalletRegistered)
        pwallet->SetBestThinChain(loc);
}

// notify wallets about an updated transaction
void static UpdatedTransaction(const uint256& hashTx)
{
    BOOST_FOREACH(CWallet* pwallet, setpwalletRegistered)
        pwallet->UpdatedTransaction(hashTx);
}

// dump all wallets
void static PrintWallets(const CBlock& block)
{
    BOOST_FOREACH(CWallet* pwallet, setpwalletRegistered)
        pwallet->PrintWallet(block);
}

// notify wallets about an incoming inventory (for request counts)
void static Inventory(const uint256& hash)
{
    BOOST_FOREACH(CWallet* pwallet, setpwalletRegistered)
        pwallet->Inventory(hash);
}

// ask wallets to resend their transactions
void ResendWalletTransactions(bool fForce)
{
    BOOST_FOREACH(CWallet* pwallet, setpwalletRegistered)
        pwallet->ResendWalletTransactions(fForce);
}

bool SetHeightFilteredNeeded()
{
    LOCK2(cs_main, pwalletMain->cs_wallet);

    if (fDebug)
        LogPrintf("SetHeightFilteredNeeded() last filtered height: %d, time first key: %d\n", pwalletMain->nLastFilteredHeight, pwalletMain->nTimeFirstKey);

    if (!fThinFullIndex && pindexRear
        && pwalletMain->nLastFilteredHeight < pindexRear->nHeight
        && pwalletMain->nTimeFirstKey < pindexRear->nTime)
    {
        CDiskBlockThinIndex diskindex;
        uint256 hashPrev = pindexRear->GetBlockHash();

        CTxDB txdb("r");
        while (hashPrev != 0)
        {
            if (!txdb.ReadBlockThinIndex(hashPrev, diskindex))
            {
                LogPrintf("Read header %s from db failed.\n", hashPrev.ToString().c_str());
                return false;
            };

            if (diskindex.nTime <= pwalletMain->nTimeFirstKey
                || pwalletMain->nLastFilteredHeight >= diskindex.nHeight)
            {
                nHeightFilteredNeeded = diskindex.nHeight;
                break;
            };

            hashPrev = diskindex.hashPrev;
        };

        if (fDebug)
            LogPrintf("SetHeightFilteredNeeded set from db: %d, %s\n", nHeightFilteredNeeded, hashPrev.ToString().c_str());
        return true;
    };


    CBlockThinIndex* pindex = pindexBestHeader;
    while (pindex && pindex->pprev)
    {
        if (pindex->nTime <= pwalletMain->nTimeFirstKey
            || pwalletMain->nLastFilteredHeight >= pindex->nHeight)
        {
            nHeightFilteredNeeded = pindex->nHeight;
            break;
        };
        pindex = pindex->pprev;
    };
    return true;
};

bool ChangeNodeState(int newState, bool fProcess)
{

    switch (newState)
    {
        case NS_STARTUP:
            break;
        case NS_GET_HEADERS:
            break;
        case NS_GET_FILTERED_BLOCKS:
            {
                if (fProcess)
                    SetHeightFilteredNeeded();

                LogPrintf("Need filtered blocks from height %d\n", nHeightFilteredNeeded);
            }
            break;
        case NS_READY:
            break;
        default:
            LogPrintf("ChangeNodeState: Unknown state ind %d\n", newState);
            return false;
    };

    nNodeState = newState;
    LogPrintf("Node state set to: %s\n", GetNodeStateName(nNodeState));

    return true;
};

bool AddDataToMerkleFilters(const std::vector<unsigned char>& vData)
{
    LOCK(pwalletMain->cs_wallet);

    if (!pwalletMain->pBloomFilter)
    {
        LogPrintf("AddDataToMerkleFilters(): Filter is not created.\n");
        return false;
    };

    if (pwalletMain->pBloomFilter->contains(vData))
        return true;

    pwalletMain->pBloomFilter->UpdateEmptyFull();
    if (pwalletMain->pBloomFilter->IsFull())
    {
        // TODO: try resize?
        LogPrintf("AddDataToMerkleFilters(): Filter is full.\n");
        return false;
    };

    pwalletMain->pBloomFilter->insert(vData);

    {
        LOCK(cs_vNodes);
        BOOST_FOREACH(CNode* pnode, vNodes)
            pnode->PushMessage("filteradd", vData);
    }

    return true;
};

bool AddKeyToMerkleFilters(const CTxDestination& address)
{
    CBitcoinAddress coinAddress(address);
    if (!coinAddress.IsValid())
        return false;

    CKeyID keyId;
    if (!coinAddress.GetKeyID(keyId))
    {
        LogPrintf("coinAddress.GetKeyID failed: %s.\n", coinAddress.ToString().c_str());
        return false;
    };

    std::vector<unsigned char> vData(keyId.begin(), keyId.end());

    if (!AddDataToMerkleFilters(vData))
        return false;

    if (fDebug)
        LogPrintf("Added key %s to bloom filter.\n", coinAddress.ToString().c_str());

    return true;
};

bool GetCoinAgeThin(CTransaction txCoinStake, uint64_t& nCoinAge, std::vector<const CWalletTx*> &vWtxPrev)
{
    CBigNum bnCentSecond = 0;  // coin age in the unit of cent-seconds
    nCoinAge = 0;

    if (txCoinStake.IsCoinBase())
        return true;

    LOCK(cs_main);

    const CWalletTx *txPrev = NULL;
    BOOST_FOREACH(const CTxIn& txin, txCoinStake.vin)
    {
        for (std::vector<const CWalletTx*>::iterator it = vWtxPrev.begin(); it != vWtxPrev.end(); ++it)
        {
            if ((*it)->GetHash() == txin.prevout.hash)
                txPrev = *it;
        };

        if (!txPrev)
        {
            LogPrintf("GetCoinAgeThin txPrev not found.\n");
            continue;
        };

        if (txCoinStake.nTime < txPrev->nTime)
            return false;  // Transaction timestamp violation

        int64_t nBlockTime = 0;

        std::map<uint256, CBlockThinIndex*>::iterator mi = mapBlockThinIndex.find(txPrev->hashBlock);
        if (mi == mapBlockThinIndex.end())
        {
            if (fThinFullIndex
                || !pindexRear)
            {
                LogPrintf("GetCoinAgeThin Header not in chain %s.\n", txPrev->hashBlock.ToString().c_str());
                continue;
            };

            CTxDB txdb("r");
            CDiskBlockThinIndex diskindex;
            if (!txdb.ReadBlockThinIndex(txPrev->hashBlock, diskindex)
                || diskindex.hashNext == 0)
            {
                LogPrintf("GetCoinAgeThin Header not in chain %s.\n", txPrev->hashBlock.ToString().c_str());
                continue;
            };

            nBlockTime = diskindex.GetBlockTime();

        } else
        {
            nBlockTime = (*mi).second->GetBlockTime();
        };

        if (nBlockTime + nStakeMinAge > txCoinStake.nTime)
            continue; // only count coins meeting min age requirement

        int64_t nValueIn = txPrev->vout[txin.prevout.n].nValue;
        bnCentSecond += CBigNum(nValueIn) * (txCoinStake.nTime-txPrev->nTime) / CENT;

        if (fDebugPoS)
            LogPrintf("coin age nValueIn=%d nTimeDiff=%d bnCentSecond=%s\n", nValueIn, txCoinStake.nTime - txPrev->nTime, bnCentSecond.ToString().c_str());
    };


    CBigNum bnCoinDay = bnCentSecond * CENT / COIN / (24 * 60 * 60);
    if (fDebugPoS)
        LogPrintf("coin age bnCoinDay=%s\n", bnCoinDay.ToString().c_str());
    nCoinAge = bnCoinDay.getuint64();
    return true;
}

bool GetNodeStateStats(NodeId nodeid, CNodeStateStats &stats)
{
    // TODO:
    return false;
}



//////////////////////////////////////////////////////////////////////////////
//
// mapOrphanTransactions
//

bool AddOrphanTx(const CTransaction& tx)
{
    uint256 hash = tx.GetHash();
    if (mapOrphanTransactions.count(hash))
        return false;

    // Ignore big transactions, to avoid a
    // send-big-orphans memory exhaustion attack. If a peer has a legitimate
    // large transaction with a missing parent then we assume
    // it will rebroadcast it later, after the parent transaction(s)
    // have been mined or received.
    // 10,000 orphans, each of which is at most 5,000 bytes big is
    // at most 500 megabytes of orphans:

    size_t nSize = tx.GetSerializeSize(SER_NETWORK, CTransaction::CURRENT_VERSION);

    if (nSize > 5000)
    {
        LogPrintf("ignoring large orphan tx (size: %u, hash: %s)\n", nSize, hash.ToString().substr(0,10).c_str());
        return false;
    };

    mapOrphanTransactions[hash] = tx;
    BOOST_FOREACH(const CTxIn& txin, tx.vin)
        mapOrphanTransactionsByPrev[txin.prevout.hash].insert(hash);

    LogPrintf("stored orphan tx %s (mapsz %u)\n", hash.ToString().substr(0,10).c_str(),
        mapOrphanTransactions.size());
    return true;
}

void static EraseOrphanTx(uint256 hash)
{
    if (!mapOrphanTransactions.count(hash))
        return;

    const CTransaction& tx = mapOrphanTransactions[hash];
    BOOST_FOREACH(const CTxIn& txin, tx.vin)
    {
        mapOrphanTransactionsByPrev[txin.prevout.hash].erase(hash);
        if (mapOrphanTransactionsByPrev[txin.prevout.hash].empty())
            mapOrphanTransactionsByPrev.erase(txin.prevout.hash);
    };

    mapOrphanTransactions.erase(hash);
}

unsigned int LimitOrphanTxSize(unsigned int nMaxOrphans)
{
    unsigned int nEvicted = 0;
    while (mapOrphanTransactions.size() > nMaxOrphans)
    {
        // Evict a random orphan:
        uint256 randomhash = GetRandHash();
        map<uint256, CTransaction>::iterator it = mapOrphanTransactions.lower_bound(randomhash);
        if (it == mapOrphanTransactions.end())
            it = mapOrphanTransactions.begin();
        EraseOrphanTx(it->first);
        ++nEvicted;
    }
    return nEvicted;
}







//////////////////////////////////////////////////////////////////////////////
//
// CTransaction and CTxIndex
//

bool CTransaction::ReadFromDisk(CTxDB& txdb, COutPoint prevout, CTxIndex& txindexRet)
{
    SetNull();
    if (!txdb.ReadTxIndex(prevout.hash, txindexRet))
        return false;
    if (!ReadFromDisk(txindexRet.pos))
        return false;
    if (prevout.n >= vout.size())
    {
        SetNull();
        return false;
    }
    return true;
}

bool CTransaction::ReadFromDisk(CTxDB& txdb, COutPoint prevout)
{
    CTxIndex txindex;
    return ReadFromDisk(txdb, prevout, txindex);
}

bool CTransaction::ReadFromDisk(COutPoint prevout)
{
    CTxDB txdb("r");
    CTxIndex txindex;
    return ReadFromDisk(txdb, prevout, txindex);
}

bool CTransaction::IsStandard() const
{
    if (nVersion > CTransaction::CURRENT_VERSION
        && nVersion != ANON_TXN_VERSION)
        return false;

    BOOST_FOREACH(const CTxIn& txin, vin)
    {
        if (txin.IsAnonInput())
        {
            int nRingSize = txin.ExtractRingSize();
            
            if (nVersion != ANON_TXN_VERSION
                || nRingSize < (int)MIN_RING_SIZE
                || nRingSize > (int)MAX_RING_SIZE
                || txin.scriptSig.size() > sizeof(COutPoint) + 2 + (33 + 32 + 32) * nRingSize)
            {
                LogPrintf("IsStandard() anon txin failed.\n");
                return false;
            };
            continue;
        };

        // Biggest 'standard' txin is a 3-signature 3-of-3 CHECKMULTISIG
        // pay-to-script-hash, which is 3 ~80-byte signatures, 3
        // ~65-byte public keys, plus a few script ops.
        if (txin.scriptSig.size() > 500)
            return false;
        if (!txin.scriptSig.IsPushOnly())
            return false;
        if (fEnforceCanonical && !txin.scriptSig.HasCanonicalPushes())
            return false;
    };

    unsigned int nDataOut = 0;
    unsigned int nTxnOut = 0;

    txnouttype whichType;
    BOOST_FOREACH(const CTxOut& txout, vout)
    {
        if (txout.IsAnonOutput())
        {
            if (nVersion != ANON_TXN_VERSION
                || txout.nValue < 1
                || txout.scriptPubKey.size() > MIN_ANON_OUT_SIZE + MAX_ANON_NARRATION_SIZE)
            {
                LogPrintf("IsStandard() anon txout failed.\n");
                return false;
            }
            //nTxnOut++; anon outputs don't count (narrations are embedded in scriptPubKey)
            continue;
        };

        if (!::IsStandard(txout.scriptPubKey, whichType))
            return false;
        if (whichType == TX_NULL_DATA)
        {
            nDataOut++;
        } else
        {
            if (txout.nValue == 0)
                return false;
            nTxnOut++;
        };

        if (fEnforceCanonical && !txout.scriptPubKey.HasCanonicalPushes())
        {
            return false;
        };
    };

    // only one OP_RETURN txout per txn out is permitted
    if (nDataOut > nTxnOut)
        return false;

    return true;
}

//
// Check transaction inputs, and make sure any
// pay-to-script-hash transactions are evaluating IsStandard scripts
//
// Why bother? To avoid denial-of-service attacks; an attacker
// can submit a standard HASH... OP_EQUAL transaction,
// which will get accepted into blocks. The redemption
// script can be anything; an attacker could use a very
// expensive-to-check-upon-redemption script like:
//   DUP CHECKSIG DROP ... repeated 100 times... OP_1
//
bool CTransaction::AreInputsStandard(const MapPrevTx& mapInputs) const
{
    if (IsCoinBase())
        return true; // Coinbases don't use vin normally

    for (unsigned int i = 0; i < vin.size(); i++)
    {
        if (nVersion == ANON_TXN_VERSION
            && vin[i].IsAnonInput())
            continue;

        const CTxOut& prev = GetOutputFor(vin[i], mapInputs);

        vector<vector<unsigned char> > vSolutions;
        txnouttype whichType;
        // get the scriptPubKey corresponding to this input:
        const CScript& prevScript = prev.scriptPubKey;
        if (!Solver(prevScript, whichType, vSolutions))
            return false;
        int nArgsExpected = ScriptSigArgsExpected(whichType, vSolutions);
        if (nArgsExpected < 0)
            return false;

        // Transactions with extra stuff in their scriptSigs are
        // non-standard. Note that this EvalScript() call will
        // be quick, because if there are any operations
        // beside "push data" in the scriptSig the
        // IsStandard() call returns false
        vector<vector<unsigned char> > stack;
        if (!EvalScript(stack, vin[i].scriptSig, *this, i, SCRIPT_VERIFY_NONE, 0))
            return false;

        if (whichType == TX_SCRIPTHASH)
        {
            if (stack.empty())
                return false;
            CScript subscript(stack.back().begin(), stack.back().end());
            vector<vector<unsigned char> > vSolutions2;
            txnouttype whichType2;
            if (!Solver(subscript, whichType2, vSolutions2))
                return false;
            if (whichType2 == TX_SCRIPTHASH)
                return false;

            int tmpExpected;
            tmpExpected = ScriptSigArgsExpected(whichType2, vSolutions2);
            if (tmpExpected < 0)
                return false;
            nArgsExpected += tmpExpected;
        };

        if (stack.size() != (unsigned int)nArgsExpected)
            return false;
    };

    return true;
}

bool CTransaction::HasStealthOutput() const
{
    // -- todo: scan without using GetOp

    std::vector<uint8_t> vchEphemPK;
    opcodetype opCode;

    for (vector<CTxOut>::const_iterator it = vout.begin(); it != vout.end(); ++it)
    {
        if (nVersion == ANON_TXN_VERSION
            && it->IsAnonOutput())
            continue;

        CScript::const_iterator itScript = it->scriptPubKey.begin();

        if (!it->scriptPubKey.GetOp(itScript, opCode, vchEphemPK)
            || opCode != OP_RETURN
            || !it->scriptPubKey.GetOp(itScript, opCode, vchEphemPK) // rule out np narrations
            || vchEphemPK.size() != ec_compressed_size)
            continue;

        return true;
    };

    return false;
};

unsigned int CTransaction::GetLegacySigOpCount() const
{
    unsigned int nSigOps = 0;
    BOOST_FOREACH(const CTxIn& txin, vin)
    {
        nSigOps += txin.scriptSig.GetSigOpCount(false);
    };

    BOOST_FOREACH(const CTxOut& txout, vout)
    {
        nSigOps += txout.scriptPubKey.GetSigOpCount(false);
    };
    return nSigOps;
}


int CMerkleTx::SetMerkleBranch(const CBlock* pblock)
{
    AssertLockHeld(cs_main);

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
    };

    // Update the tx's hashBlock
    hashBlock = pblock->GetHash();

    // Locate the transaction
    for (nIndex = 0; nIndex < (int)pblock->vtx.size(); nIndex++)
        if (pblock->vtx[nIndex] == *(CTransaction*)this)
            break;

    if (nIndex == (int)pblock->vtx.size())
    {
        vMerkleBranch.clear();
        nIndex = -1;
        LogPrintf("ERROR: SetMerkleBranch() : couldn't find tx in block\n");
        return 0;
    };

    // Fill in merkle branch
    vMerkleBranch = pblock->GetMerkleBranch(nIndex);

    // Is the tx in a block that's in the main chain
    map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hashBlock);

    if (mi == mapBlockIndex.end())
        return 0;
    CBlockIndex* pindex = (*mi).second;
    if (!pindex || !pindex->IsInMainChain())
        return 0;

    return pindexBest->nHeight - pindex->nHeight + 1;
}


bool CTransaction::CheckTransaction() const
{
    // Basic checks that don't depend on any context
    if (vin.empty())
        return DoS(10, error("CTransaction::CheckTransaction() : vin empty"));
    if (vout.empty())
        return DoS(10, error("CTransaction::CheckTransaction() : vout empty"));
    // Size limits
    if (::GetSerializeSize(*this, SER_NETWORK, PROTOCOL_VERSION) > MAX_BLOCK_SIZE)
        return DoS(100, error("CTransaction::CheckTransaction() : size limits failed"));

    // Check for negative or overflow output values
    int64_t nValueOut = 0;
    for (unsigned int i = 0; i < vout.size(); i++)
    {
        const CTxOut& txout = vout[i];
        if (txout.IsEmpty() && !IsCoinBase() && !IsCoinStake())
            return DoS(100, error("CTransaction::CheckTransaction() : txout empty for user transaction"));
        if (txout.nValue < 0)
            return DoS(100, error("CTransaction::CheckTransaction() : txout.nValue negative"));
        if (txout.nValue > MAX_MONEY)
            return DoS(100, error("CTransaction::CheckTransaction() : txout.nValue too high"));
        nValueOut += txout.nValue;
        if (!MoneyRange(nValueOut))
            return DoS(100, error("CTransaction::CheckTransaction() : txout total out of range"));
    };

    // Check for duplicate inputs
    set<COutPoint> vInOutPoints;
    BOOST_FOREACH(const CTxIn& txin, vin)
    {
        if (nVersion == ANON_TXN_VERSION
            && txin.IsAnonInput())
        {
            // -- blank the upper 3 bytes of n to prevent the same keyimage passing with different ring sizes
            COutPoint opTest = txin.prevout;
            opTest.n &= 0xFF;
            if (vInOutPoints.count(opTest))
            {
                if (fDebugRingSig)
                    LogPrintf("CheckTransaction() failed - found duplicate keyimage in txn %s\n", GetHash().ToString().c_str());
                return false;
            };
            vInOutPoints.insert(opTest);
            continue;
        };
        
        if (vInOutPoints.count(txin.prevout))
            return false;
        vInOutPoints.insert(txin.prevout);
    };
    
    
    if (nVersion == ANON_TXN_VERSION)
    {
        // -- Prevent anon txns in live until after block 500
        
        if (!fTestNet && nBestHeight < 500)
        {
            LogPrintf("CheckTransaction() failed - anon txn in live before block 500, %s.\n", GetHash().ToString().c_str());
            return false;
        };
        
        
        // -- Check for duplicate anon outputs
        // NOTE: is this necessary, duplicate coins would not be spendable anyway?
        set<CPubKey> vAnonOutPubkeys;
        CPubKey pkTest;
        BOOST_FOREACH(const CTxOut& txout, vout)
        {
            if (!txout.IsAnonOutput())
                continue;

            const CScript &s = txout.scriptPubKey;
            pkTest = CPubKey(&s[2+1], 33);
            if (vAnonOutPubkeys.count(pkTest))
                return false;
            vAnonOutPubkeys.insert(pkTest);
        };
    };


    if (IsCoinBase())
    {
        if (vin[0].scriptSig.size() < 2 || vin[0].scriptSig.size() > 100)
            return DoS(100, error("CTransaction::CheckTransaction() : coinbase script size is invalid"));
    } else
    {
        BOOST_FOREACH(const CTxIn& txin, vin)
            if (txin.prevout.IsNull())
                return DoS(10, error("CTransaction::CheckTransaction() : prevout is null"));
    };


    return true;
}

int64_t CTransaction::GetMinFee(unsigned int nBlockSize, enum GetMinFee_mode mode, unsigned int nBytes) const
{
    // Base fee is either MIN_TX_FEE or MIN_RELAY_TX_FEE for standard txns, and MIN_TX_FEE_ANON for anon txns

    // -- force GMF_ANON if anon txn
    if (nVersion == ANON_TXN_VERSION)
        mode = GMF_ANON;

    int64_t nBaseFee;
    switch (mode)
    {
        case GMF_RELAY: nBaseFee = MIN_RELAY_TX_FEE; break;
        case GMF_ANON:  nBaseFee = MIN_TX_FEE_ANON;  break;
        default:        nBaseFee = MIN_TX_FEE;       break;
    };

    unsigned int nNewBlockSize = nBlockSize + nBytes;
    int64_t nMinFee = (1 + (int64_t)nBytes / 1000) * nBaseFee;

    // To limit dust spam, require MIN_TX_FEE/MIN_RELAY_TX_FEE if any output is less than 0.01
    if (nMinFee < nBaseFee)
    {
        BOOST_FOREACH(const CTxOut& txout, vout)
            if (txout.nValue < CENT)
                nMinFee = nBaseFee;
    };

    // Raise the price as the block approaches full
    if (mode != GMF_ANON && nBlockSize != 1 && nNewBlockSize >= MAX_BLOCK_SIZE_GEN/2)
    {
        if (nNewBlockSize >= MAX_BLOCK_SIZE_GEN)
            return MAX_MONEY;
        nMinFee *= MAX_BLOCK_SIZE_GEN / (MAX_BLOCK_SIZE_GEN - nNewBlockSize);
    };

    if (!MoneyRange(nMinFee))
        nMinFee = MAX_MONEY;
    return nMinFee;
}

bool AcceptToMemoryPool(CTxMemPool &pool, CTransaction &tx, CTxDB &txdb, bool *pfMissingInputs)
{
    AssertLockHeld(cs_main);
    if (pfMissingInputs)
        *pfMissingInputs = false;

    if (!tx.CheckTransaction())
        return error("AcceptToMemoryPool() : CheckTransaction failed");

    // Coinbase is only valid in a block, not as a loose transaction
    if (tx.IsCoinBase())
        return tx.DoS(100, error("AcceptToMemoryPool() : coinbase as individual tx"));

    // ppcoin: coinstake is also only valid in a block, not as a loose transaction
    if (tx.IsCoinStake())
        return tx.DoS(100, error("AcceptToMemoryPool() : coinstake as individual tx"));

    // Rather not work on nonstandard transactions (unless -testnet)
    //if (!fTestNet && !tx.IsStandard())
    if (!tx.IsStandard())
        return error("AcceptToMemoryPool() : nonstandard transaction type");

    // Do we already have it?
    uint256 hash = tx.GetHash();

    if (pool.exists(hash))
        return false;
    
    // Check for conflicts with in-memory transactions
    {
    LOCK(pool.cs); // protect pool.mapNextTx
    for (unsigned int i = 0; i < tx.vin.size(); i++)
    {
        COutPoint outpoint = tx.vin[i].prevout;
        if (pool.mapNextTx.count(outpoint))
        {
            // Disable replacement feature for now
            return false;
        };
    };
    }
    
    if (txdb.ContainsTx(hash))
        return false;
    
    {
        MapPrevTx mapInputs;
        map<uint256, CTxIndex> mapUnused;
        bool fInvalid = false;

        int64_t nFees;
        if (nNodeMode == NT_FULL)
        {
            if (!tx.FetchInputs(txdb, mapUnused, false, false, mapInputs, fInvalid))
            {
                if (fInvalid)
                    return error("AcceptToMemoryPool() : FetchInputs found invalid tx %s", hash.ToString().substr(0,10).c_str());
                if (pfMissingInputs)
                    *pfMissingInputs = true;
                return false;
            };

            // Check for non-standard pay-to-script-hash in inputs
            if (!tx.AreInputsStandard(mapInputs) && !fTestNet)
                return error("AcceptToMemoryPool() : nonstandard transaction input");

            nFees = tx.GetValueIn(mapInputs) - tx.GetValueOut();

            GetMinFee_mode feeMode = GMF_RELAY;

            if (tx.nVersion == ANON_TXN_VERSION)
            {
                int64_t nSumAnon;
                if (!tx.CheckAnonInputs(txdb, nSumAnon, fInvalid, true))
                {
                    if (fInvalid)
                        return error("AcceptToMemoryPool() : CheckAnonInputs found invalid tx %s", hash.ToString().substr(0,10).c_str());
                    if (pfMissingInputs)
                        *pfMissingInputs = true;
                    return false;
                };

                nFees += nSumAnon;

                feeMode = GMF_ANON;
            };


            // Note: if you modify this code to accept non-standard transactions, then
            // you should add code here to check that the transaction does a
            // reasonable number of ECDSA signature verifications.

            unsigned int nSize = ::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION);

            // Don't accept it if it can't get into a block

            int64_t txMinFee = tx.GetMinFee(1000, feeMode, nSize);

            if (nFees < txMinFee)
            {
                return error("AcceptToMemoryPool() : not enough fees %s, %d < %d",
                             hash.ToString().c_str(),
                             nFees, txMinFee);
            };

            // Continuously rate-limit free transactions
            // This mitigates 'penny-flooding' -- sending thousands of free transactions just to
            // be annoying or make others' transactions take longer to confirm.
            if (nFees < MIN_RELAY_TX_FEE)
            {
                static CCriticalSection csFreeLimiter;
                static double dFreeCount;
                static int64_t nLastTime;
                int64_t nNow = GetTime();
                
                LOCK(csFreeLimiter);
                // Use an exponentially decaying ~10-minute window:
                dFreeCount *= pow(1.0 - 1.0/600.0, (double)(nNow - nLastTime));
                nLastTime = nNow;
                // -limitfreerelay unit is thousand-bytes-per-minute
                // At default rate it would take over a month to fill 1GB
                if (dFreeCount > GetArg("-limitfreerelay", 15)*10*1000 && !IsFromMe(tx))
                    return error("AcceptToMemoryPool() : free transaction rejected by rate limiter");
                if (fDebug)
                    LogPrintf("Rate limit dFreeCount: %g => %g\n", dFreeCount, dFreeCount+nSize);
                dFreeCount += nSize;
            };

            // Check against previous transactions
            // This is done last to help prevent CPU exhaustion denial-of-service attacks.
            if (!tx.ConnectInputs(txdb, mapInputs, mapUnused, CDiskTxPos(1,1,1), pindexBest, false, false, MANDATORY_SCRIPT_VERIFY_FLAGS))
            {
                return error("AcceptToMemoryPool() : ConnectInputs failed %s", hash.ToString().substr(0,10).c_str());
            };
        };
    }
    
    // Store transaction in memory
    pool.addUnchecked(hash, tx);
    
    LogPrintf("AcceptToMemoryPool() : accepted %s (poolsz %u)\n",
        hash.ToString().substr(0,10).c_str(),
        pool.mapTx.size());
    return true;
}



bool CWalletTx::AcceptWalletTransaction(CTxDB& txdb)
{
    {
        // Add previous supporting transactions first
        BOOST_FOREACH(CMerkleTx& tx, vtxPrev)
        {
            if (!(tx.IsCoinBase() || tx.IsCoinStake()))
            {
                uint256 hash = tx.GetHash();
                if (!mempool.exists(hash) && !txdb.ContainsTx(hash))
                    ::AcceptToMemoryPool(mempool, tx, txdb);
            };
        };
        return ::AcceptToMemoryPool(mempool, *this, txdb);
    }
    return false;
}

bool CWalletTx::AcceptWalletTransaction()
{
    CTxDB txdb("r");
    return AcceptWalletTransaction(txdb);
}

int CTxIndex::GetDepthInMainChainFromIndex() const
{
    if (nNodeMode != NT_FULL)
    {
        // Read block header
        CBlockThin block;
        if (!block.ReadBlockThinFromDisk(pos.nFile, pos.nBlockPos))
            return 0;

        map<uint256, CBlockThinIndex*>::iterator mi = mapBlockThinIndex.find(block.GetHash());
        if (mi == mapBlockThinIndex.end())
            return 0;
        CBlockThinIndex* pindex = (*mi).second;
        if (!pindex || !pindex->IsInMainChain())
            return 0;
        return 1 + nBestHeight - pindex->nHeight;
    };

    // Read block header
    CBlock block;
    if (!block.ReadFromDisk(pos.nFile, pos.nBlockPos, false))
        return 0;
    // Find the block in the index
    map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(block.GetHash());
    if (mi == mapBlockIndex.end())
        return 0;
    CBlockIndex* pindex = (*mi).second;
    if (!pindex || !pindex->IsInMainChain())
        return 0;
    return 1 + nBestHeight - pindex->nHeight;
}

// Return transaction in tx, and if it was found inside a block, its hash is placed in hashBlock
bool GetTransaction(const uint256 &hash, CTransaction &tx, uint256 &hashBlock)
{
    {
        LOCK(cs_main);
        {
            if (mempool.lookup(hash, tx))
            {
                return true;
            }
        }
        CTxDB txdb("r");
        CTxIndex txindex;
        if (tx.ReadFromDisk(txdb, COutPoint(hash, 0), txindex))
        {
            CBlock block;
            if (block.ReadFromDisk(txindex.pos.nFile, txindex.pos.nBlockPos, false))
                hashBlock = block.GetHash();
            return true;
        }
    }
    return false;
}

bool GetTransactionBlockHash(const uint256 &hash, uint256 &hashBlock)
{
    {
        LOCK(cs_main);
        CTxDB txdb("r");
        CTxIndex txindex;
        if (txdb.ReadTxIndex(hash, txindex))
        {
            CBlock block;
            if (block.ReadFromDisk(txindex.pos.nFile, txindex.pos.nBlockPos, false))
            {
                hashBlock = block.GetHash();
                return true;
            };
        };
    }
    return false;
}

bool GetKeyImage(CTxDB* ptxdb, ec_point& keyImage, CKeyImageSpent& keyImageSpent, bool& fInMempool)
{
    AssertLockHeld(cs_main);
    
    
    // -- check txdb first
    fInMempool = false;
    if (ptxdb->ReadKeyImage(keyImage, keyImageSpent))
        return true;
    
    if (mempool.lookupKeyImage(keyImage, keyImageSpent))
    {
        fInMempool = true;
        return true;
    };
    
    return false;
};

bool TxnHashInSystem(CTxDB* ptxdb, uint256& txnHash)
{
    // -- is the transaction hash known in the system
    
    AssertLockHeld(cs_main);
    
    // TODO: thin mode
    
    if (mempool.exists(txnHash))
        return true;
    
    CTxIndex txnIndex;
    if (ptxdb->ReadTxIndex(txnHash, txnIndex))
    {
        if (txnIndex.GetDepthInMainChainFromIndex() > 0)
            return true;
    };
    
    return false;
};


//////////////////////////////////////////////////////////////////////////////
//
// CBlockThin and CBlockThinIndex
//

static CBlockThinIndex* pblockHeaderIndexFBBHLast = NULL;
CBlockThinIndex* FindBlockThinByHeight(int nHeight)
{
    CBlockThinIndex *pblockindex;
    if (nHeight < nBestHeight / 2)
        pblockindex = pindexGenesisBlockThin;
    else
        pblockindex = pindexBestHeader;

    if (!pblockindex)
        return NULL;

    if (pblockHeaderIndexFBBHLast && abs(nHeight - pblockindex->nHeight) > abs(nHeight - pblockHeaderIndexFBBHLast->nHeight))
        pblockindex = pblockHeaderIndexFBBHLast;

    while (pblockindex
        && pblockindex->nHeight > nHeight
        && pblockindex->pprev)
        pblockindex = pblockindex->pprev;

    while (pblockindex
        && pblockindex->nHeight < nHeight
        && pblockindex->pnext)
        pblockindex = pblockindex->pnext;

    if (!pblockindex)
        return NULL;

    if (pblockindex->nHeight != nHeight)
        return NULL;

    pblockHeaderIndexFBBHLast = pblockindex;
    return pblockindex;
}

void static InvalidHeaderChainFound(CBlockThinIndex* pindexNew)
{
    if (pindexNew->nChainTrust > nBestInvalidTrust)
    {
        nBestInvalidTrust = pindexNew->nChainTrust;
        //CTxDB().WriteBestInvalidTrust(CBigNum(nBestInvalidTrust));
        uiInterface.NotifyBlocksChanged();
    };

    uint256 nBestInvalidBlockTrust = pindexNew->nChainTrust - pindexNew->pprev->nChainTrust;
    uint256 nBestBlockTrust = pindexBest->nHeight != 0 ? (pindexBest->nChainTrust - pindexBest->pprev->nChainTrust) : pindexBest->nChainTrust;

    LogPrintf("InvalidHeaderChainFound: invalid block=%s  height=%d  trust=%s  blocktrust=%d  date=%s\n",
        pindexNew->GetBlockHash().ToString().substr(0,20).c_str(), pindexNew->nHeight,
        CBigNum(pindexNew->nChainTrust).ToString().c_str(), nBestInvalidBlockTrust.Get64(),
        DateTimeStrFormat("%x %H:%M:%S", pindexNew->GetBlockTime()).c_str());

    LogPrintf("InvalidHeaderChainFound:  current best=%s  height=%d  trust=%s  blocktrust=%d  date=%s\n",
        hashBestChain.ToString().substr(0,20).c_str(), nBestHeight,
        CBigNum(pindexBest->nChainTrust).ToString().c_str(),
        nBestBlockTrust.Get64(),
        DateTimeStrFormat("%x %H:%M:%S", pindexBest->GetBlockTime()).c_str());
}

bool static ReorganizeHeaders(CTxDB& txdb, CBlockThinIndex* pindexNew)
{
    LogPrintf("REORGANIZE HEADERS\n");
    
    // Find the fork
    CBlockThinIndex* pfork = pindexBestHeader;
    CBlockThinIndex* plonger = pindexNew;
    while (pfork != plonger)
    {
        while (plonger->nHeight > pfork->nHeight)
            if (!(plonger = plonger->pprev))
                return error("ReorganizeHeaders() : plonger->pprev is null");
        
        if (pfork == plonger)
            break;
        if (!(pfork = pfork->pprev))
            return error("ReorganizeHeaders() : pfork->pprev is null");
    };

    // List of what to disconnect
    vector<CBlockThinIndex*> vDisconnect;
    for (CBlockThinIndex* pindex = pindexBestHeader; pindex != pfork; pindex = pindex->pprev)
        vDisconnect.push_back(pindex);

    // List of what to connect
    vector<CBlockThinIndex*> vConnect;
    for (CBlockThinIndex* pindex = pindexNew; pindex != pfork; pindex = pindex->pprev)
        vConnect.push_back(pindex);
    reverse(vConnect.begin(), vConnect.end());

    LogPrintf("REORGANIZE HEADERS: Disconnect %u blocks; %s..%s\n", vDisconnect.size(), pfork->GetBlockHash().ToString().substr(0,20).c_str(), pindexBestHeader->GetBlockHash().ToString().substr(0,20).c_str());
    LogPrintf("REORGANIZE HEADERS: Connect %u blocks; %s..%s\n", vConnect.size(), pfork->GetBlockHash().ToString().substr(0,20).c_str(), pindexNew->GetBlockHash().ToString().substr(0,20).c_str());

    // Disconnect shorter branch
    list<CTransaction> vResurrect;
    BOOST_FOREACH(CBlockThinIndex* pindex, vDisconnect)
    {
        CBlockThin block = pindex->GetBlockThin();
        
        if (!block.DisconnectBlockThin(txdb, pindex))
            return error("ReorganizeHeaders() : DisconnectBlockThin %s failed", pindex->GetBlockHash().ToString().substr(0,20).c_str());
        
        // Queue memory transactions to resurrect.
        // We only do this for blocks after the last checkpoint (reorganisation before that
        // point should only happen with -reindex/-loadblock, or a misbehaving peer.

        {
            LOCK(pwalletMain->cs_wallet);

            if (pindex->nHeight > Checkpoints::GetTotalBlocksEstimate())
            {
                for (map<uint256, CWalletTx>::const_iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
                {
                    const CWalletTx& wtx = it->second;

                    // -- how to get the order of txns in block?
                    if (pindex->phashBlock && wtx.hashBlock == *pindex->phashBlock)
                        vResurrect.push_back(wtx);
                };
            };
        }

        /*
        BOOST_REVERSE_FOREACH(const CTransaction& tx, block.vtx)
            if (!(tx.IsCoinBase() || tx.IsCoinStake()) && pindex->nHeight > Checkpoints::GetTotalBlocksEstimate())
                vResurrect.push_front(tx);
        */
    };

    // Connect longer branch
    vector<CTransaction> vDelete;
    for (unsigned int i = 0; i < vConnect.size(); i++)
    {
        CBlockThinIndex* pindex = vConnect[i];
        CBlockThin block = pindex->GetBlockThin();

        if (!block.ConnectBlockThin(txdb, pindex))
        {
            // Invalid block
            return error("ReorganizeHeaders() : ConnectBlockThin %s failed", pindex->GetBlockHash().ToString().substr(0,20).c_str());
        };

        // Queue memory transactions to delete
        //BOOST_FOREACH(const CTransaction& tx, block.vtx)
        //    vDelete.push_back(tx);
    };

    if (!txdb.WriteHashBestHeaderChain(pindexNew->GetBlockHash()))
        return error("ReorganizeHeaders() : WriteHashBestHeaderChain failed");

    // Make sure it's successfully written to disk before changing memory structure
    if (!txdb.TxnCommit())
        return error("ReorganizeHeaders() : TxnCommit failed");

    // Disconnect shorter branch
    BOOST_FOREACH(CBlockThinIndex* pindex, vDisconnect)
        if (pindex->pprev)
            pindex->pprev->pnext = NULL;

    // Connect longer branch
    BOOST_FOREACH(CBlockThinIndex* pindex, vConnect)
        if (pindex->pprev)
            pindex->pprev->pnext = pindex;

    // Resurrect memory transactions that were in the disconnected branch
    BOOST_FOREACH(CTransaction& tx, vResurrect)
        AcceptToMemoryPool(mempool, tx, txdb);

    // Delete redundant memory transactions that are in the connected branch
    BOOST_FOREACH(CTransaction& tx, vDelete)
    {
        mempool.remove(tx);
        mempool.removeConflicts(tx);
    };


    LogPrintf("REORGANIZE HEADERS: done\n");

    return true;
}

bool CBlockThin::CheckBlockThin(bool fCheckPOW, bool fCheckMerkleRoot, bool fCheckSig) const
{
    // Check proof of work matches claimed amount
    if (fCheckPOW && IsProofOfWork() && !CheckProofOfWork(GetHash(), nBits))
        return DoS(50, error("CheckBlockThin() : proof of work failed"));

    return true;
}

bool CBlockThin::AcceptBlockThin()
{
    AssertLockHeld(cs_main);
    
    assert(nNodeMode == NT_THIN);
    

    if (nVersion > CURRENT_VERSION)
        return error("AcceptBlockThin() : reject unknown block version %d", nVersion);

    // Check for duplicate
    uint256 hash = GetHash();
    if (mapBlockThinIndex.count(hash))
        return error("AcceptBlockThin() : header already in mapBlockThinIndex");

    // Get prev block index
    map<uint256, CBlockThinIndex*>::iterator mi = mapBlockThinIndex.find(hashPrevBlock);

    if (mi == mapBlockThinIndex.end())
        return error("AcceptBlockThin() : prev header not found");
    
    CBlockThinIndex* pindexPrev = (*mi).second;
    int nHeight = pindexPrev->nHeight+1;

    if (IsProofOfWork() && nHeight > Params().LastPOWBlock())
        return DoS(100, error("AcceptBlockThin() : reject proof-of-work at height %d", nHeight));

    if (IsProofOfStake() && nHeight < Params().FirstPosBlock())
        return DoS(100, error("AcceptBlock() : reject proof-of-stake at height %d", nHeight));

    // Check proof-of-work or proof-of-stake
    if (nBits != GetNextTargetRequiredThin(pindexPrev, IsProofOfStake()))
        return DoS(100, error("AcceptBlockThin() : incorrect %s", IsProofOfWork() ? "proof-of-work" : "proof-of-stake"));

    // Check timestamp against prev
    if (GetBlockTime() <= pindexPrev->GetPastTimeLimit() || FutureDrift(GetBlockTime(), nHeight) < pindexPrev->GetBlockTime())
        return error("AcceptBlockThin() : header's timestamp is too early");

    // Check that the block chain matches the known block chain up to a checkpoint
    if (!Checkpoints::CheckHardened(nHeight, hash))
        return error("AcceptBlockThin() : rejected by hardened checkpoint lock-in at %d", nHeight);
        
    /*
    // Verify hash target and signature of coinstake tx
    if (IsProofOfStake())
    {
        uint256 targetProofOfStake;
        if (!CheckProofOfStake(vtx[1], nBits, hashProof, targetProofOfStake))
        {
            LogPrintf("WARNING: AcceptHeader(): check proof-of-stake failed for block %s\n", hash.ToString().c_str());
            return false; // do not error here as we expect this during initial block download
        };
    };
    */

    /*
    // PoW is checked in CheckBlock()
    if (IsProofOfWork())
    {
        hashProof = GetHash();
    };
    */

    //bool cpSatisfies = Checkpoints::CheckSync(hash, pindexPrev);

    // Check that the block satisfies synchronized checkpoint
    //if (CheckpointsMode == Checkpoints::STRICT && !cpSatisfies)
    //    return error("AcceptHeader() : rejected by synchronized checkpoint");

    //if (CheckpointsMode == Checkpoints::ADVISORY && !cpSatisfies)
    //    strMiscWarning = _("WARNING: syncronized checkpoint violation detected, but skipped!");

    /*
    // Enforce rule that the coinbase starts with serialized block height
    CScript expect = CScript() << nHeight;
    if (vtx[0].vin[0].scriptSig.size() < expect.size() ||
        !std::equal(expect.begin(), expect.end(), vtx[0].vin[0].scriptSig.begin()))
        return DoS(100, error("AcceptHeader() : block height mismatch in coinbase"));
    */

    // Write header to history file
    if (!CheckDiskSpace(::GetSerializeSize(*this, SER_DISK, CLIENT_VERSION)))
        return error("AcceptBlockThin() : out of disk space");

    unsigned int nFile = -1;
    unsigned int nBlockPos = 0;

    //if (!WriteBlockThinToDisk(nFile, nBlockPos))
    //    return error("AcceptBlockThin() : WriteBlockThinToDisk failed");

    if (!AddToBlockThinIndex(nFile, nBlockPos, hashProof))
        return error("AcceptBlockThin() : AddToBlockThinIndex failed");

    return true;
};

bool CBlockThin::AddToBlockThinIndex(unsigned int nFile, unsigned int nBlockPos, const uint256& hashProof)
{
    // Check for duplicate
    uint256 hash = GetHash();
    if (mapBlockThinIndex.count(hash))
        return error("AddToBlockThinIndex() : %s already exists", hash.ToString().substr(0,20).c_str());

    // Construct new block index object
    CBlockThinIndex* pindexNew = new CBlockThinIndex(nFile, nBlockPos, *this);
    if (!pindexNew)
        return error("AddToBlockThinIndex() : new CBlockThinIndex failed");

    pindexNew->phashBlock = &hash;
    map<uint256, CBlockThinIndex*>::iterator miPrev = mapBlockThinIndex.find(hashPrevBlock);
    if (miPrev != mapBlockThinIndex.end())
    {
        pindexNew->pprev = (*miPrev).second;
        pindexNew->nHeight = pindexNew->pprev->nHeight + 1;
    };

    // Record proof hash value
    pindexNew->hashProof = hashProof;

    // ppcoin: compute chain trust score
    pindexNew->nChainTrust = (pindexNew->pprev ? pindexNew->pprev->nChainTrust : 0) + pindexNew->GetBlockTrust();

    // ppcoin: compute stake modifier
    uint64_t nStakeModifier = 0;
    bool fGeneratedStakeModifier = false;
    if (!ComputeNextStakeModifierThin(pindexNew->pprev, nStakeModifier, fGeneratedStakeModifier))
        return error("AddToBlockThinIndex() : ComputeNextStakeModifier() failed");

    pindexNew->SetStakeModifier(nStakeModifier, fGeneratedStakeModifier);
    
    // Add to mapBlockThinIndex
    std::map<uint256, CBlockThinIndex*>::iterator mi = mapBlockThinIndex.insert(make_pair(hash, pindexNew)).first;
    //if (pindexNew->IsProofOfStake())
    //    setStakeSeen.insert(make_pair(pindexNew->prevoutStake, pindexNew->nStakeTime));
    pindexNew->phashBlock = &((*mi).first);


    // Write to disk block index
    CTxDB txdb;
    if (!txdb.TxnBegin())
        return false;

    txdb.WriteBlockThinIndex(CDiskBlockThinIndex(pindexNew));

    if (!txdb.TxnCommit())
        return false;

    // New best
    if (pindexNew->nChainTrust > nBestChainTrust)
    {
        if (!SetBestThinChain(txdb, pindexNew))
            return false;
    };


    while (!fThinFullIndex && pindexRear
        && pindexNew->nHeight - pindexRear->nHeight > nThinIndexWindow)
    {
        const uint256* pRemHash = pindexRear->phashBlock;

        pindexRear = pindexRear->pnext;
        pindexRear->pprev = NULL;

        std::map<uint256, CBlockThinIndex*>::iterator mi = mapBlockThinIndex.find(*pRemHash);

        if (mi != mapBlockThinIndex.end())
        {
            delete mi->second;
            mapBlockThinIndex.erase(mi);
        };
    };


    uiInterface.NotifyBlocksChanged();
    return true;
}


// Called from inside SetBestThinChain: attaches a block to the new best chain being built
bool CBlockThin::SetBestThinChainInner(CTxDB& txdb, CBlockThinIndex *pindexNew)
{
    uint256 hash = GetHash();

    // Adding to current best branch
    if (!ConnectBlockThin(txdb, pindexNew)
        || !txdb.WriteHashBestHeaderChain(hash))
    {
        txdb.TxnAbort();
        InvalidHeaderChainFound(pindexNew);
        return false;
    };

    if (!txdb.TxnCommit())
        return error("SetBestThinChain() : TxnCommit failed");

    // Add to current best branch
    pindexNew->pprev->pnext = pindexNew;

    return true;
}

bool CBlockThin::DisconnectBlockThin(CTxDB& txdb, CBlockThinIndex* pindex)
{
    // Update block index on disk without changing it in memory.
    // The memory index structure will be changed after the db commits.
    if (pindex->pprev)
    {
        CDiskBlockThinIndex blockindexPrev(pindex->pprev);
        blockindexPrev.hashNext = 0;
        if (!txdb.WriteBlockThinIndex(blockindexPrev))
            return error("DisconnectBlock() : WriteBlockIndex failed");
    };

    uint256 blockhash = GetHash();

    // -- search mapwallet and unspend the inputs of txns in this block
    for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
    {
        CWalletTx& wtx = (*it).second;

        if (wtx.hashBlock != blockhash)
            continue;

        // -- mark input outputs as unspent
        BOOST_FOREACH(const CTxIn& txin, wtx.vin)
        {
            std::map<uint256, CWalletTx>::iterator miIn = pwalletMain->mapWallet.find(txin.prevout.hash);
            if (miIn == pwalletMain->mapWallet.end())
                continue;

            CWalletTx& wtxPrev = (*miIn).second;

            wtxPrev.BindWallet(pwalletMain);
            wtxPrev.MarkUnspent(txin.prevout.n);
            wtxPrev.WriteToDisk();
            //pwalletMain->NotifyTransactionChanged(this, txin.prevout.hash, CT_UPDATED);
        };

        SyncWithWalletsThin(wtx, blockhash, false, false);
    };
    
    return true;
}

bool CBlockThin::ConnectBlockThin(CTxDB& txdb, CBlockThinIndex* pindex, bool fJustCheck)
{
    if (fDebugChain)
        LogPrintf("ConnectBlockThin()\n");

    if (!txdb.WriteBlockThinIndex(CDiskBlockThinIndex(pindex)))
        return error("ConnectBlockThin() : WriteBlockThinIndex for pindex failed");

    // Update block index on disk without changing it in memory.
    // The memory index structure will be changed after the db commits.
    if (pindex->pprev)
    {
        CDiskBlockThinIndex blockindexPrev(pindex->pprev);
        blockindexPrev.hashNext = pindex->GetBlockHash();
        if (!txdb.WriteBlockThinIndex(blockindexPrev))
            return error("ConnectBlockThin() : WriteBlockIndex failed");
    };

    uint256 blockhash = GetHash();
    // -- search mapwallet and spend the inputs of txns in this block
    for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
    {
        CWalletTx& wtx = (*it).second;

        if (wtx.hashBlock != blockhash)
            continue;

        // -- mark input outputs as spent
        BOOST_FOREACH(const CTxIn& txin, wtx.vin)
        {
            std::map<uint256, CWalletTx>::iterator miIn = pwalletMain->mapWallet.find(txin.prevout.hash);
            if (miIn == pwalletMain->mapWallet.end())
                continue;

            CWalletTx& wtxPrev = (*miIn).second;

            wtxPrev.BindWallet(pwalletMain);
            wtxPrev.MarkSpent(txin.prevout.n);
            wtxPrev.WriteToDisk();
            //pwalletMain->NotifyTransactionChanged(this, txin.prevout.hash, CT_UPDATED);
        };
        //SyncWithWalletsThin(wtx, blockhash, false, false);
    };
    
    return true;
}

bool CBlockThin::SetBestThinChain(CTxDB& txdb, CBlockThinIndex* pindexNew)
{
    uint256 hash = GetHash();

    if (!txdb.TxnBegin())
        return error("SetBestThinChain() : TxnBegin failed");

    if (pindexGenesisBlockThin == NULL && hash == Params().HashGenesisBlock())
    {
        txdb.WriteHashBestHeaderChain(hash);
        if (!txdb.TxnCommit())
            return error("SetBestThinChain() : TxnCommit failed");
        pindexGenesisBlockThin = pindexNew;
    } else
    if (hashPrevBlock == hashBestChain)
    {
        if (!SetBestThinChainInner(txdb, pindexNew))
            return error("SetBestThinChain() : SetBestChainInner failed");
    } else
    {
        // the first block in the new chain that will cause it to become the new best chain
        CBlockThinIndex *pindexIntermediate = pindexNew;

        // list of blocks that need to be connected afterwards
        std::vector<CBlockThinIndex*> vpindexSecondary;

        // ReorganizeHeaders is costly in terms of db load, as it works in a single db transaction.
        // Try to limit how much needs to be done inside
        while (pindexIntermediate->pprev && pindexIntermediate->pprev->nChainTrust > pindexBestHeader->nChainTrust)
        {
            vpindexSecondary.push_back(pindexIntermediate);
            pindexIntermediate = pindexIntermediate->pprev;
        };

        if (!vpindexSecondary.empty())
            LogPrintf("Postponing %u reconnects\n", vpindexSecondary.size());

        // Switch to new best branch
        if (!ReorganizeHeaders(txdb, pindexIntermediate))
        {
            txdb.TxnAbort();
            InvalidHeaderChainFound(pindexNew);
            return error("SetBestThinChain() : Reorganize failed");
        };

        // Connect further blocks
        BOOST_REVERSE_FOREACH(CBlockThinIndex *pindex, vpindexSecondary)
        {
            CBlockThin block = pindex->GetBlockThin();

            if (!txdb.TxnBegin())
            {
                LogPrintf("SetBestThinChain() : TxnBegin 2 failed\n");
                break;
            };

            // errors now are not fatal, we still did a reorganisation to a new chain in a valid way
            if (!block.SetBestThinChainInner(txdb, pindex))
                break;
        };
    };

    // Update best block in wallet (so we can detect restored wallets)
    bool fIsInitialDownload = IsInitialBlockDownload();
    if (!fIsInitialDownload)
    {

        const CBlockThinLocator locator(pindexNew);
        ::SetBestThinChain(locator);
    };

    // New best block
    hashBestChain = hash;
    pindexBestHeader = pindexNew;
    //pblockindexFBBHLast = NULL;
    nBestHeight = pindexBestHeader->nHeight;
    nBestChainTrust = pindexBestHeader->nChainTrust;
    nTimeBestReceived = GetTime();
    mempool.AddTransactionsUpdated(1);

    uint256 nBestBlockTrust = pindexBestHeader->nHeight != 0 ? (pindexBestHeader->nChainTrust - pindexBestHeader->pprev->nChainTrust) : pindexBestHeader->nChainTrust;

    if (fDebugChain)
    {
        LogPrintf("SetBestThinChain: new best=%s  height=%d  trust=%s  blocktrust=%d  date=%s\n",
          hashBestChain.ToString().substr(0,20).c_str(), nBestHeight,
          CBigNum(nBestChainTrust).ToString().c_str(),
          nBestBlockTrust.Get64(),
          DateTimeStrFormat("%x %H:%M:%S", pindexBestHeader->GetBlockTime()).c_str());
    };

    std::string strCmd = GetArg("-blocknotify", "");

    if (!fIsInitialDownload && !strCmd.empty())
    {
        boost::replace_all(strCmd, "%s", hashBestChain.GetHex());
        boost::thread t(runCommand, strCmd); // thread runs free
    };


    return true;
}


//////////////////////////////////////////////////////////////////////////////
//
// CBlock and CBlockIndex
//
static CBlockIndex* pblockindexFBBHLast;
CBlockIndex* FindBlockByHeight(int nHeight)
{
    CBlockIndex *pblockindex;
    if (nHeight < nBestHeight / 2)
        pblockindex = pindexGenesisBlock;
    else
        pblockindex = pindexBest;
    if (pblockindexFBBHLast && abs(nHeight - pblockindex->nHeight) > abs(nHeight - pblockindexFBBHLast->nHeight))
        pblockindex = pblockindexFBBHLast;

    while (pblockindex->pprev && pblockindex->nHeight > nHeight)
        pblockindex = pblockindex->pprev;

    while (pblockindex->pnext && pblockindex->nHeight < nHeight)
        pblockindex = pblockindex->pnext;

    if (pblockindex->nHeight != nHeight)
        return NULL;

    pblockindexFBBHLast = pblockindex;
    return pblockindex;
}

bool CBlock::ReadFromDisk(const CBlockIndex* pindex, bool fReadTransactions)
{
    if (!fReadTransactions)
    {
        *this = pindex->GetBlockHeader();
        return true;
    };

    if (!ReadFromDisk(pindex->nFile, pindex->nBlockPos, fReadTransactions))
        return false;
    if (GetHash() != pindex->GetBlockHash())
        return error("CBlock::ReadFromDisk() : GetHash() doesn't match index");
    return true;
}

uint256 static GetOrphanRoot(const CBlock* pblock)
{
    // Work back to the first block in the orphan chain
    while (mapOrphanBlocks.count(pblock->hashPrevBlock))
        pblock = mapOrphanBlocks[pblock->hashPrevBlock];
    return pblock->GetHash();
}

uint256 static GetOrphanHeaderRoot(const CBlockThin* pblock)
{
    // Work back to the first block in the orphan chain
    while (mapOrphanBlockThins.count(pblock->hashPrevBlock))
        pblock = mapOrphanBlockThins[pblock->hashPrevBlock];
    return pblock->GetHash();
}

// ppcoin: find block wanted by given orphan block
uint256 WantedByOrphan(const CBlock* pblockOrphan)
{
    // Work back to the first block in the orphan chain
    while (mapOrphanBlocks.count(pblockOrphan->hashPrevBlock))
        pblockOrphan = mapOrphanBlocks[pblockOrphan->hashPrevBlock];
    return pblockOrphan->hashPrevBlock;
}

uint256 WantedByOrphanHeader(const CBlockThin* pblockOrphan)
{
    // Work back to the first block in the orphan chain
    while (mapOrphanBlockThins.count(pblockOrphan->hashPrevBlock))
        pblockOrphan = mapOrphanBlockThins[pblockOrphan->hashPrevBlock];
    return pblockOrphan->hashPrevBlock;
}

static const int64_t nTargetTimespan = 16 * 60;  // 16 mins


// ppcoin: find last block index up to pindex
const CBlockIndex* GetLastBlockIndex(const CBlockIndex* pindex, bool fProofOfStake)
{
    while (pindex && pindex->pprev && (pindex->IsProofOfStake() != fProofOfStake))
        pindex = pindex->pprev;
    return pindex;
}

const CBlockThinIndex* GetLastBlockThinIndex(const CBlockThinIndex* pindex, bool fProofOfStake)
{
    while (pindex && pindex->pprev && (pindex->IsProofOfStake() != fProofOfStake))
        pindex = pindex->pprev;
    return pindex;
}

unsigned int GetNextTargetRequired(const CBlockIndex* pindexLast, bool fProofOfStake)
{
    CBigNum bnTargetLimit = fProofOfStake ? Params().ProofOfStakeLimit(pindexLast->nHeight) : Params().ProofOfWorkLimit();
    

    if (pindexLast == NULL)
        return bnTargetLimit.GetCompact(); // genesis block

    const CBlockIndex* pindexPrev = GetLastBlockIndex(pindexLast, fProofOfStake);
    if (pindexPrev->pprev == NULL)
        return bnTargetLimit.GetCompact(); // first block
    const CBlockIndex* pindexPrevPrev = GetLastBlockIndex(pindexPrev->pprev, fProofOfStake);
    if (pindexPrevPrev->pprev == NULL)
        return bnTargetLimit.GetCompact(); // second block
    
    int64_t nTargetSpacing = GetTargetSpacing(pindexLast->nHeight);
    int64_t nActualSpacing = pindexPrev->GetBlockTime() - pindexPrevPrev->GetBlockTime();
    if (nActualSpacing < 0)
        nActualSpacing = nTargetSpacing;

    // ppcoin: target change every block
    // ppcoin: retarget with exponential moving toward target spacing
    CBigNum bnNew;
    bnNew.SetCompact(pindexPrev->nBits);
    int64_t nInterval = nTargetTimespan / nTargetSpacing;
    bnNew *= ((nInterval - 1) * nTargetSpacing + nActualSpacing + nActualSpacing);
    bnNew /= ((nInterval + 1) * nTargetSpacing);

    if (bnNew <= 0 || bnNew > bnTargetLimit)
        bnNew = bnTargetLimit;

    return bnNew.GetCompact();
}

unsigned int GetNextTargetRequiredThin(const CBlockThinIndex* pindexLast, bool fProofOfStake)
{
    CBigNum bnTargetLimit = fProofOfStake ? Params().ProofOfStakeLimit(pindexLast->nHeight) : Params().ProofOfWorkLimit();

    if (pindexLast == NULL)
        return bnTargetLimit.GetCompact(); // genesis block

    const CBlockThinIndex* pindexPrev = GetLastBlockThinIndex(pindexLast, fProofOfStake);

    if (pindexPrev->pprev == NULL)
        return bnTargetLimit.GetCompact(); // first block

    const CBlockThinIndex* pindexPrevPrev = GetLastBlockThinIndex(pindexPrev->pprev, fProofOfStake);

    if (pindexPrevPrev->pprev == NULL)
        return bnTargetLimit.GetCompact(); // second block
    
    int64_t nTargetSpacing = GetTargetSpacing(pindexLast->nHeight);
    int64_t nActualSpacing = pindexPrev->GetBlockTime() - pindexPrevPrev->GetBlockTime();
    if (nActualSpacing < 0)
        nActualSpacing = nTargetSpacing;

    // ppcoin: target change every block
    // ppcoin: retarget with exponential moving toward target spacing
    CBigNum bnNew;
    bnNew.SetCompact(pindexPrev->nBits);
    int64_t nInterval = nTargetTimespan / nTargetSpacing;
    bnNew *= ((nInterval - 1) * nTargetSpacing + nActualSpacing + nActualSpacing);
    bnNew /= ((nInterval + 1) * nTargetSpacing);

    if (bnNew <= 0 || bnNew > bnTargetLimit)
        bnNew = bnTargetLimit;

    return bnNew.GetCompact();
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits)
{
    CBigNum bnTarget;
    bnTarget.SetCompact(nBits);

    // Check range
    if (bnTarget <= 0 || bnTarget > Params().ProofOfWorkLimit())
        return error("CheckProofOfWork() : nBits below minimum work");

    // Check proof of work matches claimed amount
    if (hash > bnTarget.getuint256())
        return error("CheckProofOfWork() : hash doesn't match nBits");

    return true;
}

// Return maximum amount of blocks that other nodes claim to have
int GetNumBlocksOfPeers()
{
    return std::max(cPeerBlockCounts.median(), Checkpoints::GetTotalBlocksEstimate());
}

bool IsInitialBlockDownload()
{
    LOCK(cs_main);
    
    if (pindexBest == NULL || nBestHeight < Checkpoints::GetTotalBlocksEstimate())
        return true;

    static int64_t nLastUpdate;
    static CBlockIndex* pindexLastBest;

    if (pindexBest != pindexLastBest)
    {
        pindexLastBest = pindexBest;
        nLastUpdate = GetTime();
    };

    return (GetTime() - nLastUpdate < 15 &&
            pindexBest->GetBlockTime() < GetTime() - 8 * 60 * 60);
}

void static InvalidChainFound(CBlockIndex* pindexNew)
{
    if (pindexNew->nChainTrust > nBestInvalidTrust)
    {
        nBestInvalidTrust = pindexNew->nChainTrust;
        CTxDB().WriteBestInvalidTrust(CBigNum(nBestInvalidTrust));
        uiInterface.NotifyBlocksChanged();
    };

    uint256 nBestInvalidBlockTrust = pindexNew->nChainTrust - pindexNew->pprev->nChainTrust;
    uint256 nBestBlockTrust = pindexBest->nHeight != 0 ? (pindexBest->nChainTrust - pindexBest->pprev->nChainTrust) : pindexBest->nChainTrust;

    LogPrintf("InvalidChainFound: invalid block=%s  height=%d  trust=%s  blocktrust=%d  date=%s\n",
      pindexNew->GetBlockHash().ToString().substr(0,20).c_str(), pindexNew->nHeight,
      CBigNum(pindexNew->nChainTrust).ToString().c_str(), nBestInvalidBlockTrust.Get64(),
      DateTimeStrFormat("%x %H:%M:%S", pindexNew->GetBlockTime()).c_str());
    LogPrintf("InvalidChainFound:  current best=%s  height=%d  trust=%s  blocktrust=%d  date=%s\n",
      hashBestChain.ToString().substr(0,20).c_str(), nBestHeight,
      CBigNum(pindexBest->nChainTrust).ToString().c_str(),
      nBestBlockTrust.Get64(),
      DateTimeStrFormat("%x %H:%M:%S", pindexBest->GetBlockTime()).c_str());
}


void CBlock::UpdateTime(const CBlockIndex* pindexPrev)
{
    nTime = max(GetBlockTime(), GetAdjustedTime());
}











bool CTransaction::DisconnectInputs(CTxDB& txdb)
{
    // Relinquish previous transactions' spent pointers
    if (!IsCoinBase())
    {
        BOOST_FOREACH(const CTxIn& txin, vin)
        {
            COutPoint prevout = txin.prevout;

            // Get prev txindex from disk
            CTxIndex txindex;
            if (!txdb.ReadTxIndex(prevout.hash, txindex))
            {
                LogPrintf("Warning: DisconnectInputs() : ReadTxIndex failed.\n");
                //return error("DisconnectInputs() : ReadTxIndex failed");
                continue;
            };

            if (prevout.n >= txindex.vSpent.size())
                return error("DisconnectInputs() : prevout.n out of range");

            // Mark outpoint as not spent
            txindex.vSpent[prevout.n].SetNull();

            // Write back
            if (!txdb.UpdateTxIndex(prevout.hash, txindex))
                return error("DisconnectInputs() : UpdateTxIndex failed");
        };
    };

    // Remove transaction from index
    // This can fail if a duplicate of this transaction was in a chain that got
    // reorganized away. This is only possible if this transaction was completely
    // spent, so erasing it would be a no-op anyway.
    txdb.EraseTxIndex(*this);

    return true;
}


bool CTransaction::FetchInputs(CTxDB& txdb, const map<uint256, CTxIndex>& mapTestPool,
                               bool fBlock, bool fMiner, MapPrevTx& inputsRet, bool& fInvalid)
{
    // FetchInputs can return false either because we just haven't seen some inputs
    // (in which case the transaction should be stored as an orphan)
    // or because the transaction is malformed (in which case the transaction should
    // be dropped).  If tx is definitely invalid, fInvalid will be set to true.
    fInvalid = false;

    if (IsCoinBase())
        return true; // Coinbase transactions have no inputs to fetch.

    for (unsigned int i = 0; i < vin.size(); i++)
    {
        if (nVersion == ANON_TXN_VERSION
            && vin[i].IsAnonInput())
            continue;
        COutPoint prevout = vin[i].prevout;
        if (inputsRet.count(prevout.hash))
            continue; // Got it already

        // Read txindex
        CTxIndex& txindex = inputsRet[prevout.hash].first;
        bool fFound = true;
        if ((fBlock || fMiner) && mapTestPool.count(prevout.hash))
        {
            // Get txindex from current proposed changes
            txindex = mapTestPool.find(prevout.hash)->second;
        } else
        {
            // Read txindex from txdb
            fFound = txdb.ReadTxIndex(prevout.hash, txindex);
        };

        if (!fFound && (fBlock || fMiner))
            return fMiner ? false : error("FetchInputs() : %s prev tx %s index entry not found", GetHash().ToString().substr(0,10).c_str(),  prevout.hash.ToString().substr(0,10).c_str());

        // Read txPrev
        CTransaction& txPrev = inputsRet[prevout.hash].second;
        if (!fFound || txindex.pos == CDiskTxPos(1,1,1))
        {
            // Get prev tx from single transactions in memory
            if (!mempool.lookup(prevout.hash, txPrev))
                return error("FetchInputs() : %s mempool Tx prev not found %s", GetHash().ToString().substr(0,10).c_str(),  prevout.hash.ToString().substr(0,10).c_str());
            if (!fFound)
                txindex.vSpent.resize(txPrev.vout.size());
        } else
        {
            // Get prev tx from disk
            if (!txPrev.ReadFromDisk(txindex.pos))
                return error("FetchInputs() : %s ReadFromDisk prev tx %s failed", GetHash().ToString().substr(0,10).c_str(),  prevout.hash.ToString().substr(0,10).c_str());
        };
    };

    // Make sure all prevout.n indexes are valid:
    for (unsigned int i = 0; i < vin.size(); i++)
    {
        if (nVersion == ANON_TXN_VERSION
            && vin[i].IsAnonInput())
            continue;

        const COutPoint prevout = vin[i].prevout;
        assert(inputsRet.count(prevout.hash) != 0);
        const CTxIndex& txindex = inputsRet[prevout.hash].first;
        const CTransaction& txPrev = inputsRet[prevout.hash].second;
        if (prevout.n >= txPrev.vout.size() || prevout.n >= txindex.vSpent.size())
        {
            // Revisit this if/when transaction replacement is implemented and allows
            // adding inputs:
            fInvalid = true;
            return DoS(100, error("FetchInputs() : %s prevout.n out of range %d %u %u prev tx %s\n%s", GetHash().ToString().substr(0,10).c_str(), prevout.n, txPrev.vout.size(), txindex.vSpent.size(), prevout.hash.ToString().substr(0,10).c_str(), txPrev.ToString().c_str()));
        };
    };

    return true;
}

bool CTransaction::CheckAnonInputs(CTxDB& txdb, int64_t& nSumValue, bool& fInvalid, bool fCheckExists)
{
    if (fDebugRingSig)
        LogPrintf("CheckAnonInputs()\n");
    
    // - fCheckExists should only run for anonInputs entering this node
    
    AssertLockHeld(cs_main);

    fInvalid = false; // TODO: is it acceptable to not find ring members?

    nSumValue = 0;

    uint256 preimage;
    if (pwalletMain->GetTxnPreImage(*this, preimage) != 0)
    {
        LogPrintf("CheckAnonInputs(): Error GetTxnPreImage() failed.\n");
        fInvalid = true; return false;
    };

    uint256 txnHash = GetHash();

    for (uint32_t i = 0; i < vin.size(); i++)
    {
        const CTxIn& txin = vin[i];

        if (!txin.IsAnonInput())
            continue;
        
        const CScript &s = txin.scriptSig;
        
        std::vector<uint8_t> vchImage;
        txin.ExtractKeyImage(vchImage);

        CKeyImageSpent spentKeyImage;
        bool fInMemPool;
        if (GetKeyImage(&txdb, vchImage, spentKeyImage, fInMemPool))
        {
            // -- this can happen for transactions created by the local node
            if (spentKeyImage.txnHash == txnHash)
            {
                if (fDebugRingSig)
                    LogPrintf("Input %d keyimage %s matches txn %s.\n", i, HexStr(vchImage).c_str(), txnHash.ToString().c_str());
            } else
            {
                if (fCheckExists
                    && !TxnHashInSystem(&txdb, spentKeyImage.txnHash))
                {
                    if (fDebugRingSig)
                        LogPrintf("Input %d keyimage %s matches unknown txn %s, continuing.\n", i, HexStr(vchImage).c_str(), spentKeyImage.txnHash.ToString().c_str());
                    
                    // -- spentKeyImage is invalid as points to unknown txnHash
                    //    continue
                } else
                {
                    LogPrintf("CheckAnonInputs(): Error input %d keyimage %s already spent.\n", i, HexStr(vchImage).c_str());
                    fInvalid = true; return false;
                };
            };
        };

        int64_t nCoinValue = -1;
        int nRingSize = txin.ExtractRingSize();
        if (nRingSize < (int)MIN_RING_SIZE
            || nRingSize > (int)MAX_RING_SIZE)
        {
            LogPrintf("CheckAnonInputs(): Error input %d ringsize %d not in range [%d, %d].\n", i, nRingSize, MIN_RING_SIZE, MAX_RING_SIZE);
            fInvalid = true; return false;
        };

        if (s.size() < 2 + (ec_compressed_size + ec_secret_size + ec_secret_size) * nRingSize)
        {
            LogPrintf("CheckAnonInputs(): Error input %d scriptSig too small.\n", i);
            fInvalid = true; return false;
        };


        CPubKey pkRingCoin;
        CAnonOutput ao;
        CTxIndex txindex;
        const unsigned char* pPubkeys = &s[2];
        const unsigned char* pSigc    = &txin.scriptSig[2 + ec_compressed_size * nRingSize];
        const unsigned char* pSigr    = &txin.scriptSig[2 + (ec_compressed_size + ec_secret_size) * nRingSize];
        for (int ri = 0; ri < nRingSize; ++ri)
        {
            pkRingCoin = CPubKey(&pPubkeys[ri * ec_compressed_size], ec_compressed_size);
            if (!txdb.ReadAnonOutput(pkRingCoin, ao))
            {
                LogPrintf("CheckAnonInputs(): Error input %d, element %d AnonOutput %s not found.\n", i, ri, HexStr(pkRingCoin).c_str());
                fInvalid = true; return false;
            };

            if (nCoinValue == -1)
            {
                nCoinValue = ao.nValue;
            } else
            if (nCoinValue != ao.nValue)
            {
                LogPrintf("CheckAnonInputs(): Error input %d, element %d ring amount mismatch %d, %d.\n", i, ri, nCoinValue, ao.nValue);
                fInvalid = true; return false;
            };

            if (ao.nBlockHeight == 0
                || nBestHeight - ao.nBlockHeight < MIN_ANON_SPEND_DEPTH)
            {
                LogPrintf("CheckAnonInputs(): Error input %d, element %d depth < MIN_ANON_SPEND_DEPTH.\n", i, ri);
                fInvalid = true; return false;
            };
        };
        
        if (verifyRingSignature(vchImage, preimage, nRingSize, pPubkeys, pSigc, pSigr) != 0)
        {
            LogPrintf("CheckAnonInputs(): Error input %d verifyRingSignature() failed.\n", i);
            fInvalid = true; return false;
        };

        nSumValue += nCoinValue;
    };

    return true;
};

const CTxOut& CTransaction::GetOutputFor(const CTxIn& input, const MapPrevTx& inputs) const
{
    MapPrevTx::const_iterator mi = inputs.find(input.prevout.hash);
    if (mi == inputs.end())
        throw std::runtime_error("CTransaction::GetOutputFor() : prevout.hash not found");

    const CTransaction& txPrev = (mi->second).second;
    if (input.prevout.n >= txPrev.vout.size())
        throw std::runtime_error("CTransaction::GetOutputFor() : prevout.n out of range");

    return txPrev.vout[input.prevout.n];
}

int64_t CTransaction::GetValueIn(const MapPrevTx& inputs) const
{
    if (IsCoinBase())
        return 0;

    int64_t nResult = 0;
    for (unsigned int i = 0; i < vin.size(); i++)
    {
        if (nVersion == ANON_TXN_VERSION
            && vin[i].IsAnonInput())
        {
            continue;
        };
        nResult += GetOutputFor(vin[i], inputs).nValue;
    };

    return nResult;
}

unsigned int CTransaction::GetP2SHSigOpCount(const MapPrevTx& inputs) const
{
    if (IsCoinBase())
        return 0;

    unsigned int nSigOps = 0;
    for (unsigned int i = 0; i < vin.size(); i++)
    {
        if (nVersion == ANON_TXN_VERSION
            && vin[i].IsAnonInput())
            continue;
        const CTxOut& prevout = GetOutputFor(vin[i], inputs);
        if (prevout.scriptPubKey.IsPayToScriptHash())
            nSigOps += prevout.scriptPubKey.GetSigOpCount(vin[i].scriptSig);
    };

    return nSigOps;
}

bool CTransaction::ConnectInputs(CTxDB& txdb, MapPrevTx inputs, map<uint256, CTxIndex>& mapTestPool, const CDiskTxPos& posThisTx,
    const CBlockIndex* pindexBlock, bool fBlock, bool fMiner, unsigned int flags)
{
    // Take over previous transactions' spent pointers
    // fBlock is true when this is called from AcceptBlock when a new best-block is added to the blockchain
    // fMiner is true when called from the internal bitcoin miner
    // ... both are false when called from CTransaction::AcceptToMemoryPool
    if (!IsCoinBase())
    {
        int64_t nValueIn = 0;
        int64_t nFees = 0;
        for (unsigned int i = 0; i < vin.size(); i++)
        {
            if (nVersion == ANON_TXN_VERSION
                && vin[i].IsAnonInput())
                continue;

            COutPoint prevout = vin[i].prevout;
            assert(inputs.count(prevout.hash) > 0);
            CTxIndex& txindex = inputs[prevout.hash].first;
            CTransaction& txPrev = inputs[prevout.hash].second;

            if (prevout.n >= txPrev.vout.size() || prevout.n >= txindex.vSpent.size())
                return DoS(100, error("ConnectInputs() : %s, txv %d prevout.n out of range %d %u %u prev tx %s\n%s", GetHash().ToString().substr(0,10).c_str(), txPrev.nVersion, prevout.n, txPrev.vout.size(), txindex.vSpent.size(), prevout.hash.ToString().substr(0,10).c_str(), txPrev.ToString().c_str()));

            // If prev is coinbase or coinstake, check that it's matured
            if (txPrev.IsCoinBase() || txPrev.IsCoinStake())
                for (const CBlockIndex* pindex = pindexBlock; pindex && pindexBlock->nHeight - pindex->nHeight < nCoinbaseMaturity; pindex = pindex->pprev)
                    if (pindex->nBlockPos == txindex.pos.nBlockPos && pindex->nFile == txindex.pos.nFile)
                        return error("ConnectInputs() : tried to spend %s at depth %d", txPrev.IsCoinBase() ? "coinbase" : "coinstake", pindexBlock->nHeight - pindex->nHeight);

            // ppcoin: check transaction timestamp
            if (txPrev.nTime > nTime)
                return DoS(100, error("ConnectInputs() : transaction timestamp earlier than input transaction"));

            // Check for negative or overflow input values
            nValueIn += txPrev.vout[prevout.n].nValue;
            if (!MoneyRange(txPrev.vout[prevout.n].nValue) || !MoneyRange(nValueIn))
                return DoS(100, error("ConnectInputs() : txin values out of range"));

        };

        // The first loop above does all the inexpensive checks.
        // Only if ALL inputs pass do we perform expensive ECDSA signature checks.
        // Helps prevent CPU exhaustion attacks.
        for (unsigned int i = 0; i < vin.size(); i++)
        {
            if (nVersion == ANON_TXN_VERSION
                && vin[i].IsAnonInput())
                continue;
            COutPoint prevout = vin[i].prevout;
            assert(inputs.count(prevout.hash) > 0);
            CTxIndex& txindex = inputs[prevout.hash].first;
            CTransaction& txPrev = inputs[prevout.hash].second;

            // Check for conflicts (double-spend)
            // This doesn't trigger the DoS code on purpose; if it did, it would make it easier
            // for an attacker to attempt to split the network.
            if (!txindex.vSpent[prevout.n].IsNull())
                return fMiner ? false : error("ConnectInputs() : %s prev tx already used at %s", GetHash().ToString().substr(0,10).c_str(), txindex.vSpent[prevout.n].ToString().c_str());

            // Skip ECDSA signature verification when connecting blocks (fBlock=true)
            // before the last blockchain checkpoint. This is safe because block merkle hashes are
            // still computed and checked, and any change will be caught at the next checkpoint.
            if (!(fBlock && (nBestHeight < Checkpoints::GetTotalBlocksEstimate())))
            {
                // Verify signature
                if (!VerifySignature(txPrev, *this, i, flags, 0))
                {
                    if (flags & STANDARD_NOT_MANDATORY_VERIFY_FLAGS) {
                        // Check whether the failure was caused by a
                        // non-mandatory script verification check, such as
                        // non-null dummy arguments;
                        // if so, don't trigger DoS protection to
                        // avoid splitting the network between upgraded and
                        // non-upgraded nodes.
                        if (VerifySignature(txPrev, *this, i, flags & ~STANDARD_NOT_MANDATORY_VERIFY_FLAGS, 0))
                            return error("ConnectInputs() : %s non-mandatory VerifySignature failed", GetHash().ToString());
                    }
                    // Failures of other flags indicate a transaction that is
                    // invalid in new blocks, e.g. a invalid P2SH. We DoS ban
                    // such nodes as they are not following the protocol. That
                    // said during an upgrade careful thought should be taken
                    // as to the correct behavior - we may want to continue
                    // peering with non-upgraded nodes even after a soft-fork
                    // super-majority vote has passed.
                    return DoS(100,error("ConnectInputs() : %s VerifySignature failed", GetHash().ToString()));
                }
            }

            // Mark outpoints as spent
            txindex.vSpent[prevout.n] = posThisTx;

            // Write back
            if (fBlock || fMiner)
            {
                mapTestPool[prevout.hash] = txindex;
            };
        };

        if (nVersion == ANON_TXN_VERSION)
        {
            int64_t nSumAnon;
            bool fInvalid;
            if (!CheckAnonInputs(txdb, nSumAnon, fInvalid, true))
            {
                //if (fInvalid)
                DoS(100, error("ConnectInputs() : CheckAnonInputs found invalid tx %s", GetHash().ToString().substr(0,10).c_str()));
            };

            nValueIn += nSumAnon;
        };

        if (!IsCoinStake())
        {
            int64_t nValueOut = GetValueOut();

            if (nValueIn < nValueOut)
                return DoS(100, error("ConnectInputs() : %s, txv %d value in < value out", GetHash().ToString().substr(0,10).c_str(), nVersion));

            // Tally transaction fees
            int64_t nTxFee = nValueIn - nValueOut;
            if (nTxFee < 0)
                return DoS(100, error("ConnectInputs() : %s, txv %d nTxFee < 0", GetHash().ToString().substr(0,10).c_str(), nVersion));

            // enforce transaction fees for every block
            if (nTxFee < GetMinFee())
                return fBlock? DoS(100, error("ConnectInputs() : %s, txv %d not paying required fee=%s, paid=%s", GetHash().ToString().substr(0,10).c_str(), nVersion, FormatMoney(GetMinFee()).c_str(), FormatMoney(nTxFee).c_str())) : false;

            nFees += nTxFee;
            if (!MoneyRange(nFees))
                return DoS(100, error("ConnectInputs() : nFees out of range"));
        };
    };

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
        if (!txdb.WriteBlockIndex(blockindexPrev))
            return error("DisconnectBlock() : WriteBlockIndex failed");
    };

    // ppcoin: clean up wallet after disconnecting coinstake
    BOOST_FOREACH(CTransaction& tx, vtx)
        SyncWithWallets(tx, this, false, false);

    return true;
}

bool CBlock::ConnectBlock(CTxDB& txdb, CBlockIndex* pindex, bool fJustCheck)
{
    // Check it again in case a previous version let a bad block in, but skip BlockSig checking
    if (!CheckBlock(!fJustCheck, !fJustCheck, false))
        return false;
    
    unsigned int flags = SCRIPT_VERIFY_NOCACHE;

    //// issue here: it doesn't know the version
    unsigned int nTxPos;
    if (fJustCheck)
        // FetchInputs treats CDiskTxPos(1,1,1) as a special "refer to memorypool" indicator
        // Since we're just checking the block and not actually connecting it, it might not (and probably shouldn't) be on the disk to get the transaction from
        nTxPos = 1;
    else
        nTxPos = pindex->nBlockPos + ::GetSerializeSize(CBlock(), SER_DISK, CLIENT_VERSION) - (2 * GetSizeOfCompactSize(0)) + GetSizeOfCompactSize(vtx.size());

    map<uint256, CTxIndex> mapQueuedChanges;
    int64_t nFees = 0;
    int64_t nValueIn = 0;
    int64_t nValueOut = 0;
    int64_t nStakeReward = 0;
    unsigned int nSigOps = 0;
    BOOST_FOREACH(CTransaction& tx, vtx)
    {
        uint256 hashTx = tx.GetHash();

        // Do not allow blocks that contain transactions which 'overwrite' older transactions,
        // unless those are already completely spent.
        // If such overwrites are allowed, coinbases and transactions depending upon those
        // can be duplicated to remove the ability to spend the first instance -- even after
        // being sent to another address.
        // See BIP30 and http://r6.ca/blog/20120206T005236Z.html for more information.
        // This logic is not necessary for memory pool transactions, as AcceptToMemoryPool
        // already refuses previously-known transaction ids entirely.
        // This rule was originally applied all blocks whose timestamp was after March 15, 2012, 0:00 UTC.
        // Now that the whole chain is irreversibly beyond that time it is applied to all blocks except the
        // two in the chain that violate it. This prevents exploiting the issue against nodes in their
        // initial block download.
        CTxIndex txindexOld;
        if (txdb.ReadTxIndex(hashTx, txindexOld))
        {
            BOOST_FOREACH(CDiskTxPos &pos, txindexOld.vSpent)
            {
                if (pos.IsNull())
                    return false;
            };
        };

        nSigOps += tx.GetLegacySigOpCount();
        if (nSigOps > MAX_BLOCK_SIGOPS)
            return DoS(100, error("ConnectBlock() : too many sigops"));

        CDiskTxPos posThisTx(pindex->nFile, pindex->nBlockPos, nTxPos);
        if (!fJustCheck)
            nTxPos += ::GetSerializeSize(tx, SER_DISK, CLIENT_VERSION);

        MapPrevTx mapInputs;
        if (tx.IsCoinBase())
        {
            nValueOut += tx.GetValueOut();
        } else
        {
            bool fInvalid;
            if (!tx.FetchInputs(txdb, mapQueuedChanges, true, false, mapInputs, fInvalid))
                return false;

            // Add in sigops done by pay-to-script-hash inputs;
            // this is to prevent a "rogue miner" from creating
            // an incredibly-expensive-to-validate block.
            nSigOps += tx.GetP2SHSigOpCount(mapInputs);
            if (nSigOps > MAX_BLOCK_SIGOPS)
                return DoS(100, error("ConnectBlock() : too many sigops"));

            int64_t nTxValueIn = tx.GetValueIn(mapInputs);
            int64_t nTxValueOut = tx.GetValueOut();

            if (tx.nVersion == ANON_TXN_VERSION)
            {
                int64_t nSumAnon;
                if (!tx.CheckAnonInputs(txdb, nSumAnon, fInvalid, true))
                {
                    if (fInvalid)
                        return error("ConnectBlock() : CheckAnonInputs found invalid tx %s", tx.GetHash().ToString().substr(0,10).c_str());
                    return false;
                };

                nTxValueIn += nSumAnon;
            };

            nValueIn += nTxValueIn;
            nValueOut += nTxValueOut;

            if (!tx.IsCoinStake())
                nFees += nTxValueIn - nTxValueOut;

            if (tx.IsCoinStake())
                nStakeReward = nTxValueOut - nTxValueIn;

            if (!tx.ConnectInputs(txdb, mapInputs, mapQueuedChanges, posThisTx, pindex, true, false, flags))
                return false;
        };

        mapQueuedChanges[hashTx] = CTxIndex(posThisTx, tx.vout.size());
    };

    if (IsProofOfWork())
    {
        int64_t nReward = Params().GetProofOfWorkReward(pindex->nHeight, nFees);
        // Check coinbase reward
        if (vtx[0].GetValueOut() > nReward)
            return DoS(50, error("ConnectBlock() : coinbase reward exceeded (actual=%d vs calculated=%d)",
                   vtx[0].GetValueOut(),
                   nReward));
    };

    if (IsProofOfStake())
    {
        // ppcoin: coin stake tx earns reward instead of paying fee
        uint64_t nCoinAge;
        if (!vtx[1].GetCoinAge(txdb, nCoinAge))
            return error("ConnectBlock() : %s unable to get coin age for coinstake", vtx[1].GetHash().ToString().substr(0,10).c_str());

        int64_t nCalculatedStakeReward = Params().GetProofOfStakeReward(nCoinAge, nFees);

        if (nStakeReward > nCalculatedStakeReward)
            return DoS(100, error("ConnectBlock() : coinstake pays too much(actual=%d vs calculated=%d)", nStakeReward, nCalculatedStakeReward));
    };

    // ppcoin: track money supply and mint amount info
    pindex->nMint = nValueOut - nValueIn + nFees;
    pindex->nMoneySupply = (pindex->pprev? pindex->pprev->nMoneySupply : 0) + nValueOut - nValueIn;
    if (!txdb.WriteBlockIndex(CDiskBlockIndex(pindex)))
        return error("ConnectBlock() : WriteBlockIndex for pindex failed");

    if (fJustCheck)
        return true;

    // Write queued txindex changes
    for (map<uint256, CTxIndex>::iterator mi = mapQueuedChanges.begin(); mi != mapQueuedChanges.end(); ++mi)
    {
        if (!txdb.UpdateTxIndex((*mi).first, (*mi).second))
            return error("ConnectBlock() : UpdateTxIndex failed");
    };

    // Update block index on disk without changing it in memory.
    // The memory index structure will be changed after the db commits.
    if (pindex->pprev)
    {
        CDiskBlockIndex blockindexPrev(pindex->pprev);
        blockindexPrev.hashNext = pindex->GetBlockHash();
        if (!txdb.WriteBlockIndex(blockindexPrev))
            return error("ConnectBlock() : WriteBlockIndex failed");
    };

    // Watch for transactions paying to me
    BOOST_FOREACH(CTransaction& tx, vtx)
        SyncWithWallets(tx, this, true);

    return true;
}

bool static Reorganize(CTxDB& txdb, CBlockIndex* pindexNew)
{
    LogPrintf("REORGANIZE\n");

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
    };

    // List of what to disconnect
    vector<CBlockIndex*> vDisconnect;
    for (CBlockIndex* pindex = pindexBest; pindex != pfork; pindex = pindex->pprev)
        vDisconnect.push_back(pindex);

    // List of what to connect
    vector<CBlockIndex*> vConnect;
    for (CBlockIndex* pindex = pindexNew; pindex != pfork; pindex = pindex->pprev)
        vConnect.push_back(pindex);
    reverse(vConnect.begin(), vConnect.end());

    LogPrintf("REORGANIZE: Disconnect %u blocks; %s..%s\n", vDisconnect.size(), pfork->GetBlockHash().ToString().substr(0,20).c_str(), pindexBest->GetBlockHash().ToString().substr(0,20).c_str());
    LogPrintf("REORGANIZE: Connect %u blocks; %s..%s\n", vConnect.size(), pfork->GetBlockHash().ToString().substr(0,20).c_str(), pindexNew->GetBlockHash().ToString().substr(0,20).c_str());

    // Disconnect shorter branch
    list<CTransaction> vResurrect;
    BOOST_FOREACH(CBlockIndex* pindex, vDisconnect)
    {
        CBlock block;
        if (!block.ReadFromDisk(pindex))
            return error("Reorganize() : ReadFromDisk for disconnect failed");
        if (!block.DisconnectBlock(txdb, pindex))
            return error("Reorganize() : DisconnectBlock %s failed", pindex->GetBlockHash().ToString().substr(0,20).c_str());

        // Queue memory transactions to resurrect.
        // We only do this for blocks after the last checkpoint (reorganisation before that
        // point should only happen with -reindex/-loadblock, or a misbehaving peer.
        BOOST_REVERSE_FOREACH(const CTransaction& tx, block.vtx)
            if (!(tx.IsCoinBase() || tx.IsCoinStake()) && pindex->nHeight > Checkpoints::GetTotalBlocksEstimate())
                vResurrect.push_front(tx);
    };

    // Connect longer branch
    vector<CTransaction> vDelete;
    for (unsigned int i = 0; i < vConnect.size(); i++)
    {
        CBlockIndex* pindex = vConnect[i];
        CBlock block;
        if (!block.ReadFromDisk(pindex))
            return error("Reorganize() : ReadFromDisk for connect failed");
        if (!block.ConnectBlock(txdb, pindex))
        {
            // Invalid block
            return error("Reorganize() : ConnectBlock %s failed", pindex->GetBlockHash().ToString().substr(0,20).c_str());
        };

        // Queue memory transactions to delete
        BOOST_FOREACH(const CTransaction& tx, block.vtx)
            vDelete.push_back(tx);
    };

    if (!txdb.WriteHashBestChain(pindexNew->GetBlockHash()))
        return error("Reorganize() : WriteHashBestChain failed");

    // Make sure it's successfully written to disk before changing memory structure
    if (!txdb.TxnCommit())
        return error("Reorganize() : TxnCommit failed");

    // Disconnect shorter branch
    BOOST_FOREACH(CBlockIndex* pindex, vDisconnect)
        if (pindex->pprev)
            pindex->pprev->pnext = NULL;

    // Connect longer branch
    BOOST_FOREACH(CBlockIndex* pindex, vConnect)
        if (pindex->pprev)
            pindex->pprev->pnext = pindex;

    // Resurrect memory transactions that were in the disconnected branch
    BOOST_FOREACH(CTransaction& tx, vResurrect)
        AcceptToMemoryPool(mempool, tx, txdb);

    // Delete redundant memory transactions that are in the connected branch
    BOOST_FOREACH(CTransaction& tx, vDelete)
    {
        mempool.remove(tx);
        mempool.removeConflicts(tx);
    };

    LogPrintf("REORGANIZE: done\n");

    return true;
}


// Called from inside SetBestChain: attaches a block to the new best chain being built
bool CBlock::SetBestChainInner(CTxDB& txdb, CBlockIndex *pindexNew)
{
    uint256 hash = GetHash();

    // Adding to current best branch
    if (!ConnectBlock(txdb, pindexNew) || !txdb.WriteHashBestChain(hash))
    {
        txdb.TxnAbort();
        InvalidChainFound(pindexNew);
        return false;
    };

    if (!txdb.TxnCommit())
        return error("SetBestChain() : TxnCommit failed");

    // Add to current best branch
    pindexNew->pprev->pnext = pindexNew;

    // Delete redundant memory transactions
    BOOST_FOREACH(CTransaction& tx, vtx)
        mempool.remove(tx);

    return true;
}

bool CBlock::SetBestChain(CTxDB& txdb, CBlockIndex* pindexNew)
{
    uint256 hash = GetHash();

    if (!txdb.TxnBegin())
        return error("SetBestChain() : TxnBegin failed");

    if (pindexGenesisBlock == NULL && hash == Params().HashGenesisBlock())
    {
        txdb.WriteHashBestChain(hash);
        if (!txdb.TxnCommit())
            return error("SetBestChain() : TxnCommit failed");
        pindexGenesisBlock = pindexNew;
    } else
    if (hashPrevBlock == hashBestChain)
    {
        if (!SetBestChainInner(txdb, pindexNew))
            return error("SetBestChain() : SetBestChainInner failed");
    } else
    {
        // the first block in the new chain that will cause it to become the new best chain
        CBlockIndex *pindexIntermediate = pindexNew;

        // list of blocks that need to be connected afterwards
        std::vector<CBlockIndex*> vpindexSecondary;

        // Reorganize is costly in terms of db load, as it works in a single db transaction.
        // Try to limit how much needs to be done inside
        while (pindexIntermediate->pprev && pindexIntermediate->pprev->nChainTrust > pindexBest->nChainTrust)
        {
            vpindexSecondary.push_back(pindexIntermediate);
            pindexIntermediate = pindexIntermediate->pprev;
        };

        if (!vpindexSecondary.empty())
            LogPrintf("Postponing %u reconnects\n", vpindexSecondary.size());

        // Switch to new best branch
        if (!Reorganize(txdb, pindexIntermediate))
        {
            txdb.TxnAbort();
            InvalidChainFound(pindexNew);
            return error("SetBestChain() : Reorganize failed");
        };

        // Connect further blocks
        BOOST_REVERSE_FOREACH(CBlockIndex *pindex, vpindexSecondary)
        {
            CBlock block;
            if (!block.ReadFromDisk(pindex))
            {
                LogPrintf("SetBestChain() : ReadFromDisk failed\n");
                break;
            };

            if (!txdb.TxnBegin())
            {
                LogPrintf("SetBestChain() : TxnBegin 2 failed\n");
                break;
            };
            // errors now are not fatal, we still did a reorganisation to a new chain in a valid way
            if (!block.SetBestChainInner(txdb, pindex))
                break;
        };
    };

    // Update best block in wallet (so we can detect restored wallets)
    bool fIsInitialDownload = IsInitialBlockDownload();
    if (!fIsInitialDownload)
    {
        const CBlockLocator locator(pindexNew);
        ::SetBestChain(locator);
    };

    // New best block
    hashBestChain = hash;
    pindexBest = pindexNew;
    pblockindexFBBHLast = NULL;
    nBestHeight = pindexBest->nHeight;
    nBestChainTrust = pindexNew->nChainTrust;
    nTimeBestReceived = GetTime();
    mempool.AddTransactionsUpdated(1);

    uint256 nBestBlockTrust = pindexBest->nHeight != 0 ? (pindexBest->nChainTrust - pindexBest->pprev->nChainTrust) : pindexBest->nChainTrust;

    LogPrintf("SetBestChain: new best=%s  height=%d  trust=%s  blocktrust=%d  date=%s\n",
        hashBestChain.ToString().substr(0,20).c_str(), nBestHeight,
        CBigNum(nBestChainTrust).ToString().c_str(),
        nBestBlockTrust.Get64(),
        DateTimeStrFormat("%x %H:%M:%S", pindexBest->GetBlockTime()).c_str());

    // Check the version of the last 100 blocks to see if we need to upgrade:
    if (!fIsInitialDownload)
    {
        int nUpgraded = 0;
        const CBlockIndex* pindex = pindexBest;
        for (int i = 0; i < 100 && pindex != NULL; i++)
        {
            if (pindex->nVersion > CBlock::CURRENT_VERSION)
                ++nUpgraded;
            pindex = pindex->pprev;
        };

        if (nUpgraded > 0)
            LogPrintf("SetBestChain: %d of last 100 blocks above version %d\n", nUpgraded, (int)CBlock::CURRENT_VERSION);
        if (nUpgraded > 100/2)
            // strMiscWarning is read by GetWarnings(), called by Qt and the JSON-RPC code to warn the user:
            strMiscWarning = _("Warning: This version is obsolete, upgrade required!");
    };

    std::string strCmd = GetArg("-blocknotify", "");

    if (!fIsInitialDownload && !strCmd.empty())
    {
        boost::replace_all(strCmd, "%s", hashBestChain.GetHex());
        boost::thread t(runCommand, strCmd); // thread runs free
    };

    return true;
}

int CMerkleTx::GetDepthInMainChainINTERNAL(CBlockIndex* &pindexRet) const
{
    if (hashBlock == 0 || nIndex == -1)
        return 0;

    AssertLockHeld(cs_main);

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
    };

    pindexRet = pindex;
    return pindexBest->nHeight - pindex->nHeight + 1;
}

int CMerkleTx::GetDepthInMainChain(CBlockIndex* &pindexRet) const
{
    AssertLockHeld(cs_main);
    int nResult = GetDepthInMainChainINTERNAL(pindexRet);
    if (nResult == 0 && !mempool.exists(GetHash()))
        return -1; // Not in chain, not in mempool

    return nResult;
}

int CMerkleTx::GetDepthInMainChainINTERNAL(CBlockThinIndex* &pindexRet) const
{
    //if (hashBlock == 0 || nIndex == -1)
    if (hashBlock == 0)
        return 0;

    AssertLockHeld(cs_main);

    // Find the block it claims to be in
    map<uint256, CBlockThinIndex*>::iterator mi = mapBlockThinIndex.find(hashBlock);
    if (mi == mapBlockThinIndex.end())
    {
        pindexRet = NULL;

        if (!fThinFullIndex)
        {
            //LogPrintf("[rem] GetDepthInMainChainINTERNAL ReadBlockThinIndex\n");

            CTxDB txdb("r");
            CDiskBlockThinIndex diskindex;
            if (txdb.ReadBlockThinIndex(hashBlock, diskindex)
                && diskindex.hashNext != 0)
                return pindexBestHeader->nHeight - diskindex.nHeight + 1;
        };

        return 0;
    };

    CBlockThinIndex* pindex = (*mi).second;
    if (!pindex || !pindex->IsInMainChain())
        return 0;

    /*
    // Make sure the merkle branch connects to this block
    if (!fMerkleVerified)
    {
        if (CBlock::CheckMerkleBranch(GetHash(), vMerkleBranch, nIndex) != pindex->hashMerkleRoot)
            return 0;
        fMerkleVerified = true;
    };
    */
    pindexRet = pindex;
    return pindexBestHeader->nHeight - pindex->nHeight + 1;
}

int CMerkleTx::GetDepthInMainChain(CBlockThinIndex* &pindexRet) const
{
    AssertLockHeld(cs_main);
    int nResult = GetDepthInMainChainINTERNAL(pindexRet);
    if (nResult == 0 && !mempool.exists(GetHash()))
        return -1; // Not in chain, not in mempool

    return nResult;
}

int CMerkleTx::GetBlocksToMaturity() const
{
    if (!(IsCoinBase() || IsCoinStake()))
        return 0;
    
    return max(0, (nCoinbaseMaturity + 5) - GetDepthInMainChain());
}


bool CMerkleTx::AcceptToMemoryPool(CTxDB& txdb)
{
    return ::AcceptToMemoryPool(mempool, *this, txdb);
}

bool CMerkleTx::AcceptToMemoryPool()
{
    CTxDB txdb("r");
    return AcceptToMemoryPool(txdb);
}

uint256 CPartialMerkleTree::CalcHash(int height, unsigned int pos, const std::vector<uint256> &vTxid)
{
    if (height == 0)
    {
        // hash at height 0 is the txids themself
        return vTxid[pos];
    } else
    {
        // calculate left hash
        uint256 left = CalcHash(height-1, pos*2, vTxid), right;
        // calculate right hash if not beyong the end of the array - copy left hash otherwise1
        if (pos*2+1 < CalcTreeWidth(height-1))
            right = CalcHash(height-1, pos*2+1, vTxid);
        else
            right = left;
        // combine subhashes
        return Hash(BEGIN(left), END(left), BEGIN(right), END(right));
    };
}

void CPartialMerkleTree::TraverseAndBuild(int height, unsigned int pos, const std::vector<uint256> &vTxid, const std::vector<bool> &vMatch)
{
    // determine whether this node is the parent of at least one matched txid
    bool fParentOfMatch = false;
    for (unsigned int p = pos << height; p < (pos+1) << height && p < nTransactions; p++)
        fParentOfMatch |= vMatch[p];

    // store as flag bit
    vBits.push_back(fParentOfMatch);
    if (height==0 || !fParentOfMatch)
    {
        // if at height 0, or nothing interesting below, store hash and stop
        vHash.push_back(CalcHash(height, pos, vTxid));
    } else
    {
        // otherwise, don't store any hash, but descend into the subtrees
        TraverseAndBuild(height-1, pos*2, vTxid, vMatch);
        if (pos*2+1 < CalcTreeWidth(height-1))
            TraverseAndBuild(height-1, pos*2+1, vTxid, vMatch);
    };
}

uint256 CPartialMerkleTree::TraverseAndExtract(int height, unsigned int pos, unsigned int &nBitsUsed, unsigned int &nHashUsed, std::vector<uint256> &vMatch)
{
    if (nBitsUsed >= vBits.size())
    {
        // overflowed the bits array - failure
        fBad = true;
        return 0;
    };

    bool fParentOfMatch = vBits[nBitsUsed++];
    if (height==0 || !fParentOfMatch)
    {
        // if at height 0, or nothing interesting below, use stored hash and do not descend
        if (nHashUsed >= vHash.size())
        {
            // overflowed the hash array - failure
            fBad = true;
            return 0;
        };

        const uint256 &hash = vHash[nHashUsed++];
        if (height==0 && fParentOfMatch) // in case of height 0, we have a matched txid
            vMatch.push_back(hash);
        return hash;
    } else
    {
        // otherwise, descend into the subtrees to extract matched txids and hashes
        uint256 left = TraverseAndExtract(height-1, pos*2, nBitsUsed, nHashUsed, vMatch), right;
        if (pos*2+1 < CalcTreeWidth(height-1))
            right = TraverseAndExtract(height-1, pos*2+1, nBitsUsed, nHashUsed, vMatch);
        else
            right = left;
        // and combine them before returning
        return Hash(BEGIN(left), END(left), BEGIN(right), END(right));
    };
}

CPartialMerkleTree::CPartialMerkleTree(const std::vector<uint256> &vTxid, const std::vector<bool> &vMatch) : nTransactions(vTxid.size()), fBad(false)
{
    // reset state
    vBits.clear();
    vHash.clear();

    // calculate height of tree
    int nHeight = 0;
    while (CalcTreeWidth(nHeight) > 1)
        nHeight++;

    // traverse the partial tree
    TraverseAndBuild(nHeight, 0, vTxid, vMatch);
}

CPartialMerkleTree::CPartialMerkleTree() : nTransactions(0), fBad(true) {}

uint256 CPartialMerkleTree::ExtractMatches(std::vector<uint256> &vMatch)
{
    vMatch.clear();
    // An empty set will not work
    if (nTransactions == 0)
        return 0;
    // check for excessively high numbers of transactions
    if (nTransactions > MAX_BLOCK_SIZE / 60) // 60 is the lower bound for the size of a serialized CTransaction
        return 0;
    // there can never be more hashes provided than one for every txid
    if (vHash.size() > nTransactions)
        return 0;
    // there must be at least one bit per node in the partial tree, and at least one node per hash
    if (vBits.size() < vHash.size())
        return 0;
    // calculate height of tree
    int nHeight = 0;
    while (CalcTreeWidth(nHeight) > 1)
        nHeight++;
    // traverse the partial tree
    unsigned int nBitsUsed = 0, nHashUsed = 0;
    uint256 hashMerkleRoot = TraverseAndExtract(nHeight, 0, nBitsUsed, nHashUsed, vMatch);
    // verify that no problems occured during the tree traversal
    if (fBad)
        return 0;
    // verify that all bits were consumed (except for the padding caused by serializing it as a byte sequence)
    if ((nBitsUsed+7)/8 != (vBits.size()+7)/8)
        return 0;
    // verify that all hashes were consumed
    if (nHashUsed != vHash.size())
        return 0;

    return hashMerkleRoot;
};


CMerkleBlock::CMerkleBlock(const CBlock& block, CBlockIndex *pBlockIndex, CBloomFilter& filter)
{
    header = pBlockIndex->GetBlockThinOnly();

    vector<bool> vMatch;
    vector<uint256> vHashes;

    vMatch.reserve(block.vtx.size());
    vHashes.reserve(block.vtx.size());

    for (unsigned int i = 0; i < block.vtx.size(); i++)
    {
        const uint256& hash = block.vtx[i].GetHash();

        if (filter.IsRelevantAndUpdate(block.vtx[i]))
        {
            vMatch.push_back(true);
            vMatchedTxn.push_back(make_pair(i, hash));
        } else
        {
            vMatch.push_back(false);
        };

        vHashes.push_back(hash);
    };

    txn = CPartialMerkleTree(vHashes, vMatch);
}

// ppcoin: total coin age spent in transaction, in the unit of coin-days.
// Only those coins meeting minimum age requirement counts. As those
// transactions not in main chain are not currently indexed so we
// might not find out about their coin age. Older transactions are
// guaranteed to be in main chain by sync-checkpoint. This rule is
// introduced to help nodes establish a consistent view of the coin
// age (trust score) of competing branches.
bool CTransaction::GetCoinAge(CTxDB& txdb, uint64_t& nCoinAge) const
{
    CBigNum bnCentSecond = 0;  // coin age in the unit of cent-seconds
    nCoinAge = 0;

    if (IsCoinBase())
        return true;

    BOOST_FOREACH(const CTxIn& txin, vin)
    {
        // First try finding the previous transaction in database
        CTransaction txPrev;
        CTxIndex txindex;
        if (!txPrev.ReadFromDisk(txdb, txin.prevout, txindex))
            continue;  // previous transaction not in main chain
        if (nTime < txPrev.nTime)
            return false;  // Transaction timestamp violation

        // Read block header
        CBlock block;
        if (!block.ReadFromDisk(txindex.pos.nFile, txindex.pos.nBlockPos, false))
            return false; // unable to read block of previous transaction
        if (block.GetBlockTime() + nStakeMinAge > nTime)
            continue; // only count coins meeting min age requirement

        int64_t nValueIn = txPrev.vout[txin.prevout.n].nValue;
        bnCentSecond += CBigNum(nValueIn) * (nTime-txPrev.nTime) / CENT;

        if (fDebugPoS)
            LogPrintf("coin age nValueIn=%d nTimeDiff=%d bnCentSecond=%s\n", nValueIn, nTime - txPrev.nTime, bnCentSecond.ToString().c_str());
    };

    CBigNum bnCoinDay = bnCentSecond * CENT / COIN / (24 * 60 * 60);
    if (fDebugPoS)
        LogPrintf("coin age bnCoinDay=%s\n", bnCoinDay.ToString().c_str());
    nCoinAge = bnCoinDay.getuint64();
    return true;
}





CBlockThin CBlock::GetBlockThinOnly() const
{
    CBlockThin block;
    block.nVersion       = nVersion;
    block.hashPrevBlock  = hashPrevBlock;
    block.hashMerkleRoot = hashMerkleRoot;
    block.nTime          = nTime;
    block.nBits          = nBits;
    block.nNonce         = nNonce;

    block.nFlags         = 0;
    if (IsProofOfStake())
        block.nFlags |= BLOCK_PROOF_OF_STAKE;

    return block;
};

// ppcoin: total coin age spent in block, in the unit of coin-days.
bool CBlock::GetCoinAge(uint64_t& nCoinAge) const
{
    nCoinAge = 0;

    CTxDB txdb("r");
    BOOST_FOREACH(const CTransaction& tx, vtx)
    {
        uint64_t nTxCoinAge;
        if (tx.GetCoinAge(txdb, nTxCoinAge))
            nCoinAge += nTxCoinAge;
        else
            return false;
    };

    if (nCoinAge == 0) // block coin age minimum 1 coin-day
        nCoinAge = 1;
    if (fDebugPoS)
        LogPrintf("block coin age total nCoinDays=%d\n", nCoinAge);
    return true;
}

bool CBlock::AddToBlockIndex(unsigned int nFile, unsigned int nBlockPos, const uint256& hashProof)
{
    // Check for duplicate
    uint256 hash = GetHash();
    if (mapBlockIndex.count(hash))
        return error("AddToBlockIndex() : %s already exists", hash.ToString().substr(0,20).c_str());

    // Construct new block index object
    CBlockIndex* pindexNew = new CBlockIndex(nFile, nBlockPos, *this);
    if (!pindexNew)
        return error("AddToBlockIndex() : new CBlockIndex failed");

    pindexNew->phashBlock = &hash;
    map<uint256, CBlockIndex*>::iterator miPrev = mapBlockIndex.find(hashPrevBlock);
    if (miPrev != mapBlockIndex.end())
    {
        pindexNew->pprev = (*miPrev).second;
        pindexNew->nHeight = pindexNew->pprev->nHeight + 1;
    };

    // ppcoin: compute chain trust score
    pindexNew->nChainTrust = (pindexNew->pprev ? pindexNew->pprev->nChainTrust : 0) + pindexNew->GetBlockTrust();

    // ppcoin: compute stake entropy bit for stake modifier
    if (!pindexNew->SetStakeEntropyBit(GetStakeEntropyBit()))
        return error("AddToBlockIndex() : SetStakeEntropyBit() failed");

    // Record proof hash value
    pindexNew->hashProof = hashProof;

    // ppcoin: compute stake modifier
    uint64_t nStakeModifier = 0;
    bool fGeneratedStakeModifier = false;
    if (!ComputeNextStakeModifier(pindexNew->pprev, nStakeModifier, fGeneratedStakeModifier))
        return error("AddToBlockIndex() : ComputeNextStakeModifier() failed");
    pindexNew->SetStakeModifier(nStakeModifier, fGeneratedStakeModifier);
    
    // Add to mapBlockIndex
    map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.insert(make_pair(hash, pindexNew)).first;
    if (pindexNew->IsProofOfStake())
        setStakeSeen.insert(make_pair(pindexNew->prevoutStake, pindexNew->nStakeTime));
    pindexNew->phashBlock = &((*mi).first);

    // Write to disk block index
    CTxDB txdb;
    if (!txdb.TxnBegin())
        return false;

    txdb.WriteBlockIndex(CDiskBlockIndex(pindexNew));

    if (!txdb.TxnCommit())
        return false;
    
    LOCK(cs_main);
    
    
    // New best
    if (pindexNew->nChainTrust > nBestChainTrust)
        if (!SetBestChain(txdb, pindexNew))
            return false;

    if (pindexNew == pindexBest)
    {
        // Notify UI to display prev block's coinbase if it was ours
        static uint256 hashPrevBestCoinBase;
        UpdatedTransaction(hashPrevBestCoinBase);
        hashPrevBestCoinBase = vtx[0].GetHash();
    };

    uiInterface.NotifyBlocksChanged();
    return true;
}




bool CBlock::CheckBlock(bool fCheckPOW, bool fCheckMerkleRoot, bool fCheckSig) const
{
    // These are checks that are independent of context
    // that can be verified before saving an orphan block.

    // Size limits
    if (vtx.empty() || vtx.size() > MAX_BLOCK_SIZE || ::GetSerializeSize(*this, SER_NETWORK, PROTOCOL_VERSION) > MAX_BLOCK_SIZE)
        return DoS(100, error("CheckBlock() : size limits failed"));

    // Check proof of work matches claimed amount
    if (fCheckPOW && IsProofOfWork() && !CheckProofOfWork(GetHash(), nBits))
        return DoS(50, error("CheckBlock() : proof of work failed"));

    // Check timestamp
    if (GetBlockTime() > FutureDrift(GetAdjustedTime(), nBestHeight + 1))
        return error("CheckBlock() : block timestamp too far in the future");

    // First transaction must be coinbase, the rest must not be
    if (vtx.empty() || !vtx[0].IsCoinBase())
        return DoS(100, error("CheckBlock() : first tx is not coinbase"));
    for (unsigned int i = 1; i < vtx.size(); i++)
        if (vtx[i].IsCoinBase())
            return DoS(100, error("CheckBlock() : more than one coinbase"));
    
    if (IsProofOfStake())
    {
        // Coinbase output should be empty if proof-of-stake block
        if (vtx[0].vout.size() != 1 || !vtx[0].vout[0].IsEmpty())
            return DoS(100, error("CheckBlock() : coinbase output not empty for proof-of-stake block"));

        // Second transaction must be coinstake, the rest must not be
        if (vtx.empty() || !vtx[1].IsCoinStake())
            return DoS(100, error("CheckBlock() : second tx is not coinstake"));

        for (unsigned int i = 2; i < vtx.size(); i++)
            if (vtx[i].IsCoinStake())
                return DoS(100, error("CheckBlock() : more than one coinstake"));
        
        // Check proof-of-stake block signature
        if (fCheckSig && !CheckBlockSignature())
            return DoS(100, error("CheckBlock() : bad proof-of-stake block signature"));
    };

    // Check transactions
    BOOST_FOREACH(const CTransaction& tx, vtx)
    {
        if (!tx.CheckTransaction())
            return DoS(tx.nDoS, error("CheckBlock() : CheckTransaction failed"));

        // ppcoin: check transaction timestamp
        if (GetBlockTime() < (int64_t)tx.nTime)
            return DoS(50, error("CheckBlock() : block timestamp earlier than transaction timestamp"));
    };

    // Check for duplicate txids. This is caught by ConnectInputs(),
    // but catching it earlier avoids a potential DoS attack:
    set<uint256> uniqueTx;
    BOOST_FOREACH(const CTransaction& tx, vtx)
    {
        uniqueTx.insert(tx.GetHash());
    };

    if (uniqueTx.size() != vtx.size())
        return DoS(100, error("CheckBlock() : duplicate transaction"));

    unsigned int nSigOps = 0;
    BOOST_FOREACH(const CTransaction& tx, vtx)
    {
        nSigOps += tx.GetLegacySigOpCount();
    };

    if (nSigOps > MAX_BLOCK_SIGOPS)
        return DoS(100, error("CheckBlock() : out-of-bounds SigOpCount"));

    // Check merkle root
    if (fCheckMerkleRoot && hashMerkleRoot != BuildMerkleTree())
        return DoS(100, error("CheckBlock() : hashMerkleRoot mismatch"));


    return true;
}

bool CBlock::AcceptBlock()
{
    AssertLockHeld(cs_main);

    assert(nNodeMode == NT_FULL);

    if (nVersion > CURRENT_VERSION)
        return DoS(100, error("AcceptBlock() : reject unknown block version %d", nVersion));

    // Check for duplicate
    uint256 hash = GetHash();
    if (mapBlockIndex.count(hash))
        return error("AcceptBlock() : block already in mapBlockIndex");

    // Get prev block index
    map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hashPrevBlock);
    if (mi == mapBlockIndex.end())
        return DoS(10, error("AcceptBlock() : prev block not found"));
    CBlockIndex* pindexPrev = (*mi).second;
    int nHeight = pindexPrev->nHeight+1;
    
    if (Params().IsProtocolV2(nHeight) && nVersion < 7)
        return DoS(100, error("AcceptBlock() : reject too old nVersion = %d", nVersion));
    if (!Params().IsProtocolV2(nHeight) && nVersion > 6) 
        return DoS(100, error("AcceptBlock() : reject too new nVersion = %d", nVersion));

    if (IsProofOfWork() && nHeight > Params().LastPOWBlock())
        return DoS(100, error("AcceptBlock() : reject proof-of-work at height %d", nHeight));
   
    if (IsProofOfStake() && nHeight < Params().FirstPosBlock())
        return DoS(100, error("AcceptBlock() : reject proof-of-stake at height %d", nHeight));
 
    // Check coinbase timestamp
    if (GetBlockTime() > FutureDrift((int64_t)vtx[0].nTime, nHeight))
        return DoS(50, error("AcceptBlock() : coinbase timestamp is too early"));

    // Check coinstake timestamp
    if (IsProofOfStake() && !CheckCoinStakeTimestamp(nHeight, GetBlockTime(), (int64_t)vtx[1].nTime))
        return DoS(50, error("AcceptBlock() : coinstake timestamp violation nTimeBlock=%d nTimeTx=%u", GetBlockTime(), vtx[1].nTime));
    
    
    // Check proof-of-work or proof-of-stake
    if (nBits != GetNextTargetRequired(pindexPrev, IsProofOfStake()))
        return DoS(100, error("AcceptBlock() : incorrect %s", IsProofOfWork() ? "proof-of-work" : "proof-of-stake"));

    // Check timestamp against prev
    if (GetBlockTime() <= pindexPrev->GetPastTimeLimit() || FutureDrift(GetBlockTime(), nHeight) < pindexPrev->GetBlockTime())
        return error("AcceptBlock() : block's timestamp is too early");

    // Check that all transactions are finalized
    BOOST_FOREACH(const CTransaction& tx, vtx)
        if (!tx.IsFinal(nHeight, GetBlockTime()))
            return DoS(10, error("AcceptBlock() : contains a non-final transaction"));

    // Check that the block chain matches the known block chain up to a checkpoint
    if (!Checkpoints::CheckHardened(nHeight, hash))
        return DoS(100, error("AcceptBlock() : rejected by hardened checkpoint lock-in at %d", nHeight));

    uint256 hashProof;
    // Verify hash target and signature of coinstake tx
    if (IsProofOfStake())
    {
        uint256 targetProofOfStake;
        if (!CheckProofOfStake(pindexPrev, vtx[1], nBits, hashProof, targetProofOfStake))
        {
            LogPrintf("WARNING: AcceptBlock(): check proof-of-stake failed for block %s\n", hash.ToString().c_str());
            return false; // do not error here as we expect this during initial block download
        };
    };

    // PoW is checked in CheckBlock()
    if (IsProofOfWork())
    {
        hashProof = GetHash();
    };

    bool cpSatisfies = Checkpoints::CheckSync(hash, pindexPrev);

    // Check that the block satisfies synchronized checkpoint
    if (CheckpointsMode == Checkpoints::STRICT && !cpSatisfies)
        return error("AcceptBlock() : rejected by synchronized checkpoint");

    if (CheckpointsMode == Checkpoints::ADVISORY && !cpSatisfies)
        strMiscWarning = _("WARNING: syncronized checkpoint violation detected, but skipped!");

    // Enforce rule that the coinbase starts with serialized block height
    CScript expect = CScript() << nHeight;
    if (vtx[0].vin[0].scriptSig.size() < expect.size()
        || !std::equal(expect.begin(), expect.end(), vtx[0].vin[0].scriptSig.begin()))
        return DoS(100, error("AcceptBlock() : block height mismatch in coinbase"));
    
    if (fReindexing)
        return true;
    
    // Write block to history file
    if (!CheckDiskSpace(::GetSerializeSize(*this, SER_DISK, CLIENT_VERSION)))
        return error("AcceptBlock() : out of disk space");
    unsigned int nFile = -1;
    unsigned int nBlockPos = 0;
    if (!WriteToDisk(nFile, nBlockPos))
        return error("AcceptBlock() : WriteToDisk failed");
    if (!AddToBlockIndex(nFile, nBlockPos, hashProof))
        return error("AcceptBlock() : AddToBlockIndex failed");
    
    
    // Relay inventory, but don't relay old inventory during initial block download
    int nBlockEstimate = Checkpoints::GetTotalBlocksEstimate();
    if (hashBestChain == hash)
    {
        LOCK(cs_vNodes);
        BOOST_FOREACH(CNode* pnode, vNodes)
            if (nBestHeight > (pnode->nChainHeight != -1 ? pnode->nChainHeight - 2000 : nBlockEstimate))
                pnode->PushInventory(CInv(MSG_BLOCK, hash));
    };

    // ppcoin: check pending sync-checkpoint
    Checkpoints::AcceptPendingSyncCheckpoint();

    return true;
}

uint256 CBlockIndex::GetBlockTrust() const
{
    CBigNum bnTarget;
    bnTarget.SetCompact(nBits);

    if (bnTarget <= 0)
        return 0;

    return ((CBigNum(1)<<256) / (bnTarget+1)).getuint256();
}

bool CBlockIndex::IsSuperMajority(int minVersion, const CBlockIndex* pstart, unsigned int nRequired, unsigned int nToCheck)
{
    unsigned int nFound = 0;
    for (unsigned int i = 0; i < nToCheck && nFound < nRequired && pstart != NULL; i++)
    {
        if (pstart->nVersion >= minVersion)
            ++nFound;
        pstart = pstart->pprev;
    };
    return (nFound >= nRequired);
}

uint256 CBlockThinIndex::GetBlockTrust() const
{
    CBigNum bnTarget;
    bnTarget.SetCompact(nBits);

    if (bnTarget <= 0)
        return 0;

    return ((CBigNum(1)<<256) / (bnTarget+1)).getuint256();
}

bool ProcessBlock(CNode* pfrom, CBlock* pblock, uint256& hash)
{
    AssertLockHeld(cs_main);

    // Check for duplicate
    //uint256 hash = pblock->GetHash();
    std::string strHash = fDebug ? hash.ToString() : hash.ToString().substr(0,20);

    if (mapBlockIndex.count(hash))
        return error("ProcessBlock() : already have block %d %s", mapBlockIndex[hash]->nHeight, strHash.c_str());
    if (mapOrphanBlocks.count(hash))
        return error("ProcessBlock() : already have block (orphan) %s", strHash.c_str());

    // ppcoin: check proof-of-stake
    // Limited duplicity on stake: prevents block flood attack
    // Duplicate stake allowed only when there is orphan child block
    if (pblock->IsProofOfStake() && setStakeSeen.count(pblock->GetProofOfStake()) && !mapOrphanBlocksByPrev.count(hash) && !Checkpoints::WantedByPendingSyncCheckpoint(hash) && !fReindexing)
        return error("ProcessBlock() : duplicate proof-of-stake (%s, %d) for block %s", pblock->GetProofOfStake().first.ToString().c_str(), pblock->GetProofOfStake().second, strHash.c_str());

    // Preliminary checks
    if (!pblock->CheckBlock())
        return error("ProcessBlock() : CheckBlock %s FAILED", strHash.c_str());

    CBlockIndex* pcheckpoint = Checkpoints::GetLastSyncCheckpoint();
    if (pcheckpoint && pblock->hashPrevBlock != hashBestChain && !Checkpoints::WantedByPendingSyncCheckpoint(hash))
    {
        // Extra checks to prevent "fill up memory by spamming with bogus blocks"
        int64_t deltaTime = pblock->GetBlockTime() - pcheckpoint->nTime;
        if (deltaTime < 0)
        {
            if (pfrom)
                pfrom->Misbehaving(1);
            return error("ProcessBlock() : block with timestamp before last checkpoint");
        }
    };

    // ppcoin: ask for pending sync-checkpoint if any
    if (!IsInitialBlockDownload())
        Checkpoints::AskForPendingSyncCheckpoint(pfrom);

    // If don't already have its previous block, shunt it off to holding area until we get it
    if (!mapBlockIndex.count(pblock->hashPrevBlock))
    {
        LogPrintf("ProcessBlock: ORPHAN BLOCK, prev=%s\n", pblock->hashPrevBlock.ToString().substr(0,20).c_str());
        // ppcoin: check proof-of-stake
        if (pblock->IsProofOfStake())
        {
            // Limited duplicity on stake: prevents block flood attack
            // Duplicate stake allowed only when there is orphan child block
            if (setStakeSeenOrphan.count(pblock->GetProofOfStake()) && !mapOrphanBlocksByPrev.count(hash) && !Checkpoints::WantedByPendingSyncCheckpoint(hash))
                return error("ProcessBlock() : duplicate proof-of-stake (%s, %d) for orphan block %s", pblock->GetProofOfStake().first.ToString().c_str(), pblock->GetProofOfStake().second, hash.ToString().c_str());
            else
                setStakeSeenOrphan.insert(pblock->GetProofOfStake());
        };

        CBlock* pblock2 = new CBlock(*pblock);
        mapOrphanBlocks.insert(make_pair(hash, pblock2));
        mapOrphanBlocksByPrev.insert(make_pair(pblock2->hashPrevBlock, pblock2));

        // Ask this guy to fill in what we're missing
        if (pfrom)
        {
            pfrom->PushGetBlocks(pindexBest, GetOrphanRoot(pblock2));
            // ppcoin: getblocks may not obtain the ancestor block rejected
            // earlier by duplicate-stake check so we ask for it again directly
            if (!IsInitialBlockDownload())
                pfrom->AskFor(CInv(MSG_BLOCK, WantedByOrphan(pblock2)));
        };

        return true;
    };

    // Store to disk
    if (!pblock->AcceptBlock())
        return error("ProcessBlock() : AcceptBlock %s FAILED", strHash.c_str());

    // Recursively process any orphan blocks that depended on this one
    vector<uint256> vWorkQueue;
    vWorkQueue.push_back(hash);
    for (unsigned int i = 0; i < vWorkQueue.size(); i++)
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
            setStakeSeenOrphan.erase(pblockOrphan->GetProofOfStake());
            delete pblockOrphan;
        };
        mapOrphanBlocksByPrev.erase(hashPrev);
    };

    LogPrintf("ProcessBlock: ACCEPTED %s\n", strHash.c_str());

    // ppcoin: if responsible for sync-checkpoint send it
    //if (!Params().IsProtocolV2(pindex->nHeight)) // TODO: posv2 remove
    if (pfrom && !CSyncCheckpoint::strMasterPrivKey.empty())
        Checkpoints::SendSyncCheckpoint(Checkpoints::AutoSelectSyncCheckpoint());

    return true;
}

bool ProcessBlockThin(CNode* pfrom, CBlockThin* pblock)
{
    AssertLockHeld(cs_main);

    // Check for duplicate
    uint256 hash = pblock->GetHash();
    if (mapBlockThinIndex.count(hash))
        return error("ProcessBlockThin() : already have header %d %s", mapBlockThinIndex[hash]->nHeight, hash.ToString().substr(0,20).c_str());
    if (mapOrphanBlockThins.count(hash))
        return error("ProcessBlockThin() : already have header (orphan) %s", hash.ToString().substr(0,20).c_str());

    // ppcoin: check proof-of-stake
    // Limited duplicity on stake: prevents block flood attack
    // Duplicate stake allowed only when there is orphan child block
    //if (pblock->IsProofOfStake() && setStakeSeen.count(pblock->GetProofOfStake()) && !mapOrphanBlocksByPrev.count(hash) && !Checkpoints::WantedByPendingSyncCheckpoint(hash))
    //    return error("ProcessBlockThin() : duplicate proof-of-stake (%s, %d) for block %s", pblock->GetProofOfStake().first.ToString().c_str(), pblock->GetProofOfStake().second, hash.ToString().c_str());

    // Preliminary checks
    if (!pblock->CheckBlockThin())
        return error("ProcessBlockThin() : CheckBlockThin FAILED");

    CBlockThinIndex* pcheckpoint = Checkpoints::GetLastSyncCheckpointHeader();
    if (pcheckpoint && pblock->hashPrevBlock != hashBestChain && !Checkpoints::WantedByPendingSyncCheckpointHeader(hash))
    {
        // Extra checks to prevent "fill up memory by spamming with bogus blocks"
        int64_t deltaTime = pblock->GetBlockTime() - pcheckpoint->nTime;
        if (deltaTime < 0)
        {
            if (pfrom)
                pfrom->Misbehaving(1);
            return error("ProcessBlock() : block with timestamp before last checkpoint");
        };
    };

    // ppcoin: ask for pending sync-checkpoint if any
    //if (!IsInitialBlockDownload())
    //    Checkpoints::AskForPendingSyncCheckpoint(pfrom);

    // If don't already have its previous block, shunt it off to holding area until we get it
    if (!mapBlockThinIndex.count(pblock->hashPrevBlock))
    {
        LogPrintf("ProcessBlockThin: ORPHAN BLOCK, prev=%s\n", pblock->hashPrevBlock.ToString().substr(0,20).c_str());

        /*
        // ppcoin: check proof-of-stake
        if (pblock->IsProofOfStake())
        {
            // Limited duplicity on stake: prevents block flood attack
            // Duplicate stake allowed only when there is orphan child block
            if (setStakeSeenOrphan.count(pblock->GetProofOfStake()) && !mapOrphanBlocksByPrev.count(hash) && !Checkpoints::WantedByPendingSyncCheckpoint(hash))
                return error("ProcessBlock() : duplicate proof-of-stake (%s, %d) for orphan block %s", pblock->GetProofOfStake().first.ToString().c_str(), pblock->GetProofOfStake().second, hash.ToString().c_str());
            else
                setStakeSeenOrphan.insert(pblock->GetProofOfStake());
        };
        */
        CBlockThin* pblock2 = new CBlockThin(*pblock);
        mapOrphanBlockThins.insert(make_pair(hash, pblock2));
        mapOrphanBlockThinsByPrev.insert(make_pair(pblock2->hashPrevBlock, pblock2));

        /*
        // Ask this guy to fill in what we're missing
        if (pfrom)
        {
            pfrom->PushGetBlocks(pindexBest, GetOrphanRoot(pblock2));
            // ppcoin: getblocks may not obtain the ancestor block rejected
            // earlier by duplicate-stake check so we ask for it again directly
            if (!IsInitialBlockDownload())
                pfrom->AskFor(CInv(MSG_BLOCK, WantedByOrphan(pblock2)));
        };
        */
        return true;
    };

    // Store to disk
    if (!pblock->AcceptBlockThin())
        return error("ProcessBlockThin() : AcceptHeader FAILED");

    // Recursively process any orphan blocks that depended on this one
    vector<uint256> vWorkQueue;
    vWorkQueue.push_back(hash);
    for (unsigned int i = 0; i < vWorkQueue.size(); i++)
    {
        uint256 hashPrev = vWorkQueue[i];
        for (multimap<uint256, CBlockThin*>::iterator mi = mapOrphanBlockThinsByPrev.lower_bound(hashPrev);
             mi != mapOrphanBlockThinsByPrev.upper_bound(hashPrev);
             ++mi)
        {
            CBlockThin* pblockOrphan = (*mi).second;

            if (pblockOrphan->AcceptBlockThin())
                vWorkQueue.push_back(pblockOrphan->GetHash());

            mapOrphanBlockThins.erase(pblockOrphan->GetHash());
            //setStakeSeenOrphan.erase(pblockOrphan->GetProofOfStake());
            delete pblockOrphan;
        };
        mapOrphanBlockThinsByPrev.erase(hashPrev);
    };

    if (fDebugChain)
        LogPrintf("ProcessBlockThin: ACCEPTED\n");

    if ((nNodeState == NS_GET_HEADERS)
        && nBestHeight >= GetNumBlocksOfPeers())
    {
        ChangeNodeState(NS_GET_FILTERED_BLOCKS);
    };


    // -- only full nodes send checkpoints

    return true;
}

// novacoin: attempt to generate suitable proof-of-stake
bool CBlock::SignBlock(CWallet& wallet, int64_t nFees)
{

    // if we are trying to sign
    //    something except proof-of-stake block template
    if (!vtx[0].vout[0].IsEmpty())
        return false;

    // if we are trying to sign
    //    a complete proof-of-stake block
    if (IsProofOfStake())
        return true;

    static int64_t nLastCoinStakeSearchTime = GetAdjustedTime(); // startup timestamp

    CKey key;
    CTransaction txCoinStake;
    if (Params().IsProtocolV2(nBestHeight+1))
        txCoinStake.nTime &= ~STAKE_TIMESTAMP_MASK;
    
    int64_t nSearchTime = txCoinStake.nTime; // search to current time

    if (nSearchTime > nLastCoinStakeSearchTime)
    {
        int64_t nSearchInterval = Params().IsProtocolV2(nBestHeight+1) ? 1 : nSearchTime - nLastCoinStakeSearchTime;
        if (wallet.CreateCoinStake(wallet, nBits, nSearchInterval, nFees, txCoinStake, key))
        {
            if (txCoinStake.nTime >= pindexBest->GetPastTimeLimit()+1)
            {

                // make sure coinstake would meet timestamp protocol
                //    as it would be the same as the block timestamp
                vtx[0].nTime = nTime = txCoinStake.nTime;

                // we have to make sure that we have no future timestamps in
                //    our transactions set
                for (vector<CTransaction>::iterator it = vtx.begin(); it != vtx.end();)
                {
                    if (it->nTime > nTime)
                    {
                        it = vtx.erase(it);
                    } else
                    {
                        ++it;
                    };
                };

                vtx.insert(vtx.begin() + 1, txCoinStake);
                hashMerkleRoot = BuildMerkleTree();

                // append a signature to our block
                return key.Sign(GetHash(), vchBlockSig);
            };
        };
        nLastCoinStakeSearchInterval = nSearchTime - nLastCoinStakeSearchTime;
        nLastCoinStakeSearchTime = nSearchTime;
    };

    return false;
}

bool CBlock::CheckBlockSignature() const
{
    if (IsProofOfWork())
        return vchBlockSig.empty();

    vector<valtype> vSolutions;
    txnouttype whichType;

    const CTxOut& txout = vtx[1].vout[1];

    if (!Solver(txout.scriptPubKey, whichType, vSolutions))
        return false;

    if (whichType == TX_PUBKEY)
    {
        valtype& vchPubKey = vSolutions[0];
        return CPubKey(vchPubKey).Verify(GetHash(), vchBlockSig);
    };

    return false;
}

bool CBlock::GetHashProof(uint256& hashProof)
{
    // PoW is checked in CheckBlock()
    if (IsProofOfWork())
    {
        hashProof = GetHash();
        return true;
    };
    
    // Get prev block index
    map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hashPrevBlock);
    if (mi == mapBlockIndex.end())
        return DoS(10, error("GetHashProof() : prev block not found"));
    CBlockIndex* pindexPrev = (*mi).second;
    int nHeight = pindexPrev->nHeight+1;

    uint256 targetProofOfStake;
    if (!CheckProofOfStake(pindexPrev, vtx[1], nBits, hashProof, targetProofOfStake))
    {
        LogPrintf("WARNING: GetHashProof(): check proof-of-stake failed, height %d\n", nHeight);
        return false;
    };
    
    return true;
};

bool CheckDiskSpace(uint64_t nAdditionalBytes)
{
    uint64_t nFreeBytesAvailable = boost::filesystem::space(GetDataDir()).available;

    // Check for nMinDiskSpace bytes (currently 50MB)
    if (nFreeBytesAvailable < nMinDiskSpace + nAdditionalBytes)
    {
        string strMessage = _("Warning: Disk space is low!");
        strMiscWarning = strMessage;
        LogPrintf("*** %s\n", strMessage.c_str());
        uiInterface.ThreadSafeMessageBox(strMessage, "Gameunits", CClientUIInterface::BTN_OK | CClientUIInterface::ICON_WARNING | CClientUIInterface::MODAL);
        StartShutdown();
        return false;
    };
    return true;
}


static unsigned int nCurrentBlockFile = 1;
static unsigned int nCurrentBlockThinFile = 1;

FILE* OpenBlockFile(bool fHeaderFile, unsigned int nFile, unsigned int nBlockPos, const char* pszMode)
{
    if ((nFile < 1) || (nFile == (unsigned int) -1))
        return NULL;

    string strBlockFn = strprintf(fHeaderFile ? "blk_hdr%04u.dat": "blk%04u.dat", nFile);
    FILE* file = fopen((GetDataDir() / strBlockFn).string().c_str(), pszMode);
    if (!file)
        return NULL;

    if (nBlockPos != 0 && !strchr(pszMode, 'a') && !strchr(pszMode, 'w'))
    {
        if (fseek(file, nBlockPos, SEEK_SET) != 0)
        {
            fclose(file);
            return NULL;
        };
    };
    return file;
}

FILE* AppendBlockFile(bool fHeaderFile, unsigned int& nFileRet, const char* fmode)
{
    nFileRet = 0;
    while (true)
    {
        FILE* file = OpenBlockFile(fHeaderFile, fHeaderFile ? nCurrentBlockThinFile : nCurrentBlockFile,
            0, fmode);

        if (!file)
            return NULL;
        if (fseek(file, 0, SEEK_END) != 0)
            return NULL;
        // FAT32 file size max 4GB, fseek and ftell max 2GB, so we must stay under 2GB
        if (ftell(file) < (long)(0x7F000000 - MAX_SIZE))
        {
            nFileRet = nCurrentBlockFile;
            return file;
        };

        fclose(file);
        nCurrentBlockFile++;
    };
}


int LoadBlockIndex(bool fAllowNew)
{
    LOCK(cs_main);
    
    //
    // Load block index
    //
    CTxDB txdb("cr+");
    
    // CheckVersion() will wipe old databases, try reload db from chain with reindex
    // before reinexing the genesis block must be created
    int tryReindex = txdb.CheckVersion() != 0 ? 1 : 0;
    
    if (nNodeMode == NT_FULL)
    {
        if (!txdb.LoadBlockIndex())
            return 1;
        
        if (!pwalletMain->CacheAnonStats())
            LogPrintf("CacheAnonStats() failed.\n");
    } else
    {
        if (!txdb.LoadBlockThinIndex())
            return 1;
    };
    
    //
    // Init with genesis block
    //
    if ((nNodeMode == NT_FULL && mapBlockIndex.empty())
        || (nNodeMode != NT_FULL && mapBlockThinIndex.empty()))
    {
        if (!fAllowNew)
            return 1;

        CBlock &block = const_cast<CBlock&>(Params().GenesisBlock());
        // -- debug print
        if (fDebugChain)
        {
            LogPrintf("Initialised genesis block:\n");
            block.print();
        };
        
        // Start new block file
        unsigned int nFile;
        unsigned int nBlockPos;
        
        if (nNodeMode == NT_FULL)
        {
            if (!block.WriteToDisk(nFile, nBlockPos))
                return error1("LoadBlockIndex() : writing genesis block to disk failed");
            if (!block.AddToBlockIndex(nFile, nBlockPos, Params().HashGenesisBlock()))
                return error1("LoadBlockIndex() : genesis block not accepted");
        } else
        {
            CBlockThin blockThin(block);
            if (!blockThin.AddToBlockThinIndex(nFile, nBlockPos, Params().HashGenesisBlock()))
                return error1("LoadBlockIndex() : genesis block not accepted");
            
            pindexRear = pindexGenesisBlockThin;
        };
        
        // ppcoin: initialize synchronized checkpoint
        if (!Checkpoints::WriteSyncCheckpoint(Params().HashGenesisBlock()))
            return error1("LoadBlockIndex() : failed to init sync checkpoint");
    };
    
    string strPubKey = "";
    
    // if checkpoint master key changed must reset sync-checkpoint
    if (!txdb.ReadCheckpointPubKey(strPubKey) || strPubKey != CSyncCheckpoint::strMasterPubKey)
    {
        // write checkpoint master key to db
        txdb.TxnBegin();
        if (!txdb.WriteCheckpointPubKey(CSyncCheckpoint::strMasterPubKey))
            return error1("LoadBlockIndex() : failed to write new checkpoint master key to db");
        if (!txdb.TxnCommit())
            return error1("LoadBlockIndex() : failed to commit new checkpoint master key to db");
        if (!fTestNet)
        {
            if ((nNodeMode == NT_FULL && !Checkpoints::ResetSyncCheckpoint())
                || (nNodeMode == NT_THIN && !Checkpoints::ResetSyncCheckpointThin()))
                return error1("LoadBlockIndex() : failed to reset sync-checkpoint");
        };
    };

    return tryReindex ? 2 : 0;
}



void PrintBlockTree()
{
    AssertLockHeld(cs_main);
    // pre-compute tree structure
    map<CBlockIndex*, vector<CBlockIndex*> > mapNext;
    for (map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.begin(); mi != mapBlockIndex.end(); ++mi)
    {
        CBlockIndex* pindex = (*mi).second;
        mapNext[pindex->pprev].push_back(pindex);
        // test
        //while (rand() % 3 == 0)
        //    mapNext[pindex->pprev].push_back(pindex);
    };

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
                LogPrintf("| ");
            LogPrintf("|\\\n");
        } else
        if (nCol < nPrevCol)
        {
            for (int i = 0; i < nCol; i++)
                LogPrintf("| ");
            LogPrintf("|\n");
        };
        nPrevCol = nCol;

        // print columns
        for (int i = 0; i < nCol; i++)
            LogPrintf("| ");

        // print item
        CBlock block;
        block.ReadFromDisk(pindex);
        LogPrintf("%d (%u,%u) %s  %08x  %s  mint %7s  tx %u",
            pindex->nHeight,
            pindex->nFile,
            pindex->nBlockPos,
            block.GetHash().ToString().c_str(),
            block.nBits,
            DateTimeStrFormat("%x %H:%M:%S", block.GetBlockTime()).c_str(),
            FormatMoney(pindex->nMint).c_str(),
            block.vtx.size());

        PrintWallets(block);

        // put the main time-chain first
        vector<CBlockIndex*>& vNext = mapNext[pindex];
        for (unsigned int i = 0; i < vNext.size(); i++)
        {
            if (vNext[i]->pnext)
            {
                swap(vNext[0], vNext[i]);
                break;
            };
        };

        // iterate children
        for (unsigned int i = 0; i < vNext.size(); i++)
            vStack.push_back(make_pair(nCol+i, vNext[i]));
    };
}

bool LoadExternalBlockFile(int nFile, FILE* fileIn)
{
    if (nNodeMode != NT_FULL)
    {
        LogPrintf("LoadExternalBlockFile() for thin client is not implemented yet.");
        return false;
    };

    int64_t nStart = GetTimeMillis();

    int nLoaded = 0;

    {
        LOCK(cs_main);
        try {
            CAutoFile blkdat(fileIn, SER_DISK, CLIENT_VERSION);
            unsigned int nPos = 0;
            while (nPos != (unsigned int)-1 && blkdat.good())
            {
                boost::this_thread::interruption_point();
                unsigned char pchData[65536];
                for (;;)
                {
                    boost::this_thread::interruption_point();
                    fseek(blkdat, nPos, SEEK_SET);
                    int nRead = fread(pchData, 1, sizeof(pchData), blkdat);
                    if (nRead <= 8)
                    {
                        nPos = (unsigned int)-1;
                        break;
                    };

                    void* nFind = memchr(pchData, Params().MessageStart()[0], nRead+1-MESSAGE_START_SIZE);
                    if (nFind)
                    {
                        if (memcmp(nFind, Params().MessageStart(), MESSAGE_START_SIZE)==0)
                        {
                            nPos += ((unsigned char*)nFind - pchData) + MESSAGE_START_SIZE;
                            break;
                        };
                        nPos += ((unsigned char*)nFind - pchData) + 1;
                    } else
                    {
                        nPos += sizeof(pchData) - MESSAGE_START_SIZE + 1;
                    };
                };

                if (nPos == (unsigned int)-1)
                    break;

                fseek(blkdat, nPos, SEEK_SET);
                unsigned int nSize;
                blkdat >> nSize;
                if (nSize > 0 && nSize <= MAX_BLOCK_SIZE)
                {
                    CBlock block;
                    blkdat >> block;
                    uint256 hashblock = block.GetHash();
                    if (ProcessBlock(NULL, &block, hashblock))
                    {
                        uint256 hashProof;
                        if (fReindexing
                            && (!block.GetHashProof(hashProof)
                                || !block.AddToBlockIndex(nFile, nPos+sizeof(nSize), hashProof))) // blockPos is after nSize
                            LogPrintf("LoadExternalBlockFile() : AddToBlockIndex failed %s\n", hashblock.ToString().c_str());
                        nLoaded++;
                        nPos += 4 + nSize;
                        
                        if (nLoaded % 20000 == 0)
                            LogPrintf("Loaded %d blocks and counting.\n", nLoaded);
                    };
                };
            };
        } catch (std::exception &e)
        {
            LogPrintf("%s() : Deserialize or I/O error caught during load\n",
                   __PRETTY_FUNCTION__);
        };
    }
    LogPrintf("Loaded %i blocks from external file in %dms\n", nLoaded, GetTimeMillis() - nStart);
    return nLoaded > 0;
}

//////////////////////////////////////////////////////////////////////////////
//
// CAlert
//

extern map<uint256, CAlert> mapAlerts;
extern CCriticalSection cs_mapAlerts;

string GetWarnings(string strFor)
{
    int nPriority = 0;
    string strStatusBar;
    string strRPC;

    if (GetBoolArg("-testsafemode"))
        strRPC = "test";

    // Misc warnings like out of disk space and clock is wrong
    if (strMiscWarning != "")
    {
        nPriority = 1000;
        strStatusBar = strMiscWarning;
    };

    // if detected invalid checkpoint enter safe mode
    if (Checkpoints::hashInvalidCheckpoint != 0)
    {
        nPriority = 3000;
        strStatusBar = strRPC = _("WARNING: Invalid checkpoint found! Displayed transactions may not be correct! You may need to upgrade, or notify developers.");
    };

    // Alerts
    {
        LOCK(cs_mapAlerts);
        BOOST_FOREACH(PAIRTYPE(const uint256, CAlert)& item, mapAlerts)
        {
            const CAlert& alert = item.second;
            if (alert.AppliesToMe() && alert.nPriority > nPriority)
            {
                nPriority = alert.nPriority;
                strStatusBar = alert.strStatusBar;
                if (nPriority > 1000)
                    strRPC = strStatusBar;
            };
        };
    }

    if (strFor == "statusbar")
        return strStatusBar;
    else
    if (strFor == "rpc")
        return strRPC;
    assert(!"GetWarnings() : invalid parameter");
    return "error";
}








//////////////////////////////////////////////////////////////////////////////
//
// Messages
//


bool static AlreadyHave(CTxDB& txdb, const CInv& inv)
{
    switch (inv.type)
    {
    case MSG_TX:
        {
        bool txInMap = false;
        txInMap = mempool.exists(inv.hash);
        return txInMap
            || mapOrphanTransactions.count(inv.hash)
            || txdb.ContainsTx(inv.hash);
        }

    case MSG_BLOCK:
        return mapBlockIndex.count(inv.hash)
            || mapOrphanBlocks.count(inv.hash);
    }
    // Don't know what it is, just say we already got one
    return true;
}

bool static AlreadyHaveThin(CTxDB& txdb, const CInv& inv)
{
    switch (inv.type)
    {
    case MSG_TX:
        {
        bool txInMap = false;
        txInMap = mempool.exists(inv.hash);
        return txInMap
            || mapOrphanTransactions.count(inv.hash)
            || txdb.ContainsTx(inv.hash);
        }

    case MSG_BLOCK:
        return mapBlockThinIndex.count(inv.hash)
            || mapOrphanBlockThins.count(inv.hash);

    case MSG_FILTERED_BLOCK:
        return mapBlockThinIndex.count(inv.hash)
            || mapOrphanBlockThins.count(inv.hash);
    }
    // Don't know what it is, just say we already got one
    return true;
}

static void ProcessGetData(CNode* pfrom)
{
    if (fDebugNet)
        LogPrintf("ProcessGetData\n");


    std::deque<CInv>::iterator it = pfrom->vRecvGetData.begin();

    vector<CInv> vNotFound;
    vector<CInv> vMerkleBlocks;

    LOCK(cs_main);
    std::vector<CBlock> vMultiBlock;
    std::vector<CMBlkThinElement> vMultiBlockThin; // TODO: split ProcessGetDataThinPeer from ProcessGetData
    uint32_t nMultiBlockBytes = 0;

    while (it != pfrom->vRecvGetData.end())
    {
        // Don't bother if send buffer is too full to respond anyway
        if (pfrom->nSendSize >= SendBufferSize())
            break;

        boost::this_thread::interruption_point();

        const CInv &inv = *it;
        it++;

        if (nNodeMode != NT_FULL
            && inv.type != MSG_TX)
        {
            if (fDebug)
                LogPrintf("strCommand getdata not for txn.\n");
            continue;
        };

        if (inv.type == MSG_BLOCK
            || inv.type == MSG_FILTERED_BLOCK)
        {
            bool send = false;
            CBlockIndex *pBlockIndex;

            map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(inv.hash);

            if (mi != mapBlockIndex.end())
            {
                pBlockIndex = (*mi).second;

                // If the requested block is at a height below our last
                // checkpoint, only serve it if it's in the checkpointed chain
                int nHeight = mi->second->nHeight;
                CBlockIndex* pcheckpoint = Checkpoints::GetLastCheckpoint(mapBlockIndex);
                if (pcheckpoint && nHeight < pcheckpoint->nHeight)
                {
                    //if (!chainActive.Contains(mi->second))

                    // -- check if best chain contains block
                    //    necessary? faster way? (mark unlinked blocks)
                    CBlockIndex *pindex = pindexBest;
                    while (pindex && pindex != mi->second && pindex->pprev)
                        pindex = pindex->pprev;

                    if ((!pindex->pprev && pindex != mi->second)) // reached start of chain.
                    {
                        LogPrintf("ProcessGetData(): ignoring request for old block that isn't in the main chain\n");
                    } else
                    {
                        send = true;
                    };
                } else
                {
                    send = true;
                };
            };

            if (send)
            {
                // Send block from disk
                CBlock block;
                if (!block.ReadFromDisk(pBlockIndex))
                {
                    LogPrintf("Error: block.ReadFromDisk failed - Terminating.");
                    exit(1);
                };

                if (inv.type == MSG_BLOCK)
                {
                    if (pfrom->nVersion >= MIN_MBLK_VERSION)
                    {
                        uint32_t nBlockBytes = ::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION);

                        if (vMultiBlock.size() >= MAX_MULTI_BLOCK_ELEMENTS
                            || nMultiBlockBytes + nBlockBytes > MAX_MULTI_BLOCK_SIZE)
                        {
                            pfrom->PushMessage("mblk", vMultiBlock);
                            vMultiBlock.clear();
                            nMultiBlockBytes = 0;
                        }

                        vMultiBlock.push_back(block);
                        nMultiBlockBytes += nBlockBytes;
                    } else
                    {
                        pfrom->PushMessage("block", block);
                    }
                } else
                {
                    // MSG_FILTERED_BLOCK)
                    LOCK(pfrom->cs_filter);
                    if (pfrom->pfilter)
                    {
                        CMerkleBlock merkleBlock(block, pBlockIndex, *(pfrom->pfilter));
                        typedef std::pair<unsigned int, uint256> PairType;

                        if (pfrom->nVersion >= MIN_MBLK_VERSION)
                        {
                            uint32_t nBlockBytes = ::GetSerializeSize(merkleBlock, SER_NETWORK, PROTOCOL_VERSION);

                            CMBlkThinElement mbElem;
                            mbElem.merkleBlock = merkleBlock;

                            BOOST_FOREACH(PairType& pair, merkleBlock.vMatchedTxn)
                            {
                                if (!pfrom->setInventoryKnown.count(CInv(MSG_TX, pair.second)))
                                {
                                    nBlockBytes += ::GetSerializeSize(block.vtx[pair.first], SER_NETWORK, PROTOCOL_VERSION);
                                    mbElem.vtx.push_back(block.vtx[pair.first]);
                                };
                            };

                            if (vMultiBlockThin.size() >= MAX_MULTI_BLOCK_THIN_ELEMENTS
                                || nMultiBlockBytes + nBlockBytes > MAX_MULTI_BLOCK_SIZE)
                            {
                                pfrom->PushMessage("mblkt", vMultiBlockThin);
                                vMultiBlockThin.clear();
                                nMultiBlockBytes = 0;
                            };

                            vMultiBlockThin.push_back(mbElem);
                            nMultiBlockBytes += nBlockBytes;

                        } else
                        {
                            pfrom->PushMessage("merkleblock", merkleBlock);

                            // CMerkleBlock just contains hashes, so also push any transactions in the block the client did not see
                            // This avoids hurting performance by pointlessly requiring a round-trip
                            // Note that there is currently no way for a node to request any single transactions we didnt send here -
                            // they must either disconnect and retry or request the full block.
                            // Thus, the protocol spec specified allows for us to provide duplicate txn here,
                            // however we MUST always provide at least what the remote peer needs
                            BOOST_FOREACH(PairType& pair, merkleBlock.vMatchedTxn)
                            {
                                if (!pfrom->setInventoryKnown.count(CInv(MSG_TX, pair.second)))
                                {
                                    pfrom->PushMessage("tx", block.vtx[pair.first]);
                                };
                            };
                        }
                    };
                    // else
                        // no response
                };

                // Trigger them to send a getblocks request for the next batch of inventory
                if (inv.hash == pfrom->hashContinue)
                {
                    // Bypass PushInventory, this must send even if redundant,
                    // and we want it right after the last block so they don't
                    // wait for other stuff first.
                    vector<CInv> vInv;

                    // ppcoin: send latest proof-of-work block to allow the
                    // download node to accept as orphan (proof-of-stake
                    // block might be rejected by stake connection check)

                    // unless PoW phase is over
                    bool fReturnPoSBlock = nBestHeight <= Params().LastPOWBlock() ? true : false;

                    vInv.push_back(CInv(MSG_BLOCK, GetLastBlockIndex(pindexBest, fReturnPoSBlock)->GetBlockHash()));
                    pfrom->PushMessage("inv", vInv);
                    pfrom->hashContinue = 0;
                };
            };
        } else
        if (inv.IsKnownType())
        {
            // Send stream from relay memory
            bool pushed = false;
            {
                LOCK(cs_mapRelay);
                map<CInv, CDataStream>::iterator mi = mapRelay.find(inv);
                if (mi != mapRelay.end())
                {
                    pfrom->PushMessage(inv.GetCommand(), (*mi).second);
                    pushed = true;
                };
            };

            if (!pushed && inv.type == MSG_TX)
            {
                CTransaction tx;
                if (mempool.lookup(inv.hash, tx))
                {
                    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
                    ss.reserve(1000);
                    ss << tx;
                    pfrom->PushMessage("tx", ss);
                    pushed = true;
                };
            };

            if (!pushed)
            {
                vNotFound.push_back(inv);
            };
        };

        // Track requests for our stuff.
        Inventory(inv.hash);

        // -- break here to give chance to process other messages
        //    ProcessGetData will be called again in ProcessMessages
        if (pfrom->nVersion >= MIN_MBLK_VERSION)
        {
            if (vMultiBlock.size() >= MAX_MULTI_BLOCK_ELEMENTS)
            {
                pfrom->PushMessage("mblk", vMultiBlock);
                vMultiBlock.clear();
                break;
            };
            if (vMultiBlockThin.size() >= MAX_MULTI_BLOCK_THIN_ELEMENTS)
            {
                pfrom->PushMessage("mblkt", vMultiBlockThin);
                vMultiBlockThin.clear();
                break;
            };
        } else
        {
            if (inv.type == MSG_BLOCK || inv.type == MSG_FILTERED_BLOCK)
                break;
        };
    };

    if (vMultiBlock.size() > 0)
        pfrom->PushMessage("mblk", vMultiBlock);

    if (vMultiBlockThin.size() > 0)
        pfrom->PushMessage("mblkt", vMultiBlockThin);

    pfrom->vRecvGetData.erase(pfrom->vRecvGetData.begin(), it);

    if (!vNotFound.empty())
    {
        // Let the peer know that we didn't find what it asked for, so it doesn't
        // have to wait around forever. Currently only SPV clients actually care
        // about this message: it's needed when they are recursively walking the
        // dependencies of relevant unconfirmed transactions. SPV clients want to
        // do that because they want to know about (and store and rebroadcast and
        // risk analyze) the dependencies of transactions relevant to them, without
        // having to download the entire memory pool.
        pfrom->PushMessage("notfound", vNotFound);
    };

    if (fDebugNet)
        LogPrintf("ProcessGetData - End\n");
}

static int ProcessMerkleBlock(CNode* pfrom, CMerkleBlockIncoming& merkleBlock, std::vector<CTransaction>* pvTxns)
{
    if (fDebugNet)
        LogPrintf("ProcessMerkleBlock\n");

    uint256 hashBlock = merkleBlock.header.GetHash();
    
    LOCK(cs_main);
    
    bool fAlloc = false;

    CBlockThinIndex *pBlockThinIndex = NULL;
    std::map<uint256, CBlockThinIndex*>::iterator mi = mapBlockThinIndex.find(hashBlock);

    if (mi != mapBlockThinIndex.end())
    {
        pBlockThinIndex = mi->second;
    } else
    {
        if (!fThinFullIndex && pindexRear
            && merkleBlock.header.nTime < pindexRear->nTime)
        {
            // -- find block outside of index window
            CTxDB txdb("r");
            CDiskBlockThinIndex diskindex;
            if (txdb.ReadBlockThinIndex(hashBlock, diskindex))
            {
                if ((pBlockThinIndex = new CBlockThinIndex()))
                {
                    fAlloc = true;
                    pBlockThinIndex->nHeight              = diskindex.nHeight;
                    pBlockThinIndex->nFlags               = diskindex.nFlags;
                    pBlockThinIndex->nStakeModifier       = diskindex.nStakeModifier;
                    pBlockThinIndex->hashProof            = diskindex.hashProof;
                    pBlockThinIndex->nVersion             = diskindex.nVersion;
                    pBlockThinIndex->hashMerkleRoot       = diskindex.hashMerkleRoot;
                    pBlockThinIndex->nTime                = diskindex.nTime;
                    pBlockThinIndex->nBits                = diskindex.nBits;
                    pBlockThinIndex->nNonce               = diskindex.nNonce;
                } else
                {
                    LogPrintf("receive merkleblock new failed.\n");
                };
            } else
            {
                if (fDebug)
                    LogPrintf("ReadBlockThinIndex %s\n", hashBlock.ToString().c_str());
            };
        } else
        {
            // -- add new block
            ProcessBlockThin(pfrom, &merkleBlock.header);
            mi = mapBlockThinIndex.find(hashBlock);
            if (mi != mapBlockThinIndex.end())
                pBlockThinIndex = mi->second;
        };
    };

    if (!pBlockThinIndex)
    {
        LogPrintf("Header %s not found in chain.\n", hashBlock.ToString().c_str());
        return 0;
    };

    int nHeight = pBlockThinIndex->nHeight;

    if (nNodeState == NS_GET_FILTERED_BLOCKS)
    {
        std::vector<CPendingFilteredChunk>::iterator it;

        // -- see if node has received all of vPendingFilteredChunks
        for (it = vPendingFilteredChunks.begin(); it < vPendingFilteredChunks.end(); ++it)
        {
            if (it->endHash != hashBlock)
                continue;

            if (fDebugNet)
                LogPrintf("Got end of merbleblock chunk %s\n", hashBlock.ToString().c_str());

            {
                LOCK(pwalletMain->cs_wallet);
                if (nHeight > pwalletMain->nLastFilteredHeight)
                {
                    pwalletMain->nLastFilteredHeight = nHeight;
                    CWalletDB walletdb(pwalletMain->strWalletFile);
                    walletdb.WriteLastFilteredHeight(nHeight);
                };
            }

            vPendingFilteredChunks.erase(it);
            break;
        };

        // -- when in NS_GET_FILTERED_BLOCKS mode,
        //    only update pwalletMain->nLastFilteredHeight at end of chunk
        //    otherwise an out of order merkle block (checkpoint) will break continuity
        if (vPendingFilteredChunks.size() == 0
            && pwalletMain->nLastFilteredHeight >= nBestHeight)
        {
            ChangeNodeState(NS_READY);
        };
    } else
    {
        {
            LOCK(pwalletMain->cs_wallet);
            if (nHeight > pwalletMain->nLastFilteredHeight)
            {
                pwalletMain->nLastFilteredHeight = nHeight;
                if (nHeight % 5 == 0) // save height to db every 5 blocks
                {
                    CWalletDB walletdb(pwalletMain->strWalletFile);
                    walletdb.WriteLastFilteredHeight(nHeight);
                };
            };
        }
    };


    uint256 merkleroot = merkleBlock.txn.ExtractMatches(merkleBlock.vMatch);

    if (merkleroot == 0
        || merkleroot != pBlockThinIndex->hashMerkleRoot)
    {
        LogPrintf("Error: merkleBlock - merkleroot mismatch!, %s\n", merkleroot.ToString().c_str());
        if (fAlloc) delete pBlockThinIndex;
        return false;
    };

    if (fAlloc)
        delete pBlockThinIndex;

    if (fDebugChain)
        LogPrintf("merkleBlock: merkleroot %s, no. txn hashes %u\n", merkleroot.ToString().c_str(), merkleBlock.vMatch.size());

    if (merkleBlock.vMatch.size() < 1)
        return true;

    merkleBlock.nProcessed = 0;
    merkleBlock.nTimeRecv = GetTime();

    if (fDebugChain)
        LogPrintf("Added incoming merkle block.\n");


    // -- check txns in mempool
    //    txns recieved in mempool will not be resent after the merkleblock
    CTransaction txMp;
    for (uint32_t i = 0; i < merkleBlock.vMatch.size(); ++i)
    {
        if (!mempool.lookup(merkleBlock.vMatch[i], txMp))
            continue;

        if (fDebugChain)
            LogPrintf("Found txn match in mempool.\n");

        BOOST_FOREACH(CWallet* pwallet, setpwalletRegistered)
            pwallet->AddToWalletIfInvolvingMe(txMp, merkleBlock.vMatch[i], (void*)&hashBlock, true);

        mempool.remove(txMp);
        merkleBlock.nProcessed++;
        break;
    };

    if (pvTxns)
    {
        for (uint32_t i = 0; i < pvTxns->size(); ++i)
        {
            CTransaction &txMp = (*pvTxns)[i];
            uint256 txnhash = txMp.GetHash();
            BOOST_FOREACH(CWallet* pwallet, setpwalletRegistered)
                pwallet->AddToWalletIfInvolvingMe(txMp, txnhash, (void*)&hashBlock, true);

            /*
            // necessary?
            for (uint32_t i = 0; i < merkleBlock.vMatch.size(); ++i)
                if (merkleBlock.vMatch[i] == txnhash)
                    merkleBlock.nProcessed++;
            */
        };
    } else
    if (merkleBlock.nProcessed < merkleBlock.vMatch.size())
        vIncomingMerkleBlocks.push_back(merkleBlock);

    //for (uint32_t i = 0; i < merkleBlock.vMatch.size(); ++i)
    //    LogPrintf("vMatch %d %s\n", i, merkleBlock.vMatch[i].ToString().c_str());
    return 0;
};

/*
void static ThreadCloseSocket(void* parg)
{
    // -- Hack: give socket some time to send buffer before closing

    CNode* pcloseNode = (CNode*)parg;

    MilliSleep(256);

    // -- loop incase pcloseNode is gone by now
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes)
    {
        if (pnode != pcloseNode)
            continue;

        pnode->fDisconnect = true;
        break;
    };
}
*/
static bool SetNodeType(CNode* pfrom, int nTypeInd)
{
    if (fDebugNet)
        LogPrintf("SetNodeType %d\n", nTypeInd);

    if (nTypeInd < NT_FULL
        || nTypeInd >= NT_UNKNOWN)
    {
        LogPrintf("Rejected peer %s: Using unknown node type %d.\n", pfrom->addr.ToString().c_str(), nTypeInd);
        pfrom->fDisconnect = true;
        return false;
    };

    if (nTypeInd != NT_FULL
        && !(nLocalServices & THIN_SUPPORT))
    {
        LogPrintf("Rejected peer %s: This node does not support thin peers.\n", pfrom->addr.ToString().c_str());
        pfrom->fDisconnect = true;
        return false;
    };

    pfrom->nTypeInd = nTypeInd;

    int nMax = nMaxThinPeers;
    // -- check available thin slots
    if (pfrom->nTypeInd != NT_FULL)
    {
        LOCK(cs_vNodes);
        BOOST_FOREACH(CNode* pnode, vNodes)
        {
            if (pnode->nTypeInd != NT_FULL)
                nMax--;

            if (nMax < 1)
            {
                LogPrintf("Rejected peer %s: Max thin peers (%d) reached.\n", pfrom->addr.ToString().c_str(), nMaxThinPeers);

                pfrom->PushMessage("reject", _("conn"), (unsigned char)REJ_MAX_THIN_PEERS, _("max"));
                pfrom->TryFlushSend(); // -- SocketSendData could be called by EndMessage(), try again in case it was not

                //if (!NewThread(ThreadCloseSocket, pfrom))
                pfrom->fDisconnect = true;
                return false;
            };
        };
    };


    switch (pfrom->nTypeInd)
    {
        case NT_THIN:
            pfrom->fRelayTxes = false; // set to true after we get the first filter* message
            break;

        case NT_FULL:
        default:
            pfrom->fRelayTxes = true;
            break;
    };

    return true;
}


bool static ProcessMessage(CNode* pfrom, string strCommand, CDataStream& vRecv, int64_t nTimeReceived)
{
    RandAddSeedPerfmon();

    if (fDebug)
        LogPrintf("received: %s (%u bytes)\n", strCommand.c_str(), vRecv.size());

    if (mapArgs.count("-dropmessagestest") && GetRand(atoi(mapArgs["-dropmessagestest"])) == 0)
    {
        LogPrintf("dropmessagestest DROPPING RECV MESSAGE\n");
        return true;
    };

    if (strCommand == "version")
    {
        // Each connection can only send one version message
        if (pfrom->nVersion != 0)
        {
            pfrom->Misbehaving(1);
            return false;
        };

        int64_t nTime;
        CAddress addrMe;
        CAddress addrFrom;
        uint64_t nNonce = 1;
        vRecv >> pfrom->nVersion >> pfrom->nServices >> nTime >> addrMe;

        if (pfrom->nVersion < MIN_PEER_PROTO_VERSION)
        {
            // disconnect from peers older than this proto version
            LogPrintf("Peer %s using obsolete version %i; disconnecting.\n", pfrom->addr.ToString().c_str(), pfrom->nVersion);
            pfrom->PushMessage("reject", strCommand, REJECT_OBSOLETE, strprintf("node < %d", MIN_PEER_PROTO_VERSION));
            pfrom->fDisconnect = true;
            return false;
        };

        if (nNodeMode != NT_FULL
            && !(pfrom->nServices & THIN_SUPPORT))
        {
            LogPrintf("Peer %s does not support thin clients.\n", pfrom->addr.ToString().c_str());

            pfrom->PushMessage("reject", _("conn"), (unsigned char) REJ_NEED_THIN_SUPPORT, _("thin"));
            pfrom->TryFlushSend();      // SocketSendData could be called by EndMessage(), try again in case it was not

            // -- seems like closesocket (on linux at least) doesn't always wait for data to be sent before destroying the connection
            //if (!NewThread(ThreadCloseSocket, pfrom))
            pfrom->fDisconnect = true;

            return false;
        }

        if (!vRecv.empty())
            vRecv >> addrFrom >> nNonce;
        if (!vRecv.empty())
            vRecv >> pfrom->strSubVer;
        if (!vRecv.empty())
            vRecv >> pfrom->nChainHeight;

        // -- peers should inform node of their mode (FULL/THIN)
        if (!vRecv.empty())
        {
            int nNodeType;

            vRecv >> nNodeType;
            if (!SetNodeType(pfrom, nNodeType))
                return false;
        } else
        {
            pfrom->fRelayTxes = true;
        };

        if (pfrom->fInbound && addrMe.IsRoutable())
        {
            pfrom->addrLocal = addrMe;
            SeenLocal(addrMe);
        };

        // Disconnect if we connected to ourself
        if (nNonce == nLocalHostNonce && nNonce > 1)
        {
            LogPrintf("Connected to self at %s, disconnecting\n", pfrom->addr.ToString().c_str());
            pfrom->fDisconnect = true;
            return true;
        };

        // record my external IP reported by peer
        if (addrFrom.IsRoutable() && addrMe.IsRoutable())
        {
            addrSeenByPeer = addrMe;
            AddLocal(addrSeenByPeer, LOCAL_BIND);
        };
        
        // Be shy and don't send version until we hear
        if (pfrom->fInbound)
            pfrom->PushVersion();

        pfrom->fClient = !(pfrom->nServices & NODE_NETWORK);
        
        int64_t nTimeOffset = nTime - GetTime();
            pfrom->nTimeOffset = nTimeOffset;
        if (GetBoolArg("-synctime", true))
            AddTimeData(pfrom->addr, nTime);
        
        
        
        // Change version
        pfrom->PushMessage("verack");
        pfrom->ssSend.SetVersion(min(pfrom->nVersion, PROTOCOL_VERSION));

        if (!pfrom->fInbound)
        {
            // Advertise our address
            if (!fNoListen && !IsInitialBlockDownload())
            {
                CAddress addr = GetLocalAddress(&pfrom->addr);
                if (addr.IsRoutable())
                    pfrom->PushAddress(addr);
            };

            // Get recent addresses
            if (pfrom->fOneShot || pfrom->nVersion >= CADDR_TIME_VERSION || addrman.size() < 1000)
            {
                pfrom->PushMessage("getaddr");
                pfrom->fGetAddr = true;
            };
            addrman.Good(pfrom->addr);
        } else
        {
            if (((CNetAddr)pfrom->addr) == (CNetAddr)addrFrom)
            {
                addrman.Add(addrFrom, addrFrom);
                addrman.Good(addrFrom);
            };
        };

        // Ask the first connected node for block updates
        static int nAskedForBlocks = 0;

        if (nNodeMode == NT_FULL)
        {
            if (pfrom->nTypeInd == NT_FULL
                && !pfrom->fClient && !pfrom->fOneShot
                && (pfrom->nChainHeight > (nBestHeight - 144))
                && (nAskedForBlocks < 1 || vNodes.size() <= 1))
            {
                nAskedForBlocks++;

                pfrom->PushGetBlocks(pindexBest, uint256(0));
            };
        } else
        {
            // -- TODO it should be able to request headers from thin clients, but providing headers should be optional for a thin client
            if (pfrom->nTypeInd == NT_FULL
                && !pfrom->fClient && !pfrom->fOneShot)
            {
                if ((pfrom->nChainHeight > (nBestHeight - 144))
                    && (nAskedForBlocks < 1 || vNodes.size() <= 1))
                {
                    nAskedForBlocks++;
                    ChangeNodeState(NS_GET_HEADERS);
                    pfrom->PushGetHeaders(pindexBestHeader, uint256(0));
                } else
                if (nNodeState == NS_STARTUP
                    && nAskedForBlocks < 1)
                {
                    ChangeNodeState(NS_GET_FILTERED_BLOCKS);
                }
            };
        };

        // Relay alerts
        {
            LOCK(cs_mapAlerts);
            BOOST_FOREACH(PAIRTYPE(const uint256, CAlert)& item, mapAlerts)
                item.second.RelayTo(pfrom);
        }

        // Relay sync-checkpoint
        {
            LOCK(Checkpoints::cs_hashSyncCheckpoint);
            if (!Checkpoints::checkpointMessage.IsNull())
                Checkpoints::checkpointMessage.RelayTo(pfrom);
        }

        pfrom->fSuccessfullyConnected = true;

        LogPrintf("receive version message: version %d, blocks=%d, us=%s, them=%s, peer=%s, type=%s\n", pfrom->nVersion, pfrom->nChainHeight, addrMe.ToString().c_str(), addrFrom.ToString().c_str(), pfrom->addr.ToString().c_str(), GetNodeModeName(pfrom->nTypeInd));

        cPeerBlockCounts.input(pfrom->nChainHeight);

        // ppcoin: ask for pending sync-checkpoint if any
        if (!IsInitialBlockDownload()
            || nNodeState == NS_READY)
            Checkpoints::AskForPendingSyncCheckpoint(pfrom);
    } else
    if (pfrom->nVersion == 0)
    {
        // Must have a version message before anything else
        pfrom->Misbehaving(1);
        return false;
    } else
    if (strCommand == "verack")
    {
        pfrom->SetRecvVersion(min(pfrom->nVersion, PROTOCOL_VERSION));

        // -- connection is most probably accepted when receive verack

        if (nNodeMode != NT_FULL
            && pwalletMain->pBloomFilter)
        {
            LOCK(pwalletMain->cs_wallet);
            pfrom->PushMessage("filterload", *pwalletMain->pBloomFilter);
        };
    } else
    if (strCommand == "addr")
    {
        vector<CAddress> vAddr;
        vRecv >> vAddr;

        // Don't want addr from older versions unless seeding
        if (pfrom->nVersion < CADDR_TIME_VERSION && addrman.size() > 1000)
            return true;

        if (vAddr.size() > 1000)
        {
            pfrom->Misbehaving(20);
            return error("message addr size() = %u", vAddr.size());
        };

        // Store the new addresses
        vector<CAddress> vAddrOk;
        int64_t nNow = GetAdjustedTime();
        int64_t nSince = nNow - 10 * 60;

        BOOST_FOREACH(CAddress& addr, vAddr)
        {
            boost::this_thread::interruption_point();
            if (addr.nTime <= 100000000 || addr.nTime > nNow + 10 * 60)
                addr.nTime = nNow - 5 * 24 * 60 * 60;
            pfrom->AddAddressKnown(addr);
            bool fReachable = IsReachable(addr);
            if (addr.nTime > nSince && !pfrom->fGetAddr && vAddr.size() <= 10 && addr.IsRoutable())
            {
                // Relay to a limited number of other nodes
                {
                    LOCK(cs_vNodes);
                    // Use deterministic randomness to send to the same nodes for 24 hours
                    // at a time so the setAddrKnowns of the chosen nodes prevent repeats
                    static uint256 hashSalt;
                    if (hashSalt == 0)
                        hashSalt = GetRandHash();
                    uint64_t hashAddr = addr.GetHash();
                    uint256 hashRand = hashSalt ^ (hashAddr<<32) ^ ((GetTime()+hashAddr)/(24*60*60));
                    hashRand = Hash(BEGIN(hashRand), END(hashRand));
                    multimap<uint256, CNode*> mapMix;
                    BOOST_FOREACH(CNode* pnode, vNodes)
                    {
                        if (pnode->nVersion < CADDR_TIME_VERSION)
                            continue;
                        unsigned int nPointer;
                        memcpy(&nPointer, &pnode, sizeof(nPointer));
                        uint256 hashKey = hashRand ^ nPointer;
                        hashKey = Hash(BEGIN(hashKey), END(hashKey));
                        mapMix.insert(make_pair(hashKey, pnode));
                    };

                    int nRelayNodes = fReachable ? 2 : 1; // limited relaying of addresses outside our network(s)
                    for (multimap<uint256, CNode*>::iterator mi = mapMix.begin(); mi != mapMix.end() && nRelayNodes-- > 0; ++mi)
                        ((*mi).second)->PushAddress(addr);
                }
            };
            // Do not store addresses outside our network
            if (fReachable)
                vAddrOk.push_back(addr);
        };

        addrman.Add(vAddrOk, pfrom->addr, 2 * 60 * 60);
        if (vAddr.size() < 1000)
            pfrom->fGetAddr = false;
        if (pfrom->fOneShot)
            pfrom->fDisconnect = true;
    } else
    if (strCommand == "inv")
    {
        vector<CInv> vInv;
        vRecv >> vInv;

        if (vInv.size() > MAX_INV_SZ)
        {
            pfrom->Misbehaving(20);
            return error("message inv size() = %u", vInv.size());
        };
        
        LOCK(cs_main);

        // find last block in inv vector
        unsigned int nLastBlock = (unsigned int)(-1);
        for (unsigned int nInv = 0; nInv < vInv.size(); nInv++)
        {
            if (vInv[vInv.size() - 1 - nInv].type == MSG_BLOCK)
            {
                nLastBlock = vInv.size() - 1 - nInv;
                break;
            };
        };


        if (nNodeMode == NT_FULL)
        {
            CTxDB txdb("r");
            for (unsigned int nInv = 0; nInv < vInv.size(); nInv++)
            {
                const CInv &inv = vInv[nInv];

                boost::this_thread::interruption_point();

                // -- don't mark as known if block from thin peer, need to send merkleblock later if accepted
                if (pfrom->nTypeInd != NT_THIN
                    || inv.type != MSG_BLOCK)
                    pfrom->AddInventoryKnown(inv);

                bool fAlreadyHave = AlreadyHave(txdb, inv);
                if (fDebug)
                    LogPrintf("  got inventory: %s  %s\n", inv.ToString().c_str(), fAlreadyHave ? "have" : "new");

                if (!fAlreadyHave)
                {
                    pfrom->AskFor(inv);
                } else
                if (inv.type == MSG_BLOCK && mapOrphanBlocks.count(inv.hash))
                {
                    pfrom->PushGetBlocks(pindexBest, GetOrphanRoot(mapOrphanBlocks[inv.hash]));
                } else
                if (nInv == nLastBlock)
                {
                    // In case we are on a very long side-chain, it is possible that we already have
                    // the last block in an inv bundle sent in response to getblocks. Try to detect
                    // this situation and push another getblocks to continue.
                    pfrom->PushGetBlocks(mapBlockIndex[inv.hash], uint256(0));
                    if (fDebug)
                        LogPrintf("force request: %s\n", inv.ToString().c_str());
                };

                // Track requests for our stuff
                Inventory(inv.hash);
            };
        } else
        {
            CTxDB txdb("r");
            for (unsigned int nInv = 0; nInv < vInv.size(); nInv++)
            {
                CInv &inv = vInv[nInv];

                boost::this_thread::interruption_point();
                pfrom->AddInventoryKnown(inv);

                bool fAlreadyHave = AlreadyHaveThin(txdb, inv);
                if (fDebug)
                    LogPrintf("  got inventory: %s  %s\n", inv.ToString().c_str(), fAlreadyHave ? "have" : "new");

                if (nNodeMode != NT_FULL
                    && inv.type == MSG_BLOCK)
                    inv.type = MSG_FILTERED_BLOCK;

                if (!fAlreadyHave)
                {
                    pfrom->AskFor(inv);
                } else
                if (inv.type == MSG_BLOCK
                    && mapOrphanBlockThins.count(inv.hash))
                {
                    pfrom->PushGetBlocks(pindexBestHeader, GetOrphanHeaderRoot(mapOrphanBlockThins[inv.hash]));
                } else
                if (nInv == nLastBlock)
                {
                    // In case we are on a very long side-chain, it is possible that we already have
                    // the last block in an inv bundle sent in response to getblocks. Try to detect
                    // this situation and push another getblocks to continue.
                    if (mapBlockThinIndex.count(inv.hash))
                    {
                        pfrom->PushGetBlocks(mapBlockThinIndex[inv.hash], uint256(0));
                    } else
                    if (!fThinFullIndex)
                    {
                        pfrom->PushGetBlocks(inv.hash, uint256(0));
                    };

                    if (fDebug)
                        LogPrintf("force request: %s\n", inv.ToString().c_str());
                };

                // Track requests for our stuff
                Inventory(inv.hash);
            };
        };
    } else
    if (strCommand == "getdata")
    {
        vector<CInv> vInv;
        vRecv >> vInv;
        if (vInv.size() > MAX_INV_SZ)
        {
            pfrom->Misbehaving(20);
            return error("message getdata size() = %u", vInv.size());
        };

        if (fDebugNet || (vInv.size() != 1))
            LogPrintf("received getdata (%u invsz)\n", vInv.size());

        pfrom->vRecvGetData.insert(pfrom->vRecvGetData.end(), vInv.begin(), vInv.end());
        ProcessGetData(pfrom);

    } else
    if (strCommand == "getblocks")
    {
        if (nNodeMode != NT_FULL)
        {
            LogPrintf("[rem] strCommand getblocks\n");
            return 0;
        };

        CBlockLocator locator;
        uint256 hashStop;
        vRecv >> locator >> hashStop;
        
        LOCK(cs_main);

        // Find the last block the caller has in the main chain
        CBlockIndex* pindex = locator.GetBlockIndex();

        // Send the rest of the chain
        if (pindex)
            pindex = pindex->pnext;
        int nLimit = 500;
        LogPrintf("getblocks %d to %s limit %d\n", (pindex ? pindex->nHeight : -1), hashStop.ToString().substr(0,20).c_str(), nLimit);
        for (; pindex; pindex = pindex->pnext)
        {
            if (pindex->GetBlockHash() == hashStop)
            {
                LogPrintf("  getblocks stopping at %d %s\n", pindex->nHeight, pindex->GetBlockHash().ToString().substr(0,20).c_str());
                // ppcoin: tell downloading node about the latest block if it's
                // without risk being rejected due to stake connection check
                if (hashStop != hashBestChain && pindex->GetBlockTime() + nStakeMinAge > pindexBest->GetBlockTime())
                    pfrom->PushInventory(CInv(MSG_BLOCK, hashBestChain));
                break;
            };

            pfrom->PushInventory(CInv(MSG_BLOCK, pindex->GetBlockHash()));
            if (--nLimit <= 0)
            {
                // When this block is requested, we'll send an inv that'll make them
                // getblocks the next batch of inventory.
                LogPrintf("  getblocks stopping at limit %d %s\n", pindex->nHeight, pindex->GetBlockHash().ToString().substr(0,20).c_str());
                pfrom->hashContinue = pindex->GetBlockHash();
                break;
            };
        };
    } else
    if (strCommand == "checkpoint")
    {
        CSyncCheckpoint checkpoint;
        vRecv >> checkpoint;
        
        LOCK(cs_main);

        if ((nNodeMode == NT_FULL
            && checkpoint.ProcessSyncCheckpoint(pfrom))
            || (nNodeMode != NT_FULL
                && checkpoint.ProcessSyncCheckpointHeaders(pfrom)))
        {
            // Relay
            pfrom->hashCheckpointKnown = checkpoint.hashCheckpoint;
            LOCK(cs_vNodes);
            BOOST_FOREACH(CNode* pnode, vNodes)
                checkpoint.RelayTo(pnode);
        };
    } else
    if (strCommand == "getheaders")
    {
        if (nNodeMode != NT_FULL)
        {
            LogPrintf("[rem] strCommand getheaders\n");
            return false;
        };

        // -- full nodes don't request headers
        if (pfrom->nTypeInd == NT_FULL
            && !SetNodeType(pfrom, NT_THIN))
            return false;


        CBlockLocator locator;
        uint256 hashStop;
        vRecv >> locator >> hashStop;
        
        LOCK(cs_main);

        CBlockIndex* pindex = NULL;
        if (locator.IsNull())
        {
            // If locator is null, return the hashStop block
            map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hashStop);
            if (mi == mapBlockIndex.end())
                return true;
            pindex = (*mi).second;
        } else
        {
            // Find the last block the caller has in the main chain
            pindex = locator.GetBlockIndex();
            if (pindex)
                pindex = pindex->pnext;
        };

        vector<CBlockThin> vHeaders;
        int nLimit = MAX_GETHEADERS_SZ;
        LogPrintf("getheaders %d to %s\n", (pindex ? pindex->nHeight : -1), hashStop.ToString().substr(0,20).c_str());
        for (; pindex; pindex = pindex->pnext)
        {
            CBlockThin blockThin = pindex->GetBlockThinOnly();
            vHeaders.push_back(blockThin);
            if (--nLimit <= 0 || pindex->GetBlockHash() == hashStop)
                break;
        };
        pfrom->PushMessage("headers", vHeaders);
    } else
    if (strCommand == "tx")
    {
        vector<uint256> vWorkQueue;
        vector<uint256> vEraseQueue;
        
        CTransaction tx;
        vRecv >> tx;
        
        LOCK(cs_main);
        
        CTxDB txdb("r");

        if (nNodeMode == NT_FULL)
        {
            CInv inv(MSG_TX, tx.GetHash());
            pfrom->AddInventoryKnown(inv);

            bool fMissingInputs = false;
            if (AcceptToMemoryPool(mempool, tx, txdb, &fMissingInputs))
            {
                SyncWithWallets(tx, NULL, true);
                RelayTransaction(tx, inv.hash);
                mapAlreadyAskedFor.erase(inv);
                vWorkQueue.push_back(inv.hash);
                vEraseQueue.push_back(inv.hash);

                // Recursively process any orphan transactions that depended on this one
                for (unsigned int i = 0; i < vWorkQueue.size(); i++)
                {
                    uint256 hashPrev = vWorkQueue[i];
                    for (set<uint256>::iterator mi = mapOrphanTransactionsByPrev[hashPrev].begin();
                         mi != mapOrphanTransactionsByPrev[hashPrev].end();
                         ++mi)
                    {
                        const uint256& orphanTxHash = *mi;
                        CTransaction& orphanTx = mapOrphanTransactions[orphanTxHash];
                        bool fMissingInputs2 = false;

                        if (AcceptToMemoryPool(mempool, orphanTx, txdb, &fMissingInputs2))
                        {
                            LogPrintf("   accepted orphan tx %s\n", orphanTxHash.ToString().substr(0,10).c_str());
                            SyncWithWallets(tx, NULL, true);
                            RelayTransaction(orphanTx, orphanTxHash);
                            mapAlreadyAskedFor.erase(CInv(MSG_TX, orphanTxHash));
                            vWorkQueue.push_back(orphanTxHash);
                            vEraseQueue.push_back(orphanTxHash);
                        } else
                        if (!fMissingInputs2)
                        {
                            // invalid orphan
                            vEraseQueue.push_back(orphanTxHash);
                            LogPrintf("   removed invalid orphan tx %s\n", orphanTxHash.ToString().substr(0,10).c_str());
                        };
                    };
                };

                BOOST_FOREACH(uint256 hash, vEraseQueue)
                    EraseOrphanTx(hash);
            } else
            if (fMissingInputs)
            {
                AddOrphanTx(tx);

                // DoS prevention: do not allow mapOrphanTransactions to grow unbounded
                unsigned int nEvicted = LimitOrphanTxSize(MAX_ORPHAN_TRANSACTIONS);
                if (nEvicted > 0)
                    LogPrintf("mapOrphan overflow, removed %u tx\n", nEvicted);
            };
        } else
        {
            uint256 txHash = tx.GetHash();

            CInv inv(MSG_TX, txHash);
            pfrom->AddInventoryKnown(inv);

            bool fProcessed = false;
            std::vector<CMerkleBlockIncoming>::iterator it;
            for (it = vIncomingMerkleBlocks.begin(); !fProcessed && it < vIncomingMerkleBlocks.end(); ++it)
            {
                for (uint32_t i = 0; i < it->vMatch.size(); ++i)
                {
                    if (it->vMatch[i] != txHash)
                        continue;

                    //LogPrintf("Found match.\n");
                    uint256 blockhash = it->header.GetHash();

                    BOOST_FOREACH(CWallet* pwallet, setpwalletRegistered)
                        pwallet->AddToWalletIfInvolvingMe(tx, txHash, (void*)&blockhash, true);

                    it->nProcessed++;
                    fProcessed = true;
                    if (it->nProcessed == it->vMatch.size())
                        vIncomingMerkleBlocks.erase(it);
                    break;

                    //LogPrintf("vMatch %d %s\n", i, it->vMatch[i].ToString().c_str());
                };
            };

            if (!fProcessed)
            {
                if (fDebugChain)
                    LogPrintf("txn %s not found in merkleblock, adding to mempool.\n", txHash.ToString().c_str());

                // TODO: this is wasteful, test timedout first?

                bool fMissingInputs = false;
                if (AcceptToMemoryPool(mempool, tx, txdb, &fMissingInputs))
                {
                    //SyncWithWallets(tx, NULL, true);
                    
                    bool added = false;
                    uint256 hash = tx.GetHash();
                    BOOST_FOREACH(CWallet* pwallet, setpwalletRegistered)
                    {
                        added = added | pwallet->AddToWalletIfInvolvingMe(tx, hash, NULL, true);
                    };
                    
                    if (added)
                    {
                        RelayTransaction(tx, inv.hash);
                        mapAlreadyAskedFor.erase(inv);
                    } else
                    {
                        // -- not interested in txn if not for this wallet
                        mempool.remove(tx);
                    };
                };
            };
        };

        if (tx.nDoS)
            pfrom->Misbehaving(tx.nDoS);

    } else
    if (strCommand == "mblk")
    {
        // multiblock
        if (nNodeMode != NT_FULL)
        {
            LogPrintf("[rem] strCommand mblk, !NT_FULL\n");
            return 0;
        };
        
        std::vector<CBlock> vBlocks;
        vRecv >> vBlocks; // TODO: use a plain byte buffer?
        uint32_t nBlocks = vBlocks.size();
        
        
        if (nBlocks > MAX_MULTI_BLOCK_ELEMENTS)
        {
            LogPrintf("Warning: Peer sent too many blocks in mblk %u.\n", vRecv.size());
            pfrom->Misbehaving(10);
            return false;
        };
        
        LogPrintf("Received mblk %d\n", nBlocks);
        
        for (uint32_t i = 0; i < nBlocks; ++i)
        {
            CBlock &block = vBlocks[i];
            
            uint256 hashBlock = block.GetHash();
            //LogPrintf("received block %s\n", hashBlock.ToString().substr(0,20).c_str());
            // block.print();
            
            CInv inv(MSG_BLOCK, hashBlock);
            
            {
                LOCK(cs_main);
                // -- if peer is thin, it will want the (merkle) block sent if accepted
                if (pfrom->nTypeInd == NT_FULL)
                    pfrom->AddInventoryKnown(inv);
                
                if (ProcessBlock(pfrom, &block, hashBlock))
                    mapAlreadyAskedFor.erase(inv);

                if (block.nDoS)
                    pfrom->Misbehaving(block.nDoS);
            } // cs_main
            
            if (fSecMsgEnabled)
                SecureMsgScanBlock(block);
        };


    } else
    if (strCommand == "mblkt")
    {
        // multiblock thin
        if (nNodeMode == NT_FULL)
        {
            LogPrintf("[rem] strCommand mblkt, NT_FULL\n");
            return 0;
        };


        std::vector<CMBlkThinElement> vMultiBlockThin;
        vRecv >> vMultiBlockThin; // TODO: use a plain byte buffer?
        uint32_t nBlocks = vMultiBlockThin.size();
        
        LOCK(cs_main);

        if (nBlocks > MAX_MULTI_BLOCK_THIN_ELEMENTS)
        {
            LogPrintf("Warning: Peer sent too many blocks in mblkt %u.\n", vRecv.size());
            pfrom->Misbehaving(10);
            return false;
        };

        LogPrintf("Received mblkt %d\n", nBlocks);

        std::vector<CTransaction> vTxns;
        for (uint32_t i = 0; i < nBlocks; ++i)
        {
            CMerkleBlockIncoming mbi = CMerkleBlockIncoming(vMultiBlockThin[i].merkleBlock);
            vTxns = vMultiBlockThin[i].vtx;
            ProcessMerkleBlock(pfrom, mbi, &vTxns);
        };
    } else
    if (strCommand == "block")
    {
        if (nNodeMode != NT_FULL)
        {
            LogPrintf("[rem] strCommand block, !NT_FULL\n");
            return 0;
        };
        
        CBlock block;
        vRecv >> block;
        uint256 hashBlock = block.GetHash();
        
        LOCK(cs_main);
        
        LogPrintf("Received block %s\n", hashBlock.ToString().substr(0,20).c_str());
        // block.print();

        CInv inv(MSG_BLOCK, hashBlock);

        // -- if peer is thin, it will want the (merkle) block sent if accepted
        if (pfrom->nTypeInd == NT_FULL)
            pfrom->AddInventoryKnown(inv);

        if (ProcessBlock(pfrom, &block, hashBlock))
            mapAlreadyAskedFor.erase(inv);

        if (block.nDoS)
            pfrom->Misbehaving(block.nDoS);

        if (fSecMsgEnabled)
            SecureMsgScanBlock(block);
    } else
    if (strCommand == "merkleblock")
    {
        if (nNodeState != NS_READY
            && nNodeState != NS_GET_FILTERED_BLOCKS)
        {
            if (fDebug)
                LogPrintf("Ignoring merkleblock received in mode %s.\n", GetNodeStateName(nNodeState));
            return false;
        };

        CMerkleBlockIncoming merkleBlock;
        vRecv >> merkleBlock;

        ProcessMerkleBlock(pfrom, merkleBlock, NULL);

    } else
    if (strCommand == "headers")
    {
        if (nNodeMode == NT_FULL)
        {
            LogPrintf("Warning: Peer sent headers to full node.\n");
            pfrom->Misbehaving(10);
            return false;
        };

        vector<CBlockThin> vHeaders;
        vRecv >> vHeaders;
        
        LOCK(cs_main);
        
        if (fDebugChain)
            LogPrintf("Received %u headers\n", vHeaders.size());

        if (vHeaders.size() == 0)
        {
            // -- no headers found, this node must be up to date.
            if (nNodeState == NS_GET_HEADERS)
                ChangeNodeState(NS_GET_FILTERED_BLOCKS);
            return true;
        };

        if (vHeaders.size() > MAX_GETHEADERS_SZ)
        {
            LogPrintf("Warning: Peer sent too many headers %u.\n", vHeaders.size());
            pfrom->Misbehaving(10);
            return false;
        };

        for (std::vector<CBlockThin>::iterator it = vHeaders.begin(); it < vHeaders.end(); ++it)
        {
            if (!fThinFullIndex && pindexRear
                && it->nTime < pindexRear->nTime)
            {
                if (fDebug)
                    LogPrintf("Warning: header %s is before chain index window.\n", it->GetHash().ToString().c_str());
                pfrom->Misbehaving(10);
            } else
            {
                ProcessBlockThin(pfrom, &(*it));
            };
        };

        if (nBestHeight < pfrom->nChainHeight)
        {
            pfrom->PushGetHeaders(pindexBestHeader, uint256(0));
        } else
        {
            if (nNodeState == NS_GET_HEADERS)
                ChangeNodeState(NS_GET_FILTERED_BLOCKS);
        };
    } else
    if (strCommand == "getaddr")
    {
        // Don't return addresses older than nCutOff timestamp
        int64_t nCutOff = GetTime() - (nNodeLifespan * 24 * 60 * 60);
        pfrom->vAddrToSend.clear();
        vector<CAddress> vAddr = addrman.GetAddr();
        BOOST_FOREACH(const CAddress &addr, vAddr)
        {
            if (addr.nTime > nCutOff)
                pfrom->PushAddress(addr);
        };
    } else
    if (strCommand == "mempool")
    {
        LOCK2(cs_main, pfrom->cs_filter);

        std::vector<uint256> vtxid;
        mempool.queryHashes(vtxid);
        vector<CInv> vInv;

        BOOST_FOREACH(uint256& hash, vtxid)
        {
            CInv inv(MSG_TX, hash);
            CTransaction tx;
            bool fInMemPool = mempool.lookup(hash, tx);
            if (!fInMemPool)
                continue; // another thread removed since queryHashes, maybe...
            
            // -- node requirements are packed into the top 32 bits of nServices
            if (pfrom->pfilter
                && !pfrom->pfilter->IsRelevantAndUpdate(tx))
                continue;
            
            vInv.push_back(inv);
            
            if (vInv.size() == MAX_INV_SZ)
            {
                pfrom->PushMessage("inv", vInv);
                vInv.clear();
            };
        };

        if (vInv.size() > 0)
            pfrom->PushMessage("inv", vInv);
    } else
    if (strCommand == "ping")
    {
        uint64_t nonce = 0;
        vRecv >> nonce;
        // Echo the message back with the nonce. This allows for two useful features:
        //
        // 1) A remote node can quickly check if the connection is operational
        // 2) Remote nodes can measure the latency of the network thread. If this node
        //    is overloaded it won't respond to pings quickly and the remote node can
        //    avoid sending us more work, like chain download requests.
        //
        // The nonce stops the remote getting confused between different pings: without
        // it, if the remote node sends a ping once per second and this node takes 5
        // seconds to respond to each, the 5th ping the remote sends would appear to
        // return very quickly.
        pfrom->PushMessage("pong", nonce);
        
        // -- keep the network height updated, needed for thin mode
        int nPeerHeight;
        vRecv >> nPeerHeight;
        
        LOCK(cs_main);
        cPeerBlockCounts.input(nPeerHeight);
        pfrom->nChainHeight = nPeerHeight;
        
        if (fDebugNet)
            LogPrintf("peer %s chain height %d\n", pfrom->addr.ToString().c_str(), nPeerHeight);
    } else
    if (strCommand == "pong")
    {
        int64_t pingUsecEnd = nTimeReceived;
        uint64_t nonce = 0;
        size_t nAvail = vRecv.in_avail();
        bool bPingFinished = false;
        std::string sProblem;

        if (nAvail >= sizeof(nonce))
        {
            vRecv >> nonce;

            // Only process pong message if there is an outstanding ping (old ping without nonce should never pong)
            if (pfrom->nPingNonceSent != 0)
            {
                if (nonce == pfrom->nPingNonceSent)
                {
                    // Matching pong received, this ping is no longer outstanding
                    bPingFinished = true;
                    int64_t pingUsecTime = pingUsecEnd - pfrom->nPingUsecStart;
                    if (pingUsecTime > 0)
                    {
                        // Successful ping time measurement, replace previous
                        pfrom->nPingUsecTime = pingUsecTime;
                    } else
                    {
                        // This should never happen
                        sProblem = "Timing mishap";
                    };
                } else
                {
                    // Nonce mismatches are normal when pings are overlapping
                    sProblem = "Nonce mismatch";
                    if (nonce == 0)
                    {
                        // This is most likely a bug in another implementation somewhere, cancel this ping
                        bPingFinished = true;
                        sProblem = "Nonce zero";
                    };
                };
            } else
            {
                sProblem = "Unsolicited pong without ping";
            };
        } else
        {
            // This is most likely a bug in another implementation somewhere, cancel this ping
            bPingFinished = true;
            sProblem = "Short payload";
        };

        if (!(sProblem.empty()))
        {
            LogPrintf("pong %s %s: %s, %x expected, %x received, %u bytes\n"
                , pfrom->addr.ToString().c_str()
                , pfrom->strSubVer.c_str()
                , sProblem.c_str()
                , pfrom->nPingNonceSent
                , nonce
                , nAvail);
        };

        if (bPingFinished)
        {
            pfrom->nPingNonceSent = 0;
        };
    } else
    if (strCommand == "alert")
    {
        CAlert alert;
        vRecv >> alert;

        uint256 alertHash = alert.GetHash();
        if (pfrom->setKnown.count(alertHash) == 0)
        {
            if (alert.ProcessAlert())
            {
                // Relay
                pfrom->setKnown.insert(alertHash);
                {
                    LOCK(cs_vNodes);
                    BOOST_FOREACH(CNode* pnode, vNodes)
                        alert.RelayTo(pnode);
                }
            } else
            {
                // Small DoS penalty so peers that send us lots of
                // duplicate/expired/invalid-signature/whatever alerts
                // eventually get banned.
                // This isn't a Misbehaving(100) (immediate ban) because the
                // peer might be an older or different implementation with
                // a different signature key, etc.
                pfrom->Misbehaving(10);
            };
        };
    } else
    if (strCommand == "filterload")
    {
        // -- full nodes won't use filters, if a node does it is NT_THIN
        if (pfrom->nTypeInd == NT_FULL
            && !SetNodeType(pfrom, NT_THIN))
            return false;
        
        CBloomFilter filter;
        vRecv >> filter;
        
        if (!filter.IsWithinSizeConstraints())
        {
            // There is no excuse for sending a too-large filter
            pfrom->Misbehaving(100);
        } else
        if ((filter.nFlags & BLOOM_ACCEPT_STEALTH)
            && !(nLocalServices & THIN_STEALTH))
        {
            // -- peer has requested that node forwards all stealth txns, node does not support this

            if (fDebug)
                LogPrintf("Warning: Peer %s requested merklebloks include all stealth txns, function disabled.\n", pfrom->addr.ToString().c_str());
            filter.nFlags &= ~(BLOOM_ACCEPT_STEALTH);
        };
        
        {
            LOCK(pfrom->cs_filter);
            delete pfrom->pfilter;
            pfrom->pfilter = new CBloomFilter(filter);
            pfrom->pfilter->UpdateEmptyFull();
        };
        
        if (fDebug)
            LogPrintf("Loaded bloom filter of size %u for peer %s.\n", pfrom->pfilter->vData.size(), pfrom->addr.ToString().c_str());
        
        pfrom->fRelayTxes = true;
    } else
    if (strCommand == "filteradd")
    {
        if (pfrom->nTypeInd == NT_FULL
            && !SetNodeType(pfrom, NT_THIN))
            return false;
        
        vector<unsigned char> vData;
        vRecv >> vData;
        
        // Nodes must NEVER send a data item > 520 bytes (the max size for a script data object,
        // and thus, the maximum size any matched object can have) in a filteradd message
        if (vData.size() > MAX_SCRIPT_ELEMENT_SIZE)
        {
            pfrom->Misbehaving(100);
        } else
        {
            LOCK(pfrom->cs_filter);
            if (pfrom->pfilter)
            {
                if (!pfrom->pfilter->contains(vData))
                {
                    pfrom->pfilter->insert(vData);
                    pfrom->pfilter->UpdateEmptyFull();
                    if (fDebug)
                        LogPrintf("Added data to bloom filter of peer %s, is full %d.\n", pfrom->addr.ToString().c_str(), pfrom->pfilter->IsFull());
                };
            } else
            {
                pfrom->Misbehaving(100); // peer must send filterload first
            };
        };
    } else
    if (strCommand == "filterclear")
    {
        LOCK(pfrom->cs_filter);
        delete pfrom->pfilter;
        pfrom->pfilter = NULL;
        //pfrom->pfilter = new CBloomFilter();
        pfrom->fRelayTxes = true;
    } else
    if (strCommand == "reject")
    {
        string strMsg;
        string strReason;
        unsigned char ccode;
        vRecv >> strMsg >> ccode >> strReason;
        
        if (strMsg == "conn")
        {
            char *reason;
            switch (ccode)
            {
                case REJ_NEED_THIN_SUPPORT:     reason = (char*)"Requires thin client support";             break;
                case REJ_MAX_THIN_PEERS:        reason = (char*)"Peer is connected to max thin peers";      break;
                default:                        reason = (char*)"unknown code";                             break;
            };
            
            LogPrintf("Peer %s rejected connection, code %d, reason: %s.\n", pfrom->addr.ToString().c_str(), ccode, reason);
            pfrom->SoftBan(); // don't allow connecting again for a while
        } else
        if (fDebug)
        {
            ostringstream ss;
            ss << strMsg << " code " << itostr(ccode) << ": " << strReason;

            if (strMsg == "block" || strMsg == "tx")
            {
                uint256 hash;
                vRecv >> hash;
                ss << ": hash " << hash.ToString();
            };

            // Truncate to reasonable length and sanitize before printing:
            string s = ss.str();
            if (s.size() > 111)
                s.erase(111, string::npos);
            LogPrintf("Reject %s\n", SanitizeString(s).c_str());
        };
    
    } else
    {
        if (fSecMsgEnabled)
            SecureMsgReceiveData(pfrom, strCommand, vRecv);

        // Ignore unknown commands for extensibility
    };
    
    
    // Update the last seen time for this node's address
    if (pfrom->fNetworkNode)
    {
        if (strCommand == "version" || strCommand == "addr" || strCommand == "inv" || strCommand == "getdata" || strCommand == "ping")
            AddressCurrentlyConnected(pfrom->addr);
    };

    return true;
}

// requires LOCK(cs_vRecvMsg)
bool ProcessMessages(CNode* pfrom)
{
    //if (fDebug)
    //    LogPrintf("ProcessMessages(%zu messages)\n", pfrom->vRecvMsg.size());
    
    //
    // Message format
    //  (4) message start
    //  (12) command
    //  (4) size
    //  (4) checksum
    //  (x) data
    //
    bool fOk = true;
    
    // continue sending if inv is left over from last round (not enough space in send buffer)
    if (!pfrom->vRecvGetData.empty())
        ProcessGetData(pfrom);
    
    // this maintains the order of responses
    if (!pfrom->vRecvGetData.empty())
        return fOk;

    std::deque<CNetMessage>::iterator it = pfrom->vRecvMsg.begin();
    while (!pfrom->fDisconnect && it != pfrom->vRecvMsg.end())
    {
        // Don't bother if send buffer is too full to respond anyway
        if (pfrom->nSendSize >= SendBufferSize())
            break;
        
        // get next message
        CNetMessage& msg = *it;
        
        //if (fDebug)
        //    LogPrintf("ProcessMessages(message %u msgsz, %zu bytes, complete:%s)\n",
        //            msg.hdr.nMessageSize, msg.vRecv.size(),
        //            msg.complete() ? "Y" : "N");
        
        // end, if an incomplete message is found
        if (!msg.complete())
            break;

        // at this point, any failure means we can delete the current message
        it++;

        // Scan for message start
        if (memcmp(msg.hdr.pchMessageStart, Params().MessageStart(), MESSAGE_START_SIZE) != 0)
        {
            LogPrintf("\n\nPROCESSMESSAGE: INVALID MESSAGESTART\n\n");
            pfrom->Misbehaving(5);
            fOk = false;
            break;
        };

        // Read header
        CMessageHeader& hdr = msg.hdr;
        if (!hdr.IsValid())
        {
            LogPrintf("\n\nPROCESSMESSAGE: ERRORS IN HEADER %s\n\n\n", hdr.GetCommand().c_str());
            pfrom->Misbehaving(5);
            continue;
        };

        string strCommand = hdr.GetCommand();

        // Message size
        unsigned int nMessageSize = hdr.nMessageSize;

        // Checksum
        CDataStream& vRecv = msg.vRecv;
        uint256 hash = Hash(vRecv.begin(), vRecv.begin() + nMessageSize);
        unsigned int nChecksum = 0;
        memcpy(&nChecksum, &hash, sizeof(nChecksum));

        if (nChecksum != hdr.nChecksum)
        {
            LogPrintf("ProcessMessages(%s, %u bytes) : CHECKSUM ERROR nChecksum=%08x hdr.nChecksum=%08x\n",
               strCommand.c_str(), nMessageSize, nChecksum, hdr.nChecksum);
            continue;
        };

        // Process message
        bool fRet = false;
        try
        {
            fRet = ProcessMessage(pfrom, strCommand, vRecv, msg.nTime);
            boost::this_thread::interruption_point();
        } catch (std::ios_base::failure& e)
        {
            if (strstr(e.what(), "end of data"))
            {
                // Allow exceptions from under-length message on vRecv
                LogPrintf("ProcessMessages(%s, %u bytes) : Exception '%s' caught, normally caused by a message being shorter than its stated length\n", strCommand.c_str(), nMessageSize, e.what());
            } else
            if (strstr(e.what(), "size too large"))
            {
                // Allow exceptions from over-long size
                LogPrintf("ProcessMessages(%s, %u bytes) : Exception '%s' caught\n", strCommand.c_str(), nMessageSize, e.what());
            } else
            {
                PrintExceptionContinue(&e, "ProcessMessages()");
            };
        } catch (boost::thread_interrupted)
        {
            throw;
        } catch (std::exception& e)
        {
            PrintExceptionContinue(&e, "ProcessMessages()");
        } catch (...)
        {
            PrintExceptionContinue(NULL, "ProcessMessages()");
        };

        if (!fRet)
            LogPrintf("ProcessMessage(%s, %u bytes) FAILED\n", strCommand.c_str(), nMessageSize);
    };

    // In case the connection got shut down, its receive buffer was wiped
    if (!pfrom->fDisconnect)
        pfrom->vRecvMsg.erase(pfrom->vRecvMsg.begin(), it);

    return fOk;
}


bool SendMessages(CNode* pto, std::vector<CNode*> &vNodesCopy, bool fSendTrickle)
{
    // Don't send anything until we get their version message
    if (pto->nVersion == 0)
        return true;

    //
    // Message: ping
    //
    bool pingSend = false;
    if (pto->fPingQueued)
    {
        // RPC ping request by user
        pingSend = true;
    };

    if (pto->nPingNonceSent == 0 && pto->nPingUsecStart + PING_INTERVAL * 1000000 < GetTimeMicros())
    {
        // Ping automatically sent as a latency probe & keepalive.
        pingSend = true;
    };

    if (pingSend)
    {
        uint64_t nonce = 0;
        while (nonce == 0)
        {
            RAND_bytes((unsigned char*)&nonce, sizeof(nonce));
        };

        pto->fPingQueued = false;
        pto->nPingUsecStart = GetTimeMicros();
        
        pto->nPingNonceSent = nonce;
        pto->PushMessage("ping", nonce, nBestHeight);
    };
    
    TRY_LOCK(cs_main, lockMain); // Acquire cs_main for IsInitialBlockDownload() and CNodeState()
    if (!lockMain)
        return true;
        
    // Resend wallet transactions that haven't gotten in a block yet
    ResendWalletTransactions();

    // Address refresh broadcast
    static int64_t nLastRebroadcast;
    if (!IsInitialBlockDownload() && (GetTime() - nLastRebroadcast > 24 * 60 * 60))
    {
        BOOST_FOREACH(CNode* pnode, vNodesCopy)
        {
            // Periodically clear setAddrKnown to allow refresh broadcasts
            if (nLastRebroadcast)
                pnode->setAddrKnown.clear();

            // Rebroadcast our address
            if (!fNoListen)
            {
                CAddress addr = GetLocalAddress(&pnode->addr);
                if (addr.IsRoutable())
                    pnode->PushAddress(addr);
            };
        };
        nLastRebroadcast = GetTime();
    };

    //
    // Message: addr
    //
    if (fSendTrickle)
    {
        vector<CAddress> vAddr;
        vAddr.reserve(pto->vAddrToSend.size());
        BOOST_FOREACH(const CAddress& addr, pto->vAddrToSend)
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
                };
            };
        };
        pto->vAddrToSend.clear();
        if (!vAddr.empty())
            pto->PushMessage("addr", vAddr);
    };


    //
    // Message: inventory
    //
    std::vector<CInv> vInv;
    std::vector<CInv> vInvWait;

    {
        LOCK2(pwalletMain->cs_wallet, pto->cs_inventory);
        vInv.reserve(pto->vInventoryToSend.size());
        vInvWait.reserve(pto->vInventoryToSend.size());
        BOOST_FOREACH(const CInv& inv, pto->vInventoryToSend)
        {
            if (pto->setInventoryKnown.count(inv))
                continue;

            // trickle out tx inv to protect privacy
            if (inv.type == MSG_TX && !fSendTrickle)
            {
                // 1/4 of tx invs blast to all immediately
                static uint256 hashSalt;
                if (hashSalt == 0)
                    hashSalt = GetRandHash();
                uint256 hashRand = inv.hash ^ hashSalt;
                hashRand = Hash(BEGIN(hashRand), END(hashRand));
                bool fTrickleWait = ((hashRand & 3) != 0);

                // always trickle our own transactions
                if (!fTrickleWait)
                {
                    CWalletTx wtx;
                    if (GetTransaction(inv.hash, wtx))
                        if (wtx.fFromMe)
                            fTrickleWait = true;
                };

                if (fTrickleWait)
                {
                    vInvWait.push_back(inv);
                    continue;
                };
            };

            // returns true if wasn't already contained in the set
            if (pto->setInventoryKnown.insert(inv).second)
            {
                vInv.push_back(inv);
                if (vInv.size() >= 1000)
                {
                    pto->PushMessage("inv", vInv);
                    vInv.clear();
                };
            };
        };
        pto->vInventoryToSend = vInvWait;
    }

    if (!vInv.empty())
        pto->PushMessage("inv", vInv);

    int64_t nTimeNow = GetTime();
    std::vector<CInv> vGetFilteredBlocks;

    if (nNodeState == NS_GET_FILTERED_BLOCKS
        && pto->nTypeInd == NT_FULL)
    {
        uint256 hash;
        CInv inv(MSG_FILTERED_BLOCK, hash);

        if (nHeightFilteredNeeded < 1)
            nHeightFilteredNeeded = 1;

        if (nHeightFilteredNeeded <= nBestHeight
            && vPendingFilteredChunks.size() < 4)
        {
            if (!fThinFullIndex
                && pindexRear
                && nHeightFilteredNeeded < pindexRear->nHeight)
            {
                if (fDebug)
                    LogPrintf("Finding block height %d from db.\n", nHeightFilteredNeeded);

                CDiskBlockThinIndex diskindex;
                uint256 hashPrev;

                if (pindexRear->phashBlock)
                    hashPrev = *pindexRear->phashBlock;
                else
                    hashPrev = pindexRear->GetBlockHash();

                // -- find closest checkpoint
                Checkpoints::MapCheckpoints& checkpoints = (fTestNet ? Checkpoints::mapCheckpointsTestnet : Checkpoints::mapCheckpoints);
                Checkpoints::MapCheckpoints::reverse_iterator rit;

                for (rit = checkpoints.rbegin(); rit != checkpoints.rend(); ++rit)
                {
                    if (rit->first < nHeightFilteredNeeded)
                        break;
                    hashPrev = rit->second;
                };

                if (fDebug
                    && pindexRear->phashBlock
                    && *pindexRear->phashBlock != hashPrev)
                {
                    LogPrintf("Starting from checkpoint %s.\n", hashPrev.ToString().c_str());
                }

                CTxDB txdb("r");
                while (hashPrev != 0)
                {
                    if (!txdb.ReadBlockThinIndex(hashPrev, diskindex))
                    {
                        LogPrintf("NS_GET_FILTERED_BLOCKS Read header %s failed.\n", hashPrev.ToString().c_str());
                        break;
                    };

                    if (diskindex.nHeight <= nHeightFilteredNeeded)
                    {
                        if (fDebug)
                            LogPrintf("Found block height %d, %s - reading forwards.\n", nHeightFilteredNeeded, hashPrev.ToString().c_str());

                        inv.hash = hashPrev;
                        vGetFilteredBlocks.push_back(inv);
                        nHeightFilteredNeeded++;

                        uint256 hashNext = diskindex.hashNext;
                        // -- TODO: cache backwards to avoid reading twice, and detect when in index window

                        for (uint32_t i = 0; i < 127; ++i) // choose a low number to spread the load over more peers
                        {
                            if (hashNext == 0)
                                break;

                            inv.hash = hashNext;
                            vGetFilteredBlocks.push_back(inv);
                            nHeightFilteredNeeded++;

                            if (!txdb.ReadBlockThinIndex(hashNext, diskindex))
                            {
                                LogPrintf("NS_GET_FILTERED_BLOCKS (inner) Read header %s failed.\n", hashNext.ToString().c_str());
                                break;
                            };
                            hashNext = diskindex.hashNext;
                        };
                        break;
                    };

                    hashPrev = diskindex.hashPrev;
                };

            } else
            {
                CBlockThinIndex* pblockindex = FindBlockThinByHeight(nHeightFilteredNeeded);

                if (!pblockindex)
                {
                    LogPrintf("Warning: FindBlockThinByHeight() %d failed.\n", nHeightFilteredNeeded);
                } else
                {
                    for (uint32_t i = 0; i < 128; ++i) // choose a low number to spread the load over more peers
                    {
                        if (pblockindex->phashBlock)
                            inv.hash = *pblockindex->phashBlock;
                        else
                            inv.hash = pblockindex->GetBlockHash();

                        vGetFilteredBlocks.push_back(inv);
                        nHeightFilteredNeeded++;

                        if (!pblockindex->pnext)
                            break;
                        pblockindex = pblockindex->pnext;
                    };
                };
            };

        } else
        if (vPendingFilteredChunks.size() > 0)
        {
            std::vector<CPendingFilteredChunk>::iterator it;
            for (it = vPendingFilteredChunks.begin(); it < vPendingFilteredChunks.end(); ++it)
            {
                // -- delay, request again
                if (nTimeNow - it->nTime > 60 * 5)
                {
                    if (fDebugNet)
                        LogPrintf("Timeout: Re-requesting chunk, starting from %s\n", it->startHash.ToString().c_str());

                    std::map<uint256, CBlockThinIndex*>::iterator mi = mapBlockThinIndex.find(it->startHash);

                    if (mi != mapBlockThinIndex.end())
                    {
                        CBlockThinIndex *pblockindex = mi->second;

                        for (uint32_t i = 0; i < 512; ++i) // choose a low number to spread the load over more peers
                        {
                            if (pblockindex->phashBlock)
                                inv.hash = *pblockindex->phashBlock;
                            else
                                inv.hash = pblockindex->GetBlockHash();

                            vGetFilteredBlocks.push_back(inv);
                            nHeightFilteredNeeded++;

                            if (!pblockindex->pnext
                                || inv.hash == it->endHash)
                                break;
                            pblockindex = pblockindex->pnext;
                        };
                    } else
                    {
                        LogPrintf("Error: Can't find hash %s, to re-request filtered blocks.\n", it->startHash.ToString().c_str());
                    };

                    vPendingFilteredChunks.erase(it);
                    break; // give other peers a chance
                };
            };
        };
    };

    if (!vGetFilteredBlocks.empty())
    {
        if (fDebugNet)
            LogPrintf("Requesting filtered chunk from peer %s, starting from %s\n", pto->addr.ToString().c_str(), vGetFilteredBlocks[0].hash.ToString().c_str());

        vPendingFilteredChunks.push_back(CPendingFilteredChunk(vGetFilteredBlocks[0].hash, vGetFilteredBlocks[vGetFilteredBlocks.size()-1].hash, GetTime()));
        pto->PushMessage("getdata", vGetFilteredBlocks);
    };


    //
    // Message: getdata
    //
    std::vector<CInv> vGetData;
    int64_t nNow = GetTime() * 1000000;
    CTxDB txdb("r");

    while (!pto->mapAskFor.empty() && (*pto->mapAskFor.begin()).first <= nNow)
    {
        const CInv& inv = (*pto->mapAskFor.begin()).second;
    
        if ((nNodeMode == NT_FULL && !AlreadyHave(txdb, inv))
            || (nNodeMode == NT_THIN && !AlreadyHaveThin(txdb, inv)))
        {
            if (fDebugNet)
                LogPrintf("sending getdata: %s\n", inv.ToString().c_str());

            vGetData.push_back(inv);
            if (vGetData.size() >= 1000)
            {
                pto->PushMessage("getdata", vGetData);
                vGetData.clear();
            };
            mapAlreadyAskedFor[inv] = nNow;
        };
        pto->mapAskFor.erase(pto->mapAskFor.begin());
    };

    if (!vGetData.empty())
        pto->PushMessage("getdata", vGetData);


    if (fSecMsgEnabled)
        SecureMsgSendData(pto, fSendTrickle); // should be in cs_main?
    
    return true;
}

