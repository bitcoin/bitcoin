/*
 * Copyright 2015 MongoDB, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <bson.h>

#include <stdlib.h>
#include <string.h>

#include "bson-tests.h"
#include "TestSuite.h"


/*
 *----------------------------------------------------------------------------
 * decimal128_to_string tests
 *
 * The following tests confirm functionality of the decimal128_to_string
 *function.
 *
 * All decimal test data is generated using the Intel Decimal Floating-Point
 * Math Library's bid128_from_string routine.
 *
 *----------------------------------------------------------------------------
 */


#define DECIMAL128_FROM_ULLS(dec, h, l) \
   do {                                 \
      (dec).high = (h);                 \
      (dec).low = (l);                  \
   } while (0);


static void
test_decimal128_to_string__infinity (void)
{
   bson_decimal128_t positive_infinity;
   bson_decimal128_t negative_infinity;
   char bid_string[BSON_DECIMAL128_STRING];

   DECIMAL128_FROM_ULLS (positive_infinity, 0x7800000000000000, 0);
   DECIMAL128_FROM_ULLS (negative_infinity, 0xf800000000000000, 0);

   bson_decimal128_to_string (&positive_infinity, bid_string);
   BSON_ASSERT (!strcmp (bid_string, "Infinity"));

   bson_decimal128_to_string (&negative_infinity, bid_string);
   BSON_ASSERT (!strcmp (bid_string, "-Infinity"));
}


static void
test_decimal128_to_string__nan (void)
{
   bson_decimal128_t dec_pnan;
   bson_decimal128_t dec_nnan;
   bson_decimal128_t dec_psnan;
   bson_decimal128_t dec_nsnan;
   bson_decimal128_t dec_payload_nan;

   /* All the above should just be NaN. */
   char bid_string[BSON_DECIMAL128_STRING];

   DECIMAL128_FROM_ULLS (dec_pnan, 0x7c00000000000000, 0);
   DECIMAL128_FROM_ULLS (dec_nnan, 0xfc00000000000000, 0);
   DECIMAL128_FROM_ULLS (dec_psnan, 0x7e00000000000000, 0);
   DECIMAL128_FROM_ULLS (dec_nsnan, 0xfe00000000000000, 0);
   DECIMAL128_FROM_ULLS (dec_payload_nan, 0x7e00000000000000, 12);

   bson_decimal128_to_string (&dec_pnan, bid_string);
   BSON_ASSERT (!strcmp (bid_string, "NaN"));

   bson_decimal128_to_string (&dec_nnan, bid_string);
   BSON_ASSERT (!strcmp (bid_string, "NaN"));

   bson_decimal128_to_string (&dec_psnan, bid_string);
   BSON_ASSERT (!strcmp (bid_string, "NaN"));

   bson_decimal128_to_string (&dec_nsnan, bid_string);
   BSON_ASSERT (!strcmp (bid_string, "NaN"));

   bson_decimal128_to_string (&dec_payload_nan, bid_string);
   BSON_ASSERT (!strcmp (bid_string, "NaN"));
}


