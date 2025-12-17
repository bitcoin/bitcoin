#include "elementsJets.h"

#include "ops.h"
#include "txEnv.h"
#include "../taptweak.h"
#include "../simplicity_assert.h"

/* Read a 256-bit hash value from the 'src' frame, advancing the cursor 256 cells.
 *
 * Precondition: '*src' is a valid read frame for 256 more cells;
 *               NULL != h;
 */
static void readHash(sha256_midstate* h, frameItem *src) {
  read32s(h->s, 8, src);
}

/* Write a 256-bit hash value to the 'dst' frame, advancing the cursor 256 cells.
 *
 * Precondition: '*dst' is a valid write frame for 256 more cells;
 *               NULL != h;
 */
static void writeHash(frameItem* dst, const sha256_midstate* h) {
  write32s(dst, h->s, 8);
}

/* Write an outpoint value to the 'dst' frame, advancing the cursor 288 cells.
 *
 * Precondition: '*dst' is a valid write frame for 288 more cells;
 *               NULL != op;
 */
static void prevOutpoint(frameItem* dst, const outpoint* op) {
  writeHash(dst, &op->txid);
  simplicity_write32(dst, op->ix);
}

/* Write an confidential asset to the 'dst' frame, advancing the cursor 258 cells.
 *
 * Precondition: '*dst' is a valid write frame for 258 more cells;
 *               NULL != asset;
 */
static void asset(frameItem* dst, const confidential* asset) {
  if (writeBit(dst, EXPLICIT == asset->prefix)) {
    skipBits(dst, 1);
  } else {
    writeBit(dst, ODD_Y == asset->prefix);
  }
  writeHash(dst, &asset->data);
}

/* Write an confidential amount to the 'dst' frame, advancing the cursor 258 cells.
 *
 * Precondition: '*dst' is a valid write frame for 258 more cells;
 *               NULL != amt;
 */
static void amt(frameItem* dst, const confAmount* amt) {
  if (writeBit(dst, EXPLICIT == amt->prefix)) {
    skipBits(dst, 1 + 256 - 64);
    simplicity_write64(dst, amt->explicit);
  } else {
    writeBit(dst, ODD_Y == amt->prefix);
    writeHash(dst, &amt->confidential);
  }
}

/* Write an optional confidential nonce to the 'dst' frame, advancing the cursor 259 cells.
 *
 * Precondition: '*dst' is a valid write frame for 259 more cells;
 *               NULL != nonce;
 */
static void nonce(frameItem* dst, const confidential* nonce) {
  if (writeBit(dst, NONE != nonce->prefix)) {
    if (writeBit(dst, EXPLICIT == nonce->prefix)) {
      skipBits(dst, 1);
    } else {
      writeBit(dst, ODD_Y == nonce->prefix);
    }
    writeHash(dst, &nonce->data);
  } else {
    skipBits(dst, 1+1+256);
  }
}

/* Write an optional 'blindingNonce' from an 'assetIssuance' to the 'dst' frame, advancing the cursor 257 cells.
 *
 * Precondition: '*dst' is a valid write frame for 257 more cells;
 *               NULL != issuance;
 */
static void reissuanceBlinding(frameItem* dst, const assetIssuance* issuance) {
  if (writeBit(dst, REISSUANCE == issuance->type)) {
    writeHash(dst, &issuance->blindingNonce);
  } else {
    skipBits(dst, 256);
  }
}

/* Write an optional 'contractHash' from an 'assetIssuance' to the 'dst' frame, advancing the cursor 257 cells.
 *
 * Precondition: '*dst' is a valid write frame for 257 more cells;
 *               NULL != issuance;
 */
static void newIssuanceContract(frameItem* dst, const assetIssuance* issuance) {
  if (writeBit(dst, NEW_ISSUANCE == issuance->type)) {
    writeHash(dst, &issuance->contractHash);
  } else {
    skipBits(dst, 256);
  }
}

/* Write an optional 'entropy' from an 'assetIssuance' to the 'dst' frame, advancing the cursor 257 cells.
 *
 * Precondition: '*dst' is a valid write frame for 257 more cells;
 *               NULL != issuance;
 */
static void reissuanceEntropy(frameItem* dst, const assetIssuance* issuance) {
  if (writeBit(dst, REISSUANCE == issuance->type)) {
    writeHash(dst, &issuance->entropy);
  } else {
    skipBits(dst, 256);
  }
}

/* Write an optional confidential asset amount from an 'assetIssuance' to the 'dst' frame, advancing the cursor 259 cells.
 *
 * Precondition: '*dst' is a valid write frame for 259 more cells;
 *               NULL != issuance;
 */
static void issuanceAssetAmt(frameItem* dst, const assetIssuance* issuance) {
  if (writeBit(dst, NO_ISSUANCE != issuance->type)) {
    amt(dst, &issuance->assetAmt);
  } else {
    skipBits(dst, 258);
  }
}

/* Write an optional confidential token amount from an 'assetIssuance' to the 'dst' frame, advancing the cursor.
 *
 * Precondition: '*dst' is a valid write frame for 259 more cells;
 *               NULL != issuance;
 */
static void issuanceTokenAmt(frameItem* dst, const assetIssuance* issuance) {
  if (writeBit(dst, NO_ISSUANCE != issuance->type)) {
    amt(dst, NEW_ISSUANCE == issuance->type ? &issuance->tokenAmt : &(confAmount){ .prefix = EXPLICIT, .explicit = 0});
  } else {
    skipBits(dst, 258);
  }
}

