#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Copyright (c) 2017 The Raven Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Testing asset use cases

"""
from test_framework.test_framework import RavenTestFramework
from test_framework.util import (
    assert_equal,
    assert_is_hash_string,
)

class AssetTest(RavenTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3

    def run_test(self):
        self.log.info("Running test!")

        n0, n1, n2 = self.nodes[0], self.nodes[1], self.nodes[2]

        self.log.info("Generating RVN for node[0] and activating assets...")
        n0.generate(1)
        self.sync_all()
        n2.generate(431)
        self.sync_all()
        assert_equal(n0.getbalance(), 5000)

        self.log.info("Calling issue()...")
        address0 = n0.getnewaddress()
        ipfs_hash = "QmacSRmrkVmvJfbCpmU6pK72furJ8E8fbKHindrLxmYMQo"
        n0.issue(asset_name="MY_ASSET", qty=1000, to_address=address0, change_address="",\
                 units=4, reissuable=True, has_ipfs=True, ipfs_hash=ipfs_hash)

        self.log.info("Waiting for ten confirmations after issue...")
        n0.generate(10)
        self.sync_all()

        self.log.info("Checkout getassetdata()...")
        assetdata = n0.getassetdata("MY_ASSET")
        assert_equal(assetdata["name"], "MY_ASSET")
        assert_equal(assetdata["amount"], 1000)
        assert_equal(assetdata["units"], 4)
        assert_equal(assetdata["reissuable"], 1)
        assert_equal(assetdata["has_ipfs"], 1)
        assert_equal(assetdata["ipfs_hash"], ipfs_hash)

        self.log.info("Checking listmyassets()...")
        myassets = n0.listmyassets(asset="MY_ASSET*", verbose=True)
        assert_equal(len(myassets), 2)
        asset_names = list(myassets.keys())
        assert_equal(asset_names.count("MY_ASSET"), 1)
        assert_equal(asset_names.count("MY_ASSET!"), 1)
        assert_equal(myassets["MY_ASSET"]["balance"], 1000)
        assert_equal(myassets["MY_ASSET!"]["balance"], 1)
        assert_equal(len(myassets["MY_ASSET"]["outpoints"]), 1)
        assert_equal(len(myassets["MY_ASSET!"]["outpoints"]), 1)
        assert_is_hash_string(myassets["MY_ASSET"]["outpoints"][0]["txid"])
        assert_equal(myassets["MY_ASSET"]["outpoints"][0]["txid"], \
                     myassets["MY_ASSET!"]["outpoints"][0]["txid"])
        assert(int(myassets["MY_ASSET"]["outpoints"][0]["vout"]) >= 0)
        assert(int(myassets["MY_ASSET!"]["outpoints"][0]["vout"]) >= 0)
        assert_equal(myassets["MY_ASSET"]["outpoints"][0]["amount"], 1000)
        assert_equal(myassets["MY_ASSET!"]["outpoints"][0]["amount"], 1)

        self.log.info("Checking listassetbalancesbyaddress()...")
        assert_equal(n0.listassetbalancesbyaddress(address0)["MY_ASSET"], 1000)
        assert_equal(n0.listassetbalancesbyaddress(address0)["MY_ASSET!"], 1)

        self.log.info("Calling transfer()...")
        address1 = n1.getnewaddress()
        n0.transfer(asset_name="MY_ASSET", qty=200, to_address=address1)

        self.log.info("Waiting for ten confirmations after transfer...")
        n0.generate(10)
        self.sync_all()

        self.log.info("Checking listmyassets()...")
        myassets = n1.listmyassets(asset="MY_ASSET*", verbose=True)
        assert_equal(len(myassets), 1)
        asset_names = list(myassets.keys())
        assert_equal(asset_names.count("MY_ASSET"), 1)
        assert_equal(asset_names.count("MY_ASSET!"), 0)
        assert_equal(myassets["MY_ASSET"]["balance"], 200)
        assert_equal(len(myassets["MY_ASSET"]["outpoints"]), 1)
        assert_is_hash_string(myassets["MY_ASSET"]["outpoints"][0]["txid"])
        assert(int(myassets["MY_ASSET"]["outpoints"][0]["vout"]) >= 0)
        assert_equal(n0.listmyassets(asset="MY_ASSET")["MY_ASSET"], 800)

        self.log.info("Checking listassetbalancesbyaddress()...")
        assert_equal(n1.listassetbalancesbyaddress(address1)["MY_ASSET"], 200)
        changeaddress = None
        for assaddr in n0.listaddressesbyasset("MY_ASSET").keys():
            if n0.validateaddress(assaddr)["ismine"] == True:
                changeaddress = assaddr
                assert_equal(n0.listassetbalancesbyaddress(changeaddress)["MY_ASSET"], 800)
        assert(changeaddress != None)
        assert_equal(n0.listassetbalancesbyaddress(address0)["MY_ASSET!"], 1)

        self.log.info("Calling reissue()...")
        n0.reissue(asset_name="MY_ASSET", qty=2000, to_address=address0, change_address="",\
                   reissuable=False, new_ipfs=ipfs_hash[::-1])

        self.log.info("Waiting for ten confirmations after reissue...")
        n0.generate(10)
        self.sync_all()

        self.log.info("Checkout getassetdata()...")
        assetdata = n0.getassetdata("MY_ASSET")
        assert_equal(assetdata["name"], "MY_ASSET")
        assert_equal(assetdata["amount"], 3000)
        assert_equal(assetdata["units"], 4)
        assert_equal(assetdata["reissuable"], 0)
        assert_equal(assetdata["has_ipfs"], 1)
        assert_equal(assetdata["ipfs_hash"], ipfs_hash[::-1])

        self.log.info("Checking listassetbalancesbyaddress()...")
        assert_equal(n0.listassetbalancesbyaddress(address0)["MY_ASSET"], 2000)

        self.log.info("Checking listassets()...")
        n0.issue("RAVEN1", 1000)
        n0.issue("RAVEN2", 1000)
        n0.issue("RAVEN3", 1000)
        n0.generate(10)
        self.sync_all()

        raven_assets = n0.listassets(asset="RAVEN*", verbose=False, count=2, start=-2)
        assert_equal(len(raven_assets), 2)
        assert_equal(raven_assets[0], "RAVEN2")
        assert_equal(raven_assets[1], "RAVEN3")


if __name__ == '__main__':
    AssetTest().main()
