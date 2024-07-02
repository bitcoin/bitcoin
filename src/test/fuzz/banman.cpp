// Copyright (c) 2020-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <banman.h>
#include <common/args.h>
#include <netaddress.h>
#include <test/fuzz/FuzzedDataProvider.h>
#include <test/fuzz/fuzz.h>
#include <test/fuzz/util.h>
#include <test/fuzz/util/net.h>
#include <test/util/setup_common.h>
#include <util/fs.h>
#include <util/readwritefile.h>

#include <cassert>
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

void initialize_banman()
{
    static const auto testing_setup = MakeNoLogFileContext<>();
}

static bool operator==(const CBanEntry& lhs, const CBanEntry& rhs)
{
    return lhs.nVersion == rhs.nVersion &&
           lhs.nCreateTime == rhs.nCreateTime &&
           lhs.nBanUntil == rhs.nBanUntil;
}

FUZZ_TARGET(banman, .init = initialize_banman)
{
    FuzzedDataProvider fuzzed_data_provider{buffer.data(), buffer.size()};
    SetMockTime(ConsumeTime(fuzzed_data_provider));
    fs::path banlist_file = gArgs.GetDataDirNet() / "fuzzed_banlist";

    const bool start_with_corrupted_banlist{fuzzed_data_provider.ConsumeBool()};
    bool force_read_and_write_to_err{false};
    if (start_with_corrupted_banlist) {
        assert(WriteBinaryFile(banlist_file + ".json",
                               fuzzed_data_provider.ConsumeRandomLengthString()));
    } else {
        force_read_and_write_to_err = fuzzed_data_provider.ConsumeBool();
        if (force_read_and_write_to_err) {
            banlist_file = fs::path{"path"} / "to" / "inaccessible" / "fuzzed_banlist";
        }
    }

    {
        BanMan ban_man{banlist_file, /*client_interface=*/nullptr, /*default_ban_time=*/ConsumeBanTimeOffset(fuzzed_data_provider)};
        // The complexity is O(N^2), where N is the input size, because each call
        // might call DumpBanlist (or other methods that are at least linear
        // complexity of the input size).
        bool contains_invalid{false};
        LIMITED_WHILE(fuzzed_data_provider.ConsumeBool(), 300)
        {
            CallOneOf(
                fuzzed_data_provider,
                [&] {
                    CNetAddr net_addr{ConsumeNetAddr(fuzzed_data_provider)};
                    if (!net_addr.IsCJDNS() || !net_addr.IsValid()) {
                        const std::optional<CNetAddr>& addr{LookupHost(net_addr.ToStringAddr(), /*fAllowLookup=*/false)};
                        if (addr.has_value() && addr->IsValid()) {
                            net_addr = *addr;
                        } else {
                            contains_invalid = true;
                        }
                    }
                    auto ban_time_offset = ConsumeBanTimeOffset(fuzzed_data_provider);
                    auto since_unix_epoch = fuzzed_data_provider.ConsumeBool();
                    ban_man.Ban(net_addr, ban_time_offset, since_unix_epoch);
                },
                [&] {
                    CSubNet subnet{ConsumeSubNet(fuzzed_data_provider)};
                    subnet = LookupSubNet(subnet.ToString());
                    if (!subnet.IsValid()) {
                        contains_invalid = true;
                    }
                    auto ban_time_offset = ConsumeBanTimeOffset(fuzzed_data_provider);
                    auto since_unix_epoch = fuzzed_data_provider.ConsumeBool();
                    ban_man.Ban(subnet, ban_time_offset, since_unix_epoch);
                },
                [&] {
                    ban_man.ClearBanned();
                },
                [&] {
                    ban_man.IsBanned(ConsumeNetAddr(fuzzed_data_provider));
                },
                [&] {
                    ban_man.IsBanned(ConsumeSubNet(fuzzed_data_provider));
                },
                [&] {
                    ban_man.Unban(ConsumeNetAddr(fuzzed_data_provider));
                },
                [&] {
                    ban_man.Unban(ConsumeSubNet(fuzzed_data_provider));
                },
                [&] {
                    banmap_t banmap;
                    ban_man.GetBanned(banmap);
                },
                [&] {
                    ban_man.DumpBanlist();
                },
                [&] {
                    ban_man.Discourage(ConsumeNetAddr(fuzzed_data_provider));
                });
        }
        if (!force_read_and_write_to_err) {
            ban_man.DumpBanlist();
            SetMockTime(ConsumeTime(fuzzed_data_provider));
            banmap_t banmap;
            ban_man.GetBanned(banmap);
            BanMan ban_man_read{banlist_file, /*client_interface=*/nullptr, /*default_ban_time=*/0};
            banmap_t banmap_read;
            ban_man_read.GetBanned(banmap_read);
            if (!contains_invalid) {
                assert(banmap == banmap_read);
            }
        }
    }
    fs::remove(fs::PathToString(banlist_file + ".json"));
}
