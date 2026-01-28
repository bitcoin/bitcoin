// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/private_broadcast_persist.h>

#include <node/private_broadcast_persist_args.h>
#include <private_broadcast.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>

#include <cstdint>
#include <vector>

using node::DumpPrivateBroadcast;
using node::LoadPrivateBroadcast;
using node::PrivateBroadcastPath;

namespace {
const TestingSetup* g_setup;
} // namespace

void initialize_private_broadcast_persist()
{
    static const auto testing_setup = MakeNoLogFileContext<const TestingSetup>();
    g_setup = testing_setup.get();
}

FUZZ_TARGET(private_broadcast_persist, .init = initialize_private_broadcast_persist)
{
    SeedRandomStateForTest(SeedRand::ZEROS);
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    FuzzedFileProvider fuzzed_file_provider{fuzzed_data_provider};

    auto fuzzed_fopen = [&](const fs::path&, const char*) {
        return fuzzed_file_provider.open();
    };

    PrivateBroadcast pb;
    LoadPrivateBroadcast(pb, PrivateBroadcastPath(g_setup->m_args), fuzzed_fopen);
    DumpPrivateBroadcast(pb, PrivateBroadcastPath(g_setup->m_args), fuzzed_fopen, /*skip_file_commit=*/true);
}
