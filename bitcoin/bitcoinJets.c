#include "bitcoinJets.h"

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

static uint_fast32_t lockHeight(const bitcoinTransaction* tx) {
  return !tx->isFinal && tx->lockTime < 500000000U ? tx->lockTime : 0;
}

static uint_fast32_t lockTime(const bitcoinTransaction* tx) {
  return !tx->isFinal && 500000000U <= tx->lockTime ? tx->lockTime : 0;
}

static uint_fast16_t lockDistance(const bitcoinTransaction* tx) {
  return 2 <= tx->version ? tx->lockDistance : 0;
}

static uint_fast16_t lockDuration(const bitcoinTransaction* tx) {
  return 2 <= tx->version ? tx->lockDuration : 0;
}

/* version : ONE |- TWO^32 */
bool simplicity_bitcoin_version(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  simplicity_write32(dst, env->tx->version);
  return true;
}

/* lock_time : ONE |- TWO^32 */
bool simplicity_bitcoin_lock_time(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  simplicity_write32(dst, env->tx->lockTime);
  return true;
}

/* input_prev_outpoint : TWO^32 |- S (TWO^256 * TWO^32) */
bool simplicity_bitcoin_input_prev_outpoint(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numInputs)) {
    prevOutpoint(dst, &env->tx->input[i].prevOutpoint);
  } else {
    skipBits(dst, 288);
  }
  return true;
}

/* input_value : TWO^32 |- S TWO^64 */
bool simplicity_bitcoin_input_value(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numInputs)) {
    simplicity_write64(dst, env->tx->input[i].txo.value);
  } else {
    skipBits(dst, 64);
  }
  return true;
}

/* input_script_hash : TWO^32 |- S TWO^256 */
bool simplicity_bitcoin_input_script_hash(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numInputs)) {
    writeHash(dst, &env->tx->input[i].txo.scriptPubKey);
  } else {
    skipBits(dst, 256);
  }
  return true;
}

/* input_sequence : TWO^32 |- S TWO^32 */
bool simplicity_bitcoin_input_sequence(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numInputs)) {
    simplicity_write32(dst, env->tx->input[i].sequence);
  } else {
    skipBits(dst, 32);
  }
  return true;
}

/* input_annex_hash : TWO^32 |- S (S (TWO^256)) */
bool simplicity_bitcoin_input_annex_hash(frameItem* dst, frameItem src, const txEnv* env) {
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
bool simplicity_bitcoin_input_script_sig_hash(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numInputs)) {
    writeHash(dst, &env->tx->input[i].scriptSigHash);
  } else {
    skipBits(dst, 256);
  }
  return true;
}

/* output_value : TWO^32 |- S (TWO^64) */
bool simplicity_bitcoin_output_value(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numOutputs)) {
    simplicity_write64(dst, env->tx->output[i].value);
  } else {
    skipBits(dst, 64);
  }
  return true;
}

/* output_script_hash : TWO^32 |- S TWO^256 */
bool simplicity_bitcoin_output_script_hash(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numOutputs)) {
    writeHash(dst, &env->tx->output[i].scriptPubKey);
  } else {
    skipBits(dst, 256);
  }
  return true;
}

/* fee : ONE |- TWO^64 */
bool simplicity_bitcoin_fee(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  simplicity_write64(dst, env->tx->totalInputValue - env->tx->totalOutputValue);
  return true;
}

/* total_input_value : ONE |- TWO^64 */
bool simplicity_bitcoin_total_input_value(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  simplicity_write64(dst, env->tx->totalInputValue);
  return true;
}

/* total_output_value : ONE |- TWO^64 */
bool simplicity_bitcoin_total_output_value(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  simplicity_write64(dst, env->tx->totalOutputValue);
  return true;
}

/* script_cmr : ONE |- TWO^256 */
bool simplicity_bitcoin_script_cmr(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  write32s(dst, env->taproot->scriptCMR.s, 8);
  return true;
}

/* transaction_id : ONE |- TWO^256 */
bool simplicity_bitcoin_transaction_id(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  write32s(dst, env->tx->txid.s, 8);
  return true;
}

/* current_index : ONE |- TWO^32 */
bool simplicity_bitcoin_current_index(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  simplicity_write32(dst, env->ix);
  return true;
}

/* current_prev_outpoint : ONE |- TWO^256 * TWO^32 */
bool simplicity_bitcoin_current_prev_outpoint(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  if (env->tx->numInputs <= env->ix) return false;
  prevOutpoint(dst, &env->tx->input[env->ix].prevOutpoint);
  return true;
}

/* current_value : ONE |- (TWO^64) */
bool simplicity_bitcoin_current_value(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  if (env->tx->numInputs <= env->ix) return false;
  simplicity_write64(dst, env->tx->input[env->ix].txo.value);
  return true;
}

/* current_script_hash : ONE |- TWO^256 */
bool simplicity_bitcoin_current_script_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  if (env->tx->numInputs <= env->ix) return false;
  writeHash(dst, &env->tx->input[env->ix].txo.scriptPubKey);
  return true;
}

