#ifndef SIMPLICITY_RSORT_H
#define SIMPLICITY_RSORT_H

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>

#include "limitations.h"
#include "sha256.h"

/* Sorts an array of pointers to 'sha256_midstate's in place in memcmp order.
 * If malloc fails, returns false.
 * Otherwise, returns true.
 *
 * The time complexity of rsort is O('len').
 *
 * We are sorting in memcmp order, which is the lexicographical order of the object representation, i.e. the order that one
 * gets when casting 'sha256_midstate' to a 'unsigned char[]'. This representation is implementation defined, and will differ
 * on big endian and little endian architectures.
 *
 * It is critical that the details of this order remain unobservable from the consensus rules.
 *
 * Precondition: For all 0 <= i < len, NULL != a[i];
 */
bool simplicity_rsort(const sha256_midstate** a, uint_fast32_t len);

/* Searches for duplicates in an array of 'sha256_midstate's.
 * If malloc fails, returns -1.
 * If no duplicates are found, returns 0.
 * If duplicates are found, returns a positive value.
 *
 * Precondition: const sha256_midstate a[len];
 *               len <= DAG_LEN_MAX;
 */
int simplicity_hasDuplicates(const sha256_midstate* a, uint_fast32_t len);

#endif
