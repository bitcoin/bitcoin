#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test wallet transaction broadcast when -privatebroadcast is enabled.

When -privatebroadcast is enabled, wallet transactions (sendtoaddress, sendmany,
send, etc.) should be broadcast privately through Tor/I2P networks instead of
being announced to all connected peers.

Test coverage:
- test_sendtoaddress_private_broadcast: New tx broadcasts via SOCKS5 proxy
- test_private_flag_prevents_public_rebroadcast: Flag persists and prevents leaks
- test_public_tx_rebroadcast_privately: Public txs rebroadcast privately (plausible deniability)
- test_error_when_tor_i2p_unavailable: Proper error when no anonymous network

Note: P2P-level private broadcast behavior (transaction returning from network,
rebroadcast timing, etc.) is tested in p2p_private_broadcast.py.
"""

import time

from test_framework.blocktools import create_block, create_coinbase
from test_framework.p2p import P2PInterface
from test_framework.socks5 import (
    FAKE_ONION_ADDRESSES,
    Socks5ProxyHelper,
    add_addresses_to_addrman,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_raises_rpc_error,
    tor_port,
)


class WalletPrivateBroadcastTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.disable_autoconnect = False
        self.extra_args = [[], []]

    def skip_test_if_missing_module(self):
        self.skip_if_no_wallet()

    def setup_nodes(self):
        """Set up SOCKS5 proxy and configure nodes for private broadcast testing."""
        self.proxy = Socks5ProxyHelper(self.log)
        self.proxy.start_simple_redirect("127.0.0.1", tor_port(1))

        self.extra_args = [
            [
                "-privatebroadcast=1",
                f"-proxy=127.0.0.1:{self.proxy.port}",
                "-cjdnsreachable",
                "-v2transport=0",
                "-test=addrman",
                "-debug=privatebroadcast",
                "-debug=mempool",
            ],
            [
                "-connect=0",
                f"-bind=127.0.0.1:{tor_port(1)}=onion",
                "-v2transport=0",
                "-debug=mempool",
            ],
        ]
        super().setup_nodes()

    def setup_network(self):
        self.setup_nodes()
        # Don't connect nodes - private broadcast goes through SOCKS5

    # =========================================================================
    # Helper methods
    # =========================================================================

    def create_and_fund_wallets(self):
        """Create wallets on both nodes and fund the sender."""
        self.log.info("Creating and funding wallets")

        self.nodes[0].createwallet(wallet_name="sender_wallet")
        self.sender_wallet = self.nodes[0].get_wallet_rpc("sender_wallet")

        self.nodes[1].createwallet(wallet_name="receiver_wallet")
        self.receiver_wallet = self.nodes[1].get_wallet_rpc("receiver_wallet")

        address = self.sender_wallet.getnewaddress()
        self.generatetoaddress(self.nodes[0], 101, address, sync_fun=self.no_op)

        balance = self.sender_wallet.getbalance()
        assert balance > 0, f"Wallet should have balance, got {balance}"
        self.log.info(f"Sender wallet funded with {balance} BTC")

    def reload_sender_wallet(self):
        """Load the sender wallet and return its RPC interface."""
        self.nodes[0].loadwallet("sender_wallet")
        self.sender_wallet = self.nodes[0].get_wallet_rpc("sender_wallet")

    def get_private_broadcast_args(self):
        """Return node args for private broadcast mode with current proxy."""
        return [
            "-privatebroadcast=1",
            f"-proxy=127.0.0.1:{self.proxy.port}",
            "-cjdnsreachable",
            "-v2transport=0",
            "-test=addrman",
            "-debug=privatebroadcast",
            "-debug=mempool",
        ]

    def trigger_wallet_resubmit(self, node):
        """Trigger MaybeResendWalletTxs by advancing time and mining a block.

        Wallet resubmission requires:
        1. A block at least 5 minutes newer than the transaction
        2. Sufficient time passed (12-36 hours) for the resubmit timer
        """
        block_time = int(time.time()) + 6 * 60  # 6 minutes in the future
        node.setmocktime(block_time)

        best_block_hash = int(node.getbestblockhash(), 16)
        block = create_block(best_block_hash, create_coinbase(node.getblockcount() + 1), block_time)
        block.solve()
        node.submitblock(block.serialize().hex())
        node.syncwithvalidationinterfacequeue()

        # Advance 36 hours to exceed maximum resubmit interval
        node.setmocktime(block_time + 36 * 60 * 60)
        node.mockscheduler(60)

    def assert_tx_unconfirmed(self, txid, context=""):
        """Assert that a transaction has 0 confirmations."""
        tx_info = self.sender_wallet.gettransaction(txid)
        context_str = f" for {context}" if context else ""
        assert tx_info["confirmations"] == 0, \
            f"tx {txid} should be unconfirmed{context_str}, got {tx_info['confirmations']} confirmations"

    def get_log_pos(self, node):
        """Get current position in node's debug.log for tracking new messages."""
        try:
            with open(node.debug_log_path, 'r', encoding='utf-8') as f:
                f.seek(0, 2)
                return f.tell()
        except FileNotFoundError:
            return 0

    def wait_for_log_message(self, node, message, start_pos=0, timeout=30):
        """Wait for a message to appear in the node's debug.log."""
        def check():
            try:
                with open(node.debug_log_path, 'r', encoding='utf-8') as f:
                    if start_pos > 0:
                        f.seek(start_pos)
                    return message in f.read()
            except FileNotFoundError:
                return False
        self.wait_until(check, timeout=timeout)

    def saturate_reachable_networks(self, node):
        """Establish connections to all reachable networks to prevent race conditions.

        Bitcoin Core's network-specific connection logic tries to maintain at least
        one connection to each reachable network. This can race with private broadcast
        connections through the SOCKS5 proxy. By pre-establishing connections to all
        networks, we prevent unpredictable network-specific connections.
        See https://github.com/bitcoin/bitcoin/issues/34387
        """
        node.addnode(node="20.0.0.1:8333", command="onetry")  # IPv4
        node.addnode(node="[20::1]:8333", command="onetry")  # IPv6
        node.addnode(node="testonlyp7jkg2rwd3gfdo7ptme3puk2xfvd3hka6qdnypf4gqy3jahid.onion:8333", command="onetry")  # Tor
        node.addnode(node="[fc00::1]:8333", command="onetry")  # CJDNS

    # =========================================================================
    # Test methods
    # =========================================================================

    def test_sendtoaddress_private_broadcast(self):
        """Test that sendtoaddress broadcasts via SOCKS5 proxy when -privatebroadcast enabled."""
        self.log.info("Testing sendtoaddress with private broadcast...")

        sender = self.nodes[0]
        receiver = self.nodes[1]
        dest = self.receiver_wallet.getnewaddress()
        initial_destinations = len(self.proxy.destinations)

        # Send transaction
        self.txid_private = self.sender_wallet.sendtoaddress(dest, 1.0)
        self.log.info(f"Sent transaction: {self.txid_private}")

        # Verify NOT in sender's mempool (private broadcast skips local mempool)
        assert_equal(len(sender.getrawmempool()), 0)
        self.log.info("Transaction not in sender's mempool (expected)")

        # Verify SOCKS5 connection established
        self.wait_until(lambda: len(self.proxy.destinations) > initial_destinations)
        self.log.info("SOCKS5 connection established")

        # Verify private broadcast: INV sent via proxy (this is THE key verification)
        self.wait_for_log_message(sender, f"P2P handshake completed, sending INV for txid={self.txid_private}")
        self.log.info("Private broadcast INV sent via SOCKS5")

        # Verify PONG received (confirms peer received the tx and it will propagate)
        self.wait_for_log_message(sender, "Got a PONG (the transaction will probably reach the network)")
        self.log.info("PONG received - delivery confirmed")

        # Verify transaction reached receiver and was accepted to mempool
        self.wait_until(lambda: self.txid_private in receiver.getrawmempool(), timeout=30)
        self.wait_for_log_message(receiver, f"accepted {self.txid_private}")
        self.log.info("Transaction accepted by receiver node")

        # Verify observer sees the relayed transaction
        self.observer.wait_for_tx(self.txid_private)
        self.log.info("Transaction relayed to observer")

        # Verify transaction received back from network (round-trip complete)
        self.wait_for_log_message(sender, f"Received our privately broadcast transaction (txid={self.txid_private})")
        self.log.info("Transaction received back from network")

    def test_private_flag_prevents_public_rebroadcast(self):
        """Test that private_broadcast flag prevents rebroadcast when -privatebroadcast disabled."""
        self.log.info("Testing that private_broadcast flag prevents public rebroadcast...")

        sender = self.nodes[0]

        # Stop proxy and restart without -privatebroadcast
        self.proxy.stop()
        self.restart_node(0, extra_args=["-v2transport=0"])
        self.reload_sender_wallet()

        # Verify transaction still unconfirmed (precondition)
        self.assert_tx_unconfirmed(self.txid_private, "rebroadcast test")

        # Trigger resubmission and verify tx is skipped
        skip_msg = f"skipping tx {self.txid_private} originally sent via private broadcast"
        with sender.assert_debug_log(expected_msgs=[skip_msg]):
            self.trigger_wallet_resubmit(sender)

        self.log.info("Transaction with private_broadcast flag was skipped")

    def test_public_tx_rebroadcast_privately(self):
        """Test that publicly-sent txs can be rebroadcast privately (adds plausible deniability)."""
        self.log.info("Testing that public txs are rebroadcast privately...")

        sender = self.nodes[0]

        # Reset mocktime and create a public transaction (node currently has no -privatebroadcast)
        sender.setmocktime(0)
        dest = self.receiver_wallet.getnewaddress()
        self.txid_public = self.sender_wallet.sendtoaddress(dest, 0.5)
        self.log.info(f"Created public transaction: {self.txid_public}")

        # Verify public tx is in mempool (normal behavior)
        assert self.txid_public in sender.getrawmempool()

        # Start new proxy and restart with -privatebroadcast
        self.proxy = Socks5ProxyHelper(self.log)
        self.proxy.start_simple_redirect("127.0.0.1", tor_port(1))

        self.restart_node(0, extra_args=self.get_private_broadcast_args())
        self.reload_sender_wallet()

        # Verify both transactions still unconfirmed
        self.assert_tx_unconfirmed(self.txid_private, "Test 3")
        self.assert_tx_unconfirmed(self.txid_public, "Test 3")

        # Saturate network-specific connection logic before populating addrman
        self.saturate_reachable_networks(self.nodes[0])
        # Re-populate addrman for private broadcast
        add_addresses_to_addrman(self.nodes[0], FAKE_ONION_ADDRESSES[:5], self.log)

        # Track log position to verify NEW messages (not from earlier tests)
        log_pos = self.get_log_pos(sender)

        # Trigger resubmission and verify:
        # - txid_private (has flag): resubmitted via private broadcast
        # - txid_public (no flag): also resubmitted privately (adds plausible deniability)
        skip_private_msg = f"skipping tx {self.txid_private} originally sent via private broadcast"

        with sender.assert_debug_log(expected_msgs=[], unexpected_msgs=[skip_private_msg]):
            self.trigger_wallet_resubmit(sender)

        # Verify both transactions triggered private broadcast
        self.wait_for_log_message(sender, f"Requesting 3 new connections due to txid={self.txid_private}", log_pos)
        self.wait_for_log_message(sender, f"Requesting 3 new connections due to txid={self.txid_public}", log_pos)
        self.wait_until(lambda: len(self.proxy.destinations) >= 1)
        self.wait_for_log_message(sender, f"P2P handshake completed, sending INV for txid={self.txid_private}", log_pos)
        self.wait_for_log_message(sender, f"P2P handshake completed, sending INV for txid={self.txid_public}", log_pos)

        self.log.info("Both transactions rebroadcast via private broadcast")

        # Cleanup
        self.proxy.stop()

    def test_error_when_tor_i2p_unavailable(self):
        """Test that wallet RPCs fail gracefully when Tor/I2P is not reachable."""
        self.log.info("Testing error when Tor/I2P unavailable...")

        # Restart with -privatebroadcast but broken Tor setup:
        # - -listenonion allows startup (expects to get proxy from Tor later)
        # - -torcontrol=127.0.0.1:1 fails (port 1 is privileged, nothing listens)
        # - Result: No working Tor proxy, no I2P, private broadcast unavailable
        self.restart_node(0, extra_args=[
            "-privatebroadcast=1",
            "-v2transport=0",
            "-listenonion",
            "-torcontrol=127.0.0.1:1",
        ])
        self.reload_sender_wallet()

        dest = self.receiver_wallet.getnewaddress()

        assert_raises_rpc_error(
            -1,
            "none of the Tor or I2P networks is reachable",
            self.sender_wallet.sendtoaddress, dest, 0.1
        )
        self.log.info("sendtoaddress correctly rejected")

        # Also test that rebroadcast logs the error (not silently fails)
        # txid_private still has private_broadcast=1 flag and should attempt rebroadcast
        self.assert_tx_unconfirmed(self.txid_private, "rebroadcast error test")

        error_msg = f"failed to resubmit {self.txid_private}"
        with self.nodes[0].assert_debug_log(expected_msgs=[error_msg, "Tor or I2P networks is reachable"]):
            self.trigger_wallet_resubmit(self.nodes[0])

        self.log.info("Rebroadcast failure correctly logged")

    # =========================================================================
    # Main test execution
    # =========================================================================

    def run_test(self):
        # Setup: Add observer and populate addrman
        self.observer = self.nodes[1].add_p2p_connection(P2PInterface())
        self.observer.wait_for_verack()

        # Saturate network-specific connection logic before populating addrman
        self.saturate_reachable_networks(self.nodes[0])
        add_addresses_to_addrman(self.nodes[0], FAKE_ONION_ADDRESSES[:5], self.log)
        self.create_and_fund_wallets()

        # Run tests in sequence (they share state)
        self.test_sendtoaddress_private_broadcast()
        self.test_private_flag_prevents_public_rebroadcast()
        self.test_public_tx_rebroadcast_privately()
        self.test_error_when_tor_i2p_unavailable()

        self.log.info("All wallet private broadcast tests passed!")


if __name__ == "__main__":
    WalletPrivateBroadcastTest(__file__).main()
