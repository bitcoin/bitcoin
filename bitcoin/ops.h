/* This module defines operations used in the construction the environment ('txEnv') and some jets.
 */
#ifndef SIMPLICITY_BITCOIN_OPS_H
#define SIMPLICITY_BITCOIN_OPS_H

#include "../sha256.h"

/* Compute Bitcoin's tapleaf hash from a tapleaf version and a 256-bit script value.
 * A reimplementation of ComputeTapleafHash from Bitcoin's 'interpreter.cpp'.
 * Only 256-bit script values are supported as that is the size used for Simplicity CMRs.
 *
 * Precondition: NULL != cmr;
 */
sha256_midstate simplicity_bitcoin_make_tapleaf(unsigned char version, const sha256_midstate* cmr);

/* Compute an Bitcoin's tapbrach hash from two branches.
 *
 * Precondition: NULL != a;
 *               NULL != b;
 */
sha256_midstate simplicity_bitcoin_make_tapbranch(const sha256_midstate* a, const sha256_midstate* b);

#endif
