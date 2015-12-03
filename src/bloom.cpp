// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bloom.h"

#include "primitives/transaction.h"
#include "hash.h"
#include "script/script.h"
#include "script/standard.h"
#include "random.h"
#include "streams.h"

#include <math.h>
#include <stdlib.h>

#include <boost/foreach.hpp>

#define LN2SQUARED 0.4804530139182014246671025263266649717305529515945455
#define LN2 0.6931471805599453094172321214581765680755001343602552

using namespace std;

CBloomFilter::CBloomFilter(unsigned int nElements, double nFPRate, unsigned int nTweakIn, unsigned char nFlagsIn) :
    /**
     * The ideal size for a bloom filter with a given number of elements and false positive rate is:
     * - nElements * log(fp rate) / ln(2)^2
     * We ignore filter parameters which will create a bloom filter larger than the protocol limits
     */
    vData(min((unsigned int)(-1  / LN2SQUARED * nElements * log(nFPRate)), MAX_BLOOM_FILTER_SIZE * 8) / 8),
    /**
     * The ideal number of hash functions is filter size * ln(2) / number of elements
     * Again, we ignore filter parameters which will create a bloom filter with more hash functions than the protocol limits
     * See https://en.wikipedia.org/wiki/Bloom_filter for an explanation of these formulas
     */
    isFull(false),
    isEmpty(true),
    nHashFuncs(min((unsigned int)(vData.size() * 8 / nElements * LN2), MAX_HASH_FUNCS)),
    nTweak(nTweakIn),
    nFlags(nFlagsIn)
{
}

// Private constructor used by CRollingBloomFilter
CBloomFilter::CBloomFilter(unsigned int nElements, double nFPRate, unsigned int nTweakIn) :
    vData((unsigned int)(-1  / LN2SQUARED * nElements * log(nFPRate)) / 8),
    isFull(false),
    isEmpty(true),
    nHashFuncs((unsigned int)(vData.size() * 8 / nElements * LN2)),
    nTweak(nTweakIn),
    nFlags(BLOOM_UPDATE_NONE)
{
}

inline unsigned int CBloomFilter::Hash(unsigned int nHashNum, const std::vector<unsigned char>& vDataToHash) const
{
    // 0xFBA4C795 chosen as it guarantees a reasonable bit difference between nHashNum values.
    return MurmurHash3(nHashNum * 0xFBA4C795 + nTweak, vDataToHash) % (vData.size() * 8);
}

void CBloomFilter::insert(const vector<unsigned char>& vKey)
{
    if (isFull)
        return;
    for (unsigned int i = 0; i < nHashFuncs; i++)
    {
        unsigned int nIndex = Hash(i, vKey);
        // Sets bit nIndex of vData
        vData[nIndex >> 3] |= (1 << (7 & nIndex));
    }
    isEmpty = false;
}

void CBloomFilter::insert(const COutPoint& outpoint)
{
    CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
    stream << outpoint;
    vector<unsigned char> data(stream.begin(), stream.end());
    insert(data);
}

void CBloomFilter::insert(const uint256& hash)
{
    vector<unsigned char> data(hash.begin(), hash.end());
    insert(data);
}

bool CBloomFilter::contains(const vector<unsigned char>& vKey) const
{
    if (isFull)
        return true;
    if (isEmpty)
        return false;
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
    vector<unsigned char> data(stream.begin(), stream.end());
    return contains(data);
}

bool CBloomFilter::contains(const uint256& hash) const
{
    vector<unsigned char> data(hash.begin(), hash.end());
    return contains(data);
}

void CBloomFilter::clear()
{
    vData.assign(vData.size(),0);
    isFull = false;
    isEmpty = true;
}

void CBloomFilter::reset(unsigned int nNewTweak)
{
    clear();
    nTweak = nNewTweak;
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
    if (isFull)
        return true;
    if (isEmpty)
        return false;
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
        vector<unsigned char> data;
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
                    txnouttype type;
                    vector<vector<unsigned char> > vSolutions;
                    if (Solver(txout.scriptPubKey, type, vSolutions) &&
                            (type == TX_PUBKEY || type == TX_MULTISIG))
                        insert(COutPoint(hash, i));
                }
                break;
            }
        }
    }

    if (fFound)
        return true;

    BOOST_FOREACH(const CTxIn& txin, tx.vin)
    {
        // Match if the filter contains an outpoint tx spends
        if (contains(txin.prevout))
            return true;

        // Match if the filter contains any arbitrary script data element in any scriptSig in tx
        CScript::const_iterator pc = txin.scriptSig.begin();
        vector<unsigned char> data;
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

void CBloomFilter::UpdateEmptyFull()
{
    bool full = true;
    bool empty = true;
    for (unsigned int i = 0; i < vData.size(); i++)
    {
        full &= vData[i] == 0xff;
        empty &= vData[i] == 0;
    }
    isFull = full;
    isEmpty = empty;
}

CRollingBloomFilter::CRollingBloomFilter(unsigned int nElements, double fpRate)
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
    /* We store up to 16 'bits' per data element. */
    data.resize((nFilterBits + 15) / 16);
    reset();
}

/* Similar to CBloomFilter::Hash */
inline unsigned int CRollingBloomFilter::Hash(unsigned int nHashNum, const std::vector<unsigned char>& vDataToHash) const {
    return MurmurHash3(nHashNum * 0xFBA4C795 + nTweak, vDataToHash) % (data.size() * 16);
}

void CRollingBloomFilter::insert(const std::vector<unsigned char>& vKey)
{
    if (nEntriesThisGeneration == nEntriesPerGeneration) {
        nEntriesThisGeneration = 0;
        nGeneration++;
        if (nGeneration == 4) {
            nGeneration = 1;
        }
        /* Wipe old entries that used this generation number. */
        for (uint32_t p = 0; p < data.size() * 16; p++) {
            if (get(p) == nGeneration) {
                put(p, 0);
            }
        }
    }
    nEntriesThisGeneration++;

    for (int n = 0; n < nHashFuncs; n++) {
        uint32_t h = Hash(n, vKey);
        put(h, nGeneration);
    }
}

void CRollingBloomFilter::insert(const uint256& hash)
{
    vector<unsigned char> data(hash.begin(), hash.end());
    insert(data);
}

bool CRollingBloomFilter::contains(const std::vector<unsigned char>& vKey) const
{
    for (int n = 0; n < nHashFuncs; n++) {
        uint32_t h = Hash(n, vKey);
        if (get(h) == 0) {
            return false;
        }
    }
    return true;
}

bool CRollingBloomFilter::contains(const uint256& hash) const
{
    vector<unsigned char> data(hash.begin(), hash.end());
    return contains(data);
}

void CRollingBloomFilter::reset()
{
    nTweak = GetRand(std::numeric_limits<unsigned int>::max());
    nEntriesThisGeneration = 0;
    nGeneration = 1;
    for (std::vector<uint32_t>::iterator it = data.begin(); it != data.end(); it++) {
        *it = 0;
    }
}
