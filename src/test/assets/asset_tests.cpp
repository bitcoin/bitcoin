// Copyright (c) 2017 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <assets/assets.h>

#include <test/test_raven.h>

#include <boost/test/unit_test.hpp>

#include <amount.h>
#include <base58.h>
#include <chainparams.h>

BOOST_FIXTURE_TEST_SUITE(asset_tests, BasicTestingSetup)

    BOOST_AUTO_TEST_CASE(unit_validation_tests) {
        BOOST_CHECK(IsAssetUnitsValid(COIN));
        BOOST_CHECK(IsAssetUnitsValid(CENT));
    }

    BOOST_AUTO_TEST_CASE(name_validation_tests) {
        // regular
        BOOST_CHECK(IsAssetNameValid("MIN"));
        BOOST_CHECK(IsAssetNameValid("MAX_ASSET_IS_30_CHARACTERS_LNG"));
        BOOST_CHECK(!IsAssetNameValid("MAX_ASSET_IS_31_CHARACTERS_LONG"));
        BOOST_CHECK(IsAssetNameValid("A_BCDEFGHIJKLMNOPQRSTUVWXY.Z"));
        BOOST_CHECK(IsAssetNameValid("0_12345678.9"));

        BOOST_CHECK(!IsAssetNameValid("NO"));
        BOOST_CHECK(!IsAssetNameValid("nolower"));
        BOOST_CHECK(!IsAssetNameValid("NO SPACE"));
        BOOST_CHECK(!IsAssetNameValid("(#&$(&*^%$))"));

        BOOST_CHECK(!IsAssetNameValid("_RVN"));
        BOOST_CHECK(!IsAssetNameValid("_RVN"));
        BOOST_CHECK(!IsAssetNameValid("RVN_"));
        BOOST_CHECK(!IsAssetNameValid(".RVN"));
        BOOST_CHECK(!IsAssetNameValid("RVN."));
        BOOST_CHECK(!IsAssetNameValid("RV..N"));
        BOOST_CHECK(!IsAssetNameValid("R__VN"));
        BOOST_CHECK(!IsAssetNameValid("R._VN"));
        BOOST_CHECK(!IsAssetNameValid("RV_.N"));

        // subs
        BOOST_CHECK(IsAssetNameValid("RVN/A"));
        BOOST_CHECK(IsAssetNameValid("RVN/A/1"));
        BOOST_CHECK(IsAssetNameValid("RVN/A_1/1.A"));
        BOOST_CHECK(IsAssetNameValid("RVN/AB/XYZ/STILL/MAX/30/123456"));

        BOOST_CHECK(!IsAssetNameValid("RVN//MIN_1"));
        BOOST_CHECK(!IsAssetNameValid("RVN/"));
        BOOST_CHECK(!IsAssetNameValid("RVN/NOTRAIL/"));
        BOOST_CHECK(!IsAssetNameValid("RVN/_X"));
        BOOST_CHECK(!IsAssetNameValid("RVN/X_"));
        BOOST_CHECK(!IsAssetNameValid("RVN/.X"));
        BOOST_CHECK(!IsAssetNameValid("RVN/X."));
        BOOST_CHECK(!IsAssetNameValid("RVN/X__X"));
        BOOST_CHECK(!IsAssetNameValid("RVN/X..X"));
        BOOST_CHECK(!IsAssetNameValid("RVN/X_.X"));
        BOOST_CHECK(!IsAssetNameValid("RVN/X._X"));
        BOOST_CHECK(!IsAssetNameValid("RVN/nolower"));
        BOOST_CHECK(!IsAssetNameValid("RVN/NO SPACE"));
        BOOST_CHECK(!IsAssetNameValid("RVN/(*#^&$%)"));
        BOOST_CHECK(!IsAssetNameValid("RVN/AB/XYZ/STILL/MAX/30/OVERALL/1234"));

        // unique
        BOOST_CHECK(IsAssetNameValid("RVN#AZaz09"));
        BOOST_CHECK(IsAssetNameValid("RVN#@$%&*()[]{}<>-_.;?\\:"));
        BOOST_CHECK(!IsAssetNameValid("RVN#no!bangs"));
        BOOST_CHECK(IsAssetNameValid("RVN/THING#_STILL_30_MAX------_"));

        BOOST_CHECK(!IsAssetNameValid("MIN#"));
        BOOST_CHECK(!IsAssetNameValid("RVN#NO#HASH"));
        BOOST_CHECK(!IsAssetNameValid("RVN#NO SPACE"));
        BOOST_CHECK(!IsAssetNameValid("RVN#RESERVED/"));
        BOOST_CHECK(!IsAssetNameValid("RVN#RESERVED~"));
        BOOST_CHECK(!IsAssetNameValid("RVN#RESERVED^"));

        // channel
        BOOST_CHECK(IsAssetNameValid("RVN~1"));
        BOOST_CHECK(IsAssetNameValid("RVN~STILL_MAX_OF_30.CHARS_1234"));

        BOOST_CHECK(!IsAssetNameValid("MIN~"));
        BOOST_CHECK(!IsAssetNameValid("RVN~NO~TILDE"));
        BOOST_CHECK(!IsAssetNameValid("RVN~_ANN"));
        BOOST_CHECK(!IsAssetNameValid("RVN~ANN_"));
        BOOST_CHECK(!IsAssetNameValid("RVN~.ANN"));
        BOOST_CHECK(!IsAssetNameValid("RVN~ANN."));
        BOOST_CHECK(!IsAssetNameValid("RVN~X__X"));
        BOOST_CHECK(!IsAssetNameValid("RVN~X._X"));
        BOOST_CHECK(!IsAssetNameValid("RVN~X_.X"));
        BOOST_CHECK(!IsAssetNameValid("RVN~X..X"));
        BOOST_CHECK(!IsAssetNameValid("RVN~nolower"));

        // owner
        BOOST_CHECK(IsAssetNameAnOwner("RVN!"));
        BOOST_CHECK(!IsAssetNameAnOwner("RVN"));
        BOOST_CHECK(!IsAssetNameAnOwner("RVN!COIN"));
        BOOST_CHECK(IsAssetNameAnOwner("MAX_ASSET_IS_30_CHARACTERS_LNG!"));
        BOOST_CHECK(!IsAssetNameAnOwner("MAX_ASSET_IS_31_CHARACTERS_LONG!"));
        BOOST_CHECK(IsAssetNameAnOwner("RVN/A!"));
        BOOST_CHECK(IsAssetNameAnOwner("RVN/A/1!"));
        BOOST_CHECK(IsAssetNameValid("RVN#AZaz09"));


    }

    BOOST_AUTO_TEST_CASE(transfer_asset_coin_check) {

        SelectParams(CBaseChainParams::MAIN);

        // Create the asset scriptPubKey
        CAssetTransfer asset("RAVEN", 1000);
        CScript scriptPubKey = GetScriptForDestination(DecodeDestination(Params().GlobalBurnAddress()));
        asset.ConstructTransaction(scriptPubKey);

        CTxOut txOut;
        txOut.nValue = 0;
        txOut.scriptPubKey = scriptPubKey;


        Coin coin(txOut, 0, 0);

        BOOST_CHECK_MESSAGE(coin.IsAsset(), "Transfer Asset Coin isn't as asset");
    }

    BOOST_AUTO_TEST_CASE(new_asset_coin_check) {

        SelectParams(CBaseChainParams::MAIN);

        // Create the asset scriptPubKey
        CNewAsset asset("RAVEN", 1000, 8, 1, 0, "");
        CScript scriptPubKey = GetScriptForDestination(DecodeDestination(Params().GlobalBurnAddress()));
        asset.ConstructTransaction(scriptPubKey);

        CTxOut txOut;
        txOut.nValue = 0;
        txOut.scriptPubKey = scriptPubKey;

        Coin coin(txOut, 0, 0);

        BOOST_CHECK_MESSAGE(coin.IsAsset(), "New Asset Coin isn't as asset");
    }

BOOST_AUTO_TEST_SUITE_END()
