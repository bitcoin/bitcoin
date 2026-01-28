"""Test low-level secp256k1 field and group arithmetic classes."""
from random import randint
import unittest

from secp256k1lab.secp256k1 import FE, G, GE, Scalar


class PrimeFieldTests(unittest.TestCase):
    def test_fe_constructors(self):
        P = FE.SIZE
        random_fe_valid = randint(0, P-1)
        random_fe_overflowing = randint(P, 2**256-1)

        # wrapping constructors
        for init_value in [0, P-1, P, P+1, random_fe_valid, random_fe_overflowing]:
            fe1 = FE(init_value)
            fe2 = FE.from_int_wrapping(init_value)
            fe3 = FE.from_bytes_wrapping(init_value.to_bytes(32, 'big'))
            reduced_value = init_value % P
            self.assertEqual(int(fe1), reduced_value)
            self.assertEqual(int(fe1), int(fe2))
            self.assertEqual(int(fe2), int(fe3))

        # checking constructors (should throw on overflow)
        for valid_value in [0, P-1, random_fe_valid]:
            fe1 = FE.from_int_checked(valid_value)
            fe2 = FE.from_bytes_checked(valid_value.to_bytes(32, 'big'))
            self.assertEqual(int(fe1), valid_value)
            self.assertEqual(int(fe1), int(fe2))

        for overflow_value in [P, P+1, random_fe_overflowing]:
            with self.assertRaises(ValueError):
                _ = FE.from_int_checked(overflow_value)
            with self.assertRaises(ValueError):
                _ = FE.from_bytes_checked(overflow_value.to_bytes(32, 'big'))

    def test_scalar_constructors(self):
        N = Scalar.SIZE
        random_scalar_valid = randint(0, N-1)
        random_scalar_overflowing = randint(N, 2**256-1)

        # wrapping constructors
        for init_value in [0, N-1, N, N+1, random_scalar_valid, random_scalar_overflowing]:
            s1 = Scalar(init_value)
            s2 = Scalar.from_int_wrapping(init_value)
            s3 = Scalar.from_bytes_wrapping(init_value.to_bytes(32, 'big'))
            reduced_value = init_value % N
            self.assertEqual(int(s1), reduced_value)
            self.assertEqual(int(s1), int(s2))
            self.assertEqual(int(s2), int(s3))

        # checking constructors (should throw on overflow)
        for valid_value in [0, N-1, random_scalar_valid]:
            s1 = Scalar.from_int_checked(valid_value)
            s2 = Scalar.from_bytes_checked(valid_value.to_bytes(32, 'big'))
            self.assertEqual(int(s1), valid_value)
            self.assertEqual(int(s1), int(s2))

        for overflow_value in [N, N+1, random_scalar_overflowing]:
            with self.assertRaises(ValueError):
                _ = Scalar.from_int_checked(overflow_value)
            with self.assertRaises(ValueError):
                _ = Scalar.from_bytes_checked(overflow_value.to_bytes(32, 'big'))

        # non-zero checking constructors (should throw on zero or overflow, only for Scalar)
        random_nonzero_scalar_valid = randint(1, N-1)
        for valid_value in [1, N-1, random_nonzero_scalar_valid]:
            s1 = Scalar.from_int_nonzero_checked(valid_value)
            s2 = Scalar.from_bytes_nonzero_checked(valid_value.to_bytes(32, 'big'))
            self.assertEqual(int(s1), valid_value)
            self.assertEqual(int(s1), int(s2))

        for invalid_value in [0, N, random_scalar_overflowing]:
            with self.assertRaises(ValueError):
                _ = Scalar.from_int_nonzero_checked(invalid_value)
            with self.assertRaises(ValueError):
                _ = Scalar.from_bytes_nonzero_checked(invalid_value.to_bytes(32, 'big'))


class GeSerializationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.point_at_infinity = GE()
        cls.group_elements_on_curve = [
            # generator point
            G,
            # Bitcoin genesis block public key
            GE(0x678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb6,
               0x49f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f),
        ]
        # generate a few random points, to likely cover both even/odd y polarity
        cls.group_elements_on_curve.extend([randint(1, Scalar.SIZE-1) * G for _ in range(8)])
        # generate x coordinates that don't have a valid point on the curve
        # (note that ~50% of all x coordinates are valid, so finding one needs two loop iterations on average)
        cls.x_coords_not_on_curve = []
        while len(cls.x_coords_not_on_curve) < 8:
            x = randint(0, FE.SIZE-1)
            if not GE.is_valid_x(x):
                cls.x_coords_not_on_curve.append(x)

        cls.group_elements = [cls.point_at_infinity] + cls.group_elements_on_curve

    def test_infinity_raises(self):
        with self.assertRaises(AssertionError):
            _ = self.point_at_infinity.to_bytes_uncompressed()
        with self.assertRaises(AssertionError):
            _ = self.point_at_infinity.to_bytes_compressed()
        with self.assertRaises(AssertionError):
            _ = self.point_at_infinity.to_bytes_xonly()

    def test_not_on_curve_raises(self):
        # for compressed and x-only GE deserialization, test with invalid x coordinate
        for x in self.x_coords_not_on_curve:
            x_bytes = x.to_bytes(32, 'big')
            with self.assertRaises(ValueError):
                _ = GE.from_bytes_compressed(b'\x02' + x_bytes)
            with self.assertRaises(ValueError):
                _ = GE.from_bytes_compressed(b'\x03' + x_bytes)
            with self.assertRaises(ValueError):
                _ = GE.from_bytes_compressed_with_infinity(b'\x02' + x_bytes)
            with self.assertRaises(ValueError):
                _ = GE.from_bytes_compressed_with_infinity(b'\x03' + x_bytes)
            with self.assertRaises(ValueError):
                _ = GE.from_bytes_xonly(x_bytes)

        # for uncompressed GE serialization, test by invalidating either coordinate
        for ge in self.group_elements_on_curve:
            valid_x = ge.x
            valid_y = ge.y
            invalid_x = ge.x + 1
            invalid_y = ge.y + 1

            # valid cases (if point (x,y) is on the curve, then point(x,-y) is on the curve as well)
            _ = GE.from_bytes_uncompressed(b'\x04' + valid_x.to_bytes() + valid_y.to_bytes())
            _ = GE.from_bytes_uncompressed(b'\x04' + valid_x.to_bytes() + (-valid_y).to_bytes())
            # invalid cases (curve equation y**2 = x**3 + 7 doesn't hold)
            self.assertNotEqual(invalid_y**2, valid_x**3 + 7)
            with self.assertRaises(ValueError):
                _ = GE.from_bytes_uncompressed(b'\x04' + valid_x.to_bytes() + invalid_y.to_bytes())
            self.assertNotEqual(valid_y**2, invalid_x**3 + 7)
            with self.assertRaises(ValueError):
                _ = GE.from_bytes_uncompressed(b'\x04' + invalid_x.to_bytes() + valid_y.to_bytes())

    def test_affine(self):
        # GE serialization and parsing round-trip (variants that only support serializing points on the curve)
        for ge_orig in self.group_elements_on_curve:
            # uncompressed serialization: 65 bytes, starts with 0x04
            ge_ser = ge_orig.to_bytes_uncompressed()
            self.assertEqual(len(ge_ser), 65)
            self.assertEqual(ge_ser[0], 0x04)
            ge_deser = GE.from_bytes_uncompressed(ge_ser)
            self.assertEqual(ge_deser, ge_orig)

            # compressed serialization: 33 bytes, starts with 0x02 (if y is even) or 0x03 (if y is odd)
            ge_ser = ge_orig.to_bytes_compressed()
            self.assertEqual(len(ge_ser), 33)
            self.assertEqual(ge_ser[0], 0x02 if ge_orig.has_even_y() else 0x03)
            ge_deser = GE.from_bytes_compressed(ge_ser)
            self.assertEqual(ge_deser, ge_orig)

            # x-only serialization: 32 bytes
            ge_ser = ge_orig.to_bytes_xonly()
            self.assertEqual(len(ge_ser), 32)
            ge_deser = GE.from_bytes_xonly(ge_ser)
            if not ge_orig.has_even_y():  # x-only implies even y, so flip if necessary
                ge_deser = -ge_deser
            self.assertEqual(ge_deser, ge_orig)

    def test_affine_with_infinity(self):
        # GE serialization and parsing round-trip (variants that also support serializing the point at infinity)
        for ge_orig in self.group_elements:
            # compressed serialization: 33 bytes, all-zeros for point at infinity
            ge_ser = ge_orig.to_bytes_compressed_with_infinity()
            self.assertEqual(len(ge_ser), 33)
            if ge_orig.infinity:
                self.assertEqual(ge_ser, b'\x00'*33)
            else:
                self.assertEqual(ge_ser[0], 0x02 if ge_orig.has_even_y() else 0x03)
            ge_deser = GE.from_bytes_compressed_with_infinity(ge_ser)
            self.assertEqual(ge_deser, ge_orig)
