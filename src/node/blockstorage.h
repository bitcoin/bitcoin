// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_BLOCKSTORAGE_H
#define BITCOIN_NODE_BLOCKSTORAGE_H

#include <cstdint>
#include <vector>

#include <fs.h>
#include <protocol.h> // For CMessageHeader::MessageStartChars

class ArgsManager;
class CBlock;
class CBlockIndex;
class CBlockUndo;
class CChain;
class CChainParams;
class ChainstateManager;
struct FlatFilePos;
namespace Consensus {
struct Params;
}

static constexpr bool DEFAULT_STOPAFTERBLOCKIMPORT{false};

extern std::atomic_bool fImporting;
extern std::atomic_bool fReindex;
/** Pruning-related variables and constants */
/** True if any block files have ever been pruned. */
extern bool fHavePruned;
/** True if we're running in -prune mode. */
extern bool fPruneMode;
/** Number of MiB of block files that we're trying to stay below. */
extern uint64_t nPruneTarget;

//! Check whether the block associated with this index entry is pruned or not.
bool IsBlockPruned(const CBlockIndex* pblockindex);

void CleanupBlockRevFiles();

/** Functions for disk access for blocks */
bool ReadBlockFromDisk(CBlock& block, const FlatFilePos& pos, const Consensus::Params& consensusParams);
bool ReadBlockFromDisk(CBlock& block, const CBlockIndex* pindex, const Consensus::Params& consensusParams);
bool ReadRawBlockFromDisk(std::vector<uint8_t>& block, const FlatFilePos& pos, const CMessageHeader::MessageStartChars& message_start);
bool ReadRawBlockFromDisk(std::vector<uint8_t>& block, const CBlockIndex* pindex, const CMessageHeader::MessageStartChars& message_start);

bool UndoReadFromDisk(CBlockUndo& blockundo, const CBlockIndex* pindex);

FlatFilePos SaveBlockToDisk(const CBlock& block, int nHeight, CChain& active_chain, const CChainParams& chainparams, const FlatFilePos* dbp);

void ThreadImport(ChainstateManager& chainman, std::vector<fs::path> vImportFiles, const ArgsManager& args);

#endif // BITCOIN_NODE_BLOCKSTORAGE_H
