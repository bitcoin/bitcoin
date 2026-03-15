#ifndef SECP256K1_P2SKH_H
#define SECP256K1_P2SKH_H

#include "secp256k1.h"
#include "secp256k1_extrakeys.h"

#ifdef __cplusplus
extern "C" {
#endif

/** P2SKH (Pay-to-Schnorr-Key-Hash) Schnorr-based key-path signature scheme.
 *
 * Locking script: OP_2 <hash160(P.x)>  (20-byte program)
 *
 * Signing:
 *   R = k*G  (negate k if R.y is odd)
 *   e = TaggedHash("P2SKH/challenge", R.x || hash160(P.x) || msg32)
 *   S = k + e*d  (mod n)
 *   sig = R.x || S
 *
 * Verification (key recovery):
 *   e = TaggedHash("P2SKH/challenge", R.x || pubkey_hash20 || msg32)
 *   P = (S*G - R) * e^-1
 *   accept iff hash160(P.x) == pubkey_hash20
 */

/** Sign a 32-byte message hash for P2SKH key-path spending.
 *
 *  Returns: 1 on success, 0 on failure.
 *  Args:    ctx:            pointer to a context object (must have signing capability).
 *  Out:     sig64:          pointer to a 64-byte array to place the signature in.
 *  In:      msg32:          the 32-byte message being signed.
 *           keypair:        pointer to an initialized keypair.
 *           pubkey_hash20:  the 20-byte hash160(P.x) of the public key, as encoded in
 *                           the locking script. The caller is responsible for computing
 *                           this (e.g. RIPEMD160(SHA256(P.x))). Must not be NULL.
 *           aux_rand32:     32 bytes of fresh randomness. While recommended, passing
 *                           NULL is accepted and will make signing deterministic.
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_p2skh_sign(
    const secp256k1_context *ctx,
    unsigned char *sig64,
    const unsigned char *msg32,
    const secp256k1_keypair *keypair,
    const unsigned char *pubkey_hash20,
    const unsigned char *aux_rand32
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4) SECP256K1_ARG_NONNULL(5);

/** Recover the public-key x-coordinate from a P2SKH signature.
 *
 *  Returns: 1 if the signature is geometrically valid (R.x in range, S non-zero,
 *           recovered P not at infinity), 0 otherwise.
 *  Args:    ctx:             pointer to a context object.
 *  Out:     out_px32:        32-byte buffer to receive the recovered P.x coordinate.
 *  In:      sig64:           pointer to the 64-byte signature (R.x || S).
 *           msg32:           the 32-byte message that was signed.
 *           pubkey_hash20:   the 20-byte hash160(P.x) from the locking script (used
 *                            to reconstruct the challenge scalar e).
 *
 *  Note: A return value of 1 does NOT mean the signature is valid.  The caller
 *  MUST additionally verify that hash160(out_px32) equals pubkey_hash20.
 */
SECP256K1_API SECP256K1_WARN_UNUSED_RESULT int secp256k1_p2skh_verify(
    const secp256k1_context *ctx,
    unsigned char *out_px32,
    const unsigned char *sig64,
    const unsigned char *msg32,
    const unsigned char *pubkey_hash20
) SECP256K1_ARG_NONNULL(1) SECP256K1_ARG_NONNULL(2) SECP256K1_ARG_NONNULL(3) SECP256K1_ARG_NONNULL(4) SECP256K1_ARG_NONNULL(5);

#ifdef __cplusplus
}
#endif

#endif /* SECP256K1_P2SKH_H */
