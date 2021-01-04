#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Copyright (c) 2019-2020 Xenios SEZC
# https://www.veriblock.org
# Distributed under the MIT software license, see the accompanying
# file LICENSE or http://www.opensource.org/licenses/mit-license.php.

"""
Feature POP popdata max size test

"""
from test_framework.pop import mine_vbk_blocks
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    connect_nodes,
)

class PopPayouts(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2
        self.extra_args = [["-txindex"], ["-txindex"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()
        self.skip_if_no_pypopminer()

    def setup_network(self):
        self.setup_nodes()

        connect_nodes(self.nodes[0], 1)
        self.sync_all(self.nodes)


    def generate_simple_transaction(self, node_id, tx_amount):
        one_satoshi = 1000000
        transaction_max_sequnce = 25
        coinbase_maturity = 100
        fee = 0.00025

        self.nodes[node_id].generate(nblocks=coinbase_maturity + int(tx_amount / transaction_max_sequnce))

        receiver_addr = self.nodes[node_id].get_deterministic_priv_key().address
        private_key = self.nodes[node_id].get_deterministic_priv_key().key

        tmp_b = self.nodes[0].getblock(self.nodes[0].getblockhash(1))
        spend_tx = self.nodes[0].decoderawtransaction(self.nodes[0].gettransaction(tmp_b['tx'][0])['hex'])

        for k in range(int(tx_amount / transaction_max_sequnce)):
            tmp_b = self.nodes[0].getblock(self.nodes[0].getblockhash(1 + k))
            spend_tx = self.nodes[0].decoderawtransaction(self.nodes[0].gettransaction(tmp_b['tx'][0])['hex'])

            # Generate simple transactions sequence
            for i in range(transaction_max_sequnce):
                spend_amount = (int(spend_tx['vout'][0]['value'] * one_satoshi) - int(fee * one_satoshi)) / one_satoshi
                tx = self.nodes[node_id].createrawtransaction(
                    inputs=[{  # coinbase
                        "txid": spend_tx['txid'],
                        "vout": 0
                    }],
                    outputs=[{receiver_addr: spend_amount},],
                )
                tx = self.nodes[node_id].signrawtransactionwithkey(
                    hexstring=tx,
                    privkeys=[private_key],
                )['hex']

                tx_id = self.nodes[node_id].sendrawtransaction(tx)
                spend_tx = self.nodes[0].getrawtransaction(tx_id, True)


    def _test_case(self):
        self.log.warning("running _test_case()")

        # Generate simple transactions
        tx_amount = 10000
        self.log.info("generate simple transactions on node0, amount {}".format(tx_amount))
        self.generate_simple_transaction(node_id = 0, tx_amount = tx_amount)

        # Generate vbk_blocks
        vbk_blocks = 15000
        self.log.info("generate vbk blocks on node0, amount {}".format(vbk_blocks))
        mine_vbk_blocks(self.nodes[0], self.apm, vbk_blocks)

        containingblockhash = self.nodes[0].generate(nblocks=1)[0]
        containingblock = self.nodes[0].getblock(containingblockhash)

        assert len(containingblock['tx']) > 1
        assert len(containingblock['tx']) < tx_amount + 1
        assert len(containingblock['pop']['data']['vbkblocks']) != 0
        assert len(containingblock['pop']['data']['vbkblocks']) < vbk_blocks

        self.log.info("sync nodes")
        self.sync_blocks(self.nodes, timeout=600)

        containingblock = self.nodes[1].getblock(containingblockhash)
        assert len(containingblock['tx']) > 1
        assert len(containingblock['tx']) < tx_amount + 1
        assert len(containingblock['pop']['data']['vbkblocks']) != 0
        assert len(containingblock['pop']['data']['vbkblocks']) < vbk_blocks

        self.log.warning("success! _test_case()")

    def run_test(self):
        """Main test logic"""

        self.nodes[0].generate(nblocks=10)
        self.sync_all(self.nodes)

        from pypopminer2 import MockMiner2
        self.apm = MockMiner2()

        self._test_case()

if __name__ == '__main__':
    PopPayouts().main()
