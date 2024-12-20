// Copyright (c) 2017-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_MEMPOOL_H
#define BITCOIN_RPC_MEMPOOL_H

class CTxMemPool;
class UniValue;
namespace llmq {
class CInstantSendManager;
} // namespace llmq

/** Mempool information to JSON */
UniValue MempoolInfoToJSON(const CTxMemPool& pool, const llmq::CInstantSendManager& isman);

/** Mempool to JSON */
UniValue MempoolToJSON(const CTxMemPool& pool, const llmq::CInstantSendManager* isman, bool verbose = false, bool include_mempool_sequence = false);

#endif // BITCOIN_RPC_MEMPOOL_H
