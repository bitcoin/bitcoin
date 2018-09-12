// Copyright (c) 2017 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <assets/assets.h>

#include <test/test_raven.h>

#include <boost/test/unit_test.hpp>
#include "core_write.cpp"

#include <amount.h>
#include <base58.h>
#include <chainparams.h>

BOOST_FIXTURE_TEST_SUITE(asset_tests, BasicTestingSetup)

    BOOST_AUTO_TEST_CASE(unit_validation_tests) {
        BOOST_CHECK(IsAssetUnitsValid(COIN));
        BOOST_CHECK(IsAssetUnitsValid(CENT));
    }

    BOOST_AUTO_TEST_CASE(name_validation_tests) {
        AssetType type;
    
        // regular
        BOOST_CHECK(IsAssetNameValid("MIN", type));
        BOOST_CHECK(type == AssetType::ROOT);
        BOOST_CHECK(IsAssetNameValid("MAX_ASSET_IS_30_CHARACTERS_LNG", type));
        BOOST_CHECK(!IsAssetNameValid("MAX_ASSET_IS_31_CHARACTERS_LONG", type));
        BOOST_CHECK(type == AssetType::INVALID);
        BOOST_CHECK(IsAssetNameValid("A_BCDEFGHIJKLMNOPQRSTUVWXY.Z", type));
        BOOST_CHECK(IsAssetNameValid("0_12345678.9", type));

        BOOST_CHECK(!IsAssetNameValid("NO", type));
        BOOST_CHECK(!IsAssetNameValid("nolower", type));
        BOOST_CHECK(!IsAssetNameValid("NO SPACE", type));
        BOOST_CHECK(!IsAssetNameValid("(#&$(&*^%$))", type));

        BOOST_CHECK(!IsAssetNameValid("_ABC", type));
        BOOST_CHECK(!IsAssetNameValid("_ABC", type));
        BOOST_CHECK(!IsAssetNameValid("ABC_", type));
        BOOST_CHECK(!IsAssetNameValid(".ABC", type));
        BOOST_CHECK(!IsAssetNameValid("ABC.", type));
        BOOST_CHECK(!IsAssetNameValid("AB..C", type));
        BOOST_CHECK(!IsAssetNameValid("A__BC", type));
        BOOST_CHECK(!IsAssetNameValid("A._BC", type));
        BOOST_CHECK(!IsAssetNameValid("AB_.C", type));

        //- Versions of RAVENCOIN NOT allowed
        BOOST_CHECK(!IsAssetNameValid("RVN", type));
        BOOST_CHECK(!IsAssetNameValid("RAVEN", type));
        BOOST_CHECK(!IsAssetNameValid("RAVENCOIN", type));
        BOOST_CHECK(!IsAssetNameValid("RAVENC0IN", type));
        BOOST_CHECK(!IsAssetNameValid("RAVENCO1N", type));
        BOOST_CHECK(!IsAssetNameValid("RAVENC01N", type));

        //- Versions of RAVENCOIN ALLOWED
        BOOST_CHECK(IsAssetNameValid("RAVEN.COIN", type));
        BOOST_CHECK(IsAssetNameValid("RAVEN_COIN", type));
        BOOST_CHECK(IsAssetNameValid("RVNSPYDER", type));
        BOOST_CHECK(IsAssetNameValid("SPYDERRVN", type));
        BOOST_CHECK(IsAssetNameValid("RAVENSPYDER", type));
        BOOST_CHECK(IsAssetNameValid("SPYDERAVEN", type));
        BOOST_CHECK(IsAssetNameValid("BLACK_RAVENS", type));
        BOOST_CHECK(IsAssetNameValid("SERVNOT", type));

        // subs
        BOOST_CHECK(IsAssetNameValid("ABC/A", type));
        BOOST_CHECK(type == AssetType::SUB);
        BOOST_CHECK(IsAssetNameValid("ABC/A/1", type));
        BOOST_CHECK(IsAssetNameValid("ABC/A_1/1.A", type));
        BOOST_CHECK(IsAssetNameValid("ABC/AB/XYZ/STILL/MAX/30/123456", type));

        BOOST_CHECK(!IsAssetNameValid("ABC//MIN_1", type));
        BOOST_CHECK(!IsAssetNameValid("ABC/", type));
        BOOST_CHECK(!IsAssetNameValid("ABC/NOTRAIL/", type));
        BOOST_CHECK(!IsAssetNameValid("ABC/_X", type));
        BOOST_CHECK(!IsAssetNameValid("ABC/X_", type));
        BOOST_CHECK(!IsAssetNameValid("ABC/.X", type));
        BOOST_CHECK(!IsAssetNameValid("ABC/X.", type));
        BOOST_CHECK(!IsAssetNameValid("ABC/X__X", type));
        BOOST_CHECK(!IsAssetNameValid("ABC/X..X", type));
        BOOST_CHECK(!IsAssetNameValid("ABC/X_.X", type));
        BOOST_CHECK(!IsAssetNameValid("ABC/X._X", type));
        BOOST_CHECK(!IsAssetNameValid("ABC/nolower", type));
        BOOST_CHECK(!IsAssetNameValid("ABC/NO SPACE", type));
        BOOST_CHECK(!IsAssetNameValid("ABC/(*#^&$%)", type));
        BOOST_CHECK(!IsAssetNameValid("ABC/AB/XYZ/STILL/MAX/30/OVERALL/1234", type));

        // unique
        BOOST_CHECK(IsAssetNameValid("ABC#AZaz09", type));
        BOOST_CHECK(type == AssetType::UNIQUE);
        BOOST_CHECK(IsAssetNameValid("ABC#abc123ABC@$%&*()[]{}-_.?:", type));
        BOOST_CHECK(!IsAssetNameValid("ABC#no!bangs", type));
        BOOST_CHECK(IsAssetNameValid("ABC/THING#_STILL_31_MAX-------_", type));

        BOOST_CHECK(!IsAssetNameValid("MIN#", type));
        BOOST_CHECK(!IsAssetNameValid("ABC#NO#HASH", type));
        BOOST_CHECK(!IsAssetNameValid("ABC#NO SPACE", type));
        BOOST_CHECK(!IsAssetNameValid("ABC#RESERVED/", type));
        BOOST_CHECK(!IsAssetNameValid("ABC#RESERVED~", type));
        BOOST_CHECK(!IsAssetNameValid("ABC#RESERVED^", type));

        // channel
        BOOST_CHECK(IsAssetNameValid("ABC~1", type));
        BOOST_CHECK(type == AssetType::MSGCHANNEL);
        BOOST_CHECK(IsAssetNameValid("ABC~MAX_OF_12_CR", type));
        BOOST_CHECK(!IsAssetNameValid("ABC~MAX_OF_12_CHR", type));
        BOOST_CHECK(IsAssetNameValid("TEST/TEST~CHANNEL", type));
        BOOST_CHECK(type == AssetType::MSGCHANNEL);

        BOOST_CHECK(!IsAssetNameValid("MIN~", type));
        BOOST_CHECK(!IsAssetNameValid("ABC~NO~TILDE", type));
        BOOST_CHECK(!IsAssetNameValid("ABC~_ANN", type));
        BOOST_CHECK(!IsAssetNameValid("ABC~ANN_", type));
        BOOST_CHECK(!IsAssetNameValid("ABC~.ANN", type));
        BOOST_CHECK(!IsAssetNameValid("ABC~ANN.", type));
        BOOST_CHECK(!IsAssetNameValid("ABC~X__X", type));
        BOOST_CHECK(!IsAssetNameValid("ABC~X._X", type));
        BOOST_CHECK(!IsAssetNameValid("ABC~X_.X", type));
        BOOST_CHECK(!IsAssetNameValid("ABC~X..X", type));
        BOOST_CHECK(!IsAssetNameValid("ABC~nolower", type));

        // owner
        BOOST_CHECK(IsAssetNameAnOwner("ABC!"));
        BOOST_CHECK(!IsAssetNameAnOwner("ABC"));
        BOOST_CHECK(!IsAssetNameAnOwner("ABC!COIN"));
        BOOST_CHECK(IsAssetNameAnOwner("MAX_ASSET_IS_30_CHARACTERS_LNG!"));
        BOOST_CHECK(!IsAssetNameAnOwner("MAX_ASSET_IS_31_CHARACTERS_LONG!"));
        BOOST_CHECK(IsAssetNameAnOwner("ABC/A!"));
        BOOST_CHECK(IsAssetNameAnOwner("ABC/A/1!"));
        BOOST_CHECK(IsAssetNameValid("ABC!", type));
        BOOST_CHECK(type == AssetType::OWNER);

        // vote
        BOOST_CHECK(IsAssetNameValid("ABC^VOTE"));
        BOOST_CHECK(!IsAssetNameValid("ABC^"));
        BOOST_CHECK(IsAssetNameValid("ABC^VOTING"));
        BOOST_CHECK(IsAssetNameValid("ABC^VOTING_IS_30_CHARACTERS_LN"));
        BOOST_CHECK(!IsAssetNameValid("ABC^VOTING_IS_31_CHARACTERS_LN!"));
        BOOST_CHECK(IsAssetNameValid("ABC/SUB/SUB/SUB/SUB^VOTE"));
        BOOST_CHECK(IsAssetNameValid("ABC/SUB/SUB/SUB/SUB/SUB/30^VOT"));
        BOOST_CHECK(IsAssetNameValid("ABC/SUB/SUB/SUB/SUB/SUB/31^VOTE"));
        BOOST_CHECK(!IsAssetNameValid("ABC/SUB/SUB/SUB/SUB/SUB/32X^VOTE"));
        BOOST_CHECK(IsAssetNameValid("ABC/SUB/SUB^VOTE", type));
        BOOST_CHECK(type == AssetType::VOTE);

        // Check type for different type of sub assets
        BOOST_CHECK(IsAssetNameValid("TEST/UYTH#UNIQUE", type));
        BOOST_CHECK(type == AssetType::UNIQUE);

        BOOST_CHECK(IsAssetNameValid("TEST/UYTH/SUB#UNIQUE", type));
        BOOST_CHECK(type == AssetType::UNIQUE);

        BOOST_CHECK(IsAssetNameValid("TEST/UYTH/SUB~CHANNEL", type));
        BOOST_CHECK(type == AssetType::MSGCHANNEL);

        BOOST_CHECK(!IsAssetNameValid("TEST/UYTH/SUB#UNIQUE^VOTE", type));
        BOOST_CHECK(!IsAssetNameValid("TEST/UYTH/SUB#UNIQUE#UNIQUE", type));
        BOOST_CHECK(!IsAssetNameValid("TEST/UYTH/SUB~CHANNEL^VOTE", type));
        BOOST_CHECK(!IsAssetNameValid("TEST/UYTH/SUB~CHANNEL^UNIQUE", type));
        BOOST_CHECK(!IsAssetNameValid("TEST/UYTH/SUB~CHANNEL!", type));
        BOOST_CHECK(!IsAssetNameValid("TEST/UYTH/SUB^VOTE!", type));

        // Check ParentName function
        BOOST_CHECK(GetParentName("TEST!") == "TEST!");
        BOOST_CHECK(GetParentName("TEST") == "TEST");
        BOOST_CHECK(GetParentName("TEST/SUB") == "TEST");
        BOOST_CHECK(GetParentName("TEST/SUB#UNIQUE") == "TEST/SUB");
        BOOST_CHECK(GetParentName("TEST/TEST/SUB/SUB") == "TEST/TEST/SUB");
        BOOST_CHECK(GetParentName("TEST/SUB^VOTE") == "TEST/SUB");
        BOOST_CHECK(GetParentName("TEST/SUB/SUB~CHANNEL") == "TEST/SUB/SUB");
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

    BOOST_AUTO_TEST_CASE(dwg_version_check) {


        int32_t version = 0x30000000;
        int32_t mask = 0xF0000000;
        int32_t new_version = version & mask;
        int32_t shifted = new_version >> 28;

        BOOST_CHECK_MESSAGE(shifted == 3, "New version didn't equal 3");
    }

    BOOST_AUTO_TEST_CASE(asset_formatting) {

        CAmount amount = 50000010000;
        BOOST_CHECK(ValueFromAmountString(amount, 4) == "500.0001");

        amount = 100;
        BOOST_CHECK(ValueFromAmountString(amount, 6) == "0.000001");

        amount = 1000;
        BOOST_CHECK(ValueFromAmountString(amount, 6) == "0.000010");

        amount = 50010101010;
        BOOST_CHECK(ValueFromAmountString(amount, 8) == "500.10101010");

        amount = 111111111;
        BOOST_CHECK(ValueFromAmountString(amount, 8) == "1.11111111");

        amount = 1;
        BOOST_CHECK(ValueFromAmountString(amount, 8) == "0.00000001");

        amount = 40000000;
        BOOST_CHECK(ValueFromAmountString(amount, 8) == "0.40000000");

    }

BOOST_AUTO_TEST_SUITE_END()
