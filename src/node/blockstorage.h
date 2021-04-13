// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_NODE_BLOCKSTORAGE_H
#define SYSCOIN_NODE_BLOCKSTORAGE_H

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
// SYSCOIN
class CDSNotificationInterface;
class CDeterministicMNManager;
class CBlockHeader;
class WalletInitInterface;
struct FlatFilePos;
namespace Consensus {
struct Params;
}

static constexpr bool DEFAULT_STOPAFTERBLOCKIMPORT{false};

/** Functions for disk access for blocks */
bool ReadBlockFromDisk(CBlock& block, const FlatFilePos& pos, const Consensus::Params& consensusParams);
bool ReadBlockFromDisk(CBlock& block, const CBlockIndex* pindex, const Consensus::Params& consensusParams);
bool ReadRawBlockFromDisk(std::vector<uint8_t>& block, const FlatFilePos& pos, const CMessageHeader::MessageStartChars& message_start);
bool ReadRawBlockFromDisk(std::vector<uint8_t>& block, const CBlockIndex* pindex, const CMessageHeader::MessageStartChars& message_start);
// SYSCOIN
bool ReadBlockHeaderFromDisk(CBlockHeader& block, const CBlockIndex* pindex, const Consensus::Params& consensusParams);
bool UndoReadFromDisk(CBlockUndo& blockundo, const CBlockIndex* pindex);

FlatFilePos SaveBlockToDisk(const CBlock& block, int nHeight, CChain& active_chain, const CChainParams& chainparams, const FlatFilePos* dbp);
// SYSCOIN
void ThreadImport(ChainstateManager& chainman, std::vector<fs::path> vImportFiles, const ArgsManager& args, CDSNotificationInterface* pdsNotificationInterface, std::unique_ptr<CDeterministicMNManager> &deterministicMNManager, const WalletInitInterface &g_wallet_init_interface);

#endif // SYSCOIN_NODE_BLOCKSTORAGE_H
