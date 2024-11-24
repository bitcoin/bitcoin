// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-chainstate.h>
#include <boost/test/unit_test.hpp>
#include <chainparams.h>
#include <test/util/setup_common.h>
#include <util/fs.h>

#include <fstream>
#include <sstream>

using util::Contains;

BOOST_FIXTURE_TEST_SUITE(bitcoin_chainstate_tests, TestChain100Setup)

BOOST_AUTO_TEST_CASE(basic_chainstate_run)
{
    std::stringstream in{""};
    std::stringstream out, err;
    auto result{kernel_chainstate::process(m_path_root / "regtest", in, out, err)};
    BOOST_CHECK_EQUAL(result, 0);
    BOOST_CHECK(err.str().empty());
    BOOST_CHECK(Contains(out.str(), "Shutting down"));
}

BOOST_AUTO_TEST_CASE(chainstate_load_failure_test)
{
    auto custom_datadir{m_path_root / "corrupt_datadir"};
    auto index{custom_datadir / "blocks" / "index"};
    BOOST_REQUIRE(fs::create_directories(index));

    std::ofstream(index / "CURRENT") << "invalid data to trigger error";

    std::stringstream out, err;
    int result{kernel_chainstate::process(custom_datadir, std::cin, out, err)};

    BOOST_CHECK_EQUAL(result, 0); // TODO: This should be 1
    BOOST_CHECK(Contains(err.str(), "Failed to load Chain state"));
    BOOST_CHECK(Contains(out.str(), "Shutting down"));
}

BOOST_AUTO_TEST_SUITE_END()