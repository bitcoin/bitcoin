// Copyright (c) 2013 Primecoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PRIMECOIN_PRIME_H
#define PRIMECOIN_PRIME_H

#include "main.h"


// Generate small prime table
void GeneratePrimeTable();

// Compute Primorial number p#
void Primorial(unsigned int p, CBigNum& bnPrimorial);

// Test Probable Cunningham Chain for: n
// fSophieGermain:
//   true - Test for Cunningham Chain of first kind (n, 2n+1, 4n+3, ...)
//   false - Test for Cunningham Chain of second kind (n, 2n-1, 4n-3, ...)
// Return value:
//   true - Probable Cunningham Chain found (nProbableChainLength >= 2)
//   false - Not Cunningham Chain (nProbableChainLength <= 1)
bool ProbableCunninghamChainTest(const CBigNum& n, bool fSophieGermain, unsigned int& nProbableChainLength);

// Mine probable Cunningham Chain of form: n = h * p# +/- 1
bool MineProbableCunninghamChain(CBlockHeader& block, CBigNum& bnPrimorial, unsigned int& nProbableChainLength);

// Find last block index up to pindex of the given proof-of-work type
// Returns: depth of last block index of shorter or equal type
unsigned int GetLastBlockIndex(const CBlockIndex* pindex, int nProofOfWorkType, const CBlockIndex** pindexPrev);

#endif
