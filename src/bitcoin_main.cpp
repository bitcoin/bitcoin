// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bitcoin_main.h"

#include "addrman.h"
#include "alert.h"
#include "chainparams.h"
#include "bitcoin_checkpoints.h"
#include "checkqueue.h"
#include "init.h"
#include "net.h"
#include "bitcoin_txdb.h"
#include "bitcoin_txmempool.h"
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

MainState bitcoin_mainState;

Bitcoin_CTxMemPool bitcoin_mempool;

map<uint256, Bitcoin_CBlockIndex*> bitcoin_mapBlockIndex;
Bitcoin_CChain bitcoin_chainActive;
Bitcoin_CChain bitcoin_chainMostWork;
int64_t bitcoin_nTimeBestReceived = 0;
int bitcoin_nScriptCheckThreads = 0;
bool bitcoin_fBenchmark = false;
bool bitcoin_fTxIndex = false;
bool bitcoin_fTrimBlockFiles = true;
bool bitcoin_fSimplifiedBlockValidation = true;
unsigned int bitcoin_nCoinCacheSize = 5000;

/** Fees smaller than this (in satoshi) are considered zero fee (for transaction creation) */
int64_t Bitcoin_CTransaction::nMinTxFee = 10000;  // Override with -mintxfee
/** Fees smaller than this (in satoshi) are considered zero fee (for relaying and mining) */
int64_t Bitcoin_CTransaction::nMinRelayTxFee = 1000;

map<uint256, COrphanBlock*> bitcoin_mapOrphanBlocks;
multimap<uint256, COrphanBlock*> bitcoin_mapOrphanBlocksByPrev;

map<uint256, Bitcoin_CTransaction> bitcoin_mapOrphanTransactions;
map<uint256, set<uint256> > bitcoin_mapOrphanTransactionsByPrev;

// Constant stuff for coinbase transactions we create:
CScript BITCOIN_COINBASE_FLAGS;

const string bitcoin_strMessageMagic = "Bitcoin Signed Message:\n";

