# Copyright (c) 2022-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

"""Test-only implementation of low-level secp256k1 field and group arithmetic

It is designed for ease of understanding, not performance.

WARNING: This code is slow and trivially vulnerable to side channel attacks. Do not use for
anything but tests.

Exports:
* FE: class for secp256k1 field elements
* GE: class for secp256k1 group elements
* G: the secp256k1 generator point
"""

import functools
import unittest
from hashlib import sha256
from test_framework.util import assert_equal, assert_not_equal

class FE:
    """Objects of this class represent elements of the field GF(2**256 - 2**32 - 977).

    They are represented internally in numerator / denominator form, in order to delay inversions.
    """

    # The size of the field (also its modulus and characteristic).
    SIZE = 2**256 - 2**32 - 977

    def __init__(self, a=0, b=1):
        """Initialize a field element a/b; both a and b can be ints or field elements."""
        if isinstance(a, FE):
            num = a._num
            den = a._den
        else:
            num = a % FE.SIZE
            den = 1
        if isinstance(b, FE):
            den = (den * b._num) % FE.SIZE
            num = (num * b._den) % FE.SIZE
        else:
            den = (den * b) % FE.SIZE
        assert_not_equal(den, 0)
        if num == 0:
            den = 1
        self._num = num
        self._den = den

    def __add__(self, a):
        """Compute the sum of two field elements (second may be int)."""
        if isinstance(a, FE):
            return FE(self._num * a._den + self._den * a._num, self._den * a._den)
        return FE(self._num + self._den * a, self._den)

    def __radd__(self, a):
        """Compute the sum of an integer and a field element."""
        return FE(a) + self

    def __sub__(self, a):
        """Compute the difference of two field elements (second may be int)."""
        if isinstance(a, FE):
            return FE(self._num * a._den - self._den * a._num, self._den * a._den)
        return FE(self._num - self._den * a, self._den)

    def __rsub__(self, a):
        """Compute the difference of an integer and a field element."""
        return FE(a) - self

    def __mul__(self, a):
        """Compute the product of two field elements (second may be int)."""
        if isinstance(a, FE):
            return FE(self._num * a._num, self._den * a._den)
        return FE(self._num * a, self._den)

    def __rmul__(self, a):
        """Compute the product of an integer with a field element."""
        return FE(a) * self

    def __truediv__(self, a):
        """Compute the ratio of two field elements (second may be int)."""
        return FE(self, a)

    def __pow__(self, a):
        """Raise a field element to an integer power."""
        return FE(pow(self._num, a, FE.SIZE), pow(self._den, a, FE.SIZE))

    def __neg__(self):
        """Negate a field element."""
        return FE(-self._num, self._den)

    def __int__(self):
        """Convert a field element to an integer in range 0..p-1. The result is cached."""
        if self._den != 1:
            self._num = (self._num * pow(self._den, -1, FE.SIZE)) % FE.SIZE
            self._den = 1
        return self._num

    def sqrt(self):
        """Compute the square root of a field element if it exists (None otherwise).

        Due to the fact that our modulus is of the form (p % 4) == 3, the Tonelli-Shanks
        algorithm (https://en.wikipedia.org/wiki/Tonelli-Shanks_algorithm) is simply
        raising the argument to the power (p + 1) / 4.

        To see why: (p-1) % 2 = 0, so 2 divides the order of the multiplicative group,
        and thus only half of the non-zero field elements are squares. An element a is
        a (nonzero) square when Euler's criterion, a^((p-1)/2) = 1 (mod p), holds. We're
        looking for x such that x^2 = a (mod p). Given a^((p-1)/2) = 1, that is equivalent
        to x^2 = a^(1 + (p-1)/2) mod p. As (1 + (p-1)/2) is even, this is equivalent to
        x = a^((1 + (p-1)/2)/2) mod p, or x = a^((p+1)/4) mod p."""
        v = int(self)
        s = pow(v, (FE.SIZE + 1) // 4, FE.SIZE)
        if s**2 % FE.SIZE == v:
            return FE(s)
        return None

    def is_square(self):
        """Determine if this field element has a square root."""
        # A more efficient algorithm is possible here (Jacobi symbol).
        return self.sqrt() is not None

    def is_even(self):
        """Determine whether this field element, represented as integer in 0..p-1, is even."""
        return int(self) & 1 == 0

    def __eq__(self, a):
        """Check whether two field elements are equal (second may be an int)."""
        if isinstance(a, FE):
            return (self._num * a._den - self._den * a._num) % FE.SIZE == 0
        return (self._num - self._den * a) % FE.SIZE == 0

    def to_bytes(self):
        """Convert a field element to a 32-byte array (BE byte order)."""
        return int(self).to_bytes(32, 'big')

    @staticmethod
    def from_bytes(b):
        """Convert a 32-byte array to a field element (BE byte order, no overflow allowed)."""
        v = int.from_bytes(b, 'big')
        if v >= FE.SIZE:
            return None
        return FE(v)

    def __str__(self):
        """Convert this field element to a 64 character hex string."""
        return f"{int(self):064x}"

    def __repr__(self):
        """Get a string representation of this field element."""
        return f"FE(0x{int(self):x})"


class GE:
    """Objects of this class represent secp256k1 group elements (curve points or infinity)

    Normal points on the curve have fields:
    * x: the x coordinate (a field element)
    * y: the y coordinate (a field element, satisfying y^2 = x^3 + 7)
    * infinity: False

    The point at infinity has field:
    * infinity: True
    """

    # Order of the group (number of points on the curve, plus 1 for infinity)
    ORDER = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141

    # Number of valid distinct x coordinates on the curve.
    ORDER_HALF = ORDER // 2

    def __init__(self, x=None, y=None):
        """Initialize a group element with specified x and y coordinates, or infinity."""
        if x is None:
            # Initialize as infinity.
            assert y is None
            self.infinity = True
        else:
            # Initialize as point on the curve (and check that it is).
            fx = FE(x)
            fy = FE(y)
            assert_equal(fy**2, fx**3 + 7)
            self.infinity = False
            self.x = fx
            self.y = fy

    def __add__(self, a):
        """Add two group elements together."""
        # Deal with infinity: a + infinity == infinity + a == a.
        if self.infinity:
            return a
        if a.infinity:
            return self
        if self.x == a.x:
            if self.y != a.y:
                # A point added to its own negation is infinity.
                assert_equal(self.y + a.y, 0)
                return GE()
            else:
                # For identical inputs, use the tangent (doubling formula).
                lam = (3 * self.x**2) / (2 * self.y)
        else:
            # For distinct inputs, use the line through both points (adding formula).
            lam = (self.y - a.y) / (self.x - a.x)
        # Determine point opposite to the intersection of that line with the curve.
        x = lam**2 - (self.x + a.x)
        y = lam * (self.x - x) - self.y
        return GE(x, y)

    @staticmethod
    def mul(*aps):
        """Compute a (batch) scalar group element multiplication.

        GE.mul((a1, p1), (a2, p2), (a3, p3)) is identical to a1*p1 + a2*p2 + a3*p3,
        but more efficient."""
        # Reduce all the scalars modulo order first (so we can deal with negatives etc).
        naps = [(a % GE.ORDER, p) for a, p in aps]
        # Start with point at infinity.
        r = GE()
        # Iterate over all bit positions, from high to low.
        for i in range(255, -1, -1):
            # Double what we have so far.
            r = r + r
            # Add then add the points for which the corresponding scalar bit is set.
            for (a, p) in naps:
                if (a >> i) & 1:
                    r += p
        return r

    def __rmul__(self, a):
        """Multiply an integer with a group element."""
        if self == G:
            return mul_G(a % GE.ORDER)
        return GE.mul((a, self))

    def __neg__(self):
        """Compute the negation of a group element."""
        if self.infinity:
            return self
        return GE(self.x, -self.y)

    def to_bytes_compressed(self):
        """Convert a non-infinite group element to 33-byte compressed encoding."""
        assert not self.infinity
        return bytes([3 - self.y.is_even()]) + self.x.to_bytes()

    def to_bytes_uncompressed(self):
        """Convert a non-infinite group element to 65-byte uncompressed encoding."""
        assert not self.infinity
        return b'\x04' + self.x.to_bytes() + self.y.to_bytes()

    def to_bytes_xonly(self):
        """Convert (the x coordinate of) a non-infinite group element to 32-byte xonly encoding."""
        assert not self.infinity
        return self.x.to_bytes()

    @staticmethod
    def lift_x(x):
        """Return group element with specified field element as x coordinate (and even y)."""
        y = (FE(x)**3 + 7).sqrt()
        if y is None:
            return None
        if not y.is_even():
            y = -y
        return GE(x, y)

    @staticmethod
    def from_bytes(b):
        """Convert a compressed or uncompressed encoding to a group element."""
        assert len(b) in (33, 65)
        if len(b) == 33:
            if b[0] != 2 and b[0] != 3:
                return None
            x = FE.from_bytes(b[1:])
            if x is None:
                return None
            r = GE.lift_x(x)
            if r is None:
                return None
            if b[0] == 3:
                r = -r
            return r
        else:
            if b[0] != 4:
                return None
            x = FE.from_bytes(b[1:33])
            y = FE.from_bytes(b[33:])
            if y**2 != x**3 + 7:
                return None
            return GE(x, y)

    @staticmethod
    def from_bytes_xonly(b):
        """Convert a point given in xonly encoding to a group element."""
        assert_equal(len(b), 32)
        x = FE.from_bytes(b)
        if x is None:
            return None
        return GE.lift_x(x)

    @staticmethod
    def is_valid_x(x):
        """Determine whether the provided field element is a valid X coordinate."""
        return (FE(x)**3 + 7).is_square()

    def __str__(self):
        """Convert this group element to a string."""
        if self.infinity:
            return "(inf)"
        return f"({self.x},{self.y})"

    def __repr__(self):
        """Get a string representation for this group element."""
        if self.infinity:
            return "GE()"
        return f"GE(0x{int(self.x):x},0x{int(self.y):x})"

# The secp256k1 generator point
G = GE.lift_x(0x79BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798)


class FastGEMul:
    """Table for fast multiplication with a constant group element.

    Speed up scalar multiplication with a fixed point P by using a precomputed lookup table.
    The scalar is split into WINDOW-bit chunks, and for every chunk position i the table holds
    a row [(0 * 2^(WINDOW*i)) * P, (1 * 2^(WINDOW*i)) * P, ..., ((2^WINDOW-1) * 2^(WINDOW*i)) * P]:

        table[i][j] = j * (2^(WINDOW*i)) * P

    During multiplication, one table entry per chunk is added up, i.e. 256/WINDOW point
    additions take place (instead of ~128 on average for a bit-by-bit approach).

    The table entries are stored as affine integer coordinates (or None for infinity), and the
    running sum is kept in Jacobian coordinates (x = X/Z^2, y = Y/Z^3) using plain integers,
    so that only a single modular inversion is needed at the end, instead of performing a full
    affine GE addition (with FE fraction arithmetic) for every table entry.
    """

    WINDOW = 4

    def __init__(self, p):
        self.table = []  # table[i][j] = j * (2^(WINDOW*i)) * p, as (x, y) integers or None
        for _ in range((256 + self.WINDOW - 1) // self.WINDOW):
            row = [GE()]
            for _ in range((1 << self.WINDOW) - 1):
                row.append(row[-1] + p)
            self.table.append([None if e.infinity else (int(e.x), int(e.y)) for e in row])
            for _ in range(self.WINDOW):
                p = p + p

    @staticmethod
    def _jacobian_double(x1, y1, z1):
        """Double a point in Jacobian coordinates (curve has a=0); (_, _, 0) is infinity."""
        # y1 == 0 cannot happen on secp256k1 (no points of order 2), kept for completeness.
        if y1 == 0 or z1 == 0:
            return 0, 1, 0
        m = FE.SIZE
        yy = y1 * y1 % m
        s = 4 * x1 * yy % m
        u = 3 * x1 * x1 % m
        x3 = (u * u - 2 * s) % m
        y3 = (u * (s - x3) - 8 * yy * yy) % m
        return x3, y3, 2 * y1 * z1 % m

    @staticmethod
    def _jacobian_add_affine(x1, y1, z1, x2, y2):
        """Add affine point (x2, y2) to Jacobian point (x1, y1, z1)."""
        if z1 == 0:
            return x2, y2, 1
        m = FE.SIZE
        z1z1 = z1 * z1 % m
        h = (x2 * z1z1 - x1) % m
        r = (y2 * z1 * z1z1 - y1) % m
        if h == 0:
            # Same x coordinate: either the same point (double) or its negation (infinity).
            return FastGEMul._jacobian_double(x1, y1, z1) if r == 0 else (0, 1, 0)
        hh = h * h % m
        hhh = h * hh % m
        v = x1 * hh % m
        x3 = (r * r - hhh - 2 * v) % m
        y3 = (r * (v - x3) - y1 * hhh) % m
        return x3, y3, z1 * h % m

    def mul(self, a):
        a = a % GE.ORDER
        mask = (1 << self.WINDOW) - 1
        x, y, z = 0, 1, 0  # Jacobian point at infinity
        for row in self.table:
            entry = row[a & mask]
            a >>= self.WINDOW
            if entry is not None:
                x, y, z = self._jacobian_add_affine(x, y, z, entry[0], entry[1])
        if z == 0:
            return GE()
        m = FE.SIZE
        z_inv = pow(z, -1, m)
        z_inv2 = z_inv * z_inv % m
        return GE(x * z_inv2 % m, y * z_inv2 * z_inv % m)

# Precomputed table with multiples of G for fast multiplication
FAST_G = FastGEMul(G)

@functools.lru_cache(maxsize=1024)
def mul_G(a):
    return FAST_G.mul(a)

class TestFrameworkSecp256k1(unittest.TestCase):
    def test_fast_mul(self):
        """Check FastGEMul against the generic GE.mul implementation."""
        def coords(p):
            return None if p.infinity else (int(p.x), int(p.y))
        H = GE.lift_x(0x50929b74c1a04954b78b4b6035e97a5e078a5a0f28ec96d547bfee9ace803ac0)
        fast_h = FastGEMul(H)
        scalars = [0, 1, 2, 15, 16, 17, 2**255, GE.ORDER - 1, GE.ORDER, GE.ORDER + 1, -1, 2**256 - 1]
        scalars += [int.from_bytes(sha256(i.to_bytes(4, 'big')).digest(), 'big') for i in range(32)]
        for a in scalars:
            self.assertEqual(coords(FAST_G.mul(a)), coords(GE.mul((a, G))))
            self.assertEqual(coords(mul_G(a % GE.ORDER)), coords(GE.mul((a, G))))
            self.assertEqual(coords(fast_h.mul(a)), coords(GE.mul((a, H))))

    def test_jacobian_degenerate(self):
        """Check the P + P and P + (-P) cases of the Jacobian helpers, unreachable via FastGEMul.mul."""
        m = FE.SIZE
        def to_affine(x, y, z):
            z_inv = pow(z, -1, m)
            return x * z_inv * z_inv % m, y * z_inv * z_inv * z_inv % m
        for k in (1, 7, 12345):
            P = GE.mul((k, G))
            P2 = GE.mul((2 * k, G))
            px, py = int(P.x), int(P.y)
            z = 0xdeadbeef + k
            jx, jy = px * z * z % m, py * z * z * z % m
            self.assertEqual(to_affine(*FastGEMul._jacobian_add_affine(jx, jy, z, px, py)), (int(P2.x), int(P2.y)))
            self.assertEqual(to_affine(*FastGEMul._jacobian_double(jx, jy, z)), (int(P2.x), int(P2.y)))
            self.assertEqual(FastGEMul._jacobian_add_affine(jx, jy, z, px, -py % m)[2], 0)
        self.assertEqual(FastGEMul._jacobian_double(0, 1, 0)[2], 0)

    def test_H(self):
        H = sha256(G.to_bytes_uncompressed()).digest()
        assert GE.lift_x(FE.from_bytes(H)) is not None
        self.assertEqual(H.hex(), "50929b74c1a04954b78b4b6035e97a5e078a5a0f28ec96d547bfee9ace803ac0")