static uint_fast32_t lockHeight(const elementsTransaction* tx) {
  return !tx->isFinal && tx->lockTime < 500000000U ? tx->lockTime : 0;
}

static uint_fast32_t lockTime(const elementsTransaction* tx) {
  return !tx->isFinal && 500000000U <= tx->lockTime ? tx->lockTime : 0;
}

static uint_fast16_t lockDistance(const elementsTransaction* tx) {
  return 2 <= tx->version ? tx->lockDistance : 0;
}

static uint_fast16_t lockDuration(const elementsTransaction* tx) {
  return 2 <= tx->version ? tx->lockDuration : 0;
}

static bool isFee(const sigOutput* output) {
  /* As specified in https://github.com/ElementsProject/elements/blob/de942511a67c3a3fcbdf002a8ee7e9ba49679b78/src/primitives/transaction.h#L304-L307. */
  return output->emptyScript && EXPLICIT == output->asset.prefix && EXPLICIT == output->amt.prefix;
}

/* Lookup the assetFee from a sorted array of feeOutputs by the given assetid, returning 0 if no entry is found.
 *
 * Precondition: NULL != assetid;
 *               feeOutputs is uniquely sorted by it asset.data.s field, which is to say
 *               for all 0 <= i < j < len,
 *                 0 < memcmp(feeOutputs[j]->asset.data.s, feeOutputs[i]->asset.data.s, sizeof(feeOutputs[i]->asset.data.s));
 */
static uint_fast64_t lookup_fee(const sha256_midstate* assetid, const sigOutput* const * feeOutputs, uint_fast32_t len) {
  /* This loop runs in O(log(len)) time. */
  while(len) {
    int cmp = memcmp(assetid->s, feeOutputs[len/2]->asset.data.s, sizeof(assetid->s));
    if (0 == cmp) return feeOutputs[len/2]->assetFee;
    if (0 < cmp) {
      feeOutputs += len/2 + 1;
      len -= len/2 + 1;
    } else {
      len /= 2;
    }
  }
  return 0;
}

/* version : ONE |- TWO^32 */
bool simplicity_version(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  simplicity_write32(dst, env->tx->version);
  return true;
}

/* lock_time : ONE |- TWO^32 */
bool simplicity_lock_time(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  simplicity_write32(dst, env->tx->lockTime);
  return true;
}

/* input_pegin : TWO^32 |- S (S TWO^256) */
bool simplicity_input_pegin(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numInputs)) {
    if (writeBit(dst, env->tx->input[i].isPegin)) {
      writeHash(dst, &env->tx->input[i].pegin);
    } else {
      skipBits(dst, 256);
    }
  } else {
    skipBits(dst, 257);
  }
  return true;
}

/* input_prev_outpoint : TWO^32 |- S (TWO^256 * TWO^32) */
bool simplicity_input_prev_outpoint(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numInputs)) {
    prevOutpoint(dst, &env->tx->input[i].prevOutpoint);
  } else {
    skipBits(dst, 288);
  }
  return true;
}

/* input_asset : TWO^32 |- S (Conf TWO^256) */
bool simplicity_input_asset(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numInputs)) {
    asset(dst, &env->tx->input[i].txo.asset);
  } else {
    skipBits(dst, 258);
  }
  return true;
}

/* input_amount : TWO^32 |- S (Conf TWO^256, Conf TWO^64) */
bool simplicity_input_amount(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numInputs)) {
    asset(dst, &env->tx->input[i].txo.asset);
    amt(dst, &env->tx->input[i].txo.amt);
  } else {
    skipBits(dst, 516);
  }
  return true;
}

/* input_script_hash : TWO^32 |- S TWO^256 */
bool simplicity_input_script_hash(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numInputs)) {
    writeHash(dst, &env->tx->input[i].txo.scriptPubKey);
  } else {
    skipBits(dst, 256);
  }
  return true;
}

/* input_sequence : TWO^32 |- S TWO^32 */
bool simplicity_input_sequence(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numInputs)) {
    simplicity_write32(dst, env->tx->input[i].sequence);
  } else {
    skipBits(dst, 32);
  }
  return true;
}

/* reissuance_blinding : TWO^32 |- S (S TWO^256) */
bool simplicity_reissuance_blinding(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numInputs)) {
    reissuanceBlinding(dst, &env->tx->input[i].issuance);
  } else {
    skipBits(dst, 257);
  }
  return true;
}

/* new_issuance_contract : TWO^32 |- S (S TWO^256) */
bool simplicity_new_issuance_contract(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numInputs)) {
    newIssuanceContract(dst, &env->tx->input[i].issuance);
  } else {
    skipBits(dst, 257);
  }
  return true;
}

/* reissuance_entropy : TWO^32 |- S (S TWO^256) */
bool simplicity_reissuance_entropy(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numInputs)) {
    reissuanceEntropy(dst, &env->tx->input[i].issuance);
  } else {
    skipBits(dst, 257);
  }
  return true;
}

/* issuance_asset_amount : TWO^32 |- S (S (Conf TWO^64)) */
bool simplicity_issuance_asset_amount(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numInputs)) {
    issuanceAssetAmt(dst, &env->tx->input[i].issuance);
  } else {
    skipBits(dst, 259);
  }
  return true;
}

/* issuance_token_amount : TWO^32 |- S (S (Conf TWO^64)) */
bool simplicity_issuance_token_amount(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numInputs)) {
    issuanceTokenAmt(dst, &env->tx->input[i].issuance);
  } else {
    skipBits(dst, 259);
  }
  return true;
}

