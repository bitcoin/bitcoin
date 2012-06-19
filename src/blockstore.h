#ifndef BITCOIN_BLOCKSTORE_H
#define BITCOIN_BLOCKSTORE_H

// This API is considered stable ONLY for existing bitcoin codebases,
// any futher uses are not yet supported.
// This API is subject to change dramatically overnight, do not
// depend on it for anything.

#include <boost/signals2/signal.hpp>
#include <queue>

#include "sync.h"
#include "uint256.h"

class CBlock;
class CTxDB;
class CBlockIndex;
class CHub;
class CNode;

class CBlockStoreSignalTable
{
public:
    CCriticalSection cs_sigCommitBlock;
    boost::signals2::signal<void (const CBlock&)> sigCommitBlock;

    CCriticalSection cs_sigAskForBlocks;
    boost::signals2::signal<void (const uint256, const uint256)> sigAskForBlocks;

    CCriticalSection cs_sigDoS;
    boost::function<void (CNode* pNode, const int nDoS)> sigDoS;
};

class CBlockStore
{
private:
    CBlockStoreSignalTable sigtable;

    void CallbackCommitBlock(const CBlock &block) { LOCK(sigtable.cs_sigCommitBlock); sigtable.sigCommitBlock(block); }

    void CallbackAskForBlocks(const uint256 hashEnd, const uint256 hashOriginator)  { LOCK(sigtable.cs_sigAskForBlocks); sigtable.sigAskForBlocks(hashEnd, hashOriginator); }

    void CallbackDoS(CNode* pNode, const int nDoS) { LOCK(sigtable.cs_sigDoS); sigtable.sigDoS(pNode, nDoS); }

    CCriticalSection cs_callbacks;
    CSemaphore sem_callbacks;
    bool fProcessCallbacks;
    bool fProcessingCallbacks;

    std::queue<std::pair<CBlock*, CNode*> > queueFinishEmitBlockCallbacks;
    void SubmitCallbackFinishEmitBlock(CBlock& block, CNode* pNodeDoS);
    bool FinishEmitBlock(CBlock& block, CNode* pNodeDoS);

    bool Reorganize(CTxDB& txdb, CBlockIndex* pindexNew);
    bool DisconnectBlock(CBlock& block, CTxDB& txdb, CBlockIndex* pindex);
    bool ConnectBlock(CBlock& block, CTxDB& txdb, CBlockIndex* pindex);
    bool SetBestChainInner(CBlock& block, uint256& hash, CTxDB& txdb, CBlockIndex *pindexNew);
    bool SetBestChain(CBlock& block, uint256& hash, CTxDB& txdb, CBlockIndex* pindexNew);
    bool AddToBlockIndex(CBlock& block, uint256& hash, unsigned int nFile, unsigned int nBlockPos);
    bool AcceptBlock(CBlock& block, uint256& hash);
public:
    // Loops to process callbacks (do not call manually, automatically started in the constructor)
        void ProcessCallbacks();
    // Stop callback processing threads
    void StopProcessCallbacks();

    CBlockStore();
    ~CBlockStore()  { StopProcessCallbacks(); }

    bool LoadBlockIndex(bool fReadOnly=false);

//Register methods
    // Register a handler (of the form void f(const CBlock& block)) to be called after every block commit
    void RegisterCommitBlock(boost::function<void (const CBlock&)> func) { LOCK(sigtable.cs_sigCommitBlock); sigtable.sigCommitBlock.connect(func); }

    // Register a handler (of the form void f(const uint256 hashEnd, const uint256 hashOriginator)) to be called when we need to ask for blocks up to hashEnd
    //   Should always start from the best block (GetBestBlockIndex())
    //   The receiver should check if it has a peer which is known to have a block with hash hashOriginator and if it does, it should
    //    send the block query to that node.
    void RegisterAskForBlocks(boost::function<void (const uint256, const uint256)> func) { LOCK(sigtable.cs_sigAskForBlocks); sigtable.sigAskForBlocks.connect(func); }

    // Register a handler (of the form void f(CNode* pNode, const int nDoS)) that calls pNode->Misbehaving(nDoS)
    void RegisterDoSHandler(boost::function<void (CNode* pNode, const int nDoS)> func) { LOCK(sigtable.cs_sigDoS); sigtable.sigDoS = func; }

//Blockchain access methods
    // Emit methods will verify the object, commit it to memory/disk and then place it in queue to
    //   be handled by listeners

    // if (!fBlocking) only initial checks will be performed before returning
    //   This means block.nDoS may not be set to its final value before returning
    // DoSHandler will be called with the final value of block.nDoS at some point during callbacks.
    bool EmitBlock(CBlock& block, bool fBlocking=true, CNode* pNodeDoS=NULL);
};

#endif
