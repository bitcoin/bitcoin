#!/usr/bin/env python3
# Copyright (c) 2020 Pieter Wuille
# Distributed under the MIT software license, see the accompanying
# file LICENSE or http://www.opensource.org/licenses/mit-license.php.

"""Native Python (slow) reimplementation of libminisketch' algorithms."""

import random
import unittest

# Irreducible polynomials over GF(2) to use (represented as integers).
#
# Most fields can be defined by multiple such polynomials. Minisketch uses the one with the minimal
# number of nonzero coefficients, and tie-breaking by picking the lexicographically first among
# those.
#
# All polynomials for degrees 2 through 64 (inclusive) are given.
GF2_MODULI = [
    None, None,
    2**2 + 2**1 + 1,
    2**3 + 2**1 + 1,
    2**4 + 2**1 + 1,
    2**5 + 2**2 + 1,
    2**6 + 2**1 + 1,
    2**7 + 2**1 + 1,
    2**8 + 2**4 + 2**3 + 2**1 + 1,
    2**9 + 2**1 + 1,
    2**10 + 2**3 + 1,
    2**11 + 2**2 + 1,
    2**12 + 2**3 + 1,
    2**13 + 2**4 + 2**3 + 2**1 + 1,
    2**14 + 2**5 + 1,
    2**15 + 2**1 + 1,
    2**16 + 2**5 + 2**3 + 2**1 + 1,
    2**17 + 2**3 + 1,
    2**18 + 2**3 + 1,
    2**19 + 2**5 + 2**2 + 2**1 + 1,
    2**20 + 2**3 + 1,
    2**21 + 2**2 + 1,
    2**22 + 2**1 + 1,
    2**23 + 2**5 + 1,
    2**24 + 2**4 + 2**3 + 2**1 + 1,
    2**25 + 2**3 + 1,
    2**26 + 2**4 + 2**3 + 2**1 + 1,
    2**27 + 2**5 + 2**2 + 2**1 + 1,
    2**28 + 2**1 + 1,
    2**29 + 2**2 + 1,
    2**30 + 2**1 + 1,
    2**31 + 2**3 + 1,
    2**32 + 2**7 + 2**3 + 2**2 + 1,
    2**33 + 2**10 + 1,
    2**34 + 2**7 + 1,
    2**35 + 2**2 + 1,
    2**36 + 2**9 + 1,
    2**37 + 2**6 + 2**4 + 2**1 + 1,
    2**38 + 2**6 + 2**5 + 2**1 + 1,
    2**39 + 2**4 + 1,
    2**40 + 2**5 + 2**4 + 2**3 + 1,
    2**41 + 2**3 + 1,
    2**42 + 2**7 + 1,
    2**43 + 2**6 + 2**4 + 2**3 + 1,
    2**44 + 2**5 + 1,
    2**45 + 2**4 + 2**3 + 2**1 + 1,
    2**46 + 2**1 + 1,
    2**47 + 2**5 + 1,
    2**48 + 2**5 + 2**3 + 2**2 + 1,
    2**49 + 2**9 + 1,
    2**50 + 2**4 + 2**3 + 2**2 + 1,
    2**51 + 2**6 + 2**3 + 2**1 + 1,
    2**52 + 2**3 + 1,
    2**53 + 2**6 + 2**2 + 2**1 + 1,
    2**54 + 2**9 + 1,
    2**55 + 2**7 + 1,
    2**56 + 2**7 + 2**4 + 2**2 + 1,
    2**57 + 2**4 + 1,
    2**58 + 2**19 + 1,
    2**59 + 2**7 + 2**4 + 2**2 + 1,
    2**60 + 2**1 + 1,
    2**61 + 2**5 + 2**2 + 2**1 + 1,
    2**62 + 2**29 + 1,
    2**63 + 2**1 + 1,
    2**64 + 2**4 + 2**3 + 2**1 + 1
]

