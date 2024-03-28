#!/usr/bin/env python3
# Copyright (c) 2024-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test opportunistic 1p1c package submission logic.
"""

from decimal import Decimal
from test_framework.messages import (
    CInv,
    MSG_WTX,
    msg_inv,
    msg_tx,
)
from test_framework.p2p import (
    P2PInterface,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
    fill_mempool,
)
from test_framework.wallet import (
    MiniWallet,
    MiniWalletMode,
)

# 1sat/vB feerate denominated in BTC/KvB
FEERATE_1SAT_VB = Decimal("0.00001000")

class PackageRelayTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1
        self.noban_tx_relay = True
        self.extra_args = [[
            "-datacarriersize=100000",
            "-maxmempool=5",
        ]]
        self.supports_cli = False

    def raise_network_minfee(self):

        self.log.debug("Wait for the network to sync mempools")
        self.sync_mempools()

        self.log.debug("Check that all nodes' mempool minimum feerates are above min relay feerate")
        for node in self.nodes:
            assert_equal(node.getmempoolinfo()['minrelaytxfee'], FEERATE_1SAT_VB)
            assert_greater_than(node.getmempoolinfo()['mempoolminfee'], FEERATE_1SAT_VB)

    def test_basic_child_then_parent(self):
        node = self.nodes[0]
        self.log.info("Check that opportunistic 1p1c logic works when child is received before parent")

        low_fee_parent = self.wallet.create_self_transfer(fee_rate=FEERATE_1SAT_VB, confirmed_only=True)
        high_fee_child = self.wallet.create_self_transfer(utxo_to_spend=low_fee_parent["new_utxo"], fee_rate=20*FEERATE_1SAT_VB)

        peer_sender = node.add_outbound_p2p_connection(P2PInterface(), p2p_idx=1, connection_type="outbound-full-relay")

        # 1. Child is received first (perhaps the low feerate parent didn't meet feefilter or the requests were sent to different nodes). It is missing an input.
        high_child_wtxid_int = int(high_fee_child["tx"].getwtxid(), 16)
        peer_sender.send_and_ping(msg_inv([CInv(t=MSG_WTX, h=high_child_wtxid_int)]))
        peer_sender.wait_for_getdata([high_child_wtxid_int])
        peer_sender.send_and_ping(msg_tx(high_fee_child["tx"]))

        # 2. Node requests the missing parent by txid.
        parent_txid_int = int(low_fee_parent["txid"], 16)
        peer_sender.wait_for_getdata([parent_txid_int])

        # 3. Sender relays the parent. Parent+Child are evaluated as a package and accepted.
        peer_sender.send_and_ping(msg_tx(low_fee_parent["tx"]))

        # 4. Both transactions should now be in mempool.
        node_mempool = node.getrawmempool()
        assert low_fee_parent["txid"] in node_mempool
        assert high_fee_child["txid"] in node_mempool

        node.disconnect_p2ps()

    def test_basic_parent_then_child(self, wallet):
        node = self.nodes[0]
        low_fee_parent = wallet.create_self_transfer(fee_rate=FEERATE_1SAT_VB, confirmed_only=True)
        high_fee_child = wallet.create_self_transfer(utxo_to_spend=low_fee_parent["new_utxo"], fee_rate=20*FEERATE_1SAT_VB)

        peer_sender = node.add_outbound_p2p_connection(P2PInterface(), p2p_idx=1, connection_type="outbound-full-relay")
        peer_ignored = node.add_outbound_p2p_connection(P2PInterface(), p2p_idx=2, connection_type="outbound-full-relay")

        # 1. Parent is relayed first. It is too low feerate.
        parent_wtxid_int = int(low_fee_parent["tx"].getwtxid(), 16)
        peer_sender.send_and_ping(msg_inv([CInv(t=MSG_WTX, h=parent_wtxid_int)]))
        peer_sender.wait_for_getdata([parent_wtxid_int])
        peer_sender.send_and_ping(msg_tx(low_fee_parent["tx"]))
        assert low_fee_parent["txid"] not in node.getrawmempool()

        # Send again from peer_ignored, check that it is ignored
        peer_ignored.send_and_ping(msg_inv([CInv(t=MSG_WTX, h=parent_wtxid_int)]))
        assert "getdata" not in peer_ignored.last_message

        # 2. Child is relayed next. It is missing an input.
        high_child_wtxid_int = int(high_fee_child["tx"].getwtxid(), 16)
        peer_sender.send_and_ping(msg_inv([CInv(t=MSG_WTX, h=high_child_wtxid_int)]))
        peer_sender.wait_for_getdata([high_child_wtxid_int])
        peer_sender.send_and_ping(msg_tx(high_fee_child["tx"]))

        # 3. Node requests the missing parent by txid.
        # It should do so even if it has previously rejected that parent for being too low feerate.
        parent_txid_int = int(low_fee_parent["txid"], 16)
        peer_sender.wait_for_getdata([parent_txid_int])

        # 4. Sender re-relays the parent. Parent+Child are evaluated as a package and accepted.
        peer_sender.send_and_ping(msg_tx(low_fee_parent["tx"]))

        # 5. Both transactions should now be in mempool.
        node_mempool = node.getrawmempool()
        assert low_fee_parent["txid"] in node_mempool
        assert high_fee_child["txid"] in node_mempool

        node.disconnect_p2ps()

    def test_low_and_high_child(self, wallet):
        node = self.nodes[0]
        low_fee_parent = wallet.create_self_transfer(fee_rate=FEERATE_1SAT_VB, confirmed_only=True)
        low_fee_child = wallet.create_self_transfer(utxo_to_spend=low_fee_parent["new_utxo"], fee_rate=2*FEERATE_1SAT_VB)
        high_fee_child = wallet.create_self_transfer(utxo_to_spend=low_fee_parent["new_utxo"], fee_rate=999*FEERATE_1SAT_VB)

        peer_sender = node.add_outbound_p2p_connection(P2PInterface(), p2p_idx=3, connection_type="outbound-full-relay")
        peer_ignored = node.add_outbound_p2p_connection(P2PInterface(), p2p_idx=4, connection_type="outbound-full-relay")

        self.log.info("Check that tx caches low fee parent + low fee child package rejections")

        # 1. Send parent, rejected for being low feerate.
        parent_wtxid_int = int(low_fee_parent["tx"].getwtxid(), 16)
        peer_sender.send_and_ping(msg_inv([CInv(t=MSG_WTX, h=parent_wtxid_int)]))
        peer_sender.wait_for_getdata([parent_wtxid_int])
        peer_sender.send_and_ping(msg_tx(low_fee_parent["tx"]))
        assert low_fee_parent["txid"] not in node.getrawmempool()

        # Send again from peer_ignored, check that it is ignored
        peer_ignored.send_and_ping(msg_inv([CInv(t=MSG_WTX, h=parent_wtxid_int)]))
        assert "getdata" not in peer_ignored.last_message

        # 2. Send an (orphan) child that has a higher feerate, but not enough to bump the parent.
        low_child_wtxid_int = int(low_fee_child["tx"].getwtxid(), 16)
        peer_sender.send_and_ping(msg_inv([CInv(t=MSG_WTX, h=low_child_wtxid_int)]))
        peer_sender.wait_for_getdata([low_child_wtxid_int])
        peer_sender.send_and_ping(msg_tx(low_fee_child["tx"]))

        # 3. Node requests the orphan's missing parent.
        parent_txid_int = int(low_fee_parent["txid"], 16)
        peer_sender.wait_for_getdata([parent_txid_int])

        # 4. The low parent + low child are submitted as a package. They are not accepted due to low package feerate.
        peer_sender.send_and_ping(msg_tx(low_fee_parent["tx"]))

        assert low_fee_parent["txid"] not in node.getrawmempool()
        assert low_fee_child["txid"] not in node.getrawmempool()

        # If peer_ignored announces the low feerate child, it should be ignored
        peer_ignored.send_and_ping(msg_inv([CInv(t=MSG_WTX, h=low_child_wtxid_int)]))
        assert "getdata" not in peer_ignored.last_message
        # If either peer sends the parent again, package evaluation should not be attempted
        peer_sender.send_and_ping(msg_tx(low_fee_parent["tx"]))
        peer_ignored.send_and_ping(msg_tx(low_fee_parent["tx"]))

        # 5. Send the high feerate (orphan) child
        high_child_wtxid_int = int(high_fee_child["tx"].getwtxid(), 16)
        peer_sender.send_and_ping(msg_inv([CInv(t=MSG_WTX, h=high_child_wtxid_int)]))
        peer_sender.wait_for_getdata([high_child_wtxid_int])
        peer_sender.send_and_ping(msg_tx(high_fee_child["tx"]))

        # 6. Node requests the orphan's parent, even though it has already been rejected, both by
        # itself and with a child. This is necessary, otherwise high_fee_child can be censored.
        parent_txid_int = int(low_fee_parent["txid"], 16)
        peer_sender.wait_for_getdata([parent_txid_int])

        # 7. The low feerate parent + high feerate child are submitted as a package.
        peer_sender.send_and_ping(msg_tx(low_fee_parent["tx"]))

        # 8. Both transactions should now be in mempool
        node_mempool = node.getrawmempool()
        assert low_fee_parent["txid"] in node_mempool
        assert high_fee_child["txid"] in node_mempool

        node.disconnect_p2ps()

    def run_test(self):
        node = self.nodes[0]
        self.wallet = MiniWallet(node)
        self.wallet_nonsegwit = MiniWallet(node, mode=MiniWalletMode.RAW_P2PK)
        self.generate(self.wallet_nonsegwit, 10)
        self.generate(self.wallet, 20)

        fill_mempool(self, node, self.wallet)

        self.log.info("Check opportunistic 1p1c logic when parent (txid != wtxid) is received before child")
        self.test_basic_parent_then_child(self.wallet)

        self.log.info("Check opportunistic 1p1c logic when parent (txid == wtxid) is received before child")
        self.test_basic_parent_then_child(self.wallet_nonsegwit)

        self.log.info("Check opportunistic 1p1c logic when child is received before parent")
        self.test_basic_child_then_parent()

        self.log.info("Check opportunistic 1p1c logic when 2 candidate children exist (parent txid != wtxid)")
        self.test_low_and_high_child(self.wallet)

        self.log.info("Check opportunistic 1p1c logic when 2 candidate children exist (parent txid == wtxid)")
        self.test_low_and_high_child(self.wallet_nonsegwit)


if __name__ == '__main__':
    PackageRelayTest().main()
