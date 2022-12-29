from __future__ import annotations

from collections import namedtuple
from copy import deepcopy
from typing import List, Optional

import bls12381
from fields import FieldExtBase, Fq, Fq2, Fq6, Fq12
from util import hash256

# Struct for elliptic curve parameters
EC = namedtuple("EC", "q a b gx gy g2x g2y n h x k sqrt_n3 sqrt_n3m1o2")

default_ec = EC(*bls12381.parameters())
default_ec_twist = EC(*bls12381.parameters_twist())


class AffinePoint:
    """
    Elliptic curve point, can represent any curve, and use Fq or Fq2
    coordinates.
    """

    def __init__(self, x, y, infinity: bool, ec=default_ec):
        if (
            (not isinstance(x, Fq) and not isinstance(x, FieldExtBase))
            or (not isinstance(y, Fq) and not isinstance(y, FieldExtBase))
            or type(x) != type(y)
        ):
            raise Exception("x,y should be field elements")
        self.FE = type(x)
        self.x = x
        self.y = y
        self.infinity = infinity
        self.ec = ec

    def is_on_curve(self) -> bool:
        """
        Check that y^2 = x^3 + ax + b.
        """
        if self.infinity:
            return True
        left = self.y * self.y
        right = self.x * self.x * self.x + self.ec.a * self.x + self.ec.b

        return left == right

    def __add__(self, other: AffinePoint) -> AffinePoint:
        if other == 0:
            return self
        if not isinstance(other, AffinePoint):
            raise Exception("Incorrect object")

        return add_points(self, other, self.ec, self.FE)

    def __radd__(self, other: AffinePoint) -> AffinePoint:
        return self.__add__(other)

    def __sub__(self, other: AffinePoint) -> AffinePoint:
        return self.__add__(other.negate())

    def __rsub__(self, other: AffinePoint) -> AffinePoint:
        return self.negate().__add__(other)

    def __str__(self) -> str:
        return (
            "AffinePoint(x="
            + self.x.__str__()
            + ", y="
            + self.y.__str__()
            + ", i="
            + str(self.infinity)
            + ")\n"
        )

    def __repr__(self) -> str:
        return (
            "AffinePoint(x="
            + self.x.__repr__()
            + ", y="
            + self.y.__repr__()
            + ", i="
            + str(self.infinity)
            + ")\n"
        )

    def __eq__(self, other) -> bool:
        if not isinstance(other, AffinePoint):
            return False
        return (
            self.x == other.x and self.y == other.y and self.infinity == other.infinity
        )

    def __ne__(self, other) -> bool:
        return not self.__eq__(other)

    def __mul__(self, c) -> AffinePoint:
        if not isinstance(c, Fq) and not isinstance(c, int):
            raise ValueError("Error, must be int or Fq")
        return scalar_mult_jacobian(c, self.to_jacobian(), self.ec).to_affine()

    def negate(self) -> AffinePoint:
        return AffinePoint(self.x, -self.y, self.infinity, self.ec)

    def __rmul__(self, c: Fq) -> AffinePoint:
        return self.__mul__(c)

    def to_jacobian(self) -> JacobianPoint:
        return JacobianPoint(
            self.x, self.y, self.FE.one(self.ec.q), self.infinity, self.ec
        )

    def __deepcopy__(self, memo) -> AffinePoint:
        return AffinePoint(
            deepcopy(self.x, memo), deepcopy(self.y, memo), self.infinity, self.ec
        )


