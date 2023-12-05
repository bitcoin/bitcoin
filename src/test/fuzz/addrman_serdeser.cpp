// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addrdb.h>
#include <addrman.h>
#include <addrman_impl.h>
#include <chainparams.h>
#include <common/args.h>
#include <merkleblock.h>
#include <random.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/addrman.h>
#include <test/fuzz/util/net.h>
#include <test/util/setup_common.h>
#include <time.h>
#include <util/asmap.h>
#include <util/chaintype.h>

#include <cassert>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace {
const BasicTestingSetup* g_setup;
} // namespace

void initialize_addrman()
{
    static const auto testing_setup = MakeNoLogFileContext<>(ChainType::REGTEST);
    g_setup = testing_setup.get();
}

// Check that serialize followed by unserialize produces the same addrman.
FUZZ_TARGET(addrman_serdeser, .init = initialize_addrman)
{
    FuzzedDataProvider fuzzed_data_provider(buffer.data(), buffer.size());
    SetMockTime(ConsumeTime(fuzzed_data_provider));

    NetGroupManager netgroupman{ConsumeNetGroupManager(fuzzed_data_provider)};
    AddrManDeterministic addr_man1{netgroupman, fuzzed_data_provider, GetCheckRatio(g_setup->m_args)};
    AddrManDeterministic addr_man2{netgroupman, fuzzed_data_provider, GetCheckRatio(g_setup->m_args)};

    DataStream data_stream{};

    FillAddrman(addr_man1, fuzzed_data_provider);
    data_stream << addr_man1;
    data_stream >> addr_man2;
    assert(addr_man1 == addr_man2);
}
