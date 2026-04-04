from random import randbytes
import unittest

from secp256k1lab.ecdh import ecdh_libsecp256k1
from secp256k1lab.keys import pubkey_gen_plain


class ECDHTests(unittest.TestCase):
    """Test ECDH module."""

    def test_correctness(self):
        seckey_alice = randbytes(32)
        pubkey_alice = pubkey_gen_plain(seckey_alice)
        seckey_bob = randbytes(32)
        pubkey_bob = pubkey_gen_plain(seckey_bob)
        shared_secret1 = ecdh_libsecp256k1(seckey_alice, pubkey_bob)
        shared_secret2 = ecdh_libsecp256k1(seckey_bob, pubkey_alice)
        self.assertEqual(shared_secret1, shared_secret2)
