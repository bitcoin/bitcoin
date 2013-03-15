// Copyright (c) 2013 Primecoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PRIMECOIN_PRIME_H
#define PRIMECOIN_PRIME_H

#include "main.h"


void GeneratePrimeTable();

// Check Proth primality proof:  a ** (h * 2**(k-1)) = -1 mod (h * 2**k + 1)
bool ProthPrimalityProof(uint256& h, unsigned int k, unsigned int a);

// Compute Primorial number
void Primorial(unsigned int p, CBigNum& bnPrimorial);

// Attempt to prove primality for: h * p# + 1
// Return values
// true - primality proved via Pocklington Theorem
// false - unable to prove prime (check fComposite)
// fComposite - if true then n is proven to be composite
bool PrimorialPrimalityProve(uint256& h, unsigned int p, bool& fComposite);

#endif
