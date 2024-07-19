#ifndef BITCOIN_UTIL_TRANSACTION_IDENTIFIER_H
#define BITCOIN_UTIL_TRANSACTION_IDENTIFIER_H

#include <attributes.h>
#include <uint256.h>
#include <util/types.h>

/** transaction_identifier represents the two canonical transaction identifier
 * types (txid, wtxid).*/
template <bool has_witness>
class transaction_identifier
{
    uint256 m_wrapped;

    // Note: Use FromUint256 externally instead.
    transaction_identifier(const uint256& wrapped) : m_wrapped{wrapped} {}

    // TODO: Comparisons with uint256 should be disallowed once we have
    // converted most of the code to using the new txid types.
    constexpr int Compare(const uint256& other) const { return m_wrapped.Compare(other); }
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
    bool operator!=(const Other& other) const { return Compare(other) != 0; }
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

    /** Conversion function to `uint256`.
     *
     * Note: new code should use `ToUint256`.
     *
     * TODO: This should be removed once the majority of the code has switched
     * to using the Txid and Wtxid types. Until then it makes for a smoother
     * transition to allow this conversion. */
    operator const uint256&() const LIFETIMEBOUND { return m_wrapped; }
};

/** Txid commits to all transaction fields except the witness. */
using Txid = transaction_identifier<false>;
/** Wtxid commits to all transaction fields including the witness. */
using Wtxid = transaction_identifier<true>;

/** DEPRECATED due to missing length-check and hex-check, please use the safer FromHex, or FromUint256 */
inline Txid TxidFromString(std::string_view str)
{
    return Txid::FromUint256(uint256S(str));
}

#endif // BITCOIN_UTIL_TRANSACTION_IDENTIFIER_H