static void
test_decimal128_to_string__regular (void)
{
   char bid_string[BSON_DECIMAL128_STRING];
   bson_decimal128_t one;
   bson_decimal128_t zero;
   bson_decimal128_t two;
   bson_decimal128_t negative_one;
   bson_decimal128_t negative_zero;
   bson_decimal128_t tenth;
   bson_decimal128_t smallest_regular;
   bson_decimal128_t largest_regular;
   bson_decimal128_t trailing_zeros;
   bson_decimal128_t all_digits;
   bson_decimal128_t full_house;

   DECIMAL128_FROM_ULLS (one, 0x3040000000000000, 0x0000000000000001);
   DECIMAL128_FROM_ULLS (zero, 0x3040000000000000, 0x0000000000000000);
   DECIMAL128_FROM_ULLS (two, 0x3040000000000000, 0x0000000000000002);
   DECIMAL128_FROM_ULLS (negative_one, 0xb040000000000000, 0x0000000000000001);
   DECIMAL128_FROM_ULLS (negative_zero, 0xb040000000000000, 0x0000000000000000);
   DECIMAL128_FROM_ULLS (
      tenth, 0x303e000000000000, 0x0000000000000001); /* 0.1 */
   /* 0.001234 */
   DECIMAL128_FROM_ULLS (
      smallest_regular, 0x3034000000000000, 0x00000000000004d2);
   /* 12345789012 */
   DECIMAL128_FROM_ULLS (
      largest_regular, 0x3040000000000000, 0x0000001cbe991a14);
   /* 0.00123400000 */
   DECIMAL128_FROM_ULLS (
      trailing_zeros, 0x302a000000000000, 0x00000000075aef40);
   /* 0.1234567890123456789012345678901234 */
   DECIMAL128_FROM_ULLS (all_digits, 0x2ffc3cde6fff9732, 0xde825cd07e96aff2);

   /* 5192296858534827628530496329220095 */
   DECIMAL128_FROM_ULLS (full_house, 0x3040ffffffffffff, 0xffffffffffffffff);

   bson_decimal128_to_string (&one, bid_string);
   BSON_ASSERT (!strcmp ("1", bid_string));

   bson_decimal128_to_string (&zero, bid_string);
   BSON_ASSERT (!strcmp ("0", bid_string));

   bson_decimal128_to_string (&two, bid_string);
   BSON_ASSERT (!strcmp ("2", bid_string));

   bson_decimal128_to_string (&negative_one, bid_string);
   BSON_ASSERT (!strcmp ("-1", bid_string));

   bson_decimal128_to_string (&negative_zero, bid_string);
   BSON_ASSERT (!strcmp ("-0", bid_string));

   bson_decimal128_to_string (&tenth, bid_string);
   BSON_ASSERT (!strcmp ("0.1", bid_string));

   bson_decimal128_to_string (&smallest_regular, bid_string);
   BSON_ASSERT (!strcmp ("0.001234", bid_string));

   bson_decimal128_to_string (&largest_regular, bid_string);
   BSON_ASSERT (!strcmp ("123456789012", bid_string));

   bson_decimal128_to_string (&trailing_zeros, bid_string);
   BSON_ASSERT (!strcmp ("0.00123400000", bid_string));

   bson_decimal128_to_string (&all_digits, bid_string);
   BSON_ASSERT (!strcmp ("0.1234567890123456789012345678901234", bid_string));

   bson_decimal128_to_string (&full_house, bid_string);
   BSON_ASSERT (!strcmp ("5192296858534827628530496329220095", bid_string));
}


