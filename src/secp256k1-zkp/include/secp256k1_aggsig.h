#ifndef _SECP256K1_AGGSIG_
# define _SECP256K1_AGGSIG_

# include "secp256k1.h"

# ifdef __cplusplus
extern "C" {
# endif

/** Opaque data structure that holds context for the aggregated signature state machine
 *
 *  During execution of an aggregated signature this context object will contain secret
 *  data. It MUST be destroyed by `secp256k1_aggsig_context_destroy` to erase this data
 *  before freeing it. Context objects are sized based on the number of signatures to
 *  aggregate, and can be reused for multiple signature runs, provided that each run
 *  aggregates the same number of signatures.
 *
 *  Destroying and recreating a context object is essentially just deallocating and
 *  reallocating memory, there is no expensive precomputation as there is with the general
 *  libsecp256k1 context.
 *
 *  Once a context object is created with `secp256k1_aggsig_context_create` the workflow
 *  is as follows.
 *
 *      1. For each index controlled by the user, use `secp256k1_aggsig_generate_nonce`
 *         to generate a public/private nonce pair for that index. [TODO export the
 *         public nonce for other users]
 *      2. [TODO import others' public nonces]
 *      3. For each index controlled by the user, use `secp256k1_aggsig_partial_sign`
 *         to generate a partial signature that should be distributed to all peers.
 */
typedef struct secp256k1_aggsig_context_struct secp256k1_aggsig_context;

/** Opaque data structure that holds a partial signature
 *
 *  The exact representation of data inside is implementation defined and not
 *  guaranteed to be portable between different platforms or versions. It is
 *  however guaranteed to be 32 bytes in size, and can be safely copied, moved.
 *  and transmitted as raw bytes.
 */
typedef struct {
    unsigned char data[32];
} secp256k1_aggsig_partial_signature;


/** Create an aggregated signature context object with a given size
 *
 *  Returns: a newly created context object.
 *  Args: ctx:  an existing context object (cannot be NULL)
 *  In:     pubkeys: public keys for each signature (cannot be NULL)
 *        n_pubkeys: number of public keys/signatures to aggregate
 *             seed: a 32-byte seed to use for the nonce-generating RNG (cannot be NULL)
 */
SECP256K1_API secp256k1_aggsig_context* secp256k1_aggsig_context_create(
    const secp256k1_context *ctx,
    const secp256k1_pubkey *pubkeys,
    size_t n_pubkeys,
    const unsigned char *seed
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(4) SECP256K1_WARN_UNUSED_RESULT;


/** Destroy an aggregated signature context object. If passed NULL, is a no-op.
 *
 *  Args: aggctx:  an existing context object
 */
SECP256K1_API void secp256k1_aggsig_context_destroy(
    secp256k1_aggsig_context *aggctx
);

/** Generate a nonce pair for a single signature part in an aggregated signature
 *
 *  Returns: 1 on success
 *           0 if a nonce has already been generated for this index
 *  Args:    ctx: an existing context object, initialized for signing (cannot be NULL)
 *        aggctx: an aggsig context object (cannot be NULL)
 *  In:    index: which signature to generate a nonce for
 */
SECP256K1_API int secp256k1_aggsig_generate_nonce(
    const secp256k1_context* ctx,
    secp256k1_aggsig_context* aggctx,
    size_t index
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_WARN_UNUSED_RESULT;

/** Generates and exports a secure nonce, of which the public part can be shared
 *  and fed back for a later signature
 *
 *  Returns: 1 on success
 *  Args:    ctx: an existing context object, initialized for signing (cannot be NULL)
 *  In:    seed: A random seed value
 *  Out:   secnonce32: The secure nonce (scalar), guaranteed to be Jacobi 1
 */
SECP256K1_API int secp256k1_aggsig_export_secnonce_single(
    const secp256k1_context* ctx,
    unsigned char* secnonce32,
    const unsigned char* seed
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_WARN_UNUSED_RESULT;

/** Generate a single-signer signature (or partial sig), without a stored context 
 *
 *  Returns: 1 on success, 0 on failure
 *  Args:    ctx: an existing context object, initialized for signing (cannot be NULL)
 *  Out:     sig64: the completed signature (cannot be NULL)
 *  In:      msg32: the message to sign (cannot be NULL)
 *           seckey32: the secret signing key (cannot be NULL)
 *           secnonce32: secret nonce to use. If NULL, a nonce will be generated
 *           extra32: if non-NULL, add this key to s
 *           pubnonce_for_e: If this is non-NULL, encode this value in e instead of the derived
 *           pubnonce_total: If non-NULL, allow this signature to be included in combined sig
 *               in all cases by negating secnonce32 if the public nonce total has jacobi symbol 
 *               -1. secnonce32 must also be provided
 *           pubkey_for_e: If this is non-NULL, encode this value in e
 *           seed: a 32-byte seed to use for the nonce-generating RNG (cannot be NULL)
 */
SECP256K1_API int secp256k1_aggsig_sign_single(
    const secp256k1_context* ctx,
    unsigned char *sig64,
    const unsigned char *msg32,
    const unsigned char *seckey32,
    const unsigned char* secnonce32,
    const unsigned char* extra32,
    const secp256k1_pubkey *pubnonce_for_e,
    const secp256k1_pubkey* pubnonce_total,
    const secp256k1_pubkey* pubkey_for_e,
    const unsigned char* seed)
SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4) SECP256K1_ARG_NONNULL(10) SECP256K1_WARN_UNUSED_RESULT;

/** Generate a single signature part in an aggregated signature
 *
 *  Returns: 1 on success, 0 on failure
 *  Args:    ctx: an existing context object, initialized for signing (cannot be NULL)
 *        aggctx: an aggsig context object (cannot be NULL)
 *  Out:   partial: the generated signature part (cannot be NULL)
 *  In:  msg32: the message to sign (cannot be NULL)
 *        seckey32: the secret signing key (cannot be NULL)
 *           index: the index of this signature in the aggregate signature
 */
SECP256K1_API int secp256k1_aggsig_partial_sign(
    const secp256k1_context* ctx,
    secp256k1_aggsig_context* aggctx,
    secp256k1_aggsig_partial_signature *partial,
    const unsigned char *msg32,
    const unsigned char *seckey32,
    size_t index
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4) SECP256K1_ARG_NONNULL(5) SECP256K1_WARN_UNUSED_RESULT;


/** Aggregate multiple signature parts into a single aggregated signature
 *
 *  Returns: 1 on success, 0 on failure
 *  Args:    ctx: an existing context object, initialized for signing (cannot be NULL)
 *        aggctx: an aggsig context object (cannot be NULL)
 *  Out:     sig64: the completed signature (cannot be NULL)
 *  In:    partial: an array of partial signatures to aggregate (cannot be NULL)
 *          n_sigs: the number of partial signatures provided
 */
SECP256K1_API int secp256k1_aggsig_combine_signatures(
    const secp256k1_context* ctx,
    secp256k1_aggsig_context* aggctx,
    unsigned char *sig64,
    const secp256k1_aggsig_partial_signature *partial,
    size_t n_sigs
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4) SECP256K1_WARN_UNUSED_RESULT;

/** Simple addition of two signatures + two public nonces into a single signature
 *
 *  Returns: 1 on success, 0 on failure
 *  Args:    ctx: an existing context object, initialized for signing (cannot be NULL)
 *  Out:     sig64: the completed signature (s1+s2,n1+n2) (cannot be NULL)
 *  In:     sig1_64: a signature (from which s1 will2be taken)
 *          sig2_64: another signature (from which s1 will be taken)
 *          pubnonce_total: the total of all public nonces, will simple become R (negated if needed)
 */

SECP256K1_API int secp256k1_aggsig_add_signatures_single(
    const secp256k1_context* ctx,
    unsigned char *sig64,
    const unsigned char** sigs,
    size_t num_sigs,
    const secp256k1_pubkey* pubnonce_total
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(5) SECP256K1_WARN_UNUSED_RESULT;


/** Verify a single-signer signature, without a stored context 
 *
 *  Returns: 1 on success, 0 on failure
 *  Args:    ctx: an existing context object, initialized for signing (cannot be NULL)
 *  In:      sig64: signature (cannot be NULL)
 *           msg32: the message to verify (cannot be NULL)
 *           pubnonce: if non-NULL, override the public nonce used to calculate e
 *           pubkey: the public key (cannot be NULL)
 *           pubkey_total: if non-NULL, encode this value in e
 *           extra_pubkey: if non-NULL, subtract this pubkey from sG
 *           is_partial: whether to ignore the jacobi symbol of the combined R, set this to 1
 *               to verify partial signatures that may have had their secret nonces negated
 */
SECP256K1_API int secp256k1_aggsig_verify_single(
    const secp256k1_context* ctx,
    const unsigned char *sig64,
    const unsigned char *msg32,
    const secp256k1_pubkey *pubnonce,
    const secp256k1_pubkey *pubkey,
    const secp256k1_pubkey *pubkey_total,
    const secp256k1_pubkey *extra_pubkey,
    const int is_partial)
SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(5) SECP256K1_WARN_UNUSED_RESULT;

/** Verify an aggregate signature
 *
 *  Returns: 1 if the signature is valid, 0 if not
 *  Args:    ctx: an existing context object (cannot be NULL)
 *       scratch: a scratch space (cannot be NULL)
 *  In:    sig64: the signature to verify (cannot be NULL)
 *         msg32: the message that should be signed (cannot be NULL)
 *       pubkeys: array of public keys (cannot be NULL)
 *        n_keys: the number of public keys
 */
SECP256K1_API int secp256k1_aggsig_verify(
    const secp256k1_context* ctx,
    secp256k1_scratch_space* scratch,
    const unsigned char *sig64,
    const unsigned char *msg32,
    const secp256k1_pubkey *pubkeys,
    size_t n_pubkeys
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4) SECP256K1_ARG_NONNULL(5) SECP256K1_WARN_UNUSED_RESULT;

/** Verify an aggregate signature, building scratch space interally beforehand
 *
 *  Returns: 1 if the signature is valid, 0 if not
 *  Args:    ctx: an existing context object (cannot be NULL)
 *  In:    sig64: the signature to verify (cannot be NULL)
 *         msg32: the message that should be signed (cannot be NULL)
 *       pubkeys: array of public keys (cannot be NULL)
 *        n_keys: the number of public keys
 */

SECP256K1_API int secp256k1_aggsig_build_scratch_and_verify(
    const secp256k1_context* ctx,
    const unsigned char *sig64,
    const unsigned char *msg32,
    const secp256k1_pubkey *pubkeys,
    size_t n_pubkeys
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4) SECP256K1_WARN_UNUSED_RESULT;

# ifdef __cplusplus
}
# endif

#endif
