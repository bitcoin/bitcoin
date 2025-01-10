// Copyright (c) 2012-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/bloom.h>

#include <hash.h>
#include <primitives/transaction.h>
#include <random.h>
#include <script/script.h>
#include <script/solver.h>
#include <span.h>
#include <streams.h>
#include <util/fastrange.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <vector>

static constexpr double LN2SQUARED = 0.4804530139182014246671025263266649717305529515945455;
static constexpr double LN2 = 0.6931471805599453094172321214581765680755001343602552;

CBloomFilter::CBloomFilter(const unsigned int nElements, const double nFPRate, const unsigned int nTweakIn, unsigned char nFlagsIn) :
    vData(std::min((unsigned int)(-1  / LN2SQUARED * nElements * log(nFPRate)), MAX_BLOOM_FILTER_SIZE * 8) / 8),
    nHashFuncs(std::min((unsigned int)(vData.size() * 8 / nElements * LN2), MAX_HASH_FUNCS)),
    nTweak(nTweakIn),
    nFlags(nFlagsIn)
{
}

inline unsigned int CBloomFilter::Hash(unsigned int nHashNum, Span<const unsigned char> vDataToHash) const
{
    // 0xFBA4C795 chosen as it guarantees a reasonable bit difference between nHashNum values.
    return MurmurHash3(nHashNum * 0xFBA4C795 + nTweak, vDataToHash) % (vData.size() * 8);
}

void CBloomFilter::insert(Span<const unsigned char> vKey)
{
    if (vData.empty()) // Avoid divide-by-zero (CVE-2013-5700)
        return;

    unsigned int dataSizeInBits = vData.size() * 8;  // Precompute the size for hash functions
    for (unsigned int i = 0; i < nHashFuncs; i++)
    {
        unsigned int nIndex = Hash(i, vKey);
        vData[nIndex >> 3] |= (1 << (7 & nIndex));
    }
}

void CBloomFilter::insert(const COutPoint& outpoint)
{
    DataStream stream{};
    stream << outpoint;
    insert(MakeUCharSpan(stream));
}

bool CBloomFilter::contains(Span<const unsigned char> vKey) const
{
    if (vData.empty()) // Avoid divide-by-zero (CVE-2013-5700)
        return true;
        
    unsigned int dataSizeInBits = vData.size() * 8;  // Precompute the size for hash functions
    for (unsigned int i = 0; i < nHashFuncs; i++)
    {
        unsigned int nIndex = Hash(i, vKey);
        if (!(vData[nIndex >> 3] & (1 << (7 & nIndex))))
            return false;
    }
    return true;
}

bool CBloomFilter::contains(const COutPoint& outpoint) const
{
    DataStream stream{};
    stream << outpoint;
    return contains(MakeUCharSpan(stream));
}

bool CBloomFilter::IsWithinSizeConstraints() const
{
    return vData.size() <= MAX_BLOOM_FILTER_SIZE && nHashFuncs <= MAX_HASH_FUNCS;
}

bool CBloomFilter::IsRelevantAndUpdate(const CTransaction& tx)
{
    bool fFound = false;
    
    if (vData.empty()) // zero-size = "match-all" filter
        return true;
        
    const Txid& hash = tx.GetHash();
    if (contains(hash.ToUint256()))
        fFound = true;

    for (unsigned int i = 0; i < tx.vout.size(); i++)
    {
        const CTxOut& txout = tx.vout[i];
        CScript::const_iterator pc = txout.scriptPubKey.begin();
        std::vector<unsigned char> data;
        while (pc < txout.scriptPubKey.end())
        {
            opcodetype opcode;
            if (!txout.scriptPubKey.GetOp(pc, opcode, data))
                break;
            if (data.size() != 0 && contains(data))
            {
                fFound = true;
                if ((nFlags & BLOOM_UPDATE_MASK) == BLOOM_UPDATE_ALL)
                    insert(COutPoint(hash, i));
                else if ((nFlags & BLOOM_UPDATE_MASK) == BLOOM_UPDATE_P2PUBKEY_ONLY)
                {
                    std::vector<std::vector<unsigned char>> vSolutions;
                    TxoutType type = Solver(txout.scriptPubKey, vSolutions);
                    if (type == TxoutType::PUBKEY || type == TxoutType::MULTISIG)
                    {
                        insert(COutPoint(hash, i));
                    }
                }
                break;
            }
        }
    }

    if (fFound)
        return true;

    for (const CTxIn& txin : tx.vin)
    {
        if (contains(txin.prevout))
            return true;

        CScript::const_iterator pc = txin.scriptSig.begin();
        std::vector<unsigned char> data;
        while (pc < txin.scriptSig.end())
        {
            opcodetype opcode;
            if (!txin.scriptSig.GetOp(pc, opcode, data))
                break;
            if (data.size() != 0 && contains(data))
                return true;
        }
    }

    return false;
}

