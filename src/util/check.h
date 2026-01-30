// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_CHECK_H
#define BITCOIN_UTIL_CHECK_H

#include <attributes.h>

#include <atomic>
#include <cassert> // IWYU pragma: export
#include <source_location>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

constexpr bool G_FUZZING_BUILD{
#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
    true
#else
    false
#endif
};
constexpr bool G_ABORT_ON_FAILED_ASSUME{G_FUZZING_BUILD ||
#ifdef ABORT_ON_FAILED_ASSUME
    true
#else
    false
#endif
};

extern std::atomic<bool> g_enable_dynamic_fuzz_determinism;

inline bool EnableFuzzDeterminism()
{
    if constexpr (G_FUZZING_BUILD) {
        return true;
    } else if constexpr (!G_ABORT_ON_FAILED_ASSUME) {
        // Running fuzz tests is always disabled if Assume() doesn't abort
        // (ie, non-fuzz non-debug builds), as otherwise tests which
        // should fail due to a failing Assume may still pass. As such,
        // we also statically disable fuzz determinism in that case.
        return false;
    } else {
        return g_enable_dynamic_fuzz_determinism;
    }
}

extern bool g_detail_test_only_CheckFailuresAreExceptionsNotAborts;
struct test_only_CheckFailuresAreExceptionsNotAborts {
    test_only_CheckFailuresAreExceptionsNotAborts() { g_detail_test_only_CheckFailuresAreExceptionsNotAborts = true; };
    ~test_only_CheckFailuresAreExceptionsNotAborts() { g_detail_test_only_CheckFailuresAreExceptionsNotAborts = false; };
};

std::string StrFormatInternalBug(std::string_view msg, const std::source_location& loc);

class NonFatalCheckError : public std::runtime_error
{
public:
    NonFatalCheckError(std::string_view msg, const std::source_location& loc);
};

/** Internal helper */
void assertion_fail(const std::source_location& loc, std::string_view assertion);

/** Helper for CHECK_NONFATAL() */
template <typename T>
T&& inline_check_non_fatal(LIFETIMEBOUND T&& val, const std::source_location& loc, std::string_view assertion)
{
    if (!val) {
        if constexpr (G_ABORT_ON_FAILED_ASSUME) {
            assertion_fail(loc, assertion);
        }
        throw NonFatalCheckError{assertion, loc};
    }
    return std::forward<T>(val);
}

#if defined(NDEBUG)
#error "Cannot compile without assertions!"
#endif

/** Helper for Assert()/Assume() */
template <bool IS_ASSERT, typename T>
constexpr T&& inline_assertion_check(LIFETIMEBOUND T&& val, [[maybe_unused]] const std::source_location& loc, [[maybe_unused]] std::string_view assertion)
{
    if (IS_ASSERT || std::is_constant_evaluated() || G_ABORT_ON_FAILED_ASSUME) {
        if (!val) {
            assertion_fail(loc, assertion);
        }
    }
    return std::forward<T>(val);
}

#define STR_INTERNAL_BUG(msg) StrFormatInternalBug((msg), std::source_location::current())

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
    inline_check_non_fatal(condition, std::source_location::current(), #condition)

/** Identity function. Abort if the value compares equal to zero */
#define Assert(val) inline_assertion_check<true>(val, std::source_location::current(), #val)

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
#define Assume(val) inline_assertion_check<false>(val, std::source_location::current(), #val)

/**
 * NONFATAL_UNREACHABLE() is a macro that is used to mark unreachable code. It throws a NonFatalCheckError.
 */
#define NONFATAL_UNREACHABLE() \
    throw NonFatalCheckError { "Unreachable code reached (non-fatal)", std::source_location::current() }

#if defined(__has_feature)
#    if __has_feature(address_sanitizer)
#       include <sanitizer/asan_interface.h>
#    endif
#endif

#ifndef ASAN_POISON_MEMORY_REGION
#   define ASAN_POISON_MEMORY_REGION(addr, size) ((void)(addr), (void)(size))
#   define ASAN_UNPOISON_MEMORY_REGION(addr, size) ((void)(addr), (void)(size))
#endif

#endif // BITCOIN_UTIL_CHECK_H
