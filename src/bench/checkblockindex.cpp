// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bench/bench.h>
#include <test/util/setup_common.h>
#include <validation.h>

static void CheckBlockIndex(benchmark::Bench& bench)
{
    auto testing_setup{MakeNoLogFileContext<TestChain100Setup>()};
    // Mine some more blocks
    testing_setup->mineBlocks(1000);
    bench.run([&] {
        testing_setup->m_node.chainman->CheckBlockIndex();
    });
}


BENCHMARK(CheckBlockIndex, benchmark::PriorityLevel::HIGH);
