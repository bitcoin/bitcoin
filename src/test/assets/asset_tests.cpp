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

        BOOST_CHECK(!IsAssetNameValid("_ABC"));
        BOOST_CHECK(!IsAssetNameValid("_ABC"));
        BOOST_CHECK(!IsAssetNameValid("ABC_"));
        BOOST_CHECK(!IsAssetNameValid(".ABC"));
        BOOST_CHECK(!IsAssetNameValid("ABC."));
        BOOST_CHECK(!IsAssetNameValid("AB..C"));
        BOOST_CHECK(!IsAssetNameValid("A__BC"));
        BOOST_CHECK(!IsAssetNameValid("A._BC"));
        BOOST_CHECK(!IsAssetNameValid("AB_.C"));

        //- Versions of RAVENCOIN NOT allowed
        BOOST_CHECK(!IsAssetNameValid("RVN"));
        BOOST_CHECK(!IsAssetNameValid("RAVEN"));
        BOOST_CHECK(!IsAssetNameValid("RAVENCOIN"));
        BOOST_CHECK(!IsAssetNameValid("RAVENC0IN"));
        BOOST_CHECK(!IsAssetNameValid("RAVENCO1N"));
        BOOST_CHECK(!IsAssetNameValid("RAVENC01N"));

        //- Versions of RAVENCOIN ALLOWED
        BOOST_CHECK(IsAssetNameValid("RAVEN.COIN"));
        BOOST_CHECK(IsAssetNameValid("RAVEN_COIN"));
        BOOST_CHECK(IsAssetNameValid("RVNSPYDER"));
        BOOST_CHECK(IsAssetNameValid("SPYDERRVN"));
        BOOST_CHECK(IsAssetNameValid("RAVENSPYDER"));
        BOOST_CHECK(IsAssetNameValid("SPYDERAVEN"));
        BOOST_CHECK(IsAssetNameValid("BLACK_RAVENS"));
        BOOST_CHECK(IsAssetNameValid("SERVNOT"));

        // subs
        BOOST_CHECK(IsAssetNameValid("ABC/A"));
        BOOST_CHECK(IsAssetNameValid("ABC/A/1"));
        BOOST_CHECK(IsAssetNameValid("ABC/A_1/1.A"));
        BOOST_CHECK(IsAssetNameValid("ABC/AB/XYZ/STILL/MAX/30/123456"));

        BOOST_CHECK(!IsAssetNameValid("ABC//MIN_1"));
        BOOST_CHECK(!IsAssetNameValid("ABC/"));
        BOOST_CHECK(!IsAssetNameValid("ABC/NOTRAIL/"));
        BOOST_CHECK(!IsAssetNameValid("ABC/_X"));
        BOOST_CHECK(!IsAssetNameValid("ABC/X_"));
        BOOST_CHECK(!IsAssetNameValid("ABC/.X"));
        BOOST_CHECK(!IsAssetNameValid("ABC/X."));
        BOOST_CHECK(!IsAssetNameValid("ABC/X__X"));
        BOOST_CHECK(!IsAssetNameValid("ABC/X..X"));
        BOOST_CHECK(!IsAssetNameValid("ABC/X_.X"));
        BOOST_CHECK(!IsAssetNameValid("ABC/X._X"));
        BOOST_CHECK(!IsAssetNameValid("ABC/nolower"));
        BOOST_CHECK(!IsAssetNameValid("ABC/NO SPACE"));
        BOOST_CHECK(!IsAssetNameValid("ABC/(*#^&$%)"));
        BOOST_CHECK(!IsAssetNameValid("ABC/AB/XYZ/STILL/MAX/30/OVERALL/1234"));

        // unique
        BOOST_CHECK(IsAssetNameValid("ABC#AZaz09"));
        BOOST_CHECK(IsAssetNameValid("ABC#@$%&*()[]{}<>-_.;?\\:"));
        BOOST_CHECK(!IsAssetNameValid("ABC#no!bangs"));
        BOOST_CHECK(IsAssetNameValid("ABC/THING#_STILL_30_MAX------_"));

        BOOST_CHECK(!IsAssetNameValid("MIN#"));
        BOOST_CHECK(!IsAssetNameValid("ABC#NO#HASH"));
        BOOST_CHECK(!IsAssetNameValid("ABC#NO SPACE"));
        BOOST_CHECK(!IsAssetNameValid("ABC#RESERVED/"));
        BOOST_CHECK(!IsAssetNameValid("ABC#RESERVED~"));
        BOOST_CHECK(!IsAssetNameValid("ABC#RESERVED^"));

        // channel
        BOOST_CHECK(IsAssetNameValid("ABC~1"));
        BOOST_CHECK(IsAssetNameValid("ABC~STILL_MAX_OF_30.CHARS_1234"));

        BOOST_CHECK(!IsAssetNameValid("MIN~"));
        BOOST_CHECK(!IsAssetNameValid("ABC~NO~TILDE"));
        BOOST_CHECK(!IsAssetNameValid("ABC~_ANN"));
        BOOST_CHECK(!IsAssetNameValid("ABC~ANN_"));
        BOOST_CHECK(!IsAssetNameValid("ABC~.ANN"));
        BOOST_CHECK(!IsAssetNameValid("ABC~ANN."));
        BOOST_CHECK(!IsAssetNameValid("ABC~X__X"));
        BOOST_CHECK(!IsAssetNameValid("ABC~X._X"));
        BOOST_CHECK(!IsAssetNameValid("ABC~X_.X"));
        BOOST_CHECK(!IsAssetNameValid("ABC~X..X"));
        BOOST_CHECK(!IsAssetNameValid("ABC~nolower"));

        // owner
        BOOST_CHECK(IsAssetNameAnOwner("ABC!"));
        BOOST_CHECK(!IsAssetNameAnOwner("ABC"));
        BOOST_CHECK(!IsAssetNameAnOwner("ABC!COIN"));
        BOOST_CHECK(IsAssetNameAnOwner("MAX_ASSET_IS_30_CHARACTERS_LNG!"));
        BOOST_CHECK(!IsAssetNameAnOwner("MAX_ASSET_IS_31_CHARACTERS_LONG!"));
        BOOST_CHECK(IsAssetNameAnOwner("ABC/A!"));
        BOOST_CHECK(IsAssetNameAnOwner("ABC/A/1!"));
        BOOST_CHECK(IsAssetNameValid("ABC#AZaz09"));


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
