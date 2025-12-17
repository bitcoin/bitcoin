#include <simplicity/elements/env.h>

#include <stdalign.h>
#include <stddef.h>
#include <string.h>
#include "txEnv.h"
#include "ops.h"
#include "../rsort.h"
#include "../sha256.h"
#include "../simplicity_assert.h"
#include "../simplicity_alloc.h"

#define PADDING(alignType, allocated) ((alignof(alignType) - (allocated) % alignof(alignType)) % alignof(alignType))

/* Compute the SHA-256 hash of a scriptPubKey and write it into 'result'.
 *
 * Precondition: NULL != result;
 *               NULL != scriptPubKey;
 */
static void hashBuffer(sha256_midstate* result, const rawElementsBuffer* buffer) {
  sha256_context ctx = sha256_init(result->s);
  sha256_uchars(&ctx, buffer->buf, buffer->len);
  sha256_finalize(&ctx);
}

/* Initialize a 'confidential' asset or 'confidential' nonce from an unsigned char array from a 'rawElementsTransaction'.
 *
 * Precondition: NULL != conf;
 *               unsigned char rawConf[33] or rawConf == NULL;
 */
static void copyRawConfidential(confidential* conf, const unsigned char* rawConf) {
  if (rawConf) {
    *conf = (confidential){ .prefix = 0x01 == rawConf[0] ? EXPLICIT
                                    : 0x01 == (0x01 & rawConf[0]) ? ODD_Y
                                    : EVEN_Y
                          };
    sha256_toMidstate(conf->data.s, &rawConf[1]);
  } else {
    *conf = (confidential){0};
  }
}

/* Initialize a 'confAmount' from an unsigned char array from a 'rawElementsTransaction'.
 *
 * Precondition: NULL != amt;
 *               unsigned char rawAmt[rawAmt[0] == 0x01 ? 9 : 33] or rawAmt == NULL
 */
static void copyRawAmt(confAmount* amt, const unsigned char* rawAmt) {
  if (rawAmt) {
    if (0x01 == rawAmt[0]) {
      amt->prefix = EXPLICIT;
      amt->explicit = ReadBE64(&rawAmt[1]);
    } else {
      amt->prefix = 0x01 == (0x01 & rawAmt[0]) ? ODD_Y : EVEN_Y;
      sha256_toMidstate(amt->confidential.s, &rawAmt[1]);
    }
  } else {
    amt->prefix = EXPLICIT;
    amt->explicit = 0;
  }
}

/* Initialize a 'sigInput' from a 'rawElementsInput', copying or hashing the data as needed.
 *
 * Precondition: NULL != result;
 *               NULL != input;
 */
static void copyInput(sigInput* result, const rawElementsInput* input) {
  *result = (sigInput){ .prevOutpoint = { .ix = input->prevIx }
                      , .sequence = input->sequence
                      , .isPegin = !!input->pegin
                      , .hasAnnex = !!input->annex
                      };

  if (input->annex) hashBuffer(&result->annexHash, input->annex);
  if (input->pegin) sha256_toMidstate(result->pegin.s, input->pegin);
  sha256_toMidstate(result->prevOutpoint.txid.s, input->prevTxid);
  hashBuffer(&result->txo.scriptPubKey, &input->txo.scriptPubKey);
  copyRawConfidential(&result->txo.asset, input->txo.asset);
  copyRawAmt(&result->txo.amt, input->txo.value);
  hashBuffer(&result->scriptSigHash, &input->scriptSig);
  hashBuffer(&result->issuance.assetRangeProofHash, &(rawElementsBuffer){0});
  hashBuffer(&result->issuance.tokenRangeProofHash, &(rawElementsBuffer){0});
  if (input->issuance.amount || input->issuance.inflationKeys) {
    sha256_toMidstate(result->issuance.blindingNonce.s, input->issuance.blindingNonce);
    copyRawAmt(&result->issuance.assetAmt, input->issuance.amount);
    if (is_confidential(result->issuance.assetAmt.prefix)) hashBuffer(&result->issuance.assetRangeProofHash, &input->issuance.amountRangePrf);
    if (0 == result->issuance.blindingNonce.s[0] && 0 == result->issuance.blindingNonce.s[1] &&
        0 == result->issuance.blindingNonce.s[2] && 0 == result->issuance.blindingNonce.s[3] &&
        0 == result->issuance.blindingNonce.s[4] && 0 == result->issuance.blindingNonce.s[5] &&
        0 == result->issuance.blindingNonce.s[6] && 0 == result->issuance.blindingNonce.s[7]) {
      sha256_toMidstate(result->issuance.contractHash.s, input->issuance.assetEntropy);
      result->issuance.entropy = simplicity_generateIssuanceEntropy(&result->prevOutpoint, &result->issuance.contractHash);
      copyRawAmt(&result->issuance.tokenAmt, input->issuance.inflationKeys);
      if (is_confidential(result->issuance.tokenAmt.prefix)) hashBuffer(&result->issuance.tokenRangeProofHash, &input->issuance.inflationKeysRangePrf);
      result->issuance.type = NEW_ISSUANCE;
    } else {
      sha256_toMidstate(result->issuance.entropy.s, input->issuance.assetEntropy);
      result->issuance.type = REISSUANCE;
    }
    result->issuance.assetId = simplicity_calculateAsset(&result->issuance.entropy);
    result->issuance.tokenId = simplicity_calculateToken(&result->issuance.entropy, result->issuance.assetAmt.prefix);
  }
}

