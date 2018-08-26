// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BLOCKFILTER_H
#define BITCOIN_BLOCKFILTER_H

#include <set>
#include <stdint.h>
#include <vector>

#include <primitives/block.h>
#include <serialize.h>
#include <uint256.h>
#include <undo.h>

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

    /** Helper method used to implement Match and MatchAny */
    bool MatchInternal(const uint64_t* sorted_element_hashes, size_t size) const;

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

constexpr uint8_t BASIC_FILTER_P = 19;
constexpr uint32_t BASIC_FILTER_M = 784931;

enum BlockFilterType : uint8_t
{
    BASIC = 0,
};

/**
 * Complete block filter struct as defined in BIP 157. Serialization matches
 * payload of "cfilter" messages.
 */
class BlockFilter
{
private:
    BlockFilterType m_filter_type;
    uint256 m_block_hash;
    GCSFilter m_filter;

public:

    // Construct a new BlockFilter of the specified type from a block.
    BlockFilter(BlockFilterType filter_type, const CBlock& block, const CBlockUndo& block_undo);

    BlockFilterType GetFilterType() const { return m_filter_type; }

    const GCSFilter& GetFilter() const { return m_filter; }

    const std::vector<unsigned char>& GetEncodedFilter() const
    {
        return m_filter.GetEncoded();
    }

    // Compute the filter hash.
    uint256 GetHash() const;

    // Compute the filter header given the previous one.
    uint256 ComputeHeader(const uint256& prev_header) const;

    template <typename Stream>
    void Serialize(Stream& s) const {
        s << m_block_hash
          << static_cast<uint8_t>(m_filter_type)
          << m_filter.GetEncoded();
    }

    template <typename Stream>
    void Unserialize(Stream& s) {
        std::vector<unsigned char> encoded_filter;
        uint8_t filter_type;

        s >> m_block_hash
          >> filter_type
          >> encoded_filter;

        m_filter_type = static_cast<BlockFilterType>(filter_type);

        switch (m_filter_type) {
        case BlockFilterType::BASIC:
            m_filter = GCSFilter(m_block_hash.GetUint64(0), m_block_hash.GetUint64(1),
                                 BASIC_FILTER_P, BASIC_FILTER_M, std::move(encoded_filter));
            break;

        default:
            throw std::ios_base::failure("unknown filter_type");
        }
    }
};

#endif // BITCOIN_BLOCKFILTER_H
