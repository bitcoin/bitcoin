from util import hash256, hmac256
from ec import (generator_Fq, hash_to_point_Fq2, default_ec,
                hash_to_point_prehashed_Fq2, y_for_x,
                AffinePoint)
from fields import Fq
from copy import deepcopy
from aggregation_info import AggregationInfo
from signature import Signature


class PublicKey:
    """
    Public keys are G1 elements, which are elliptic curve points (x, y), where
    each x, y is a 381 bit Fq element. The serialized represenentation is just
    the x value, and thus 48 bytes. (With the 1st bit determining the valid y).
    """
    PUBLIC_KEY_SIZE = 48

    def __init__(self, value):
        self.value = value

    @staticmethod
    def from_bytes(buffer):
        bit1 = buffer[0] & 0x80
        buffer = bytes([buffer[0] & 0x1f]) + buffer[1:]
        x = Fq(default_ec.q, int.from_bytes(buffer, "big"))
        y_values = y_for_x(Fq(default_ec.q, x))
        y_values.sort()
        y = y_values[0]

        if bit1:
            y = y_values[1]

        return PublicKey(AffinePoint(x, y, False, default_ec).to_jacobian())

    @staticmethod
    def from_g1(g1_el):
        return PublicKey(g1_el)

    def get_fingerprint(self):
        ser = self.serialize()
        return int.from_bytes(hash256(ser)[:4], "big")

    def serialize(self):
        return self.value.serialize()

    def size(self):
        return self.PUBLIC_KEY_SIZE

    def __eq__(self, other):
        return self.value.serialize() == other.value.serialize()

    def __hash__(self):
        return int.from_bytes(self.value.serialize(), "big")

    def __lt__(self, other):
        return self.value.serialize() < other.value.serialize()

    def __str__(self):
        return "PublicKey(" + self.value.to_affine().__str__() + ")"

    def __repr__(self):
        return "PublicKey(" + self.value.to_affine().__repr__() + ")"

    def __deepcopy__(self, memo):
        return PublicKey.from_g1(deepcopy(self.value, memo))


class PrivateKey:
    """
    Private keys are just random integers between 1 and the group order.
    """
    PRIVATE_KEY_SIZE = 32

    def __init__(self, value):
        self.value = value

    @staticmethod
    def from_bytes(buffer):
        return PrivateKey(int.from_bytes(buffer, "big"))

    @staticmethod
    def from_seed(seed):
        hashed = hmac256(seed, b"BLS private key seed")
        return PrivateKey(int.from_bytes(hashed, "big") % default_ec.n)

    def get_public_key(self):
        return PublicKey.from_g1((self.value * generator_Fq())
                                    .to_jacobian())

    def sign(self, m):
        r = hash_to_point_Fq2(m).to_jacobian()
        aggregation_info = AggregationInfo.from_msg(self.get_public_key(), m)
        return Signature.from_g2(self.value * r, aggregation_info)

    def sign_prehashed(self, h):
        r = hash_to_point_prehashed_Fq2(h).to_jacobian()
        aggregation_info = AggregationInfo.from_msg_hash(self.get_public_key(),
                                                         h)
        return Signature.from_g2(self.value * r, aggregation_info)

    def __lt__(self, other):
        return self.value < other.value

    def __eq__(self, other):
        return self.value == other.value

    def __hash__(self):
        return self.value

    def serialize(self):
        return self.value.to_bytes(self.PRIVATE_KEY_SIZE, "big")

    def size(self):
        return self.PRIVATE_KEY_SIZE

    def __str__(self):
        return "PrivateKey(" + hex(self.value) + ")"

    def __repr__(self):
        return "PrivateKey(" + hex(self.value) + ")"


