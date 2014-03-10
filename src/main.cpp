// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"

#include "addrman.h"
#include "alert.h"
#include "chainparams.h"
#include "checkpoints.h"
#include "checkqueue.h"
#include "init.h"
#include "net.h"
#include "txdb.h"
#include "txmempool.h"
#include "ui_interface.h"
#include "util.h"

#include <sstream>

#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

using namespace std;
using namespace boost;

#if defined(NDEBUG)
# error "Bitcoin cannot be compiled without assertions."
#endif

//
// Global state
//

CCriticalSection cs_main;

CTxMemPool mempool;

map<uint256, CBlockIndex*> mapBlockIndex;
CChain chainActive;
CChain chainMostWork;
int64_t nTimeBestReceived = 0;
int nScriptCheckThreads = 0;
bool fImporting = false;
bool fReindex = false;
bool fBenchmark = false;
bool fTxIndex = false;
unsigned int nCoinCacheSize = 5000;

/** Fees smaller than this (in satoshi) are considered zero fee (for transaction creation) */
int64_t CTransaction::nMinTxFee = 10000;  // Override with -mintxfee
/** Fees smaller than this (in satoshi) are considered zero fee (for relaying and mining) */
int64_t CTransaction::nMinRelayTxFee = 1000;

static CMedianFilter<int> cPeerBlockCounts(8, 0); // Amount of blocks that other nodes claim to have

struct COrphanBlock {
    uint256 hashBlock;
    uint256 hashPrev;
    vector<unsigned char> vchBlock;
};
map<uint256, COrphanBlock*> mapOrphanBlocks;
multimap<uint256, COrphanBlock*> mapOrphanBlocksByPrev;

map<uint256, CTransaction> mapOrphanTransactions;
map<uint256, set<uint256> > mapOrphanTransactionsByPrev;

// Constant stuff for coinbase transactions we create:
CScript COINBASE_FLAGS;

const string strMessageMagic = "Bitcoin Signed Message:\n";

// Internal stuff
namespace {
struct CBlockIndexWorkComparator
{
    bool operator()(CBlockIndex *pa, CBlockIndex *pb) {
        // First sort by most total work, ...
        if (pa->nChainWork > pb->nChainWork) return false;
        if (pa->nChainWork < pb->nChainWork) return true;

        // ... then by earliest time received, ...
        if (pa->nSequenceId < pb->nSequenceId) return false;
        if (pa->nSequenceId > pb->nSequenceId) return true;

        // Use pointer address as tie breaker (should only happen with blocks
        // loaded from disk, as those all have id 0).
        if (pa < pb) return false;
        if (pa > pb) return true;

        // Identical blocks.
        return false;
    }
};

CBlockIndex *pindexBestInvalid;
set<CBlockIndex*, CBlockIndexWorkComparator> setBlockIndexValid; // may contain all CBlockIndex*'s that have validness >=BLOCK_VALID_TRANSACTIONS, and must contain those who aren't failed

CCriticalSection cs_LastBlockFile;
CBlockFileInfo infoLastBlockFile;
int nLastBlockFile = 0;

// Every received block is assigned a unique and increasing identifier, so we
// know which one to give priority in case of a fork.
CCriticalSection cs_nBlockSequenceId;
// Blocks loaded from disk are assigned id 0, so start the counter at 1.
uint32_t nBlockSequenceId = 1;

// Sources of received blocks, to be able to send them reject messages or ban
// them, if processing happens afterwards. Protected by cs_main.
map<uint256, NodeId> mapBlockSource;
}

//////////////////////////////////////////////////////////////////////////////
//
// dispatching functions
//

// These functions dispatch to one or all registered wallets

namespace {
struct CMainSignals {
    // Notifies listeners of updated transaction data (passing hash, transaction, and optionally the block it is found in.
    boost::signals2::signal<void (const uint256 &, const CTransaction &, const CBlock *)> SyncTransaction;
    // Notifies listeners of an erased transaction (currently disabled, requires transaction replacement).
    boost::signals2::signal<void (const uint256 &)> EraseTransaction;
    // Notifies listeners of an updated transaction without new data (for now: a coinbase potentially becoming visible).
    boost::signals2::signal<void (const uint256 &)> UpdatedTransaction;
    // Notifies listeners of a new active block chain.
    boost::signals2::signal<void (const CBlockLocator &)> SetBestChain;
    // Notifies listeners about an inventory item being seen on the network.
    boost::signals2::signal<void (const uint256 &)> Inventory;
    // Tells listeners to broadcast their data.
    boost::signals2::signal<void ()> Broadcast;
} g_signals;
}

void RegisterWallet(CWalletInterface* pwalletIn) {
    g_signals.SyncTransaction.connect(boost::bind(&CWalletInterface::SyncTransaction, pwalletIn, _1, _2, _3));
    g_signals.EraseTransaction.connect(boost::bind(&CWalletInterface::EraseFromWallet, pwalletIn, _1));
    g_signals.UpdatedTransaction.connect(boost::bind(&CWalletInterface::UpdatedTransaction, pwalletIn, _1));
    g_signals.SetBestChain.connect(boost::bind(&CWalletInterface::SetBestChain, pwalletIn, _1));
    g_signals.Inventory.connect(boost::bind(&CWalletInterface::Inventory, pwalletIn, _1));
    g_signals.Broadcast.connect(boost::bind(&CWalletInterface::ResendWalletTransactions, pwalletIn));
}

void UnregisterWallet(CWalletInterface* pwalletIn) {
    g_signals.Broadcast.disconnect(boost::bind(&CWalletInterface::ResendWalletTransactions, pwalletIn));
    g_signals.Inventory.disconnect(boost::bind(&CWalletInterface::Inventory, pwalletIn, _1));
    g_signals.SetBestChain.disconnect(boost::bind(&CWalletInterface::SetBestChain, pwalletIn, _1));
    g_signals.UpdatedTransaction.disconnect(boost::bind(&CWalletInterface::UpdatedTransaction, pwalletIn, _1));
    g_signals.EraseTransaction.disconnect(boost::bind(&CWalletInterface::EraseFromWallet, pwalletIn, _1));
    g_signals.SyncTransaction.disconnect(boost::bind(&CWalletInterface::SyncTransaction, pwalletIn, _1, _2, _3));
}

void UnregisterAllWallets() {
    g_signals.Broadcast.disconnect_all_slots();
    g_signals.Inventory.disconnect_all_slots();
    g_signals.SetBestChain.disconnect_all_slots();
    g_signals.UpdatedTransaction.disconnect_all_slots();
    g_signals.EraseTransaction.disconnect_all_slots();
    g_signals.SyncTransaction.disconnect_all_slots();
}

void SyncWithWallets(const uint256 &hash, const CTransaction &tx, const CBlock *pblock) {
    g_signals.SyncTransaction(hash, tx, pblock);
}

//////////////////////////////////////////////////////////////////////////////
//
// Registration of network node signals.
//

namespace {

struct CBlockReject {
    unsigned char chRejectCode;
    string strRejectReason;
    uint256 hashBlock;
};

// Maintain validation-specific state about nodes, protected by cs_main, instead
// by CNode's own locks. This simplifies asynchronous operation, where
// processing of incoming data is done after the ProcessMessage call returns,
// and we're no longer holding the node's locks.
struct CNodeState {
    // Accumulated misbehaviour score for this peer.
    int nMisbehavior;
    // Whether this peer should be disconnected and banned.
    bool fShouldBan;
    // String name of this peer (debugging/logging purposes).
    std::string name;
    // List of asynchronously-determined block rejections to notify this peer about.
    std::vector<CBlockReject> rejects;

    CNodeState() {
        nMisbehavior = 0;
        fShouldBan = false;
    }
};

// Map maintaining per-node state. Requires cs_main.
map<NodeId, CNodeState> mapNodeState;

// Requires cs_main.
CNodeState *State(NodeId pnode) {
    map<NodeId, CNodeState>::iterator it = mapNodeState.find(pnode);
    if (it == mapNodeState.end())
        return NULL;
    return &it->second;
}

int GetHeight()
{
    LOCK(cs_main);
    return chainActive.Height();
}

void InitializeNode(NodeId nodeid, const CNode *pnode) {
    LOCK(cs_main);
    CNodeState &state = mapNodeState.insert(std::make_pair(nodeid, CNodeState())).first->second;
    state.name = pnode->addrName;
}

void FinalizeNode(NodeId nodeid) {
    LOCK(cs_main);
    mapNodeState.erase(nodeid);
}
}

bool GetNodeStateStats(NodeId nodeid, CNodeStateStats &stats) {
    LOCK(cs_main);
    CNodeState *state = State(nodeid);
    if (state == NULL)
        return false;
    stats.nMisbehavior = state->nMisbehavior;
    return true;
}

void RegisterNodeSignals(CNodeSignals& nodeSignals)
{
    nodeSignals.GetHeight.connect(&GetHeight);
    nodeSignals.ProcessMessages.connect(&ProcessMessages);
    nodeSignals.SendMessages.connect(&SendMessages);
    nodeSignals.InitializeNode.connect(&InitializeNode);
    nodeSignals.FinalizeNode.connect(&FinalizeNode);
}

void UnregisterNodeSignals(CNodeSignals& nodeSignals)
{
    nodeSignals.GetHeight.disconnect(&GetHeight);
    nodeSignals.ProcessMessages.disconnect(&ProcessMessages);
    nodeSignals.SendMessages.disconnect(&SendMessages);
    nodeSignals.InitializeNode.disconnect(&InitializeNode);
    nodeSignals.FinalizeNode.disconnect(&FinalizeNode);
}

//////////////////////////////////////////////////////////////////////////////
//
// CChain implementation
//

CBlockIndex *CChain::SetTip(CBlockIndex *pindex) {
    if (pindex == NULL) {
        vChain.clear();
        return NULL;
    }
    vChain.resize(pindex->nHeight + 1);
    while (pindex && vChain[pindex->nHeight] != pindex) {
        vChain[pindex->nHeight] = pindex;
        pindex = pindex->pprev;
    }
    return pindex;
}

CBlockLocator CChain::GetLocator(const CBlockIndex *pindex) const {
    int nStep = 1;
    std::vector<uint256> vHave;
    vHave.reserve(32);

    if (!pindex)
        pindex = Tip();
    while (pindex) {
        vHave.push_back(pindex->GetBlockHash());
        // Stop when we have added the genesis block.
        if (pindex->nHeight == 0)
            break;
        // Exponentially larger steps back, plus the genesis block.
        int nHeight = std::max(pindex->nHeight - nStep, 0);
        // In case pindex is not in this chain, iterate pindex->pprev to find blocks.
        while (pindex->nHeight > nHeight && !Contains(pindex))
            pindex = pindex->pprev;
        // If pindex is in this chain, use direct height-based access.
        if (pindex->nHeight > nHeight)
            pindex = (*this)[nHeight];
        if (vHave.size() > 10)
            nStep *= 2;
    }

    return CBlockLocator(vHave);
}

CBlockIndex *CChain::FindFork(const CBlockLocator &locator) const {
    // Find the first block the caller has in the main chain
    BOOST_FOREACH(const uint256& hash, locator.vHave) {
        std::map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hash);
        if (mi != mapBlockIndex.end())
        {
            CBlockIndex* pindex = (*mi).second;
            if (Contains(pindex))
                return pindex;
        }
    }
    return Genesis();
}

CCoinsViewCache *pcoinsTip = NULL;
CBlockTreeDB *pblocktree = NULL;

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
    unsigned int sz = tx.GetSerializeSize(SER_NETWORK, CTransaction::CURRENT_VERSION);
    if (sz > 5000)
    {
        LogPrint("mempool", "ignoring large orphan tx (size: %u, hash: %s)\n", sz, hash.ToString());
        return false;
    }

    mapOrphanTransactions[hash] = tx;
    BOOST_FOREACH(const CTxIn& txin, tx.vin)
        mapOrphanTransactionsByPrev[txin.prevout.hash].insert(hash);

    LogPrint("mempool", "stored orphan tx %s (mapsz %"PRIszu")\n", hash.ToString(),
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
    }
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







bool IsStandardTx(const CTransaction& tx, string& reason)
{
    if (tx.nVersion > CTransaction::CURRENT_VERSION || tx.nVersion < 1) {
        reason = "version";
        return false;
    }

    // Treat non-final transactions as non-standard to prevent a specific type
    // of double-spend attack, as well as DoS attacks. (if the transaction
    // can't be mined, the attacker isn't expending resources broadcasting it)
    // Basically we don't want to propagate transactions that can't included in
    // the next block.
    //
    // However, IsFinalTx() is confusing... Without arguments, it uses
    // chainActive.Height() to evaluate nLockTime; when a block is accepted, chainActive.Height()
    // is set to the value of nHeight in the block. However, when IsFinalTx()
    // is called within CBlock::AcceptBlock(), the height of the block *being*
    // evaluated is what is used. Thus if we want to know if a transaction can
    // be part of the *next* block, we need to call IsFinalTx() with one more
    // than chainActive.Height().
    //
    // Timestamps on the other hand don't get any special treatment, because we
    // can't know what timestamp the next block will have, and there aren't
    // timestamp applications where it matters.
    if (!IsFinalTx(tx, chainActive.Height() + 1)) {
        reason = "non-final";
        return false;
    }

    // Extremely large transactions with lots of inputs can cost the network
    // almost as much to process as they cost the sender in fees, because
    // computing signature hashes is O(ninputs*txsize). Limiting transactions
    // to MAX_STANDARD_TX_SIZE mitigates CPU exhaustion attacks.
    unsigned int sz = tx.GetSerializeSize(SER_NETWORK, CTransaction::CURRENT_VERSION);
    if (sz >= MAX_STANDARD_TX_SIZE) {
        reason = "tx-size";
        return false;
    }

    BOOST_FOREACH(const CTxIn& txin, tx.vin)
    {
        // Biggest 'standard' txin is a 3-signature 3-of-3 CHECKMULTISIG
        // pay-to-script-hash, which is 3 ~80-byte signatures, 3
        // ~65-byte public keys, plus a few script ops.
        if (txin.scriptSig.size() > 500) {
            reason = "scriptsig-size";
            return false;
        }
        if (!txin.scriptSig.IsPushOnly()) {
            reason = "scriptsig-not-pushonly";
            return false;
        }
        if (!txin.scriptSig.HasCanonicalPushes()) {
            reason = "scriptsig-non-canonical-push";
            return false;
        }
    }

    unsigned int nDataOut = 0;
    txnouttype whichType;
    BOOST_FOREACH(const CTxOut& txout, tx.vout) {
        if (!::IsStandard(txout.scriptPubKey, whichType)) {
            reason = "scriptpubkey";
            return false;
        }
        if (whichType == TX_NULL_DATA)
            nDataOut++;
        else if (txout.IsDust(CTransaction::nMinRelayTxFee)) {
            reason = "dust";
            return false;
        }
    }

    // only one OP_RETURN txout is permitted
    if (nDataOut > 1) {
        reason = "multi-op-return";
        return false;
    }

    return true;
}

bool IsFinalTx(const CTransaction &tx, int nBlockHeight, int64_t nBlockTime)
{
    // Time based nLockTime implemented in 0.1.6
    if (tx.nLockTime == 0)
        return true;
    if (nBlockHeight == 0)
        nBlockHeight = chainActive.Height();
    if (nBlockTime == 0)
        nBlockTime = GetAdjustedTime();
    if ((int64_t)tx.nLockTime < ((int64_t)tx.nLockTime < LOCKTIME_THRESHOLD ? (int64_t)nBlockHeight : nBlockTime))
        return true;
    BOOST_FOREACH(const CTxIn& txin, tx.vin)
        if (!txin.IsFinal())
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
bool AreInputsStandard(const CTransaction& tx, CCoinsViewCache& mapInputs)
{
    if (tx.IsCoinBase())
        return true; // Coinbases don't use vin normally

    for (unsigned int i = 0; i < tx.vin.size(); i++)
    {
        const CTxOut& prev = mapInputs.GetOutputFor(tx.vin[i]);

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
        if (!EvalScript(stack, tx.vin[i].scriptSig, tx, i, false, 0))
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
        }

        if (stack.size() != (unsigned int)nArgsExpected)
            return false;
    }

    return true;
}

unsigned int GetLegacySigOpCount(const CTransaction& tx)
{
    unsigned int nSigOps = 0;
    BOOST_FOREACH(const CTxIn& txin, tx.vin)
    {
        nSigOps += txin.scriptSig.GetSigOpCount(false);
    }
    BOOST_FOREACH(const CTxOut& txout, tx.vout)
    {
        nSigOps += txout.scriptPubKey.GetSigOpCount(false);
    }
    return nSigOps;
}

unsigned int GetP2SHSigOpCount(const CTransaction& tx, CCoinsViewCache& inputs)
{
    if (tx.IsCoinBase())
        return 0;

    unsigned int nSigOps = 0;
    for (unsigned int i = 0; i < tx.vin.size(); i++)
    {
        const CTxOut &prevout = inputs.GetOutputFor(tx.vin[i]);
        if (prevout.scriptPubKey.IsPayToScriptHash())
            nSigOps += prevout.scriptPubKey.GetSigOpCount(tx.vin[i].scriptSig);
    }
    return nSigOps;
}

int CMerkleTx::SetMerkleBranch(const CBlock* pblock)
{
    CBlock blockTmp;

    if (pblock == NULL) {
        CCoins coins;
        if (pcoinsTip->GetCoins(GetHash(), coins)) {
            CBlockIndex *pindex = chainActive[coins.nHeight];
            if (pindex) {
                if (!ReadBlockFromDisk(blockTmp, pindex))
                    return 0;
                pblock = &blockTmp;
            }
        }
    }

    if (pblock) {
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
        }

        // Fill in merkle branch
        vMerkleBranch = pblock->GetMerkleBranch(nIndex);
    }

    // Is the tx in a block that's in the main chain
    map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hashBlock);
    if (mi == mapBlockIndex.end())
        return 0;
    CBlockIndex* pindex = (*mi).second;
    if (!pindex || !chainActive.Contains(pindex))
        return 0;

    return chainActive.Height() - pindex->nHeight + 1;
}







