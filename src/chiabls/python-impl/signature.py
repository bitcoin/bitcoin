from ec import (default_ec, default_ec_twist, y_for_x, AffinePoint,
                JacobianPoint)
from fields import Fq, Fq2
from copy import deepcopy


class Signature:
    """
    Signatures are G1 elements, which are elliptic curve points (x, y), where
    each x, y is a (2*381) bit Fq2 element. The serialized represenentation is
    just the x value, and thus 96 bytes. (With the 1st bit determining the
    valid y).
    """
    SIGNATURE_SIZE = 96

    def __init__(self, value, aggregation_info=None):
        self.value = value
        self.aggregation_info = aggregation_info

    @staticmethod
    def from_bytes(buffer, aggregation_info=None):
        use_big_y = buffer[0] & 0x80

        buffer = bytes([buffer[0] & 0x1f]) + buffer[1:]

        x0 = int.from_bytes(buffer[:48], "big")
        x1 = int.from_bytes(buffer[48:], "big")
        x = Fq2(default_ec.q, Fq(default_ec.q, x0), Fq(default_ec.q, x1))
        ys = y_for_x(x, default_ec_twist, Fq2)
        y = ys[0]
        if ((use_big_y and ys[1][1] > default_ec.q // 2) or
                (not use_big_y and ys[1][1] < default_ec.q // 2)):
            y = ys[1]

        return Signature(AffinePoint(x, y, False, default_ec_twist)
                            .to_jacobian(),
                            aggregation_info)

    @staticmethod
    def from_g2(g2_el, aggregation_info=None):
        return Signature(g2_el, aggregation_info)

    def divide_by(self, divisor_signatures):
        """
        Signature division (elliptic curve subtraction). This is useful if
        you have already verified parts of the tree, since verification
        of the resulting quotient signature will be faster (less pairings
        have to be perfomed).

        This function Divides an aggregate signature by other signatures
        in the aggregate trees. A signature can only be divided if it is
        part of the subset, and all message/public key pairs in the
        aggregationInfo for the divisor signature are unique. i.e you cannot
        divide s1 / s2, if s2 is an aggregate signature containing m1,pk1,
        which is also present somewhere else in s1's tree. Note, s2 itself
        does not have to be unique.
        """
        message_hashes_to_remove = []
        pubkeys_to_remove = []
        prod = JacobianPoint(Fq2.one(default_ec.q), Fq2.one(default_ec.q),
                             Fq2.zero(default_ec.q), True, default_ec)
        for divisor_sig in divisor_signatures:
            pks = divisor_sig.aggregation_info.public_keys
            message_hashes = divisor_sig.aggregation_info.message_hashes
            if len(pks) != len(message_hashes):
                raise Exception("Invalid aggregation info")

            for i in range(len(pks)):
                divisor = divisor_sig.aggregation_info.tree[
                        (message_hashes[i], pks[i])]
                try:
                    dividend = self.aggregation_info.tree[
                        (message_hashes[i], pks[i])]
                except KeyError:
                    raise Exception("Signature is not a subset")
                if i == 0:
                    quotient = (Fq(default_ec.n, dividend)
                                / Fq(default_ec.n, divisor))
                else:
                    # Makes sure the quotient is identical for each public
                    # key, which means message/pk pair is unique.
                    new_quotient = (Fq(default_ec.n, dividend)
                                    / Fq(default_ec.n, divisor))
                    if quotient != new_quotient:
                        raise Exception("Cannot divide by aggregate signature,"
                                        + "msg/pk pairs are not unique")
                message_hashes_to_remove.append(message_hashes[i])
                pubkeys_to_remove.append(pks[i])
            prod += (divisor_sig.value * -quotient)
        copy = Signature(deepcopy(self.value + prod),
                            deepcopy(self.aggregation_info))

        for i in range(len(message_hashes_to_remove)):
            a = message_hashes_to_remove[i]
            b = pubkeys_to_remove[i]
            if (a, b) in copy.aggregation_info.tree:
                del copy.aggregation_info.tree[(a, b)]
        sorted_keys = list(copy.aggregation_info.tree.keys())
        sorted_keys.sort()
        copy.aggregation_info.message_hashes = [t[0] for t in sorted_keys]
        copy.aggregation_info.public_keys = [t[1] for t in sorted_keys]
        return copy

    def set_aggregation_info(self, aggregation_info):
        self.aggregation_info = aggregation_info

    def get_aggregation_info(self):
        return self.aggregation_info

    def __eq__(self, other):
        return self.value.serialize() == other.value.serialize()

    def __hash__(self):
        return int.from_bytes(self.value.serialize(), "big")

    def __lt__(self, other):
        return self.value.serialize() < other.value.serialize()

    def serialize(self):
        return self.value.serialize()

    def size(self):
        return self.SIGNATURE_SIZE

    def __str__(self):
        return "Signature(" + self.value.to_affine().__str__() + ")"

    def __repr__(self):
        return "Signature(" + self.value.to_affine().__repr__() + ")"


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
