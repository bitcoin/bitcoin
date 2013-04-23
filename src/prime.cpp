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

// Compute first primorial number greater than or equal to pn
void PrimorialAt(CBigNum& bn, CBigNum& bnPrimorial)
{
    bnPrimorial = 1;
    BOOST_FOREACH(unsigned int nPrime, vPrimes)
    {
        bnPrimorial *= nPrime;
        if (bnPrimorial >= bn)
            return;
    }
}

// Check Fermat probable primality test (2-PRP): 2 ** (n-1) = 1 (mod n)
// true: n is probable prime
// false: n is composite
static bool FermatProbablePrimalityTest(const CBigNum& n, unsigned int& nLength)
{
    CAutoBN_CTX pctx;
    CBigNum a = 2; // base; Fermat witness
    CBigNum e = n - 1;
    CBigNum r;
    BN_mod_exp(&r, &a, &e, &n, pctx);
    if (r == 1)
    {
        return true;
    }
    else
    {
        nLength = (nLength & 0xfff00000u) | ((((n-r) << nFractionalBits) / n).getuint());
        return false;
    }
}

// Test probable primality of n = 2p +/- 1 based on Euler, Lagrange and Lifchitz
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

// Proof-of-work Target (prime chain target):
//   format - 32 bit, 2 flags bits, 10 length bits, 20 fractional length bits
// Flags:
//   fSophieGermain  0x80000000  (Cunningham chain of first kind)
//   fBiTwin         0x40000000  (BiTwin chain)
// Flags value (high 2 bits):
//   0 - Cunningham chain of second kind
//   1 - BiTwin chain (allow one non-twin prime at the end for odd length chain)
//   2 - Cunningham chain of first kind

static const unsigned int TARGET_SOPHIE_GERMAIN = 0x80000000u;
static const unsigned int TARGET_BI_TWIN        = 0x40000000u;
static unsigned int TargetGetLength(unsigned int nBits)
{
    return ((nBits & 0x3ff00000u) >> 20);
}

bool TargetSetLength(unsigned int nLength, unsigned int& nBits)
{
    if (nLength >= 0xff)
        return error("TargetSetLength() : invalid length=%u", nLength);
    nBits &= ~(0x3ff00000u);
    nBits |= (nLength << nFractionalBits);
    return true;
}

static unsigned int TargetGetLengthWithFractional(unsigned int nBits)
{
    return (nBits & 0x3fffffffu);
}

unsigned int TargetGetFractional(unsigned int nBits)
{
    return (nBits & 0xfffffu);
}

uint64 TargetGetFractionalDifficulty(unsigned int nBits)
{
    return ((1llu << (nFractionalBits+32)) / (uint64) (0x00100000u - TargetGetFractional(nBits)));
}

bool TargetSetFractionalDifficulty(uint64 nFractionalDifficulty, unsigned int& nBits)
{
    if (nFractionalDifficulty < nFractionalDifficultyMin)
        return error("TargetSetFractionalDifficulty() : difficulty below min");
    uint64 nFractional = (1llu << (nFractionalBits+32)) / nFractionalDifficulty;
    if (nFractional >= 0x00100000u)
        return error("TargetSetFractionalDifficulty() : difficulty overflow");
    nFractional = 0x00100000u - nFractional;
    nBits &= 0xfff00000u;
    nBits |= (unsigned int)nFractional;
    return true;
}

bool TargetIsSophieGermain(unsigned int nBits)
{
    return (nBits & TARGET_SOPHIE_GERMAIN);
}

void TargetSetSophieGermain(bool fSophieGermain, unsigned int& nBits)
{
    if (fSophieGermain)
        nBits |= TARGET_SOPHIE_GERMAIN;
    else
        nBits &= ~(TARGET_SOPHIE_GERMAIN);
}

bool TargetIsBiTwin(unsigned int nBits)
{
    return (nBits & TARGET_BI_TWIN);
}

void TargetSetBiTwin(bool fBiTwin, unsigned int& nBits)
{
    if (fBiTwin)
        nBits |= TARGET_BI_TWIN;
    else
        nBits &= ~(TARGET_BI_TWIN);
}

std::string TargetGetName(unsigned int nBits)
{
    return strprintf("%s%02u", TargetIsBiTwin(nBits)? "TWN" : (TargetIsSophieGermain(nBits)? "1CC" : "2CC"), TargetGetLength(nBits));
}

static unsigned int TargetLengthFromInt(unsigned int nLength)
{
    return (nLength << 20);
}

// Test Probable Cunningham Chain for: n
// fSophieGermain:
//   true - Test for Cunningham Chain of first kind (n, 2n+1, 4n+3, ...)
//   false - Test for Cunningham Chain of second kind (n, 2n-1, 4n-3, ...)
// Return value:
//   true - Probable Cunningham Chain found (nProbableChainLength >= 2)
//   false - Not Cunningham Chain (nProbableChainLength <= 1)
static bool ProbableCunninghamChainTest(const CBigNum& n, bool fSophieGermain, unsigned int& nProbableChainLength)
{
    nProbableChainLength = 0;
    CBigNum N = n;

    // Fermat test for n first
    if (!FermatProbablePrimalityTest(N, nProbableChainLength))
        return false;

    // Euler-Lagrange-Lifchitz test for the following numbers in chain
    do
    {
        nProbableChainLength += (1 << nFractionalBits);
        N = N + N + (fSophieGermain? 1 : (-1));
    }
    while (EulerLagrangeLifchitzPrimalityTest(N, fSophieGermain));

    FermatProbablePrimalityTest(N, nProbableChainLength);
    return (nProbableChainLength >= (2 << nFractionalBits));
}

