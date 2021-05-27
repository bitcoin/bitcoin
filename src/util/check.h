// Copyright (c) 2019-2020 The Bitcoin Core developers
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

/** Helper for Assert() */
template <typename T>
T get_pure_r_value(T&& val)
{
    return std::forward<T>(val);
}

/** Identity function. Abort if the value compares equal to zero */
#define Assert(val) ([&]() -> decltype(get_pure_r_value(val)) { auto&& check = (val); assert(#val && check); return std::forward<decltype(get_pure_r_value(val))>(check); }())

/**
 * Assume is the identity function.
 *
 * - Should be used to run non-fatal checks. In debug builds it behaves like
 *   Assert()/assert() to notify developers and testers about non-fatal errors.
 *   In production it doesn't warn or log anything.
 * - For fatal errors, use Assert().
 * - For non-fatal errors in interactive sessions (e.g. RPC or command line
 *   interfaces), CHECK_NONFATAL() might be more appropriate.
 */
#ifdef ABORT_ON_FAILED_ASSUME
#define Assume(val) Assert(val)
#else
#define Assume(val) ([&]() -> decltype(get_pure_r_value(val)) { auto&& check = (val); return std::forward<decltype(get_pure_r_value(val))>(check); }())
#endif

#endif // BITCOIN_UTIL_CHECK_H
