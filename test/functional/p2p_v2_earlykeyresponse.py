#!/usr/bin/env python3
# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import random

from test_framework.test_framework import BitcoinTestFramework
from test_framework.crypto.ellswift import ellswift_create
from test_framework.p2p import P2PInterface
from test_framework.v2_p2p import EncryptedP2PState


class TestEncryptedP2PState(EncryptedP2PState):
    """ Modify v2 P2P protocol functions for testing that "The responder waits until one byte is received which does
    not match the 16 bytes consisting of the network magic followed by "version\x00\x00\x00\x00\x00"." (see BIP 324)

    - if `send_net_magic` is True, send first 4 bytes of ellswift (match network magic) else send remaining 60 bytes
    - `can_data_be_received` is a variable used to assert if data is received on recvbuf.
            - v2 TestNode shouldn't respond back if we send V1_PREFIX and data shouldn't be received on recvbuf.
              This state is represented using `can_data_be_received` = False.
            - v2 TestNode responds back when mismatch from V1_PREFIX happens and data can be received on recvbuf.
              This state is represented using `can_data_be_received` = True.
    """

    def __init__(self):
        super().__init__(initiating=True, net='regtest')
        self.send_net_magic = True
        self.can_data_be_received = False

    def initiate_v2_handshake(self, garbage_len=random.randrange(4096)):
        """Initiator begins the v2 handshake by sending its ellswift bytes and garbage.
        Here, the 64 bytes ellswift is assumed to have it's 4 bytes match network magic bytes. It is sent in 2 phases:
            1. when `send_network_magic` = True, send first 4 bytes of ellswift (matches network magic bytes)
            2. when `send_network_magic` = False, send remaining 60 bytes of ellswift
        """
        if self.send_net_magic:
            self.privkey_ours, self.ellswift_ours = ellswift_create()
            self.sent_garbage = random.randbytes(garbage_len)
            self.send_net_magic = False
            return b"\xfa\xbf\xb5\xda"
        else:
            self.can_data_be_received = True
            return self.ellswift_ours[4:] + self.sent_garbage


class PeerEarlyKey(P2PInterface):
    """Custom implementation of P2PInterface which uses modified v2 P2P protocol functions for testing purposes."""
    def __init__(self):
        super().__init__()
        self.v2_state = None
        self.connection_opened = False

    def connection_made(self, transport):
        """64 bytes ellswift is sent in 2 parts during `initial_v2_handshake()`"""
        self.v2_state = TestEncryptedP2PState()
        super().connection_made(transport)

    def data_received(self, t):
        # check that data can be received on recvbuf only when mismatch from V1_PREFIX happens (send_net_magic = False)
        assert self.v2_state.can_data_be_received and not self.v2_state.send_net_magic

    def on_open(self):
        self.connection_opened = True

class P2PEarlyKey(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-v2transport=1", "-peertimeout=3"]]

    def run_test(self):
        self.log.info('Sending ellswift bytes in parts to ensure that response from responder is received only when')
        self.log.info('ellswift bytes have a mismatch from the 16 bytes(network magic followed by "version\\x00\\x00\\x00\\x00\\x00")')
        node0 = self.nodes[0]
        self.log.info('Sending first 4 bytes of ellswift which match network magic')
        self.log.info('If a response is received, assertion failure would happen in our custom data_received() function')
        # send happens in `initiate_v2_handshake()` in `connection_made()`
        peer1 = node0.add_p2p_connection(PeerEarlyKey(), wait_for_verack=False, send_version=False, supports_v2_p2p=True)
        self.wait_until(lambda: peer1.connection_opened)
        self.log.info('Sending remaining ellswift and garbage which are different from V1_PREFIX. Since a response is')
        self.log.info('expected now, our custom data_received() function wouldn\'t result in assertion failure')
        ellswift_and_garbage_data = peer1.v2_state.initiate_v2_handshake()
        peer1.send_raw_message(ellswift_and_garbage_data)
        peer1.wait_for_disconnect(timeout=5)
        self.log.info('successful disconnection when MITM happens in the key exchange phase')


if __name__ == '__main__':
    P2PEarlyKey().main()
