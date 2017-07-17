#ifndef _SECP256K1_SURJECTIONPROOF_
#define _SECP256K1_SURJECTIONPROOF_

#include "secp256k1.h"
#include "secp256k1_rangeproof.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum number of inputs that may be given in a surjection proof */
#define SECP256K1_SURJECTIONPROOF_MAX_N_INPUTS 256

/** Number of bytes a serialized surjection proof requires given the
 *  number of inputs and the number of used inputs.
 */
#define SECP256K1_SURJECTIONPROOF_SERIALIZATION_BYTES(n_inputs, n_used_inputs) \
    (2 + (n_inputs + 7)/8 + 32 * (1 + (n_used_inputs)))

/** Maximum number of bytes a serialized surjection proof requires. */
#define SECP256K1_SURJECTIONPROOF_SERIALIZATION_BYTES_MAX \
    SECP256K1_SURJECTIONPROOF_SERIALIZATION_BYTES(SECP256K1_SURJECTIONPROOF_MAX_N_INPUTS, SECP256K1_SURJECTIONPROOF_MAX_N_INPUTS)

/** Opaque data structure that holds a parsed surjection proof
 *
 *  The exact representation of data inside is implementation defined and not
 *  guaranteed to be portable between different platforms or versions. Nor is
 *  it guaranteed to have any particular size, nor that identical proofs
 *  will have identical representation. (That is, memcmp may return nonzero
 *  even for identical proofs.)
 *
 *  To obtain these properties, instead use secp256k1_surjectionproof_parse
 *  and secp256k1_surjectionproof_serialize to encode/decode proofs into a
 *  well-defined format.
 *
 *  The representation is exposed to allow creation of these objects on the
 *  stack; please *do not* use these internals directly.
 */
typedef struct {
#ifdef VERIFY
    /** Mark whether this proof has gone through `secp256k1_surjectionproof_initialize` */
    int initialized;
#endif
    /** Total number of input asset tags */
    size_t n_inputs;
    /** Bitmap of which input tags are used in the surjection proof */
    unsigned char used_inputs[SECP256K1_SURJECTIONPROOF_MAX_N_INPUTS / 8];
    /** Borromean signature: e0, scalars */
    unsigned char data[32 * (1 + SECP256K1_SURJECTIONPROOF_MAX_N_INPUTS)];
} secp256k1_surjectionproof;

/** Parse a surjection proof
 *
 *  Returns: 1 when the proof could be parsed, 0 otherwise.
 *  Args: ctx:    a secp256k1 context object
 *  Out:  proof:  a pointer to a proof object
 *  In:   input:  a pointer to the array to parse
 *        inputlen: length of the array pointed to by input
 *
 *  The proof must consist of:
 *    - A 2-byte little-endian total input count `n`
 *    - A ceil(n/8)-byte bitmap indicating which inputs are used.
 *    - A big-endian 32-byte borromean signature e0 value
 *    - `m` big-endian 32-byte borromean signature s values, where `m`
 *      is the number of set bits in the bitmap
 */
