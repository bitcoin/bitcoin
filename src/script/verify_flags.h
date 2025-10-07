// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SCRIPT_VERIFY_FLAGS_H
#define BITCOIN_SCRIPT_VERIFY_FLAGS_H

#include <compare>
#include <cstdint>

enum class script_verify_flag_name : uint8_t;

class script_verify_flags
{
public:
    using value_type = uint64_t;

    consteval script_verify_flags() = default;

    // also allow construction with hard-coded 0 (but not other integers)
    consteval explicit(false) script_verify_flags(value_type f) : m_value{f} { if (f != 0) throw 0; }

    // implicit construction from a hard-coded SCRIPT_VERIFY_* constant is also okay
    constexpr explicit(false) script_verify_flags(script_verify_flag_name f) : m_value{value_type{1} << static_cast<uint8_t>(f)} { }

    // rule of 5
    constexpr script_verify_flags(const script_verify_flags&) = default;
    constexpr script_verify_flags(script_verify_flags&&) = default;
    constexpr script_verify_flags& operator=(const script_verify_flags&) = default;
    constexpr script_verify_flags& operator=(script_verify_flags&&) = default;
    constexpr ~script_verify_flags() = default;

    // integer conversion needs to be very explicit
    static constexpr script_verify_flags from_int(value_type f) { script_verify_flags r; r.m_value = f; return r; }
    constexpr value_type as_int() const { return m_value; }

    // bitwise operations
    constexpr script_verify_flags operator~() const { return from_int(~m_value); }
    friend constexpr script_verify_flags operator|(script_verify_flags a, script_verify_flags b) { return from_int(a.m_value | b.m_value); }
    friend constexpr script_verify_flags operator&(script_verify_flags a, script_verify_flags b) { return from_int(a.m_value & b.m_value); }

    // in-place bitwise operations
    constexpr script_verify_flags& operator|=(script_verify_flags vf) { m_value |= vf.m_value; return *this; }
    constexpr script_verify_flags& operator&=(script_verify_flags vf) { m_value &= vf.m_value; return *this; }

    // tests
    constexpr explicit operator bool() const { return m_value != 0; }
    constexpr bool operator==(script_verify_flags other) const { return m_value == other.m_value; }

    /** Compare two script_verify_flags. <, >, <=, and >= are auto-generated from this. */
    friend constexpr std::strong_ordering operator<=>(const script_verify_flags& a, const script_verify_flags& b) noexcept
    {
        return a.m_value <=> b.m_value;
    }

private:
    value_type m_value{0}; // default value is SCRIPT_VERIFY_NONE
};

inline constexpr script_verify_flags operator~(script_verify_flag_name f)
{
    return ~script_verify_flags{f};
}

inline constexpr script_verify_flags operator|(script_verify_flag_name f1, script_verify_flag_name f2)
{
    return script_verify_flags{f1} | f2;
}

#endif // BITCOIN_SCRIPT_VERIFY_FLAGS_H
