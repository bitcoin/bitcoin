
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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "bson-decimal128.h"
#include "bson-types.h"
#include "bson-macros.h"
#include "bson-string.h"


#define BSON_DECIMAL128_EXPONENT_MAX 6111
#define BSON_DECIMAL128_EXPONENT_MIN -6176
#define BSON_DECIMAL128_EXPONENT_BIAS 6176
#define BSON_DECIMAL128_MAX_DIGITS 34

#define BSON_DECIMAL128_SET_NAN(dec)      \
   do {                                   \
      (dec).high = 0x7c00000000000000ull; \
      (dec).low = 0;                      \
   } while (0);
#define BSON_DECIMAL128_SET_INF(dec, isneg)                                 \
   do {                                                                     \
      (dec).high = 0x7800000000000000ull + 0x8000000000000000ull * (isneg); \
      (dec).low = 0;                                                        \
   } while (0);

/**
 * _bson_uint128_t:
 *
 * This struct represents a 128 bit integer.
 */
typedef struct {
   uint32_t parts[4]; /* 32-bit words stored high to low. */
} _bson_uint128_t;


/**
 *------------------------------------------------------------------------------
 *
 * _bson_uint128_divide1B --
 *
 *    This function divides a #_bson_uint128_t by 1000000000 (1 billion) and
 *    computes the quotient and remainder.
 *
 *    The remainder will contain 9 decimal digits for conversion to string.
 *
 * @value     The #_bson_uint128_t operand.
 * @quotient  A pointer to store the #_bson_uint128_t quotient.
 * @rem       A pointer to store the #uint64_t remainder.
 *
 * Returns:
 *    The quotient at @quotient and the remainder at @rem.
 *
 * Side effects:
 *    None.
 *
 *------------------------------------------------------------------------------
 */
static void
_bson_uint128_divide1B (_bson_uint128_t value,     /* IN */
                        _bson_uint128_t *quotient, /* OUT */
                        uint32_t *rem)             /* OUT */
{
   const uint32_t DIVISOR = 1000 * 1000 * 1000;
   uint64_t _rem = 0;
   int i = 0;

   if (!value.parts[0] && !value.parts[1] && !value.parts[2] &&
       !value.parts[3]) {
      *quotient = value;
      *rem = 0;
      return;
   }


   for (i = 0; i <= 3; i++) {
      _rem <<= 32; /* Adjust remainder to match value of next dividend */
      _rem += value.parts[i]; /* Add the divided to _rem */
      value.parts[i] = (uint32_t) (_rem / DIVISOR);
      _rem %= DIVISOR; /* Store the remainder */
   }

   *quotient = value;
   *rem = (uint32_t) _rem;
}


/**
 *------------------------------------------------------------------------------
 *
 * bson_decimal128_to_string --
 *
 *    This function converts a BID formatted decimal128 value to string,
 *    accepting a &bson_decimal128_t as @dec.  The string is stored at @str.
 *
 * @dec : The BID formatted decimal to convert.
 * @str : The output decimal128 string. At least %BSON_DECIMAL128_STRING
 *characters.
 *
 * Returns:
 *    None.
 *
 * Side effects:
 *    None.
 *
 *------------------------------------------------------------------------------
 */
