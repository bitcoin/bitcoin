#!/usr/bin/env python3
# Copyright (c) 2015-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test transaction signing using the signrawtransactionwithkey RPC."""

from test_framework.blocktools import (
    COINBASE_MATURITY,
)
from test_framework.address import (
    address_to_scriptpubkey,
    script_to_p2sh,
)
from test_framework.key import ECKey
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    find_vout_for_address,
)
from test_framework.script_util import (
    key_to_p2pk_script,
    key_to_p2pkh_script,
    script_to_p2sh_p2wsh_script,
    script_to_p2wsh_script,
)
from test_framework.wallet_util import (
    bytes_to_wif,
)

from decimal import (
    Decimal,
)
from test_framework.wallet import (
    getnewdestination,
)


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

        outputs = {'mpLQjfK79b7CCV4VMJWEWAj5Mpx8Up5zxB': 0.1}

        rawTx = self.nodes[0].createrawtransaction(inputs, outputs)
        rawTxSigned = self.nodes[0].signrawtransactionwithkey(rawTx, privKeys, inputs)

        # 1) The transaction has a complete set of signatures
        assert rawTxSigned['complete']

        # 2) No script verification error occurred
        assert 'errors' not in rawTxSigned

    def witness_script_test(self):
        self.log.info("Test signing transaction to P2SH-P2WSH addresses without wallet")
        # Create a new P2SH-P2WSH 1-of-1 multisig address:
        eckey = ECKey()
        eckey.generate()
        embedded_privkey = bytes_to_wif(eckey.get_bytes())
        embedded_pubkey = eckey.get_pubkey().get_bytes().hex()
        p2sh_p2wsh_address = self.nodes[1].createmultisig(1, [embedded_pubkey], "p2sh-segwit")
        # send transaction to P2SH-P2WSH 1-of-1 multisig address
        self.block_hash = self.generate(self.nodes[0], COINBASE_MATURITY + 1)
        self.blk_idx = 0
        self.send_to_address(p2sh_p2wsh_address["address"], 49.999)
        self.generate(self.nodes[0], 1)
        # Get the UTXO info from scantxoutset
        unspent_output = self.nodes[1].scantxoutset('start', [p2sh_p2wsh_address['descriptor']])['unspents'][0]
        spk = script_to_p2sh_p2wsh_script(p2sh_p2wsh_address['redeemScript']).hex()
        unspent_output['witnessScript'] = p2sh_p2wsh_address['redeemScript']
        unspent_output['redeemScript'] = script_to_p2wsh_script(unspent_output['witnessScript']).hex()
        assert_equal(spk, unspent_output['scriptPubKey'])
        # Now create and sign a transaction spending that output on node[0], which doesn't know the scripts or keys
        spending_tx = self.nodes[0].createrawtransaction([unspent_output], {getnewdestination()[2]: Decimal("49.998")})
        spending_tx_signed = self.nodes[0].signrawtransactionwithkey(spending_tx, [embedded_privkey], [unspent_output])
        # Check the signing completed successfully
        assert 'complete' in spending_tx_signed
        assert_equal(spending_tx_signed['complete'], True)

        # Now test with P2PKH and P2PK scripts as the witnessScript
        for tx_type in ['P2PKH', 'P2PK']:  # these tests are order-independent
            self.verify_txn_with_witness_script(tx_type)

    def verify_txn_with_witness_script(self, tx_type):
        self.log.info("Test with a {} script as the witnessScript".format(tx_type))
        eckey = ECKey()
        eckey.generate()
        embedded_privkey = bytes_to_wif(eckey.get_bytes())
        embedded_pubkey = eckey.get_pubkey().get_bytes().hex()
        witness_script = {
            'P2PKH': key_to_p2pkh_script(embedded_pubkey).hex(),
            'P2PK': key_to_p2pk_script(embedded_pubkey).hex()
        }.get(tx_type, "Invalid tx_type")
        redeem_script = script_to_p2wsh_script(witness_script).hex()
        addr = script_to_p2sh(redeem_script)
        script_pub_key = address_to_scriptpubkey(addr).hex()
        # Fund that address
        txid = self.send_to_address(addr, 10)
        vout = find_vout_for_address(self.nodes[0], txid, addr)
        self.generate(self.nodes[0], 1)
        # Now create and sign a transaction spending that output on node[0], which doesn't know the scripts or keys
        spending_tx = self.nodes[0].createrawtransaction([{'txid': txid, 'vout': vout}], {getnewdestination()[2]: Decimal("9.999")})
        spending_tx_signed = self.nodes[0].signrawtransactionwithkey(spending_tx, [embedded_privkey], [{'txid': txid, 'vout': vout, 'scriptPubKey': script_pub_key, 'redeemScript': redeem_script, 'witnessScript': witness_script, 'amount': 10}])
        # Check the signing completed successfully
        assert 'complete' in spending_tx_signed
        assert_equal(spending_tx_signed['complete'], True)
        self.nodes[0].sendrawtransaction(spending_tx_signed['hex'])

    def run_test(self):
        self.successful_signing_test()
        self.witness_script_test()


if __name__ == '__main__':
    SignRawTransactionWithKeyTest().main()
