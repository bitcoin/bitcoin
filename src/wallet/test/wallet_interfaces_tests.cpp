// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <interfaces/wallet.h>
#include <test/util/setup_common.h>
#include <util/bip32.h>
#include <wallet/context.h>
#include <wallet/test/util.h>
#include <wallet/wallet.h>

#include <boost/test/unit_test.hpp>
#include <optional>
#include <vector>

namespace wallet {

BOOST_FIXTURE_TEST_SUITE(wallet_interfaces_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(derivehdkey)
{
    WalletContext context;
    context.args = &m_args;
    auto wallet = TestCreateWallet(context);
    BOOST_REQUIRE(wallet);
    auto interface = interfaces::MakeWallet(context, wallet);

    const std::vector<uint32_t> path{87 | BIP32_HARDENED_FLAG};
    const auto result{interface->deriveHDKey(path, std::nullopt)};
    BOOST_REQUIRE(result);

    LOCK(wallet->cs_wallet);
    const auto active{wallet->GetHDPubKeys(CWallet::HDKeyFilter::Active)};
    BOOST_REQUIRE_EQUAL(active.size(), 1U);
    const auto xprv{wallet->GetExtKey(active.begin()->first)};
    BOOST_REQUIRE(xprv);
    const auto expected{DeriveExtKey(*xprv, path)};
    BOOST_REQUIRE(expected);
    BOOST_CHECK(result->first == expected->first.Neuter());
    BOOST_CHECK(result->second == expected->second);
}

BOOST_AUTO_TEST_CASE(derivehdkey_error_is_forwarded)
{
    WalletContext context;
    context.args = &m_args;
    auto wallet = TestCreateWallet(context);
    BOOST_REQUIRE(wallet);
    BOOST_REQUIRE(wallet->EncryptWallet("hunter2"));
    BOOST_REQUIRE(wallet->Lock());

    auto interface = interfaces::MakeWallet(context, wallet);
    const auto result{interface->deriveHDKey(/*path=*/{87 | BIP32_HARDENED_FLAG}, std::nullopt)};
    BOOST_REQUIRE(!result);
    BOOST_CHECK(result.error().code == WalletErrorCode::UnlockNeeded);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace wallet
