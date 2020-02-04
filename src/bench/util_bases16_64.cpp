// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>

#include <random.h>
#include <util/strencodings.h>

#include <array>
#include <vector>


static void Base64Encode(benchmark::State& state)
{
    static const std::array<unsigned char, 32> buff = {
        {17, 79, 8, 99, 150, 189, 208, 162, 22, 23, 203, 163, 36, 58, 147,
            227, 139, 2, 215, 100, 91, 38, 11, 141, 253, 40, 117, 21, 16, 90,
            200, 24}};
    while (state.KeepRunning()) {
        (void)EncodeBase64(buff.data(), buff.size());
    }
}


static void Base64Decode(benchmark::State& state)
{
    std::string str = "X19jb29raWVfXzpkMzE5MjE2NTU2MmY3ZDZiYjZmY2Q0NzQ5YzhhZGIyMjlkMzYxNGRjMjQ0OTBjMTUwOTcwNjI3NWZjMDMyNzUy==";
    while (state.KeepRunning()) {
        DecodeBase64(str, nullptr);
    }
}

static void Base64DecodeBig(benchmark::State& state)
{
    FastRandomContext insecure_rand(true);
    auto bytes = insecure_rand.randbytes(1024);
    std::string str = EncodeBase64(bytes.data(), bytes.size());
    while (state.KeepRunning()) {
        DecodeBase64(str, nullptr);
    }
}

static void Base64DecodeVec(benchmark::State& state)
{
    std::string str = "X19jb29raWVfXzpkMzE5MjE2NTU2MmY3ZDZiYjZmY2Q0NzQ5YzhhZGIyMjlkMzYxNGRjMjQ0OTBjMTUwOTcwNjI3NWZjMDMyNzUy==";
    while (state.KeepRunning()) {
        DecodeBase64(str.c_str(), nullptr);
    }
}

static void HexParse(benchmark::State& state)
{
    std::string str = "fc7aba077ad5f4c3a0988d8daa4810d0d4a0e3bcb53af662998898f33df0556a";
    while (state.KeepRunning()) {
        ParseHex(str);
    }
}


BENCHMARK(Base64Encode, 1000 * 1000);
BENCHMARK(Base64Decode, 1000 * 1000);
BENCHMARK(Base64DecodeBig, 400 * 1000);
BENCHMARK(Base64DecodeVec, 1000 * 1000);
BENCHMARK(HexParse, 1000 * 1000);
