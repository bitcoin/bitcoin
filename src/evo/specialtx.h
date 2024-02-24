// Copyright (c) 2018-2024 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_EVO_SPECIALTX_H
#define BITCOIN_EVO_SPECIALTX_H

#include <primitives/transaction.h>
#include <serialize.h>
#include <streams.h>
#include <uint256.h>
#include <version.h>

#include <string_view>
#include <optional>
#include <vector>

template <typename T>
std::optional<T> GetTxPayload(const std::vector<unsigned char>& payload)
{
    CDataStream ds(payload, SER_NETWORK, PROTOCOL_VERSION);
    try {
        T obj;
        ds >> obj;
        return ds.empty() ? std::make_optional(std::move(obj)) : std::nullopt;
    } catch (const std::exception& e) {
        return std::nullopt;
    }
}
template <typename T, typename TxType>
std::optional<T> GetTxPayload(const TxType& tx, bool assert_type = true)
{
    if (assert_type) { ASSERT_IF_DEBUG(tx.nType == T::SPECIALTX_TYPE); }
    if (tx.nType != T::SPECIALTX_TYPE) return std::nullopt;
    return GetTxPayload<T>(tx.vExtraPayload);
}

template <typename T>
void SetTxPayload(CMutableTransaction& tx, const T& payload)
{
    CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
    ds << payload;
    tx.vExtraPayload.assign(UCharCast(ds.data()), UCharCast(ds.data() + ds.size()));
}

uint256 CalcTxInputsHash(const CTransaction& tx);

#endif // BITCOIN_EVO_SPECIALTX_H