/* issuance_asset_proof : TWO^32 |- S TWO^256 */
bool simplicity_issuance_asset_proof(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numInputs)) {
    writeHash(dst, &env->tx->input[i].issuance.assetRangeProofHash);
  } else {
    skipBits(dst, 256);
  }
  return true;
}

/* issuance_token_proof : TWO^32 |- S TWO^256 */
bool simplicity_issuance_token_proof(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numInputs)) {
    writeHash(dst, &env->tx->input[i].issuance.tokenRangeProofHash);
  } else {
    skipBits(dst, 256);
  }
  return true;
}

/* input_annex_hash : TWO^32 |- S (S (TWO^256)) */
bool simplicity_input_annex_hash(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numInputs)) {
    if (writeBit(dst, env->tx->input[i].hasAnnex)) {
      writeHash(dst, &env->tx->input[i].annexHash);
    } else {
      skipBits(dst, 256);
    }
  } else {
    skipBits(dst, 257);
  }
  return true;
}

/* input_script_sig_hash : TWO^32 |- (S (TWO^256) */
bool simplicity_input_script_sig_hash(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numInputs)) {
    writeHash(dst, &env->tx->input[i].scriptSigHash);
  } else {
    skipBits(dst, 256);
  }
  return true;
}

/* output_asset : TWO^32 |- S (Conf TWO^256) */
bool simplicity_output_asset(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numOutputs)) {
    asset(dst, &env->tx->output[i].asset);
  } else {
    skipBits(dst, 258);
  }
  return true;
}

/* output_amount : TWO^32 |- S (Conf TWO^256, Conf TWO^64) */
bool simplicity_output_amount(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numOutputs)) {
    asset(dst, &env->tx->output[i].asset);
    amt(dst, &env->tx->output[i].amt);
  } else {
    skipBits(dst, 516);
  }
  return true;
}

/* output_nonce : TWO^32 |- S (S (Conf TWO^256)) */
bool simplicity_output_nonce(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numOutputs)) {
    nonce(dst, &env->tx->output[i].nonce);
  } else {
    skipBits(dst, 259);
  }
  return true;
}

/* output_script_hash : TWO^32 |- S TWO^256 */
bool simplicity_output_script_hash(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numOutputs)) {
    writeHash(dst, &env->tx->output[i].scriptPubKey);
  } else {
    skipBits(dst, 256);
  }
  return true;
}

/* output_null_datum : TWO^32 * TWO^32 |- S (S (TWO^2 * TWO^256 + (TWO + TWO^4)))  */
bool simplicity_output_null_datum(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numOutputs && env->tx->output[i].isNullData)) {
    uint_fast32_t j = simplicity_read32(&src);
    if (writeBit(dst, j < env->tx->output[i].pnd.len)) {
      if (writeBit(dst, OP_PUSHDATA4 < env->tx->output[i].pnd.op[j].code)) {
        skipBits(dst, 2 + 256 - 5);
        if (writeBit(dst, OP_1 <= env->tx->output[i].pnd.op[j].code)) {
          switch (env->tx->output[i].pnd.op[j].code) {
            case OP_1 : writeBit(dst, 0); writeBit(dst, 0); writeBit(dst, 0); writeBit(dst, 0); break;
            case OP_2 : writeBit(dst, 0); writeBit(dst, 0); writeBit(dst, 0); writeBit(dst, 1); break;
            case OP_3 : writeBit(dst, 0); writeBit(dst, 0); writeBit(dst, 1); writeBit(dst, 0); break;
            case OP_4 : writeBit(dst, 0); writeBit(dst, 0); writeBit(dst, 1); writeBit(dst, 1); break;
            case OP_5 : writeBit(dst, 0); writeBit(dst, 1); writeBit(dst, 0); writeBit(dst, 0); break;
            case OP_6 : writeBit(dst, 0); writeBit(dst, 1); writeBit(dst, 0); writeBit(dst, 1); break;
            case OP_7 : writeBit(dst, 0); writeBit(dst, 1); writeBit(dst, 1); writeBit(dst, 0); break;
            case OP_8 : writeBit(dst, 0); writeBit(dst, 1); writeBit(dst, 1); writeBit(dst, 1); break;
            case OP_9 : writeBit(dst, 1); writeBit(dst, 0); writeBit(dst, 0); writeBit(dst, 0); break;
            case OP_10: writeBit(dst, 1); writeBit(dst, 0); writeBit(dst, 0); writeBit(dst, 1); break;
            case OP_11: writeBit(dst, 1); writeBit(dst, 0); writeBit(dst, 1); writeBit(dst, 0); break;
            case OP_12: writeBit(dst, 1); writeBit(dst, 0); writeBit(dst, 1); writeBit(dst, 1); break;
            case OP_13: writeBit(dst, 1); writeBit(dst, 1); writeBit(dst, 0); writeBit(dst, 0); break;
            case OP_14: writeBit(dst, 1); writeBit(dst, 1); writeBit(dst, 0); writeBit(dst, 1); break;
            case OP_15: writeBit(dst, 1); writeBit(dst, 1); writeBit(dst, 1); writeBit(dst, 0); break;
            case OP_16: writeBit(dst, 1); writeBit(dst, 1); writeBit(dst, 1); writeBit(dst, 1); break;
            default: SIMPLICITY_UNREACHABLE;
          }
        } else {
          simplicity_debug_assert(OP_RESERVED == env->tx->output[i].pnd.op[j].code ||
                 OP_1NEGATE == env->tx->output[i].pnd.op[j].code);
          skipBits(dst, 3);
          writeBit(dst, OP_RESERVED == env->tx->output[i].pnd.op[j].code);
        }
      } else {
        switch (env->tx->output[i].pnd.op[j].code) {
          case OP_IMMEDIATE: writeBit(dst, 0); writeBit(dst, 0); break;
          case OP_PUSHDATA: writeBit(dst, 0); writeBit(dst, 1); break;
          case OP_PUSHDATA2: writeBit(dst, 1); writeBit(dst, 0); break;
          case OP_PUSHDATA4: writeBit(dst, 1); writeBit(dst, 1); break;
          default: SIMPLICITY_UNREACHABLE;
        }
        writeHash(dst, &env->tx->output[i].pnd.op[j].dataHash);
      }
    } else {
      skipBits(dst, 1 + 2 + 256);
    }
  } else {
    skipBits(dst, 1 + 1 + 2 + 256);
  }
  return true;
}

