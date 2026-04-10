import hashlib


# This implementation can be sped up by storing the midstate after hashing
# tag_hash instead of rehashing it all the time.
def tagged_hash(tag: str, msg: bytes) -> bytes:
    tag_hash = hashlib.sha256(tag.encode()).digest()
    return hashlib.sha256(tag_hash + tag_hash + msg).digest()


def bytes_from_int(x: int) -> bytes:
    return x.to_bytes(32, byteorder="big")


def xor_bytes(b0: bytes, b1: bytes) -> bytes:
    return bytes(x ^ y for (x, y) in zip(b0, b1))


def int_from_bytes(b: bytes) -> int:
    return int.from_bytes(b, byteorder="big")


def hash_sha256(b: bytes) -> bytes:
    return hashlib.sha256(b).digest()
