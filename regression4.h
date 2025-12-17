#ifndef SIMPLICITY_REGRESSION4_H
#define SIMPLICITY_REGRESSION4_H

#include <stddef.h>
#include <stdint.h>
#include "bounded.h"

/* A length-prefixed encoding of the following Simplicity program:
 *     uWitness OneV : f 15 ++ [uComp (3*2^16) 1]
 *      where
 *       f 0 = [uIden, uTake 1, uIden, uDrop 1, uComp 3 1]
 *       f n = rec ++ rec ++ [uComp (3*2^n) 1]
 *        where
 *         rec = f (n-1)
 */
extern const unsigned char regression4[];
extern const size_t sizeof_regression4;

#endif