SECP256K1_API int secp256k1_surjectionproof_parse(
  const secp256k1_context* ctx,
  secp256k1_surjectionproof *proof,
  const unsigned char *input,
  size_t inputlen
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Serialize a surjection proof
 *
 *  Returns: 1 if enough space was available to serialize, 0 otherwise
 *  Args:   ctx:        a secp256k1 context object
 *  Out:    output:     a pointer to an array to store the serialization
 *  In/Out: outputlen:  a pointer to an integer which is initially set to the
 *                      size of output, and is overwritten with the written
 *                      size.
 *  In:     proof:      a pointer to an initialized proof object
 *
 *  See secp256k1_surjectionproof_parse for details about the encoding.
 */
SECP256K1_API int secp256k1_surjectionproof_serialize(
  const secp256k1_context* ctx,
  unsigned char *output,
  size_t *outputlen,
  const secp256k1_surjectionproof *proof
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

/** Data structure that holds a fixed asset tag.
 *
 * This data type is *not* opaque. It will always be 32 bytes of whatever
 * data the API user wants to use as an asset tag. Its contents have no
 * semantic meaning to libsecp whatsoever.
 */
typedef struct {
    unsigned char data[32];
} secp256k1_fixed_asset_tag;

/** Returns the total number of inputs a proof expects to be over.
 *
 * Returns: the number of inputs for the given proof
 * In:   ctx: pointer to a context object
 *     proof: a pointer to a proof object
 */
SECP256K1_API size_t secp256k1_surjectionproof_n_total_inputs(
  const secp256k1_context* ctx,
  const secp256k1_surjectionproof* proof
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2);

/** Returns the actual number of inputs that a proof uses
 *
 * Returns: the number of inputs for the given proof
 * In:   ctx: pointer to a context object
 *     proof: a pointer to a proof object
 */
SECP256K1_API size_t secp256k1_surjectionproof_n_used_inputs(
  const secp256k1_context* ctx,
  const secp256k1_surjectionproof* proof
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2);

/** Returns the total size this proof would take, in bytes, when serialized
 *
 * Returns: the total size
 * In:   ctx: pointer to a context object
 *     proof: a pointer to a proof object
 */
SECP256K1_API size_t secp256k1_surjectionproof_serialized_size(
  const secp256k1_context* ctx,
  const secp256k1_surjectionproof* proof
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2);

/** Surjection proof initialization function; decides on inputs to use
 * Returns 0: inputs could not be selected
 *         n: inputs were selected after n iterations of random selection
 *
 * In:               ctx: pointer to a context object
 *      fixed_input_tags: fixed input tags `A_i` for all inputs. (If the fixed tag is not known,
 *                        e.g. in a coinjoin with others' inputs, an ephemeral tag can be given;
 *                        this won't match the output tag but might be used in the anonymity set.)
 *          n_input_tags: the number of entries in the fixed_input_tags array
 *      n_input_tags_to_use: the number of inputs to select randomly to put in the anonymity set
 *      fixed_output_tag: fixed output tag
 *      max_n_iterations: the maximum number of iterations to do before giving up
 *         random_seed32: a random seed to be used for input selection
 * Out:            proof: The proof whose bitvector will be initialized. In case of failure,
 *                        the state of the proof is undefined.
 *          input_index: The index of the actual input that is secretly mapped to the output
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_surjectionproof_initialize(
  const secp256k1_context* ctx,
  secp256k1_surjectionproof* proof,
  size_t *input_index,
  const secp256k1_fixed_asset_tag* fixed_input_tags,
  const size_t n_input_tags,
  const size_t n_input_tags_to_use,
  const secp256k1_fixed_asset_tag* fixed_output_tag,
  const size_t n_max_iterations,
  const unsigned char *random_seed32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4) SECP256K1_ARG_NONNULL(7);

/** Surjection proof generation function
 * Returns 0: proof could not be created
 *         1: proof was successfully created
 *
 * In:                   ctx: pointer to a context object, initialized for signing and verification
 *      ephemeral_input_tags: the ephemeral asset tag of all inputs
 *    n_ephemeral_input_tags: the number of entries in the ephemeral_input_tags array
 *      ephemeral_output_tag: the ephemeral asset tag of the output
 *               input_index: the index of the input that actually maps to the output
 *        input_blinding_key: the blinding key of the input
 *       output_blinding_key: the blinding key of the output
 * In/Out: proof: The produced surjection proof. Must have already gone through `secp256k1_surjectionproof_initialize`
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_surjectionproof_generate(
  const secp256k1_context* ctx,
  secp256k1_surjectionproof* proof,
  const secp256k1_generator* ephemeral_input_tags,
  size_t n_ephemeral_input_tags,
  const secp256k1_generator* ephemeral_output_tag,
  size_t input_index,
  const unsigned char *input_blinding_key,
  const unsigned char *output_blinding_key
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(5) SECP256K1_ARG_NONNULL(7) SECP256K1_ARG_NONNULL(8);


/** Surjection proof verification function
 * Returns 0: proof was invalid
 *         1: proof was valid
 *
 * In:     ctx: pointer to a context object, initialized for signing and verification
 *         proof: proof to be verified
 *      ephemeral_input_tags: the ephemeral asset tag of all inputs
 *    n_ephemeral_input_tags: the number of entries in the ephemeral_input_tags array
 *      ephemeral_output_tag: the ephemeral asset tag of the output
 */
SECP256K1_API int secp256k1_surjectionproof_verify(
  const secp256k1_context* ctx,
  const secp256k1_surjectionproof* proof,
  const secp256k1_generator* ephemeral_input_tags,
  size_t n_ephemeral_input_tags,
  const secp256k1_generator* ephemeral_output_tag
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(5);

#ifdef __cplusplus
}
#endif

#endif
