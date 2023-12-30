// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/mcl/mcl.h>
#include <blsct/double_public_key.h>
#include <blsct/wallet/address.h>
#include <test/util/setup_common.h>
#include <uint256.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(blsct_address_tests)

using Scalar = MclScalar;

BOOST_FIXTURE_TEST_CASE(address_test, BasicTestingSetup)
{
    SelectParams(ChainType::MAIN);

    std::string privateViewKeyString("2c12e402aee1c7afb1dc32f1040bf4f14db8200ce4b97e95e85fc3e0edb713d");
    std::string privateSpendKeyString("6063199b4c1460d7e9715883e2c053ffecddc0a663cdf2276c7244b590abf577");

    blsct::PrivateKey privateViewKey(Scalar(privateViewKeyString, 16));
    blsct::PrivateKey privateSpendKey(Scalar(privateSpendKeyString, 16));

    blsct::SubAddress subAddress(privateViewKey, privateSpendKey.GetPublicKey(), {0, 0});

    blsct::DoublePublicKey subAddressDoubleKey = subAddress.GetKeys();

    blsct::PublicKey viewKey;
    blsct::PublicKey spendKey;

    BOOST_ASSERT(subAddressDoubleKey.GetViewKey(viewKey));
    BOOST_ASSERT(subAddressDoubleKey.GetSpendKey(spendKey));

    BOOST_CHECK(viewKey.ToString() == "809ad665b3de4e1d44d835f1b8de36aafeea3279871aeceb56dbdda90c0426c022e8a6dda7313dc5e4c1817287805e3b");

    BOOST_CHECK(spendKey.ToString() == "8258aadbb42f57dae65587ac18b164b538e3886ab9f0f350c96331ac7de0c2599eb88ac1464a0b0d8dda01bcf32e5dd4");

    BOOST_CHECK(subAddressDoubleKey.GetID().ToString() == "dc103f3afbfa5fccf51bfcba8a65fb14ddd0c1a7");

    BOOST_CHECK(subAddress.GetString() == "nv1szddvednme8p63xcxhcm3h3k4tlw5vnesudwe66km0w6jrqyymqz969xmknnz0w9unqczu58sp0rhqjc4tdmgt6hmtn9tpavrzckfdfcuwyx4w0s7dgvjce34377psjen6ug4s2xfg9smrw6qx70xtja6s8wrt28dc");
}

BOOST_AUTO_TEST_SUITE_END()