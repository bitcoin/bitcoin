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

/** Create an ECDSA signature.
 *  Returns: 1: signature created
 *           0: nonce invalid, try another one
 *  In:      msg:    the message being signed
 *           msglen: the length of the message being signed
 *           seckey: pointer to a 32-byte secret key (assumed to be valid)
 *           nonce:  pointer to a 32-byte nonce (generated with a cryptographic PRNG)
 *  Out:     sig:    pointer to a 72-byte array where the signature will be placed.
 *           siglen: pointer to an int, which will be updated to the signature length (<=72).
 */
int secp256k1_ecdsa_sign(const unsigned char *msg, int msglen,
                         unsigned char *sig, int *siglen,
                         const unsigned char *seckey,
                         const unsigned char *nonce);

/** Create a compact ECDSA signature (64 byte + recovery id).
 *  Returns: 1: signature created
 *           0: nonce invalid, try another one
 *  In:      msg:    the message being signed
 *           msglen: the length of the message being signed
 *           seckey: pointer to a 32-byte secret key (assumed to be valid)
 *           nonce:  pointer to a 32-byte nonce (generated with a cryptographic PRNG)
 *  Out:     sig:    pointer to a 64-byte array where the signature will be placed.
 *           recid:  pointer to an int, which will be updated to contain the recovery id.
 */
int secp256k1_ecdsa_sign_compact(const unsigned char *msg, int msglen,
                                 unsigned char *sig64,
                                 const unsigned char *seckey,
                                 const unsigned char *nonce,
                                 int *recid);

/** Recover an ECDSA public key from a compact signature.
 *  Returns: 1: public key succesfully recovered (which guarantees a correct signature).
 *           0: otherwise.
 *  In:      msg:        the message assumed to be signed
 *           msglen:     the length of the message
 *           sig64:      signature as 64 byte array
 *           compressed: whether to recover a compressed or uncompressed pubkey
 *           recid:      the recovery id (as returned by ecdsa_sign_compact)
 *  Out:     pubkey:     pointer to a 33 or 65 byte array to put the pubkey.
 *           pubkeylen:  pointer to an int that will contain the pubkey length.
 */

int secp256k1_ecdsa_recover_compact(const unsigned char *msg, int msglen,
                                    const unsigned char *sig64,
                                    unsigned char *pubkey, int *pubkeylen,
                                    int compressed, int recid);

/** Verify an ECDSA secret key.
 *  Returns: 1: secret key is valid
 *           0: secret key is invalid
 *  In:      seckey: pointer to a 32-byte secret key
 */
int secp256k1_ecdsa_seckey_verify(const unsigned char *seckey);

/** Just validate a public key.
 *  Returns: 1: valid public key
 *           0: invalid public key
 */
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
int secp256k1_ecdsa_pubkey_create(unsigned char *pubkey, int *pubkeylen, const unsigned char *seckey, int compressed);

int secp256k1_ecdsa_pubkey_decompress(unsigned char *pubkey, int *pubkeylen);

int secp256k1_ecdsa_privkey_export(const unsigned char *seckey,
                                   unsigned char *privkey, int *privkeylen,
                                   int compressed);

int secp256k1_ecdsa_privkey_import(unsigned char *seckey,
                                   const unsigned char *privkey, int privkeylen);

int secp256k1_ecdsa_privkey_tweak_add(unsigned char *seckey, const unsigned char *tweak);
int secp256k1_ecdsa_pubkey_tweak_add(unsigned char *pubkey, int pubkeylen, const unsigned char *tweak);
int secp256k1_ecdsa_privkey_tweak_mul(unsigned char *seckey, const unsigned char *tweak);
int secp256k1_ecdsa_pubkey_tweak_mul(unsigned char *pubkey, int pubkeylen, const unsigned char *tweak);

#ifdef __cplusplus
}
#endif

#endif