CRollingBloomFilter::CRollingBloomFilter(const unsigned int nElements, const double fpRate)
{
    double logFpRate = log(fpRate);
    nHashFuncs = std::max(1, std::min((int)round(logFpRate / log(0.5)), 50));
    nEntriesPerGeneration = (nElements + 1) / 2;
    uint32_t nMaxElements = nEntriesPerGeneration * 3;
    uint32_t nFilterBits = (uint32_t)ceil(-1.0 * nHashFuncs * nMaxElements / log(1.0 - exp(logFpRate / nHashFuncs)));
    
    data.clear();
    data.resize(((nFilterBits + 63) / 64) << 1);  // Reserve space for the bitfields
    reset();
}

static inline uint32_t RollingBloomHash(unsigned int nHashNum, uint32_t nTweak, Span<const unsigned char> vDataToHash)
{
    return MurmurHash3(nHashNum * 0xFBA4C795 + nTweak, vDataToHash);
}

void CRollingBloomFilter::insert(Span<const unsigned char> vKey)
{
    if (nEntriesThisGeneration == nEntriesPerGeneration)
    {
        nEntriesThisGeneration = 0;
        nGeneration++;
        if (nGeneration == 4)
            nGeneration = 1;
        
        uint64_t nGenerationMask1 = 0 - (uint64_t)(nGeneration & 1);
        uint64_t nGenerationMask2 = 0 - (uint64_t)(nGeneration >> 1);
        
        // Wipe old entries that used this generation number
        for (uint32_t p = 0; p < data.size(); p += 2)
        {
            uint64_t p1 = data[p], p2 = data[p + 1];
            uint64_t mask = (p1 ^ nGenerationMask1) | (p2 ^ nGenerationMask2);
            data[p] = p1 & mask;
            data[p + 1] = p2 & mask;
        }
    }
    nEntriesThisGeneration++;

    for (int n = 0; n < nHashFuncs; n++)
    {
        uint32_t h = RollingBloomHash(n, nTweak, vKey);
        int bit = h & 0x3F;
        uint32_t pos = FastRange32(h, data.size());
        data[pos & ~1U] = (data[pos & ~1U] & ~(uint64_t{1} << bit)) | (uint64_t(nGeneration & 1)) << bit;
        data[pos | 1] = (data[pos | 1] & ~(uint64_t{1} << bit)) | (uint64_t(nGeneration >> 1)) << bit;
    }
}

bool CRollingBloomFilter::contains(Span<const unsigned char> vKey) const
{
    for (int n = 0; n < nHashFuncs; n++)
    {
        uint32_t h = RollingBloomHash(n, nTweak, vKey);
        int bit = h & 0x3F;
        uint32_t pos = FastRange32(h, data.size());
        if (!(((data[pos & ~1U] | data[pos | 1]) >> bit) & 1))
        {
            return false;
        }
    }
    return true;
}

void CRollingBloomFilter::reset()
{
    nTweak = FastRandomContext().rand<unsigned int>();
    nEntriesThisGeneration = 0;
    nGeneration = 1;
    std::fill(data.begin(), data.end(), 0);
}
