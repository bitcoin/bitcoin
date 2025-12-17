#include <simplicity/bitcoin/env.h>

#include <stdalign.h>
#include <stddef.h>
#include <string.h>
#include "txEnv.h"
#include "ops.h"
#include "../sha256.h"
#include "../simplicity_assert.h"
#include "../simplicity_alloc.h"

#define PADDING(alignType, allocated) ((alignof(alignType) - (allocated) % alignof(alignType)) % alignof(alignType))

/* Compute the SHA-256 hash of a scriptPubKey and write it into 'result'.
 *
 * Precondition: NULL != result;
 *               NULL != scriptPubKey;
 */
static void hashBuffer(sha256_midstate* result, const rawBitcoinBuffer* buffer) {
  sha256_context ctx = sha256_init(result->s);
  sha256_uchars(&ctx, buffer->buf, buffer->len);
  sha256_finalize(&ctx);
}

/* Initialize a 'sigOutput' from a 'rawOutput', copying or hashing the data as needed.
 *
 * Precondition: NULL != result;
 *               NULL != output;
 */
static void copyOutput(sigOutput* result, const rawBitcoinOutput* output) {
  hashBuffer(&result->scriptPubKey, &output->scriptPubKey);
  result->value = output->value;
}

/* Initialize a 'sigInput' from a 'rawBitcoinInput', copying or hashing the data as needed.
 *
 * Precondition: NULL != result;
 *               NULL != input;
 */
static void copyInput(sigInput* result, const rawBitcoinInput* input) {
  *result = (sigInput){ .prevOutpoint = { .ix = input->prevIx }
                      , .sequence = input->sequence
                      , .hasAnnex = !!input->annex
                      };

  if (input->annex) hashBuffer(&result->annexHash, input->annex);
  sha256_toMidstate(result->prevOutpoint.txid.s, input->prevTxid);
  copyOutput(&result->txo, &input->txo);
  hashBuffer(&result->scriptSigHash, &input->scriptSig);
}

/* Allocate and initialize a 'bitcoinTransaction' from a 'rawBitcoinTransaction', copying or hashing the data as needed.
 * Returns NULL if malloc fails (or if malloc cannot be called because we require an allocation larger than SIZE_MAX).
 *
 * Precondition: NULL != rawTx
 */
