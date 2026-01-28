from .secp256k1 import GE, G
from .util import int_from_bytes

# The following function is based on the BIP 327 reference implementation
# https://github.com/bitcoin/bips/blob/master/bip-0327/reference.py


# Return the plain public key corresponding to a given secret key
def pubkey_gen_plain(seckey: bytes) -> bytes:
    d0 = int_from_bytes(seckey)
    if not (1 <= d0 <= GE.ORDER - 1):
        raise ValueError("The secret key must be an integer in the range 1..n-1.")
    P = d0 * G
    assert not P.infinity
    return P.to_bytes_compressed()
