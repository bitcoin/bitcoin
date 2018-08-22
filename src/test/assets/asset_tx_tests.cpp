// Copyright (c) 2017 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <assets/assets.h>

#include <test/test_raven.h>

#include <boost/test/unit_test.hpp>

#include <amount.h>
#include <script/standard.h>
#include <base58.h>
#include <consensus/validation.h>
#include <consensus/tx_verify.h>

BOOST_FIXTURE_TEST_SUITE(asset_tx_tests, BasicTestingSetup)

    BOOST_AUTO_TEST_CASE(asset_tx_valid) {

        SelectParams(CBaseChainParams::MAIN);

        // Create the asset scriptPubKey
        CAssetTransfer asset("RAVEN", 1000);
        CScript scriptPubKey = GetScriptForDestination(DecodeDestination(Params().GlobalBurnAddress()));
        asset.ConstructTransaction(scriptPubKey);

        CCoinsView view;
        CCoinsViewCache coins(&view);

        // Create CTxOut and add it to a coin
        CTxOut txOut;
        txOut.nValue = 0;
        txOut.scriptPubKey = scriptPubKey;

        // Create a random hash
        uint256 hash = uint256S("BF50CB9A63BE0019171456252989A459A7D0A5F494735278290079D22AB704A2");

        // Add the coin to the cache
        COutPoint outpoint(hash, 1);
        coins.AddCoin(outpoint, Coin(txOut, 10, 0), true);

        // Create transaction and input for the outpoint of the coin we just created
        CMutableTransaction mutTx;

        CTxIn in;
        in.prevout = outpoint;

        // Add the input, and an output into the transaction
        mutTx.vin.emplace_back(in);
        mutTx.vout.emplace_back(txOut);

        CTransaction tx(mutTx);
        CValidationState state;

        // The inputs are spending 1000 Assets
        // The outputs are assigning a destination to 1000 Assets
        // This test should pass because all assets are assigned a destination
        std::vector<std::pair<std::string, uint256>> vReissueAssets;
        BOOST_CHECK_MESSAGE(Consensus::CheckTxAssets(tx, state, coins, vReissueAssets, true), "CheckTxAssets Failed");
    }

    BOOST_AUTO_TEST_CASE(asset_tx_not_valid) {

        SelectParams(CBaseChainParams::MAIN);

        // Create the asset scriptPubKey
        CAssetTransfer asset("RAVEN", 1000);
        CScript scriptPubKey = GetScriptForDestination(DecodeDestination(Params().GlobalBurnAddress()));
        asset.ConstructTransaction(scriptPubKey);

        CCoinsView view;
        CCoinsViewCache coins(&view);

        // Create CTxOut and add it to a coin
        CTxOut txOut;
        txOut.nValue = 0;
        txOut.scriptPubKey = scriptPubKey;

        // Create a random hash
        uint256 hash = uint256S("BF50CB9A63BE0019171456252989A459A7D0A5F494735278290079D22AB704A2");

        // Add the coin to the cache
        COutPoint outpoint(hash, 1);
        coins.AddCoin(outpoint, Coin(txOut, 10, 0), true);

        // Create transaction and input for the outpoint of the coin we just created
        CMutableTransaction mutTx;

        CTxIn in;
        in.prevout = outpoint;

        // Create CTxOut that will only send 100 of the asset
        // This should fail because 900 RAVEN doesn't have a destination
        CAssetTransfer assetTransfer("RAVEN", 100);
        CScript scriptLess = GetScriptForDestination(DecodeDestination(Params().GlobalBurnAddress()));
        assetTransfer.ConstructTransaction(scriptLess);

        CTxOut txOut2;
        txOut2.nValue = 0;
        txOut2.scriptPubKey = scriptLess;

        // Add the input, and an output into the transaction
        mutTx.vin.emplace_back(in);
        mutTx.vout.emplace_back(txOut2);

        CTransaction tx(mutTx);
        CValidationState state;

        // The inputs of this transaction are spending 1000 Assets
        // The outputs are assigning a destination to only 100 Assets
        // This should fail because 900 Assets aren't being assigned a destination (Trying to burn 900 Assets)
        std::vector<std::pair<std::string, uint256>> vReissueAssets;
        BOOST_CHECK_MESSAGE(!Consensus::CheckTxAssets(tx, state, coins, vReissueAssets, true), "CheckTxAssets should of failed");
    }

    BOOST_AUTO_TEST_CASE(asset_tx_valid_multiple_outs) {

        SelectParams(CBaseChainParams::MAIN);

        // Create the asset scriptPubKey
        CAssetTransfer asset("RAVEN", 1000);
        CScript scriptPubKey = GetScriptForDestination(DecodeDestination(Params().GlobalBurnAddress()));
        asset.ConstructTransaction(scriptPubKey);

        CCoinsView view;
        CCoinsViewCache coins(&view);

        // Create CTxOut and add it to a coin
        CTxOut txOut;
        txOut.nValue = 0;
        txOut.scriptPubKey = scriptPubKey;

        // Create a random hash
        uint256 hash = uint256S("BF50CB9A63BE0019171456252989A459A7D0A5F494735278290079D22AB704A2");

        // Add the coin to the cache
        COutPoint outpoint(hash, 1);
        coins.AddCoin(outpoint, Coin(txOut, 10, 0), true);

        // Create transaction and input for the outpoint of the coin we just created
        CMutableTransaction mutTx;

        CTxIn in;
        in.prevout = outpoint;

        // Create CTxOut that will only send 100 of the asset 10 times total = 1000
        for (int i = 0; i < 10; i++) {
            CAssetTransfer asset2("RAVEN", 100);
            CScript scriptPubKey2 = GetScriptForDestination(DecodeDestination(Params().GlobalBurnAddress()));
            asset2.ConstructTransaction(scriptPubKey2);

            CTxOut txOut2;
            txOut2.nValue = 0;
            txOut2.scriptPubKey = scriptPubKey2;

            // Add the output into the transaction
            mutTx.vout.emplace_back(txOut2);
        }

        // Add the input, and an output into the transaction
        mutTx.vin.emplace_back(in);

        CTransaction tx(mutTx);
        CValidationState state;

        // The inputs are spending 1000 Assets
        // The outputs are assigned 100 Assets to 10 destinations (10 * 100) = 1000
        // This test should pass all assets that are being spent are assigned to a destination
        std::vector<std::pair<std::string, uint256>> vReissueAssets;
        BOOST_CHECK_MESSAGE(Consensus::CheckTxAssets(tx, state, coins, vReissueAssets, true), "CheckTxAssets failed");
    }

    BOOST_AUTO_TEST_CASE(asset_tx_multiple_outs_invalid) {

        SelectParams(CBaseChainParams::MAIN);

        // Create the asset scriptPubKey
        CAssetTransfer asset("RAVEN", 1000);
        CScript scriptPubKey = GetScriptForDestination(DecodeDestination(Params().GlobalBurnAddress()));
        asset.ConstructTransaction(scriptPubKey);

        CCoinsView view;
        CCoinsViewCache coins(&view);

        // Create CTxOut and add it to a coin
        CTxOut txOut;
        txOut.nValue = 0;
        txOut.scriptPubKey = scriptPubKey;

        // Create a random hash
        uint256 hash = uint256S("BF50CB9A63BE0019171456252989A459A7D0A5F494735278290079D22AB704A2");

        // Add the coin to the cache
        COutPoint outpoint(hash, 1);
        coins.AddCoin(outpoint, Coin(txOut, 10, 0), true);

        // Create transaction and input for the outpoint of the coin we just created
        CMutableTransaction mutTx;

        CTxIn in;
        in.prevout = outpoint;

        // Create CTxOut that will only send 100 of the asset 12 times, total = 1200
        for (int i = 0; i < 12; i++) {
            CAssetTransfer asset2("RAVEN", 100);
            CScript scriptPubKey2 = GetScriptForDestination(DecodeDestination(Params().GlobalBurnAddress()));
            asset2.ConstructTransaction(scriptPubKey2);

            CTxOut txOut2;
            txOut2.nValue = 0;
            txOut2.scriptPubKey = scriptPubKey2;

            // Add the output into the transaction
            mutTx.vout.emplace_back(txOut2);
        }

        // Add the input, and an output into the transaction
        mutTx.vin.emplace_back(in);

        CTransaction tx(mutTx);
        CValidationState state;

        // The inputs are spending 1000 Assets
        // The outputs are assigning 100 Assets to 12 destinations (12 * 100 = 1200)
        // This test should fail because the Outputs are greater than the inputs
        std::vector<std::pair<std::string, uint256>> vReissueAssets;
        BOOST_CHECK_MESSAGE(!Consensus::CheckTxAssets(tx, state, coins, vReissueAssets, true), "CheckTxAssets passed when it should of failed");
    }

    BOOST_AUTO_TEST_CASE(asset_tx_multiple_assets) {

        SelectParams(CBaseChainParams::MAIN);

        // Create the asset scriptPubKeys
        CAssetTransfer asset("RAVEN", 1000);
        CScript scriptPubKey = GetScriptForDestination(DecodeDestination(Params().GlobalBurnAddress()));
        asset.ConstructTransaction(scriptPubKey);

        CAssetTransfer asset2("RAVENTEST", 1000);
        CScript scriptPubKey2 = GetScriptForDestination(DecodeDestination(Params().GlobalBurnAddress()));
        asset2.ConstructTransaction(scriptPubKey2);

        CAssetTransfer asset3("RAVENTESTTEST", 1000);
        CScript scriptPubKey3 = GetScriptForDestination(DecodeDestination(Params().GlobalBurnAddress()));
        asset3.ConstructTransaction(scriptPubKey3);

        CCoinsView view;
        CCoinsViewCache coins(&view);

        // Create CTxOuts
        CTxOut txOut;
        txOut.nValue = 0;
        txOut.scriptPubKey = scriptPubKey;

        CTxOut txOut2;
        txOut2.nValue = 0;
        txOut2.scriptPubKey = scriptPubKey2;

        CTxOut txOut3;
        txOut3.nValue = 0;
        txOut3.scriptPubKey = scriptPubKey3;

        // Create a random hash
        uint256 hash = uint256S("BF50CB9A63BE0019171456252989A459A7D0A5F494735278290079D22AB704A2");
        uint256 hash2 = uint256S("BF50CB9A63BE0019171456252989A459A7D0A5F494735278290079D22AB704A3");
        uint256 hash3 = uint256S("BF50CB9A63BE0019171456252989A459A7D0A5F494735278290079D22AB704A4");

        // Add the coins to the cache
        COutPoint outpoint(hash, 1);
        coins.AddCoin(outpoint, Coin(txOut, 10, 0), true);

        COutPoint outpoint2(hash2, 1);
        coins.AddCoin(outpoint2, Coin(txOut2, 10, 0), true);

        COutPoint outpoint3(hash3, 1);
        coins.AddCoin(outpoint3, Coin(txOut3, 10, 0), true);

        Coin coinTemp;
        BOOST_CHECK_MESSAGE(coins.GetCoin(outpoint, coinTemp), "Failed to get coin 1");
        BOOST_CHECK_MESSAGE(coins.GetCoin(outpoint2, coinTemp), "Failed to get coin 2");
        BOOST_CHECK_MESSAGE(coins.GetCoin(outpoint3, coinTemp), "Failed to get coin 3");

        // Create transaction and input for the outpoint of the coin we just created
        CMutableTransaction mutTx;

        CTxIn in;
        in.prevout = outpoint;

        CTxIn in2;
        in2.prevout = outpoint2;

        CTxIn in3;
        in3.prevout = outpoint3;

        // Create CTxOut for each asset that spends 100 assets 10 time = 1000 asset in total
        for (int i = 0; i < 10; i++) {
            // Add the first asset
            CAssetTransfer outAsset("RAVEN", 100);
            CScript outScript = GetScriptForDestination(DecodeDestination(Params().GlobalBurnAddress()));
            outAsset.ConstructTransaction(outScript);

            CTxOut txOutNew;
            txOutNew.nValue = 0;
            txOutNew.scriptPubKey = outScript;

            mutTx.vout.emplace_back(txOutNew);

            // Add the second asset
            CAssetTransfer outAsset2("RAVENTEST", 100);
            CScript outScript2 = GetScriptForDestination(DecodeDestination(Params().GlobalBurnAddress()));
            outAsset2.ConstructTransaction(outScript2);

            CTxOut txOutNew2;
            txOutNew2.nValue = 0;
            txOutNew2.scriptPubKey = outScript2;

            mutTx.vout.emplace_back(txOutNew2);

            // Add the third asset
            CAssetTransfer outAsset3("RAVENTESTTEST", 100);
            CScript outScript3 = GetScriptForDestination(DecodeDestination(Params().GlobalBurnAddress()));
            outAsset3.ConstructTransaction(outScript3);

            CTxOut txOutNew3;
            txOutNew3.nValue = 0;
            txOutNew3.scriptPubKey = outScript3;

            mutTx.vout.emplace_back(txOutNew3);
        }

        // Add the inputs
        mutTx.vin.emplace_back(in);
        mutTx.vin.emplace_back(in2);
        mutTx.vin.emplace_back(in3);

        CTransaction tx(mutTx);
        CValidationState state;

        // The inputs are spending 3000 Assets (1000 of each RAVEN, RAVENTEST, RAVENTESTTEST)
        // The outputs are spending 100 Assets to 10 destinations (10 * 100 = 1000) (of each RAVEN, RAVENTEST, RAVENTESTTEST)
        // This test should pass because for each asset that is spent. It is assigned a destination
        std::vector<std::pair<std::string, uint256>> vReissueAssets;
        BOOST_CHECK_MESSAGE(Consensus::CheckTxAssets(tx, state, coins, vReissueAssets, true), "CheckTxAssets Failed");


        // Try it not but only spend 900 of each asset instead of 1000
        CMutableTransaction mutTx2;

        // Create CTxOut for each asset that spends 100 assets 9 time = 900 asset in total
        for (int i = 0; i < 9; i++) {
            // Add the first asset
            CAssetTransfer outAsset("RAVEN", 100);
            CScript outScript = GetScriptForDestination(DecodeDestination(Params().GlobalBurnAddress()));
            outAsset.ConstructTransaction(outScript);

            CTxOut txOutNew;
            txOutNew.nValue = 0;
            txOutNew.scriptPubKey = outScript;

            mutTx2.vout.emplace_back(txOutNew);

            // Add the second asset
            CAssetTransfer outAsset2("RAVENTEST", 100);
            CScript outScript2 = GetScriptForDestination(DecodeDestination(Params().GlobalBurnAddress()));
            outAsset2.ConstructTransaction(outScript2);

            CTxOut txOutNew2;
            txOutNew2.nValue = 0;
            txOutNew2.scriptPubKey = outScript2;

            mutTx2.vout.emplace_back(txOutNew2);

            // Add the third asset
            CAssetTransfer outAsset3("RAVENTESTTEST", 100);
            CScript outScript3 = GetScriptForDestination(DecodeDestination(Params().GlobalBurnAddress()));
            outAsset3.ConstructTransaction(outScript3);

            CTxOut txOutNew3;
            txOutNew3.nValue = 0;
            txOutNew3.scriptPubKey = outScript3;

            mutTx2.vout.emplace_back(txOutNew3);
        }

        // Add the inputs
        mutTx2.vin.emplace_back(in);
        mutTx2.vin.emplace_back(in2);
        mutTx2.vin.emplace_back(in3);

        CTransaction tx2(mutTx2);

        // Check the transaction that contains inputs that are spending 1000 Assets for 3 different assets
        // While only outputs only contain 900 Assets being sent to a destination
        // This should fail because 100 of each Asset isn't being sent to a destination (Trying to burn 100 Assets each)
        BOOST_CHECK_MESSAGE(!Consensus::CheckTxAssets(tx2, state, coins, vReissueAssets, true), "CheckTxAssets should of failed");
    }

BOOST_AUTO_TEST_SUITE_END()