/* current_sequence : ONE |- TWO^32 */
bool simplicity_bitcoin_current_sequence(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  if (env->tx->numInputs <= env->ix) return false;
  simplicity_write32(dst, env->tx->input[env->ix].sequence);
  return true;
}

/* current_script_sig_hash : ONE |- TWO^256 */
bool simplicity_bitcoin_current_script_sig_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  if (env->tx->numInputs <= env->ix) return false;
  writeHash(dst, &env->tx->input[env->ix].scriptSigHash);
  return true;
}

/* current_annex_hash : ONE |- S (TWO^256) */
bool simplicity_bitcoin_current_annex_hash(frameItem* dst, frameItem src, const txEnv* env) {
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
bool simplicity_bitcoin_tapleaf_version(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  simplicity_write8(dst, env->taproot->leafVersion);
  return true;
}

/* tappath : TWO^8 |- S (TWO^256) */
bool simplicity_bitcoin_tappath(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast8_t i = simplicity_read8(&src);
  if (writeBit(dst, i < env->taproot->pathLen)) {
    writeHash(dst, &env->taproot->path[i]);
  } else {
    skipBits(dst, 256);
  }
  return true;
}

/* internal_key : ONE |- TWO^256 */
bool simplicity_bitcoin_internal_key(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->taproot->internalKey);
  return true;
}

/* num_inputs : ONE |- TWO^32 */
bool simplicity_bitcoin_num_inputs(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  simplicity_write32(dst, env->tx->numInputs);
  return true;
}

/* num_outputs : ONE |- TWO^32 */
bool simplicity_bitcoin_num_outputs(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  simplicity_write32(dst, env->tx->numOutputs);
  return true;
}

/* tx_is_final : ONE |- TWO */
bool simplicity_bitcoin_tx_is_final(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeBit(dst, env->tx->isFinal);
  return true;
}

/* tx_lock_height : ONE |- TWO^32 */
bool simplicity_bitcoin_tx_lock_height(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  simplicity_write32(dst, lockHeight(env->tx));
  return true;
}

/* tx_lock_time : ONE |- TWO^32 */
bool simplicity_bitcoin_tx_lock_time(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  simplicity_write32(dst, lockTime(env->tx));
  return true;
}

/* tx_lock_distance : ONE |- TWO^16 */
bool simplicity_bitcoin_tx_lock_distance(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  simplicity_write16(dst, lockDistance(env->tx));
  return true;
}

/* tx_lock_duration : ONE |- TWO^16 */
bool simplicity_bitcoin_tx_lock_duration(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  simplicity_write16(dst, lockDuration(env->tx));
  return true;
}

/* check_lock_height : TWO^32 |- ONE */
bool simplicity_bitcoin_check_lock_height(frameItem* dst, frameItem src, const txEnv* env) {
  (void) dst; // dst is unused;
  uint_fast32_t x = simplicity_read32(&src);
  return x <= lockHeight(env->tx);
}

/* check_lock_time : TWO^32 |- ONE */
bool simplicity_bitcoin_check_lock_time(frameItem* dst, frameItem src, const txEnv* env) {
  (void) dst; // dst is unused;
  uint_fast32_t x = simplicity_read32(&src);
  return x <= lockTime(env->tx);
}

/* check_lock_distance : TWO^16 |- ONE */
bool simplicity_bitcoin_check_lock_distance(frameItem* dst, frameItem src, const txEnv* env) {
  (void) dst; // dst is unused;
  uint_fast16_t x = simplicity_read16(&src);
  return x <= lockDistance(env->tx);
}

/* check_lock_duration : TWO^16 |- ONE */
bool simplicity_bitcoin_check_lock_duration(frameItem* dst, frameItem src, const txEnv* env) {
  (void) dst; // dst is unused;
  uint_fast16_t x = simplicity_read16(&src);
  return x <= lockDuration(env->tx);
}

/* build_tapleaf_simplicity : TWO^256 |- TWO^256 */
bool simplicity_bitcoin_build_tapleaf_simplicity(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; // env is unused.
  sha256_midstate cmr;
  readHash(&cmr, &src);
  sha256_midstate result = simplicity_bitcoin_make_tapleaf(0xbe, &cmr);
  writeHash(dst, &result);
  return true;
}

/* build_tapbranch : TWO^256 * TWO^256 |- TWO^256 */
bool simplicity_bitcoin_build_tapbranch(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; // env is unused.
  sha256_midstate a, b;
  readHash(&a, &src);
  readHash(&b, &src);

  sha256_midstate result = simplicity_bitcoin_make_tapbranch(&a, &b);
  writeHash(dst, &result);
  return true;
}

/* build_taptweak : PUBKEY * TWO^256 |- PUBKEY */
bool simplicity_bitcoin_build_taptweak(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; // env is unused.
  static unsigned char taptweak[] = "TapTweak";
  return simplicity_generic_taptweak(dst, &src, taptweak, sizeof(taptweak)-1);
}