void
bson_decimal128_to_string (const bson_decimal128_t *dec, /* IN  */
                           char *str)                    /* OUT */
{
   uint32_t COMBINATION_MASK = 0x1f;   /* Extract least significant 5 bits */
   uint32_t EXPONENT_MASK = 0x3fff;    /* Extract least significant 14 bits */
   uint32_t COMBINATION_INFINITY = 30; /* Value of combination field for Inf */
   uint32_t COMBINATION_NAN = 31;      /* Value of combination field for NaN */
   uint32_t EXPONENT_BIAS = 6176;      /* decimal128 exponent bias */

   char *str_out = str;      /* output pointer in string */
   char significand_str[35]; /* decoded significand digits */


   /* Note: bits in this routine are referred to starting at 0, */
   /* from the sign bit, towards the coefficient. */
   uint32_t high;                   /* bits 0 - 31 */
   uint32_t midh;                   /* bits 32 - 63 */
   uint32_t midl;                   /* bits 64 - 95 */
   uint32_t low;                    /* bits 96 - 127 */
   uint32_t combination;            /* bits 1 - 5 */
   uint32_t biased_exponent;        /* decoded biased exponent (14 bits) */
   uint32_t significand_digits = 0; /* the number of significand digits */
   uint32_t significand[36] = {0};  /* the base-10 digits in the significand */
   uint32_t *significand_read = significand; /* read pointer into significand */
   int32_t exponent;                         /* unbiased exponent */
   int32_t scientific_exponent; /* the exponent if scientific notation is
                                 * used */
   bool is_zero = false;        /* true if the number is zero */

   uint8_t significand_msb; /* the most signifcant significand bits (50-46) */
   _bson_uint128_t
      significand128; /* temporary storage for significand decoding */
   size_t i;          /* indexing variables */
   int j, k;

   memset (significand_str, 0, sizeof (significand_str));

   if ((int64_t) dec->high < 0) { /* negative */
      *(str_out++) = '-';
   }

   low = (uint32_t) dec->low, midl = (uint32_t) (dec->low >> 32),
   midh = (uint32_t) dec->high, high = (uint32_t) (dec->high >> 32);

   /* Decode combination field and exponent */
   combination = (high >> 26) & COMBINATION_MASK;

   if (BSON_UNLIKELY ((combination >> 3) == 3)) {
      /* Check for 'special' values */
      if (combination == COMBINATION_INFINITY) { /* Infinity */
         strcpy (str_out, BSON_DECIMAL128_INF);
         return;
      } else if (combination == COMBINATION_NAN) { /* NaN */
         /* str, not str_out, to erase the sign */
         strcpy (str, BSON_DECIMAL128_NAN);
         /* we don't care about the NaN payload. */
         return;
      } else {
         biased_exponent = (high >> 15) & EXPONENT_MASK;
         significand_msb = 0x8 + ((high >> 14) & 0x1);
      }
   } else {
      significand_msb = (high >> 14) & 0x7;
      biased_exponent = (high >> 17) & EXPONENT_MASK;
   }

   exponent = biased_exponent - EXPONENT_BIAS;
   /* Create string of significand digits */

   /* Convert the 114-bit binary number represented by */
   /* (high, midh, midl, low) to at most 34 decimal */
   /* digits through modulo and division. */
   significand128.parts[0] = (high & 0x3fff) + ((significand_msb & 0xf) << 14);
   significand128.parts[1] = midh;
   significand128.parts[2] = midl;
   significand128.parts[3] = low;

   if (significand128.parts[0] == 0 && significand128.parts[1] == 0 &&
       significand128.parts[2] == 0 && significand128.parts[3] == 0) {
      is_zero = true;
   } else if (significand128.parts[0] >= (1 << 17)) {
      /* The significand is non-canonical or zero.
       * In order to preserve compatability with the densely packed decimal
       * format, the maximum value for the significand of decimal128 is
       * 1e34 - 1.  If the value is greater than 1e34 - 1, the IEEE 754
       * standard dictates that the significand is interpreted as zero.
       */
      is_zero = true;
   } else {
      for (k = 3; k >= 0; k--) {
         uint32_t least_digits = 0;
         _bson_uint128_divide1B (
            significand128, &significand128, &least_digits);

         /* We now have the 9 least significant digits (in base 2). */
         /* Convert and output to string. */
         if (!least_digits) {
            continue;
         }

         for (j = 8; j >= 0; j--) {
            significand[k * 9 + j] = least_digits % 10;
            least_digits /= 10;
         }
      }
   }

   /* Output format options: */
   /* Scientific - [-]d.dddE(+/-)dd or [-]dE(+/-)dd */
   /* Regular    - ddd.ddd */

   if (is_zero) {
      significand_digits = 1;
      *significand_read = 0;
   } else {
      significand_digits = 36;
      while (!(*significand_read)) {
         significand_digits--;
         significand_read++;
      }
   }

   scientific_exponent = significand_digits - 1 + exponent;

   /* The scientific exponent checks are dictated by the string conversion
    * specification and are somewhat arbitrary cutoffs.
    *
    * We must check exponent > 0, because if this is the case, the number
    * has trailing zeros.  However, we *cannot* output these trailing zeros,
    * because doing so would change the precision of the value, and would
    * change stored data if the string converted number is round tripped.
    */
   if (scientific_exponent < -6 || exponent > 0) {
      /* Scientific format */
      *(str_out++) = *(significand_read++) + '0';
      significand_digits--;

      if (significand_digits) {
         *(str_out++) = '.';
      }

      for (i = 0; i < significand_digits; i++) {
         *(str_out++) = *(significand_read++) + '0';
      }
      /* Exponent */
      *(str_out++) = 'E';
      bson_snprintf (str_out, 6, "%+d", scientific_exponent);
   } else {
      /* Regular format with no decimal place */
      if (exponent >= 0) {
         for (i = 0; i < significand_digits; i++) {
            *(str_out++) = *(significand_read++) + '0';
         }
         *str_out = '\0';
      } else {
         int32_t radix_position = significand_digits + exponent;

         if (radix_position > 0) { /* non-zero digits before radix */
            for (i = 0; i < radix_position; i++) {
               *(str_out++) = *(significand_read++) + '0';
            }
         } else { /* leading zero before radix point */
            *(str_out++) = '0';
         }

         *(str_out++) = '.';
         while (radix_position++ < 0) { /* add leading zeros after radix */
            *(str_out++) = '0';
         }

         for (i = 0; i < significand_digits - BSON_MAX (radix_position - 1, 0);
              i++) {
            *(str_out++) = *(significand_read++) + '0';
         }
         *str_out = '\0';
      }
   }
}

