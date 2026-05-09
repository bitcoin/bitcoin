#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""
Test that a private-broadcast decoy connection is opened on a schedule and
announces the most recent mempool transaction.
"""

import time

from test_framework.socks5 import (
    Socks5Configuration,
    Socks5Server,
)
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import (
    assert_equal,
    p2p_port,
    tor_port,
)
from test_framework.wallet import MiniWallet

DUMMY_ONION_ADDR = "testonlyad777777777777777777777777777777777777777775b6qd.onion"
DUMMY_ONION_PORT = 8333


class P2PPrivateBroadcastDecoyTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.disable_autoconnect = False

    def setup_network(self):
        socks5_config = Socks5Configuration()
        socks5_config.addr = ("127.0.0.1", p2p_port(self.num_nodes))
        socks5_config.unauth = True

        def factory(_addr, _port):
            return {"actual_to_addr": "127.0.0.1", "actual_to_port": tor_port(1)}

        self.socks5_server = Socks5Server(socks5_config)
        self.socks5_server.conf.destinations_factory = factory
        self.socks5_server.start()

        self.extra_args = [
            [
                f"-onion=127.0.0.1:{socks5_config.addr[1]}",
                # We can't use connect=0 for private broadcast
                "-maxconnections=0",
                "-test=addrman",
                "-v2transport=0",
            ],
            [
                "-connect=0",
                f"-bind=127.0.0.1:{tor_port(1)}=onion",
            ],
        ]
        self.setup_nodes()

    def run_test(self):
        sender, receiver = self.nodes

        sender.addpeeraddress(
            address=DUMMY_ONION_ADDR, port=DUMMY_ONION_PORT, tried=False
        )

        wallet = MiniWallet(sender)
        tx = wallet.create_self_transfer()
        sender.sendrawtransaction(hexstring=tx["hex"])
        assert tx["txid"] in sender.getrawmempool()
        assert tx["txid"] not in receiver.getrawmempool()

        # No connections in either direction yet.
        assert_equal(sender.getpeerinfo(), [])
        assert_equal(receiver.getpeerinfo(), [])

        # The decoy schedule is exponential with a 3h mean, so the next
        # decoy is virtually certain (~1 in 9 million) to fire within 48h.
        self.log.info("Advancing nodes[0]'s scheduler 48h to fire the decoy")
        for _ in range(48):
            sender.mockscheduler(60 * 60)

        self.log.info("Waiting for nodes[1] to receive the decoy-broadcast tx")
        self.wait_until(lambda: tx["txid"] in receiver.getrawmempool())

        self.log.info(
            "Advancing nodes[0]'s clock 4min to timeout any decoy connections"
        )
        sender.setmocktime(int(time.time()) + 4 * 60)

        self.log.info("Verifying nodes[0] and nodes[1] are not connected")
        self.wait_until(lambda: sender.getpeerinfo() == [])
        self.wait_until(lambda: receiver.getpeerinfo() == [])


if __name__ == "__main__":
    P2PPrivateBroadcastDecoyTest(__file__).main()
