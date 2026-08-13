#!/usr/bin/env python3
# Copyright (c) 2014-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the wallet accounts properly when there are cloned transactions with malleated scriptsigs."""

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_not_equal,
    assert_raises_rpc_error,
)
from test_framework.messages import (
    COIN,
    tx_from_hex,
)


class TxnMallTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 3
        self.extra_args = [[] for i in range(self.num_nodes)]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def add_options(self, parser):
        parser.add_argument("--mineblock", dest="mine_block", default=False, action="store_true",
                            help="Test double-spend of 1-confirmed transaction")
        parser.add_argument("--segwit", dest="segwit", default=False, action="store_true",
                            help="Test behaviour with SegWit txn (which should fail)")

    def setup_network(self):
        # Start with split network:
        super().setup_network()
        self.disconnect_nodes(1, 2)

    def spend_utxo(self, utxo, outputs):
        inputs = [utxo]
        tx = self.nodes[0].createrawtransaction(inputs, outputs)
        tx = self.nodes[0].fundrawtransaction(tx, fee_rate=100)
        tx = self.nodes[0].signrawtransactionwithwallet(tx['hex'])
        return self.nodes[0].sendrawtransaction(tx['hex'])

    def run_test(self):
        if self.options.segwit:
            output_type = "p2sh-segwit"
        else:
            output_type = "legacy"

        # All nodes should start with 1,250 BTC:
        starting_balance = 1250
        for i in range(3):
            assert_equal(self.nodes[i].getbalance(), starting_balance)

        node0_address1 = self.nodes[0].getnewaddress(address_type=output_type)
        node0_utxo1 = self.create_outpoints(self.nodes[0], outputs=[{node0_address1: 1219}])[0]
        node0_tx1 = self.nodes[0].gettransaction(node0_utxo1['txid'])
        self.nodes[0].lockunspent(False, [node0_utxo1])

        node0_address2 = self.nodes[0].getnewaddress(address_type=output_type)
        node0_utxo2 = self.create_outpoints(self.nodes[0], outputs=[{node0_address2: 29}])[0]
        node0_tx2 = self.nodes[0].gettransaction(node0_utxo2['txid'])

        assert_equal(self.nodes[0].getbalance(),
                     starting_balance + node0_tx1["fee"] + node0_tx2["fee"])

        # Coins are sent to node1_address
        node1_address = self.nodes[1].getnewaddress()

        # Send tx1, and another transaction tx2 that won't be cloned
        txid1 = self.spend_utxo(node0_utxo1, {node1_address: 40})
        txid2 = self.spend_utxo(node0_utxo2, {node1_address: 20})

        # Construct a clone of tx1, to be malleated
        rawtx1 = self.nodes[0].getrawtransaction(txid1, 1)
        clone_inputs = [{"txid": rawtx1["vin"][0]["txid"], "vout": rawtx1["vin"][0]["vout"], "sequence": rawtx1["vin"][0]["sequence"]}]
        clone_outputs = {rawtx1["vout"][0]["scriptPubKey"]["address"]: rawtx1["vout"][0]["value"],
                         rawtx1["vout"][1]["scriptPubKey"]["address"]: rawtx1["vout"][1]["value"]}
        clone_locktime = rawtx1["locktime"]
        clone_raw = self.nodes[0].createrawtransaction(clone_inputs, clone_outputs, clone_locktime)

        # createrawtransaction randomizes the order of its outputs, so swap them if necessary.
        clone_tx = tx_from_hex(clone_raw)
        if (rawtx1["vout"][0]["value"] == 40 and clone_tx.vout[0].nValue != 40*COIN or rawtx1["vout"][0]["value"] != 40 and clone_tx.vout[0].nValue == 40*COIN):
            (clone_tx.vout[0], clone_tx.vout[1]) = (clone_tx.vout[1], clone_tx.vout[0])

        # Use a different signature hash type to sign.  This creates an equivalent but malleated clone.
        # Don't send the clone anywhere yet
        tx1_clone = self.nodes[0].signrawtransactionwithwallet(clone_tx.serialize().hex(), None, "ALL|ANYONECANPAY")
        assert_equal(tx1_clone["complete"], True)

        # Have node0 mine a block, if requested:
        if (self.options.mine_block):
            self.generate(self.nodes[0], 1, sync_fun=lambda: self.sync_blocks(self.nodes[0:2]))

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

        # Send clone and its parent to miner
        self.nodes[2].sendrawtransaction(node0_tx1["hex"])
        txid1_clone = self.nodes[2].sendrawtransaction(tx1_clone["hex"])
        if self.options.segwit:
            assert_equal(txid1, txid1_clone)
            return

        # ... mine a block...
        self.generate(self.nodes[2], 1, sync_fun=self.no_op)

        # Reconnect the split network, and sync chain:
        self.connect_nodes(1, 2)
        self.nodes[2].sendrawtransaction(node0_tx2["hex"])
        self.nodes[2].sendrawtransaction(tx2["hex"])
        self.generate(self.nodes[2], 1)  # Mine another block to make sure we sync

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

        self.test_malleated_metadata_synced()
        self.test_malleated_rbf_metadata_synced()

    def malleate_tx(self, wallet, txid):
        rawtx = wallet.getrawtransaction(txid)
        tx = tx_from_hex(rawtx)
        for txin in tx.vin:
            txin.scriptSig = b""
        for wit in tx.wit.vtxinwit:
            wit.scriptWitness.stack.clear()
        unsigned_tx = tx.serialize_without_witness().hex()

        # malleate the tx by signing with a different sighash
        malleated_tx = wallet.signrawtransactionwithwallet(hexstring=unsigned_tx, sighashtype="ALL|ANYONECANPAY")["hex"]
        malleated_txid = wallet.decoderawtransaction(malleated_tx)["txid"]
        assert_not_equal(malleated_txid, txid)
        return malleated_tx, malleated_txid


    def test_malleated_metadata_synced(self):
        self.log.info("Test malleated tx has copied user provided metadata")
        self.nodes[0].createwallet("metadata_clone")
        wallet = self.nodes[0].get_wallet_rpc("metadata_clone")
        def_wallet = self.nodes[0].get_wallet_rpc(self.default_wallet_name)

        # Make non-segwit UTXOs that can be malleated
        for _ in range(5):
            def_wallet.sendtoaddress(wallet.getnewaddress(address_type="legacy"), 1)

        self.generate(self.nodes[0], 1)

        # Bumping either should prevent the other from being bumped as well
        for bump_malleated in [False, True]:
            original_txid = wallet.sendtoaddress(def_wallet.getnewaddress(), 0.9, comment="testing", fee_rate=1)
            malleated_tx, malleated_txid = self.malleate_tx(wallet, original_txid)

            blockhash = self.generateblock(self.nodes[0], def_wallet.getnewaddress(), [malleated_tx])["hash"]

            assert_equal(wallet.gettransaction(malleated_txid)["comment"], "testing")

            # Put the malleated back into the mempol by invalidating the block
            self.nodes[0].invalidateblock(blockhash)

            if bump_malleated:
                to_bump = malleated_txid
                other_bump = original_txid
            else:
                to_bump = original_txid
                other_bump = malleated_txid

            bumped = wallet.bumpfee(to_bump, fee_rate=10)

            def check_metadata():
                original_txinfo = wallet.gettransaction(original_txid)
                malleated_txinfo = wallet.gettransaction(malleated_txid)
                assert_equal(original_txinfo["replaced_by_txid"], bumped["txid"])
                assert_equal(malleated_txinfo["replaced_by_txid"], bumped["txid"])

                assert_raises_rpc_error(-4, f"Cannot bump transaction {other_bump} which was already bumped by transaction", wallet.bumpfee, other_bump, fee_rate=20)

            check_metadata()

            # Check persistence
            wallet.unloadwallet()
            self.nodes[0].loadwallet("metadata_clone")

            check_metadata()

            self.nodes[0].reconsiderblock(blockhash)

    def test_malleated_rbf_metadata_synced(self):
        self.log.info("Test malleation of a rbf has copied user provided and replacement metadata")
        self.nodes[0].createwallet("rbf_metadata_clone")
        wallet = self.nodes[0].get_wallet_rpc("rbf_metadata_clone")
        def_wallet = self.nodes[0].get_wallet_rpc(self.default_wallet_name)

        def_wallet.sendtoaddress(wallet.getnewaddress(address_type="legacy"), 1)

        self.generate(self.nodes[0], 1)

        orig_txid = wallet.sendtoaddress(def_wallet.getnewaddress(), 0.9999, comment="testing")
        txid = wallet.bumpfee(orig_txid)["txid"]
        malleated_tx, malleated_txid = self.malleate_tx(wallet, txid)

        self.generateblock(self.nodes[0], def_wallet.getnewaddress(), [malleated_tx])

        txinfo = wallet.gettransaction(malleated_txid)
        assert_equal(txinfo["comment"], "testing")
        assert_equal(txinfo["replaces_txid"], orig_txid)


if __name__ == '__main__':
    TxnMallTest(__file__).main()
