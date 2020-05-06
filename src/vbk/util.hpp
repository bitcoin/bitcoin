// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SRC_VBK_UTIL_HPP
#define BITCOIN_SRC_VBK_UTIL_HPP

#include <primitives/block.h>
#include <primitives/transaction.h>
#include <streams.h>
#include <version.h>

#include <vbk/config.hpp>
#include <vbk/service_locator.hpp>

#include <algorithm>
#include <amount.h>
#include <chain.h>
#include <functional>


namespace VeriBlock {


inline void setVBKNoInput(COutPoint& out) noexcept
{
    out.hash.SetHex("56424b206973207468652062657374");
    out.n = uint32_t(-1);
}

inline bool isVBKNoInput(const COutPoint& out) noexcept
{
    return ("000000000000000000000000000000000056424b206973207468652062657374" == out.hash.GetHex() && out.n == uint32_t(-1));
}

inline bool isPopTx(const CTransaction& tx) noexcept
{
    // pop tx has 1 input
    if (tx.vin.size() != 1) {
        return false;
    }

    auto& in = tx.vin[0];

    // scriptSig is not empty
    if (in.scriptSig.empty()) {
        return false;
    }

    // output of this single input is "no input"
    if (!isVBKNoInput(in.prevout)) {
        return false;
    }

    // pop tx has 1 output
    if (tx.vout.size() != 1) {
        return false;
    }

    auto& out = tx.vout[0];

    // size of scriptsig is 1 operator, and it is OP_RETURN
    auto& script = out.scriptPubKey;
    return script.size() == 1 && script[0] == OP_RETURN;
}

inline size_t RegularTxesTotalSize(const std::vector<CTransactionRef>& vtx)
{
    return std::count_if(vtx.begin(), vtx.end(), [](const CTransactionRef& tx) {
        return !isPopTx(*tx);
    });
}
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

inline CAmount getCoinbaseSubsidy(const CAmount& subsidy)
{
    return subsidy * (100 - VeriBlock::getService<VeriBlock::Config>().POP_REWARD_PERCENTAGE) / 100;
}

inline CMutableTransaction MakePopTx(const CScript& scriptSig)
{
    CMutableTransaction tx;

    tx.vout.resize(1);
    tx.vout[0].nValue = 0;
    tx.vout[0].scriptPubKey << OP_RETURN;

    tx.vin.resize(1);
    VeriBlock::setVBKNoInput(tx.vin[0].prevout);
    tx.vin[0].scriptSig = scriptSig;

    assert(VeriBlock::isPopTx(CTransaction(tx)));

    return tx;
}

template <typename DefaultComparatorTag>
struct poptx_priority {
};

/** \class CompareTxMemPoolEntryByPoPtxPriority
 *
 *  Sort an entry that PoP transaction has a higher priority, if both transaction are standart compared by DefaultComparator
 */
template <typename DefaultComparator>
class CompareTxMemPoolEntryByPoPtxPriority
{
public:
    template <typename T>
    bool operator()(const T& a, const T& b) const
    {
        if (isPopTx(a.GetTx()))
            return true;

        if (isPopTx(b.GetTx()))
            return false;

        DefaultComparator defaultComparator;
        return defaultComparator(a, b);
    }
};

inline CBlockHeader headerFromBytes(const std::vector<uint8_t>& v)
{
    CDataStream stream(v, SER_NETWORK, PROTOCOL_VERSION);
    CBlockHeader header;
    stream >> header;
    return header;
}

inline altintegration::AltBlock blockToAltBlock(int nHeight, const CBlockHeader& block)
{
    altintegration::AltBlock alt;
    alt.height = nHeight;
    alt.timestamp = block.nTime;
    alt.previousBlock = std::vector<uint8_t>(block.hashPrevBlock.begin(), block.hashPrevBlock.end());
    auto hash = block.GetHash();
    alt.hash = std::vector<uint8_t>(hash.begin(), hash.end());
    return alt;
}

inline altintegration::AltBlock blockToAltBlock(const CBlockIndex& index)
{
    return blockToAltBlock(index.nHeight, index.GetBlockHeader());
}

} // namespace VeriBlock
#endif
