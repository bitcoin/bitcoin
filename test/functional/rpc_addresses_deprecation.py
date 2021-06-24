#!/usr/bin/env python3
# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test deprecation of reqSigs and addresses RPC fields."""

from test_framework.messages import (
    tx_from_hex,
)
from test_framework.test_framework import SyscoinTestFramework
from test_framework.util import (
    assert_equal,
    hex_str_to_bytes
)


class AddressesDeprecationTest(SyscoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.extra_args = [[], ["-deprecatedrpc=addresses"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.test_addresses_deprecation()

    def test_addresses_deprecation(self):
        node = self.nodes[0]
        coin = node.listunspent().pop()

        inputs = [{'txid': coin['txid'], 'vout': coin['vout']}]
        outputs = {node.getnewaddress(): 0.99}
        raw = node.createrawtransaction(inputs, outputs)
        signed = node.signrawtransactionwithwallet(raw)['hex']

        # This transaction is derived from test/util/data/txcreatemultisig1.json
        tx = tx_from_hex(signed)
        tx.vout[0].scriptPubKey = hex_str_to_bytes("522102a5613bd857b7048924264d1e70e08fb2a7e6527d32b7ab1bb993ac59964ff39721021ac43c7ff740014c3b33737ede99c967e4764553d1b2b83db77c83b8715fa72d2102df2089105c77f266fa11a9d33f05c735234075f2e8780824c6b709415f9fb48553ae")
        tx_signed = node.signrawtransactionwithwallet(tx.serialize().hex())['hex']
        txid = node.sendrawtransaction(hexstring=tx_signed, maxfeerate=0)

        self.log.info("Test RPCResult scriptPubKey no longer returns the fields addresses or reqSigs by default")
        hash = node.generateblock(output=node.getnewaddress(), transactions=[txid])['hash']
        # Ensure both nodes have the newly generated block on disk.
        self.sync_blocks()
        script_pub_key = node.getblock(blockhash=hash, verbose=2)['tx'][-1]['vout'][0]['scriptPubKey']
        assert 'addresses' not in script_pub_key and 'reqSigs' not in script_pub_key

        self.log.info("Test RPCResult scriptPubKey returns the addresses field with -deprecatedrpc=addresses")
        script_pub_key = self.nodes[1].getblock(blockhash=hash, verbose=2)['tx'][-1]['vout'][0]['scriptPubKey']
        assert_equal(script_pub_key['addresses'], ['mvKDK6D54HU8wQumJBLHY95eq5iHFqXSBz', 'mv3rHCQSwKp2BLSuMHD8uCS32LW5xiNAA5', 'mirrsyhAQYzo5CwVhcaYJKwUJu1WJRCRJe'])
        assert_equal(script_pub_key['reqSigs'], 2)


if __name__ == "__main__":
    AddressesDeprecationTest().main()
