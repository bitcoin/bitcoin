// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kernel/chainparams.h>
#include <privbcast/params.h>

#include <boost/test/unit_test.hpp>

#include <set>
#include <string>

using namespace privbcast;

BOOST_AUTO_TEST_SUITE(privbcast_plan_tests)

BOOST_AUTO_TEST_CASE(mainnet_dns_seeds)
{
    // R3: a change that takes the mainnet seed list below MIN_DNS_SEEDS distinct names fails here.
    // Below it, a job that loses one seed leaves opportunities empty or takes two first attempts
    // from one seed. Add a seed, or change QUERIES_PER_SEED, MIN_DNS_SEEDS and R3 together,
    // knowing that more answers per seed let one lying seed reach more of a job.
    const auto main{CChainParams::Main()};
    const std::set<std::string> seeds{main->DNSSeeds().begin(), main->DNSSeeds().end()};
    BOOST_CHECK_GE(seeds.size(), MIN_DNS_SEEDS);
}

BOOST_AUTO_TEST_SUITE_END()
