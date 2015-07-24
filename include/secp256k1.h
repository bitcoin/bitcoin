#ifndef _SECP256K1_
# define _SECP256K1_

# ifdef __cplusplus
extern "C" {
# endif

# if !defined(SECP256K1_GNUC_PREREQ)
#  if defined(__GNUC__)&&defined(__GNUC_MINOR__)
#   define SECP256K1_GNUC_PREREQ(_maj,_min) \
 ((__GNUC__<<16)+__GNUC_MINOR__>=((_maj)<<16)+(_min))
#  else
#   define SECP256K1_GNUC_PREREQ(_maj,_min) 0
#  endif
# endif

# if (!defined(__STDC_VERSION__) || (__STDC_VERSION__ < 199901L) )
#  if SECP256K1_GNUC_PREREQ(2,7)
#   define SECP256K1_INLINE __inline__
#  elif (defined(_MSC_VER))
#   define SECP256K1_INLINE __inline
#  else
#   define SECP256K1_INLINE
#  endif
# else
#  define SECP256K1_INLINE inline
# endif

/**Warning attributes
  * NONNULL is not used if SECP256K1_BUILD is set to avoid the compiler optimizing out
  * some paranoid null checks. */
# if defined(__GNUC__) && SECP256K1_GNUC_PREREQ(3, 4)
#  define SECP256K1_WARN_UNUSED_RESULT __attribute__ ((__warn_unused_result__))
# else
#  define SECP256K1_WARN_UNUSED_RESULT
# endif
# if !defined(SECP256K1_BUILD) && defined(__GNUC__) && SECP256K1_GNUC_PREREQ(3, 4)
#  define SECP256K1_ARG_NONNULL(_x)  __attribute__ ((__nonnull__(_x)))
# else
#  define SECP256K1_ARG_NONNULL(_x)
# endif

/** Opaque data structure that holds context information (precomputed tables etc.).
 *  Only functions that take a pointer to a non-const context require exclusive
 *  access to it. Multiple functions that take a pointer to a const context may
 *  run simultaneously.
 */
typedef struct secp256k1_context_struct secp256k1_context_t;

/** Flags to pass to secp256k1_context_create. */
# define SECP256K1_CONTEXT_VERIFY (1 << 0)
# define SECP256K1_CONTEXT_SIGN   (1 << 1)

/** Create a secp256k1 context object.
 *  Returns: a newly created context object.
 *  In:      flags: which parts of the context to initialize.
 */
secp256k1_context_t* secp256k1_context_create(
  int flags
) SECP256K1_WARN_UNUSED_RESULT;

/** Copies a secp256k1 context object.
 *  Returns: a newly created context object.
 *  In:      ctx: an existing context to copy
 */
secp256k1_context_t* secp256k1_context_clone(
  const secp256k1_context_t* ctx
) SECP256K1_WARN_UNUSED_RESULT;

/** Destroy a secp256k1 context object.
 *  The context pointer may not be used afterwards.
 */
void secp256k1_context_destroy(
  secp256k1_context_t* ctx
) SECP256K1_ARG_NONNULL(1);

/** Set a callback function to be called when an illegal argument is passed to
 *  an API call. The philosophy is that these shouldn't be dealt with through a
 *  specific return value, as calling code should not have branches to deal with
 *  the case that this code itself is broken.
 *  On the other hand, during debug stage, one would want to be informed about
 *  such mistakes, and the default (crashing) may be inadvisable.
 *  When this callback is triggered, the API function called is guaranteed not
 *  to cause a crash, though its return value and output arguments are
 *  undefined.
 */
void secp256k1_context_set_illegal_callback(
  secp256k1_context_t* ctx,
  void (*fun)(const char* message, void* data),
  void* data
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2);

/** Set a callback function to be called when an internal consistency check
 *  fails. The default is crashing.
 *  This can only trigger in case of a hardware failure, miscompilation,
 *  memory corruption, serious bug in the library, or other error would can
 *  otherwise result in undefined behaviour. It will not trigger due to mere
 *  incorrect usage of the API (see secp256k1_context_set_illegal_callback
 *  for that). After this callback returns, anything may happen, including
 *  crashing.
 */
void secp256k1_context_set_error_callback(
  secp256k1_context_t* ctx,
  void (*fun)(const char* message, void* data),
  void* data
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2);

/** Data type to hold a parsed and valid public key.
    This data type should be considered opaque to the user, and only created
    through API functions. It is not guaranteed to be compatible between
    different implementations. If you need to convert to a format suitable
    for storage or transmission, use secp256k1_ec_pubkey_serialize and
    secp256k1_ec_pubkey_parse.
 */
typedef struct {
  unsigned char data[64];
} secp256k1_pubkey_t;

/** Parse a variable-length public key into the pubkey object.
 *  Returns: 1 if the public key was fully valid.
 *           0 if the public key could not be parsed or is invalid.
 *  In:  ctx:      a secp256k1 context object.
 *       input:    pointer to a serialized public key
 *       inputlen: length of the array pointed to by input
 *  Out: pubkey:   pointer to a pubkey object. If 1 is returned, it is set to a
 *                 parsed version of input. If not, its value is undefined.
 *  This function supports parsing compressed (33 bytes, header byte 0x02 or
 *  0x03), uncompressed (65 bytes, header byte 0x04), or hybrid (65 bytes, header
 *  byte 0x06 or 0x07) format public keys.
 */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_pubkey_parse(
  const secp256k1_context_t* ctx,
  secp256k1_pubkey_t* pubkey,
  const unsigned char *input,
  int inputlen
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Serialize a pubkey object into a serialized byte sequence.
 *  Returns: 1 always.
 *  In:   ctx:        a secp256k1 context object.
 *        pubkey:     a pointer to a secp256k1_pubkey_t containing an initialized
 *                    public key.
 *        compressed: whether to serialize in compressed format.
 *  Out:  output:     a pointer to a 65-byte (if compressed==0) or 33-byte (if
 *                    compressed==1) byte array to place the serialized key in.
 *        outputlen:  a pointer to an integer which will contain the serialized
 *                    size.
 */
int secp256k1_ec_pubkey_serialize(
  const secp256k1_context_t* ctx,
  unsigned char *output,
  int *outputlen,
  const secp256k1_pubkey_t* pubkey,
  int compressed
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

/** Data type to hold a parsed ECDSA signature, optionally supporting pubkey
 *  recovery.
    This data type should be considered opaque to the user, and only created
    through API functions. It is not guaranteed to be compatible between
    different implementations. If you need to convert to a format suitable
    for storage or transmission, use secp256k1_ecdsa_signature_serialize_* and
    secp256k1_ecdsa_signature_parse_* functions. */
typedef struct {
  unsigned char data[65];
} secp256k1_ecdsa_signature_t;

/** Parse a DER ECDSA signature.
 *  Returns: 1 when the signature could be parsed, 0 otherwise.
 *  In:  ctx:      a secp256k1 context object
 *       input:    a pointer to the signature to be parsed
 *       inputlen: the length of the array pointed to be input
 *  Out: sig:      a pointer to a signature object
 *
 *  Note that this function also supports some violations of DER.
 *
 *  The resulting signature object will not support pubkey recovery.
 */
int secp256k1_ecdsa_signature_parse_der(
  const secp256k1_context_t* ctx,
  secp256k1_ecdsa_signature_t* sig,
  const unsigned char *input,
  int inputlen
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Parse a compact ECDSA signature (64 bytes + recovery id).
 *  Returns: 1 when the signature could be parsed, 0 otherwise
 *  In:  ctx:     a secp256k1 context object
 *       input64: a pointer to a 64-byte compact signature
 *       recid:   the recovery id (0, 1, 2 or 3, or -1 for unknown)
 *  Out: sig:     a pointer to a signature object
 *
 *  If recid is not -1, the resulting signature object will support pubkey
 *  recovery.
 */
int secp256k1_ecdsa_signature_parse_compact(
  const secp256k1_context_t* ctx,
  secp256k1_ecdsa_signature_t* sig,
  const unsigned char *input64,
  int recid
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Serialize an ECDSA signature in DER format.
 *  Returns: 1 if enough space was available to serialize, 0 otherwise
 *  In:     ctx:       a secp256k1 context object
 *          sig:       a pointer to an initialized signature object
 *  Out:    output:    a pointer to an array to store the DER serialization
 *  In/Out: outputlen: a pointer to a length integer. Initially, this integer
 *                     should be set to the length of output. After the call
 *                     it will be set to the length of the serialization (even
 *                     if 0 was returned).
 */
int secp256k1_ecdsa_signature_serialize_der(
  const secp256k1_context_t* ctx,
  unsigned char *output,
  int *outputlen,
  const secp256k1_ecdsa_signature_t* sig
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

/** Serialize an ECDSA signature in compact format (64 bytes + recovery id).
 *  Returns: 1
 *  In: ctx:       a secp256k1 context object
 *      sig:       a pointer to an initialized signature object (cannot be NULL)
 *  Out: output64: a pointer to a 64-byte array of the compact signature (cannot be NULL)
 *       recid:    a pointer to an integer to hold the recovery id (can be NULL).
 *
 *  If recid is not NULL, the signature must support pubkey recovery.
 */
int secp256k1_ecdsa_signature_serialize_compact(
  const secp256k1_context_t* ctx,
  unsigned char *output64,
  int *recid,
  const secp256k1_ecdsa_signature_t* sig
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(4);

/** Verify an ECDSA signature.
 *  Returns: 1: correct signature
 *           0: incorrect or unparseable signature
 * In:       ctx:       a secp256k1 context object, initialized for verification.
 *           msg32:     the 32-byte message hash being verified (cannot be NULL)
 *           sig:       the signature being verified (cannot be NULL)
 *           pubkey:    pointer to an initialized public key to verify with (cannot be NULL)
 */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_ecdsa_verify(
  const secp256k1_context_t* ctx,
  const unsigned char *msg32,
  const secp256k1_ecdsa_signature_t *sig,
  const secp256k1_pubkey_t *pubkey
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

/** A pointer to a function to deterministically generate a nonce.
 * Returns: 1 if a nonce was successfully generated. 0 will cause signing to fail.
 * In:      msg32:     the 32-byte message hash being verified (will not be NULL)
 *          key32:     pointer to a 32-byte secret key (will not be NULL)
 *          algo16:    pointer to a 16-byte array describing the signature
 *                     algorithm (will be NULL for ECDSA for compatibility).
 *          attempt:   how many iterations we have tried to find a nonce.
 *                     This will almost always be 0, but different attempt values
 *                     are required to result in a different nonce.
 *          data:      Arbitrary data pointer that is passed through.
 * Out:     nonce32:   pointer to a 32-byte array to be filled by the function.
 * Except for test cases, this function should compute some cryptographic hash of
 * the message, the key and the attempt.
 */
typedef int (*secp256k1_nonce_function_t)(
  unsigned char *nonce32,
  const unsigned char *msg32,
  const unsigned char *key32,
  const unsigned char *algo16,
  unsigned int attempt,
  const void *data
);

/** An implementation of RFC6979 (using HMAC-SHA256) as nonce generation function.
 * If a data pointer is passed, it is assumed to be a pointer to 32 bytes of
 * extra entropy.
 */
extern const secp256k1_nonce_function_t secp256k1_nonce_function_rfc6979;

/** A default safe nonce generation function (currently equal to secp256k1_nonce_function_rfc6979). */
extern const secp256k1_nonce_function_t secp256k1_nonce_function_default;

/** Create an ECDSA signature.
 *  Returns: 1: signature created
 *           0: the nonce generation function failed, or the private key was invalid.
 *  In:      ctx:    pointer to a context object, initialized for signing (cannot be NULL)
 *           msg32:  the 32-byte message hash being signed (cannot be NULL)
 *           seckey: pointer to a 32-byte secret key (cannot be NULL)
 *           noncefp:pointer to a nonce generation function. If NULL, secp256k1_nonce_function_default is used
 *           ndata:  pointer to arbitrary data used by the nonce generation function (can be NULL)
 *  Out:     sig:    pointer to an array where the signature will be placed (cannot be NULL)
 *
 * The resulting signature will support pubkey recovery.
 *
 * The sig always has an s value in the lower half of the range (From 0x1
 * to 0x7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF5D576E7357A4501DDFE92F46681B20A0,
 * inclusive), unlike many other implementations.
 * With ECDSA a third-party can can forge a second distinct signature
 * of the same message given a single initial signature without knowing
 * the key by setting s to its additive inverse mod-order, 'flipping' the
 * sign of the random point R which is not included in the signature.
 * Since the forgery is of the same message this isn't universally
 * problematic, but in systems where message malleability or uniqueness
 * of signatures is important this can cause issues.  This forgery can be
 * blocked by all verifiers forcing signers to use a canonical form. The
 * lower-S form reduces the size of signatures slightly on average when
 * variable length encodings (such as DER) are used and is cheap to
 * verify, making it a good choice. Security of always using lower-S is
 * assured because anyone can trivially modify a signature after the
 * fact to enforce this property.  Adjusting it inside the signing
 * function avoids the need to re-serialize or have curve specific
 * constants outside of the library.  By always using a canonical form
 * even in applications where it isn't needed it becomes possible to
 * impose a requirement later if a need is discovered.
 * No other forms of ECDSA malleability are known and none seem likely,
 * but there is no formal proof that ECDSA, even with this additional
 * restriction, is free of other malleability.  Commonly used serialization
 * schemes will also accept various non-unique encodings, so care should
 * be taken when this property is required for an application.
 */
int secp256k1_ecdsa_sign(
  const secp256k1_context_t* ctx,
  const unsigned char *msg32,
  secp256k1_ecdsa_signature_t *sig,
  const unsigned char *seckey,
  secp256k1_nonce_function_t noncefp,
  const void *ndata
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

/** Recover an ECDSA public key from a signature.
 *  Returns: 1: public key successfully recovered (which guarantees a correct signature).
 *           0: otherwise.
 *  In:      ctx:        pointer to a context object, initialized for verification (cannot be NULL)
 *           msg32:      the 32-byte message hash assumed to be signed (cannot be NULL)
 *           sig64:      pointer to initialized signature that supports pubkey recovery (cannot be NULL)
 *  Out:     pubkey:     pointer to the recoved public key (cannot be NULL)
 */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_ecdsa_recover(
  const secp256k1_context_t* ctx,
  const unsigned char *msg32,
  const secp256k1_ecdsa_signature_t *sig,
  secp256k1_pubkey_t *pubkey
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

/** Verify an ECDSA secret key.
 *  Returns: 1: secret key is valid
 *           0: secret key is invalid
 *  In:      ctx: pointer to a context object (cannot be NULL)
 *           seckey: pointer to a 32-byte secret key (cannot be NULL)
 */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_seckey_verify(
  const secp256k1_context_t* ctx,
  const unsigned char *seckey
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2);

/** Compute the public key for a secret key.
 *  In:     ctx:        pointer to a context object, initialized for signing (cannot be NULL)
 *          seckey:     pointer to a 32-byte private key (cannot be NULL)
 *  Out:    pubkey:     pointer to the created public key (cannot be NULL)
 *  Returns: 1: secret was valid, public key stores
 *           0: secret was invalid, try again
 */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_pubkey_create(
  const secp256k1_context_t* ctx,
  secp256k1_pubkey_t *pubkey,
  const unsigned char *seckey
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Export a private key in DER format.
 * In: ctx: pointer to a context object, initialized for signing (cannot be NULL)
 */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_privkey_export(
  const secp256k1_context_t* ctx,
  const unsigned char *seckey,
  unsigned char *privkey,
  int *privkeylen,
  int compressed
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

/** Import a private key in DER format. */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_privkey_import(
  const secp256k1_context_t* ctx,
  unsigned char *seckey,
  const unsigned char *privkey,
  int privkeylen
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Tweak a private key by adding tweak to it. */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_privkey_tweak_add(
  const secp256k1_context_t* ctx,
  unsigned char *seckey,
  const unsigned char *tweak
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Tweak a public key by adding tweak times the generator to it.
 * In: ctx: pointer to a context object, initialized for verification (cannot be NULL)
 */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_pubkey_tweak_add(
  const secp256k1_context_t* ctx,
  secp256k1_pubkey_t *pubkey,
  const unsigned char *tweak
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Tweak a private key by multiplying it with tweak. */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_privkey_tweak_mul(
  const secp256k1_context_t* ctx,
  unsigned char *seckey,
  const unsigned char *tweak
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Tweak a public key by multiplying it with tweak.
 * In: ctx: pointer to a context object, initialized for verification (cannot be NULL)
 */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_pubkey_tweak_mul(
  const secp256k1_context_t* ctx,
  secp256k1_pubkey_t *pubkey,
  const unsigned char *tweak
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Updates the context randomization.
 *  Returns: 1: randomization successfully updated
 *           0: error
 *  In:      ctx:       pointer to a context object (cannot be NULL)
 *           seed32:    pointer to a 32-byte random seed (NULL resets to initial state)
 */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_context_randomize(
  secp256k1_context_t* ctx,
  const unsigned char *seed32
) SECP256K1_ARG_NONNULL(1);

/** Add a number of public keys together.
 *  Returns: 1: the sum of the public keys is valid.
 *           0: the sum of the public keys is not valid.
 *  In:     ctx:        pointer to a context object
 *          out:        pointer to pubkey for placing the resulting public key
 *                      (cannot be NULL)
 *          n:          the number of public keys to add together (must be at least 1)
 *          ins:        pointer to array of pointers to public keys (cannot be NULL)
 *  Use secp256k1_ec_pubkey_compress and secp256k1_ec_pubkey_decompress if the
 *  uncompressed format is needed.
 */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_pubkey_combine(
  const secp256k1_context_t* ctx,
  secp256k1_pubkey_t *out,
  int n,
  const secp256k1_pubkey_t * const * ins
) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(4);

# ifdef __cplusplus
}
# endif

#endif
