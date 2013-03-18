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

// Binary logarithm scale of a big number, times 65536
// Can be used as an approximate log scale for numbers up to 2 ** 65536 - 1
bool LogScale(const CBigNum& bn, unsigned int& nLogScale)
{
    // Get number of bits
    if (bn < 1)
        return error("LogScale() : not positive number");
    unsigned int nBytes = BN_num_bytes(&bn);
    if (nBytes < 1)
        return error("LogScale() : zero size");
    CBigNum bnMantissa;
    BN_rshift(&bnMantissa, &bn, 8 * (nBytes - 1));
    unsigned int nByte = BN_get_word(&bnMantissa);
    unsigned int nBits = 8 * (nBytes - 1);
    for (; nByte ; nByte >>= 1)
        nBits++;
    if (nBits < 1)
        return error("LogScale() : zero bits");
    // Get 17-bit mantissa
    bnMantissa = ((bn << 17) >> nBits);
    unsigned int nMantissa = BN_get_word(&bnMantissa);
    // Combine to form approximate log scale
    nLogScale = (nMantissa & 0xffff);
    nLogScale |= ((nBits - 1) << 16);
    return true;
}

// Binary log log scale of a big number, times 65536
// Can be used as an approximate log log scale for numbers up to 2 ** 65536 - 1
static bool LogLogScale(const CBigNum& bn, unsigned int& nLogLogScale)
{
    if (bn < 2)
        return error("LogLogScale() : undefined for log log; must be at least 2");
    unsigned int nLogScale;
    if (!LogScale(bn, nLogScale))
        return error("LogLogScale() : unable to calculate log scale");
    if (!LogScale(CBigNum(nLogScale), nLogLogScale))
        return error("LogLogScale() : unable to calculate log log scale");
    // Here log refers to base 2 (binary) logarithm
    // nLogLogScale ~ 65536 * (16 + log(log(bn)))
    if (nLogLogScale < (1 << 20))
        return error("GetProofOfWorkHashTarget() : log log scale below mininum");
    nLogLogScale -= (1 << 20); // ~ 65536 * log(log(bn))
    return true;
}

// Design Note: Hash target vs. Prime target
//
// In Primecoin's hybrid proof-of-work model, a block must meet both a hash
// target (hash proof-of-work) and a prime target (prime proof-of-work).
// Hash target is understood in the traditional Bitcoin sense, that is,
// the block hash must be less than or equal to the hash target.
// Prime target is the target that the first probable prime of the
// probable Cunningham Chain must meet, that is, the first probable prime
// of the chain must be greater than or equal to the prime target. Prime
// target is stored as nBits inside block header.
//
// The reason that hash proof-of-work is still needed is because the size of
// the probable prime must be capped in order to limit the amount of work
// required to verify proof-of-work.
//
// The cap is set at 2 ** 2039 - 1 as this is the max value of Bitcoin's
// compact target representation, and a reasonably good cap for performing
// Fermat-Euler-Lagrange-Lifchitz primality testing on Cunningham Chains.
//
// The prime target is then mapped to hash target via a chosen curve such
// that very little amount of hash proof-of-work is required until the prime
// approaches the cap, and as it's approaching the cap, hash proof-of-work
// required would rise toward infinity.
//
// This design allows any given length of Cunningham Chain to be permanently 
// present on block chain even if finding such length of chain at the cap
// size becomes too easy by itself. As combined with the required hash 
// proof-of-work it still becomes infinitely difficult as the prime approaches
// the cap.


