#ifndef _SECP256K1_ECDH_
# define _SECP256K1_ECDH_

# include "secp256k1.h"

# ifdef __cplusplus
extern "C" {
# endif

/** Compute an EC Diffie-Hellman secret in constant time
 *  Returns: 1: exponentiation was successful
 *           0: scalar was invalid (zero or overflow)
 *  In:      ctx:      pointer to a context object (cannot be NULL)
 *           point:    pointer to a public point
 *           scalar:   a 32-byte scalar with which to multiply the point
 *  Out:     result:   a 32-byte array which will be populated by an ECDH
 *                     secret computed from the point and scalar
 */
SECP256K1_WARN_UNUSED_RESULT int secp256k1_ecdh(
  const secp256k1_context_t* ctx,
  unsigned char *result,
  const secp256k1_pubkey_t *point,
  const unsigned char *scalar
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4);

# ifdef __cplusplus
}
# endif

#endif
