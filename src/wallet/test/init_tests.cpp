// Copyright (c) 2018-2022 The Tortoisecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <common/args.h>
#include <noui.h>
#include <test/util/logging.h>
#include <test/util/setup_common.h>
#include <wallet/test/init_test_fixture.h>

namespace wallet {
BOOST_FIXTURE_TEST_SUITE(init_tests, InitWalletDirTestingSetup)

BOOST_AUTO_TEST_CASE(walletinit_verify_walletdir_default)
{
    SetWalletDir(m_walletdir_path_cases["default"]);
    bool result = m_wallet_loader->verify();
    BOOST_CHECK(result == true);
    fs::path walletdir = m_args.GetPathArg("-walletdir");
    fs::path expected_path = fs::canonical(m_walletdir_path_cases["default"]);
    BOOST_CHECK_EQUAL(walletdir, expected_path);
}

BOOST_AUTO_TEST_CASE(walletinit_verify_walletdir_custom)
{
    SetWalletDir(m_walletdir_path_cases["custom"]);
    bool result = m_wallet_loader->verify();
    BOOST_CHECK(result == true);
    fs::path walletdir = m_args.GetPathArg("-walletdir");
    fs::path expected_path = fs::canonical(m_walletdir_path_cases["custom"]);
    BOOST_CHECK_EQUAL(walletdir, expected_path);
}

BOOST_AUTO_TEST_CASE(walletinit_verify_walletdir_does_not_exist)
{
    SetWalletDir(m_walletdir_path_cases["nonexistent"]);
    {
        ASSERT_DEBUG_LOG("does not exist");
        bool result = m_wallet_loader->verify();
        BOOST_CHECK(result == false);
    }
}

BOOST_AUTO_TEST_CASE(walletinit_verify_walletdir_is_not_directory)
{
    SetWalletDir(m_walletdir_path_cases["file"]);
    {
        ASSERT_DEBUG_LOG("is not a directory");
        bool result = m_wallet_loader->verify();
        BOOST_CHECK(result == false);
    }
}

BOOST_AUTO_TEST_CASE(walletinit_verify_walletdir_is_not_relative)
{
    SetWalletDir(m_walletdir_path_cases["relative"]);
    {
        ASSERT_DEBUG_LOG("is a relative path");
        bool result = m_wallet_loader->verify();
        BOOST_CHECK(result == false);
    }
}

BOOST_AUTO_TEST_CASE(walletinit_verify_walletdir_no_trailing)
{
    SetWalletDir(m_walletdir_path_cases["trailing"]);
    bool result = m_wallet_loader->verify();
    BOOST_CHECK(result == true);
    fs::path walletdir = m_args.GetPathArg("-walletdir");
    fs::path expected_path = fs::canonical(m_walletdir_path_cases["default"]);
    BOOST_CHECK_EQUAL(walletdir, expected_path);
}

BOOST_AUTO_TEST_CASE(walletinit_verify_walletdir_no_trailing2)
{
    SetWalletDir(m_walletdir_path_cases["trailing2"]);
    bool result = m_wallet_loader->verify();
    BOOST_CHECK(result == true);
    fs::path walletdir = m_args.GetPathArg("-walletdir");
    fs::path expected_path = fs::canonical(m_walletdir_path_cases["default"]);
    BOOST_CHECK_EQUAL(walletdir, expected_path);
}

BOOST_AUTO_TEST_SUITE_END()
} // namespace wallet
