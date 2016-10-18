// Copyright (c) 2016-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_HEADER_VERIFY_H
#define BITCOIN_HEADER_VERIFY_H

#include "consensus/params.h"

class CBlockHeader;
class CBlockIndex;
class CValidationState;
namespace Consensus { struct Params; };

bool CheckProof(const Consensus::Params& consensusParams, const CBlockHeader& pblock);
bool MaybeGenerateProof(const Consensus::Params& consensusParams, CBlockHeader* pblock, uint64_t& nTries);
/** More convenient by hiding ref var nTries for testing */
bool GenerateProof(const Consensus::Params& consensusParams, CBlockHeader* pblock);
void ResetProof(const Consensus::Params& consensusParams, CBlockHeader* pblock);

bool CheckChallenge(const Consensus::Params& consensusParams, CValidationState& state, const CBlockHeader *pblock, const CBlockIndex* pindexPrev);
void ResetChallenge(const Consensus::Params& consensusParams, CBlockHeader* pblock, const CBlockIndex* pindexPrev);

#endif // BITCOIN_HEADER_VERIFY_H