/* As specified in https://github.com/ElementsProject/elements/blob/de942511a67c3a3fcbdf002a8ee7e9ba49679b78/src/primitives/transaction.h#L304-L307. */
static bool isFee(const rawElementsOutput* output) {
  return 0 == output->scriptPubKey.len &&                 /* Empty scriptPubKey */
    NULL != output->asset && 0x01 == output->asset[0] &&  /* Explicit asset */
    NULL != output->value && 0x01 == output->value[0];    /* Explicit amount */
}

static uint_fast32_t countFeeOutputs(const rawElementsTransaction* rawTx) {
  uint_fast32_t result = 0;
  for (uint_fast32_t i = 0; i < rawTx->numOutputs; ++i) {
    result += isFee(&rawTx->output[i]);
  }
  return result;
}

/* If the 'scriptPubKey' is a TX_NULL_DATA, return a count of the number of "push only" operations (this excludes the OP_RETURN).
 * Otherwise return 0.
 *
 * Note: that a TX_NULL_DATA could have zero "push only" operations,
 * in which case 0 is returned even though it is a TX_NULL_DATA scriptPubKey.
 *
 * Precondition: NULL != scriptPubKey
 */
static uint_fast32_t countNullDataCodes(const rawElementsBuffer* scriptPubKey) {
  if (0 == scriptPubKey->len || 0x6a != scriptPubKey->buf[0] ) return 0;

  uint_fast32_t result = 0;
  for (uint_fast32_t i = 1; i < scriptPubKey->len;) {
    uint_fast32_t skip = 0;
    unsigned char code = scriptPubKey->buf[i++];
    if (0x60 < code) return 0;
    if (code < 0x4c) {
      skip = code;
    } else if (code < 0x4f) {
      if (scriptPubKey->len == i) return 0;
      skip = scriptPubKey->buf[i++];
      if (0x4d <= code) {
        if (scriptPubKey->len == i) return 0;
        skip += (uint_fast32_t)(scriptPubKey->buf[i++]) << 8;
        if (0x4e <= code) {
          if (scriptPubKey->len == i) return 0;
          skip += (uint_fast32_t)(scriptPubKey->buf[i++]) << 16;
          if (scriptPubKey->len == i) return 0;
          skip += (uint_fast32_t)(scriptPubKey->buf[i++]) << 24;
        }
      }
    }
    if (scriptPubKey->len - i < skip) return 0;
    i += skip;
    result++;
  }
  return result;
}

/* Return a count of the total number of "push only" operations in all output scriptPubKey's that are TX_NULL_DATA.
 *
 * Precondition: NULL != rawTx
 */
static uint_fast64_t countTotalNullDataCodes(const rawElementsTransaction* rawTx) {
  uint_fast64_t result = 0;
  for (uint_fast32_t i = 0; i < rawTx->numOutputs; ++i) {
    result += countNullDataCodes(&rawTx->output[i].scriptPubKey);
  }
  return result;
}

