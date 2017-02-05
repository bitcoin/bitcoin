// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POW_H
#define BITCOIN_POW_H

#include "consensus/params.h"

#include <stdint.h>

class CBlockHeader;
class CBlockIndex;
class CKeyStore;
class CValidationState;
class uint256;

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params&);
unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params&);

bool CheckProof(const CBlockHeader& block, const uint256& block_hash, CValidationState& state, const Consensus::Params& consensusParams);
bool MaybeGenerateProof(const Consensus::Params& params, CBlockHeader* pblock, const CKeyStore* pkeystore, uint64_t& nTries);
bool GenerateProof(const Consensus::Params& params, CBlockHeader* pblock);
void ResetProof(const Consensus::Params& params, CBlockHeader* pblock);

#endif // BITCOIN_POW_H
