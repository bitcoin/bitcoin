// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BLOCKFILTER_H
#define BITCOIN_BLOCKFILTER_H

#include <set>
#include <stdint.h>
#include <vector>

#include <serialize.h>
#include <uint256.h>

/**
 * This implements a Golomb-coded set as defined in BIP 158. It is a
 * compact, probabilistic data structure for testing set membership.
 */
class GCSFilter
{
public:
    typedef std::vector<unsigned char> Element;
    typedef std::set<Element> ElementSet;

private:
    uint64_t m_siphash_k0;
    uint64_t m_siphash_k1;
    uint8_t m_P;  //!< Golomb-Rice coding parameter
    uint32_t m_M;  //!< Inverse false positive rate
    uint32_t m_N;  //!< Number of elements in the filter
    uint64_t m_F;  //!< Range of element hashes, F = N * M
    std::vector<unsigned char> m_encoded;

    /** Hash a data element to an integer in the range [0, N * M). */
    uint64_t HashToRange(const Element& element) const;

    std::vector<uint64_t> BuildHashedSet(const ElementSet& elements) const;

public:

    /** Constructs an empty filter. */
    GCSFilter(uint64_t siphash_k0 = 0, uint64_t siphash_k1 = 0, uint8_t P = 0, uint32_t M = 0);

    /** Reconstructs an already-created filter from an encoding. */
    GCSFilter(uint64_t siphash_k0, uint64_t siphash_k1, uint8_t P, uint32_t M,
              std::vector<unsigned char> encoded_filter);

    /** Builds a new filter from the params and set of elements. */
    GCSFilter(uint64_t siphash_k0, uint64_t siphash_k1, uint8_t P, uint32_t M,
              const ElementSet& elements);

    uint8_t GetP() const { return m_P; }
    uint32_t GetN() const { return m_N; }
    uint32_t GetM() const { return m_M; }
    const std::vector<unsigned char>& GetEncoded() const { return m_encoded; }

    /**
     * Checks if the element may be in the set. False positives are possible
     * with probability 1/M.
     */
    bool Match(const Element& element) const;

    /**
     * Checks if any of the given elements may be in the set. False positives
     * are possible with probability 1/M per element checked. This is more
     * efficient that checking Match on multiple elements separately.
     */
    bool MatchAny(const ElementSet& elements) const;
};

#endif // BITCOIN_BLOCKFILTER_H
