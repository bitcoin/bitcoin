// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_CHECK_H
#define BITCOIN_UTIL_CHECK_H

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <tinyformat.h>

#include <stdexcept>

class NonFatalCheckError : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

/**
 * Throw a NonFatalCheckError when the condition evaluates to false
 *
 * This should only be used
 * - where the condition is assumed to be true, not for error handling or validating user input
 * - where a failure to fulfill the condition is recoverable and does not abort the program
 *
 * For example in RPC code, where it is undesirable to crash the whole program, this can be generally used to replace
 * asserts or recoverable logic errors. A NonFatalCheckError in RPC code is caught and passed as a string to the RPC
 * caller, which can then report the issue to the developers.
 */
#define CHECK_NONFATAL(condition)                                 \
    do {                                                          \
        if (!(condition)) {                                       \
            throw NonFatalCheckError(                             \
                strprintf("%s:%d (%s)\n"                          \
                          "Internal bug detected: '%s'\n"         \
                          "You may report this issue here: %s\n", \
                    __FILE__, __LINE__, __func__,                 \
                    (#condition),                                 \
                    PACKAGE_BUGREPORT));                          \
        }                                                         \
    } while (false)

#if defined(NDEBUG)
#error "Cannot compile without assertions!"
#endif

/** Helper for Assert(). TODO remove in C++14 and replace `decltype(get_pure_r_value(val))` with `T` (templated lambda) */
template <typename T>
T get_pure_r_value(T&& val)
{
    return std::forward<T>(val);
}

/** Identity function. Abort if the value compares equal to zero */
#define Assert(val) [&]() -> decltype(get_pure_r_value(val))& { auto& check = (val); assert(#val && check); return check; }()

/** Helper for static_check_equal, no type inference **/
template <typename T1, typename T2, T1 value_1, T2 value_2, bool b>
struct static_check_struct {
    constexpr static_check_struct() {
        static_assert(b, "Static Check Failed");
    }
};

/** Check if the template arguments are (operator) at compile time. Compiler error prints values if
 * failure. **/
#define static_check_macro_msg(f, s1, s2, msg) static_check_struct<decltype(s1), decltype(s2), s1, s2, s1 f s2>()
#define static_check_macro_nomsg(f, s1, s2)  static_check_macro_msg(f, s1, s2, "")
#define static_check_macro_selector(f, s1, s2, msg, FUNC, ...) FUNC
#define static_check_macro(...) static_check_macro_selector(__VA_ARGS__, static_check_macro_msg(__VA_ARGS__), static_check_macro_nomsg(__VA_ARGS__),)
#define static_check_equal(...) static_check_macro(==, __VA_ARGS__)
#define static_check_not_equal(...) static_check_macro(!=, __VA_ARGS__)
#define static_check_greater(...) static_check_macro(>, __VA_ARGS__)
#define static_check_less(...) static_check_macro(<, __VA_ARGS__)
#define static_check_greater_equal(...) static_check_macro(>=, __VA_ARGS__)
#define static_check_less_equal(...) static_check_macro(<=, __VA_ARGS__)


#endif // BITCOIN_UTIL_CHECK_H