/* Determine if 'scriptPubKey' is a TX_NULL_DATA script, and fill 'result' with (digests of) the push data opcodes.
 * If 'scriptPubKey' isn't a TX_NULL_DATA, then 'result->op' is set to NULL.
 * Otherwise '*result' is set to '(parsedNullData){ .op = *allocation, .len = countNullDataCodes(scriptPubKey) }.
 * and then the 'opcode result->op[result->len]' array is filled in with (digests of) the pus data opcode.
 *      and '*allocation' is incremented by 'result->len'
 *      and '*allocationLen' is decremented by 'result->len'.
 * Values in the '*allocation' array may be modified.
 *
 * Note: even if '*allocationLen == 0' we require that '*allocation != NULL' as a precondition
 * in order for 'result' to distinguish between non-NULL_TX_DATA and empty NULL_TX_data
 *
 * Precondition: NULL != result;
 *               NULL != *allocation;
 *               opcode (*allocation)[*allocationLen];
 *               NULL != scriptPubKey;
 *               countNullDataCodes(scriptPubKey) <= *allocationLen
 */
static void parseNullData(parsedNullData* result, opcode** allocation, size_t* allocationLen, const rawElementsBuffer* scriptPubKey) {
  *result = (parsedNullData){ .op = *allocation };

  if (0 == scriptPubKey->len || 0x6a != scriptPubKey->buf[0] ) { result->op = NULL; return; }

  for (uint_fast32_t i = 1; i < scriptPubKey->len; ++result->len) {
    unsigned char code = scriptPubKey->buf[i++];
    if (*allocationLen <= result->len || 0x60 < code) { result->op = NULL; return; }
    if (0x4f <= code) {
      (*allocation)[result->len].code = OP_1NEGATE + (code - 0x4f);
    } else {
      uint_fast32_t skip = 0;
      if (code < 0x4c) {
        skip = code;
        (*allocation)[result->len].code = OP_IMMEDIATE;
      } else {
        if (scriptPubKey->len == i) { result->op = NULL; return; }
        skip = scriptPubKey->buf[i++];
        if (code < 0x4d) {
          (*allocation)[result->len].code = OP_PUSHDATA;
        } else {
          if (scriptPubKey->len == i) { result->op = NULL; return; }
          skip += (uint_fast32_t)(scriptPubKey->buf[i++]) << 8;
          if (code < 0x4e) {
            (*allocation)[result->len].code = OP_PUSHDATA2;
          } else {
            if (scriptPubKey->len == i) { result->op = NULL; return; }
            skip += (uint_fast32_t)(scriptPubKey->buf[i++]) << 16;
            if (scriptPubKey->len == i) { result->op = NULL; return; }
            skip += (uint_fast32_t)(scriptPubKey->buf[i++]) << 24;
            (*allocation)[result->len].code = OP_PUSHDATA4;
          }
        }
      }
      if (scriptPubKey->len - i < skip) { result->op = NULL; return; }
      {
        sha256_context ctx = sha256_init((*allocation)[result->len].dataHash.s);
        sha256_uchars(&ctx, &scriptPubKey->buf[i], skip);
        sha256_finalize(&ctx);
      }
      i += skip;
    }
  }
  *allocation += result->len; /* C requires '*allocation != NULL', even when 'result->len == 0'. */
  *allocationLen -= result->len;
}

/* Initialize a 'sigOutput' from a 'rawElementsOutput', copying or hashing the data as needed.
 *
 * '*allocation' is incremented by 'countNullDataCodes(&output->scriptPubKey)'
 * '*allocationLen' is decremented by 'countNullDataCodes(&output->scriptPubKey)'.
 * Values in the '*allocation' array may be modified.
 *
 * Precondition: NULL != result;
 *               NULL != *allocation;
 *               opcode (*allocation)[*allocationLen];
 *               NULL != output;
 *               countNullDataCodes(&output->scriptPubKey) <= *allocationLen
 */
