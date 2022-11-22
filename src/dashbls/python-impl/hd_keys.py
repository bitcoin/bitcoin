from ec import G1Generator, G2Generator, JacobianPoint, default_ec
from hkdf import extract_expand
from private_key import PrivateKey
from util import hash256


def key_gen(seed: bytes) -> PrivateKey:
    # KeyGen
    # 1. PRK = HKDF-Extract("BLS-SIG-KEYGEN-SALT-", IKM || I2OSP(0, 1))
    # 2. OKM = HKDF-Expand(PRK, keyInfo || I2OSP(L, 2), L)
    # 3. SK = OS2IP(OKM) mod r
    # 4. return SK

    L = 48
    # `ceil((3 * ceil(log2(r))) / 16)`, where `r` is the order of the BLS 12-381 curve
    okm = extract_expand(L, seed + bytes([0]), b"BLS-SIG-KEYGEN-SALT-", bytes([0, L]))
    return PrivateKey(int.from_bytes(okm, "big") % default_ec.n)


def ikm_to_lamport_sk(ikm: bytes, salt: bytes) -> bytes:
    return extract_expand(32 * 255, ikm, salt, b"")


def parent_sk_to_lamport_pk(parent_sk: PrivateKey, index: int) -> bytes:
    salt = index.to_bytes(4, "big")
    ikm = bytes(parent_sk)
    not_ikm = bytes([e ^ 0xFF for e in ikm])  # Flip bits
    lamport0 = ikm_to_lamport_sk(ikm, salt)
    lamport1 = ikm_to_lamport_sk(not_ikm, salt)

    lamport_pk = bytes()
    for i in range(255):
        lamport_pk += hash256(lamport0[i * 32 : (i + 1) * 32])
    for i in range(255):
        lamport_pk += hash256(lamport1[i * 32 : (i + 1) * 32])

    return hash256(lamport_pk)


def derive_child_sk(parent_sk: PrivateKey, index: int) -> PrivateKey:
    """
    Derives a hardened EIP-2333 child private key, from a parent private key,
    at the specified index.
    """
    lamport_pk = parent_sk_to_lamport_pk(parent_sk, index)
    return key_gen(lamport_pk)


def derive_child_sk_unhardened(parent_sk: PrivateKey, index: int) -> PrivateKey:
    """
    Derives an unhardened BIP-32 child private key, from a parent private key,
    at the specified index. WARNING: this key is not as secure as a hardened key.
    """
    h = hash256(bytes(parent_sk.get_g1()) + index.to_bytes(4, "big"))
    return PrivateKey.aggregate([PrivateKey.from_bytes(h), parent_sk])


def derive_child_g1_unhardened(parent_pk: JacobianPoint, index: int) -> JacobianPoint:
    """
    Derives an unhardened BIP-32 child public key, from a parent public key,
    at the specified index. WARNING: this key is not as secure as a hardened key.
    """
    h = hash256(bytes(parent_pk) + index.to_bytes(4, "big"))
    return parent_pk + PrivateKey.from_bytes(h).value * G1Generator()


def derive_child_g2_unhardened(parent_pk: JacobianPoint, index: int) -> JacobianPoint:
    """
    Derives an unhardened BIP-32 child public key, from a parent public key,
    at the specified index. WARNING: this key is not as secure as a hardened key.
    """
    h = hash256(bytes(parent_pk) + index.to_bytes(4, "big"))
    return parent_pk + PrivateKey.from_bytes(h) * G2Generator()


"""
Copyright 2020 Chia Network Inc

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
"""
