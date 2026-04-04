# Copyright (c) 2022-2023 The Bitcoin Core developers
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

# TODO Docstrings of methods still say "field element"
class APrimeFE:
    """Objects of this class represent elements of a prime field.

    They are represented internally in numerator / denominator form, in order to delay inversions.
    """

    # The size of the field (also its modulus and characteristic).
    SIZE: int

    def __init__(self, a=0, b=1):
        """Initialize a field element a/b; both a and b can be ints or field elements."""
        if isinstance(a, type(self)):
            num = a._num
            den = a._den
        else:
            num = a % self.SIZE
            den = 1
        if isinstance(b, type(self)):
            den = (den * b._num) % self.SIZE
            num = (num * b._den) % self.SIZE
        else:
            den = (den * b) % self.SIZE
        assert den != 0
        if num == 0:
            den = 1
        self._num = num
        self._den = den

    def __add__(self, a):
        """Compute the sum of two field elements (second may be int)."""
        if isinstance(a, type(self)):
            return type(self)(self._num * a._den + self._den * a._num, self._den * a._den)
        if isinstance(a, int):
            return type(self)(self._num + self._den * a, self._den)
        return NotImplemented

    def __radd__(self, a):
        """Compute the sum of an integer and a field element."""
        return type(self)(a) + self

    @classmethod
    # REVIEW This should be
    #    def sum(cls, *es: Iterable[Self]) -> Self:
    # but Self needs the typing_extension package on Python <= 3.12.
    def sum(cls, *es):
        """Compute the sum of field elements.

        sum(a, b, c, ...) is identical to (0 + a + b + c + ...)."""
        return sum(es, start=cls(0))

    def __sub__(self, a):
        """Compute the difference of two field elements (second may be int)."""
        if isinstance(a, type(self)):
            return type(self)(self._num * a._den - self._den * a._num, self._den * a._den)
        if isinstance(a, int):
            return type(self)(self._num - self._den * a, self._den)
        return NotImplemented

    def __rsub__(self, a):
        """Compute the difference of an integer and a field element."""
        return type(self)(a) - self

    def __mul__(self, a):
        """Compute the product of two field elements (second may be int)."""
        if isinstance(a, type(self)):
            return type(self)(self._num * a._num, self._den * a._den)
        if isinstance(a, int):
            return type(self)(self._num * a, self._den)
        return NotImplemented

    def __rmul__(self, a):
        """Compute the product of an integer with a field element."""
        return type(self)(a) * self

    def __truediv__(self, a):
        """Compute the ratio of two field elements (second may be int)."""
        if isinstance(a, type(self)) or isinstance(a, int):
            return type(self)(self, a)
        return NotImplemented

    def __pow__(self, a):
        """Raise a field element to an integer power."""
        return type(self)(pow(self._num, a, self.SIZE), pow(self._den, a, self.SIZE))

    def __neg__(self):
        """Negate a field element."""
        return type(self)(-self._num, self._den)

    def __int__(self):
        """Convert a field element to an integer in range 0..SIZE-1. The result is cached."""
        if self._den != 1:
            self._num = (self._num * pow(self._den, -1, self.SIZE)) % self.SIZE
            self._den = 1
        return self._num

    def sqrt(self):
        """Compute the square root of a field element if it exists (None otherwise)."""
        raise NotImplementedError

    def is_square(self):
        """Determine if this field element has a square root."""
        # A more efficient algorithm is possible here (Jacobi symbol).
        return self.sqrt() is not None

    def is_even(self):
        """Determine whether this field element, represented as integer in 0..SIZE-1, is even."""
        return int(self) & 1 == 0

    def __eq__(self, a):
        """Check whether two field elements are equal (second may be an int)."""
        if isinstance(a, type(self)):
            return (self._num * a._den - self._den * a._num) % self.SIZE == 0
        return (self._num - self._den * a) % self.SIZE == 0

    def to_bytes(self):
        """Convert a field element to a 32-byte array (BE byte order)."""
        return int(self).to_bytes(32, 'big')

    @classmethod
    def from_int_checked(cls, v):
        """Convert an integer to a field element (no overflow allowed)."""
        if v >= cls.SIZE:
            raise ValueError
        return cls(v)

    @classmethod
    def from_int_wrapping(cls, v):
        """Convert an integer to a field element (reduced modulo SIZE)."""
        return cls(v % cls.SIZE)

    @classmethod
    def from_bytes_checked(cls, b):
        """Convert a 32-byte array to a field element (BE byte order, no overflow allowed)."""
        v = int.from_bytes(b, 'big')
        return cls.from_int_checked(v)

    @classmethod
    def from_bytes_wrapping(cls, b):
        """Convert a 32-byte array to a field element (BE byte order, reduced modulo SIZE)."""
        v = int.from_bytes(b, 'big')
        return cls.from_int_wrapping(v)

    def __str__(self):
        """Convert this field element to a 64 character hex string."""
        return f"{int(self):064x}"

    def __repr__(self):
        """Get a string representation of this field element."""
        return f"{type(self).__qualname__}(0x{int(self):x})"


