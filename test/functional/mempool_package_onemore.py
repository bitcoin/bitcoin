#!/usr/bin/env python3
# Copyright (c) 2014-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test descendant package tracking carve-out allowing one final transaction in
   an otherwise-full package as long as it has only one parent and is <= 10k in
   size.
"""

from test_framework.messages import (
    DEFAULT_ANCESTOR_LIMIT,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
)
from test_framework.wallet import MiniWallet


class MempoolPackagesTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-maxorphantx=1000"]]

    def chain_tx(self, utxos_to_spend, *, num_outputs=1):
        return self.wallet.send_self_transfer_multi(
            from_node=self.nodes[0],
            utxos_to_spend=utxos_to_spend,
            num_outputs=num_outputs)['new_utxos']

    def run_test(self):
        self.wallet = MiniWallet(self.nodes[0])

        # DEFAULT_ANCESTOR_LIMIT transactions off a confirmed tx should be fine
        chain = []
        utxo = self.wallet.get_utxo()
        for _ in range(4):
            utxo, utxo2 = self.chain_tx([utxo], num_outputs=2)
            chain.append(utxo2)
        for _ in range(DEFAULT_ANCESTOR_LIMIT - 4):
            utxo, = self.chain_tx([utxo])
            chain.append(utxo)
        second_chain, = self.chain_tx([self.wallet.get_utxo()])

        # Check mempool has DEFAULT_ANCESTOR_LIMIT + 1 transactions in it
        assert_equal(len(self.nodes[0].getrawmempool()), DEFAULT_ANCESTOR_LIMIT + 1)

        # Adding one more transaction on to the chain should fail.
        assert_raises_rpc_error(-26, "too-long-mempool-chain, too many unconfirmed ancestors [limit: 25]", self.chain_tx, [utxo])
        # ...even if it chains on from some point in the middle of the chain.
        assert_raises_rpc_error(-26, "too-long-mempool-chain, too many descendants", self.chain_tx, [chain[2]])
        assert_raises_rpc_error(-26, "too-long-mempool-chain, too many descendants", self.chain_tx, [chain[1]])
        # ...even if it chains on to two parent transactions with one in the chain.
        assert_raises_rpc_error(-26, "too-long-mempool-chain, too many descendants", self.chain_tx, [chain[0], second_chain])
        # ...especially if its > 40k weight
        assert_raises_rpc_error(-26, "too-long-mempool-chain, too many descendants", self.chain_tx, [chain[0]], num_outputs=350)
        # ...not if it's submitted with other transactions
        replacable_tx = self.wallet.create_self_transfer_multi(utxos_to_spend=[chain[0]])
        txns = [replacable_tx["tx"], self.wallet.create_self_transfer_multi(utxos_to_spend=replacable_tx["new_utxos"])["tx"]]
        txns_hex = [tx.serialize().hex() for tx in txns]
        assert_equal(self.nodes[0].testmempoolaccept(txns_hex)[0]["reject-reason"], "too-long-mempool-chain")
        pkg_result = self.nodes[0].submitpackage(txns_hex)
        assert "too-long-mempool-chain" in pkg_result["tx-results"][txns[0].getwtxid()]["error"]
        assert_equal(pkg_result["tx-results"][txns[1].getwtxid()]["error"], "bad-txns-inputs-missingorspent")
        # But not if it chains directly off the first transaction
        self.nodes[0].sendrawtransaction(replacable_tx["hex"])
        # and the second chain should work just fine
        self.chain_tx([second_chain])

        # Make sure we can RBF the chain which used our carve-out rule
        replacement_tx = replacable_tx["tx"]
        replacement_tx.vout[0].nValue -= 1000000
        self.nodes[0].sendrawtransaction(replacement_tx.serialize().hex())

        # Finally, check that we added two transactions
        assert_equal(len(self.nodes[0].getrawmempool()), DEFAULT_ANCESTOR_LIMIT + 3)


if __name__ == '__main__':
    MempoolPackagesTest().main()
