#ifndef SECP256K1_H
#define SECP256K1_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/** Unless explicitly stated all pointer arguments must not be NULL.
 *
 * The following rules specify the order of arguments in API calls:
 *
 * 1. Context pointers go first, followed by output arguments, combined
 *    output/input arguments, and finally input-only arguments.
 * 2. Array lengths always immediately follow the argument whose length
 *    they describe, even if this violates rule 1.
 * 3. Within the OUT/OUTIN/IN groups, pointers to data that is typically generated
 *    later go first. This means: signatures, public nonces, secret nonces,
 *    messages, public keys, secret keys, tweaks.
 * 4. Arguments that are not data pointers go last, from more complex to less
 *    complex: function pointers, algorithm names, messages, void pointers,
 *    counts, flags, booleans.
 * 5. Opaque data pointers follow the function pointer they are to be passed to.
 */

/** Opaque data structure that holds context information
 *
 *  The primary purpose of context objects is to store randomization data for
 *  enhanced protection against side-channel leakage. This protection is only
 *  effective if the context is randomized after its creation. See
 *  secp256k1_context_create for creation of contexts and
 *  secp256k1_context_randomize for randomization.
 *
 *  A secondary purpose of context objects is to store pointers to callback
 *  functions that the library will call when certain error states arise. See
 *  secp256k1_context_set_error_callback as well as
 *  secp256k1_context_set_illegal_callback for details. Future library versions
 *  may use context objects for additional purposes.
 *
 *  A constructed context can safely be used from multiple threads
 *  simultaneously, but API calls that take a non-const pointer to a context
 *  need exclusive access to it. In particular this is the case for
 *  secp256k1_context_destroy, secp256k1_context_preallocated_destroy,
 *  and secp256k1_context_randomize.
 *
 *  Regarding randomization, either do it once at creation time (in which case
 *  you do not need any locking for the other calls), or use a read-write lock.
 */
typedef struct secp256k1_context_struct secp256k1_context;

/** Opaque data structure that holds rewritable "scratch space"
 *
 *  The purpose of this structure is to replace dynamic memory allocations,
 *  because we target architectures where this may not be available. It is
 *  essentially a resizable (within specified parameters) block of bytes,
 *  which is initially created either by memory allocation or TODO as a pointer
 *  into some fixed rewritable space.
 *
 *  Unlike the context object, this cannot safely be shared between threads
 *  without additional synchronization logic.
 */
typedef struct secp256k1_scratch_space_struct secp256k1_scratch_space;

/** Opaque data structure that holds a parsed and valid public key.
 *
 *  The exact representation of data inside is implementation defined and not
 *  guaranteed to be portable between different platforms or versions. It is
 *  however guaranteed to be 64 bytes in size, and can be safely copied/moved.
 *  If you need to convert to a format suitable for storage or transmission,
 *  use secp256k1_ec_pubkey_serialize and secp256k1_ec_pubkey_parse. To
 *  compare keys, use secp256k1_ec_pubkey_cmp.
 */
typedef struct {
    unsigned char data[64];
} secp256k1_pubkey;

/** Opaque data structured that holds a parsed ECDSA signature.
 *
 *  The exact representation of data inside is implementation defined and not
 *  guaranteed to be portable between different platforms or versions. It is
 *  however guaranteed to be 64 bytes in size, and can be safely copied/moved.
 *  If you need to convert to a format suitable for storage, transmission, or
 *  comparison, use the secp256k1_ecdsa_signature_serialize_* and
 *  secp256k1_ecdsa_signature_parse_* functions.
 */
typedef struct {
    unsigned char data[64];
} secp256k1_ecdsa_signature;

/** A pointer to a function to deterministically generate a nonce.
 *
 * Returns: 1 if a nonce was successfully generated. 0 will cause signing to fail.
 * Out:     nonce32:   pointer to a 32-byte array to be filled by the function.
 * In:      msg32:     the 32-byte message hash being verified (will not be NULL)
 *          key32:     pointer to a 32-byte secret key (will not be NULL)
 *          algo16:    pointer to a 16-byte array describing the signature
 *                     algorithm (will be NULL for ECDSA for compatibility).
 *          data:      Arbitrary data pointer that is passed through.
 *          attempt:   how many iterations we have tried to find a nonce.
 *                     This will almost always be 0, but different attempt values
 *                     are required to result in a different nonce.
 *
 * Except for test cases, this function should compute some cryptographic hash of
 * the message, the algorithm, the key and the attempt.
 */
typedef int (*secp256k1_nonce_function)(
    unsigned char *nonce32,
    const unsigned char *msg32,
    const unsigned char *key32,
    const unsigned char *algo16,
    void *data,
    unsigned int attempt
);

# if !defined(SECP256K1_GNUC_PREREQ)
#  if defined(__GNUC__)&&defined(__GNUC_MINOR__)
#   define SECP256K1_GNUC_PREREQ(_maj,_min) \
 ((__GNUC__<<16)+__GNUC_MINOR__>=((_maj)<<16)+(_min))