typedef struct {
   uint64_t high, low;
} _bson_uint128_6464_t;


/**
 *-------------------------------------------------------------------------
 *
 * mul64x64 --
 *
 *    This function multiplies two &uint64_t into a &_bson_uint128_6464_t.
 *
 * Returns:
 *    The product of @left and @right.
 *
 * Side Effects:
 *    None.
 *
 *-------------------------------------------------------------------------
 */
static void
_mul_64x64 (uint64_t left,                 /* IN */
            uint64_t right,                /* IN */
            _bson_uint128_6464_t *product) /* OUT */
{
   uint64_t left_high, left_low, right_high, right_low, product_high,
      product_mid, product_mid2, product_low;
   _bson_uint128_6464_t rt = {0};

   if (!left && !right) {
      *product = rt;
      return;
   }

   left_high = left >> 32;
   left_low = (uint32_t) left;
   right_high = right >> 32;
   right_low = (uint32_t) right;

   product_high = left_high * right_high;
   product_mid = left_high * right_low;
   product_mid2 = left_low * right_high;
   product_low = left_low * right_low;

   product_high += product_mid >> 32;
   product_mid = (uint32_t) product_mid + product_mid2 + (product_low >> 32);

   product_high = product_high + (product_mid >> 32);
   product_low = (product_mid << 32) + (uint32_t) product_low;

   rt.high = product_high;
   rt.low = product_low;
   *product = rt;
}

/**
 *------------------------------------------------------------------------------
 *
 * _dec128_tolower --
 *
 *    This function converts the ASCII character @c to lowercase.  It is locale
 *    insensitive (unlike the stdlib tolower).
 *
 * Returns:
 *    The lowercased character.
 */
char
_dec128_tolower (char c)
{
   if (isupper (c)) {
      c += 32;
   }

   return c;
}

/**
 *------------------------------------------------------------------------------
 *
 * _dec128_istreq --
 *
 *    This function compares the null-terminated *ASCII* strings @a and @b
 *    for case-insensitive equality.
 *
 * Returns:
 *    true if the strings are equal, false otherwise.
 */
bool
_dec128_istreq (const char *a, /* IN */
                const char *b /* IN */)
{
   while (*a != '\0' || *b != '\0') {
      /* strings are different lengths. */
      if (*a == '\0' || *b == '\0') {
         return false;
      }

      if (_dec128_tolower (*a) != _dec128_tolower (*b)) {
         return false;
      }

      a++;
      b++;
   }

   return true;
}

/**
 *------------------------------------------------------------------------------
 *
 * bson_decimal128_from_string --
 *
 *    This function converts @string in the format [+-]ddd[.]ddd[E][+-]dddd to
 *    decimal128.  Out of range values are converted to +/-Infinity.  Invalid
 *    strings are converted to NaN.
 *
 *    If more digits are provided than the available precision allows,
 *    round to the nearest expressable decimal128 with ties going to even will
 *    occur.
 *
 *    Note: @string must be ASCII only!
 *
 * Returns:
 *    true on success, or false on failure. @dec will be NaN if @str was invalid
 *    The &bson_decimal128_t converted from @string at @dec.
 *
 * Side effects:
 *    None.
 *
 *------------------------------------------------------------------------------
 */
