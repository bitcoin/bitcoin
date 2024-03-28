/***********************************************************************
 * Copyright (c) 2021 Russell O'Connor, Jonas Nick                     *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_HSORT_H
#define SECP256K1_HSORT_H

#include <stddef.h>
#include <string.h>

/* In-place, iterative heapsort with an interface matching glibc's qsort_r. This
 * is preferred over standard library implementations because they generally
 * make no guarantee about being fast for malicious inputs.
 *
 * See the qsort_r manpage for a description of the interface.
 */
static void secp256k1_hsort(void *ptr, size_t count, size_t size,
                            int (*cmp)(const void *, const void *, void *),
                            void *cmp_data);
#endif
