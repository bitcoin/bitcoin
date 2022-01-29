#ifndef _SECP256K1_COMMITMENT_
# define _SECP256K1_COMMITMENT_

# include "secp256k1.h"
# include "secp256k1_generator.h"

# ifdef __cplusplus
extern "C" {
# endif

#include <stdint.h>

/** Opaque data structure that stores a Pedersen commitment
 *
 *  The exact representation of data inside is implementation defined and not
 *  guaranteed to be portable between different platforms or versions. It is
 *  however guaranteed to be 64 bytes in size, and can be safely copied/moved.
 *  If you need to convert to a format suitable for storage, transmission, or
 *  comparison, use secp256k1_pedersen_commitment_serialize and
 *  secp256k1_pedersen_commitment_parse.
 */
typedef struct {
    unsigned char data[64];
} secp256k1_pedersen_commitment;

/**
 * Static constant generator 'h' maintained for historical reasons.
 */
SECP256K1_API extern const secp256k1_generator *secp256k1_generator_h;

/** Parse a 33-byte commitment into a commitment object.
 *
 *  Returns: 1 if input contains a valid commitment.
 *  Args: ctx:      a secp256k1 context object.
 *  Out:  commit:   pointer to the output commitment object
 *  In:   input:    pointer to a 33-byte serialized commitment key
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_pedersen_commitment_parse(
    const secp256k1_context* ctx,
    secp256k1_pedersen_commitment* commit,
    const unsigned char *input
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Serialize a commitment object into a serialized byte sequence.
 *
 *  Returns: 1 always.
 *  Args:   ctx:        a secp256k1 context object.
 *  Out:    output:     a pointer to a 33-byte byte array
 *  In:     commit:     a pointer to a secp256k1_pedersen_commitment containing an
 *                      initialized commitment
 */
SECP256K1_API int secp256k1_pedersen_commitment_serialize(
    const secp256k1_context* ctx,
    unsigned char *output,
    const secp256k1_pedersen_commitment* commit
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Initialize a context for usage with Pedersen commitments. */
void secp256k1_pedersen_context_initialize(secp256k1_context* ctx);

/** Generate a Pedersen commitment.
 *  Returns 1: Commitment successfully created.
 *          0: Error. The blinding factor is larger than the group order
 *             (probability for random 32 byte number < 2^-127) or results in the
 *             point at infinity. Retry with a different factor.
 *  In:     ctx:        pointer to a context object (cannot be NULL)
 *          blind:      pointer to a 32-byte blinding factor (cannot be NULL)
 *          value:      unsigned 64-bit integer value to commit to.
 *          value_gen:  value generator 'h'
 *          blind_gen:  blinding factor generator 'g'
 *  Out:    commit:     pointer to the commitment (cannot be NULL)
 *
 *  Blinding factors can be generated and verified in the same way as secp256k1 private keys for ECDSA.
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_pedersen_commit(
  const secp256k1_context* ctx,
  secp256k1_pedersen_commitment *commit,
  const unsigned char *blind,
  uint64_t value,
  const secp256k1_generator *value_gen,
  const secp256k1_generator *blind_gen
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(5) SECP256K1_ARG_NONNULL(6);

/** Generate a Pedersen commitment from two blinding factors.
 *  Returns 1: Commitment successfully created.
 *          0: Error. The blinding factor is larger than the group order
 *             (probability for random 32 byte number < 2^-127) or results in the
 *             point at infinity. Retry with a different factor.
 *  In:     ctx:        pointer to a context object (cannot be NULL)
 *          blind:      pointer to a 32-byte blinding factor (cannot be NULL)
 *          value:      pointer to a 32-byte blinding factor (cannot be NULL)
 *          value_gen:  value generator 'h'
 *          blind_gen:  blinding factor generator 'g'
 *  Out:    commit:     pointer to the commitment (cannot be NULL)
 *
 *  Blinding factors can be generated and verified in the same way as secp256k1 private keys for ECDSA.
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_pedersen_blind_commit(
  const secp256k1_context* ctx,
  secp256k1_pedersen_commitment *commit,
  const unsigned char *blind,
  const unsigned char *value,
  const secp256k1_generator *value_gen,
  const secp256k1_generator *blind_gen
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4) SECP256K1_ARG_NONNULL(5) SECP256K1_ARG_NONNULL(6);

/** Computes the sum of multiple positive and negative blinding factors.
 *  Returns 1: Sum successfully computed.
 *          0: Error. A blinding factor is larger than the group order
 *             (probability for random 32 byte number < 2^-127). Retry with
 *             different factors.
 *  In:     ctx:        pointer to a context object (cannot be NULL)
 *          blinds:     pointer to pointers to 32-byte character arrays for blinding factors. (cannot be NULL)
 *          n:          number of factors pointed to by blinds.
 *          npositive:  how many of the input factors should be treated with a positive sign.
 *  Out:    blind_out:  pointer to a 32-byte array for the sum (cannot be NULL)
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_pedersen_blind_sum(
  const secp256k1_context* ctx,
  unsigned char *blind_out,
  const unsigned char * const *blinds,
  size_t n,
  size_t npositive
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Computes the sum of multiple positive and negative pedersen commitments
 * Returns 1: sum successfully computed.
 * In:     ctx:        pointer to a context object, initialized for Pedersen commitment (cannot be NULL)
 *         commits:    pointer to array of pointers to the commitments. (cannot be NULL if pcnt is non-zero)
 *         pcnt:       number of commitments pointed to by commits.
 *         ncommits:   pointer to array of pointers to the negative commitments. (cannot be NULL if ncnt is non-zero)
 *         ncnt:       number of commitments pointed to by ncommits.
 *  Out:   commit_out: pointer to the commitment (cannot be NULL)
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_pedersen_commit_sum(
  const secp256k1_context* ctx,
  secp256k1_pedersen_commitment *commit_out,
  const secp256k1_pedersen_commitment * const* commits,
  size_t pcnt,
  const secp256k1_pedersen_commitment * const* ncommits,
  size_t ncnt
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2);

/** Verify a tally of Pedersen commitments
 * Returns 1: commitments successfully sum to zero.
 *         0: Commitments do not sum to zero or other error.
 * In:     ctx:    pointer to a context object (cannot be NULL)
 *         pos:    pointer to array of pointers to the commitments. (cannot be NULL if `n_pos` is non-zero)
 *         n_pos:  number of commitments pointed to by `pos`.
 *         neg:    pointer to array of pointers to the negative commitments. (cannot be NULL if `n_neg` is non-zero)
 *         n_neg:  number of commitments pointed to by `neg`.
 *
 * This computes sum(pos[0..n_pos)) - sum(neg[0..n_neg)) == 0.
 *
 * A Pedersen commitment is xG + vA where G and A are generators for the secp256k1 group and x is a blinding factor,
 * while v is the committed value. For a collection of commitments to sum to zero, for each distinct generator
 * A all blinding factors and all values must sum to zero.
 *
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_pedersen_verify_tally(
  const secp256k1_context* ctx,
  const secp256k1_pedersen_commitment * const* pos,
  size_t n_pos,
  const secp256k1_pedersen_commitment * const* neg,
  size_t n_neg
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(4);

/** Sets the final Pedersen blinding factor correctly when the generators themselves
 *  have blinding factors.
 *
 * Consider a generator of the form A' = A + rG, where A is the "real" generator
 * but A' is the generator provided to verifiers. Then a Pedersen commitment
 * P = vA' + r'G really has the form vA + (vr + r')G. To get all these (vr + r')
 * to sum to zero for multiple commitments, we take three arrays consisting of
 * the `v`s, `r`s, and `r'`s, respectively called `value`s, `generator_blind`s
 * and `blinding_factor`s, and sum them.
 *
 * The function then subtracts the sum of all (vr + r') from the last element
 * of the `blinding_factor` array, setting the total sum to zero.
 *
 * Returns 1: Blinding factor successfully computed.
 *         0: Error. A blinding_factor or generator_blind are larger than the group
 *            order (probability for random 32 byte number < 2^-127). Retry with
 *            different values.
 *
 * In:                 ctx: pointer to a context object
 *                   value: array of asset values, `v` in the above paragraph.
 *                          May not be NULL unless `n_total` is 0.
 *         generator_blind: array of asset blinding factors, `r` in the above paragraph
 *                          May not be NULL unless `n_total` is 0.
 *                 n_total: Total size of the above arrays
 *                n_inputs: How many of the initial array elements represent commitments that
 *                          will be negated in the final sum
 * In/Out: blinding_factor: array of commitment blinding factors, `r'` in the above paragraph
 *                          May not be NULL unless `n_total` is 0.
 *                          the last value will be modified to get the total sum to zero.
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_pedersen_blind_generator_blind_sum(
  const secp256k1_context* ctx,
  const uint64_t *value,
  const unsigned char* const* generator_blind,
  unsigned char* const* blinding_factor,
  size_t n_total,
  size_t n_inputs
);

/** Calculates the blinding factor x' = x + SHA256(xG+vH | xJ), used in the switch commitment x'G+vH
 *
 * Returns 1: Blinding factor successfully computed.
 *         0: Error. Retry with different values.
 *
 * Args:           ctx: pointer to a context object
 * Out:   blind_switch: blinding factor for the switch commitment
 * In:           blind: pointer to a 32-byte blinding factor
 *               value: unsigned 64-bit integer value to commit to
 *           value_gen: value generator 'h'
 *           blind_gen: blinding factor generator 'g'
 *       switch_pubkey: pointer to public key 'j'
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_blind_switch(
  const secp256k1_context* ctx, 
  unsigned char* blind_switch, 
  const unsigned char* blind, 
  uint64_t value, 
  const secp256k1_generator* value_gen, 
  const secp256k1_generator* blind_gen, 
  const secp256k1_pubkey* switch_pubkey
);

/** Converts a pedersent commit to a pubkey
 *
 * Returns 1: Public key succesfully computed.
 *         0: Error.
*
 * In:                 ctx: pointer to a context object
 *                   commit: pointer to a single commit
 * Out:              pubkey: resulting pubkey
 *
 */

SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_pedersen_commitment_to_pubkey(
  const secp256k1_context* ctx,
  secp256k1_pubkey* pubkey,
  const secp256k1_pedersen_commitment* commit
);

# ifdef __cplusplus
}
# endif

#endif
