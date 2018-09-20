// Copyright (c) 2017 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <assets/assets.h>

#include <test/test_raven.h>

#include <boost/test/unit_test.hpp>

#include <amount.h>
#include <base58.h>
#include <chainparams.h>

BOOST_FIXTURE_TEST_SUITE(serialization_tests, BasicTestingSetup)

    BOOST_AUTO_TEST_CASE(issue_asset_serialization) {
        SelectParams("test");

        // Create asset
        CNewAsset asset("SERIALIZATION", 100000000, 0, 0, 1, DecodeIPFS("QmacSRmrkVmvJfbCpmU6pK72furJ8E8fbKHindrLxmYMQo"));

        // Create destination
        CTxDestination dest = DecodeDestination("mfe7MqgYZgBuXzrT2QTFqZwBXwRDqagHTp"); // Testnet Address

        BOOST_CHECK(IsValidDestination(dest));

        CScript scriptPubKey = GetScriptForDestination(dest);

        asset.ConstructTransaction(scriptPubKey);

        CNewAsset serializedAsset;
        std::string address;
        BOOST_CHECK_MESSAGE(AssetFromScript(scriptPubKey, serializedAsset, address), "Failed to get asset from script");
        BOOST_CHECK_MESSAGE(address == "mfe7MqgYZgBuXzrT2QTFqZwBXwRDqagHTp", "Addresses weren't equal");
        BOOST_CHECK_MESSAGE(serializedAsset.strName == "SERIALIZATION", "Asset names weren't equal");
        BOOST_CHECK_MESSAGE(serializedAsset.nAmount == 100000000, "Amount weren't equal");
        BOOST_CHECK_MESSAGE(serializedAsset.units == 0, "Units weren't equal");
        BOOST_CHECK_MESSAGE(serializedAsset.nReissuable == 0, "Reissuable wasn't equal");
        BOOST_CHECK_MESSAGE(serializedAsset.nHasIPFS == 1, "HasIPFS wasn't equal");
        BOOST_CHECK_MESSAGE(EncodeIPFS(serializedAsset.strIPFSHash) == "QmacSRmrkVmvJfbCpmU6pK72furJ8E8fbKHindrLxmYMQo", "IPFSHash wasn't equal");
        
        // Bare asset
        CNewAsset asset2("SERIALIZATION", 100000000);
        scriptPubKey = GetScriptForDestination(dest);
        asset2.ConstructTransaction(scriptPubKey);
        CNewAsset serializedAsset2;
        BOOST_CHECK_MESSAGE(AssetFromScript(scriptPubKey, serializedAsset2, address), "Failed to get asset from script");
        BOOST_CHECK_MESSAGE(address == "mfe7MqgYZgBuXzrT2QTFqZwBXwRDqagHTp", "Addresses weren't equal");
        BOOST_CHECK_MESSAGE(serializedAsset2.strName == "SERIALIZATION", "Asset names weren't equal");
        BOOST_CHECK_MESSAGE(serializedAsset2.nAmount == 100000000, "Amount weren't equal");
        BOOST_CHECK_MESSAGE(serializedAsset2.units == 0, "Units weren't equal");
        BOOST_CHECK_MESSAGE(serializedAsset2.nReissuable == 1, "Reissuable wasn't equal");
        BOOST_CHECK_MESSAGE(serializedAsset2.nHasIPFS == 0, "HasIPFS wasn't equal");
        BOOST_CHECK_MESSAGE(serializedAsset2.strIPFSHash == "", "IPFSHash wasn't equal");
    }

    BOOST_AUTO_TEST_CASE(reissue_asset_serialization) {
        SelectParams("test");

        // Create asset
        std::string name = "SERIALIZATION";
        CReissueAsset reissue(name, 100000000, 0, 0, DecodeIPFS("QmacSRmrkVmvJfbCpmU6pK72furJ8E8fbKHindrLxmYMQo"));

        // Create destination
        CTxDestination dest = DecodeDestination("mfe7MqgYZgBuXzrT2QTFqZwBXwRDqagHTp"); // Testnet Address

        BOOST_CHECK(IsValidDestination(dest));

        CScript scriptPubKey = GetScriptForDestination(dest);

        reissue.ConstructTransaction(scriptPubKey);

        CReissueAsset serializedAsset;
        std::string address;
        BOOST_CHECK_MESSAGE(ReissueAssetFromScript(scriptPubKey, serializedAsset, address), "Failed to get asset from script");
        BOOST_CHECK_MESSAGE(address == "mfe7MqgYZgBuXzrT2QTFqZwBXwRDqagHTp", "Addresses weren't equal");
        BOOST_CHECK_MESSAGE(serializedAsset.strName == "SERIALIZATION", "Asset names weren't equal");
        BOOST_CHECK_MESSAGE(serializedAsset.nAmount == 100000000, "Amount weren't equal");
        BOOST_CHECK_MESSAGE(EncodeIPFS(serializedAsset.strIPFSHash) == "QmacSRmrkVmvJfbCpmU6pK72furJ8E8fbKHindrLxmYMQo", "IPFSHash wasn't equal");

        // Empty IPFS
        CReissueAsset reissue2(name, 100000000, 0, 0, "");
        scriptPubKey = GetScriptForDestination(dest);
        reissue2.ConstructTransaction(scriptPubKey);
        CReissueAsset serializedAsset2;
        BOOST_CHECK_MESSAGE(ReissueAssetFromScript(scriptPubKey, serializedAsset2, address), "Failed to get asset from script");
        BOOST_CHECK_MESSAGE(address == "mfe7MqgYZgBuXzrT2QTFqZwBXwRDqagHTp", "Addresses weren't equal");
        BOOST_CHECK_MESSAGE(serializedAsset2.strName == "SERIALIZATION", "Asset names weren't equal");
        BOOST_CHECK_MESSAGE(serializedAsset2.nAmount == 100000000, "Amount weren't equal");
        BOOST_CHECK_MESSAGE(serializedAsset2.strIPFSHash == "", "IPFSHash wasn't equal");
    }

    BOOST_AUTO_TEST_CASE(owner_asset_serialization) {
        SelectParams("test");

        // Create asset
        std::string name = "SERIALIZATION";
        CNewAsset asset(name, 100000000);

        // Create destination
        CTxDestination dest = DecodeDestination("mfe7MqgYZgBuXzrT2QTFqZwBXwRDqagHTp"); // Testnet Address

        BOOST_CHECK(IsValidDestination(dest));

        CScript scriptPubKey = GetScriptForDestination(dest);

        asset.ConstructOwnerTransaction(scriptPubKey);

        std::string strOwnerName;
        std::string address;
        std::stringstream ownerName;
        ownerName << name << OWNER_TAG;
        BOOST_CHECK_MESSAGE(OwnerAssetFromScript(scriptPubKey, strOwnerName, address), "Failed to get asset from script");
        BOOST_CHECK_MESSAGE(address == "mfe7MqgYZgBuXzrT2QTFqZwBXwRDqagHTp", "Addresses weren't equal");
        BOOST_CHECK_MESSAGE(strOwnerName == ownerName.str(), "Asset names weren't equal");
    }

BOOST_AUTO_TEST_SUITE_END()