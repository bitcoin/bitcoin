// Copyright (c) 2012-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bloom.h>

#include <primitives/transaction.h>
#include <hash.h>
#include <script/script.h>
#include <script/standard.h>
#include <random.h>
#include <streams.h>

#include <math.h>
#include <stdlib.h>

#include <algorithm>

static constexpr double LN2SQUARED = 0.4804530139182014246671025263266649717305529515945455;
static constexpr double LN2 = 0.6931471805599453094172321214581765680755001343602552;

CBloomFilter::CBloomFilter(const unsigned int nElements, const double nFPRate, const unsigned int nTweakIn, unsigned char nFlagsIn) :
    /**
     * The ideal size for a bloom filter with a given number of elements and false positive rate is:
     * - nElements * log(fp rate) / ln(2)^2
     * We ignore filter parameters which will create a bloom filter larger than the protocol limits
     */
    vData(std::min((unsigned int)(-1  / LN2SQUARED * nElements * log(nFPRate)), MAX_BLOOM_FILTER_SIZE * 8) / 8),
    /**
     * The ideal number of hash functions is filter size * ln(2) / number of elements
     * Again, we ignore filter parameters which will create a bloom filter with more hash functions than the protocol limits
     * See https://en.wikipedia.org/wiki/Bloom_filter for an explanation of these formulas
     */
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
    for (unsigned int i = 0; i < nHashFuncs; i++)
    {
        unsigned int nIndex = Hash(i, vKey);
        // Sets bit nIndex of vData
        vData[nIndex >> 3] |= (1 << (7 & nIndex));
    }
}

void CBloomFilter::insert(const COutPoint& outpoint)
{
    CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
    stream << outpoint;
    insert(stream);
}

bool CBloomFilter::contains(Span<const unsigned char> vKey) const
{
    if (vData.empty()) // Avoid divide-by-zero (CVE-2013-5700)
        return true;
    for (unsigned int i = 0; i < nHashFuncs; i++)
    {
        unsigned int nIndex = Hash(i, vKey);
        // Checks bit nIndex of vData
        if (!(vData[nIndex >> 3] & (1 << (7 & nIndex))))
            return false;
    }
    return true;
}