#  else
#   define SECP256K1_GNUC_PREREQ(_maj,_min) 0
#  endif
# endif

/*  When this header is used at build-time the SECP256K1_BUILD define needs to be set
 *  to correctly setup export attributes and nullness checks.  This is normally done
 *  by secp256k1.c but to guard against this header being included before secp256k1.c
 *  has had a chance to set the define (e.g. via test harnesses that just includes
 *  secp256k1.c) we set SECP256K1_NO_BUILD when this header is processed without the
 *  BUILD define so this condition can be caught.
 */
#ifndef SECP256K1_BUILD
# define SECP256K1_NO_BUILD
#endif

/* Symbol visibility. */
#if defined(_WIN32)
  /* GCC for Windows (e.g., MinGW) accepts the __declspec syntax
   * for MSVC compatibility. A __declspec declaration implies (but is not
   * exactly equivalent to) __attribute__ ((visibility("default"))), and so we
   * actually want __declspec even on GCC, see "Microsoft Windows Function
   * Attributes" in the GCC manual and the recommendations in
   * https://gcc.gnu.org/wiki/Visibility. */
# if defined(SECP256K1_BUILD)
#  if defined(DLL_EXPORT) || defined(SECP256K1_DLL_EXPORT)
    /* Building libsecp256k1 as a DLL.
     * 1. If using Libtool, it defines DLL_EXPORT automatically.
     * 2. In other cases, SECP256K1_DLL_EXPORT must be defined. */
#   define SECP256K1_API extern __declspec (dllexport)
#  endif
  /* The user must define SECP256K1_STATIC when consuming libsecp256k1 as a static
   * library on Windows. */
# elif !defined(SECP256K1_STATIC)
   /* Consuming libsecp256k1 as a DLL. */
#  define SECP256K1_API extern __declspec (dllimport)
# endif
#endif
#ifndef SECP256K1_API
# if defined(__GNUC__) && (__GNUC__ >= 4) && defined(SECP256K1_BUILD)
   /* Building libsecp256k1 on non-Windows using GCC or compatible. */
#  define SECP256K1_API extern __attribute__ ((visibility ("default")))
# else
   /* All cases not captured above. */
#  define SECP256K1_API extern
# endif
#endif

/* Warning attributes
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

/* Attribute for marking functions, types, and variables as deprecated */
#if !defined(SECP256K1_BUILD) && defined(__has_attribute)
# if __has_attribute(__deprecated__)
#  define SECP256K1_DEPRECATED(_msg) __attribute__ ((__deprecated__(_msg)))
# else
#  define SECP256K1_DEPRECATED(_msg)
# endif
#else
# define SECP256K1_DEPRECATED(_msg)
#endif

/* All flags' lower 8 bits indicate what they're for. Do not use directly. */
#define SECP256K1_FLAGS_TYPE_MASK ((1 << 8) - 1)
#define SECP256K1_FLAGS_TYPE_CONTEXT (1 << 0)
#define SECP256K1_FLAGS_TYPE_COMPRESSION (1 << 1)
/* The higher bits contain the actual data. Do not use directly. */
#define SECP256K1_FLAGS_BIT_CONTEXT_VERIFY (1 << 8)
#define SECP256K1_FLAGS_BIT_CONTEXT_SIGN (1 << 9)
#define SECP256K1_FLAGS_BIT_CONTEXT_DECLASSIFY (1 << 10)
#define SECP256K1_FLAGS_BIT_COMPRESSION (1 << 8)

/** Context flags to pass to secp256k1_context_create, secp256k1_context_preallocated_size, and
 *  secp256k1_context_preallocated_create. */
#define SECP256K1_CONTEXT_NONE (SECP256K1_FLAGS_TYPE_CONTEXT)

/** Deprecated context flags. These flags are treated equivalent to SECP256K1_CONTEXT_NONE. */
#define SECP256K1_CONTEXT_VERIFY (SECP256K1_FLAGS_TYPE_CONTEXT | SECP256K1_FLAGS_BIT_CONTEXT_VERIFY)
#define SECP256K1_CONTEXT_SIGN (SECP256K1_FLAGS_TYPE_CONTEXT | SECP256K1_FLAGS_BIT_CONTEXT_SIGN)

/* Testing flag. Do not use. */
#define SECP256K1_CONTEXT_DECLASSIFY (SECP256K1_FLAGS_TYPE_CONTEXT | SECP256K1_FLAGS_BIT_CONTEXT_DECLASSIFY)

/** Flag to pass to secp256k1_ec_pubkey_serialize. */
#define SECP256K1_EC_COMPRESSED (SECP256K1_FLAGS_TYPE_COMPRESSION | SECP256K1_FLAGS_BIT_COMPRESSION)
#define SECP256K1_EC_UNCOMPRESSED (SECP256K1_FLAGS_TYPE_COMPRESSION)

