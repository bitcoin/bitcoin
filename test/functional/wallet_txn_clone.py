#!/usr/bin/env python3
# Copyright (c) 2014-2018 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the wallet accounts properly when there are cloned transactions with malleated scriptsigs."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    connect_nodes,
    disconnect_nodes,
    find_output,
    sync_blocks,
)

class TxnMallTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 4

    def add_options(self, parser):
        parser.add_argument("--mineblock", dest="mine_block", default=False, action="store_true",
                            help="Test double-spend of 1-confirmed transaction")
        parser.add_argument("--segwit", dest="segwit", default=False, action="store_true",
                            help="Test behaviour with SegWit txn (which should fail")

    def setup_network(self):
        # Start with split network:
        super(TxnMallTest, self).setup_network()
        disconnect_nodes(self.nodes[1], 2)
        disconnect_nodes(self.nodes[2], 1)

    def run_test(self):
        if self.options.segwit:
            output_type = "p2sh-segwit"
        else:
            output_type = "legacy"

        # All nodes should start with 1,250 BTC:
        starting_balance = 1250
        for i in range(4):
            assert_equal(self.nodes[i].getbalance(), starting_balance)
            self.nodes[i].getnewaddress()  # bug workaround, coins generated assigned to first getnewaddress!

        self.nodes[0].settxfee(.001)

        node0_address1 = self.nodes[0].getnewaddress(address_type=output_type)
        node0_txid1 = self.nodes[0].sendtoaddress(node0_address1, 1219)
        node0_tx1 = self.nodes[0].gettransaction(node0_txid1)

        # lock node0_tx1 output to prevent node0_tx2 from spending it
        self.nodes[0].lockunspent(False, [{'txid' : node0_txid1, 'vout' : find_output(self.nodes[0], node0_txid1, 1219)}])

        node0_address2 = self.nodes[0].getnewaddress(address_type=output_type)
        node0_txid2 = self.nodes[0].sendtoaddress(node0_address2, 29)
        node0_tx2 = self.nodes[0].gettransaction(node0_txid2)
        self.nodes[0].lockunspent(True)

        assert_equal(self.nodes[0].getbalance(),
                     starting_balance + node0_tx1["fee"] + node0_tx2["fee"])

        # Coins are sent to node1_address
        node1_address = self.nodes[1].getnewaddress()

        # Lock the outputs for node0_tx2 to avoid having that as a parent for tx1
        to_lock = []
        for vout in self.nodes[0].decoderawtransaction(node0_tx2['hex'])['vout']:
            to_lock.append({'txid' : node0_txid2, 'vout' : vout['n']})
        self.nodes[0].lockunspent(False, to_lock)

        # Send tx1, and another transaction tx2 that won't be cloned
        txid1 = self.nodes[0].sendtoaddress(node1_address, 40)
        self.nodes[0].lockunspent(True) # Unlock locked outputs

        # Lock the outputs for tx1 to avoid having that as a parent for tx2
        to_lock = []
        for vout in self.nodes[0].decoderawtransaction(self.nodes[0].getrawtransaction(txid1))['vout']:
            to_lock.append({'txid' : txid1, 'vout' : vout['n']})
        self.nodes[0].lockunspent(False, to_lock)
        txid2 = self.nodes[0].sendtoaddress(node1_address, 20)
        self.nodes[0].lockunspent(True)

        # Construct a clone of tx1, to be malleated
        rawtx1 = self.nodes[0].getrawtransaction(txid1, 1)
        clone_inputs = []
        for vin in rawtx1['vin']:
            clone_inputs.append({"txid": vin["txid"], "vout": vin["vout"]})
        clone_outputs = {}
        for vout in rawtx1['vout']:
            clone_outputs[vout["scriptPubKey"]["addresses"][0]] = vout['value']
        clone_locktime = rawtx1["locktime"]
        clone_raw = self.nodes[0].createrawtransaction(clone_inputs, clone_outputs, clone_locktime)

        # createrawtransaction randomizes the order of its outputs, so swap them if necessary.
        # output 0 is at version+#inputs+input+sigstub+sequence+#outputs
        # 40 BTC serialized is 00286bee00000000
        pos0 = 2 * (4 + 1 + 36 + 1 + 4 + 1)
        hex40 = "00286bee00000000"
        output_len = 16 + 2 + 2 * int("0x" + clone_raw[pos0 + 16:pos0 + 16 + 2], 0)
        if (rawtx1["vout"][0]["value"] == 40 and clone_raw[pos0:pos0 + 16] != hex40 or rawtx1["vout"][0]["value"] != 40 and clone_raw[pos0:pos0 + 16] == hex40):
            output0 = clone_raw[pos0:pos0 + output_len]
            output1 = clone_raw[pos0 + output_len:pos0 + 2 * output_len]
            clone_raw = clone_raw[:pos0] + output1 + output0 + clone_raw[pos0 + 2 * output_len:]

        # Use a different signature hash type to sign.  This creates an equivalent but malleated clone.
        # Don't send the clone anywhere yet
        tx1_clone = self.nodes[0].signrawtransactionwithwallet(clone_raw, None, "ALL|ANYONECANPAY")
        assert_equal(tx1_clone["complete"], True)

        # Have node0 mine a block, if requested:
        if (self.options.mine_block):
            self.nodes[0].generate(1)
            sync_blocks(self.nodes[0:2])

        tx1 = self.nodes[0].gettransaction(txid1)
        tx2 = self.nodes[0].gettransaction(txid2)

        # Node0's balance should be starting balance, plus 50BTC for another
        # matured block, minus tx1 and tx2 amounts, and minus transaction fees:
        expected = starting_balance + node0_tx1["fee"] + node0_tx2["fee"]
        if self.options.mine_block:
            expected += 50
        expected += tx1["amount"] + tx1["fee"]
        expected += tx2["amount"] + tx2["fee"]
        assert_equal(self.nodes[0].getbalance(), expected)

        if self.options.mine_block:
            assert_equal(tx1["confirmations"], 1)
            assert_equal(tx2["confirmations"], 1)
        else:
            assert_equal(tx1["confirmations"], 0)
            assert_equal(tx2["confirmations"], 0)

        # Send clone and its parents to miner
        for vin in rawtx1['vin']:
            self.nodes[2].sendrawtransaction(self.nodes[0].getrawtransaction(vin['txid']))
        txid1_clone = self.nodes[2].sendrawtransaction(tx1_clone["hex"])
        if self.options.segwit:
            assert_equal(txid1, txid1_clone)
            return

        # ... mine a block...
        self.nodes[2].generate(1)

        # Reconnect the split network, and sync chain:
        connect_nodes(self.nodes[1], 2)
        for vin in self.nodes[0].decoderawtransaction(tx2['hex'])['vin']:
            self.nodes[2].sendrawtransaction(self.nodes[0].getrawtransaction(vin['txid']))
        self.nodes[2].sendrawtransaction(tx2["hex"])
        self.nodes[2].generate(1)  # Mine another block to make sure we sync
        sync_blocks(self.nodes)

        # Re-fetch transaction info:
        tx1 = self.nodes[0].gettransaction(txid1)
        tx1_clone = self.nodes[0].gettransaction(txid1_clone)
        tx2 = self.nodes[0].gettransaction(txid2)

        # Verify expected confirmations
        assert_equal(tx1["confirmations"], -2)
        assert_equal(tx1_clone["confirmations"], 2)
        assert_equal(tx2["confirmations"], 1)

        # Check node0's total balance; should be same as before the clone, + 100 BTC for 2 matured,
        # less possible orphaned matured subsidy
        expected += 100
        if (self.options.mine_block):
            expected -= 50
        assert_equal(self.nodes[0].getbalance(), expected)

if __name__ == '__main__':
    TxnMallTest().main()
