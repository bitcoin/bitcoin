#!/usr/bin/env python3
# Copyright (c) 2014-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the listtransactions API."""

from decimal import Decimal
import os
import shutil

from test_framework.messages import (
    COIN,
    tx_from_hex,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_array_result,
    assert_equal,
    assert_raises_rpc_error,
)


class ListTransactionsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 3
        # whitelist peers to speed up tx relay / mempool sync
        self.noban_tx_relay = True
        self.extra_args = [["-walletrbf=0"]] * self.num_nodes

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.log.info("Test simple send from node0 to node1")
        txid = self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 0.1)
        self.sync_all()
        assert_array_result(self.nodes[0].listtransactions(),
                            {"txid": txid},
                            {"category": "send", "amount": Decimal("-0.1"), "confirmations": 0, "trusted": True})
        assert_array_result(self.nodes[1].listtransactions(),
                            {"txid": txid},
                            {"category": "receive", "amount": Decimal("0.1"), "confirmations": 0, "trusted": False})
        self.log.info("Test confirmations change after mining a block")
        blockhash = self.generate(self.nodes[0], 1)[0]
        blockheight = self.nodes[0].getblockheader(blockhash)['height']
        assert_array_result(self.nodes[0].listtransactions(),
                            {"txid": txid},
                            {"category": "send", "amount": Decimal("-0.1"), "confirmations": 1, "blockhash": blockhash, "blockheight": blockheight})
        assert_array_result(self.nodes[1].listtransactions(),
                            {"txid": txid},
                            {"category": "receive", "amount": Decimal("0.1"), "confirmations": 1, "blockhash": blockhash, "blockheight": blockheight})

        self.log.info("Test send-to-self on node0")
        txid = self.nodes[0].sendtoaddress(self.nodes[0].getnewaddress(), 0.2)
        assert_array_result(self.nodes[0].listtransactions(),
                            {"txid": txid, "category": "send"},
                            {"amount": Decimal("-0.2")})
        assert_array_result(self.nodes[0].listtransactions(),
                            {"txid": txid, "category": "receive"},
                            {"amount": Decimal("0.2")})

        self.log.info("Test sendmany from node1: twice to self, twice to node0")
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

        self.run_rbf_opt_in_test()
        self.run_externally_generated_address_test()
        self.run_coinjoin_test()
        self.run_invalid_parameters_test()
        self.test_op_return()

    def run_rbf_opt_in_test(self):
        """Test the opt-in-rbf flag for sent and received transactions."""

        def is_opt_in(node, txid):
            """Check whether a transaction signals opt-in RBF itself."""
            rawtx = node.getrawtransaction(txid, 1)
            for x in rawtx["vin"]:
                if x["sequence"] < 0xfffffffe:
                    return True
            return False

        def get_unconfirmed_utxo_entry(node, txid_to_match):
            """Find an unconfirmed output matching a certain txid."""
            utxo = node.listunspent(0, 0)
            for i in utxo:
                if i["txid"] == txid_to_match:
                    return i
            return None

        self.log.info("Test txs w/o opt-in RBF (bip125-replaceable=no)")
        # Chain a few transactions that don't opt in.
        txid_1 = self.nodes[0].sendtoaddress(self.nodes[1].getnewaddress(), 1)
        assert not is_opt_in(self.nodes[0], txid_1)
        assert_array_result(self.nodes[0].listtransactions(), {"txid": txid_1}, {"bip125-replaceable": "no"})
        self.sync_mempools()
        assert_array_result(self.nodes[1].listtransactions(), {"txid": txid_1}, {"bip125-replaceable": "no"})

        # Tx2 will build off tx1, still not opting in to RBF.
        utxo_to_use = get_unconfirmed_utxo_entry(self.nodes[0], txid_1)
        assert_equal(utxo_to_use["safe"], True)
        utxo_to_use = get_unconfirmed_utxo_entry(self.nodes[1], txid_1)
        assert_equal(utxo_to_use["safe"], False)

        # Create tx2 using createrawtransaction
        inputs = [{"txid": utxo_to_use["txid"], "vout": utxo_to_use["vout"]}]
        outputs = {self.nodes[0].getnewaddress(): 0.999}
        tx2 = self.nodes[1].createrawtransaction(inputs=inputs, outputs=outputs, replaceable=False)
        tx2_signed = self.nodes[1].signrawtransactionwithwallet(tx2)["hex"]
        txid_2 = self.nodes[1].sendrawtransaction(tx2_signed)

        # ...and check the result
        assert not is_opt_in(self.nodes[1], txid_2)
        assert_array_result(self.nodes[1].listtransactions(), {"txid": txid_2}, {"bip125-replaceable": "no"})
        self.sync_mempools()
        assert_array_result(self.nodes[0].listtransactions(), {"txid": txid_2}, {"bip125-replaceable": "no"})

        self.log.info("Test txs with opt-in RBF (bip125-replaceable=yes)")
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
        tx4 = self.nodes[1].createrawtransaction(inputs=inputs, outputs=outputs, replaceable=False)
        tx4_signed = self.nodes[1].signrawtransactionwithwallet(tx4)["hex"]
        txid_4 = self.nodes[1].sendrawtransaction(tx4_signed)

        assert not is_opt_in(self.nodes[1], txid_4)
        assert_array_result(self.nodes[1].listtransactions(), {"txid": txid_4}, {"bip125-replaceable": "yes"})
        self.sync_mempools()
        assert_array_result(self.nodes[0].listtransactions(), {"txid": txid_4}, {"bip125-replaceable": "yes"})

        self.log.info("Test tx with unknown RBF state (bip125-replaceable=unknown)")
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

        self.log.info("Test bip125-replaceable status with gettransaction RPC")
        for n in self.nodes[0:2]:
            assert_equal(n.gettransaction(txid_1)["bip125-replaceable"], "no")
            assert_equal(n.gettransaction(txid_2)["bip125-replaceable"], "no")
            assert_equal(n.gettransaction(txid_3)["bip125-replaceable"], "yes")
            assert_equal(n.gettransaction(txid_3b)["bip125-replaceable"], "yes")
            assert_equal(n.gettransaction(txid_4)["bip125-replaceable"], "unknown")

        self.log.info("Test bip125-replaceable status with listsinceblock")
        for n in self.nodes[0:2]:
            txs = {tx['txid']: tx['bip125-replaceable'] for tx in n.listsinceblock()['transactions']}
            assert_equal(txs[txid_1], "no")
            assert_equal(txs[txid_2], "no")
            assert_equal(txs[txid_3], "yes")
            assert_equal(txs[txid_3b], "yes")
            assert_equal(txs[txid_4], "unknown")

        self.log.info("Test mined transactions are no longer bip125-replaceable")
        self.generate(self.nodes[0], 1)
        assert txid_3b not in self.nodes[0].getrawmempool()
        assert_equal(self.nodes[0].gettransaction(txid_3b)["bip125-replaceable"], "no")
        assert_equal(self.nodes[0].gettransaction(txid_4)["bip125-replaceable"], "unknown")

    def run_externally_generated_address_test(self):
        """Test behavior when receiving address is not in the address book."""

        self.log.info("Setup the same wallet on two nodes")
        # refill keypool otherwise the second node wouldn't recognize addresses generated on the first nodes
        self.nodes[0].keypoolrefill(1000)
        self.stop_nodes()
        wallet0 = os.path.join(self.nodes[0].chain_path, self.default_wallet_name, "wallet.dat")
        wallet2 = os.path.join(self.nodes[2].chain_path, self.default_wallet_name, "wallet.dat")
        shutil.copyfile(wallet0, wallet2)
        self.start_nodes()
        # reconnect nodes
        self.connect_nodes(0, 1)
        self.connect_nodes(1, 2)
        self.connect_nodes(2, 0)

        addr1 = self.nodes[0].getnewaddress("pizza1", 'legacy')
        addr2 = self.nodes[0].getnewaddress("pizza2", 'p2sh-segwit')
        addr3 = self.nodes[0].getnewaddress("pizza3", 'bech32')

        self.log.info("Send to externally generated addresses")
        # send to an address beyond the next to be generated to test the keypool gap
        self.nodes[1].sendtoaddress(addr3, "0.001")
        self.generate(self.nodes[1], 1)

        # send to an address that is already marked as used due to the keypool gap mechanics
        self.nodes[1].sendtoaddress(addr2, "0.001")
        self.generate(self.nodes[1], 1)

        # send to self transaction
        self.nodes[0].sendtoaddress(addr1, "0.001")
        self.generate(self.nodes[0], 1)

        self.log.info("Verify listtransactions is the same regardless of where the address was generated")
        transactions0 = self.nodes[0].listtransactions()
        transactions2 = self.nodes[2].listtransactions()

        # normalize results: remove fields that normally could differ and sort
        def normalize_list(txs):
            for tx in txs:
                tx.pop('label', None)
                tx.pop('time', None)
                tx.pop('timereceived', None)
            txs.sort(key=lambda x: x['txid'])

        normalize_list(transactions0)
        normalize_list(transactions2)
        assert_equal(transactions0, transactions2)

        self.log.info("Verify labels are persistent on the node that generated the addresses")
        assert_equal(['pizza1'], self.nodes[0].getaddressinfo(addr1)['labels'])
        assert_equal(['pizza2'], self.nodes[0].getaddressinfo(addr2)['labels'])
        assert_equal(['pizza3'], self.nodes[0].getaddressinfo(addr3)['labels'])

    def run_coinjoin_test(self):
        self.log.info('Check "coin-join" transaction')
        input_0 = next(i for i in self.nodes[0].listunspent(query_options={"minimumAmount": 0.2}, include_unsafe=False))
        input_1 = next(i for i in self.nodes[1].listunspent(query_options={"minimumAmount": 0.2}, include_unsafe=False))
        raw_hex = self.nodes[0].createrawtransaction(
            inputs=[
                {
                    "txid": input_0["txid"],
                    "vout": input_0["vout"],
                },
                {
                    "txid": input_1["txid"],
                    "vout": input_1["vout"],
                },
            ],
            outputs={
                self.nodes[0].getnewaddress(): 0.123,
                self.nodes[1].getnewaddress(): 0.123,
            },
        )
        raw_hex = self.nodes[0].signrawtransactionwithwallet(raw_hex)["hex"]
        raw_hex = self.nodes[1].signrawtransactionwithwallet(raw_hex)["hex"]
        txid_join = self.nodes[0].sendrawtransaction(hexstring=raw_hex, maxfeerate=0)
        fee_join = self.nodes[0].getmempoolentry(txid_join)["fees"]["base"]
        # Fee should be correct: assert_equal(fee_join, self.nodes[0].gettransaction(txid_join)['fee'])
        # But it is not, see for example https://github.com/bitcoin/bitcoin/issues/14136:
        assert fee_join != self.nodes[0].gettransaction(txid_join)["fee"]

    def run_invalid_parameters_test(self):
        self.log.info("Test listtransactions RPC parameter validity")
        assert_raises_rpc_error(-8, 'Label argument must be a valid label name or "*".', self.nodes[0].listtransactions, label="")
        self.nodes[0].listtransactions(label="*")
        assert_raises_rpc_error(-8, "Negative count", self.nodes[0].listtransactions, count=-1)
        assert_raises_rpc_error(-8, "Negative from", self.nodes[0].listtransactions, skip=-1)

    def test_op_return(self):
        """Test if OP_RETURN outputs will be displayed correctly."""
        raw_tx = self.nodes[0].createrawtransaction([], [{'data': 'aa'}])
        funded_tx = self.nodes[0].fundrawtransaction(raw_tx)
        signed_tx = self.nodes[0].signrawtransactionwithwallet(funded_tx['hex'])
        tx_id = self.nodes[0].sendrawtransaction(signed_tx['hex'])

        op_ret_tx = [tx for tx in self.nodes[0].listtransactions() if tx['txid'] == tx_id][0]

        assert 'address' not in op_ret_tx


if __name__ == '__main__':
    ListTransactionsTest().main()