bool CBloomFilter::contains(const COutPoint& outpoint) const
{
    CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
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
    // Match if the filter contains the hash of tx
    //  for finding tx when they appear in a block
    if (vData.empty()) // zero-size = "match-all" filter
        return true;
    const uint256& hash = tx.GetHash();
    if (contains(hash))
        fFound = true;

    for (unsigned int i = 0; i < tx.vout.size(); i++)
    {
        const CTxOut& txout = tx.vout[i];
        // Match if the filter contains any arbitrary script data element in any scriptPubKey in tx
        // If this matches, also add the specific output that was matched.
        // This means clients don't have to update the filter themselves when a new relevant tx
        // is discovered in order to find spending transactions, which avoids round-tripping and race conditions.
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
                    std::vector<std::vector<unsigned char> > vSolutions;
                    TxoutType type = Solver(txout.scriptPubKey, vSolutions);
                    if (type == TxoutType::PUBKEY || type == TxoutType::MULTISIG) {
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
        // Match if the filter contains an outpoint tx spends
        if (contains(txin.prevout))
            return true;

        // Match if the filter contains any arbitrary script data element in any scriptSig in tx
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
    /* The optimal number of hash functions is log(fpRate) / log(0.5), but
     * restrict it to the range 1-50. */
    nHashFuncs = std::max(1, std::min((int)round(logFpRate / log(0.5)), 50));
    /* In this rolling bloom filter, we'll store between 2 and 3 generations of nElements / 2 entries. */
    nEntriesPerGeneration = (nElements + 1) / 2;
    uint32_t nMaxElements = nEntriesPerGeneration * 3;
    /* The maximum fpRate = pow(1.0 - exp(-nHashFuncs * nMaxElements / nFilterBits), nHashFuncs)
     * =>          pow(fpRate, 1.0 / nHashFuncs) = 1.0 - exp(-nHashFuncs * nMaxElements / nFilterBits)
     * =>          1.0 - pow(fpRate, 1.0 / nHashFuncs) = exp(-nHashFuncs * nMaxElements / nFilterBits)
     * =>          log(1.0 - pow(fpRate, 1.0 / nHashFuncs)) = -nHashFuncs * nMaxElements / nFilterBits
     * =>          nFilterBits = -nHashFuncs * nMaxElements / log(1.0 - pow(fpRate, 1.0 / nHashFuncs))
     * =>          nFilterBits = -nHashFuncs * nMaxElements / log(1.0 - exp(logFpRate / nHashFuncs))
     */
    uint32_t nFilterBits = (uint32_t)ceil(-1.0 * nHashFuncs * nMaxElements / log(1.0 - exp(logFpRate / nHashFuncs)));
    data.clear();
    /* For each data element we need to store 2 bits. If both bits are 0, the
     * bit is treated as unset. If the bits are (01), (10), or (11), the bit is
     * treated as set in generation 1, 2, or 3 respectively.
     * These bits are stored in separate integers: position P corresponds to bit
     * (P & 63) of the integers data[(P >> 6) * 2] and data[(P >> 6) * 2 + 1]. */
    data.resize(((nFilterBits + 63) / 64) << 1);
    reset();
}

/* Similar to CBloomFilter::Hash */
static inline uint32_t RollingBloomHash(unsigned int nHashNum, uint32_t nTweak, Span<const unsigned char> vDataToHash)
{
    return MurmurHash3(nHashNum * 0xFBA4C795 + nTweak, vDataToHash);
}


// A replacement for x % n. This assumes that x and n are 32bit integers, and x is a uniformly random distributed 32bit value
// which should be the case for a good hash.
// See https://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction/
static inline uint32_t FastMod(uint32_t x, size_t n) {
    return ((uint64_t)x * (uint64_t)n) >> 32;
}

void CRollingBloomFilter::insert(Span<const unsigned char> vKey)
{
    if (nEntriesThisGeneration == nEntriesPerGeneration) {
        nEntriesThisGeneration = 0;
        nGeneration++;
        if (nGeneration == 4) {
            nGeneration = 1;
        }
        uint64_t nGenerationMask1 = 0 - (uint64_t)(nGeneration & 1);
        uint64_t nGenerationMask2 = 0 - (uint64_t)(nGeneration >> 1);
        /* Wipe old entries that used this generation number. */
        for (uint32_t p = 0; p < data.size(); p += 2) {
            uint64_t p1 = data[p], p2 = data[p + 1];
            uint64_t mask = (p1 ^ nGenerationMask1) | (p2 ^ nGenerationMask2);
            data[p] = p1 & mask;
            data[p + 1] = p2 & mask;
        }
    }
    nEntriesThisGeneration++;

    for (int n = 0; n < nHashFuncs; n++) {
        uint32_t h = RollingBloomHash(n, nTweak, vKey);
        int bit = h & 0x3F;
        /* FastMod works with the upper bits of h, so it is safe to ignore that the lower bits of h are already used for bit. */
        uint32_t pos = FastMod(h, data.size());
        /* The lowest bit of pos is ignored, and set to zero for the first bit, and to one for the second. */
        data[pos & ~1] = (data[pos & ~1] & ~(((uint64_t)1) << bit)) | ((uint64_t)(nGeneration & 1)) << bit;
        data[pos | 1] = (data[pos | 1] & ~(((uint64_t)1) << bit)) | ((uint64_t)(nGeneration >> 1)) << bit;
    }
}

bool CRollingBloomFilter::contains(Span<const unsigned char> vKey) const
{
    for (int n = 0; n < nHashFuncs; n++) {
        uint32_t h = RollingBloomHash(n, nTweak, vKey);
        int bit = h & 0x3F;
        uint32_t pos = FastMod(h, data.size());
        /* If the relevant bit is not set in either data[pos & ~1] or data[pos | 1], the filter does not contain vKey */
        if (!(((data[pos & ~1] | data[pos | 1]) >> bit) & 1)) {
            return false;
        }
    }
    return true;
}

void CRollingBloomFilter::reset()
{
    nTweak = GetRand(std::numeric_limits<unsigned int>::max());
    nEntriesThisGeneration = 0;
    nGeneration = 1;
    std::fill(data.begin(), data.end(), 0);
}
