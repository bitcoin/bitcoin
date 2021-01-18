// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_FUZZ_FUZZ_H
#define BITCOIN_TEST_FUZZ_FUZZ_H

#include <cstdint>
#include <functional>
#include <string_view>
#include <vector>

using TypeTestOneInput = std::function<void(const std::vector<uint8_t>&)>;
using TypeInitialize = std::function<void()>;

void FuzzFrameworkRegisterTarget(std::string_view name, TypeTestOneInput target, TypeInitialize init);

inline void FuzzFrameworkEmptyFun() {}

#define FUZZ_TARGET(name) \
    FUZZ_TARGET_INIT(name, FuzzFrameworkEmptyFun)

#define FUZZ_TARGET_INIT(name, init_fun)                                      \
    void name##_fuzz_target(const std::vector<uint8_t>&);                     \
    struct name##_Before_Main {                                               \
        name##_Before_Main()                                                  \
        {                                                                     \
            FuzzFrameworkRegisterTarget(#name, name##_fuzz_target, init_fun); \
        }                                                                     \
    } const static g_##name##_before_main;                                    \
    void name##_fuzz_target(const std::vector<uint8_t>& buffer)

#endif // BITCOIN_TEST_FUZZ_FUZZ_H
