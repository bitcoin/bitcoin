#ifndef SIMPLICITY_HASHBLOCK_H
#define SIMPLICITY_HASHBLOCK_H

#include <stddef.h>
#include <stdint.h>
#include "bounded.h"

/* A length-prefixed encoding of the following Simplicity program:
 *     hashBlock
 */
extern const unsigned char hashBlock[];
extern const size_t sizeof_hashBlock;
extern const unsigned char hashBlock_witness[];
extern const size_t sizeof_hashBlock_witness;

/* The commitment Merkle root of the above hashBlock Simplicity expression. */
extern const uint32_t hashBlock_cmr[];

/* The identity hash of the root of the above hashBlock Simplicity expression. */
extern const uint32_t hashBlock_ihr[];

/* The annotated Merkle root of the above hashBlock Simplicity expression. */
extern const uint32_t hashBlock_amr[];

/* The cost of the above hashBlock Simplicity expression in milli weight units. */
extern const ubounded hashBlock_cost;

#endif
