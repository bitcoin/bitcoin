// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SRC_VBK_POP_SERVICE_HPP
#define BITCOIN_SRC_VBK_POP_SERVICE_HPP

#include <veriblock/storage/block_batch_adaptor.hpp>
#include <vbk/adaptors/payloads_provider.hpp>
#include "pop_common.hpp"

class BlockValidationState;
class CBlock;
class CBlockTreeDB;
class CBlockIndex;
class CDBIterator;
class CDBWrapper;

namespace Consensus {
struct Params;
}

namespace VeriBlock {

using BlockBytes = std::vector<uint8_t>;
using PoPRewards = std::map<CScript, CAmount>;

void SetPop(CDBWrapper& db);

PayloadsProvider& GetPayloadsProvider();

CBlockIndex* compareTipToBlock(CBlockIndex* candidate);
bool acceptBlock(const CBlockIndex& indexNew, BlockValidationState& state);
bool checkPopDataSize(const altintegration::PopData& popData, altintegration::ValidationState& state);
bool popdataStatelessValidation(const altintegration::PopData& popData, altintegration::ValidationState& state);
bool addAllBlockPayloads(const CBlock& block, BlockValidationState& state);
bool setState(const uint256& block, altintegration::ValidationState& state);

PoPRewards getPopRewards(const CBlockIndex& pindexPrev, const Consensus::Params& consensusParams);
void addPopPayoutsIntoCoinbaseTx(CMutableTransaction& coinbaseTx, const CBlockIndex& pindexPrev, const Consensus::Params& consensusParams);
bool checkCoinbaseTxWithPopRewards(const CTransaction& tx, const CAmount& nFees, const CBlockIndex& pindexPrev, const Consensus::Params& consensusParams, BlockValidationState& state);

std::vector<BlockBytes> getLastKnownVBKBlocks(size_t blocks);
std::vector<BlockBytes> getLastKnownBTCBlocks(size_t blocks);

//! returns true if all tips are stored in database, false otherwise
bool hasPopData(CBlockTreeDB& db);
altintegration::PopData getPopData();
void saveTrees(altintegration::BlockBatchAdaptor& batch);
bool loadTrees(CDBIterator& iter);

void removePayloadsFromMempool(const altintegration::PopData& popData);

int compareForks(const CBlockIndex& left, const CBlockIndex& right);

CAmount getCoinbaseSubsidy(const CAmount& subsidy);

void updatePopMempoolForReorg();

void addDisconnectedPopdata(const altintegration::PopData& popData);

} // namespace VeriBlock

#endif //BITCOIN_SRC_VBK_POP_SERVICE_HPP