/* output_is_fee : TWO^32 |- S TWO */
bool simplicity_output_is_fee(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numOutputs)) {
    writeBit(dst, isFee(&env->tx->output[i]));
  } else {
    skipBits(dst, 1);
  }
  return true;
}

/* output_surjection_proof : TWO^32 |- S TWO^256 */
bool simplicity_output_surjection_proof(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numOutputs)) {
    writeHash(dst, &env->tx->output[i].surjectionProofHash);
  } else {
    skipBits(dst, 256);
  }
  return true;
}

/* output_range_proof : TWO^32 |- S TWO^256 */
bool simplicity_output_range_proof(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numOutputs)) {
    writeHash(dst, &env->tx->output[i].rangeProofHash);
  } else {
    skipBits(dst, 256);
  }
  return true;
}

/* total_fee : TWO^256 |- TWO^64 */
bool simplicity_total_fee(frameItem* dst, frameItem src, const txEnv* env) {
  sha256_midstate assetid;
  readHash(&assetid, &src);
  simplicity_write64(dst, lookup_fee(&assetid, env->tx->feeOutputs, env->tx->numFees));
  return true;
}

/* genesis_block_hash : ONE |- TWO^256 */
bool simplicity_genesis_block_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  write32s(dst, env->genesisHash.s, 8);
  return true;
}

/* script_cmr : ONE |- TWO^256 */
bool simplicity_script_cmr(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  write32s(dst, env->taproot->scriptCMR.s, 8);
  return true;
}

/* transaction_id : ONE |- TWO^256 */
bool simplicity_transaction_id(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  write32s(dst, env->tx->txid.s, 8);
  return true;
}

/* current_index : ONE |- TWO^32 */
bool simplicity_current_index(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  simplicity_write32(dst, env->ix);
  return true;
}

/* current_pegin : ONE |- S TWO^256 */
bool simplicity_current_pegin(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  if (env->tx->numInputs <= env->ix) return false;
  if (writeBit(dst, env->tx->input[env->ix].isPegin)) {
    writeHash(dst, &env->tx->input[env->ix].pegin);
  } else {
    skipBits(dst, 256);
  }
  return true;
}

/* current_prev_outpoint : ONE |- TWO^256 * TWO^32 */
bool simplicity_current_prev_outpoint(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  if (env->tx->numInputs <= env->ix) return false;
  prevOutpoint(dst, &env->tx->input[env->ix].prevOutpoint);
  return true;
}

/* current_asset : ONE |- Conf TWO^256 */
bool simplicity_current_asset(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  if (env->tx->numInputs <= env->ix) return false;
  asset(dst, &env->tx->input[env->ix].txo.asset);
  return true;
}

/* current_amount : ONE |- (Conf TWO^256, Conf TWO^64) */
bool simplicity_current_amount(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  if (env->tx->numInputs <= env->ix) return false;
  asset(dst, &env->tx->input[env->ix].txo.asset);
  amt(dst, &env->tx->input[env->ix].txo.amt);
  return true;
}

/* current_script_hash : ONE |- TWO^256 */
bool simplicity_current_script_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  if (env->tx->numInputs <= env->ix) return false;
  writeHash(dst, &env->tx->input[env->ix].txo.scriptPubKey);
  return true;
}

/* current_sequence : ONE |- TWO^32 */
bool simplicity_current_sequence(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  if (env->tx->numInputs <= env->ix) return false;
  simplicity_write32(dst, env->tx->input[env->ix].sequence);
  return true;
}

/* current_reissuance_blinding : ONE |- S (Conf TWO^256) */
bool simplicity_current_reissuance_blinding(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  if (env->tx->numInputs <= env->ix) return false;
  reissuanceBlinding(dst, &env->tx->input[env->ix].issuance);
  return true;
}

/* current_new_issuance_contract : ONE |- S (Conf TWO^256) */
bool simplicity_current_new_issuance_contract(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  if (env->tx->numInputs <= env->ix) return false;
  newIssuanceContract(dst, &env->tx->input[env->ix].issuance);
  return true;
}

/* current_reissuance_entropy : ONE |- S (Conf TWO^256) */
bool simplicity_current_reissuance_entropy(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  if (env->tx->numInputs <= env->ix) return false;
  reissuanceEntropy(dst, &env->tx->input[env->ix].issuance);
  return true;
}

/* current_issuance_asset_amount : ONE |- S (Conf TWO^64) */
bool simplicity_current_issuance_asset_amount(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  if (env->tx->numInputs <= env->ix) return false;
  issuanceAssetAmt(dst, &env->tx->input[env->ix].issuance);
  return true;
}

