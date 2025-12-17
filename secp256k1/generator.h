#ifndef SECP256K1_GENERATOR_H
# define SECP256K1_GENERATOR_H

# include "secp256k1.h"

#include <stdint.h>

/** Opaque data structure that stores a base point
 *
 *  The exact representation of data inside is implementation defined and not
 *  guaranteed to be portable between different platforms or versions. It is
 *  however guaranteed to be 64 bytes in size, and can be safely copied/moved.
 *  If you need to convert to a format suitable for storage, transmission, or
 *  comparison, use secp256k1_generator_serialize and secp256k1_generator_parse.
 */
typedef struct {
    unsigned char data[64];
} secp256k1_generator;

/** Generate a generator for the curve.
 *
 *  Returns: 0 in the highly unlikely case the seed is not acceptable,
 *           1 otherwise.
 *  Out:  gen:     a generator object
 *  In:   seed32:  a 32-byte seed
 *
 *  If successful a valid generator will be placed in gen. The produced
 *  generators are distributed uniformly over the curve, and will not have a
 *  known discrete logarithm with respect to any other generator produced,
 *  or to the base generator G.
 */
static SECP256K1_WARN_UNUSED_RESULT int secp256k1_generator_generate(
    secp256k1_generator *gen,
    const unsigned char *seed32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

#endif