bool CheckTransaction(const CTransaction& tx, CValidationState &state)
{
    // Basic checks that don't depend on any context
    if (tx.vin.empty())
        return state.DoS(10, error("CheckTransaction() : vin empty"),
                         REJECT_INVALID, "bad-txns-vin-empty");
    if (tx.vout.empty())
        return state.DoS(10, error("CheckTransaction() : vout empty"),
                         REJECT_INVALID, "bad-txns-vout-empty");
    // Size limits
    if (::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION) > MAX_BLOCK_SIZE)
        return state.DoS(100, error("CheckTransaction() : size limits failed"),
                         REJECT_INVALID, "bad-txns-oversize");

    // Check for negative or overflow output values
    int64_t nValueOut = 0;
    BOOST_FOREACH(const CTxOut& txout, tx.vout)
    {
        if (txout.nValue < 0)
            return state.DoS(100, error("CheckTransaction() : txout.nValue negative"),
                             REJECT_INVALID, "bad-txns-vout-negative");
        if (txout.nValue > MAX_MONEY)
            return state.DoS(100, error("CheckTransaction() : txout.nValue too high"),
                             REJECT_INVALID, "bad-txns-vout-toolarge");
        nValueOut += txout.nValue;
        if (!MoneyRange(nValueOut))
            return state.DoS(100, error("CheckTransaction() : txout total out of range"),
                             REJECT_INVALID, "bad-txns-txouttotal-toolarge");
    }

    // Check for duplicate inputs
    set<COutPoint> vInOutPoints;
    BOOST_FOREACH(const CTxIn& txin, tx.vin)
    {
        if (vInOutPoints.count(txin.prevout))
            return state.DoS(100, error("CheckTransaction() : duplicate inputs"),
                             REJECT_INVALID, "bad-txns-inputs-duplicate");
        vInOutPoints.insert(txin.prevout);
    }

    if (tx.IsCoinBase())
    {
        if (tx.vin[0].scriptSig.size() < 2 || tx.vin[0].scriptSig.size() > 100)
            return state.DoS(100, error("CheckTransaction() : coinbase script size"),
                             REJECT_INVALID, "bad-cb-length");
    }
    else
    {
        BOOST_FOREACH(const CTxIn& txin, tx.vin)
            if (txin.prevout.IsNull())
                return state.DoS(10, error("CheckTransaction() : prevout is null"),
                                 REJECT_INVALID, "bad-txns-prevout-null");
    }

    return true;
}

int64_t GetMinFee(const CTransaction& tx, unsigned int nBytes, bool fAllowFree, enum GetMinFee_mode mode)
{
    // Base fee is either nMinTxFee or nMinRelayTxFee
    int64_t nBaseFee = (mode == GMF_RELAY) ? tx.nMinRelayTxFee : tx.nMinTxFee;

    int64_t nMinFee = (1 + (int64_t)nBytes / 1000) * nBaseFee;

    if (fAllowFree)
    {
        // There is a free transaction area in blocks created by most miners,
        // * If we are relaying we allow transactions up to DEFAULT_BLOCK_PRIORITY_SIZE - 1000
        //   to be considered to fall into this category. We don't want to encourage sending
        //   multiple transactions instead of one big transaction to avoid fees.
        // * If we are creating a transaction we allow transactions up to 1,000 bytes
        //   to be considered safe and assume they can likely make it into this section.
        if (nBytes < (mode == GMF_SEND ? 1000 : (DEFAULT_BLOCK_PRIORITY_SIZE - 1000)))
            nMinFee = 0;
    }

    // This code can be removed after enough miners have upgraded to version 0.9.
    // Until then, be safe when sending and require a fee if any output
    // is less than CENT:
    if (nMinFee < nBaseFee && mode == GMF_SEND)
    {
        BOOST_FOREACH(const CTxOut& txout, tx.vout)
            if (txout.nValue < CENT)
                nMinFee = nBaseFee;
    }

    if (!MoneyRange(nMinFee))
        nMinFee = MAX_MONEY;
    return nMinFee;
}


bool AcceptToMemoryPool(CTxMemPool& pool, CValidationState &state, const CTransaction &tx, bool fLimitFree,
                        bool* pfMissingInputs, bool fRejectInsaneFee)
{
    if (pfMissingInputs)
        *pfMissingInputs = false;

    if (!CheckTransaction(tx, state))
        return error("AcceptToMemoryPool: : CheckTransaction failed");

    // Coinbase is only valid in a block, not as a loose transaction
    if (tx.IsCoinBase())
        return state.DoS(100, error("AcceptToMemoryPool: : coinbase as individual tx"),
                         REJECT_INVALID, "coinbase");

    // Rather not work on nonstandard transactions (unless -testnet/-regtest)
    string reason;
    if (Params().NetworkID() == CChainParams::MAIN && !IsStandardTx(tx, reason))
        return state.DoS(0,
                         error("AcceptToMemoryPool : nonstandard transaction: %s", reason),
                         REJECT_NONSTANDARD, reason);

    // is it already in the memory pool?
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
        }
    }
    }

    {
        CCoinsView dummy;
        CCoinsViewCache view(dummy);

        {
        LOCK(pool.cs);
        CCoinsViewMemPool viewMemPool(*pcoinsTip, pool);
        view.SetBackend(viewMemPool);

        // do we already have it?
        if (view.HaveCoins(hash))
            return false;

        // do all inputs exist?
        // Note that this does not check for the presence of actual outputs (see the next check for that),
        // only helps filling in pfMissingInputs (to determine missing vs spent).
        BOOST_FOREACH(const CTxIn txin, tx.vin) {
            if (!view.HaveCoins(txin.prevout.hash)) {
                if (pfMissingInputs)
                    *pfMissingInputs = true;
                return false;
            }
        }

        // are the actual inputs available?
        if (!view.HaveInputs(tx))
            return state.Invalid(error("AcceptToMemoryPool : inputs already spent"),
                                 REJECT_DUPLICATE, "bad-txns-inputs-spent");

        // Bring the best block into scope
        view.GetBestBlock();

        // we have all inputs cached now, so switch back to dummy, so we don't need to keep lock on mempool
        view.SetBackend(dummy);
        }

        // Check for non-standard pay-to-script-hash in inputs
        if (Params().NetworkID() == CChainParams::MAIN && !AreInputsStandard(tx, view))
            return error("AcceptToMemoryPool: : nonstandard transaction input");

        // Note: if you modify this code to accept non-standard transactions, then
        // you should add code here to check that the transaction does a
        // reasonable number of ECDSA signature verifications.

        int64_t nValueIn = view.GetValueIn(tx);
        int64_t nValueOut = tx.GetValueOut();
        int64_t nFees = nValueIn-nValueOut;
        double dPriority = view.GetPriority(tx, chainActive.Height());

        CTxMemPoolEntry entry(tx, nFees, GetTime(), dPriority, chainActive.Height());
        unsigned int nSize = entry.GetTxSize();

        // Don't accept it if it can't get into a block
        int64_t txMinFee = GetMinFee(tx, nSize, true, GMF_RELAY);
        if (fLimitFree && nFees < txMinFee)
            return state.DoS(0, error("AcceptToMemoryPool : not enough fees %s, %d < %d",
                                      hash.ToString(), nFees, txMinFee),
                             REJECT_INSUFFICIENTFEE, "insufficient fee");

        // Continuously rate-limit free transactions
        // This mitigates 'penny-flooding' -- sending thousands of free transactions just to
        // be annoying or make others' transactions take longer to confirm.
        if (fLimitFree && nFees < CTransaction::nMinRelayTxFee)
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
            if (dFreeCount >= GetArg("-limitfreerelay", 15)*10*1000)
                return state.DoS(0, error("AcceptToMemoryPool : free transaction rejected by rate limiter"),
                                 REJECT_INSUFFICIENTFEE, "insufficient priority");
            LogPrint("mempool", "Rate limit dFreeCount: %g => %g\n", dFreeCount, dFreeCount+nSize);
            dFreeCount += nSize;
        }

        if (fRejectInsaneFee && nFees > CTransaction::nMinRelayTxFee * 10000)
            return error("AcceptToMemoryPool: : insane fees %s, %d > %d",
                         hash.ToString(),
                         nFees, CTransaction::nMinRelayTxFee * 10000);

        // Check against previous transactions
        // This is done last to help prevent CPU exhaustion denial-of-service attacks.
        if (!CheckInputs(tx, state, view, true, SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_STRICTENC))
        {
            return error("AcceptToMemoryPool: : ConnectInputs failed %s", hash.ToString());
        }
        // Store transaction in memory
        pool.addUnchecked(hash, entry);
    }

    g_signals.SyncTransaction(hash, tx, NULL);

    return true;
}


int CMerkleTx::GetDepthInMainChainINTERNAL(CBlockIndex* &pindexRet) const
{
    if (hashBlock == 0 || nIndex == -1)
        return 0;

    // Find the block it claims to be in
    map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hashBlock);
    if (mi == mapBlockIndex.end())
        return 0;
    CBlockIndex* pindex = (*mi).second;
    if (!pindex || !chainActive.Contains(pindex))
        return 0;

    // Make sure the merkle branch connects to this block
    if (!fMerkleVerified)
    {
        if (CBlock::CheckMerkleBranch(GetHash(), vMerkleBranch, nIndex) != pindex->hashMerkleRoot)
            return 0;
        fMerkleVerified = true;
    }

    pindexRet = pindex;
    return chainActive.Height() - pindex->nHeight + 1;
}

int CMerkleTx::GetDepthInMainChain(CBlockIndex* &pindexRet) const
{
    int nResult = GetDepthInMainChainINTERNAL(pindexRet);
    if (nResult == 0 && !mempool.exists(GetHash()))
        return -1; // Not in chain, not in mempool

    return nResult;
}

int CMerkleTx::GetBlocksToMaturity() const
{
    if (!IsCoinBase())
        return 0;
    return max(0, (COINBASE_MATURITY+1) - GetDepthInMainChain());
}


bool CMerkleTx::AcceptToMemoryPool(bool fLimitFree)
{
    CValidationState state;
    return ::AcceptToMemoryPool(mempool, state, *this, fLimitFree, NULL);
}


// Return transaction in tx, and if it was found inside a block, its hash is placed in hashBlock
bool GetTransaction(const uint256 &hash, CTransaction &txOut, uint256 &hashBlock, bool fAllowSlow)
{
    CBlockIndex *pindexSlow = NULL;
    {
        LOCK(cs_main);
        {
            if (mempool.lookup(hash, txOut))
            {
                return true;
            }
        }

        if (fTxIndex) {
            CDiskTxPos postx;
            if (pblocktree->ReadTxIndex(hash, postx)) {
                CAutoFile file(OpenBlockFile(postx, true), SER_DISK, CLIENT_VERSION);
                CBlockHeader header;
                try {
                    file >> header;
                    fseek(file, postx.nTxOffset, SEEK_CUR);
                    file >> txOut;
                } catch (std::exception &e) {
                    return error("%s : Deserialize or I/O error - %s", __PRETTY_FUNCTION__, e.what());
                }
                hashBlock = header.GetHash();
                if (txOut.GetHash() != hash)
                    return error("%s : txid mismatch", __PRETTY_FUNCTION__);
                return true;
            }
        }

        if (fAllowSlow) { // use coin database to locate block that contains transaction, and scan it
            int nHeight = -1;
            {
                CCoinsViewCache &view = *pcoinsTip;
                CCoins coins;
                if (view.GetCoins(hash, coins))
                    nHeight = coins.nHeight;
            }
            if (nHeight > 0)
                pindexSlow = chainActive[nHeight];
        }
    }

    if (pindexSlow) {
        CBlock block;
        if (ReadBlockFromDisk(block, pindexSlow)) {
            BOOST_FOREACH(const CTransaction &tx, block.vtx) {
                if (tx.GetHash() == hash) {
                    txOut = tx;
                    hashBlock = pindexSlow->GetBlockHash();
                    return true;
                }
            }
        }
    }

    return false;
}






//////////////////////////////////////////////////////////////////////////////
//
// CBlock and CBlockIndex
//

bool WriteBlockToDisk(CBlock& block, CDiskBlockPos& pos)
{
    // Open history file to append
    CAutoFile fileout = CAutoFile(OpenBlockFile(pos), SER_DISK, CLIENT_VERSION);
    if (!fileout)
        return error("WriteBlockToDisk : OpenBlockFile failed");

    // Write index header
    unsigned int nSize = fileout.GetSerializeSize(block);
    fileout << FLATDATA(Params().MessageStart()) << nSize;

    // Write block
    long fileOutPos = ftell(fileout);
    if (fileOutPos < 0)
        return error("WriteBlockToDisk : ftell failed");
    pos.nPos = (unsigned int)fileOutPos;
    fileout << block;

    // Flush stdio buffers and commit to disk before returning
    fflush(fileout);
    if (!IsInitialBlockDownload())
        FileCommit(fileout);

    return true;
}

bool ReadBlockFromDisk(CBlock& block, const CDiskBlockPos& pos)
{
    block.SetNull();

    // Open history file to read
    CAutoFile filein = CAutoFile(OpenBlockFile(pos, true), SER_DISK, CLIENT_VERSION);
    if (!filein)
        return error("ReadBlockFromDisk : OpenBlockFile failed");

    // Read block
    try {
        filein >> block;
    }
    catch (std::exception &e) {
        return error("%s : Deserialize or I/O error - %s", __PRETTY_FUNCTION__, e.what());
    }

    // Check the header
    if (!CheckProofOfWork(block.GetHash(), block.nBits))
        return error("ReadBlockFromDisk : Errors in block header");

    return true;
}

bool ReadBlockFromDisk(CBlock& block, const CBlockIndex* pindex)
{
    if (!ReadBlockFromDisk(block, pindex->GetBlockPos()))
        return false;
    if (block.GetHash() != pindex->GetBlockHash())
        return error("ReadBlockFromDisk(CBlock&, CBlockIndex*) : GetHash() doesn't match index");
    return true;
}

uint256 static GetOrphanRoot(const uint256& hash)
{
    map<uint256, COrphanBlock*>::iterator it = mapOrphanBlocks.find(hash);
    if (it == mapOrphanBlocks.end())
        return hash;

    // Work back to the first block in the orphan chain
    do {
        map<uint256, COrphanBlock*>::iterator it2 = mapOrphanBlocks.find(it->second->hashPrev);
        if (it2 == mapOrphanBlocks.end())
            return it->first;
        it = it2;
    } while(true);
}

// Remove a random orphan block (which does not have any dependent orphans).
void static PruneOrphanBlocks()
{
    if (mapOrphanBlocksByPrev.size() <= MAX_ORPHAN_BLOCKS)
        return;

    // Pick a random orphan block.
    int pos = insecure_rand() % mapOrphanBlocksByPrev.size();
    std::multimap<uint256, COrphanBlock*>::iterator it = mapOrphanBlocksByPrev.begin();
    while (pos--) it++;

    // As long as this block has other orphans depending on it, move to one of those successors.
    do {
        std::multimap<uint256, COrphanBlock*>::iterator it2 = mapOrphanBlocksByPrev.find(it->second->hashBlock);
        if (it2 == mapOrphanBlocksByPrev.end())
            break;
        it = it2;
    } while(1);

    uint256 hash = it->second->hashBlock;
    delete it->second;
    mapOrphanBlocksByPrev.erase(it);
    mapOrphanBlocks.erase(hash);
}

int64_t GetBlockValue(int nHeight, int64_t nFees)
{
    int64_t nSubsidy = 50 * COIN;

    // Subsidy is cut in half every 210,000 blocks which will occur approximately every 4 years.
    nSubsidy >>= (nHeight / Params().SubsidyHalvingInterval());

    return nSubsidy + nFees;
}

static const int64_t nTargetTimespan = 14 * 24 * 60 * 60; // two weeks
static const int64_t nTargetSpacing = 10 * 60;
static const int64_t nInterval = nTargetTimespan / nTargetSpacing;

//
// minimum amount of work that could possibly be required nTime after
// minimum work required was nBase
//
unsigned int ComputeMinWork(unsigned int nBase, int64_t nTime)
{
    const CBigNum &bnLimit = Params().ProofOfWorkLimit();
    // Testnet has min-difficulty blocks
    // after nTargetSpacing*2 time between blocks:
    if (TestNet() && nTime > nTargetSpacing*2)
        return bnLimit.GetCompact();

    CBigNum bnResult;
    bnResult.SetCompact(nBase);
    while (nTime > 0 && bnResult < bnLimit)
    {
        // Maximum 400% adjustment...
        bnResult *= 4;
        // ... in best-case exactly 4-times-normal target time
        nTime -= nTargetTimespan*4;
    }
    if (bnResult > bnLimit)
        bnResult = bnLimit;
    return bnResult.GetCompact();
}

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock)
{
    unsigned int nProofOfWorkLimit = Params().ProofOfWorkLimit().GetCompact();

    // Genesis block
    if (pindexLast == NULL)
        return nProofOfWorkLimit;

    // Only change once per interval
    if ((pindexLast->nHeight+1) % nInterval != 0)
    {
        if (TestNet())
        {
            // Special difficulty rule for testnet:
            // If the new block's timestamp is more than 2* 10 minutes
            // then allow mining of a min-difficulty block.
            if (pblock->nTime > pindexLast->nTime + nTargetSpacing*2)
                return nProofOfWorkLimit;
            else
            {
                // Return the last non-special-min-difficulty-rules-block
                const CBlockIndex* pindex = pindexLast;
                while (pindex->pprev && pindex->nHeight % nInterval != 0 && pindex->nBits == nProofOfWorkLimit)
                    pindex = pindex->pprev;
                return pindex->nBits;
            }
        }
        return pindexLast->nBits;
    }

    // Go back by what we want to be 14 days worth of blocks
    const CBlockIndex* pindexFirst = pindexLast;
    for (int i = 0; pindexFirst && i < nInterval-1; i++)
        pindexFirst = pindexFirst->pprev;
    assert(pindexFirst);

    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - pindexFirst->GetBlockTime();
    LogPrintf("  nActualTimespan = %d  before bounds\n", nActualTimespan);
    if (nActualTimespan < nTargetTimespan/4)
        nActualTimespan = nTargetTimespan/4;
    if (nActualTimespan > nTargetTimespan*4)
        nActualTimespan = nTargetTimespan*4;

    // Retarget
    CBigNum bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew *= nActualTimespan;
    bnNew /= nTargetTimespan;

    if (bnNew > Params().ProofOfWorkLimit())
        bnNew = Params().ProofOfWorkLimit();

    /// debug print
    LogPrintf("GetNextWorkRequired RETARGET\n");
    LogPrintf("nTargetTimespan = %d    nActualTimespan = %d\n", nTargetTimespan, nActualTimespan);
    LogPrintf("Before: %08x  %s\n", pindexLast->nBits, CBigNum().SetCompact(pindexLast->nBits).getuint256().ToString());
    LogPrintf("After:  %08x  %s\n", bnNew.GetCompact(), bnNew.getuint256().ToString());

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
    if (fImporting || fReindex || chainActive.Height() < Checkpoints::GetTotalBlocksEstimate())
        return true;
    static int64_t nLastUpdate;
    static CBlockIndex* pindexLastBest;
    if (chainActive.Tip() != pindexLastBest)
    {
        pindexLastBest = chainActive.Tip();
        nLastUpdate = GetTime();
    }
    return (GetTime() - nLastUpdate < 10 &&
            chainActive.Tip()->GetBlockTime() < GetTime() - 24 * 60 * 60);
}

