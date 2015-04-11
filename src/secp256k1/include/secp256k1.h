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


/** Flags to pass to secp256k1_start. */
# define SECP256K1_START_VERIFY (1 << 0)
# define SECP256K1_START_SIGN   (1 << 1)

/** Initialize the library. This may take some time (10-100 ms).
 *  You need to call this before calling any other function.
 *  It cannot run in parallel with any other functions, but once
 *  secp256k1_start() returns, all other functions are thread-safe.
 */
void secp256k1_start(unsigned int flags);

/** Free all memory associated with this library. After this, no
 *  functions can be called anymore, except secp256k1_start()
 */
void secp256k1_stop(void);

/** Verify an ECDSA signature.
 *  Returns: 1: correct signature
 *           0: incorrect signature
 *          -1: invalid public key
 *          -2: invalid signature
 * In:       msg32:     the 32-byte message hash being verified (cannot be NULL)
 *           sig:       the signature being verified (cannot be NULL)
 *           siglen:    the length of the signature
 *           pubkey:    the public key to verify with (cannot be NULL)
 *           pubkeylen: the length of pubkey
 * Requires starting using SECP256K1_START_VERIFY.
 */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_ecdsa_verify(
  const unsigned char *msg32,
  const unsigned char *sig,
  int siglen,
  const unsigned char *pubkey,
  int pubkeylen
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(4);

/** A pointer to a function to deterministically generate a nonce.
 * Returns: 1 if a nonce was successfully generated. 0 will cause signing to fail.
 * In:      msg32:     the 32-byte message hash being verified (will not be NULL)
 *          key32:     pointer to a 32-byte secret key (will not be NULL)
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
 *           0: the nonce generation function failed, the private key was invalid, or there is not
 *              enough space in the signature (as indicated by siglen).
 *  In:      msg32:  the 32-byte message hash being signed (cannot be NULL)
 *           seckey: pointer to a 32-byte secret key (cannot be NULL)
 *           noncefp:pointer to a nonce generation function. If NULL, secp256k1_nonce_function_default is used
 *           ndata:  pointer to arbitrary data used by the nonce generation function (can be NULL)
 *  Out:     sig:    pointer to an array where the signature will be placed (cannot be NULL)
 *  In/Out:  siglen: pointer to an int with the length of sig, which will be updated
 *                   to contain the actual signature length (<=72). If 0 is returned, this will be
 *                   set to zero.
 * Requires starting using SECP256K1_START_SIGN.
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
  const unsigned char *msg32,
  unsigned char *sig,
  int *siglen,
  const unsigned char *seckey,
  secp256k1_nonce_function_t noncefp,
  const void *ndata
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

/** Create a compact ECDSA signature (64 byte + recovery id).
 *  Returns: 1: signature created
 *           0: the nonce generation function failed, or the secret key was invalid.
 *  In:      msg32:  the 32-byte message hash being signed (cannot be NULL)
 *           seckey: pointer to a 32-byte secret key (cannot be NULL)
 *           noncefp:pointer to a nonce generation function. If NULL, secp256k1_nonce_function_default is used
 *           ndata:  pointer to arbitrary data used by the nonce generation function (can be NULL)
 *  Out:     sig:    pointer to a 64-byte array where the signature will be placed (cannot be NULL)
 *                   In case 0 is returned, the returned signature length will be zero.
 *           recid:  pointer to an int, which will be updated to contain the recovery id (can be NULL)
 * Requires starting using SECP256K1_START_SIGN.
 */
int secp256k1_ecdsa_sign_compact(
  const unsigned char *msg32,
  unsigned char *sig64,
  const unsigned char *seckey,
  secp256k1_nonce_function_t noncefp,
  const void *ndata,
  int *recid
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Recover an ECDSA public key from a compact signature.
 *  Returns: 1: public key successfully recovered (which guarantees a correct signature).
 *           0: otherwise.
 *  In:      msg32:      the 32-byte message hash assumed to be signed (cannot be NULL)
 *           sig64:      signature as 64 byte array (cannot be NULL)
 *           compressed: whether to recover a compressed or uncompressed pubkey
 *           recid:      the recovery id (0-3, as returned by ecdsa_sign_compact)
 *  Out:     pubkey:     pointer to a 33 or 65 byte array to put the pubkey (cannot be NULL)
 *           pubkeylen:  pointer to an int that will contain the pubkey length (cannot be NULL)
 * Requires starting using SECP256K1_START_VERIFY.
 */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_ecdsa_recover_compact(
  const unsigned char *msg32,
  const unsigned char *sig64,
  unsigned char *pubkey,
  int *pubkeylen,
  int compressed,
  int recid
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

/** Verify an ECDSA secret key.
 *  Returns: 1: secret key is valid
 *           0: secret key is invalid
 *  In:      seckey: pointer to a 32-byte secret key (cannot be NULL)
 */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_seckey_verify(const unsigned char *seckey) SECP256K1_ARG_NONNULL(1);

/** Just validate a public key.
 *  Returns: 1: valid public key
 *           0: invalid public key
 *  In:      pubkey:    pointer to a 33-byte or 65-byte public key (cannot be NULL).
 *           pubkeylen: length of pubkey
 */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_pubkey_verify(const unsigned char *pubkey, int pubkeylen) SECP256K1_ARG_NONNULL(1);

/** Compute the public key for a secret key.
 *  In:     compressed: whether the computed public key should be compressed
 *          seckey:     pointer to a 32-byte private key (cannot be NULL)
 *  Out:    pubkey:     pointer to a 33-byte (if compressed) or 65-byte (if uncompressed)
 *                      area to store the public key (cannot be NULL)
 *          pubkeylen:  pointer to int that will be updated to contains the pubkey's
 *                      length (cannot be NULL)
 *  Returns: 1: secret was valid, public key stores
 *           0: secret was invalid, try again.
 * Requires starting using SECP256K1_START_SIGN.
 */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_pubkey_create(
  unsigned char *pubkey,
  int *pubkeylen,
  const unsigned char *seckey,
  int compressed
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Decompress a public key.
 * In/Out: pubkey:    pointer to a 65-byte array to put the decompressed public key.
                      It must contain a 33-byte or 65-byte public key already (cannot be NULL)
 *         pubkeylen: pointer to the size of the public key pointed to by pubkey (cannot be NULL)
                      It will be updated to reflect the new size.
 * Returns: 0 if the passed public key was invalid, 1 otherwise. If 1 is returned, the
            pubkey is replaced with its decompressed version.
 */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_pubkey_decompress(
  unsigned char *pubkey,
  int *pubkeylen
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2);

/** Export a private key in DER format. */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_privkey_export(
  const unsigned char *seckey,
  unsigned char *privkey,
  int *privkeylen,
  int compressed
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Import a private key in DER format. */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_privkey_import(
  unsigned char *seckey,
  const unsigned char *privkey,
  int privkeylen
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2);

/** Tweak a private key by adding tweak to it. */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_privkey_tweak_add(
  unsigned char *seckey,
  const unsigned char *tweak
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2);

/** Tweak a public key by adding tweak times the generator to it.
 * Requires starting with SECP256K1_START_VERIFY.
 */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_pubkey_tweak_add(
  unsigned char *pubkey,
  int pubkeylen,
  const unsigned char *tweak
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(3);

/** Tweak a private key by multiplying it with tweak. */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_privkey_tweak_mul(
  unsigned char *seckey,
  const unsigned char *tweak
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2);

/** Tweak a public key by multiplying it with tweak.
 * Requires starting with SECP256K1_START_VERIFY.
 */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_pubkey_tweak_mul(
  unsigned char *pubkey,
  int pubkeylen,
  const unsigned char *tweak
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(3);

# ifdef __cplusplus
}
# endif

#endif
