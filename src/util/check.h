// Copyright (c) 2019-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_CHECK_H
#define BITCOIN_UTIL_CHECK_H

#include <attributes.h>

#include <cassert> // IWYU pragma: export
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

std::string StrFormatInternalBug(std::string_view msg, std::string_view file, int line, std::string_view func);

class NonFatalCheckError : public std::runtime_error
{
public:
    NonFatalCheckError(std::string_view msg, std::string_view file, int line, std::string_view func);
};

/** Helper for CHECK_NONFATAL() */
template <typename T>
T&& inline_check_non_fatal(LIFETIMEBOUND T&& val, const char* file, int line, const char* func, const char* assertion)
{
    if (!val) {
        throw NonFatalCheckError{assertion, file, line, func};
    }
    return std::forward<T>(val);
}

#if defined(NDEBUG)
#error "Cannot compile without assertions!"
#endif

/** Helper for Assert() */
void assertion_fail(std::string_view file, int line, std::string_view func, std::string_view assertion);

/** Helper for Assert()/Assume() */
template <bool IS_ASSERT, typename T>
T&& inline_assertion_check(LIFETIMEBOUND T&& val, [[maybe_unused]] const char* file, [[maybe_unused]] int line, [[maybe_unused]] const char* func, [[maybe_unused]] const char* assertion)
{
    if constexpr (IS_ASSERT
#ifdef ABORT_ON_FAILED_ASSUME
                  || true
#endif
    ) {
        if (!val) {
            assertion_fail(file, line, func, assertion);
        }
    }
    return std::forward<T>(val);
}

// All macros may use __func__ inside a lambda, so put them under nolint.
// NOLINTBEGIN(bugprone-lambda-function-name)

#define STR_INTERNAL_BUG(msg) StrFormatInternalBug((msg), __FILE__, __LINE__, __func__)

/**
 * Identity function. Throw a NonFatalCheckError when the condition evaluates to false
 *
 * This should only be used
 * - where the condition is assumed to be true, not for error handling or validating user input
 * - where a failure to fulfill the condition is recoverable and does not abort the program
 *
 * For example in RPC code, where it is undesirable to crash the whole program, this can be generally used to replace
 * asserts or recoverable logic errors. A NonFatalCheckError in RPC code is caught and passed as a string to the RPC
 * caller, which can then report the issue to the developers.
 */
#define CHECK_NONFATAL(condition) \
    inline_check_non_fatal(condition, __FILE__, __LINE__, __func__, #condition)

/** Identity function. Abort if the value compares equal to zero */
#define Assert(val) inline_assertion_check<true>(val, __FILE__, __LINE__, __func__, #val)

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
#define Assume(val) inline_assertion_check<false>(val, __FILE__, __LINE__, __func__, #val)

/**
 * NONFATAL_UNREACHABLE() is a macro that is used to mark unreachable code. It throws a NonFatalCheckError.
 */
#define NONFATAL_UNREACHABLE()                                        \
    throw NonFatalCheckError(                                         \
        "Unreachable code reached (non-fatal)", __FILE__, __LINE__, __func__)

// NOLINTEND(bugprone-lambda-function-name)

#endif // BITCOIN_UTIL_CHECK_H