bool fLargeWorkForkFound = false;
bool fLargeWorkInvalidChainFound = false;
CBlockIndex *pindexBestForkTip = NULL, *pindexBestForkBase = NULL;

void CheckForkWarningConditions()
{
    // Before we get past initial download, we cannot reliably alert about forks
    // (we assume we don't get stuck on a fork before the last checkpoint)
    if (IsInitialBlockDownload())
        return;

    // If our best fork is no longer within 72 blocks (+/- 12 hours if no one mines it)
    // of our head, drop it
    if (pindexBestForkTip && chainActive.Height() - pindexBestForkTip->nHeight >= 72)
        pindexBestForkTip = NULL;

    if (pindexBestForkTip || (pindexBestInvalid && pindexBestInvalid->nChainWork > chainActive.Tip()->nChainWork + (chainActive.Tip()->GetBlockWork() * 6).getuint256()))
    {
        if (!fLargeWorkForkFound)
        {
            std::string strCmd = GetArg("-alertnotify", "");
            if (!strCmd.empty())
            {
                std::string warning = std::string("'Warning: Large-work fork detected, forking after block ") +
                                      pindexBestForkBase->phashBlock->ToString() + std::string("'");
                boost::replace_all(strCmd, "%s", warning);
                boost::thread t(runCommand, strCmd); // thread runs free
            }
        }
        if (pindexBestForkTip)
        {
            LogPrintf("CheckForkWarningConditions: Warning: Large valid fork found\n  forking the chain at height %d (%s)\n  lasting to height %d (%s).\nChain state database corruption likely.\n",
                   pindexBestForkBase->nHeight, pindexBestForkBase->phashBlock->ToString(),
                   pindexBestForkTip->nHeight, pindexBestForkTip->phashBlock->ToString());
            fLargeWorkForkFound = true;
        }
        else
        {
            LogPrintf("CheckForkWarningConditions: Warning: Found invalid chain at least ~6 blocks longer than our best chain.\nChain state database corruption likely.\n");
            fLargeWorkInvalidChainFound = true;
        }
    }
    else
    {
        fLargeWorkForkFound = false;
        fLargeWorkInvalidChainFound = false;
    }
}

void CheckForkWarningConditionsOnNewFork(CBlockIndex* pindexNewForkTip)
{
    // If we are on a fork that is sufficiently large, set a warning flag
    CBlockIndex* pfork = pindexNewForkTip;
    CBlockIndex* plonger = chainActive.Tip();
    while (pfork && pfork != plonger)
    {
        while (plonger && plonger->nHeight > pfork->nHeight)
            plonger = plonger->pprev;
        if (pfork == plonger)
            break;
        pfork = pfork->pprev;
    }

    // We define a condition which we should warn the user about as a fork of at least 7 blocks
    // who's tip is within 72 blocks (+/- 12 hours if no one mines it) of ours
    // We use 7 blocks rather arbitrarily as it represents just under 10% of sustained network
    // hash rate operating on the fork.
    // or a chain that is entirely longer than ours and invalid (note that this should be detected by both)
    // We define it this way because it allows us to only store the highest fork tip (+ base) which meets
    // the 7-block condition and from this always have the most-likely-to-cause-warning fork
    if (pfork && (!pindexBestForkTip || (pindexBestForkTip && pindexNewForkTip->nHeight > pindexBestForkTip->nHeight)) &&
            pindexNewForkTip->nChainWork - pfork->nChainWork > (pfork->GetBlockWork() * 7).getuint256() &&
            chainActive.Height() - pindexNewForkTip->nHeight < 72)
    {
        pindexBestForkTip = pindexNewForkTip;
        pindexBestForkBase = pfork;
    }

    CheckForkWarningConditions();
}

void Misbehaving(NodeId pnode, int howmuch)
{
    if (howmuch == 0)
        return;

    CNodeState *state = State(pnode);
    if (state == NULL)
        return;

    state->nMisbehavior += howmuch;
    if (state->nMisbehavior >= GetArg("-banscore", 100))
    {
        LogPrintf("Misbehaving: %s (%d -> %d) BAN THRESHOLD EXCEEDED\n", state->name, state->nMisbehavior-howmuch, state->nMisbehavior);
        state->fShouldBan = true;
    } else
        LogPrintf("Misbehaving: %s (%d -> %d)\n", state->name, state->nMisbehavior-howmuch, state->nMisbehavior);
}

void static InvalidChainFound(CBlockIndex* pindexNew)
{
    if (!pindexBestInvalid || pindexNew->nChainWork > pindexBestInvalid->nChainWork)
    {
        pindexBestInvalid = pindexNew;
        // The current code doesn't actually read the BestInvalidWork entry in
        // the block database anymore, as it is derived from the flags in block
        // index entry. We only write it for backward compatibility.
        pblocktree->WriteBestInvalidWork(CBigNum(pindexBestInvalid->nChainWork));
        uiInterface.NotifyBlocksChanged();
    }
    LogPrintf("InvalidChainFound: invalid block=%s  height=%d  log2_work=%.8g  date=%s\n",
      pindexNew->GetBlockHash().ToString(), pindexNew->nHeight,
      log(pindexNew->nChainWork.getdouble())/log(2.0), DateTimeStrFormat("%Y-%m-%d %H:%M:%S",
      pindexNew->GetBlockTime()));
    LogPrintf("InvalidChainFound:  current best=%s  height=%d  log2_work=%.8g  date=%s\n",
      chainActive.Tip()->GetBlockHash().ToString(), chainActive.Height(), log(chainActive.Tip()->nChainWork.getdouble())/log(2.0),
      DateTimeStrFormat("%Y-%m-%d %H:%M:%S", chainActive.Tip()->GetBlockTime()));
    CheckForkWarningConditions();
}

void static InvalidBlockFound(CBlockIndex *pindex, const CValidationState &state) {
    int nDoS = 0;
    if (state.IsInvalid(nDoS)) {
        std::map<uint256, NodeId>::iterator it = mapBlockSource.find(pindex->GetBlockHash());
        if (it != mapBlockSource.end() && State(it->second)) {
            CBlockReject reject = {state.GetRejectCode(), state.GetRejectReason(), pindex->GetBlockHash()};
            State(it->second)->rejects.push_back(reject);
            if (nDoS > 0)
                Misbehaving(it->second, nDoS);
        }
    }
    if (!state.CorruptionPossible()) {
        pindex->nStatus |= BLOCK_FAILED_VALID;
        pblocktree->WriteBlockIndex(CDiskBlockIndex(pindex));
        setBlockIndexValid.erase(pindex);
        InvalidChainFound(pindex);
    }
}

void UpdateTime(CBlockHeader& block, const CBlockIndex* pindexPrev)
{
    block.nTime = max(pindexPrev->GetMedianTimePast()+1, GetAdjustedTime());

    // Updating time can change work required on testnet:
    if (TestNet())
        block.nBits = GetNextWorkRequired(pindexPrev, &block);
}











void UpdateCoins(const CTransaction& tx, CValidationState &state, CCoinsViewCache &inputs, CTxUndo &txundo, int nHeight, const uint256 &txhash)
{
    bool ret;
    // mark inputs spent
    if (!tx.IsCoinBase()) {
        BOOST_FOREACH(const CTxIn &txin, tx.vin) {
            CCoins &coins = inputs.GetCoins(txin.prevout.hash);
            CTxInUndo undo;
            ret = coins.Spend(txin.prevout, undo);
            assert(ret);
            txundo.vprevout.push_back(undo);
        }
    }

    // add outputs
    ret = inputs.SetCoins(txhash, CCoins(tx, nHeight));
    assert(ret);
}

bool CScriptCheck::operator()() const {
    const CScript &scriptSig = ptxTo->vin[nIn].scriptSig;
    if (!VerifyScript(scriptSig, scriptPubKey, *ptxTo, nIn, nFlags, nHashType))
        return error("CScriptCheck() : %s VerifySignature failed", ptxTo->GetHash().ToString());
    return true;
}

bool VerifySignature(const CCoins& txFrom, const CTransaction& txTo, unsigned int nIn, unsigned int flags, int nHashType)
{
    return CScriptCheck(txFrom, txTo, nIn, flags, nHashType)();
}

bool CheckInputs(const CTransaction& tx, CValidationState &state, CCoinsViewCache &inputs, bool fScriptChecks, unsigned int flags, std::vector<CScriptCheck> *pvChecks)
{
    if (!tx.IsCoinBase())
    {
        if (pvChecks)
            pvChecks->reserve(tx.vin.size());

        // This doesn't trigger the DoS code on purpose; if it did, it would make it easier
        // for an attacker to attempt to split the network.
        if (!inputs.HaveInputs(tx))
            return state.Invalid(error("CheckInputs() : %s inputs unavailable", tx.GetHash().ToString()));

        // While checking, GetBestBlock() refers to the parent block.
        // This is also true for mempool checks.
        CBlockIndex *pindexPrev = mapBlockIndex.find(inputs.GetBestBlock())->second;
        int nSpendHeight = pindexPrev->nHeight + 1;
        int64_t nValueIn = 0;
        int64_t nFees = 0;
        for (unsigned int i = 0; i < tx.vin.size(); i++)
        {
            const COutPoint &prevout = tx.vin[i].prevout;
            const CCoins &coins = inputs.GetCoins(prevout.hash);

            // If prev is coinbase, check that it's matured
            if (coins.IsCoinBase()) {
                if (nSpendHeight - coins.nHeight < COINBASE_MATURITY)
                    return state.Invalid(
                        error("CheckInputs() : tried to spend coinbase at depth %d", nSpendHeight - coins.nHeight),
                        REJECT_INVALID, "bad-txns-premature-spend-of-coinbase");
            }

            // Check for negative or overflow input values
            nValueIn += coins.vout[prevout.n].nValue;
            if (!MoneyRange(coins.vout[prevout.n].nValue) || !MoneyRange(nValueIn))
                return state.DoS(100, error("CheckInputs() : txin values out of range"),
                                 REJECT_INVALID, "bad-txns-inputvalues-outofrange");

        }

        if (nValueIn < tx.GetValueOut())
            return state.DoS(100, error("CheckInputs() : %s value in < value out", tx.GetHash().ToString()),
                             REJECT_INVALID, "bad-txns-in-belowout");

        // Tally transaction fees
        int64_t nTxFee = nValueIn - tx.GetValueOut();
        if (nTxFee < 0)
            return state.DoS(100, error("CheckInputs() : %s nTxFee < 0", tx.GetHash().ToString()),
                             REJECT_INVALID, "bad-txns-fee-negative");
        nFees += nTxFee;
        if (!MoneyRange(nFees))
            return state.DoS(100, error("CheckInputs() : nFees out of range"),
                             REJECT_INVALID, "bad-txns-fee-outofrange");

        // The first loop above does all the inexpensive checks.
        // Only if ALL inputs pass do we perform expensive ECDSA signature checks.
        // Helps prevent CPU exhaustion attacks.

        // Skip ECDSA signature verification when connecting blocks
        // before the last block chain checkpoint. This is safe because block merkle hashes are
        // still computed and checked, and any change will be caught at the next checkpoint.
        if (fScriptChecks) {
            for (unsigned int i = 0; i < tx.vin.size(); i++) {
                const COutPoint &prevout = tx.vin[i].prevout;
                const CCoins &coins = inputs.GetCoins(prevout.hash);

                // Verify signature
                CScriptCheck check(coins, tx, i, flags, 0);
                if (pvChecks) {
                    pvChecks->push_back(CScriptCheck());
                    check.swap(pvChecks->back());
                } else if (!check()) {
                    if (flags & SCRIPT_VERIFY_STRICTENC) {
                        // For now, check whether the failure was caused by non-canonical
                        // encodings or not; if so, don't trigger DoS protection.
                        CScriptCheck check(coins, tx, i, flags & (~SCRIPT_VERIFY_STRICTENC), 0);
                        if (check())
                            return state.Invalid(false, REJECT_NONSTANDARD, "non-canonical");
                    }
                    return state.DoS(100,false, REJECT_NONSTANDARD, "non-canonical");
                }
            }
        }
    }

    return true;
}



bool DisconnectBlock(CBlock& block, CValidationState& state, CBlockIndex* pindex, CCoinsViewCache& view, bool* pfClean)
{
    assert(pindex->GetBlockHash() == view.GetBestBlock());

    if (pfClean)
        *pfClean = false;

    bool fClean = true;

    CBlockUndo blockUndo;
    CDiskBlockPos pos = pindex->GetUndoPos();
    if (pos.IsNull())
        return error("DisconnectBlock() : no undo data available");
    if (!blockUndo.ReadFromDisk(pos, pindex->pprev->GetBlockHash()))
        return error("DisconnectBlock() : failure reading undo data");

    if (blockUndo.vtxundo.size() + 1 != block.vtx.size())
        return error("DisconnectBlock() : block and undo data inconsistent");

    // undo transactions in reverse order
    for (int i = block.vtx.size() - 1; i >= 0; i--) {
        const CTransaction &tx = block.vtx[i];
        uint256 hash = tx.GetHash();

        // Check that all outputs are available and match the outputs in the block itself
        // exactly. Note that transactions with only provably unspendable outputs won't
        // have outputs available even in the block itself, so we handle that case
        // specially with outsEmpty.
        CCoins outsEmpty;
        CCoins &outs = view.HaveCoins(hash) ? view.GetCoins(hash) : outsEmpty;
        outs.ClearUnspendable();

        CCoins outsBlock = CCoins(tx, pindex->nHeight);
        // The CCoins serialization does not serialize negative numbers.
        // No network rules currently depend on the version here, so an inconsistency is harmless
        // but it must be corrected before txout nversion ever influences a network rule.
        if (outsBlock.nVersion < 0)
            outs.nVersion = outsBlock.nVersion;
        if (outs != outsBlock)
            fClean = fClean && error("DisconnectBlock() : added transaction mismatch? database corrupted");

        // remove outputs
        outs = CCoins();

        // restore inputs
        if (i > 0) { // not coinbases
            const CTxUndo &txundo = blockUndo.vtxundo[i-1];
            if (txundo.vprevout.size() != tx.vin.size())
                return error("DisconnectBlock() : transaction and undo data inconsistent");
            for (unsigned int j = tx.vin.size(); j-- > 0;) {
                const COutPoint &out = tx.vin[j].prevout;
                const CTxInUndo &undo = txundo.vprevout[j];
                CCoins coins;
                view.GetCoins(out.hash, coins); // this can fail if the prevout was already entirely spent
                if (undo.nHeight != 0) {
                    // undo data contains height: this is the last output of the prevout tx being spent
                    if (!coins.IsPruned())
                        fClean = fClean && error("DisconnectBlock() : undo data overwriting existing transaction");
                    coins = CCoins();
                    coins.fCoinBase = undo.fCoinBase;
                    coins.nHeight = undo.nHeight;
                    coins.nVersion = undo.nVersion;
                } else {
                    if (coins.IsPruned())
                        fClean = fClean && error("DisconnectBlock() : undo data adding output to missing transaction");
                }
                if (coins.IsAvailable(out.n))
                    fClean = fClean && error("DisconnectBlock() : undo data overwriting existing output");
                if (coins.vout.size() < out.n+1)
                    coins.vout.resize(out.n+1);
                coins.vout[out.n] = undo.txout;
                if (!view.SetCoins(out.hash, coins))
                    return error("DisconnectBlock() : cannot restore coin inputs");
            }
        }
    }

    // move best block pointer to prevout block
    view.SetBestBlock(pindex->pprev->GetBlockHash());

    if (pfClean) {
        *pfClean = fClean;
        return true;
    } else {
        return fClean;
    }
}

void static FlushBlockFile(bool fFinalize = false)
{
    LOCK(cs_LastBlockFile);

    CDiskBlockPos posOld(nLastBlockFile, 0);

    FILE *fileOld = OpenBlockFile(posOld);
    if (fileOld) {
        if (fFinalize)
            TruncateFile(fileOld, infoLastBlockFile.nSize);
        FileCommit(fileOld);
        fclose(fileOld);
    }

    fileOld = OpenUndoFile(posOld);
    if (fileOld) {
        if (fFinalize)
            TruncateFile(fileOld, infoLastBlockFile.nUndoSize);
        FileCommit(fileOld);
        fclose(fileOld);
    }
}

bool FindUndoPos(CValidationState &state, int nFile, CDiskBlockPos &pos, unsigned int nAddSize);

static CCheckQueue<CScriptCheck> scriptcheckqueue(128);

void ThreadScriptCheck() {
    RenameThread("bitcoin-scriptch");
    scriptcheckqueue.Thread();
}

