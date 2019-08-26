#!/usr/bin/env python3
# Copyright (c) 2015-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test multisig RPCs"""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_raises_rpc_error,
    assert_equal,
)
from test_framework.key import ECPubKey

import binascii
import decimal
import itertools

class RpcCreateMultiSigTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 3

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def get_keys(self):
        node0, node1, node2 = self.nodes
        add = [node1.getnewaddress() for _ in range(self.nkeys)]
        self.pub = [node1.getaddressinfo(a)["pubkey"] for a in add]
        self.priv = [node1.dumpprivkey(a) for a in add]
        self.final = node2.getnewaddress()

    def run_test(self):
        node0, node1, node2 = self.nodes

        self.check_addmultisigaddress_errors()

        self.log.info('Generating blocks ...')
        node0.generate(149)
        self.sync_all()

        self.moved = 0
        for self.nkeys in [3, 5]:
            for self.nsigs in [2, 3]:
                for self.output_type in ["bech32", "p2sh-segwit", "legacy"]:
                    self.get_keys()
                    self.do_multisig()

        self.checkbalances()

        # Test mixed compressed and uncompressed pubkeys
        self.log.info('Mixed compressed and uncompressed multisigs are not allowed')
        pk0 = node0.getaddressinfo(node0.getnewaddress())['pubkey']
        pk1 = node1.getaddressinfo(node1.getnewaddress())['pubkey']
        pk2 = node2.getaddressinfo(node2.getnewaddress())['pubkey']

        # decompress pk2
        pk_obj = ECPubKey()
        pk_obj.set(binascii.unhexlify(pk2))
        pk_obj.compressed = False
        pk2 = binascii.hexlify(pk_obj.get_bytes()).decode()

        # Check all permutations of keys because order matters apparently
        for keys in itertools.permutations([pk0, pk1, pk2]):
            # Results should be the same as this legacy one
            legacy_addr = node0.createmultisig(2, keys, 'legacy')['address']
            assert_equal(legacy_addr, node0.addmultisigaddress(2, keys, '', 'legacy')['address'])

            # Generate addresses with the segwit types. These should all make legacy addresses
            assert_equal(legacy_addr, node0.createmultisig(2, keys, 'bech32')['address'])
            assert_equal(legacy_addr, node0.createmultisig(2, keys, 'p2sh-segwit')['address'])
            assert_equal(legacy_addr, node0.addmultisigaddress(2, keys, '', 'bech32')['address'])
            assert_equal(legacy_addr, node0.addmultisigaddress(2, keys, '', 'p2sh-segwit')['address'])

    def check_addmultisigaddress_errors(self):
        self.log.info('Check that addmultisigaddress fails when the private keys are missing')
        addresses = [self.nodes[1].getnewaddress(address_type='legacy') for _ in range(2)]
        assert_raises_rpc_error(-5, 'no full public key for address', lambda: self.nodes[0].addmultisigaddress(nrequired=1, keys=addresses))
        for a in addresses:
            # Importing all addresses should not change the result
            self.nodes[0].importaddress(a)
        assert_raises_rpc_error(-5, 'no full public key for address', lambda: self.nodes[0].addmultisigaddress(nrequired=1, keys=addresses))

    def checkbalances(self):
        node0, node1, node2 = self.nodes
        node0.generate(100)
        self.sync_all()

        bal0 = node0.getbalance()
        bal1 = node1.getbalance()
        bal2 = node2.getbalance()

        height = node0.getblockchaininfo()["blocks"]
        assert 150 < height < 350
        total = 149 * 50 + (height - 149 - 100) * 25
        assert bal1 == 0
        assert bal2 == self.moved
        assert bal0 + bal1 + bal2 == total

    def do_multisig(self):
        node0, node1, node2 = self.nodes

        msig = node2.createmultisig(self.nsigs, self.pub, self.output_type)
        madd = msig["address"]
        mredeem = msig["redeemScript"]
        if self.output_type == 'bech32':
            assert madd[0:4] == "bcrt"  # actually a bech32 address

        # compare against addmultisigaddress
        msigw = node1.addmultisigaddress(self.nsigs, self.pub, None, self.output_type)
        maddw = msigw["address"]
        mredeemw = msigw["redeemScript"]
        # addmultisigiaddress and createmultisig work the same
        assert maddw == madd
        assert mredeemw == mredeem

        txid = node0.sendtoaddress(madd, 40)

        tx = node0.getrawtransaction(txid, True)
        vout = [v["n"] for v in tx["vout"] if madd in v["scriptPubKey"].get("addresses", [])]
        assert len(vout) == 1
        vout = vout[0]
        scriptPubKey = tx["vout"][vout]["scriptPubKey"]["hex"]
        value = tx["vout"][vout]["value"]
        prevtxs = [{"txid": txid, "vout": vout, "scriptPubKey": scriptPubKey, "redeemScript": mredeem, "amount": value}]

        node0.generate(1)

        outval = value - decimal.Decimal("0.00001000")
        rawtx = node2.createrawtransaction([{"txid": txid, "vout": vout}], [{self.final: outval}])

        prevtx_err = dict(prevtxs[0])
        del prevtx_err["redeemScript"]

        assert_raises_rpc_error(-8, "Missing redeemScript/witnessScript", node2.signrawtransactionwithkey, rawtx, self.priv[0:self.nsigs-1], [prevtx_err])

        rawtx2 = node2.signrawtransactionwithkey(rawtx, self.priv[0:self.nsigs - 1], prevtxs)
        rawtx3 = node2.signrawtransactionwithkey(rawtx2["hex"], [self.priv[-1]], prevtxs)

        self.moved += outval
        tx = node0.sendrawtransaction(rawtx3["hex"], True)
        blk = node0.generate(1)[0]
        assert tx in node0.getblock(blk)["tx"]

        txinfo = node0.getrawtransaction(tx, True, blk)
        self.log.info("n/m=%d/%d %s size=%d vsize=%d weight=%d" % (self.nsigs, self.nkeys, self.output_type, txinfo["size"], txinfo["vsize"], txinfo["weight"]))


if __name__ == '__main__':
    RpcCreateMultiSigTest().main()