/* outpoint_hash : CTX8 * TWO^256 * TWO^32 |- CTX8 */
bool simplicity_bitcoin_outpoint_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; // env is unused.
  sha256_midstate midstate;
  unsigned char buf[36];
  sha256_context ctx = {.output = midstate.s};

  /* Read a SHA-256 context. */
  if (!simplicity_read_sha256_context(&ctx, &src)) return false;

  /* Read an outpoint (hash and index). */
  read8s(buf, 36, &src);
  sha256_uchars(&ctx, buf, 36);

  return simplicity_write_sha256_context(dst, &ctx);
}

/* annex_hash : CTX8 * S TWO^256 |- CTX8 */
bool simplicity_bitcoin_annex_hash(frameItem* dst, frameItem src, const txEnv* env) {
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

/* output_amounts_hash : ONE |- TWO^256 */
bool simplicity_bitcoin_output_values_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->tx->outputValuesHash);
  return true;
}

/* output_scripts_hash : ONE |- TWO^256 */
bool simplicity_bitcoin_output_scripts_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->tx->outputScriptsHash);
  return true;
}

/* outputs_hash : ONE |- TWO^256 */
bool simplicity_bitcoin_outputs_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->tx->outputsHash);
  return true;
}

/* output_hash : TWO^32 |- S TWO^256 */
bool simplicity_bitcoin_output_hash(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numOutputs)) {
    const sigOutput* output = &env->tx->output[i];
    sha256_midstate midstate;
    sha256_context ctx = sha256_init(midstate.s);
    sha256_u64be(&ctx, output->value);
    sha256_hash(&ctx, &output->scriptPubKey);
    sha256_finalize(&ctx);
    writeHash(dst, &midstate);
  } else {
    skipBits(dst, 256);
  }
  return true;
}

/* input_outpoints_hash : ONE |- TWO^256 */
bool simplicity_bitcoin_input_outpoints_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->tx->inputOutpointsHash);
  return true;
}

/* input_values_hash : ONE |- TWO^256 */
bool simplicity_bitcoin_input_values_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->tx->inputValuesHash);
  return true;
}

/* input_scripts_hash : ONE |- TWO^256 */
bool simplicity_bitcoin_input_scripts_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->tx->inputScriptsHash);
  return true;
}

/* input_utxos_hash : ONE |- TWO^256 */
bool simplicity_bitcoin_input_utxos_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->tx->inputUTXOsHash);
  return true;
}

/* input_utxo_hash : TWO^32 |- S TWO^256 */
bool simplicity_bitcoin_input_utxo_hash(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numInputs)) {
    const sigOutput* txo = &env->tx->input[i].txo;
    sha256_midstate midstate;
    sha256_context ctx = sha256_init(midstate.s);
    sha256_u64be(&ctx, txo->value);
    sha256_hash(&ctx, &txo->scriptPubKey);
    sha256_finalize(&ctx);
    writeHash(dst, &midstate);
  } else {
    skipBits(dst, 256);
  }
  return true;
}

/* input_sequences_hash : ONE |- TWO^256 */
bool simplicity_bitcoin_input_sequences_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->tx->inputSequencesHash);
  return true;
}

/* input_annexes_hash : ONE |- TWO^256 */
bool simplicity_bitcoin_input_annexes_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->tx->inputAnnexesHash);
  return true;
}

/* input_script_sigs_hash : ONE |- TWO^256 */
bool simplicity_bitcoin_input_script_sigs_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->tx->inputScriptSigsHash);
  return true;
}

/* inputs_hash : ONE |- TWO^256 */
bool simplicity_bitcoin_inputs_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->tx->inputsHash);
  return true;
}

/* input_hash : TWO^32 |- S TWO^256 */
bool simplicity_bitcoin_input_hash(frameItem* dst, frameItem src, const txEnv* env) {
  uint_fast32_t i = simplicity_read32(&src);
  if (writeBit(dst, i < env->tx->numInputs)) {
    const sigInput* input = &env->tx->input[i];
    sha256_midstate midstate;
    sha256_context ctx = sha256_init(midstate.s);
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

/* tx_hash : ONE |- TWO^256 */
bool simplicity_bitcoin_tx_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->tx->txHash);
  return true;
}

/* tapleaf_hash : ONE |- TWO^256 */
bool simplicity_bitcoin_tapleaf_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->taproot->tapLeafHash);
  return true;
}

/* tappath_hash : ONE |- TWO^256 */
bool simplicity_bitcoin_tappath_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->taproot->tappathHash);
  return true;
}

/* tap_env_hash : ONE |- TWO^256 */
bool simplicity_bitcoin_tap_env_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->taproot->tapEnvHash);
  return true;
}

/* sig_all_hash : ONE |- TWO^256 */
bool simplicity_bitcoin_sig_all_hash(frameItem* dst, frameItem src, const txEnv* env) {
  (void) src; // src is unused;
  writeHash(dst, &env->sigAllHash);
  return true;
}
