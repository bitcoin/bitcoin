/* This modules defines a structure representing bit strings. */
#ifndef SIMPLICITY_BITSTRING_H
#define SIMPLICITY_BITSTRING_H

#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <stdbool.h>
#include "simplicity_assert.h"

/* Represents a bitstring of length 'len' bits using an array of unsigned char.
 * The bit at index 'n', where 0 <= 'n' < 'len', is located at bit '1 << (CHAR_BIT - 1 - (offset + n) % CHAR_BIT)' of
 * array element 'arr[(offset + n) / CHAR_BIT]'.
 * Other bits in the array may be any value.
 *
 * Invariant: len <= 2^31
 *            offset + length <= SIZE_MAX
 *            0 < len implies unsigned char arr[(offset + len - 1) / CHAR_BIT + 1];
 */
typedef struct bitstring {
  const unsigned char* arr;
  size_t len;
  size_t offset;
} bitstring;

/* Return the nth bit from a bitstring.
 *
 * Precondition: NULL != s
 *               n < s->len;
 */
static inline bool getBit(const bitstring *s, size_t n) {
  size_t total_offset = s->offset + n;
  simplicity_assert(n < s->len);
  return 1 & (s->arr[total_offset / CHAR_BIT] >> (CHAR_BIT - 1 - (total_offset % CHAR_BIT)));
}

/* Return 8 bits from a bitstring staring from the nth bit.
 *
 * Precondition: NULL != s
 *               n + 8 <= s->len;
 */
static inline uint_fast8_t getByte(const bitstring *s, size_t n) {
  simplicity_assert(8 <= s->len);
  simplicity_assert(n <= s->len - 8);
  size_t total_offset = s->offset + n;
  if (total_offset % CHAR_BIT <= CHAR_BIT - 8) {
    return (uint_fast8_t)(0xff & (s->arr[total_offset / CHAR_BIT] >> (CHAR_BIT - 8 - (total_offset % CHAR_BIT))));
  } else {
    /* CHAR_BIT < total_offset % CHAR_BIT + 8 */
    return (uint_fast8_t)(0xff & (((1U * s->arr[total_offset / CHAR_BIT]) << (total_offset % CHAR_BIT - (CHAR_BIT - 8)))
                                  | (s->arr[total_offset / CHAR_BIT + 1] >> (CHAR_BIT - (total_offset % CHAR_BIT - (CHAR_BIT - 8))))));
  }
}
#endif