/* current_issuance_token_amount : ONE |- S (Conf TWO^64) */
bool simplicity_current_issuance_token_amount(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  if (env->tx->numInputs <= env->ix) return false;
  issuanceTokenAmt(dst, &env->tx->input[env->ix].issuance);
  return true;
}

/* current_issuance_asset_proof : ONE |- TWO^256 */
bool simplicity_current_issuance_asset_proof(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  if (env->tx->numInputs <= env->ix) return false;
  writeHash(dst, &env->tx->input[env->ix].issuance.assetRangeProofHash);
  return true;
}

/* current_issuance_token_proof : ONE |- TWO^256 */
bool simplicity_current_issuance_token_proof(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  if (env->tx->numInputs <= env->ix) return false;
  writeHash(dst, &env->tx->input[env->ix].issuance.tokenRangeProofHash);
  return true;
}

/* current_script_sig_hash : ONE |- TWO^256 */
bool simplicity_current_script_sig_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  if (env->tx->numInputs <= env->ix) return false;
  writeHash(dst, &env->tx->input[env->ix].scriptSigHash);
  return true;
}

/* current_annex_hash : ONE |- S (TWO^256) */
bool simplicity_current_annex_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  if (env->tx->numInputs <= env->ix) return false;
  if (writeBit(dst, env->tx->input[env->ix].hasAnnex)) {
    writeHash(dst, &env->tx->input[env->ix].annexHash);
  } else {
    skipBits(dst, 256);
  }
  return true;
}

/* tapleaf_version : ONE |- TWO^8 */
bool simplicity_tapleaf_version(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  simplicity_write8(dst, env->taproot->leafVersion);
  return true;
}

/* tappath : TWO^8 |- S (TWO^256) */
bool simplicity_tappath(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast8_t i = simplicity_read8(&src);
  if (writeBit(dst, i < env->taproot->pathLen)) {
    writeHash(dst, &env->taproot->path[i]);
  } else {
    skipBits(dst, 256);
  }
  return true;
}

/* internal_key : ONE |- TWO^256 */
bool simplicity_internal_key(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->taproot->internalKey);
  return true;
}

/* num_inputs : ONE |- TWO^32 */
bool simplicity_num_inputs(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  simplicity_write32(dst, env->tx->numInputs);
  return true;
}

/* num_outputs : ONE |- TWO^32 */
bool simplicity_num_outputs(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  simplicity_write32(dst, env->tx->numOutputs);
  return true;
}

/* tx_is_final : ONE |- TWO */
bool simplicity_tx_is_final(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeBit(dst, env->tx->isFinal);
  return true;
}

/* tx_lock_height : ONE |- TWO^32 */
bool simplicity_tx_lock_height(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  simplicity_write32(dst, lockHeight(env->tx));
  return true;
}

/* tx_lock_time : ONE |- TWO^32 */
bool simplicity_tx_lock_time(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  simplicity_write32(dst, lockTime(env->tx));
  return true;
}

/* tx_lock_distance : ONE |- TWO^16 */
bool simplicity_tx_lock_distance(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  simplicity_write16(dst, lockDistance(env->tx));
  return true;
}

/* tx_lock_duration : ONE |- TWO^16 */
bool simplicity_tx_lock_duration(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  simplicity_write16(dst, lockDuration(env->tx));
  return true;
}

/* check_lock_height : TWO^32 |- ONE */
bool simplicity_check_lock_height(frameItem* dst, frameItem src, const txEnv* env) {
  (void) dst; // dst is unused;
  uint_fast32_t x = simplicity_read32(&src);
  return x <= lockHeight(env->tx);
}

/* check_lock_time : TWO^32 |- ONE */
bool simplicity_check_lock_time(frameItem* dst, frameItem src, const txEnv* env) {
  (void) dst; // dst is unused;
  uint_fast32_t x = simplicity_read32(&src);
  return x <= lockTime(env->tx);
}

/* check_lock_distance : TWO^16 |- ONE */
bool simplicity_check_lock_distance(frameItem* dst, frameItem src, const txEnv* env) {
  (void) dst; // dst is unused;
  uint_fast16_t x = simplicity_read16(&src);
  return x <= lockDistance(env->tx);
}

/* check_lock_duration : TWO^16 |- ONE */
bool simplicity_check_lock_duration(frameItem* dst, frameItem src, const txEnv* env) {
  (void) dst; // dst is unused;
  uint_fast16_t x = simplicity_read16(&src);
  return x <= lockDuration(env->tx);
}

/* calculate_issuance_entropy : TWO^256 * TWO^32 * TWO^256 |- TWO^256 */
bool simplicity_calculate_issuance_entropy(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; // env is unused.
  outpoint op;
  sha256_midstate contract;
  sha256_midstate result;

  read32s(op.txid.s, 8, &src);
  op.ix = simplicity_read32(&src);
  read32s(contract.s, 8, &src);

  result = simplicity_generateIssuanceEntropy(&op, &contract);
  writeHash(dst, &result);
  return true;
}

/* calculate_asset : TWO^256 |- TWO^256 */
bool simplicity_calculate_asset(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; // env is unused.
  sha256_midstate entropy;
  sha256_midstate result;

  read32s(entropy.s, 8, &src);
  result = simplicity_calculateAsset(&entropy);

  writeHash(dst, &result);
  return true;
}

