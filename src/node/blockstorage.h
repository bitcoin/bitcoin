// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_BLOCKSTORAGE_H
#define BITCOIN_NODE_BLOCKSTORAGE_H

#include <fs.h>
#include <protocol.h> // For CMessageHeader::MessageStartChars

#include <cstdint>
#include <vector>

class ArgsManager;
class CBlock;
class CBlockIndex;
class CBlockUndo;
class CChain;
class CChainParams;
class CDSNotificationInterface;
class ChainstateManager;
struct FlatFilePos;
namespace Consensus {
struct Params;
}

static constexpr bool DEFAULT_STOPAFTERBLOCKIMPORT{false};

/** Functions for disk access for blocks */
bool ReadBlockFromDisk(CBlock& block, const FlatFilePos& pos, const Consensus::Params& consensusParams);
bool ReadBlockFromDisk(CBlock& block, const CBlockIndex* pindex, const Consensus::Params& consensusParams);

bool UndoReadFromDisk(CBlockUndo& blockundo, const CBlockIndex* pindex);

FlatFilePos SaveBlockToDisk(const CBlock& block, int nHeight, CChain& active_chain, const CChainParams& chainparams, const FlatFilePos* dbp);

void ThreadImport(ChainstateManager& chainman, CDSNotificationInterface& dsnfi, std::vector<fs::path> vImportFiles, const ArgsManager& args);

#endif // BITCOIN_NODE_BLOCKSTORAGE_H
