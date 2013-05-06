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
// false: n is composite; set fractional length in the nLength output
static bool FermatProbablePrimalityTest(const CBigNum& n, unsigned int& nLength)
{
    CAutoBN_CTX pctx;
    CBigNum a = 2; // base; Fermat witness
    CBigNum e = n - 1;
    CBigNum r;
    BN_mod_exp(&r, &a, &e, &n, pctx);
    if (r == 1)
        return true;
    // Failed Fermat test, calculate fractional length
    unsigned int nFractionalLength = (((n-r) << nFractionalBits) / n).getuint();
    if (nFractionalLength >= (1 << nFractionalBits))
        return error("FermatProbablePrimalityTest() : fractional assert");
    nLength = (nLength & 0xfff00000u) | nFractionalLength;
    return false;
}

// Test probable primality of n = 2p +/- 1 based on Euler, Lagrange and Lifchitz
// fSophieGermain:
//   true:  n = 2p+1, p prime, aka Cunningham Chain of first kind
//   false: n = 2p-1, p prime, aka Cunningham Chain of second kind
// Return values
//   true: n is probable prime
//   false: n is composite; set fractional length in the nLength output
static bool EulerLagrangeLifchitzPrimalityTest(const CBigNum& n, bool fSophieGermain, unsigned int& nLength)
{
    CAutoBN_CTX pctx;
    CBigNum a = 2;
    CBigNum e = (n - 1) >> 1;
    CBigNum r;
    BN_mod_exp(&r, &a, &e, &n, pctx);
    CBigNum nMod8 = n % 8;
    bool fPassedTest = false;
    if (fSophieGermain && (nMod8 == 7)) // Euler & Lagrange
        fPassedTest = (r == 1);
    else if (fSophieGermain && (nMod8 == 3)) // Lifchitz
        fPassedTest = ((r+1) == n);
    else if ((!fSophieGermain) && (nMod8 == 5)) // Lifchitz
        fPassedTest = ((r+1) == n);
    else if ((!fSophieGermain) && (nMod8 == 1)) // LifChitz
        fPassedTest = (r == 1);
    else
        return error("EulerLagrangeLifchitzPrimalityTest() : invalid n %% 8 = %d, %s", nMod8.getint(), (fSophieGermain? "first kind" : "second kind"));

    if (fPassedTest)
        return true;
    // Failed test, calculate fractional length
    r = (r * r) % n; // derive Fermat test remainder
    unsigned int nFractionalLength = (((n-r) << nFractionalBits) / n).getuint();
    if (nFractionalLength >= (1 << nFractionalBits))
        return error("EulerLagrangeLifchitzPrimalityTest() : fractional assert");
    nLength = (nLength & 0xfff00000u) | nFractionalLength;
    return false;
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

static const unsigned int TARGET_MIN_LENGTH = 2;

static unsigned int TargetGetLength(unsigned int nBits)
{
    return ((nBits & 0x3ff00000u) >> nFractionalBits);
}

bool TargetSetLength(unsigned int nLength, unsigned int& nBits)
{
    if (nLength >= 0xff)
        return error("TargetSetLength() : invalid length=%u", nLength);
    nBits &= ~(0x3ff00000u);
    nBits |= (nLength << nFractionalBits);
    return true;
}

void TargetIncrementLength(unsigned int& nBits)
{
    nBits += (1 << nFractionalBits);
}

void TargetDecrementLength(unsigned int& nBits)
{
    if (TargetGetLength(nBits) > TARGET_MIN_LENGTH)
        nBits -= (1 << nFractionalBits);
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
    if (nFractional > 0x00100000u)
        return error("TargetSetFractionalDifficulty() : fractional overflow: nFractionalDifficulty=%016"PRI64x, nFractionalDifficulty);
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
    return (nLength << nFractionalBits);
}

// Get mint value from target
// Primecoin mint rate is determined by target
//   mint = 9999 / (length ** 3)
// Inflation is controlled via Moore's Law
bool TargetGetMint(unsigned int nBits, uint64& nMint)
{
    static uint64 nMintLimit = 9999llu * COIN;
    CBigNum bnMint = nMintLimit;
    if (TargetGetLength(nBits) < TARGET_MIN_LENGTH)
        return error("TargetGetMint() : length below minimum required, nBits=%08x", nBits);
    unsigned int nLengthWithFractional = TargetGetLengthWithFractional(nBits);
    bnMint = (bnMint << nFractionalBits) / nLengthWithFractional;
    bnMint = (bnMint << nFractionalBits) / nLengthWithFractional;
    bnMint = (bnMint << nFractionalBits) / nLengthWithFractional;
    bnMint = (bnMint / CENT) * CENT;  // mint value rounded to cent
    nMint = bnMint.getuint256().Get64();
    return true;
}

// Get next target value
bool TargetGetNext(unsigned int nBits, int64 nInterval, int64 nTargetSpacing, int64 nActualSpacing, unsigned int& nBitsNext)
{
    nBitsNext = nBits;
    // Convert length into fractional difficulty
    uint64 nFractionalDifficulty = TargetGetFractionalDifficulty(nBits);
    // Compute new difficulty via exponential moving toward target spacing
    CBigNum bnFractionalDifficulty = nFractionalDifficulty;
    bnFractionalDifficulty *= ((nInterval + 1) * nTargetSpacing);
    bnFractionalDifficulty /= ((nInterval - 1) * nTargetSpacing + nActualSpacing + nActualSpacing);
    if (bnFractionalDifficulty > nFractionalDifficultyMax)
        bnFractionalDifficulty = nFractionalDifficultyMax;
    if (bnFractionalDifficulty < nFractionalDifficultyMin)
        bnFractionalDifficulty = nFractionalDifficultyMin;
    uint64 nFractionalDifficultyNew = bnFractionalDifficulty.getuint256().Get64();
    // Step up length if fractional past threshold
    if (nFractionalDifficultyNew > nFractionalDifficultyThreshold)
    {
        nFractionalDifficultyNew = nFractionalDifficultyMin;
        TargetIncrementLength(nBitsNext);
    }
    // Step down length if fractional at minimum
    if (nFractionalDifficultyNew == nFractionalDifficultyMin && TargetGetLength(nBits) > TARGET_MIN_LENGTH)
    {
        nFractionalDifficultyNew = nFractionalDifficultyThreshold;
        TargetDecrementLength(nBitsNext);
    }
    // Convert fractional difficulty back to length
    if (!TargetSetFractionalDifficulty(nFractionalDifficultyNew, nBitsNext))
        return error("TargetGetNext() : unable to set fractional difficulty prev=0x%016"PRI64x" new=0x%016"PRI64x, nFractionalDifficulty, nFractionalDifficultyNew);
    return true;
}

// Test Probable Cunningham Chain for: n
// fSophieGermain:
//   true - Test for Cunningham Chain of first kind (n, 2n+1, 4n+3, ...)
//   false - Test for Cunningham Chain of second kind (n, 2n-1, 4n-3, ...)
// Return value:
//   true - Probable Cunningham Chain found (length at least 2)
//   false - Not Cunningham Chain
static bool ProbableCunninghamChainTest(const CBigNum& n, bool fSophieGermain, bool fFermatTest, unsigned int& nProbableChainLength)
{
    nProbableChainLength = 0;
    CBigNum N = n;

    // Fermat test for n first
    if (!FermatProbablePrimalityTest(N, nProbableChainLength))
        return false;

    // Euler-Lagrange-Lifchitz test for the following numbers in chain
    while (true)
    {
        TargetIncrementLength(nProbableChainLength);
        N = N + N + (fSophieGermain? 1 : (-1));
        if (fFermatTest)
        {
            if (!FermatProbablePrimalityTest(N, nProbableChainLength))
                break;
        }
        else
        {
            if (!EulerLagrangeLifchitzPrimalityTest(N, fSophieGermain, nProbableChainLength))
                break;
        }
    }

    FermatProbablePrimalityTest(N, nProbableChainLength);
    return (TargetGetLength(nProbableChainLength) >= 2);
}

// Test probable prime chain for: n
// Return value:
//   true - Probable prime chain found (nProbableChainLength meeting target)
//   false - prime chain too short (nProbableChainLength not meeting target)
bool ProbablePrimeChainTest(const CBigNum& n, unsigned int nBits, bool fFermatTest, unsigned int& nProbableChainLength)
{
    nProbableChainLength = 0;
    bool fSophieGermain= TargetIsSophieGermain(nBits);
    bool fBiTwin = TargetIsBiTwin(nBits);

    if (fBiTwin)
    {
        // BiTwin chain allows a single prime at the end for odd length chain
        unsigned int nLengthFirstKind = 0;
        unsigned int nLengthSecondKind = 0;
        ProbableCunninghamChainTest(n, true, fFermatTest, nLengthFirstKind);
        ProbableCunninghamChainTest(n+2, false, fFermatTest, nLengthSecondKind);
        nProbableChainLength = (TargetGetLength(nLengthFirstKind) > TargetGetLength(nLengthSecondKind))? (nLengthSecondKind + TargetLengthFromInt(TargetGetLength(nLengthSecondKind)+1)) : (nLengthFirstKind + TargetLengthFromInt(TargetGetLength(nLengthFirstKind)));
    }
    else
        ProbableCunninghamChainTest(n, fSophieGermain, fFermatTest, nProbableChainLength);
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
        if (ProbablePrimeChainTest(n, block.nBits, false, nProbableChainLength))
        {
            printf("Probable prime chain of type %s found for block=%s!! \n", TargetGetName(block.nBits).c_str(), block.GetHash().GetHex().c_str());
            block.bnPrimeChainMultiplier = bnPrimorial * bnTried;
            return true;
        }
        else if(TargetGetLength(nProbableChainLength) >= 1)
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
    if (TargetGetLength(nBits) < TARGET_MIN_LENGTH || TargetGetLength(nBits) > 99)
        return error("CheckPrimeProofOfWork() : invalid chain length target %u", TargetGetLength(nBits));

    // Check target for prime proof-of-work
    CBigNum bnProbablePrime = (CBigNum(hash) * bnPrimeChainMultiplier) + ((fSophieGermain || fBiTwin)? (-1) : 1);
    if (bnProbablePrime < bnPrimeMin)
        return error("CheckPrimeProofOfWork() : prime too small");
    // First prime in chain must not exceed cap
    if (bnProbablePrime > bnPrimeMax)
        return error("CheckPrimeProofOfWork() : prime too big");

    // Check prime chain
    unsigned int nChainLength = 0;
    if ((!ProbablePrimeChainTest(bnProbablePrime, nBits, false, nChainLength)) || nChainLength < TargetGetLengthWithFractional(nBits))
        return error("CheckPrimeProofOfWork() : failed prime chain test type=%s nBits=%08x length=%08x", TargetGetName(nBits).c_str(), nBits, nChainLength);

    // Double check prime chain with Fermat tests only
    unsigned int nChainLengthFermatTest = 0;
    if (!ProbablePrimeChainTest(bnProbablePrime, nBits, true, nChainLengthFermatTest))
        return error("CheckPrimeProofOfWork() : failed Fermat test");
    if (nChainLength != nChainLengthFermatTest)
        return error("CheckPrimeProofOfWork() : failed Fermat-only double check type=%s nBits=%08x length=%08x lengthFermat=%08x", TargetGetName(nBits).c_str(), nBits, nChainLength, nChainLengthFermatTest);
    return true;
}

// prime target difficulty value for visualization
double GetPrimeDifficulty(unsigned int nBits)
{
    return ((double) TargetGetLengthWithFractional(nBits) / (double) (1 << nFractionalBits));
}