/* calculate_explicit_token : TWO^256 |- TWO^256 */
bool simplicity_calculate_explicit_token(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; // env is unused.
  sha256_midstate entropy;
  sha256_midstate result;

  read32s(entropy.s, 8, &src);
  result = simplicity_calculateToken(&entropy, EXPLICIT);

  writeHash(dst, &result);
  return true;
}

/* calculate_confidential_token : TWO^256 |- TWO^256 */
bool simplicity_calculate_confidential_token(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; // env is unused.
  sha256_midstate entropy;
  sha256_midstate result;

  read32s(entropy.s, 8, &src);
  result = simplicity_calculateToken(&entropy, EVEN_Y /* ODD_Y would also work. */);

  writeHash(dst, &result);
  return true;
}

/* lbtc_asset : ONE |- TWO^256 */
bool simplicity_lbtc_asset(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused.
  (void) env; // env is unused.
  const sha256_midstate lbtc_assetid = {{
    0x6d521c38u, 0xec1ea157u, 0x34ae22b7u, 0xc4606441u, 0x2829c0d0u, 0x579f0a71u, 0x3d1c04edu, 0xe979026fu
  }};

  writeHash(dst, &lbtc_assetid);
  return true;
}

/* build_tapleaf_simplicity : TWO^256 |- TWO^256 */
bool simplicity_build_tapleaf_simplicity(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; // env is unused.
  sha256_midstate cmr;
  readHash(&cmr, &src);
  sha256_midstate result = simplicity_make_tapleaf(0xbe, &cmr);
  writeHash(dst, &result);
  return true;
}

/* build_tapbranch : TWO^256 * TWO^256 |- TWO^256 */
bool simplicity_build_tapbranch(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; // env is unused.
  sha256_midstate a, b;
  readHash(&a, &src);
  readHash(&b, &src);

  sha256_midstate result = simplicity_make_tapbranch(&a, &b);
  writeHash(dst, &result);
  return true;
}

/* build_taptweak : PUBKEY * TWO^256 |- PUBKEY */
bool simplicity_build_taptweak(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; // env is unused.
  static unsigned char taptweak[] = "TapTweak/elements";
  return simplicity_generic_taptweak(dst, &src, taptweak, sizeof(taptweak)-1);
}

/* outpoint_hash : CTX8 * S TWO^256 * TWO^256 * TWO^32 |- CTX8 */
bool simplicity_outpoint_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; // env is unused.
  sha256_midstate midstate;
  unsigned char buf[36];
  sha256_context ctx = {.output = midstate.s};

  /* Read a SHA-256 context. */
  if (!simplicity_read_sha256_context(&ctx, &src)) return false;

  /* Read an optional pegin parent chain hash. */
  if (readBit(&src)) {
    /* Read a pegin parent chain hash. */
    read8s(buf, 32, &src);
    sha256_uchar(&ctx, 0x01);
    sha256_uchars(&ctx, buf, 32);
  } else {
    /* No pegin. */
    sha256_uchar(&ctx, 0x00);
    forwardBits(&src, 256);
  }

  /* Read an outpoint (hash and index). */
  read8s(buf, 36, &src);
  sha256_uchars(&ctx, buf, 36);

  return simplicity_write_sha256_context(dst, &ctx);
}

/* asset_amount_hash : CTX8 * Conf TWO^256 * Conf TWO^64 |- CTX8 */
bool simplicity_asset_amount_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; // env is unused.
  sha256_midstate midstate;
  unsigned char buf[32];
  sha256_context ctx = {.output = midstate.s};

  /* Read a SHA-256 context. */
  if (!simplicity_read_sha256_context(&ctx, &src)) return false;

  /* Read an asset id prefix. (2 bits) */
  if (readBit(&src)) {
    /* Read an explicit asset id prefix. (1 bit) */
    forwardBits(&src, 1);
    sha256_uchar(&ctx, 0x01);
  } else {
    /* Read an confidential asset id prefix. (1 bit) */
    if (readBit(&src)) {
      sha256_uchar(&ctx, 0x0b);
    } else {
      sha256_uchar(&ctx, 0x0a);
    }
  }
  /* Read an asset id body (both confidential and explicit asset bodies are the same size). (256 bits) */
  read8s(buf, 32, &src);
  sha256_uchars(&ctx, buf, 32);

  /* Read an amount. (258 bits) */
  if (readBit(&src)) {
    /* Read an explicit amount. (257 bits) */
    sha256_uchar(&ctx, 0x01);
    forwardBits(&src, 257-64);
    read8s(buf, 8, &src);
    sha256_uchars(&ctx, buf, 8);
  } else {
    /* Read an confidential amount. (257 bits) */
    if (readBit(&src)) {
      sha256_uchar(&ctx, 0x09);
    } else {
      sha256_uchar(&ctx, 0x08);
    }
    read8s(buf, 32, &src);
    sha256_uchars(&ctx, buf, 32);
  }

  return simplicity_write_sha256_context(dst, &ctx);
}

/* nonce_hash : CTX8 * S (Conf TWO^256) |- CTX8 */
bool simplicity_nonce_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; // env is unused.
  sha256_midstate midstate;
  unsigned char buf[32];
  sha256_context ctx = {.output = midstate.s};

  /* Read a SHA-256 context. */
  if (!simplicity_read_sha256_context(&ctx, &src)) return false;

  /* Read an optional nonce. (259 bits) */
  if (readBit(&src)) {
    /* Read a nonce prefix. (2 bits) */
    if (readBit(&src)) {
      /* Read an explicit none prefix. (1 bit) */
      forwardBits(&src, 1);
      sha256_uchar(&ctx, 0x01);
    } else {
      /* Read a confidential none prefix. (1 bit) */
      if (readBit(&src)) {
        sha256_uchar(&ctx, 0x03);
      } else {
        sha256_uchar(&ctx, 0x02);
      }
    }
    /* Read a nonce id body (both confidential and explicit nonce bodies are the same size). (256 bits) */
    read8s(buf, 32, &src);
    sha256_uchars(&ctx, buf, 32);
  } else {
    sha256_uchar(&ctx, 0x00);
  }

  return simplicity_write_sha256_context(dst, &ctx);
}

