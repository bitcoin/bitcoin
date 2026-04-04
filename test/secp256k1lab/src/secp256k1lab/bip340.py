# The following functions are based on the BIP 340 reference implementation:
# https://github.com/bitcoin/bips/blob/master/bip-0340/reference.py

from .secp256k1 import FE, GE, G
from .util import int_from_bytes, bytes_from_int, xor_bytes, tagged_hash


def pubkey_gen(seckey: bytes) -> bytes:
    d0 = int_from_bytes(seckey)
    if not (1 <= d0 <= GE.ORDER - 1):
        raise ValueError("The secret key must be an integer in the range 1..n-1.")
    P = d0 * G
    assert not P.infinity
    return P.to_bytes_xonly()


def schnorr_sign(
    msg: bytes, seckey: bytes, aux_rand: bytes, tag_prefix: str = "BIP0340"
) -> bytes:
    d0 = int_from_bytes(seckey)
    if not (1 <= d0 <= GE.ORDER - 1):
        raise ValueError("The secret key must be an integer in the range 1..n-1.")
    if len(aux_rand) != 32:
        raise ValueError("aux_rand must be 32 bytes instead of %i." % len(aux_rand))
    P = d0 * G
    assert not P.infinity
    d = d0 if P.has_even_y() else GE.ORDER - d0
    t = xor_bytes(bytes_from_int(d), tagged_hash(tag_prefix + "/aux", aux_rand))
    k0 = (
        int_from_bytes(tagged_hash(tag_prefix + "/nonce", t + P.to_bytes_xonly() + msg))
        % GE.ORDER
    )
    if k0 == 0:
        raise RuntimeError("Failure. This happens only with negligible probability.")
    R = k0 * G
    assert not R.infinity
    k = k0 if R.has_even_y() else GE.ORDER - k0
    e = (
        int_from_bytes(
            tagged_hash(
                tag_prefix + "/challenge", R.to_bytes_xonly() + P.to_bytes_xonly() + msg
            )
        )
        % GE.ORDER
    )
    sig = R.to_bytes_xonly() + bytes_from_int((k + e * d) % GE.ORDER)
    assert schnorr_verify(msg, P.to_bytes_xonly(), sig, tag_prefix=tag_prefix)
    return sig


def schnorr_verify(
    msg: bytes, pubkey: bytes, sig: bytes, tag_prefix: str = "BIP0340"
) -> bool:
    if len(pubkey) != 32:
        raise ValueError("The public key must be a 32-byte array.")
    if len(sig) != 64:
        raise ValueError("The signature must be a 64-byte array.")
    try:
        P = GE.from_bytes_xonly(pubkey)
    except ValueError:
        return False
    r = int_from_bytes(sig[0:32])
    s = int_from_bytes(sig[32:64])
    if (r >= FE.SIZE) or (s >= GE.ORDER):
        return False
    e = (
        int_from_bytes(tagged_hash(tag_prefix + "/challenge", sig[0:32] + pubkey + msg))
        % GE.ORDER
    )
    R = s * G - e * P
    if R.infinity or (not R.has_even_y()) or (R.x != r):
        return False
    return True
