#ifndef SIMPLICITY_CTX8PRUNED_H
#define SIMPLICITY_CTX8PRUNED_H

#include <stddef.h>
#include <stdint.h>
#include "bounded.h"

/* A length-prefixed encoding of the following Simplicity program:
 *     (scribe (toWord256 0x067C531269735CA7F541FDACA8F0DC76305D3CADA140F89372A410FE5EFF6E4D) &&&
 *      (ctx8Init &&& scribe (toWord128 0xDE188941A3375D3A8A061E67576E926D)) >>> ctx8Addn vector16 >>> ctx8Finalize) >>>
 *     eq >>> verify
 */
extern const unsigned char ctx8Pruned[];
extern const size_t sizeof_ctx8Pruned;
extern const unsigned char ctx8Pruned_witness[];
extern const size_t sizeof_ctx8Pruned_witness;

/* The commitment Merkle root of the above ctx8Pruned Simplicity expression. */
extern const uint32_t ctx8Pruned_cmr[];

/* The identity hash of the root of the above ctx8Pruned Simplicity expression. */
extern const uint32_t ctx8Pruned_ihr[];

/* The annotated Merkle root of the above ctx8Pruned Simplicity expression. */
extern const uint32_t ctx8Pruned_amr[];

/* The cost of the above ctx8Pruned Simplicity expression in milli weight units. */
extern const ubounded ctx8Pruned_cost;

#endif
