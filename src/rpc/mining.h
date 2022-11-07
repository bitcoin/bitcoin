// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_RPC_MINING_H
#define BITCOIN_RPC_MINING_H

#include <script/script.h>

#include <univalue.h>

namespace llmq {
class CChainLocksHandler;
class CInstantSendManager;
class CQuorumBlockProcessor;
} // namespace llmq

/** Generate blocks (mine) */
UniValue generateBlocks(ChainstateManager& chainman, const CTxMemPool& mempool, llmq::CQuorumBlockProcessor& quorum_block_processor, llmq::CChainLocksHandler& clhandler,
                        llmq::CInstantSendManager& isman, std::shared_ptr<CReserveScript> coinbaseScript, int nGenerate, uint64_t nMaxTries, bool keepScript);

#endif // BITCOIN_RPC_MINING_H