// Internal stuff
namespace {
    struct Bitcoin_CBlockIndexWorkComparator
    {
        bool operator()(Bitcoin_CBlockIndex *pa, Bitcoin_CBlockIndex *pb) {
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

    Bitcoin_CBlockIndex *bitcoin_pindexBestInvalid;
    // may contain all CBlockIndex*'s that have validness >=BLOCK_VALID_TRANSACTIONS, and must contain those who aren't failed
    set<Bitcoin_CBlockIndex*, Bitcoin_CBlockIndexWorkComparator> bitcoin_setBlockIndexValid;

    // Every received block is assigned a unique and increasing identifier, so we
    // know which one to give priority in case of a fork.
    CCriticalSection bitcoin_cs_nBlockSequenceId;
    // Blocks loaded from disk are assigned id 0, so start the counter at 1.
    uint32_t bitcoin_nBlockSequenceId = 1;

    // Sources of received blocks, to be able to send them reject messages or ban
    // them, if processing happens afterwards. Protected by cs_main.
    map<uint256, NodeId> bitcoin_mapBlockSource;

    map<uint256, pair<NodeId, list<QueuedBlock>::iterator> > bitcoin_mapBlocksInFlight;
    map<uint256, pair<NodeId, list<uint256>::iterator> > bitcoin_mapBlocksToDownload;
}

//////////////////////////////////////////////////////////////////////////////
//
// dispatching functions
//

// These functions dispatch to one or all registered wallets

namespace {
struct Bitcoin_CMainSignals {
    // Notifies listeners of updated transaction data (passing hash, transaction, and optionally the block it is found in.
    boost::signals2::signal<void (const uint256 &, const Bitcoin_CTransaction &, const Bitcoin_CBlock *)> SyncTransaction;
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
} bitcoin_g_signals;
}

void Bitcoin_RegisterWallet(Bitcoin_CWalletInterface* pwalletIn) {
    bitcoin_g_signals.SyncTransaction.connect(boost::bind(&Bitcoin_CWalletInterface::SyncTransaction, pwalletIn, _1, _2, _3));
    bitcoin_g_signals.EraseTransaction.connect(boost::bind(&Bitcoin_CWalletInterface::EraseFromWallet, pwalletIn, _1));
    bitcoin_g_signals.UpdatedTransaction.connect(boost::bind(&Bitcoin_CWalletInterface::UpdatedTransaction, pwalletIn, _1));
    bitcoin_g_signals.SetBestChain.connect(boost::bind(&Bitcoin_CWalletInterface::SetBestChain, pwalletIn, _1));
    bitcoin_g_signals.Inventory.connect(boost::bind(&Bitcoin_CWalletInterface::Inventory, pwalletIn, _1));
    bitcoin_g_signals.Broadcast.connect(boost::bind(&Bitcoin_CWalletInterface::ResendWalletTransactions, pwalletIn));
}

void Bitcoin_UnregisterWallet(Bitcoin_CWalletInterface* pwalletIn) {
    bitcoin_g_signals.Broadcast.disconnect(boost::bind(&Bitcoin_CWalletInterface::ResendWalletTransactions, pwalletIn));
    bitcoin_g_signals.Inventory.disconnect(boost::bind(&Bitcoin_CWalletInterface::Inventory, pwalletIn, _1));
    bitcoin_g_signals.SetBestChain.disconnect(boost::bind(&Bitcoin_CWalletInterface::SetBestChain, pwalletIn, _1));
    bitcoin_g_signals.UpdatedTransaction.disconnect(boost::bind(&Bitcoin_CWalletInterface::UpdatedTransaction, pwalletIn, _1));
    bitcoin_g_signals.EraseTransaction.disconnect(boost::bind(&Bitcoin_CWalletInterface::EraseFromWallet, pwalletIn, _1));
    bitcoin_g_signals.SyncTransaction.disconnect(boost::bind(&Bitcoin_CWalletInterface::SyncTransaction, pwalletIn, _1, _2, _3));
}

void Bitcoin_UnregisterAllWallets() {
    bitcoin_g_signals.Broadcast.disconnect_all_slots();
    bitcoin_g_signals.Inventory.disconnect_all_slots();
    bitcoin_g_signals.SetBestChain.disconnect_all_slots();
    bitcoin_g_signals.UpdatedTransaction.disconnect_all_slots();
    bitcoin_g_signals.EraseTransaction.disconnect_all_slots();
    bitcoin_g_signals.SyncTransaction.disconnect_all_slots();
}

void Bitcoin_SyncWithWallets(const uint256 &hash, const Bitcoin_CTransaction &tx, const Bitcoin_CBlock *pblock) {
    bitcoin_g_signals.SyncTransaction(hash, tx, pblock);
}

//////////////////////////////////////////////////////////////////////////////
//
// Registration of network node signals.
//

namespace {

// Map maintaining per-node state. Requires cs_main.
map<NodeId, CNodeState> bitcoin_mapNodeState;

// Requires cs_main.
CNodeState *Bitcoin_State(NodeId pnode) {
    map<NodeId, CNodeState>::iterator it = bitcoin_mapNodeState.find(pnode);
    if (it == bitcoin_mapNodeState.end())
        return NULL;
    return &it->second;
}

int Bitcoin_GetHeight()
{
    LOCK(bitcoin_mainState.cs_main);
    return bitcoin_chainActive.Height();
}

void Bitcoin_InitializeNode(NodeId nodeid, const CNode *pnode) {
    LOCK(bitcoin_mainState.cs_main);
    CNodeState &state = bitcoin_mapNodeState.insert(std::make_pair(nodeid, CNodeState())).first->second;
    state.name = pnode->addrName;
}

void Bitcoin_FinalizeNode(NodeId nodeid) {
    LOCK(bitcoin_mainState.cs_main);
    CNodeState *state = Bitcoin_State(nodeid);

    BOOST_FOREACH(const QueuedBlock& entry, state->vBlocksInFlight)
        bitcoin_mapBlocksInFlight.erase(entry.hash);
    BOOST_FOREACH(const uint256& hash, state->vBlocksToDownload)
        bitcoin_mapBlocksToDownload.erase(hash);

    bitcoin_mapNodeState.erase(nodeid);
}

// Requires cs_main.
void Bitcoin_MarkBlockAsReceived(const uint256 &hash, NodeId nodeFrom = -1) {
    map<uint256, pair<NodeId, list<uint256>::iterator> >::iterator itToDownload = bitcoin_mapBlocksToDownload.find(hash);
    if (itToDownload != bitcoin_mapBlocksToDownload.end()) {
        CNodeState *state = Bitcoin_State(itToDownload->second.first);
        state->vBlocksToDownload.erase(itToDownload->second.second);
        state->nBlocksToDownload--;
        bitcoin_mapBlocksToDownload.erase(itToDownload);
    }

    map<uint256, pair<NodeId, list<QueuedBlock>::iterator> >::iterator itInFlight = bitcoin_mapBlocksInFlight.find(hash);
    if (itInFlight != bitcoin_mapBlocksInFlight.end()) {
        CNodeState *state = Bitcoin_State(itInFlight->second.first);
        state->vBlocksInFlight.erase(itInFlight->second.second);
        state->nBlocksInFlight--;
        if (itInFlight->second.first == nodeFrom)
            state->nLastBlockReceive = GetTimeMicros();
        bitcoin_mapBlocksInFlight.erase(itInFlight);
    }

}

// Requires cs_main.
bool Bitcoin_AddBlockToQueue(NodeId nodeid, const uint256 &hash) {
    if (bitcoin_mapBlocksToDownload.count(hash) || bitcoin_mapBlocksInFlight.count(hash))
        return false;

    CNodeState *state = Bitcoin_State(nodeid);
    if (state == NULL)
        return false;

    list<uint256>::iterator it = state->vBlocksToDownload.insert(state->vBlocksToDownload.end(), hash);
    state->nBlocksToDownload++;
    if (state->nBlocksToDownload > 5000)
        Bitcoin_Misbehaving(nodeid, 10);
    bitcoin_mapBlocksToDownload[hash] = std::make_pair(nodeid, it);
    return true;
}

// Requires cs_main.
void Bitcoin_MarkBlockAsInFlight(NodeId nodeid, const uint256 &hash) {
    CNodeState *state = Bitcoin_State(nodeid);
    assert(state != NULL);

    // Make sure it's not listed somewhere already.
    Bitcoin_MarkBlockAsReceived(hash);

    QueuedBlock newentry = {hash, GetTimeMicros(), state->nBlocksInFlight};
    if (state->nBlocksInFlight == 0)
        state->nLastBlockReceive = newentry.nTime; // Reset when a first request is sent.
    list<QueuedBlock>::iterator it = state->vBlocksInFlight.insert(state->vBlocksInFlight.end(), newentry);
    state->nBlocksInFlight++;
    bitcoin_mapBlocksInFlight[hash] = std::make_pair(nodeid, it);
}

}

bool Bitcoin_GetNodeStateStats(NodeId nodeid, CNodeStateStats &stats) {
    LOCK(bitcoin_mainState.cs_main);
    CNodeState *state = Bitcoin_State(nodeid);
    if (state == NULL)
        return false;
    stats.nMisbehavior = state->nMisbehavior;
    return true;
}

void Bitcoin_RegisterNodeSignals(CNodeSignals& nodeSignals)
{
    nodeSignals.GetHeight.connect(&Bitcoin_GetHeight);
    nodeSignals.ProcessMessages.connect(&Bitcoin_ProcessMessages);
    nodeSignals.SendMessages.connect(&Bitcoin_SendMessages);
    nodeSignals.InitializeNode.connect(&Bitcoin_InitializeNode);
    nodeSignals.FinalizeNode.connect(&Bitcoin_FinalizeNode);
}

void Bitcoin_UnregisterNodeSignals(CNodeSignals& nodeSignals)
{
    nodeSignals.GetHeight.disconnect(&Bitcoin_GetHeight);
    nodeSignals.ProcessMessages.disconnect(&Bitcoin_ProcessMessages);
    nodeSignals.SendMessages.disconnect(&Bitcoin_SendMessages);
    nodeSignals.InitializeNode.disconnect(&Bitcoin_InitializeNode);
    nodeSignals.FinalizeNode.disconnect(&Bitcoin_FinalizeNode);
}

//////////////////////////////////////////////////////////////////////////////
//
// CChain implementation
//

CBlockIndexBase *Bitcoin_CChain::SetTip(CBlockIndexBase *pindex) {
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

CBlockLocator Bitcoin_CChain::GetLocator(const CBlockIndexBase *pindex) const {
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

Bitcoin_CBlockIndex *Bitcoin_CChain::FindFork(const CBlockLocator &locator) const {
    // Find the first block the caller has in the main chain
    BOOST_FOREACH(const uint256& hash, locator.vHave) {
        std::map<uint256, Bitcoin_CBlockIndex*>::iterator mi = bitcoin_mapBlockIndex.find(hash);
        if (mi != bitcoin_mapBlockIndex.end())
        {
            Bitcoin_CBlockIndex* pindex = (*mi).second;
            if (Contains(pindex))
                return pindex;
        }
    }
    return Genesis();
}

Bitcoin_CCoinsViewCache *bitcoin_pcoinsTip = NULL;
Bitcoin_CBlockTreeDB *bitcoin_pblocktree = NULL;

//////////////////////////////////////////////////////////////////////////////
//
// mapOrphanTransactions
//

bool Bitcoin_AddOrphanTx(const Bitcoin_CTransaction& tx)
{
    uint256 hash = tx.GetHash();
    if (bitcoin_mapOrphanTransactions.count(hash))
        return false;

    // Ignore big transactions, to avoid a
    // send-big-orphans memory exhaustion attack. If a peer has a legitimate
    // large transaction with a missing parent then we assume
    // it will rebroadcast it later, after the parent transaction(s)
    // have been mined or received.
    // 10,000 orphans, each of which is at most 5,000 bytes big is
    // at most 500 megabytes of orphans:
    unsigned int sz = tx.GetSerializeSize(SER_NETWORK, Bitcoin_CTransaction::CURRENT_VERSION);
    if (sz > 5000)
    {
        LogPrint("mempool", "ignoring large orphan tx (size: %u, hash: %s)\n", sz, hash.ToString());
        return false;
    }

    bitcoin_mapOrphanTransactions[hash] = tx;
    BOOST_FOREACH(const Bitcoin_CTxIn& txin, tx.vin)
        bitcoin_mapOrphanTransactionsByPrev[txin.prevout.hash].insert(hash);

    LogPrint("mempool", "stored orphan tx %s (mapsz %u)\n", hash.ToString(),
        bitcoin_mapOrphanTransactions.size());
    return true;
}

void static Bitcoin_EraseOrphanTx(uint256 hash)
{
    if (!bitcoin_mapOrphanTransactions.count(hash))
        return;
    const Bitcoin_CTransaction& tx = bitcoin_mapOrphanTransactions[hash];
    BOOST_FOREACH(const Bitcoin_CTxIn& txin, tx.vin)
    {
        bitcoin_mapOrphanTransactionsByPrev[txin.prevout.hash].erase(hash);
        if (bitcoin_mapOrphanTransactionsByPrev[txin.prevout.hash].empty())
            bitcoin_mapOrphanTransactionsByPrev.erase(txin.prevout.hash);
    }
    bitcoin_mapOrphanTransactions.erase(hash);
}

unsigned int Bitcoin_LimitOrphanTxSize(unsigned int nMaxOrphans)
{
    unsigned int nEvicted = 0;
    while (bitcoin_mapOrphanTransactions.size() > nMaxOrphans)
    {
        // Evict a random orphan:
        uint256 randomhash = GetRandHash();
        map<uint256, Bitcoin_CTransaction>::iterator it = bitcoin_mapOrphanTransactions.lower_bound(randomhash);
        if (it == bitcoin_mapOrphanTransactions.end())
            it = bitcoin_mapOrphanTransactions.begin();
        Bitcoin_EraseOrphanTx(it->first);
        ++nEvicted;
    }
    return nEvicted;
}







bool Bitcoin_IsStandardTx(const Bitcoin_CTransaction& tx, string& reason)
{
    AssertLockHeld(bitcoin_mainState.cs_main);
    if (tx.nVersion > Bitcoin_CTransaction::CURRENT_VERSION || tx.nVersion < 1) {
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
    // is set to the value of nHeight in the block. However, when Bitcoin_IsFinalTx()
    // is called within CBlock::AcceptBlock(), the height of the block *being*
    // evaluated is what is used. Thus if we want to know if a transaction can
    // be part of the *next* block, we need to call IsFinalTx() with one more
    // than chainActive.Height().
    //
    // Timestamps on the other hand don't get any special treatment, because we
    // can't know what timestamp the next block will have, and there aren't
    // timestamp applications where it matters.
    if (!Bitcoin_IsFinalTx(tx, bitcoin_chainActive.Height() + 1)) {
        reason = "non-final";
        return false;
    }

    // Extremely large transactions with lots of inputs can cost the network
    // almost as much to process as they cost the sender in fees, because
    // computing signature hashes is O(ninputs*txsize). Limiting transactions
    // to MAX_STANDARD_TX_SIZE mitigates CPU exhaustion attacks.
    unsigned int sz = tx.GetSerializeSize(SER_NETWORK, Bitcoin_CTransaction::CURRENT_VERSION);
    if (sz >= BITCOIN_MAX_STANDARD_TX_SIZE) {
        reason = "tx-size";
        return false;
    }

    BOOST_FOREACH(const Bitcoin_CTxIn& txin, tx.vin)
    {
        // Biggest 'standard' txin is a 15-of-15 P2SH multisig with compressed
        // keys. (remember the 520 byte limit on redeemScript size) That works
        // out to a (15*(33+1))+3=513 byte redeemScript, 513+1+15*(73+1)=1624
        // bytes of scriptSig, which we round off to 1650 bytes for some minor
        // future-proofing. That's also enough to spend a 20-of-20
        // CHECKMULTISIG scriptPubKey, though such a scriptPubKey is not
        // considered standard)
        if (txin.scriptSig.size() > 1650) {
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
        else if (txout.IsDust(Bitcoin_CTransaction::nMinRelayTxFee)) {
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

bool Bitcoin_IsFinalTx(const Bitcoin_CTransaction &tx, int nBlockHeight, int64_t nBlockTime)
{
    AssertLockHeld(bitcoin_mainState.cs_main);
    // Time based nLockTime implemented in 0.1.6
    if (tx.nLockTime == 0)
        return true;
    if (nBlockHeight == 0)
        nBlockHeight = bitcoin_chainActive.Height();
    if (nBlockTime == 0)
        nBlockTime = GetAdjustedTime();
    if ((int64_t)tx.nLockTime < ((int64_t)tx.nLockTime < BITCOIN_LOCKTIME_THRESHOLD ? (int64_t)nBlockHeight : nBlockTime))
        return true;
    BOOST_FOREACH(const Bitcoin_CTxIn& txin, tx.vin)
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
bool Bitcoin_AreInputsStandard(const Bitcoin_CTransaction& tx, Bitcoin_CCoinsViewCache& mapInputs)
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
        if (!Bitcoin_EvalScript(stack, tx.vin[i].scriptSig, tx, i, false, 0))
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

unsigned int Bitcoin_GetLegacySigOpCount(const Bitcoin_CTransaction& tx)
{
    unsigned int nSigOps = 0;
    BOOST_FOREACH(const Bitcoin_CTxIn& txin, tx.vin)
    {
        nSigOps += txin.scriptSig.GetSigOpCount(false);
    }
    BOOST_FOREACH(const CTxOut& txout, tx.vout)
    {
        nSigOps += txout.scriptPubKey.GetSigOpCount(false);
    }
    return nSigOps;
}

unsigned int Bitcoin_GetP2SHSigOpCount(const Bitcoin_CTransaction& tx, Bitcoin_CCoinsViewCache& inputs)
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

int Bitcoin_CMerkleTx::SetMerkleBranch(const Bitcoin_CBlock* pblock)
{
    AssertLockHeld(bitcoin_mainState.cs_main);
    Bitcoin_CBlock blockTmp;

    if (pblock == NULL) {
        Claim_CCoins coins;
        if (credits_pcoinsTip->Claim_GetCoins(GetHash(), coins)) {
            Bitcoin_CBlockIndex *pindex = bitcoin_chainActive[coins.nHeight];
            if (pindex) {
                if (!Bitcoin_ReadBlockFromDisk(blockTmp, pindex))
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
            if (pblock->vtx[nIndex] == *(Bitcoin_CTransaction*)this)
                break;
        if (nIndex == (int)pblock->vtx.size())
        {
            vMerkleBranch.clear();
            nIndex = -1;
            LogPrintf("Bitcoin: ERROR: SetMerkleBranch() : couldn't find tx in block\n");
            return 0;
        }

        // Fill in merkle branch
        vMerkleBranch = pblock->GetMerkleBranch(nIndex);
    }

    // Is the tx in a block that's in the main chain
    map<uint256, Bitcoin_CBlockIndex*>::iterator mi = bitcoin_mapBlockIndex.find(hashBlock);
    if (mi == bitcoin_mapBlockIndex.end())
        return 0;
    Bitcoin_CBlockIndex* pindex = (*mi).second;
    if (!pindex || !bitcoin_chainActive.Contains(pindex))
        return 0;

    return bitcoin_chainActive.Height() - pindex->nHeight + 1;
}







bool Bitcoin_CheckTransaction(const Bitcoin_CTransaction& tx, CValidationState &state)
{
    // Basic checks that don't depend on any context
    if (tx.vin.empty())
        return state.DoS(10, error("Bitcoin: CheckTransaction() : vin empty"),
                         BITCOIN_REJECT_INVALID, "bad-txns-vin-empty");
    if (tx.vout.empty())
        return state.DoS(10, error("Bitcoin: CheckTransaction() : vout empty"),
                         BITCOIN_REJECT_INVALID, "bad-txns-vout-empty");
    // Size limits
    if (::GetSerializeSize(tx, SER_NETWORK, BITCOIN_PROTOCOL_VERSION) > BITCOIN_MAX_BLOCK_SIZE)
        return state.DoS(100, error("Bitcoin: CheckTransaction() : size limits failed"),
                         BITCOIN_REJECT_INVALID, "bad-txns-oversize");

    // Check for negative or overflow output values
    int64_t nValueOut = 0;
    BOOST_FOREACH(const CTxOut& txout, tx.vout)
    {
        if (txout.nValue < 0)
            return state.DoS(100, error("Bitcoin: CheckTransaction() : txout.nValue negative"),
                             BITCOIN_REJECT_INVALID, "bad-txns-vout-negative");
        if (txout.nValue > BITCOIN_MAX_MONEY)
            return state.DoS(100, error("Bitcoin: CheckTransaction() : txout.nValue too high"),
                             BITCOIN_REJECT_INVALID, "bad-txns-vout-toolarge");
        nValueOut += txout.nValue;
        if (!Bitcoin_MoneyRange(nValueOut))
            return state.DoS(100, error("Bitcoin: CheckTransaction() : txout total out of range"),
                             BITCOIN_REJECT_INVALID, "bad-txns-txouttotal-toolarge");
    }

    // Check for duplicate inputs
    set<COutPoint> vInOutPoints;
    BOOST_FOREACH(const Bitcoin_CTxIn& txin, tx.vin)
    {
        if (vInOutPoints.count(txin.prevout))
            return state.DoS(100, error("Bitcoin: CheckTransaction() : duplicate inputs"),
                             BITCOIN_REJECT_INVALID, "bad-txns-inputs-duplicate");
        vInOutPoints.insert(txin.prevout);
    }

    if (tx.IsCoinBase())
    {
        if (tx.vin[0].scriptSig.size() < 2 || tx.vin[0].scriptSig.size() > 100)
            return state.DoS(100, error("Bitcoin: CheckTransaction() : coinbase script size"),
                             BITCOIN_REJECT_INVALID, "bad-cb-length");
    }
    else
    {
        BOOST_FOREACH(const Bitcoin_CTxIn& txin, tx.vin)
            if (txin.prevout.IsNull())
                return state.DoS(10, error("Bitcoin: CheckTransaction() : prevout is null"),
                                 BITCOIN_REJECT_INVALID, "bad-txns-prevout-null");
    }

    return true;
}

int64_t Bitcoin_GetMinFee(const Bitcoin_CTransaction& tx, unsigned int nBytes, bool fAllowFree, enum GetMinFee_mode mode)
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
        if (nBytes < (mode == GMF_SEND ? 1000 : (BITCOIN_DEFAULT_BLOCK_PRIORITY_SIZE - 1000)))
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

    if (!Bitcoin_MoneyRange(nMinFee))
        nMinFee = BITCOIN_MAX_MONEY;
    return nMinFee;
}


bool Bitcoin_AcceptToMemoryPool(Bitcoin_CTxMemPool& pool, CValidationState &state, const Bitcoin_CTransaction &tx, bool fLimitFree,
                        bool* pfMissingInputs, bool fRejectInsaneFee)
{
    AssertLockHeld(bitcoin_mainState.cs_main);
    if (pfMissingInputs)
        *pfMissingInputs = false;

    if (!Bitcoin_CheckTransaction(tx, state))
        return error("Bitcoin: AcceptToMemoryPool: : CheckTransaction failed");

    // Coinbase is only valid in a block, not as a loose transaction
    if (tx.IsCoinBase())
        return state.DoS(100, error("Bitcoin: AcceptToMemoryPool: : coinbase as individual tx"),
                         BITCOIN_REJECT_INVALID, "coinbase");

    // Rather not work on nonstandard transactions (unless -testnet/-regtest)
    string reason;
    if (Bitcoin_Params().NetworkID() == CChainParams::MAIN && !Bitcoin_IsStandardTx(tx, reason))
        return state.DoS(0,
                         error("Bitcoin: AcceptToMemoryPool : nonstandard transaction: %s", reason),
                         BITCOIN_REJECT_NONSTANDARD, reason);

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
        Bitcoin_CCoinsView dummy;
        Bitcoin_CCoinsViewCache view(dummy);

        {
        LOCK(pool.cs);
        Bitcoin_CCoinsViewMemPool viewMemPool(*bitcoin_pcoinsTip, pool);
        view.SetBackend(viewMemPool);

        // do we already have it?
        if (view.HaveCoins(hash))
            return false;

        // do all inputs exist?
        // Note that this does not check for the presence of actual outputs (see the next check for that),
        // only helps filling in pfMissingInputs (to determine missing vs spent).
        BOOST_FOREACH(const Bitcoin_CTxIn txin, tx.vin) {
            if (!view.HaveCoins(txin.prevout.hash)) {
                if (pfMissingInputs)
                    *pfMissingInputs = true;
                return false;
            }
        }

        // are the actual inputs available?
        if (!view.HaveInputs(tx))
            return state.Invalid(error("Bitcoin: AcceptToMemoryPool : inputs already spent"),
                                 BITCOIN_REJECT_DUPLICATE, "bad-txns-inputs-spent");

        // Bring the best block into scope
        view.GetBestBlock();

        // we have all inputs cached now, so switch back to dummy, so we don't need to keep lock on mempool
        view.SetBackend(dummy);
        }

        // Check for non-standard pay-to-script-hash in inputs
        if (Bitcoin_Params().NetworkID() == CChainParams::MAIN && !Bitcoin_AreInputsStandard(tx, view))
            return error("Bitcoin: AcceptToMemoryPool: : nonstandard transaction input");

        // Note: if you modify this code to accept non-standard transactions, then
        // you should add code here to check that the transaction does a
        // reasonable number of ECDSA signature verifications.

        int64_t nValueIn = view.GetValueIn(tx);
        int64_t nValueOut = tx.GetValueOut();
        int64_t nFees = nValueIn-nValueOut;
        double dPriority = view.GetPriority(tx, bitcoin_chainActive.Height());

        Bitcoin_CTxMemPoolEntry entry(tx, nFees, GetTime(), dPriority, bitcoin_chainActive.Height());
        unsigned int nSize = entry.GetTxSize();

        // Don't accept it if it can't get into a block
        int64_t txMinFee = Bitcoin_GetMinFee(tx, nSize, true, GMF_RELAY);
        if (fLimitFree && nFees < txMinFee)
            return state.DoS(0, error("Bitcoin: AcceptToMemoryPool : not enough fees %s, %d < %d",
                                      hash.ToString(), nFees, txMinFee),
                             BITCOIN_REJECT_INSUFFICIENTFEE, "insufficient fee");

        // Continuously rate-limit free transactions
        // This mitigates 'penny-flooding' -- sending thousands of free transactions just to
        // be annoying or make others' transactions take longer to confirm.
        if (fLimitFree && nFees < Bitcoin_CTransaction::nMinRelayTxFee)
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
                return state.DoS(0, error("Bitcoin: AcceptToMemoryPool : free transaction rejected by rate limiter"),
                                 BITCOIN_REJECT_INSUFFICIENTFEE, "insufficient priority");
            LogPrint("mempool", "Rate limit dFreeCount: %g => %g\n", dFreeCount, dFreeCount+nSize);
            dFreeCount += nSize;
        }

        if (fRejectInsaneFee && nFees > Bitcoin_CTransaction::nMinRelayTxFee * 10000)
            return error("Bitcoin: AcceptToMemoryPool: : insane fees %s, %d > %d",
                         hash.ToString(),
                         nFees, Bitcoin_CTransaction::nMinRelayTxFee * 10000);

        // Check against previous transactions
        // This is done last to help prevent CPU exhaustion denial-of-service attacks.
        if (!Bitcoin_CheckInputs(tx, state, view, true, STANDARD_SCRIPT_VERIFY_FLAGS))
        {
            return error("Bitcoin: AcceptToMemoryPool: : ConnectInputs failed %s", hash.ToString());
        }
        // Store transaction in memory
        pool.addUnchecked(hash, entry);
    }

    bitcoin_g_signals.SyncTransaction(hash, tx, NULL);

    return true;
}


int Bitcoin_CMerkleTx::GetDepthInMainChainINTERNAL(Bitcoin_CBlockIndex* &pindexRet) const
{
    if (hashBlock == 0 || nIndex == -1)
        return 0;
    AssertLockHeld(bitcoin_mainState.cs_main);

    // Find the block it claims to be in
    map<uint256, Bitcoin_CBlockIndex*>::iterator mi = bitcoin_mapBlockIndex.find(hashBlock);
    if (mi == bitcoin_mapBlockIndex.end())
        return 0;
    Bitcoin_CBlockIndex* pindex = (*mi).second;
    if (!pindex || !bitcoin_chainActive.Contains(pindex))
        return 0;

    // Make sure the merkle branch connects to this block
    if (!fMerkleVerified)
    {
        if (Bitcoin_CBlock::CheckMerkleBranch(GetHash(), vMerkleBranch, nIndex) != pindex->hashMerkleRoot)
            return 0;
        fMerkleVerified = true;
    }

    pindexRet = pindex;
    return bitcoin_chainActive.Height() - pindex->nHeight + 1;
}

int Bitcoin_CMerkleTx::GetDepthInMainChain(Bitcoin_CBlockIndex* &pindexRet) const
{
    AssertLockHeld(bitcoin_mainState.cs_main);
    int nResult = GetDepthInMainChainINTERNAL(pindexRet);
    if (nResult == 0 && !bitcoin_mempool.exists(GetHash()))
        return -1; // Not in chain, not in mempool

    return nResult;
}

int Bitcoin_CMerkleTx::GetBlocksToMaturity() const
{
    if (!IsCoinBase())
        return 0;
    return max(0, (BITCOIN_COINBASE_MATURITY+1) - GetDepthInMainChain());
}


bool Bitcoin_CMerkleTx::AcceptToMemoryPool(bool fLimitFree)
{
    CValidationState state;
    return ::Bitcoin_AcceptToMemoryPool(bitcoin_mempool, state, *this, fLimitFree, NULL);
}


// Return transaction in tx, and if it was found inside a block, its hash is placed in hashBlock
bool Bitcoin_GetTransaction(const uint256 &hash, Bitcoin_CTransaction &txOut, uint256 &hashBlock, bool fAllowSlow)
{
    Bitcoin_CBlockIndex *pindexSlow = NULL;
    {
        LOCK(bitcoin_mainState.cs_main);
        {
            if (bitcoin_mempool.lookup(hash, txOut))
            {
                return true;
            }
        }

        if (bitcoin_fTxIndex) {
            CDiskTxPos postx;
            if (bitcoin_pblocktree->ReadTxIndex(hash, postx)) {
                CAutoFile file(Bitcoin_OpenBlockFile(postx, true), SER_DISK, Bitcoin_Params().ClientVersion());
                if(!file && bitcoin_fTrimBlockFiles)
                	 return error("Bitcoin: %s : block file pos nonexistant, probably trimmed, file: %d, pos: %d, hash: %s", __func__, postx.nFile, postx.nPos, hash.GetHex());
                Bitcoin_CBlockHeader header;
                try {
                    file >> header;
                    fseek(file, postx.nTxOffset, SEEK_CUR);
                    file >> txOut;
                } catch (std::exception &e) {
                    return error("Bitcoin: %s : Deserialize or I/O error - %s", __func__, e.what());
                }
                hashBlock = header.GetHash();
                if (txOut.GetHash() != hash)
                    return error("Bitcoin: %s : txid mismatch", __func__);
                return true;
            }
        }

        if (fAllowSlow) { // use coin database to locate block that contains transaction, and scan it
            int nHeight = -1;
            {
                Bitcoin_CCoinsViewCache &view = *bitcoin_pcoinsTip;
                Bitcoin_CCoins coins;
                if (view.GetCoins(hash, coins))
                    nHeight = coins.nHeight;
            }
            if (nHeight > 0)
                pindexSlow = bitcoin_chainActive[nHeight];
        }
    }

    if (pindexSlow) {
        Bitcoin_CBlock block;
        if (Bitcoin_ReadBlockFromDisk(block, pindexSlow)) {
            BOOST_FOREACH(const Bitcoin_CTransaction &tx, block.vtx) {
                if (tx.GetHash() == hash) {
                    txOut = tx;
                    hashBlock = pindexSlow->GetBlockHash();
                    return true;
                }
            }
        } else {
            if(bitcoin_fTrimBlockFiles)
            	 return error("Bitcoin: %s : block file pos nonexistant, probably trimmed, file: %d, pos: %d, hash: %s", __func__, pindexSlow->GetBlockPos().nFile, pindexSlow->GetBlockPos().nPos, hash.GetHex());
        }
    }

    return false;
}






//////////////////////////////////////////////////////////////////////////////
//
// CBlock and CBlockIndex
//

bool Bitcoin_WriteBlockToDisk(Bitcoin_CBlock& block, CDiskBlockPos& pos)
{
    // Open history file to append
    CAutoFile fileout = CAutoFile(Bitcoin_OpenBlockFile(pos), SER_DISK, Bitcoin_Params().ClientVersion());
    if (!fileout)
        return error("Bitcoin: WriteBlockToDisk : OpenBlockFile failed");

    // Write index header
    unsigned int nSize = fileout.GetSerializeSize(block);
    fileout << FLATDATA(Bitcoin_Params().MessageStart()) << nSize;

    // Write block
    long fileOutPos = ftell(fileout);
    if (fileOutPos < 0)
        return error("Bitcoin: WriteBlockToDisk : ftell failed");
    pos.nPos = (unsigned int)fileOutPos;
    fileout << block;

    // Flush stdio buffers and commit to disk before returning
    fflush(fileout);
    if (!Bitcoin_IsInitialBlockDownload())
        FileCommit(fileout);

    return true;
}

bool Bitcoin_ReadBlockFromDisk(Bitcoin_CBlock& block, const CDiskBlockPos& pos)
{
    block.SetNull();

    // Open history file to read
    CAutoFile filein = CAutoFile(Bitcoin_OpenBlockFile(pos, true), SER_DISK, Bitcoin_Params().ClientVersion());
    if (!filein)
        return error("Bitcoin: ReadBlockFromDisk : OpenBlockFile failed");

    // Read block
    try {
        filein >> block;
    }
    catch (std::exception &e) {
        return error("Bitcoin: %s : Deserialize or I/O error - %s", __func__, e.what());
    }

    // Check the header
    if (!Bitcoin_CheckProofOfWork(block.GetHash(), block.nBits))
        return error("Bitcoin: ReadBlockFromDisk : Errors in block header");

    return true;
}

bool Bitcoin_ReadBlockFromDisk(Bitcoin_CBlock& block, const Bitcoin_CBlockIndex* pindex)
{
    if (!Bitcoin_ReadBlockFromDisk(block, pindex->GetBlockPos()))
        return false;
    if (block.GetHash() != pindex->GetBlockHash())
        return error("Bitcoin: ReadBlockFromDisk(CBlock&, CBlockIndex*) : GetHash() doesn't match index");
    return true;
}

uint256 static Bitcoin_GetOrphanRoot(const uint256& hash)
{
    map<uint256, COrphanBlock*>::iterator it = bitcoin_mapOrphanBlocks.find(hash);
    if (it == bitcoin_mapOrphanBlocks.end())
        return hash;

    // Work back to the first block in the orphan chain
    do {
        map<uint256, COrphanBlock*>::iterator it2 = bitcoin_mapOrphanBlocks.find(it->second->hashPrev);
        if (it2 == bitcoin_mapOrphanBlocks.end())
            return it->first;
        it = it2;
    } while(true);
}

// Remove a random orphan block (which does not have any dependent orphans).
void static Bitcoin_PruneOrphanBlocks()
{
    if (bitcoin_mapOrphanBlocksByPrev.size() <= (size_t)std::max((int64_t)0, GetArg("-bitcoin_maxorphanblocks", BITCOIN_DEFAULT_MAX_ORPHAN_BLOCKS)))
        return;

    // Pick a random orphan block.
    int pos = insecure_rand() % bitcoin_mapOrphanBlocksByPrev.size();
    std::multimap<uint256, COrphanBlock*>::iterator it = bitcoin_mapOrphanBlocksByPrev.begin();
    while (pos--) it++;

    // As long as this block has other orphans depending on it, move to one of those successors.
    do {
        std::multimap<uint256, COrphanBlock*>::iterator it2 = bitcoin_mapOrphanBlocksByPrev.find(it->second->hashBlock);
        if (it2 == bitcoin_mapOrphanBlocksByPrev.end())
            break;
        it = it2;
    } while(1);

    uint256 hash = it->second->hashBlock;
    delete it->second;
    bitcoin_mapOrphanBlocksByPrev.erase(it);
    bitcoin_mapOrphanBlocks.erase(hash);
}

int64_t Bitcoin_GetBlockValue(int nHeight, int64_t nFees)
{
    int64_t nSubsidy = 50 * COIN;
    int halvings = nHeight / Bitcoin_Params().SubsidyHalvingInterval();

    // Force block reward to zero when right shift is undefined.
    if (halvings >= 64)
        return nFees;

    // Subsidy is cut in half every 210,000 blocks which will occur approximately every 4 years.
    nSubsidy >>= halvings;

    return nSubsidy + nFees;
}

//This function only gives an approximation and should not be used for exact results
//That can only be achieved by parsing the full blockchain
uint64_t Bitcoin_GetTotalMonetaryBase(int nHeight) {
	uint64_t nTotalMonetaryBase = 0;
	uint64_t nSubsidy = 50 * COIN;
	const uint64_t nSubSidyHalvingInterval = Bitcoin_Params().SubsidyHalvingInterval();
	while(nHeight >= nSubSidyHalvingInterval) {
		nTotalMonetaryBase += nSubsidy * nSubSidyHalvingInterval;
		nHeight -= nSubSidyHalvingInterval;
		nSubsidy /= 2;
	}
	nTotalMonetaryBase += nSubsidy * nHeight;
	return nTotalMonetaryBase;
}

static const int64_t bitcoin_nTargetTimespan = 14 * 24 * 60 * 60; // two weeks
static const int64_t bitcoin_nTargetSpacing = 10 * 60;
static const int64_t bitcoin_nInterval = bitcoin_nTargetTimespan / bitcoin_nTargetSpacing;

//
// minimum amount of work that could possibly be required nTime after
// minimum work required was nBase
//
unsigned int Bitcoin_ComputeMinWork(unsigned int nBase, int64_t nTime)
{
    const uint256 &bnLimit = Bitcoin_Params().ProofOfWorkLimit();
    // Testnet has min-difficulty blocks
    // after bitcoin_nTargetSpacing*2 time between blocks:
    if (Bitcoin_TestNet() && nTime > bitcoin_nTargetSpacing*2)
        return bnLimit.GetCompact();

    uint256 bnResult;
    bnResult.SetCompact(nBase);
    while (nTime > 0 && bnResult < bnLimit)
    {
        // Maximum 400% adjustment...
        bnResult *= 4;
        // ... in best-case exactly 4-times-normal target time
        nTime -= bitcoin_nTargetTimespan*4;
    }
    if (bnResult > bnLimit)
        bnResult = bnLimit;
    return bnResult.GetCompact();
}

unsigned int Bitcoin_GetNextWorkRequired(const Bitcoin_CBlockIndex* pindexLast, const Bitcoin_CBlockHeader *pblock)
{
    unsigned int nProofOfWorkLimit = Bitcoin_Params().ProofOfWorkLimit().GetCompact();

    // Genesis block
    if (pindexLast == NULL)
        return nProofOfWorkLimit;

    // Only change once per interval
    if ((pindexLast->nHeight+1) % bitcoin_nInterval != 0)
    {
        if (Bitcoin_TestNet())
        {
            // Special difficulty rule for testnet:
            // If the new block's timestamp is more than 2* 10 minutes
            // then allow mining of a min-difficulty block.
            if (pblock->nTime > pindexLast->nTime + bitcoin_nTargetSpacing*2)
                return nProofOfWorkLimit;
            else
            {
                // Return the last non-special-min-difficulty-rules-block
                const Bitcoin_CBlockIndex* pindex = pindexLast;
                while (pindex->pprev && pindex->nHeight % bitcoin_nInterval != 0 && pindex->nBits == nProofOfWorkLimit)
                    pindex = (Bitcoin_CBlockIndex*)pindex->pprev;
                return pindex->nBits;
            }
        }
        return pindexLast->nBits;
    }

    // Go back by what we want to be 14 days worth of blocks
    const Bitcoin_CBlockIndex* pindexFirst = pindexLast;
    for (int i = 0; pindexFirst && i < bitcoin_nInterval-1; i++)
        pindexFirst = (Bitcoin_CBlockIndex*)pindexFirst->pprev;
    assert(pindexFirst);

    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - pindexFirst->GetBlockTime();
    LogPrintf("Bitcoin:   nActualTimespan = %d  before bounds\n", nActualTimespan);
    if (nActualTimespan < bitcoin_nTargetTimespan/4)
        nActualTimespan = bitcoin_nTargetTimespan/4;
    if (nActualTimespan > bitcoin_nTargetTimespan*4)
        nActualTimespan = bitcoin_nTargetTimespan*4;

    // Retarget
    uint256 bnNew;
    uint256 bnOld;
    bnNew.SetCompact(pindexLast->nBits);
    bnOld = bnNew;
    bnNew *= nActualTimespan;
    bnNew /= bitcoin_nTargetTimespan;

    if (bnNew > Bitcoin_Params().ProofOfWorkLimit())
        bnNew = Bitcoin_Params().ProofOfWorkLimit();

    /// debug print
    LogPrintf("Bitcoin: GetNextWorkRequired RETARGET\n");
    LogPrintf("Bitcoin: bitcoin_nTargetTimespan = %d    nActualTimespan = %d\n", bitcoin_nTargetTimespan, nActualTimespan);
    LogPrintf("Bitcoin: Before: %08x  %s\n", pindexLast->nBits, bnOld.ToString());
    LogPrintf("Bitcoin: After:  %08x  %s\n", bnNew.GetCompact(), bnNew.ToString());

    return bnNew.GetCompact();
}

bool Bitcoin_CheckProofOfWork(uint256 hash, unsigned int nBits)
{
    bool fNegative;
    bool fOverflow;
    uint256 bnTarget;
    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > Bitcoin_Params().ProofOfWorkLimit())
        return error("Bitcoin: CheckProofOfWork() : nBits below minimum work");

    // Check proof of work matches claimed amount
    if (hash > bnTarget)
        return error("Bitcoin: CheckProofOfWork() : hash doesn't match nBits");

    return true;
}

bool Bitcoin_IsInitialBlockDownload()
{
    LOCK(bitcoin_mainState.cs_main);
    if (bitcoin_mainState.ImportingOrReindexing() || bitcoin_chainActive.Height() < Checkpoints::Bitcoin_GetTotalBlocksEstimate())
        return true;
    static int64_t nLastUpdate;
    static CBlockIndexBase* pindexLastBest;
    if (bitcoin_chainActive.Tip() != pindexLastBest)
    {
        pindexLastBest = bitcoin_chainActive.Tip();
        nLastUpdate = GetTime();
    }
    return (GetTime() - nLastUpdate < 10 &&
    		bitcoin_chainActive.Tip()->GetBlockTime() < GetTime() - 24 * 60 * 60);
}

bool bitcoin_fLargeWorkForkFound = false;
bool bitcoin_fLargeWorkInvalidChainFound = false;
Bitcoin_CBlockIndex *bitcoin_pindexBestForkTip = NULL, *bitcoin_pindexBestForkBase = NULL;

void Bitcoin_CheckForkWarningConditions()
{
    AssertLockHeld(bitcoin_mainState.cs_main);
    // Before we get past initial download, we cannot reliably alert about forks
    // (we assume we don't get stuck on a fork before the last checkpoint)
    if (Bitcoin_IsInitialBlockDownload())
        return;

    // If our best fork is no longer within 72 blocks (+/- 12 hours if no one mines it)
    // of our head, drop it
    if (bitcoin_pindexBestForkTip && bitcoin_chainActive.Height() - bitcoin_pindexBestForkTip->nHeight >= 72)
        bitcoin_pindexBestForkTip = NULL;

    if (bitcoin_pindexBestForkTip || (bitcoin_pindexBestInvalid && bitcoin_pindexBestInvalid->nChainWork > bitcoin_chainActive.Tip()->nChainWork + (bitcoin_chainActive.Tip()->GetBlockWork() * 6)))
    {
        if (!bitcoin_fLargeWorkForkFound)
        {
            std::string strCmd = GetArg("-alertnotify", "");
            if (!strCmd.empty())
            {
                std::string warning = std::string("'Warning: Large-work fork detected, forking after block ") +
                                      bitcoin_pindexBestForkBase->phashBlock->ToString() + std::string("'");
                boost::replace_all(strCmd, "%s", warning);
                boost::thread t(runCommand, strCmd); // thread runs free
            }
        }
        if (bitcoin_pindexBestForkTip)
        {
            LogPrintf("Bitcoin: CheckForkWarningConditions: Warning: Large valid fork found\n  forking the chain at height %d (%s)\n  lasting to height %d (%s).\nChain state database corruption likely.\n",
                   bitcoin_pindexBestForkBase->nHeight, bitcoin_pindexBestForkBase->phashBlock->ToString(),
                   bitcoin_pindexBestForkTip->nHeight, bitcoin_pindexBestForkTip->phashBlock->ToString());
            bitcoin_fLargeWorkForkFound = true;
        }
        else
        {
            LogPrintf("Bitcoin: CheckForkWarningConditions: Warning: Found invalid chain at least ~6 blocks longer than our best chain.\nChain state database corruption likely.\n");
            bitcoin_fLargeWorkInvalidChainFound = true;
        }
    }
    else
    {
        bitcoin_fLargeWorkForkFound = false;
        bitcoin_fLargeWorkInvalidChainFound = false;
    }
}

void Bitcoin_CheckForkWarningConditionsOnNewFork(Bitcoin_CBlockIndex* pindexNewForkTip)
{
    AssertLockHeld(bitcoin_mainState.cs_main);
    // If we are on a fork that is sufficiently large, set a warning flag
    Bitcoin_CBlockIndex* pfork = pindexNewForkTip;
    Bitcoin_CBlockIndex* plonger = (Bitcoin_CBlockIndex*)bitcoin_chainActive.Tip();
    while (pfork && pfork != plonger)
    {
        while (plonger && plonger->nHeight > pfork->nHeight)
            plonger = (Bitcoin_CBlockIndex*)plonger->pprev;
        if (pfork == plonger)
            break;
        pfork = (Bitcoin_CBlockIndex*)pfork->pprev;
    }

    // We define a condition which we should warn the user about as a fork of at least 7 blocks
    // who's tip is within 72 blocks (+/- 12 hours if no one mines it) of ours
    // We use 7 blocks rather arbitrarily as it represents just under 10% of sustained network
    // hash rate operating on the fork.
    // or a chain that is entirely longer than ours and invalid (note that this should be detected by both)
    // We define it this way because it allows us to only store the highest fork tip (+ base) which meets
    // the 7-block condition and from this always have the most-likely-to-cause-warning fork
    if (pfork && (!bitcoin_pindexBestForkTip || (bitcoin_pindexBestForkTip && pindexNewForkTip->nHeight > bitcoin_pindexBestForkTip->nHeight)) &&
            pindexNewForkTip->nChainWork - pfork->nChainWork > (pfork->GetBlockWork() * 7) &&
            bitcoin_chainActive.Height() - pindexNewForkTip->nHeight < 72)
    {
        bitcoin_pindexBestForkTip = pindexNewForkTip;
        bitcoin_pindexBestForkBase = pfork;
    }

    Bitcoin_CheckForkWarningConditions();
}

// Requires cs_main.
void Bitcoin_Misbehaving(NodeId pnode, int howmuch)
{
    if (howmuch == 0)
        return;

    CNodeState *state = Bitcoin_State(pnode);
    if (state == NULL)
        return;

    state->nMisbehavior += howmuch;
    if (state->nMisbehavior >= GetArg("-banscore", 100))
    {
        LogPrintf("Bitcoin: Misbehaving: %s (%d -> %d) BAN THRESHOLD EXCEEDED\n", state->name, state->nMisbehavior-howmuch, state->nMisbehavior);
        state->fShouldBan = true;
    } else
        LogPrintf("Bitcoin: Misbehaving: %s (%d -> %d)\n", state->name, state->nMisbehavior-howmuch, state->nMisbehavior);
}

void static Bitcoin_InvalidChainFound(Bitcoin_CBlockIndex* pindexNew)
{
    if (!bitcoin_pindexBestInvalid || pindexNew->nChainWork > bitcoin_pindexBestInvalid->nChainWork)
    {
        bitcoin_pindexBestInvalid = pindexNew;
        uiInterface.NotifyBlocksChanged();
    }
    LogPrintf("Bitcoin: InvalidChainFound: invalid block=%s  height=%d  log2_work=%.8g  date=%s\n",
      pindexNew->GetBlockHash().ToString(), pindexNew->nHeight,
      log(pindexNew->nChainWork.getdouble())/log(2.0), DateTimeStrFormat("%Y-%m-%d %H:%M:%S",
      pindexNew->GetBlockTime()));
    LogPrintf("Bitcoin: InvalidChainFound:  current best=%s  height=%d  log2_work=%.8g  date=%s\n",
      bitcoin_chainActive.Tip()->GetBlockHash().ToString(), bitcoin_chainActive.Height(), log(bitcoin_chainActive.Tip()->nChainWork.getdouble())/log(2.0),
      DateTimeStrFormat("%Y-%m-%d %H:%M:%S", bitcoin_chainActive.Tip()->GetBlockTime()));
    Bitcoin_CheckForkWarningConditions();
}

void static Bitcoin_InvalidBlockFound(Bitcoin_CBlockIndex *pindex, const CValidationState &state) {
    int nDoS = 0;
    if (state.IsInvalid(nDoS)) {
        std::map<uint256, NodeId>::iterator it = bitcoin_mapBlockSource.find(pindex->GetBlockHash());
        if (it != bitcoin_mapBlockSource.end() && Bitcoin_State(it->second)) {
            CBlockReject reject = {state.GetRejectCode(), state.GetRejectReason(), pindex->GetBlockHash()};
            Bitcoin_State(it->second)->rejects.push_back(reject);
            if (nDoS > 0)
                Bitcoin_Misbehaving(it->second, nDoS);
        }
    }
    if (!state.CorruptionPossible()) {
        pindex->nStatus |= BLOCK_FAILED_VALID;
        bitcoin_pblocktree->WriteBlockIndex(Bitcoin_CDiskBlockIndex(pindex));
        bitcoin_setBlockIndexValid.erase(pindex);
        Bitcoin_InvalidChainFound(pindex);
    }
}

void Bitcoin_UpdateTime(Bitcoin_CBlockHeader& block, const Bitcoin_CBlockIndex* pindexPrev)
{
    block.nTime = max(pindexPrev->GetMedianTimePast()+1, GetAdjustedTime());

    // Updating time can change work required on testnet:
    if (Bitcoin_TestNet())
        block.nBits = Bitcoin_GetNextWorkRequired(pindexPrev, &block);
}











void Bitcoin_UpdateCoins(const Bitcoin_CTransaction& tx, CValidationState &state, Bitcoin_CCoinsViewCache &inputs, Bitcoin_CTxUndo &txundo, Credits_CCoinsViewCache &claim_inputs, Bitcoin_CTxUndoClaim &claim_txundo, int nHeight, const uint256 &txhash, const bool& fastForwardClaimState)
{
    bool ret;
    // mark inputs spent
    if (!tx.IsCoinBase()) {
        BOOST_FOREACH(const Bitcoin_CTxIn &txin, tx.vin) {
            Bitcoin_CCoins &coins = inputs.GetCoins(txin.prevout.hash);
            Bitcoin_CTxInUndo undo;
            ret = coins.Spend(txin.prevout, undo);
            assert(ret);
            txundo.vprevout.push_back(undo);

            if(fastForwardClaimState) {
            	Claim_CCoins &claim_coins = claim_inputs.Claim_GetCoins(txin.prevout.hash);
            	Bitcoin_CTxInUndoClaim claim_undo;
                ret = claim_coins.Spend(txin.prevout, claim_undo);
                assert(ret);
                claim_txundo.vprevout.push_back(claim_undo);
            }
        }
    }

    // add outputs
    ret = inputs.SetCoins(txhash, Bitcoin_CCoins(tx, nHeight));
    assert(ret);
    if(fastForwardClaimState) {
        ret = claim_inputs.Claim_SetCoins(txhash, Claim_CCoins(tx, nHeight));
        assert(ret);
    }
}

void Bitcoin_UpdateCoinsForClaim(const Bitcoin_CTransaction& tx, CValidationState &state, Credits_CCoinsViewCache &inputs, int64_t& nFeesOriginal, int64_t& nFeesClaimable, Bitcoin_CTxUndoClaim &txundo, int nHeight, const uint256 &txhash)
{
	assert(!tx.IsCoinBase());

	//Store the two different levels of values before spending.
	//These two are used to calculate the difference between the
	//original and the claimable state
	ClaimSum claimSum;
	inputs.Claim_GetValueIn(tx, claimSum);
	claimSum.Validate();

    bool ret;

    // mark inputs spent
	//Reduce the fee with the fraction that has already been claimed
	const int64_t nFeeOriginal = claimSum.nValueOriginalSum-tx.GetValueOut();
	const int64_t nFeeClaimable = ReduceByFraction(nFeeOriginal, claimSum.nValueClaimableSum, claimSum.nValueOriginalSum);
	assert(nFeeOriginal >= nFeeClaimable);

	nFeesOriginal += nFeeOriginal;
	nFeesClaimable += nFeeClaimable;

	BOOST_FOREACH(const Bitcoin_CTxIn &txin, tx.vin) {
		Claim_CCoins &coins = inputs.Claim_GetCoins(txin.prevout.hash);

		Bitcoin_CTxInUndoClaim undo;
		ret = coins.Spend(txin.prevout, undo);
		assert(ret);

		txundo.vprevout.push_back(undo);
	}

    // add outputs
    ret = inputs.Claim_SetCoins(txhash, Claim_CCoins(tx, nHeight, claimSum));
    assert(ret);
}
void Bitcoin_UpdateCoinsForClaimCoinbase(const Bitcoin_CTransaction& tx, CValidationState &state, Credits_CCoinsViewCache &inputs, int64_t& nFeesOriginal, int64_t& nFeesClaimable, int nHeight, const uint256 &txhash)
{
	assert(tx.IsCoinBase());

    bool ret;

	ClaimSum claimSum;
	claimSum.nValueOriginalSum = nFeesOriginal;
	claimSum.nValueClaimableSum = nFeesClaimable;

    // add outputs
    ret = inputs.Claim_SetCoins(txhash, Claim_CCoins(tx, nHeight, claimSum));
    assert(ret);
}

bool Bitcoin_CScriptCheck::operator()() const {
    const CScript &scriptSig = ptxTo->vin[nIn].scriptSig;
    if (!Bitcoin_VerifyScript(scriptSig, scriptPubKey, *ptxTo, nIn, nFlags, nHashType))
        return error("Bitcoin: CScriptCheck() : %s VerifySignature failed", ptxTo->GetHash().ToString());
    return true;
}

bool Bitcoin_VerifySignature(const Bitcoin_CCoins& txFrom, const Bitcoin_CTransaction& txTo, unsigned int nIn, unsigned int flags, int nHashType)
{
    return Bitcoin_CScriptCheck(txFrom, txTo, nIn, flags, nHashType)();
}

bool Bitcoin_CheckInputs(const Bitcoin_CTransaction& tx, CValidationState &state, Bitcoin_CCoinsViewCache &inputs, bool fScriptChecks, unsigned int flags, std::vector<Bitcoin_CScriptCheck> *pvChecks)
{
    if (!tx.IsCoinBase())
    {
        if (pvChecks)
            pvChecks->reserve(tx.vin.size());

        // This doesn't trigger the DoS code on purpose; if it did, it would make it easier
        // for an attacker to attempt to split the network.
        if (!inputs.HaveInputs(tx))
            return state.Invalid(error("Bitcoin: CheckInputs() : %s inputs unavailable", tx.GetHash().ToString()));

        // While checking, GetBestBlock() refers to the parent block.
        // This is also true for mempool checks.
        Bitcoin_CBlockIndex *pindexPrev = bitcoin_mapBlockIndex.find(inputs.GetBestBlock())->second;
        int nSpendHeight = pindexPrev->nHeight + 1;
        int64_t nValueIn = 0;
        int64_t nFees = 0;
        for (unsigned int i = 0; i < tx.vin.size(); i++)
        {
            const COutPoint &prevout = tx.vin[i].prevout;
            const Bitcoin_CCoins &coins = inputs.GetCoins(prevout.hash);

            // If prev is coinbase, check that it's matured
            if (coins.IsCoinBase()) {
                if (nSpendHeight - coins.nHeight < BITCOIN_COINBASE_MATURITY)
                    return state.Invalid(
                        error("Bitcoin: CheckInputs() : tried to spend coinbase at depth %d", nSpendHeight - coins.nHeight),
                        BITCOIN_REJECT_INVALID, "bad-txns-premature-spend-of-coinbase");
            }

            // Check for negative or overflow input values
            nValueIn += coins.vout[prevout.n].nValue;
            if (!Bitcoin_MoneyRange(coins.vout[prevout.n].nValue) || !Bitcoin_MoneyRange(nValueIn))
                return state.DoS(100, error("Bitcoin: CheckInputs() : txin values out of range"),
                                 BITCOIN_REJECT_INVALID, "bad-txns-inputvalues-outofrange");

        }

        if (nValueIn < tx.GetValueOut())
            return state.DoS(100, error("Bitcoin: CheckInputs() : %s value in < value out", tx.GetHash().ToString()),
                             BITCOIN_REJECT_INVALID, "bad-txns-in-belowout");

        // Tally transaction fees
        int64_t nTxFee = nValueIn - tx.GetValueOut();
        if (nTxFee < 0)
            return state.DoS(100, error("Bitcoin: CheckInputs() : %s nTxFee < 0", tx.GetHash().ToString()),
                             BITCOIN_REJECT_INVALID, "bad-txns-fee-negative");
        nFees += nTxFee;
        if (!Bitcoin_MoneyRange(nFees))
            return state.DoS(100, error("Bitcoin: CheckInputs() : nFees out of range"),
                             BITCOIN_REJECT_INVALID, "bad-txns-fee-outofrange");

        // The first loop above does all the inexpensive checks.
        // Only if ALL inputs pass do we perform expensive ECDSA signature checks.
        // Helps prevent CPU exhaustion attacks.

        // Skip ECDSA signature verification when connecting blocks
        // before the last block chain checkpoint. This is safe because block merkle hashes are
        // still computed and checked, and any change will be caught at the next checkpoint.
        if (fScriptChecks) {
            for (unsigned int i = 0; i < tx.vin.size(); i++) {
                const COutPoint &prevout = tx.vin[i].prevout;
                const Bitcoin_CCoins &coins = inputs.GetCoins(prevout.hash);

                // Verify signature
                Bitcoin_CScriptCheck check(coins, tx, i, flags, 0);
                if (pvChecks) {
                    pvChecks->push_back(Bitcoin_CScriptCheck());
                    check.swap(pvChecks->back());
                } else if (!check()) {
                    if (flags & STANDARD_NOT_MANDATORY_VERIFY_FLAGS) {
                        // Check whether the failure was caused by a
                        // non-mandatory script verification check, such as
                        // non-standard DER encodings or non-null dummy
                        // arguments; if so, don't trigger DoS protection to
                        // avoid splitting the network between upgraded and
                        // non-upgraded nodes.
                        Bitcoin_CScriptCheck check(coins, tx, i,
                                flags & ~STANDARD_NOT_MANDATORY_VERIFY_FLAGS, 0);
                        if (check())
                            return state.Invalid(false, BITCOIN_REJECT_NONSTANDARD, "non-mandatory-script-verify-flag");
                    }
                    // Failures of other flags indicate a transaction that is
                    // invalid in new blocks, e.g. a invalid P2SH. We DoS ban
                    // such nodes as they are not following the protocol. That
                    // said during an upgrade careful thought should be taken
                    // as to the correct behavior - we may want to continue
                    // peering with non-upgraded nodes even after a soft-fork
                    // super-majority vote has passed.
                    return state.DoS(100,false, BITCOIN_REJECT_INVALID, "mandatory-script-verify-flag-failed");
                }
            }
        }
    }

    return true;
}



bool Bitcoin_DisconnectBlock(Bitcoin_CBlock& block, CValidationState& state, Bitcoin_CBlockIndex* pindex, Bitcoin_CCoinsViewCache& view, Credits_CCoinsViewCache& claim_view, bool updateUndo, bool* pfClean)
{
    const bool fastForwardClaimState = FastForwardClaimStateFor(pindex->nHeight, pindex->GetBlockHash());

    assert(pindex->GetBlockHash() == view.GetBestBlock());
    if(fastForwardClaimState) {
        assert(claim_view.Claim_GetBestBlock() == pindex->GetBlockHash());
        assert(claim_view.Claim_GetBitcreditClaimTip() == uint256(1));
    }

    if (pfClean)
        *pfClean = false;

    bool fClean = true;

    Bitcoin_CBlockUndo blockUndo;
    CDiskBlockPos pos = pindex->GetUndoPos();
    if (pos.IsNull())
        return error("Bitcoin: DisconnectBlock() : no undo data available");
    if (!blockUndo.ReadFromDisk(Bitcoin_OpenUndoFile(pos, true), pos, pindex->pprev->GetBlockHash(), Bitcoin_NetParams()))
        return error("Bitcoin: DisconnectBlock() : failure reading undo data");

    if (blockUndo.vtxundo.size() + 1 != block.vtx.size())
        return error("Bitcoin: DisconnectBlock() : block and undo data inconsistent");

    // undo transactions in reverse order
    for (int i = block.vtx.size() - 1; i >= 0; i--) {
        const Bitcoin_CTransaction &tx = block.vtx[i];
        uint256 hash = tx.GetHash();

        // Check that all outputs are available and match the outputs in the block itself
        // exactly. Note that transactions with only provably unspendable outputs won't
        // have outputs available even in the block itself, so we handle that case
        // specially with outsEmpty.
        Bitcoin_CCoins outsEmpty;
        Bitcoin_CCoins &outs = view.HaveCoins(hash) ? view.GetCoins(hash) : outsEmpty;
        outs.ClearUnspendable();

        Bitcoin_CCoins outsBlock = Bitcoin_CCoins(tx, pindex->nHeight);
        // The Bitcoin_CCoins serialization does not serialize negative numbers.
        // No network rules currently depend on the version here, so an inconsistency is harmless
        // but it must be corrected before txout nversion ever influences a network rule.
        if (outsBlock.nVersion < 0)
            outs.nVersion = outsBlock.nVersion;
        if (outs != outsBlock)
            fClean = fClean && error("Bitcoin: DisconnectBlock() : added transaction mismatch? database corrupted");

        // remove outputs
        outs = Bitcoin_CCoins();

        // restore inputs
        if (i > 0) { // not coinbases
            const Bitcoin_CTxUndo &txundo = blockUndo.vtxundo[i-1];
            if (txundo.vprevout.size() != tx.vin.size())
                return error("Bitcoin: DisconnectBlock() : transaction and undo data inconsistent");
            for (unsigned int j = tx.vin.size(); j-- > 0;) {
                const COutPoint &out = tx.vin[j].prevout;
                const Bitcoin_CTxInUndo &undo = txundo.vprevout[j];
                Bitcoin_CCoins coins;
                view.GetCoins(out.hash, coins); // this can fail if the prevout was already entirely spent
                if (undo.nHeight != 0) {
                    // undo data contains height: this is the last output of the prevout tx being spent
                    if (!coins.IsPruned())
                        fClean = fClean && error("Bitcoin: DisconnectBlock() : undo data overwriting existing transaction");
                    coins = Bitcoin_CCoins();
                    coins.fCoinBase = undo.fCoinBase;
                    coins.nHeight = undo.nHeight;
                    coins.nVersion = undo.nVersion;
                } else {
                    if (coins.IsPruned())
                        fClean = fClean && error("Bitcoin: DisconnectBlock() : undo data adding output to missing transaction");
                }
                if (coins.IsAvailable(out.n))
                    fClean = fClean && error("Bitcoin: DisconnectBlock() : undo data overwriting existing output");
                if (coins.vout.size() < out.n+1)
                    coins.vout.resize(out.n+1);
                coins.vout[out.n] = undo.txout;
                if (!view.SetCoins(out.hash, coins))
                    return error("Bitcoin: DisconnectBlock() : cannot restore coin inputs");
            }
        }
    }

    // move best block pointer to prevout block
    view.SetBestBlock(pindex->pprev->GetBlockHash());

    if(fastForwardClaimState) {
        Bitcoin_CBlockUndoClaim claim_blockUndo;
        CDiskBlockPos claim_pos = pindex->GetUndoPosClaim();
        if (claim_pos.IsNull())
            return error("Bitcoin: DisconnectBlock() : no claim undo data available");
        if (!claim_blockUndo.ReadFromDisk(Bitcoin_OpenUndoFileClaim(claim_pos, true), claim_pos, pindex->pprev->GetBlockHash(), Bitcoin_NetParams()))
            return error("Bitcoin: DisconnectBlock() : failure reading claim undo data");

        if (claim_blockUndo.vtxundo.size() + 1 != block.vtx.size())
            return error("Bitcoin: DisconnectBlock() : block and claim undo data inconsistent");

        // undo transactions in reverse order
        for (int i = block.vtx.size() - 1; i >= 0; i--) {
            const Bitcoin_CTransaction &tx = block.vtx[i];
            uint256 hash = tx.GetHash();

            // Check that all outputs are available and match the outputs in the block itself
            // exactly. Note that transactions with only provably unspendable outputs won't
            // have outputs available even in the block itself, so we handle that case
            // specially with outsEmpty.
            Claim_CCoins claim_outsEmpty;
            Claim_CCoins &claim_outs = claim_view.Claim_HaveCoins(hash) ? claim_view.Claim_GetCoins(hash) : claim_outsEmpty;
            claim_outs.ClearUnspendable();

            Claim_CCoins claim_outsBlock = Claim_CCoins(tx, pindex->nHeight);
            // The Bitcoin_CCoins serialization does not serialize negative numbers.
            // No network rules currently depend on the version here, so an inconsistency is harmless
            // but it must be corrected before txout nversion ever influences a network rule.
            if (claim_outsBlock.nVersion < 0)
            	claim_outs.nVersion = claim_outsBlock.nVersion;
            if(!claim_outs.equalsExcludingClaimable(claim_outsBlock))
                fClean = fClean && error("Bitcoin: DisconnectBlock() : claim added transaction mismatch? database corrupted");

            // remove outputs
            claim_outs = Claim_CCoins();

            // restore inputs
            if (i > 0) { // not coinbases
                const Bitcoin_CTxUndoClaim &claim_txundo = claim_blockUndo.vtxundo[i-1];
                if (claim_txundo.vprevout.size() != tx.vin.size())
                    return error("Bitcoin: DisconnectBlock() : transaction and claim undo data inconsistent");
                for (unsigned int j = tx.vin.size(); j-- > 0;) {
                    const COutPoint &claim_out = tx.vin[j].prevout;
                    const Bitcoin_CTxInUndoClaim &claim_undo = claim_txundo.vprevout[j];
                    Claim_CCoins claim_coins;
                    claim_view.Claim_GetCoins(claim_out.hash, claim_coins); // this can fail if the prevout was already entirely spent
                    if (claim_undo.nHeight != 0) {
                        // undo data contains height: this is the last output of the prevout tx being spent
                        if (!claim_coins.IsPruned())
                            fClean = fClean && error("Bitcoin: DisconnectBlock() : claim undo data overwriting existing transaction");
                        claim_coins = Claim_CCoins();
                        claim_coins.fCoinBase = claim_undo.fCoinBase;
                        claim_coins.nHeight = claim_undo.nHeight;
                        claim_coins.nVersion = claim_undo.nVersion;
                    } else {
                        if (claim_coins.IsPruned())
                            fClean = fClean && error("Bitcoin: DisconnectBlock() : claim undo data adding output to missing transaction");
                    }
                    if (claim_coins.IsAvailable(claim_out.n))
                        fClean = fClean && error("Bitcoin: DisconnectBlock() : claim undo data overwriting existing output");
                    if (claim_coins.vout.size() < claim_out.n+1)
                    	claim_coins.vout.resize(claim_out.n+1);
                    claim_coins.vout[claim_out.n] = claim_undo.txout;
                    if (!claim_view.Claim_SetCoins(claim_out.hash, claim_coins))
                        return error("Bitcoin: DisconnectBlock() : cannot restore claim coin inputs");
                }
            }
        }

		if(updateUndo) {
			// We must delete the old undo claim data once it has been applied. The reason is that moving
			// up and down the bitcoin blockchain will produce different results depending on which bitcredit
			// block that is responsible for the moving of the tip. This is different from the "normal" chainstate
			//functions where undo data never change, since a block is immutable
			pindex->nUndoPosClaim = 0;
			pindex->nStatus = pindex->nStatus & ~BLOCK_HAVE_UNDO_CLAIM;

			Bitcoin_CDiskBlockIndex blockindex(pindex);
			if (!bitcoin_pblocktree->WriteBlockIndex(blockindex))
				return state.Abort(_("Failed to write block index"));
		}

		claim_view.Claim_SetBestBlock(pindex->pprev->GetBlockHash());
        //Claim tip set to 1 to indicate credits non-aligned chainstate. Only happens on bitcoin initial download
		claim_view.Claim_SetBitcreditClaimTip(uint256(1));
    }

    if (pfClean) {
        *pfClean = fClean;
        return true;
    } else {
        return fClean;
    }
}

void RevertCoin(const Bitcoin_CTransaction &tx, Bitcoin_CBlockIndex* pindex, Credits_CCoinsViewCache& view, bool& fClean) {
    const uint256 hash = tx.GetHash();

    //Disconnecting this low into the blockchain we will encounter two transactions where the coinbase tx has been reused, causing the transactions to be
    //unspendable from the first block where they appeared. Search for BIP30 in the codebase for more info. Block 91842 and 91880 are the blocks
    //that reuses the coinbases.
    if( (pindex->nHeight==91812 && tx.GetHash() == uint256("0xd5d27987d2a3dfc724e359870c6644b40e497bdc0589a033220fe15429d88599")) ||
    		(pindex->nHeight==91722 && tx.GetHash() == uint256("0xe3bf3d07d4b0375638d5f1db5255fe07ba2c4cb067cd81b84ee974b6585fb468")) ) {
    	return;
    }

    // Check that all outputs are available and match the outputs in the block itself
    // exactly. Note that transactions with only provably unspendable outputs won't
    // have outputs available even in the block itself, so we handle that case
    // specially with outsEmpty.
    Claim_CCoins outsEmpty;
    Claim_CCoins &outs = view.Claim_HaveCoins(hash) ? view.Claim_GetCoins(hash) : outsEmpty;
    outs.ClearUnspendable();

    Claim_CCoins outsBlock = Claim_CCoins(tx, pindex->nHeight);
    // The Claim_CCoins serialization does not serialize negative numbers.
    // No network rules currently depend on the version here, so an inconsistency is harmless
    // but it must be corrected before txout nversion ever influences a network rule.
    if (outsBlock.nVersion < 0)
        outs.nVersion = outsBlock.nVersion;
    if(!outs.equalsExcludingClaimable(outsBlock)) {
		fClean = fClean && error("Bitcoin: DisconnectBlockForClaim() : added transaction mismatch? database corrupted, \nfrom disk chainstate: \n%s, \nrecreated from block tx: \n%s", outs.ToString(), outsBlock.ToString());
    }
    // remove outputs
    outs = Claim_CCoins();
}

bool Bitcoin_DeleteBlockUndoClaimsFromDisk(CValidationState& state, std::vector<pair<Bitcoin_CBlockIndex*, Bitcoin_CBlockUndoClaim> > &vBlockUndoClaims) {
	std::vector<Bitcoin_CDiskBlockIndex> vblockindexes;

	for (unsigned int i = 0; i < vBlockUndoClaims.size(); i++)
    {
        Bitcoin_CBlockIndex *pindex = vBlockUndoClaims[i].first;

		// We must delete the old undo claim data once it has been applied. The reason is that moving
		// up and down the bitcoin blockchain will produce different results depending on which bitcredit
		// block that is responsible for the moving of the tip. This is different from the "normal" chainstate
		//functions where undo data never change, since a block is immutable
        pindex->nUndoPosClaim = 0;
        pindex->nStatus = pindex->nStatus & ~BLOCK_HAVE_UNDO_CLAIM;

		vblockindexes.push_back(Bitcoin_CDiskBlockIndex(pindex));
    }

	if(vblockindexes.size() > 0) {
		if (!bitcoin_pblocktree->BatchWriteBlockIndex(vblockindexes))
			return state.Abort(_("Failed to write block index"));
	}

    return true;
}

bool Bitcoin_DisconnectBlockForClaim(Bitcoin_CBlock& block, CValidationState& state, Bitcoin_CBlockIndex* pindex, Credits_CCoinsViewCache& view, bool updateUndo, std::vector<pair<Bitcoin_CBlockIndex*, Bitcoin_CBlockUndoClaim> > &vBlockUndoClaims, bool* pfClean)
{
    assert(pindex->GetBlockHash() == view.Claim_GetBestBlock());

    if (pfClean)
        *pfClean = false;

    bool fClean = true;

    Bitcoin_CBlockUndoClaim blockUndo;
    CDiskBlockPos pos = pindex->GetUndoPosClaim();
    if (pos.IsNull())
        return error("Bitcoin: DisconnectBlockForClaim() : no undo data available");
    if (!blockUndo.ReadFromDisk(Bitcoin_OpenUndoFileClaim(pos, true), pos, pindex->pprev->GetBlockHash(), Bitcoin_NetParams()))
        return error("Bitcoin: DisconnectBlockForClaim() : failure reading undo data");

    if (blockUndo.vtxundo.size() + 1 != block.vtx.size())
        return error("Bitcoin: Bitcoin_DisconnectBlockForClaim() : block and claim undo data inconsistent");

    //First undo coinbase, since it is added at the end for Bitcoin_ConnectBlockForClaim().
    //It is not strictly neccessary since coinbases cannot be spent in the same block as they exist
    //but it is done this way to keep it in sync with how Bitcoin_ConnectBlockForClaim() works
    RevertCoin(block.vtx[0], pindex, view, fClean);

    // then undo transactions in reverse order
    for (int i = block.vtx.size() - 1; i >= 1; i--) {
        const Bitcoin_CTransaction &tx = block.vtx[i];
        RevertCoin(tx, pindex, view, fClean);

		const Bitcoin_CTxUndoClaim &txundo = blockUndo.vtxundo[i-1];

		if (txundo.vprevout.size() != tx.vin.size())
			return error("Bitcoin: Bitcoin_DisconnectBlockForClaim() : transaction and claim undo data inconsistent");

		for (unsigned int j = tx.vin.size(); j-- > 0;) {
			const COutPoint &out = tx.vin[j].prevout;

			const Bitcoin_CTxInUndoClaim &undo = txundo.vprevout[j];

			Claim_CCoins coins;

			view.Claim_GetCoins(out.hash, coins); // this can fail if the prevout was already entirely spent

			if (undo.nHeight != 0) {
				// undo data contains height: this is the last output of the prevout tx being spent
					if (!coins.IsPruned())
						fClean = fClean && error("Bitcoin: Bitcoin_DisconnectBlockForClaim() : claim undo data overwriting existing transaction");

					coins = Claim_CCoins();
					coins.fCoinBase = undo.fCoinBase;
					coins.nHeight = undo.nHeight;
					coins.nVersion = undo.nVersion;
			} else {
				if (coins.IsPruned())
					fClean = fClean && error("Bitcoin: Bitcoin_DisconnectBlockForClaim() : claim undo data adding output to missing transaction");
			}

			if (coins.IsAvailable(out.n))
				fClean = fClean && error("Bitcoin: Bitcoin_DisconnectBlockForClaim() : claim undo data overwriting existing output");

			if (coins.vout.size() < out.n+1)
				coins.vout.resize(out.n+1);
			coins.vout[out.n] = undo.txout;

			if (!view.Claim_SetCoins(out.hash, coins))
				return error("Bitcoin: Bitcoin_DisconnectBlockForClaim() : cannot restore coin inputs");
		}
    }

    if(updateUndo) {
    	Bitcoin_CBlockUndoClaim empty;
		vBlockUndoClaims.push_back(make_pair(pindex, empty));
    }

    // move best block pointer to prevout block
    view.Claim_SetBestBlock(pindex->pprev->GetBlockHash());

    if (pfClean) {
        *pfClean = fClean;
        return true;
    } else {
        return fClean;
    }
}

void static Bitcoin_FlushBlockFile(bool fFinalize = false)
{
    LOCK(bitcoin_mainState.cs_LastBlockFile);

    CDiskBlockPos posOld(bitcoin_mainState.nLastBlockFile, 0);

    FILE *fileOld = Bitcoin_OpenBlockFile(posOld);
    if (fileOld) {
        if (fFinalize)
            TruncateFile(fileOld, bitcoin_mainState.infoLastBlockFile.nSize);
        FileCommit(fileOld);
        fclose(fileOld);
    }

    fileOld = Bitcoin_OpenUndoFile(posOld);
    if (fileOld) {
        if (fFinalize)
            TruncateFile(fileOld, bitcoin_mainState.infoLastBlockFile.nUndoSize);
        FileCommit(fileOld);
        fclose(fileOld);
    }

    fileOld = Bitcoin_OpenUndoFileClaim(posOld);
    if (fileOld) {
        if (fFinalize)
            TruncateFile(fileOld, bitcoin_mainState.infoLastBlockFile.nUndoSizeClaim);
        FileCommit(fileOld);
        fclose(fileOld);
    }

}

bool Bitcoin_FindUndoPos(MainState& mainState, CValidationState &state, int nFile, CDiskBlockPos &pos, unsigned int nAddSize);
bool Bitcoin_FindUndoPosClaim(MainState& mainState, CValidationState &state, int nFile, CDiskBlockPos &pos, unsigned int nAddSize);

static CCheckQueue<Bitcoin_CScriptCheck> bitcoin_scriptcheckqueue(128);

void Bitcoin_ThreadScriptCheck() {
    RenameThread("bitcoin_bitcoin-scriptch");
    bitcoin_scriptcheckqueue.Thread();
}

bool Bitcoin_ConnectBlock(Bitcoin_CBlock& block, CValidationState& state, Bitcoin_CBlockIndex* pindex, Bitcoin_CCoinsViewCache& view, Credits_CCoinsViewCache& claim_view, bool updateUndo, bool fJustCheck)
{
    AssertLockHeld(bitcoin_mainState.cs_main);
    // Check it again in case a previous version let a bad block in
    if (!Bitcoin_CheckBlock(block, state, !fJustCheck, !fJustCheck))
        return false;

    const bool fastForwardClaimState = FastForwardClaimStateFor(pindex->nHeight, pindex->GetBlockHash());

    // verify that the view's current state corresponds to the previous block
    uint256 hashPrevBlock = pindex->pprev == NULL ? uint256(0) : pindex->pprev->GetBlockHash();
    assert(hashPrevBlock == view.GetBestBlock());
    if(fastForwardClaimState) {
        assert(claim_view.Claim_GetBestBlock() == hashPrevBlock);
        if(pindex->pprev == NULL) {
        	assert(claim_view.Claim_GetBitcreditClaimTip() == uint256(0));
        } else {
        	assert(claim_view.Claim_GetBitcreditClaimTip() == uint256(1));
        }
    }

    // Special case for the genesis block, skipping connection of its transactions
    // (its coinbase is unspendable)
    if (block.GetHash() == Bitcoin_Params().HashGenesisBlock()) {
        view.SetBestBlock(pindex->GetBlockHash());
        if(fastForwardClaimState) {
            claim_view.Claim_SetBestBlock(pindex->GetBlockHash());
            //Claim tip set to 1 to indicate credits non-aligned chainstate. Only happens on bitcoin initial download
            claim_view.Claim_SetBitcreditClaimTip(uint256(1));
        }
        return true;
    }

    bool fScriptChecks = pindex->nHeight >= Checkpoints::Bitcoin_GetTotalBlocksEstimate();

    //!Avoid validation of blocks that are known to cause failures on Linux, probably related to openssl version.
    //Note that this validation skipping is not a problem since these blocks have been validated to be correct many
    //times by the Bitcoin network
    //The block height for simplified validation is arbitrary set. Should be updated as chain grows.
    bool skipVerifySignature = false;
    if (Credits_Params().NetworkID() == CChainParams::MAIN) {
    	skipVerifySignature = bitcoin_fSimplifiedBlockValidation && pindex->nHeight < 360000;
    } else {
    	//The Bitcoin chain is the same for testnet and regtest
    	skipVerifySignature = bitcoin_fSimplifiedBlockValidation && pindex->nHeight < 467000;
    }

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
                return state.DoS(100, error("Bitcoin: ConnectBlock() : tried to overwrite transaction"),
                                 BITCOIN_REJECT_INVALID, "bad-txns-BIP30");
        }
    }

    // BIP16 didn't become active until Apr 1 2012
    int64_t nBIP16SwitchTime = 1333238400;
    bool fStrictPayToScriptHash = (pindex->nTime >= nBIP16SwitchTime);

    unsigned int flags = SCRIPT_VERIFY_NOCACHE |
                         (fStrictPayToScriptHash ? SCRIPT_VERIFY_P2SH : SCRIPT_VERIFY_NONE);

    Bitcoin_CBlockUndo blockundo;
    Bitcoin_CBlockUndoClaim claim_blockundo;

    CCheckQueueControl<Bitcoin_CScriptCheck> control(fScriptChecks && bitcoin_nScriptCheckThreads ? &bitcoin_scriptcheckqueue : NULL);

    int64_t nStart = GetTimeMicros();
    int64_t nFees = 0;
    int nInputs = 0;
    unsigned int nSigOps = 0;
    CDiskTxPos pos(pindex->GetBlockPos(), GetSizeOfCompactSize(block.vtx.size()));
    std::vector<std::pair<uint256, CDiskTxPos> > vPos;
    vPos.reserve(block.vtx.size());
    for (unsigned int i = 0; i < block.vtx.size(); i++)
    {
        const Bitcoin_CTransaction &tx = block.vtx[i];

        nInputs += tx.vin.size();
        nSigOps += Bitcoin_GetLegacySigOpCount(tx);
        if (nSigOps > BITCOIN_MAX_BLOCK_SIGOPS)
            return state.DoS(100, error("Bitcoin: ConnectBlock() : too many sigops"),
                             BITCOIN_REJECT_INVALID, "bad-blk-sigops");

        if (!tx.IsCoinBase())
        {
            if (!view.HaveInputs(tx))
                return state.DoS(100, error("Bitcoin: ConnectBlock() : inputs missing/spent"),
                                 BITCOIN_REJECT_INVALID, "bad-txns-inputs-missingorspent");

            if (fStrictPayToScriptHash)
            {
                // Add in sigops done by pay-to-script-hash inputs;
                // this is to prevent a "rogue miner" from creating
                // an incredibly-expensive-to-validate block.
                nSigOps += Bitcoin_GetP2SHSigOpCount(tx, view);
                if (nSigOps > BITCOIN_MAX_BLOCK_SIGOPS)
                    return state.DoS(100, error("Bitcoin: ConnectBlock() : too many sigops"),
                                     BITCOIN_REJECT_INVALID, "bad-blk-sigops");
            }

            nFees += view.GetValueIn(tx)-tx.GetValueOut();

            if(!skipVerifySignature) {
				std::vector<Bitcoin_CScriptCheck> vChecks;
				if (!Bitcoin_CheckInputs(tx, state, view, fScriptChecks, flags, bitcoin_nScriptCheckThreads ? &vChecks : NULL))
					return false;
				control.Add(vChecks);
            }
        }

        Bitcoin_CTxUndo txundo;
        Bitcoin_CTxUndoClaim claim_txundo;
        Bitcoin_UpdateCoins(tx, state, view, txundo, claim_view, claim_txundo, pindex->nHeight, block.GetTxHash(i), fastForwardClaimState);
        if (!tx.IsCoinBase()) {
            blockundo.vtxundo.push_back(txundo);
			if(fastForwardClaimState) {
				claim_blockundo.vtxundo.push_back(claim_txundo);
			}
        }

        vPos.push_back(std::make_pair(block.GetTxHash(i), pos));
        pos.nTxOffset += ::GetSerializeSize(tx, SER_DISK, Bitcoin_Params().ClientVersion());
    }
    int64_t nTime = GetTimeMicros() - nStart;
    if (bitcoin_fBenchmark)
        LogPrintf("Bitcoin: - Connect %u transactions: %.2fms (%.3fms/tx, %.3fms/txin)\n", (unsigned)block.vtx.size(), 0.001 * nTime, 0.001 * nTime / block.vtx.size(), nInputs <= 1 ? 0 : 0.001 * nTime / (nInputs-1));

    if (block.vtx[0].GetValueOut() > Bitcoin_GetBlockValue(pindex->nHeight, nFees))
        return state.DoS(100,
                         error("Bitcoin: ConnectBlock() : coinbase pays too much (actual=%d vs limit=%d)",
                               block.vtx[0].GetValueOut(), Bitcoin_GetBlockValue(pindex->nHeight, nFees)),
                               BITCOIN_REJECT_INVALID, "bad-cb-amount");

    if (!control.Wait())
        return state.DoS(100, false);
    int64_t nTime2 = GetTimeMicros() - nStart;
    if (bitcoin_fBenchmark)
        LogPrintf("Bitcoin: - Verify %u txins: %.2fms (%.3fms/txin)\n", nInputs - 1, 0.001 * nTime2, nInputs <= 1 ? 0 : 0.001 * nTime2 / (nInputs-1));

    if (fJustCheck)
        return true;

    // Correct transaction counts.
    pindex->nTx = block.vtx.size();
    if (pindex->pprev)
        pindex->nChainTx = pindex->pprev->nChainTx + block.vtx.size();

    // Write undo information to disk
    if (pindex->GetUndoPos().IsNull() || !pindex->IsValid(BLOCK_VALID_SCRIPTS))
    {
        if (pindex->GetUndoPos().IsNull()) {
            CDiskBlockPos pos;
            if (!Bitcoin_FindUndoPos(bitcoin_mainState, state, pindex->nFile, pos, ::GetSerializeSize(blockundo, SER_DISK, Bitcoin_Params().ClientVersion()) + 40))
                return error("Bitcoin: ConnectBlock() : FindUndoPos failed");
            if (!blockundo.WriteToDisk(Bitcoin_OpenUndoFile(pos), pos, pindex->pprev->GetBlockHash(), Bitcoin_NetParams()))
                return state.Abort(_("Failed to write undo data"));

            // update nUndoPos in block index
            pindex->nUndoPos = pos.nPos;
            pindex->nStatus |= BLOCK_HAVE_UNDO;

			if(fastForwardClaimState) {
				if(updateUndo) {
					CDiskBlockPos claim_pos;
					if (!Bitcoin_FindUndoPosClaim(bitcoin_mainState, state, pindex->nFile, claim_pos, ::GetSerializeSize(claim_blockundo, SER_DISK, Bitcoin_Params().ClientVersion()) + 40))
						return error("Bitcoin: ConnectBlockForClaim() : FindUndoPosClaim failed");
					if (!claim_blockundo.WriteToDisk(Bitcoin_OpenUndoFileClaim(claim_pos), claim_pos, pindex->pprev->GetBlockHash(), Bitcoin_NetParams()))
						return state.Abort(_("Failed to write undo claim data"));

					pindex->nUndoPosClaim = claim_pos.nPos;
					pindex->nStatus |= BLOCK_HAVE_UNDO_CLAIM;
				}
            }
        }

        pindex->RaiseValidity(BLOCK_VALID_SCRIPTS);

        Bitcoin_CDiskBlockIndex blockindex(pindex);
        if (!bitcoin_pblocktree->WriteBlockIndex(blockindex))
            return state.Abort(_("Failed to write block index"));
    }

    if (bitcoin_fTxIndex)
        if (!bitcoin_pblocktree->WriteTxIndex(vPos))
            return state.Abort(_("Failed to write transaction index"));

    // add this block to the view's block chain
    bool ret;
    ret = view.SetBestBlock(pindex->GetBlockHash());
    assert(ret);
    if(fastForwardClaimState) {
    	ret = claim_view.Claim_SetBestBlock(pindex->GetBlockHash());
    	assert(ret);
        //Claim tip set to 1 to indicate credits non-aligned chainstate. Only happens on bitcoin initial download
    	ret = claim_view.Claim_SetBitcreditClaimTip(uint256(1));
    	assert(ret);
    }

    // Watch for transactions paying to me
    for (unsigned int i = 0; i < block.vtx.size(); i++)
        bitcoin_g_signals.SyncTransaction(block.GetTxHash(i), block.vtx[i], &block);

    return true;
}

bool Bitcoin_WriteBlockUndoClaimsToDisk(CValidationState& state, std::vector<pair<Bitcoin_CBlockIndex*, Bitcoin_CBlockUndoClaim> > &vBlockUndoClaims) {
	std::vector<Bitcoin_CDiskBlockIndex> vblockindexes;

	for (unsigned int i = 0; i < vBlockUndoClaims.size(); i++)
    {
        Bitcoin_CBlockIndex *pindex = vBlockUndoClaims[i].first;
        Bitcoin_CBlockUndoClaim &blockUndoClaim = vBlockUndoClaims[i].second;

		if (pindex->GetUndoPosClaim().IsNull() || !pindex->IsValid(BLOCK_VALID_SCRIPTS))
		{
			if (pindex->GetUndoPosClaim().IsNull()) {
				CDiskBlockPos pos;
				if (!Bitcoin_FindUndoPosClaim(bitcoin_mainState, state, pindex->nFile, pos, ::GetSerializeSize(blockUndoClaim, SER_DISK, Bitcoin_Params().ClientVersion()) + 40))
					return error("Bitcoin: ConnectBlockForClaim() : FindUndoPosClaim failed");
				if (!blockUndoClaim.WriteToDisk(Bitcoin_OpenUndoFileClaim(pos), pos, pindex->pprev->GetBlockHash(), Bitcoin_NetParams()))
					return state.Abort(_("Failed to write undo claim data"));

				pindex->nUndoPosClaim = pos.nPos;
				pindex->nStatus |= BLOCK_HAVE_UNDO_CLAIM;
			}

			vblockindexes.push_back(Bitcoin_CDiskBlockIndex(pindex));
		}
    }

	if(vblockindexes.size() > 0) {
		if (!bitcoin_pblocktree->BatchWriteBlockIndex(vblockindexes))
			return state.Abort(_("Failed to write block index"));
	}

	return true;
}

/**
 * This function should only be used for blocks in the main chain, it assumes that the blocks are
 * valid in many respects, therefore most block validation checks are skipped.
 */
bool Bitcoin_ConnectBlockForClaim(Bitcoin_CBlock& block, CValidationState& state, Bitcoin_CBlockIndex* pindex, Credits_CCoinsViewCache& view, bool updateUndo, std::vector<pair<Bitcoin_CBlockIndex*, Bitcoin_CBlockUndoClaim> > &vBlockUndoClaims)
{
    AssertLockHeld(bitcoin_mainState.cs_main);

    // verify that the view's current state corresponds to the previous block
    uint256 hashPrevBlock = pindex->pprev == NULL ? uint256(0) : pindex->pprev->GetBlockHash();
    assert(hashPrevBlock == view.Claim_GetBestBlock());

    // Special case for the genesis block, skipping connection of its transactions
    // (its coinbase is unspendable)
    if (block.GetHash() == Bitcoin_Params().HashGenesisBlock()) {
        view.Claim_SetBestBlock(pindex->GetBlockHash());
        return true;
    }

    Bitcoin_CBlockUndoClaim blockundo;
    int64_t nStart = GetTimeMicros();

    int nInputs = 0;

    int64_t nFeesOriginal = 0;
    int64_t nFeesClaimable = 0;

    //NOTE! First spend everyting BUT the coinbase. Once that is done we will have the
    //the values used to reduce the fee in the coinbase.
    //counter starts at 1
    for (unsigned int i = 1; i < block.vtx.size(); i++)
    {
        const Bitcoin_CTransaction &tx = block.vtx[i];

        nInputs += tx.vin.size();

        Bitcoin_CTxUndoClaim txundo;
        Bitcoin_UpdateCoinsForClaim(tx, state, view, nFeesOriginal, nFeesClaimable, txundo, pindex->nHeight, block.GetTxHash(i));
        blockundo.vtxundo.push_back(txundo);
    }

    //Now update the coinbase
    const Bitcoin_CTransaction &tx = block.vtx[0];
    nInputs += tx.vin.size();
    Bitcoin_UpdateCoinsForClaimCoinbase(tx, state, view, nFeesOriginal, nFeesClaimable, pindex->nHeight, block.GetTxHash(0));

    int64_t nTime = GetTimeMicros() - nStart;
    if (bitcoin_fBenchmark)
        LogPrintf("Bitcoin: - Claim: Connect %u transactions: %.2fms (%.3fms/tx, %.3fms/txin)\n", (unsigned)block.vtx.size(), 0.001 * nTime, 0.001 * nTime / block.vtx.size(), nInputs <= 1 ? 0 : 0.001 * nTime / (nInputs-1));

    // Store undo information for writing to disk on flush. Write undo only when we are connecting blocks "for real", otherwise just forget them
    if(updateUndo) {
		if (pindex->GetUndoPosClaim().IsNull() || !pindex->IsValid(BLOCK_VALID_SCRIPTS))
		{
			vBlockUndoClaims.push_back(make_pair(pindex, blockundo));
		}
    }

    // add this block to the view's block chain
    bool ret;
    ret = view.Claim_SetBestBlock(pindex->GetBlockHash());
    assert(ret);

    return true;
}

// Update the on-disk chain state.
bool static Bitcoin_WriteChainState(CValidationState &state) {
    static int64_t nLastWrite = 0;
    if (!Bitcoin_IsInitialBlockDownload() || bitcoin_pcoinsTip->GetCacheSize() > bitcoin_nCoinCacheSize || GetTimeMicros() > nLastWrite + 600*1000000) {
        // Typical Bitcoin_CCoins structures on disk are around 100 bytes in size.
        // Pushing a new one to the database can cause it to be written
        // twice (once in the log, and once in the tables). This is already
        // an overestimation, as most will delete an existing entry or
        // overwrite one. Still, use a conservative safety factor of 2.
        if (!Bitcoin_CheckDiskSpace(100 * 2 * 2 * bitcoin_pcoinsTip->GetCacheSize()))
            return state.Error("out of disk space");
        Bitcoin_FlushBlockFile();
        bitcoin_pblocktree->Sync();
        if (!bitcoin_pcoinsTip->Flush())
            return state.Abort(_("Failed to write to coin database"));
        nLastWrite = GetTimeMicros();
    }
    return true;
}

// Update chainActive and related internal data structures.
void static Bitcoin_UpdateTip(Bitcoin_CBlockIndex *pindexNew) {
    bitcoin_chainActive.SetTip(pindexNew);

//    // Update best block in wallet (so we can detect restored wallets)
    bool fIsInitialDownload = Bitcoin_IsInitialBlockDownload();
    if ((bitcoin_chainActive.Height() % 20160) == 0 || (!fIsInitialDownload && (bitcoin_chainActive.Height() % 144) == 0))
        bitcoin_g_signals.SetBestChain(bitcoin_chainActive.GetLocator());

    // New best block
    bitcoin_nTimeBestReceived = GetTime();
    bitcoin_mempool.AddTransactionsUpdated(1);
    LogPrintf("Bitcoin: UpdateTip: new best=%s  height=%d  log2_work=%.8g  tx=%lu  date=%s progress=%f\n",
      bitcoin_chainActive.Tip()->GetBlockHash().ToString(), bitcoin_chainActive.Height(), log(bitcoin_chainActive.Tip()->nChainWork.getdouble())/log(2.0), (unsigned long)bitcoin_chainActive.Tip()->nChainTx,
      DateTimeStrFormat("%Y-%m-%d %H:%M:%S", bitcoin_chainActive.Tip()->GetBlockTime()),
      Checkpoints::Bitcoin_GuessVerificationProgress((Bitcoin_CBlockIndex*)bitcoin_chainActive.Tip()));

    // Check the version of the last 100 blocks to see if we need to upgrade:
    if (!fIsInitialDownload)
    {
        int nUpgraded = 0;
        const Bitcoin_CBlockIndex* pindex = (Bitcoin_CBlockIndex*)bitcoin_chainActive.Tip();
        for (int i = 0; i < 100 && pindex != NULL; i++)
        {
            if (pindex->nVersion > Bitcoin_CBlock::CURRENT_VERSION)
                ++nUpgraded;
            pindex = (Bitcoin_CBlockIndex*)pindex->pprev;
        }
        if (nUpgraded > 0)
            LogPrintf("Bitcoin: SetBestChain: %d of last 100 blocks above version %d\n", nUpgraded, (int)Bitcoin_CBlock::CURRENT_VERSION);
        if (nUpgraded > 100/2)
            // strMiscWarning is read by GetWarnings(), called by Qt and the JSON-RPC code to warn the user:
            strMiscWarning = _("Bitcoin: Warning: This version is obsolete, upgrade required!");
    }
}

// Disconnect chainActive's tip.
bool static Bitcoin_DisconnectTip(CValidationState &state) {
    Bitcoin_CBlockIndex *pindexDelete = (Bitcoin_CBlockIndex*)bitcoin_chainActive.Tip();
    assert(pindexDelete);
    bitcoin_mempool.check(bitcoin_pcoinsTip);

    bool fastForwardClaimState = FastForwardClaimStateFor(pindexDelete->nHeight, pindexDelete->GetBlockHash());
    // Read block from disk.
    Bitcoin_CBlock block;
    if (!Bitcoin_ReadBlockFromDisk(block, pindexDelete))
        return state.Abort(_("Failed to read block"));
    // Apply the block atomically to the chain state.
    int64_t nStart = GetTimeMicros();
    {
        Bitcoin_CCoinsViewCache view(*bitcoin_pcoinsTip, true);
        Credits_CCoinsViewCache credits_view(*credits_pcoinsTip, true);
        if (!Bitcoin_DisconnectBlock(block, state, pindexDelete, view, credits_view, true))
            return error("Bitcoin: DisconnectTip() : DisconnectBlock %s failed", pindexDelete->GetBlockHash().ToString());
        assert(view.Flush());
        if(fastForwardClaimState) {
            LogPrintf("Bitcoin: DisconnectTip(): fastforward claim state applied\n");
        	assert(credits_view.Claim_Flush());
        }
    }
    if (bitcoin_fBenchmark)
        LogPrintf("Bitcoin: - Disconnect: %.2fms\n", (GetTimeMicros() - nStart) * 0.001);
    // Write the chain state to disk, if necessary.
    if (!Bitcoin_WriteChainState(state))
        return false;
    if(fastForwardClaimState) {
    	//Workaround to flush coins - used instead of Bitcredit_WriteChainState()
    	//If bitcredit_mainState.cs_main is used here, we will end up in deadlock
    	//This flushing will only happen on fast forward, which can be assumed to be initial block download or reindex
    	//Therefore, flush only when cache grows to big. This could cause corruption on power failure or similar
    	//Question is, do flushing to *_pcoinsTip require a lock? Does not seem that way in *_WriteChainState.
        if (credits_pcoinsTip->GetCacheSize() > bitcredit_nCoinCacheSize) {
            if (!credits_pcoinsTip->Claim_Flush())
                return state.Abort(_("Failed to write to coin database"));
        }
    }
    // Resurrect mempool transactions from the disconnected block.
    BOOST_FOREACH(const Bitcoin_CTransaction &tx, block.vtx) {
        // ignore validation errors in resurrected transactions
        list<Bitcoin_CTransaction> removed;
        CValidationState stateDummy;
        if (!tx.IsCoinBase())
            if (!Bitcoin_AcceptToMemoryPool(bitcoin_mempool, stateDummy, tx, false, NULL))
                bitcoin_mempool.remove(tx, removed, true);
    }
    bitcoin_mempool.check(bitcoin_pcoinsTip);
    // Update chainActive and related variables.
    Bitcoin_UpdateTip((Bitcoin_CBlockIndex*)pindexDelete->pprev);

    // Let wallets know transactions went from 1-confirmed to
    // 0-confirmed or conflicted:
    BOOST_FOREACH(const Bitcoin_CTransaction &tx, block.vtx) {
        Bitcoin_SyncWithWallets(tx.GetHash(), tx, NULL);
    }

    return true;
}

bool static Bitcoin_DisconnectTipForClaim(CValidationState &state, Credits_CCoinsViewCache& view, Bitcoin_CBlockIndex *pindex, bool updateUndo, std::vector<pair<Bitcoin_CBlockIndex*, Bitcoin_CBlockUndoClaim> > &vBlockUndoClaims) {
    // Read block from disk.
    Bitcoin_CBlock block;
    if (!Bitcoin_ReadBlockFromDisk(block, pindex))
        return state.Abort(_("Failed to read block"));
    // Apply the block atomically to the chain state.
    int64_t nStart = GetTimeMicros();
    {
        if (!Bitcoin_DisconnectBlockForClaim(block, state, pindex, view, updateUndo, vBlockUndoClaims))
            return error("Bitcoin: DisconnectTip() : DisconnectBlock %s failed", pindex->GetBlockHash().ToString());
    }
    if (bitcoin_fBenchmark)
        LogPrintf("Bitcoin: - Disconnect: %.2fms\n", (GetTimeMicros() - nStart) * 0.001);

    return true;
}

bool Bitcoin_CheckTrimBlockFile(FILE* fileIn, const int nTrimToTime)
{
    int64_t nStart = GetTimeMillis();

    int nLoaded = 0;
    try {
        CBufferedFile blkdat(fileIn, 2*BITCOIN_MAX_BLOCK_SIZE, BITCOIN_MAX_BLOCK_SIZE+8, SER_DISK, Bitcoin_Params().ClientVersion());
        uint64_t nStartByte = 0;
        uint64_t nRewind = blkdat.GetPos();
        while (blkdat.good() && !blkdat.eof()) {
            boost::this_thread::interruption_point();

            blkdat.SetPos(nRewind);
            nRewind++; // start one byte further next time, in case of failure
            blkdat.SetLimit(); // remove former limit
            unsigned int nSize = 0;
            try {
                // locate a header
                unsigned char buf[MESSAGE_START_SIZE];
                blkdat.FindByte(Bitcoin_Params().MessageStart()[0]);
                nRewind = blkdat.GetPos()+1;
                blkdat >> FLATDATA(buf);
                if (memcmp(buf, Bitcoin_Params().MessageStart(), MESSAGE_START_SIZE))
                    continue;
                // read size
                blkdat >> nSize;
                if (nSize < 80 || nSize > BITCOIN_MAX_BLOCK_SIZE)
                    continue;
            } catch (std::exception &e) {
                // no valid block header found; don't complain
                break;
            }
            try {
                // read block
                uint64_t nBlockPos = blkdat.GetPos();
                blkdat.SetLimit(nBlockPos + nSize);
                Bitcoin_CBlock block;
                blkdat >> block;
                nRewind = blkdat.GetPos();

                // check block
                if (nBlockPos >= nStartByte) {
                	if(block.nTime >= nTrimToTime) {
                		return false;
                	}
                    nLoaded++;
                }
            } catch (std::exception &e) {
                LogPrintf("Bitcoin: %s : Deserialize or I/O error - %s", __func__, e.what());
            }
        }
        fclose(fileIn);
    } catch(std::runtime_error &e) {
        AbortNode(_("Error: system error: ") + e.what());
    }
    if (nLoaded > 0) {
        LogPrintf("Bitcoin: Loaded %i blocks from file in %dms\n", nLoaded, GetTimeMillis() - nStart);
		return true;
    } else {
    	return false;
    }
}

bool Bitcoin_TrimBlockHistory(const Bitcoin_CBlockIndex * pTrimTo) {
    AssertLockHeld(bitcoin_mainState.cs_main);

    if(bitcoin_fTrimBlockFiles) {
		const int nTrimToTime = pTrimTo->nTime;
		const int nTrimToBlockFile = pTrimTo->nFile;

		int nFile = 0;
		while (true) {
			if(nFile >= nTrimToBlockFile)
				break;
			CDiskBlockPos pos(nFile, 0);
			FILE *fileTrim = Bitcoin_OpenBlockFile(pos, true);
			if (!fileTrim)
				continue;
			LogPrintf("Bitcoin: Scanning block file blk%05u.dat for trimming...\n", (unsigned int)nFile);

			if(Bitcoin_CheckTrimBlockFile(fileTrim, nTrimToTime)) {
				LogPrintf("Bitcoin: Trimming file blk%05u.dat...\n", (unsigned int)nFile);

				fileTrim = Bitcoin_OpenBlockFile(pos);
				if (fileTrim) {
					TruncateFile(fileTrim, 0);
					FileCommit(fileTrim);
					fclose(fileTrim);
				}

				fileTrim = Bitcoin_OpenUndoFile(pos);
				if (fileTrim) {
					TruncateFile(fileTrim, 0);
					FileCommit(fileTrim);
					fclose(fileTrim);
				}

				fileTrim = Bitcoin_OpenUndoFileClaim(pos);
				if (fileTrim) {
					TruncateFile(fileTrim, 0);
					FileCommit(fileTrim);
					fclose(fileTrim);
				}
			}
			nFile++;
		}

		bitcoin_mainState.nTrimToTime = nTrimToTime;
		bitcoin_pblocktree->WriteTrimToTime(bitcoin_mainState.nTrimToTime);
    }

	return true;
}

// Connect a new block to chainActive.
bool static Bitcoin_ConnectTip(CValidationState &state, Bitcoin_CBlockIndex *pindexNew) {
    assert(pindexNew->pprev == bitcoin_chainActive.Tip());
    bitcoin_mempool.check(bitcoin_pcoinsTip);

    bool fastForwardClaimState = FastForwardClaimStateFor(pindexNew->nHeight, pindexNew->GetBlockHash());
    // Read block from disk.
    Bitcoin_CBlock block;
    if (!Bitcoin_ReadBlockFromDisk(block, pindexNew))
        return state.Abort(_("Failed to read block"));
    // Apply the block atomically to the chain state.
    int64_t nStart = GetTimeMicros();
    {
        Bitcoin_CCoinsViewCache view(*bitcoin_pcoinsTip, true);
        Credits_CCoinsViewCache claim_view(*credits_pcoinsTip, true);
        CInv inv(MSG_BLOCK, pindexNew->GetBlockHash());
        if (!Bitcoin_ConnectBlock(block, state, pindexNew, view, claim_view, true, false)) {
            if (state.IsInvalid())
                Bitcoin_InvalidBlockFound(pindexNew, state);
            return error("Bitcoin: ConnectTip() : ConnectBlock %s failed", pindexNew->GetBlockHash().ToString());
        }
        bitcoin_mapBlockSource.erase(inv.hash);
        assert(view.Flush());
        if(fastForwardClaimState) {
            LogPrintf("Bitcoin: ConnectTip(): fastforward claim state applied\n");
        	assert(claim_view.Claim_Flush());
        }
    }
    if (bitcoin_fBenchmark)
        LogPrintf("Bitcoin: - Connect: %.2fms\n", (GetTimeMicros() - nStart) * 0.001);
    // Write the chain state to disk, if necessary.
    if (!Bitcoin_WriteChainState(state))
        return false;
    if(fastForwardClaimState) {
    	//Workaround to flush coins - used instead of Bitcredit_WriteChainState()
    	//If bitcredit_mainState.cs_main is used here, we will end up in deadlock
    	//This flushing will only happen on fast forward, which can be assumed to be initial block download or reindex
    	//Therefore, flush only when cache grows to big. This could cause corruption on power failure or similar
    	//Question is, do flushing to *_pcoinsTip require a lock? Does not seem that way in *_WriteChainState.
        if (credits_pcoinsTip->GetCacheSize() > bitcredit_nCoinCacheSize) {
            if (!credits_pcoinsTip->Claim_Flush())
                return state.Abort(_("Failed to write to coin database"));
        }
    }
    // Remove conflicting transactions from the mempool.
    list<Bitcoin_CTransaction> txConflicted;
    BOOST_FOREACH(const Bitcoin_CTransaction &tx, block.vtx) {
        list<Bitcoin_CTransaction> unused;
        bitcoin_mempool.remove(tx, unused);
        bitcoin_mempool.removeConflicts(tx, txConflicted);
    }
    bitcoin_mempool.check(bitcoin_pcoinsTip);
    // Update chainActive & related variables.
    Bitcoin_UpdateTip(pindexNew);

    if(fastForwardClaimState) {
    	//Every 20000 blocks, trim the bitcoin block files
		int trimFrequency = 20000;
		const Bitcoin_CBlockIndex * ptip = (Bitcoin_CBlockIndex*) bitcoin_chainActive.Tip();
		if (ptip->nHeight > trimFrequency && ptip->nHeight % trimFrequency == 0) {
			const Bitcoin_CBlockIndex * pTrimTo = bitcoin_chainActive[ptip->nHeight - trimFrequency];
			if (!Bitcoin_TrimBlockHistory(pTrimTo)) {
				return error("Bitcoin: ConnectTip() : Could not trim block history!");
			}
		}
    }

    // Tell wallet about transactions that went from mempool
    // to conflicted:
    BOOST_FOREACH(const Bitcoin_CTransaction &tx, txConflicted) {
        Bitcoin_SyncWithWallets(tx.GetHash(), tx, NULL);
    }
    // ... and about transactions that got confirmed:
    BOOST_FOREACH(const Bitcoin_CTransaction &tx, block.vtx) {
        Bitcoin_SyncWithWallets(tx.GetHash(), tx, &block);
    }
    return true;
}

// Everything is finalized from Bitcredit_Connect/DisconnectBlock
bool static Bitcoin_ConnectTipForClaim(CValidationState &state, Credits_CCoinsViewCache& view, Bitcoin_CBlockIndex *pindex, bool updateUndo, std::vector<pair<Bitcoin_CBlockIndex*, Bitcoin_CBlockUndoClaim> > &vBlockUndoClaims) {
    // Read block from disk.
    Bitcoin_CBlock block;
    if (!Bitcoin_ReadBlockFromDisk(block, pindex))
        return state.Abort(_("Claim: Failed to read block"));

    //Must be run to setup the merkle tree
    block.BuildMerkleTree();

    // Apply the block atomically to the chain state.
    int64_t nStart = GetTimeMicros();
    {
        if (!Bitcoin_ConnectBlockForClaim(block, state, pindex, view, updateUndo, vBlockUndoClaims)) {
            if (state.IsInvalid())
                Bitcoin_InvalidBlockFound(pindex, state);
            return error("Claim: ConnectTip() : ConnectBlock %s failed", pindex->GetBlockHash().ToString());
        }
    }

    if (bitcoin_fBenchmark)
        LogPrintf("Bitcoin: - Claim: Connect: %.2fms, height: %d\n", (GetTimeMicros() - nStart) * 0.001, pindex->nHeight);

    return true;
}

// Make chainMostWork correspond to the chain with the most work in it, that isn't
// known to be invalid (it's however far from certain to be valid).
void static Bitcoin_FindMostWorkChain() {
    Bitcoin_CBlockIndex *pindexNew = NULL;

    // In case the current best is invalid, do not consider it.
    while (bitcoin_chainMostWork.Tip() && (bitcoin_chainMostWork.Tip()->nStatus & BLOCK_FAILED_MASK)) {
        bitcoin_setBlockIndexValid.erase((Bitcoin_CBlockIndex*)bitcoin_chainMostWork.Tip());
        bitcoin_chainMostWork.SetTip((Bitcoin_CBlockIndex*)bitcoin_chainMostWork.Tip()->pprev);
    }

    do {
        // Find the best candidate header.
        {
            std::set<Bitcoin_CBlockIndex*, Bitcoin_CBlockIndexWorkComparator>::reverse_iterator it = bitcoin_setBlockIndexValid.rbegin();
            if (it == bitcoin_setBlockIndexValid.rend())
                return;
            pindexNew = *it;
        }

        // Check whether all blocks on the path between the currently active chain and the candidate are valid.
        // Just going until the active chain is an optimization, as we know all blocks in it are valid already.
        Bitcoin_CBlockIndex *pindexTest = pindexNew;
        bool fInvalidAncestor = false;
        while (pindexTest && !bitcoin_chainActive.Contains(pindexTest)) {
            if (!pindexTest->IsValid(BLOCK_VALID_TRANSACTIONS) || !(pindexTest->nStatus & BLOCK_HAVE_DATA)) {
                // Candidate has an invalid ancestor, remove entire chain from the set.
                if (bitcoin_pindexBestInvalid == NULL || pindexNew->nChainWork > bitcoin_pindexBestInvalid->nChainWork)
                    bitcoin_pindexBestInvalid = pindexNew;
                Bitcoin_CBlockIndex *pindexFailed = pindexNew;
                while (pindexTest != pindexFailed) {
                    pindexFailed->nStatus |= BLOCK_FAILED_CHILD;
                    bitcoin_setBlockIndexValid.erase(pindexFailed);
                    pindexFailed = (Bitcoin_CBlockIndex*)pindexFailed->pprev;
                }
                fInvalidAncestor = true;
                break;
            }
            pindexTest = (Bitcoin_CBlockIndex*)pindexTest->pprev;
        }
        if (fInvalidAncestor)
            continue;

        break;
    } while(true);

    // Check whether it's actually an improvement.
    if (bitcoin_chainMostWork.Tip() && !Bitcoin_CBlockIndexWorkComparator()((Bitcoin_CBlockIndex*)bitcoin_chainMostWork.Tip(), pindexNew))
        return;

    // We have a new best.
    bitcoin_chainMostWork.SetTip(pindexNew);
}

// Try to activate to the most-work chain (thereby connecting it).
bool Bitcoin_ActivateBestChain(CValidationState &state) {
    LOCK(bitcoin_mainState.cs_main);
    Bitcoin_CBlockIndex *pindexOldTip = (Bitcoin_CBlockIndex*)bitcoin_chainActive.Tip();
    bool fComplete = false;
    while (!fComplete) {
        Bitcoin_FindMostWorkChain();
        fComplete = true;

        // Check whether we have something to do.
        if (bitcoin_chainMostWork.Tip() == NULL) break;

        // Disconnect active blocks which are no longer in the best chain.
        while (bitcoin_chainActive.Tip() && !bitcoin_chainMostWork.Contains(bitcoin_chainActive.Tip())) {
            if (!Bitcoin_DisconnectTip(state))
                return false;
        }

        // Connect new blocks.
        while (!bitcoin_chainActive.Contains(bitcoin_chainMostWork.Tip())) {
            Bitcoin_CBlockIndex *pindexConnect = bitcoin_chainMostWork[bitcoin_chainActive.Height() + 1];
            if (!Bitcoin_ConnectTip(state, pindexConnect)) {
                if (state.IsInvalid()) {
                    // The block violates a consensus rule.
                    if (!state.CorruptionPossible())
                        Bitcoin_InvalidChainFound((Bitcoin_CBlockIndex*)bitcoin_chainMostWork.Tip());
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

    if (bitcoin_chainActive.Tip() != pindexOldTip) {
        std::string strCmd = GetArg("-blocknotify", "");
        if (!Bitcoin_IsInitialBlockDownload() && !strCmd.empty())
        {
            boost::replace_all(strCmd, "%s", bitcoin_chainActive.Tip()->GetBlockHash().GetHex());
            boost::thread t(runCommand, strCmd); // thread runs free
        }
    }

    return true;
}

void PrintClaimMovement(std::string prefix, Bitcoin_CBlockIndex* pCurrentIndex, const Bitcoin_CBlockIndex* pmoveToIndex) {
	if(pCurrentIndex->nHeight % 50 == 0 || pCurrentIndex->nHeight == pmoveToIndex->nHeight) {
		LogPrintf("%sblock=%s  height=%d  date=%s progress=%f\n",
				prefix,
				pCurrentIndex->phashBlock->GetHex(),
				pCurrentIndex->nHeight,
				DateTimeStrFormat("%Y-%m-%d %H:%M:%S", pCurrentIndex->nTime),
				Checkpoints::Bitcoin_GuessVerificationProgress(pCurrentIndex));
	}
	if(pCurrentIndex->nHeight % 50 == 0 && pCurrentIndex->nHeight != pmoveToIndex->nHeight) {
		LogPrintf("...\n");
	}
}

/** Move the position of the claim tip (a structure similar to chainstate + undo) */
bool Bitcoin_AlignClaimTip(const Credits_CBlockIndex * expectedCurrentBitcreditBlockIndex, const Credits_CBlockIndex *palignToBitcreditBlockIndex, Credits_CCoinsViewCache& view, CValidationState &state, bool updateUndo, std::vector<pair<Bitcoin_CBlockIndex*, Bitcoin_CBlockUndoClaim> > &vBlockUndoClaims) {
	LOCK(bitcoin_mainState.cs_main);

	const uint256 moveToBitcoinHash = palignToBitcreditBlockIndex->hashLinkedBitcoinBlock;
	std::map<uint256, Bitcoin_CBlockIndex*>::iterator mi = bitcoin_mapBlockIndex.find(moveToBitcoinHash);
	if (mi == bitcoin_mapBlockIndex.end()) {
        return state.Abort(strprintf(_("Referenced claim bitcoin block %s can not be found"), moveToBitcoinHash.ToString()));
    }
	const Bitcoin_CBlockIndex* pmoveToBitcoinIndex = (*mi).second;

	if(expectedCurrentBitcreditBlockIndex->nHeight < palignToBitcreditBlockIndex->nHeight) {
	    //Connecting credits block
		if(FastForwardClaimStateFor(pmoveToBitcoinIndex->nHeight, pmoveToBitcoinIndex->GetBlockHash())) {
	    	return true;
	    }
		//This section changes from fast forward claim update to standard claim update
	    if(view.Claim_GetBestBlock() == Credits_Params().FastForwardClaimBitcoinBlockHash() && (view.Claim_GetBitcreditClaimTip() == uint256(1))) {
	    	view.Claim_SetBitcreditClaimTip(expectedCurrentBitcreditBlockIndex->GetBlockHash());
	    }
	} else {
		//This section changes from standard claim update to fast forward claim update
	    if(view.Claim_GetBestBlock() == Credits_Params().FastForwardClaimBitcoinBlockHash() && (view.Claim_GetBitcreditClaimTip() != uint256(1))) {
	    	view.Claim_SetBitcreditClaimTip(uint256(1));
	    }

		//Disconnecting credits block
		const uint256 moveFromBitcoinHash = expectedCurrentBitcreditBlockIndex->hashLinkedBitcoinBlock;
		std::map<uint256, Bitcoin_CBlockIndex*>::iterator mi2 = bitcoin_mapBlockIndex.find(moveFromBitcoinHash);
		if (mi2 == bitcoin_mapBlockIndex.end()) {
	        return state.Abort(strprintf(_("Referenced claim bitcoin block %s can not be found"), moveFromBitcoinHash.ToString()));
	    }
		const Bitcoin_CBlockIndex* pmoveFromIndex = (*mi2).second;

		if(FastForwardClaimStateFor(pmoveFromIndex->nHeight, pmoveFromIndex->GetBlockHash())) {
	    	return true;
	    }

	}

	// verify that the view's current state corresponds to the previous block
    uint256 hashExpected = expectedCurrentBitcreditBlockIndex->GetBlockHash() == Credits_Params().GenesisBlock().GetHash() ? uint256(0) : expectedCurrentBitcreditBlockIndex->GetBlockHash();
    if(hashExpected != view.Claim_GetBitcreditClaimTip()) {
		return state.Abort(strprintf(_("Error moving claim tip. Expected active credits block: %s, was: %s"), hashExpected.GetHex(), view.Claim_GetBitcreditClaimTip().GetHex()));
    }

	if (!bitcoin_chainActive.Contains(pmoveToBitcoinIndex)) {
		return state.Abort(strprintf(_("Referenced claim block %s is not in active bitcoin block chain"), pmoveToBitcoinIndex->phashBlock->ToString()));
	}
	if(bitcoin_chainActive.Tip()->nHeight - pmoveToBitcoinIndex->nHeight < Credits_Params().AcceptDepthLinkedBitcoinBlock()) {
		return state.Abort(strprintf(_("Referenced claim block %s is not deep enough in the bitcoin blockchain"), pmoveToBitcoinIndex->phashBlock->ToString()));
	}

	// Initialize coinstip if not done yet. This code should never execute since the fast forward claim state should have handled the updating of the tip before this segment is reached.
	uint256 claimBestBlockHash = view.Claim_GetBestBlock();
	if (claimBestBlockHash == uint256(0)) {
		if (!Bitcoin_ConnectTipForClaim(state, view, bitcoin_chainActive.Genesis(), updateUndo, vBlockUndoClaims)) {
			return state.Abort(_("Could not connect genesis block for claim db."));
		}
	}
	claimBestBlockHash = view.Claim_GetBestBlock();
	//Set starting point for movement
	Bitcoin_CBlockIndex* pCurrentIndex;
	std::map<uint256, Bitcoin_CBlockIndex*>::iterator mi2 = bitcoin_mapBlockIndex.find(claimBestBlockHash);
	if (mi2 == bitcoin_mapBlockIndex.end()) {
		return state.Abort(strprintf(_("Referenced claim block %s can not be found"), claimBestBlockHash.ToString()));
	}
	pCurrentIndex = (*mi2).second;

	//Marker for dirty state
	view.Claim_SetBitcreditClaimTip(uint256(1));
	//Move up or down depending on the state of the claim chain
	if (pCurrentIndex->nHeight < pmoveToBitcoinIndex->nHeight) {
	    if (bitcredit_fBenchmark) {
			LogPrintf("Moving claim tip from height %d up to height %d\n", pCurrentIndex->nHeight, pmoveToBitcoinIndex->nHeight);
			if(pmoveToBitcoinIndex->nHeight - pCurrentIndex->nHeight > 1) {
				LogPrintf("...\n");
			}
	    }
		do {
			pCurrentIndex = bitcoin_chainActive.Next(pCurrentIndex);

			if (!Bitcoin_ConnectTipForClaim(state, view, pCurrentIndex, updateUndo, vBlockUndoClaims)) {
				PrintClaimMovement("ERROR!!! Bitcoin: ConnectTipClaim: error when connecting to ", pCurrentIndex, pmoveToBitcoinIndex);
				return state.Abort(_("Could not connect block for claim db."));
			}

		    if (bitcredit_fBenchmark)
		    	PrintClaimMovement("Bitcoin: ConnectTipClaim: moved to ", pCurrentIndex, pmoveToBitcoinIndex);
		} while (pCurrentIndex->nHeight < pmoveToBitcoinIndex->nHeight);

	} else if (pCurrentIndex->nHeight > pmoveToBitcoinIndex->nHeight) {
	    if (bitcredit_fBenchmark) {
			LogPrintf("Moving claim tip from height %d down to height %d\n", pCurrentIndex->nHeight, pmoveToBitcoinIndex->nHeight);
			if(pCurrentIndex->nHeight - pmoveToBitcoinIndex->nHeight > 1) {
				LogPrintf("...\n");
			}
	    }
		 do {
			if(!Bitcoin_DisconnectTipForClaim(state, view, pCurrentIndex, updateUndo, vBlockUndoClaims)) {
				PrintClaimMovement("ERROR!!! Bitcoin: DisconnectTipClaim: error when disconnecting from ", pCurrentIndex, pmoveToBitcoinIndex);
				return state.Abort(_("Could not disconnect block for claim db."));
			}

			pCurrentIndex = (Bitcoin_CBlockIndex*)pCurrentIndex->pprev;

			if (bitcredit_fBenchmark)
				PrintClaimMovement("Bitcoin: DisconnectTipClaim: disconnected to ", pCurrentIndex, pmoveToBitcoinIndex);
		} while (pCurrentIndex->nHeight > pmoveToBitcoinIndex->nHeight);
	}

	if(view.Claim_GetBestBlock() != moveToBitcoinHash) {
		return state.Abort(_("Claim tip not in correct position."));
	}

	if(!view.Claim_SetBitcreditClaimTip(palignToBitcreditBlockIndex->GetBlockHeader().GetHash())) {
		return state.Abort(_("Credits claim tip could not be set."));
	}

    return true;
}


Bitcoin_CBlockIndex* Bitcoin_AddToBlockIndex(Bitcoin_CBlockHeader& block)
{
    // Check for duplicate
    uint256 hash = block.GetHash();
    std::map<uint256, Bitcoin_CBlockIndex*>::iterator it = bitcoin_mapBlockIndex.find(hash);
    if (it != bitcoin_mapBlockIndex.end())
        return it->second;

    // Construct new block index object
    Bitcoin_CBlockIndex* pindexNew = new Bitcoin_CBlockIndex(block);
    assert(pindexNew);
    {
         LOCK(bitcoin_cs_nBlockSequenceId);
         pindexNew->nSequenceId = bitcoin_nBlockSequenceId++;
    }
    map<uint256, Bitcoin_CBlockIndex*>::iterator mi = bitcoin_mapBlockIndex.insert(make_pair(hash, pindexNew)).first;
    pindexNew->phashBlock = &((*mi).first);
    map<uint256, Bitcoin_CBlockIndex*>::iterator miPrev = bitcoin_mapBlockIndex.find(block.hashPrevBlock);
    if (miPrev != bitcoin_mapBlockIndex.end())
    {
        pindexNew->pprev = (*miPrev).second;
        pindexNew->nHeight = pindexNew->pprev->nHeight + 1;
    }
    pindexNew->nChainWork = (pindexNew->pprev ? pindexNew->pprev->nChainWork : 0) + pindexNew->GetBlockWork();
    pindexNew->RaiseValidity(BLOCK_VALID_TREE);

    return pindexNew;
}


// Mark a block as having its data received and checked (up to BLOCK_VALID_TRANSACTIONS).
bool Bitcoin_ReceivedBlockTransactions(const Bitcoin_CBlock &block, CValidationState& state, Bitcoin_CBlockIndex *pindexNew, const CDiskBlockPos& pos)
{
    pindexNew->nTx = block.vtx.size();
    if (pindexNew->pprev) {
        // Not the genesis block.
        if (pindexNew->pprev->nChainTx) {
            // This parent's block's total number transactions is known, so compute outs.
            pindexNew->nChainTx = pindexNew->pprev->nChainTx + pindexNew->nTx;
        } else {
            // The total number of transactions isn't known yet.
            // We will compute it when the block is connected.
            pindexNew->nChainTx = 0;
        }
    } else {
        // Genesis block.
        pindexNew->nChainTx = pindexNew->nTx;
    }
    pindexNew->nFile = pos.nFile;
    pindexNew->nDataPos = pos.nPos;
    pindexNew->nUndoPos = 0;
    pindexNew->nUndoPosClaim = 0;
    pindexNew->nStatus |= BLOCK_HAVE_DATA;

    if (pindexNew->RaiseValidity(BLOCK_VALID_TRANSACTIONS))
        bitcoin_setBlockIndexValid.insert(pindexNew);

    std::vector<pair<uint256, std::vector<COutPoint> > > vTxHashesWithInputs;
    for (unsigned int i = 0; i < block.vtx.size(); i++) {
		const Bitcoin_CTransaction& tx = block.vtx[i];

		std::vector<COutPoint> inputs;
		for (unsigned int j = 0; j < tx.vin.size(); j++) {
			inputs.push_back(tx.vin[j].prevout);
		}
		vTxHashesWithInputs.push_back(make_pair(tx.GetHash(), inputs));
	}

    Bitcoin_CDiskBlockIndex diskIndexNew(pindexNew);

    if (!bitcoin_pblocktree->WriteBlockIndex(diskIndexNew))
        return state.Abort(_("Failed to write block index"));

    if (!bitcoin_pblocktree->WriteBlockTxHashesWithInputs(diskIndexNew, vTxHashesWithInputs))
        return state.Abort(_("Failed to write block tx hashes"));

    // New best?
    if (!Bitcoin_ActivateBestChain(state))
        return false;

    LOCK(bitcoin_mainState.cs_main);
    if (pindexNew == bitcoin_chainActive.Tip())
    {
        // Clear fork warning if its no longer applicable
        Bitcoin_CheckForkWarningConditions();

        // Notify UI to display prev block's coinbase if it was ours
        static uint256 hashPrevBestCoinBase;
        bitcoin_g_signals.UpdatedTransaction(hashPrevBestCoinBase);
        hashPrevBestCoinBase = block.GetTxHash(0);
    } else
        Bitcoin_CheckForkWarningConditionsOnNewFork(pindexNew);

    if (!bitcoin_pblocktree->Flush())
        return state.Abort(_("Failed to sync block index"));

    uiInterface.NotifyBlocksChanged();
    return true;
}


bool Bitcoin_FindBlockPos(CValidationState &state, CDiskBlockPos &pos, unsigned int nAddSize, unsigned int nHeight, uint64_t nTime, bool fKnown = false)
{
    bool fUpdatedLast = false;

    LOCK(bitcoin_mainState.cs_LastBlockFile);

    if (fKnown) {
        if (bitcoin_mainState.nLastBlockFile != pos.nFile) {
        	bitcoin_mainState.nLastBlockFile = pos.nFile;
        	bitcoin_mainState.infoLastBlockFile.SetNull();
            bitcoin_pblocktree->ReadBlockFileInfo(bitcoin_mainState.nLastBlockFile, bitcoin_mainState.infoLastBlockFile);
            fUpdatedLast = true;
        }
    } else {
        while (bitcoin_mainState.infoLastBlockFile.nSize + nAddSize >= BITCOIN_MAX_BLOCKFILE_SIZE) {
            LogPrintf("Bitcoin: Leaving block file %i: %s\n", bitcoin_mainState.nLastBlockFile, bitcoin_mainState.infoLastBlockFile.ToString());
            Bitcoin_FlushBlockFile(true);
            bitcoin_mainState.nLastBlockFile++;
            bitcoin_mainState.infoLastBlockFile.SetNull();
            bitcoin_pblocktree->ReadBlockFileInfo(bitcoin_mainState.nLastBlockFile, bitcoin_mainState.infoLastBlockFile); // check whether data for the new file somehow already exist; can fail just fine
            fUpdatedLast = true;
        }
        pos.nFile = bitcoin_mainState.nLastBlockFile;
        pos.nPos = bitcoin_mainState.infoLastBlockFile.nSize;
    }

    bitcoin_mainState.infoLastBlockFile.nSize += nAddSize;
    bitcoin_mainState.infoLastBlockFile.AddBlock(nHeight, nTime);

    if (!fKnown) {
        unsigned int nOldChunks = (pos.nPos + BITCOIN_BLOCKFILE_CHUNK_SIZE - 1) / BITCOIN_BLOCKFILE_CHUNK_SIZE;
        unsigned int nNewChunks = (bitcoin_mainState.infoLastBlockFile.nSize + BITCOIN_BLOCKFILE_CHUNK_SIZE - 1) / BITCOIN_BLOCKFILE_CHUNK_SIZE;
        if (nNewChunks > nOldChunks) {
            if (Bitcoin_CheckDiskSpace(nNewChunks * BITCOIN_BLOCKFILE_CHUNK_SIZE - pos.nPos)) {
                FILE *file = Bitcoin_OpenBlockFile(pos);
                if (file) {
                    LogPrintf("Bitcoin: Pre-allocating up to position 0x%x in blk%05u.dat\n", nNewChunks * BITCOIN_BLOCKFILE_CHUNK_SIZE, pos.nFile);
                    AllocateFileRange(file, pos.nPos, nNewChunks * BITCOIN_BLOCKFILE_CHUNK_SIZE - pos.nPos);
                    fclose(file);
                }
            }
            else
                return state.Error("out of disk space");
        }
    }

    if (!bitcoin_pblocktree->WriteBlockFileInfo(bitcoin_mainState.nLastBlockFile, bitcoin_mainState.infoLastBlockFile))
        return state.Abort(_("Failed to write file info"));
    if (fUpdatedLast)
        bitcoin_pblocktree->WriteLastBlockFile(bitcoin_mainState.nLastBlockFile);

    return true;
}

bool Bitcoin_FindUndoPos(MainState& mainState, CValidationState &state, int nFile, CDiskBlockPos &pos, unsigned int nAddSize)
{
    pos.nFile = nFile;

    LOCK(mainState.cs_LastBlockFile);

    unsigned int nNewSize;
    if (nFile == mainState.nLastBlockFile) {
        pos.nPos = mainState.infoLastBlockFile.nUndoSize;
        nNewSize = (mainState.infoLastBlockFile.nUndoSize += nAddSize);
        if (!bitcoin_pblocktree->WriteBlockFileInfo(mainState.nLastBlockFile, mainState.infoLastBlockFile))
            return state.Abort(_("Failed to write block info"));
    } else {
    	CBlockFileInfo info;
        if (!bitcoin_pblocktree->ReadBlockFileInfo(nFile, info))
            return state.Abort(_("Failed to read block info"));
        pos.nPos = info.nUndoSize;
        nNewSize = (info.nUndoSize += nAddSize);
        if (!bitcoin_pblocktree->WriteBlockFileInfo(nFile, info))
            return state.Abort(_("Failed to write block info"));
    }

    unsigned int nOldChunks = (pos.nPos + BITCOIN_UNDOFILE_CHUNK_SIZE - 1) / BITCOIN_UNDOFILE_CHUNK_SIZE;
    unsigned int nNewChunks = (nNewSize + BITCOIN_UNDOFILE_CHUNK_SIZE - 1) / BITCOIN_UNDOFILE_CHUNK_SIZE;
    if (nNewChunks > nOldChunks) {
        if (Bitcoin_CheckDiskSpace(nNewChunks * BITCOIN_UNDOFILE_CHUNK_SIZE - pos.nPos)) {
            FILE *file = Bitcoin_OpenUndoFile(pos);
            if (file) {
                LogPrintf("Bitcoin: Pre-allocating up to position 0x%x in rev%05u.dat\n", nNewChunks * BITCOIN_UNDOFILE_CHUNK_SIZE, pos.nFile);
                AllocateFileRange(file, pos.nPos, nNewChunks * BITCOIN_UNDOFILE_CHUNK_SIZE - pos.nPos);
                fclose(file);
            }
        }
        else
            return state.Error("out of disk space");
    }

    return true;
}

bool Bitcoin_FindUndoPosClaim(MainState& mainState, CValidationState &state, int nFile, CDiskBlockPos &pos, unsigned int nAddSize)
{
    pos.nFile = nFile;

    LOCK(mainState.cs_LastBlockFile);

    unsigned int nNewSize;
    if (nFile == mainState.nLastBlockFile) {
        pos.nPos = mainState.infoLastBlockFile.nUndoSizeClaim;
        nNewSize = (mainState.infoLastBlockFile.nUndoSizeClaim += nAddSize);
        if (!bitcoin_pblocktree->WriteBlockFileInfo(mainState.nLastBlockFile, mainState.infoLastBlockFile))
            return state.Abort(_("Bitcoin: Failed to write block info"));
    } else {
    	CBlockFileInfo info;
        if (!bitcoin_pblocktree->ReadBlockFileInfo(nFile, info))
            return state.Abort(_("Bitcoin: Failed to read block info"));
        pos.nPos = info.nUndoSizeClaim;
        nNewSize = (info.nUndoSizeClaim += nAddSize);
        if (!bitcoin_pblocktree->WriteBlockFileInfo(nFile, info))
            return state.Abort(_("Bitcoin: Failed to write block info"));
    }

    unsigned int nOldChunks = (pos.nPos + BITCOIN_UNDOFILE_CHUNK_SIZE - 1) / BITCOIN_UNDOFILE_CHUNK_SIZE;
    unsigned int nNewChunks = (nNewSize + BITCOIN_UNDOFILE_CHUNK_SIZE - 1) / BITCOIN_UNDOFILE_CHUNK_SIZE;
    if (nNewChunks > nOldChunks) {
        if (Bitcoin_CheckDiskSpace(nNewChunks * BITCOIN_UNDOFILE_CHUNK_SIZE - pos.nPos)) {
            FILE *file = Bitcoin_OpenUndoFileClaim(pos);
            if (file) {
                LogPrintf("Bitcoin: Pre-allocating up to position 0x%x in cla%05u.dat\n", nNewChunks * BITCOIN_UNDOFILE_CHUNK_SIZE, pos.nFile);
                AllocateFileRange(file, pos.nPos, nNewChunks * BITCOIN_UNDOFILE_CHUNK_SIZE - pos.nPos);
                fclose(file);
            }
        }
        else
            return state.Error("out of disk space");
    }

    return true;
}


bool Bitcoin_CheckBlockHeader(const Bitcoin_CBlockHeader& block, CValidationState& state, bool fCheckPOW)
{
    // Check proof of work matches claimed amount
    if (fCheckPOW && !Bitcoin_CheckProofOfWork(block.GetHash(), block.nBits))
        return state.DoS(50, error("Bitcoin: CheckBlockHeader() : proof of work failed"),
                         BITCOIN_REJECT_INVALID, "high-hash");

    // Check timestamp
    if (block.GetBlockTime() > GetAdjustedTime() + 2 * 60 * 60)
        return state.Invalid(error("Bitcoin: CheckBlockHeader() : block timestamp too far in the future"),
                             BITCOIN_REJECT_INVALID, "time-too-new");

    Bitcoin_CBlockIndex* pcheckpoint = Checkpoints::Bitcoin_GetLastCheckpoint(bitcoin_mapBlockIndex);
    if (pcheckpoint && block.hashPrevBlock != (bitcoin_chainActive.Tip() ? bitcoin_chainActive.Tip()->GetBlockHash() : uint256(0)))
    {
        // Extra checks to prevent "fill up memory by spamming with bogus blocks"
        int64_t deltaTime = block.GetBlockTime() - pcheckpoint->nTime;
        if (deltaTime < 0)
        {
            return state.DoS(100, error("Bitcoin: CheckBlockHeader() : block with timestamp before last checkpoint"),
                             BITCOIN_REJECT_CHECKPOINT, "time-too-old");
        }
        bool fOverflow = false;
        uint256 bnNewBlock;
        bnNewBlock.SetCompact(block.nBits, NULL, &fOverflow);
        uint256 bnRequired;
        bnRequired.SetCompact(Bitcoin_ComputeMinWork(pcheckpoint->nBits, deltaTime));
        if (fOverflow || bnNewBlock > bnRequired)
        {
            return state.DoS(100, error("Bitcoin: CheckBlockHeader() : block with too little proof-of-work"),
                             BITCOIN_REJECT_INVALID, "bad-diffbits");
        }
    }

    return true;
}

bool Bitcoin_CheckBlock(const Bitcoin_CBlock& block, CValidationState& state, bool fCheckPOW, bool fCheckMerkleRoot)
{
    // These are checks that are independent of context
    // that can be verified before saving an orphan block.

    if (!Bitcoin_CheckBlockHeader(block, state, fCheckPOW))
        return false;

    // Size limits
    if (block.vtx.empty() || block.vtx.size() > BITCOIN_MAX_BLOCK_SIZE || ::GetSerializeSize(block, SER_NETWORK, BITCOIN_PROTOCOL_VERSION) > BITCOIN_MAX_BLOCK_SIZE)
        return state.DoS(100, error("Bitcoin: CheckBlock() : size limits failed"),
                         BITCOIN_REJECT_INVALID, "bad-blk-length");

    // First transaction must be coinbase, the rest must not be
    if (block.vtx.empty() || !block.vtx[0].IsCoinBase())
        return state.DoS(100, error("Bitcoin: CheckBlock() : first tx is not coinbase"),
                         BITCOIN_REJECT_INVALID, "bad-cb-missing");
    for (unsigned int i = 1; i < block.vtx.size(); i++)
        if (block.vtx[i].IsCoinBase())
            return state.DoS(100, error("Bitcoin: CheckBlock() : more than one coinbase"),
                             BITCOIN_REJECT_INVALID, "bad-cb-multiple");

    // Check transactions
    BOOST_FOREACH(const Bitcoin_CTransaction& tx, block.vtx)
        if (!Bitcoin_CheckTransaction(tx, state))
            return error("Bitcoin: CheckBlock() : CheckTransaction failed");

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
        return state.DoS(100, error("Bitcoin: CheckBlock() : duplicate transaction"),
                         BITCOIN_REJECT_INVALID, "bad-txns-duplicate", true);

    unsigned int nSigOps = 0;
    BOOST_FOREACH(const Bitcoin_CTransaction& tx, block.vtx)
    {
        nSigOps += Bitcoin_GetLegacySigOpCount(tx);
    }
    if (nSigOps > BITCOIN_MAX_BLOCK_SIGOPS)
        return state.DoS(100, error("Bitcoin: CheckBlock() : out-of-bounds SigOpCount"),
                         BITCOIN_REJECT_INVALID, "bad-blk-sigops", true);

    // Check merkle root
    if (fCheckMerkleRoot && block.hashMerkleRoot != block.vMerkleTree.back())
        return state.DoS(100, error("Bitcoin: CheckBlock() : hashMerkleRoot mismatch"),
                         BITCOIN_REJECT_INVALID, "bad-txnmrklroot", true);

    return true;
}

bool Bitcoin_AcceptBlockHeader(Bitcoin_CBlockHeader& block, CValidationState& state, Bitcoin_CBlockIndex** ppindex)
{
    AssertLockHeld(bitcoin_mainState.cs_main);
    // Check for duplicate
    uint256 hash = block.GetHash();
    std::map<uint256, Bitcoin_CBlockIndex*>::iterator miSelf = bitcoin_mapBlockIndex.find(hash);
    Bitcoin_CBlockIndex *pindex = NULL;
    if (miSelf != bitcoin_mapBlockIndex.end()) {
        pindex = miSelf->second;
        if (pindex->nStatus & BLOCK_FAILED_MASK)
            return state.Invalid(error("Bitcoin: AcceptBlock() : block is marked invalid"), 0, "duplicate");
    }

    // Get prev block index
    Bitcoin_CBlockIndex* pindexPrev = NULL;
    int nHeight = 0;
    if (hash != Bitcoin_Params().HashGenesisBlock()) {
        map<uint256, Bitcoin_CBlockIndex*>::iterator mi = bitcoin_mapBlockIndex.find(block.hashPrevBlock);
        if (mi == bitcoin_mapBlockIndex.end())
            return state.DoS(10, error("Bitcoin: AcceptBlock() : prev block not found"), 0, "bad-prevblk");
        pindexPrev = (*mi).second;
        nHeight = pindexPrev->nHeight+1;

        // Check proof of work
        if (block.nBits != Bitcoin_GetNextWorkRequired(pindexPrev, &block))
            return state.DoS(100, error("Bitcoin: AcceptBlock() : incorrect proof of work"),
                             BITCOIN_REJECT_INVALID, "bad-diffbits");

        // Check timestamp against prev
        if (block.GetBlockTime() <= pindexPrev->GetMedianTimePast())
            return state.Invalid(error("Bitcoin: AcceptBlock() : block's timestamp is too early"),
                                 BITCOIN_REJECT_INVALID, "time-too-old");

        // Check that the block chain matches the known block chain up to a checkpoint
        if (!Checkpoints::Bitcoin_CheckBlock(nHeight, hash))
            return state.DoS(100, error("Bitcoin: AcceptBlock() : rejected by checkpoint lock-in at %d", nHeight),
                             BITCOIN_REJECT_CHECKPOINT, "checkpoint mismatch");

        // Don't accept any forks from the main chain prior to last checkpoint
        Bitcoin_CBlockIndex* pcheckpoint = Checkpoints::Bitcoin_GetLastCheckpoint(bitcoin_mapBlockIndex);
        if (pcheckpoint && nHeight < pcheckpoint->nHeight)
            return state.DoS(100, error("Bitcoin: AcceptBlock() : forked chain older than last checkpoint (height %d)", nHeight));

        // Reject block.nVersion=1 blocks when 95% (75% on testnet) of the network has upgraded:
        if (block.nVersion < 2)
        {
            if ((!Bitcoin_TestNet() && Bitcoin_CBlockIndex::IsSuperMajority(2, pindexPrev, 950, 1000)) ||
                (Bitcoin_TestNet() && Bitcoin_CBlockIndex::IsSuperMajority(2, pindexPrev, 75, 100)))
            {
                return state.Invalid(error("Bitcoin: AcceptBlock() : rejected nVersion=1 block"),
                                     BITCOIN_REJECT_OBSOLETE, "bad-version");
            }
        }
    }

    if (pindex == NULL)
        pindex = Bitcoin_AddToBlockIndex(block);

    if (ppindex)
        *ppindex = pindex;

    return true;
}

bool Bitcoin_AcceptBlock(Bitcoin_CBlock& block, CValidationState& state, Bitcoin_CBlockIndex** ppindex, CDiskBlockPos* dbp, CNetParams * netParams)
{
    AssertLockHeld(bitcoin_mainState.cs_main);

    Bitcoin_CBlockIndex *&pindex = *ppindex;

    if (!Bitcoin_AcceptBlockHeader(block, state, &pindex))
        return false;

    if (!Bitcoin_CheckBlock(block, state)) {
        if (state.Invalid() && !state.CorruptionPossible()) {
            pindex->nStatus |= BLOCK_FAILED_VALID;
        }
        return false;
    }

    int nHeight = pindex->nHeight;
    uint256 hash = pindex->GetBlockHash();

    // Check that all transactions are finalized
    BOOST_FOREACH(const Bitcoin_CTransaction& tx, block.vtx)
        if (!Bitcoin_IsFinalTx(tx, nHeight, block.GetBlockTime())) {
            pindex->nStatus |= BLOCK_FAILED_VALID;
            return state.DoS(10, error("Bitcoin: AcceptBlock() : contains a non-final transaction"),
                             BITCOIN_REJECT_INVALID, "bad-txns-nonfinal");
        }

    // Enforce block.nVersion=2 rule that the coinbase starts with serialized block height
    if (block.nVersion >= 2)
    {
        // if 750 of the last 1,000 blocks are version 2 or greater (51/100 if testnet):
        if ((!Bitcoin_TestNet() && Bitcoin_CBlockIndex::IsSuperMajority(2, (Bitcoin_CBlockIndex*)pindex->pprev, 750, 1000)) ||
            (Bitcoin_TestNet() && Bitcoin_CBlockIndex::IsSuperMajority(2, (Bitcoin_CBlockIndex*)pindex->pprev, 51, 100)))
        {
            CScript expect = CScript() << nHeight;
            if (block.vtx[0].vin[0].scriptSig.size() < expect.size() ||
                !std::equal(expect.begin(), expect.end(), block.vtx[0].vin[0].scriptSig.begin())) {
                pindex->nStatus |= BLOCK_FAILED_VALID;
                return state.DoS(100, error("Bitcoin: AcceptBlock() : block height mismatch in coinbase"),
                                 BITCOIN_REJECT_INVALID, "bad-cb-height");
            }
        }
    }

    // Write block to history file
    try {
        unsigned int nBlockSize = ::GetSerializeSize(block, SER_DISK, Bitcoin_Params().ClientVersion());
        CDiskBlockPos blockPos;
        if (dbp != NULL)
            blockPos = *dbp;
        if (!Bitcoin_FindBlockPos(state, blockPos, nBlockSize+8, nHeight, block.nTime, dbp != NULL))
            return error("Bitcoin: AcceptBlock() : FindBlockPos failed");
        if (dbp == NULL)
            if (!Bitcoin_WriteBlockToDisk(block, blockPos))
                return state.Abort(_("Failed to write block"));
        if (!Bitcoin_ReceivedBlockTransactions(block, state, pindex, blockPos))
            return error("Bitcoin: AcceptBlock() : ReceivedBlockTransactions failed");
    } catch(std::runtime_error &e) {
        return state.Abort(_("System error: ") + e.what());
    }

    // Relay inventory, but don't relay old inventory during initial block download
    int nBlockEstimate = Checkpoints::Bitcoin_GetTotalBlocksEstimate();
    if (bitcoin_chainActive.Tip()->GetBlockHash() == hash)
    {
        LOCK(netParams->cs_vNodes);
        BOOST_FOREACH(CNode* pnode, netParams->vNodes)
            if (bitcoin_chainActive.Height() > (pnode->nStartingHeight != -1 ? pnode->nStartingHeight - 2000 : nBlockEstimate))
                pnode->PushInventory(CInv(MSG_BLOCK, hash));
    }

    return true;
}

bool Bitcoin_CBlockIndex::IsSuperMajority(int minVersion, const Bitcoin_CBlockIndex* pstart, unsigned int nRequired, unsigned int nToCheck)
{
    unsigned int nFound = 0;
    for (unsigned int i = 0; i < nToCheck && nFound < nRequired && pstart != NULL; i++)
    {
        if (pstart->nVersion >= minVersion)
            ++nFound;
        pstart = (Bitcoin_CBlockIndex*)pstart->pprev;
    }
    return (nFound >= nRequired);
}

void Bitcoin_PushGetBlocks(CNode* pnode, Bitcoin_CBlockIndex* pindexBegin, uint256 hashEnd)
{
    AssertLockHeld(bitcoin_mainState.cs_main);
    // Filter out duplicate requests
    if (pindexBegin == pnode->pindexLastGetBlocksBegin && hashEnd == pnode->hashLastGetBlocksEnd)
        return;
    pnode->pindexLastGetBlocksBegin = pindexBegin;
    pnode->hashLastGetBlocksEnd = hashEnd;

    pnode->PushMessage("getblocks", bitcoin_chainActive.GetLocator(pindexBegin), hashEnd);
}

bool Bitcoin_ProcessBlock(CValidationState &state, CNode* pfrom, Bitcoin_CBlock* pblock, CDiskBlockPos *dbp)
{
    AssertLockHeld(bitcoin_mainState.cs_main);

    // Check for duplicate
    uint256 hash = pblock->GetHash();
    if (bitcoin_mapBlockIndex.count(hash))
        return state.Invalid(error("Bitcoin: ProcessBlock() : already have block %d %s", bitcoin_mapBlockIndex[hash]->nHeight, hash.ToString()), 0, "duplicate");
    if (bitcoin_mapOrphanBlocks.count(hash))
        return state.Invalid(error("Bitcoin: ProcessBlock() : already have block (orphan) %s", hash.ToString()), 0, "duplicate");

    // Preliminary checks
    if (!Bitcoin_CheckBlock(*pblock, state))
        return error("Bitcoin: ProcessBlock() : CheckBlock FAILED");

    // If we don't already have its previous block (with full data), shunt it off to holding area until we get it
    std::map<uint256, Bitcoin_CBlockIndex*>::iterator it = bitcoin_mapBlockIndex.find(pblock->hashPrevBlock);
    if (pblock->hashPrevBlock != 0 && (it == bitcoin_mapBlockIndex.end() || !(it->second->nStatus & BLOCK_HAVE_DATA)))
    {
        LogPrintf("Bitcoin: ProcessBlock: ORPHAN BLOCK %lu, prev=%s\n", (unsigned long)bitcoin_mapOrphanBlocks.size(), pblock->hashPrevBlock.ToString());

        // Accept orphans as long as there is a node to request its parents from
        if (pfrom) {
            Bitcoin_PruneOrphanBlocks();
            COrphanBlock* pblock2 = new COrphanBlock();
            {
                CDataStream ss(SER_DISK, Bitcoin_Params().ClientVersion());
                ss << *pblock;
                pblock2->vchBlock = std::vector<unsigned char>(ss.begin(), ss.end());
            }
            pblock2->hashBlock = hash;
            pblock2->hashPrev = pblock->hashPrevBlock;
            bitcoin_mapOrphanBlocks.insert(make_pair(hash, pblock2));
            bitcoin_mapOrphanBlocksByPrev.insert(make_pair(pblock2->hashPrev, pblock2));

            // Ask this guy to fill in what we're missing
            Bitcoin_PushGetBlocks(pfrom, (Bitcoin_CBlockIndex*)bitcoin_chainActive.Tip(), Bitcoin_GetOrphanRoot(hash));
        }
        return true;
    }

    // Store to disk
    Bitcoin_CBlockIndex *pindex = NULL;
    bool ret = Bitcoin_AcceptBlock(*pblock, state, &pindex, dbp, Bitcoin_NetParams());
    if (!ret)
        return error("Bitcoin: ProcessBlock() : AcceptBlock FAILED");

    // Recursively process any orphan blocks that depended on this one
    vector<uint256> vWorkQueue;
    vWorkQueue.push_back(hash);
    for (unsigned int i = 0; i < vWorkQueue.size(); i++)
    {
        uint256 hashPrev = vWorkQueue[i];
        for (multimap<uint256, COrphanBlock*>::iterator mi = bitcoin_mapOrphanBlocksByPrev.lower_bound(hashPrev);
             mi != bitcoin_mapOrphanBlocksByPrev.upper_bound(hashPrev);
             ++mi)
        {
            Bitcoin_CBlock block;
            {
                CDataStream ss(mi->second->vchBlock, SER_DISK, Bitcoin_Params().ClientVersion());
                ss >> block;
            }
            block.BuildMerkleTree();
            // Use a dummy CValidationState so someone can't setup nodes to counter-DoS based on orphan resolution (that is, feeding people an invalid block based on LegitBlockX in order to get anyone relaying LegitBlockX banned)
            CValidationState stateDummy;
            Bitcoin_CBlockIndex *pindexChild = NULL;
            if (Bitcoin_AcceptBlock(block, stateDummy, &pindexChild, NULL, Bitcoin_NetParams()))
                vWorkQueue.push_back(mi->second->hashBlock);
            bitcoin_mapOrphanBlocks.erase(mi->second->hashBlock);
            delete mi->second;
        }
        bitcoin_mapOrphanBlocksByPrev.erase(hashPrev);
    }

    LogPrintf("Bitcoin: ProcessBlock: ACCEPTED\n");

    return true;
}








Bitcoin_CMerkleBlock::Bitcoin_CMerkleBlock(const Bitcoin_CBlock& block, CBloomFilter& filter)
{
    header = block.GetBlockHeader();

    vector<bool> vMatch;
    vector<uint256> vHashes;

    vMatch.reserve(block.vtx.size());
    vHashes.reserve(block.vtx.size());

    for (unsigned int i = 0; i < block.vtx.size(); i++)
    {
        uint256 hash = block.vtx[i].GetHash();
        if (filter.bitcoin_IsRelevantAndUpdate(block.vtx[i], hash))
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

bool AbortNode(const std::string &strMessage) {
    strMiscWarning = strMessage;
    LogPrintf("*** %s\n", strMessage);
    uiInterface.ThreadSafeMessageBox(strMessage, "", CClientUIInterface::MSG_ERROR);
    StartShutdown();
    return false;
}


bool Bitcoin_CheckDiskSpace(uint64_t nAdditionalBytes)
{
    uint64_t nFreeBytesAvailable = filesystem::space(GetDataDir()).available;

    // Check for nMinDiskSpace bytes (currently 50MB)
    if (nFreeBytesAvailable < bitcoin_nMinDiskSpace + nAdditionalBytes)
        return AbortNode(_("Error: Disk space is low!"));

    return true;
}

FILE* Bitcoin_OpenBlockFile(const CDiskBlockPos &pos, bool fReadOnly) {
    return OpenDiskFile(pos, "bitcoin_blocks", "blk", fReadOnly);
}

FILE* Bitcoin_OpenUndoFile(const CDiskBlockPos &pos, bool fReadOnly) {
    return OpenDiskFile(pos, "bitcoin_blocks", "rev", fReadOnly);
}
FILE* Bitcoin_OpenUndoFileClaim(const CDiskBlockPos &pos, bool fReadOnly) {
    return OpenDiskFile(pos, "bitcoin_blocks", "cla", fReadOnly);
}

bool static Bitcoin_LoadBlockIndexDB()
{
    if (!bitcoin_pblocktree->LoadBlockIndexGuts())
        return false;

    boost::this_thread::interruption_point();

    // Calculate nChainWork
    vector<pair<int, Bitcoin_CBlockIndex*> > vSortedByHeight;
    vSortedByHeight.reserve(bitcoin_mapBlockIndex.size());
    BOOST_FOREACH(const PAIRTYPE(uint256, Bitcoin_CBlockIndex*)& item, bitcoin_mapBlockIndex)
    {
        Bitcoin_CBlockIndex* pindex = item.second;
        vSortedByHeight.push_back(make_pair(pindex->nHeight, pindex));
    }
    sort(vSortedByHeight.begin(), vSortedByHeight.end());
    BOOST_FOREACH(const PAIRTYPE(int, Bitcoin_CBlockIndex*)& item, vSortedByHeight)
    {
        Bitcoin_CBlockIndex* pindex = item.second;
        pindex->nChainWork = (pindex->pprev ? pindex->pprev->nChainWork : 0) + pindex->GetBlockWork();
        pindex->nChainTx = (pindex->pprev ? pindex->pprev->nChainTx : 0) + pindex->nTx;
        if (pindex->IsValid(BLOCK_VALID_TRANSACTIONS))
            bitcoin_setBlockIndexValid.insert(pindex);
        if ((pindex->nStatus & BLOCK_FAILED_MASK) && (!bitcoin_pindexBestInvalid || pindex->nChainWork > bitcoin_pindexBestInvalid->nChainWork))
            bitcoin_pindexBestInvalid = pindex;
    }

    // Load block file info
    bitcoin_pblocktree->ReadLastBlockFile(bitcoin_mainState.nLastBlockFile);
    LogPrintf("Bitcoin: LoadBlockIndexDB(): last block file = %i\n", bitcoin_mainState.nLastBlockFile);
    bitcoin_pblocktree->ReadTrimToTime(bitcoin_mainState.nTrimToTime);
    LogPrintf("Bitcoin: LoadBlockIndexDB(): trim to time = %i\n", bitcoin_mainState.nTrimToTime);
    if (bitcoin_pblocktree->ReadBlockFileInfo(bitcoin_mainState.nLastBlockFile, bitcoin_mainState.infoLastBlockFile))
        LogPrintf("Bitcoin: LoadBlockIndexDB(): last block file info: %s\n", bitcoin_mainState.infoLastBlockFile.ToString());

    // Check presence of blk files
    LogPrintf("Bitcoin: Checking all blk files are present...\n");
    set<int> setBlkDataFiles;
    BOOST_FOREACH(const PAIRTYPE(uint256, Bitcoin_CBlockIndex*)& item, bitcoin_mapBlockIndex)
    {
        Bitcoin_CBlockIndex* pindex = item.second;
        if (pindex->nStatus & BLOCK_HAVE_DATA) {
            setBlkDataFiles.insert(pindex->nFile);
        }
    }
    for (std::set<int>::iterator it = setBlkDataFiles.begin(); it != setBlkDataFiles.end(); it++)
    {
        CDiskBlockPos pos(*it, 0);
        if (!CAutoFile(Bitcoin_OpenBlockFile(pos, true), SER_DISK, Bitcoin_Params().ClientVersion())) {
            return false;
        }
    }

    // Check whether we need to continue reindexing
    bool fReindexing = false;
    bitcoin_pblocktree->ReadReindexing(fReindexing);
    bitcoin_mainState.fReindex |= fReindexing;

    // Check whether we have a transaction index
    bitcoin_pblocktree->ReadFlag("txindex", bitcoin_fTxIndex);
    LogPrintf("Bitcoin: LoadBlockIndexDB(): transaction index %s\n", bitcoin_fTxIndex ? "enabled" : "disabled");

    // Check whether we have are trimming the bitcoin block files
    bitcoin_pblocktree->ReadFlag("bitcoin_trimblockfiles", bitcoin_fTrimBlockFiles);
    LogPrintf("Bitcoin: LoadBlockIndexDB(): trimming of block files %s\n", bitcoin_fTrimBlockFiles ? "enabled" : "disabled");

    // Load pointer to end of best chain
    std::map<uint256, Bitcoin_CBlockIndex*>::iterator it = bitcoin_mapBlockIndex.find(bitcoin_pcoinsTip->GetBestBlock());
    if (it == bitcoin_mapBlockIndex.end())
        return true;
    bitcoin_chainActive.SetTip(it->second);
    LogPrintf("Bitcoin: LoadBlockIndexDB(): hashBestChain=%s height=%d date=%s progress=%f\n",
        bitcoin_chainActive.Tip()->GetBlockHash().ToString(), bitcoin_chainActive.Height(),
        DateTimeStrFormat("%Y-%m-%d %H:%M:%S", bitcoin_chainActive.Tip()->GetBlockTime()),
        Checkpoints::Bitcoin_GuessVerificationProgress((Bitcoin_CBlockIndex*)bitcoin_chainActive.Tip()));

    return true;
}

Bitcoin_CVerifyDB::Bitcoin_CVerifyDB()
{
    uiInterface.ShowProgress(_("Bitcoin: Verifying blocks..."), 0);
}

Bitcoin_CVerifyDB::~Bitcoin_CVerifyDB()
{
    uiInterface.ShowProgress("", 100);
}

bool Bitcoin_CVerifyDB::VerifyDB(int nCheckLevel, int nCheckDepth)
{
    LOCK(bitcoin_mainState.cs_main);
    if (bitcoin_chainActive.Tip() == NULL || bitcoin_chainActive.Tip()->pprev == NULL)
        return true;

    // Verify blocks in the best chain
    if (nCheckDepth <= 0)
        nCheckDepth = 1000000000; // suffices until the year 19000
    if (nCheckDepth > bitcoin_chainActive.Height())
        nCheckDepth = bitcoin_chainActive.Height();
    nCheckLevel = std::max(0, std::min(4, nCheckLevel));
    LogPrintf("Bitcoin: Verifying last %i blocks at level %i\n", nCheckDepth, nCheckLevel);
    Bitcoin_CCoinsViewCache coins(*bitcoin_pcoinsTip, true);
    Credits_CCoinsViewCache claim_coins(*credits_pcoinsTip, true);
    Bitcoin_CBlockIndex* pindexState = (Bitcoin_CBlockIndex*)bitcoin_chainActive.Tip();
    Bitcoin_CBlockIndex* pindexFailure = NULL;
    int nGoodTransactions = 0;
    CValidationState state;
    for (Bitcoin_CBlockIndex* pindex = (Bitcoin_CBlockIndex*)bitcoin_chainActive.Tip(); pindex && pindex->pprev; pindex = (Bitcoin_CBlockIndex*)pindex->pprev)
    {
        boost::this_thread::interruption_point();
        uiInterface.ShowProgress(_("Bitcoin: Verifying blocks..."), std::max(1, std::min(99, (int)(((double)(bitcoin_chainActive.Height() - pindex->nHeight)) / (double)nCheckDepth * (nCheckLevel >= 4 ? 50 : 100)))));
        if (pindex->nHeight < bitcoin_chainActive.Height()-nCheckDepth)
            break;
        Bitcoin_CBlock block;
        // check level 0: read from disk
        if (!Bitcoin_ReadBlockFromDisk(block, pindex))
            return error("Bitcoin: VerifyDB() : *** ReadBlockFromDisk failed at %d, hash=%s", pindex->nHeight, pindex->GetBlockHash().ToString());
        // check level 1: verify block validity
        if (nCheckLevel >= 1 && !Bitcoin_CheckBlock(block, state))
            return error("Bitcoin: VerifyDB() : *** found bad block at %d, hash=%s\n", pindex->nHeight, pindex->GetBlockHash().ToString());
        // check level 2: verify undo validity
        if (nCheckLevel >= 2 && pindex) {
        	//Check the normal undo for the block
            Bitcoin_CBlockUndo undo;
            CDiskBlockPos pos = pindex->GetUndoPos();
            if (!pos.IsNull()) {
                if (!undo.ReadFromDisk(Bitcoin_OpenUndoFile(pos, true), pos, pindex->pprev->GetBlockHash(), Bitcoin_NetParams()))
                    return error("Bitcoin: VerifyDB() : *** found bad undo data at %d, hash=%s\n", pindex->nHeight, pindex->GetBlockHash().ToString());
            }

            //Check the claim undo for the block
            Bitcoin_CBlockUndoClaim undoClaim;
            CDiskBlockPos posClaim = pindex->GetUndoPosClaim();
            if (!posClaim.IsNull()) {
                if (!undoClaim.ReadFromDisk(Bitcoin_OpenUndoFileClaim(posClaim, true), posClaim, pindex->pprev->GetBlockHash(), Bitcoin_NetParams()))
                    return error("Bitcoin: VerifyDB() : *** found bad claim undo data at %d, hash=%s\n", pindex->nHeight, pindex->GetBlockHash().ToString());
            }
        }
        // check level 3: check for inconsistencies during memory-only disconnect of tip blocks
        if (nCheckLevel >= 3 && pindex == pindexState && (coins.GetCacheSize() + bitcoin_pcoinsTip->GetCacheSize()) <= 2*bitcoin_nCoinCacheSize + 32000) {
            bool fClean = true;
            if (!Bitcoin_DisconnectBlock(block, state, pindex, coins, claim_coins, false, &fClean))
                return error("Bitcoin: VerifyDB() : *** irrecoverable inconsistency in block data at %d, hash=%s", pindex->nHeight, pindex->GetBlockHash().ToString());
            pindexState = (Bitcoin_CBlockIndex*)pindex->pprev;
            if (!fClean) {
                nGoodTransactions = 0;
                pindexFailure = pindex;
            } else
                nGoodTransactions += block.vtx.size();
        }
    }
    if (pindexFailure)
        return error("Bitcoin: VerifyDB() : *** coin database inconsistencies found (last %i blocks, %i good transactions before that)\n", bitcoin_chainActive.Height() - pindexFailure->nHeight + 1, nGoodTransactions);

    // check level 4: try reconnecting blocks
    if (nCheckLevel >= 4) {
        Bitcoin_CBlockIndex *pindex = pindexState;
        while (pindex != bitcoin_chainActive.Tip()) {
            boost::this_thread::interruption_point();
            uiInterface.ShowProgress(_("Bitcoin: Verifying blocks..."), std::max(1, std::min(99, 100 - (int)(((double)(bitcoin_chainActive.Height() - pindex->nHeight)) / (double)nCheckDepth * 50))));
            pindex = bitcoin_chainActive.Next(pindex);
            Bitcoin_CBlock block;
            if (!Bitcoin_ReadBlockFromDisk(block, pindex))
                return error("Bitcoin: VerifyDB() : *** ReadBlockFromDisk failed at %d, hash=%s", pindex->nHeight, pindex->GetBlockHash().ToString());

            if (!Bitcoin_ConnectBlock(block, state, pindex, coins, claim_coins, false, false))
                return error("Bitcoin: VerifyDB() : *** found unconnectable block at %d, hash=%s", pindex->nHeight, pindex->GetBlockHash().ToString());
        }
    }

    LogPrintf("Bitcoin: No coin database inconsistencies in last %i blocks (%i transactions)\n", bitcoin_chainActive.Height() - pindexState->nHeight, nGoodTransactions);

    return true;
}

void Bitcoin_UnloadBlockIndex()
{
    bitcoin_mapBlockIndex.clear();
    bitcoin_setBlockIndexValid.clear();
    bitcoin_chainActive.SetTip(NULL);
    bitcoin_pindexBestInvalid = NULL;
}

bool Bitcoin_LoadBlockIndex()
{
    // Load block index from databases
    if (!bitcoin_mainState.fReindex && !Bitcoin_LoadBlockIndexDB())
        return false;
    return true;
}


bool Bitcoin_InitBlockIndex() {
    LOCK(bitcoin_mainState.cs_main);
    // Check whether we're already initialized
    if (bitcoin_chainActive.Genesis() != NULL)
        return true;

    // Use the provided setting for -txindex in the new database
    bitcoin_fTxIndex = GetBoolArg("-txindex", false);
    bitcoin_pblocktree->WriteFlag("txindex", bitcoin_fTxIndex);

    // Use the provided setting for -bitcoin_trimblockfiles in the new database
    bitcoin_fTrimBlockFiles = GetBoolArg("-bitcoin_trimblockfiles", true);
    bitcoin_pblocktree->WriteFlag("bitcoin_trimblockfiles", bitcoin_fTrimBlockFiles);

    LogPrintf("Bitcoin: Initializing databases...\n");

    // Only add the genesis block if not reindexing (in which case we reuse the one already on disk)
    if (!bitcoin_mainState.fReindex) {
        try {
            Bitcoin_CBlock &block = const_cast<Bitcoin_CBlock&>(Bitcoin_Params().GenesisBlock());
            // Start new block file
            unsigned int nBlockSize = ::GetSerializeSize(block, SER_DISK, Bitcoin_Params().ClientVersion());
            CDiskBlockPos blockPos;
            CValidationState state;
            if (!Bitcoin_FindBlockPos(state, blockPos, nBlockSize+8, 0, block.nTime))
                return error("Bitcoin: LoadBlockIndex() : FindBlockPos failed");
            if (!Bitcoin_WriteBlockToDisk(block, blockPos))
                return error("Bitcoin: LoadBlockIndex() : writing genesis block to disk failed");
            Bitcoin_CBlockIndex *pindex = Bitcoin_AddToBlockIndex(block);
            if (!Bitcoin_ReceivedBlockTransactions(block, state, pindex, blockPos))
                return error("Bitcoin: LoadBlockIndex() : genesis block not accepted");
        } catch(std::runtime_error &e) {
            return error("Bitcoin: LoadBlockIndex() : failed to initialize block database: %s", e.what());
        }
    }

    return true;
}



void Bitcoin_PrintBlockTree()
{
    AssertLockHeld(bitcoin_mainState.cs_main);
    // pre-compute tree structure
    map<Bitcoin_CBlockIndex*, vector<Bitcoin_CBlockIndex*> > mapNext;
    for (map<uint256, Bitcoin_CBlockIndex*>::iterator mi = bitcoin_mapBlockIndex.begin(); mi != bitcoin_mapBlockIndex.end(); ++mi)
    {
        Bitcoin_CBlockIndex* pindex = (*mi).second;
        mapNext[(Bitcoin_CBlockIndex*)pindex->pprev].push_back(pindex);
        // test
        //while (rand() % 3 == 0)
        //    mapNext[pindex->pprev].push_back(pindex);
    }

    vector<pair<int, Bitcoin_CBlockIndex*> > vStack;
    vStack.push_back(make_pair(0, bitcoin_chainActive.Genesis()));

    int nPrevCol = 0;
    while (!vStack.empty())
    {
        int nCol = vStack.back().first;
        Bitcoin_CBlockIndex* pindex = vStack.back().second;
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
        Bitcoin_CBlock block;
        if(Bitcoin_ReadBlockFromDisk(block, pindex)) {
            LogPrintf("%d (blk%05u.dat:0x%x)  %s  tx %u\n",
                pindex->nHeight,
                pindex->GetBlockPos().nFile, pindex->GetBlockPos().nPos,
                DateTimeStrFormat("%Y-%m-%d %H:%M:%S", block.GetBlockTime()),
                block.vtx.size());
		} else {
			if(bitcoin_fTrimBlockFiles) {
				LogPrintf("Bitcoin_Bitcoin_PrintBlockTree: Read attempt for missing block file, probably due to trimmed block files: %s\n", pindex->phashBlock->GetHex());
			}
		}

        // put the main time-chain first
        vector<Bitcoin_CBlockIndex*>& vNext = mapNext[pindex];
        for (unsigned int i = 0; i < vNext.size(); i++)
        {
            if (bitcoin_chainActive.Next(vNext[i]))
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

bool Bitcoin_LoadExternalBlockFile(FILE* fileIn, CDiskBlockPos *dbp)
{
    int64_t nStart = GetTimeMillis();

    int nLoaded = 0;
    try {
        CBufferedFile blkdat(fileIn, 2*BITCOIN_MAX_BLOCK_SIZE, BITCOIN_MAX_BLOCK_SIZE+8, SER_DISK, Bitcoin_Params().ClientVersion());
        uint64_t nStartByte = 0;
        if (dbp) {
            // (try to) skip already indexed part
        	CBlockFileInfo info;
            if (bitcoin_pblocktree->ReadBlockFileInfo(dbp->nFile, info)) {
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
                unsigned char buf[MESSAGE_START_SIZE];
                blkdat.FindByte(Bitcoin_Params().MessageStart()[0]);
                nRewind = blkdat.GetPos()+1;
                blkdat >> FLATDATA(buf);
                if (memcmp(buf, Bitcoin_Params().MessageStart(), MESSAGE_START_SIZE))
                    continue;
                // read size
                blkdat >> nSize;
                if (nSize < 80 || nSize > BITCOIN_MAX_BLOCK_SIZE)
                    continue;
            } catch (std::exception &e) {
                // no valid block header found; don't complain
                break;
            }
            try {
                // read block
                uint64_t nBlockPos = blkdat.GetPos();
                blkdat.SetLimit(nBlockPos + nSize);
                Bitcoin_CBlock block;
                blkdat >> block;
                nRewind = blkdat.GetPos();

                // process block
                if (nBlockPos >= nStartByte) {
                    LOCK(bitcoin_mainState.cs_main);
                    if (dbp)
                        dbp->nPos = nBlockPos;
                    CValidationState state;
                    if (Bitcoin_ProcessBlock(state, NULL, &block, dbp))
                        nLoaded++;
                    if (state.IsError())
                        break;
                }
            } catch (std::exception &e) {
                LogPrintf("Bitcoin: %s : Deserialize or I/O error - %s", __func__, e.what());
            }
        }
        fclose(fileIn);
    } catch(std::runtime_error &e) {
        AbortNode(_("Error: system error: ") + e.what());
    }
    if (nLoaded > 0)
        LogPrintf("Bitcoin: Loaded %i blocks from external file in %dms\n", nLoaded, GetTimeMillis() - nStart);
    return nLoaded > 0;
}










//////////////////////////////////////////////////////////////////////////////
//
// CAlert
//

string Bitcoin_GetWarnings(string strFor)
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

    if (bitcoin_fLargeWorkForkFound)
    {
        nPriority = 2000;
        strStatusBar = strRPC = _("Warning: The network does not appear to fully agree! Some miners appear to be experiencing issues.");
    }
    else if (bitcoin_fLargeWorkInvalidChainFound)
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


bool static Bitcoin_AlreadyHave(const CInv& inv)
{
    switch (inv.type)
    {
    case MSG_TX:
        {
            bool txInMap = false;
            txInMap = bitcoin_mempool.exists(inv.hash);
            return txInMap || bitcoin_mapOrphanTransactions.count(inv.hash) ||
                bitcoin_pcoinsTip->HaveCoins(inv.hash);
        }
    case MSG_BLOCK:
        return bitcoin_mapBlockIndex.count(inv.hash) ||
               bitcoin_mapOrphanBlocks.count(inv.hash);
    }
    // Don't know what it is, just say we already got one
    return true;
}


void static Bitcoin_ProcessGetData(CNode* pfrom)
{
    std::deque<CInv>::iterator it = pfrom->vRecvGetData.begin();

    vector<CInv> vNotFound;

    LOCK(bitcoin_mainState.cs_main);

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
                map<uint256, Bitcoin_CBlockIndex*>::iterator mi = bitcoin_mapBlockIndex.find(inv.hash);
                if (mi != bitcoin_mapBlockIndex.end())
                {
                    // If the requested block is at a height below our last
                    // checkpoint, only serve it if it's in the checkpointed chain
                    int nHeight = mi->second->nHeight;
                    Bitcoin_CBlockIndex* pcheckpoint = Checkpoints::Bitcoin_GetLastCheckpoint(bitcoin_mapBlockIndex);
                    if (pcheckpoint && nHeight < pcheckpoint->nHeight) {
                        if (!bitcoin_chainActive.Contains(mi->second))
                        {
                            LogPrintf("Bitcoin: ProcessGetData(): ignoring request for old block that isn't in the main chain\n");
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
                    Bitcoin_CBlock block;
                    bool blockFound = true;
                    if(bitcoin_fTrimBlockFiles) {
                    	blockFound = Bitcoin_ReadBlockFromDisk(block, (*mi).second);
                    } else {
						assert(Bitcoin_ReadBlockFromDisk(block, (*mi).second));
                    }
                    if(blockFound) {
						if (inv.type == MSG_BLOCK)
							pfrom->PushMessage("block", block);
						else // MSG_FILTERED_BLOCK)
						{
							LOCK(pfrom->cs_filter);
							if (pfrom->pfilter)
							{
								Bitcoin_CMerkleBlock merkleBlock(block, *pfrom->pfilter);
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
                    }

                    // Trigger them to send a getblocks request for the next batch of inventory
                    if (inv.hash == pfrom->hashContinue)
                    {
                        // Bypass PushInventory, this must send even if redundant,
                        // and we want it right after the last block so they don't
                        // wait for other stuff first.
                        vector<CInv> vInv;
                        vInv.push_back(CInv(MSG_BLOCK, bitcoin_chainActive.Tip()->GetBlockHash()));
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
                	CNetParams * netParams = pfrom->netParams;
                    LOCK(netParams->cs_mapRelay);
                    map<CInv, CDataStream>::iterator mi = netParams->mapRelay.find(inv);
                    if (mi != netParams->mapRelay.end()) {
                        pfrom->PushMessage(inv.GetCommand(), (*mi).second);
                        pushed = true;
                    }
                }
                if (!pushed && inv.type == MSG_TX) {
                    Bitcoin_CTransaction tx;
                    if (bitcoin_mempool.lookup(inv.hash, tx)) {
                        CDataStream ss(SER_NETWORK, BITCOIN_PROTOCOL_VERSION);
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
            bitcoin_g_signals.Inventory(inv.hash);

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

bool static Bitcoin_ProcessMessage(CNode* pfrom, string strCommand, CDataStream& vRecv)
{
	CNetParams * netParams = Bitcoin_NetParams();

    RandAddSeedPerfmon();
    LogPrint(netParams->DebugCategory(), "Bitcoin: received: %s (%u bytes)\n", strCommand, vRecv.size());
    if (mapArgs.count("-dropmessagestest") && GetRand(atoi(mapArgs["-dropmessagestest"])) == 0)
    {
        LogPrintf("Bitcoin: dropmessagestest DROPPING RECV MESSAGE\n");
        return true;
    }

    {
        LOCK(bitcoin_mainState.cs_main);
        Bitcoin_State(pfrom->GetId())->nLastBlockProcess = GetTimeMicros();
    }



    if (strCommand == "version")
    {
        // Each connection can only send one version message
        if (pfrom->nVersion != 0)
        {
            pfrom->PushMessage("reject", strCommand, BITCOIN_REJECT_DUPLICATE, string("Duplicate version message"));
            Bitcoin_Misbehaving(pfrom->GetId(), 1);
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
            LogPrintf("Bitcoin: partner %s using obsolete version %i; disconnecting\n", pfrom->addr.ToString(), pfrom->nVersion);
            pfrom->PushMessage("reject", strCommand, BITCOIN_REJECT_OBSOLETE,
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
            SeenLocal(addrMe, netParams);
        }

        // Disconnect if we connected to ourself
        if (nNonce == netParams->nLocalHostNonce && nNonce > 1)
        {
            LogPrintf("Bitcoin: connected to self at %s, disconnecting\n", pfrom->addr.ToString());
            pfrom->fDisconnect = true;
            return true;
        }

        // Be shy and don't send version until we hear
        if (pfrom->fInbound)
            pfrom->PushVersion();

        pfrom->fClient = !(pfrom->nServices & NODE_NETWORK);


        // Change version
        pfrom->PushMessage("verack");
        pfrom->ssSend.SetVersion(min(pfrom->nVersion, BITCOIN_PROTOCOL_VERSION));

        if (!pfrom->fInbound)
        {
            // Advertise our address
            if (netParams->fListen && !Bitcoin_IsInitialBlockDownload())
            {
                CAddress addr = GetLocalAddress(&pfrom->addr, netParams);
                if (addr.IsRoutable())
                    pfrom->PushAddress(addr);
            }

            // Get recent addresses
            if (pfrom->fOneShot || pfrom->nVersion >= CADDR_TIME_VERSION || netParams->addrman.size() < 1000)
            {
                pfrom->PushMessage("getaddr");
                pfrom->fGetAddr = true;
            }
            netParams->addrman.Good(pfrom->addr);
        } else {
            if (((CNetAddr)pfrom->addr) == (CNetAddr)addrFrom)
            {
                netParams->addrman.Add(addrFrom, addrFrom);
                netParams->addrman.Good(addrFrom);
            }
        }

        // Relay alerts
        {
            LOCK(cs_mapAlerts);
            BOOST_FOREACH(PAIRTYPE(const uint256, CAlert)& item, mapAlerts)
                item.second.RelayTo(pfrom);
        }

        pfrom->fSuccessfullyConnected = true;

        LogPrintf("Bitcoin: receive version message: %s: version %d, blocks=%d, us=%s, them=%s, peer=%s\n", pfrom->cleanSubVer, pfrom->nVersion, pfrom->nStartingHeight, addrMe.ToString(), addrFrom.ToString(), pfrom->addr.ToString());

        AddTimeData(pfrom->addr, nTime);
    }


    else if (pfrom->nVersion == 0)
    {
        // Must have a version message before anything else
        Bitcoin_Misbehaving(pfrom->GetId(), 1);
        return false;
    }


    else if (strCommand == "verack")
    {
        pfrom->SetRecvVersion(min(pfrom->nVersion, BITCOIN_PROTOCOL_VERSION));
    }


    else if (strCommand == "addr")
    {
        vector<CAddress> vAddr;
        vRecv >> vAddr;

        // Don't want addr from older versions unless seeding
        if (pfrom->nVersion < CADDR_TIME_VERSION && netParams->addrman.size() > 1000)
            return true;
        if (vAddr.size() > 1000)
        {
            Bitcoin_Misbehaving(pfrom->GetId(), 20);
            return error("Bitcoin: message addr size() = %u", vAddr.size());
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
            bool fReachable = IsReachable(addr, netParams);
            if (addr.nTime > nSince && !pfrom->fGetAddr && vAddr.size() <= 10 && addr.IsRoutable())
            {
                // Relay to a limited number of other nodes
                {
                    LOCK(netParams->cs_vNodes);
                    // Use deterministic randomness to send to the same nodes for 24 hours
                    // at a time so the setAddrKnowns of the chosen nodes prevent repeats
                    static uint256 hashSalt;
                    if (hashSalt == 0)
                        hashSalt = GetRandHash();
                    uint64_t hashAddr = addr.GetHash();
                    uint256 hashRand = hashSalt ^ (hashAddr<<32) ^ ((GetTime()+hashAddr)/(24*60*60));
                    hashRand = Hash(BEGIN(hashRand), END(hashRand));
                    multimap<uint256, CNode*> mapMix;
                    BOOST_FOREACH(CNode* pnode, netParams->vNodes)
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
        netParams->addrman.Add(vAddrOk, pfrom->addr, 2 * 60 * 60);
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
            Bitcoin_Misbehaving(pfrom->GetId(), 20);
            return error("Bitcoin: message inv size() = %u", vInv.size());
        }

        LOCK(bitcoin_mainState.cs_main);

        for (unsigned int nInv = 0; nInv < vInv.size(); nInv++)
        {
            const CInv &inv = vInv[nInv];

            boost::this_thread::interruption_point();
            pfrom->AddInventoryKnown(inv);

            bool fAlreadyHave = Bitcoin_AlreadyHave(inv);
            LogPrint(netParams->DebugCategory(), "Bitcoin:   got inventory: %s  %s\n", inv.ToString(), fAlreadyHave ? "have" : "new");

            if (!fAlreadyHave) {
                if (!bitcoin_mainState.ImportingOrReindexing()) {
                    if (inv.type == MSG_BLOCK)
                        Bitcoin_AddBlockToQueue(pfrom->GetId(), inv.hash);
                    else
                        pfrom->AskFor(inv);
                }
            } else if (inv.type == MSG_BLOCK && bitcoin_mapOrphanBlocks.count(inv.hash)) {
                Bitcoin_PushGetBlocks(pfrom, (Bitcoin_CBlockIndex*)bitcoin_chainActive.Tip(), Bitcoin_GetOrphanRoot(inv.hash));
            }

            // Track requests for our stuff
            bitcoin_g_signals.Inventory(inv.hash);
        }
    }


    else if (strCommand == "getdata")
    {
        vector<CInv> vInv;
        vRecv >> vInv;
        if (vInv.size() > MAX_INV_SZ)
        {
            Bitcoin_Misbehaving(pfrom->GetId(), 20);
            return error("Bitcoin: message getdata size() = %u", vInv.size());
        }

        if (fDebug || (vInv.size() != 1))
            LogPrint(netParams->DebugCategory(), "Bitcoin: received getdata (%u invsz)\n", vInv.size());

        if ((fDebug && vInv.size() > 0) || (vInv.size() == 1))
            LogPrint(netParams->DebugCategory(), "Bitcoin: received getdata for: %s\n", vInv[0].ToString());

        pfrom->vRecvGetData.insert(pfrom->vRecvGetData.end(), vInv.begin(), vInv.end());
        Bitcoin_ProcessGetData(pfrom);
    }


    else if (strCommand == "getblocks")
    {
        CBlockLocator locator;
        uint256 hashStop;
        vRecv >> locator >> hashStop;

        LOCK(bitcoin_mainState.cs_main);

        // Find the last block the caller has in the main chain
        Bitcoin_CBlockIndex* pindex = bitcoin_chainActive.FindFork(locator);

        // Send the rest of the chain
        if (pindex)
            pindex = bitcoin_chainActive.Next(pindex);
        int nLimit = 500;
        LogPrint(netParams->DebugCategory(), "Bitcoin: getblocks %d to %s limit %d\n", (pindex ? pindex->nHeight : -1), hashStop.ToString(), nLimit);
        for (; pindex; pindex = bitcoin_chainActive.Next(pindex))
        {
            if (pindex->GetBlockHash() == hashStop)
            {
                LogPrint(netParams->DebugCategory(), "Bitcoin:   getblocks stopping at %d %s\n", pindex->nHeight, pindex->GetBlockHash().ToString());
                break;
            }
            pfrom->PushInventory(CInv(MSG_BLOCK, pindex->GetBlockHash()));
            if (--nLimit <= 0)
            {
                // When this block is requested, we'll send an inv that'll make them
                // getblocks the next batch of inventory.
                LogPrint(netParams->DebugCategory(), "Bitcoin:   getblocks stopping at limit %d %s\n", pindex->nHeight, pindex->GetBlockHash().ToString());
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

        LOCK(bitcoin_mainState.cs_main);

        Bitcoin_CBlockIndex* pindex = NULL;
        if (locator.IsNull())
        {
            // If locator is null, return the hashStop block
            map<uint256, Bitcoin_CBlockIndex*>::iterator mi = bitcoin_mapBlockIndex.find(hashStop);
            if (mi == bitcoin_mapBlockIndex.end())
                return true;
            pindex = (*mi).second;
        }
        else
        {
            // Find the last block the caller has in the main chain
            pindex = bitcoin_chainActive.FindFork(locator);
            if (pindex)
                pindex = bitcoin_chainActive.Next(pindex);
        }

        // we must use CBlocks, as CBlockHeaders won't include the 0x00 nTx count at the end
        vector<Bitcoin_CBlock> vHeaders;
        int nLimit = 2000;
        LogPrint(netParams->DebugCategory(), "Bitcoin: getheaders %d to %s\n", (pindex ? pindex->nHeight : -1), hashStop.ToString());
        for (; pindex; pindex = bitcoin_chainActive.Next(pindex))
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
        Bitcoin_CTransaction tx;
        vRecv >> tx;

        CInv inv(MSG_TX, tx.GetHash());
        pfrom->AddInventoryKnown(inv);

        LOCK(bitcoin_mainState.cs_main);

        bool fMissingInputs = false;
        CValidationState state;
        if (Bitcoin_AcceptToMemoryPool(bitcoin_mempool, state, tx, true, &fMissingInputs))
        {
            bitcoin_mempool.check(bitcoin_pcoinsTip);
            Bitcoin_RelayTransaction(tx, inv.hash, netParams);
            netParams->mapAlreadyAskedFor.erase(inv);
            vWorkQueue.push_back(inv.hash);
            vEraseQueue.push_back(inv.hash);


            LogPrint("mempool", "AcceptToMemoryPool: %s %s : accepted %s (poolsz %u)\n",
                pfrom->addr.ToString(), pfrom->cleanSubVer,
                tx.GetHash().ToString(),
                bitcoin_mempool.mapTx.size());

            // Recursively process any orphan transactions that depended on this one
            for (unsigned int i = 0; i < vWorkQueue.size(); i++)
            {
                uint256 hashPrev = vWorkQueue[i];
                for (set<uint256>::iterator mi = bitcoin_mapOrphanTransactionsByPrev[hashPrev].begin();
                     mi != bitcoin_mapOrphanTransactionsByPrev[hashPrev].end();
                     ++mi)
                {
                    const uint256& orphanHash = *mi;
                    const Bitcoin_CTransaction& orphanTx = bitcoin_mapOrphanTransactions[orphanHash];
                    bool fMissingInputs2 = false;
                    // Use a dummy CValidationState so someone can't setup nodes to counter-DoS based on orphan
                    // resolution (that is, feeding people an invalid transaction based on LegitTxX in order to get
                    // anyone relaying LegitTxX banned)
                    CValidationState stateDummy;

                    if (Bitcoin_AcceptToMemoryPool(bitcoin_mempool, stateDummy, orphanTx, true, &fMissingInputs2))
                    {
                        LogPrint("mempool", "   accepted orphan tx %s\n", orphanHash.ToString());
                        Bitcoin_RelayTransaction(orphanTx, orphanHash, netParams);
                        netParams->mapAlreadyAskedFor.erase(CInv(MSG_TX, orphanHash));
                        vWorkQueue.push_back(orphanHash);
                        vEraseQueue.push_back(orphanHash);
                    }
                    else if (!fMissingInputs2)
                    {
                        // invalid or too-little-fee orphan
                        vEraseQueue.push_back(orphanHash);
                        LogPrint("mempool", "   removed orphan tx %s\n", orphanHash.ToString());
                    }
                    bitcoin_mempool.check(bitcoin_pcoinsTip);
                }
            }

            BOOST_FOREACH(uint256 hash, vEraseQueue)
                Bitcoin_EraseOrphanTx(hash);
        }
        else if (fMissingInputs)
        {
            Bitcoin_AddOrphanTx(tx);

            // DoS prevention: do not allow mapOrphanTransactions to grow unbounded
            unsigned int nEvicted = Bitcoin_LimitOrphanTxSize(BITCOIN_MAX_ORPHAN_TRANSACTIONS);
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
                Bitcoin_Misbehaving(pfrom->GetId(), nDoS);
        }
    }

    else if (strCommand == "block" && !bitcoin_mainState.ImportingOrReindexing()) // Ignore blocks received while importing
    {
        Bitcoin_CBlock block;
        vRecv >> block;

        LogPrint(netParams->DebugCategory(), "Bitcoin: received block %s\n", block.GetHash().ToString());
        // block.print();

        CInv inv(MSG_BLOCK, block.GetHash());
        pfrom->AddInventoryKnown(inv);

        LOCK(bitcoin_mainState.cs_main);
        // Remember who we got this block from.
        bitcoin_mapBlockSource[inv.hash] = pfrom->GetId();
        Bitcoin_MarkBlockAsReceived(inv.hash, pfrom->GetId());

        CValidationState state;
        Bitcoin_ProcessBlock(state, pfrom, &block);
    }


    else if (strCommand == "getaddr")
    {
        pfrom->vAddrToSend.clear();
        vector<CAddress> vAddr = netParams->addrman.GetAddr();
        BOOST_FOREACH(const CAddress &addr, vAddr)
            pfrom->PushAddress(addr);
    }


    else if (strCommand == "mempool")
    {
        LOCK2(bitcoin_mainState.cs_main, pfrom->cs_filter);

        std::vector<uint256> vtxid;
        bitcoin_mempool.queryHashes(vtxid);
        vector<CInv> vInv;
        BOOST_FOREACH(uint256& hash, vtxid) {
            CInv inv(MSG_TX, hash);
            Bitcoin_CTransaction tx;
            bool fInMemPool = bitcoin_mempool.lookup(hash, tx);
            if (!fInMemPool) continue; // another thread removed since queryHashes, maybe...

            if ((pfrom->pfilter && pfrom->pfilter->bitcoin_IsRelevantAndUpdate(tx, hash)) ||
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
            LogPrint(netParams->DebugCategory(), "Bitcoin: pong %s %s: %s, %x expected, %x received, %u bytes\n",
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
                    LOCK(netParams->cs_vNodes);
                    BOOST_FOREACH(CNode* pnode, netParams->vNodes)
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
                Bitcoin_Misbehaving(pfrom->GetId(), 10);
            }
        }
    }


    else if (strCommand == "filterload")
    {
        CBloomFilter filter;
        vRecv >> filter;

        if (!filter.IsWithinSizeConstraints())
            // There is no excuse for sending a too-large filter
            Bitcoin_Misbehaving(pfrom->GetId(), 100);
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
            Bitcoin_Misbehaving(pfrom->GetId(), 100);
        } else {
            LOCK(pfrom->cs_filter);
            if (pfrom->pfilter)
                pfrom->pfilter->insert(vData);
            else
                Bitcoin_Misbehaving(pfrom->GetId(), 100);
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
            LogPrint(netParams->DebugCategory(), "Bitcoin: Reject %s\n", SanitizeString(s));
        }
    }

    else
    {
        // Ignore unknown commands for extensibility
    }


    // Update the last seen time for this node's address
    if (pfrom->fNetworkNode)
        if (strCommand == "version" || strCommand == "addr" || strCommand == "inv" || strCommand == "getdata" || strCommand == "ping")
            netParams->addrman.AddressCurrentlyConnected(pfrom->addr);


    return true;
}

// requires LOCK(cs_vRecvMsg)
bool Bitcoin_ProcessMessages(CNode* pfrom)
{
	CNetParams * netParams = pfrom->netParams;

    //if (fDebug)
    //    LogPrintf("Bitcoin: ProcessMessages(%u messages)\n", pfrom->vRecvMsg.size());

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
        Bitcoin_ProcessGetData(pfrom);

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
        //    LogPrintf("Bitcoin: ProcessMessages(message %u msgsz, %u bytes, complete:%s)\n",
        //            msg.hdr.nMessageSize, msg.vRecv.size(),
        //            msg.complete() ? "Y" : "N");

        // end, if an incomplete message is found
        if (!msg.complete())
            break;

        // at this point, any failure means we can delete the current message
        it++;

        // Scan for message start
        if (memcmp(msg.hdr.pchMessageStart, netParams->MessageStart(), MESSAGE_START_SIZE) != 0) {
            LogPrintf("\n\nBITCOIN: PROCESSMESSAGE: INVALID MESSAGESTART\n\n");
            fOk = false;
            break;
        }

        // Read header
        CMessageHeader& hdr = msg.hdr;
        if (!hdr.IsValid())
        {
            LogPrintf("\n\nBITCOIN: PROCESSMESSAGE: ERRORS IN HEADER %s\n\n\n", hdr.GetCommand());
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
            LogPrintf("Bitcoin: ProcessMessages(%s, %u bytes) : CHECKSUM ERROR nChecksum=%08x hdr.nChecksum=%08x\n",
               strCommand, nMessageSize, nChecksum, hdr.nChecksum);
            continue;
        }

        // Process message
        bool fRet = false;
        try
        {
            fRet = Bitcoin_ProcessMessage(pfrom, strCommand, vRecv);
            boost::this_thread::interruption_point();
        }
        catch (std::ios_base::failure& e)
        {
            pfrom->PushMessage("reject", strCommand, BITCOIN_REJECT_MALFORMED, string("error parsing message"));
            if (strstr(e.what(), "end of data"))
            {
                // Allow exceptions from under-length message on vRecv
                LogPrintf("Bitcoin: ProcessMessages(%s, %u bytes) : Exception '%s' caught, normally caused by a message being shorter than its stated length\n", strCommand, nMessageSize, e.what());
            }
            else if (strstr(e.what(), "size too large"))
            {
                // Allow exceptions from over-long size
                LogPrintf("Bitcoin: ProcessMessages(%s, %u bytes) : Exception '%s' caught\n", strCommand, nMessageSize, e.what());
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
            LogPrintf("Bitcoin: ProcessMessage(%s, %u bytes) FAILED\n", strCommand, nMessageSize);

        break;
    }

    // In case the connection got shut down, its receive buffer was wiped
    if (!pfrom->fDisconnect)
        pfrom->vRecvMsg.erase(pfrom->vRecvMsg.begin(), it);

    return fOk;
}


bool Bitcoin_SendMessages(CNode* pto, bool fSendTrickle)
{
	CNetParams * netParams = pto->netParams;

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

        TRY_LOCK(bitcoin_mainState.cs_main, lockMain); // Acquire cs_main for IsInitialBlockDownload() and CNodeState()
        if (!lockMain)
            return true;

        // Address refresh broadcast
        static int64_t nLastRebroadcast;
        if (!Bitcoin_IsInitialBlockDownload() && (GetTime() - nLastRebroadcast > 24 * 60 * 60))
        {
            {
                LOCK(netParams->cs_vNodes);
                BOOST_FOREACH(CNode* pnode, netParams->vNodes)
                {
                    // Periodically clear setAddrKnown to allow refresh broadcasts
                    if (nLastRebroadcast)
                        pnode->setAddrKnown.clear();

                    // Rebroadcast our address
                    if (netParams->fListen)
                    {
                        CAddress addr = GetLocalAddress(&pnode->addr, netParams);
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

        CNodeState &state = *Bitcoin_State(pto->GetId());
        if (state.fShouldBan) {
            if (pto->addr.IsLocal())
                LogPrintf("Bitcoin: Warning: not banning local node %s!\n", pto->addr.ToString());
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
        if (pto->fStartSync && !bitcoin_mainState.ImportingOrReindexing()) {
            pto->fStartSync = false;
            Bitcoin_PushGetBlocks(pto, (Bitcoin_CBlockIndex*)bitcoin_chainActive.Tip(), uint256(0));
        }

        // Resend wallet transactions that haven't gotten in a block yet
        // Except during reindex, importing and IBD, when old wallet
        // transactions become unconfirmed and spams other nodes.
        if (!bitcoin_mainState.ImportingOrReindexing() && !Bitcoin_IsInitialBlockDownload())
        {
            bitcoin_g_signals.Broadcast();
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


        // Detect stalled peers. Require that blocks are in flight, we haven't
        // received a (requested) block in one minute, and that all blocks are
        // in flight for over two minutes, since we first had a chance to
        // process an incoming block.
        int64_t nNow = GetTimeMicros();
        if (!pto->fDisconnect && state.nBlocksInFlight &&
            state.nLastBlockReceive < state.nLastBlockProcess - BITCOIN_BLOCK_DOWNLOAD_TIMEOUT*1000000 &&
            state.vBlocksInFlight.front().nTime < state.nLastBlockProcess - 2*BITCOIN_BLOCK_DOWNLOAD_TIMEOUT*1000000) {
            LogPrintf("Bitcoin: Peer %s is stalling block download, disconnecting\n", state.name.c_str());
            pto->fDisconnect = true;
        }

        //
        // Message: getdata (blocks)
        //
        vector<CInv> vGetData;
        while (!pto->fDisconnect && state.nBlocksToDownload && state.nBlocksInFlight < BITCOIN_MAX_BLOCKS_IN_TRANSIT_PER_PEER) {
            uint256 hash = state.vBlocksToDownload.front();
            vGetData.push_back(CInv(MSG_BLOCK, hash));
            Bitcoin_MarkBlockAsInFlight(pto->GetId(), hash);
            LogPrint(netParams->DebugCategory(), "Bitcoin: Requesting block %s from %s\n", hash.ToString(), state.name);
            if (vGetData.size() >= 1000)
            {
                pto->PushMessage("getdata", vGetData);
                vGetData.clear();
            }
        }

        //
        // Message: getdata (non-blocks)
        //
        while (!pto->fDisconnect && !pto->mapAskFor.empty() && (*pto->mapAskFor.begin()).first <= nNow)
        {
            const CInv& inv = (*pto->mapAskFor.begin()).second;
            if (!Bitcoin_AlreadyHave(inv))
            {
                if (fDebug)
                    LogPrint(netParams->DebugCategory(), "Bitcoin: sending getdata: %s\n", inv.ToString());
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






class Bitcoin_CMainCleanup
{
public:
    Bitcoin_CMainCleanup() {}
    ~Bitcoin_CMainCleanup() {
        // block headers
        std::map<uint256, Bitcoin_CBlockIndex*>::iterator it1 = bitcoin_mapBlockIndex.begin();
        for (; it1 != bitcoin_mapBlockIndex.end(); it1++)
            delete (*it1).second;
        bitcoin_mapBlockIndex.clear();

        // orphan blocks
        std::map<uint256, COrphanBlock*>::iterator it2 = bitcoin_mapOrphanBlocks.begin();
        for (; it2 != bitcoin_mapOrphanBlocks.end(); it2++)
            delete (*it2).second;
        bitcoin_mapOrphanBlocks.clear();

        // orphan transactions
        bitcoin_mapOrphanTransactions.clear();
    }
} bitcoin_instance_of_cmaincleanup;
