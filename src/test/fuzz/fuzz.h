// Copyright (c) 2009-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_FUZZ_FUZZ_H
#define BITCOIN_TEST_FUZZ_FUZZ_H

#include <span.h>

#include <cstdint>
#include <functional>
#include <string_view>

/**
 * Can be used to limit a theoretically unbounded loop. This caps the runtime
 * to avoid timeouts or OOMs.
 */
#define LIMITED_WHILE(condition, limit) \
    for (unsigned _count{limit}; (condition) && _count; --_count)

using FuzzBufferType = Span<const uint8_t>;

enum class FuzzResult
{
    /** Normal fuzzing result. */
    MAYBE_INTERESTING,

    /** This value can be returned by fuzz tests to indicate the input was uninteresting.
     *
     * libfuzzer can make use of this and will not insert the input in its corpus, even when it
     * appears to increase coverage. */
    UNINTERESTING
};

using TypeTestOneInput = std::function<FuzzResult(FuzzBufferType)>;
using TypeInitialize = std::function<void()>;
using TypeHidden = bool;

void FuzzFrameworkRegisterTarget(std::string_view name, TypeTestOneInput target, TypeInitialize init, TypeHidden hidden);

inline void FuzzFrameworkEmptyInitFun() {}

/** Fuzz target without initialization function that always succeeds. */
#define FUZZ_TARGET(name) \
    FUZZ_TARGET_INIT(name, FuzzFrameworkEmptyInitFun)

/** Fuzz target without initialization function that returns FuzzResult. */
#define FUZZ_PARTIAL_TARGET(name) \
    FUZZ_PARTIAL_TARGET_INIT(name, FuzzFrameworkEmptyInitFun)

/** Fuzz target with initialization function that always returns MAYBE_INTERESTING. */
#define FUZZ_TARGET_INIT(name, init_fun) \
    FUZZ_TARGET_INIT_HIDDEN(name, init_fun, false)

/** Fuzz target with initialization function that returns FuzzResult. */
#define FUZZ_PARTIAL_TARGET_INIT(name, init_fun) \
    FUZZ_PARTIAL_TARGET_INIT_HIDDEN(name, init_fun, false)

/** Potentially hidden fuzz target with initialization that returns FuzzResult. */
#define FUZZ_PARTIAL_TARGET_INIT_HIDDEN(name, init_fun, hidden)                       \
    FuzzResult name##_fuzz_target(FuzzBufferType);                                    \
    struct name##_Before_Main {                                                       \
        name##_Before_Main()                                                          \
        {                                                                             \
            FuzzFrameworkRegisterTarget(#name, name##_fuzz_target, init_fun, hidden); \
        }                                                                             \
    } const static g_##name##_before_main;                                            \
    FuzzResult name##_fuzz_target(FuzzBufferType buffer)

/** Potentially hidden fuzz target with initialization that always returns MAYBE_INTERESTING. */
#define FUZZ_TARGET_INIT_HIDDEN(name, init_fun, hidden)                               \
    void name##_fuzz_target_complete(FuzzBufferType);                                 \
    FUZZ_PARTIAL_TARGET_INIT_HIDDEN(name, init_fun, hidden)                           \
    {                                                                                 \
        name##_fuzz_target_complete(buffer);                                          \
        return FuzzResult::MAYBE_INTERESTING;                                         \
    }                                                                                 \
    void name##_fuzz_target_complete(FuzzBufferType buffer)

#endif // BITCOIN_TEST_FUZZ_FUZZ_H
