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
class BlockValidationState;
class CBlock;
class CBlockFileInfo;
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

/** The pre-allocation chunk size for blk?????.dat files (since 0.8) */
static const unsigned int BLOCKFILE_CHUNK_SIZE = 0x1000000; // 16 MiB
/** The pre-allocation chunk size for rev?????.dat files (since 0.8) */
static const unsigned int UNDOFILE_CHUNK_SIZE = 0x100000; // 1 MiB
/** The maximum size of a blk?????.dat file (since 0.8) */
static const unsigned int MAX_BLOCKFILE_SIZE = 0x8000000; // 128 MiB

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

/** Open a block file (blk?????.dat) */
FILE* OpenBlockFile(const FlatFilePos &pos, bool fReadOnly = false);
/** Translation to a filesystem path */
fs::path GetBlockPosFilename(const FlatFilePos &pos);

/** Get block file info entry for one block file */
CBlockFileInfo* GetBlockFileInfo(size_t n);

/** Calculate the amount of disk space the block & undo files currently use */
uint64_t CalculateCurrentUsage();

/**
 *  Actually unlink the specified files
 */
void UnlinkPrunedFiles(const std::set<int>& setFilesToPrune);

/** Functions for disk access for blocks */
bool ReadBlockFromDisk(CBlock& block, const FlatFilePos& pos, const Consensus::Params& consensusParams);
bool ReadBlockFromDisk(CBlock& block, const CBlockIndex* pindex, const Consensus::Params& consensusParams);
bool ReadRawBlockFromDisk(std::vector<uint8_t>& block, const FlatFilePos& pos, const CMessageHeader::MessageStartChars& message_start);
bool ReadRawBlockFromDisk(std::vector<uint8_t>& block, const CBlockIndex* pindex, const CMessageHeader::MessageStartChars& message_start);

bool UndoReadFromDisk(CBlockUndo& blockundo, const CBlockIndex* pindex);
bool WriteUndoDataForBlock(const CBlockUndo& blockundo, BlockValidationState& state, CBlockIndex* pindex, const CChainParams& chainparams);

FlatFilePos SaveBlockToDisk(const CBlock& block, int nHeight, CChain& active_chain, const CChainParams& chainparams, const FlatFilePos* dbp);

void ThreadImport(ChainstateManager& chainman, std::vector<fs::path> vImportFiles, const ArgsManager& args);

#endif // BITCOIN_NODE_BLOCKSTORAGE_H
