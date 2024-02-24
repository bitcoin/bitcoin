// Copyright (c) 2018-2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_SPECIALTXMAN_H
#define BITCOIN_EVO_SPECIALTXMAN_H

#include <primitives/transaction.h>
#include <sync.h>
#include <threadsafety.h>

#include <optional>

class BlockValidationState;
class CBlock;
class CBlockIndex;
class CCoinsViewCache;
class CMNHFManager;
class TxValidationState;
struct MNListUpdates;
namespace llmq {
class CQuorumBlockProcessor;
class CChainLocksHandler;
} // namespace llmq
namespace Consensus {
struct Params;
} // namespace Consensus

extern RecursiveMutex cs_main;

bool CheckSpecialTx(const CTransaction& tx, const CBlockIndex* pindexPrev, const CCoinsViewCache& view, bool check_sigs,
                    TxValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
bool ProcessSpecialTxsInBlock(const CBlock& block, const CBlockIndex* pindex, CMNHFManager& mnhfManager,
                              llmq::CQuorumBlockProcessor& quorum_block_processor, const llmq::CChainLocksHandler& chainlock_handler,
                              const Consensus::Params& consensusParams, const CCoinsViewCache& view, bool fJustCheck, bool fCheckCbTxMerleRoots,
                              BlockValidationState& state, std::optional<MNListUpdates>& updatesRet) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
bool UndoSpecialTxsInBlock(const CBlock& block, const CBlockIndex* pindex, CMNHFManager& mnhfManager,
                           llmq::CQuorumBlockProcessor& quorum_block_processor, std::optional<MNListUpdates>& updatesRet) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
bool CheckCreditPoolDiffForBlock(const CBlock& block, const CBlockIndex* pindex, const Consensus::Params& consensusParams,
                                const CAmount blockSubsidy, BlockValidationState& state);

#endif // BITCOIN_EVO_SPECIALTXMAN_H
