#include "blockstore.h"
#include "main.h"

void ProcessCallbacks(void* parg)
{
    ((CBlockStore*)parg)->ProcessCallbacks();
}

void CBlockStore::ProcessCallbacks()
{
    loop
    {
        bool fHaveDoneSomething = false;

        std::pair<uint256, uint256> hashParams;
        {
            LOCK(cs_callbacks);
            if (!queueAskForBlocksCallbacks.empty())
            {
                hashParams = queueAskForBlocksCallbacks.front();
                queueAskForBlocksCallbacks.pop();
                fHaveDoneSomething = true;
            }
        }
        if (fHaveDoneSomething)
            sigAskForBlocks(hashParams.first, hashParams.second);

        uint256 hashRelayed;
        bool fRelayedToBeCalled = false;
        {
            LOCK(cs_callbacks);
            if (!queueRelayedCallbacks.empty())
            {
                hashRelayed = queueRelayedCallbacks.front();
                queueRelayedCallbacks.pop();
                fHaveDoneSomething = fRelayedToBeCalled = true;
            }
        }
        if (fRelayedToBeCalled)
            sigRelayed(hashRelayed);

        uint256 hashTransactionReplaced;
        bool fTransactionReplacedToBeCalled = false;
        {
            LOCK(cs_callbacks);
            if (!queueTransactionReplacedCallbacks.empty())
            {
                hashTransactionReplaced = queueTransactionReplacedCallbacks.front();
                queueTransactionReplacedCallbacks.pop();
                fHaveDoneSomething = fTransactionReplacedToBeCalled = true;
            }
        }
        if (fTransactionReplacedToBeCalled)
            sigRelayed(hashTransactionReplaced);

        CBlock* pBlockToProcess = NULL;
        {
            LOCK(cs_callbacks);
            if (!queueCommitBlockCallbacks.empty())
            {
                pBlockToProcess = queueCommitBlockCallbacks.front();
                queueCommitBlockCallbacks.pop();
                fHaveDoneSomething = true;
            }
        }
        if (pBlockToProcess)
        {
            sigCommitBlock(*pBlockToProcess);
            delete pBlockToProcess;
        }

        CTransaction* pTxToProcess = NULL;
        {
            LOCK(cs_callbacks);
            if (!queueCommitTransactionToMemoryPoolCallbacks.empty())
            {
                pTxToProcess = queueCommitTransactionToMemoryPoolCallbacks.front();
                queueCommitTransactionToMemoryPoolCallbacks.pop();
                fHaveDoneSomething = true;
            }
        }
        if (pTxToProcess)
        {
            sigCommitTransactionToMemoryPool(*pTxToProcess);
            delete pTxToProcess;
        }

        if (!fHaveDoneSomething)
            Sleep(100);

        if (fShutdown)
            return;
    }
}
