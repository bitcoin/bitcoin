# Copyright (c) 2019-2020 Pieter Wuille
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test-only secp256k1 elliptic curve protocols implementation

WARNING: This code is slow, uses bad randomness, does not properly protect
keys, and is trivially vulnerable to side channel attacks. Do not use for
anything but tests."""
import csv
import hashlib
import hmac
import os
import random
import unittest

from test_framework.crypto import secp256k1
from test_framework.util import random_bitflip

# Point with no known discrete log.
H_POINT = "50929b74c1a04954b78b4b6035e97a5e078a5a0f28ec96d547bfee9ace803ac0"

# Order of the secp256k1 curve
ORDER = secp256k1.GE.ORDER

def TaggedHash(tag, data):
    ss = hashlib.sha256(tag.encode('utf-8')).digest()
    ss += ss
    ss += data
    return hashlib.sha256(ss).digest()


class ECPubKey:
    """A secp256k1 public key"""

    def __init__(self):
        """Construct an uninitialized public key"""
        self.p = None

    def set(self, data):
        """Construct a public key from a serialization in compressed or uncompressed format"""
        self.p = secp256k1.GE.from_bytes(data)
        self.compressed = len(data) == 33

    @property
    def is_compressed(self):
        return self.compressed

    @property
    def is_valid(self):
        return self.p is not None

    def get_bytes(self):
        assert self.is_valid
        if self.compressed:
            return self.p.to_bytes_compressed()
        else:
            return self.p.to_bytes_uncompressed()

    def verify_ecdsa(self, sig, msg, low_s=True):
        """Verify a strictly DER-encoded ECDSA signature against this pubkey.

        See https://en.wikipedia.org/wiki/Elliptic_Curve_Digital_Signature_Algorithm for the
        ECDSA verifier algorithm"""
        assert self.is_valid

        # Extract r and s from the DER formatted signature. Return false for
        # any DER encoding errors.
        if (sig[1] + 2 != len(sig)):
            return False
        if (len(sig) < 4):
            return False
        if (sig[0] != 0x30):
            return False
        if (sig[2] != 0x02):
            return False
        rlen = sig[3]
        if (len(sig) < 6 + rlen):
            return False
        if rlen < 1 or rlen > 33:
            return False
        if sig[4] >= 0x80:
            return False
        if (rlen > 1 and (sig[4] == 0) and not (sig[5] & 0x80)):
            return False
        r = int.from_bytes(sig[4:4+rlen], 'big')
        if (sig[4+rlen] != 0x02):
            return False
        slen = sig[5+rlen]
        if slen < 1 or slen > 33:
            return False
        if (len(sig) != 6 + rlen + slen):
            return False
        if sig[6+rlen] >= 0x80:
            return False
        if (slen > 1 and (sig[6+rlen] == 0) and not (sig[7+rlen] & 0x80)):
            return False
        s = int.from_bytes(sig[6+rlen:6+rlen+slen], 'big')

        # Verify that r and s are within the group order
        if r < 1 or s < 1 or r >= ORDER or s >= ORDER:
            return False
        if low_s and s >= secp256k1.GE.ORDER_HALF:
            return False
        z = int.from_bytes(msg, 'big')

        # Run verifier algorithm on r, s
        w = pow(s, -1, ORDER)
        R = secp256k1.GE.mul((z * w, secp256k1.G), (r * w, self.p))
        if R.infinity or (int(R.x) % ORDER) != r:
            return False
        return True

def generate_privkey():
    """Generate a valid random 32-byte private key."""
    return random.randrange(1, ORDER).to_bytes(32, 'big')

def rfc6979_nonce(key):
    """Compute signing nonce using RFC6979."""
    v = bytes([1] * 32)
    k = bytes([0] * 32)
    k = hmac.new(k, v + b"\x00" + key, 'sha256').digest()
    v = hmac.new(k, v, 'sha256').digest()
    k = hmac.new(k, v + b"\x01" + key, 'sha256').digest()
    v = hmac.new(k, v, 'sha256').digest()
    return hmac.new(k, v, 'sha256').digest()

class ECKey:
    """A secp256k1 private key"""

    def __init__(self):
        self.valid = False

    def set(self, secret, compressed):
        """Construct a private key object with given 32-byte secret and compressed flag."""
        assert len(secret) == 32
        secret = int.from_bytes(secret, 'big')
        self.valid = (secret > 0 and secret < ORDER)
        if self.valid:
            self.secret = secret
            self.compressed = compressed

    def generate(self, compressed=True):
        """Generate a random private key (compressed or uncompressed)."""
        self.set(generate_privkey(), compressed)

    def get_bytes(self):
        """Retrieve the 32-byte representation of this key."""
        assert self.valid
        return self.secret.to_bytes(32, 'big')

    @property
    def is_valid(self):
        return self.valid

    @property
    def is_compressed(self):
        return self.compressed

    def get_pubkey(self):
        """Compute an ECPubKey object for this secret key."""
        assert self.valid
        ret = ECPubKey()
        ret.p = self.secret * secp256k1.G
        ret.compressed = self.compressed
        return ret

    def sign_ecdsa(self, msg, low_s=True, rfc6979=False):
        """Construct a DER-encoded ECDSA signature with this key.

        See https://en.wikipedia.org/wiki/Elliptic_Curve_Digital_Signature_Algorithm for the
        ECDSA signer algorithm."""
        assert self.valid
        z = int.from_bytes(msg, 'big')
        # Note: no RFC6979 by default, but a simple random nonce (some tests rely on distinct transactions for the same operation)
        if rfc6979:
            k = int.from_bytes(rfc6979_nonce(self.secret.to_bytes(32, 'big') + msg), 'big')
        else:
            k = random.randrange(1, ORDER)
        R = k * secp256k1.G
        r = int(R.x) % ORDER
        s = (pow(k, -1, ORDER) * (z + self.secret * r)) % ORDER
        if low_s and s > secp256k1.GE.ORDER_HALF:
            s = ORDER - s
        # Represent in DER format. The byte representations of r and s have
        # length rounded up (255 bits becomes 32 bytes and 256 bits becomes 33
        # bytes).
        rb = r.to_bytes((r.bit_length() + 8) // 8, 'big')
        sb = s.to_bytes((s.bit_length() + 8) // 8, 'big')
        return b'\x30' + bytes([4 + len(rb) + len(sb), 2, len(rb)]) + rb + bytes([2, len(sb)]) + sb

def compute_xonly_pubkey(key):
    """Compute an x-only (32 byte) public key from a (32 byte) private key.

    This also returns whether the resulting public key was negated.
    """

    assert len(key) == 32
    x = int.from_bytes(key, 'big')
    if x == 0 or x >= ORDER:
        return (None, None)
    P = x * secp256k1.G
    return (P.to_bytes_xonly(), not P.y.is_even())

def tweak_add_privkey(key, tweak):
    """Tweak a private key (after negating it if needed)."""

    assert len(key) == 32
    assert len(tweak) == 32

    x = int.from_bytes(key, 'big')
    if x == 0 or x >= ORDER:
        return None
    if not (x * secp256k1.G).y.is_even():
       x = ORDER - x
    t = int.from_bytes(tweak, 'big')
    if t >= ORDER:
        return None
    x = (x + t) % ORDER
    if x == 0:
        return None
    return x.to_bytes(32, 'big')

def tweak_add_pubkey(key, tweak):
    """Tweak a public key and return whether the result had to be negated."""

    assert len(key) == 32
    assert len(tweak) == 32

    P = secp256k1.GE.from_bytes_xonly(key)
    if P is None:
        return None
    t = int.from_bytes(tweak, 'big')
    if t >= ORDER:
        return None
    Q = t * secp256k1.G + P
    if Q.infinity:
        return None
    return (Q.to_bytes_xonly(), not Q.y.is_even())

def verify_schnorr(key, sig, msg):
    """Verify a Schnorr signature (see BIP 340).

    - key is a 32-byte xonly pubkey (computed using compute_xonly_pubkey).
    - sig is a 64-byte Schnorr signature
    - msg is a 32-byte message
    """
    assert len(key) == 32
    assert len(msg) == 32
    assert len(sig) == 64

    P = secp256k1.GE.from_bytes_xonly(key)
    if P is None:
        return False
    r = int.from_bytes(sig[0:32], 'big')
    if r >= secp256k1.FE.SIZE:
        return False
    s = int.from_bytes(sig[32:64], 'big')
    if s >= ORDER:
        return False
    e = int.from_bytes(TaggedHash("BIP0340/challenge", sig[0:32] + key + msg), 'big') % ORDER
    R = secp256k1.GE.mul((s, secp256k1.G), (-e, P))
    if R.infinity or not R.y.is_even():
        return False
    if r != R.x:
        return False
    return True

def sign_schnorr(key, msg, aux=None, flip_p=False, flip_r=False):
    """Create a Schnorr signature (see BIP 340)."""

    if aux is None:
        aux = bytes(32)

    assert len(key) == 32
    assert len(msg) == 32
    assert len(aux) == 32

    sec = int.from_bytes(key, 'big')
    if sec == 0 or sec >= ORDER:
        return None
    P = sec * secp256k1.G
    if P.y.is_even() == flip_p:
        sec = ORDER - sec
    t = (sec ^ int.from_bytes(TaggedHash("BIP0340/aux", aux), 'big')).to_bytes(32, 'big')
    kp = int.from_bytes(TaggedHash("BIP0340/nonce", t + P.to_bytes_xonly() + msg), 'big') % ORDER
    assert kp != 0
    R = kp * secp256k1.G
    k = kp if R.y.is_even() != flip_r else ORDER - kp
    e = int.from_bytes(TaggedHash("BIP0340/challenge", R.to_bytes_xonly() + P.to_bytes_xonly() + msg), 'big') % ORDER
    return R.to_bytes_xonly() + ((k + e * sec) % ORDER).to_bytes(32, 'big')


class TestFrameworkKey(unittest.TestCase):
    def test_ecdsa_and_schnorr(self):
        """Test the Python ECDSA and Schnorr implementations."""
        byte_arrays = [generate_privkey() for _ in range(3)] + [v.to_bytes(32, 'big') for v in [0, ORDER - 1, ORDER, 2**256 - 1]]
        keys = {}
        for privkey_bytes in byte_arrays:  # build array of key/pubkey pairs
            privkey = ECKey()
            privkey.set(privkey_bytes, compressed=True)
            if privkey.is_valid:
                keys[privkey] = privkey.get_pubkey()
        for msg in byte_arrays:  # test every combination of message, signing key, verification key
            for sign_privkey, _ in keys.items():
                sig_ecdsa = sign_privkey.sign_ecdsa(msg)
                sig_schnorr = sign_schnorr(sign_privkey.get_bytes(), msg)
                for verify_privkey, verify_pubkey in keys.items():
                    verify_xonly_pubkey = verify_pubkey.get_bytes()[1:]
                    if verify_privkey == sign_privkey:
                        self.assertTrue(verify_pubkey.verify_ecdsa(sig_ecdsa, msg))
                        self.assertTrue(verify_schnorr(verify_xonly_pubkey, sig_schnorr, msg))
                        sig_ecdsa = random_bitflip(sig_ecdsa)  # damaging signature should break things
                        sig_schnorr = random_bitflip(sig_schnorr)
                    self.assertFalse(verify_pubkey.verify_ecdsa(sig_ecdsa, msg))
                    self.assertFalse(verify_schnorr(verify_xonly_pubkey, sig_schnorr, msg))

    def test_schnorr_testvectors(self):
        """Implement the BIP340 test vectors (read from bip340_test_vectors.csv)."""
        num_tests = 0
        vectors_file = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'bip340_test_vectors.csv')
        with open(vectors_file, newline='', encoding='utf8') as csvfile:
            reader = csv.reader(csvfile)
            next(reader)
            for row in reader:
                (i_str, seckey_hex, pubkey_hex, aux_rand_hex, msg_hex, sig_hex, result_str, comment) = row
                i = int(i_str)
                pubkey = bytes.fromhex(pubkey_hex)
                msg = bytes.fromhex(msg_hex)
                sig = bytes.fromhex(sig_hex)
                result = result_str == 'TRUE'
                if seckey_hex != '':
                    seckey = bytes.fromhex(seckey_hex)
                    pubkey_actual = compute_xonly_pubkey(seckey)[0]
                    self.assertEqual(pubkey.hex(), pubkey_actual.hex(), "BIP340 test vector %i (%s): pubkey mismatch" % (i, comment))
                    aux_rand = bytes.fromhex(aux_rand_hex)
                    try:
                        sig_actual = sign_schnorr(seckey, msg, aux_rand)
                        self.assertEqual(sig.hex(), sig_actual.hex(), "BIP340 test vector %i (%s): sig mismatch" % (i, comment))
                    except RuntimeError as e:
                        self.fail("BIP340 test vector %i (%s): signing raised exception %s" % (i, comment, e))
                result_actual = verify_schnorr(pubkey, sig, msg)
                if result:
                    self.assertEqual(result, result_actual, "BIP340 test vector %i (%s): verification failed" % (i, comment))
                else:
                    self.assertEqual(result, result_actual, "BIP340 test vector %i (%s): verification succeeded unexpectedly" % (i, comment))
                num_tests += 1
        self.assertTrue(num_tests >= 15) # expect at least 15 test vectors
