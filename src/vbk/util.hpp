// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SRC_VBK_UTIL_HPP
#define BITCOIN_SRC_VBK_UTIL_HPP

#include <consensus/consensus.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <streams.h>
#include <version.h>

#include <veriblock/pop.hpp>

#include <algorithm>
#include <amount.h>
#include <chain.h>
#include <functional>

namespace VeriBlock {

/**
 * Create new Container with elements filtered elements of original container. All elements for which pred returns false will be removed.
 * @tparam Container any container, such as std::vector
 * @param v instance of container to be filtered
 * @param pred predicate. Returns true for elements that need to stay in container.
 */
template <typename Container>
Container filter_if(const Container& inp, std::function<bool(const typename Container::value_type&)> pred)
{
    Container v = inp;
    v.erase(std::remove_if(
                v.begin(), v.end(), [&](const typename Container::value_type& t) {
                    return !pred(t);
                }),
        v.end());
    return v;
}

inline CBlockHeader headerFromBytes(const std::vector<uint8_t>& v)
{
    CDataStream stream(v, SER_NETWORK, PROTOCOL_VERSION);
    CBlockHeader header;
    stream >> header;
    if(!stream.eof()) {
        throw std::runtime_error("stream is not empty");
    }
    return header;
}

inline altintegration::AltBlock blockToAltBlock(int nHeight, const CBlockHeader& block)
{
    altintegration::AltBlock alt;
    alt.height = nHeight;
    alt.timestamp = block.nTime;
    alt.previousBlock = block.hashPrevBlock.asVector();
    alt.hash = block.GetHash().asVector();
    return alt;
}

inline altintegration::AltBlock blockToAltBlock(const CBlockIndex& index)
{
    return blockToAltBlock(index.nHeight, index.GetBlockHeader());
}

//PopData weight
inline int64_t GetPopDataWeight(const altintegration::PopData& pop_data)
{
    return ::GetSerializeSize(pop_data, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS) * (WITNESS_SCALE_FACTOR - 1) + ::GetSerializeSize(pop_data, PROTOCOL_VERSION);
}

template <typename T>
bool FindPayloadInBlock(const CBlock& block, const typename T::id_t& id, T& out)
{
    (void)block;
    (void)id;
    (void)out;
    static_assert(sizeof(T) == 0, "Undefined type in FindPayloadInBlock");
    return false;
}

template <>
inline bool FindPayloadInBlock(const CBlock& block, const altintegration::VbkBlock::id_t& id, altintegration::VbkBlock& out)
{
    for (auto& blk : block.popData.context) {
        if (blk.getShortHash() == id) {
            out = blk;
            return true;
        }
    }

    return false;
}

template <>
inline bool FindPayloadInBlock(const CBlock& block, const altintegration::VTB::id_t& id, altintegration::VTB& out)
{
    for (auto& vtb : block.popData.vtbs) {
        if (vtb.getId() == id) {
            out = vtb;
            return true;
        }
    }

    return false;
}

template <>
inline bool FindPayloadInBlock(const CBlock& block, const altintegration::ATV::id_t& id, altintegration::ATV& out)
{
    for (auto& atv : block.popData.atvs) {
        if (atv.getId() == id) {
            out = atv;
            return true;
        }
    }
    return false;
}


} // namespace VeriBlock
#endif
