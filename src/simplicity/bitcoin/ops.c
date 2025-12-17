#include "ops.h"

/* Compute Bitcoins's tapleaf hash from a tapleaf version and a 256-bit script value.
 * A reimplementation of ComputeTapleafHash from Bitcoin's 'interpreter.cpp'.
 * Only 256-bit script values are supported as that is the size used for Simplicity CMRs.
 *
 * Precondition: NULL != cmr;
 */
sha256_midstate simplicity_bitcoin_make_tapleaf(unsigned char version, const sha256_midstate* cmr) {
  sha256_midstate result;
  sha256_midstate tapleafTag;
  {
    static unsigned char tagName[] = "TapLeaf";
    sha256_context ctx = sha256_init(tapleafTag.s);
    sha256_uchars(&ctx, tagName, sizeof(tagName) - 1);
    sha256_finalize(&ctx);
  }
  sha256_context ctx = sha256_init(result.s);
  sha256_hash(&ctx, &tapleafTag);
  sha256_hash(&ctx, &tapleafTag);
  sha256_uchar(&ctx, version);
  sha256_uchar(&ctx, 32);
  sha256_hash(&ctx, cmr);
  sha256_finalize(&ctx);

  return result;
}

/* Compute Bitcoins's tapbrach hash from two branches.
 *
 * Precondition: NULL != a;
 *               NULL != b;
 */
sha256_midstate simplicity_bitcoin_make_tapbranch(const sha256_midstate* a, const sha256_midstate* b) {
  sha256_midstate result;
  sha256_midstate tapbranchTag;
  {
    static unsigned char tagName[] = "TapBranch";
    sha256_context ctx = sha256_init(tapbranchTag.s);
    sha256_uchars(&ctx, tagName, sizeof(tagName) - 1);
    sha256_finalize(&ctx);
  }
  sha256_context ctx = sha256_init(result.s);
  sha256_hash(&ctx, &tapbranchTag);
  sha256_hash(&ctx, &tapbranchTag);
  if (sha256_cmp_be(a, b) < 0) {
    sha256_hash(&ctx, a);
    sha256_hash(&ctx, b);
  } else {
    sha256_hash(&ctx, b);
    sha256_hash(&ctx, a);
  }
  sha256_finalize(&ctx);

  return result;
}