static void
test_decimal128_to_string__scientific (void)
{
   char bid_string[BSON_DECIMAL128_STRING];

   bson_decimal128_t huge;     /* 1.000000000000000000000000000000000E+6144 */
   bson_decimal128_t tiny;     /* 1E-6176 */
   bson_decimal128_t neg_tiny; /* -1E-6176 */
   bson_decimal128_t large;    /* 9.999987654321E+112 */
   bson_decimal128_t largest;  /* 9.999999999999999999999999999999999E+6144 */
   bson_decimal128_t tiniest;  /* 9.999999999999999999999999999999999E-6143 */
   bson_decimal128_t trailing_zero;            /* 1.050E9 */
   bson_decimal128_t one_trailing_zero;        /* 1.050E4 */
   bson_decimal128_t move_decimal;             /* 105 */
   bson_decimal128_t move_decimal_after;       /* 1.05E3 */
   bson_decimal128_t trailing_zero_no_decimal; /* 1E3 */

   DECIMAL128_FROM_ULLS (huge, 0x5ffe314dc6448d93, 0x38c15b0a00000000);
   DECIMAL128_FROM_ULLS (tiny, 0x0000000000000000, 0x0000000000000001);
   DECIMAL128_FROM_ULLS (neg_tiny, 0x8000000000000000, 0x0000000000000001);
   DECIMAL128_FROM_ULLS (large, 0x3108000000000000, 0x000009184db63eb1);
   DECIMAL128_FROM_ULLS (largest, 0x5fffed09bead87c0, 0x378d8e63ffffffff);
   DECIMAL128_FROM_ULLS (tiniest, 0x0001ed09bead87c0, 0x378d8e63ffffffff);
   DECIMAL128_FROM_ULLS (trailing_zero, 0x304c000000000000, 0x000000000000041a);
   DECIMAL128_FROM_ULLS (
      one_trailing_zero, 0x3042000000000000, 0x000000000000041a);
   DECIMAL128_FROM_ULLS (move_decimal, 0x3040000000000000, 0x0000000000000069);
   DECIMAL128_FROM_ULLS (
      move_decimal_after, 0x3042000000000000, 0x0000000000000069);
   DECIMAL128_FROM_ULLS (
      trailing_zero_no_decimal, 0x3046000000000000, 0x0000000000000001);

   bson_decimal128_to_string (&huge, bid_string);
   BSON_ASSERT (!strcmp ("1.000000000000000000000000000000000E+6144", bid_string));

   bson_decimal128_to_string (&tiny, bid_string);
   BSON_ASSERT (!strcmp ("1E-6176", bid_string));

   bson_decimal128_to_string (&neg_tiny, bid_string);
   BSON_ASSERT (!strcmp ("-1E-6176", bid_string));

   bson_decimal128_to_string (&neg_tiny, bid_string);
   BSON_ASSERT (!strcmp ("-1E-6176", bid_string));

   bson_decimal128_to_string (&large, bid_string);
   BSON_ASSERT (!strcmp ("9.999987654321E+112", bid_string));

   bson_decimal128_to_string (&largest, bid_string);
   BSON_ASSERT (!strcmp ("9.999999999999999999999999999999999E+6144", bid_string));

   bson_decimal128_to_string (&tiniest, bid_string);
   BSON_ASSERT (!strcmp ("9.999999999999999999999999999999999E-6143", bid_string));

   bson_decimal128_to_string (&trailing_zero, bid_string);
   BSON_ASSERT (!strcmp ("1.050E+9", bid_string));

   bson_decimal128_to_string (&one_trailing_zero, bid_string);
   BSON_ASSERT (!strcmp ("1.050E+4", bid_string));

   bson_decimal128_to_string (&move_decimal, bid_string);
   BSON_ASSERT (!strcmp ("105", bid_string));

   bson_decimal128_to_string (&move_decimal_after, bid_string);
   BSON_ASSERT (!strcmp ("1.05E+3", bid_string));

   bson_decimal128_to_string (&trailing_zero_no_decimal, bid_string);
   BSON_ASSERT (!strcmp ("1E+3", bid_string));
}


static void
test_decimal128_to_string__zeros (void)
{
   char bid_string[BSON_DECIMAL128_STRING];

   bson_decimal128_t zero;         /* 0 */
   bson_decimal128_t pos_exp_zero; /* 0E+300 */
   bson_decimal128_t neg_exp_zero; /* 0E-600 */

   DECIMAL128_FROM_ULLS (zero, 0x3040000000000000, 0x0000000000000000);
   DECIMAL128_FROM_ULLS (pos_exp_zero, 0x3298000000000000, 0x0000000000000000);
   DECIMAL128_FROM_ULLS (neg_exp_zero, 0x2b90000000000000, 0x0000000000000000);

   bson_decimal128_to_string (&zero, bid_string);
   BSON_ASSERT (!strcmp ("0", bid_string));

   bson_decimal128_to_string (&pos_exp_zero, bid_string);
   BSON_ASSERT (!strcmp ("0E+300", bid_string));

   bson_decimal128_to_string (&neg_exp_zero, bid_string);
   BSON_ASSERT (!strcmp ("0E-600", bid_string));
}


#define IS_NAN(dec) (dec).high == 0x7c00000000000000ull


