#!/usr/bin/env python3
# Copyright (c) 2022 The Tortoisecoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Class for v2 P2P protocol (see BIP 324)"""

import random

from .crypto.bip324_cipher import FSChaCha20Poly1305
from .crypto.chacha20 import FSChaCha20
from .crypto.ellswift import ellswift_create, ellswift_ecdh_xonly
from .crypto.hkdf import hkdf_sha256
from .key import TaggedHash
from .messages import MAGIC_BYTES


CHACHA20POLY1305_EXPANSION = 16
HEADER_LEN = 1
IGNORE_BIT_POS = 7
LENGTH_FIELD_LEN = 3
MAX_GARBAGE_LEN = 4095

SHORTID = {
    1: b"addr",
    2: b"block",
    3: b"blocktxn",
    4: b"cmpctblock",
    5: b"feefilter",
    6: b"filteradd",
    7: b"filterclear",
    8: b"filterload",
    9: b"getblocks",
    10: b"getblocktxn",
    11: b"getdata",
    12: b"getheaders",
    13: b"headers",
    14: b"inv",
    15: b"mempool",
    16: b"merkleblock",
    17: b"notfound",
    18: b"ping",
    19: b"pong",
    20: b"sendcmpct",
    21: b"tx",
    22: b"getcfilters",
    23: b"cfilter",
    24: b"getcfheaders",
    25: b"cfheaders",
    26: b"getcfcheckpt",
    27: b"cfcheckpt",
    28: b"addrv2",
}

# Dictionary which contains short message type ID for the P2P message
MSGTYPE_TO_SHORTID = {msgtype: shortid for shortid, msgtype in SHORTID.items()}


