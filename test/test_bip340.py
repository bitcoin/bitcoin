import csv
from pathlib import Path
from random import randbytes
import unittest

from secp256k1lab.bip340 import pubkey_gen, schnorr_sign, schnorr_verify


class BIP340Tests(unittest.TestCase):
    """Test schnorr signatures (BIP 340)."""

    def test_correctness(self):
        seckey = randbytes(32)
        pubkey_xonly = pubkey_gen(seckey)
        aux_rand = randbytes(32)
        message = b'this is some arbitrary message'
        signature = schnorr_sign(message, seckey, aux_rand)
        success = schnorr_verify(message, pubkey_xonly, signature)
        self.assertTrue(success)

    def test_vectors(self):
        # Test against vectors from the BIPs repository
        # [https://github.com/bitcoin/bips/blob/master/bip-0340/test-vectors.csv]
        vectors_file = Path(__file__).parent / "vectors" / "bip340.csv"
        with open(vectors_file, encoding='utf8') as csvfile:
            reader = csv.DictReader(csvfile)
            for row in reader:
                with self.subTest(i=int(row['index'])):
                    self.subtest_vectors_case(row)

    def subtest_vectors_case(self, row):
        seckey = bytes.fromhex(row['secret key'])
        pubkey_xonly = bytes.fromhex(row['public key'])
        aux_rand = bytes.fromhex(row['aux_rand'])
        msg = bytes.fromhex(row['message'])
        sig = bytes.fromhex(row['signature'])
        result_str = row['verification result']
        comment = row['comment']

        result = result_str == 'TRUE'
        assert result or result_str == 'FALSE'
        if seckey != b'':
            pubkey_xonly_actual = pubkey_gen(seckey)
            self.assertEqual(pubkey_xonly.hex(), pubkey_xonly_actual.hex(), f"BIP340 test vector ({comment}): pubkey mismatch")
            sig_actual = schnorr_sign(msg, seckey, aux_rand)
            self.assertEqual(sig.hex(), sig_actual.hex(), f"BIP340 test vector ({comment}): sig mismatch")
        result_actual = schnorr_verify(msg, pubkey_xonly, sig)
        if result:
            self.assertEqual(result, result_actual, f"BIP340 test vector ({comment}): verification failed unexpectedly")
        else:
            self.assertEqual(result, result_actual, f"BIP340 test vector ({comment}): verification succeeded unexpectedly")
