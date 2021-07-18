// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>

#include <fs.h>
#include <streams.h>

static void FindByte(benchmark::Bench& bench)
{
    // Setup
    FILE* file = fsbridge::fopen("streams_tmp", "w+b");
    const size_t file_size = 200;
    uint8_t b = 0;
    for (size_t i = 0; i < file_size; ++i) {
        fwrite(&b, 1, 1, file);
    }
    b = 1;
    fwrite(&b, 1, 1, file);
    rewind(file);
    CBufferedFile bf(file, file_size * 2, file_size, 0, 0);

    bench.minEpochIterations(1e7).run([&] {
        bf.SetPos(0);
        bf.FindByte(1);
    });

    // Cleanup
    bf.fclose();
    fs::remove("streams_tmp");
}

BENCHMARK(FindByte);
