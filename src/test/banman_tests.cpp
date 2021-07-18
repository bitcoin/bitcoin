// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <banman.h>
#include <chainparams.h>
#include <netbase.h>
#include <streams.h>
#include <test/util/logging.h>
#include <test/util/setup_common.h>
#include <util/readwritefile.h>


#include <boost/test/unit_test.hpp>

static CNetAddr ResolveIP(const std::string& ip)
{
    CNetAddr addr;
    BOOST_CHECK_MESSAGE(LookupHost(ip, addr, false), strprintf("failed to resolve: %s", ip));
    return addr;
}

BOOST_FIXTURE_TEST_SUITE(banman_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(file)
{
    SetMockTime(777s);
    const auto banlist_path{m_args.GetDataDirBase() / "banlist_test"};
    {
        const CSubNet dummy_subnet_invalid{};
        const CSubNet dummy_subnet_valid{ResolveIP("1.2.3.4"), 8};
        CBanEntry dummy_entry{};
        dummy_entry.nBanUntil = count_seconds(GetTime<std::chrono::seconds>()) + 1;

        banmap_t entries_write{
            {dummy_subnet_invalid, dummy_entry},
            {dummy_subnet_valid, dummy_entry},
        };

        CDataStream data_s{SER_DISK, 0};
        CHashWriter hasher{0, 0};
        hasher << Params().MessageStart() << entries_write;
        data_s << Params().MessageStart() << entries_write << hasher.GetHash();

        assert(WriteBinaryFile(banlist_path.string() + ".dat", data_s.str()));
        {
            // The invalid entry will be dropped, but the valid one remains
            ASSERT_DEBUG_LOG("Cannot parse banned address or subnet: ::/0");
            BanMan banman{banlist_path, nullptr, 0};
            banmap_t entries_read;
            banman.GetBanned(entries_read);
            assert(entries_read.size() == 1);
        }
    }
    {
        const std::string entries_write{
            "{ \"banned_nets\": ["
            "  { \"version\": 1, \"ban_created\": 0, \"banned_until\": 778, \"address\": \"aaaaaaaaa\" },"
            "  { \"version\": 2, \"ban_created\": 0, \"banned_until\": 778, \"address\": \"bbbbbbbbb\" },"
            "  { \"version\": 1, \"ban_created\": 0, \"banned_until\": 778, \"address\": \"1.0.0.0/8\" }"
            "] }"};
        assert(WriteBinaryFile(banlist_path.string() + ".json", entries_write));
        {
            // The invalid entries will be dropped, but the valid one remains
            ASSERT_DEBUG_LOG("Dropping entry with unparseable address or subnet: aaaaaaaaa");
            ASSERT_DEBUG_LOG("Dropping entry with unknown version (2) from ban list");
            BanMan banman{banlist_path, nullptr, 0};
            banmap_t entries_read;
            banman.GetBanned(entries_read);
            assert(entries_read.size() == 1);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