static void copyOutput(sigOutput* result, opcode** allocation, size_t* allocationLen, const rawElementsOutput* output) {
  hashBuffer(&result->scriptPubKey, &output->scriptPubKey);
  result->emptyScript = 0 == output->scriptPubKey.len;
  copyRawConfidential(&result->asset, output->asset);
  copyRawAmt(&result->amt, output->value);
  copyRawConfidential(&result->nonce, output->nonce);
  parseNullData(&result->pnd, allocation, allocationLen, &output->scriptPubKey);
  result->isNullData = NULL != result->pnd.op;
  hashBuffer(&result->surjectionProofHash, is_confidential(result->asset.prefix) ? &output->surjectionProof : &(rawElementsBuffer){0});
  hashBuffer(&result->rangeProofHash, is_confidential(result->amt.prefix) ? &output->rangeProof : &(rawElementsBuffer){0});
  result->assetFee = 0;
}

/* Tally a sorted list of feeOutputs
 *
 * Given a sorted array of feeOutput pointers, tally all the (explicit) amounts of the entries with the same asset id,
 * which are all necessarily next to each other, into the assetFee field of the first entry of the bunch.
 *
 * Discard all entries other than the first one of each bunch.
 * Return 'ret_value', the number of remaining entries in the array after these discards.
 *
 * Note: the array is not re-allocated, so there will be "junk" values in the array past the end of 'ret_value'.
 *
 * Precondition: feeOutputs is sorted by it asset.data.s field, which is to say
 *               for all 0 <= i <= j < numFees,
 *                 0 <= memcmp(feeOutputs[j]->asset.data.s, feeOutputs[i]->asset.data.s, sizeof(feeOutputs[i]->asset.data.s));
 *               for all 0 <= i < numFees,
 *                feeOutputs[i]->assetFee = 0 and
 *                feeOutputs[i]->amt.explicit is the active union member.
 * Postcondition: feeOutputs is uniquely sorted by it asset.data.s field, which is to say
 *                for all 0 <= i < j < ret_value,
 *                  0 < memcmp(feeOutputs[j]->asset.data.s, feeOutputs[i]->asset.data.s, sizeof(feeOutputs[i]->asset.data.s));
 *                for all ret_value <= i < numFees,
 *                  feeOutputs[i] remains allocated.
 */
static uint_fast32_t sumFees(sigOutput** feeOutputs, uint_fast32_t numFees) {
  uint_fast32_t result = 0;

  if (numFees < 1) return result;

  for(uint_fast32_t i = 0; i < numFees; ++i) {
    int cmp = memcmp(feeOutputs[i]->asset.data.s, feeOutputs[result]->asset.data.s, sizeof(feeOutputs[i]->asset.data.s));
    simplicity_assert(0 <= cmp);
    if (0 < cmp) {
      result++;
      feeOutputs[result] = feeOutputs[i];
    }

    static_assert(0 == offsetof(sigOutput, asset.data), "asset ID is not first field of sigOutput.");
    /* In Elements consensus rules, the total about of fees is not allowed to exceed MoneyRange:
     * https://github.com/ElementsProject/elements/blob/de942511a67c3a3fcbdf002a8ee7e9ba49679b78/src/confidential_validation.cpp#L36-L38
     *
     * In case of invalid transaction environments, we end up taking the result modulo the size of uint_fast64_t,
     * which in turn is compatible with our jet specification which returns the tally modulo 2^64.
     */
    feeOutputs[result]->assetFee += feeOutputs[i]->amt.explicit;
  }

  return result + 1;
}

/* Allocate and initialize a 'elementsTransaction' from a 'rawElementsTransaction', copying or hashing the data as needed.
 * Returns NULL if malloc fails (or if malloc cannot be called because we require an allocation larger than SIZE_MAX).
 *
 * Precondition: NULL != rawTx
 */
