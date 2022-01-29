#ifndef _SECP256K1_GENERATOR_
# define _SECP256K1_GENERATOR_

# include "secp256k1.h"

# ifdef __cplusplus
extern "C" {
# endif

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

/** Standard secp256k1 generator G */
SECP256K1_API extern const secp256k1_generator secp256k1_generator_const_g;

/** Alternate secp256k1 generator from Elements Alpha */
SECP256K1_API extern const secp256k1_generator secp256k1_generator_const_h;

/** Parse a 33-byte generator byte sequence into a generator object.
 *
 *  Returns: 1 if input contains a valid generator.
 *  Args: ctx:      a secp256k1 context object.
 *  Out:  commit:   pointer to the output generator object
 *  In:   input:    pointer to a 33-byte serialized generator
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_generator_parse(
    const secp256k1_context* ctx,
    secp256k1_generator* commit,
    const unsigned char *input
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Serialize a 33-byte generator into a serialized byte sequence.
 *
 *  Returns: 1 always.
 *  Args:   ctx:        a secp256k1 context object.
 *  Out:    output:     a pointer to a 33-byte byte array
 *  In:     commit:     a pointer to a generator
 */
SECP256K1_API int secp256k1_generator_serialize(
    const secp256k1_context* ctx,
    unsigned char *output,
    const secp256k1_generator* commit
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Generate a generator for the curve.
 *
 *  Returns: 0 in the highly unlikely case the seed is not acceptable,
 *           1 otherwise.
 *  Args: ctx:     a secp256k1 context object
 *  Out:  gen:     a generator object
 *  In:   seed32:  a 32-byte seed
 *
 *  If successful a valid generator will be placed in gen. The produced
 *  generators are distributed uniformly over the curve, and will not have a
 *  known discrete logarithm with respect to any other generator produced,
 *  or to the base generator G.
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_generator_generate(
    const secp256k1_context* ctx,
    secp256k1_generator* gen,
    const unsigned char *seed32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Generate a blinded generator for the curve.
 *
 *  Returns: 0 in the highly unlikely case the seed is not acceptable or when
 *           blind is out of range. 1 otherwise.
 *  Args: ctx:     a secp256k1 context object, initialized for signing
 *  Out:  gen:     a generator object
 *  In:   seed32:  a 32-byte seed
 *        blind32: a 32-byte secret value to blind the generator with.
 *
 *  The result is equivalent to first calling secp256k1_generator_generate,
 *  converting the result to a public key, calling secp256k1_ec_pubkey_tweak_add,
 *  and then converting back to generator form.
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_generator_generate_blinded(
    const secp256k1_context* ctx,
    secp256k1_generator* gen,
    const unsigned char *key32,
    const unsigned char *blind32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

# ifdef __cplusplus
}
# endif

#endif
