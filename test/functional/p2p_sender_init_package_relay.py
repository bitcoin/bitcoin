#!/usr/bin/env python3
# Copyright (c) 2025 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test sender-initiated package relay"""

from decimal import Decimal
import time

from test_framework.mempool_util import (
    fill_mempool,
)
from test_framework.messages import (
    CInv,
    msg_feefilter,
    msg_getdata,
    msg_inv,
    msg_pkgtxns,
    msg_sendpackages,
    msg_tx,
    msg_wtxidrelay,
    msg_verack,
    MSG_WTX,
    PKG_RELAY_PKGTXNS,
)
from test_framework.p2p import (
    P2PInterface,
    p2p_lock,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.wallet import MiniWallet
from test_framework.util import assert_equal

# 1sat/vB feerate denominated in BTC/KvB
FEERATE_1SAT_VB = Decimal("0.00001000")

def cleanup(func):
    def wrapper(self):
        assert len(self.nodes[0].getpeerinfo()) == 0
        assert len(self.nodes[0].getrawmempool()) == 0
        try:
            func(self)
        finally:
            # Clear mempool
            self.generate(self.nodes[0], 1)
            self.nodes[0].disconnect_p2ps()

            # Reset time
            self.nodes[0].setmocktime(0)
    return wrapper

class PackageRelayer(P2PInterface):
    def __init__(self, send_sendpackages=True, send_wtxidrelay=True):
        super().__init__()
        # List versions of each sendpackages received
        self._sendpackages_received = []
        self._send_sendpackages = send_sendpackages
        self._send_wtxidrelay = send_wtxidrelay
        self._pkgtxns_received = []
        self._tx_received = []

    @property
    def sendpackages_received(self):
        with p2p_lock:
            return self._sendpackages_received

    def on_version(self, message):
        if self._send_wtxidrelay:
            self.send_without_ping(msg_wtxidrelay())
        if self._send_sendpackages:
            sendpackages_message = msg_sendpackages()
            sendpackages_message.versions = PKG_RELAY_PKGTXNS
            self.send_without_ping(sendpackages_message)
        self.send_without_ping(msg_verack())
        self.nServices = message.nServices

    def on_sendpackages(self, message):
        self._sendpackages_received.append(message.versions)

    def on_pkgtxns(self, message):
        for tx in message.txns:
            self._pkgtxns_received.append(tx.wtxid_int)

    def on_tx(self, message):
        self._tx_received.append(message.tx.wtxid_int)

class PackageRelayTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-packagerelay=1", "-maxmempool=5"]]

    def run_test(self):
        self.wallet = MiniWallet(self.nodes[0])
        self.generate(self.wallet, 20)

        self.log.info("Test that a node can receive a `pkgtxns` message")
        self.test_pkg_received()
        self.log.info("Test that a node can send a `pkgtxns` message in the appropriate situation")
        self.test_pkg_sent()
        self.log.info("Test that we can receive a 1p1c package if there are two unconfirmed parents but one of them is known")
        self.test_pkg_received_if_one_parent_known()
        self.log.info("Test that a package is not sent if the parent is known")
        self.test_pkg_not_sent_if_known_parent()
        self.log.info("Test that a package is not sent if it is below the fee filter")
        self.test_pkg_not_sent_if_below_feefilter()
        self.log.info("Test that a package is dropped by the node if it is not 1p1c")
        self.test_pkg_dropped_if_not_1p1c()
        self.log.info("Test that we disconnect the peer if fRelay not sent")
        self.test_disconnect_if_no_tx_relay()

        # We need a different set-up for the following tests, and we don't run @cleanup
        assert_equal(len(self.nodes[0].getrawmempool()), 0)
        self.restart_node(0, extra_args=["-packagerelay=1", "-maxmempool=5"])
        fill_mempool(self, self.nodes[0])

        self.log.info("Test that we send a 1p1c package if there are two unconfirmed parents but one of them is known")
        self.test_pkg_sent_if_one_parent_known()
        self.log.info("Test that a package is not sent if there are multiple unconfirmed ancestors")
        self.test_pkg_not_sent_if_multiple_unknown_ancestors()

    @cleanup
    def test_pkg_received(self):
        package_receiver = self.nodes[0]
        package_sender = self.nodes[0].add_p2p_connection(PackageRelayer())

        low_fee_parent = self.wallet.create_self_transfer(fee_rate=FEERATE_1SAT_VB/2, confirmed_only=True, version=3)
        high_fee_child = self.wallet.create_self_transfer(utxo_to_spend=low_fee_parent["new_utxo"], fee_rate=40*FEERATE_1SAT_VB, version=3)

        # tell package_receiver about the low parent and wait for getdata
        low_parent_wtxid_int = low_fee_parent["tx"].wtxid_int
        package_sender.send_and_ping(msg_inv([CInv(t=MSG_WTX, h=low_parent_wtxid_int)]))
        package_sender.wait_for_getdata([low_parent_wtxid_int])
        package_sender.send_and_ping(msg_tx(low_fee_parent["tx"]))

        # package_receiver does not accept low fee parent
        assert_equal(package_receiver.getrawmempool(), [])

        # send pkgtxns with both txs upon high fee child getdata
        high_child_wtxid_int = high_fee_child["tx"].wtxid_int
        package_sender.send_and_ping(msg_inv([CInv(t=MSG_WTX, h=high_child_wtxid_int)]))
        package_sender.wait_for_getdata([high_child_wtxid_int])
        package_sender.send_and_ping(msg_pkgtxns(txns=[low_fee_parent["tx"], high_fee_child["tx"]]))

        # check that we now accept the package
        node_mempool = package_receiver.getrawmempool()
        assert low_fee_parent["txid"] in node_mempool
        assert high_fee_child["txid"] in node_mempool

    @cleanup
    def test_pkg_sent(self):
        package_sender = self.nodes[0]
        package_receiver = self.nodes[0].add_p2p_connection(PackageRelayer())

        # set a fee filter of 1 sat/byte
        package_receiver.send_and_ping(msg_feefilter(1000))
        self.wait_until(lambda: package_sender.getpeerinfo()[0]["minfeefilter"] == Decimal("0.000010000"))

        low_fee_parent = self.wallet.create_self_transfer(fee_rate=FEERATE_1SAT_VB/2, confirmed_only=True, version=3)
        high_fee_child = self.wallet.create_self_transfer(utxo_to_spend=low_fee_parent["new_utxo"], fee_rate=40*FEERATE_1SAT_VB, version=3)

        high_child_wtxid_int = high_fee_child["tx"].wtxid_int
        low_parent_wtxid_int = low_fee_parent["tx"].wtxid_int

        # submit package to package_sender's mempool
        assert_equal(package_sender.submitpackage([low_fee_parent["hex"], high_fee_child["hex"]])["package_msg"], "success")

        self.wait_until(lambda: "pkgtxns" in package_sender.getpeerinfo()[0]["bytessent_per_msg"])

        assert_equal(package_sender.getpeerinfo()[0]["bytessent_per_msg"]["pkgtxns"], 291)
        assert_equal(len(package_receiver._pkgtxns_received), 2)
        assert low_parent_wtxid_int in package_receiver._pkgtxns_received
        assert high_child_wtxid_int in package_receiver._pkgtxns_received

    @cleanup
    def test_pkg_received_if_one_parent_known(self):
        package_receiver = self.nodes[0]
        package_sender = self.nodes[0].add_p2p_connection(PackageRelayer())

        parent1 = self.wallet.create_self_transfer(fee_rate=20*FEERATE_1SAT_VB, confirmed_only=True)

        package_sender.send_and_ping(msg_tx(parent1["tx"]))
        self.wait_until(lambda: parent1["txid"] in package_receiver.getrawmempool())

        low_fee_parent2 = self.wallet.create_self_transfer(fee_rate=FEERATE_1SAT_VB, confirmed_only=True)
        high_fee_child = self.wallet.create_self_transfer_multi(utxos_to_spend=[parent1["new_utxo"], low_fee_parent2["new_utxo"]], fee_per_output=4000)

        # submit package to package_sender's mempool
        package_sender.send_and_ping(msg_pkgtxns(txns=[low_fee_parent2["tx"], high_fee_child["tx"]]))

        self.wait_until(lambda: (low_fee_parent2["txid"] in package_receiver.getrawmempool()) and (high_fee_child["txid"] in package_receiver.getrawmempool()))

    @cleanup
    def test_pkg_not_sent_if_known_parent(self):
        package_sender = self.nodes[0]
        package_receiver = self.nodes[0].add_p2p_connection(PackageRelayer())

        self.nodes[0].setmocktime(int(time.time()))

        # set a fee filter of 1 sat/byte
        package_receiver.send_and_ping(msg_feefilter(1000))
        self.wait_until(lambda: package_sender.getpeerinfo()[0]["minfeefilter"] == Decimal("0.000010000"))

        low_fee_parent = self.wallet.create_self_transfer(fee_rate=FEERATE_1SAT_VB/2, confirmed_only=True, version=3)
        high_fee_child = self.wallet.create_self_transfer(utxo_to_spend=low_fee_parent["new_utxo"], fee_rate=40*FEERATE_1SAT_VB, version=3)

        high_child_wtxid_int = high_fee_child["tx"].wtxid_int
        low_parent_wtxid_int = low_fee_parent["tx"].wtxid_int

        package_receiver.send_and_ping(msg_inv([CInv(t=MSG_WTX, h=low_parent_wtxid_int)]))

        # submit package to package_sender's mempool
        assert_equal(package_sender.submitpackage([low_fee_parent["hex"], high_fee_child["hex"]])["package_msg"], "success")

        # move time ahead
        self.nodes[0].bumpmocktime(600)

        assert_equal(len(package_receiver._pkgtxns_received), 0)
        assert_equal(len(package_receiver._tx_received), 0)

    @cleanup
    def test_pkg_not_sent_if_below_feefilter(self):
        package_sender = self.nodes[0]
        package_receiver = self.nodes[0].add_p2p_connection(PackageRelayer())

        current_time = int(time.time())
        self.nodes[0].setmocktime(current_time)

        # set a fee filter of 1 sat/byte
        package_receiver.send_and_ping(msg_feefilter(2000))
        self.wait_until(lambda: package_sender.getpeerinfo()[0]["minfeefilter"] == Decimal("0.000020000"))

        low_fee_parent = self.wallet.create_self_transfer(fee_rate=FEERATE_1SAT_VB, confirmed_only=True, version=3)
        high_fee_child = self.wallet.create_self_transfer(utxo_to_spend=low_fee_parent["new_utxo"], fee_rate=FEERATE_1SAT_VB, version=3)

        high_child_wtxid_int = high_fee_child["tx"].wtxid_int
        low_parent_wtxid_int = low_fee_parent["tx"].wtxid_int

        # submit package to package_sender's mempool
        assert_equal(package_sender.submitpackage([low_fee_parent["hex"], high_fee_child["hex"]])["package_msg"], "success")

        # move time ahead
        self.nodes[0].setmocktime(current_time+600)

        assert_equal(len(package_receiver._pkgtxns_received), 0)
        assert_equal(len(package_receiver._tx_received), 0)

    @cleanup
    def test_pkg_dropped_if_not_1p1c(self):
        package_receiver = self.nodes[0]
        package_sender = self.nodes[0].add_p2p_connection(PackageRelayer())

        low_fee_child = self.wallet.create_self_transfer(fee_rate=FEERATE_1SAT_VB, confirmed_only=True)
        high_fee_child = self.wallet.create_self_transfer(fee_rate=FEERATE_1SAT_VB*20, confirmed_only=True)

        # submit package to package_sender's mempool
        package_sender.send_and_ping(msg_pkgtxns(txns=[low_fee_child["tx"], high_fee_child["tx"]]))

        self.wait_until(lambda: "pkgtxns" in package_receiver.getpeerinfo()[0]["bytesrecv_per_msg"])

        assert_equal(len(package_receiver.getrawmempool()), 0)

    @cleanup
    def test_disconnect_if_no_tx_relay(self):
        self.restart_node(0, extra_args=["-blocksonly", "-packagerelay=1"])
        package_receiver = self.nodes[0]
        package_sender = self.nodes[0].add_p2p_connection(PackageRelayer())

        low_fee_parent = self.wallet.create_self_transfer(fee_rate=FEERATE_1SAT_VB/2, confirmed_only=True, version=3)
        high_fee_child = self.wallet.create_self_transfer(utxo_to_spend=low_fee_parent["new_utxo"], fee_rate=40*FEERATE_1SAT_VB, version=3)

        low_parent_wtxid_int = low_fee_parent["tx"].wtxid_int
        high_child_wtxid_int = high_fee_child["tx"].wtxid_int

        package_sender.send_without_ping(msg_pkgtxns(txns=[low_fee_parent["tx"], high_fee_child["tx"]]))

        package_sender.wait_for_disconnect()

    def test_pkg_sent_if_one_parent_known(self):
        package_sender = self.nodes[0]
        package_receiver = self.nodes[0].add_p2p_connection(PackageRelayer())

        # set a fee filter of 2 sat/byte
        package_receiver.send_and_ping(msg_feefilter(2000))
        self.wait_until(lambda: package_sender.getpeerinfo()[0]["minfeefilter"] == Decimal("0.000020000"))

        parent1 = self.wallet.create_self_transfer(fee_rate=20*FEERATE_1SAT_VB, confirmed_only=True)

        package_sender.sendrawtransaction(parent1["hex"])
        self.wait_until(lambda: len(package_receiver._tx_received) == 1)

        low_fee_parent2 = self.wallet.create_self_transfer(fee_rate=FEERATE_1SAT_VB, confirmed_only=True)
        high_fee_child = self.wallet.create_self_transfer_multi(utxos_to_spend=[parent1["new_utxo"], low_fee_parent2["new_utxo"]], fee_per_output=4000)

        # submit package to package_sender's mempool
        assert_equal(package_sender.submitpackage([low_fee_parent2["hex"], high_fee_child["hex"]])["package_msg"], "success")

        self.wait_until(lambda: len(package_receiver._pkgtxns_received) == 2)

        self.generate(self.nodes[0], 1)
        self.nodes[0].disconnect_p2ps()

    def test_pkg_not_sent_if_multiple_unknown_ancestors(self):
        package_sender = self.nodes[0]
        package_receiver = self.nodes[0].add_p2p_connection(PackageRelayer())

        current_time = int(time.time())
        self.nodes[0].setmocktime(current_time)

        # set a fee filter of 2 sat/byte
        package_receiver.send_and_ping(msg_feefilter(2000))
        self.wait_until(lambda: package_sender.getpeerinfo()[0]["minfeefilter"] == Decimal("0.000020000"))

        low_fee_parent1 = self.wallet.create_self_transfer(fee_rate=FEERATE_1SAT_VB, confirmed_only=True)
        low_fee_parent2 = self.wallet.create_self_transfer(fee_rate=FEERATE_1SAT_VB, confirmed_only=True)
        high_fee_child = self.wallet.create_self_transfer_multi(utxos_to_spend=[low_fee_parent1["new_utxo"], low_fee_parent2["new_utxo"]], fee_per_output=4000)

        # submit package to package_sender's mempool
        assert_equal(package_sender.submitpackage([low_fee_parent1["hex"], low_fee_parent2["hex"], high_fee_child["hex"]])["package_msg"], "success")

        self.nodes[0].setmocktime(current_time+600)

        assert_equal(len(package_receiver._pkgtxns_received), 0)
        assert_equal(len(package_receiver._tx_received), 0)

        self.nodes[0].disconnect_p2ps()

if __name__ == '__main__':
    PackageRelayTest(__file__).main()
