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

#include "bson-compat.h"

#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "bson-context-private.h"
#include "bson-md5.h"
#include "bson-oid.h"
#include "bson-string.h"


/*
 * This table contains an array of two character pairs for every possible
 * uint8_t. It is used as a lookup table when encoding a bson_oid_t
 * to hex formatted ASCII. Performing two characters at a time roughly
 * reduces the number of operations by one-half.
 */
static const uint16_t gHexCharPairs[] = {
#if BSON_BYTE_ORDER == BSON_BIG_ENDIAN
   12336, 12337, 12338, 12339, 12340, 12341, 12342, 12343, 12344, 12345, 12385,
   12386, 12387, 12388, 12389, 12390, 12592, 12593, 12594, 12595, 12596, 12597,
   12598, 12599, 12600, 12601, 12641, 12642, 12643, 12644, 12645, 12646, 12848,
   12849, 12850, 12851, 12852, 12853, 12854, 12855, 12856, 12857, 12897, 12898,
   12899, 12900, 12901, 12902, 13104, 13105, 13106, 13107, 13108, 13109, 13110,
   13111, 13112, 13113, 13153, 13154, 13155, 13156, 13157, 13158, 13360, 13361,
   13362, 13363, 13364, 13365, 13366, 13367, 13368, 13369, 13409, 13410, 13411,
   13412, 13413, 13414, 13616, 13617, 13618, 13619, 13620, 13621, 13622, 13623,
   13624, 13625, 13665, 13666, 13667, 13668, 13669, 13670, 13872, 13873, 13874,
   13875, 13876, 13877, 13878, 13879, 13880, 13881, 13921, 13922, 13923, 13924,
   13925, 13926, 14128, 14129, 14130, 14131, 14132, 14133, 14134, 14135, 14136,
   14137, 14177, 14178, 14179, 14180, 14181, 14182, 14384, 14385, 14386, 14387,
   14388, 14389, 14390, 14391, 14392, 14393, 14433, 14434, 14435, 14436, 14437,
   14438, 14640, 14641, 14642, 14643, 14644, 14645, 14646, 14647, 14648, 14649,
   14689, 14690, 14691, 14692, 14693, 14694, 24880, 24881, 24882, 24883, 24884,
   24885, 24886, 24887, 24888, 24889, 24929, 24930, 24931, 24932, 24933, 24934,
   25136, 25137, 25138, 25139, 25140, 25141, 25142, 25143, 25144, 25145, 25185,
   25186, 25187, 25188, 25189, 25190, 25392, 25393, 25394, 25395, 25396, 25397,
   25398, 25399, 25400, 25401, 25441, 25442, 25443, 25444, 25445, 25446, 25648,
   25649, 25650, 25651, 25652, 25653, 25654, 25655, 25656, 25657, 25697, 25698,
   25699, 25700, 25701, 25702, 25904, 25905, 25906, 25907, 25908, 25909, 25910,
   25911, 25912, 25913, 25953, 25954, 25955, 25956, 25957, 25958, 26160, 26161,
   26162, 26163, 26164, 26165, 26166, 26167, 26168, 26169, 26209, 26210, 26211,
   26212, 26213, 26214
#else
   12336, 12592, 12848, 13104, 13360, 13616, 13872, 14128, 14384, 14640, 24880,
   25136, 25392, 25648, 25904, 26160, 12337, 12593, 12849, 13105, 13361, 13617,
   13873, 14129, 14385, 14641, 24881, 25137, 25393, 25649, 25905, 26161, 12338,
   12594, 12850, 13106, 13362, 13618, 13874, 14130, 14386, 14642, 24882, 25138,
   25394, 25650, 25906, 26162, 12339, 12595, 12851, 13107, 13363, 13619, 13875,
   14131, 14387, 14643, 24883, 25139, 25395, 25651, 25907, 26163, 12340, 12596,
   12852, 13108, 13364, 13620, 13876, 14132, 14388, 14644, 24884, 25140, 25396,
   25652, 25908, 26164, 12341, 12597, 12853, 13109, 13365, 13621, 13877, 14133,
   14389, 14645, 24885, 25141, 25397, 25653, 25909, 26165, 12342, 12598, 12854,
   13110, 13366, 13622, 13878, 14134, 14390, 14646, 24886, 25142, 25398, 25654,
   25910, 26166, 12343, 12599, 12855, 13111, 13367, 13623, 13879, 14135, 14391,
   14647, 24887, 25143, 25399, 25655, 25911, 26167, 12344, 12600, 12856, 13112,
   13368, 13624, 13880, 14136, 14392, 14648, 24888, 25144, 25400, 25656, 25912,
   26168, 12345, 12601, 12857, 13113, 13369, 13625, 13881, 14137, 14393, 14649,
   24889, 25145, 25401, 25657, 25913, 26169, 12385, 12641, 12897, 13153, 13409,
   13665, 13921, 14177, 14433, 14689, 24929, 25185, 25441, 25697, 25953, 26209,
   12386, 12642, 12898, 13154, 13410, 13666, 13922, 14178, 14434, 14690, 24930,
   25186, 25442, 25698, 25954, 26210, 12387, 12643, 12899, 13155, 13411, 13667,
   13923, 14179, 14435, 14691, 24931, 25187, 25443, 25699, 25955, 26211, 12388,
   12644, 12900, 13156, 13412, 13668, 13924, 14180, 14436, 14692, 24932, 25188,
   25444, 25700, 25956, 26212, 12389, 12645, 12901, 13157, 13413, 13669, 13925,
   14181, 14437, 14693, 24933, 25189, 25445, 25701, 25957, 26213, 12390, 12646,
   12902, 13158, 13414, 13670, 13926, 14182, 14438, 14694, 24934, 25190, 25446,
   25702, 25958, 26214
#endif
};


