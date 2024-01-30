#!/usr/bin/env python3
# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

from enum import Enum

from test_framework.messages import MAGIC_BYTES
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.v2_p2p import EncryptedP2PState


class TestType(Enum):
    """ Scenarios to be tested:

    1. EARLY_KEY_RESPONSE - The responder needs to wait until one byte is received which does not match the 16 bytes
    consisting of network magic followed by "version\x00\x00\x00\x00\x00" before sending out its ellswift + garbage bytes
    """
    EARLY_KEY_RESPONSE = 0


class EarlyKeyResponseState(EncryptedP2PState):
    """ Modify v2 P2P protocol functions for testing EARLY_KEY_RESPONSE scenario"""
    def __init__(self, initiating, net):
        super().__init__(initiating=initiating, net=net)
        self.can_data_be_received = False  # variable used to assert if data is received on recvbuf.

    def initiate_v2_handshake(self):
        """Send ellswift and garbage bytes in 2 parts when TestType = (EARLY_KEY_RESPONSE)"""
        self.generate_keypair_and_garbage()
        return b""


class MisbehavingV2Peer(P2PInterface):
    """Custom implementation of P2PInterface which uses modified v2 P2P protocol functions for testing purposes."""
    def __init__(self, test_type):
        super().__init__()
        self.test_type = test_type

    def connection_made(self, transport):
        if self.test_type == TestType.EARLY_KEY_RESPONSE:
            self.v2_state = EarlyKeyResponseState(initiating=True, net='regtest')
        super().connection_made(transport)

    def data_received(self, t):
        if self.test_type == TestType.EARLY_KEY_RESPONSE:
            # check that data can be received on recvbuf only when mismatch from V1_PREFIX happens
            assert self.v2_state.can_data_be_received
        else:
            super().data_received(t)


class EncryptedP2PMisbehaving(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-v2transport=1", "-peertimeout=3"]]

    def run_test(self):
        self.test_earlykeyresponse()

    def test_earlykeyresponse(self):
        self.log.info('Sending ellswift bytes in parts to ensure that response from responder is received only when')
        self.log.info('ellswift bytes have a mismatch from the 16 bytes(network magic followed by "version\\x00\\x00\\x00\\x00\\x00")')
        node0 = self.nodes[0]
        self.log.info('Sending first 4 bytes of ellswift which match network magic')
        self.log.info('If a response is received, assertion failure would happen in our custom data_received() function')
        peer1 = node0.add_p2p_connection(MisbehavingV2Peer(TestType.EARLY_KEY_RESPONSE), wait_for_verack=False, send_version=False, supports_v2_p2p=True, wait_for_v2_handshake=False)
        peer1.send_raw_message(MAGIC_BYTES['regtest'])
        self.log.info('Sending remaining ellswift and garbage which are different from V1_PREFIX. Since a response is')
        self.log.info('expected now, our custom data_received() function wouldn\'t result in assertion failure')
        peer1.v2_state.can_data_be_received = True
        peer1.send_raw_message(peer1.v2_state.ellswift_ours[4:] + peer1.v2_state.sent_garbage)
        with node0.assert_debug_log(['V2 handshake timeout peer=0']):
            peer1.wait_for_disconnect(timeout=5)
        self.log.info('successful disconnection since modified ellswift was sent as response')


if __name__ == '__main__':
    EncryptedP2PMisbehaving().main()