class ExtendedPrivateKey:
    version = 1
    EXTENDED_PRIVATE_KEY_SIZE = 77

    def __init__(self, version, depth, parent_fingerprint,
                 child_number, chain_code, private_key):
        self.version = version
        self.depth = depth
        self.parent_fingerprint = parent_fingerprint
        self.child_number = child_number
        self.chain_code = chain_code
        self.private_key = private_key

    @staticmethod
    def from_seed(seed):
        i_left = hmac256(seed + bytes([0]), b"BLS HD seed")
        i_right = hmac256(seed + bytes([1]), b"BLS HD seed")

        sk_int = int.from_bytes(i_left, "big") % default_ec.n
        sk = PrivateKey.from_bytes(
            sk_int.to_bytes(PrivateKey.PRIVATE_KEY_SIZE, "big"))
        return ExtendedPrivateKey(ExtendedPrivateKey.version, 0, 0,
                                  0, i_right, sk)

    def private_child(self, i):
        if (self.depth >= 255):
            raise Exception("Cannot go further than 255 levels")

        # Hardened keys have i >= 2^31. Non-hardened have i < 2^31
        hardened = (i >= (2 ** 31))

        if (hardened):
            hmac_input = self.private_key.serialize()
        else:
            hmac_input = self.private_key.get_public_key().serialize()

        hmac_input += i.to_bytes(4, "big")
        i_left = hmac256(hmac_input + bytes([0]), self.chain_code)
        i_right = hmac256(hmac_input + bytes([1]), self.chain_code)

        sk_int = ((int.from_bytes(i_left, "big") + self.private_key.value)
                  % default_ec.n)
        sk = PrivateKey.from_bytes(
            sk_int.to_bytes(PrivateKey.PRIVATE_KEY_SIZE, "big"))

        return ExtendedPrivateKey(ExtendedPrivateKey.version, self.depth + 1,
                                  self.private_key.get_public_key()
                                      .get_fingerprint(), i,
                                  i_right, sk)

    def public_child(self, i):
        return self.private_child(i).get_extended_public_key()

    def get_extended_public_key(self):
        serialized = (self.version.to_bytes(4, "big") +
                      bytes([self.depth]) +
                      self.parent_fingerprint.to_bytes(4, "big") +
                      self.child_number.to_bytes(4, "big") +
                      self.chain_code +
                      self.private_key.get_public_key().serialize())
        return ExtendedPublicKey.from_bytes(serialized)

    def get_private_key(self):
        return self.private_key

    def get_public_key(self):
        return self.private_key.get_public_key()

    def size(self):
        return self.EXTENDED_PRIVATE_KEY_SIZE

    def serialize(self):
        return (self.version.to_bytes(4, "big") +
                bytes([self.depth]) +
                self.parent_fingerprint.to_bytes(4, "big") +
                self.child_number.to_bytes(4, "big") +
                self.chain_code +
                self.private_key.serialize())

    def __eq__(self, other):
        return self.serialize() == other.serialize()

    def __hash__(self):
        return int.from_bytes(self.serialize())


class ExtendedPublicKey:
    EXTENDED_PUBLIC_KEY_SIZE = 93

    def __init__(self, version, depth, parent_fingerprint,
                 child_number, chain_code, public_key):
        self.version = version
        self.depth = depth
        self.parent_fingerprint = parent_fingerprint
        self.child_number = child_number
        self.chain_code = chain_code
        self.public_key = public_key

    @staticmethod
    def from_bytes(serialized):
        version = int.from_bytes(serialized[:4], "big")
        depth = int.from_bytes(serialized[4:5], "big")
        parent_fingerprint = int.from_bytes(serialized[5:9], "big")
        child_number = int.from_bytes(serialized[9:13], "big")
        chain_code = serialized[13:45]
        public_key = PublicKey.from_bytes(serialized[45:])
        return ExtendedPublicKey(version, depth, parent_fingerprint,
                                 child_number, chain_code, public_key)

    def public_child(self, i):
        if (self.depth >= 255):
            raise Exception("Cannot go further than 255 levels")

        # Hardened keys have i >= 2^31. Non-hardened have i < 2^31
        if i >= (2 ** 31):
            raise Exception("Cannot derive hardened children from public key")

        hmac_input = self.public_key.serialize() + i.to_bytes(4, "big")
        i_left = hmac256(hmac_input + bytes([0]), self.chain_code)
        i_right = hmac256(hmac_input + bytes([1]), self.chain_code)

        sk_left_int = (int.from_bytes(i_left, "big") % default_ec.n)
        sk_left = PrivateKey.from_bytes(
            sk_left_int.to_bytes(PrivateKey.PRIVATE_KEY_SIZE, "big"))

        new_pk = PublicKey.from_g1(sk_left.get_public_key().value +
                                      self.public_key.value)
        return ExtendedPublicKey(self.version, self.depth + 1,
                                 self.public_key.get_fingerprint(), i,
                                 i_right, new_pk)

    def get_public_key(self):
        return self.public_key

    def size(self):
        return self.EXTENDED_PUBLIC_KEY_SIZE

    def serialize(self):
        return (self.version.to_bytes(4, "big") +
                bytes([self.depth]) +
                self.parent_fingerprint.to_bytes(4, "big") +
                self.child_number.to_bytes(4, "big") +
                self.chain_code +
                self.public_key.serialize())

    def __eq__(self, other):
        return self.serialize() == other.serialize()

    def __hash__(self):
        return int.from_bytes(self.serialize())


"""
Copyright 2018 Chia Network Inc

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
