#!/usr/bin/env python3
# Copyright (c) 2014-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test generatecustomblock rpc.
"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.address import ADDRESS_BCRT1_UNSPENDABLE_DESCRIPTOR

class GenerateCustomBlockTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        node = self.nodes[0]

        # Generate an empty block to address
        address = node.getnewaddress()
        node.generatecustomblock(address, [])

        # Generate an empty block to a descriptor
        node.generatecustomblock(ADDRESS_BCRT1_UNSPENDABLE_DESCRIPTOR, [])

        # Generate 110 blocks to spend
        node.generatetoaddress(110, address)

        # Generate some extra mempool transactions to verify they don't get mined
        for i in range(10):
            node.sendtoaddress(address, 0.001)

        # Generate custom block with raw tx
        utxos = node.listunspent(addresses=[address])
        raw = node.createrawtransaction([{"txid":utxos[0]["txid"], "vout":utxos[0]["vout"]}],[{address:1}])
        signed_raw = node.signrawtransactionwithwallet(raw)['hex']
        hash = node.generatecustomblock(address, [signed_raw])
        block = node.getblock(hash, 1)
        assert len(block['tx']) == 2
        txid = block['tx'][1]
        assert node.gettransaction(txid)['hex'] == signed_raw

        # Generate custom block with txid
        txid = node.sendtoaddress(address, 1)
        hash = node.generatecustomblock(address, [txid])
        block = node.getblock(hash, 1)
        assert len(block['tx']) == 2
        assert block['tx'][1] == txid

if __name__ == '__main__':
    GenerateCustomBlockTest().main()