extern elementsTransaction* simplicity_elements_mallocTransaction(const rawElementsTransaction* rawTx) {
  if (!rawTx) return NULL;

  size_t allocationSize = sizeof(elementsTransaction);

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

  const size_t pad3 = PADDING(sigOutput*, allocationSize);
  if (SIZE_MAX - allocationSize < pad3) return NULL;
  allocationSize += pad3;

  const uint_fast32_t numFees = countFeeOutputs(rawTx);
  /* Multiply by (size_t)1 to disable type-limits warning. */
  if (SIZE_MAX / sizeof(sigOutput*) < (size_t)1 * numFees) return NULL;
  if (SIZE_MAX - allocationSize < numFees * sizeof(sigOutput*)) return NULL;
  allocationSize += numFees * sizeof(sigOutput*);

  const size_t pad4 = PADDING(opcode, allocationSize);
  if (SIZE_MAX - allocationSize < pad4) return NULL;
  allocationSize += pad4;

  const uint_fast64_t totalNullDataCodes = countTotalNullDataCodes(rawTx);
  /* Multiply by (size_t)1 to disable type-limits warning. */
  if (SIZE_MAX / sizeof(opcode) < (size_t)1 * totalNullDataCodes) return NULL;
  if (SIZE_MAX - allocationSize < totalNullDataCodes * sizeof(opcode)) return NULL;
  allocationSize += (size_t)totalNullDataCodes * sizeof(opcode);

  char *allocation = simplicity_malloc(allocationSize);
  if (!allocation) return NULL;

  /* Casting through void* to avoid warning about pointer alignment.
   * Our padding is done carefully to ensure alignment.
   */
  elementsTransaction* const tx = (elementsTransaction*)(void*)allocation;
  allocation += sizeof(elementsTransaction) + pad1;

  sigInput* const input = (sigInput*)(void*)allocation;
  allocation += rawTx->numInputs * sizeof(sigInput) + pad2;

  sigOutput* const output = (sigOutput*)(void*)allocation;
  allocation += rawTx->numOutputs * sizeof(sigOutput) + pad3;

  sigOutput** const feeOutputs = (sigOutput**)(void*)allocation;
  allocation += numFees * sizeof(sigOutput*) + pad4;

  opcode* ops = (opcode*)(void*)allocation;
  size_t opsLen = (size_t)totalNullDataCodes;

  /* In C++ an assignment from (sigOutput**) to (const sigOutput * const *) is allowed,
     but C forgoes the complicated specification of C++.  Therefore we must make an explicit cast of feeOutputs in C.
     See <https://c-faq.com/ansi/constmismatch.html> for details.
  */
  *tx = (elementsTransaction){ .input = input
                             , .output = output
                             , .feeOutputs = (sigOutput const * const *)feeOutputs
                             , .numInputs = rawTx->numInputs
                             , .numOutputs = rawTx->numOutputs
                             , .version = rawTx->version
                             , .lockTime = rawTx->lockTime
                             , .isFinal = true
                             };

  sha256_toMidstate(tx->txid.s, rawTx->txid);
  {
    sha256_context ctx_inputOutpointsHash = sha256_init(tx->inputOutpointsHash.s);
    sha256_context ctx_inputAssetAmountsHash = sha256_init(tx->inputAssetAmountsHash.s);
    sha256_context ctx_inputScriptsHash = sha256_init(tx->inputScriptsHash.s);
    sha256_context ctx_inputUTXOsHash = sha256_init(tx->inputUTXOsHash.s);
    sha256_context ctx_inputSequencesHash = sha256_init(tx->inputSequencesHash.s);
    sha256_context ctx_inputAnnexesHash = sha256_init(tx->inputAnnexesHash.s);
    sha256_context ctx_inputScriptSigsHash = sha256_init(tx->inputScriptSigsHash.s);
    sha256_context ctx_inputsHash = sha256_init(tx->inputsHash.s);
    sha256_context ctx_issuanceAssetAmountsHash = sha256_init(tx->issuanceAssetAmountsHash.s);
    sha256_context ctx_issuanceTokenAmountsHash = sha256_init(tx->issuanceTokenAmountsHash.s);
    sha256_context ctx_issuanceRangeProofsHash = sha256_init(tx->issuanceRangeProofsHash.s);
    sha256_context ctx_issuanceBlindingEntropyHash = sha256_init(tx->issuanceBlindingEntropyHash.s);
    sha256_context ctx_issuancesHash = sha256_init(tx->issuancesHash.s);
    for (uint_fast32_t i = 0; i < tx->numInputs; ++i) {
      copyInput(&input[i], &rawTx->input[i]);
      if (input[i].sequence < 0xffffffff) { tx->isFinal = false; }
      if (input[i].sequence < 0x80000000) {
         const uint_fast16_t maskedSequence = input[i].sequence & 0xffff;
         if (input[i].sequence & ((uint_fast32_t)1 << 22)) {
            if (tx->lockDuration < maskedSequence) tx->lockDuration = maskedSequence;
         } else {
            if (tx->lockDistance < maskedSequence) tx->lockDistance = maskedSequence;
         }
      }
      if (input[i].isPegin) {
        sha256_uchar(&ctx_inputOutpointsHash, 1);
        sha256_hash(&ctx_inputOutpointsHash, &input[i].pegin);
      } else {
        sha256_uchar(&ctx_inputOutpointsHash, 0);
      }
      sha256_hash(&ctx_inputOutpointsHash, &input[i].prevOutpoint.txid);
      sha256_u32be(&ctx_inputOutpointsHash, input[i].prevOutpoint.ix);
      simplicity_sha256_confAsset(&ctx_inputAssetAmountsHash, &input[i].txo.asset);
      simplicity_sha256_confAmt(&ctx_inputAssetAmountsHash, &input[i].txo.amt);
      sha256_hash(&ctx_inputScriptsHash, &input[i].txo.scriptPubKey);
      sha256_u32be(&ctx_inputSequencesHash, input[i].sequence);
      if (input[i].hasAnnex) {
        sha256_uchar(&ctx_inputAnnexesHash, 1);
        sha256_hash(&ctx_inputAnnexesHash, &input[i].annexHash);
      } else {
        sha256_uchar(&ctx_inputAnnexesHash, 0);
      }
      sha256_hash(&ctx_inputScriptSigsHash, &input[i].scriptSigHash);
      if (NO_ISSUANCE == input[i].issuance.type) {
        sha256_uchar(&ctx_issuanceAssetAmountsHash, 0);
        sha256_uchar(&ctx_issuanceAssetAmountsHash, 0);
        sha256_uchar(&ctx_issuanceTokenAmountsHash, 0);
        sha256_uchar(&ctx_issuanceTokenAmountsHash, 0);
        sha256_uchar(&ctx_issuanceBlindingEntropyHash, 0);
      } else {
        simplicity_sha256_confAsset(&ctx_issuanceAssetAmountsHash, &(confidential){ .prefix = EXPLICIT, .data = input[i].issuance.assetId});
        simplicity_sha256_confAsset(&ctx_issuanceTokenAmountsHash, &(confidential){ .prefix = EXPLICIT, .data = input[i].issuance.tokenId});
        simplicity_sha256_confAmt(&ctx_issuanceAssetAmountsHash, &input[i].issuance.assetAmt);
        simplicity_sha256_confAmt(&ctx_issuanceTokenAmountsHash, NEW_ISSUANCE == input[i].issuance.type
                                                    ? &input[i].issuance.tokenAmt
                                                    : &(confAmount){ .prefix = EXPLICIT, .explicit = 0});
        sha256_uchar(&ctx_issuanceBlindingEntropyHash, 1);
        if (NEW_ISSUANCE == input[i].issuance.type) {
          sha256_uchars(&ctx_issuanceBlindingEntropyHash, (unsigned char[32]){0}, 32);
          sha256_hash(&ctx_issuanceBlindingEntropyHash, &input[i].issuance.contractHash);
        } else {
          sha256_hash(&ctx_issuanceBlindingEntropyHash, &input[i].issuance.blindingNonce);
          sha256_hash(&ctx_issuanceBlindingEntropyHash, &input[i].issuance.entropy);
        }
      }
      sha256_hash(&ctx_issuanceRangeProofsHash, &input[i].issuance.assetRangeProofHash);
      sha256_hash(&ctx_issuanceRangeProofsHash, &input[i].issuance.tokenRangeProofHash);
    }
    sha256_finalize(&ctx_inputOutpointsHash);
    sha256_finalize(&ctx_inputAssetAmountsHash);
    sha256_finalize(&ctx_inputScriptsHash);
    sha256_finalize(&ctx_inputSequencesHash);
    sha256_finalize(&ctx_inputAnnexesHash);
    sha256_finalize(&ctx_inputScriptSigsHash);

    sha256_hash(&ctx_inputUTXOsHash, &tx->inputAssetAmountsHash);
    sha256_hash(&ctx_inputUTXOsHash, &tx->inputScriptsHash);
    sha256_finalize(&ctx_inputUTXOsHash);

    sha256_hash(&ctx_inputsHash, &tx->inputOutpointsHash);
    sha256_hash(&ctx_inputsHash, &tx->inputSequencesHash);
    sha256_hash(&ctx_inputsHash, &tx->inputAnnexesHash);
    sha256_finalize(&ctx_inputsHash);

    sha256_finalize(&ctx_issuanceAssetAmountsHash);
    sha256_finalize(&ctx_issuanceTokenAmountsHash);
    sha256_finalize(&ctx_issuanceRangeProofsHash);
    sha256_finalize(&ctx_issuanceBlindingEntropyHash);

    sha256_hash(&ctx_issuancesHash, &tx->issuanceAssetAmountsHash);
    sha256_hash(&ctx_issuancesHash, &tx->issuanceTokenAmountsHash);
    sha256_hash(&ctx_issuancesHash, &tx->issuanceRangeProofsHash);
    sha256_hash(&ctx_issuancesHash, &tx->issuanceBlindingEntropyHash);
    sha256_finalize(&ctx_issuancesHash);
  }

  {
    sha256_context ctx_outputAssetAmountsHash = sha256_init(tx->outputAssetAmountsHash.s);
    sha256_context ctx_outputNoncesHash = sha256_init(tx->outputNoncesHash.s);
    sha256_context ctx_outputScriptsHash = sha256_init(tx->outputScriptsHash.s);
    sha256_context ctx_outputRangeProofsHash = sha256_init(tx->outputRangeProofsHash.s);
    sha256_context ctx_outputSurjectionProofsHash = sha256_init(tx->outputSurjectionProofsHash.s);
    sha256_context ctx_outputsHash = sha256_init(tx->outputsHash.s);
    uint_fast32_t ix_fee = 0;

    /* perm is a temporary array the same length (numFees) and size as feeOutputs.
     * perm is used to initialize feeOutputs and is not used afterward.
     * This makes it safe for perm to use the same memory allocation as feeOutputs.
     */
    static_assert(sizeof(const sha256_midstate*) == sizeof(sigOutput*), "Pointers (to structures) ought to have the same size.");
    static_assert(alignof(const sha256_midstate*) == alignof(sigOutput*), "Pointers (to structures) ought to have the same alignment.");
    const sha256_midstate** const perm = (const sha256_midstate**)(void*)feeOutputs;

    for (uint_fast32_t i = 0; i < tx->numOutputs; ++i) {
      copyOutput(&output[i], &ops, &opsLen, &rawTx->output[i]);
      if (isFee(&rawTx->output[i])) {
        simplicity_assert(ix_fee < numFees);
        perm[ix_fee] = &output[i].asset.data;
        ++ix_fee;
      }
      simplicity_sha256_confAsset(&ctx_outputAssetAmountsHash, &output[i].asset);
      simplicity_sha256_confAmt(&ctx_outputAssetAmountsHash, &output[i].amt);
      simplicity_sha256_confNonce(&ctx_outputNoncesHash, &output[i].nonce);
      sha256_hash(&ctx_outputScriptsHash, &output[i].scriptPubKey);
      sha256_hash(&ctx_outputRangeProofsHash, &output[i].rangeProofHash);
      sha256_hash(&ctx_outputSurjectionProofsHash, &output[i].surjectionProofHash);
    }

    simplicity_assert(numFees == ix_fee);
    if (!simplicity_rsort(perm, numFees)) {
      simplicity_free(tx);
      return NULL;
    }

    /* Initialize the feeOutputs array from the perm array.
     * Because the perm array entries are the same size as the feeOutputs array entries, it is safe to initialize one by one.
     *
     * In practical C implementations, the feeOutputs array entries are initialized to the same value as the perm array entries.
     * In practical C implementations, this is a no-op, and generally compilers are able to see this fact and eliminate this loop.
     *
     * We keep the loop in the code just to be pedantic.
     */
    for (size_t i = 0; i < numFees; ++i) {
      static_assert(0 == offsetof(sigOutput, asset.data), "asset ID is not first field of sigOutput.");
      /* The uintptr_t cast is to suppress warning about both casting away const and a change in pointer alignment.
       * Each pointer in perm is pointing the the first field of some (non-const) sigOutput*, so this cast is safe.
       */
      feeOutputs[i] = (sigOutput*)(uintptr_t)(perm[i]);
    }
    tx->numFees = sumFees(feeOutputs, numFees);

    sha256_finalize(&ctx_outputAssetAmountsHash);
    sha256_finalize(&ctx_outputNoncesHash);
    sha256_finalize(&ctx_outputScriptsHash);
    sha256_finalize(&ctx_outputRangeProofsHash);
    sha256_finalize(&ctx_outputSurjectionProofsHash);

    sha256_hash(&ctx_outputsHash, &tx->outputAssetAmountsHash);
    sha256_hash(&ctx_outputsHash, &tx->outputNoncesHash);
    sha256_hash(&ctx_outputsHash, &tx->outputScriptsHash);
    sha256_hash(&ctx_outputsHash, &tx->outputRangeProofsHash);
    sha256_finalize(&ctx_outputsHash);
  }
  {
    sha256_context ctx_txHash = sha256_init(tx->txHash.s);
    sha256_u32be(&ctx_txHash, tx->version);
    sha256_u32be(&ctx_txHash, tx->lockTime);
    sha256_hash(&ctx_txHash, &tx->inputsHash);
    sha256_hash(&ctx_txHash, &tx->outputsHash);
    sha256_hash(&ctx_txHash, &tx->issuancesHash);
    sha256_hash(&ctx_txHash, &tx->outputSurjectionProofsHash);
    sha256_hash(&ctx_txHash, &tx->inputUTXOsHash);
    sha256_finalize(&ctx_txHash);
  }

  return tx;
}

