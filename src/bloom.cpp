// Copyright (c) 2012-2015 The Bitcoin Core developers
// Copyright (c) 2015-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bloom.h"

#include "hash.h"
#include "primitives/transaction.h"
#include "random.h"
#include "script/script.h"
#include "script/standard.h"
#include "streams.h"
#include "util.h"

#include <math.h>
#include <stdlib.h>

#include <boost/foreach.hpp>

#define LN2SQUARED 0.4804530139182014246671025263266649717305529515945455
#define LN2 0.6931471805599453094172321214581765680755001343602552

using namespace std;

void CBloomFilter::setup(unsigned int nElements,
    double nFPRate,
    unsigned int nTweakIn,
    unsigned char nFlagsIn,
    bool size_constrained,
    uint32_t nMaxFilterSize = SMALLEST_MAX_BLOOM_FILTER_SIZE)
{
    if (nElements == 0)
    {
        LogPrintf("Construction of empty CBloomFilter attempted.\n");
        nElements = 1;
    }
    unsigned desired_vdata_size = (unsigned int)(-1 / LN2SQUARED * nElements * log(nFPRate) / 8);

    if (size_constrained)
        desired_vdata_size = min(desired_vdata_size, nMaxFilterSize);

    vData.resize(desired_vdata_size, 0);
    isFull = vData.size() == 0;
    isEmpty = true;

    nHashFuncs = (unsigned int)(vData.size() * 8 / nElements * LN2);

    if (size_constrained)
        nHashFuncs = min(nHashFuncs, MAX_HASH_FUNCS);

    nTweak = nTweakIn;
    nFlags = nFlagsIn;
}

CBloomFilter::CBloomFilter(unsigned int nElements,
    double nFPRate,
    unsigned int nTweakIn,
    unsigned char nFlagsIn,
    uint32_t nMaxFilterSize)
{
    setup(nElements, nFPRate, nTweakIn, nFlagsIn, true, nMaxFilterSize);
}

// Private constructor used by CRollingBloomFilter
CBloomFilter::CBloomFilter(unsigned int nElements, double nFPRate, unsigned int nTweakIn)
{
    setup(nElements, nFPRate, nTweakIn, BLOOM_UPDATE_NONE, false);
}

inline unsigned int CBloomFilter::Hash(unsigned int nHashNum, const std::vector<unsigned char> &vDataToHash) const
{
    // 0xFBA4C795 chosen as it guarantees a reasonable bit difference between nHashNum values.
    return MurmurHash3(nHashNum * 0xFBA4C795 + nTweak, vDataToHash) % (vData.size() * 8);
}

void CBloomFilter::insert(const vector<unsigned char> &vKey)
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

void CBloomFilter::insert(const COutPoint &outpoint)
{
    CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
    stream << outpoint;
    vector<unsigned char> data(stream.begin(), stream.end());
    insert(data);
}

void CBloomFilter::insert(const uint256 &hash)
{
    vector<unsigned char> data(hash.begin(), hash.end());
    insert(data);
}

bool CBloomFilter::contains(const vector<unsigned char> &vKey) const
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

bool CBloomFilter::contains(const COutPoint &outpoint) const
{
    CDataStream stream(SER_NETWORK, PROTOCOL_VERSION);
    stream << outpoint;
    vector<unsigned char> data(stream.begin(), stream.end());
    return contains(data);
}

bool CBloomFilter::contains(const uint256 &hash) const
{
    vector<unsigned char> data(hash.begin(), hash.end());
    return contains(data);
}

void CBloomFilter::clear()
{
    vData.assign(vData.size(), 0);
    isFull = vData.size() == 0;
    isEmpty = true;
}

void CBloomFilter::reset(unsigned int nNewTweak)
{
    clear();
    nTweak = nNewTweak;
}

bool CBloomFilter::IsWithinSizeConstraints() const
{
    return vData.size() <= SMALLEST_MAX_BLOOM_FILTER_SIZE && nHashFuncs <= MAX_HASH_FUNCS;
}

bool CBloomFilter::IsRelevantAndUpdate(const CTransaction &tx)
{
    bool fFound = false;
    // Match if the filter contains the hash of tx
    //  for finding tx when they appear in a block
    if (isFull)
        return true;
    if (isEmpty)
        return false;
    const uint256 &hash = tx.GetHash();
    if (contains(hash))
        fFound = true;

    for (unsigned int i = 0; i < tx.vout.size(); i++)
    {
        const CTxOut &txout = tx.vout[i];
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
                        (type == TX_PUBKEY || type == TX_MULTISIG || type == TX_CLTV))
                        insert(COutPoint(hash, i));
                }
                break;
            }
        }
    }

    if (fFound)
        return true;

    BOOST_FOREACH (const CTxIn &txin, tx.vin)
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
    : b1(nElements * 2, fpRate, 0), b2(nElements * 2, fpRate, 0)
{
    // Implemented using two bloom filters of 2 * nElements each.
    // We fill them up, and clear them, staggered, every nElements
    // inserted, so at least one always contains the last nElements
    // inserted.
    nInsertions = 0;
    nBloomSize = nElements * 2;

    reset();
}

void CRollingBloomFilter::insert(const std::vector<unsigned char> &vKey)
{
    if (nInsertions == 0)
    {
        b1.clear();
    }
    else if (nInsertions == nBloomSize / 2)
    {
        b2.clear();
    }
    b1.insert(vKey);
    b2.insert(vKey);
    if (++nInsertions == nBloomSize)
    {
        nInsertions = 0;
    }
}

void CRollingBloomFilter::insert(const uint256 &hash)
{
    vector<unsigned char> data(hash.begin(), hash.end());
    insert(data);
}

bool CRollingBloomFilter::contains(const std::vector<unsigned char> &vKey) const
{
    if (nInsertions < nBloomSize / 2)
    {
        return b2.contains(vKey);
    }
    return b1.contains(vKey);
}

bool CRollingBloomFilter::contains(const uint256 &hash) const
{
    vector<unsigned char> data(hash.begin(), hash.end());
    return contains(data);
}

void CRollingBloomFilter::reset()
{
    unsigned int nNewTweak = GetRand(std::numeric_limits<unsigned int>::max());
    b1.reset(nNewTweak);
    b2.reset(nNewTweak);
    nInsertions = 0;
}
