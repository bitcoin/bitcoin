/* This module defines operations used in the construction the environment ('txEnv') and some jets.
 */
#ifndef SIMPLICITY_ELEMENTS_OPS_H
#define SIMPLICITY_ELEMENTS_OPS_H

#include "../sha256.h"
#include "txEnv.h"

/* Add an 'confidential' value to be consumed by an ongoing SHA-256 evaluation.
 * If the 'confidential' value is blinded, then the 'evenPrefix' used if the y coordinate is even,
 * and the 'oddPrefix' is used if the y coordinate is odd.
 * If the 'confidential' value is explicit, then '0x01' is used as the prefix.
 * If the 'confidential' value is "NULL" then only '0x00' added.
 *
 * Precondition: NULL != ctx;
 *               NULL != conf;
 */
void simplicity_sha256_confidential(unsigned char evenPrefix, unsigned char oddPrefix, sha256_context* ctx, const confidential* conf);

/* Add an 'confidential' asset to be consumed by an ongoing SHA-256 evaluation.
 *
 * Precondition: NULL != ctx;
 *               NULL != asset;
 */
void simplicity_sha256_confAsset(sha256_context* ctx, const confidential* asset);

/* Add an 'confidential' nonce to be consumed by an ongoing SHA-256 evaluation.
 *
 * Precondition: NULL != ctx;
 *               NULL != nonce;
 */
void simplicity_sha256_confNonce(sha256_context* ctx, const confidential* nonce);

/* Add an 'confidential' amount to be consumed by an ongoing SHA-256 evaluation.
 *
 * Precondition: NULL != ctx;
 *               NULL != amt;
 */
void simplicity_sha256_confAmt(sha256_context* ctx, const confAmount* amt);

/* Compute an Element's entropy value from a prevoutpoint and a contract hash.
 * A reimplementation of GenerateAssetEntropy from Element's 'issuance.cpp'.
 *
 * Precondition: NULL != op;
 *               NULL != contract;
 */
sha256_midstate simplicity_generateIssuanceEntropy(const outpoint* op, const sha256_midstate* contract);

/* Compute an Element's issuance Asset ID value from an entropy value.
 * A reimplementation of CalculateAsset from Element's 'issuance.cpp'.
 *
 * Precondition: NULL != entropy;
 */
sha256_midstate simplicity_calculateAsset(const sha256_midstate* entropy);

/* Compute an Element's issuance Token ID value from an entropy value and an amount prefix.
 * A reimplementation of CalculateReissuanceToken from Element's 'issuance.cpp'.
 *
 * Precondition: NULL != entropy;
 */
sha256_midstate simplicity_calculateToken(const sha256_midstate* entropy, confPrefix prefix);

/* Compute an Element's tapleaf hash from a tapleaf version and a 256-bit script value.
 * A reimplementation of ComputeTapleafHash from Element's 'interpreter.cpp'.
 * Only 256-bit script values are supported as that is the size used for Simplicity CMRs.
 *
 * Precondition: NULL != cmr;
 */
sha256_midstate simplicity_make_tapleaf(unsigned char version, const sha256_midstate* cmr);

/* Compute an Element's tapbrach hash from two branches.
 *
 * Precondition: NULL != a;
 *               NULL != b;
 */
sha256_midstate simplicity_make_tapbranch(const sha256_midstate* a, const sha256_midstate* b);

#endif
