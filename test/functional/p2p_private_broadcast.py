#!/usr/bin/env python3
# Copyright (c) 2017-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test how locally submitted transactions are sent to the network when private broadcast is used.

The topology in the test is:

Bitcoin Core (tx_originator)
               ^   v       The default full-outbound + block-only connections
               |   |       (MAX_OUTBOUND_FULL_RELAY_CONNECTIONS + MAX_BLOCK_RELAY_ONLY_CONNECTIONS):
               |   |
               |   *-----> SOCKS5 Proxy ---> P2PInterface (self.socks5_server.conf.destinations[0]["node"])
               |   |
               |   *-----> SOCKS5 Proxy ---> P2PInterface (self.socks5_server.conf.destinations[1]["node"])
               |   ...
               |   |       The private broadcast TX recipients
               |   |       (NUM_PRIVATE_BROADCAST_PER_TX)
               |   |       plus maybe some feeler or extra block only connections:
               |   |
               |   *-----> SOCKS5 Proxy ---> Bitcoin Core (node1) (self.socks5_server.conf.destinations[i]["node"] is None)
               |   |                          ^
               |   |                          *----< P2PInterface (far_observer) (to check Bitcoin Core relays the tx)
               |   |
               |   *-----> SOCKS5 Proxy ---> P2PInterface (self.socks5_server.conf.destinations[i + 1]["node"])
               |   ...
               |
               *---------< P2PDataStore (observer_inbound)
