#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the listtransactions API."""
from decimal import Decimal
from io import BytesIO

from test_framework.messages import COIN, CTransaction
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_array_result,
    assert_equal,
    hex_str_to_bytes,
)

def tx_from_hex(hexstring):
    tx = CTransaction()
    f = BytesIO(hex_str_to_bytes(hexstring))
    tx.deserialize(f)
    return tx

class ListTransactionsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.nodes[0].generate(1)  # Get out of IBD
        self.sync_all()
        # Simple send, 0 to 1:
        txid = self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 0.1)
        self.sync_all()
        assert_array_result(self.nodes[0].listtransactions(),
                            {"txid": txid},
                            {"category": "send", "amount": Decimal("-0.1"), "confirmations": 0})
        assert_array_result(self.nodes[1].listtransactions(),
                            {"txid": txid},
                            {"category": "receive", "amount": Decimal("0.1"), "confirmations": 0})
        # mine a block, confirmations should change:
        blockhash = self.nodes[0].generate(1)[0]
        blockheight = self.nodes[0].getblockheader(blockhash)['height']
        self.sync_all()
        assert_array_result(self.nodes[0].listtransactions(),
                            {"txid": txid},
                            {"category": "send", "amount": Decimal("-0.1"), "confirmations": 1, "blockhash": blockhash, "blockheight": blockheight})
        assert_array_result(self.nodes[1].listtransactions(),
                            {"txid": txid},
                            {"category": "receive", "amount": Decimal("0.1"), "confirmations": 1, "blockhash": blockhash, "blockheight": blockheight})

        # send-to-self:
        txid = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 0.2)
        assert_array_result(self.nodes[0].listtransactions(),
                            {"txid": txid, "category": "send"},
                            {"amount": Decimal("-0.2")})
        assert_array_result(self.nodes[0].listtransactions(),
                            {"txid": txid, "category": "receive"},
                            {"amount": Decimal("0.2")})

        # sendmany from node1: twice to self, twice to node2:
        send_to = {self.nodes[0].getnewaddress(): 0.11,
                   self.nodes[1].getnewaddress(): 0.22,
                   self.nodes[0].getnewaddress(): 0.33,
                   self.nodes[1].getnewaddress(): 0.44}
        txid = self.nodes[1].sendmany("", send_to)
        self.sync_all()
        assert_array_result(self.nodes[1].listtransactions(),
                            {"category": "send", "amount": Decimal("-0.11")},
                            {"txid": txid})
        assert_array_result(self.nodes[0].listtransactions(),
                            {"category": "receive", "amount": Decimal("0.11")},
                            {"txid": txid})
        assert_array_result(self.nodes[1].listtransactions(),
                            {"category": "send", "amount": Decimal("-0.22")},
                            {"txid": txid})
        assert_array_result(self.nodes[1].listtransactions(),
                            {"category": "receive", "amount": Decimal("0.22")},
                            {"txid": txid})
        assert_array_result(self.nodes[1].listtransactions(),
                            {"category": "send", "amount": Decimal("-0.33")},
                            {"txid": txid})
        assert_array_result(self.nodes[0].listtransactions(),
                            {"category": "receive", "amount": Decimal("0.33")},
                            {"txid": txid})
        assert_array_result(self.nodes[1].listtransactions(),
                            {"category": "send", "amount": Decimal("-0.44")},
                            {"txid": txid})
        assert_array_result(self.nodes[1].listtransactions(),
                            {"category": "receive", "amount": Decimal("0.44")},
                            {"txid": txid})

        if not self.options.descriptors:
            # include_watchonly is a legacy wallet feature, so don't test it for descriptor wallets
            pubkey = self.nodes[1].getaddressinfo(self.nodes[1].getnewaddress())['pubkey']
            multisig = self.nodes[1].createmultisig(1, [pubkey])
            self.nodes[0].importaddress(multisig["redeemScript"], "watchonly", False, True)
            txid = self.nodes[1].sendtoaddress(multisig["address"], 0.1)
            self.nodes[1].generate(1)
            self.sync_all()
            assert_equal(len(self.nodes[0].listtransactions(label="watchonly", include_watchonly=True)), 1)
            assert_equal(len(self.nodes[0].listtransactions(dummy="watchonly", include_watchonly=True)), 1)
            assert len(self.nodes[0].listtransactions(label="watchonly", count=100, include_watchonly=False)) == 0
            assert_array_result(self.nodes[0].listtransactions(label="watchonly", count=100, include_watchonly=True),
                                {"category": "receive", "amount": Decimal("0.1")},
                                {"txid": txid, "label": "watchonly"})

        self.run_rbf_opt_in_test()

    # Check that the opt-in-rbf flag works properly, for sent and received
    # transactions.
    def run_rbf_opt_in_test(self):
        # Check whether a transaction signals opt-in RBF itself
        def is_opt_in(node, txid):
            rawtx = node.getrawtransaction(txid, 1)
            for x in rawtx["vin"]:
                if x["sequence"] < 0xfffffffe:
                    return True
            return False

        # Find an unconfirmed output matching a certain txid
        def get_unconfirmed_utxo_entry(node, txid_to_match):
            utxo = node.listunspent(0, 0)
            for i in utxo:
                if i["txid"] == txid_to_match:
                    return i
            return None

        # 1. Chain a few transactions that don't opt-in.
        txid_1 = self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 1)
        assert not is_opt_in(self.nodes[0], txid_1)
        assert_array_result(self.nodes[0].listtransactions(), {"txid": txid_1}, {"bip125-replaceable": "no"})
        self.sync_mempools()
        assert_array_result(self.nodes[1].listtransactions(), {"txid": txid_1}, {"bip125-replaceable": "no"})

        # Tx2 will build off txid_1, still not opting in to RBF.
        utxo_to_use = get_unconfirmed_utxo_entry(self.nodes[0], txid_1)
        assert_equal(utxo_to_use["safe"], True)
        utxo_to_use = get_unconfirmed_utxo_entry(self.nodes[1], txid_1)
        utxo_to_use = get_unconfirmed_utxo_entry(self.nodes[1], txid_1)
        assert_equal(utxo_to_use["safe"], False)

        # Create tx2 using createrawtransaction
        inputs = [{"txid": utxo_to_use["txid"], "vout": utxo_to_use["vout"]}]
        outputs = {self.nodes[0].getnewaddress(): 0.999}
        tx2 = self.nodes[1].createrawtransaction(inputs, outputs)
        tx2_signed = self.nodes[1].signrawtransactionwithwallet(tx2)["hex"]
        txid_2 = self.nodes[1].sendrawtransaction(tx2_signed)

        # ...and check the result
        assert not is_opt_in(self.nodes[1], txid_2)
        assert_array_result(self.nodes[1].listtransactions(), {"txid": txid_2}, {"bip125-replaceable": "no"})
        self.sync_mempools()
        assert_array_result(self.nodes[0].listtransactions(), {"txid": txid_2}, {"bip125-replaceable": "no"})

        # Tx3 will opt-in to RBF
        utxo_to_use = get_unconfirmed_utxo_entry(self.nodes[0], txid_2)
        inputs = [{"txid": txid_2, "vout": utxo_to_use["vout"]}]
        outputs = {self.nodes[1].getnewaddress(): 0.998}
        tx3 = self.nodes[0].createrawtransaction(inputs, outputs)
        tx3_modified = tx_from_hex(tx3)
        tx3_modified.vin[0].nSequence = 0
        tx3 = tx3_modified.serialize().hex()
        tx3_signed = self.nodes[0].signrawtransactionwithwallet(tx3)['hex']
        txid_3 = self.nodes[0].sendrawtransaction(tx3_signed)

        assert is_opt_in(self.nodes[0], txid_3)
        assert_array_result(self.nodes[0].listtransactions(), {"txid": txid_3}, {"bip125-replaceable": "yes"})
        self.sync_mempools()
        assert_array_result(self.nodes[1].listtransactions(), {"txid": txid_3}, {"bip125-replaceable": "yes"})

        # Tx4 will chain off tx3.  Doesn't signal itself, but depends on one
        # that does.
        utxo_to_use = get_unconfirmed_utxo_entry(self.nodes[1], txid_3)
        inputs = [{"txid": txid_3, "vout": utxo_to_use["vout"]}]
        outputs = {self.nodes[0].getnewaddress(): 0.997}
        tx4 = self.nodes[1].createrawtransaction(inputs, outputs)
        tx4_signed = self.nodes[1].signrawtransactionwithwallet(tx4)["hex"]
        txid_4 = self.nodes[1].sendrawtransaction(tx4_signed)

        assert not is_opt_in(self.nodes[1], txid_4)
        assert_array_result(self.nodes[1].listtransactions(), {"txid": txid_4}, {"bip125-replaceable": "yes"})
        self.sync_mempools()
        assert_array_result(self.nodes[0].listtransactions(), {"txid": txid_4}, {"bip125-replaceable": "yes"})

        # Replace tx3, and check that tx4 becomes unknown
        tx3_b = tx3_modified
        tx3_b.vout[0].nValue -= int(Decimal("0.004") * COIN)  # bump the fee
        tx3_b = tx3_b.serialize().hex()
        tx3_b_signed = self.nodes[0].signrawtransactionwithwallet(tx3_b)['hex']
        txid_3b = self.nodes[0].sendrawtransaction(tx3_b_signed, 0)
        assert is_opt_in(self.nodes[0], txid_3b)

        assert_array_result(self.nodes[0].listtransactions(), {"txid": txid_4}, {"bip125-replaceable": "unknown"})
        self.sync_mempools()
        assert_array_result(self.nodes[1].listtransactions(), {"txid": txid_4}, {"bip125-replaceable": "unknown"})

        # Check gettransaction as well:
        for n in self.nodes[0:2]:
            assert_equal(n.gettransaction(txid_1)["bip125-replaceable"], "no")
            assert_equal(n.gettransaction(txid_2)["bip125-replaceable"], "no")
            assert_equal(n.gettransaction(txid_3)["bip125-replaceable"], "yes")
            assert_equal(n.gettransaction(txid_3b)["bip125-replaceable"], "yes")
            assert_equal(n.gettransaction(txid_4)["bip125-replaceable"], "unknown")

        # After mining a transaction, it's no longer BIP125-replaceable
        self.nodes[0].generate(1)
        assert txid_3b not in self.nodes[0].getrawmempool()
        assert_equal(self.nodes[0].gettransaction(txid_3b)["bip125-replaceable"], "no")
        assert_equal(self.nodes[0].gettransaction(txid_4)["bip125-replaceable"], "unknown")

if __name__ == '__main__':
    ListTransactionsTest().main()
