#ifndef SIMPLICITY_TYPESKIPTEST_H
#define SIMPLICITY_TYPESKIPTEST_H

#include <stddef.h>
#include <stdint.h>
#include "bounded.h"

/* A length-prefixed encoding of the following Simplicity program:
 *     witness (runIdentity (getValue (return True))) >>> mn >>> unit
 *      where
 *       l1 = take l0 &&& drop l0
 *       l2 = take l1 &&& drop l1
 *       l3 = take l2 &&& drop l2
 *       ltop = l3
 *       m1 = copair l3 l3
 *       m2 = take l1 &&& drop m1
 *       m3 = take m2 &&& drop l2
 *       m4 = take l3 &&& drop m3
 *       m5 = copair (injl m4) (injr ltop)
 *       m6 = take l1 &&& drop m5
 *       m7 = take m6 &&& drop l2
 *       m8 = take l3 &&& drop m7
 *       n1 = copair l3 l3
 *       n2 = take n1 &&& drop l1
 *       n3 = take l2 &&& drop n2
 *       n4 = take n3 &&& drop l3
 *       n5 = copair (injl ltop) (injr n4)
 *       n6 = take n5 &&& drop l0
 *       n7 = take l1 &&& drop n6
 *       n8 = take n7 &&& drop l2
 *       mn = copair (injl m8) (injr n8)
 */
extern const unsigned char typeSkipTest[];
extern const size_t sizeof_typeSkipTest;
extern const unsigned char typeSkipTest_witness[];
extern const size_t sizeof_typeSkipTest_witness;

/* The commitment Merkle root of the above typeSkipTest Simplicity expression. */
extern const uint32_t typeSkipTest_cmr[];

/* The identity hash of the root of the above typeSkipTest Simplicity expression. */
extern const uint32_t typeSkipTest_ihr[];

/* The annotated Merkle root of the above typeSkipTest Simplicity expression. */
extern const uint32_t typeSkipTest_amr[];

/* The cost of the above typeSkipTest Simplicity expression in milli weight units. */
extern const ubounded typeSkipTest_cost;

#endif
