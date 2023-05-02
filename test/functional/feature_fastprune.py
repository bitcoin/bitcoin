#!/usr/bin/env python3
# Copyright (c) 2023 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test fastprune mode."""
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal
)
from test_framework.blocktools import (
    create_block,
    create_coinbase,
    add_witness_commitment
)
from test_framework.wallet import MiniWallet


class FeatureFastpruneTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-fastprune"]]

    def run_test(self):
        self.log.info("ensure that large blocks don't crash or freeze in -fastprune")
        wallet = MiniWallet(self.nodes[0])
        tx = wallet.create_self_transfer()['tx']
        annex = [0x50]
        for _ in range(0x10000):
            annex.append(0xff)
        tx.wit.vtxinwit[0].scriptWitness.stack.append(bytes(annex))
        tip = int(self.nodes[0].getbestblockhash(), 16)
        time = self.nodes[0].getblock(self.nodes[0].getbestblockhash())['time'] + 1
        height = self.nodes[0].getblockcount() + 1
        block = create_block(hashprev=tip, ntime=time, txlist=[tx], coinbase=create_coinbase(height=height))
        add_witness_commitment(block)
        block.solve()
        self.nodes[0].submitblock(block.serialize().hex())
        assert_equal(int(self.nodes[0].getbestblockhash(), 16), block.sha256)


if __name__ == '__main__':
    FeatureFastpruneTest().main()