class GF2Ops:
    """Class to perform GF(2^field_size) operations on elements represented as integers.

    Given that elements are represented as integers, addition is simply xor, and not
    exposed here.
    """

    def __init__(self, field_size):
        """Construct a GF2Ops object for the specified field size."""
        self.field_size = field_size
        self._modulus = GF2_MODULI[field_size]
        assert self._modulus is not None

    def mul2(self, x):
        """Multiply x by 2 in GF(2^field_size)."""
        x <<= 1
        if x >> self.field_size:
            x ^= self._modulus
        return x

    def mul(self, x, y):
        """Multiply x by y in GF(2^field_size)."""
        ret = 0
        while y:
            if y & 1:
                ret ^= x
            y >>= 1
            x = self.mul2(x)
        return ret

    def sqr(self, x):
        """Square x in GF(2^field_size)."""
        return self.mul(x, x)

    def inv(self, x):
        """Compute the inverse of x in GF(2^field_size)."""
        assert x != 0
        # Use the extended polynomial Euclidean GCD algorithm on (modulus, x), over GF(2).
        # See https://en.wikipedia.org/wiki/Polynomial_greatest_common_divisor.
        t1, t2 = 0, 1
        r1, r2 = self._modulus, x
        r1l, r2l = self.field_size + 1, r2.bit_length()
        while r2:
            q = r1l - r2l
            r1 ^= r2 << q
            t1 ^= t2 << q
            r1l = r1.bit_length()
            if r1 < r2:
                t1, t2 = t2, t1
                r1, r2 = r2, r1
                r1l, r2l = r2l, r1l
        assert r1 == 1
        return t1

class TestGF2Ops(unittest.TestCase):
    """Test class for basic arithmetic properties of GF2Ops."""

    def field_size_test(self, field_size):
        """Test operations for given field_size."""

        gf = GF2Ops(field_size)
        for i in range(100):
            x = random.randrange(1 << field_size)
            y = random.randrange(1 << field_size)
            x2 = gf.mul2(x)
            xy = gf.mul(x, y)
            self.assertEqual(x2, gf.mul(x, 2)) # mul2(x) == x*2
            self.assertEqual(x2, gf.mul(2, x)) # mul2(x) == 2*x
            self.assertEqual(xy == 0, x == 0 or y == 0)
            self.assertEqual(xy == x, y == 1 or x == 0)
            self.assertEqual(xy == y, x == 1 or y == 0)
            self.assertEqual(xy, gf.mul(y, x)) # x*y == y*x
            if i < 10:
                xp = x
                for _ in range(field_size):
                    xp = gf.sqr(xp)
                self.assertEqual(xp, x) # x^(2^field_size) == x
            if y != 0:
                yi = gf.inv(y)
                self.assertEqual(y == yi, y == 1) # y==1/x iff y==1
                self.assertEqual(gf.mul(y, yi), 1) # y*(1/y) == 1
                yii = gf.inv(yi)
                self.assertEqual(y, yii) # 1/(1/y) == y
                if x != 0:
                    xi = gf.inv(x)
                    xyi = gf.inv(xy)
                    self.assertEqual(xyi, gf.mul(xi, yi)) # (1/x)*(1/y) == 1/(x*y)

    def test(self):
        """Run tests."""
        for field_size in range(2, 65):
            self.field_size_test(field_size)

# The operations below operate on polynomials over GF(2^field_size), represented as lists of
# integers:
#
#   [a, b, c, ...] = a + b*x + c*x^2 + ...
#
# As an invariant, there are never any trailing zeroes in the list representation.
#
# Examples:
# * [] = 0
# * [3] = 3
# * [0, 1] = x
# * [2, 0, 5] = 5*x^2 + 2

def poly_monic(poly, gf):
    """Return a monic version of the polynomial poly."""
    # Multiply every coefficient with the inverse of the top coefficient.
    inv = gf.inv(poly[-1])
    return [gf.mul(inv, v) for v in poly]

def poly_divmod(poly, mod, gf):
    """Return the polynomial (quotient, remainder) of poly divided by mod."""
    assert len(mod) > 0 and mod[-1] == 1 # Require monic mod.
    if len(poly) < len(mod):
        return ([], poly)
    val = list(poly)
    div = [0 for _ in range(len(val) - len(mod) + 1)]
    while len(val) >= len(mod):
        term = val[-1]
        div[len(val) - len(mod)] = term
        # If the highest coefficient in val is nonzero, subtract a multiple of mod from it.
        val.pop()
        if term != 0:
            for x in range(len(mod) - 1):
                val[1 + x - len(mod)] ^= gf.mul(term, mod[x])
    # Prune trailing zero coefficients.
    while len(val) > 0 and val[-1] == 0:
        val.pop()
    return div, val