static void
test_decimal128_from_string__invalid_inputs (void)
{
   bson_decimal128_t dec;

   bson_decimal128_from_string (".", &dec);
   BSON_ASSERT (IS_NAN (dec));
   bson_decimal128_from_string (".e", &dec);
   BSON_ASSERT (IS_NAN (dec));
   bson_decimal128_from_string ("", &dec);
   BSON_ASSERT (IS_NAN (dec));
   bson_decimal128_from_string ("invalid", &dec);
   BSON_ASSERT (IS_NAN (dec));
   bson_decimal128_from_string ("in", &dec);
   BSON_ASSERT (IS_NAN (dec));
   bson_decimal128_from_string ("i", &dec);
   BSON_ASSERT (IS_NAN (dec));
   bson_decimal128_from_string ("E02", &dec);
   BSON_ASSERT (IS_NAN (dec));
   bson_decimal128_from_string ("..1", &dec);
   BSON_ASSERT (IS_NAN (dec));
   bson_decimal128_from_string ("1abcede", &dec);
   BSON_ASSERT (IS_NAN (dec));
   bson_decimal128_from_string ("1.24abc", &dec);
   BSON_ASSERT (IS_NAN (dec));
   bson_decimal128_from_string ("1.24abcE+02", &dec);
   BSON_ASSERT (IS_NAN (dec));
   bson_decimal128_from_string ("1.24E+02abc2d", &dec);
   BSON_ASSERT (IS_NAN (dec));
   bson_decimal128_from_string ("E+02", &dec);
   BSON_ASSERT (IS_NAN (dec));
   bson_decimal128_from_string ("e+02", &dec);
   BSON_ASSERT (IS_NAN (dec));
}


static void
test_decimal128_from_string__nan (void)
{
   bson_decimal128_t dec;

   bson_decimal128_from_string ("NaN", &dec);
   BSON_ASSERT (IS_NAN (dec));
   bson_decimal128_from_string ("+NaN", &dec);
   BSON_ASSERT (IS_NAN (dec));
   bson_decimal128_from_string ("-NaN", &dec);
   BSON_ASSERT (IS_NAN (dec));
   bson_decimal128_from_string ("-nan", &dec);
   BSON_ASSERT (IS_NAN (dec));
   bson_decimal128_from_string ("1e", &dec);
   BSON_ASSERT (IS_NAN (dec));
   bson_decimal128_from_string ("+nan", &dec);
   BSON_ASSERT (IS_NAN (dec));
   bson_decimal128_from_string ("nan", &dec);
   BSON_ASSERT (IS_NAN (dec));
   bson_decimal128_from_string ("Nan", &dec);
   BSON_ASSERT (IS_NAN (dec));
   bson_decimal128_from_string ("+Nan", &dec);
   BSON_ASSERT (IS_NAN (dec));
   bson_decimal128_from_string ("-Nan", &dec);
   BSON_ASSERT (IS_NAN (dec));
}


#define IS_PINFINITY(dec) (dec).high == 0x7800000000000000
#define IS_NINFINITY(dec) (dec).high == 0xf800000000000000


static void
test_decimal128_from_string__infinity (void)
{
   bson_decimal128_t dec;

   bson_decimal128_from_string ("Infinity", &dec);
   BSON_ASSERT (IS_PINFINITY (dec));
   bson_decimal128_from_string ("+Infinity", &dec);
   BSON_ASSERT (IS_PINFINITY (dec));
   bson_decimal128_from_string ("+Inf", &dec);
   BSON_ASSERT (IS_PINFINITY (dec));
   bson_decimal128_from_string ("-Inf", &dec);
   BSON_ASSERT (IS_NINFINITY (dec));
   bson_decimal128_from_string ("-Infinity", &dec);
   BSON_ASSERT (IS_NINFINITY (dec));
}


static bool
decimal128_equal (bson_decimal128_t *dec, uint64_t high, uint64_t low)
{
   bson_decimal128_t test;
   DECIMAL128_FROM_ULLS (test, high, low);
   return memcmp (dec, &test, sizeof (*dec)) == 0;
}

