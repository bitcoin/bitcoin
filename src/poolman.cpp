
#include "poolman.h"
#include "core.h"
#include "main.h"
#include "wallet.h"
#include "init.h"

using namespace std;

int64_t janitorExpire; // global; expire TXs n seconds older than this

static unsigned int IgnoreWalletTransactions(vector<CTransaction>& vtx)
{
    unsigned int nMine = 0;

#ifdef ENABLE_WALLET
    if (!pwalletMain)
        return 0;

    vector<CTransaction> vtxTmp;
    BOOST_FOREACH(const CTransaction& tx, vtx) {
        if (pwalletMain->IsMine(tx))
            nMine++;
        else
            vtxTmp.push_back(tx);
    }

    if (nMine > 0)
        vtx = vtxTmp;
#endif // ENABLE_WALLET

    return nMine;
}

void TxMempoolJanitor()
{
    int64_t nStart = GetTimeMillis();
    int64_t expireTime = GetTime() - janitorExpire;

    // pass 1: get matching transactions
    vector<CTransaction> vtx;
    mempool.queryOld(vtx, expireTime);
    unsigned int nOld = vtx.size();

    // pass 2: ignore wallet transactions (remove from vtx)
    unsigned int nMine = IgnoreWalletTransactions(vtx);

    // pass 3: remove old transactions from mempool
    bool fRecursive = false;
    std::list<CTransaction> removed;
    mempool.removeBatch(vtx, removed, fRecursive);

    LogPrint("mempool", "mempool janitor run complete,  %dms\n",
             GetTimeMillis() - nStart);
    LogPrint("mempool", "    %u old, %u old wallet, %u removed\n",
             nOld, nMine, removed.size());
}