"""

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
    msg_inv,
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
    p2p_port,
)
from test_framework.wallet import (
    MiniWallet,
)

MAX_OUTBOUND_FULL_RELAY_CONNECTIONS = 8
MAX_BLOCK_RELAY_ONLY_CONNECTIONS = 2
NUM_INITIAL_CONNECTIONS = MAX_OUTBOUND_FULL_RELAY_CONNECTIONS + MAX_BLOCK_RELAY_ONLY_CONNECTIONS
NUM_PRIVATE_BROADCAST_PER_TX = 5

# Fill addrman with these addresses. Must have enough Tor addresses, so that even
# if all 10 default connections are opened to a Tor address (!?) there must be more
# for private broadcast.
addresses = [
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

        ports_base = p2p_port(MAX_NODES) + 15000

        # Add 2 because one feeler and one extra block relay connections may
        # sneak in between the private broadcast connections.
        for i in range(NUM_INITIAL_CONNECTIONS + NUM_PRIVATE_BROADCAST_PER_TX + 2):
            if i == NUM_INITIAL_CONNECTIONS:
                # Instruct the SOCKS5 server to redirect the first private
                # broadcast connection from nodes[0] to nodes[1]
                self.socks5_server.conf.destinations.append({
                    "to_addr": "127.0.0.1", # nodes[1] listen address
                    "to_port": p2p_port(1), # nodes[1] listen port
                    "node": None,
                    "requested_to_addr": None,
                })
            else:
                # Create a Python P2P listening node and add it to self.socks5_server.conf.destinations[]
                listener = P2PInterface()
                listener.peer_connect_helper(dstaddr="0.0.0.0", dstport=0, net=self.chain, timeout_factor=self.options.timeout_factor)
                listener.peer_connect_send_version(services=P2P_SERVICES)
                self.network_thread.listen(
                    p2p=listener,
                    # Instruct the SOCKS5 server to redirect a connection to this Python P2P listener.
                    callback=lambda addr, port: self.socks5_server.conf.destinations.append({
                        "to_addr": addr,
                        "to_port": port,
                        "node": None,
                        "requested_to_addr": None,
                    }),
                    addr="127.0.0.1",
                    port=ports_base + i)
                # Wait until the callback has been called and it has done append().
                self.wait_until(lambda: len(self.socks5_server.conf.destinations) == i + 1)

                self.socks5_server.conf.destinations[i]["node"] = listener

        self.extra_args = [
            [
                "-cjdnsreachable", # needed to be able to add CJDNS addresses to addrman (otherwise they are unroutable).
                "-test=addrman",
                "-privatebroadcast",
                f"-proxy={socks5_server_config.addr[0]}:{socks5_server_config.addr[1]}",
            ],
            [
                "-connect=0",
                f"-bind=127.0.0.1:{p2p_port(1)}=onion", # consider all incoming as coming from Tor
            ],
        ]
        super().setup_nodes()

    def setup_network(self):
        self.setup_nodes()

    def run_test(self):
        tx_originator = self.nodes[0]

        observer_inbound = tx_originator.add_p2p_connection(P2PDataStore())

        # Fill tx_originator's addrman.
        for addr in addresses:
            res = tx_originator.addpeeraddress(address=addr, port=8333, tried=False)
            if not res["success"]:
                self.log.debug(f"Could not add {addr} to tx_originator's addrman (collision?)")

        self.wait_until(lambda: self.socks5_server.conf.destinations_used == NUM_INITIAL_CONNECTIONS)

        node1 = self.nodes[1]
        far_observer = node1.add_p2p_connection(P2PInterface())

        # The next opened connections should be "private broadcast" for sending the transaction.

        miniwallet = MiniWallet(tx_originator)
        tx = miniwallet.send_self_transfer(from_node=tx_originator)
        self.log.info(f"Created transaction txid={tx['txid']}")

        self.log.debug(f"Inspecting outbound connection i={NUM_INITIAL_CONNECTIONS}, must be the first private broadcast connection")
        self.wait_until(lambda: len(node1.getrawmempool()) > 0)
        far_observer.wait_for_tx(tx["txid"])
        self.log.debug(f"Outbound connection i={NUM_INITIAL_CONNECTIONS}: the private broadcast target received and further relayed the transaction")

        num_private_broadcast_opened = 1 # already 1 connection to node1, confirmed by far_observer getting the tx
        for i in range(NUM_INITIAL_CONNECTIONS + 1, len(self.socks5_server.conf.destinations)):
            self.log.debug(f"Inspecting outbound connection i={i}")
            # At this point the connection may not yet have been established (A),
            # may be active (B), or may have already been closed (C).
            peer = self.socks5_server.conf.destinations[i]["node"]
            peer.wait_until(lambda: peer.message_count["version"] == 1, check_connected=False)
            # Now it is either (B) or (C).
            requested_to_addr = self.socks5_server.conf.destinations[i]["requested_to_addr"]
            if peer.last_message["version"].nServices != 0:
                self.log.debug(f"Outbound connection i={i} to {requested_to_addr} not a private broadcast, ignoring it (maybe feeler or extra block only)")
                continue
            self.log.debug(f"Outbound connection i={i} to {requested_to_addr} must be a private broadcast, checking it")
            peer.wait_for_disconnect()
            # Now it is (C).
            assert_equal(peer.message_count, {
                "version": 1,
                "verack": 1,
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
            assert_equal(peer.last_message["tx"].tx.rehash(), tx["txid"])
            self.log.debug(f"Outbound connection i={i} is proper private broadcast, test ok")
            num_private_broadcast_opened += 1
            if num_private_broadcast_opened == NUM_PRIVATE_BROADCAST_PER_TX:
                break
        assert_equal(num_private_broadcast_opened, NUM_PRIVATE_BROADCAST_PER_TX)

        wtxid_int = int(tx["wtxid"], 16)
        inv = CInv(MSG_WTX, wtxid_int)

        self.log.info("Checking that the transaction is not in the originator node's mempool")
        assert_equal(len(tx_originator.getrawmempool()), 0)

        self.log.info("Sending INV from an observer and waiting for GETDATA from node")
        observer_inbound.tx_store[wtxid_int] = tx["tx"]
        assert "getdata" not in observer_inbound.last_message
        observer_inbound.send_message(msg_inv([inv]))
        observer_inbound.wait_until(lambda: "getdata" in observer_inbound.last_message)
        self.wait_until(lambda: len(tx_originator.getrawmempool()) > 0)

        self.log.info("Waiting for normal broadcast to another observer")
        observer_outbound = self.socks5_server.conf.destinations[0]["node"]
        observer_outbound.wait_for_inv([inv])


if __name__ == "__main__":
    P2PPrivateBroadcast().main()
