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

