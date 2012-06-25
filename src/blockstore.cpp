#include "blockstore.h"
#include "util.h"
#include "main.h"

void CBlockStore::CallbackCommitBlock(const CBlock &block)
{
    {
        LOCK(cs_mapGetBlockIndexWaits);
        std::map<uint256, CSemaphore*>::iterator it = mapGetBlockIndexWaits.find(block.GetHash());
        if (it != mapGetBlockIndexWaits.end() && it->second != NULL)
            it->second->post_all();
    }
    LOCK(sigtable.cs_sigCommitBlock);
    sigtable.sigCommitBlock(block);
}
