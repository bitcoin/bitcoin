#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test sending a transaction via private broadcast and then, from the same running
node, sending the same transaction again non-privately.

With -privatebroadcast, sendrawtransaction does not add the transaction to the
mempool and only sends it via short-lived private broadcast connections. However,
other code paths add a transaction to the mempool and announce it to all peers
regardless of -privatebroadcast (net_processing's InitiateTxBroadcastToAll()),
for example the submitpackage RPC or the wallet (on commit or on periodic
rebroadcast, via MEMPOOL_AND_BROADCAST_TO_ALL). Here submitpackage and (if
compiled) the periodic rebroadcast of the wallet are used.

Topology:
* nodes[0]: the transaction originator, runs with -privatebroadcast. All its
  outbound connections go through a SOCKS5 proxy controlled by this test.
* nodes[1]: receives the first private broadcast connection of each transaction
  from nodes[0] (redirected to it by the proxy). In the wallet case it also gets a
  temporary normal (manual) connection from nodes[0], to send the transaction back.
* nodes[2]: has a normal (manual) connection from nodes[0]. Not connected to
  anybody else.
"""

import threading
import time

from test_framework.blocktools import create_block
from test_framework.messages import CAddress
from test_framework.p2p import (
    P2PInterface,
    P2P_SERVICES,
    start_p2p_listener,
)
from test_framework.socks5 import start_socks5_server
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    tor_port,
)
from test_framework.wallet import MiniWallet

NUM_PRIVATE_BROADCAST_PER_TX = 3
# Upper limit of the wallet resend timer, see CWallet::GetDefaultNextResend()
WALLET_RESEND_TIMER_LIMIT = 36 * 60 * 60


class P2PPrivateBroadcastThenPublic(BitcoinTestFramework):
    def set_test_params(self):
        self.disable_autoconnect = False
        self.num_nodes = 3
        self.uses_wallet = None

    def setup_nodes(self):
        self.private_broadcast_to_node1_done = False
        self.lock = threading.Lock()

        def destinations_factory(requested_to_addr, requested_to_port, proxy_client):
            """
            Instruct the SOCKS5 proxy to redirect connections:
            * The first private broadcast connection -> nodes[1]
            * Manual connections -> unchanged (the requested destination)
            * Anything else -> a new P2PInterface
            """
            # SOCKS handlers run in separate threads, so each needs its own RPC connection.
            rpc = self.nodes[0].create_new_rpc_connection()
            conn_type = None

            def connection_type_found():
                nonlocal conn_type
                # The proxy client address equals the node's addrbind for this connection.
                for peer in rpc.getpeerinfo():
                    if peer.get("addrbind") == proxy_client:
                        conn_type = peer["connection_type"]
                        return True
                return False

            self.wait_until(connection_type_found)

            if conn_type == "manual":
                self.log.debug(f"Passing through a manual connection to {requested_to_addr}:{requested_to_port}")
                return {"actual_to_addr": requested_to_addr, "actual_to_port": requested_to_port}

            with self.lock:
                if conn_type == "private-broadcast" and not self.private_broadcast_to_node1_done:
                    self.private_broadcast_to_node1_done = True
                    self.log.debug("Redirecting the first private broadcast connection to nodes[1]")
                    return {"actual_to_addr": "127.0.0.1", "actual_to_port": tor_port(1)}

            listener = P2PInterface()
            listener.peer_connect_helper(dstaddr="0.0.0.0", dstport=0, net=self.chain, timeout_factor=self.options.timeout_factor)
            listener.peer_connect_send_version(services=P2P_SERVICES)
            actual_to_addr, actual_to_port = start_p2p_listener(self.network_thread, listener)
            self.log.debug(f"Redirecting a {conn_type} connection to a Python P2PInterface")
            return {"actual_to_addr": actual_to_addr, "actual_to_port": actual_to_port}

        self.socks5_server = start_socks5_server(destinations_factory)

        self.extra_args = [
            [
                "-privatebroadcast",
                "-v2transport=0",
                f"-proxy={self.socks5_server.conf.addr[0]}:{self.socks5_server.conf.addr[1]}",
            ],
            [
                "-connect=0",
                f"-bind=127.0.0.1:{tor_port(1)}=onion",
            ],
            [
                "-connect=0",
                "-listen=1",
            ],
        ]
        super().setup_nodes()

    def setup_network(self):
        # Keep the nodes disconnected from each other.
        self.setup_nodes()

    def send_privately(self, tx_hex, txid, wtxid):
        """Send a transaction via private broadcast from nodes[0], wait until nodes[1]
        has received it and all private broadcast connections are done."""
        tx_originator = self.nodes[0]
        private_observer = self.nodes[1]

        with self.lock:
            self.private_broadcast_to_node1_done = False
        tx_originator.sendrawtransaction(hexstring=tx_hex, maxfeerate=0.1)

        self.log.info("Observing the private broadcast on nodes[1]")
        self.wait_until(lambda: txid in private_observer.getrawmempool())
        assert_equal(private_observer.getmempoolentry(txid)["wtxid"], wtxid)

        def all_private_broadcasts_done():
            pending = [t for t in tx_originator.getprivatebroadcastinfo()["transactions"] if t["wtxid"] == wtxid]
            assert_equal(len(pending), 1)
            return sum(1 for p in pending[0]["peers"] if "received" in p) >= NUM_PRIVATE_BROADCAST_PER_TX
        self.wait_until(all_private_broadcasts_done)

    def run_test(self):
        tx_originator = self.nodes[0]
        public_observer = self.nodes[2]

        self.log.info("Connecting nodes[0] to nodes[2] with a normal connection")
        self.connect_nodes(0, 2)

        self.fill_node_addrman(node_index=0, address_types_to_add=[CAddress.NET_TORV3])

        wallet = MiniWallet(tx_originator)
        tx = wallet.create_self_transfer()
        self.log.info(f"Created txid={tx['txid']} wtxid={tx['wtxid']}")

        self.log.info("Sending the transaction via private broadcast")
        self.send_privately(tx["hex"], tx["txid"], tx["wtxid"])

        self.log.info("Checking that the transaction did not propagate otherwise")
        assert tx["txid"] not in tx_originator.getrawmempool()
        assert tx["txid"] not in public_observer.getrawmempool()

        # Note: we want to make the node to send the TX again non-privately,
        # Currently the `submitpackage` is a good option for that, but if it also gets changes
        # to be privateproadcast-aware, it should be changed to another method -- TODO
        self.log.info("Sending the same transaction again, non-privately, via submitpackage")
        res = tx_originator.submitpackage([tx["hex"]])
        assert_equal(res["package_msg"], "success")
        assert tx["txid"] in tx_originator.getrawmempool()

        self.log.info("Observing the non-private broadcast on nodes[2]")
        self.wait_until(lambda: tx["txid"] in public_observer.getrawmempool())
        assert_equal(public_observer.getmempoolentry(tx["txid"])["wtxid"], tx["wtxid"])

        if self.is_wallet_compiled():
            self.test_wallet_resend()

        self.stop_node(0)
        self.socks5_server.stop()

    def test_wallet_resend(self):
        tx_originator = self.nodes[0]
        private_observer = self.nodes[1]
        public_observer = self.nodes[2]

        self.init_wallet(node=0)

        self.log.info("Wallet: creating a transaction without adding it to the wallet")
        res = tx_originator.send(outputs=[{tx_originator.getnewaddress(): 1}], add_to_wallet=False)
        assert_equal(res["complete"], True)
        tx_hex = res["hex"]
        tx = tx_originator.decoderawtransaction(tx_hex)
        txid, wtxid = tx["txid"], tx["hash"]
        self.log.info(f"Created txid={txid} wtxid={wtxid}")

        self.log.info("Wallet: sending the transaction via private broadcast")
        self.send_privately(tx_hex, txid, wtxid)
        assert txid not in tx_originator.getrawmempool()

        # Let the transaction come back from the network, so that it gets into the
        # mempool and the wallet of nodes[0]. Keep nodes[2] disconnected meanwhile,
        # so that the relay from nodes[0] does not reach it.
        self.log.info("Wallet: receiving the transaction back from nodes[1]")
        self.disconnect_nodes(0, 2)
        self.connect_nodes(0, 1)
        # nodes[1] already has the transaction, ask it to announce it again.
        private_observer.sendrawtransaction(tx_hex)
        self.wait_until(lambda: txid in tx_originator.getrawmempool())
        assert_equal(tx_originator.gettransaction(txid)["confirmations"], 0)
        self.disconnect_nodes(0, 1)
        assert txid not in public_observer.getrawmempool()

        # Transactions are only resent if there has been a block at least five minutes
        # after they were received by the wallet. Create a block without the transaction.
        block_time = int(time.time()) + 6 * 60
        tx_originator.setmocktime(block_time)
        block = create_block(int(tx_originator.getbestblockhash(), 16), height=tx_originator.getblockcount() + 1, ntime=block_time)
        block.solve()
        assert_equal(tx_originator.submitblock(block.serialize().hex()), None)
        # Update the best block time of the wallet, used in ResubmitWalletTransactions()
        tx_originator.syncwithvalidationinterfacequeue()

        self.log.info("Wallet: connecting nodes[0] to nodes[2] again")
        self.connect_nodes(0, 2)

        self.log.info("Wallet: triggering the periodic resend of the wallet")
        with tx_originator.assert_debug_log(expected_msgs=["resubmit 1 unconfirmed transactions"]):
            tx_originator.setmocktime(block_time + WALLET_RESEND_TIMER_LIMIT)
            # Run MaybeResendWalletTxs() now
            tx_originator.mockscheduler(60)

            self.log.info("Wallet: observing the non-private broadcast on nodes[2]")
            self.wait_until(lambda: txid in public_observer.getrawmempool())
        assert_equal(public_observer.getmempoolentry(txid)["wtxid"], wtxid)

if __name__ == "__main__":
    P2PPrivateBroadcastThenPublic(__file__).main()
