import hashlib

from .secp256k1 import GE, Scalar


def ecdh_compressed_in_raw_out(seckey: bytes, pubkey: bytes) -> GE:
    """TODO"""
    shared_secret = Scalar.from_bytes_checked(seckey) * GE.from_bytes_compressed(pubkey)
    assert not shared_secret.infinity  # prime-order group
    return shared_secret


def ecdh_libsecp256k1(seckey: bytes, pubkey: bytes) -> bytes:
    """TODO"""
    shared_secret = ecdh_compressed_in_raw_out(seckey, pubkey)
    return hashlib.sha256(shared_secret.to_bytes_compressed()).digest()
