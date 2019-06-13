// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_CHECK_H
#define BITCOIN_UTIL_CHECK_H

#include <config/bitcoin-config.h>
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
 * For example in RPC code, where it is undersirable to crash the whole program, this can be generally used to replace
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

/**
 * ASSUME(expression)
 *
 * ASSUME(...) is used to document assumptions.
 *
 * Bitcoin Core is always compiled with assertions enabled. assert(...)
 * is thus only appropriate for documenting critical assumptions
 * where a failed assumption should lead to immediate abortion also
 * in production environments.
 *
 * ASSUME(...) works like assert(...) if -DABORT_ON_FAILED_ASSUME
 * and is side-effect safe.
 *
 * More specifically:
 *
 * * If compiled with -DABORT_ON_FAILED_ASSUME (set by --enable-debug
 *   and --enable-fuzz): Evaluate expression and abort if expression is
 *   false.
 *
 * * Otherwise:
 *   Evaluate expression and continue execution.
 *
 * ABORT_ON_FAILED_ASSUME should not be set in production environments.
 *
 * Example
 * =======
 * int main(void) {
 *     ASSUME(IsFoo());
 *     ...
 * }
 *
 * If !IsFoo() and -DABORT_ON_FAILED_ASSUME, then:
 *     filename.cpp:123: main: ASSUME(IsFoo()) failed.
 *     Aborted
 * Otherwise the execution continues.
 */

#ifdef ABORT_ON_FAILED_ASSUME
#define ACTION_ON_FAILED_ASSUME std::abort();
#else
#define ACTION_ON_FAILED_ASSUME
#endif

#define ASSUME(expression)                                                                               \
    do {                                                                                                 \
        if (!(expression)) {                                                                             \
            tfm::format(std::cerr, "%s:%d: %s: ASSUME(%s) failed. You may report this issue here: %s\n", \
                __FILE__, __LINE__, __func__, #expression, PACKAGE_BUGREPORT);                           \
            ACTION_ON_FAILED_ASSUME                                                                      \
        }                                                                                                \
    } while (false)

#endif // BITCOIN_UTIL_CHECK_H
