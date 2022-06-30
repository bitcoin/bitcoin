#!/usr/bin/env python3
# Copyright (c) 2015-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test transaction signing using the signrawtransactionwithkey RPC."""

from test_framework.test_framework import BitcoinTestFramework

class SignRawTransactionWithKeyTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 2

    def send_to_address(self, addr, amount):
        input = {"txid": self.nodes[0].getblock(self.block_hash[self.blk_idx])["tx"][0], "vout": 0}
        output = {addr: amount}
        self.blk_idx += 1
        rawtx = self.nodes[0].createrawtransaction([input], output)
        txid = self.nodes[0].sendrawtransaction(self.nodes[0].signrawtransactionwithkey(rawtx, [self.nodes[0].get_deterministic_priv_key().key])["hex"], 0)
        return txid

    def successful_signing_test(self):
        """Create and sign a valid raw transaction with one input.

        Expected results:

        1) The transaction has a complete set of signatures
        2) No script verification error occurred"""
        self.log.info("Test valid raw transaction with one input")
        privKeys = ['cUeKHd5orzT3mz8P9pxyREHfsWtVfgsfDjiZZBcjUBAaGk1BTj7N', 'cVKpPfVKSJxKqVpE9awvXNWuLHCa5j5tiE7K6zbUSptFpTEtiFrA']

        inputs = [
            # Valid pay-to-pubkey scripts
            {'txid': '9b907ef1e3c26fc71fe4a4b3580bc75264112f95050014157059c736f0202e71', 'vout': 0,
             'scriptPubKey': '76a91460baa0f494b38ce3c940dea67f3804dc52d1fb9488ac'},
            {'txid': '83a4f6a6b73660e13ee6cb3c6063fa3759c50c9b7521d0536022961898f4fb02', 'vout': 0,
             'scriptPubKey': '76a914669b857c03a5ed269d5d85a1ffac9ed5d663072788ac'},
        ]

        outputs = {'ycwedq2f3sz2Yf9JqZsBCQPxp18WU3Hp4J': 0.1}

        rawTx = self.nodes[0].createrawtransaction(inputs, outputs)
        rawTxSigned = self.nodes[0].signrawtransactionwithkey(rawTx, privKeys, inputs)

        # 1) The transaction has a complete set of signatures
        assert rawTxSigned['complete']

        # 2) No script verification error occurred
        assert 'errors' not in rawTxSigned

    def run_test(self):
        self.successful_signing_test()


if __name__ == '__main__':
    SignRawTransactionWithKeyTest().main()
