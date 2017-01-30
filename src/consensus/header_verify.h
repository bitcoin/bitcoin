// Copyright (c) 2016-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_HEADER_VERIFY_H
#define BITCOIN_CONSENSUS_HEADER_VERIFY_H

#include <stdint.h>

class CBlockHeader;
class CBlockIndex;
class CValidationState;
namespace Consensus { struct Params; };
struct BlockIndexInterface;

/** Context-independent validity checks */
bool CheckBlockHeader(const CBlockHeader& block, CValidationState& state, const Consensus::Params& consensusParams, bool fCheckPOW = true);

/** Context-dependent validity checks.
 *  By "context", we mean only the previous block headers, but not the UTXO
 *  set; UTXO-related validity checks are done in ConnectBlock(). */
bool ContextualCheckHeader(const CBlockHeader& block, CValidationState& state, const Consensus::Params& consensusParams, const void* indexObject, const BlockIndexInterface& iBlockIndex, int64_t nAdjustedTime);

bool VerifyHeader(const CBlockHeader& block, CValidationState& state, const Consensus::Params& consensusParams, const void* indexObject, const BlockIndexInterface& iBlockIndex, int64_t nAdjustedTime);

#endif // BITCOIN_CONSENSUS_HEADER_VERIFY_H