// Compute hash target from prime target
static bool GetProofOfWorkHashTarget(unsigned int nBits, CBigNum& bnHashTarget)
{
    CBigNum bnPrimeTarget = CBigNum().SetCompact(nBits);
    if (bnPrimeTarget < bnProofOfWorkLimit || bnPrimeTarget >= bnProofOfWorkMax)
        return error("GetProofOfWorkHashTarget() : prime target out of valid range");
    if (bnProofOfWorkMax / bnPrimeTarget < 2)
    {
        bnHashTarget = 0;
        return true;
    }
    unsigned int nLogLogScale;
    if (!LogLogScale(bnProofOfWorkMax / bnPrimeTarget, nLogLogScale))
        return error("GetProofOfWorkHashTarget() : unable to calculate log log scale of prime target");
    unsigned int nLogLogScaleMax;
    if (!LogLogScale(bnProofOfWorkMax / bnProofOfWorkLimit, nLogLogScaleMax))
        return error("GetProofOfWorkHashTarget() : unable to calculate max log log scale");
    if (nLogLogScale > nLogLogScaleMax || nLogLogScale >= (((unsigned int )1) << 31))
        return error("GetProofOfWorkHashTarget() : log log scale exceeds max");
    nLogLogScale = (nLogLogScaleMax - nLogLogScale) * 2;
    uint64 nLogLogSquared = ((uint64) nLogLogScale * (uint64) nLogLogScale);
    // log refers to binary logarithm
    // nLogLogSquared ~ (2 ** 32) * ((2 log(log(bnPOWMax/bnPrimeTarget))) ** 2)
    // bnHashTarget = bnPOWLimit / (2 ** ((2 log(log(bnPOWMax/bnPrimeTarget))) ** 2))
    CBigNum bnHashDifficulty = CBigNum((nLogLogSquared & 0xffffffffllu) + 0x100000000llu) << (nLogLogSquared >> 32);
    bnHashTarget = (bnProofOfWorkLimit << 32) / bnHashDifficulty;
    return true;
}

void PrintMappingPrimeTargetToHashTarget()
{
    CBigNum bnHashTarget;
    printf("proof-of-work target mapping:\n  prime target => hash target\n");
    for (unsigned int n = 256; n < 2039; n++)
    {
        unsigned int nBits = (CBigNum(1) << n).GetCompact();
        GetProofOfWorkHashTarget(nBits, bnHashTarget);
        printf("  0x%08x      0x%08x\n", nBits, bnHashTarget.GetCompact());
    }
}

// Check hash proof-of-work
bool CheckHashProofOfWork(uint256 hash, unsigned int nBits)
{
    CBigNum bnHashTarget;
    if (!GetProofOfWorkHashTarget(nBits, bnHashTarget))
        return error("CheckHashProofOfWork() : failed to get hash target");
    // Check target for hash proof-of-work
    if (hash > bnHashTarget.getuint256())
        return error("CheckHashProofOfWork() : hash not meeting hash target");
    return true;
}

// Check prime proof-of-work
bool CheckPrimeProofOfWork(uint256 hash, unsigned int nBits, const CBigNum& bnProbablePrime, int& nProofOfWorkType)
{
    CBigNum bnPrimeTarget;
    bnPrimeTarget.SetCompact(nBits);

    // Check range
    if (bnPrimeTarget < bnProofOfWorkLimit)
        return error("CheckPrimeProofOfWork() : nBits below minimum work");

    // Check target for prime proof-of-work 
    if (bnProbablePrime < bnPrimeTarget)
        return error("CheckPrimeProofOfWork() : prime not meeting prime target");
    // Prime must not exceed cap
    if (bnProbablePrime > bnProofOfWorkMax)
        return error("CheckPrimeProofOfWork() : prime exceeds cap");

    // Check bnProbablePrime % hash = +/- 1
    CBigNum bnRemainder = bnProbablePrime % CBigNum(hash);
    if ((bnRemainder != 1) && (bnRemainder + 1 != CBigNum(hash)))
        return error("CheckPrimeProofOfWork() : bnProbablePrime+/-1 not divisible by hash");

    // Check Cunningham Chain of first kind
    nProofOfWorkType = 0;
    unsigned int nChainLength;
    if (ProbableCunninghamChainTest(bnProbablePrime, true, nChainLength))
        nProofOfWorkType = nChainLength;
    // Check Cunningham Chain of second kind
    if (ProbableCunninghamChainTest(bnProbablePrime, false, nChainLength) && (int)nChainLength > nProofOfWorkType)
        nProofOfWorkType = -nChainLength;
    if (nProofOfWorkType == 0)
        return error("CheckPrimeProofOfWork() : failed Cunningham Chain test");
    return true;
}