extern bitcoinTransaction* simplicity_bitcoin_mallocTransaction(const rawBitcoinTransaction* rawTx) {
  if (!rawTx) return NULL;

  size_t allocationSize = sizeof(bitcoinTransaction);

  const size_t pad1 = PADDING(sigInput, allocationSize);
  if (SIZE_MAX - allocationSize < pad1) return NULL;
  allocationSize += pad1;

  /* Multiply by (size_t)1 to disable type-limits warning. */
  if (SIZE_MAX / sizeof(sigInput) < (size_t)1 * rawTx->numInputs) return NULL;
  if (SIZE_MAX - allocationSize < rawTx->numInputs * sizeof(sigInput)) return NULL;
  allocationSize += rawTx->numInputs * sizeof(sigInput);

  const size_t pad2 = PADDING(sigOutput, allocationSize);
  if (SIZE_MAX - allocationSize < pad2) return NULL;
  allocationSize += pad2;

  /* Multiply by (size_t)1 to disable type-limits warning. */
  if (SIZE_MAX / sizeof(sigOutput) < (size_t)1 * rawTx->numOutputs) return NULL;
  if (SIZE_MAX - allocationSize < rawTx->numOutputs * sizeof(sigOutput)) return NULL;
  allocationSize += rawTx->numOutputs * sizeof(sigOutput);

  char *allocation = simplicity_malloc(allocationSize);
  if (!allocation) return NULL;

  /* Casting through void* to avoid warning about pointer alignment.
   * Our padding is done carefully to ensure alignment.
   */
  bitcoinTransaction* const tx = (bitcoinTransaction*)(void*)allocation;
  allocation += sizeof(bitcoinTransaction) + pad1;

  sigInput* const input = (sigInput*)(void*)allocation;
  allocation += rawTx->numInputs * sizeof(sigInput) + pad2;

  sigOutput* const output = (sigOutput*)(void*)allocation;

  *tx = (bitcoinTransaction){ .input = input
                            , .output = output
                            , .numInputs = rawTx->numInputs
                            , .numOutputs = rawTx->numOutputs
                            , .version = rawTx->version
                            , .lockTime = rawTx->lockTime
                            , .isFinal = true
                            };

  sha256_toMidstate(tx->txid.s, rawTx->txid);
  {
    sha256_context ctx_inputOutpointsHash = sha256_init(tx->inputOutpointsHash.s);
    sha256_context ctx_inputValuesHash = sha256_init(tx->inputValuesHash.s);
    sha256_context ctx_inputScriptsHash = sha256_init(tx->inputScriptsHash.s);
    sha256_context ctx_inputUTXOsHash = sha256_init(tx->inputUTXOsHash.s);
    sha256_context ctx_inputSequencesHash = sha256_init(tx->inputSequencesHash.s);
    sha256_context ctx_inputAnnexesHash = sha256_init(tx->inputAnnexesHash.s);
    sha256_context ctx_inputScriptSigsHash = sha256_init(tx->inputScriptSigsHash.s);
    sha256_context ctx_inputsHash = sha256_init(tx->inputsHash.s);
    for (uint_fast32_t i = 0; i < tx->numInputs; ++i) {
      copyInput(&input[i], &rawTx->input[i]);
      tx->totalInputValue += input[i].txo.value;
      if (input[i].sequence < 0xffffffff) { tx->isFinal = false; }
      if (input[i].sequence < 0x80000000) {
         const uint_fast16_t maskedSequence = input[i].sequence & 0xffff;
         if (input[i].sequence & ((uint_fast32_t)1 << 22)) {
            if (tx->lockDuration < maskedSequence) tx->lockDuration = maskedSequence;
         } else {
            if (tx->lockDistance < maskedSequence) tx->lockDistance = maskedSequence;
         }
      }
      sha256_hash(&ctx_inputOutpointsHash, &input[i].prevOutpoint.txid);
      sha256_u32be(&ctx_inputOutpointsHash, input[i].prevOutpoint.ix);
      sha256_u64be(&ctx_inputValuesHash, input[i].txo.value);
      sha256_hash(&ctx_inputScriptsHash, &input[i].txo.scriptPubKey);
      sha256_u32be(&ctx_inputSequencesHash, input[i].sequence);
      if (input[i].hasAnnex) {
        sha256_uchar(&ctx_inputAnnexesHash, 1);
        sha256_hash(&ctx_inputAnnexesHash, &input[i].annexHash);
      } else {
        sha256_uchar(&ctx_inputAnnexesHash, 0);
      }
      sha256_hash(&ctx_inputScriptSigsHash, &input[i].scriptSigHash);
    }
    sha256_finalize(&ctx_inputOutpointsHash);
    sha256_finalize(&ctx_inputValuesHash);
    sha256_finalize(&ctx_inputScriptsHash);
    sha256_finalize(&ctx_inputSequencesHash);
    sha256_finalize(&ctx_inputAnnexesHash);
    sha256_finalize(&ctx_inputScriptSigsHash);

    sha256_hash(&ctx_inputUTXOsHash, &tx->inputValuesHash);
    sha256_hash(&ctx_inputUTXOsHash, &tx->inputScriptsHash);
    sha256_finalize(&ctx_inputUTXOsHash);

    sha256_hash(&ctx_inputsHash, &tx->inputOutpointsHash);
    sha256_hash(&ctx_inputsHash, &tx->inputSequencesHash);
    sha256_hash(&ctx_inputsHash, &tx->inputAnnexesHash);
    sha256_finalize(&ctx_inputsHash);
  }

  {
    sha256_context ctx_outputValuesHash = sha256_init(tx->outputValuesHash.s);
    sha256_context ctx_outputScriptsHash = sha256_init(tx->outputScriptsHash.s);
    sha256_context ctx_outputsHash = sha256_init(tx->outputsHash.s);

    for (uint_fast32_t i = 0; i < tx->numOutputs; ++i) {
      copyOutput(&output[i], &rawTx->output[i]);
      tx->totalOutputValue += output[i].value;
      sha256_u64be(&ctx_outputValuesHash, output[i].value);
      sha256_hash(&ctx_outputScriptsHash, &output[i].scriptPubKey);
    }

    sha256_finalize(&ctx_outputValuesHash);
    sha256_finalize(&ctx_outputScriptsHash);

    sha256_hash(&ctx_outputsHash, &tx->outputValuesHash);
    sha256_hash(&ctx_outputsHash, &tx->outputScriptsHash);
    sha256_finalize(&ctx_outputsHash);
  }
  {
    sha256_context ctx_txHash = sha256_init(tx->txHash.s);
    sha256_u32be(&ctx_txHash, tx->version);
    sha256_u32be(&ctx_txHash, tx->lockTime);
    sha256_hash(&ctx_txHash, &tx->inputsHash);
    sha256_hash(&ctx_txHash, &tx->outputsHash);
    sha256_hash(&ctx_txHash, &tx->inputUTXOsHash);
    sha256_finalize(&ctx_txHash);
  }

  return tx;
}