bool ConnectBlock(CBlock& block, CValidationState& state, CBlockIndex* pindex, CCoinsViewCache& view, bool fJustCheck)
{
    // Check it again in case a previous version let a bad block in
    if (!CheckBlock(block, state, !fJustCheck, !fJustCheck))
        return false;

    // verify that the view's current state corresponds to the previous block
    uint256 hashPrevBlock = pindex->pprev == NULL ? uint256(0) : pindex->pprev->GetBlockHash();
    assert(hashPrevBlock == view.GetBestBlock());

    // Special case for the genesis block, skipping connection of its transactions
    // (its coinbase is unspendable)
    if (block.GetHash() == Params().HashGenesisBlock()) {
        view.SetBestBlock(pindex->GetBlockHash());
        return true;
    }

    bool fScriptChecks = pindex->nHeight >= Checkpoints::GetTotalBlocksEstimate();

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
    bool fEnforceBIP30 = (!pindex->phashBlock) || // Enforce on CreateNewBlock invocations which don't have a hash.
                          !((pindex->nHeight==91842 && pindex->GetBlockHash() == uint256("0x00000000000a4d0a398161ffc163c503763b1f4360639393e0e4c8e300e0caec")) ||
                           (pindex->nHeight==91880 && pindex->GetBlockHash() == uint256("0x00000000000743f190a18c5577a3c2d2a1f610ae9601ac046a38084ccb7cd721")));
    if (fEnforceBIP30) {
        for (unsigned int i = 0; i < block.vtx.size(); i++) {
            uint256 hash = block.GetTxHash(i);
            if (view.HaveCoins(hash) && !view.GetCoins(hash).IsPruned())
                return state.DoS(100, error("ConnectBlock() : tried to overwrite transaction"),
                                 REJECT_INVALID, "bad-txns-BIP30");
        }
    }

    // BIP16 didn't become active until Apr 1 2012
    int64_t nBIP16SwitchTime = 1333238400;
    bool fStrictPayToScriptHash = (pindex->nTime >= nBIP16SwitchTime);

    unsigned int flags = SCRIPT_VERIFY_NOCACHE |
                         (fStrictPayToScriptHash ? SCRIPT_VERIFY_P2SH : SCRIPT_VERIFY_NONE);

    CBlockUndo blockundo;

    CCheckQueueControl<CScriptCheck> control(fScriptChecks && nScriptCheckThreads ? &scriptcheckqueue : NULL);

    int64_t nStart = GetTimeMicros();
    int64_t nFees = 0;
    int nInputs = 0;
    unsigned int nSigOps = 0;
    CDiskTxPos pos(pindex->GetBlockPos(), GetSizeOfCompactSize(block.vtx.size()));
    std::vector<std::pair<uint256, CDiskTxPos> > vPos;
    vPos.reserve(block.vtx.size());
    for (unsigned int i = 0; i < block.vtx.size(); i++)
    {
        const CTransaction &tx = block.vtx[i];

        nInputs += tx.vin.size();
        nSigOps += GetLegacySigOpCount(tx);
        if (nSigOps > MAX_BLOCK_SIGOPS)
            return state.DoS(100, error("ConnectBlock() : too many sigops"),
                             REJECT_INVALID, "bad-blk-sigops");

        if (!tx.IsCoinBase())
        {
            if (!view.HaveInputs(tx))
                return state.DoS(100, error("ConnectBlock() : inputs missing/spent"),
                                 REJECT_INVALID, "bad-txns-inputs-missingorspent");

            if (fStrictPayToScriptHash)
            {
                // Add in sigops done by pay-to-script-hash inputs;
                // this is to prevent a "rogue miner" from creating
                // an incredibly-expensive-to-validate block.
                nSigOps += GetP2SHSigOpCount(tx, view);
                if (nSigOps > MAX_BLOCK_SIGOPS)
                    return state.DoS(100, error("ConnectBlock() : too many sigops"),
                                     REJECT_INVALID, "bad-blk-sigops");
            }

            nFees += view.GetValueIn(tx)-tx.GetValueOut();

            std::vector<CScriptCheck> vChecks;
            if (!CheckInputs(tx, state, view, fScriptChecks, flags, nScriptCheckThreads ? &vChecks : NULL))
                return false;
            control.Add(vChecks);
        }

        CTxUndo txundo;
        UpdateCoins(tx, state, view, txundo, pindex->nHeight, block.GetTxHash(i));
        if (!tx.IsCoinBase())
            blockundo.vtxundo.push_back(txundo);

        vPos.push_back(std::make_pair(block.GetTxHash(i), pos));
        pos.nTxOffset += ::GetSerializeSize(tx, SER_DISK, CLIENT_VERSION);
    }
    int64_t nTime = GetTimeMicros() - nStart;
    if (fBenchmark)
        LogPrintf("- Connect %u transactions: %.2fms (%.3fms/tx, %.3fms/txin)\n", (unsigned)block.vtx.size(), 0.001 * nTime, 0.001 * nTime / block.vtx.size(), nInputs <= 1 ? 0 : 0.001 * nTime / (nInputs-1));

    if (block.vtx[0].GetValueOut() > GetBlockValue(pindex->nHeight, nFees))
        return state.DoS(100,
                         error("ConnectBlock() : coinbase pays too much (actual=%d vs limit=%d)",
                               block.vtx[0].GetValueOut(), GetBlockValue(pindex->nHeight, nFees)),
                         REJECT_INVALID, "bad-cb-amount");

    if (!control.Wait())
        return state.DoS(100, false);
    int64_t nTime2 = GetTimeMicros() - nStart;
    if (fBenchmark)
        LogPrintf("- Verify %u txins: %.2fms (%.3fms/txin)\n", nInputs - 1, 0.001 * nTime2, nInputs <= 1 ? 0 : 0.001 * nTime2 / (nInputs-1));

    if (fJustCheck)
        return true;

    // Write undo information to disk
    if (pindex->GetUndoPos().IsNull() || (pindex->nStatus & BLOCK_VALID_MASK) < BLOCK_VALID_SCRIPTS)
    {
        if (pindex->GetUndoPos().IsNull()) {
            CDiskBlockPos pos;
            if (!FindUndoPos(state, pindex->nFile, pos, ::GetSerializeSize(blockundo, SER_DISK, CLIENT_VERSION) + 40))
                return error("ConnectBlock() : FindUndoPos failed");
            if (!blockundo.WriteToDisk(pos, pindex->pprev->GetBlockHash()))
                return state.Abort(_("Failed to write undo data"));

            // update nUndoPos in block index
            pindex->nUndoPos = pos.nPos;
            pindex->nStatus |= BLOCK_HAVE_UNDO;
        }

        pindex->nStatus = (pindex->nStatus & ~BLOCK_VALID_MASK) | BLOCK_VALID_SCRIPTS;

        CDiskBlockIndex blockindex(pindex);
        if (!pblocktree->WriteBlockIndex(blockindex))
            return state.Abort(_("Failed to write block index"));
    }

    if (fTxIndex)
        if (!pblocktree->WriteTxIndex(vPos))
            return state.Abort(_("Failed to write transaction index"));

    // add this block to the view's block chain
    bool ret;
    ret = view.SetBestBlock(pindex->GetBlockHash());
    assert(ret);

    // Watch for transactions paying to me
    for (unsigned int i = 0; i < block.vtx.size(); i++)
        g_signals.SyncTransaction(block.GetTxHash(i), block.vtx[i], &block);

    return true;
}

// Update the on-disk chain state.
bool static WriteChainState(CValidationState &state) {
    static int64_t nLastWrite = 0;
    if (!IsInitialBlockDownload() || pcoinsTip->GetCacheSize() > nCoinCacheSize || GetTimeMicros() > nLastWrite + 600*1000000) {
        // Typical CCoins structures on disk are around 100 bytes in size.
        // Pushing a new one to the database can cause it to be written
        // twice (once in the log, and once in the tables). This is already
        // an overestimation, as most will delete an existing entry or
        // overwrite one. Still, use a conservative safety factor of 2.
        if (!CheckDiskSpace(100 * 2 * 2 * pcoinsTip->GetCacheSize()))
            return state.Error("out of disk space");
        FlushBlockFile();
        pblocktree->Sync();
        if (!pcoinsTip->Flush())
            return state.Abort(_("Failed to write to coin database"));
        nLastWrite = GetTimeMicros();
    }
    return true;
}

// Update chainActive and related internal data structures.
void static UpdateTip(CBlockIndex *pindexNew) {
    chainActive.SetTip(pindexNew);

    // Update best block in wallet (so we can detect restored wallets)
    bool fIsInitialDownload = IsInitialBlockDownload();
    if ((chainActive.Height() % 20160) == 0 || (!fIsInitialDownload && (chainActive.Height() % 144) == 0))
        g_signals.SetBestChain(chainActive.GetLocator());

    // New best block
    nTimeBestReceived = GetTime();
    mempool.AddTransactionsUpdated(1);
    LogPrintf("UpdateTip: new best=%s  height=%d  log2_work=%.8g  tx=%lu  date=%s progress=%f\n",
      chainActive.Tip()->GetBlockHash().ToString(), chainActive.Height(), log(chainActive.Tip()->nChainWork.getdouble())/log(2.0), (unsigned long)chainActive.Tip()->nChainTx,
      DateTimeStrFormat("%Y-%m-%d %H:%M:%S", chainActive.Tip()->GetBlockTime()),
      Checkpoints::GuessVerificationProgress(chainActive.Tip()));

    // Check the version of the last 100 blocks to see if we need to upgrade:
    if (!fIsInitialDownload)
    {
        int nUpgraded = 0;
        const CBlockIndex* pindex = chainActive.Tip();
        for (int i = 0; i < 100 && pindex != NULL; i++)
        {
            if (pindex->nVersion > CBlock::CURRENT_VERSION)
                ++nUpgraded;
            pindex = pindex->pprev;
        }
        if (nUpgraded > 0)
            LogPrintf("SetBestChain: %d of last 100 blocks above version %d\n", nUpgraded, (int)CBlock::CURRENT_VERSION);
        if (nUpgraded > 100/2)
            // strMiscWarning is read by GetWarnings(), called by Qt and the JSON-RPC code to warn the user:
            strMiscWarning = _("Warning: This version is obsolete, upgrade required!");
    }
}

// Disconnect chainActive's tip.
bool static DisconnectTip(CValidationState &state) {
    CBlockIndex *pindexDelete = chainActive.Tip();
    assert(pindexDelete);
    mempool.check(pcoinsTip);
    // Read block from disk.
    CBlock block;
    if (!ReadBlockFromDisk(block, pindexDelete))
        return state.Abort(_("Failed to read block"));
    // Apply the block atomically to the chain state.
    int64_t nStart = GetTimeMicros();
    {
        CCoinsViewCache view(*pcoinsTip, true);
        if (!DisconnectBlock(block, state, pindexDelete, view))
            return error("DisconnectTip() : DisconnectBlock %s failed", pindexDelete->GetBlockHash().ToString());
        assert(view.Flush());
    }
    if (fBenchmark)
        LogPrintf("- Disconnect: %.2fms\n", (GetTimeMicros() - nStart) * 0.001);
    // Write the chain state to disk, if necessary.
    if (!WriteChainState(state))
        return false;
    // Resurrect mempool transactions from the disconnected block.
    BOOST_FOREACH(const CTransaction &tx, block.vtx) {
        // ignore validation errors in resurrected transactions
        list<CTransaction> removed;
        CValidationState stateDummy; 
        if (!tx.IsCoinBase())
            if (!AcceptToMemoryPool(mempool, stateDummy, tx, false, NULL))
                mempool.remove(tx, removed, true);
    }
    mempool.check(pcoinsTip);
    // Update chainActive and related variables.
    UpdateTip(pindexDelete->pprev);
    // Let wallets know transactions went from 1-confirmed to
    // 0-confirmed or conflicted:
    BOOST_FOREACH(const CTransaction &tx, block.vtx) {
        SyncWithWallets(tx.GetHash(), tx, NULL);
    }
    return true;
}

// Connect a new block to chainActive.
bool static ConnectTip(CValidationState &state, CBlockIndex *pindexNew) {
    assert(pindexNew->pprev == chainActive.Tip());
    mempool.check(pcoinsTip);
    // Read block from disk.
    CBlock block;
    if (!ReadBlockFromDisk(block, pindexNew))
        return state.Abort(_("Failed to read block"));
    // Apply the block atomically to the chain state.
    int64_t nStart = GetTimeMicros();
    {
        CCoinsViewCache view(*pcoinsTip, true);
        CInv inv(MSG_BLOCK, pindexNew->GetBlockHash());
        if (!ConnectBlock(block, state, pindexNew, view)) {
            if (state.IsInvalid())
                InvalidBlockFound(pindexNew, state);
            return error("ConnectTip() : ConnectBlock %s failed", pindexNew->GetBlockHash().ToString());
        }
        mapBlockSource.erase(inv.hash);
        assert(view.Flush());
    }
    if (fBenchmark)
        LogPrintf("- Connect: %.2fms\n", (GetTimeMicros() - nStart) * 0.001);
    // Write the chain state to disk, if necessary.
    if (!WriteChainState(state))
        return false;
    // Remove conflicting transactions from the mempool.
    list<CTransaction> txConflicted;
    BOOST_FOREACH(const CTransaction &tx, block.vtx) {
        list<CTransaction> unused;
        mempool.remove(tx, unused);
        mempool.removeConflicts(tx, txConflicted);
    }
    mempool.check(pcoinsTip);
    // Update chainActive & related variables.
    UpdateTip(pindexNew);
    // Tell wallet about transactions that went from mempool
    // to conflicted:
    BOOST_FOREACH(const CTransaction &tx, txConflicted) {
        SyncWithWallets(tx.GetHash(), tx, NULL);
    }
    // ... and about transactions that got confirmed:
    BOOST_FOREACH(const CTransaction &tx, block.vtx) {
        SyncWithWallets(tx.GetHash(), tx, &block);
    }
    return true;
}

// Make chainMostWork correspond to the chain with the most work in it, that isn't
// known to be invalid (it's however far from certain to be valid).
void static FindMostWorkChain() {
    CBlockIndex *pindexNew = NULL;

    // In case the current best is invalid, do not consider it.
    while (chainMostWork.Tip() && (chainMostWork.Tip()->nStatus & BLOCK_FAILED_MASK)) {
        setBlockIndexValid.erase(chainMostWork.Tip());
        chainMostWork.SetTip(chainMostWork.Tip()->pprev);
    }

    do {
        // Find the best candidate header.
        {
            std::set<CBlockIndex*, CBlockIndexWorkComparator>::reverse_iterator it = setBlockIndexValid.rbegin();
            if (it == setBlockIndexValid.rend())
                return;
            pindexNew = *it;
        }

        // Check whether all blocks on the path between the currently active chain and the candidate are valid.
        // Just going until the active chain is an optimization, as we know all blocks in it are valid already.
        CBlockIndex *pindexTest = pindexNew;
        bool fInvalidAncestor = false;
        while (pindexTest && !chainActive.Contains(pindexTest)) {
            if (pindexTest->nStatus & BLOCK_FAILED_MASK) {
                // Candidate has an invalid ancestor, remove entire chain from the set.
                if (pindexBestInvalid == NULL || pindexNew->nChainWork > pindexBestInvalid->nChainWork)
                    pindexBestInvalid = pindexNew;                CBlockIndex *pindexFailed = pindexNew;
                while (pindexTest != pindexFailed) {
                    pindexFailed->nStatus |= BLOCK_FAILED_CHILD;
                    setBlockIndexValid.erase(pindexFailed);
                    pindexFailed = pindexFailed->pprev;
                }
                fInvalidAncestor = true;
                break;
            }
            pindexTest = pindexTest->pprev;
        }
        if (fInvalidAncestor)
            continue;

        break;
    } while(true);

    // Check whether it's actually an improvement.
    if (chainMostWork.Tip() && !CBlockIndexWorkComparator()(chainMostWork.Tip(), pindexNew))
        return;

    // We have a new best.
    chainMostWork.SetTip(pindexNew);
}

// Try to activate to the most-work chain (thereby connecting it).
bool ActivateBestChain(CValidationState &state) {
    CBlockIndex *pindexOldTip = chainActive.Tip();
    bool fComplete = false;
    while (!fComplete) {
        FindMostWorkChain();
        fComplete = true;

        // Check whether we have something to do.
        if (chainMostWork.Tip() == NULL) break;

        // Disconnect active blocks which are no longer in the best chain.
        while (chainActive.Tip() && !chainMostWork.Contains(chainActive.Tip())) {
            if (!DisconnectTip(state))
                return false;
        }

        // Connect new blocks.
        while (!chainActive.Contains(chainMostWork.Tip())) {
            CBlockIndex *pindexConnect = chainMostWork[chainActive.Height() + 1];
            if (!ConnectTip(state, pindexConnect)) {
                if (state.IsInvalid()) {
                    // The block violates a consensus rule.
                    if (!state.CorruptionPossible())
                        InvalidChainFound(chainMostWork.Tip());
                    fComplete = false;
                    state = CValidationState();
                    break;
                } else {
                    // A system error occurred (disk space, database error, ...).
                    return false;
                }
            }
        }
    }

    if (chainActive.Tip() != pindexOldTip) {
        std::string strCmd = GetArg("-blocknotify", "");
        if (!IsInitialBlockDownload() && !strCmd.empty())
        {
            boost::replace_all(strCmd, "%s", chainActive.Tip()->GetBlockHash().GetHex());
            boost::thread t(runCommand, strCmd); // thread runs free
        }
    }

    return true;
}

