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
 * Remember that heapsort is unstable.
 *
 * In/Out: ptr: pointer to the array to sort. The contents of the array are
 *              sorted in ascending order according to the comparison function.
 * In:   count: number of elements in the array.
 *        size: size in bytes of each element.
 *         cmp: pointer to a comparison function that is called with two
 *              arguments that point to the objects being compared. The cmp_data
 *              argument of secp256k1_hsort is passed as third argument. The
 *              function must return an integer less than, equal to, or greater
 *              than zero if the first argument is considered to be respectively
 *              less than, equal to, or greater than the second.
 *    cmp_data: pointer passed as third argument to cmp.
 */
static void secp256k1_hsort(void *ptr, size_t count, size_t size,
                            int (*cmp)(const void *, const void *, void *),
                            void *cmp_data);
#endif
