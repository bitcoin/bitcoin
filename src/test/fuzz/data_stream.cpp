// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addrman.h>
#include <net.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/util/setup_common.h>

#include <cstdint>
#include <vector>

void initialize_data_stream_addr_man()
{
    static const auto testing_setup = MakeNoLogFileContext<>();
}

FUZZ_TARGET_INIT(data_stream_addr_man, initialize_data_stream_addr_man)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    CDataStream data_stream = ConsumeDataStream(fuzzed_data_provider);
    CAddrMan addr_man(/* asmap */ std::vector<bool>(), /* deterministic */ false, /* consistency_check_ratio */ 0);
    CAddrDB::Read(addr_man, data_stream);
}
