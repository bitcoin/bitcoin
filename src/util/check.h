// Copyright (c) 2019-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_CHECK_H
#define BITCOIN_UTIL_CHECK_H

#include <attributes.h>

#include <atomic>
#include <cassert> // IWYU pragma: export
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

std::string StrFormatInternalBug(std::string_view msg, std::string_view file, int line, std::string_view func);

class NonFatalCheckError : public std::runtime_error
{
public:
    NonFatalCheckError(std::string_view msg, std::string_view file, int line, std::string_view func);
};

/** Internal helper */
void assertion_fail(std::string_view file, int line, std::string_view func, std::string_view assertion);

/** Helper for CHECK_NONFATAL() */
template <typename T>
T&& inline_check_non_fatal(LIFETIMEBOUND T&& val, const char* file, int line, const char* func, const char* assertion)
{
    if (!val) {
        if constexpr (G_ABORT_ON_FAILED_ASSUME) {
            assertion_fail(file, line, func, assertion);
        }
        throw NonFatalCheckError{assertion, file, line, func};
    }
    return std::forward<T>(val);
}

#if defined(NDEBUG)
#error "Cannot compile without assertions!"
#endif

/** Helper for Assert()/Assume() */
template <bool IS_ASSERT, typename T>
constexpr T&& inline_assertion_check(LIFETIMEBOUND T&& val, [[maybe_unused]] const char* file, [[maybe_unused]] int line, [[maybe_unused]] const char* func, [[maybe_unused]] const char* assertion)
{
    if (IS_ASSERT || std::is_constant_evaluated() || G_ABORT_ON_FAILED_ASSUME) {
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