bool AddToBlockIndex(CBlock& block, CValidationState& state, const CDiskBlockPos& pos)
{
    // Check for duplicate
    uint256 hash = block.GetHash();
    if (mapBlockIndex.count(hash))
        return state.Invalid(error("AddToBlockIndex() : %s already exists", hash.ToString()), 0, "duplicate");

    // Construct new block index object
    CBlockIndex* pindexNew = new CBlockIndex(block);
    {
         LOCK(cs_nBlockSequenceId);
         pindexNew->nSequenceId = nBlockSequenceId++;
    }
    assert(pindexNew);
    mapAlreadyAskedFor.erase(CInv(MSG_BLOCK, hash));
    map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.insert(make_pair(hash, pindexNew)).first;
    pindexNew->phashBlock = &((*mi).first);
    map<uint256, CBlockIndex*>::iterator miPrev = mapBlockIndex.find(block.hashPrevBlock);
    if (miPrev != mapBlockIndex.end())
    {
        pindexNew->pprev = (*miPrev).second;
        pindexNew->nHeight = pindexNew->pprev->nHeight + 1;
    }
    pindexNew->nTx = block.vtx.size();
    pindexNew->nChainWork = (pindexNew->pprev ? pindexNew->pprev->nChainWork : 0) + pindexNew->GetBlockWork().getuint256();
    pindexNew->nChainTx = (pindexNew->pprev ? pindexNew->pprev->nChainTx : 0) + pindexNew->nTx;
    pindexNew->nFile = pos.nFile;
    pindexNew->nDataPos = pos.nPos;
    pindexNew->nUndoPos = 0;
    pindexNew->nStatus = BLOCK_VALID_TRANSACTIONS | BLOCK_HAVE_DATA;
    setBlockIndexValid.insert(pindexNew);

    if (!pblocktree->WriteBlockIndex(CDiskBlockIndex(pindexNew)))
        return state.Abort(_("Failed to write block index"));

    // New best?
    if (!ActivateBestChain(state))
        return false;

    if (pindexNew == chainActive.Tip())
    {
        // Clear fork warning if its no longer applicable
        CheckForkWarningConditions();
        // Notify UI to display prev block's coinbase if it was ours
        static uint256 hashPrevBestCoinBase;
        g_signals.UpdatedTransaction(hashPrevBestCoinBase);
        hashPrevBestCoinBase = block.GetTxHash(0);
    } else
        CheckForkWarningConditionsOnNewFork(pindexNew);

    if (!pblocktree->Flush())
        return state.Abort(_("Failed to sync block index"));

    uiInterface.NotifyBlocksChanged();
    return true;
}


bool FindBlockPos(CValidationState &state, CDiskBlockPos &pos, unsigned int nAddSize, unsigned int nHeight, uint64_t nTime, bool fKnown = false)
{
    bool fUpdatedLast = false;

    LOCK(cs_LastBlockFile);

    if (fKnown) {
        if (nLastBlockFile != pos.nFile) {
            nLastBlockFile = pos.nFile;
            infoLastBlockFile.SetNull();
            pblocktree->ReadBlockFileInfo(nLastBlockFile, infoLastBlockFile);
            fUpdatedLast = true;
        }
    } else {
        while (infoLastBlockFile.nSize + nAddSize >= MAX_BLOCKFILE_SIZE) {
            LogPrintf("Leaving block file %i: %s\n", nLastBlockFile, infoLastBlockFile.ToString());
            FlushBlockFile(true);
            nLastBlockFile++;
            infoLastBlockFile.SetNull();
            pblocktree->ReadBlockFileInfo(nLastBlockFile, infoLastBlockFile); // check whether data for the new file somehow already exist; can fail just fine
            fUpdatedLast = true;
        }
        pos.nFile = nLastBlockFile;
        pos.nPos = infoLastBlockFile.nSize;
    }

    infoLastBlockFile.nSize += nAddSize;
    infoLastBlockFile.AddBlock(nHeight, nTime);

    if (!fKnown) {
        unsigned int nOldChunks = (pos.nPos + BLOCKFILE_CHUNK_SIZE - 1) / BLOCKFILE_CHUNK_SIZE;
        unsigned int nNewChunks = (infoLastBlockFile.nSize + BLOCKFILE_CHUNK_SIZE - 1) / BLOCKFILE_CHUNK_SIZE;
        if (nNewChunks > nOldChunks) {
            if (CheckDiskSpace(nNewChunks * BLOCKFILE_CHUNK_SIZE - pos.nPos)) {
                FILE *file = OpenBlockFile(pos);
                if (file) {
                    LogPrintf("Pre-allocating up to position 0x%x in blk%05u.dat\n", nNewChunks * BLOCKFILE_CHUNK_SIZE, pos.nFile);
                    AllocateFileRange(file, pos.nPos, nNewChunks * BLOCKFILE_CHUNK_SIZE - pos.nPos);
                    fclose(file);
                }
            }
            else
                return state.Error("out of disk space");
        }
    }

    if (!pblocktree->WriteBlockFileInfo(nLastBlockFile, infoLastBlockFile))
        return state.Abort(_("Failed to write file info"));
    if (fUpdatedLast)
        pblocktree->WriteLastBlockFile(nLastBlockFile);

    return true;
}

bool FindUndoPos(CValidationState &state, int nFile, CDiskBlockPos &pos, unsigned int nAddSize)
{
    pos.nFile = nFile;

    LOCK(cs_LastBlockFile);

    unsigned int nNewSize;
    if (nFile == nLastBlockFile) {
        pos.nPos = infoLastBlockFile.nUndoSize;
        nNewSize = (infoLastBlockFile.nUndoSize += nAddSize);
        if (!pblocktree->WriteBlockFileInfo(nLastBlockFile, infoLastBlockFile))
            return state.Abort(_("Failed to write block info"));
    } else {
        CBlockFileInfo info;
        if (!pblocktree->ReadBlockFileInfo(nFile, info))
            return state.Abort(_("Failed to read block info"));
        pos.nPos = info.nUndoSize;
        nNewSize = (info.nUndoSize += nAddSize);
        if (!pblocktree->WriteBlockFileInfo(nFile, info))
            return state.Abort(_("Failed to write block info"));
    }

    unsigned int nOldChunks = (pos.nPos + UNDOFILE_CHUNK_SIZE - 1) / UNDOFILE_CHUNK_SIZE;
    unsigned int nNewChunks = (nNewSize + UNDOFILE_CHUNK_SIZE - 1) / UNDOFILE_CHUNK_SIZE;
    if (nNewChunks > nOldChunks) {
        if (CheckDiskSpace(nNewChunks * UNDOFILE_CHUNK_SIZE - pos.nPos)) {
            FILE *file = OpenUndoFile(pos);
            if (file) {
                LogPrintf("Pre-allocating up to position 0x%x in rev%05u.dat\n", nNewChunks * UNDOFILE_CHUNK_SIZE, pos.nFile);
                AllocateFileRange(file, pos.nPos, nNewChunks * UNDOFILE_CHUNK_SIZE - pos.nPos);
                fclose(file);
            }
        }
        else
            return state.Error("out of disk space");
    }

    return true;
}


bool CheckBlock(const CBlock& block, CValidationState& state, bool fCheckPOW, bool fCheckMerkleRoot)
{
    // These are checks that are independent of context
    // that can be verified before saving an orphan block.

    // Size limits
    if (block.vtx.empty() || block.vtx.size() > MAX_BLOCK_SIZE || ::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION) > MAX_BLOCK_SIZE)
        return state.DoS(100, error("CheckBlock() : size limits failed"),
                         REJECT_INVALID, "bad-blk-length");

    // Check proof of work matches claimed amount
    if (fCheckPOW && !CheckProofOfWork(block.GetHash(), block.nBits))
        return state.DoS(50, error("CheckBlock() : proof of work failed"),
                         REJECT_INVALID, "high-hash");

    // Check timestamp
    if (block.GetBlockTime() > GetAdjustedTime() + 2 * 60 * 60)
        return state.Invalid(error("CheckBlock() : block timestamp too far in the future"),
                             REJECT_INVALID, "time-too-new");

    // First transaction must be coinbase, the rest must not be
    if (block.vtx.empty() || !block.vtx[0].IsCoinBase())
        return state.DoS(100, error("CheckBlock() : first tx is not coinbase"),
                         REJECT_INVALID, "bad-cb-missing");
    for (unsigned int i = 1; i < block.vtx.size(); i++)
        if (block.vtx[i].IsCoinBase())
            return state.DoS(100, error("CheckBlock() : more than one coinbase"),
                             REJECT_INVALID, "bad-cb-multiple");

    // Check transactions
    BOOST_FOREACH(const CTransaction& tx, block.vtx)
        if (!CheckTransaction(tx, state))
            return error("CheckBlock() : CheckTransaction failed");

    // Build the merkle tree already. We need it anyway later, and it makes the
    // block cache the transaction hashes, which means they don't need to be
    // recalculated many times during this block's validation.
    block.BuildMerkleTree();

    // Check for duplicate txids. This is caught by ConnectInputs(),
    // but catching it earlier avoids a potential DoS attack:
    set<uint256> uniqueTx;
    for (unsigned int i = 0; i < block.vtx.size(); i++) {
        uniqueTx.insert(block.GetTxHash(i));
    }
    if (uniqueTx.size() != block.vtx.size())
        return state.DoS(100, error("CheckBlock() : duplicate transaction"),
                         REJECT_INVALID, "bad-txns-duplicate", true);

    unsigned int nSigOps = 0;
    BOOST_FOREACH(const CTransaction& tx, block.vtx)
    {
        nSigOps += GetLegacySigOpCount(tx);
    }
    if (nSigOps > MAX_BLOCK_SIGOPS)
        return state.DoS(100, error("CheckBlock() : out-of-bounds SigOpCount"),
                         REJECT_INVALID, "bad-blk-sigops", true);

    // Check merkle root
    if (fCheckMerkleRoot && block.hashMerkleRoot != block.vMerkleTree.back())
        return state.DoS(100, error("CheckBlock() : hashMerkleRoot mismatch"),
                         REJECT_INVALID, "bad-txnmrklroot", true);

    return true;
}

bool AcceptBlock(CBlock& block, CValidationState& state, CDiskBlockPos* dbp)
{
    // Check for duplicate
    uint256 hash = block.GetHash();
    if (mapBlockIndex.count(hash))
        return state.Invalid(error("AcceptBlock() : block already in mapBlockIndex"), 0, "duplicate");

    // Get prev block index
    CBlockIndex* pindexPrev = NULL;
    int nHeight = 0;
    if (hash != Params().HashGenesisBlock()) {
        map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(block.hashPrevBlock);
        if (mi == mapBlockIndex.end())
            return state.DoS(10, error("AcceptBlock() : prev block not found"), 0, "bad-prevblk");
        pindexPrev = (*mi).second;
        nHeight = pindexPrev->nHeight+1;

        // Check proof of work
        if (block.nBits != GetNextWorkRequired(pindexPrev, &block))
            return state.DoS(100, error("AcceptBlock() : incorrect proof of work"),
                             REJECT_INVALID, "bad-diffbits");

        // Check timestamp against prev
        if (block.GetBlockTime() <= pindexPrev->GetMedianTimePast())
            return state.Invalid(error("AcceptBlock() : block's timestamp is too early"),
                                 REJECT_INVALID, "time-too-old");

        // Check that all transactions are finalized
        BOOST_FOREACH(const CTransaction& tx, block.vtx)
            if (!IsFinalTx(tx, nHeight, block.GetBlockTime()))
                return state.DoS(10, error("AcceptBlock() : contains a non-final transaction"),
                                 REJECT_INVALID, "bad-txns-nonfinal");

        // Check that the block chain matches the known block chain up to a checkpoint
        if (!Checkpoints::CheckBlock(nHeight, hash))
            return state.DoS(100, error("AcceptBlock() : rejected by checkpoint lock-in at %d", nHeight),
                             REJECT_CHECKPOINT, "checkpoint mismatch");

        // Don't accept any forks from the main chain prior to last checkpoint
        CBlockIndex* pcheckpoint = Checkpoints::GetLastCheckpoint(mapBlockIndex);
        if (pcheckpoint && nHeight < pcheckpoint->nHeight)
            return state.DoS(100, error("AcceptBlock() : forked chain older than last checkpoint (height %d)", nHeight));

        // Reject block.nVersion=1 blocks when 95% (75% on testnet) of the network has upgraded:
        if (block.nVersion < 2)
        {
            if ((!TestNet() && CBlockIndex::IsSuperMajority(2, pindexPrev, 950, 1000)) ||
                (TestNet() && CBlockIndex::IsSuperMajority(2, pindexPrev, 75, 100)))
            {
                return state.Invalid(error("AcceptBlock() : rejected nVersion=1 block"),
                                     REJECT_OBSOLETE, "bad-version");
            }
        }
        // Enforce block.nVersion=2 rule that the coinbase starts with serialized block height
        if (block.nVersion >= 2)
        {
            // if 750 of the last 1,000 blocks are version 2 or greater (51/100 if testnet):
            if ((!TestNet() && CBlockIndex::IsSuperMajority(2, pindexPrev, 750, 1000)) ||
                (TestNet() && CBlockIndex::IsSuperMajority(2, pindexPrev, 51, 100)))
            {
                CScript expect = CScript() << nHeight;
                if (block.vtx[0].vin[0].scriptSig.size() < expect.size() ||
                    !std::equal(expect.begin(), expect.end(), block.vtx[0].vin[0].scriptSig.begin()))
                    return state.DoS(100, error("AcceptBlock() : block height mismatch in coinbase"),
                                     REJECT_INVALID, "bad-cb-height");
            }
        }
    }

    // Write block to history file
    try {
        unsigned int nBlockSize = ::GetSerializeSize(block, SER_DISK, CLIENT_VERSION);
        CDiskBlockPos blockPos;
        if (dbp != NULL)
            blockPos = *dbp;
        if (!FindBlockPos(state, blockPos, nBlockSize+8, nHeight, block.nTime, dbp != NULL))
            return error("AcceptBlock() : FindBlockPos failed");
        if (dbp == NULL)
            if (!WriteBlockToDisk(block, blockPos))
                return state.Abort(_("Failed to write block"));
        if (!AddToBlockIndex(block, state, blockPos))
            return error("AcceptBlock() : AddToBlockIndex failed");
    } catch(std::runtime_error &e) {
        return state.Abort(_("System error: ") + e.what());
    }

    // Relay inventory, but don't relay old inventory during initial block download
    int nBlockEstimate = Checkpoints::GetTotalBlocksEstimate();
    if (chainActive.Tip()->GetBlockHash() == hash)
    {
        LOCK(cs_vNodes);
        BOOST_FOREACH(CNode* pnode, vNodes)
            if (chainActive.Height() > (pnode->nStartingHeight != -1 ? pnode->nStartingHeight - 2000 : nBlockEstimate))
                pnode->PushInventory(CInv(MSG_BLOCK, hash));
    }

    return true;
}

bool CBlockIndex::IsSuperMajority(int minVersion, const CBlockIndex* pstart, unsigned int nRequired, unsigned int nToCheck)
{
    unsigned int nFound = 0;
    for (unsigned int i = 0; i < nToCheck && nFound < nRequired && pstart != NULL; i++)
    {
        if (pstart->nVersion >= minVersion)
            ++nFound;
        pstart = pstart->pprev;
    }
    return (nFound >= nRequired);
}

int64_t CBlockIndex::GetMedianTime() const
{
    const CBlockIndex* pindex = this;
    for (int i = 0; i < nMedianTimeSpan/2; i++)
    {
        if (!chainActive.Next(pindex))
            return GetBlockTime();
        pindex = chainActive.Next(pindex);
    }
    return pindex->GetMedianTimePast();
}

void PushGetBlocks(CNode* pnode, CBlockIndex* pindexBegin, uint256 hashEnd)
{
    // Filter out duplicate requests
    if (pindexBegin == pnode->pindexLastGetBlocksBegin && hashEnd == pnode->hashLastGetBlocksEnd)
        return;
    pnode->pindexLastGetBlocksBegin = pindexBegin;
    pnode->hashLastGetBlocksEnd = hashEnd;

    pnode->PushMessage("getblocks", chainActive.GetLocator(pindexBegin), hashEnd);
}

bool ProcessBlock(CValidationState &state, CNode* pfrom, CBlock* pblock, CDiskBlockPos *dbp)
{
    AssertLockHeld(cs_main);

    // Check for duplicate
    uint256 hash = pblock->GetHash();
    if (mapBlockIndex.count(hash))
        return state.Invalid(error("ProcessBlock() : already have block %d %s", mapBlockIndex[hash]->nHeight, hash.ToString()), 0, "duplicate");
    if (mapOrphanBlocks.count(hash))
        return state.Invalid(error("ProcessBlock() : already have block (orphan) %s", hash.ToString()), 0, "duplicate");

    // Preliminary checks
    if (!CheckBlock(*pblock, state)) {
        if (state.CorruptionPossible())
            mapAlreadyAskedFor.erase(CInv(MSG_BLOCK, hash));
        return error("ProcessBlock() : CheckBlock FAILED");
    }

    CBlockIndex* pcheckpoint = Checkpoints::GetLastCheckpoint(mapBlockIndex);
    if (pcheckpoint && pblock->hashPrevBlock != (chainActive.Tip() ? chainActive.Tip()->GetBlockHash() : uint256(0)))
    {
        // Extra checks to prevent "fill up memory by spamming with bogus blocks"
        int64_t deltaTime = pblock->GetBlockTime() - pcheckpoint->nTime;
        if (deltaTime < 0)
        {
            return state.DoS(100, error("ProcessBlock() : block with timestamp before last checkpoint"),
                             REJECT_CHECKPOINT, "time-too-old");
        }
        CBigNum bnNewBlock;
        bnNewBlock.SetCompact(pblock->nBits);
        CBigNum bnRequired;
        bnRequired.SetCompact(ComputeMinWork(pcheckpoint->nBits, deltaTime));
        if (bnNewBlock > bnRequired)
        {
            return state.DoS(100, error("ProcessBlock() : block with too little proof-of-work"),
                             REJECT_INVALID, "bad-diffbits");
        }
    }


    // If we don't already have its previous block, shunt it off to holding area until we get it
    if (pblock->hashPrevBlock != 0 && !mapBlockIndex.count(pblock->hashPrevBlock))
    {
        LogPrintf("ProcessBlock: ORPHAN BLOCK %lu, prev=%s\n", (unsigned long)mapOrphanBlocks.size(), pblock->hashPrevBlock.ToString());

        // Accept orphans as long as there is a node to request its parents from
        if (pfrom) {
            PruneOrphanBlocks();
            COrphanBlock* pblock2 = new COrphanBlock();
            {
                CDataStream ss(SER_DISK, CLIENT_VERSION);
                ss << *pblock;
                pblock2->vchBlock = std::vector<unsigned char>(ss.begin(), ss.end());
            }
            pblock2->hashBlock = hash;
            pblock2->hashPrev = pblock->hashPrevBlock;
            mapOrphanBlocks.insert(make_pair(hash, pblock2));
            mapOrphanBlocksByPrev.insert(make_pair(pblock2->hashPrev, pblock2));

            // Ask this guy to fill in what we're missing
            PushGetBlocks(pfrom, chainActive.Tip(), GetOrphanRoot(hash));
        }
        return true;
    }

    // Store to disk
    if (!AcceptBlock(*pblock, state, dbp))
        return error("ProcessBlock() : AcceptBlock FAILED");

    // Recursively process any orphan blocks that depended on this one
    vector<uint256> vWorkQueue;
    vWorkQueue.push_back(hash);
    for (unsigned int i = 0; i < vWorkQueue.size(); i++)
    {
        uint256 hashPrev = vWorkQueue[i];
        for (multimap<uint256, COrphanBlock*>::iterator mi = mapOrphanBlocksByPrev.lower_bound(hashPrev);
             mi != mapOrphanBlocksByPrev.upper_bound(hashPrev);
             ++mi)
        {
            CBlock block;
            {
                CDataStream ss(mi->second->vchBlock, SER_DISK, CLIENT_VERSION);
                ss >> block;
            }
            block.BuildMerkleTree();
            // Use a dummy CValidationState so someone can't setup nodes to counter-DoS based on orphan resolution (that is, feeding people an invalid block based on LegitBlockX in order to get anyone relaying LegitBlockX banned)
            CValidationState stateDummy;
            if (AcceptBlock(block, stateDummy))
                vWorkQueue.push_back(mi->second->hashBlock);
            mapOrphanBlocks.erase(mi->second->hashBlock);
            delete mi->second;
        }
        mapOrphanBlocksByPrev.erase(hashPrev);
    }

    LogPrintf("ProcessBlock: ACCEPTED\n");
    return true;
}








