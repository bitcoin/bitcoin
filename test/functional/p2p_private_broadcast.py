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
    malleate_tx,
    msg_inv,
)
from test_framework.netutil import (
    format_addr_port
)
from test_framework.socks5 import (
    Socks5Configuration,
    Socks5Server,
)
from test_framework.test_framework import (
    BitcoinTestFramework,
)
from test_framework.util import (
    MAX_NODES,
    assert_equal,
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
    "1.65.195.98",
    "2.59.236.56",
    "2.83.114.20",
    "2.248.194.16",
    "5.2.154.6",
    "5.101.140.30",
    "5.128.87.126",
    "5.144.21.49",
    "5.172.132.104",
    "5.188.62.18",
    "5.200.2.180",
    "8.129.184.255",
    "8.209.105.138",
    "12.34.98.148",
    "14.199.102.151",
    "18.27.79.17",
    "18.27.124.231",
    "18.216.249.151",
    "23.88.155.58",
    "23.93.101.158",
    "[2001:19f0:1000:1db3:5400:4ff:fe56:5a8d]",
    "[2001:19f0:5:24da:3eec:efff:feb9:f36e]",
    "[2001:19f0:5:24da::]",
    "[2001:19f0:5:4535:3eec:efff:feb9:87e4]",
    "[2001:19f0:5:4535::]",
    "[2001:1bc0:c1::2000]",
    "[2001:1c04:4008:6300:8a5f:2678:114b:a660]",
    "[2001:41d0:203:3739::]",
    "[2001:41d0:203:8f49::]",
    "[2001:41d0:203:bb0a::]",
    "[2001:41d0:2:bf8f::]",
    "[2001:41d0:303:de8b::]",
    "[2001:41d0:403:3d61::]",
    "[2001:41d0:405:9600::]",
    "[2001:41d0:8:ed7f::1]",
    "[2001:41d0:a:69a2::1]",
    "[2001:41f0::62:6974:636f:696e]",
    "[2001:470:1b62::]",
    "[2001:470:1f05:43b:2831:8530:7179:5864]",
    "[2001:470:1f09:b14::11]",
    "2bqghnldu6mcug4pikzprwhtjjnsyederctvci6klcwzepnjd46ikjyd.onion",
    "4lr3w2iyyl5u5l6tosizclykf5v3smqroqdn2i4h3kq6pfbbjb2xytad.onion",
    "5g72ppm3krkorsfopcm2bi7wlv4ohhs4u4mlseymasn7g7zhdcyjpfid.onion",
    "5sbmcl4m5api5tqafi4gcckrn3y52sz5mskxf3t6iw4bp7erwiptrgqd.onion",
    "776aegl7tfhg6oiqqy76jnwrwbvcytsx2qegcgh2mjqujll4376ohlid.onion",
    "77mdte42srl42shdh2mhtjr7nf7dmedqrw6bkcdekhdvmnld6ojyyiad.onion",
    "azbpsh4arqlm6442wfimy7qr65bmha2zhgjg7wbaji6vvaug53hur2qd.onion",
    "b64xcbleqmwgq2u46bh4hegnlrzzvxntyzbmucn3zt7cssm7y4ubv3id.onion",
    "bsqbtcparrfihlwolt4xgjbf4cgqckvrvsfyvy6vhiqrnh4w6ghixoid.onion",
    "bsqbtctulf2g4jtjsdfgl2ed7qs6zz5wqx27qnyiik7laockryvszqqd.onion",
    "cwi3ekrwhig47dhhzfenr5hbvckj7fzaojygvazi2lucsenwbzwoyiqd.onion",
    "devinbtcmwkuitvxl3tfi5of4zau46ymeannkjv6fpnylkgf3q5fa3id.onion",
    "devinbtcyk643iruzfpaxw3on2jket7rbjmwygm42dmdyub3ietrbmid.onion",
    "dtql5vci4iaml4anmueftqr7bfgzqlauzfy4rc2tfgulldd3ekyijjyd.onion",
    "emzybtc25oddoa2prol2znpz2axnrg6k77xwgirmhv7igoiucddsxiad.onion",
    "emzybtc3ewh7zihpkdvuwlgxrhzcxy2p5fvjggp7ngjbxcytxvt4rjid.onion",
    "emzybtc454ewbviqnmgtgx3rgublsgkk23r4onbhidcv36wremue4kqd.onion",
    "emzybtc5bnpb2o6gh54oquiox54o4r7yn4a2wiiwzrjonlouaibm2zid.onion",
    "fpz6r5ppsakkwypjcglz6gcnwt7ytfhxskkfhzu62tnylcknh3eq6pad.onion",
    "255fhcp6ajvftnyo7bwz3an3t4a4brhopm3bamyh2iu5r3gnr2rq.b32.i2p",
    "27yrtht5b5bzom2w5ajb27najuqvuydtzb7bavlak25wkufec5mq.b32.i2p",
    "3gocb7wc4zvbmmebktet7gujccuux4ifk3kqilnxnj5wpdpqx2hq.b32.i2p",
    "4fcc23wt3hyjk3csfzcdyjz5pcwg5dzhdqgma6bch2qyiakcbboa.b32.i2p",
    "4osyqeknhx5qf3a73jeimexwclmt42cju6xdp7icja4ixxguu2hq.b32.i2p",
    "4umsi4nlmgyp4rckosg4vegd2ysljvid47zu7pqsollkaszcbpqq.b32.i2p",
    "6j2ezegd3e2e2x3o3pox335f5vxfthrrigkdrbgfbdjchm5h4awa.b32.i2p",
    "6n36ljyr55szci5ygidmxqer64qr24f4qmnymnbvgehz7qinxnla.b32.i2p",
    "72yjs6mvlby3ky6mgpvvlemmwq5pfcznrzd34jkhclgrishqdxva.b32.i2p",
    "a5qsnv3maw77mlmmzlcglu6twje6ttctd3fhpbfwcbpmewx6fczq.b32.i2p",
    "aovep2pco7v2k4rheofrgytbgk23eg22dczpsjqgqtxcqqvmxk6a.b32.i2p",
    "bitcoi656nll5hu6u7ddzrmzysdtwtnzcnrjd4rfdqbeey7dmn5a.b32.i2p",
    "brifkruhlkgrj65hffybrjrjqcgdgqs2r7siizb5b2232nruik3a.b32.i2p",
    "c4gfnttsuwqomiygupdqqqyy5y5emnk5c73hrfvatri67prd7vyq.b32.i2p",
    "day3hgxyrtwjslt54sikevbhxxs4qzo7d6vi72ipmscqtq3qmijq.b32.i2p",
    "du5kydummi23bjfp6bd7owsvrijgt7zhvxmz5h5f5spcioeoetwq.b32.i2p",
    "e55k6wu46rzp4pg5pk5npgbr3zz45bc3ihtzu2xcye5vwnzdy7pq.b32.i2p",
    "eciohu5nq7vsvwjjc52epskuk75d24iccgzmhbzrwonw6lx4gdva.b32.i2p",
    "ejlnngarmhqvune74ko7kk55xtgbz5i5ncs4vmnvjpy3l7y63xaa.b32.i2p",
    "fhzlp3xroabohnmjonu5iqazwhlbbwh5cpujvw2azcu3srqdceja.b32.i2p",
    "[fc32:17ea:e415:c3bf:9808:149d:b5a2:c9aa]",
    "[fcc7:be49:ccd1:dc91:3125:f0da:457d:8ce]",
    "[fcdc:73ae:b1a9:1bf8:d4c2:811:a4c7:c34e]",
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
        # thus we tell the SOCKS5 server to listen on p2p_port(self.num_nodes (which is 2))
        socks5_server_config.addr = ("127.0.0.1", p2p_port(self.num_nodes))
        socks5_server_config.unauth = True
        socks5_server_config.auth = True

        self.socks5_server = Socks5Server(socks5_server_config)
        self.socks5_server.start()

        # Tor ports are the highest among p2p/rpc/tor, so this should be the first available port.
        ports_base = tor_port(MAX_NODES) + 1

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

                    self.network_thread.listen(
                        addr="127.0.0.1",
                        port=ports_base + i,
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

        self.log.info("Resending the same transaction via RPC again")
        ignoring_msg = f"Ignoring unnecessary request to schedule an already scheduled transaction: txid={txs[0]['txid']}, wtxid={txs[0]['wtxid']}"
        with tx_originator.busy_wait_for_debug_log(expected_msgs=[ignoring_msg.encode()]):
            tx_originator.sendrawtransaction(hexstring=txs[0]["hex"], maxfeerate=0.1)

        # TODO: Create a malleated valid witness (how?) and substitute malleate_tx() below:
        #self.log.info("Sending a malleated transaction with a valid witness via RPC")
        #malleated_valid = malleate_tx(txs[0])
        #ignoring_msg = f"Ignoring unnecessary request to schedule an already scheduled transaction: txid={malleated_valid.txid_hex}, wtxid={malleated_valid.getwtxid()}"
        #with tx_originator.busy_wait_for_debug_log(expected_msgs=[ignoring_msg.encode()]):
        #    tx_originator.sendrawtransaction(hexstring=malleated_valid.serialize_with_witness().hex(), maxfeerate=0.1)

        self.log.info("Sending a malleated transaction with an invalid witness via RPC")
        malleated_invalid = malleate_tx(txs[0])
        assert_raises_rpc_error(-26, "mandatory-script-verify-flag-failed",
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
