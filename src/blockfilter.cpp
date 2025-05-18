// Copyright (c) 2018-2022 The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <mutex>
#include <set>

#include <blockfilter.h>
#include <crypto/siphash.h>
#include <hash.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <streams.h>
#include <undo.h>
#include <util/golombrice.h>
#include <util/string.h>

using util::Join;

static const std::map<BlockFilterType, std::string> g_filter_types = {
    {BlockFilterType::BASIC, "basic"},
};

uint64_t GCSFilter::HashToRange(const Element& element) const
{
    uint64_t hash = CSipHasher(m_params.m_siphash_k0, m_params.m_siphash_k1)
        .Write(element)
        .Finalize();
    return FastRange64(hash, m_F);
}

std::vector<uint64_t> GCSFilter::BuildHashedSet(const ElementSet& elements) const
{
    std::vector<uint64_t> hashed_elements;
    hashed_elements.reserve(elements.size());
    for (const Element& element : elements) {
        hashed_elements.push_back(HashToRange(element));
    }
    std::sort(hashed_elements.begin(), hashed_elements.end());
    return hashed_elements;
}

GCSFilter::GCSFilter(const Params& params)
    : m_params(params), m_N(0), m_F(0), m_encoded{0}
{}

GCSFilter::GCSFilter(const Params& params, std::vector<unsigned char> encoded_filter, bool skip_decode_check)
    : m_params(params), m_encoded(std::move(encoded_filter))
{
    SpanReader stream{m_encoded};

    uint64_t N = ReadCompactSize(stream);
    m_N = static_cast<uint32_t>(N);
    if (m_N != N) {
        throw std::ios_base::failure("N must be <2^32");
    }
    m_F = static_cast<uint64_t>(m_N) * static_cast<uint64_t>(m_params.m_M);

    if (skip_decode_check) return;

    // Verify that the encoded filter contains exactly N elements. If it has too much or too little
    // data, a std::ios_base::failure exception will be raised.
    BitStreamReader bitreader{stream};
    for (uint64_t i = 0; i < m_N; ++i) {
        GolombRiceDecode(bitreader, m_params.m_P);
    }
    if (!stream.empty()) {
        throw std::ios_base::failure("encoded_filter contains excess data");
    }
}

GCSFilter::GCSFilter(const Params& params, const ElementSet& elements)
    : m_params(params)
{
    size_t N = elements.size();
    m_N = static_cast<uint32_t>(N);
    if (m_N != N) {
        throw std::invalid_argument("N must be <2^32");
    }
    m_F = static_cast<uint64_t>(m_N) * static_cast<uint64_t>(m_params.m_M);

    VectorWriter stream{m_encoded, 0};

    WriteCompactSize(stream, m_N);

    if (elements.empty()) {
        return;
    }

    BitStreamWriter bitwriter{stream};

    uint64_t last_value = 0;
    for (uint64_t value : BuildHashedSet(elements)) {
        uint64_t delta = value - last_value;
        GolombRiceEncode(bitwriter, m_params.m_P, delta);
        last_value = value;
    }

    bitwriter.Flush();
}

bool GCSFilter::MatchInternal(const uint64_t* element_hashes, size_t size) const
{
    SpanReader stream{m_encoded};

    // Seek forward by size of N
    uint64_t N = ReadCompactSize(stream);
    assert(N == m_N);

    BitStreamReader bitreader{stream};

    uint64_t value = 0;
    size_t hashes_index = 0;
    for (uint32_t i = 0; i < m_N; ++i) {
        uint64_t delta = GolombRiceDecode(bitreader, m_params.m_P);
        value += delta;

        while (true) {
            if (hashes_index == size) {
                return false;
            } else if (element_hashes[hashes_index] == value) {
                return true;
            } else if (element_hashes[hashes_index] > value) {
                break;
            }

            hashes_index++;
        }
    }

    return false;
}

bool GCSFilter::Match(const Element& element) const
{
    uint64_t query = HashToRange(element);
    return MatchInternal(&query, 1);
}

bool GCSFilter::MatchAny(const ElementSet& elements) const
{
    const std::vector<uint64_t> queries = BuildHashedSet(elements);
    return MatchInternal(queries.data(), queries.size());
}

const std::string& BlockFilterTypeName(BlockFilterType filter_type)
{
    static std::string unknown_retval;
    auto it = g_filter_types.find(filter_type);
    return it != g_filter_types.end() ? it->second : unknown_retval;
}

bool BlockFilterTypeByName(const std::string& name, BlockFilterType& filter_type) {
    for (const auto& entry : g_filter_types) {
        if (entry.second == name) {
            filter_type = entry.first;
            return true;
        }
    }
    return false;
}

const std::set<BlockFilterType>& AllBlockFilterTypes()
{
    static std::set<BlockFilterType> types;

    static std::once_flag flag;
    std::call_once(flag, []() {
            for (const auto& entry : g_filter_types) {
                types.insert(entry.first);
            }
        });

    return types;
}

const std::string& ListBlockFilterTypes()
{
    static std::string type_list{Join(g_filter_types, ", ", [](const auto& entry) { return entry.second; })};

    return type_list;
}

static GCSFilter::ElementSet BasicFilterElements(const CBlock& block,
                                                 const CBlockUndo& block_undo)
{
    GCSFilter::ElementSet elements;

    for (const CTransactionRef& tx : block.vtx) {
        for (const CTxOut& txout : tx->vout) {
            const CScript& script = txout.scriptPubKey;
            if (script.empty() || script[0] == OP_RETURN) continue;
            elements.emplace(script.begin(), script.end());
        }
    }

    for (const CTxUndo& tx_undo : block_undo.vtxundo) {
        for (const Coin& prevout : tx_undo.vprevout) {
            const CScript& script = prevout.out.scriptPubKey;
            if (script.empty()) continue;
            elements.emplace(script.begin(), script.end());
        }
    }

    return elements;
}

BlockFilter::BlockFilter(BlockFilterType filter_type, const uint256& block_hash,
                         std::vector<unsigned char> filter, bool skip_decode_check)
    : m_filter_type(filter_type), m_block_hash(block_hash)
{
    GCSFilter::Params params;
    if (!BuildParams(params)) {
        throw std::invalid_argument("unknown filter_type");
    }
    m_filter = GCSFilter(params, std::move(filter), skip_decode_check);
}

BlockFilter::BlockFilter(BlockFilterType filter_type, const CBlock& block, const CBlockUndo& block_undo)
    : m_filter_type(filter_type), m_block_hash(block.GetHash())
{
    GCSFilter::Params params;
    if (!BuildParams(params)) {
        throw std::invalid_argument("unknown filter_type");
    }
    m_filter = GCSFilter(params, BasicFilterElements(block, block_undo));
}

bool BlockFilter::BuildParams(GCSFilter::Params& params) const
{
    switch (m_filter_type) {
    case BlockFilterType::BASIC:
        params.m_siphash_k0 = m_block_hash.GetUint64(0);
        params.m_siphash_k1 = m_block_hash.GetUint64(1);
        params.m_P = BASIC_FILTER_P;
        params.m_M = BASIC_FILTER_M;
        return true;
    case BlockFilterType::INVALID:
        return false;
    }

    return false;
}

uint256 BlockFilter::GetHash() const
{
    return Hash(GetEncodedFilter());
}

uint256 BlockFilter::ComputeHeader(const uint256& prev_header) const
{
    return Hash(GetHash(), prev_header);
}
