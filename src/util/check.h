// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_CHECK_H
#define BITCOIN_UTIL_CHECK_H

#include <tinyformat.h>

#include <stdexcept>

/**
 * Abort the program when the condition evaluates to false
 *
 * This should only be used
 * - where the condition is assumed to be true, not for error handling or validating user input
 * - where a failure to fulfill the condition is NOT recoverable and needs to ABORT the program
 *
 * For example in script validation, where any further action after a logic error could lead to a consensus failure, it
 * is better to abort the program and not proceed with validation.
 */
#define ASSERT(condition)                                                  \
    do {                                                                   \
        if (!(condition)) {                                                \
            tfm::format(std::cerr, "%s:%d (%s)\n"                          \
                                   "Internal bug detected: '%s'\n"         \
                                   "Aborting!\n"                           \
                                   "You may report this issue here: %s\n", \
                __FILE__, __LINE__, __func__,                              \
                (#condition),                                              \
                PACKAGE_BUGREPORT);                                        \
            std::abort();                                                  \
        }                                                                  \
    } while (false)

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

#endif // BITCOIN_UTIL_CHECK_H
