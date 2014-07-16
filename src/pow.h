// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POW_H
#define BITCOIN_POW_H

#include "serialize.h"
#include "util.h"

#include <stdint.h>
#include <string>

class CBlockIndex;
class CBlockHeader;
class uint256;

class CProof
{
    uint32_t nTime;
    uint32_t nBits;
    uint32_t nNonce;

    unsigned int GetNextChallenge(const CBlockIndex* pindexLast) const;
public:
    CProof();
    bool CheckSolution(const uint256 hash) const;
    void ResetSolution();
    void ResetChallenge(const CBlockIndex* pindexPrev);
    bool CheckChallenge(const CBlockIndex* pindexPrev) const;
    bool CheckMinChallenge(const CProof& checkpoint) const; 
    void UpdateTime(const CBlockIndex* pindexPrev);
    bool GenerateSolution(CBlockHeader* pblock);
    uint256 GetProofIncrement() const;
    int64_t GetBlockTime() const;
    void SetBlockTime(int64_t nTime);
    bool IsNull() const;
    void SetNull();
    std::string GetChallenge() const;
    std::string GetSolution() const;
    std::string ToString() const;

    IMPLEMENT_SERIALIZE;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action, int nType, int nVersion) {
        READWRITE(nTime);
        READWRITE(nBits);
        READWRITE(nNonce);
    }

    CProof(unsigned int nTime, unsigned int nBits, unsigned int nNonce);
    unsigned int GetChallengeUint() const;
    double GetChallengeDouble() const;
    std::string GetChallengeHex() const;
    uint64_t GetSolutionInt64() const;
    void SetSolutionUint(unsigned int solution);
};

#endif // BITCOIN_POW_H
