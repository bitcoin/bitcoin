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

using TypeTestOneInput = std::function<void(FuzzBufferType)>;
struct FuzzTargetOptions {
    std::function<void()> init{[] {}};
    bool hidden{false};
};

void FuzzFrameworkRegisterTarget(std::string_view name, TypeTestOneInput target, FuzzTargetOptions opts);

#if defined(__clang__)
#define FUZZ_TARGET(...) _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wgnu-zero-variadic-macro-arguments\"") DETAIL_FUZZ(__VA_ARGS__) _Pragma("clang diagnostic pop")
#else
#define FUZZ_TARGET(...) DETAIL_FUZZ(__VA_ARGS__)
#endif

#define DETAIL_FUZZ(name, ...)                                                        \
    void name##_fuzz_target(FuzzBufferType);                                          \
    struct name##_Before_Main {                                                       \
        name##_Before_Main()                                                          \
        {                                                                             \
            FuzzFrameworkRegisterTarget(#name, name##_fuzz_target, {__VA_ARGS__});    \
        }                                                                             \
    } const static g_##name##_before_main;                                            \
    void name##_fuzz_target(FuzzBufferType buffer)

#endif // BITCOIN_TEST_FUZZ_FUZZ_H
