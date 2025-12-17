#ifndef SIMPLICITY_BOUNDED_H
#define SIMPLICITY_BOUNDED_H

#include <stdbool.h>
#include <stdint.h>

typedef uint_least32_t ubounded;
#define UBOUNDED_MAX UINT32_MAX

static inline ubounded bounded_max(ubounded x, ubounded y) {
  return x <= y ? y : x;
}

/* Returns min(x + y, UBOUNDED_MAX) */
static inline ubounded bounded_add(ubounded x, ubounded y) {
  return UBOUNDED_MAX < x ? UBOUNDED_MAX
       : UBOUNDED_MAX - x < y ? UBOUNDED_MAX
       : x + y;
}

/* *x = min(*x + 1, UBOUNDED_MAX) */
static inline void bounded_inc(ubounded* x) {
  if (*x < UBOUNDED_MAX) (*x)++;
}

/* 'pad(false, a, b)' computes the PADL(a, b) function.
 * 'pad( true, a, b)' computes the PADR(a, b) function.
 */
static inline ubounded pad(bool right, ubounded a, ubounded b) {
  return bounded_max(a, b) - (right ? b : a);
}

enum { overhead = 100 }; /* milli weight units */
#endif