static void
test_decimal128_from_string__simple (void)
{
   bson_decimal128_t one;
   bson_decimal128_t negative_one;
   bson_decimal128_t zero;
   bson_decimal128_t negative_zero;
   bson_decimal128_t number;
   bson_decimal128_t number_two;
   bson_decimal128_t negative_number;
   bson_decimal128_t fractional_number;
   bson_decimal128_t leading_zeros;
   bson_decimal128_t leading_insignificant_zeros;

   bson_decimal128_from_string ("1", &one);
   bson_decimal128_from_string ("-1", &negative_one);
   bson_decimal128_from_string ("0", &zero);
   bson_decimal128_from_string ("-0", &negative_zero);
   bson_decimal128_from_string ("12345678901234567", &number);
   bson_decimal128_from_string ("989898983458", &number_two);
   bson_decimal128_from_string ("-12345678901234567", &negative_number);

   bson_decimal128_from_string ("0.12345", &fractional_number);
   bson_decimal128_from_string ("0.0012345", &leading_zeros);

   bson_decimal128_from_string ("00012345678901234567",
                                &leading_insignificant_zeros);

   BSON_ASSERT (decimal128_equal (&one, 0x3040000000000000, 0x0000000000000001));
   BSON_ASSERT (
      decimal128_equal (&negative_one, 0xb040000000000000, 0x0000000000000001));
   BSON_ASSERT (decimal128_equal (&zero, 0x3040000000000000, 0x0000000000000000));
   BSON_ASSERT (decimal128_equal (
      &negative_zero, 0xb040000000000000, 0x0000000000000000));
   BSON_ASSERT (decimal128_equal (&number, 0x3040000000000000, 0x002bdc545d6b4b87));
   BSON_ASSERT (
      decimal128_equal (&number_two, 0x3040000000000000, 0x000000e67a93c822));
   BSON_ASSERT (decimal128_equal (
      &negative_number, 0xb040000000000000, 0x002bdc545d6b4b87));
   BSON_ASSERT (decimal128_equal (
      &fractional_number, 0x3036000000000000, 0x0000000000003039));
   BSON_ASSERT (decimal128_equal (
      &leading_zeros, 0x3032000000000000, 0x0000000000003039));
   BSON_ASSERT (decimal128_equal (
      &leading_insignificant_zeros, 0x3040000000000000, 0x002bdc545d6b4b87));
}


static void
test_decimal128_from_string__scientific (void)
{
   bson_decimal128_t ten;
   bson_decimal128_t ten_again;
   bson_decimal128_t one;
   bson_decimal128_t huge_exp;
   bson_decimal128_t tiny_exp;
   bson_decimal128_t fractional;
   bson_decimal128_t trailing_zeros;

   bson_decimal128_from_string ("10e0", &ten);
   bson_decimal128_from_string ("1e1", &ten_again);
   bson_decimal128_from_string ("10e-1", &one);

   BSON_ASSERT (decimal128_equal (&ten, 0x3040000000000000, 0x000000000000000a));
   BSON_ASSERT (
      decimal128_equal (&ten_again, 0x3042000000000000, 0x0000000000000001));
   BSON_ASSERT (decimal128_equal (&one, 0x303e000000000000, 0x000000000000000a));

   bson_decimal128_from_string ("12345678901234567e6111", &huge_exp);
   bson_decimal128_from_string ("1e-6176", &tiny_exp);

   BSON_ASSERT (
      decimal128_equal (&huge_exp, 0x5ffe000000000000, 0x002bdc545d6b4b87));
   BSON_ASSERT (
      decimal128_equal (&tiny_exp, 0x0000000000000000, 0x0000000000000001));

   bson_decimal128_from_string ("-100E-10", &fractional);
   bson_decimal128_from_string ("10.50E8", &trailing_zeros);

   BSON_ASSERT (
      decimal128_equal (&fractional, 0xb02c000000000000, 0x0000000000000064));
   BSON_ASSERT (decimal128_equal (
      &trailing_zeros, 0x304c000000000000, 0x000000000000041a));
}


static void
test_decimal128_from_string__large (void)
{
   bson_decimal128_t large;
   bson_decimal128_t all_digits;
   bson_decimal128_t largest;
   bson_decimal128_t tiniest;
   bson_decimal128_t full_house;

   bson_decimal128_from_string ("12345689012345789012345", &large);
   bson_decimal128_from_string ("1234567890123456789012345678901234",
                                &all_digits);
   bson_decimal128_from_string ("9.999999999999999999999999999999999E+6144",
                                &largest);
   bson_decimal128_from_string ("9.999999999999999999999999999999999E-6143",
                                &tiniest);
   bson_decimal128_from_string ("5.192296858534827628530496329220095E+33",
                                &full_house);

   BSON_ASSERT (decimal128_equal (&large, 0x304000000000029d, 0x42da3a76f9e0d979));
   BSON_ASSERT (
      decimal128_equal (&all_digits, 0x30403cde6fff9732, 0xde825cd07e96aff2));
   BSON_ASSERT (decimal128_equal (&largest, 0x5fffed09bead87c0, 0x378d8e63ffffffff));
   BSON_ASSERT (decimal128_equal (&tiniest, 0x0001ed09bead87c0, 0x378d8e63ffffffff));
   BSON_ASSERT (
      decimal128_equal (&full_house, 0x3040ffffffffffff, 0xffffffffffffffff));
}


