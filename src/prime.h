// Copyright (c) 2013 Primecoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PRIMECOIN_PRIME_H
#define PRIMECOIN_PRIME_H

#include "main.h"


// Generate small prime table
void GeneratePrimeTable();

// Compute primorial number p#
void Primorial(unsigned int p, CBigNum& bnPrimorial);
// Compute the first primorial number greater than or equal to bn
void PrimorialAt(CBigNum& bn, CBigNum& bnPrimorial);

// Test probable prime chain for: n
// Return value:
//   true - Probable prime chain found (nProbableChainLength >= TypeGetLength)
//   false - Not prime chain (nProbableChainLength < TypeGetLength)
bool ProbablePrimeChainTest(const CBigNum& n, unsigned int nProofOfWorkType, unsigned int& nProbableChainLength);

std::string TypeGetName(unsigned int nProofOfWorkType);

// Mine probable prime chain of form: n = h * p# +/- 1
bool MineProbablePrimeChain(CBlock& block, CBigNum& bnPrimorial, CBigNum& bnTried, unsigned int nProofOfWorkType, unsigned int& nProbableChainLength, unsigned int& nTests, unsigned int& nPrimesHit);

// Find last block index up to pindex of the given proof-of-work type
// Returns: depth of last block index of shorter or equal type
unsigned int GetLastBlockIndex(const CBlockIndex* pindex, unsigned int nProofOfWorkType, const CBlockIndex** pindexPrev);

// Size of a big number (in bits), times 65536
// Can be used as an approximate log scale for numbers up to 2 ** 65536 - 1
bool LogScale(const CBigNum& bn, unsigned int& nLogScale);

// Compute hash target from prime target
bool GetProofOfWorkHashTarget(unsigned int nBits, CBigNum& bnHashTarget);

// Print mapping from prime target to hash target
void PrintMappingPrimeTargetToHashTarget();

// Check hash and prime proof-of-work
bool CheckHashProofOfWork(uint256 hash, unsigned int nBits);
bool CheckPrimeProofOfWork(uint256 hash, unsigned int nBits, unsigned int nProofOfWorkType, const CBigNum& bnProbablePrime);

// prime target difficulty value for visualization
unsigned int GetPrimeDifficulty(unsigned int nBits);
// hash target difficulty value for visualization
unsigned int GetHashDifficulty(unsigned int nBits);

#endif
