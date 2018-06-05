// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bench.h"

#include "validation.h"
#include "base58.h"

#include <array>
#include <vector>
#include <string>


static void Base58Encode(benchmark::State& state)
{
    static const std::array<unsigned char, 32> buff = {
        {
            17, 79, 8, 99, 150, 189, 208, 162, 22, 23, 203, 163, 36, 58, 147,
            227, 139, 2, 215, 100, 91, 38, 11, 141, 253, 40, 117, 21, 16, 90,
            200, 24
        }
    };
    while (state.KeepRunning()) {
        EncodeBase58(buff.begin(), buff.end());
    }
}


static void Base58CheckEncode(benchmark::State& state)
{
    static const std::array<unsigned char, 32> buff = {
        {
            17, 79, 8, 99, 150, 189, 208, 162, 22, 23, 203, 163, 36, 58, 147,
            227, 139, 2, 215, 100, 91, 38, 11, 141, 253, 40, 117, 21, 16, 90,
            200, 24
        }
    };
    std::vector<unsigned char> vch;
    vch.assign(buff.begin(), buff.end());
    while (state.KeepRunning()) {
        EncodeBase58Check(vch);
    }
}


static void Base58Decode(benchmark::State& state)
{
    const char* addr = "17VZNX1SN5NtKa8UQFxwQbFeFc3iqRYhem";
    std::vector<unsigned char> vch;
    while (state.KeepRunning()) {
        DecodeBase58(addr, vch);
    }
}


BENCHMARK(Base58Encode);
BENCHMARK(Base58CheckEncode);
BENCHMARK(Base58Decode);
