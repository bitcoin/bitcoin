#!/usr/bin/env python3
# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

import random
from enum import Enum

from test_framework.crypto.ellswift import ellswift_create
from test_framework.p2p import P2PInterface
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import random_bitflip
from test_framework.v2_p2p import (
    logger,
    EncryptedP2PState,
    MAX_GARBAGE_LEN,
    TRANSPORT_VERSION,
)


class TestType(Enum):
    """ Scenarios to be tested:

    1. EARLY_KEY_RESPONSE - The responder needs to wait until one byte is received which does not match the 16 bytes
    consisting of network magic followed by "version\x00\x00\x00\x00\x00" before sending out its ellswift + garbage bytes
    2. EXCESS_GARBAGE - Disconnection happens when > MAX_GARBAGE_LEN bytes garbage is sent
    3. WRONG_GARBAGE_TERMINATOR - Disconnection happens when incorrect garbage terminator is sent
    """
    EARLY_KEY_RESPONSE = 0
    EXCESS_GARBAGE = 1
    WRONG_GARBAGE_TERMINATOR = 2


class TestEncryptedP2PState(EncryptedP2PState):
    """ Modify v2 P2P protocol functions for testing scenarios listed in `TestType`"""
    def __init__(self, test_type):
        super().__init__(initiating=True, net='regtest')
        self.test_type = test_type
        if test_type == TestType.EARLY_KEY_RESPONSE:
            self.magic_sent = False  # set to True after first 4 bytes of ellswift which match network magic is sent.
            self.can_data_be_received = False  # variable used to assert if data is received on recvbuf.

    def generate_keypair_and_garbage(self):
        """Generate > MAX_GARBAGE_LEN garbage bytes, MAX_GARBAGE_LEN//2 garbage bytes
        when TestType = (EXCESS_GARBAGE, WRONG_GARBAGE_TERMINATOR)"""
        self.privkey_ours, self.ellswift_ours = ellswift_create()

        if self.test_type == TestType.EXCESS_GARBAGE:
            # send > 4095 bytes garbage
            garbage_len = MAX_GARBAGE_LEN + random.randrange(1, 10)
        elif self.test_type == TestType.WRONG_GARBAGE_TERMINATOR:
            garbage_len = random.randrange(MAX_GARBAGE_LEN//2)
        else:
            garbage_len = random.randrange(MAX_GARBAGE_LEN + 1)

        self.sent_garbage = random.randbytes(garbage_len)
        logger.debug(f"sending {garbage_len} bytes of garbage data")
        return self.ellswift_ours + self.sent_garbage

    def initiate_v2_handshake(self):
        """Send ellswift and garbage bytes in 2 parts when TestType = (EARLY_KEY_RESPONSE)"""
        if self.test_type == TestType.EARLY_KEY_RESPONSE:
            # Here, the 64 bytes ellswift is assumed to have it's first 4 bytes match network magic bytes.
            # It is sent in 2 phases:
            # 1. when `magic_sent` = False, send first 4 bytes of ellswift (matches network magic bytes)
            # 2. when `magic_sent` = True, send remaining 60 bytes of ellswift
            if not self.magic_sent:
                self.generate_keypair_and_garbage()
                self.magic_sent = True
                return b"\xfa\xbf\xb5\xda"
            else:
                # `can_data_be_received` is a variable used to assert if data is received on recvbuf.
                # 1. v2 TestNode shouldn't respond back if we send V1_PREFIX and data shouldn't be received on recvbuf.
                # This state is represented using `can_data_be_received` = False.
                # 2. v2 TestNode responds back when mismatch from V1_PREFIX happens and data can be received on recvbuf.
                # This state is represented using `can_data_be_received` = True.
                self.can_data_be_received = True
                return self.ellswift_ours[4:] + self.sent_garbage
        else:
            return super().initiate_v2_handshake()

    def complete_handshake(self, response):
        """Add option for sending wrong garbage terminator
        when TestType = (WRONG_GARBAGE_TERMINATOR)"""
        ellswift_theirs = self.received_prefix + response.read(64 - len(self.received_prefix))
        # return b"" if we need to receive more bytes
        if len(ellswift_theirs) != 64:
            return 0, b""
        ecdh_secret = self.v2_ecdh(self.privkey_ours, ellswift_theirs, self.ellswift_ours, self.initiating)
        self.initialize_v2_transport(ecdh_secret)
        # Send garbage terminator
        msg_to_send = self.peer['send_garbage_terminator']
        aad = self.sent_garbage

        if self.test_type == TestType.WRONG_GARBAGE_TERMINATOR:
            msg_to_send = random_bitflip(msg_to_send)

        # Optionally send decoy packets after garbage terminator.
        for decoy_content_len in [random.randint(1, 100) for _ in range(random.randint(0, 10))]:
            msg_to_send += self.v2_enc_packet(decoy_content_len * b'\x00', aad=aad, ignore=True)
            aad = b''
        # Send version packet.
        msg_to_send += self.v2_enc_packet(TRANSPORT_VERSION, aad=aad)
        return 64 - len(self.received_prefix), msg_to_send


class MisbehavingV2Peer(P2PInterface):
    """Custom implementation of P2PInterface which uses modified v2 P2P protocol functions for testing purposes."""
    def __init__(self, test_type):
        super().__init__()
        self.v2_state = None
        self.connection_opened = False
        self.test_type = test_type

    def connection_made(self, transport):
        """Only first 4 ellswift bytes which match network magic bytes is sent using `initial_v2_handshake()` in
        `connection_made()` when TestType = (EARLY_KEY_RESPONSE)"""
        self.v2_state = TestEncryptedP2PState(self.test_type)
        super().connection_made(transport)

    def data_received(self, t):
        if self.test_type == TestType.EARLY_KEY_RESPONSE:
            # check that data can be received on recvbuf only when mismatch from V1_PREFIX happens (magic_sent = True)
            assert self.v2_state.can_data_be_received and self.v2_state.magic_sent
        else:
            super().data_received(t)

    def on_open(self):
        self.connection_opened = True


class EncryptedP2PMisbehaving(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-v2transport=1", "-peertimeout=3"]]

    def run_test(self):
        self.test_earlykeyresponse()
        self.test_v2disconnection()

    def test_earlykeyresponse(self):
        self.log.info('Sending ellswift bytes in parts to ensure that response from responder is received only when')
        self.log.info('ellswift bytes have a mismatch from the 16 bytes(network magic followed by "version\\x00\\x00\\x00\\x00\\x00")')
        node0 = self.nodes[0]
        self.log.info('Sending first 4 bytes of ellswift which match network magic')
        self.log.info('If a response is received, assertion failure would happen in our custom data_received() function')
        # send happens in `initiate_v2_handshake()` in `connection_made()`
        peer1 = node0.add_p2p_connection(MisbehavingV2Peer(TestType.EARLY_KEY_RESPONSE), wait_for_verack=False, send_version=False, supports_v2_p2p=True, wait_for_v2_handshake=False)
        self.wait_until(lambda: peer1.connection_opened)
        self.log.info('Sending remaining ellswift and garbage which are different from V1_PREFIX. Since a response is')
        self.log.info('expected now, our custom data_received() function wouldn\'t result in assertion failure')
        ellswift_and_garbage_data = peer1.v2_state.initiate_v2_handshake()
        peer1.send_raw_message(ellswift_and_garbage_data)
        with node0.assert_debug_log(['version handshake timeout peer=0']):
            peer1.wait_for_disconnect(timeout=5)
        self.log.info('successful disconnection since modified ellswift was sent as response')

    def test_v2disconnection(self):
        # test v2 disconnection scenarios
        node0 = self.nodes[0]
        expected_debug_message = [
            [],  # EARLY_KEY_RESPONSE
            ["V2 transport error: missing garbage terminator, peer=1"],  # EXCESS_GARBAGE
            ["version handshake timeout peer=2"],  # WRONG_GARBAGE_TERMINATOR
        ]
        for test_type in TestType:
            if test_type == TestType.EARLY_KEY_RESPONSE:
                continue
            with node0.assert_debug_log(expected_debug_message[test_type.value], timeout=5):
                peer = node0.add_p2p_connection(MisbehavingV2Peer(test_type), wait_for_verack=False, send_version=False, supports_v2_p2p=True, expect_success=False)
                peer.wait_for_disconnect()
            self.log.info(f"Expected disconnection for {test_type.name}")


if __name__ == '__main__':
    EncryptedP2PMisbehaving().main()
