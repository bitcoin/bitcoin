#!/usr/bin/env python3
# Copyright (c) 2018-2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test wallet group functionality."""

from test_framework.blocktools import COINBASE_MATURITY
from test_framework.test_framework import BitcoinTestFramework
from test_framework.messages import (
    tx_from_hex,
)
from test_framework.util import (
    assert_approx,
    assert_equal,
)


class WalletGroupTest(BitcoinTestFramework):
    def add_options(self, parser):
        self.add_wallet_options(parser)

    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 5
        # whitelist peers to speed up tx relay / mempool sync
        self.noban_tx_relay = True
        self.extra_args = [
            [],
            [],
            ["-avoidpartialspends"],
            ["-maxapsfee=0.00002719"],
            ["-maxapsfee=0.00002720"],
        ]

        for args in self.extra_args:
            args.append(f"-paytxfee={20 * 1e3 / 1e8}")  # apply feerate of 20 sats/vB across all nodes

        self.rpc_timeout = 480

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def run_test(self):
        self.log.info("Setting up")
        # Mine some coins
        self.generate(self.nodes[0], COINBASE_MATURITY + 1)

        # Get some addresses from the two nodes
        addr1 = [self.nodes[1].getnewaddress() for _ in range(3)]
        addr2 = [self.nodes[2].getnewaddress() for _ in range(3)]
        addrs = addr1 + addr2

        # Send 1 + 0.5 coin to each address
        [self.nodes[0].sendtoaddress(addr, 1.0) for addr in addrs]
        [self.nodes[0].sendtoaddress(addr, 0.5) for addr in addrs]

        self.generate(self.nodes[0], 1)

        # For each node, send 0.2 coins back to 0;
        # - node[1] should pick one 0.5 UTXO and leave the rest
        # - node[2] should pick one (1.0 + 0.5) UTXO group corresponding to a
        #   given address, and leave the rest
        self.log.info("Test sending transactions picks one UTXO group and leaves the rest")
        txid1 = self.nodes[1].sendtoaddress(self.nodes[0].getnewaddress(), 0.2)
        tx1 = self.nodes[1].getrawtransaction(txid1, True)
        # txid1 should have 1 input and 2 outputs
        assert_equal(1, len(tx1["vin"]))
        assert_equal(2, len(tx1["vout"]))
        # one output should be 0.2, the other should be ~0.3
        v = [vout["value"] for vout in tx1["vout"]]
        v.sort()
        assert_approx(v[0], vexp=0.2, vspan=0.0001)
        assert_approx(v[1], vexp=0.3, vspan=0.0001)

        txid2 = self.nodes[2].sendtoaddress(self.nodes[0].getnewaddress(), 0.2)
        tx2 = self.nodes[2].getrawtransaction(txid2, True)
        # txid2 should have 2 inputs and 2 outputs
        assert_equal(2, len(tx2["vin"]))
        assert_equal(2, len(tx2["vout"]))
        # one output should be 0.2, the other should be ~1.3
        v = [vout["value"] for vout in tx2["vout"]]
        v.sort()
        assert_approx(v[0], vexp=0.2, vspan=0.0001)
        assert_approx(v[1], vexp=1.3, vspan=0.0001)

        self.log.info("Test avoiding partial spends if warranted, even if avoidpartialspends is disabled")
        self.sync_all()
        self.generate(self.nodes[0], 1)
        # Nodes 1-2 now have confirmed UTXOs (letters denote destinations):
        # Node #1:      Node #2:
        # - A  1.0      - D0 1.0
        # - B0 1.0      - D1 0.5
        # - B1 0.5      - E0 1.0
        # - C0 1.0      - E1 0.5
        # - C1 0.5      - F  ~1.3
        # - D ~0.3
        assert_approx(self.nodes[1].getbalance(), vexp=4.3, vspan=0.0001)
        assert_approx(self.nodes[2].getbalance(), vexp=4.3, vspan=0.0001)
        # Sending 1.4 btc should pick one 1.0 + one more. For node #1,
        # this could be (A / B0 / C0) + (B1 / C1 / D). We ensure that it is
        # B0 + B1 or C0 + C1, because this avoids partial spends while not being
        # detrimental to transaction cost
        txid3 = self.nodes[1].sendtoaddress(self.nodes[0].getnewaddress(), 1.4)
        tx3 = self.nodes[1].getrawtransaction(txid3, True)
        # tx3 should have 2 inputs and 2 outputs
        assert_equal(2, len(tx3["vin"]))
        assert_equal(2, len(tx3["vout"]))
        # the accumulated value should be 1.5, so the outputs should be
        # ~0.1 and 1.4 and should come from the same destination
        values = [vout["value"] for vout in tx3["vout"]]
        values.sort()
        assert_approx(values[0], vexp=0.1, vspan=0.0001)
        assert_approx(values[1], vexp=1.4, vspan=0.0001)

        input_txids = [vin["txid"] for vin in tx3["vin"]]
        input_addrs = [self.nodes[1].gettransaction(txid)['details'][0]['address'] for txid in input_txids]
        assert_equal(input_addrs[0], input_addrs[1])
        # Node 2 enforces avoidpartialspends so needs no checking here

        tx4_ungrouped_fee = 2820
        tx4_grouped_fee = 4160
        tx5_6_ungrouped_fee = 5520
        tx5_6_grouped_fee = 8240

        self.log.info("Test wallet option maxapsfee")
        addr_aps = self.nodes[3].getnewaddress()
        self.nodes[0].sendtoaddress(addr_aps, 1.0)
        self.nodes[0].sendtoaddress(addr_aps, 1.0)
        self.generate(self.nodes[0], 1)
        with self.nodes[3].assert_debug_log([f'Fee non-grouped = {tx4_ungrouped_fee}, grouped = {tx4_grouped_fee}, using grouped']):
            txid4 = self.nodes[3].sendtoaddress(self.nodes[0].getnewaddress(), 0.1)
        tx4 = self.nodes[3].getrawtransaction(txid4, True)
        # tx4 should have 2 inputs and 2 outputs although one output would
        # have been enough and the transaction caused higher fees
        assert_equal(2, len(tx4["vin"]))
        assert_equal(2, len(tx4["vout"]))

        addr_aps2 = self.nodes[3].getnewaddress()
        [self.nodes[0].sendtoaddress(addr_aps2, 1.0) for _ in range(5)]
        self.generate(self.nodes[0], 1)
        with self.nodes[3].assert_debug_log([f'Fee non-grouped = {tx5_6_ungrouped_fee}, grouped = {tx5_6_grouped_fee}, using non-grouped']):
            txid5 = self.nodes[3].sendtoaddress(self.nodes[0].getnewaddress(), 2.95)
        tx5 = self.nodes[3].getrawtransaction(txid5, True)
        # tx5 should have 3 inputs (1.0, 1.0, 1.0) and 2 outputs
        assert_equal(3, len(tx5["vin"]))
        assert_equal(2, len(tx5["vout"]))

        # Test wallet option maxapsfee with node 4, which sets maxapsfee
        # 1 sat higher, crossing the threshold from non-grouped to grouped.
        self.log.info("Test wallet option maxapsfee threshold from non-grouped to grouped")
        addr_aps3 = self.nodes[4].getnewaddress()
        [self.nodes[0].sendtoaddress(addr_aps3, 1.0) for _ in range(5)]
        self.generate(self.nodes[0], 1)
        with self.nodes[4].assert_debug_log([f'Fee non-grouped = {tx5_6_ungrouped_fee}, grouped = {tx5_6_grouped_fee}, using grouped']):
            txid6 = self.nodes[4].sendtoaddress(self.nodes[0].getnewaddress(), 2.95)
        tx6 = self.nodes[4].getrawtransaction(txid6, True)
        # tx6 should have 5 inputs and 2 outputs
        assert_equal(5, len(tx6["vin"]))
        assert_equal(2, len(tx6["vout"]))

        # Empty out node2's wallet
        self.nodes[2].sendall(recipients=[self.nodes[0].getnewaddress()])
        self.sync_all()
        self.generate(self.nodes[0], 1)

        self.log.info("Fill a wallet with 10,000 outputs corresponding to the same scriptPubKey")
        for _ in range(5):
            raw_tx = self.nodes[0].createrawtransaction([{"txid":"0"*64, "vout":0}], [{addr2[0]: 0.05}])
            tx = tx_from_hex(raw_tx)
            tx.vin = []
            tx.vout = [tx.vout[0]] * 2000
            funded_tx = self.nodes[0].fundrawtransaction(tx.serialize().hex())
            signed_tx = self.nodes[0].signrawtransactionwithwallet(funded_tx['hex'])
            self.nodes[0].sendrawtransaction(signed_tx['hex'])
            self.generate(self.nodes[0], 1)

        # Check that we can create a transaction that only requires ~100 of our
        # utxos, without pulling in all outputs and creating a transaction that
        # is way too big.
        self.log.info("Test creating txn that only requires ~100 of our UTXOs without pulling in all outputs")
        assert self.nodes[2].sendtoaddress(address=addr2[0], amount=5)


if __name__ == '__main__':
    WalletGroupTest(__file__).main()