// Test probable prime chain for: n
// Return value:
//   true - Probable prime chain found (nProbableChainLength meeting target)
//   false - prime chain too short (nProbableChainLength not meeting target)
bool ProbablePrimeChainTest(const CBigNum& n, unsigned int nBits, unsigned int& nProbableChainLength)
{
    nProbableChainLength = 0;
    bool fSophieGermain= TargetIsSophieGermain(nBits);
    bool fBiTwin = TargetIsBiTwin(nBits);

    if (fBiTwin)
    {
        // BiTwin chain allows a single prime at the end for odd length chain
        unsigned int nLengthFirstKind = 0;
        unsigned int nLengthSecondKind = 0;
        ProbableCunninghamChainTest(n, true, nLengthFirstKind);
        ProbableCunninghamChainTest(n+2, false, nLengthSecondKind);
        nProbableChainLength = (TargetGetLength(nLengthFirstKind) > TargetGetLength(nLengthSecondKind))? (nLengthSecondKind + TargetLengthFromInt(TargetGetLength(nLengthSecondKind)+1)) : (nLengthFirstKind + TargetLengthFromInt(TargetGetLength(nLengthFirstKind)));
    }
    else
        ProbableCunninghamChainTest(n, fSophieGermain, nProbableChainLength);
    return (nProbableChainLength >= TargetGetLengthWithFractional(nBits));
}

// Mine probable prime chain of form: n = h * p# +/- 1
bool MineProbablePrimeChain(CBlock& block, CBigNum& bnPrimorial, CBigNum& bnTried, unsigned int& nProbableChainLength, unsigned int& nTests, unsigned int& nPrimesHit)
{
    CBigNum n;
    bool fSophieGermain = TargetIsSophieGermain(block.nBits);
    bool fBiTwin = TargetIsBiTwin(block.nBits);

    int64 nStart = GetTimeMicros();
    int64 nCurrent = nStart;
    nTests = 0;
    nPrimesHit = 0;

    while (nCurrent < nStart + 10000 && nCurrent >= nStart)
    {
        nTests++;
        bnTried++;
        n = CBigNum(block.GetHash()) * bnPrimorial * bnTried + ((fSophieGermain || fBiTwin)? -1 : 1);
        if (ProbablePrimeChainTest(n, block.nBits, nProbableChainLength))
        {
            printf("Probable prime chain of type %s found for block=%s!! \n", TargetGetName(block.nBits).c_str(), block.GetHash().GetHex().c_str());
            block.bnPrimeChainMultiplier = bnPrimorial * bnTried;
            return true;
        }
        else if(nProbableChainLength > 0)
            nPrimesHit++;

        nCurrent = GetTimeMicros();
    }
    return false; // stop as timed out
}

// Find last block index up to pindex of the given prime chain type
bool GetLastBlockIndex(const CBlockIndex* pindex, bool fSophieGermain, bool fBiTwin, const CBlockIndex** pindexPrev)
{
    if (fSophieGermain && fBiTwin)
        return error("GetLastBlockIndex() : ambiguous chain type");
    for (; pindex && pindex->pprev; pindex = pindex->pprev)
    {
        if ((TargetIsSophieGermain(pindex->nBits) == fSophieGermain) &&
            (TargetIsBiTwin(pindex->nBits) == fBiTwin))
            break;
    }
    *pindexPrev = pindex;
    return true;
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

// Check prime proof-of-work
bool CheckPrimeProofOfWork(uint256 hash, unsigned int nBits, const CBigNum& bnPrimeChainMultiplier)
{
    // Check type
    bool fSophieGermain= TargetIsSophieGermain(nBits);
    bool fBiTwin = TargetIsBiTwin(nBits);
    if (fSophieGermain && fBiTwin)
        return error("CheckPrimeProofOfWork() : invalid prime chain type");
    if (TargetGetLength(nBits) < 2 || TargetGetLength(nBits) > 99)
        return error("CheckPrimeProofOfWork() : invalid chain length target %u", TargetGetLength(nBits));

    // Check target for prime proof-of-work
    CBigNum bnProbablePrime = (CBigNum(hash) * bnPrimeChainMultiplier) + ((fSophieGermain || fBiTwin)? (-1) : 1);
    if (bnProbablePrime < bnPrimeMin)
        return error("CheckPrimeProofOfWork() : prime too small");
    // First prime in chain must not exceed cap
    if (bnProbablePrime > bnPrimeMax)
        return error("CheckPrimeProofOfWork() : prime too big");

    // Check prime chain
    unsigned int nChainLength;
    if (ProbablePrimeChainTest(bnProbablePrime, nBits, nChainLength) && nChainLength >= TargetGetLengthWithFractional(nBits))
        return true;
    return error("CheckPrimeProofOfWork() : failed prime chain test type=%s nBits=%08x length=%08x", TargetGetName(nBits).c_str(), nBits, nChainLength);
}

// prime target difficulty value for visualization
double GetPrimeDifficulty(unsigned int nBits)
{
    return ((double) TargetGetLengthWithFractional(nBits) / (double) (1 << 20));
}


