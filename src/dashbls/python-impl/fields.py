from __future__ import annotations

from copy import deepcopy
from typing import Any


class Fq:
    """
    Represents an element of a finite field mod a prime q.
    """

    value: int
    extension: int = 1

    def __init__(self, Q: int, value: int):
        self.Q = Q
        self.value = value % Q

    def __neg__(self) -> Fq:
        return Fq(self.Q, -self.value)

    def __add__(self, other: Fq) -> Fq:
        if not isinstance(other, Fq):
            return NotImplemented
        return Fq(self.Q, self.value + other.value)

    def __radd__(self, other: Fq) -> Fq:
        if not isinstance(other, Fq):
            return NotImplemented
        return self.__add__(other)

    def __sub__(self, other: Fq) -> Fq:
        if not isinstance(other, Fq):
            return NotImplemented
        return Fq(self.Q, self.value - other.value)

    def __rsub__(self, other: Fq) -> Fq:
        if not isinstance(other, Fq):
            return NotImplemented
        return Fq(self.Q, other.value - self.value)

    def __mul__(self, other: Fq) -> Fq:
        if not isinstance(other, Fq):
            return NotImplemented
        return Fq(self.Q, self.value * other.value)

    def __rmul__(self, other: Fq) -> Fq:
        return self.__mul__(other)

    def __eq__(self, other) -> bool:
        if not isinstance(other, type(self)):
            return False
        else:
            return self.value == other.value and self.Q == other.Q

    def __lt__(self, other: Fq) -> bool:
        return self.value < other.value

    def __gt__(self, other: Fq) -> bool:
        return self.value > other.value

    def __le__(self, other: Fq) -> bool:
        return self.value <= other.value

    def __ge__(self, other: Fq) -> bool:
        return self.value >= other.value

    def __str__(self):
        s = hex(self.value)
        s2 = s[0:7] + ".." + s[-5:] if len(s) > 10 else s
        return "Fq(" + s2 + ")"

    def __repr__(self):
        return "Fq(" + hex(self.value) + ")"

    def __bytes__(self):
        return self.value.to_bytes(48, "big")

    @staticmethod
    def from_bytes(buffer: bytes, q: int):
        assert len(buffer) == 48
        return Fq(q, int.from_bytes(buffer, "big"))

    def __pow__(self, other) -> Fq:
        if other == 0:
            return Fq(self.Q, 1)
        elif other == 1:
            return Fq(self.Q, self.value)
        elif other % 2 == 0:
            return Fq(self.Q, self.value * self.value) ** (other // 2)
        else:
            return Fq(self.Q, self.value * self.value) ** (other // 2) * self

    def qi_power(self, i: int) -> Fq:
        return self

    def __invert__(self) -> Fq:
        """
        Extended euclidian algorithm for inversion.
        """
        x0, x1, y0, y1 = 1, 0, 0, 1
        a = int(self.Q)
        b = int(self.value)
        while a != 0:
            q, b, a = b // a, a, b % a
            x0, x1 = x1, x0 - q * x1
            y0, y1 = y1, y0 - q * y1
        return Fq(self.Q, x0)

    def __floordiv__(self, other) -> Fq:
        if isinstance(other, int) and not isinstance(other, type(self)):
            other = Fq(self.Q, other)
        return self * ~other

    __truediv__ = __floordiv__

    def __iter__(self):
        yield self

    def modsqrt(self) -> Fq:
        if int(self.value) == 0:
            return Fq(self.Q, 0)
        if pow(int(self.value), (self.Q - 1) // 2, self.Q) != 1:
            raise ValueError("No sqrt exists")
        if self.Q % 4 == 3:
            return Fq(self.Q, pow(int(self.value), (self.Q + 1) // 4, self.Q))
        if self.Q % 8 == 5:
            return Fq(self.Q, pow(int(self.value), (self.Q + 3) // 8, self.Q))

        # p % 8 == 1. Tonelli Shanks algorithm for finding square root
        S = 0
        q = self.Q - 1

        while q % 2 == 0:
            q = q // 2
            S += 1

        z = 0
        for i in range(self.Q):
            euler = pow(i, (self.Q - 1) // 2, self.Q)
            if euler == -1 % self.Q:
                z = i
                break

        M = S
        c = pow(z, q, self.Q)
        t = pow(self.value, q, self.Q)
        R = pow(self.value, (q + 1) // 2, self.Q)

        while True:
            if t == 0:
                return Fq(self.Q, 0)
            if t == 1:
                return Fq(self.Q, R)
            i = 0
            f = t
            while f != 1:
                f = pow(f, 2, self.Q)
                i += 1
            b = pow(c, pow(2, M - i - 1, self.Q), self.Q)
            M = i
            c = pow(b, 2, self.Q)
            t = (t * c) % self.Q
            R = (R * b) % self.Q

    def __deepcopy__(self, memo) -> Fq:
        return Fq(self.Q, self.value)

    @classmethod
    def zero(cls, Q: int) -> Fq:
        return Fq(Q, 0)

    @classmethod
    def one(cls, Q: int) -> Fq:
        return Fq(Q, 1)

    @classmethod
    def from_fq(cls, Q: int, fq: Fq) -> Fq:
        return fq


class FieldExtBase(tuple):
    """
    Represents an extension of a field (or extension of an extension).
    The elements of the tuple can be other FieldExtBase or they can be
    Fq elements. For example, Fq2 = (Fq, Fq). Fq12 = (Fq6, Fq6), etc.
    """

    root = None
    extension: int
    embedding: int
    basefield: Any
    Q: int

    def __new__(cls, Q, *args):
        new_args = args[:]
        try:
            arg_extension = args[0].extension
            args[1].extension
        except AttributeError:
            if len(args) != 2:
                raise Exception("Invalid number of arguments")
            arg_extension = 1
            new_args = [Fq(Q, a) for a in args]
        if arg_extension != 1:
            if len(args) != cls.embedding:
                raise Exception("Invalid number of arguments")
            for arg in new_args:
                assert arg.extension == arg_extension
        assert all(isinstance(arg, cls.basefield) for arg in new_args)
        ret = super().__new__(cls, new_args)
        ret.Q = Q
        return ret

    def __neg__(self):
        cls = type(self)
        ret = super().__new__(cls, (-x for x in self))
        ret.Q = self.Q
        ret.root = self.root
        return ret

    def __add__(self, other):
        cls = type(self)
        if not isinstance(other, cls):
            if type(other) != int and other.extension > self.extension:
                return NotImplemented
            other_new = [cls.basefield.zero(self.Q) for _ in self]
            other_new[0] = other_new[0] + other
        else:
            other_new = other

        ret = super().__new__(cls, (a + b for a, b in zip(self, other_new)))
        ret.Q = self.Q
        ret.root = self.root
        return ret

    def __radd__(self, other):
        return self.__add__(other)

    def __sub__(self, other):
        return self + (-other)

    def __rsub__(self, other):
        return (-self) + other

    def __mul__(self, other):
        cls = type(self)
        if isinstance(other, int):
            ret = super().__new__(cls, (a * other for a in self))
            ret.Q = self.Q
            ret.root = self.root
            return ret
        if cls.extension < other.extension:
            return NotImplemented

        buf = [cls.basefield.zero(self.Q) for _ in self]

        for i, x in enumerate(self):
            if cls.extension == other.extension:
                for j, y in enumerate(other):
                    if x and y:
                        if i + j >= self.embedding:
                            buf[(i + j) % self.embedding] += x * y * self.root
                        else:
                            buf[(i + j) % self.embedding] += x * y
            else:
                if x:
                    buf[i] = x * other
        ret = super().__new__(cls, buf)
        ret.Q = self.Q
        ret.root = self.root
        return ret

    def __rmul__(self, other):
        return self.__mul__(other)

    def __floordiv__(self, other):
        return self * ~other

    def __eq__(self, other):
        if not isinstance(other, type(self)):
            if isinstance(other, FieldExtBase) or isinstance(other, int):
                if (
                    not isinstance(other, FieldExtBase)
                    or self.extension > other.extension
                ):
                    for i in range(1, self.embedding):
                        if self[i] != (type(self.root).zero(self.Q)):
                            return False
                    return self[0] == other
                return NotImplemented
            return NotImplemented
        else:
            return super().__eq__(other) and self.Q == other.Q

    def __lt__(self, other):
        # Reverse the order for comparison (3i + 1 > 2i + 7)
        return self[::-1].__lt__(other[::-1])

    def __gt__(self, other):
        return super().__gt__(other)

    def __neq__(self, other):
        return not self.__eq__(other)

    def __str__(self):
        return (
            "Fq"
            + str(self.extension)
            + "("
            + ", ".join([a.__str__() for a in self])
            + ")"
        )

    def __repr__(self):
        return (
            "Fq"
            + str(self.extension)
            + "("
            + ", ".join([a.__repr__() for a in self])
            + ")"
        )

    # Returns the concatenated coordinates in big endian bytes
    def __bytes__(self):
        sum_bytes = bytes([])
        for x in reversed(self):
            if type(x) != FieldExtBase and type(x) != Fq:
                x = Fq.from_fq(self.Q, x)
            sum_bytes += bytes(x)
        return sum_bytes

    @classmethod
    def from_bytes(cls, buffer: bytes, Q: int):
        assert len(buffer) == cls.extension * 48
        embedded_size = 48 * (cls.extension // cls.embedding)
        tup = []
        for i in range(cls.embedding):
            tup.append(buffer[i * embedded_size : (i + 1) * embedded_size])
        return cls(Q, *[cls.basefield.from_bytes(b, Q) for b in reversed(tup)])

    __truediv__ = __floordiv__

    def __pow__(self, e):
        assert isinstance(e, int) and e >= 0
        ans = type(self).one(self.Q)
        base = self
        ans.root = self.root

        while e:
            if e & 1:
                ans *= base

            base *= base
            e >>= 1

        return ans

    def __bool__(self):
        return any(x for x in self)

    def set_root(self, _root):
        self.root = _root

    @classmethod
    def zero(cls, Q):
        return cls.from_fq(Q, Fq(Q, 0))

    @classmethod
    def one(cls, Q):
        return cls.from_fq(Q, Fq(Q, 1))

    @classmethod
    def from_fq(cls, Q, fq):
        y = cls.basefield.from_fq(Q, fq)
        z = cls.basefield.zero(Q)
        ret = super().__new__(cls, (z if i else y for i in range(cls.embedding)))
        ret.Q = Q
        if cls == Fq2:
            ret.set_root(Fq(Q, -1))
        elif cls == Fq6:
            ret.set_root(Fq2(Q, Fq.one(Q), Fq.one(Q)))
        elif cls == Fq12:
            r = Fq6(Q, Fq2.zero(Q), Fq2.one(Q), Fq2.zero(Q))
            ret.set_root(r)
        return ret

    def __deepcopy__(self, memo):
        cls = type(self)
        ret = super().__new__(cls, (deepcopy(a, memo) for a in self))
        ret.Q = self.Q
        ret.root = self.root
        return ret

    def qi_power(self, i):
        if self.Q != bls12381_q:
            raise NotImplementedError
        cls = type(self)
        i %= cls.extension
        if i == 0:
            return self
        ret = super().__new__(
            cls,
            (
                a.qi_power(i) * frob_coeffs[cls.extension, i, j] if j else a.qi_power(i)
                for j, a in enumerate(self)
            ),
        )
        ret.Q = self.Q
        ret.root = self.root
        return ret


class Fq2(FieldExtBase):
    # Fq2 is constructed as Fq(u) / (u2 - β) where β = -1
    extension = 2
    embedding = 2
    basefield = Fq

    def __init__(self, Q, *args):
        super().set_root(Fq(Q, -1))

    def __invert__(self) -> Fq2:
        a, b = self
        factor = ~(a * a + b * b)
        ret = Fq2(self.Q, a * factor, -b * factor)
        return ret

    def mul_by_nonresidue(self) -> Fq2:
        # multiply by u + 1
        a, b = self
        return Fq2(self.Q, a - b, a + b)

    def modsqrt(self) -> Fq2:
        """
        Using algorithm 8 (complex method) for square roots in
        https://eprint.iacr.org/2012/685.pdf
        This is necessary for computing y value given an x value.
        """
        a0, a1 = self
        if a1 == Fq.zero(self.Q):
            return a0.modsqrt()
        alpha = pow(a0, 2) + pow(a1, 2)
        gamma = pow(alpha, (self.Q - 1) // 2)
        if gamma == Fq(self.Q, -1):
            raise ValueError("No sqrt exists")
        alpha = alpha.modsqrt()
        delta = (a0 + alpha) * ~Fq(self.Q, 2)
        gamma = pow(delta, (self.Q - 1) // 2)
        if gamma == Fq(self.Q, -1):
            delta = (a0 - alpha) * ~Fq(self.Q, 2)

        x0 = delta.modsqrt()
        x1 = a1 * ~(Fq(self.Q, 2) * x0)
        return Fq2(self.Q, x0, x1)


class Fq6(FieldExtBase):
    # Fq6 is constructed as Fq2(v) / (v3 - ξ) where ξ = u + 1
    extension = 6
    embedding = 3
    basefield = Fq2

    def __init__(self, Q: int, *args):
        super().set_root(Fq2(Q, Fq.one(Q), Fq.one(Q)))

    def __invert__(self) -> Fq6:
        a, b, c = self
        g0 = a * a - b * c.mul_by_nonresidue()
        g1 = (c * c).mul_by_nonresidue() - a * b
        g2 = b * b - a * c
        factor = ~(g0 * a + (g1 * c + g2 * b).mul_by_nonresidue())
        # TODO: no inverse

        return Fq6(self.Q, g0 * factor, g1 * factor, g2 * factor)

    def mul_by_nonresidue(self) -> Fq6:
        # multiply by v
        a, b, c = self
        return Fq6(self.Q, c * self.root, a, b)


class Fq12(FieldExtBase):
    # Fq12 is constructed as Fq6(w) / (w2 - γ) where γ = v
    extension = 12
    embedding = 2
    basefield = Fq6

    def __init__(self, Q, *args):
        super().set_root(Fq6(Q, Fq2.zero(Q), Fq2.one(Q), Fq2.zero(Q)))

    def __invert__(self) -> Fq12:
        a, b = self
        factor = ~(a * a - (b * b).mul_by_nonresidue())
        return Fq12(self.Q, a * factor, -b * factor)


# Because fields aren't done with metaclasses, and we need to
# avoid circular imports, we put a hack here for bls12381 for now.
bls12381_q = (
    q
) = 0x1A0111EA397FE69A4B1BA7B6434BACD764774B84F38512BF6730D2A0F6B0F6241EABFFFEB153FFFFB9FEFFFFFFFFAAAB

# roots of unity, used for computing square roots in Fq2
rv1 = 0x6AF0E0437FF400B6831E36D6BD17FFE48395DABC2D3435E77F76E17009241C5EE67992F72EC05F4C81084FBEDE3CC09
roots_of_unity = (Fq2(q, 1, 0), Fq2(q, 0, 1), Fq2(q, rv1, rv1), Fq2(q, rv1, q - rv1))
del rv1

# Frobenius coefficients for raising elements to q**i -th powers
# These are specific to this given q
frob_coeffs = {
    (2, 1, 1): Fq(q, -1),
    (6, 1, 1): Fq2(
        q,
        Fq(q, 0x0),
        Fq(
            q,
            0x1A0111EA397FE699EC02408663D4DE85AA0D857D89759AD4897D29650FB85F9B409427EB4F49FFFD8BFD00000000AAAC,
        ),
    ),  # noga: E501
    (6, 1, 2): Fq2(
        q,
        Fq(
            q,
            0x1A0111EA397FE699EC02408663D4DE85AA0D857D89759AD4897D29650FB85F9B409427EB4F49FFFD8BFD00000000AAAD,
        ),
        Fq(q, 0x0),
    ),  # noga: E501
    (6, 2, 1): Fq2(
        q,
        Fq(
            q,
            0x5F19672FDF76CE51BA69C6076A0F77EADDB3A93BE6F89688DE17D813620A00022E01FFFFFFFEFFFE,
        ),
        Fq(q, 0x0),
    ),  # noga: E501
    (6, 2, 2): Fq2(
        q,
        Fq(
            q,
            0x1A0111EA397FE699EC02408663D4DE85AA0D857D89759AD4897D29650FB85F9B409427EB4F49FFFD8BFD00000000AAAC,
        ),
        Fq(q, 0x0),
    ),
    (6, 3, 1): Fq2(q, Fq(q, 0x0), Fq(q, 0x1)),
    (6, 3, 2): Fq2(
        q,
        Fq(
            q,
            0x1A0111EA397FE69A4B1BA7B6434BACD764774B84F38512BF6730D2A0F6B0F6241EABFFFEB153FFFFB9FEFFFFFFFFAAAA,
        ),
        Fq(q, 0x0),
    ),
    (6, 4, 1): Fq2(
        q,
        Fq(
            q,
            0x1A0111EA397FE699EC02408663D4DE85AA0D857D89759AD4897D29650FB85F9B409427EB4F49FFFD8BFD00000000AAAC,
        ),
        Fq(q, 0x0),
    ),
    (6, 4, 2): Fq2(
        q,
        Fq(
            q,
            0x5F19672FDF76CE51BA69C6076A0F77EADDB3A93BE6F89688DE17D813620A00022E01FFFFFFFEFFFE,
        ),
        Fq(q, 0x0),
    ),
    (6, 5, 1): Fq2(
        q,
        Fq(q, 0x0),
        Fq(
            q,
            0x5F19672FDF76CE51BA69C6076A0F77EADDB3A93BE6F89688DE17D813620A00022E01FFFFFFFEFFFE,
        ),
    ),
    (6, 5, 2): Fq2(
        q,
        Fq(
            q,
            0x5F19672FDF76CE51BA69C6076A0F77EADDB3A93BE6F89688DE17D813620A00022E01FFFFFFFEFFFF,
        ),
        Fq(q, 0x0),
    ),
    (12, 1, 1): Fq6(
        q,
        Fq2(
            q,
            Fq(
                q,
                0x1904D3BF02BB0667C231BEB4202C0D1F0FD603FD3CBD5F4F7B2443D784BAB9C4F67EA53D63E7813D8D0775ED92235FB8,
            ),
            Fq(
                q,
                0xFC3E2B36C4E03288E9E902231F9FB854A14787B6C7B36FEC0C8EC971F63C5F282D5AC14D6C7EC22CF78A126DDC4AF3,
            ),
        ),
        Fq2(q, Fq(q, 0x0), Fq(q, 0x0)),
        Fq2(q, Fq(q, 0x0), Fq(q, 0x0)),
    ),
    (12, 2, 1): Fq6(
        q,
        Fq2(
            q,
            Fq(
                q,
                0x5F19672FDF76CE51BA69C6076A0F77EADDB3A93BE6F89688DE17D813620A00022E01FFFFFFFEFFFF,
            ),
            Fq(q, 0x0),
        ),
        Fq2(q, Fq(q, 0x0), Fq(q, 0x0)),
        Fq2(q, Fq(q, 0x0), Fq(q, 0x0)),
    ),
    (12, 3, 1): Fq6(
        q,
        Fq2(
            q,
            Fq(
                q,
                0x135203E60180A68EE2E9C448D77A2CD91C3DEDD930B1CF60EF396489F61EB45E304466CF3E67FA0AF1EE7B04121BDEA2,
            ),
            Fq(
                q,
                0x6AF0E0437FF400B6831E36D6BD17FFE48395DABC2D3435E77F76E17009241C5EE67992F72EC05F4C81084FBEDE3CC09,
            ),
        ),
        Fq2(q, Fq(q, 0x0), Fq(q, 0x0)),
        Fq2(q, Fq(q, 0x0), Fq(q, 0x0)),
    ),
    (12, 4, 1): Fq6(
        q,
        Fq2(
            q,
            Fq(
                q,
                0x5F19672FDF76CE51BA69C6076A0F77EADDB3A93BE6F89688DE17D813620A00022E01FFFFFFFEFFFE,
            ),
            Fq(q, 0x0),
        ),
        Fq2(q, Fq(q, 0x0), Fq(q, 0x0)),
        Fq2(q, Fq(q, 0x0), Fq(q, 0x0)),
    ),
    (12, 5, 1): Fq6(
        q,
        Fq2(
            q,
            Fq(
                q,
                0x144E4211384586C16BD3AD4AFA99CC9170DF3560E77982D0DB45F3536814F0BD5871C1908BD478CD1EE605167FF82995,
            ),
            Fq(
                q,
                0x5B2CFD9013A5FD8DF47FA6B48B1E045F39816240C0B8FEE8BEADF4D8E9C0566C63A3E6E257F87329B18FAE980078116,
            ),
        ),
        Fq2(q, Fq(q, 0x0), Fq(q, 0x0)),
        Fq2(q, Fq(q, 0x0), Fq(q, 0x0)),
    ),
    (12, 6, 1): Fq6(
        q,
        Fq2(
            q,
            Fq(
                q,
                0x1A0111EA397FE69A4B1BA7B6434BACD764774B84F38512BF6730D2A0F6B0F6241EABFFFEB153FFFFB9FEFFFFFFFFAAAA,
            ),
            Fq(q, 0x0),
        ),
        Fq2(q, Fq(q, 0x0), Fq(q, 0x0)),
        Fq2(q, Fq(q, 0x0), Fq(q, 0x0)),
    ),
    (12, 7, 1): Fq6(
        q,
        Fq2(
            q,
            Fq(
                q,
                0xFC3E2B36C4E03288E9E902231F9FB854A14787B6C7B36FEC0C8EC971F63C5F282D5AC14D6C7EC22CF78A126DDC4AF3,
            ),
            Fq(
                q,
                0x1904D3BF02BB0667C231BEB4202C0D1F0FD603FD3CBD5F4F7B2443D784BAB9C4F67EA53D63E7813D8D0775ED92235FB8,
            ),
        ),
        Fq2(q, Fq(q, 0x0), Fq(q, 0x0)),
        Fq2(q, Fq(q, 0x0), Fq(q, 0x0)),
    ),
    (12, 8, 1): Fq6(
        q,
        Fq2(
            q,
            Fq(
                q,
                0x1A0111EA397FE699EC02408663D4DE85AA0D857D89759AD4897D29650FB85F9B409427EB4F49FFFD8BFD00000000AAAC,
            ),
            Fq(q, 0x0),
        ),
        Fq2(q, Fq(q, 0x0), Fq(q, 0x0)),
        Fq2(q, Fq(q, 0x0), Fq(q, 0x0)),
    ),
    (12, 9, 1): Fq6(
        q,
        Fq2(
            q,
            Fq(
                q,
                0x6AF0E0437FF400B6831E36D6BD17FFE48395DABC2D3435E77F76E17009241C5EE67992F72EC05F4C81084FBEDE3CC09,
            ),
            Fq(
                q,
                0x135203E60180A68EE2E9C448D77A2CD91C3DEDD930B1CF60EF396489F61EB45E304466CF3E67FA0AF1EE7B04121BDEA2,
            ),
        ),
        Fq2(q, Fq(q, 0x0), Fq(q, 0x0)),
        Fq2(q, Fq(q, 0x0), Fq(q, 0x0)),
    ),
    (12, 10, 1): Fq6(
        q,
        Fq2(
            q,
            Fq(
                q,
                0x1A0111EA397FE699EC02408663D4DE85AA0D857D89759AD4897D29650FB85F9B409427EB4F49FFFD8BFD00000000AAAD,
            ),
            Fq(q, 0x0),
        ),
        Fq2(q, Fq(q, 0x0), Fq(q, 0x0)),
        Fq2(q, Fq(q, 0x0), Fq(q, 0x0)),
    ),
    (12, 11, 1): Fq6(
        q,
        Fq2(
            q,
            Fq(
                q,
                0x5B2CFD9013A5FD8DF47FA6B48B1E045F39816240C0B8FEE8BEADF4D8E9C0566C63A3E6E257F87329B18FAE980078116,
            ),
            Fq(
                q,
                0x144E4211384586C16BD3AD4AFA99CC9170DF3560E77982D0DB45F3536814F0BD5871C1908BD478CD1EE605167FF82995,
            ),
        ),
        Fq2(q, Fq(q, 0x0), Fq(q, 0x0)),
        Fq2(q, Fq(q, 0x0), Fq(q, 0x0)),
    ),
}

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