/* Free a pointer to 'elementsTransaction'.
 */
extern void simplicity_elements_freeTransaction(elementsTransaction* tx) {
  simplicity_free(tx);
}

/* Allocate and initialize a 'elementsTapEnv' from a 'rawElementsTapEnv', copying or hashing the data as needed.
 * Returns NULL if malloc fails (or if malloc cannot be called because we require an allocation larger than SIZE_MAX).
 *
 * Precondition: *rawEnv is well-formed (i.e. rawEnv->pathLen <= 128.)
 */
extern elementsTapEnv* simplicity_elements_mallocTapEnv(const rawElementsTapEnv* rawEnv) {
  if (!rawEnv) return NULL;
  if (128 < rawEnv->pathLen) return NULL;

  size_t allocationSize = sizeof(elementsTapEnv);

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
  elementsTapEnv* const env = (elementsTapEnv*)(void*)allocation;
  sha256_midstate* path = NULL;
  sha256_midstate internalKey;

  sha256_toMidstate(internalKey.s,  &rawEnv->controlBlock[1]);

  if (numMidstate)  {
    allocation += sizeof(elementsTapEnv) + pad1;

    if (rawEnv->pathLen) {
      path = (sha256_midstate*)(void*)allocation;
    }
  }

  *env = (elementsTapEnv){ .leafVersion = rawEnv->controlBlock[0] & 0xfe
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

  env->tapLeafHash = simplicity_make_tapleaf(env->leafVersion, &env->scriptCMR);

  {
    sha256_context ctx = sha256_init(env->tapEnvHash.s);
    sha256_hash(&ctx, &env->tapLeafHash);
    sha256_hash(&ctx, &env->tappathHash);
    sha256_hash(&ctx, &env->internalKey);
    sha256_finalize(&ctx);
  }
  return env;
}

/* Free a pointer to 'elementsTapEnv'.
 */
extern void simplicity_elements_freeTapEnv(elementsTapEnv* env) {
  simplicity_free(env);
}
