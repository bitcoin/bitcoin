#!/usr/bin/env python3
# Copyright (c) 2017-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test how locally submitted transactions are sent to the network when private broadcast is used.
"""

import time
import threading

from test_framework.p2p import (
    P2PDataStore,
    P2PInterface,
    P2P_SERVICES,
    P2P_VERSION,
)
from test_framework.messages import (
    CAddress,
    CInv,
    MSG_WTX,
    malleate_tx_to_invalid_witness,
    msg_inv,
    msg_tx,
)
from test_framework.netutil import (
    format_addr_port
)
from test_framework.script_util import build_malleated_tx_package
from test_framework.socks5 import (
    Socks5Configuration,
    Socks5Server,
)
from test_framework.test_framework import (
    BitcoinTestFramework,
)
from test_framework.util import (
    assert_equal,
    assert_not_equal,
    assert_raises_rpc_error,
    p2p_port,
    tor_port,
)
from test_framework.wallet import (
    MiniWallet,
)

MAX_OUTBOUND_FULL_RELAY_CONNECTIONS = 8
MAX_BLOCK_RELAY_ONLY_CONNECTIONS = 2
NUM_INITIAL_CONNECTIONS = MAX_OUTBOUND_FULL_RELAY_CONNECTIONS + MAX_BLOCK_RELAY_ONLY_CONNECTIONS
NUM_PRIVATE_BROADCAST_PER_TX = 3

# Fill addrman with these addresses. Must have enough Tor addresses, so that even
# if all 10 default connections are opened to a Tor address (!?) there must be more
# for private broadcast.
ADDRMAN_ADDRESSES = [
    "20.0.0.1",
    "30.0.0.1",
    "40.0.0.1",
    "50.0.0.1",
    "60.0.0.1",
    "70.0.0.1",
    "80.0.0.1",
    "90.0.0.1",
    "100.0.0.1",
    "110.0.0.1",
    "120.0.0.1",
    "130.0.0.1",
    "140.0.0.1",
    "150.0.0.1",
    "160.0.0.1",
    "170.0.0.1",
    "180.0.0.1",
    "190.0.0.1",
    "200.0.0.1",
    "210.0.0.1",

    "[20::1]",
    "[30::1]",
    "[40::1]",
    "[50::1]",
    "[60::1]",
    "[70::1]",
    "[80::1]",
    "[90::1]",
    "[100::1]",
    "[110::1]",
    "[120::1]",
    "[130::1]",
    "[140::1]",
    "[150::1]",
    "[160::1]",
    "[170::1]",
    "[180::1]",
    "[190::1]",
    "[200::1]",
    "[210::1]",

    "testonlyad777777777777777777777777777777777777777775b6qd.onion",
    "testonlyah77777777777777777777777777777777777777777z7ayd.onion",
    "testonlyal77777777777777777777777777777777777777777vp6qd.onion",
    "testonlyap77777777777777777777777777777777777777777r5qad.onion",
    "testonlyat77777777777777777777777777777777777777777udsid.onion",
    "testonlyax77777777777777777777777777777777777777777yciid.onion",
    "testonlya777777777777777777777777777777777777777777rhgyd.onion",
    "testonlybd77777777777777777777777777777777777777777rs4ad.onion",
    "testonlybp77777777777777777777777777777777777777777zs2ad.onion",
    "testonlybt777777777777777777777777777777777777777777x6id.onion",
    "testonlybx777777777777777777777777777777777777777775styd.onion",
    "testonlyb3777777777777777777777777777777777777777774ckid.onion",
    "testonlycd77777777777777777777777777777777777777777733id.onion",
    "testonlych77777777777777777777777777777777777777777t6kid.onion",
    "testonlycl77777777777777777777777777777777777777777tt3ad.onion",
    "testonlyct77777777777777777777777777777777777777777wvhyd.onion",
    "testonlycx7777777777777777777777777777777777777777774bad.onion",
    "testonlyc377777777777777777777777777777777777777777u6aid.onion",
    "testonlydd777777777777777777777777777777777777777777u5ad.onion",
    "testonlydh77777777777777777777777777777777777777777wgnyd.onion",

    "testonlyad77777777777777777777777777777777777777777q.b32.i2p",
    "testonlyah77777777777777777777777777777777777777777q.b32.i2p",
    "testonlyap77777777777777777777777777777777777777777q.b32.i2p",
    "testonlyat77777777777777777777777777777777777777777q.b32.i2p",
    "testonlyax77777777777777777777777777777777777777777q.b32.i2p",
    "testonlya377777777777777777777777777777777777777777q.b32.i2p",
    "testonlya777777777777777777777777777777777777777777q.b32.i2p",
    "testonlybd77777777777777777777777777777777777777777q.b32.i2p",
    "testonlybh77777777777777777777777777777777777777777q.b32.i2p",
    "testonlybl77777777777777777777777777777777777777777q.b32.i2p",
    "testonlybp77777777777777777777777777777777777777777q.b32.i2p",
    "testonlybt77777777777777777777777777777777777777777q.b32.i2p",
    "testonlybx77777777777777777777777777777777777777777q.b32.i2p",
    "testonlyb777777777777777777777777777777777777777777q.b32.i2p",
    "testonlych77777777777777777777777777777777777777777q.b32.i2p",
    "testonlycp77777777777777777777777777777777777777777q.b32.i2p",
    "testonlyct77777777777777777777777777777777777777777q.b32.i2p",
    "testonlycx77777777777777777777777777777777777777777q.b32.i2p",
    "testonlyc377777777777777777777777777777777777777777q.b32.i2p",
    "testonlyc777777777777777777777777777777777777777777q.b32.i2p",

    "[fc00::1]",
    "[fc00::2]",
    "[fc00::3]",
    "[fc00::5]",
    "[fc00::6]",
    "[fc00::7]",
    "[fc00::8]",
    "[fc00::9]",
    "[fc00::10]",
    "[fc00::11]",
    "[fc00::12]",
    "[fc00::13]",
    "[fc00::15]",
    "[fc00::16]",
    "[fc00::17]",
    "[fc00::18]",
    "[fc00::19]",
    "[fc00::20]",
    "[fc00::22]",
    "[fc00::23]",
]


class P2PPrivateBroadcast(BitcoinTestFramework):
    def set_test_params(self):
        self.disable_autoconnect = False
        self.num_nodes = 2

    def setup_nodes(self):
        # Start a SOCKS5 proxy server.
        socks5_server_config = Socks5Configuration()
        # self.nodes[0] listens on p2p_port(0),
        # self.nodes[1] listens on p2p_port(1),
        # thus we tell the SOCKS5 server to listen on p2p_port(self.num_nodes) (self.num_nodes is 2)
        socks5_server_config.addr = ("127.0.0.1", p2p_port(self.num_nodes))
        socks5_server_config.unauth = True
        socks5_server_config.auth = True

        self.socks5_server = Socks5Server(socks5_server_config)
        self.socks5_server.start()

        self.destinations = []

        self.destinations_lock = threading.Lock()

        def destinations_factory(requested_to_addr, requested_to_port):
            with self.destinations_lock:
                i = len(self.destinations)
                actual_to_addr = ""
                actual_to_port = 0
                listener = None
                if i == NUM_INITIAL_CONNECTIONS:
                    # Instruct the SOCKS5 server to redirect the first private
                    # broadcast connection from nodes[0] to nodes[1]
                    actual_to_addr = "127.0.0.1" # nodes[1] listen address
                    actual_to_port = tor_port(1) # nodes[1] listen port for Tor
                    self.log.debug(f"Instructing the SOCKS5 proxy to redirect connection i={i} to "
                                   f"{format_addr_port(actual_to_addr, actual_to_port)} (nodes[1])")
                else:
                    # Create a Python P2P listening node and instruct the SOCKS5 proxy to
                    # redirect the connection to it. The first outbound connection is used
                    # later to serve GETDATA, thus make it P2PDataStore().
                    listener = P2PDataStore() if i == 0 else P2PInterface()
                    listener.peer_connect_helper(dstaddr="0.0.0.0", dstport=0, net=self.chain, timeout_factor=self.options.timeout_factor)
                    listener.peer_connect_send_version(services=P2P_SERVICES)

                    def on_listen_done(addr, port):
                        nonlocal actual_to_addr
                        nonlocal actual_to_port
                        actual_to_addr = addr
                        actual_to_port = port

                    # Use port=0 to let the OS assign an available port. This
                    # avoids "address already in use" errors when tests run
                    # concurrently or ports are still in TIME_WAIT state.
                    self.network_thread.listen(
                        addr="127.0.0.1",
                        port=0,
                        p2p=listener,
                        callback=on_listen_done)
                    # Wait until the callback has been called.
                    self.wait_until(lambda: actual_to_port != 0)
                    self.log.debug(f"Instructing the SOCKS5 proxy to redirect connection i={i} to "
                                   f"{format_addr_port(actual_to_addr, actual_to_port)} (a Python node)")

                self.destinations.append({
                    "requested_to": format_addr_port(requested_to_addr, requested_to_port),
                    "node": listener,
                })
                assert_equal(len(self.destinations), i + 1)

                return {
                    "actual_to_addr": actual_to_addr,
                    "actual_to_port": actual_to_port,
                }

        self.socks5_server.conf.destinations_factory = destinations_factory

        self.extra_args = [
            [
                # Needed to be able to add CJDNS addresses to addrman (otherwise they are unroutable).
                "-cjdnsreachable",
                # Connecting, sending garbage, being disconnected messes up with this test's
                # check_broadcasts() which waits for a particular Python node to receive a connection.
                "-v2transport=0",
                "-test=addrman",
                "-privatebroadcast",
                f"-proxy={socks5_server_config.addr[0]}:{socks5_server_config.addr[1]}",
                # To increase coverage, make it think that the I2P network is reachable so that it
                # selects such addresses as well. Pick a proxy address where nobody is listening
                # and connection attempts fail quickly.
                "-i2psam=127.0.0.1:1",
            ],
            [
                "-connect=0",
                f"-bind=127.0.0.1:{tor_port(1)}=onion",
            ],
        ]
        super().setup_nodes()

    def setup_network(self):
        self.setup_nodes()

    def check_broadcasts(self, label, tx, broadcasts_to_expect, skip_destinations):
        broadcasts_done = 0
        i = skip_destinations - 1
        while broadcasts_done < broadcasts_to_expect:
            i += 1
            self.log.debug(f"{label}: waiting for outbound connection i={i}")
            # At this point the connection may not yet have been established (A),
            # may be active (B), or may have already been closed (C).
            self.wait_until(lambda: len(self.destinations) > i)
            dest = self.destinations[i]
            peer = dest["node"]
            peer.wait_until(lambda: peer.message_count["version"] == 1, check_connected=False)
            # Now it is either (B) or (C).
            if peer.last_message["version"].nServices != 0:
                self.log.debug(f"{label}: outbound connection i={i} to {dest['requested_to']} not a private broadcast, ignoring it (maybe feeler or extra block only)")
                continue
            self.log.debug(f"{label}: outbound connection i={i} to {dest['requested_to']} must be a private broadcast, checking it")
            peer.wait_for_disconnect()
            # Now it is (C).
            assert_equal(peer.message_count, {
                "version": 1,
                "verack": 1,
                "inv": 1,
                "tx": 1,
                "ping": 1
            })
            dummy_address = CAddress()
            dummy_address.nServices = 0
            assert_equal(peer.last_message["version"].nVersion, P2P_VERSION)
            assert_equal(peer.last_message["version"].nServices, 0)
            assert_equal(peer.last_message["version"].nTime, 0)
            assert_equal(peer.last_message["version"].addrTo, dummy_address)
            assert_equal(peer.last_message["version"].addrFrom, dummy_address)
            assert_equal(peer.last_message["version"].strSubVer, "/pynode:0.0.1/")
            assert_equal(peer.last_message["version"].nStartingHeight, 0)
            assert_equal(peer.last_message["version"].relay, 0)
            assert_equal(peer.last_message["tx"].tx.txid_hex, tx["txid"])
            self.log.info(f"{label}: ok: outbound connection i={i} is private broadcast of txid={tx['txid']}")
            broadcasts_done += 1

    def run_test(self):
        tx_originator = self.nodes[0]
        tx_receiver = self.nodes[1]
        far_observer = tx_receiver.add_p2p_connection(P2PInterface())

        wallet = MiniWallet(tx_originator)

        # Fill tx_originator's addrman.
        for addr in ADDRMAN_ADDRESSES:
            res = tx_originator.addpeeraddress(address=addr, port=8333, tried=False)
            if not res["success"]:
                self.log.debug(f"Could not add {addr} to tx_originator's addrman (collision?)")

        self.wait_until(lambda: len(self.destinations) == NUM_INITIAL_CONNECTIONS)

        # The next opened connection by tx_originator should be "private broadcast"
        # for sending the transaction. The SOCKS5 proxy should redirect it to tx_receiver.

        txs = wallet.create_self_transfer_chain(chain_length=3)
        self.log.info(f"Created txid={txs[0]['txid']}: for basic test")
        self.log.info(f"Created txid={txs[1]['txid']}: for broadcast with dependency in mempool + rebroadcast")
        self.log.info(f"Created txid={txs[2]['txid']}: for broadcast with dependency not in mempool")
        tx_originator.sendrawtransaction(hexstring=txs[0]["hex"], maxfeerate=0.1)

        self.log.debug(f"Waiting for outbound connection i={NUM_INITIAL_CONNECTIONS}, "
                       "must be the first private broadcast connection")
        self.wait_until(lambda: len(tx_receiver.getrawmempool()) > 0)
        far_observer.wait_for_tx(txs[0]["txid"])
        self.log.info(f"Outbound connection i={NUM_INITIAL_CONNECTIONS}: "
                      "the private broadcast target received and further relayed the transaction")

        # One already checked above, check the other NUM_PRIVATE_BROADCAST_PER_TX - 1 broadcasts.
        self.check_broadcasts("Basic", txs[0], NUM_PRIVATE_BROADCAST_PER_TX - 1, NUM_INITIAL_CONNECTIONS + 1)

        self.log.info("Resending the same transaction via RPC again (it is not in the mempool yet)")
        ignoring_msg = f"Ignoring unnecessary request to schedule an already scheduled transaction: txid={txs[0]['txid']}, wtxid={txs[0]['wtxid']}"
        with tx_originator.busy_wait_for_debug_log(expected_msgs=[ignoring_msg.encode()]):
            tx_originator.sendrawtransaction(hexstring=txs[0]["hex"], maxfeerate=0)

        self.log.info("Sending a malleated transaction with an invalid witness via RPC")
        malleated_invalid = malleate_tx_to_invalid_witness(txs[0])
        assert_raises_rpc_error(-26, "mempool-script-verify-flag-failed",
                                tx_originator.sendrawtransaction,
                                hexstring=malleated_invalid.serialize_with_witness().hex(),
                                maxfeerate=0.1)

        self.log.info("Checking that the transaction is not in the originator node's mempool")
        assert_equal(len(tx_originator.getrawmempool()), 0)

        wtxid_int = int(txs[0]["wtxid"], 16)
        inv = CInv(MSG_WTX, wtxid_int)

        self.log.info("Sending INV and waiting for GETDATA from node")
        tx_returner = self.destinations[0]["node"] # Will return the transaction back to the originator.
        tx_returner.tx_store[wtxid_int] = txs[0]["tx"]
        assert "getdata" not in tx_returner.last_message
        received_back_msg = f"Received our privately broadcast transaction (txid={txs[0]['txid']}) from the network"
        with tx_originator.assert_debug_log(expected_msgs=[received_back_msg]):
            tx_returner.send_without_ping(msg_inv([inv]))
            tx_returner.wait_until(lambda: "getdata" in tx_returner.last_message)
            self.wait_until(lambda: len(tx_originator.getrawmempool()) > 0)

        self.log.info("Waiting for normal broadcast to another peer")
        self.destinations[1]["node"].wait_for_inv([inv])

        self.log.info("Sending a transaction that is already in the mempool")
        skip_destinations = len(self.destinations)
        tx_originator.sendrawtransaction(hexstring=txs[0]["hex"], maxfeerate=0)
        self.check_broadcasts("Broadcast of mempool transaction", txs[0], NUM_PRIVATE_BROADCAST_PER_TX, skip_destinations)

        self.log.info("Sending a transaction with a dependency in the mempool")
        skip_destinations = len(self.destinations)
        tx_originator.sendrawtransaction(hexstring=txs[1]["hex"], maxfeerate=0.1)
        self.check_broadcasts("Dependency in mempool", txs[1], NUM_PRIVATE_BROADCAST_PER_TX, skip_destinations)

        self.log.info("Sending a transaction with a dependency not in the mempool (should be rejected)")
        assert_equal(len(tx_originator.getrawmempool()), 1)
        assert_raises_rpc_error(-25, "bad-txns-inputs-missingorspent",
                                tx_originator.sendrawtransaction, hexstring=txs[2]["hex"], maxfeerate=0.1)
        assert_raises_rpc_error(-25, "bad-txns-inputs-missingorspent",
                                tx_originator.sendrawtransaction, hexstring=txs[2]["hex"], maxfeerate=0)

        # Since txs[1] has not been received back by tx_originator,
        # it should be re-broadcast after a while. Advance tx_originator's clock
        # to trigger a re-broadcast. Should be more than the maximum returned by
        # NextTxBroadcast() in net_processing.cpp.
        self.log.info("Checking that rebroadcast works")
        delta = 20 * 60 # 20min
        skip_destinations = len(self.destinations)
        rebroadcast_msg = f"Reattempting broadcast of stale txid={txs[1]['txid']}"
        with tx_originator.busy_wait_for_debug_log(expected_msgs=[rebroadcast_msg.encode()]):
            tx_originator.setmocktime(int(time.time()) + delta)
            tx_originator.mockscheduler(delta)
        self.check_broadcasts("Rebroadcast", txs[1], 1, skip_destinations)
        tx_originator.setmocktime(0) # Let the clock tick again (it will go backwards due to this).

        self.log.info("Sending a pair of transactions with the same txid but different valid wtxids via RPC")
        parent = wallet.create_self_transfer()["tx"]
        parent_amount = parent.vout[0].nValue - 10000
        child_amount = parent_amount - 10000
        siblings_parent, sibling1, sibling2 = build_malleated_tx_package(
            parent=parent,
            rebalance_parent_output_amount=parent_amount,
            child_amount=child_amount)
        self.log.info(f"  - sibling1: txid={sibling1.txid_hex}, wtxid={sibling1.wtxid_hex}")
        self.log.info(f"  - sibling2: txid={sibling2.txid_hex}, wtxid={sibling2.wtxid_hex}")
        assert_equal(sibling1.txid_hex, sibling2.txid_hex)
        assert_not_equal(sibling1.wtxid_hex, sibling2.wtxid_hex)
        assert_equal(len(tx_originator.getrawmempool()), 1)
        tx_returner.send_without_ping(msg_tx(siblings_parent))
        self.wait_until(lambda: len(tx_originator.getrawmempool()) > 1)
        self.log.info("  - siblings' parent added to the mempool")
        tx_originator.sendrawtransaction(hexstring=sibling1.serialize_with_witness().hex(), maxfeerate=0.1)
        self.log.info("  - sent sibling1: ok")
        tx_originator.sendrawtransaction(hexstring=sibling2.serialize_with_witness().hex(), maxfeerate=0.1)
        self.log.info("  - sent sibling2: ok")

        # Stop the SOCKS5 proxy server to avoid it being upset by the bitcoin
        # node disconnecting in the middle of the SOCKS5 handshake when we
        # restart below.
        self.socks5_server.stop()

        self.log.info("Trying to send a transaction when none of Tor or I2P is reachable")
        self.restart_node(0, extra_args=[
            "-privatebroadcast",
            "-v2transport=0",
            # A location where definitely a Tor control is not listening. This would allow
            # Bitcoin Core to start, hoping/assuming that the location of the Tor proxy
            # may be retrieved after startup from the Tor control, but it will not be, so
            # the RPC should throw.
            "-torcontrol=127.0.0.1:1",
            "-listenonion",
        ])
        assert_raises_rpc_error(-1, "none of the Tor or I2P networks is reachable",
                                tx_originator.sendrawtransaction, hexstring=txs[0]["hex"], maxfeerate=0.1)


if __name__ == "__main__":
    P2PPrivateBroadcast(__file__).main()