CMerkleBlock::CMerkleBlock(const CBlock& block, CBloomFilter& filter)
{
    header = block.GetBlockHeader();

    vector<bool> vMatch;
    vector<uint256> vHashes;

    vMatch.reserve(block.vtx.size());
    vHashes.reserve(block.vtx.size());

    for (unsigned int i = 0; i < block.vtx.size(); i++)
    {
        uint256 hash = block.vtx[i].GetHash();
        if (filter.IsRelevantAndUpdate(block.vtx[i], hash))
        {
            vMatch.push_back(true);
            vMatchedTxn.push_back(make_pair(i, hash));
        }
        else
            vMatch.push_back(false);
        vHashes.push_back(hash);
    }

    txn = CPartialMerkleTree(vHashes, vMatch);
}








uint256 CPartialMerkleTree::CalcHash(int height, unsigned int pos, const std::vector<uint256> &vTxid) {
    if (height == 0) {
        // hash at height 0 is the txids themself
        return vTxid[pos];
    } else {
        // calculate left hash
        uint256 left = CalcHash(height-1, pos*2, vTxid), right;
        // calculate right hash if not beyong the end of the array - copy left hash otherwise1
        if (pos*2+1 < CalcTreeWidth(height-1))
            right = CalcHash(height-1, pos*2+1, vTxid);
        else
            right = left;
        // combine subhashes
        return Hash(BEGIN(left), END(left), BEGIN(right), END(right));
    }
}

void CPartialMerkleTree::TraverseAndBuild(int height, unsigned int pos, const std::vector<uint256> &vTxid, const std::vector<bool> &vMatch) {
    // determine whether this node is the parent of at least one matched txid
    bool fParentOfMatch = false;
    for (unsigned int p = pos << height; p < (pos+1) << height && p < nTransactions; p++)
        fParentOfMatch |= vMatch[p];
    // store as flag bit
    vBits.push_back(fParentOfMatch);
    if (height==0 || !fParentOfMatch) {
        // if at height 0, or nothing interesting below, store hash and stop
        vHash.push_back(CalcHash(height, pos, vTxid));
    } else {
        // otherwise, don't store any hash, but descend into the subtrees
        TraverseAndBuild(height-1, pos*2, vTxid, vMatch);
        if (pos*2+1 < CalcTreeWidth(height-1))
            TraverseAndBuild(height-1, pos*2+1, vTxid, vMatch);
    }
}

uint256 CPartialMerkleTree::TraverseAndExtract(int height, unsigned int pos, unsigned int &nBitsUsed, unsigned int &nHashUsed, std::vector<uint256> &vMatch) {
    if (nBitsUsed >= vBits.size()) {
        // overflowed the bits array - failure
        fBad = true;
        return 0;
    }
    bool fParentOfMatch = vBits[nBitsUsed++];
    if (height==0 || !fParentOfMatch) {
        // if at height 0, or nothing interesting below, use stored hash and do not descend
        if (nHashUsed >= vHash.size()) {
            // overflowed the hash array - failure
            fBad = true;
            return 0;
        }
        const uint256 &hash = vHash[nHashUsed++];
        if (height==0 && fParentOfMatch) // in case of height 0, we have a matched txid
            vMatch.push_back(hash);
        return hash;
    } else {
        // otherwise, descend into the subtrees to extract matched txids and hashes
        uint256 left = TraverseAndExtract(height-1, pos*2, nBitsUsed, nHashUsed, vMatch), right;
        if (pos*2+1 < CalcTreeWidth(height-1))
            right = TraverseAndExtract(height-1, pos*2+1, nBitsUsed, nHashUsed, vMatch);
        else
            right = left;
        // and combine them before returning
        return Hash(BEGIN(left), END(left), BEGIN(right), END(right));
    }
}

CPartialMerkleTree::CPartialMerkleTree(const std::vector<uint256> &vTxid, const std::vector<bool> &vMatch) : nTransactions(vTxid.size()), fBad(false) {
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

uint256 CPartialMerkleTree::ExtractMatches(std::vector<uint256> &vMatch) {
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
}







bool AbortNode(const std::string &strMessage) {
    strMiscWarning = strMessage;
    LogPrintf("*** %s\n", strMessage);
    uiInterface.ThreadSafeMessageBox(strMessage, "", CClientUIInterface::MSG_ERROR);
    StartShutdown();
    return false;
}

bool CheckDiskSpace(uint64_t nAdditionalBytes)
{
    uint64_t nFreeBytesAvailable = filesystem::space(GetDataDir()).available;

    // Check for nMinDiskSpace bytes (currently 50MB)
    if (nFreeBytesAvailable < nMinDiskSpace + nAdditionalBytes)
        return AbortNode(_("Error: Disk space is low!"));

    return true;
}

FILE* OpenDiskFile(const CDiskBlockPos &pos, const char *prefix, bool fReadOnly)
{
    if (pos.IsNull())
        return NULL;
    boost::filesystem::path path = GetDataDir() / "blocks" / strprintf("%s%05u.dat", prefix, pos.nFile);
    boost::filesystem::create_directories(path.parent_path());
    FILE* file = fopen(path.string().c_str(), "rb+");
    if (!file && !fReadOnly)
        file = fopen(path.string().c_str(), "wb+");
    if (!file) {
        LogPrintf("Unable to open file %s\n", path.string());
        return NULL;
    }
    if (pos.nPos) {
        if (fseek(file, pos.nPos, SEEK_SET)) {
            LogPrintf("Unable to seek to position %u of %s\n", pos.nPos, path.string());
            fclose(file);
            return NULL;
        }
    }
    return file;
}

FILE* OpenBlockFile(const CDiskBlockPos &pos, bool fReadOnly) {
    return OpenDiskFile(pos, "blk", fReadOnly);
}

FILE* OpenUndoFile(const CDiskBlockPos &pos, bool fReadOnly) {
    return OpenDiskFile(pos, "rev", fReadOnly);
}

CBlockIndex * InsertBlockIndex(uint256 hash)
{
    if (hash == 0)
        return NULL;

    // Return existing
    map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hash);
    if (mi != mapBlockIndex.end())
        return (*mi).second;

    // Create new
    CBlockIndex* pindexNew = new CBlockIndex();
    if (!pindexNew)
        throw runtime_error("LoadBlockIndex() : new CBlockIndex failed");
    mi = mapBlockIndex.insert(make_pair(hash, pindexNew)).first;
    pindexNew->phashBlock = &((*mi).first);

    return pindexNew;
}

bool static LoadBlockIndexDB()
{
    if (!pblocktree->LoadBlockIndexGuts())
        return false;

    boost::this_thread::interruption_point();

    // Calculate nChainWork
    vector<pair<int, CBlockIndex*> > vSortedByHeight;
    vSortedByHeight.reserve(mapBlockIndex.size());
    BOOST_FOREACH(const PAIRTYPE(uint256, CBlockIndex*)& item, mapBlockIndex)
    {
        CBlockIndex* pindex = item.second;
        vSortedByHeight.push_back(make_pair(pindex->nHeight, pindex));
    }
    sort(vSortedByHeight.begin(), vSortedByHeight.end());
    BOOST_FOREACH(const PAIRTYPE(int, CBlockIndex*)& item, vSortedByHeight)
    {
        CBlockIndex* pindex = item.second;
        pindex->nChainWork = (pindex->pprev ? pindex->pprev->nChainWork : 0) + pindex->GetBlockWork().getuint256();
        pindex->nChainTx = (pindex->pprev ? pindex->pprev->nChainTx : 0) + pindex->nTx;
        if ((pindex->nStatus & BLOCK_VALID_MASK) >= BLOCK_VALID_TRANSACTIONS && !(pindex->nStatus & BLOCK_FAILED_MASK))
            setBlockIndexValid.insert(pindex);
        if (pindex->nStatus & BLOCK_FAILED_MASK && (!pindexBestInvalid || pindex->nChainWork > pindexBestInvalid->nChainWork))
            pindexBestInvalid = pindex;
    }

    // Load block file info
    pblocktree->ReadLastBlockFile(nLastBlockFile);
    LogPrintf("LoadBlockIndexDB(): last block file = %i\n", nLastBlockFile);
    if (pblocktree->ReadBlockFileInfo(nLastBlockFile, infoLastBlockFile))
        LogPrintf("LoadBlockIndexDB(): last block file info: %s\n", infoLastBlockFile.ToString());

    // Check whether we need to continue reindexing
    bool fReindexing = false;
    pblocktree->ReadReindexing(fReindexing);
    fReindex |= fReindexing;

    // Check whether we have a transaction index
    pblocktree->ReadFlag("txindex", fTxIndex);
    LogPrintf("LoadBlockIndexDB(): transaction index %s\n", fTxIndex ? "enabled" : "disabled");

    // Load pointer to end of best chain
    std::map<uint256, CBlockIndex*>::iterator it = mapBlockIndex.find(pcoinsTip->GetBestBlock());
    if (it == mapBlockIndex.end())
        return true;
    chainActive.SetTip(it->second);
    LogPrintf("LoadBlockIndexDB(): hashBestChain=%s height=%d date=%s progress=%f\n",
        chainActive.Tip()->GetBlockHash().ToString(), chainActive.Height(),
        DateTimeStrFormat("%Y-%m-%d %H:%M:%S", chainActive.Tip()->GetBlockTime()),
        Checkpoints::GuessVerificationProgress(chainActive.Tip()));

    return true;
}

bool VerifyDB(int nCheckLevel, int nCheckDepth)
{
    if (chainActive.Tip() == NULL || chainActive.Tip()->pprev == NULL)
        return true;

    // Verify blocks in the best chain
    if (nCheckDepth <= 0)
        nCheckDepth = 1000000000; // suffices until the year 19000
    if (nCheckDepth > chainActive.Height())
        nCheckDepth = chainActive.Height();
    nCheckLevel = std::max(0, std::min(4, nCheckLevel));
    LogPrintf("Verifying last %i blocks at level %i\n", nCheckDepth, nCheckLevel);
    CCoinsViewCache coins(*pcoinsTip, true);
    CBlockIndex* pindexState = chainActive.Tip();
    CBlockIndex* pindexFailure = NULL;
    int nGoodTransactions = 0;
    CValidationState state;
    for (CBlockIndex* pindex = chainActive.Tip(); pindex && pindex->pprev; pindex = pindex->pprev)
    {
        boost::this_thread::interruption_point();
        if (pindex->nHeight < chainActive.Height()-nCheckDepth)
            break;
        CBlock block;
        // check level 0: read from disk
        if (!ReadBlockFromDisk(block, pindex))
            return error("VerifyDB() : *** ReadBlockFromDisk failed at %d, hash=%s", pindex->nHeight, pindex->GetBlockHash().ToString());
        // check level 1: verify block validity
        if (nCheckLevel >= 1 && !CheckBlock(block, state))
            return error("VerifyDB() : *** found bad block at %d, hash=%s\n", pindex->nHeight, pindex->GetBlockHash().ToString());
        // check level 2: verify undo validity
        if (nCheckLevel >= 2 && pindex) {
            CBlockUndo undo;
            CDiskBlockPos pos = pindex->GetUndoPos();
            if (!pos.IsNull()) {
                if (!undo.ReadFromDisk(pos, pindex->pprev->GetBlockHash()))
                    return error("VerifyDB() : *** found bad undo data at %d, hash=%s\n", pindex->nHeight, pindex->GetBlockHash().ToString());
            }
        }
        // check level 3: check for inconsistencies during memory-only disconnect of tip blocks
        if (nCheckLevel >= 3 && pindex == pindexState && (coins.GetCacheSize() + pcoinsTip->GetCacheSize()) <= 2*nCoinCacheSize + 32000) {
            bool fClean = true;
            if (!DisconnectBlock(block, state, pindex, coins, &fClean))
                return error("VerifyDB() : *** irrecoverable inconsistency in block data at %d, hash=%s", pindex->nHeight, pindex->GetBlockHash().ToString());
            pindexState = pindex->pprev;
            if (!fClean) {
                nGoodTransactions = 0;
                pindexFailure = pindex;
            } else
                nGoodTransactions += block.vtx.size();
        }
    }
    if (pindexFailure)
        return error("VerifyDB() : *** coin database inconsistencies found (last %i blocks, %i good transactions before that)\n", chainActive.Height() - pindexFailure->nHeight + 1, nGoodTransactions);

    // check level 4: try reconnecting blocks
    if (nCheckLevel >= 4) {
        CBlockIndex *pindex = pindexState;
        while (pindex != chainActive.Tip()) {
            boost::this_thread::interruption_point();
            pindex = chainActive.Next(pindex);
            CBlock block;
            if (!ReadBlockFromDisk(block, pindex))
                return error("VerifyDB() : *** ReadBlockFromDisk failed at %d, hash=%s", pindex->nHeight, pindex->GetBlockHash().ToString());
            if (!ConnectBlock(block, state, pindex, coins))
                return error("VerifyDB() : *** found unconnectable block at %d, hash=%s", pindex->nHeight, pindex->GetBlockHash().ToString());
        }
    }

    LogPrintf("No coin database inconsistencies in last %i blocks (%i transactions)\n", chainActive.Height() - pindexState->nHeight, nGoodTransactions);

    return true;
}

void UnloadBlockIndex()
{
    mapBlockIndex.clear();
    setBlockIndexValid.clear();
    chainActive.SetTip(NULL);
    pindexBestInvalid = NULL;
}

bool LoadBlockIndex()
{
    // Load block index from databases
    if (!fReindex && !LoadBlockIndexDB())
        return false;
    return true;
}


bool InitBlockIndex() {
    // Check whether we're already initialized
    if (chainActive.Genesis() != NULL)
        return true;

    // Use the provided setting for -txindex in the new database
    fTxIndex = GetBoolArg("-txindex", false);
    pblocktree->WriteFlag("txindex", fTxIndex);
    LogPrintf("Initializing databases...\n");

    // Only add the genesis block if not reindexing (in which case we reuse the one already on disk)
    if (!fReindex) {
        try {
            CBlock &block = const_cast<CBlock&>(Params().GenesisBlock());
            // Start new block file
            unsigned int nBlockSize = ::GetSerializeSize(block, SER_DISK, CLIENT_VERSION);
            CDiskBlockPos blockPos;
            CValidationState state;
            if (!FindBlockPos(state, blockPos, nBlockSize+8, 0, block.nTime))
                return error("LoadBlockIndex() : FindBlockPos failed");
            if (!WriteBlockToDisk(block, blockPos))
                return error("LoadBlockIndex() : writing genesis block to disk failed");
            if (!AddToBlockIndex(block, state, blockPos))
                return error("LoadBlockIndex() : genesis block not accepted");
        } catch(std::runtime_error &e) {
            return error("LoadBlockIndex() : failed to initialize block database: %s", e.what());
        }
    }

    return true;
}



void PrintBlockTree()
{
    // pre-compute tree structure
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
    vStack.push_back(make_pair(0, chainActive.Genesis()));

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
        }
        else if (nCol < nPrevCol)
        {
            for (int i = 0; i < nCol; i++)
                LogPrintf("| ");
            LogPrintf("|\n");
       }
        nPrevCol = nCol;

        // print columns
        for (int i = 0; i < nCol; i++)
            LogPrintf("| ");

        // print item
        CBlock block;
        ReadBlockFromDisk(block, pindex);
        LogPrintf("%d (blk%05u.dat:0x%x)  %s  tx %"PRIszu"",
            pindex->nHeight,
            pindex->GetBlockPos().nFile, pindex->GetBlockPos().nPos,
            DateTimeStrFormat("%Y-%m-%d %H:%M:%S", block.GetBlockTime()),
            block.vtx.size());

        // put the main time-chain first
        vector<CBlockIndex*>& vNext = mapNext[pindex];
        for (unsigned int i = 0; i < vNext.size(); i++)
        {
            if (chainActive.Next(vNext[i]))
            {
                swap(vNext[0], vNext[i]);
                break;
            }
        }

        // iterate children
        for (unsigned int i = 0; i < vNext.size(); i++)
            vStack.push_back(make_pair(nCol+i, vNext[i]));
    }
}

