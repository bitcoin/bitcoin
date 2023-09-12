// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>

#include <util/fs.h>
#include <streams.h>

static void FindByte(benchmark::Bench& bench)
{
    // Setup
    FILE* file = fsbridge::fopen("streams_tmp", "w+b");
    const size_t file_size = 200;
    uint8_t data[file_size] = {0};
    data[file_size-1] = 1;
    fwrite(&data, sizeof(uint8_t), file_size, file);
    rewind(file);
    BufferedFile bf{file, /*nBufSize=*/file_size + 1, /*nRewindIn=*/file_size, 0};

    bench.run([&] {
        bf.SetPos(0);
        bf.FindByte(std::byte(1));
    });

    // Cleanup
    bf.fclose();
    fs::remove("streams_tmp");
}

BENCHMARK(FindByte, benchmark::PriorityLevel::HIGH);