/** Prefix byte used to tag various encoded curvepoints for specific purposes */
#define SECP256K1_TAG_PUBKEY_EVEN 0x02
#define SECP256K1_TAG_PUBKEY_ODD 0x03
#define SECP256K1_TAG_PUBKEY_UNCOMPRESSED 0x04
#define SECP256K1_TAG_PUBKEY_HYBRID_EVEN 0x06
#define SECP256K1_TAG_PUBKEY_HYBRID_ODD 0x07

/** A built-in constant secp256k1 context object with static storage duration, to be
 *  used in conjunction with secp256k1_selftest.
 *
 *  This context object offers *only limited functionality* , i.e., it cannot be used
 *  for API functions that perform computations involving secret keys, e.g., signing
 *  and public key generation. If this restriction applies to a specific API function,
 *  it is mentioned in its documentation. See secp256k1_context_create if you need a
 *  full context object that supports all functionality offered by the library.
 *
 *  It is highly recommended to call secp256k1_selftest before using this context.
 */
SECP256K1_API const secp256k1_context *secp256k1_context_static;

/** Deprecated alias for secp256k1_context_static. */
SECP256K1_API const secp256k1_context *secp256k1_context_no_precomp
SECP256K1_DEPRECATED("Use secp256k1_context_static instead");

/** Perform basic self tests (to be used in conjunction with secp256k1_context_static)
 *
 *  This function performs self tests that detect some serious usage errors and
 *  similar conditions, e.g., when the library is compiled for the wrong endianness.
 *  This is a last resort measure to be used in production. The performed tests are
 *  very rudimentary and are not intended as a replacement for running the test
 *  binaries.
 *
 *  It is highly recommended to call this before using secp256k1_context_static.
 *  It is not necessary to call this function before using a context created with
 *  secp256k1_context_create (or secp256k1_context_preallocated_create), which will
 *  take care of performing the self tests.
 *
 *  If the tests fail, this function will call the default error handler to abort the
 *  program (see secp256k1_context_set_error_callback).
 */
SECP256K1_API void secp256k1_selftest(void);


/** Create a secp256k1 context object (in dynamically allocated memory).
 *
 *  This function uses malloc to allocate memory. It is guaranteed that malloc is
 *  called at most once for every call of this function. If you need to avoid dynamic
 *  memory allocation entirely, see secp256k1_context_static and the functions in
 *  secp256k1_preallocated.h.
 *
 *  Returns: pointer to a newly created context object.
 *  In:      flags: Always set to SECP256K1_CONTEXT_NONE (see below).
 *
 *  The only valid non-deprecated flag in recent library versions is
 *  SECP256K1_CONTEXT_NONE, which will create a context sufficient for all functionality
 *  offered by the library. All other (deprecated) flags will be treated as equivalent
 *  to the SECP256K1_CONTEXT_NONE flag. Though the flags parameter primarily exists for
 *  historical reasons, future versions of the library may introduce new flags.
 *
 *  If the context is intended to be used for API functions that perform computations
 *  involving secret keys, e.g., signing and public key generation, then it is highly
 *  recommended to call secp256k1_context_randomize on the context before calling
 *  those API functions. This will provide enhanced protection against side-channel
 *  leakage, see secp256k1_context_randomize for details.
 *
 *  Do not create a new context object for each operation, as construction and
 *  randomization can take non-negligible time.
 */
SECP256K1_API secp256k1_context *secp256k1_context_create(
    unsigned int flags
) SECP256K1_WARN_UNUSED_RESULT;

/** Copy a secp256k1 context object (into dynamically allocated memory).
 *
 *  This function uses malloc to allocate memory. It is guaranteed that malloc is
 *  called at most once for every call of this function. If you need to avoid dynamic
 *  memory allocation entirely, see the functions in secp256k1_preallocated.h.
 *
 *  Cloning secp256k1_context_static is not possible, and should not be emulated by
 *  the caller (e.g., using memcpy). Create a new context instead.
 *
 *  Returns: pointer to a newly created context object.
 *  Args:    ctx: pointer to a context to copy (not secp256k1_context_static).
 */
SECP256K1_API secp256k1_context *secp256k1_context_clone(
    const secp256k1_context *ctx
) SECP256K1_ARG_NONNULL(1) SECP256K1_WARN_UNUSED_RESULT;

/** Destroy a secp256k1 context object (created in dynamically allocated memory).
 *
 *  The context pointer may not be used afterwards.
 *
 *  The context to destroy must have been created using secp256k1_context_create
 *  or secp256k1_context_clone. If the context has instead been created using
 *  secp256k1_context_preallocated_create or secp256k1_context_preallocated_clone, the
 *  behaviour is undefined. In that case, secp256k1_context_preallocated_destroy must
 *  be used instead.
 *
 *  Args:   ctx: pointer to a context to destroy, constructed using
 *               secp256k1_context_create or secp256k1_context_clone
 *               (i.e., not secp256k1_context_static).
 */
SECP256K1_API void secp256k1_context_destroy(
    secp256k1_context *ctx
) SECP256K1_ARG_NONNULL(1);

