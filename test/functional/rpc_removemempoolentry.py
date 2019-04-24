#!/usr/bin/env python3

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal

class RemoveMempoolEntryTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [["-txindex"], []]

    def send_transactions(self, from_node, to_node, count):
        block_hashs = from_node.generatetoaddress(count, from_node.get_deterministic_priv_key().address)
        coinbase_txids = [from_node.getblock(block_hash)['tx'][0] for block_hash in block_hashs]

        from_node.generate(99)
        self.sync_all()

        for coinbase_txid in coinbase_txids:
            coinbase_tx = from_node.getrawtransaction(coinbase_txid, True)
            inputs = [{ "txid": coinbase_txid, "vout": 0, "scriptPubKey": coinbase_tx["vout"][0]["scriptPubKey"]["hex"]}]
            outputs = {to_node.get_deterministic_priv_key().address: 0.1}
            rawTx = from_node.createrawtransaction(inputs, outputs)
            rawTxSigned = from_node.signrawtransactionwithkey(rawTx, [from_node.get_deterministic_priv_key().key], inputs)
            from_node.sendrawtransaction(rawTxSigned["hex"], 0)

    def run_test(self):
        chain_height = self.nodes[0].getblockcount()
        assert_equal(chain_height, 200)

        self.log.debug("Send 5 transactions from node0")
        self.send_transactions(self.nodes[0], self.nodes[1], 5)
        self.sync_all()

        self.log.debug("Verify that node0 and node1 have 5 transactions in their mempools")
        assert_equal(len(self.nodes[0].getrawmempool()), 5)
        assert_equal(len(self.nodes[1].getrawmempool()), 5)

        self.log.debug("Remove first transaction from node1 mempool")
        self.nodes[0].removemempoolentry(txid=self.nodes[0].getrawmempool()[0])

        self.log.debug("Verify that node0 have 4 transactions and node1 have 5 transactions in their mempools")
        assert_equal(len(self.nodes[0].getrawmempool()), 4)
        assert_equal(len(self.nodes[1].getrawmempool()), 5)

if __name__ == '__main__':
    RemoveMempoolEntryTest().main()