class JacobianPoint:
    """
    Elliptic curve point, can represent any curve, and use Fq or Fq2
    coordinates. Uses Jacobian coordinates so that point addition
    does not require slow inversion.
    """

    def __init__(self, x, y, z, infinity: bool, ec=default_ec):

        if (
            not isinstance(x, Fq)
            and not isinstance(x, FieldExtBase)
            or (not isinstance(y, Fq) and not isinstance(y, FieldExtBase))
            or (not isinstance(z, Fq) and not isinstance(z, FieldExtBase))
        ):
            raise Exception("x,y should be field elements")
        self.FE = type(x)
        self.x = x
        self.y = y
        self.z = z
        self.infinity = infinity
        self.ec = ec

    def is_on_curve(self) -> bool:
        if self.infinity:
            return True
        return self.to_affine().is_on_curve()

    def negate(self) -> JacobianPoint:
        return self.to_affine().negate().to_jacobian()

    def to_affine(self) -> AffinePoint:
        if self.infinity:
            return AffinePoint(
                Fq.zero(self.ec.q), Fq.zero(self.ec.q), self.infinity, self.ec
            )
        new_x = self.x / (self.z ** 2)
        new_y = self.y / (self.z ** 3)
        return AffinePoint(new_x, new_y, self.infinity, self.ec)

    def check_valid(self) -> None:
        assert self.is_on_curve()
        assert self * self.ec.n == G2Infinity()

    def get_fingerprint(self) -> int:
        ser = bytes(self)
        return int.from_bytes(hash256(ser)[:4], "big")

    def __add__(self, other: JacobianPoint) -> JacobianPoint:
        if other == 0:
            return self
        if not isinstance(other, JacobianPoint):
            raise ValueError("Incorrect object")

        return add_points_jacobian(self, other, self.ec, self.FE)

    def __radd__(self, other: JacobianPoint) -> JacobianPoint:
        return self.__add__(other)

    def __eq__(self, other) -> bool:
        if not isinstance(other, JacobianPoint):
            return False
        return self.to_affine() == other.to_affine()

    def __ne__(self, other) -> bool:
        return not self.__eq__(other)

    def __mul__(self, c) -> JacobianPoint:
        if not isinstance(c, int) and not isinstance(c, Fq):
            raise ValueError("Error, must be int or Fq")
        return scalar_mult_jacobian(c, self, self.ec)

    def __rmul__(self, c) -> JacobianPoint:
        return self.__mul__(c)

    def __neg__(self) -> JacobianPoint:
        return self.to_affine().negate().to_jacobian()

    def __str__(self) -> str:
        return (
            "JacobianPoint(x="
            + self.x.__str__()
            + ", y="
            + self.y.__str__()
            + "z="
            + self.z.__str__()
            + ", i="
            + str(self.infinity)
            + ")\n"
        )

    def __repr__(self) -> str:
        return self.__str__()

    def __bytes__(self) -> bytes:
        return point_to_bytes(self, self.ec, self.FE)

    def __deepcopy__(self, memo) -> JacobianPoint:
        return JacobianPoint(
            deepcopy(self.x, memo),
            deepcopy(self.y, memo),
            deepcopy(self.z, memo),
            self.infinity,
            self.ec,
        )

    def __hash__(self) -> int:
        return int.from_bytes(bytes(self), "big")


