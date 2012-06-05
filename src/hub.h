#ifndef BITCOIN_HUB_H
#define BITCOIN_HUB_H

// This API is considered stable ONLY for existing bitcoin codebases,
// any futher uses are not yet supported.
// This API is subject to change dramatically overnight, do not
// depend on it for anything.

#include <boost/signals2/signal.hpp>
#include <queue>

#include "uint256.h"
#include "sync.h"
#include "blockstore.h"

class CBlock;
class CMerkleTx;
class CTransaction;
class CAlert;
class CInv;
class CNode;

class CHubSignalTable
{
public:
    CCriticalSection cs_sigCommitBlock;
    boost::signals2::signal<void (const CBlock&)> sigCommitBlock;

    CCriticalSection cs_sigCommitTransactionToMemoryPool;
    boost::signals2::signal<void (const CTransaction&)> sigCommitTransactionToMemoryPool;

    CCriticalSection cs_sigCommitAlert;
    boost::signals2::signal<void (const CAlert&)> sigCommitAlert;
    CCriticalSection cs_sigRemoveAlert;
    boost::signals2::signal<void (const CAlert&)> sigRemoveAlert;

    CCriticalSection cs_sigAskForBlocks;
    boost::signals2::signal<void (const uint256, const uint256)> sigAskForBlocks;
};

class CHubCallback
{
public:
    virtual ~CHubCallback() {};
    virtual void Signal(CHubSignalTable& sigtable) =0;
};

class CHub
{
private:
    CHubSignalTable sigtable;

    CCriticalSection cs_callbacks;
    std::queue<CHubCallback*> queueCallbacks;
    CSemaphore sem_callbacks;

    bool fProcessCallbacks;
    int nCallbackThreads;

    CBlockStore* pblockstore;

    void SubmitCallbackCommitBlock(const CBlock &block);

    bool EmitTransactionInner(CTransaction& tx, bool fCheckInputs);
    void SubmitCallbackCommitTransactionToMemoryPool(const CTransaction &tx);

    void SubmitCallbackCommitAlert(const CAlert &alert);
    void SubmitCallbackRemoveAlert(const CAlert &alert);

    void SubmitCallbackDoS(CNode* pNode, const int nDoS);
public:
//Util methods
    // Loops to process callbacks (do not call manually, automatically started in the constructor)
        void ProcessCallbacks();
    // Stop callback processing threads 
    void StopProcessCallbacks();

    CHub();
    ~CHub()  { StopProcessCallbacks(); }

    bool ConnectToBlockStore(CBlockStore* pblockstoreIn);

//Register methods
    // Register a handler (of the form void f(const CBlock& block)) to be called after every block commit
    void RegisterCommitBlock(boost::function<void (const CBlock&)> func) { LOCK(sigtable.cs_sigCommitBlock); sigtable.sigCommitBlock.connect(func); }

    // Register a handler (of the form void f(const CTransaction& tx)) to be called after every transaction commit to memory pool
    void RegisterCommitTransactionToMemoryPool(boost::function<void (const CTransaction&)> func) { LOCK(sigtable.cs_sigCommitTransactionToMemoryPool); sigtable.sigCommitTransactionToMemoryPool.connect(func); }

    // Register a handler (of the form void f(const CAlert& alert)) to be called after every alert commit
    void RegisterCommitAlert(boost::function<void (const CAlert&)> func) { LOCK(sigtable.cs_sigCommitAlert); sigtable.sigCommitAlert.connect(func); }
    // Register a handler (of the form void f(const CAlert& alert)) to be called after every alert cancel or expire
    void RegisterRemoveAlert(boost::function<void (const CAlert&)> func) { LOCK(sigtable.cs_sigRemoveAlert); sigtable.sigRemoveAlert.connect(func); }

    // Register a handler (of the form void f(const uint256 hashEnd, const uint256 hashOriginator)) to be called when we need to ask for blocks up to hashEnd
    //   Should always start from the best block (GetBestBlockIndex())
    //   The receiver should check if it has a peer which is known to have a block with hash hashOriginator and if it does, it should
    //    send the block query to that node.
    void RegisterAskForBlocks(boost::function<void (const uint256, const uint256)> func) { LOCK(sigtable.cs_sigAskForBlocks); sigtable.sigAskForBlocks.connect(func); }

//Blockchain access methods
    // Emit methods will verify the object, commit it to memory/disk and then place it in queue to
    //   be handled by listeners

    // if (!fBlocking) only initial checks will be performed before returning
    //   This means block.nDoS may not be set to its final value before returning
    // pNodeDoS->Misbehaving() will be called with the final value of block.nDoS at some point during callbacks.
    bool EmitBlock(CBlock& block, bool fBlocking=true, CNode* pNodeDoS=NULL) { if (!pblockstore) return false; return pblockstore->EmitBlock(block, fBlocking, pNodeDoS); }

    bool EmitAlert(CAlert& alert);

    // Emitting transactions already in a block is acceptable only if it is a supporting
    //   transaction for one of our own
    // fCheckInputs is ignored (and set to true) if !IsInitialBlockDownload() && !fClient
    // Only set fCheckInputs when tx is a supporting transaction for one of our own
    bool EmitTransaction(CMerkleTx& tx, bool fCheckInputs=true);
    bool EmitTransaction(CTransaction& tx, bool fCheckInputs=true);

    // Returns true if we haven't seen a given inv and want it
    bool NeedInv(const CInv& inv);

    // Return CBlockIndex* with *phashBlock == hash or NULL if we dont have one
    // if (fBlocking) wait for the block to be committed (assuming it has already been emitted)
    //   This can still return NULL even if the block has been emitted if the block is invalid
    // WARNING: DO NOT call this with fBlocking == true if you are holding cs_main
    const CBlockIndex* GetBlockIndex(const uint256& hash, bool fBlocking=false) { if (!pblockstore) return NULL; return pblockstore->GetBlockIndex(hash, fBlocking); }

//Connected wallet/etc access methods

    // Ask that any listeners who have access to ask other nodes for blocks
    // (ie net) ask for all blocks between GetBestBlockIndex() and hashEnd
    // If hashOriginator is specified, then a node which is known to have a block
    //   with that hash will be the one to get the block request, unless no connected
    //   nodes are known to have this block, in which case a random one will be queried.
    void AskForBlocks(const uint256 hashEnd, const uint256 hashOriginator);
};

// A simple generic CHub Listening class which can be extended, if you wish
class CHubListener
{
public:
    void RegisterWithHub(CHub* phub);
    void DeregisterFromHub();

    CHubListener() {}
    CHubListener(CHub* phub) { RegisterWithHub(phub); }
    ~CHubListener() { DeregisterFromHub(); }

protected:
    virtual void HandleCommitBlock(const CBlock& block) {}

    virtual void HandleCommitTransactionToMemoryPool(const CTransaction& tx) {}

    virtual void HandleCommitAlert(const CAlert& alert) {}
    virtual void HandleRemoveAlert(const CAlert& alert) {}

    virtual void HandleAskForBlocks(const uint256, const uint256) {}
};

extern CHub* phub;

#endif
