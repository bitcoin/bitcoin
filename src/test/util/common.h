// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_COMMON_H
#define BITCOIN_TEST_UTIL_COMMON_H

#include <tinyformat.h>

#include <algorithm>
#include <chrono>
#include <concepts>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <functional>
#include <iostream>
#include <optional>
#include <ostream>
#include <ranges>
#include <source_location>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeindex>

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
template <typename Clock, typename Duration>
inline std::ostream& operator<<(std::ostream& os, const std::chrono::time_point<Clock, Duration>& tp)
{
    return os << tp.time_since_epoch().count();
}

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

namespace detail {

/// Returns a printable Python-style b"" byte string with \x escapes.
inline std::string make_printable(std::string_view v)
{
    std::string out = "b\"";
    out.reserve(v.size() * 2); // Include a bit of overhead
    for (unsigned char c : v) {
        if (32 <= c && c <= 126 && c != '\\' && c != '\"') {
            // Any ascii_is_printing char other than quote and backslash is fine.
            out += static_cast<char>(c);
        } else {
            // The others are escaped.
            out += tfm::format("\\x%02X", c);
        }
    }
    out += '\"';
    return out;
}

/// Make a value nicely readable after a test failure.
template <typename T>
std::string format_value(const T& value)
{
    if constexpr (std::same_as<T, bool>) {
        // Use boolalpha formatting
        return value ? "true" : "false";
    } else if constexpr (std::is_integral_v<T> && sizeof(T) == 1) {
        // May represent a char, but avoid printing terminal escape codes
        // Like std::ascii_is_printing from C++26
        if (32 <= value && value <= 126) {
            return tfm::format("%s ('%c')", int{value}, static_cast<char>(value));
        } else {
            return tfm::format("%s (0x%02X)", int{value}, static_cast<unsigned char>(value));
        }
    } else if constexpr (std::is_pointer_v<T> || std::is_array_v<T>) {
        // Passing an array as a value is often wrong, because the check
        // may have meant to run on the elements of the array. Even for
        // string equality checks, char* pointers and char[N] arrays may be
        // unsafe, because they may lack a null terminator.
        //
        // Tolerate only const char* (not char*) and const char[N] to
        // compare strings. The dev should generally prefer std::string and
        // std::string_view.
        if constexpr (std::is_bounded_array_v<T> && std::same_as<std::remove_all_extents_t<T>, char>) {
            return tfm::format("%s from char array (%p)", make_printable(value), value);
        } else if constexpr (std::same_as<T, const char*>) {
            if (value != nullptr) {
                return tfm::format("%s from pointer (%p)", make_printable(value), value);
            }
        }
        return tfm::format("pointer (%p)", value);
    } else if constexpr (std::same_as<T, std::string> || std::same_as<T, std::string_view>) {
        return make_printable(value);
    } else {
        static_assert(requires(std::ostream& os) { os << value; }, "Please provide an operator<<(std::ostream&, const T&) for formatting.");
        std::ostringstream out;
        out << value;
        return out.str();
    }
}

[[noreturn]] inline void fail_report(std::string_view expr, std::string_view explanation, const std::source_location& loc)
{
    std::cerr << tfm::format("%s(%s): Error: Check '%s' failed in \"%s\"! %s\n", loc.file_name(), loc.line(), expr, loc.function_name(), explanation)
              << std::endl;
    std::abort();
}

// The fail-fast behavior of the check_* unit test helpers is intentional.
// Also, they are marked noexcept, because the handling itself must not throw.

inline void check_bool(bool result, std::string_view expr, std::source_location loc = std::source_location::current()) noexcept
{
    if (!result) fail_report(expr, "", loc);
}

template <typename L, typename R, typename OpFunc>
void check_op(const L& lhs, const R& rhs, std::string_view expr, const char* inv_op, const OpFunc& op,
              std::source_location loc = std::source_location::current()) noexcept
{
    if constexpr (std::is_integral_v<L> && std::is_integral_v<R>) {
        // Reject mixed signs for integrals after integral promotion.
        if constexpr (std::is_signed_v<decltype(std::declval<L>() + 0)> != std::is_signed_v<decltype(std::declval<R>() + 0)>) {
            static_assert(std::is_signed_v<L> == std::is_signed_v<R>,
                          "ERROR: Mixed signed/unsigned comparison! Ensure both sides have the same type of signedness.");
        }
    }
    if (!op(lhs, rhs)) {
        std::string explanation{"[" + format_value(lhs) + " " + inv_op + " " + format_value(rhs) + "]"};
        fail_report(expr, explanation, loc);
    }
}

template <std::ranges::forward_range L, std::ranges::forward_range R>
void check_eq_collections(const L& lhs, const R& rhs, std::string_view expr, std::source_location loc = std::source_location::current())
{
    auto [it1, it2] = std::ranges::mismatch(lhs, rhs);
    auto end1 = std::ranges::end(lhs);
    auto end2 = std::ranges::end(rhs);

    // If both reached end, they are identical
    if (it1 == end1 && it2 == end2) return;

    auto format_it = [](auto it, auto end) -> std::string {
        if (it == end) return "end";
        return format_value(*it);
    };

    auto index = std::ranges::distance(std::ranges::begin(lhs), it1);

    std::string explanation = tfm::format("Mismatch at index %d: [%s != %s]", index, format_it(it1, end1), format_it(it2, end2));

    fail_report(expr, explanation, loc);
}

template <typename ExType, typename Func, typename Predicate>
void check_exception(std::string_view expr, std::string_view type, std::string_view pred_expr, const Func& code, const Predicate& pred,
                     std::source_location loc = std::source_location::current()) noexcept
{
    try {
        code();
        std::string m{tfm::format("Nothing was thrown while expecting %s to be thrown.", type)};
        fail_report(expr, m, loc);
    } catch (const ExType& ex) {
        if (!pred(ex)) {
            std::string m{tfm::format("The correct exception type was caught, but the predicate [%s] failed.", pred_expr)};
            if constexpr (requires { ex.what(); }) {
                m += " Exception message: ";
                m += ex.what();
            }
            fail_report(expr, m, loc);
        }
        return;
    } catch (...) {
        std::exception_ptr ex{std::current_exception()};
        std::string detail{"Non-standard exception object"};
        try {
            if (ex) std::rethrow_exception(ex);
        } catch (const std::exception& e) {
            // name might be mangled, but this should be fine and still understandable and useful
            detail = tfm::format("[%s] with message: %s", typeid(e).name(), e.what());
        } catch (...) {
        }
        std::string m{tfm::format("Expected [%s] but caught different exception: %s", type, detail)};
        fail_report(expr, m, loc);
    }
}

} // namespace detail

#define ASSERT(expr) detail::check_bool(static_cast<bool>(expr), #expr)
#define ASSERT_EQ(l, r) detail::check_op(l, r, #l " == " #r, "!=", std::equal_to<>{})
#define ASSERT_NE(l, r) detail::check_op(l, r, #l " != " #r, "==", std::not_equal_to<>{})
#define ASSERT_LT(l, r) detail::check_op(l, r, #l " < " #r, ">=", std::less<>{})
#define ASSERT_GT(l, r) detail::check_op(l, r, #l " > " #r, "<=", std::greater<>{})
#define ASSERT_LE(l, r) detail::check_op(l, r, #l " <= " #r, ">", std::less_equal<>{})
#define ASSERT_GE(l, r) detail::check_op(l, r, #l " >= " #r, "<", std::greater_equal<>{})
#define ASSERT_EQ_COLLECTIONS(l, r) detail::check_eq_collections(l, r, "range equality of " #l ", " #r)
#define ASSERT_EXCEPTION(expr, exp_type, pred) detail::check_exception<exp_type>(#expr, #exp_type, #pred, [&]() { (void)(expr); }, pred)

#endif // BITCOIN_TEST_UTIL_COMMON_H