def poly_gcd(a, b, gf):
    """Return the polynomial GCD of a and b."""
    if len(a) < len(b):
        a, b = b, a
    # Use Euclid's algorithm to find the GCD of a and b.
    # see https://en.wikipedia.org/wiki/Polynomial_greatest_common_divisor#Euclid's_algorithm.
    while len(b) > 0:
        b = poly_monic(b, gf)
        (_, b), a = poly_divmod(a, b, gf), b
    return a

def poly_sqr(poly, gf):
    """Return the square of polynomial poly."""
    if len(poly) == 0:
        return []
    # In characteristic-2 fields, thanks to Frobenius' endomorphism ((a + b)^2 = a^2 + b^2),
    # squaring a polynomial is easy: square all the coefficients and interleave with zeroes.
    # E.g., (3 + 5*x + 17*x^2)^2 = 3^2 + (5*x)^2 + (17*x^2)^2.
    # See https://en.wikipedia.org/wiki/Frobenius_endomorphism.
    return [0 if i & 1 else gf.sqr(poly[i // 2]) for i in range(2 * len(poly) - 1)]

def poly_tracemod(poly, param, gf):
    """Compute y + y^2 + y^4 + ... + y^(2^(field_size-1)) mod poly, where y = param*x."""
    out = [0, param]
    for _ in range(gf.field_size - 1):
        # In each loop iteration i, we start with out = y + y^2 + ... + y^(2^i). By squaring that we
        # transform it into out = y^2 + y^4 + ... + y^(2^(i+1)).
        out = poly_sqr(out, gf)
        # Thus, we just need to add y again to it to get out = y + ... + y^(2^(i+1)).
        while len(out) < 2:
            out.append(0)
        out[1] = param
        # Finally take a modulus to keep the intermediary polynomials small.
        _, out = poly_divmod(out, poly, gf)
    return out

def poly_frobeniusmod(poly, gf):
    """Compute x^(2^field_size) mod poly."""
    out = [0, 1]
    for _ in range(gf.field_size):
        _, out = poly_divmod(poly_sqr(out, gf), poly, gf)
    return out

def poly_find_roots(poly, gf):
    """Find the roots of poly if fully factorizable with unique roots, [] otherwise."""
    assert len(poly) > 0
    # If the polynomial is constant (and nonzero), it has no roots.
    if len(poly) == 1:
        return []
    # Make the polynomial monic (which doesn't change its roots).
    poly = poly_monic(poly, gf)
    # If the polynomial is of the form x+a, return a.
    if len(poly) == 2:
        return [poly[0]]
    # Otherwise, first test that poly can be completely factored into unique roots. The polynomial
    # x^(2^fieldsize)-x has every field element once as root. Thus we want to know that that is a
    # multiple of poly. Compute x^(field_size) mod poly, which needs to equal x if that is the case
    # (unless poly has degree <= 1, but that case is handled above).
    if poly_frobeniusmod(poly, gf) != [0, 1]:
        return []

    def rec_split(poly, randv):
        """Recursively split poly using the Berlekamp trace algorithm."""
        # See https://hal.archives-ouvertes.fr/hal-00626997/document.
        assert len(poly) > 1 and poly[-1] == 1 # Require a monic poly.
        # If poly is of the form x+a, its root is a.
        if len(poly) == 2:
            return [poly[0]]
        # Try consecutive randomization factors randv, until one is found that factors poly.
        while True:
            # Compute the trace of (randv*x) mod poly. This is a polynomial that maps half of the
            # domain to 0, and the other half to 1. Which half that is is controlled by randv.
            # By taking it modulo poly, we only add a multiple of poly. Thus the result has at least
            # the shared roots of the trace polynomial and poly still, but may have others.
            trace = poly_tracemod(poly, randv, gf)
            # Using the set {2^i*a for i=0..fieldsize-1} gives optimally independent randv values
            # (no more than fieldsize are ever needed).
            randv = gf.mul2(randv)
            # Now take the GCD of this trace polynomial with poly. The result is a polynomial
            # that only has the shared roots of the trace polynomial and poly as roots.
            gcd = poly_gcd(trace, poly, gf)
            # If the result has a degree higher than 1, and lower than that of poly, we found a
            # useful factorization.
            if len(gcd) != len(poly) and len(gcd) > 1:
                break
            # Otherwise, continue with another randv.
        # Find the actual factors: the monic version of the GCD above, and poly divided by it.
        factor1 = poly_monic(gcd, gf)
        factor2, _ = poly_divmod(poly, gcd, gf)
        # Recurse.
        return rec_split(factor1, randv) + rec_split(factor2, randv)

    # Invoke the recursive splitting with a random initial factor, and sort the results.
    return sorted(rec_split(poly, random.randrange(1, 1 << gf.field_size)))

class TestPolyFindRoots(unittest.TestCase):
    """Test class for poly_find_roots."""

    def field_size_test(self, field_size):
        """Run tests for given field_size."""
        gf = GF2Ops(field_size)
        for test_size in [0, 1, 2, 3, 10]:
            roots = [random.randrange(1 << field_size) for _ in range(test_size)]
            roots_set = set(roots)
            # Construct a polynomial with all elements of roots as roots (with multiplicity).
            poly = [1]
            for root in roots:
                new_poly = [0] + poly
                for n, c in enumerate(poly):
                    new_poly[n] ^= gf.mul(c, root)
                poly = new_poly
            # Invoke the root finding algorithm.
            found_roots = poly_find_roots(poly, gf)
            # The result must match the input, unless any roots were repeated.
            if len(roots) == len(roots_set):
                self.assertEqual(found_roots, sorted(roots))
            else:
                self.assertEqual(found_roots, [])

    def test(self):
        """Run tests."""
        for field_size in range(2, 65):
            self.field_size_test(field_size)

def berlekamp_massey(syndromes, gf):
    """Implement the Berlekamp-Massey algorithm.

    Takes as input a sequence of GF(2^field_size) elements, and returns the shortest LSFR
    that generates it, represented as a polynomial.
    """
    # See https://en.wikipedia.org/wiki/Berlekamp%E2%80%93Massey_algorithm.
    current = [1]
    prev = [1]
    b_inv = 1
    for n, discrepancy in enumerate(syndromes):
        # Compute discrepancy
        for i in range(1, len(current)):
            discrepancy ^= gf.mul(syndromes[n - i], current[i])

        # Correct if discrepancy is nonzero.
        if discrepancy:
            x = n + 1 - (len(current) - 1) - (len(prev) - 1)
            if 2 * (len(current) - 1) <= n:
                tmp = list(current)
                current.extend(0 for _ in range(len(prev) + x - len(current)))
                mul = gf.mul(discrepancy, b_inv)
                for i, v in enumerate(prev):
                    current[i + x] ^= gf.mul(mul, v)
                prev = tmp
                b_inv = gf.inv(discrepancy)
            else:
                mul = gf.mul(discrepancy, b_inv)
                for i, v in enumerate(prev):
                    current[i + x] ^= gf.mul(mul, v)
    return current

class Minisketch:
    """A Minisketch sketch.

    This represents a sketch of a certain capacity, with elements of a certain bit size.
    """

    def __init__(self, field_size, capacity):
        """Initialize an empty sketch with the specified field_size size and capacity."""
        self.field_size = field_size
        self.capacity = capacity
        self.odd_syndromes = [0] * capacity
        self.gf = GF2Ops(field_size)

    def add(self, element):
        """Add an element to this sketch. 1 <= element < 2**field_size."""
        sqr = self.gf.sqr(element)
        for pos in range(self.capacity):
            self.odd_syndromes[pos] ^= element
            element = self.gf.mul(sqr, element)

    def serialized_size(self):
        """Compute how many bytes a serialization of this sketch will be in size."""
        return (self.capacity * self.field_size + 7) // 8

    def serialize(self):
        """Serialize this sketch to bytes."""
        val = 0
        for i in range(self.capacity):
            val |= self.odd_syndromes[i] << (self.field_size * i)
        return val.to_bytes(self.serialized_size(), 'little')

    def deserialize(self, byte_data):
        """Deserialize a byte array into this sketch, overwriting its contents."""
        assert len(byte_data) == self.serialized_size()
        val = int.from_bytes(byte_data, 'little')
        for i in range(self.capacity):
            self.odd_syndromes[i] = (val >> (self.field_size * i)) & ((1 << self.field_size) - 1)

    def clone(self):
        """Return a clone of this sketch."""
        ret = Minisketch(self.field_size, self.capacity)
        ret.odd_syndromes = list(self.odd_syndromes)
        ret.gf = self.gf
        return ret

    def merge(self, other):
        """Merge a sketch with another sketch. Corresponds to XOR'ing their serializations."""
        assert self.capacity == other.capacity
        assert self.field_size == other.field_size
        for i in range(self.capacity):
            self.odd_syndromes[i] ^= other.odd_syndromes[i]

    def decode(self, max_count=None):
        """Decode the contents of this sketch.

        Returns either a list of elements or None if undecodable.
        """
        # We know the odd syndromes s1=x+y+..., s3=x^3+y^3+..., s5=..., and reconstruct the even
        # syndromes from this:
        #  * s2 = x^2+y^2+.... = (x+y+...)^2 = s1^2
        #  * s4 = x^4+y^4+.... = (x^2+y^2+...)^2 = s2^2
        #  * s6 = x^6+y^6+.... = (x^3+y^3+...)^2 = s3^2
        all_syndromes = [0 for _ in range(2 * len(self.odd_syndromes))]
        for i in range(len(self.odd_syndromes)):
            all_syndromes[i * 2] = self.odd_syndromes[i]
            all_syndromes[i * 2 + 1] = self.gf.sqr(all_syndromes[i])
        # Given the syndromes, find the polynomial that generates them.
        poly = berlekamp_massey(all_syndromes, self.gf)
        # Deal with failure and trivial cases.
        if len(poly) == 0:
            return None
        if len(poly) == 1:
            return []
        if max_count is not None and len(poly) > 1 + max_count:
            return None
        # If the polynomial can be factored into (1-m1*x)*(1-m2*x)*...*(1-mn*x), then {m1,m2,...,mn}
        # is our set. As each factor (1-m*x) has 1/m as root, we're really just looking for the
        # inverses of the roots. We find these by reversing the order of the coefficients, and
        # finding the roots.
        roots = poly_find_roots(list(reversed(poly)), self.gf)
        if len(roots) == 0:
            return None
        return roots

class TestMinisketch(unittest.TestCase):
    """Test class for Minisketch."""

    @classmethod
    def construct_data(cls, field_size, num_a_only, num_b_only, num_both):
        """Construct two random lists of elements in [1..2**field_size-1].

        Each list will have unique elements that don't appear in the other (num_a_only in the first
        and num_b_only in the second), and num_both elements will appear in both."""
        sample = []
        # Simulate random.sample here (which doesn't work with ranges over 2**63).
        for _ in range(num_a_only + num_b_only + num_both):
            while True:
                r = random.randrange(1, 1 << field_size)
                if r not in sample:
                    sample.append(r)
                    break
        full_a = sample[:num_a_only + num_both]
        full_b = sample[num_a_only:]
        random.shuffle(full_a)
        random.shuffle(full_b)
        return full_a, full_b

    def field_size_capacity_test(self, field_size, capacity):
        """Test Minisketch methods for a specific field and capacity."""
        used_capacity = random.randrange(capacity + 1)
        num_a = random.randrange(used_capacity + 1)
        num_both = random.randrange(min(2 * capacity, (1 << field_size) - 1 - used_capacity) + 1)
        full_a, full_b = self.construct_data(field_size, num_a, used_capacity - num_a, num_both)
        sketch_a = Minisketch(field_size, capacity)
        sketch_b = Minisketch(field_size, capacity)
        for v in full_a:
            sketch_a.add(v)
        for v in full_b:
            sketch_b.add(v)
        sketch_combined = sketch_a.clone()
        sketch_b_ser = sketch_b.serialize()
        sketch_b_received = Minisketch(field_size, capacity)
        sketch_b_received.deserialize(sketch_b_ser)
        sketch_combined.merge(sketch_b_received)
        decode = sketch_combined.decode()
        self.assertEqual(decode, sorted(set(full_a) ^ set(full_b)))

    def test(self):
        """Run tests."""
        for field_size in range(2, 65):
            for capacity in [0, 1, 2, 5, 10, field_size]:
                self.field_size_capacity_test(field_size, min(capacity, (1 << field_size) - 1))

if __name__ == '__main__':
    unittest.main()
