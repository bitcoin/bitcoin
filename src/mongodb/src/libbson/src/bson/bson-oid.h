/*
 * Copyright 2013 MongoDB, Inc.
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


#ifndef BSON_OID_H
#define BSON_OID_H


#if !defined(BSON_INSIDE) && !defined(BSON_COMPILATION)
#error "Only <bson.h> can be included directly."
#endif


#include <time.h>

#include "bson-context.h"
#include "bson-macros.h"
#include "bson-types.h"
#include "bson-endian.h"


BSON_BEGIN_DECLS


BSON_EXPORT (int)
bson_oid_compare (const bson_oid_t *oid1, const bson_oid_t *oid2);
BSON_EXPORT (void)
bson_oid_copy (const bson_oid_t *src, bson_oid_t *dst);
BSON_EXPORT (bool)
bson_oid_equal (const bson_oid_t *oid1, const bson_oid_t *oid2);
BSON_EXPORT (bool)
bson_oid_is_valid (const char *str, size_t length);
BSON_EXPORT (time_t)
bson_oid_get_time_t (const bson_oid_t *oid);
BSON_EXPORT (uint32_t)
bson_oid_hash (const bson_oid_t *oid);
BSON_EXPORT (void)
bson_oid_init (bson_oid_t *oid, bson_context_t *context);
BSON_EXPORT (void)
bson_oid_init_from_data (bson_oid_t *oid, const uint8_t *data);
BSON_EXPORT (void)
bson_oid_init_from_string (bson_oid_t *oid, const char *str);
BSON_EXPORT (void)
bson_oid_init_sequence (bson_oid_t *oid, bson_context_t *context);
BSON_EXPORT (void)
bson_oid_to_string (const bson_oid_t *oid, char str[25]);


/**
 * bson_oid_compare_unsafe:
 * @oid1: A bson_oid_t.
 * @oid2: A bson_oid_t.
 *
 * Performs a qsort() style comparison between @oid1 and @oid2.
 *
 * This function is meant to be as fast as possible and therefore performs
 * no argument validation. That is the callers responsibility.
 *
 * Returns: An integer < 0 if @oid1 is less than @oid2. Zero if they are equal.
 *          An integer > 0 if @oid1 is greater than @oid2.
 */
static BSON_INLINE int
bson_oid_compare_unsafe (const bson_oid_t *oid1, const bson_oid_t *oid2)
{
   return memcmp (oid1, oid2, sizeof *oid1);
}


/**
 * bson_oid_equal_unsafe:
 * @oid1: A bson_oid_t.
 * @oid2: A bson_oid_t.
 *
 * Checks the equality of @oid1 and @oid2.
 *
 * This function is meant to be as fast as possible and therefore performs
 * no checks for argument validity. That is the callers responsibility.
 *
 * Returns: true if @oid1 and @oid2 are equal; otherwise false.
 */
static BSON_INLINE bool
bson_oid_equal_unsafe (const bson_oid_t *oid1, const bson_oid_t *oid2)
{
   return !memcmp (oid1, oid2, sizeof *oid1);
}

/**
 * bson_oid_hash_unsafe:
 * @oid: A bson_oid_t.
 *
 * This function performs a DJB style hash upon the bytes contained in @oid.
 * The result is a hash key suitable for use in a hashtable.
 *
 * This function is meant to be as fast as possible and therefore performs no
 * validation of arguments. The caller is responsible to ensure they are
 * passing valid arguments.
 *
 * Returns: A uint32_t containing a hash code.
 */
static BSON_INLINE uint32_t
bson_oid_hash_unsafe (const bson_oid_t *oid)
{
   uint32_t hash = 5381;
   uint32_t i;

   for (i = 0; i < sizeof oid->bytes; i++) {
      hash = ((hash << 5) + hash) + oid->bytes[i];
   }

   return hash;
}


/**
 * bson_oid_copy_unsafe:
 * @src: A bson_oid_t to copy from.
 * @dst: A bson_oid_t to copy into.
 *
 * Copies the contents of @src into @dst. This function is meant to be as
 * fast as possible and therefore performs no argument checking. It is the
 * callers responsibility to ensure they are passing valid data into the
 * function.
 */
static BSON_INLINE void
bson_oid_copy_unsafe (const bson_oid_t *src, bson_oid_t *dst)
{
   memcpy (dst, src, sizeof *src);
}


/**
 * bson_oid_parse_hex_char:
 * @hex: A character to parse to its integer value.
 *
 * This function contains a jump table to return the integer value for a
 * character containing a hexidecimal value (0-9, a-f, A-F). If the character
 * is not a hexidecimal character then zero is returned.
 *
 * Returns: An integer between 0 and 15.
 */
static BSON_INLINE uint8_t
bson_oid_parse_hex_char (char hex)
{
   switch (hex) {
   case '0':
      return 0;
   case '1':
      return 1;
   case '2':
      return 2;
   case '3':
      return 3;
   case '4':
      return 4;
   case '5':
      return 5;
   case '6':
      return 6;
   case '7':
      return 7;
   case '8':
      return 8;
   case '9':
      return 9;
   case 'a':
   case 'A':
      return 0xa;
   case 'b':
   case 'B':
      return 0xb;
   case 'c':
   case 'C':
      return 0xc;
   case 'd':
   case 'D':
      return 0xd;
   case 'e':
   case 'E':
      return 0xe;
   case 'f':
   case 'F':
      return 0xf;
   default:
      return 0;
   }
}


/**
 * bson_oid_init_from_string_unsafe:
 * @oid: A bson_oid_t to store the result.
 * @str: A 24-character hexidecimal encoded string.
 *
 * Parses a string containing 24 hexidecimal encoded bytes into a bson_oid_t.
 * This function is meant to be as fast as possible and inlined into your
 * code. For that purpose, the function does not perform any sort of bounds
 * checking and it is the callers responsibility to ensure they are passing
 * valid input to the function.
 */
static BSON_INLINE void
bson_oid_init_from_string_unsafe (bson_oid_t *oid, const char *str)
{
   int i;

   for (i = 0; i < 12; i++) {
      oid->bytes[i] = ((bson_oid_parse_hex_char (str[2 * i]) << 4) |
                       (bson_oid_parse_hex_char (str[2 * i + 1])));
   }
}


/**
 * bson_oid_get_time_t_unsafe:
 * @oid: A bson_oid_t.
 *
 * Fetches the time @oid was generated.
 *
 * Returns: A time_t containing the UNIX timestamp of generation.
 */
static BSON_INLINE time_t
bson_oid_get_time_t_unsafe (const bson_oid_t *oid)
{
   uint32_t t;

   memcpy (&t, oid, sizeof (t));
   return BSON_UINT32_FROM_BE (t);
}


BSON_END_DECLS


#endif /* BSON_OID_H */
