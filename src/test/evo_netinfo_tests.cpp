// Copyright (c) 2025 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>

#include <chainparams.h>
#include <evo/netinfo.h>
#include <netbase.h>
#include <streams.h>

#include <boost/test/unit_test.hpp>

const std::vector<std::pair</*input=*/std::string, /*expected_ret=*/NetInfoStatus>> vals{
    // Address and port specified
    {"1.1.1.1:8888", NetInfoStatus::Success},
    // Address specified, port should default to default P2P core
    {"1.1.1.1", NetInfoStatus::Success},
    // Mainnet P2P port on non-mainnet
    {"1.1.1.1:9999", NetInfoStatus::BadPort},
    // Valid IPv4 formatting but invalid IPv4 address
    {"0.0.0.0:8888", NetInfoStatus::BadInput},
    // Port greater than uint16_t max
    {"1.1.1.1:99999", NetInfoStatus::BadInput},
    // Only IPv4 allowed
    {"[2606:4700:4700::1111]:8888", NetInfoStatus::BadInput},
    // Domains are not allowed
    {"example.com:8888", NetInfoStatus::BadInput},
    // Incorrect IPv4 address
    {"1.1.1.256:8888", NetInfoStatus::BadInput},
    // Missing address
    {":8888", NetInfoStatus::BadInput},
};

BOOST_FIXTURE_TEST_SUITE(evo_netinfo_tests, RegTestingSetup)

void ValidateGetEntries(const CServiceList& entries, const size_t expected_size)
{
    BOOST_CHECK_EQUAL(entries.size(), expected_size);
}

BOOST_AUTO_TEST_CASE(mnnetinfo_rules)
{
    // Validate AddEntry() rules enforcement
    for (const auto& [input, expected_ret] : vals) {
        MnNetInfo netInfo;
        BOOST_CHECK_EQUAL(netInfo.AddEntry(input), expected_ret);
        if (expected_ret != NetInfoStatus::Success) {
            BOOST_CHECK(netInfo.GetEntries().empty());
        } else {
            ValidateGetEntries(netInfo.GetEntries(), /*expected_size=*/1);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
