// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_COMMON_H
#define BITCOIN_TEST_UTIL_COMMON_H

#include <ostream>
#include <optional>
#include <string>

/**
 * BOOST_CHECK_EXCEPTION predicates to check the specific validation error.
 * Use as
 * BOOST_CHECK_EXCEPTION(code that throws, exception type, HasReason("foo"));
 */
class HasReason
{
public:
    explicit HasReason(std::string_view reason) : m_reason(reason) {}
    bool operator()(std::string_view s) const { return s.find(m_reason) != std::string_view::npos; }
    bool operator()(const std::exception& e) const { return (*this)(e.what()); }

private:
    const std::string m_reason;
};

// Make types usable in BOOST_CHECK_* @{
namespace std {
template <typename T> requires std::is_enum_v<T>
inline std::ostream& operator<<(std::ostream& os, const T& e)
{
    return os << static_cast<std::underlying_type_t<T>>(e);
}

template <typename T>
inline std::ostream& operator<<(std::ostream& os, const std::optional<T>& v)
{
    return v ? os << *v
             : os << "std::nullopt";
}
} // namespace std

template <typename T>
concept HasToString = requires(const T& t) { t.ToString(); };

template <HasToString T>
inline std::ostream& operator<<(std::ostream& os, const T& obj)
{
    return os << obj.ToString();
}

// @}

#endif // BITCOIN_TEST_UTIL_COMMON_H
