#include "txEnv.h"

/* Construct a txEnv structure from its components.
 * This function will precompute any cached values.
 *
 * Precondition: NULL != tx
 *               NULL != taproot
 *               NULL != genesisHash
 *               ix < tx->numInputs
 */
txEnv simplicity_elements_build_txEnv(const elementsTransaction* tx, const elementsTapEnv* taproot, const sha256_midstate* genesisHash, uint_fast32_t ix) {
  txEnv result = { .tx = tx
                 , .taproot = taproot
                 , .genesisHash = *genesisHash
                 , .ix = ix
                 };
  sha256_context ctx = sha256_init(result.sigAllHash.s);
  sha256_hash(&ctx, genesisHash);
  sha256_hash(&ctx, genesisHash);
  sha256_hash(&ctx, &tx->txHash);
  sha256_hash(&ctx, &taproot->tapEnvHash);
  sha256_u32be(&ctx, ix);
  sha256_finalize(&ctx);

  return result;
}