bool
bson_decimal128_from_string (const char *string,     /* IN */
                             bson_decimal128_t *dec) /* OUT */
{
   _bson_uint128_6464_t significand = {0};

   const char *str_read = string; /* Read pointer for consuming str. */

   /* Parsing state tracking */
   bool is_negative = false;
   bool saw_radix = false;
   bool includes_sign = false; /* True if the input string contains a sign. */
   bool found_nonzero = false;

   size_t significant_digits = 0; /* Total number of significant digits
                                   * (no leading or trailing zero) */
   size_t ndigits_read = 0;       /* Total number of significand digits read */
   size_t ndigits = 0;        /* Total number of digits (no leading zeros) */
   size_t radix_position = 0; /* The number of the digits after radix */
   size_t first_nonzero = 0;  /* The index of the first non-zero in *str* */

   uint16_t digits[BSON_DECIMAL128_MAX_DIGITS] = {0};
   uint16_t ndigits_stored = 0;      /* The number of digits in digits */
   uint16_t *digits_insert = digits; /* Insertion pointer for digits */
   size_t first_digit = 0;           /* The index of the first non-zero digit */
   size_t last_digit = 0;            /* The index of the last digit */

   int32_t exponent = 0;
   uint64_t significand_high = 0; /* The high 17 digits of the significand */
   uint64_t significand_low = 0;  /* The low 17 digits of the significand */
   uint16_t biased_exponent = 0;  /* The biased exponent */

   BSON_ASSERT (dec);
   dec->high = 0;
   dec->low = 0;

   if (*str_read == '+' || *str_read == '-') {
      is_negative = *(str_read++) == '-';
      includes_sign = true;
   }

   /* Check for Infinity or NaN */
   if (!isdigit (*str_read) && *str_read != '.') {
      if (_dec128_istreq (str_read, "inf") ||
          _dec128_istreq (str_read, "infinity")) {
         BSON_DECIMAL128_SET_INF (*dec, is_negative);
         return true;
      } else if (_dec128_istreq (str_read, "nan")) {
         BSON_DECIMAL128_SET_NAN (*dec);
         return true;
      }

      BSON_DECIMAL128_SET_NAN (*dec);
      return false;
   }

   /* Read digits */
   while (isdigit (*str_read) || *str_read == '.') {
      if (*str_read == '.') {
         if (saw_radix) {
            BSON_DECIMAL128_SET_NAN (*dec);
            return false;
         }

         saw_radix = true;
         str_read++;
         continue;
      }

      if (ndigits_stored < 34) {
         if (*str_read != '0' || found_nonzero) {
            if (!found_nonzero) {
               first_nonzero = ndigits_read;
            }

            found_nonzero = true;
            *(digits_insert++) = *(str_read) - '0'; /* Only store 34 digits */
            ndigits_stored++;
         }
      }

      if (found_nonzero) {
         ndigits++;
      }

      if (saw_radix) {
         radix_position++;
      }

      ndigits_read++;
      str_read++;
   }

   if (saw_radix && !ndigits_read) {
      BSON_DECIMAL128_SET_NAN (*dec);
      return false;
   }

   /* Read exponent if exists */
   if (*str_read == 'e' || *str_read == 'E') {
      int nread = 0;
#ifdef _MSC_VER
#define SSCANF sscanf_s
#else
#define SSCANF sscanf
#endif
      int read_exponent = SSCANF (++str_read, "%d%n", &exponent, &nread);
      str_read += nread;

      if (!read_exponent || nread == 0) {
         BSON_DECIMAL128_SET_NAN (*dec);
         return false;
      }

#undef SSCANF
   }

   if (*str_read) {
      BSON_DECIMAL128_SET_NAN (*dec);
      return false;
   }

   /* Done reading input. */
   /* Find first non-zero digit in digits */
   first_digit = 0;

   if (!ndigits_stored) { /* value is zero */
      first_digit = 0;
      last_digit = 0;
      digits[0] = 0;
      ndigits = 1;
      ndigits_stored = 1;
      significant_digits = 0;
   } else {
      last_digit = ndigits_stored - 1;
      significant_digits = ndigits;
      /* Mark trailing zeros as non-significant */
      while (string[first_nonzero + significant_digits - 1 + includes_sign +
                    saw_radix] == '0') {
         significant_digits--;
      }
   }


   /* Normalization of exponent */
   /* Correct exponent based on radix position, and shift significand as needed
    */
   /* to represent user input */

   /* Overflow prevention */
   if (exponent <= radix_position && radix_position - exponent > (1 << 14)) {
      exponent = BSON_DECIMAL128_EXPONENT_MIN;
   } else {
      exponent -= radix_position;
   }

   /* Attempt to normalize the exponent */
   while (exponent > BSON_DECIMAL128_EXPONENT_MAX) {
      /* Shift exponent to significand and decrease */
      last_digit++;

      if (last_digit - first_digit > BSON_DECIMAL128_MAX_DIGITS) {
         /* The exponent is too great to shift into the significand. */
         if (significant_digits == 0) {
            /* Value is zero, we are allowed to clamp the exponent. */
            exponent = BSON_DECIMAL128_EXPONENT_MAX;
            break;
         }

         /* Overflow is not permitted, error. */
         BSON_DECIMAL128_SET_NAN (*dec);
         return false;
      }

      exponent--;
   }

   while (exponent < BSON_DECIMAL128_EXPONENT_MIN || ndigits_stored < ndigits) {
      /* Shift last digit */
      if (last_digit == 0) {
         /* underflow is not allowed, but zero clamping is */
         if (significant_digits == 0) {
            exponent = BSON_DECIMAL128_EXPONENT_MIN;
            break;
         }

         BSON_DECIMAL128_SET_NAN (*dec);
         return false;
      }

      if (ndigits_stored < ndigits) {
         if (string[ndigits - 1 + includes_sign + saw_radix] - '0' != 0 &&
             significant_digits != 0) {
            BSON_DECIMAL128_SET_NAN (*dec);
            return false;
         }

         ndigits--; /* adjust to match digits not stored */
      } else {
         if (digits[last_digit] != 0) {
            /* Inexact rounding is not allowed. */
            BSON_DECIMAL128_SET_NAN (*dec);
            return false;
         }


         last_digit--; /* adjust to round */
      }

      if (exponent < BSON_DECIMAL128_EXPONENT_MAX) {
         exponent++;
      } else {
         BSON_DECIMAL128_SET_NAN (*dec);
         return false;
      }
   }

   /* Round */
   /* We've normalized the exponent, but might still need to round. */
   if (last_digit - first_digit + 1 < significant_digits) {
      uint8_t round_digit;

      /* There are non-zero digits after last_digit that need rounding. */
      /* We round to nearest, ties to even */
      round_digit =
         string[first_nonzero + last_digit + includes_sign + saw_radix + 1] -
         '0';

      if (round_digit != 0) {
         /* Inexact (non-zero) rounding is not allowed */
         BSON_DECIMAL128_SET_NAN (*dec);
         return false;
      }
   }

   /* Encode significand */
   significand_high = 0,   /* The high 17 digits of the significand */
      significand_low = 0; /* The low 17 digits of the significand */

   if (significant_digits == 0) { /* read a zero */
      significand_high = 0;
      significand_low = 0;
   } else if (last_digit - first_digit < 17) {
      size_t d_idx = first_digit;
      significand_low = digits[d_idx++];

      for (; d_idx <= last_digit; d_idx++) {
         significand_low *= 10;
         significand_low += digits[d_idx];
         significand_high = 0;
      }
   } else {
      size_t d_idx = first_digit;
      significand_high = digits[d_idx++];

      for (; d_idx <= last_digit - 17; d_idx++) {
         significand_high *= 10;
         significand_high += digits[d_idx];
      }

      significand_low = digits[d_idx++];

      for (; d_idx <= last_digit; d_idx++) {
         significand_low *= 10;
         significand_low += digits[d_idx];
      }
   }

   _mul_64x64 (significand_high, 100000000000000000ull, &significand);
   significand.low += significand_low;

   if (significand.low < significand_low) {
      significand.high += 1;
   }


   biased_exponent = (exponent + (int16_t) BSON_DECIMAL128_EXPONENT_BIAS);

   /* Encode combination, exponent, and significand. */
   if ((significand.high >> 49) & 1) {
      /* Encode '11' into bits 1 to 3 */
      dec->high |= (0x3ull << 61);
      dec->high |= (biased_exponent & 0x3fffull) << 47;
      dec->high |= significand.high & 0x7fffffffffffull;
   } else {
      dec->high |= (biased_exponent & 0x3fffull) << 49;
      dec->high |= significand.high & 0x1ffffffffffffull;
   }

   dec->low = significand.low;

   /* Encode sign */
   if (is_negative) {
      dec->high |= 0x8000000000000000ull;
   }

   return true;
}