/* annex_hash : CTX8 * S TWO^256 |- CTX8 */
bool simplicity_annex_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; // env is unused.
  sha256_midstate midstate;
  unsigned char buf[32];
  sha256_context ctx = {.output = midstate.s};

  /* Read a SHA-256 context. */
  if (!simplicity_read_sha256_context(&ctx, &src)) return false;

  /* Read an optional hash. (257 bits) */
  if (readBit(&src)) {
    /* Read a hash. (256 bits) */
    read8s(buf, 32, &src);
    sha256_uchar(&ctx, 0x01);
    sha256_uchars(&ctx, buf, 32);
  } else {
    /* No hash. */
    sha256_uchar(&ctx, 0x00);
  }

  return simplicity_write_sha256_context(dst, &ctx);
}

/* issuance : TWO^256 |- S (S TWO) */
bool simplicity_issuance(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numInputs)) {
    const sigInput* input = &env->tx->input[i];
    if (writeBit(dst, NO_ISSUANCE != input->issuance.type)) {
      writeBit(dst, REISSUANCE == input->issuance.type);
    } else {
      skipBits(dst, 1);
    }
  } else {
    skipBits(dst, 2);
  }
  return true;
}

/* issuance_entropy : TWO^256 |- S (S TWO^256) */
bool simplicity_issuance_entropy(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numInputs)) {
    const sigInput* input = &env->tx->input[i];
    if (writeBit(dst, NO_ISSUANCE != input->issuance.type)) {
      writeHash(dst, &input->issuance.entropy);
    } else {
      skipBits(dst, 256);
    }
  } else {
    skipBits(dst, 257);
  }
  return true;
}

/* issuance_asset : TWO^256 |- S (S TWO^256) */
bool simplicity_issuance_asset(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numInputs)) {
    const sigInput* input = &env->tx->input[i];
    if (writeBit(dst, NO_ISSUANCE != input->issuance.type)) {
      writeHash(dst, &input->issuance.assetId);
    } else {
      skipBits(dst, 256);
    }
  } else {
    skipBits(dst, 257);
  }
  return true;
}

/* issuance_token : TWO^256 |- S (S TWO^256) */
bool simplicity_issuance_token(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numInputs)) {
    const sigInput* input = &env->tx->input[i];
    if (writeBit(dst, NO_ISSUANCE != input->issuance.type)) {
      writeHash(dst, &input->issuance.tokenId);
    } else {
      skipBits(dst, 256);
    }
  } else {
    skipBits(dst, 257);
  }
  return true;
}

/* output_amounts_hash : ONE |- TWO^256 */
bool simplicity_output_amounts_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->tx->outputAssetAmountsHash);
  return true;
}

/* output_nonces_hash : ONE |- TWO^256 */
bool simplicity_output_nonces_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->tx->outputNoncesHash);
  return true;
}

/* output_scripts_hash : ONE |- TWO^256 */
bool simplicity_output_scripts_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->tx->outputScriptsHash);
  return true;
}

/* output_range_proofs_hash : ONE |- TWO^256 */
bool simplicity_output_range_proofs_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->tx->outputRangeProofsHash);
  return true;
}

/* output_surjection_proofs_hash : ONE |- TWO^256 */
bool simplicity_output_surjection_proofs_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->tx->outputSurjectionProofsHash);
  return true;
}

/* outputs_hash : ONE |- TWO^256 */
bool simplicity_outputs_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->tx->outputsHash);
  return true;
}

/* output_hash : TWO^32 |- S TWO^256 */
bool simplicity_output_hash(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numOutputs)) {
    const sigOutput* output = &env->tx->output[i];
    sha256_midstate midstate;
    sha256_context ctx = sha256_init(midstate.s);
    simplicity_sha256_confAsset(&ctx, &output->asset);
    simplicity_sha256_confAmt(&ctx, &output->amt);
    simplicity_sha256_confNonce(&ctx, &output->nonce);
    sha256_hash(&ctx, &output->scriptPubKey);
    sha256_hash(&ctx, &output->rangeProofHash);
    sha256_finalize(&ctx);
    writeHash(dst, &midstate);
  } else {
    skipBits(dst, 256);
  }
  return true;
}

/* input_outpoints_hash : ONE |- TWO^256 */
bool simplicity_input_outpoints_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->tx->inputOutpointsHash);
  return true;
}

/* input_amounts_hash : ONE |- TWO^256 */
bool simplicity_input_amounts_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->tx->inputAssetAmountsHash);
  return true;
}

/* input_scripts_hash : ONE |- TWO^256 */
bool simplicity_input_scripts_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->tx->inputScriptsHash);
  return true;
}

/* input_utxos_hash : ONE |- TWO^256 */
bool simplicity_input_utxos_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->tx->inputUTXOsHash);
  return true;
}