bool LoadExternalBlockFile(FILE* fileIn, CDiskBlockPos *dbp)
{
    int64_t nStart = GetTimeMillis();

    int nLoaded = 0;
    try {
        CBufferedFile blkdat(fileIn, 2*MAX_BLOCK_SIZE, MAX_BLOCK_SIZE+8, SER_DISK, CLIENT_VERSION);
        uint64_t nStartByte = 0;
        if (dbp) {
            // (try to) skip already indexed part
            CBlockFileInfo info;
            if (pblocktree->ReadBlockFileInfo(dbp->nFile, info)) {
                nStartByte = info.nSize;
                blkdat.Seek(info.nSize);
            }
        }
        uint64_t nRewind = blkdat.GetPos();
        while (blkdat.good() && !blkdat.eof()) {
            boost::this_thread::interruption_point();

            blkdat.SetPos(nRewind);
            nRewind++; // start one byte further next time, in case of failure
            blkdat.SetLimit(); // remove former limit
            unsigned int nSize = 0;
            try {
                // locate a header
                unsigned char buf[4];
                blkdat.FindByte(Params().MessageStart()[0]);
                nRewind = blkdat.GetPos()+1;
                blkdat >> FLATDATA(buf);
                if (memcmp(buf, Params().MessageStart(), 4))
                    continue;
                // read size
                blkdat >> nSize;
                if (nSize < 80 || nSize > MAX_BLOCK_SIZE)
                    continue;
            } catch (std::exception &e) {
                // no valid block header found; don't complain
                break;
            }
            try {
                // read block
                uint64_t nBlockPos = blkdat.GetPos();
                blkdat.SetLimit(nBlockPos + nSize);
                CBlock block;
                blkdat >> block;
                nRewind = blkdat.GetPos();

                // process block
                if (nBlockPos >= nStartByte) {
                    LOCK(cs_main);
                    if (dbp)
                        dbp->nPos = nBlockPos;
                    CValidationState state;
                    if (ProcessBlock(state, NULL, &block, dbp))
                        nLoaded++;
                    if (state.IsError())
                        break;
                }
            } catch (std::exception &e) {
                LogPrintf("%s : Deserialize or I/O error - %s", __PRETTY_FUNCTION__, e.what());
            }
        }
        fclose(fileIn);
    } catch(std::runtime_error &e) {
        AbortNode(_("Error: system error: ") + e.what());
    }
    if (nLoaded > 0)
        LogPrintf("Loaded %i blocks from external file in %dms\n", nLoaded, GetTimeMillis() - nStart);
    return nLoaded > 0;
}










//////////////////////////////////////////////////////////////////////////////
//
// CAlert
//

string GetWarnings(string strFor)
{
    int nPriority = 0;
    string strStatusBar;
    string strRPC;

    if (GetBoolArg("-testsafemode", false))
        strRPC = "test";

    if (!CLIENT_VERSION_IS_RELEASE)
        strStatusBar = _("This is a pre-release test build - use at your own risk - do not use for mining or merchant applications");

    // Misc warnings like out of disk space and clock is wrong
    if (strMiscWarning != "")
    {
        nPriority = 1000;
        strStatusBar = strMiscWarning;
    }

    if (fLargeWorkForkFound)
    {
        nPriority = 2000;
        strStatusBar = strRPC = _("Warning: The network does not appear to fully agree! Some miners appear to be experiencing issues.");
    }
    else if (fLargeWorkInvalidChainFound)
    {
        nPriority = 2000;
        strStatusBar = strRPC = _("Warning: We do not appear to fully agree with our peers! You may need to upgrade, or other nodes may need to upgrade.");
    }

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
            }
        }
    }

    if (strFor == "statusbar")
        return strStatusBar;
    else if (strFor == "rpc")
        return strRPC;
    assert(!"GetWarnings() : invalid parameter");
    return "error";
}








//////////////////////////////////////////////////////////////////////////////
//
// Messages
//


bool static AlreadyHave(const CInv& inv)
{
    switch (inv.type)
    {
    case MSG_TX:
        {
            bool txInMap = false;
            txInMap = mempool.exists(inv.hash);
            return txInMap || mapOrphanTransactions.count(inv.hash) ||
                pcoinsTip->HaveCoins(inv.hash);
        }
    case MSG_BLOCK:
        return mapBlockIndex.count(inv.hash) ||
               mapOrphanBlocks.count(inv.hash);
    }
    // Don't know what it is, just say we already got one
    return true;
}


void static ProcessGetData(CNode* pfrom)
{
    std::deque<CInv>::iterator it = pfrom->vRecvGetData.begin();

    vector<CInv> vNotFound;

    LOCK(cs_main);

    while (it != pfrom->vRecvGetData.end()) {
        // Don't bother if send buffer is too full to respond anyway
        if (pfrom->nSendSize >= SendBufferSize())
            break;

        const CInv &inv = *it;
        {
            boost::this_thread::interruption_point();
            it++;

            if (inv.type == MSG_BLOCK || inv.type == MSG_FILTERED_BLOCK)
            {
                bool send = false;
                map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(inv.hash);
                if (mi != mapBlockIndex.end())
                {
                    // If the requested block is at a height below our last
                    // checkpoint, only serve it if it's in the checkpointed chain
                    int nHeight = mi->second->nHeight;
                    CBlockIndex* pcheckpoint = Checkpoints::GetLastCheckpoint(mapBlockIndex);
                    if (pcheckpoint && nHeight < pcheckpoint->nHeight) {
                       if (!chainActive.Contains(mi->second))
                       {
                         LogPrintf("ProcessGetData(): ignoring request for old block that isn't in the main chain\n");
                       } else {
                         send = true;
                       }
                    } else {
                      send = true;
                    }
                }
                if (send)
                {
                    // Send block from disk
                    CBlock block;
                    ReadBlockFromDisk(block, (*mi).second);
                    if (inv.type == MSG_BLOCK)
                        pfrom->PushMessage("block", block);
                    else // MSG_FILTERED_BLOCK)
                    {
                        LOCK(pfrom->cs_filter);
                        if (pfrom->pfilter)
                        {
                            CMerkleBlock merkleBlock(block, *pfrom->pfilter);
                            pfrom->PushMessage("merkleblock", merkleBlock);
                            // CMerkleBlock just contains hashes, so also push any transactions in the block the client did not see
                            // This avoids hurting performance by pointlessly requiring a round-trip
                            // Note that there is currently no way for a node to request any single transactions we didnt send here -
                            // they must either disconnect and retry or request the full block.
                            // Thus, the protocol spec specified allows for us to provide duplicate txn here,
                            // however we MUST always provide at least what the remote peer needs
                            typedef std::pair<unsigned int, uint256> PairType;
                            BOOST_FOREACH(PairType& pair, merkleBlock.vMatchedTxn)
                                if (!pfrom->setInventoryKnown.count(CInv(MSG_TX, pair.second)))
                                    pfrom->PushMessage("tx", block.vtx[pair.first]);
                        }
                        // else
                            // no response
                    }

                    // Trigger them to send a getblocks request for the next batch of inventory
                    if (inv.hash == pfrom->hashContinue)
                    {
                        // Bypass PushInventory, this must send even if redundant,
                        // and we want it right after the last block so they don't
                        // wait for other stuff first.
                        vector<CInv> vInv;
                        vInv.push_back(CInv(MSG_BLOCK, chainActive.Tip()->GetBlockHash()));
                        pfrom->PushMessage("inv", vInv);
                        pfrom->hashContinue = 0;
                    }
                }
            }
            else if (inv.IsKnownType())
            {
                // Send stream from relay memory
                bool pushed = false;
                {
                    LOCK(cs_mapRelay);
                    map<CInv, CDataStream>::iterator mi = mapRelay.find(inv);
                    if (mi != mapRelay.end()) {
                        pfrom->PushMessage(inv.GetCommand(), (*mi).second);
                        pushed = true;
                    }
                }
                if (!pushed && inv.type == MSG_TX) {
                    CTransaction tx;
                    if (mempool.lookup(inv.hash, tx)) {
                        CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
                        ss.reserve(1000);
                        ss << tx;
                        pfrom->PushMessage("tx", ss);
                        pushed = true;
                    }
                }
                if (!pushed) {
                    vNotFound.push_back(inv);
                }
            }

            // Track requests for our stuff.
            g_signals.Inventory(inv.hash);

            if (inv.type == MSG_BLOCK || inv.type == MSG_FILTERED_BLOCK)
                break;
        }
    }

    pfrom->vRecvGetData.erase(pfrom->vRecvGetData.begin(), it);

    if (!vNotFound.empty()) {
        // Let the peer know that we didn't find what it asked for, so it doesn't
        // have to wait around forever. Currently only SPV clients actually care
        // about this message: it's needed when they are recursively walking the
        // dependencies of relevant unconfirmed transactions. SPV clients want to
        // do that because they want to know about (and store and rebroadcast and
        // risk analyze) the dependencies of transactions relevant to them, without
        // having to download the entire memory pool.
        pfrom->PushMessage("notfound", vNotFound);
    }
}

