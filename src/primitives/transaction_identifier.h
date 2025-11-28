// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

#ifndef BITCOIN_PRIMITIVES_TRANSACTION_IDENTIFIER_H
#define BITCOIN_PRIMITIVES_TRANSACTION_IDENTIFIER_H

#include <attributes.h>
#include <uint256.h>
#include <util/types.h>

#include <compare>
#include <concepts>
#include <tuple>
#include <variant>

/** transaction_identifier represents the two canonical transaction identifier
 * types (txid, wtxid).*/
template <bool has_witness>
class transaction_identifier
{
    uint256 m_wrapped;

    // Note: Use FromUint256 externally instead.
    transaction_identifier(const uint256& wrapped) : m_wrapped{wrapped} {}

    constexpr int Compare(const transaction_identifier<has_witness>& other) const { return m_wrapped.Compare(other.m_wrapped); }
    template <typename Other>
    constexpr int Compare(const Other& other) const
    {
        static_assert(ALWAYS_FALSE<Other>, "Forbidden comparison type");
        return 0;
    }

public:
    transaction_identifier() : m_wrapped{} {}

    template <typename Other>
    bool operator==(const Other& other) const { return Compare(other) == 0; }
    template <typename Other>
    bool operator<(const Other& other) const { return Compare(other) < 0; }

    const uint256& ToUint256() const LIFETIMEBOUND { return m_wrapped; }
    static transaction_identifier FromUint256(const uint256& id) { return {id}; }

    /** Wrapped `uint256` methods. */
    constexpr bool IsNull() const { return m_wrapped.IsNull(); }
    constexpr void SetNull() { m_wrapped.SetNull(); }
    static std::optional<transaction_identifier> FromHex(std::string_view hex)
    {
        auto u{uint256::FromHex(hex)};
        if (!u) return std::nullopt;
        return FromUint256(*u);
    }
    std::string GetHex() const { return m_wrapped.GetHex(); }
    std::string ToString() const { return m_wrapped.ToString(); }
    static constexpr auto size() { return decltype(m_wrapped)::size(); }
    constexpr const std::byte* data() const { return reinterpret_cast<const std::byte*>(m_wrapped.data()); }
    constexpr const std::byte* begin() const { return reinterpret_cast<const std::byte*>(m_wrapped.begin()); }
    constexpr const std::byte* end() const { return reinterpret_cast<const std::byte*>(m_wrapped.end()); }
    template <typename Stream> void Serialize(Stream& s) const { m_wrapped.Serialize(s); }
    template <typename Stream> void Unserialize(Stream& s) { m_wrapped.Unserialize(s); }
};

/** Txid commits to all transaction fields except the witness. */
using Txid = transaction_identifier<false>;
/** Wtxid commits to all transaction fields including the witness. */
using Wtxid = transaction_identifier<true>;

template <typename T>
concept TxidOrWtxid = std::is_same_v<T, Txid> || std::is_same_v<T, Wtxid>;

class GenTxid : public std::variant<Txid, Wtxid>
{
public:
    using variant::variant;

    bool IsWtxid() const { return std::holds_alternative<Wtxid>(*this); }

    const uint256& ToUint256() const LIFETIMEBOUND
    {
        return std::visit([](const auto& id) -> const uint256& { return id.ToUint256(); }, *this);
    }

    friend auto operator<=>(const GenTxid& a, const GenTxid& b)
    {
        // Use a reference for read-only access to the hash, avoiding a copy that might not be optimized away.
        return std::tuple<bool, const uint256&>(a.IsWtxid(), a.ToUint256()) <=> std::tuple<bool, const uint256&>(b.IsWtxid(), b.ToUint256());
    }
};

#endif // BITCOIN_PRIMITIVES_TRANSACTION_IDENTIFIER_H
