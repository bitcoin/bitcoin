#ifndef BITCOIN_BLOCKSTORE_H
#define BITCOIN_BLOCKSTORE_H

#include <boost/signals2/signal.hpp>
#include <queue>

#include "util.h"


class CInv;
class CBlock;
class CBlockIndex;
class CBlockLocator;
class CTransaction;
class CAlert;
class CNode;

// Combiners for boost::signal2
template<typename T>
struct anyTrue
{
    typedef T result_type;

    template<typename InputIterator>
    T operator()(InputIterator first, InputIterator last) const
    {
        // If there are no slots to call, just return the
        // default-constructed value
        if (first == last) return T();
        T anyTrue = *first++;
        while (first != last)
        {
            if (*first)
            {
                anyTrue = *first;
                return anyTrue;
            }
            ++first;
        }
        return anyTrue;
    }
};

// This API is considered stable ONLY for existing bitcoin codebases,
// any futher uses are not yet supported.
// This API is subject to change dramatically overnight, do not
// depend on it for anything.

class CBlockStore
{
private:
    CCriticalSection cs_signals;

    boost::signals2::signal<void (const CBlock&)> sigCommitBlock;

    boost::signals2::signal<void (const CTransaction&)> sigCommitTransactionToMemoryPool;
    boost::signals2::signal<void (const uint256)> sigTransactionReplaced;

    boost::signals2::signal<void (const CAlert&)> sigCommitAlert;

    boost::signals2::signal<void (const uint256, const uint256)> sigAskForBlocks;
    boost::signals2::signal<void (const uint256)> sigRelayed;
    boost::signals2::signal<bool (const CTransaction&), anyTrue<bool> > sigIsTransactionFromMe;
    boost::signals2::signal<bool (const uint256), anyTrue<bool> > sigIsTransactionFromMeByHash;

    CCriticalSection cs_callbacks;
    std::queue<CBlock*> queueCommitBlockCallbacks;
    std::queue<std::pair<uint256, uint256> > queueAskForBlocksCallbacks;
    std::queue<uint256> queueRelayedCallbacks;
    std::queue<CTransaction*> queueCommitTransactionToMemoryPoolCallbacks;
    std::queue<uint256> queueTransactionReplacedCallbacks;
public:
//Util methods
    // Loops to process callbacks (call ProcessCallbacks(void* parg) with a CBlockStore as parg to launch in a thread)
    void ProcessCallbacks();

//Register methods
    // Register a handler (of the form void f(const CBlock& block)) to be called after every block commit
    void RegisterCommitBlock(boost::function<void (const CBlock&)> func) { sigCommitBlock.connect(func); }

    // Register a handler (of the form void f(const CTransaction& block)) to be called after every transaction commit to memory pool
    void RegisterCommitTransactionToMemoryPool(boost::function<void (const CTransaction&)> func) { sigCommitTransactionToMemoryPool.connect(func); }

    // Register a handler (of the form void f(const uint256)) to be called when a transaciton is replaced
    void RegisterTransactionReplaced(boost::function<void (const uint256)> func) {sigTransactionReplaced.connect(func); }

    //void RegisterCommitAlert(boost::function<void (const CAlert*)> func) { sigCommitAlert.connect(func); }

    // Register a handler (of the form void f(const uint256 hashEnd, const uint256 hashOriginator)) to be called when we need to ask for blocks up to hashEnd
    //   Should always start from the best block (GetBestBlockIndex())
    //   The receiver should check if it has a peer which is known to have a block with hash hashOriginator and if it does, it should
    //    send the block query to that node.
    void RegisterAskForBlocks(boost::function<void (const uint256, const uint256)> func) { sigAskForBlocks.connect(func); }

    // Register a handler (of the form void f(const uint256 hash)) to be called when a node announces or requests an Inv with hash hash
    //   Ideal for wallets which want to keep track of whether their transactions are being relayed to other nodes
    void RegisterRelayedNotification(boost::function<void (const uint256)> func) { sigRelayed.connect(func); }

    // Register a handler (of the form bool f(const CTransaction& tx)) which will return true if a given transaction is from a locally attached user
    //   This is used to determine how transactions should be handled in the free transaction and transaction relay logic 
    void RegisterIsTransactionFromMe(boost::function<bool (const CTransaction&)> func) { sigIsTransactionFromMe.connect(func); }

    // Register a handler (of the form bool f(const uint256)) which will return true if a transaction with the given hash is from a locally attached user
    //   This is used to determine how transactions should be handled in the free transaction and transaction relay logic 
    void RegisterIsTransactionFromMeByHash(boost::function<bool (const uint256)> func) { sigIsTransactionFromMeByHash.connect(func); }

//Blockchain access methods
    // Emit methods will verify the object, commit it to memory/disk and then place it in queue to
    //   be handled by listeners

    bool EmitBlock(CBlock& block);
    // Do not call EmitTransaction except for loose transactions (ie transactions not in a block)
    bool EmitTransaction(CTransaction& transaction);
    bool EmitAlert(CAlert& palert);

    // Returns the CBlockIndex that points to the block with specified hash or NULL
    const CBlockIndex* GetBlockIndex(const uint256 nBlockHash);

    // Returns the CBlockIndex that points to the block at the tip of the longest chain we have
    const CBlockIndex* GetBestBlockIndex() const;

    // Returns the CBlockIndex that points to the genesis block
    const CBlockIndex* GetGenesisBlockIndex() const;

    // Returns true if an inv should be requested or false otherwise
    bool NeedInv(const CInv* pinv);

    // Returns false if we are an SPV node, ie we can't provide full blocks when requested
    inline bool HasFullBlocks() { return true; }

    bool IsInitialBlockDownload();

    // Return transaction with hash in tx, and if it was found inside a block, its hash is placed in hashBlock
    bool GetTransaction(const uint256 &hash, CTransaction &tx, uint256 &hashBlock);

//Connected wallet/etc access methods

    // Ask that any listeners who have access to ask other nodes for blocks
    // (ie net) ask for all blocks between GetBestBlockIndex() and hashEnd
    // If hashOriginator is specified, then a node which is known to have a block
    //   with that hash will be the one to get the block request, unless no connected
    //   nodes are known to have this block, in which case a random one will be queried.
    void AskForBlocks(const uint256 hashEnd, const uint256 hashOriginator)
    {
        LOCK(cs_callbacks);
        queueAskForBlocksCallbacks.push(std::make_pair(hashEnd, hashOriginator));
    }

    // Relay all alerts we have to pnode
    void RelayAlerts(CNode* pnode);

    // Used to indicate a transaction is being relayed/has been announced by a peer
    //   (used eg for wallets counting relays of their txes)
    void Relayed(const uint256 hash)
    {
        LOCK(cs_callbacks);
        queueRelayedCallbacks.push(hash);
    }

    // Returns true if a given transaction is from a connected wallet
    bool IsTransactionFromMe(const CTransaction& tx) { return sigIsTransactionFromMe(tx); }
    bool IsTransactionFromMe(const uint256 hash) { return sigIsTransactionFromMeByHash(hash); }

    void TransactionReplaced(const uint256 hash)
    {
        LOCK(cs_callbacks);
        queueTransactionReplacedCallbacks.push(hash);
    }
};

extern CBlockStore* pblockstore;

void ProcessCallbacks(void* parg);

#endif
