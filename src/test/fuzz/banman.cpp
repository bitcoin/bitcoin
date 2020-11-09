// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <banman.h>
#include <fs.h>
#include <netaddress.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <util/system.h>

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace {
int64_t ConsumeBanTimeOffset(FuzzedDataProvider& fuzzed_data_provider) noexcept
{
    // Avoid signed integer overflow by capping to int32_t max:
    // banman.cpp:137:73: runtime error: signed integer overflow: 1591700817 + 9223372036854775807 cannot be represented in type 'long'
    return fuzzed_data_provider.ConsumeIntegralInRange<int64_t>(std::numeric_limits<int64_t>::min(), std::numeric_limits<int32_t>::max());
}
} // namespace

void initialize()
{
    InitializeFuzzingContext();
}

void test_one_input(const std::vector<uint8_t>& buffer)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    const fs::path banlist_file = GetDataDir() / "fuzzed_banlist.dat";
    fs::remove(banlist_file);
    {
        BanMan ban_man{banlist_file, nullptr, ConsumeBanTimeOffset(fuzzed_data_provider)};
        while (fuzzed_data_provider.ConsumeBool()) {
            switch (fuzzed_data_provider.ConsumeIntegralInRange<int>(0, 11)) {
            case 0: {
                ban_man.Ban(ConsumeNetAddr(fuzzed_data_provider),
                    ConsumeBanTimeOffset(fuzzed_data_provider), fuzzed_data_provider.ConsumeBool());
                break;
            }
            case 1: {
                ban_man.Ban(ConsumeSubNet(fuzzed_data_provider),
                    ConsumeBanTimeOffset(fuzzed_data_provider), fuzzed_data_provider.ConsumeBool());
                break;
            }
            case 2: {
                ban_man.ClearBanned();
                break;
            }
            case 4: {
                ban_man.IsBanned(ConsumeNetAddr(fuzzed_data_provider));
                break;
            }
            case 5: {
                ban_man.IsBanned(ConsumeSubNet(fuzzed_data_provider));
                break;
            }
            case 6: {
                ban_man.Unban(ConsumeNetAddr(fuzzed_data_provider));
                break;
            }
            case 7: {
                ban_man.Unban(ConsumeSubNet(fuzzed_data_provider));
                break;
            }
            case 8: {
                banmap_t banmap;
                ban_man.GetBanned(banmap);
                break;
            }
            case 9: {
                ban_man.DumpBanlist();
                break;
            }
            case 11: {
                ban_man.Discourage(ConsumeNetAddr(fuzzed_data_provider));
                break;
            }
            }
        }
    }
    fs::remove(banlist_file);
}
