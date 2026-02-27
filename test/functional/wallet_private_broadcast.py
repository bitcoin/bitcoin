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
- test_error_when_tor_i2p_unavailable: Proper error when no anonymous network
- test_rebroadcast_with_private_broadcast: Rebroadcast uses private method
- test_rebroadcast_without_private_broadcast: Rebroadcast uses normal method

Note: P2P-level private broadcast behavior (transaction returning from network,
rebroadcast timing, etc.) is tested in p2p_private_broadcast.py.
"""

import time

from test_framework.blocktools import create_block, create_coinbase
from test_framework.socks5 import (
    FAKE_ONION_ADDRESSES,
    Socks5ProxyHelper,
    add_addresses_to_addrman,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    assert_greater_than,
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
        assert_greater_than(balance, 0)
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

        # Verify wallet logged private broadcast submission
        self.wait_for_log_message(sender, f"Submitting wtx {self.txid_private} for private broadcast")
        self.log.info("Wallet logged private broadcast submission")

        # Verify SOCKS5 connection established
        self.wait_until(lambda: len(self.proxy.destinations) > initial_destinations)
        self.log.info("SOCKS5 connection established")

        # Verify private broadcast: INV sent via proxy
        self.wait_for_log_message(sender, f"P2P handshake completed, sending INV for txid={self.txid_private}")
        self.log.info("Private broadcast INV sent via SOCKS5")

        # Verify transaction reached receiver and was accepted to mempool
        self.wait_until(lambda: self.txid_private in receiver.getrawmempool(), timeout=30)
        self.wait_for_log_message(receiver, f"accepted {self.txid_private}")
        self.log.info("Transaction accepted by receiver node")

    def test_error_when_tor_i2p_unavailable(self):
        """Test that wallet RPCs fail gracefully when Tor/I2P is not reachable."""
        self.log.info("Testing error when Tor/I2P unavailable...")

        # Stop proxy and restart with -privatebroadcast but broken Tor setup:
        # - -listenonion allows startup (expects to get proxy from Tor later)
        # - -torcontrol=127.0.0.1:1 fails (port 1 is privileged, nothing listens)
        # - Result: No working Tor proxy, no I2P, private broadcast unavailable
        self.proxy.stop()
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

    def test_rebroadcast_with_private_broadcast(self):
        """Test that rebroadcast uses private broadcast method when -privatebroadcast enabled."""
        self.log.info("Testing rebroadcast with -privatebroadcast...")

        sender = self.nodes[0]

        # Start new proxy and restart with -privatebroadcast
        self.proxy = Socks5ProxyHelper(self.log)
        self.proxy.start_simple_redirect("127.0.0.1", tor_port(1))

        self.restart_node(0, extra_args=self.get_private_broadcast_args())
        self.reload_sender_wallet()

        # Verify the tx is still unconfirmed
        tx_info = self.sender_wallet.gettransaction(self.txid_private)
        assert_equal(tx_info["confirmations"], 0)

        # Saturate network-specific connection logic before populating addrman
        self.saturate_reachable_networks(sender)
        add_addresses_to_addrman(sender, FAKE_ONION_ADDRESSES[:5], self.log)

        # Track log to verify new private broadcast messages
        log_pos = self.get_log_pos(sender)

        # Trigger resubmission
        self.trigger_wallet_resubmit(sender)

        # Verify private broadcast was triggered for resubmission
        self.wait_for_log_message(sender, f"Requesting 3 new connections due to txid={self.txid_private}", log_pos)
        self.wait_until(lambda: len(self.proxy.destinations) >= 1)
        self.log.info("Rebroadcast used private broadcast method")

        # Cleanup
        self.proxy.stop()

    def test_rebroadcast_without_private_broadcast(self):
        """Test that rebroadcast uses normal method when -privatebroadcast is off."""
        self.log.info("Testing rebroadcast without -privatebroadcast...")

        sender = self.nodes[0]

        # Restart without -privatebroadcast
        self.restart_node(0, extra_args=["-v2transport=0"])
        self.reload_sender_wallet()
        sender.setmocktime(0)

        # Verify the tx is still unconfirmed
        tx_info = self.sender_wallet.gettransaction(self.txid_private)
        assert_equal(tx_info["confirmations"], 0)

        # Track log to verify rebroadcast uses normal method
        log_pos = self.get_log_pos(sender)

        # Trigger resubmission - should use MEMPOOL_AND_BROADCAST_TO_ALL (normal path)
        self.trigger_wallet_resubmit(sender)

        # Verify normal submission was used (not private broadcast)
        self.wait_for_log_message(sender, f"Submitting wtx {self.txid_private} to mempool and for broadcast to peers", log_pos)
        self.log.info("Rebroadcast used normal broadcast method")

    # =========================================================================
    # Log helpers
    # =========================================================================

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

    # =========================================================================
    # Main test execution
    # =========================================================================

    def run_test(self):
        # Saturate network-specific connection logic before populating addrman
        self.saturate_reachable_networks(self.nodes[0])
        add_addresses_to_addrman(self.nodes[0], FAKE_ONION_ADDRESSES[:5], self.log)
        self.create_and_fund_wallets()

        # Run tests in sequence (they share state via self.txid_private)
        self.test_sendtoaddress_private_broadcast()
        self.test_error_when_tor_i2p_unavailable()
        self.test_rebroadcast_with_private_broadcast()
        self.test_rebroadcast_without_private_broadcast()

        self.log.info("All wallet private broadcast tests passed!")


if __name__ == "__main__":
    WalletPrivateBroadcastTest(__file__).main()
