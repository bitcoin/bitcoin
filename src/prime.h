// Copyright (c) 2013 Primecoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PRIMECOIN_PRIME_H
#define PRIMECOIN_PRIME_H

#include "main.h"

static const CBigNum bnOne = 1;
static const CBigNum bnPrimeMax = (bnOne << 2039) - 1;
static const CBigNum bnPrimeMin = (bnOne << 256);
static const unsigned int nMaxSieveSize = 1000000u;

// Generate small prime table
void GeneratePrimeTable();

// Compute primorial number p#
void Primorial(unsigned int p, CBigNum& bnPrimorial);
// Compute the first primorial number greater than or equal to bn
void PrimorialAt(CBigNum& bn, CBigNum& bnPrimorial);

// Test probable prime chain for: n
// fFermatTest
//   true - Use only Fermat tests
//   false - Use Fermat-Euler-Lagrange-Lifchitz tests
// Return value:
//   true - Probable prime chain found (nProbableChainLength meeting target)
//   false - prime chain too short (nProbableChainLength not meeting target)
bool ProbablePrimeChainTest(const CBigNum& n, unsigned int nBits, bool fFermatTest, unsigned int& nProbableChainLength);

static const unsigned int nFractionalBits = 20;
static const uint64 nFractionalDifficultyMax = (1llu << (nFractionalBits + 32));
static const uint64 nFractionalDifficultyMin = (1llu << 32);
static const uint64 nFractionalDifficultyThreshold = (1llu << (10 + 32));
bool TargetSetLength(unsigned int nLength, unsigned int& nBits);
unsigned int TargetGetFractional(unsigned int nBits);
uint64 TargetGetFractionalDifficulty(unsigned int nBits);
bool TargetSetFractionalDifficulty(uint64 nFractionalDifficulty, unsigned int& nBits);
bool TargetIsSophieGermain(unsigned int nBits);
void TargetSetSophieGermain(bool fSophieGermain, unsigned int& nBits);
bool TargetIsBiTwin(unsigned int nBits);
void TargetSetBiTwin(bool fBiTwin, unsigned int& nBits);
std::string TargetGetName(unsigned int nBits);
bool TargetGetMint(unsigned int nBits, uint64& nMint);
bool TargetGetNext(unsigned int nBits, int64 nInterval, int64 nTargetSpacing, int64 nActualSpacing, unsigned int& nBitsNext);

// Mine probable prime chain of form: n = h * p# +/- 1
bool MineProbablePrimeChain(CBlock& block, CBigNum& bnFixedMultiplier, bool& fNewBlock, unsigned int& nTriedMultiplier, unsigned int& nProbableChainLength, unsigned int& nTests, unsigned int& nPrimesHit);

// Find last block index up to pindex of the given prime chain type
bool GetLastBlockIndex(const CBlockIndex* pindex, bool fSophieGermain, bool fBiTwin, const CBlockIndex** pindexPrev);

// Size of a big number (in bits), times 65536
// Can be used as an approximate log scale for numbers up to 2 ** 65536 - 1
bool LogScale(const CBigNum& bn, unsigned int& nLogScale);

// Check prime proof-of-work
bool CheckPrimeProofOfWork(uint256 hashBlockHeader, unsigned int nBits, const CBigNum& bnPrimeChainMultiplier);

// prime target difficulty value for visualization
double GetPrimeDifficulty(unsigned int nBits);

// Sieve of Eratosthenes for proof-of-work mining
class CSieveOfEratosthenes
{
    unsigned int nSieveSize; // size of the sieve
    unsigned int nBits; // target of the prime chain to search for
    uint256 hashBlockHeader; // block header hash
    CBigNum bnFixedFactor; // fixed factor to derive the chain
    bool fSophieGermain; // is it for Cunningham Chain of first kind
    bool fBiTwin; // is it for BiTwin Chain

    // bitmap of the sieve, index represents the variable part of multiplier
    std::vector<bool> vfCompositeInChain;

    unsigned int nPrimeSeq; // prime sequence number currently being processed
    unsigned int nCandidateMultiplier; // current candidate for power test
    unsigned int nCandidates; // number of candidates for power tests

public:
    CSieveOfEratosthenes(unsigned int nSieveSize, unsigned int nBits, uint256 hashBlockHeader, CBigNum& bnFixedMultiplier)
    {
        this->nSieveSize = nSieveSize;
        this->nBits = nBits;
        this->hashBlockHeader = hashBlockHeader;
        this->bnFixedFactor = bnFixedMultiplier * CBigNum(hashBlockHeader);
        nPrimeSeq = 0;
        fSophieGermain= TargetIsSophieGermain(nBits);
        fBiTwin = TargetIsBiTwin(nBits);
        vfCompositeInChain = std::vector<bool> (1000000, false);
        nCandidateMultiplier = 0;
        nCandidates = nSieveSize;
    }

    // Get total number of candidates for power test
    unsigned int GetCandidateCount()
    {
        return nCandidates;
    }

    // Scan for the next candidate multiplier (variable part)
    // Return values:
    //   True - found next candidate; nVariableMultiplier has the candidate
    //   False - scan complete, no more candidate and reset scan
    bool GetNextCandidateMultiplier(unsigned int& nVariableMultiplier)
    {
        while (true)
        {
            nCandidateMultiplier++;
            if (nCandidateMultiplier >= nSieveSize)
            {
                nCandidateMultiplier = 0;
                return false;
            }
            if (!vfCompositeInChain[nCandidateMultiplier])
            {
                nVariableMultiplier = nCandidateMultiplier;
                return true;
            }
        }
    }

    // Weave the sieve for the next prime in table
    // Return values:
    //   True  - weaved another prime; nComposite - number of composites removed
    //   False - sieve already completed
    bool Weave(unsigned int& nComposite);
};

#endif
