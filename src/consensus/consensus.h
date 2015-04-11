// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CONSENSUS_CONSENSUS_H
#define BITCOIN_CONSENSUS_CONSENSUS_H

class CBlockHeader;
class CValidationState;

/**
 * Consensus validations:
 * Check_ means checking everything possible with the data provided.
 * Verify_ means all data provided was enough for this level and its "consensus-verified".
 */
namespace Consensus {

class Params;

/** Block header validation functions */

bool CheckBlockHeader(const CBlockHeader& block, CValidationState& state, const Params& consensusParams, int64_t nTime, bool fCheckPOW = true);

} // namespace Consensus

#endif // BITCOIN_CONSENSUS_CONSENSUS_H
