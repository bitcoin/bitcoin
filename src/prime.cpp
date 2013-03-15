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

// Check Proth primality proof:  a ** (h * 2**(k-1)) = -1 mod (h * 2**k + 1)
bool ProthPrimalityProof(uint256& h, unsigned int k, unsigned int a)
{
    CAutoBN_CTX pctx;
    CBigNum one = 1;
    CBigNum n;
    n = ((one << k) * CBigNum(h)) + 1;
    CBigNum A = a;
    CBigNum e = (n - 1) >> 1;
    CBigNum r;
    BN_mod_exp(&r, &A, &e, &n, pctx);
    return (r + 1 == n);
}

// Compute Primorial number
void Primorial(unsigned int p, CBigNum& bnPrimorial)
{
    bnPrimorial = 1;
    BOOST_FOREACH(unsigned int nPrime, vPrimes)
    {
        if (nPrime > p) break;
        bnPrimorial *= nPrime;
    }
}

// Check Fermat primality test: a ** (n-1) = 1 (mod n)
// true: n is probable prime
// false: n is composite
static bool FermatPrimalityTest(CBigNum& n, unsigned int a)
{
    CAutoBN_CTX pctx;
    CBigNum A = a;
    CBigNum e = n - 1;
    CBigNum r;
    BN_mod_exp(&r, &A, &e, &n, pctx);
    if (r != 1)
        return false;
    return true;
}

// Check primorial primality test for n = h * p# + 1, q <= p, both p, q primes
// 1) a ** (n-1) = 1 (mod n)
// 2) gcd(a ** ((n-1)/q) - 1, n) = 1
// Return values
// true - passed test; a is a witness and n is prime candidate
// false - failed test; a is not a witness or n is composite (check fComposite)
// fComposite - if true then n is proven to be composite
static bool PocklingtonPrimalityTest(uint256& h, CBigNum& bnFactored, unsigned int q, unsigned int a, bool& fComposite)
{
    fComposite = false;
    CAutoBN_CTX pctx;
    CBigNum n;
    n = (bnFactored * CBigNum(h)) + 1;
    if (!FermatPrimalityTest(n, a))
    {
        fComposite = true;
        return false;
    }
    CBigNum A = a;
    CBigNum e = (n - 1) / q;
    CBigNum b;
    BN_mod_exp(&b, &A, &e, &n, pctx);
    if (b <= 1)
    {
        fComposite = (b == 0); // a**e = 0 (mod n) means gcd(a, n) > 1
        return false;
    }
    b -= 1;
    CBigNum r;
    BN_gcd(&r, &b, &n, pctx);
    if (r <= 0 || r >= n)
        return error("PrimorialPrimalityTest() : BN_gcd returned invalid value");
    if (r > 1)
    {
        fComposite = true;
        return false;
    }
    // passed test, a is witness and n is prime candidate
    return true;
}

// Attempt to prove primality for: h * p# + 1
bool PrimorialPrimalityProve(uint256& h, unsigned int p, bool& fComposite)
{
    fComposite = false;
    bool fWitness = false;
    CBigNum bnPrimorial;
    Primorial(p, bnPrimorial);
    if ((bnPrimorial >> 256) == 0)
        return error("PrimorialPrimalityProve() : primorial too small");

    BOOST_FOREACH(unsigned int nPrime, vPrimes)
    {
        if (nPrime > p) break;
        BOOST_FOREACH(unsigned int a, vPrimes)
        {
            if (a > 100)
                return false; // done too much work just give up
            if (PocklingtonPrimalityTest(h, bnPrimorial, nPrime, a, fComposite))
            {
                if (!fWitness)
                {
                    printf("Pocklington witnesses for %u * %u# + 1: ", (unsigned int)h.Get64(), p);
                    fWitness = true;
                }
                printf(" (%u, %u)", nPrime, a);
                break;
            }
            else if (fComposite)
                return false; // proved composite
        }
    }
    if (fWitness)
        printf("\n");
    return true; // proved prime
}