/* Free a pointer to 'bitcoinTransaction'.
 */
extern void simplicity_bitcoin_freeTransaction(bitcoinTransaction* tx) {
  simplicity_free(tx);
}

/* Allocate and initialize a 'bitcoinTapEnv' from a 'rawBitcoinTapEnv', copying or hashing the data as needed.
 * Returns NULL if malloc fails (or if malloc cannot be called because we require an allocation larger than SIZE_MAX).
 *
 * Precondition: *rawEnv is well-formed (i.e. rawEnv->pathLen <= 128.)
 */
extern bitcoinTapEnv* simplicity_bitcoin_mallocTapEnv(const rawBitcoinTapEnv* rawEnv) {
  if (!rawEnv) return NULL;
  if (128 < rawEnv->pathLen) return NULL;

  size_t allocationSize = sizeof(bitcoinTapEnv);

  const size_t numMidstate = rawEnv->pathLen;
  const size_t pad1 = PADDING(sha256_midstate, allocationSize);

  if (numMidstate) {
    if (SIZE_MAX - allocationSize < pad1) return NULL;
    allocationSize += pad1;

    if (SIZE_MAX / sizeof(sha256_midstate) < numMidstate) return NULL;
    if (SIZE_MAX - allocationSize < numMidstate * sizeof(sha256_midstate)) return NULL;
    allocationSize += numMidstate * sizeof(sha256_midstate);
  }

  char *allocation = simplicity_malloc(allocationSize);
  if (!allocation) return NULL;

  /* Casting through void* to avoid warning about pointer alignment.
   * Our padding is done carefully to ensure alignment.
   */
  bitcoinTapEnv* const env = (bitcoinTapEnv*)(void*)allocation;
  sha256_midstate* path = NULL;
  sha256_midstate internalKey;

  sha256_toMidstate(internalKey.s,  &rawEnv->controlBlock[1]);

  if (numMidstate)  {
    allocation += sizeof(bitcoinTapEnv) + pad1;

    if (rawEnv->pathLen) {
      path = (sha256_midstate*)(void*)allocation;
    }
  }

  *env = (bitcoinTapEnv){ .leafVersion = rawEnv->controlBlock[0] & 0xfe
                        , .internalKey = internalKey
                        , .path = path
                        , .pathLen = rawEnv->pathLen
                        };
  sha256_toMidstate(env->scriptCMR.s, rawEnv->scriptCMR);

  {
    sha256_context ctx = sha256_init(env->tappathHash.s);
    for (int i = 0; i < env->pathLen; ++i) {
      sha256_toMidstate(path[i].s,  &rawEnv->controlBlock[33+32*i]);
      sha256_hash(&ctx, &path[i]);
    }
    sha256_finalize(&ctx);
  }

  env->tapLeafHash = simplicity_bitcoin_make_tapleaf(env->leafVersion, &env->scriptCMR);

  {
    sha256_context ctx = sha256_init(env->tapEnvHash.s);
    sha256_hash(&ctx, &env->tapLeafHash);
    sha256_hash(&ctx, &env->tappathHash);
    sha256_hash(&ctx, &env->internalKey);
    sha256_finalize(&ctx);
  }
  return env;
}

/* Free a pointer to 'bitcoinTapEnv'.
 */
extern void simplicity_bitcoin_freeTapEnv(bitcoinTapEnv* env) {
  simplicity_free(env);
}