/*
 *--------------------------------------------------------------------------
 *
 * bson_oid_init_sequence --
 *
 *       Initializes @oid with the next oid in the sequence. The first 4
 *       bytes contain the current time and the following 8 contain a 64-bit
 *       integer in big-endian format.
 *
 *       The bson_oid_t generated by this function is not guaranteed to be
 *       globally unique. Only unique within this context. It is however,
 *       guaranteed to be sequential.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       @oid is initialized.
 *
 *--------------------------------------------------------------------------
 */

void
bson_oid_init_sequence (bson_oid_t *oid,         /* OUT */
                        bson_context_t *context) /* IN */
{
   uint32_t now = (uint32_t) (time (NULL));

   if (!context) {
      context = bson_context_get_default ();
   }

   now = BSON_UINT32_TO_BE (now);

   memcpy (&oid->bytes[0], &now, sizeof (now));
   context->oid_get_seq64 (context, oid);
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_oid_init --
 *
 *       Generates bytes for a new bson_oid_t and stores them in @oid. The
 *       bytes will be generated according to the specification and includes
 *       the current time, first 3 bytes of MD5(hostname), pid (or tid), and
 *       monotonic counter.
 *
 *       The bson_oid_t generated by this function is not guaranteed to be
 *       globally unique. Only unique within this context. It is however,
 *       guaranteed to be sequential.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       @oid is initialized.
 *
 *--------------------------------------------------------------------------
 */

void
bson_oid_init (bson_oid_t *oid,         /* OUT */
               bson_context_t *context) /* IN */
{
   uint32_t now = (uint32_t) (time (NULL));

   BSON_ASSERT (oid);

   if (!context) {
      context = bson_context_get_default ();
   }

   now = BSON_UINT32_TO_BE (now);
   memcpy (&oid->bytes[0], &now, sizeof (now));

   context->oid_get_host (context, oid);
   context->oid_get_pid (context, oid);
   context->oid_get_seq32 (context, oid);
}


/**
 * bson_oid_init_from_data:
 * @oid: A bson_oid_t to initialize.
 * @bytes: A 12-byte buffer to copy into @oid.
 *
 */
/*
 *--------------------------------------------------------------------------
 *
 * bson_oid_init_from_data --
 *
 *       Initializes an @oid from @data. @data MUST be a buffer of at least
 *       12 bytes. This method is analagous to memcpy()'ing data into @oid.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       @oid is initialized.
 *
 *--------------------------------------------------------------------------
 */

void
bson_oid_init_from_data (bson_oid_t *oid,     /* OUT */
                         const uint8_t *data) /* IN */
{
   BSON_ASSERT (oid);
   BSON_ASSERT (data);

   memcpy (oid, data, 12);
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_oid_init_from_string --
 *
 *       Parses @str containing hex formatted bytes of an object id and
 *       places the bytes in @oid.
 *
 * Parameters:
 *       @oid: A bson_oid_t
 *       @str: A string containing at least 24 characters.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       @oid is initialized.
 *
 *--------------------------------------------------------------------------
 */

void
bson_oid_init_from_string (bson_oid_t *oid, /* OUT */
                           const char *str) /* IN */
{
   BSON_ASSERT (oid);
   BSON_ASSERT (str);

   bson_oid_init_from_string_unsafe (oid, str);
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_oid_get_time_t --
 *
 *       Fetches the time for which @oid was created.
 *
 * Returns:
 *       A time_t.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

time_t
bson_oid_get_time_t (const bson_oid_t *oid) /* IN */
{
   BSON_ASSERT (oid);

   return bson_oid_get_time_t_unsafe (oid);
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_oid_to_string --
 *
 *       Formats a bson_oid_t into a string. @str must contain enough bytes
 *       for the resulting string which is 25 bytes with a terminating
 *       NUL-byte.
 *
 * Parameters:
 *       @oid: A bson_oid_t.
 *       @str: A location to store the resulting string.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

void
bson_oid_to_string (const bson_oid_t *oid,                       /* IN */
                    char str[BSON_ENSURE_ARRAY_PARAM_SIZE (25)]) /* OUT */
{
#if !defined(__i386__) && !defined(__x86_64__) && !defined(_M_IX86) && \
   !defined(_M_X64)
   BSON_ASSERT (oid);
   BSON_ASSERT (str);

   bson_snprintf (str,
                  25,
                  "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                  oid->bytes[0],
                  oid->bytes[1],
                  oid->bytes[2],
                  oid->bytes[3],
                  oid->bytes[4],
                  oid->bytes[5],
                  oid->bytes[6],
                  oid->bytes[7],
                  oid->bytes[8],
                  oid->bytes[9],
                  oid->bytes[10],
                  oid->bytes[11]);
#else
   uint16_t *dst;
   uint8_t *id = (uint8_t *) oid;

   BSON_ASSERT (oid);
   BSON_ASSERT (str);

   dst = (uint16_t *) (void *) str;
   dst[0] = gHexCharPairs[id[0]];
   dst[1] = gHexCharPairs[id[1]];
   dst[2] = gHexCharPairs[id[2]];
   dst[3] = gHexCharPairs[id[3]];
   dst[4] = gHexCharPairs[id[4]];
   dst[5] = gHexCharPairs[id[5]];
   dst[6] = gHexCharPairs[id[6]];
   dst[7] = gHexCharPairs[id[7]];
   dst[8] = gHexCharPairs[id[8]];
   dst[9] = gHexCharPairs[id[9]];
   dst[10] = gHexCharPairs[id[10]];
   dst[11] = gHexCharPairs[id[11]];
   str[24] = '\0';
#endif
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_oid_hash --
 *
 *       Hashes the bytes of the provided bson_oid_t using DJB hash.  This
 *       allows bson_oid_t to be used as keys in a hash table.
 *
 * Returns:
 *       A hash value corresponding to @oid.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

uint32_t
bson_oid_hash (const bson_oid_t *oid) /* IN */
{
   BSON_ASSERT (oid);

   return bson_oid_hash_unsafe (oid);
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_oid_compare --
 *
 *       A qsort() style compare function that will return less than zero if
 *       @oid1 is less than @oid2, zero if they are the same, and greater
 *       than zero if @oid2 is greater than @oid1.
 *
 * Returns:
 *       A qsort() style compare integer.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

int
bson_oid_compare (const bson_oid_t *oid1, /* IN */
                  const bson_oid_t *oid2) /* IN */
{
   BSON_ASSERT (oid1);
   BSON_ASSERT (oid2);

   return bson_oid_compare_unsafe (oid1, oid2);
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_oid_equal --
 *
 *       Compares for equality of @oid1 and @oid2. If they are equal, then
 *       true is returned, otherwise false.
 *
 * Returns:
 *       A boolean indicating the equality of @oid1 and @oid2.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

bool
bson_oid_equal (const bson_oid_t *oid1, /* IN */
                const bson_oid_t *oid2) /* IN */
{
   BSON_ASSERT (oid1);
   BSON_ASSERT (oid2);

   return bson_oid_equal_unsafe (oid1, oid2);
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_oid_copy --
 *
 *       Copies the contents of @src to @dst.
 *
 * Parameters:
 *       @src: A bson_oid_t to copy from.
 *       @dst: A bson_oid_t to copy to.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       @dst will contain a copy of the data in @src.
 *
 *--------------------------------------------------------------------------
 */

void
bson_oid_copy (const bson_oid_t *src, /* IN */
               bson_oid_t *dst)       /* OUT */
{
   BSON_ASSERT (src);
   BSON_ASSERT (dst);

   bson_oid_copy_unsafe (src, dst);
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_oid_is_valid --
 *
 *       Validates that @str is a valid OID string. @length MUST be 24, but
 *       is provided as a parameter to simplify calling code.
 *
 * Parameters:
 *       @str: A string to validate.
 *       @length: The length of @str.
 *
 * Returns:
 *       true if @str can be passed to bson_oid_init_from_string().
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

bool
bson_oid_is_valid (const char *str, /* IN */
                   size_t length)   /* IN */
{
   size_t i;

   BSON_ASSERT (str);

   if ((length == 25) && (str[24] == '\0')) {
      length = 24;
   }

   if (length == 24) {
      for (i = 0; i < length; i++) {
         switch (str[i]) {
         case '0':
         case '1':
         case '2':
         case '3':
         case '4':
         case '5':
         case '6':
         case '7':
         case '8':
         case '9':
         case 'a':
         case 'b':
         case 'c':
         case 'd':
         case 'e':
         case 'f':
         case 'A':
         case 'B':
         case 'C':
         case 'D':
         case 'E':
         case 'F':
            break;
         default:
            return false;
         }
      }
      return true;
   }

   return false;
}
