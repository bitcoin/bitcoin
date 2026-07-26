#!/usr/bin/env python3
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Base classes for creating dynamic xprvs and xpubs. Only basic
functionality of BIP32 is provided here. These classes work over
the ECKey and ECPubKey classes in the key.py."""

import hashlib
import hmac
import unittest

from test_framework.address import byte_to_base58
from test_framework.crypto import secp256k1
from test_framework.key import (
    ECKey,
    ECPubKey,
    generate_privkey,
    ORDER,
)
from test_framework.script import hash160

BIP32_HARDENED = 0x80000000

def get_version(private=False, mainnet=False):
    if mainnet:
        if private:
            return bytes.fromhex("0488ADE4")
        return bytes.fromhex("0488B21E")

    if private:
        return bytes.fromhex("04358394")
    return bytes.fromhex("043587CF")

def hardened(index):
    assert 0 <= index < BIP32_HARDENED
    return index | BIP32_HARDENED

def derive_path(key, path, path_idx_parser):
    if path in ("", "m"):
        return key
    parts = path.split("/")
    if parts[0] == "m":
        parts = parts[1:]
    for part in parts:
        index = path_idx_parser(part)
        key = key._derive(index)

    return key

class ExtendedPrivateKey:
    def __init__(self, key, chaincode, depth=0, parent_fingerprint_bytes=b"\x00\x00\x00\x00", child_num=0):
        self.key = key
        self.chaincode = chaincode
        self.depth = depth
        self.parent_fingerprint_bytes = parent_fingerprint_bytes
        self.child_num = child_num

    @classmethod
    def from_seed(cls, seed):
        I = hmac.new(b"Bitcoin seed", seed, hashlib.sha512).digest()

        secret = I[:32]
        chaincode = I[32:]

        secret_int = int.from_bytes(secret, "big")
        if secret_int == 0 or secret_int >= ORDER:
            raise ValueError("Invalid master key")

        key = ECKey()
        key.set(secret, compressed=True)

        return cls(key, chaincode)

    @classmethod
    def generate(cls):
        return cls.from_seed(generate_privkey())

    def _fingerprint(self):
        return hash160(self.key.get_pubkey().get_bytes())[:4]

    def _derive(self, index):
        if index >= BIP32_HARDENED:
            data = (b"\x00" + self.key.get_bytes() + index.to_bytes(4, "big"))
        else:
            data = (self.key.get_pubkey().get_bytes() + index.to_bytes(4, "big"))

        I = hmac.new(self.chaincode, data, hashlib.sha512).digest()
        IL = I[:32]
        IR = I[32:]

        IL_int = int.from_bytes(IL, "big")
        child_secret = (IL_int + int.from_bytes(self.key.get_bytes(), "big")) % ORDER
        # Per BIP32, if IL >= n or the child key is 0, the key is invalid. This is
        # astronomically unlikely (~1 in 2^127), so reject rather than retrying the next index.
        if IL_int >= ORDER or child_secret == 0:
            raise ValueError("Invalid BIP32 child key")

        child = ECKey()
        child.set(child_secret.to_bytes(32, 'big'), compressed=True)

        return ExtendedPrivateKey(child, IR, self.depth + 1, self._fingerprint(), index)

    def _serialize(self):
        return (bytes([self.depth]) + self.parent_fingerprint_bytes + self.child_num.to_bytes(4, "big") + self.chaincode + b"\x00" + self.key.get_bytes())

    def pubkey(self):
        return ExtendedPublicKey(self.key.get_pubkey(), self.chaincode, self.depth, self.parent_fingerprint_bytes, self.child_num)

    def derive_path(self, path):
        def path_idx_parser(part):
            if part.endswith(("h", "'")):
                return hardened(int(part[:-1]))
            return int(part)

        return derive_path(self, path, path_idx_parser)

    def to_string(self, mainnet=False):
        return byte_to_base58(self._serialize(), get_version(private=True, mainnet=mainnet))

class ExtendedPublicKey:
    def __init__(self, pubkey, chaincode, depth, parent_fingerprint_bytes, child_num):
        self.pubkey = pubkey
        self.chaincode = chaincode
        self.depth = depth
        self.parent_fingerprint_bytes = parent_fingerprint_bytes
        self.child_num = child_num

    def _fingerprint(self):
        return hash160(self.pubkey.get_bytes())[:4]

    def _derive(self, index):
        if index >= BIP32_HARDENED:
            raise ValueError("Cannot derive hardened child from xpub")

        data = (self.pubkey.get_bytes() + index.to_bytes(4, "big"))
        I = hmac.new(self.chaincode, data, hashlib.sha512).digest()
        IL = I[:32]
        IR = I[32:]

        IL_int = int.from_bytes(IL, "big")
        child_point = IL_int * secp256k1.G + self.pubkey.p
        # Per BIP32, if IL >= n or the resulting point is infinity, the key is invalid.
        if IL_int >= ORDER or child_point.infinity:
            raise ValueError("Invalid BIP32 child key")

        child_pubkey = ECPubKey()
        child_pubkey.set(child_point.to_bytes_compressed())

        return ExtendedPublicKey(child_pubkey, IR, self.depth + 1, self._fingerprint(), index)

    def _serialize(self):
        return (bytes([self.depth]) + self.parent_fingerprint_bytes + self.child_num.to_bytes(4, "big") + self.chaincode + self.pubkey.get_bytes())

    def derive_path(self, path):
        return derive_path(self, path, lambda x: int(x))

    def to_string(self, mainnet=False):
        return byte_to_base58(self._serialize(), get_version(private=False, mainnet=mainnet))

class TestFrameworkExtendedKey(unittest.TestCase):
    def test_bip32_vectors(self):
        vectors = [
            [
                "000102030405060708090a0b0c0d0e0f",
                [
                    ["m", "xprv9s21ZrQH143K3QTDL4LXw2F7HEK3wJUD2nW2nRk4stbPy6cq3jPPqjiChkVvvNKmPGJxWUtg6LnF5kejMRNNU3TGtRBeJgk33yuGBxrMPHi", "xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ29ESFjqJoCu1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8"],
                    ["m/0h", "xprv9uHRZZhk6KAJC1avXpDAp4MDc3sQKNxDiPvvkX8Br5ngLNv1TxvUxt4cV1rGL5hj6KCesnDYUhd7oWgT11eZG7XnxHrnYeSvkzY7d2bhkJ7", "xpub68Gmy5EdvgibQVfPdqkBBCHxA5htiqg55crXYuXoQRKfDBFA1WEjWgP6LHhwBZeNK1VTsfTFUHCdrfp1bgwQ9xv5ski8PX9rL2dZXvgGDnw"],
                    ["m/0h/1", "xprv9wTYmMFdV23N2TdNG573QoEsfRrWKQgWeibmLntzniatZvR9BmLnvSxqu53Kw1UmYPxLgboyZQaXwTCg8MSY3H2EU4pWcQDnRnrVA1xe8fs", "xpub6ASuArnXKPbfEwhqN6e3mwBcDTgzisQN1wXN9BJcM47sSikHjJf3UFHKkNAWbWMiGj7Wf5uMash7SyYq527Hqck2AxYysAA7xmALppuCkwQ"],
                    ["m/0h/1/2h", "xprv9z4pot5VBttmtdRTWfWQmoH1taj2axGVzFqSb8C9xaxKymcFzXBDptWmT7FwuEzG3ryjH4ktypQSAewRiNMjANTtpgP4mLTj34bhnZX7UiM", "xpub6D4BDPcP2GT577Vvch3R8wDkScZWzQzMMUm3PWbmWvVJrZwQY4VUNgqFJPMM3No2dFDFGTsxxpG5uJh7n7epu4trkrX7x7DogT5Uv6fcLW5"],
                    ["m/0h/1/2h/2", "xprvA2JDeKCSNNZky6uBCviVfJSKyQ1mDYahRjijr5idH2WwLsEd4Hsb2Tyh8RfQMuPh7f7RtyzTtdrbdqqsunu5Mm3wDvUAKRHSC34sJ7in334", "xpub6FHa3pjLCk84BayeJxFW2SP4XRrFd1JYnxeLeU8EqN3vDfZmbqBqaGJAyiLjTAwm6ZLRQUMv1ZACTj37sR62cfN7fe5JnJ7dh8zL4fiyLHV"],
                    ["m/0h/1/2h/2/1000000000", "xprvA41z7zogVVwxVSgdKUHDy1SKmdb533PjDz7J6N6mV6uS3ze1ai8FHa8kmHScGpWmj4WggLyQjgPie1rFSruoUihUZREPSL39UNdE3BBDu76", "xpub6H1LXWLaKsWFhvm6RVpEL9P4KfRZSW7abD2ttkWP3SSQvnyA8FSVqNTEcYFgJS2UaFcxupHiYkro49S8yGasTvXEYBVPamhGW6cFJodrTHy"],
                ]
            ],
            [
                "fffcf9f6f3f0edeae7e4e1dedbd8d5d2cfccc9c6c3c0bdbab7b4b1aeaba8a5a29f9c999693908d8a8784817e7b7875726f6c696663605d5a5754514e4b484542",
                [
                    ["m", "xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6ssrdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U", "xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB"],
                    ["m/0", "xprv9vHkqa6EV4sPZHYqZznhT2NPtPCjKuDKGY38FBWLvgaDx45zo9WQRUT3dKYnjwih2yJD9mkrocEZXo1ex8G81dwSM1fwqWpWkeS3v86pgKt", "xpub69H7F5d8KSRgmmdJg2KhpAK8SR3DjMwAdkxj3ZuxV27CprR9LgpeyGmXUbC6wb7ERfvrnKZjXoUmmDznezpbZb7ap6r1D3tgFxHmwMkQTPH"],
                    ["m/0/2147483647h", "xprv9wSp6B7kry3Vj9m1zSnLvN3xH8RdsPP1Mh7fAaR7aRLcQMKTR2vidYEeEg2mUCTAwCd6vnxVrcjfy2kRgVsFawNzmjuHc2YmYRmagcEPdU9", "xpub6ASAVgeehLbnwdqV6UKMHVzgqAG8Gr6riv3Fxxpj8ksbH9ebxaEyBLZ85ySDhKiLDBrQSARLq1uNRts8RuJiHjaDMBU4Zn9h8LZNnBC5y4a"],
                    ["m/0/2147483647h/1", "xprv9zFnWC6h2cLgpmSA46vutJzBcfJ8yaJGg8cX1e5StJh45BBciYTRXSd25UEPVuesF9yog62tGAQtHjXajPPdbRCHuWS6T8XA2ECKADdw4Ef", "xpub6DF8uhdarytz3FWdA8TvFSvvAh8dP3283MY7p2V4SeE2wyWmG5mg5EwVvmdMVCQcoNJxGoWaU9DCWh89LojfZ537wTfunKau47EL2dhHKon"],
                    ["m/0/2147483647h/1/2147483646h", "xprvA1RpRA33e1JQ7ifknakTFpgNXPmW2YvmhqLQYMmrj4xJXXWYpDPS3xz7iAxn8L39njGVyuoseXzU6rcxFLJ8HFsTjSyQbLYnMpCqE2VbFWc", "xpub6ERApfZwUNrhLCkDtcHTcxd75RbzS1ed54G1LkBUHQVHQKqhMkhgbmJbZRkrgZw4koxb5JaHWkY4ALHY2grBGRjaDMzQLcgJvLJuZZvRcEL"],
                    ["m/0/2147483647h/1/2147483646h/2", "xprvA2nrNbFZABcdryreWet9Ea4LvTJcGsqrMzxHx98MMrotbir7yrKCEXw7nadnHM8Dq38EGfSh6dqA9QWTyefMLEcBYJUuekgW4BYPJcr9E7j", "xpub6FnCn6nSzZAw5Tw7cgR9bi15UV96gLZhjDstkXXxvCLsUXBGXPdSnLFbdpq8p9HmGsApME5hQTZ3emM2rnY5agb9rXpVGyy3bdW6EEgAtqt"]
                ]
            ]
        ]

        for vector in vectors:
            seed = bytes.fromhex(vector[0])
            xprv = ExtendedPrivateKey.from_seed(seed)
            for seed_vector in vector[1]:
                path = seed_vector[0]
                derivedxprv = xprv.derive_path(path)
                self.assertEqual(derivedxprv.to_string(mainnet=True), seed_vector[1])
                self.assertEqual(derivedxprv.pubkey().to_string(mainnet=True), seed_vector[2])