static void
test_decimal128_from_string__exponent_normalization (void)
{
   bson_decimal128_t trailing_zeros;
   bson_decimal128_t one_normalize;
   bson_decimal128_t no_normalize;
   bson_decimal128_t a_disaster;

   bson_decimal128_from_string ("1000000000000000000000000000000000000000",
                                &trailing_zeros);
   bson_decimal128_from_string ("10000000000000000000000000000000000",
                                &one_normalize);
   bson_decimal128_from_string ("1000000000000000000000000000000000",
                                &no_normalize);
   bson_decimal128_from_string (
      "100000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000"
      "0000000000000000000000000000000000",
      &a_disaster);

   BSON_ASSERT (decimal128_equal (
      &trailing_zeros, 0x304c314dc6448d93, 0x38c15b0a00000000));
   BSON_ASSERT (decimal128_equal (
      &one_normalize, 0x3042314dc6448d93, 0x38c15b0a00000000));
   BSON_ASSERT (
      decimal128_equal (&no_normalize, 0x3040314dc6448d93, 0x38c15b0a00000000));
   BSON_ASSERT (
      decimal128_equal (&a_disaster, 0x37cc314dc6448d93, 0x38c15b0a00000000));
}


static void
test_decimal128_from_string__zeros (void)
{
   bson_decimal128_t zero;
   bson_decimal128_t exponent_zero;
   bson_decimal128_t large_exponent;
   bson_decimal128_t negative_zero;

   bson_decimal128_from_string ("0", &zero);
   bson_decimal128_from_string ("0e-611", &exponent_zero);
   bson_decimal128_from_string ("0e+6000", &large_exponent);
   bson_decimal128_from_string ("-0e-1", &negative_zero);

   BSON_ASSERT (decimal128_equal (&zero, 0x3040000000000000, 0x0000000000000000));
   BSON_ASSERT (decimal128_equal (
      &exponent_zero, 0x2b7a000000000000, 0x0000000000000000));
   BSON_ASSERT (decimal128_equal (
      &large_exponent, 0x5f20000000000000, 0x0000000000000000));
   BSON_ASSERT (decimal128_equal (
      &negative_zero, 0xb03e000000000000, 0x0000000000000000));
}

void
test_decimal128_install (TestSuite *suite)
{
   TestSuite_Add (suite,
                  "/bson/decimal128/to_string/infinity",
                  test_decimal128_to_string__infinity);
   TestSuite_Add (
      suite, "/bson/decimal128/to_string/nan", test_decimal128_to_string__nan);
   TestSuite_Add (suite,
                  "/bson/decimal128/to_string/regular",
                  test_decimal128_to_string__regular);
   TestSuite_Add (suite,
                  "/bson/decimal128/to_string/scientific",
                  test_decimal128_to_string__scientific);
   TestSuite_Add (suite,
                  "/bson/decimal128/to_string/zero",
                  test_decimal128_to_string__zeros);
   TestSuite_Add (suite,
                  "/bson/decimal128/from_string/invalid",
                  test_decimal128_from_string__invalid_inputs);
   TestSuite_Add (suite,
                  "/bson/decimal128/from_string/nan",
                  test_decimal128_from_string__nan);
   TestSuite_Add (suite,
                  "/bson/decimal128/from_string/infinity",
                  test_decimal128_from_string__infinity);
   TestSuite_Add (suite,
                  "/bson/decimal128/from_string/basic",
                  test_decimal128_from_string__simple);
   TestSuite_Add (suite,
                  "/bson/decimal128/from_string/scientific",
                  test_decimal128_from_string__scientific);
   TestSuite_Add (suite,
                  "/bson/decimal128/from_string/large",
                  test_decimal128_from_string__large);
   TestSuite_Add (suite,
                  "/bson/decimal128/from_string/exponent_normalization",
                  test_decimal128_from_string__exponent_normalization);
   TestSuite_Add (suite,
                  "/bson/decimal128/from_string/zero",
                  test_decimal128_from_string__zeros);
}
