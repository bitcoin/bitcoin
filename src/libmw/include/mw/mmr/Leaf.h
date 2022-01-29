#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <mw/common/Macros.h>
#include <mw/mmr/LeafIndex.h>
#include <mw/models/crypto/Hash.h>
#include <mw/crypto/Hasher.h>

MMR_NAMESPACE

class Leaf
{
public:
    Leaf() noexcept = default;
    Leaf(const LeafIndex& index, mw::Hash hash, std::vector<uint8_t> data)
        : m_index(index), m_hash(std::move(hash)), m_data(std::move(data)) { }

    static Leaf Create(const LeafIndex& index, std::vector<uint8_t> data)
    {
        mw::Hash hash = CalcHash(index, data);
        return Leaf(index, std::move(hash), std::move(data));
    }

    static mw::Hash CalcHash(const LeafIndex& index, const std::vector<uint8_t>& data)
    {
        return Hasher()
            .Append<uint64_t>(index.GetPosition())
            .Append(data)
            .hash();
    }

    bool operator!=(const Leaf& rhs) const noexcept { return m_hash != rhs.m_hash; }
    bool operator==(const Leaf& rhs) const noexcept { return m_hash == rhs.m_hash; }

    const Index& GetNodeIndex() const noexcept { return m_index.GetNodeIndex(); }
    const LeafIndex& GetLeafIndex() const noexcept { return m_index; }
    const mw::Hash& GetHash() const noexcept { return m_hash; }
    const std::vector<uint8_t>& vec() const noexcept { return m_data; }

private:
    LeafIndex m_index;
    mw::Hash m_hash;
    std::vector<uint8_t> m_data;
};

END_NAMESPACE