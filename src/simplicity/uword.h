#ifndef SIMPLICITY_UWORD_H
#define SIMPLICITY_UWORD_H

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

/* Number of bits for any value of the form 2^b - 1 where 0 <= b < 0x3fffffff * 30.
 * From: https://stackoverflow.com/a/4589384/727983
 *
 * A correctness proof can be found at
 *
 *     https://wiki.sei.cmu.edu/confluence/display/c/INT35-C.+Use+correct+integer+precisions?focusedCommentId=88018664#comment-88018664
 */
#define UMAX_BITS(m) ((m)             / ((m) % 0x3fffffffu + 1) / 0x3fffffffu % 0x3fffffffu * 30 \
                    + (m)%0x3fffffffu / ((m) % 31          + 1) / 31          % 31          * 5  \
                    + 4 - 12 / ((m) % 31 + 3))

/* UWORD can be any unsigned integer type.
 * The default ought to be 'unsigned int', but unfortunately that tends to be 32-bits on 64-bit platforms.
 * Instead we select 'uint_fast16_t' as our default (with the disadvantage that it is again 32-bits for the x32 target).
 */
#ifndef UWORD
#define UWORD uint_fast16_t
#endif

_Static_assert((UWORD)1 <= (UWORD)(-1), "UWORD must be an unsigned type");
_Static_assert(sizeof(UWORD) < 0x3fffffffu * 30 / CHAR_BIT, "UWORD must have fewer than 32 billion bits for UMAX_BITS.");
#define UWORD_MAX ((UWORD)-1)
#define UWORD_BIT UMAX_BITS(UWORD_MAX)
_Static_assert(UWORD_BIT <= SIZE_MAX, "UWORD_BIT must fit into size_t.");

/* For all 'x' and 'y',
 *
 *    'x <= UWORD_BIT * y' if and only if 'ROUND_UWORD(x) <= y'
 *
 * Precondition: 0 <= bitSize
 */
#define ROUND_UWORD(bitSize) ((bitSize) / UWORD_BIT + !!((bitSize) % UWORD_BIT))

/* Clear the 'n' least significant bits of a UWORD.
 * Precondition: 0 < n <= UWORD_BIT
 */
static inline UWORD LSBclear(UWORD x, size_t n) {
  return (UWORD)(x >> 1 >> (n - 1) << 1 << (n - 1));
}

/* Clear all but the 'n' least significant bits of a UWORD.
 * Precondition: 0 < n <= UWORD_BIT
 */
static inline UWORD LSBkeep(UWORD x, size_t n) {
  return (UWORD)(x & (UWORD_MAX >> (UWORD_BIT - n)));
}
#endif
