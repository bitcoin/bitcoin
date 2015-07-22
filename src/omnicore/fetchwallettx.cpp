// The fetch functions provide a sorted list of transaction hashes ordered by block, position in block and position in wallet including STO receipts
#include "omnicore/fetchwallettx.h"

#include "omnicore/omnicore.h"
#include "omnicore/pending.h"
#include "omnicore/utilsbitcoin.h"

#include "init.h"
#include "wallet.h"
#include "sync.h"

#include <stdint.h>

#include <set>
#include <map>
#include <string>

#include <boost/algorithm/string.hpp>

using namespace mastercore;

/**
  * Returns an ordered list of Omni transactions including STO receipts that are relevant to the wallet
  * Ignores order in the wallet (which can be skewed by watch addresses) and utilizes block height and position within block
  */
std::map<std::string,uint256> FetchWalletOmniTransactions(int count, int startBlock, int endBlock)
{
    // Iterate backwards through wallet transactions until we have count items to return:
    std::map<std::string,uint256> mapResponse;
    std::set<uint256> seenHashes;
    std::list<CAccountingEntry> acentries;
    CWallet::TxItems txOrdered;
    {
        LOCK(pwalletMain->cs_wallet);
        txOrdered = pwalletMain->OrderedTxItems(acentries, "*");
    }
    for (CWallet::TxItems::reverse_iterator it = txOrdered.rbegin(); it != txOrdered.rend(); ++it) {
        CWalletTx *const pwtx = (*it).second.first;
        if (pwtx == 0) continue;
        uint256 txHash = pwtx->GetHash();
        {
            LOCK(cs_tally);
            if (!p_txlistdb->exists(txHash)) continue;
        }
        uint256 blockHash = pwtx->hashBlock;
        if ((0 == blockHash) || (NULL == GetBlockIndex(blockHash))) continue;
        CBlockIndex* pBlockIndex = GetBlockIndex(blockHash);
        if (NULL == pBlockIndex) continue;
        int blockHeight = pBlockIndex->nHeight;
        if (blockHeight < startBlock || blockHeight > endBlock) continue;
        int blockPosition = GetTransactionByteOffset(txHash);
        std::string sortKey = strprintf("%06d%010d", blockHeight, blockPosition);
        mapResponse.insert(std::make_pair(sortKey, txHash));
        seenHashes.insert(txHash);
        if ((int)mapResponse.size() >= count) break;
    }

    // Insert STO receipts - receiving an STO has no inbound transaction to the wallet, so we will insert these manually into the response
    std::string mySTOReceipts;
    {
        LOCK(cs_tally);
        mySTOReceipts = s_stolistdb->getMySTOReceipts("");
    }
    std::vector<std::string> vecReceipts;
    if (!mySTOReceipts.empty()) {
        boost::split(vecReceipts, mySTOReceipts, boost::is_any_of(","), boost::token_compress_on);
    }
    for (size_t i = 0; i < vecReceipts.size(); i++) {
        std::vector<std::string> svstr;
        boost::split(svstr, vecReceipts[i], boost::is_any_of(":"), boost::token_compress_on);
        if (svstr.size() != 4) {
            PrintToLog("STODB Error - number of tokens is not as expected (%s)\n", vecReceipts[i]);
            continue;
        }
        int blockHeight = atoi(svstr[1]);
        if (blockHeight < startBlock || blockHeight > endBlock) continue;
        uint256 txHash(svstr[0]);
        if (seenHashes.find(txHash) != seenHashes.end()) continue; // an STO may already be in the wallet if we sent it
        int blockPosition = GetTransactionByteOffset(txHash);
        std::string sortKey = strprintf("%06d%010d", blockHeight, blockPosition);
        mapResponse.insert(std::make_pair(sortKey, txHash));
    }

    // Insert pending transactions (sets block as 999999 and position as wallet position)
    for (PendingMap::iterator it = my_pending.begin(); it != my_pending.end(); ++it) {
        uint256 txHash = it->first;
        int blockHeight = 999999;
        if (blockHeight < startBlock || blockHeight > endBlock) continue;
        int blockPosition = 0;
        {
            LOCK(pwalletMain->cs_wallet);
            std::map<uint256, CWalletTx>::const_iterator walletIt = pwalletMain->mapWallet.find(txHash);
            if (walletIt != pwalletMain->mapWallet.end()) {
                const CWalletTx* pendingWTx = &(*walletIt).second;
                blockPosition = pendingWTx->nOrderPos;
            }
        }
        std::string sortKey = strprintf("%06d%010d", blockHeight, blockPosition);
        mapResponse.insert(std::make_pair(sortKey, txHash));
    }

    return mapResponse;
}

/** Gets the byte offset of a transaction from the tx index
  */
uint64_t GetTransactionByteOffset(const uint256& txid)
{
    LOCK(cs_main);
    CDiskTxPos position;
    if (pblocktree->ReadTxIndex(txid, position)) {
        return position.nTxOffset;
    }
    return 0;
}
