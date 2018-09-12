#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Copyright (c) 2017-2018 The Raven Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Testing asset use cases

"""
from test_framework.test_framework import RavenTestFramework
from test_framework.util import (
    assert_equal,
    assert_is_hash_string,
    assert_raises_rpc_error,
)

import string

class AssetTest(RavenTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3

    def activate_assets(self):
        self.log.info("Generating RVN for node[0] and activating assets...")
        n0, n1, n2 = self.nodes[0], self.nodes[1], self.nodes[2]

        n0.generate(1)
        self.sync_all()
        n0.generate(431)
        self.sync_all()
        assert_equal("active", n0.getblockchaininfo()['bip9_softforks']['assets']['status'])

    def big_test(self):
        self.log.info("Running big test!")
        n0, n1, n2 = self.nodes[0], self.nodes[1], self.nodes[2]

        self.log.info("Calling issue()...")
        address0 = n0.getnewaddress()
        ipfs_hash = "QmacSRmrkVmvJfbCpmU6pK72furJ8E8fbKHindrLxmYMQo"
        n0.issue(asset_name="MY_ASSET", qty=1000, to_address=address0, change_address="", \
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

        self.log.info("Checking listassetbalancesbyaddress()...")
        assert_equal(n0.listaddressesbyasset("MY_ASSET"), n1.listaddressesbyasset("MY_ASSET"))

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
        assert_equal(n0.listaddressesbyasset("MY_ASSET"), n1.listaddressesbyasset("MY_ASSET"))
        assert_equal(sum(n0.listaddressesbyasset("MY_ASSET").values()), 1000)
        assert_equal(sum(n1.listaddressesbyasset("MY_ASSET").values()), 1000)
        for assaddr in n0.listaddressesbyasset("MY_ASSET").keys():
            if n0.validateaddress(assaddr)["ismine"] == True:
                changeaddress = assaddr
                assert_equal(n0.listassetbalancesbyaddress(changeaddress)["MY_ASSET"], 800)
        assert(changeaddress != None)
        assert_equal(n0.listassetbalancesbyaddress(address0)["MY_ASSET!"], 1)

        self.log.info("Calling reissue()...")
        address1 = n0.getnewaddress()
        n0.reissue(asset_name="MY_ASSET", qty=2000, to_address=address0, change_address=address1, \
                   reissuable=False, new_ipfs=ipfs_hash[::-1])

        self.log.info("Waiting for ten confirmations after reissue...")
        self.sync_all()
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
        n0.generate(1)
        self.sync_all()

        n0.listassets(asset="RAVEN*", verbose=False, count=2, start=-2)

        self.log.info("Creating some sub-assets...")
        n0.issue(asset_name="MY_ASSET/SUB1", qty=1000, to_address=address0, change_address=address0,\
                 units=4, reissuable=True, has_ipfs=True, ipfs_hash=ipfs_hash)

        self.sync_all()
        self.log.info("Waiting for ten confirmations after issuesubasset...")
        n0.generate(10)
        self.sync_all()

        self.log.info("Checkout getassetdata()...")
        assetdata = n0.getassetdata("MY_ASSET/SUB1")
        assert_equal(assetdata["name"], "MY_ASSET/SUB1")
        assert_equal(assetdata["amount"], 1000)
        assert_equal(assetdata["units"], 4)
        assert_equal(assetdata["reissuable"], 1)
        assert_equal(assetdata["has_ipfs"], 1)
        assert_equal(assetdata["ipfs_hash"], ipfs_hash)

        raven_assets = n0.listassets(asset="RAVEN*", verbose=False, count=2, start=-2)
        assert_equal(len(raven_assets), 2)
        assert_equal(raven_assets[0], "RAVEN2")
        assert_equal(raven_assets[1], "RAVEN3")
        self.sync_all()

    def issue_param_checks(self):
        self.log.info("Checking bad parameter handling!")
        n0, n1, n2 = self.nodes[0], self.nodes[1], self.nodes[2]

        # just plain bad asset name
        assert_raises_rpc_error(-8, "Invalid asset name: bad-asset-name", \
            n0.issue, "bad-asset-name");

        # trying to issue things that can't be issued
        assert_raises_rpc_error(-8, "Unsupported asset type: OWNER", \
            n0.issue, "AN_OWNER!");
        assert_raises_rpc_error(-8, "Unsupported asset type: MSGCHANNEL", \
            n0.issue, "A_MSGCHANNEL~CHANNEL_4");
        assert_raises_rpc_error(-8, "Unsupported asset type: VOTE", \
            n0.issue, "A_VOTE^PEDRO");

        # check bad unique params
        assert_raises_rpc_error(-8, "Invalid parameters for issuing a unique asset.", \
            n0.issue, "A_UNIQUE#ASSET", 2)
        assert_raises_rpc_error(-8, "Invalid parameters for issuing a unique asset.", \
            n0.issue, "A_UNIQUE#ASSET", 1, "", "", 1)
        assert_raises_rpc_error(-8, "Invalid parameters for issuing a unique asset.", \
            n0.issue, "A_UNIQUE#ASSET", 1, "", "", 0, True)

    def chain_assets(self):
        self.log.info("Issuing chained assets in depth issue()...")
        n0, n1, n2 = self.nodes[0], self.nodes[1], self.nodes[2]

        chain_address = n0.getnewaddress()
        ipfs_hash = "QmacSRmrkVmvJfbCpmU6pK72furJ8E8fbKHindrLxmYMQo"
        chain_string = "CHAIN1"
        n0.issue(asset_name=chain_string, qty=1000, to_address=chain_address, change_address="", \
                 units=4, reissuable=True, has_ipfs=True, ipfs_hash=ipfs_hash)

        for c in string.ascii_uppercase:
            chain_string += '/' + c
            if len(chain_string) > 30:
                break
            n0.issue(asset_name=chain_string, qty=1000, to_address=chain_address, change_address="", \
                     units=4, reissuable=True, has_ipfs=True, ipfs_hash=ipfs_hash)

        n0.generate(1)
        self.sync_all()

        chain_assets = n1.listassets(asset="CHAIN1*", verbose=False)
        assert_equal(len(chain_assets), 13)

        self.log.info("Issuing chained assets in width issue()...")
        chain_address = n0.getnewaddress()
        chain_string = "CHAIN2"
        n0.issue(asset_name=chain_string, qty=1000, to_address=chain_address, change_address="", \
                 units=4, reissuable=True, has_ipfs=True, ipfs_hash=ipfs_hash)

        for c in string.ascii_uppercase:
            asset_name = chain_string + '/' + c

            n0.issue(asset_name=asset_name, qty=1000, to_address=chain_address, change_address="", \
                     units=4, reissuable=True, has_ipfs=True, ipfs_hash=ipfs_hash)

        n0.generate(1)
        self.sync_all()

        chain_assets = n1.listassets(asset="CHAIN2/*", verbose=False)
        assert_equal(len(chain_assets), 26)

        self.log.info("Chaining reissue transactions...")
        address0 = n0.getnewaddress()
        n0.issue(asset_name="CHAIN_REISSUE", qty=1000, to_address=address0, change_address="", \
                 units=4, reissuable=True, has_ipfs=False)

        n0.generate(1)
        self.sync_all()

        n0.reissue(asset_name="CHAIN_REISSUE", qty=1000, to_address=address0, change_address="", \
                   reissuable=True)
        assert_raises_rpc_error(-4, "Error: The transaction was rejected! Reason given: bad-tx-reissue-chaining-not-allowed", n0.reissue, "CHAIN_REISSUE", 1000, address0, "", True)

        n0.generate(1)
        self.sync_all()

        n0.reissue(asset_name="CHAIN_REISSUE", qty=1000, to_address=address0, change_address="", \
                   reissuable=True)

        n0.generate(1)
        self.sync_all()

        assetdata = n0.getassetdata("CHAIN_REISSUE")
        assert_equal(assetdata["name"], "CHAIN_REISSUE")
        assert_equal(assetdata["amount"], 3000)
        assert_equal(assetdata["units"], 4)
        assert_equal(assetdata["reissuable"], 1)
        assert_equal(assetdata["has_ipfs"], 0)

    def run_test(self):
        self.activate_assets();
        self.big_test();
        self.issue_param_checks();
        self.chain_assets();

if __name__ == '__main__':
    AssetTest().main()
