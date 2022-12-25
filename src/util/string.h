// Copyright (c) 2019-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_UTIL_STRING_H
#define SYSCOIN_UTIL_STRING_H

#include <util/spanparsing.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <locale>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

void ReplaceAll(std::string& in_out, const std::string& search, const std::string& substitute);

[[nodiscard]] inline std::vector<std::string> SplitString(std::string_view str, char sep)
{
    return spanparsing::Split<std::string>(str, sep);
}

[[nodiscard]] inline std::vector<std::string> SplitString(std::string_view str, std::string_view separators)
{
    return spanparsing::Split<std::string>(str, separators);
}

[[nodiscard]] inline std::string_view TrimStringView(std::string_view str, std::string_view pattern = " \f\n\r\t\v")
{
    std::string::size_type front = str.find_first_not_of(pattern);
    if (front == std::string::npos) {
        return {};
    }
    std::string::size_type end = str.find_last_not_of(pattern);
    return str.substr(front, end - front + 1);
}

[[nodiscard]] inline std::string TrimString(std::string_view str, std::string_view pattern = " \f\n\r\t\v")
{
    return std::string(TrimStringView(str, pattern));
}

[[nodiscard]] inline std::string_view RemovePrefixView(std::string_view str, std::string_view prefix)
{
    if (str.substr(0, prefix.size()) == prefix) {
        return str.substr(prefix.size());
    }
    return str;
}

[[nodiscard]] inline std::string RemovePrefix(std::string_view str, std::string_view prefix)
{
    return std::string(RemovePrefixView(str, prefix));
}

/**
 * Join all container items. Typically used to concatenate strings but accepts
 * containers with elements of any type.
 *
 * @param container The items to join
 * @param separator The separator
 * @param unary_op  Apply this operator to each item
 */
template <typename C, typename S, typename UnaryOp>
auto Join(const C& container, const S& separator, UnaryOp unary_op)
{
    decltype(unary_op(*container.begin())) ret;
    bool first{true};
    for (const auto& item : container) {
        if (!first) ret += separator;
        ret += unary_op(item);
        first = false;
    }
    return ret;
}

template <typename C, typename S>
auto Join(const C& container, const S& separator)
{
    return Join(container, separator, [](const auto& i) { return i; });
}

/**
 * Create an unordered multi-line list of items.
 */
inline std::string MakeUnorderedList(const std::vector<std::string>& items)
{
    return Join(items, "\n", [](const std::string& item) { return "- " + item; });
}

/**
 * Check if a string does not contain any embedded NUL (\0) characters
 */
[[nodiscard]] inline bool ContainsNoNUL(std::string_view str) noexcept
{
    for (auto c : str) {
        if (c == 0) return false;
    }
    return true;
}

/**
 * Locale-independent version of std::to_string
 */
template <typename T>
std::string ToString(const T& t)
{
    std::ostringstream oss;
    oss.imbue(std::locale::classic());
    oss << t;
    return oss.str();
}

/**
 * Check whether a container begins with the given prefix.
 */
template <typename T1, size_t PREFIX_LEN>
[[nodiscard]] inline bool HasPrefix(const T1& obj,
                                const std::array<uint8_t, PREFIX_LEN>& prefix)
{
    return obj.size() >= PREFIX_LEN &&
           std::equal(std::begin(prefix), std::end(prefix), std::begin(obj));
}

#endif // SYSCOIN_UTIL_STRING_H
