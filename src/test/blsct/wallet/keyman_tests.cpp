// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/wallet/txfactory.h>
#include <blsct/wallet/verification.h>
#include <test/util/random.h>
#include <test/util/setup_common.h>
#include <txdb.h>
#include <wallet/receive.h>
#include <wallet/test/util.h>
#include <wallet/wallet.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(blsct_keyman_tests)

BOOST_FIXTURE_TEST_CASE(wallet_test, TestingSetup)
{
    wallet::DatabaseOptions options;
    options.create_flags |= wallet::WALLET_FLAG_BLSCT;

    std::shared_ptr<wallet::CWallet> wallet(new wallet::CWallet(m_node.chain.get(), "", wallet::CreateMockWalletDatabase(options)));

    LOCK(wallet->cs_wallet);
    auto blsct_km = wallet->GetOrCreateBLSCTKeyMan();
    blsct_km->SetHDSeed(MclScalar(uint256(1)));
    BOOST_CHECK(blsct_km->NewSubAddressPool());

    std::vector<std::string> expectedAddresses = {
        "xNVGDQwqMnD7Wi13X5FutkC11VwAf9oaZuDMs1sXUPHgGjmkh5UukhJihW56uTY4GMMLEfzF536N3zMAzMcWA6mxfm6sjsQ9k49cUNBN1B6xAGqGE2bnBsBPijYsMMQNuQjrRDLn5rs",
        "xNVMGyrMaB3eDUHENL4wGMdg838VjdyNvM5gunyb8eu8P9W2hZwXtSBGjQbpAT3XnoqESLZuiT54X3KupigzG9xabAv2XAQTbjY7SA1SsFFEuBJ4rBPzhiBKmiK2bzQcPSqkcHZJdiF",
        "xNTgvimBmRYk51LFN6GwmX7vfad1yhCQwnRvDkps9xiybD1RRxtreDQiR9wUSUXCn8VheTESxTEzfr51dfF3n9AYUTSWANUjgZvkB6CrzetaYfUxMU66suvX9xZwNogjWJ6y2xrfKBg",
        "xNVX7dGHDF5uPPYnMS5qUvsBbnhscHoVwUyeamnuyJ5rnBdAqATaqHXYXReWgwETu7zHfjrvqJp1LzjYeHVbKaArvsVpTyW1vCBUeeeTCtThV2VdpnzoMHC4fvG5jAhQzG919tGHfLP",
        "xNTnzT9xj7G3eyL7htPWmnhvTd6Lf7eaW4MJpdCBUhJLknZxjed5UxeKuEA8ZPnyPHe6QuX26zrdw6vuLa4XB5L1jqD5jEGb4HmvjhJBfbsJseoHGbtEmdkqdHSi6QCCawZdiBq7rGk",
        "xNVaahV3wgUi9dFpeJW8fZ3Z2vUnQ6vRVqYQjiJkT4Cixmpg9HKaUEmn8DtK5Tcwp8QL7PamHUB9LV9DfhS86KHkLEf3cJ2pjTcn5Eo23KR1NXYeHzYiF4hNdptYE1iJNqoa5fCYfYJ",
        "xNUoi8ebc8mZN6JPtcu77xAYirX6dvn8QC7q1FDAkUK9ErCfoTDWmaEB4hiXT2CS4PCitDF1dS6jvmujoKeJu1fSAENcLT8G8GryGvXzHRq3PrCB8WrMn4wAmf4JsUWrKqjMxir7NFd",
        "xNUFcFvEmTESGhMMuBSdN598WeyE4RYnSBgNGrLNs7hZt9S3o3xUcKi2f7QjKicgeAPJXwSssTTKqqPsrZ9YxgPLxsNt7tRsTHQKAg9RrYVQ5efWfM5unuKz9V3DRrxsgiiQrEYyzJb",
        "xNUrBof78pQtP3DFn4kv5qxheHzQh8LRCEVnYuWQv8y6RifFesFsekKMgvcuQnE45jARrhmFLTwqhW8hWRmKzMSF9xitUwJ67ZdGNg68iE7EtvBsNaRu5tstDFHS4wPS1wEWrNi24Cp",
        "xNVdeHme74VoSvihdobupHTGMJF9Wd4tqxEesE1wbeEtHED4VnMEFFEsXaoA1bADLm2w2LW11L35mv5UWUDXYbYAZXCyhrUKDXAT8EtgujFBqYBUzaahunDtUcYxGMesqNSyzxGsrQm"};

    for (size_t i = 0; i < 10; i++) {
        auto recvAddress = blsct::SubAddress(std::get<blsct::DoublePublicKey>(blsct_km->GetNewDestination(0).value()));
        BOOST_CHECK(recvAddress.GetString() == expectedAddresses[i]);
    }
}

BOOST_AUTO_TEST_SUITE_END()