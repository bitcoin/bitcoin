// Copyright (c) 2019-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <hash.h>
#include <net.h>
#include <netmessagemaker.h>
#include <protocol.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/net.h>
#include <test/util/xoroshiro128plusplus.h>
#include <util/chaintype.h>

#include <cassert>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

namespace {

std::vector<std::string> g_all_messages;

void initialize()
{
    ECC_Start();
    SelectParams(ChainType::REGTEST);
    g_all_messages = getAllNetMessageTypes();
    std::sort(g_all_messages.begin(), g_all_messages.end());
}

} // namespace

FUZZ_TARGET(p2p_transport_bidirectional, .init = initialize)
{
    // Test with two V1 transports talking to each other.
    FuzzedDataProvider provider{buffer.data(), buffer.size()};
    XoRoShiRo128PlusPlus rng(provider.ConsumeIntegral<uint64_t>());
    auto t1 = MakeV1Transport(NodeId{0});
    auto t2 = MakeV1Transport(NodeId{1});
    if (!t1 || !t2) return;
    SimulationTest(*t1, *t2, rng, provider, g_all_messages);
}