class FE(APrimeFE):
    SIZE = 2**256 - 2**32 - 977

    def sqrt(self):
        # Due to the fact that our modulus p is of the form (p % 4) == 3, the Tonelli-Shanks
        # algorithm (https://en.wikipedia.org/wiki/Tonelli-Shanks_algorithm) is simply
        # raising the argument to the power (p + 1) / 4.

        # To see why: (p-1) % 2 = 0, so 2 divides the order of the multiplicative group,
        # and thus only half of the non-zero field elements are squares. An element a is
        # a (nonzero) square when Euler's criterion, a^((p-1)/2) = 1 (mod p), holds. We're
        # looking for x such that x^2 = a (mod p). Given a^((p-1)/2) = 1, that is equivalent
        # to x^2 = a^(1 + (p-1)/2) mod p. As (1 + (p-1)/2) is even, this is equivalent to
        # x = a^((1 + (p-1)/2)/2) mod p, or x = a^((p+1)/4) mod p.
        v = int(self)
        s = pow(v, (self.SIZE + 1) // 4, self.SIZE)
        if s**2 % self.SIZE == v:
            return type(self)(s)
        return None


class Scalar(APrimeFE):
    """TODO Docstring"""
    SIZE = 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141

    @classmethod
    def from_int_nonzero_checked(cls, v):
        """Convert an integer to a scalar (no zero or overflow allowed)."""
        if not (0 < v < cls.SIZE):
            raise ValueError
        return cls(v)

    @classmethod
    def from_bytes_nonzero_checked(cls, b):
        """Convert a 32-byte array to a scalar (BE byte order, no zero or overflow allowed)."""
        v = int.from_bytes(b, 'big')
        return cls.from_int_nonzero_checked(v)


class GE:
    """Objects of this class represent secp256k1 group elements (curve points or infinity)

    GE objects are immutable.

    Normal points on the curve have fields:
    * x: the x coordinate (a field element)
    * y: the y coordinate (a field element, satisfying y^2 = x^3 + 7)
    * infinity: False

    The point at infinity has field:
    * infinity: True
    """

    # TODO The following two class attributes should probably be just getters as
    # classmethods to enforce immutability. Unfortunately Python makes it hard
    # to create "classproperties". `G` could then also be just a classmethod.

    # Order of the group (number of points on the curve, plus 1 for infinity)
    ORDER = Scalar.SIZE

    # Number of valid distinct x coordinates on the curve.
    ORDER_HALF = ORDER // 2

    @property
    def infinity(self):
        """Whether the group element is the point at infinity."""
        return self._infinity

    @property
    def x(self):
        """The x coordinate (a field element) of a non-infinite group element."""
        assert not self.infinity
        return self._x

    @property
    def y(self):
        """The y coordinate (a field element) of a non-infinite group element."""
        assert not self.infinity
        return self._y

    def __init__(self, x=None, y=None):
        """Initialize a group element with specified x and y coordinates, or infinity."""
        if x is None:
            # Initialize as infinity.
            assert y is None
            self._infinity = True
        else:
            # Initialize as point on the curve (and check that it is).
            fx = FE(x)
            fy = FE(y)
            assert fy**2 == fx**3 + 7
            self._infinity = False
            self._x = fx
            self._y = fy

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
                assert self.y + a.y == 0
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
    def sum(*ps):
        """Compute the sum of group elements.

        GE.sum(a, b, c, ...) is identical to (GE() + a + b + c + ...)."""
        return sum(ps, start=GE())

    @staticmethod
    def batch_mul(*aps):
        """Compute a (batch) scalar group element multiplication.

        GE.batch_mul((a1, p1), (a2, p2), (a3, p3)) is identical to a1*p1 + a2*p2 + a3*p3,
        but more efficient."""
        # Reduce all the scalars modulo order first (so we can deal with negatives etc).
        naps = [(int(a), p) for a, p in aps]
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
            return FAST_G.mul(Scalar(a))
        return GE.batch_mul((Scalar(a), self))

    def __neg__(self):
        """Compute the negation of a group element."""
        if self.infinity:
            return self
        return GE(self.x, -self.y)

    def __sub__(self, a):
        """Subtract a group element from another."""
        return self + (-a)

    def __eq__(self, a):
        """Check if two group elements are equal."""
        return (self - a).infinity

    def has_even_y(self):
        """Determine whether a non-infinity group element has an even y coordinate."""
        assert not self.infinity
        return self.y.is_even()

    def to_bytes_compressed(self):
        """Convert a non-infinite group element to 33-byte compressed encoding."""
        assert not self.infinity
        return bytes([3 - self.y.is_even()]) + self.x.to_bytes()

    def to_bytes_compressed_with_infinity(self):
        """Convert a group element to 33-byte compressed encoding, mapping infinity to zeros."""
        if self.infinity:
            return 33 * b"\x00"
        return self.to_bytes_compressed()

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
            raise ValueError
        if not y.is_even():
            y = -y
        return GE(x, y)

    @staticmethod
    def from_bytes_compressed(b):
        """Convert a compressed to a group element."""
        assert len(b) == 33
        if b[0] != 2 and b[0] != 3:
            raise ValueError
        x = FE.from_bytes_checked(b[1:])
        r = GE.lift_x(x)
        if b[0] == 3:
            r = -r
        return r

    @staticmethod
    def from_bytes_compressed_with_infinity(b):
        """Convert a compressed to a group element, mapping zeros to infinity."""
        if b == 33 * b"\x00":
            return GE()
        else:
            return GE.from_bytes_compressed(b)

    @staticmethod
    def from_bytes_uncompressed(b):
        """Convert an uncompressed to a group element."""
        assert len(b) == 65
        if b[0] != 4:
            raise ValueError
        x = FE.from_bytes_checked(b[1:33])
        y = FE.from_bytes_checked(b[33:])
        if y**2 != x**3 + 7:
            raise ValueError
        return GE(x, y)

    @staticmethod
    def from_bytes(b):
        """Convert a compressed or uncompressed encoding to a group element."""
        assert len(b) in (33, 65)
        if len(b) == 33:
            return GE.from_bytes_compressed(b)
        else:
            return GE.from_bytes_uncompressed(b)

    @staticmethod
    def from_bytes_xonly(b):
        """Convert a point given in xonly encoding to a group element."""
        assert len(b) == 32
        x = FE.from_bytes_checked(b)
        r = GE.lift_x(x)
        return r

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

    def __hash__(self):
        """Compute a non-cryptographic hash of the group element."""
        if self.infinity:
            return 0  # 0 is not a valid x coordinate
        return int(self.x)


# The secp256k1 generator point
G = GE.lift_x(0x79BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798)


class FastGEMul:
    """Table for fast multiplication with a constant group element.

    Speed up scalar multiplication with a fixed point P by using a precomputed lookup table with
    its powers of 2:

        table = [P, 2*P, 4*P, (2^3)*P, (2^4)*P, ..., (2^255)*P]

    During multiplication, the points corresponding to each bit set in the scalar are added up,
    i.e. on average ~128 point additions take place.
    """

    def __init__(self, p):
        self.table = [p]  # table[i] = (2^i) * p
        for _ in range(255):
            p = p + p
            self.table.append(p)

    def mul(self, a):
        result = GE()
        a = int(a)
        for bit in range(a.bit_length()):
            if a & (1 << bit):
                result += self.table[bit]
        return result

# Precomputed table with multiples of G for fast multiplication
FAST_G = FastGEMul(G)
