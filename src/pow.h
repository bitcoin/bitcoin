// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POW_H
#define BITCOIN_POW_H

#include <stdint.h>

class CBlockHeader;
class CBlockIndex;
class CProof;
class uint256;

/** Check whether a block hash satisfies the proof-of-work requirement specified by nBits */
bool CheckProof(uint256 hash, const CProof& proof);
bool CheckChallenge(const CProof& proof, const CBlockIndex* pindexLast, int64_t nTime);
void ResetChallenge(CProof& proof, const CBlockIndex* pindexLast, int64_t nTime);

void UpdateTime(CBlockHeader* block, const CBlockIndex* pindexPrev);

uint256 GetProofIncrement(const CProof& proof);

double GetChallengeDouble(const CProof& proof);

#endif // BITCOIN_POW_H
