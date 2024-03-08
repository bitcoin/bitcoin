#!/usr/bin/env python3
# Copyright (c) 2022 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Test-only implementation of ChaCha20 Poly1305 AEAD Construction in RFC 8439 and FSChaCha20Poly1305 for BIP 324

It is designed for ease of understanding, not performance.

WARNING: This code is slow and trivially vulnerable to side channel attacks. Do not use for
anything but tests.
"""

import unittest

from .chacha20 import chacha20_block, REKEY_INTERVAL
from .poly1305 import Poly1305


def pad16(x):
    if len(x) % 16 == 0:
        return b''
    return b'\x00' * (16 - (len(x) % 16))


def aead_chacha20_poly1305_encrypt(key, nonce, aad, plaintext):
    """Encrypt a plaintext using ChaCha20Poly1305."""
    if plaintext is None:
        return None
    ret = bytearray()
    msg_len = len(plaintext)
    for i in range((msg_len + 63) // 64):
        now = min(64, msg_len - 64 * i)
        keystream = chacha20_block(key, nonce, i + 1)
        for j in range(now):
            ret.append(plaintext[j + 64 * i] ^ keystream[j])
    poly1305 = Poly1305(chacha20_block(key, nonce, 0)[:32])
    mac_data = aad + pad16(aad)
    mac_data += ret + pad16(ret)
    mac_data += len(aad).to_bytes(8, 'little') + msg_len.to_bytes(8, 'little')
    ret += poly1305.tag(mac_data)
    return bytes(ret)


def aead_chacha20_poly1305_decrypt(key, nonce, aad, ciphertext):
    """Decrypt a ChaCha20Poly1305 ciphertext."""
    if ciphertext is None or len(ciphertext) < 16:
        return None
    msg_len = len(ciphertext) - 16
    poly1305 = Poly1305(chacha20_block(key, nonce, 0)[:32])
    mac_data = aad + pad16(aad)
    mac_data += ciphertext[:-16] + pad16(ciphertext[:-16])
    mac_data += len(aad).to_bytes(8, 'little') + msg_len.to_bytes(8, 'little')
    if ciphertext[-16:] != poly1305.tag(mac_data):
        return None
    ret = bytearray()
    for i in range((msg_len + 63) // 64):
        now = min(64, msg_len - 64 * i)
        keystream = chacha20_block(key, nonce, i + 1)
        for j in range(now):
            ret.append(ciphertext[j + 64 * i] ^ keystream[j])
    return bytes(ret)


class FSChaCha20Poly1305:
    """Rekeying wrapper AEAD around ChaCha20Poly1305."""
    def __init__(self, initial_key):
        self._key = initial_key
        self._packet_counter = 0

    def _crypt(self, aad, text, is_decrypt):
        nonce = ((self._packet_counter % REKEY_INTERVAL).to_bytes(4, 'little') +
                 (self._packet_counter // REKEY_INTERVAL).to_bytes(8, 'little'))
        if is_decrypt:
            ret = aead_chacha20_poly1305_decrypt(self._key, nonce, aad, text)
        else:
            ret = aead_chacha20_poly1305_encrypt(self._key, nonce, aad, text)
        if (self._packet_counter + 1) % REKEY_INTERVAL == 0:
            rekey_nonce = b"\xFF\xFF\xFF\xFF" + nonce[4:]
            self._key = aead_chacha20_poly1305_encrypt(self._key, rekey_nonce, b"", b"\x00" * 32)[:32]
        self._packet_counter += 1
        return ret

    def decrypt(self, aad, ciphertext):
        return self._crypt(aad, ciphertext, True)

    def encrypt(self, aad, plaintext):
        return self._crypt(aad, plaintext, False)


# Test vectors from RFC8439 consisting of plaintext, aad, 32 byte key, 12 byte nonce and ciphertext
AEAD_TESTS = [
    # RFC 8439 Example from section 2.8.2
    ["4c616469657320616e642047656e746c656d656e206f662074686520636c6173"
     "73206f66202739393a204966204920636f756c64206f6666657220796f75206f"
     "6e6c79206f6e652074697020666f7220746865206675747572652c2073756e73"
     "637265656e20776f756c642062652069742e",
     "50515253c0c1c2c3c4c5c6c7",
     "808182838485868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9f",
     [7, 0x4746454443424140],
     "d31a8d34648e60db7b86afbc53ef7ec2a4aded51296e08fea9e2b5a736ee62d6"
     "3dbea45e8ca9671282fafb69da92728b1a71de0a9e060b2905d6a5b67ecd3b36"
     "92ddbd7f2d778b8c9803aee328091b58fab324e4fad675945585808b4831d7bc"
     "3ff4def08e4b7a9de576d26586cec64b61161ae10b594f09e26a7e902ecbd060"
     "0691"],
    # RFC 8439 Test vector A.5
    ["496e7465726e65742d4472616674732061726520647261667420646f63756d65"
     "6e74732076616c696420666f722061206d6178696d756d206f6620736978206d"
     "6f6e74687320616e64206d617920626520757064617465642c207265706c6163"
     "65642c206f72206f62736f6c65746564206279206f7468657220646f63756d65"
     "6e747320617420616e792074696d652e20497420697320696e617070726f7072"
     "6961746520746f2075736520496e7465726e65742d4472616674732061732072"
     "65666572656e6365206d6174657269616c206f7220746f206369746520746865"
     "6d206f74686572207468616e206173202fe2809c776f726b20696e2070726f67"
     "726573732e2fe2809d",
     "f33388860000000000004e91",
     "1c9240a5eb55d38af333888604f6b5f0473917c1402b80099dca5cbc207075c0",
     [0, 0x0807060504030201],
     "64a0861575861af460f062c79be643bd5e805cfd345cf389f108670ac76c8cb2"
     "4c6cfc18755d43eea09ee94e382d26b0bdb7b73c321b0100d4f03b7f355894cf"
     "332f830e710b97ce98c8a84abd0b948114ad176e008d33bd60f982b1ff37c855"
     "9797a06ef4f0ef61c186324e2b3506383606907b6a7c02b0f9f6157b53c867e4"
     "b9166c767b804d46a59b5216cde7a4e99040c5a40433225ee282a1b0a06c523e"
     "af4534d7f83fa1155b0047718cbc546a0d072b04b3564eea1b422273f548271a"
     "0bb2316053fa76991955ebd63159434ecebb4e466dae5a1073a6727627097a10"
     "49e617d91d361094fa68f0ff77987130305beaba2eda04df997b714d6c6f2c29"
     "a6ad5cb4022b02709beead9d67890cbb22392336fea1851f38"],
    # Test vectors exercising aad and plaintext which are multiples of 16 bytes.
    ["8d2d6a8befd9716fab35819eaac83b33269afb9f1a00fddf66095a6c0cd91951"
     "a6b7ad3db580be0674c3f0b55f618e34",
     "",
     "72ddc73f07101282bbbcf853b9012a9f9695fc5d36b303a97fd0845d0314e0c3",
     [0x3432b75f, 0xb3585537eb7f4024],
     "f760b8224fb2a317b1b07875092606131232a5b86ae142df5df1c846a7f6341a"
     "f2564483dd77f836be45e6230808ffe402a6f0a3e8be074b3d1f4ea8a7b09451"],
    ["",
     "36970d8a704c065de16250c18033de5a400520ac1b5842b24551e5823a3314f3"
     "946285171e04a81ebfbe3566e312e74ab80e94c7dd2ff4e10de0098a58d0f503",
     "77adda51d6730b9ad6c995658cbd49f581b2547e7c0c08fcc24ceec797461021",
     [0x1f90da88, 0x75dafa3ef84471a4],
     "aaae5bb81e8407c94b2ae86ae0c7efbe"],
]

FSAEAD_TESTS = [
    ["d6a4cb04ef0f7c09c1866ed29dc24d820e75b0491032a51b4c3366f9ca35c19e"
     "a3047ec6be9d45f9637b63e1cf9eb4c2523a5aab7b851ebeba87199db0e839cf"
     "0d5c25e50168306377aedbe9089fd2463ded88b83211cf51b73b150608cc7a60"
     "0d0f11b9a742948482e1b109d8faf15b450aa7322e892fa2208c6691e3fecf4c"
     "711191b14d75a72147",
     "786cb9b6ebf44288974cf0",
     "5c9e1c3951a74fba66708bf9d2c217571684556b6a6a3573bff2847d38612654",
     500,
     "9dcebbd3281ea3dd8e9a1ef7d55a97abd6743e56ebc0c190cb2c4e14160b385e"
     "0bf508dddf754bd02c7c208447c131ce23e47a4a14dfaf5dd8bc601323950f75"
     "4e05d46e9232f83fc5120fbbef6f5347a826ec79a93820718d4ec7a2b7cfaaa4"
     "4b21e16d726448b62f803811aff4f6d827ed78e738ce8a507b81a8ae13131192"
     "8039213de18a5120dc9b7370baca878f50ff254418de3da50c"],
    ["8349b7a2690b63d01204800c288ff1138a1d473c832c90ea8b3fc102d0bb3adc"
     "44261b247c7c3d6760bfbe979d061c305f46d94c0582ac3099f0bf249f8cb234",
     "",
     "3bd2093fcbcb0d034d8c569583c5425c1a53171ea299f8cc3bbf9ae3530adfce",
     60000,
     "30a6757ff8439b975363f166a0fa0e36722ab35936abd704297948f45083f4d4"
     "99433137ce931f7fca28a0acd3bc30f57b550acbc21cbd45bbef0739d9caf30c"
     "14b94829deb27f0b1923a2af704ae5d6"],
]


class TestFrameworkAEAD(unittest.TestCase):
    def test_aead(self):
        """ChaCha20Poly1305 AEAD test vectors."""
        for test_vector in AEAD_TESTS:
            hex_plain, hex_aad, hex_key, hex_nonce, hex_cipher = test_vector
            plain = bytes.fromhex(hex_plain)
            aad = bytes.fromhex(hex_aad)
            key = bytes.fromhex(hex_key)
            nonce = hex_nonce[0].to_bytes(4, 'little') + hex_nonce[1].to_bytes(8, 'little')

            ciphertext = aead_chacha20_poly1305_encrypt(key, nonce, aad, plain)
            self.assertEqual(hex_cipher, ciphertext.hex())
            plaintext = aead_chacha20_poly1305_decrypt(key, nonce, aad, ciphertext)
            self.assertEqual(plain, plaintext)

    def test_fschacha20poly1305aead(self):
        "FSChaCha20Poly1305 AEAD test vectors."
        for test_vector in FSAEAD_TESTS:
            hex_plain, hex_aad, hex_key, msg_idx, hex_cipher = test_vector
            plain = bytes.fromhex(hex_plain)
            aad = bytes.fromhex(hex_aad)
            key = bytes.fromhex(hex_key)

            enc_aead = FSChaCha20Poly1305(key)
            dec_aead = FSChaCha20Poly1305(key)

            for _ in range(msg_idx):
                enc_aead.encrypt(b"", None)
            ciphertext = enc_aead.encrypt(aad, plain)
            self.assertEqual(hex_cipher, ciphertext.hex())

            for _ in range(msg_idx):
                dec_aead.decrypt(b"", None)
            plaintext = dec_aead.decrypt(aad, ciphertext)
            self.assertEqual(plain, plaintext)
