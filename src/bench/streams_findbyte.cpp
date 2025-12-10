// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <consensus/consensus.h>
#include <streams.h>
#include <test/util/setup_common.h>
#include <util/fs.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>

static void FindByte(benchmark::Bench& bench)
{
    const auto testing_setup{MakeNoLogFileContext<const BasicTestingSetup>(ChainType::REGTEST)};
    constexpr size_t file_size{2 * MAX_BLOCK_SERIALIZED_SIZE};
    constexpr size_t stride{32 << 10};
    constexpr size_t found_index{file_size - 1};
    constexpr std::byte target{1};

    AutoFile file{fsbridge::fopen(testing_setup->m_path_root / "streams_tmp", "w+b")};
    std::vector data(file_size, std::byte{0});
    data[found_index] = target;
    file << std::span{data};
    file.seek(0, SEEK_SET);
    BufferedFile bf{file, /*nBufSize=*/file_size + 1, /*nRewindIn=*/file_size};

    bench.run([&] {
        for (size_t start{0}; start < file_size; start += stride) {
            bf.SetPos(start);
            bf.FindByte(target);
            assert(bf.GetPos() == found_index);
        }
    });
    assert(file.fclose() == 0);
}

BENCHMARK(FindByte, benchmark::PriorityLevel::HIGH);
