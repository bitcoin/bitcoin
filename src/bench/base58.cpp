// Copyright (c) 2016-2019 The Widecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>

#include <base58.h>

#include <array>
#include <vector>


static void Base58Encode(benchmark::Bench& bench)
{
    static const std::array<unsigned char, 32> buff = {
        {
            17, 79, 8, 99, 150, 189, 208, 162, 22, 23, 203, 163, 36, 58, 147,
            227, 139, 2, 215, 100, 91, 38, 11, 141, 253, 40, 117, 21, 16, 90,
            200, 24
        }
    };
    bench.batch(buff.size()).unit("byte").run([&] {
        EncodeBase58(buff);
    });
}


static void Base58CheckEncode(benchmark::Bench& bench)
{
    static const std::array<unsigned char, 32> buff = {
        {
            17, 79, 8, 99, 150, 189, 208, 162, 22, 23, 203, 163, 36, 58, 147,
            227, 139, 2, 215, 100, 91, 38, 11, 141, 253, 40, 117, 21, 16, 90,
            200, 24
        }
    };
    bench.batch(buff.size()).unit("byte").run([&] {
        EncodeBase58Check(buff);
    });
}


static void Base58Decode(benchmark::Bench& bench)
{
    const char* addr = "17VZNX1SN5NtKa8UQFxwQbFeFc3iqRYhem";
    std::vector<unsigned char> vch;
    bench.batch(strlen(addr)).unit("byte").run([&] {
        (void) DecodeBase58(addr, vch, 64);
    });
}


BENCHMARK(Base58Encode);
BENCHMARK(Base58CheckEncode);
BENCHMARK(Base58Decode);