class EncryptedP2PState:
    """A class for managing the state when v2 P2P protocol is used. Performs initial v2 handshake and encrypts/decrypts
    P2P messages. P2PConnection uses an object of this class.


    Args:
        initiating (bool): defines whether the P2PConnection is an initiator or responder.
            - initiating = True for inbound connections in the test framework   [TestNode <------- P2PConnection]
            - initiating = False for outbound connections in the test framework [TestNode -------> P2PConnection]

        net (string): chain used (regtest, signet etc..)

    Methods:
        perform an advanced form of diffie-hellman handshake to instantiate the encrypted transport. before exchanging
        any P2P messages, 2 nodes perform this handshake in order to determine a shared secret that is unique to both
        of them and use it to derive keys to encrypt/decrypt P2P messages.
            - initial v2 handshakes is performed by: (see BIP324 section #overall-handshake-pseudocode)
                1. initiator using initiate_v2_handshake(), complete_handshake() and authenticate_handshake()
                2. responder using respond_v2_handshake(), complete_handshake() and authenticate_handshake()
            - initialize_v2_transport() sets various BIP324 derived keys and ciphers.

        encrypt/decrypt v2 P2P messages using v2_enc_packet() and v2_receive_packet().
    """
    def __init__(self, *, initiating, net):
        self.initiating = initiating  # True if initiator
        self.net = net
        self.peer = {}  # object with various BIP324 derived keys and ciphers
        self.privkey_ours = None
        self.ellswift_ours = None
        self.sent_garbage = b""
        self.received_garbage = b""
        self.received_prefix = b""  # received ellswift bytes till the first mismatch from 16 bytes v1_prefix
        self.tried_v2_handshake = False  # True when the initial handshake is over
        # stores length of packet contents to detect whether first 3 bytes (which contains length of packet contents)
        # has been decrypted. set to -1 if decryption hasn't been done yet.
        self.contents_len = -1
        self.found_garbage_terminator = False
        self.transport_version = b''

    @staticmethod
    def v2_ecdh(priv, ellswift_theirs, ellswift_ours, initiating):
        """Compute BIP324 shared secret.

        Returns:
        bytes - BIP324 shared secret
        """
        ecdh_point_x32 = ellswift_ecdh_xonly(ellswift_theirs, priv)
        if initiating:
            # Initiating, place our public key encoding first.
            return TaggedHash("bip324_ellswift_xonly_ecdh", ellswift_ours + ellswift_theirs + ecdh_point_x32)
        else:
            # Responding, place their public key encoding first.
            return TaggedHash("bip324_ellswift_xonly_ecdh", ellswift_theirs + ellswift_ours + ecdh_point_x32)

    def generate_keypair_and_garbage(self, garbage_len=None):
        """Generates ellswift keypair and 4095 bytes garbage at max"""
        self.privkey_ours, self.ellswift_ours = ellswift_create()
        if garbage_len is None:
            garbage_len = random.randrange(MAX_GARBAGE_LEN + 1)
        self.sent_garbage = random.randbytes(garbage_len)
        return self.ellswift_ours + self.sent_garbage

    def initiate_v2_handshake(self):
        """Initiator begins the v2 handshake by sending its ellswift bytes and garbage

        Returns:
        bytes - bytes to be sent to the peer when starting the v2 handshake as an initiator
        """
        return self.generate_keypair_and_garbage()

    def respond_v2_handshake(self, response):
        """Responder begins the v2 handshake by sending its ellswift bytes and garbage. However, the responder
        sends this after having received at least one byte that mismatches 16-byte v1_prefix.

        Returns:
        1. int - length of bytes that were consumed so that recvbuf can be updated
        2. bytes - bytes to be sent to the peer when starting the v2 handshake as a responder.
                 - returns b"" if more bytes need to be received before we can respond and start the v2 handshake.
                 - returns -1 to downgrade the connection to v1 P2P.
        """
        v1_prefix = MAGIC_BYTES[self.net] + b'version\x00\x00\x00\x00\x00'
        while len(self.received_prefix) < 16:
            byte = response.read(1)
            # return b"" if we need to receive more bytes
            if not byte:
                return len(self.received_prefix), b""
            self.received_prefix += byte
            if self.received_prefix[-1] != v1_prefix[len(self.received_prefix) - 1]:
                return len(self.received_prefix), self.generate_keypair_and_garbage()
        # return -1 to decide v1 only after all 16 bytes processed
        return len(self.received_prefix), -1

    def complete_handshake(self, response):
        """ Instantiates the encrypted transport and
        sends garbage terminator + optional decoy packets + transport version packet.
        Done by both initiator and responder.

        Returns:
        1. int - length of bytes that were consumed. returns 0 if all 64 bytes from ellswift haven't been received yet.
        2. bytes - bytes to be sent to the peer when completing the v2 handshake
        """
        ellswift_theirs = self.received_prefix + response.read(64 - len(self.received_prefix))
        # return b"" if we need to receive more bytes
        if len(ellswift_theirs) != 64:
            return 0, b""
        ecdh_secret = self.v2_ecdh(self.privkey_ours, ellswift_theirs, self.ellswift_ours, self.initiating)
        self.initialize_v2_transport(ecdh_secret)
        # Send garbage terminator
        msg_to_send = self.peer['send_garbage_terminator']
        # Optionally send decoy packets after garbage terminator.
        aad = self.sent_garbage
        for decoy_content_len in [random.randint(1, 100) for _ in range(random.randint(0, 10))]:
            msg_to_send += self.v2_enc_packet(decoy_content_len * b'\x00', aad=aad, ignore=True)
            aad = b''
        # Send version packet.
        msg_to_send += self.v2_enc_packet(self.transport_version, aad=aad)
        return 64 - len(self.received_prefix), msg_to_send

    def authenticate_handshake(self, response):
        """ Ensures that the received optional decoy packets and transport version packet are authenticated.
        Marks the v2 handshake as complete. Done by both initiator and responder.

        Returns:
        1. int - length of bytes that were processed so that recvbuf can be updated
        2. bool - True if the authentication was successful/more bytes need to be received and False otherwise
        """
        processed_length = 0

        # Detect garbage terminator in the received bytes
        if not self.found_garbage_terminator:
            received_garbage = response[:16]
            response = response[16:]
            processed_length = len(received_garbage)
            for i in range(MAX_GARBAGE_LEN + 1):
                if received_garbage[-16:] == self.peer['recv_garbage_terminator']:
                    # Receive, decode, and ignore version packet.
                    # This includes skipping decoys and authenticating the received garbage.
                    self.found_garbage_terminator = True
                    self.received_garbage = received_garbage[:-16]
                    break
                else:
                    # don't update recvbuf since more bytes need to be received
                    if len(response) == 0:
                        return 0, True
                    received_garbage += response[:1]
                    processed_length += 1
                    response = response[1:]
            else:
                # disconnect since garbage terminator was not seen after 4 KiB of garbage.
                return processed_length, False

        # Process optional decoy packets and transport version packet
        while not self.tried_v2_handshake:
            length, contents = self.v2_receive_packet(response, aad=self.received_garbage)
            if length == -1:
                return processed_length, False
            elif length == 0:
                return processed_length, True
            processed_length += length
            self.received_garbage = b""
            # decoy packets have contents = None. v2 handshake is complete only when version packet
            # (can be empty with contents = b"") with contents != None is received.
            if contents is not None:
                assert contents == b""  # currently TestNode sends an empty version packet
                self.tried_v2_handshake = True
                return processed_length, True
            response = response[length:]

    def initialize_v2_transport(self, ecdh_secret):
        """Sets the peer object with various BIP324 derived keys and ciphers."""
        peer = {}
        salt = b'tortoisecoin_v2_shared_secret' + MAGIC_BYTES[self.net]
        for name in ('initiator_L', 'initiator_P', 'responder_L', 'responder_P', 'garbage_terminators', 'session_id'):
            peer[name] = hkdf_sha256(salt=salt, ikm=ecdh_secret, info=name.encode('utf-8'), length=32)
        if self.initiating:
            self.peer['send_L'] = FSChaCha20(peer['initiator_L'])
            self.peer['send_P'] = FSChaCha20Poly1305(peer['initiator_P'])
            self.peer['send_garbage_terminator'] = peer['garbage_terminators'][:16]
            self.peer['recv_L'] = FSChaCha20(peer['responder_L'])
            self.peer['recv_P'] = FSChaCha20Poly1305(peer['responder_P'])
            self.peer['recv_garbage_terminator'] = peer['garbage_terminators'][16:]
        else:
            self.peer['send_L'] = FSChaCha20(peer['responder_L'])
            self.peer['send_P'] = FSChaCha20Poly1305(peer['responder_P'])
            self.peer['send_garbage_terminator'] = peer['garbage_terminators'][16:]
            self.peer['recv_L'] = FSChaCha20(peer['initiator_L'])
            self.peer['recv_P'] = FSChaCha20Poly1305(peer['initiator_P'])
            self.peer['recv_garbage_terminator'] = peer['garbage_terminators'][:16]
        self.peer['session_id'] = peer['session_id']

    def v2_enc_packet(self, contents, aad=b'', ignore=False):
        """Encrypt a BIP324 packet.

        Returns:
        bytes - encrypted packet contents
        """
        assert len(contents) <= 2**24 - 1
        header = (ignore << IGNORE_BIT_POS).to_bytes(HEADER_LEN, 'little')
        plaintext = header + contents
        aead_ciphertext = self.peer['send_P'].encrypt(aad, plaintext)
        enc_plaintext_len = self.peer['send_L'].crypt(len(contents).to_bytes(LENGTH_FIELD_LEN, 'little'))
        return enc_plaintext_len + aead_ciphertext

    def v2_receive_packet(self, response, aad=b''):
        """Decrypt a BIP324 packet

        Returns:
        1. int - number of bytes consumed (or -1 if error)
        2. bytes - contents of decrypted non-decoy packet if any (or None otherwise)
        """
        if self.contents_len == -1:
            if len(response) < LENGTH_FIELD_LEN:
                return 0, None
            enc_contents_len = response[:LENGTH_FIELD_LEN]
            self.contents_len = int.from_bytes(self.peer['recv_L'].crypt(enc_contents_len), 'little')
        response = response[LENGTH_FIELD_LEN:]
        if len(response) < HEADER_LEN + self.contents_len + CHACHA20POLY1305_EXPANSION:
            return 0, None
        aead_ciphertext = response[:HEADER_LEN + self.contents_len + CHACHA20POLY1305_EXPANSION]
        plaintext = self.peer['recv_P'].decrypt(aad, aead_ciphertext)
        if plaintext is None:
            return -1, None  # disconnect
        header = plaintext[:HEADER_LEN]
        length = LENGTH_FIELD_LEN + HEADER_LEN + self.contents_len + CHACHA20POLY1305_EXPANSION
        self.contents_len = -1
        return length, None if (header[0] & (1 << IGNORE_BIT_POS)) else plaintext[HEADER_LEN:]
