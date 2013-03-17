// Copyright (c) 2013 Primecoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "prime.h"

// Prime Table
std::vector<unsigned int> vPrimes;


void GeneratePrimeTable()
{
    static const unsigned int nPrimeTableLimit = 1000000;
    vPrimes.clear();
    // Generate prime table using sieve of Eratosthenes
    std::vector<unsigned int> vfComposite (nPrimeTableLimit, false);
    for (unsigned int nFactor = 2; nFactor * nFactor < nPrimeTableLimit; nFactor++)
    {
        if (vfComposite[nFactor])
            continue;
        for (unsigned int nComposite = nFactor * nFactor; nComposite < nPrimeTableLimit; nComposite += nFactor)
            vfComposite[nComposite] = true;
    }
    for (unsigned int n = 2; n < nPrimeTableLimit; n++)
        if (!vfComposite[n])
            vPrimes.push_back(n);
    printf("GeneratePrimeTable() : prime table [1, %d] generated with %lu primes", nPrimeTableLimit, vPrimes.size());
    //BOOST_FOREACH(unsigned int nPrime, vPrimes)
    //    printf(" %u", nPrime);
    printf("\n");
}

// Compute Primorial number p#
void Primorial(unsigned int p, CBigNum& bnPrimorial)
{
    bnPrimorial = 1;
    BOOST_FOREACH(unsigned int nPrime, vPrimes)
    {
        if (nPrime > p) break;
        bnPrimorial *= nPrime;
    }
}

// Check Fermat probable primality test (2-PRP): 2 ** (n-1) = 1 (mod n)
// true: n is probable prime
// false: n is composite
static bool FermatProbablePrimalityTest(const CBigNum& n)
{
    CAutoBN_CTX pctx;
    CBigNum a = 2; // base; Fermat witness
    CBigNum e = n - 1;
    CBigNum r;
    BN_mod_exp(&r, &a, &e, &n, pctx);
    if (r != 1)
        return false;
    return true;
}

// Test primality of n = 2p +/- 1 based on Euler, Lagrange and Lifchitz
// fSophieGermain:
//   true:  n = 2p+1, p prime, aka Cunningham Chain of first kind
//   false: n = 2p-1, p prime, aka Cunningham Chain of second kind
static bool EulerLagrangeLifchitzPrimalityTest(const CBigNum& n, bool fSophieGermain)
{
    CAutoBN_CTX pctx;
    CBigNum a = 2;
    CBigNum e = (n - 1) >> 1;
    CBigNum r;
    BN_mod_exp(&r, &a, &e, &n, pctx);
    CBigNum nMod8 = n % 8;
    if (fSophieGermain && (nMod8 == 7)) // Euler & Lagrange
        return (r == 1);
    if (fSophieGermain && (nMod8 == 3)) // Lifchitz
        return ((r+1) == n);
    if ((!fSophieGermain) && (nMod8 == 5)) // Lifchitz
        return ((r+1) == n);
    if ((!fSophieGermain) && (nMod8 == 1)) // LifChitz
        return (r == 1);
    return error("EulerLagrangeLifchitzPrimalityTest() : invalid n %% 8 = %d, %s", nMod8.getint(), (fSophieGermain? "first kind" : "second kind"));
}

// Test Probable Cunningham Chain for: n
// fSophieGermain:
//   true - Test for Cunningham Chain of first kind (n, 2n+1, 4n+3, ...)
//   false - Test for Cunningham Chain of second kind (n, 2n-1, 4n-3, ...)
// Return value:
//   true - Probable Cunningham Chain found (nProbableChainLength >= 2)
//   false - Not Cunningham Chain (nProbableChainLength <= 1)
bool ProbableCunninghamChainTest(const CBigNum& n, bool fSophieGermain, unsigned int& nProbableChainLength)
{
    nProbableChainLength = 0;
    CBigNum N = n;

    // Fermat test for n first
    if (!FermatProbablePrimalityTest(N))
        return false;

    // Euler-Lagrange-Lifchitz test for the following numbers in chain
    do
    {
        nProbableChainLength++;
        N = N + N + (fSophieGermain? 1 : (-1));
    }
    while (EulerLagrangeLifchitzPrimalityTest(N, fSophieGermain));

    return (nProbableChainLength >= 2);
}

// Mine probable Cunningham Chain of form: n = h * p# +/- 1
bool MineProbableCunninghamChain(CBlockHeader& block, CBigNum& bnPrimorial, unsigned int& nProbableChainLength)
{
    CBigNum n;

    int64 nStart = GetTimeMicros();
    int64 nCurrent = nStart;

    while (nCurrent < nStart + 10000 && nCurrent >= nStart)
    {
        block.nNonce++;

        // Check for first kind
        n = (CBigNum(block.GetHash()) * bnPrimorial) - 1;
        if (ProbableCunninghamChainTest(n, true, nProbableChainLength))
        {
            printf("Probable Cunningham chain of length %u of first kind found for nonce=%u!! \n", nProbableChainLength, block.nNonce);
            return true;
        }

        // Check for second kind
        n += 2;
        if (ProbableCunninghamChainTest(n, false, nProbableChainLength))
        {
            printf("Probable Cunningham chain of length %u of second kind found for nonce=%u!! \n", nProbableChainLength, block.nNonce);
            return true;
        }

        // stop if nNonce exhausted
        if (block.nNonce == 0xffffffffu)
            return false;

        nCurrent = GetTimeMicros();
    }
    return false; // stop as timed out
}

// Find last block index up to pindex of the given proof-of-work type
// Returns: depth of last block index of shorter or equal type
unsigned int GetLastBlockIndex(const CBlockIndex* pindex, int nProofOfWorkType, const CBlockIndex** pindexPrev)
{
    bool fFoundLastEqualOrShorterType = false;
    int nDepthOfEqualOrShorterType = 0;
    for (; pindex && pindex->pprev && pindex->nProofOfWorkType != nProofOfWorkType; pindex = pindex->pprev)
    {
        if (!fFoundLastEqualOrShorterType)
        {
            if (((pindex->nProofOfWorkType >= 2 && pindex->nProofOfWorkType <= nProofOfWorkType) ||
             (pindex->nProofOfWorkType <= -2 && pindex->nProofOfWorkType >= nProofOfWorkType)))
                fFoundLastEqualOrShorterType = true;
            else
                nDepthOfEqualOrShorterType++;
        }
    }
    *pindexPrev = pindex;
    return nDepthOfEqualOrShorterType;
}

