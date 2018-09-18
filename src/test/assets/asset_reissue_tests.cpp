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

BOOST_FIXTURE_TEST_SUITE(asset_reissue_tests, BasicTestingSetup)


    BOOST_AUTO_TEST_CASE(reissue_cache_test) {

    SelectParams(CBaseChainParams::MAIN);

    // Create assets cache
    CAssetsCache cache;

    CNewAsset asset1("RVNASSET", CAmount(100 * COIN), 8, 1, 0, "");

    // Add an asset to a valid rvn address
    BOOST_CHECK_MESSAGE(cache.AddNewAsset(asset1, Params().GlobalBurnAddress()), "Failed to add new asset");

    // Create a reissuance of the asset
    CReissueAsset reissue1("RVNASSET", CAmount(1 * COIN), 8, 1, DecodeIPFS("QmacSRmrkVmvJfbCpmU6pK72furJ8E8fbKHindrLxmYMQo"));
    COutPoint out(uint256S("BF50CB9A63BE0019171456252989A459A7D0A5F494735278290079D22AB704A4"), 1);

    // Add an reissuance of the asset to the cache
    BOOST_CHECK_MESSAGE(cache.AddReissueAsset(reissue1, Params().GlobalBurnAddress(), out), "Failed to add reissue");

    // Check to see if the reissue changed the cache data correctly
    BOOST_CHECK_MESSAGE(cache.mapReissuedAssetData.count("RVNASSET"), "Map Reissued Asset should contain the asset \"RVNASSET\"");
    BOOST_CHECK_MESSAGE(cache.mapAssetsAddressAmount.at(make_pair("RVNASSET", Params().GlobalBurnAddress())) == CAmount(101 * COIN), "Reissued amount wasn't added to the previous total");
    BOOST_CHECK_MESSAGE(cache.mapAssetsAddresses.at("RVNASSET").count(Params().GlobalBurnAddress()), "Reissued address wasn't in the map");

    // Get the new asset data from the cache
    CNewAsset asset2;
    BOOST_CHECK_MESSAGE(cache.GetAssetIfExists("RVNASSET", asset2), "Failed to get the asset2");

    // Chech the asset metadata
    BOOST_CHECK_MESSAGE(asset2.nReissuable == 1, "Asset2: Reissuable isn't 1");
    BOOST_CHECK_MESSAGE(asset2.nAmount == CAmount(101 * COIN), "Asset2: Amount isn't 101");
    BOOST_CHECK_MESSAGE(asset2.strName == "RVNASSET", "Asset2: Asset name is wrong");
    BOOST_CHECK_MESSAGE(asset2.units == 8, "Asset2: Units is wrong");
    BOOST_CHECK_MESSAGE(EncodeIPFS(asset2.strIPFSHash) == "QmacSRmrkVmvJfbCpmU6pK72furJ8E8fbKHindrLxmYMQo", "Asset2: IPFS hash is wrong");

    // Remove the reissue from the cache
    std::vector<std::pair<std::string, CBlockAssetUndo> > undoBlockData;
    undoBlockData.emplace_back(std::make_pair("RVNASSET", CBlockAssetUndo {true, false, "", 0}));
    BOOST_CHECK_MESSAGE(cache.RemoveReissueAsset(reissue1, Params().GlobalBurnAddress(), out, undoBlockData), "Failed to remove reissue");

    // Get the asset data from the cache now that the reissuance was removed
    CNewAsset asset3;
    BOOST_CHECK_MESSAGE(cache.GetAssetIfExists("RVNASSET", asset3), "Failed to get the asset3");

    // Chech the asset3 metadata and make sure all the changed from the reissue were removed
    BOOST_CHECK_MESSAGE(asset3.nReissuable == 1, "Asset3: Reissuable isn't 1");
    BOOST_CHECK_MESSAGE(asset3.nAmount == CAmount(100 * COIN), "Asset3: Amount isn't 100");
    BOOST_CHECK_MESSAGE(asset3.strName == "RVNASSET", "Asset3: Asset name is wrong");
    BOOST_CHECK_MESSAGE(asset3.units == 8, "Asset3: Units is wrong");
    BOOST_CHECK_MESSAGE(asset3.strIPFSHash == "", "Asset3: IPFS hash is wrong");

    // Check to see if the reissue removal updated the cache correctly
    BOOST_CHECK_MESSAGE(cache.mapReissuedAssetData.count("RVNASSET"), "Map of reissued data was removed, even though changes were made and not databased yet");
    BOOST_CHECK_MESSAGE(cache.mapAssetsAddressAmount.at(make_pair("RVNASSET", Params().GlobalBurnAddress())) == CAmount(100 * COIN), "Assets total wasn't undone when reissuance was");
    }


    BOOST_AUTO_TEST_CASE(reissue_isvalid_test) {

        SelectParams(CBaseChainParams::MAIN);

        // Create assets cache
        CAssetsCache cache;

        CNewAsset asset1("RVNASSET", CAmount(100 * COIN), 8, 1, 0, "");

        // Add an asset to a valid rvn address
        BOOST_CHECK_MESSAGE(cache.AddNewAsset(asset1, Params().GlobalBurnAddress()), "Failed to add new asset");

        // Create a reissuance of the asset that is valid
        CReissueAsset reissue1("RVNASSET", CAmount(1 * COIN), 8, 1, DecodeIPFS("QmacSRmrkVmvJfbCpmU6pK72furJ8E8fbKHindrLxmYMQo"));

        std::string error;
        BOOST_CHECK_MESSAGE(reissue1.IsValid(error, cache), "Reissue should of been valid");

        // Create a reissuance of the asset that is not valid
        CReissueAsset reissue2("NOTEXIST", CAmount(1 * COIN), 8, 1, DecodeIPFS("QmacSRmrkVmvJfbCpmU6pK72furJ8E8fbKHindrLxmYMQo"));

        BOOST_CHECK_MESSAGE(!reissue2.IsValid(error, cache), "Reissue shouldn't of been valid");

        // Create a reissuance of the asset that is not valid (unit is smaller than current asset)
        CReissueAsset reissue3("RVNASSET", CAmount(1 * COIN), 7, 1, DecodeIPFS("QmacSRmrkVmvJfbCpmU6pK72furJ8E8fbKHindrLxmYMQo"));

        BOOST_CHECK_MESSAGE(!reissue3.IsValid(error, cache), "Reissue shouldn't of been valid because of units");

        // Create a reissuance of the asset that is not valid (unit is not changed)
        CReissueAsset reissue4("RVNASSET", CAmount(1 * COIN), -1, 1, DecodeIPFS("QmacSRmrkVmvJfbCpmU6pK72furJ8E8fbKHindrLxmYMQo"));

        BOOST_CHECK_MESSAGE(reissue4.IsValid(error, cache), "Reissue4 wasn't valid");

        // Create a new asset object with units of 0
        CNewAsset asset2("RVNASSET2", CAmount(100 * COIN), 0, 1, 0, "");

        // Add new asset to a valid rvn address
        BOOST_CHECK_MESSAGE(cache.AddNewAsset(asset2, Params().GlobalBurnAddress()), "Failed to add new asset");

        // Create a reissuance of the asset that is valid unit go from 0 -> 1 and change the ipfs hash
        CReissueAsset reissue5("RVNASSET2", CAmount(1 * COIN), 1, 1, DecodeIPFS("QmacSRmrkVmvJfbCpmU6pK72furJ8E8fbKHindrLxmYMQo"));

        BOOST_CHECK_MESSAGE(reissue5.IsValid(error, cache), "Reissue5 wasn't valid");

        // Create a reissuance of the asset that is valid unit go from 1 -> 1 and change the ipfs hash
        CReissueAsset reissue6("RVNASSET2", CAmount(1 * COIN), 1, 1, DecodeIPFS("QmacSRmrkVmvJfbCpmU6pK72furJ8E8fbKHindrLxmYMQo"));

        BOOST_CHECK_MESSAGE(reissue6.IsValid(error, cache), "Reissue6 wasn't valid");
    }


BOOST_AUTO_TEST_SUITE_END()