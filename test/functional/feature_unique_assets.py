#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Copyright (c) 2017-2018 The Raven Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Testing unique asset use cases

"""
import random

from test_framework.test_framework import RavenTestFramework
from test_framework.util import (
    assert_contains,
    assert_does_not_contain_key,
    assert_equal,
    assert_raises_rpc_error,
)


def gen_root_asset_name():
    size = random.randint(3, 14)
    name = ""
    for _ in range(1, size+1):
        ch = random.randint(65, 65+25)
        name += chr(ch)
    return name

def gen_unique_asset_name(root):
    tag_ab = "-ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789@$%&*()[]{}<>_.;?\\:"
    name = root + "#"
    tag_size = random.randint(1, 15)
    for _ in range(1, tag_size+1):
        tag_c = tag_ab[random.randint(0, len(tag_ab) - 1)]
        name += tag_c
    return name

class UniqueAssetTest(RavenTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3

    def activate_assets(self):
        self.log.info("Generating RVN for node[0] and activating assets...")
        n0 = self.nodes[0]
        n0.generate(432)
        self.sync_all()
        assert_equal("active", n0.getblockchaininfo()['bip9_softforks']['assets']['status'])

    def issue_one(self):
        self.log.info("Issuing a unique asset...")
        n0 = self.nodes[0]
        root = gen_root_asset_name()
        n0.issue(asset_name=root)
        n0.generate(1)
        asset_name = gen_unique_asset_name(root)
        tx_hash = n0.issue(asset_name=asset_name)
        n0.generate(1)
        assert_equal(1, n0.listmyassets()[asset_name])

    def issue_invalid(self):
        self.log.info("Trying some invalid calls...")
        n0, n1 = self.nodes[0], self.nodes[1]
        n1.generate(10)
        self.sync_all()

        root = gen_root_asset_name()
        asset_name = gen_unique_asset_name(root)

        # no root
        assert_raises_rpc_error(-32600, f"Wallet doesn't have asset: {root}!", n0.issue, asset_name)

        # don't own root
        n0.sendtoaddress(n1.getnewaddress(), 501)
        n0.generate(1)
        self.sync_all()
        n1.issue(root)
        n1.generate(1)
        self.sync_all()
        assert_contains(root, n0.listassets())
        assert_raises_rpc_error(-32600, f"Wallet doesn't have asset: {root}!", n0.issue, asset_name)
        n1.transfer(f"{root}!", 1, n0.getnewaddress())
        n1.generate(1)
        self.sync_all()

        # bad qty
        assert_raises_rpc_error(-8, "Invalid parameters for issuing a unique asset.", n0.issue, asset_name, 2)

        # bad units
        assert_raises_rpc_error(-8, "Invalid parameters for issuing a unique asset.", n0.issue, asset_name, 1, "", "", 1)

        # reissuable
        assert_raises_rpc_error(-8, "Invalid parameters for issuing a unique asset.", n0.issue, asset_name, 1, "", "", 0, True)

        # already exists
        n0.issue(asset_name)
        n0.generate(1)
        self.sync_all()
        assert_raises_rpc_error(-8, f"Invalid parameter: asset_name '{asset_name}' has already been used", n0.issue, asset_name)


    def issueunique_test(self):
        self.log.info("Testing issueunique RPC...")
        n0, n1 = self.nodes[0], self.nodes[1]
        n0.sendtoaddress(n1.getnewaddress(), 501)

        root = gen_root_asset_name()
        n0.issue(asset_name=root)
        asset_tags = ["first", "second"]
        ipfs_hashes = ["QmWWQSuPMS6aXCbZKpEjPHPUZN2NjB3YrhJTHsV4X3vb2t"] * len(asset_tags)
        n0.issueunique(root, asset_tags, ipfs_hashes)
        block_hash = n0.generate(1)[0]

        for tag in asset_tags:
            asset_name = f"{root}#{tag}"
            assert_equal(1, n0.listmyassets()[asset_name])
            assert_equal(1, n0.listassets(asset_name, True)[asset_name]['has_ipfs'])
            assert_equal(ipfs_hashes[0], n0.listassets(asset_name, True)[asset_name]['ipfs_hash'])

        # invalidate
        n0.invalidateblock(block_hash)
        assert_does_not_contain_key(root, n0.listmyassets())
        assert_does_not_contain_key(asset_name, n0.listmyassets())

        # reconsider
        n0.reconsiderblock(block_hash)
        assert_contains(root, n0.listmyassets())
        assert_contains(asset_name, n0.listmyassets())

        # root doesn't exist
        missing_asset = "VAPOUR"
        assert_raises_rpc_error(-32600, f"Wallet doesn't have asset: {missing_asset}!", n0.issueunique, missing_asset, asset_tags)

        # don't own root
        n1.issue(missing_asset)
        n1.generate(1)
        self.sync_all()
        assert_contains(missing_asset, n0.listassets())
        assert_raises_rpc_error(-32600, f"Wallet doesn't have asset: {missing_asset}!", n0.issueunique, missing_asset, asset_tags)

    def run_test(self):
        self.activate_assets()
        self.issueunique_test()
        self.issue_one()
        self.issue_invalid()


if __name__ == '__main__':
    UniqueAssetTest().main()