bool static ProcessMessage(CNode* pfrom, string strCommand, CDataStream& vRecv)
{
    RandAddSeedPerfmon();
    LogPrint("net", "received: %s (%"PRIszu" bytes)\n", strCommand, vRecv.size());
    if (mapArgs.count("-dropmessagestest") && GetRand(atoi(mapArgs["-dropmessagestest"])) == 0)
    {
        LogPrintf("dropmessagestest DROPPING RECV MESSAGE\n");
        return true;
    }





    if (strCommand == "version")
    {
        // Each connection can only send one version message
        if (pfrom->nVersion != 0)
        {
            pfrom->PushMessage("reject", strCommand, REJECT_DUPLICATE, string("Duplicate version message"));
            Misbehaving(pfrom->GetId(), 1);
            return false;
        }

        int64_t nTime;
        CAddress addrMe;
        CAddress addrFrom;
        uint64_t nNonce = 1;
        vRecv >> pfrom->nVersion >> pfrom->nServices >> nTime >> addrMe;
        if (pfrom->nVersion < MIN_PEER_PROTO_VERSION)
        {
            // disconnect from peers older than this proto version
            LogPrintf("partner %s using obsolete version %i; disconnecting\n", pfrom->addr.ToString(), pfrom->nVersion);
            pfrom->PushMessage("reject", strCommand, REJECT_OBSOLETE,
                               strprintf("Version must be %d or greater", MIN_PEER_PROTO_VERSION));
            pfrom->fDisconnect = true;
            return false;
        }

        if (pfrom->nVersion == 10300)
            pfrom->nVersion = 300;
        if (!vRecv.empty())
            vRecv >> addrFrom >> nNonce;
        if (!vRecv.empty()) {
            vRecv >> pfrom->strSubVer;
            pfrom->cleanSubVer = SanitizeString(pfrom->strSubVer);
        }
        if (!vRecv.empty())
            vRecv >> pfrom->nStartingHeight;
        if (!vRecv.empty())
            vRecv >> pfrom->fRelayTxes; // set to true after we get the first filter* message
        else
            pfrom->fRelayTxes = true;

        if (pfrom->fInbound && addrMe.IsRoutable())
        {
            pfrom->addrLocal = addrMe;
            SeenLocal(addrMe);
        }

        // Disconnect if we connected to ourself
        if (nNonce == nLocalHostNonce && nNonce > 1)
        {
            LogPrintf("connected to self at %s, disconnecting\n", pfrom->addr.ToString());
            pfrom->fDisconnect = true;
            return true;
        }

        // Be shy and don't send version until we hear
        if (pfrom->fInbound)
            pfrom->PushVersion();

        pfrom->fClient = !(pfrom->nServices & NODE_NETWORK);


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
            }

            // Get recent addresses
            if (pfrom->fOneShot || pfrom->nVersion >= CADDR_TIME_VERSION || addrman.size() < 1000)
            {
                pfrom->PushMessage("getaddr");
                pfrom->fGetAddr = true;
            }
            addrman.Good(pfrom->addr);
        } else {
            if (((CNetAddr)pfrom->addr) == (CNetAddr)addrFrom)
            {
                addrman.Add(addrFrom, addrFrom);
                addrman.Good(addrFrom);
            }
        }

        // Relay alerts
        {
            LOCK(cs_mapAlerts);
            BOOST_FOREACH(PAIRTYPE(const uint256, CAlert)& item, mapAlerts)
                item.second.RelayTo(pfrom);
        }

        pfrom->fSuccessfullyConnected = true;

        LogPrintf("receive version message: %s: version %d, blocks=%d, us=%s, them=%s, peer=%s\n", pfrom->cleanSubVer, pfrom->nVersion, pfrom->nStartingHeight, addrMe.ToString(), addrFrom.ToString(), pfrom->addr.ToString());

        AddTimeData(pfrom->addr, nTime);

        LOCK(cs_main);
        cPeerBlockCounts.input(pfrom->nStartingHeight);
    }


    else if (pfrom->nVersion == 0)
    {
        // Must have a version message before anything else
        Misbehaving(pfrom->GetId(), 1);
        return false;
    }


    else if (strCommand == "verack")
    {
        pfrom->SetRecvVersion(min(pfrom->nVersion, PROTOCOL_VERSION));
    }


    else if (strCommand == "addr")
    {
        vector<CAddress> vAddr;
        vRecv >> vAddr;

        // Don't want addr from older versions unless seeding
        if (pfrom->nVersion < CADDR_TIME_VERSION && addrman.size() > 1000)
            return true;
        if (vAddr.size() > 1000)
        {
            Misbehaving(pfrom->GetId(), 20);
            return error("message addr size() = %"PRIszu"", vAddr.size());
        }

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
                    }
                    int nRelayNodes = fReachable ? 2 : 1; // limited relaying of addresses outside our network(s)
                    for (multimap<uint256, CNode*>::iterator mi = mapMix.begin(); mi != mapMix.end() && nRelayNodes-- > 0; ++mi)
                        ((*mi).second)->PushAddress(addr);
                }
            }
            // Do not store addresses outside our network
            if (fReachable)
                vAddrOk.push_back(addr);
        }
        addrman.Add(vAddrOk, pfrom->addr, 2 * 60 * 60);
        if (vAddr.size() < 1000)
            pfrom->fGetAddr = false;
        if (pfrom->fOneShot)
            pfrom->fDisconnect = true;
    }


    else if (strCommand == "inv")
    {
        vector<CInv> vInv;
        vRecv >> vInv;
        if (vInv.size() > MAX_INV_SZ)
        {
            Misbehaving(pfrom->GetId(), 20);
            return error("message inv size() = %"PRIszu"", vInv.size());
        }

        // find last block in inv vector
        unsigned int nLastBlock = (unsigned int)(-1);
        for (unsigned int nInv = 0; nInv < vInv.size(); nInv++) {
            if (vInv[vInv.size() - 1 - nInv].type == MSG_BLOCK) {
                nLastBlock = vInv.size() - 1 - nInv;
                break;
            }
        }

        LOCK(cs_main);

        for (unsigned int nInv = 0; nInv < vInv.size(); nInv++)
        {
            const CInv &inv = vInv[nInv];

            boost::this_thread::interruption_point();
            pfrom->AddInventoryKnown(inv);

            bool fAlreadyHave = AlreadyHave(inv);
            LogPrint("net", "  got inventory: %s  %s\n", inv.ToString(), fAlreadyHave ? "have" : "new");

            if (!fAlreadyHave) {
                if (!fImporting && !fReindex)
                    pfrom->AskFor(inv);
            } else if (inv.type == MSG_BLOCK && mapOrphanBlocks.count(inv.hash)) {
                PushGetBlocks(pfrom, chainActive.Tip(), GetOrphanRoot(inv.hash));
            } else if (nInv == nLastBlock) {
                // In case we are on a very long side-chain, it is possible that we already have
                // the last block in an inv bundle sent in response to getblocks. Try to detect
                // this situation and push another getblocks to continue.
                PushGetBlocks(pfrom, mapBlockIndex[inv.hash], uint256(0));
                if (fDebug)
                    LogPrintf("force request: %s\n", inv.ToString());
            }

            // Track requests for our stuff
            g_signals.Inventory(inv.hash);
        }
    }


    else if (strCommand == "getdata")
    {
        vector<CInv> vInv;
        vRecv >> vInv;
        if (vInv.size() > MAX_INV_SZ)
        {
            Misbehaving(pfrom->GetId(), 20);
            return error("message getdata size() = %"PRIszu"", vInv.size());
        }

        if (fDebug || (vInv.size() != 1))
            LogPrint("net", "received getdata (%"PRIszu" invsz)\n", vInv.size());

        if ((fDebug && vInv.size() > 0) || (vInv.size() == 1))
            LogPrint("net", "received getdata for: %s\n", vInv[0].ToString());

        pfrom->vRecvGetData.insert(pfrom->vRecvGetData.end(), vInv.begin(), vInv.end());
        ProcessGetData(pfrom);
    }


    else if (strCommand == "getblocks")
    {
        CBlockLocator locator;
        uint256 hashStop;
        vRecv >> locator >> hashStop;

        LOCK(cs_main);

        // Find the last block the caller has in the main chain
        CBlockIndex* pindex = chainActive.FindFork(locator);

        // Send the rest of the chain
        if (pindex)
            pindex = chainActive.Next(pindex);
        int nLimit = 500;
        LogPrint("net", "getblocks %d to %s limit %d\n", (pindex ? pindex->nHeight : -1), hashStop.ToString(), nLimit);
        for (; pindex; pindex = chainActive.Next(pindex))
        {
            if (pindex->GetBlockHash() == hashStop)
            {
                LogPrint("net", "  getblocks stopping at %d %s\n", pindex->nHeight, pindex->GetBlockHash().ToString());
                break;
            }
            pfrom->PushInventory(CInv(MSG_BLOCK, pindex->GetBlockHash()));
            if (--nLimit <= 0)
            {
                // When this block is requested, we'll send an inv that'll make them
                // getblocks the next batch of inventory.
                LogPrint("net", "  getblocks stopping at limit %d %s\n", pindex->nHeight, pindex->GetBlockHash().ToString());
                pfrom->hashContinue = pindex->GetBlockHash();
                break;
            }
        }
    }


    else if (strCommand == "getheaders")
    {
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
        }
        else
        {
            // Find the last block the caller has in the main chain
            pindex = chainActive.FindFork(locator);
            if (pindex)
                pindex = chainActive.Next(pindex);
        }

        // we must use CBlocks, as CBlockHeaders won't include the 0x00 nTx count at the end
        vector<CBlock> vHeaders;
        int nLimit = 2000;
        LogPrint("net", "getheaders %d to %s\n", (pindex ? pindex->nHeight : -1), hashStop.ToString());
        for (; pindex; pindex = chainActive.Next(pindex))
        {
            vHeaders.push_back(pindex->GetBlockHeader());
            if (--nLimit <= 0 || pindex->GetBlockHash() == hashStop)
                break;
        }
        pfrom->PushMessage("headers", vHeaders);
    }


    else if (strCommand == "tx")
    {
        vector<uint256> vWorkQueue;
        vector<uint256> vEraseQueue;
        CTransaction tx;
        vRecv >> tx;

        CInv inv(MSG_TX, tx.GetHash());
        pfrom->AddInventoryKnown(inv);

        LOCK(cs_main);

        bool fMissingInputs = false;
        CValidationState state;
        if (AcceptToMemoryPool(mempool, state, tx, true, &fMissingInputs))
        {
            mempool.check(pcoinsTip);
            RelayTransaction(tx, inv.hash);
            mapAlreadyAskedFor.erase(inv);
            vWorkQueue.push_back(inv.hash);
            vEraseQueue.push_back(inv.hash);


            LogPrint("mempool", "AcceptToMemoryPool: %s %s : accepted %s (poolsz %"PRIszu")\n",
                pfrom->addr.ToString(), pfrom->cleanSubVer,
                tx.GetHash().ToString(),
                mempool.mapTx.size());

            // Recursively process any orphan transactions that depended on this one
            for (unsigned int i = 0; i < vWorkQueue.size(); i++)
            {
                uint256 hashPrev = vWorkQueue[i];
                for (set<uint256>::iterator mi = mapOrphanTransactionsByPrev[hashPrev].begin();
                     mi != mapOrphanTransactionsByPrev[hashPrev].end();
                     ++mi)
                {
                    const uint256& orphanHash = *mi;
                    const CTransaction& orphanTx = mapOrphanTransactions[orphanHash];
                    bool fMissingInputs2 = false;
                    // Use a dummy CValidationState so someone can't setup nodes to counter-DoS based on orphan
                    // resolution (that is, feeding people an invalid transaction based on LegitTxX in order to get
                    // anyone relaying LegitTxX banned)
                    CValidationState stateDummy;

                    if (AcceptToMemoryPool(mempool, stateDummy, orphanTx, true, &fMissingInputs2))
                    {
                        LogPrint("mempool", "   accepted orphan tx %s\n", orphanHash.ToString());
                        RelayTransaction(orphanTx, orphanHash);
                        mapAlreadyAskedFor.erase(CInv(MSG_TX, orphanHash));
                        vWorkQueue.push_back(orphanHash);
                        vEraseQueue.push_back(orphanHash);
                    }
                    else if (!fMissingInputs2)
                    {
                        // invalid or too-little-fee orphan
                        vEraseQueue.push_back(orphanHash);
                        LogPrint("mempool", "   removed orphan tx %s\n", orphanHash.ToString());
                    }
                    mempool.check(pcoinsTip);
                }
            }

            BOOST_FOREACH(uint256 hash, vEraseQueue)
                EraseOrphanTx(hash);
        }
        else if (fMissingInputs)
        {
            AddOrphanTx(tx);

            // DoS prevention: do not allow mapOrphanTransactions to grow unbounded
            unsigned int nEvicted = LimitOrphanTxSize(MAX_ORPHAN_TRANSACTIONS);
            if (nEvicted > 0)
                LogPrint("mempool", "mapOrphan overflow, removed %u tx\n", nEvicted);
        }
        int nDoS = 0;
        if (state.IsInvalid(nDoS))
        { 
            LogPrint("mempool", "%s from %s %s was not accepted into the memory pool: %s\n", tx.GetHash().ToString(),
                pfrom->addr.ToString(), pfrom->cleanSubVer,
                state.GetRejectReason());
            pfrom->PushMessage("reject", strCommand, state.GetRejectCode(),
                               state.GetRejectReason(), inv.hash);
            if (nDoS > 0)
                Misbehaving(pfrom->GetId(), nDoS);
        }
    }


    else if (strCommand == "block" && !fImporting && !fReindex) // Ignore blocks received while importing
    {
        CBlock block;
        vRecv >> block;

        LogPrint("net", "received block %s\n", block.GetHash().ToString());
        // block.print();

        CInv inv(MSG_BLOCK, block.GetHash());
        pfrom->AddInventoryKnown(inv);

        LOCK(cs_main);
        // Remember who we got this block from.
        mapBlockSource[inv.hash] = pfrom->GetId();

        CValidationState state;
        ProcessBlock(state, pfrom, &block);
    }


    else if (strCommand == "getaddr")
    {
        pfrom->vAddrToSend.clear();
        vector<CAddress> vAddr = addrman.GetAddr();
        BOOST_FOREACH(const CAddress &addr, vAddr)
            pfrom->PushAddress(addr);
    }


    else if (strCommand == "mempool")
    {
        LOCK2(cs_main, pfrom->cs_filter);

        std::vector<uint256> vtxid;
        mempool.queryHashes(vtxid);
        vector<CInv> vInv;
        BOOST_FOREACH(uint256& hash, vtxid) {
            CInv inv(MSG_TX, hash);
            CTransaction tx;
            bool fInMemPool = mempool.lookup(hash, tx);
            if (!fInMemPool) continue; // another thread removed since queryHashes, maybe...
            if ((pfrom->pfilter && pfrom->pfilter->IsRelevantAndUpdate(tx, hash)) ||
               (!pfrom->pfilter))
                vInv.push_back(inv);
            if (vInv.size() == MAX_INV_SZ) {
                pfrom->PushMessage("inv", vInv);
                vInv.clear();
            }
        }
        if (vInv.size() > 0)
            pfrom->PushMessage("inv", vInv);
    }


    else if (strCommand == "ping")
    {
        if (pfrom->nVersion > BIP0031_VERSION)
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
        }
    }


    else if (strCommand == "pong")
    {
        int64_t pingUsecEnd = GetTimeMicros();
        uint64_t nonce = 0;
        size_t nAvail = vRecv.in_avail();
        bool bPingFinished = false;
        std::string sProblem;

        if (nAvail >= sizeof(nonce)) {
            vRecv >> nonce;

            // Only process pong message if there is an outstanding ping (old ping without nonce should never pong)
            if (pfrom->nPingNonceSent != 0) {
                if (nonce == pfrom->nPingNonceSent) {
                    // Matching pong received, this ping is no longer outstanding
                    bPingFinished = true;
                    int64_t pingUsecTime = pingUsecEnd - pfrom->nPingUsecStart;
                    if (pingUsecTime > 0) {
                        // Successful ping time measurement, replace previous
                        pfrom->nPingUsecTime = pingUsecTime;
                    } else {
                        // This should never happen
                        sProblem = "Timing mishap";
                    }
                } else {
                    // Nonce mismatches are normal when pings are overlapping
                    sProblem = "Nonce mismatch";
                    if (nonce == 0) {
                        // This is most likely a bug in another implementation somewhere, cancel this ping
                        bPingFinished = true;
                        sProblem = "Nonce zero";
                    }
                }
            } else {
                sProblem = "Unsolicited pong without ping";
            }
        } else {
            // This is most likely a bug in another implementation somewhere, cancel this ping
            bPingFinished = true;
            sProblem = "Short payload";
        }

        if (!(sProblem.empty())) {
            LogPrint("net", "pong %s %s: %s, %x expected, %x received, %"PRIszu" bytes\n",
                pfrom->addr.ToString(),
                pfrom->cleanSubVer,
                sProblem,
                pfrom->nPingNonceSent,
                nonce,
                nAvail);
        }
        if (bPingFinished) {
            pfrom->nPingNonceSent = 0;
        }
    }


    else if (strCommand == "alert")
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
            }
            else {
                // Small DoS penalty so peers that send us lots of
                // duplicate/expired/invalid-signature/whatever alerts
                // eventually get banned.
                // This isn't a Misbehaving(100) (immediate ban) because the
                // peer might be an older or different implementation with
                // a different signature key, etc.
                Misbehaving(pfrom->GetId(), 10);
            }
        }
    }


    else if (strCommand == "filterload")
    {
        CBloomFilter filter;
        vRecv >> filter;

        if (!filter.IsWithinSizeConstraints())
            // There is no excuse for sending a too-large filter
            Misbehaving(pfrom->GetId(), 100);
        else
        {
            LOCK(pfrom->cs_filter);
            delete pfrom->pfilter;
            pfrom->pfilter = new CBloomFilter(filter);
            pfrom->pfilter->UpdateEmptyFull();
        }
        pfrom->fRelayTxes = true;
    }


    else if (strCommand == "filteradd")
    {
        vector<unsigned char> vData;
        vRecv >> vData;

        // Nodes must NEVER send a data item > 520 bytes (the max size for a script data object,
        // and thus, the maximum size any matched object can have) in a filteradd message
        if (vData.size() > MAX_SCRIPT_ELEMENT_SIZE)
        {
            Misbehaving(pfrom->GetId(), 100);
        } else {
            LOCK(pfrom->cs_filter);
            if (pfrom->pfilter)
                pfrom->pfilter->insert(vData);
            else
                Misbehaving(pfrom->GetId(), 100);
        }
    }


    else if (strCommand == "filterclear")
    {
        LOCK(pfrom->cs_filter);
        delete pfrom->pfilter;
        pfrom->pfilter = new CBloomFilter();
        pfrom->fRelayTxes = true;
    }


    else if (strCommand == "reject")
    {
        if (fDebug)
        {
            string strMsg; unsigned char ccode; string strReason;
            vRecv >> strMsg >> ccode >> strReason;

            ostringstream ss;
            ss << strMsg << " code " << itostr(ccode) << ": " << strReason;

            if (strMsg == "block" || strMsg == "tx")
            {
                uint256 hash;
                vRecv >> hash;
                ss << ": hash " << hash.ToString();
            }
            // Truncate to reasonable length and sanitize before printing:
            string s = ss.str();
            if (s.size() > 111) s.erase(111, string::npos);
            LogPrint("net", "Reject %s\n", SanitizeString(s));
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

// requires LOCK(cs_vRecvMsg)
bool ProcessMessages(CNode* pfrom)
{
    //if (fDebug)
    //    LogPrintf("ProcessMessages(%"PRIszu" messages)\n", pfrom->vRecvMsg.size());

    //
    // Message format
    //  (4) message start
    //  (12) command
    //  (4) size
    //  (4) checksum
    //  (x) data
    //
    bool fOk = true;

    if (!pfrom->vRecvGetData.empty())
        ProcessGetData(pfrom);

    // this maintains the order of responses
    if (!pfrom->vRecvGetData.empty()) return fOk;

    std::deque<CNetMessage>::iterator it = pfrom->vRecvMsg.begin();
    while (!pfrom->fDisconnect && it != pfrom->vRecvMsg.end()) {
        // Don't bother if send buffer is too full to respond anyway
        if (pfrom->nSendSize >= SendBufferSize())
            break;

        // get next message
        CNetMessage& msg = *it;

        //if (fDebug)
        //    LogPrintf("ProcessMessages(message %u msgsz, %"PRIszu" bytes, complete:%s)\n",
        //            msg.hdr.nMessageSize, msg.vRecv.size(),
        //            msg.complete() ? "Y" : "N");

        // end, if an incomplete message is found
        if (!msg.complete())
            break;

        // at this point, any failure means we can delete the current message
        it++;

        // Scan for message start
        if (memcmp(msg.hdr.pchMessageStart, Params().MessageStart(), MESSAGE_START_SIZE) != 0) {
            LogPrintf("\n\nPROCESSMESSAGE: INVALID MESSAGESTART\n\n");
            fOk = false;
            break;
        }

        // Read header
        CMessageHeader& hdr = msg.hdr;
        if (!hdr.IsValid())
        {
            LogPrintf("\n\nPROCESSMESSAGE: ERRORS IN HEADER %s\n\n\n", hdr.GetCommand());
            continue;
        }
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
               strCommand, nMessageSize, nChecksum, hdr.nChecksum);
            continue;
        }

        // Process message
        bool fRet = false;
        try
        {
            fRet = ProcessMessage(pfrom, strCommand, vRecv);
            boost::this_thread::interruption_point();
        }
        catch (std::ios_base::failure& e)
        {
            pfrom->PushMessage("reject", strCommand, REJECT_MALFORMED, string("error parsing message"));
            if (strstr(e.what(), "end of data"))
            {
                // Allow exceptions from under-length message on vRecv
                LogPrintf("ProcessMessages(%s, %u bytes) : Exception '%s' caught, normally caused by a message being shorter than its stated length\n", strCommand, nMessageSize, e.what());
            }
            else if (strstr(e.what(), "size too large"))
            {
                // Allow exceptions from over-long size
                LogPrintf("ProcessMessages(%s, %u bytes) : Exception '%s' caught\n", strCommand, nMessageSize, e.what());
            }
            else
            {
                PrintExceptionContinue(&e, "ProcessMessages()");
            }
        }
        catch (boost::thread_interrupted) {
            throw;
        }
        catch (std::exception& e) {
            PrintExceptionContinue(&e, "ProcessMessages()");
        } catch (...) {
            PrintExceptionContinue(NULL, "ProcessMessages()");
        }

        if (!fRet)
            LogPrintf("ProcessMessage(%s, %u bytes) FAILED\n", strCommand, nMessageSize);

        break;
    }

    // In case the connection got shut down, its receive buffer was wiped
    if (!pfrom->fDisconnect)
        pfrom->vRecvMsg.erase(pfrom->vRecvMsg.begin(), it);

    return fOk;
}


bool SendMessages(CNode* pto, bool fSendTrickle)
{
    {
        // Don't send anything until we get their version message
        if (pto->nVersion == 0)
            return true;

        //
        // Message: ping
        //
        bool pingSend = false;
        if (pto->fPingQueued) {
            // RPC ping request by user
            pingSend = true;
        }
        if (pto->nLastSend && GetTime() - pto->nLastSend > 30 * 60 && pto->vSendMsg.empty()) {
            // Ping automatically sent as a keepalive
            pingSend = true;
        }
        if (pingSend) {
            uint64_t nonce = 0;
            while (nonce == 0) {
                RAND_bytes((unsigned char*)&nonce, sizeof(nonce));
            }
            pto->nPingNonceSent = nonce;
            pto->fPingQueued = false;
            if (pto->nVersion > BIP0031_VERSION) {
                // Take timestamp as close as possible before transmitting ping
                pto->nPingUsecStart = GetTimeMicros();
                pto->PushMessage("ping", nonce);
            } else {
                // Peer is too old to support ping command with nonce, pong will never arrive, disable timing
                pto->nPingUsecStart = 0;
                pto->PushMessage("ping");
            }
        }

        // Address refresh broadcast
        static int64_t nLastRebroadcast;
        if (!IsInitialBlockDownload() && (GetTime() - nLastRebroadcast > 24 * 60 * 60))
        {
            {
                LOCK(cs_vNodes);
                BOOST_FOREACH(CNode* pnode, vNodes)
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
                    }
                }
            }
            nLastRebroadcast = GetTime();
        }

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
                    }
                }
            }
            pto->vAddrToSend.clear();
            if (!vAddr.empty())
                pto->PushMessage("addr", vAddr);
        }

        TRY_LOCK(cs_main, lockMain);
        if (!lockMain)
            return true;

        CNodeState &state = *State(pto->GetId());
        if (state.fShouldBan) {
            if (pto->addr.IsLocal())
                LogPrintf("Warning: not banning local node %s!\n", pto->addr.ToString());
            else {
                pto->fDisconnect = true;
                CNode::Ban(pto->addr);
            }
            state.fShouldBan = false;
        }

        BOOST_FOREACH(const CBlockReject& reject, state.rejects)
            pto->PushMessage("reject", (string)"block", reject.chRejectCode, reject.strRejectReason, reject.hashBlock);
        state.rejects.clear();

        // Start block sync
        if (pto->fStartSync && !fImporting && !fReindex) {
            pto->fStartSync = false;
            PushGetBlocks(pto, chainActive.Tip(), uint256(0));
        }

        // Resend wallet transactions that haven't gotten in a block yet
        // Except during reindex, importing and IBD, when old wallet
        // transactions become unconfirmed and spams other nodes.
        if (!fReindex && !fImporting && !IsInitialBlockDownload())
        {
            g_signals.Broadcast();
        }

        //
        // Message: inventory
        //
        vector<CInv> vInv;
        vector<CInv> vInvWait;
        {
            LOCK(pto->cs_inventory);
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
        int64_t nNow = GetTime() * 1000000;
        while (!pto->mapAskFor.empty() && (*pto->mapAskFor.begin()).first <= nNow)
        {
            const CInv& inv = (*pto->mapAskFor.begin()).second;
            if (!AlreadyHave(inv))
            {
                if (fDebug)
                    LogPrint("net", "sending getdata: %s\n", inv.ToString());
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






class CMainCleanup
{
public:
    CMainCleanup() {}
    ~CMainCleanup() {
        // block headers
        std::map<uint256, CBlockIndex*>::iterator it1 = mapBlockIndex.begin();
        for (; it1 != mapBlockIndex.end(); it1++)
            delete (*it1).second;
        mapBlockIndex.clear();

        // orphan blocks
        std::map<uint256, COrphanBlock*>::iterator it2 = mapOrphanBlocks.begin();
        for (; it2 != mapOrphanBlocks.end(); it2++)
            delete (*it2).second;
        mapOrphanBlocks.clear();

        // orphan transactions
        mapOrphanTransactions.clear();
    }
} instance_of_cmaincleanup;