/** Set a callback function to be called when an illegal argument is passed to
 *  an API call. It will only trigger for violations that are mentioned
 *  explicitly in the header.
 *
 *  The philosophy is that these shouldn't be dealt with through a
 *  specific return value, as calling code should not have branches to deal with
 *  the case that this code itself is broken.
 *
 *  On the other hand, during debug stage, one would want to be informed about
 *  such mistakes, and the default (crashing) may be inadvisable.
 *  When this callback is triggered, the API function called is guaranteed not
 *  to cause a crash, though its return value and output arguments are
 *  undefined.
 *
 *  When this function has not been called (or called with fn==NULL), then the
 *  default handler will be used. The library provides a default handler which
 *  writes the message to stderr and calls abort. This default handler can be
 *  replaced at link time if the preprocessor macro
 *  USE_EXTERNAL_DEFAULT_CALLBACKS is defined, which is the case if the build
 *  has been configured with --enable-external-default-callbacks. Then the
 *  following two symbols must be provided to link against:
 *   - void secp256k1_default_illegal_callback_fn(const char *message, void *data);
 *   - void secp256k1_default_error_callback_fn(const char *message, void *data);
 *  The library can call these default handlers even before a proper callback data
 *  pointer could have been set using secp256k1_context_set_illegal_callback or
 *  secp256k1_context_set_error_callback, e.g., when the creation of a context
 *  fails. In this case, the corresponding default handler will be called with
 *  the data pointer argument set to NULL.
 *
 *  Args: ctx:  pointer to a context object.
 *  In:   fun:  pointer to a function to call when an illegal argument is
 *              passed to the API, taking a message and an opaque pointer.
 *              (NULL restores the default handler.)
 *        data: the opaque pointer to pass to fun above, must be NULL for the default handler.
 *
 *  See also secp256k1_context_set_error_callback.
 */
SECP256K1_API void secp256k1_context_set_illegal_callback(
    secp256k1_context *ctx,
    void (*fun)(const char *message, void *data),
    const void *data
) SECP256K1_ARG_NONNULL(1);

/** Set a callback function to be called when an internal consistency check
 *  fails.
 *
 *  The default callback writes an error message to stderr and calls abort
 *  to abort the program.
 *
 *  This can only trigger in case of a hardware failure, miscompilation,
 *  memory corruption, serious bug in the library, or other error would can
 *  otherwise result in undefined behaviour. It will not trigger due to mere
 *  incorrect usage of the API (see secp256k1_context_set_illegal_callback
 *  for that). After this callback returns, anything may happen, including
 *  crashing.
 *
 *  Args: ctx:  pointer to a context object.
 *  In:   fun:  pointer to a function to call when an internal error occurs,
 *              taking a message and an opaque pointer (NULL restores the
 *              default handler, see secp256k1_context_set_illegal_callback
 *              for details).
 *        data: the opaque pointer to pass to fun above, must be NULL for the default handler.
 *
 *  See also secp256k1_context_set_illegal_callback.
 */
SECP256K1_API void secp256k1_context_set_error_callback(
    secp256k1_context *ctx,
    void (*fun)(const char *message, void *data),
    const void *data
) SECP256K1_ARG_NONNULL(1);

/** Create a secp256k1 scratch space object.
 *
 *  Returns: a newly created scratch space.
 *  Args: ctx:  pointer to a context object.
 *  In:   size: amount of memory to be available as scratch space. Some extra
 *              (<100 bytes) will be allocated for extra accounting.
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT secp256k1_scratch_space *secp256k1_scratch_space_create(
    const secp256k1_context *ctx,
    size_t size
) SECP256K1_ARG_NONNULL(1);

/** Destroy a secp256k1 scratch space.
 *
 *  The pointer may not be used afterwards.
 *  Args:       ctx: pointer to a context object.
 *          scratch: space to destroy
 */
SECP256K1_API void secp256k1_scratch_space_destroy(
    const secp256k1_context *ctx,
    secp256k1_scratch_space *scratch
) SECP256K1_ARG_NONNULL(1);

/** Parse a variable-length public key into the pubkey object.
 *
 *  Returns: 1 if the public key was fully valid.
 *           0 if the public key could not be parsed or is invalid.
 *  Args: ctx:      pointer to a context object.
 *  Out:  pubkey:   pointer to a pubkey object. If 1 is returned, it is set to a
 *                  parsed version of input. If not, its value is undefined.
 *  In:   input:    pointer to a serialized public key
 *        inputlen: length of the array pointed to by input
 *
 *  This function supports parsing compressed (33 bytes, header byte 0x02 or
 *  0x03), uncompressed (65 bytes, header byte 0x04), or hybrid (65 bytes, header
 *  byte 0x06 or 0x07) format public keys.
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_pubkey_parse(
    const secp256k1_context *ctx,
    secp256k1_pubkey *pubkey,
    const unsigned char *input,
    size_t inputlen
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Serialize a pubkey object into a serialized byte sequence.
 *
 *  Returns: 1 always.
 *  Args:   ctx:        pointer to a context object.
 *  Out:    output:     pointer to a 65-byte (if compressed==0) or 33-byte (if
 *                      compressed==1) byte array to place the serialized key
 *                      in.
 *  In/Out: outputlen:  pointer to an integer which is initially set to the
 *                      size of output, and is overwritten with the written
 *                      size.
 *  In:     pubkey:     pointer to a secp256k1_pubkey containing an
 *                      initialized public key.
 *          flags:      SECP256K1_EC_COMPRESSED if serialization should be in
 *                      compressed format, otherwise SECP256K1_EC_UNCOMPRESSED.
 */