def sign_Fq(element, ec=default_ec) -> bool:
    return element > Fq(ec.q, ((ec.q - 1) // 2))


def sign_Fq2(element, ec=default_ec_twist) -> bool:
    if element[1] == Fq(ec.q, 0):
        return sign_Fq(element[0])

    return element[1] > Fq(ec.q, ((ec.q - 1) // 2))


def point_to_bytes(point_j: JacobianPoint, ec, FE) -> bytes:
    # Zcash serialization described in https://datatracker.ietf.org/doc/draft-irtf-cfrg-pairing-friendly-curves/
    point = point_j.to_affine()
    output = bytearray(bytes(point.x))

    # If the y coordinate is the bigger one of the two, set the first
    # bit to 1.
    if point.infinity:
        return bytes([0x40]) + bytes([0] * (len(output) - 1))

    if FE == Fq:
        sign = sign_Fq(point.y, ec)
    else:
        sign = sign_Fq2(point.y, ec)

    if sign:
        output[0] |= 0xA0
    else:
        output[0] |= 0x80
    return bytes(output)


def bytes_to_point(buffer: bytes, ec, FE) -> JacobianPoint:
    # Zcash deserialization described in https://datatracker.ietf.org/doc/draft-irtf-cfrg-pairing-friendly-curves/

    if FE == Fq:
        if len(buffer) != 48:
            raise ValueError("G1Elements must be 48 bytes")
    elif FE == Fq2:
        if len(buffer) != 96:
            raise ValueError("G2Elements must be 96 bytes")
    else:
        raise ValueError("Invalid FE")

    m_byte = buffer[0] & 0xE0

    if m_byte in [0x20, 0x60, 0xE0]:
        raise ValueError("Invalid first three bits")

    C_bit = (m_byte & 0x80) >> 7  # First bit
    I_bit = (m_byte & 0x40) >> 6  # Second bit
    S_bit = (m_byte & 0x20) >> 5  # Third bit

    if C_bit == 0:
        raise ValueError("First bit must be 1 (only compressed points)")

    buffer = bytes([buffer[0] & 0x1F]) + buffer[1:]

    if I_bit == 1:
        if any([e != 0 for e in buffer]):
            raise ValueError("Point at infinity set, but data not all zeroes")
        return AffinePoint(FE.zero(ec.q), FE.zero(ec.q), True, ec).to_jacobian()

    x = FE.from_bytes(buffer, ec.q)
    y_value = y_for_x(x, ec, FE)

    if FE == Fq:
        sign_fn = sign_Fq
    else:
        sign_fn = sign_Fq2

    if sign_fn(y_value, ec) == S_bit:
        y = y_value
    else:
        y = -y_value

    return AffinePoint(x, y, False, ec).to_jacobian()


def y_for_x(x, ec=default_ec, FE=Fq):
    """
    Solves y = sqrt(x^3 + ax + b) for both valid ys.
    """
    if not isinstance(x, FE):
        x = FE(ec.q, x)

    u = x * x * x + ec.a * x + ec.b

    y = u.modsqrt()
    if y == 0 or not AffinePoint(x, y, False, ec).is_on_curve():
        raise ValueError("No y for point x")
    return y


def double_point(p1: AffinePoint, ec=default_ec, FE=Fq) -> AffinePoint:
    """
    Basic elliptic curve point doubling
    """
    x, y = p1.x, p1.y
    left = Fq(ec.q, 3) * x * x
    left = left + ec.a
    s = left / (Fq(ec.q, 2) * y)
    new_x = s * s - x - x
    new_y = s * (x - new_x) - y
    return AffinePoint(new_x, new_y, False, ec)


def add_points(p1: AffinePoint, p2: AffinePoint, ec=default_ec, FE=Fq) -> AffinePoint:
    """
    Basic elliptic curve point addition.
    """
    assert p1.is_on_curve()
    assert p2.is_on_curve()
    if p1.infinity:
        return p2
    if p2.infinity:
        return p1
    if p1 == p2:
        return double_point(p1, ec, FE)
    if p1.x == p2.x:
        return AffinePoint(FE.zero(ec.q), FE.zero(ec.q), True, ec)

    x1, y1 = p1.x, p1.y
    x2, y2 = p2.x, p2.y
    s = (y2 - y1) / (x2 - x1)
    new_x = s * s - x1 - x2
    new_y = s * (x1 - new_x) - y1
    return AffinePoint(new_x, new_y, False, ec)


def double_point_jacobian(p1: JacobianPoint, ec=default_ec, FE=Fq) -> JacobianPoint:
    """
    Jacobian elliptic curve point doubling, see
    http://www.hyperelliptic.org/EFD/oldefd/jacobian.html
    """
    X, Y, Z = p1.x, p1.y, p1.z
    if Y == FE.zero(ec.q) or p1.infinity:
        return JacobianPoint(FE.one(ec.q), FE.one(ec.q), FE.zero(ec.q), True, ec)

    # S = 4*X*Y^2
    S = Fq(ec.q, 4) * X * Y * Y

    Z_sq = Z * Z
    Z_4th = Z_sq * Z_sq
    Y_sq = Y * Y
    Y_4th = Y_sq * Y_sq

    # M = 3*X^2 + a*Z^4
    M = Fq(ec.q, 3) * X * X
    M += ec.a * Z_4th

    # X' = M^2 - 2*S
    X_p = M * M - Fq(ec.q, 2) * S
    # Y' = M*(S - X') - 8*Y^4
    Y_p = M * (S - X_p) - Fq(ec.q, 8) * Y_4th
    # Z' = 2*Y*Z
    Z_p = Fq(ec.q, 2) * Y * Z
    return JacobianPoint(X_p, Y_p, Z_p, False, ec)


def add_points_jacobian(
    p1: JacobianPoint, p2: JacobianPoint, ec=default_ec, FE=Fq
) -> JacobianPoint:
    """
    Jacobian elliptic curve point addition, see
    http://www.hyperelliptic.org/EFD/oldefd/jacobian.html
    """
    if p1.infinity:
        return p2
    if p2.infinity:
        return p1
    # U1 = X1*Z2^2
    U1 = p1.x * (p2.z ** 2)
    # U2 = X2*Z1^2
    U2 = p2.x * (p1.z ** 2)
    # S1 = Y1*Z2^3
    S1 = p1.y * (p2.z ** 3)
    # S2 = Y2*Z1^3
    S2 = p2.y * (p1.z ** 3)
    if U1 == U2:
        if S1 != S2:
            return JacobianPoint(FE.one(ec.q), FE.one(ec.q), FE.zero(ec.q), True, ec)
        else:
            return double_point_jacobian(p1, ec, FE)

    # H = U2 - U1
    H = U2 - U1
    # R = S2 - S1
    R = S2 - S1
    H_sq = H * H
    H_cu = H * H_sq
    # X3 = R^2 - H^3 - 2*U1*H^2
    X3 = R * R - H_cu - Fq(ec.q, 2) * U1 * H_sq
    # Y3 = R*(U1*H^2 - X3) - S1*H^3
    Y3 = R * (U1 * H_sq - X3) - S1 * H_cu
    # Z3 = H*Z1*Z2
    Z3 = H * p1.z * p2.z
    return JacobianPoint(X3, Y3, Z3, False, ec)


def scalar_mult(c, p1: AffinePoint, ec=default_ec, FE=Fq) -> AffinePoint:
    """
    Double and add, see
    https://en.wikipedia.org/wiki/Elliptic_curve_point_multiplication
    """
    if p1.infinity or c % ec.q == 0:
        return AffinePoint(FE.zero(ec.q), FE.zero(ec.q), ec)
    result = AffinePoint(FE.zero(ec.q), FE.zero(ec.q), True, ec)
    addend = p1
    while c > 0:
        if c & 1:
            result += addend

        # double point
        addend += addend
        c = c >> 1

    return result


def scalar_mult_jacobian(c, p1: JacobianPoint, ec=default_ec, FE=Fq) -> JacobianPoint:
    """
    Double and add, see
    https://en.wikipedia.org/wiki/Elliptic_curve_point_multiplication
    """
    if isinstance(c, FE):
        c = c.value
    if p1.infinity or c % ec.q == 0:
        return JacobianPoint(FE.one(ec.q), FE.one(ec.q), FE.zero(ec.q), True, ec)

    result = JacobianPoint(FE.one(ec.q), FE.one(ec.q), FE.zero(ec.q), True, ec)
    addend = p1
    while c > 0:
        if c & 1:
            result += addend
        # double point
        addend += addend
        c = c >> 1
    return result


def G1Generator(ec=default_ec) -> JacobianPoint:
    return AffinePoint(ec.gx, ec.gy, False, ec).to_jacobian()


def G2Generator(ec=default_ec_twist) -> JacobianPoint:
    return AffinePoint(ec.g2x, ec.g2y, False, ec).to_jacobian()


def G1Infinity(ec=default_ec, FE=Fq) -> JacobianPoint:
    return JacobianPoint(FE.one(ec.q), FE.one(ec.q), FE.zero(ec.q), True, ec)


def G2Infinity(ec=default_ec_twist, FE=Fq2) -> JacobianPoint:
    return JacobianPoint(FE.one(ec.q), FE.one(ec.q), FE.zero(ec.q), True, ec)


def G1FromBytes(buffer: bytes, ec=default_ec, FE=Fq) -> JacobianPoint:
    return bytes_to_point(buffer, ec, FE)


def G2FromBytes(buffer: bytes, ec=default_ec_twist, FE=Fq2):
    return bytes_to_point(buffer, ec, FE)


def untwist(point: AffinePoint, ec=default_ec) -> AffinePoint:
    """
    Given a point on G2 on the twisted curve, this converts its
    coordinates back from Fq2 to Fq12. See Craig Costello book, look
    up twists.
    """
    f = Fq12.one(ec.q)
    wsq = Fq12(ec.q, f.root, Fq6.zero(ec.q))
    wcu = Fq12(ec.q, Fq6.zero(ec.q), f.root)
    return AffinePoint(point.x / wsq, point.y / wcu, False, ec)


def twist(point: AffinePoint, ec=default_ec_twist) -> AffinePoint:
    """
    Given an untwisted point, this converts it's
    coordinates to a point on the twisted curve. See Craig Costello
    book, look up twists.
    """
    f = Fq12.one(ec.q)
    wsq = Fq12(ec.q, f.root, Fq6.zero(ec.q))
    wcu = Fq12(ec.q, Fq6.zero(ec.q), f.root)
    new_x = point.x * wsq
    new_y = point.y * wcu
    return AffinePoint(new_x, new_y, False, ec)


# Isogeny map evaluation specified by map_coeffs
#
# map_coeffs should be specified as (xnum, xden, ynum, yden)
#
# This function evaluates the isogeny over Jacobian projective coordinates.
# For details, see Section 4.3 of
#    Wahby and Boneh, "Fast and simple constant-time hashing to the BLS12-381 elliptic curve."
#    ePrint # 2019/403, https://ia.cr/2019/403.
def eval_iso(P: JacobianPoint, map_coeffs, ec) -> JacobianPoint:
    (x, y, z) = (P.x, P.y, P.z)
    mapvals: List[Optional[Fq2]] = [None] * 4

    # Precompute the required powers of Z^2
    maxord = max(len(coeffs) for coeffs in map_coeffs)
    zpows: List[Optional[Fq2]] = [None] * maxord
    zpows[0] = z ** 0  # type: ignore
    zpows[1] = z ** 2  # type: ignore
    for idx in range(2, len(zpows)):
        assert zpows[idx - 1] is not None
        assert zpows[1] is not None
        zpows[idx] = zpows[idx - 1] * zpows[1]

    # Compute the numerator and denominator of the X and Y maps via Horner's rule
    for (idx, coeffs) in enumerate(map_coeffs):
        coeffs_z = [
            zpow * c for (zpow, c) in zip(reversed(coeffs), zpows[: len(coeffs)])
        ]
        tmp = coeffs_z[0]
        for coeff in coeffs_z[1:]:
            tmp *= x
            tmp += coeff
        mapvals[idx] = tmp

    # xden is of order 1 less than xnum, so one needs to multiply it by an extra factor of Z^2
    assert len(map_coeffs[1]) + 1 == len(map_coeffs[0])
    assert zpows[1] is not None
    assert mapvals[1] is not None
    mapvals[1] *= zpows[1]

    # Multiply the result of Y map by the y-coordinate y / z^3
    assert mapvals[2] is not None
    assert mapvals[3] is not None
    mapvals[2] *= y
    mapvals[3] *= z ** 3

    Z = mapvals[1] * mapvals[3]
    X = mapvals[0] * mapvals[3] * Z
    Y = mapvals[2] * mapvals[1] * Z * Z
    return JacobianPoint(X, Y, Z, P.infinity, ec)


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