/* input_utxo_hash : TWO^32 |- S TWO^256 */
bool simplicity_input_utxo_hash(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numInputs)) {
    const utxo* txo = &env->tx->input[i].txo;
    sha256_midstate midstate;
    sha256_context ctx = sha256_init(midstate.s);
    simplicity_sha256_confAsset(&ctx, &txo->asset);
    simplicity_sha256_confAmt(&ctx, &txo->amt);
    sha256_hash(&ctx, &txo->scriptPubKey);
    sha256_finalize(&ctx);
    writeHash(dst, &midstate);
  } else {
    skipBits(dst, 256);
  }
  return true;
}

/* input_sequences_hash : ONE |- TWO^256 */
bool simplicity_input_sequences_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->tx->inputSequencesHash);
  return true;
}

/* input_annexes_hash : ONE |- TWO^256 */
bool simplicity_input_annexes_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->tx->inputAnnexesHash);
  return true;
}

/* input_script_sigs_hash : ONE |- TWO^256 */
bool simplicity_input_script_sigs_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->tx->inputScriptSigsHash);
  return true;
}

/* inputs_hash : ONE |- TWO^256 */
bool simplicity_inputs_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->tx->inputsHash);
  return true;
}

/* input_hash : TWO^32 |- S TWO^256 */
bool simplicity_input_hash(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numInputs)) {
    const sigInput* input = &env->tx->input[i];
    sha256_midstate midstate;
    sha256_context ctx = sha256_init(midstate.s);
    if (input->isPegin) {
      sha256_uchar(&ctx, 1);
      sha256_hash(&ctx, &input->pegin);
    } else {
      sha256_uchar(&ctx, 0);
    }
    sha256_hash(&ctx, &input->prevOutpoint.txid);
    sha256_u32be(&ctx, input->prevOutpoint.ix);
    sha256_u32be(&ctx, input->sequence);
    if (input->hasAnnex) {
      sha256_uchar(&ctx, 1);
      sha256_hash(&ctx, &input->annexHash);
    } else {
      sha256_uchar(&ctx, 0);
    }
    sha256_finalize(&ctx);
    writeHash(dst, &midstate);
  } else {
    skipBits(dst, 256);
  }
  return true;
}

/* issuance_asset_amounts_hash : ONE |- TWO^256 */
bool simplicity_issuance_asset_amounts_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->tx->issuanceAssetAmountsHash);
  return true;
}

/* issuance_token_amounts_hash : ONE |- TWO^256 */
bool simplicity_issuance_token_amounts_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->tx->issuanceTokenAmountsHash);
  return true;
}

/* issuance_range_proofs_hash : ONE |- TWO^256 */
bool simplicity_issuance_range_proofs_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->tx->issuanceRangeProofsHash);
  return true;
}

/* issuance_blinding_entropy_hash : ONE |- TWO^256 */
bool simplicity_issuance_blinding_entropy_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->tx->issuanceBlindingEntropyHash);
  return true;
}

/* issuances_hash : ONE |- TWO^256 */
bool simplicity_issuances_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->tx->issuancesHash);
  return true;
}

/* issuance_hash : TWO^32 |- S TWO^256 */
bool simplicity_issuance_hash(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numInputs)) {
    const assetIssuance* issuance = &env->tx->input[i].issuance;
    sha256_midstate midstate;
    sha256_context ctx = sha256_init(midstate.s);
    if (NO_ISSUANCE == issuance->type) {
      sha256_uchar(&ctx, 0);
      sha256_uchar(&ctx, 0);
      sha256_uchar(&ctx, 0);
      sha256_uchar(&ctx, 0);
      sha256_hash(&ctx, &issuance->assetRangeProofHash);
      sha256_hash(&ctx, &issuance->tokenRangeProofHash);
      sha256_uchar(&ctx, 0);
    } else {
      simplicity_sha256_confAsset(&ctx, &(confidential){ .prefix = EXPLICIT, .data = issuance->assetId});
      simplicity_sha256_confAmt(&ctx, &issuance->assetAmt);
      simplicity_sha256_confAsset(&ctx, &(confidential){ .prefix = EXPLICIT, .data = issuance->tokenId});
      simplicity_sha256_confAmt(&ctx, NEW_ISSUANCE == issuance->type
                                         ? &issuance->tokenAmt
                                         : &(confAmount){ .prefix = EXPLICIT, .explicit = 0});
      sha256_hash(&ctx, &issuance->assetRangeProofHash);
      sha256_hash(&ctx, &issuance->tokenRangeProofHash);
      sha256_uchar(&ctx, 1);
      if (NEW_ISSUANCE == issuance->type) {
        sha256_uchars(&ctx, (unsigned char[32]){0}, 32);
        sha256_hash(&ctx, &issuance->contractHash);
      } else {
        sha256_hash(&ctx, &issuance->blindingNonce);
        sha256_hash(&ctx, &issuance->entropy);
      }
    }
    sha256_finalize(&ctx);
    writeHash(dst, &midstate);
  } else {
    skipBits(dst, 256);
  }
  return true;
}

/* tx_hash : ONE |- TWO^256 */
bool simplicity_tx_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->tx->txHash);
  return true;
}

/* tapleaf_hash : ONE |- TWO^256 */
bool simplicity_tapleaf_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->taproot->tapLeafHash);
  return true;
}

/* tappath_hash : ONE |- TWO^256 */
bool simplicity_tappath_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->taproot->tappathHash);
  return true;
}

/* tap_env_hash : ONE |- TWO^256 */
bool simplicity_tap_env_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->taproot->tapEnvHash);
  return true;
}

/* sig_all_hash : ONE |- TWO^256 */
bool simplicity_sig_all_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->sigAllHash);
  return true;
}
