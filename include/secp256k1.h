#ifndef _SECP256K1_
#define _SECP256K1_

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize the library. This may take some time (10-100 ms).
 *  You need to call this before calling any other function.
 *  It cannot run in parallel with any other functions, but once
 *  secp256k1_start() returns, all other functions are thread-safe.
 */
void secp256k1_start(void);

/** Free all memory associated with this library. After this, no
 *  functions can be called anymore, except secp256k1_start()
 */
void secp256k1_stop(void);

/** Verify an ECDSA signature.
 *  Returns: 1: correct signature
 *           0: incorrect signature
 *          -1: invalid public key
 *          -2: invalid signature
 */
int secp256k1_ecdsa_verify(const unsigned char *msg, int msglen,
                           const unsigned char *sig, int siglen,
                           const unsigned char *pubkey, int pubkeylen);

int secp256k1_ecdsa_sign(const unsigned char *msg, int msglen,
                         unsigned char *sig, int* siglen,
                         const unsigned char *seckey,
                         const unsigned char *nonce);

int secp256j1_ecdsa_seckey_verify(const unsigned char *seckey);

/** Just validate a public key.
 *  Returns: 1: valid public key
 *           0: invalid public key
 */
/* NOT YET IMPLEMENTED */
int secp256k1_ecdsa_pubkey_verify(const unsigned char *pubkey, int pubkeylen);

/** Compute the public key for a secret key.
 *  In:     compressed: whether the computed public key should be compressed
 *          seckey:     pointer to a 32-byte private key.
 *  Out:    pubkey:     pointer to a 33-byte (if compressed) or 65-byte (if uncompressed)
 *                      area to store the public key.
 *          pubkeylen:  pointer to int that will be updated to contains the pubkey's
 *                      length.
 *  Returns: 1: secret was valid, public key stores
 *           0: secret was invalid, try again.
 */
/* NOT YET IMPLEMENTED */
int secp256k1_ecdsa_pubkey_create(unsigned char *pubkey, int *pubkeylen, const unsigned char *seckey, int compressed)

int secp256k1_ecdsa_pubkey_decompress(unsigned char *pubkey, int *pubkeylen);

#ifdef __cplusplus
}
#endif

#endif