SECP256K1_API int secp256k1_ec_pubkey_serialize(
    const secp256k1_context *ctx,
    unsigned char *output,
    size_t *outputlen,
    const secp256k1_pubkey *pubkey,
    unsigned int flags
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

/** Compare two public keys using lexicographic (of compressed serialization) order
 *
 *  Returns: <0 if the first public key is less than the second
 *           >0 if the first public key is greater than the second
 *           0 if the two public keys are equal
 *  Args: ctx:      pointer to a context object
 *  In:   pubkey1:  first public key to compare
 *        pubkey2:  second public key to compare
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_pubkey_cmp(
    const secp256k1_context *ctx,
    const secp256k1_pubkey *pubkey1,
    const secp256k1_pubkey *pubkey2
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Sort public keys keys using lexicographic (of compressed serialization) order
 *
 *  Returns: 0 if the arguments are invalid. 1 otherwise.
 *
 *  Args:     ctx: pointer to a context object
 *  In:   pubkeys: array of pointers to pubkeys to sort
 *      n_pubkeys: number of elements in the pubkeys array
 */
SECP256K1_API int secp256k1_ec_pubkey_sort(
    const secp256k1_context *ctx,
    const secp256k1_pubkey **pubkeys,
    size_t n_pubkeys
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2);

/** Parse an ECDSA signature in compact (64 bytes) format.
 *
 *  Returns: 1 when the signature could be parsed, 0 otherwise.
 *  Args: ctx:      pointer to a context object
 *  Out:  sig:      pointer to a signature object
 *  In:   input64:  pointer to the 64-byte array to parse
 *
 *  The signature must consist of a 32-byte big endian R value, followed by a
 *  32-byte big endian S value. If R or S fall outside of [0..order-1], the
 *  encoding is invalid. R and S with value 0 are allowed in the encoding.
 *
 *  After the call, sig will always be initialized. If parsing failed or R or
 *  S are zero, the resulting sig value is guaranteed to fail verification for
 *  any message and public key.
 */
SECP256K1_API int secp256k1_ecdsa_signature_parse_compact(
    const secp256k1_context *ctx,
    secp256k1_ecdsa_signature *sig,
    const unsigned char *input64
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Parse a DER ECDSA signature.
 *
 *  Returns: 1 when the signature could be parsed, 0 otherwise.
 *  Args: ctx:      pointer to a context object
 *  Out:  sig:      pointer to a signature object
 *  In:   input:    pointer to the signature to be parsed
 *        inputlen: the length of the array pointed to be input
 *
 *  This function will accept any valid DER encoded signature, even if the
 *  encoded numbers are out of range.
 *
 *  After the call, sig will always be initialized. If parsing failed or the
 *  encoded numbers are out of range, signature verification with it is
 *  guaranteed to fail for every message and public key.
 */
SECP256K1_API int secp256k1_ecdsa_signature_parse_der(
    const secp256k1_context *ctx,
    secp256k1_ecdsa_signature *sig,
    const unsigned char *input,
    size_t inputlen
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Serialize an ECDSA signature in DER format.
 *
 *  Returns: 1 if enough space was available to serialize, 0 otherwise
 *  Args:   ctx:       pointer to a context object
 *  Out:    output:    pointer to an array to store the DER serialization
 *  In/Out: outputlen: pointer to a length integer. Initially, this integer
 *                     should be set to the length of output. After the call
 *                     it will be set to the length of the serialization (even
 *                     if 0 was returned).
 *  In:     sig:       pointer to an initialized signature object
 */
SECP256K1_API int secp256k1_ecdsa_signature_serialize_der(
    const secp256k1_context *ctx,
    unsigned char *output,
    size_t *outputlen,
    const secp256k1_ecdsa_signature *sig
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

/** Serialize an ECDSA signature in compact (64 byte) format.
 *
 *  Returns: 1
 *  Args:   ctx:       pointer to a context object
 *  Out:    output64:  pointer to a 64-byte array to store the compact serialization
 *  In:     sig:       pointer to an initialized signature object
 *
 *  See secp256k1_ecdsa_signature_parse_compact for details about the encoding.
 */
SECP256K1_API int secp256k1_ecdsa_signature_serialize_compact(
    const secp256k1_context *ctx,
    unsigned char *output64,
    const secp256k1_ecdsa_signature *sig
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Verify an ECDSA signature.
 *
 *  Returns: 1: correct signature
 *           0: incorrect or unparseable signature
 *  Args:    ctx:       pointer to a context object
 *  In:      sig:       the signature being verified.
 *           msghash32: the 32-byte message hash being verified.
 *                      The verifier must make sure to apply a cryptographic
 *                      hash function to the message by itself and not accept an
 *                      msghash32 value directly. Otherwise, it would be easy to
 *                      create a "valid" signature without knowledge of the
 *                      secret key. See also
 *                      https://bitcoin.stackexchange.com/a/81116/35586 for more
 *                      background on this topic.
 *           pubkey:    pointer to an initialized public key to verify with.
 *
 * To avoid accepting malleable signatures, only ECDSA signatures in lower-S
 * form are accepted.
 *
 * If you need to accept ECDSA signatures from sources that do not obey this
 * rule, apply secp256k1_ecdsa_signature_normalize to the signature prior to
 * verification, but be aware that doing so results in malleable signatures.
 *
 * For details, see the comments for that function.
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_ecdsa_verify(
    const secp256k1_context *ctx,
    const secp256k1_ecdsa_signature *sig,
    const unsigned char *msghash32,
    const secp256k1_pubkey *pubkey
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

/** Convert a signature to a normalized lower-S form.
 *
 *  Returns: 1 if sigin was not normalized, 0 if it already was.
 *  Args: ctx:    pointer to a context object
 *  Out:  sigout: pointer to a signature to fill with the normalized form,
 *                or copy if the input was already normalized. (can be NULL if
 *                you're only interested in whether the input was already
 *                normalized).
 *  In:   sigin:  pointer to a signature to check/normalize (can be identical to sigout)
 *
 *  With ECDSA a third-party can forge a second distinct signature of the same
 *  message, given a single initial signature, but without knowing the key. This
 *  is done by negating the S value modulo the order of the curve, 'flipping'
 *  the sign of the random point R which is not included in the signature.
 *
 *  Forgery of the same message isn't universally problematic, but in systems
 *  where message malleability or uniqueness of signatures is important this can
 *  cause issues. This forgery can be blocked by all verifiers forcing signers
 *  to use a normalized form.
 *
 *  The lower-S form reduces the size of signatures slightly on average when
 *  variable length encodings (such as DER) are used and is cheap to verify,
 *  making it a good choice. Security of always using lower-S is assured because
 *  anyone can trivially modify a signature after the fact to enforce this
 *  property anyway.
 *
 *  The lower S value is always between 0x1 and
 *  0x7FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF5D576E7357A4501DDFE92F46681B20A0,
 *  inclusive.
 *
 *  No other forms of ECDSA malleability are known and none seem likely, but
 *  there is no formal proof that ECDSA, even with this additional restriction,
 *  is free of other malleability. Commonly used serialization schemes will also
 *  accept various non-unique encodings, so care should be taken when this
 *  property is required for an application.
 *
 *  The secp256k1_ecdsa_sign function will by default create signatures in the
 *  lower-S form, and secp256k1_ecdsa_verify will not accept others. In case
 *  signatures come from a system that cannot enforce this property,
 *  secp256k1_ecdsa_signature_normalize must be called before verification.
 */
SECP256K1_API int secp256k1_ecdsa_signature_normalize(
    const secp256k1_context *ctx,
    secp256k1_ecdsa_signature *sigout,
    const secp256k1_ecdsa_signature *sigin
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(3);

/** An implementation of RFC6979 (using HMAC-SHA256) as nonce generation function.
 * If a data pointer is passed, it is assumed to be a pointer to 32 bytes of
 * extra entropy.
 */
SECP256K1_API const secp256k1_nonce_function secp256k1_nonce_function_rfc6979;

/** A default safe nonce generation function (currently equal to secp256k1_nonce_function_rfc6979). */
SECP256K1_API const secp256k1_nonce_function secp256k1_nonce_function_default;

/** Create an ECDSA signature.
 *
 *  Returns: 1: signature created
 *           0: the nonce generation function failed, or the secret key was invalid.
 *  Args:    ctx:       pointer to a context object (not secp256k1_context_static).
 *  Out:     sig:       pointer to an array where the signature will be placed.
 *  In:      msghash32: the 32-byte message hash being signed.
 *           seckey:    pointer to a 32-byte secret key.
 *           noncefp:   pointer to a nonce generation function. If NULL,
 *                      secp256k1_nonce_function_default is used.
 *           ndata:     pointer to arbitrary data used by the nonce generation function
 *                      (can be NULL). If it is non-NULL and
 *                      secp256k1_nonce_function_default is used, then ndata must be a
 *                      pointer to 32-bytes of additional data.
 *
 * The created signature is always in lower-S form. See
 * secp256k1_ecdsa_signature_normalize for more details.
 */
SECP256K1_API int secp256k1_ecdsa_sign(
    const secp256k1_context *ctx,
    secp256k1_ecdsa_signature *sig,
    const unsigned char *msghash32,
    const unsigned char *seckey,
    secp256k1_nonce_function noncefp,
    const void *ndata
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

/** Verify an ECDSA secret key.
 *
 *  A secret key is valid if it is not 0 and less than the secp256k1 curve order
 *  when interpreted as an integer (most significant byte first). The
 *  probability of choosing a 32-byte string uniformly at random which is an
 *  invalid secret key is negligible.
 *
 *  Returns: 1: secret key is valid
 *           0: secret key is invalid
 *  Args:    ctx: pointer to a context object.
 *  In:      seckey: pointer to a 32-byte secret key.
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_seckey_verify(
    const secp256k1_context *ctx,
    const unsigned char *seckey
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2);

/** Compute the public key for a secret key.
 *
 *  Returns: 1: secret was valid, public key stores.
 *           0: secret was invalid, try again.
 *  Args:    ctx:    pointer to a context object (not secp256k1_context_static).
 *  Out:     pubkey: pointer to the created public key.
 *  In:      seckey: pointer to a 32-byte secret key.
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_pubkey_create(
    const secp256k1_context *ctx,
    secp256k1_pubkey *pubkey,
    const unsigned char *seckey
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Negates a secret key in place.
 *
 *  Returns: 0 if the given secret key is invalid according to
 *           secp256k1_ec_seckey_verify. 1 otherwise
 *  Args:   ctx:    pointer to a context object
 *  In/Out: seckey: pointer to the 32-byte secret key to be negated. If the
 *                  secret key is invalid according to
 *                  secp256k1_ec_seckey_verify, this function returns 0 and
 *                  seckey will be set to some unspecified value.
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_seckey_negate(
    const secp256k1_context *ctx,
    unsigned char *seckey
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2);

/** Same as secp256k1_ec_seckey_negate, but DEPRECATED. Will be removed in
 *  future versions. */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_privkey_negate(
    const secp256k1_context *ctx,
    unsigned char *seckey
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2)
  SECP256K1_DEPRECATED("Use secp256k1_ec_seckey_negate instead");

/** Negates a public key in place.
 *
 *  Returns: 1 always
 *  Args:   ctx:        pointer to a context object
 *  In/Out: pubkey:     pointer to the public key to be negated.
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_pubkey_negate(
    const secp256k1_context *ctx,
    secp256k1_pubkey *pubkey
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2);

/** Tweak a secret key by adding tweak to it.
 *
 *  Returns: 0 if the arguments are invalid or the resulting secret key would be
 *           invalid (only when the tweak is the negation of the secret key). 1
 *           otherwise.
 *  Args:    ctx:   pointer to a context object.
 *  In/Out: seckey: pointer to a 32-byte secret key. If the secret key is
 *                  invalid according to secp256k1_ec_seckey_verify, this
 *                  function returns 0. seckey will be set to some unspecified
 *                  value if this function returns 0.
 *  In:    tweak32: pointer to a 32-byte tweak, which must be valid according to
 *                  secp256k1_ec_seckey_verify or 32 zero bytes. For uniformly
 *                  random 32-byte tweaks, the chance of being invalid is
 *                  negligible (around 1 in 2^128).
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_seckey_tweak_add(
    const secp256k1_context *ctx,
    unsigned char *seckey,
    const unsigned char *tweak32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Same as secp256k1_ec_seckey_tweak_add, but DEPRECATED. Will be removed in
 *  future versions. */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_privkey_tweak_add(
    const secp256k1_context *ctx,
    unsigned char *seckey,
    const unsigned char *tweak32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3)
  SECP256K1_DEPRECATED("Use secp256k1_ec_seckey_tweak_add instead");

/** Tweak a public key by adding tweak times the generator to it.
 *
 *  Returns: 0 if the arguments are invalid or the resulting public key would be
 *           invalid (only when the tweak is the negation of the corresponding
 *           secret key). 1 otherwise.
 *  Args:    ctx:   pointer to a context object.
 *  In/Out: pubkey: pointer to a public key object. pubkey will be set to an
 *                  invalid value if this function returns 0.
 *  In:    tweak32: pointer to a 32-byte tweak, which must be valid according to
 *                  secp256k1_ec_seckey_verify or 32 zero bytes. For uniformly
 *                  random 32-byte tweaks, the chance of being invalid is
 *                  negligible (around 1 in 2^128).
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_pubkey_tweak_add(
    const secp256k1_context *ctx,
    secp256k1_pubkey *pubkey,
    const unsigned char *tweak32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Tweak a secret key by multiplying it by a tweak.
 *
 *  Returns: 0 if the arguments are invalid. 1 otherwise.
 *  Args:   ctx:    pointer to a context object.
 *  In/Out: seckey: pointer to a 32-byte secret key. If the secret key is
 *                  invalid according to secp256k1_ec_seckey_verify, this
 *                  function returns 0. seckey will be set to some unspecified
 *                  value if this function returns 0.
 *  In:    tweak32: pointer to a 32-byte tweak. If the tweak is invalid according to
 *                  secp256k1_ec_seckey_verify, this function returns 0. For
 *                  uniformly random 32-byte arrays the chance of being invalid
 *                  is negligible (around 1 in 2^128).
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_seckey_tweak_mul(
    const secp256k1_context *ctx,
    unsigned char *seckey,
    const unsigned char *tweak32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Same as secp256k1_ec_seckey_tweak_mul, but DEPRECATED. Will be removed in
 *  future versions. */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_privkey_tweak_mul(
    const secp256k1_context *ctx,
    unsigned char *seckey,
    const unsigned char *tweak32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3)
  SECP256K1_DEPRECATED("Use secp256k1_ec_seckey_tweak_mul instead");

/** Tweak a public key by multiplying it by a tweak value.
 *
 *  Returns: 0 if the arguments are invalid. 1 otherwise.
 *  Args:    ctx:   pointer to a context object.
 *  In/Out: pubkey: pointer to a public key object. pubkey will be set to an
 *                  invalid value if this function returns 0.
 *  In:    tweak32: pointer to a 32-byte tweak. If the tweak is invalid according to
 *                  secp256k1_ec_seckey_verify, this function returns 0. For
 *                  uniformly random 32-byte arrays the chance of being invalid
 *                  is negligible (around 1 in 2^128).
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_pubkey_tweak_mul(
    const secp256k1_context *ctx,
    secp256k1_pubkey *pubkey,
    const unsigned char *tweak32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Randomizes the context to provide enhanced protection against side-channel leakage.
 *
 *  Returns: 1: randomization successful
 *           0: error
 *  Args:    ctx:       pointer to a context object (not secp256k1_context_static).
 *  In:      seed32:    pointer to a 32-byte random seed (NULL resets to initial state).
 *
 * While secp256k1 code is written and tested to be constant-time no matter what
 * secret values are, it is possible that a compiler may output code which is not,
 * and also that the CPU may not emit the same radio frequencies or draw the same
 * amount of power for all values. Randomization of the context shields against
 * side-channel observations which aim to exploit secret-dependent behaviour in
 * certain computations which involve secret keys.
 *
 * It is highly recommended to call this function on contexts returned from
 * secp256k1_context_create or secp256k1_context_clone (or from the corresponding
 * functions in secp256k1_preallocated.h) before using these contexts to call API
 * functions that perform computations involving secret keys, e.g., signing and
 * public key generation. It is possible to call this function more than once on
 * the same context, and doing so before every few computations involving secret
 * keys is recommended as a defense-in-depth measure. Randomization of the static
 * context secp256k1_context_static is not supported.
 *
 * Currently, the random seed is mainly used for blinding multiplications of a
 * secret scalar with the elliptic curve base point. Multiplications of this
 * kind are performed by exactly those API functions which are documented to
 * require a context that is not secp256k1_context_static. As a rule of thumb,
 * these are all functions which take a secret key (or a keypair) as an input.
 * A notable exception to that rule is the ECDH module, which relies on a different
 * kind of elliptic curve point multiplication and thus does not benefit from
 * enhanced protection against side-channel leakage currently.
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_context_randomize(
    secp256k1_context *ctx,
    const unsigned char *seed32
) SECP256K1_ARG_NONNULL(1);

/** Add a number of public keys together.
 *
 *  Returns: 1: the sum of the public keys is valid.
 *           0: the sum of the public keys is not valid.
 *  Args:   ctx:        pointer to a context object.
 *  Out:    out:        pointer to a public key object for placing the resulting public key.
 *  In:     ins:        pointer to array of pointers to public keys.
 *          n:          the number of public keys to add together (must be at least 1).
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_ec_pubkey_combine(
    const secp256k1_context *ctx,
    secp256k1_pubkey *out,
    const secp256k1_pubkey * const *ins,
    size_t n
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3);

/** Compute a tagged hash as defined in BIP-340.
 *
 *  This is useful for creating a message hash and achieving domain separation
 *  through an application-specific tag. This function returns
 *  SHA256(SHA256(tag)||SHA256(tag)||msg). Therefore, tagged hash
 *  implementations optimized for a specific tag can precompute the SHA256 state
 *  after hashing the tag hashes.
 *
 *  Returns: 1 always.
 *  Args:    ctx: pointer to a context object
 *  Out:  hash32: pointer to a 32-byte array to store the resulting hash
 *  In:      tag: pointer to an array containing the tag
 *        taglen: length of the tag array
 *           msg: pointer to an array containing the message
 *        msglen: length of the message array
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_tagged_sha256(
    const secp256k1_context *ctx,
    unsigned char *hash32,
    const unsigned char *tag,
    size_t taglen,
    const unsigned char *msg,
    size_t msglen
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(5);

#ifdef __cplusplus
}
#endif

#endif /* SECP256K1_H */
