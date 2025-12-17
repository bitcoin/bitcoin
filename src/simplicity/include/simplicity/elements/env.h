#ifndef SIMPLICITY_ELEMENTS_ENV_H
#define SIMPLICITY_ELEMENTS_ENV_H

#include <stdbool.h>
#include <stdint.h>

/* This section builds the 'rawElementsTransaction' structure which is the transaction data needed to build an Elements 'txEnv' environment
 * for evaluating Simplicity expressions within.
 * The 'rawElementsTransaction' is copied into an opaque 'elementsTransaction' structure that can be reused within evaluating Simplicity on multiple
 * inputs within the same transaction.
 */

/* A type for an unparsed buffer
 *
 * Invariant: if 0 < len then unsigned char buf[len]
 */
typedef struct rawElementsBuffer {
  const unsigned char* buf;
  uint32_t len;
} rawElementsBuffer;

/* A structure representing data for one output from an Elements transaction.
 *
 * Invariant: unsigned char asset[33] or asset == NULL;
 *            unsigned char value[value[0] == 1 ? 9 : 33] or value == NULL;
 *            unsigned char nonce[33] or nonce == NULL;
 */
typedef struct rawElementsOutput {
  const unsigned char* asset;
  const unsigned char* value;
  const unsigned char* nonce;
  rawElementsBuffer scriptPubKey;
  rawElementsBuffer surjectionProof;
  rawElementsBuffer rangeProof;
} rawElementsOutput;

/* A structure representing data for one input from an Elements transaction, including its taproot annex,
 * plus the TXO data of the output being redeemed.
 *
 * Invariant: unsigned char prevTxid[32];
 *            unsigned char pegin[32] or pegin == NULL;
 *            unsigned char issuance.blindingNonce[32] or (issuance.amount == NULL and issuance.inflationKeys == NULL);
 *            unsigned char issuance.assetEntropy[32] or (issuance.amount == NULL and issuance.inflationKeys == NULL);
 *            unsigned char issuance.amount[issuance.amount[0] == 1 ? 9 : 33] or issuance.amount == NULL;
 *            unsigned char issuance.inflationKeys[issuance.inflaitonKeys[0] == 1 ? 9 : 33] or issuance.inflationKeys == NULL;
 *            unsigned char txo.asset[33] or txo.asset == NULL;
 *            unsigned char txo.value[txo.value[0] == 1 ? 9 : 33] or txo.value == NULL;
 */
typedef struct rawElementsInput {
  const rawElementsBuffer* annex;
  const unsigned char* prevTxid;
  const unsigned char* pegin;
  struct {
    const unsigned char* blindingNonce;
    const unsigned char* assetEntropy;
    const unsigned char* amount;
    const unsigned char* inflationKeys;
    rawElementsBuffer amountRangePrf;
    rawElementsBuffer inflationKeysRangePrf;
  } issuance;
  struct {
    const unsigned char* asset;
    const unsigned char* value;
    rawElementsBuffer scriptPubKey;
  } txo;
  rawElementsBuffer scriptSig;
  uint32_t prevIx;
  uint32_t sequence;
} rawElementsInput;

/* A structure representing data for an Elements transaction, including the TXO data of each output being redeemed.
 *
 * Invariant: unsigned char txid[32];
 *            rawElementsInput input[numInputs];
 *            rawElementsOutput output[numOutputs];
 */
typedef struct rawElementsTransaction {
  const unsigned char* txid; /* While in theory we could recompute the txid ourselves, it is easier and safer for it to be provided. */
  const rawElementsInput* input;
  const rawElementsOutput* output;
  uint32_t numInputs;
  uint32_t numOutputs;
  uint32_t version;
  uint32_t lockTime;
} rawElementsTransaction;

/* A forward declaration for the structure containing a copy (and digest) of the rawElementsTransaction data */
typedef struct elementsTransaction elementsTransaction;

/* Allocate and initialize a 'elementsTransaction' from a 'rawElementsTransaction', copying or hashing the data as needed.
 * Returns NULL if malloc fails (or if malloc cannot be called because we require an allocation larger than SIZE_MAX).
 *
 * Precondition: NULL != rawTx
 */
extern elementsTransaction* simplicity_elements_mallocTransaction(const rawElementsTransaction* rawTx);

/* Free a pointer to 'elementsTransaction'.
 */
extern void simplicity_elements_freeTransaction(elementsTransaction* tx);

/* A structure representing taproot spending data for an Elements transaction.
 *
 * Invariant: pathLen <= 128;
 *            unsigned char controlBlock[33+pathLen*32];
 *            unsigned char scriptCMR[32];
 */
typedef struct rawElementsTapEnv {
  const unsigned char* controlBlock;
  const unsigned char* scriptCMR;
  unsigned char pathLen;
} rawElementsTapEnv;

/* A forward declaration for the structure containing a copy (and digest) of the rawElementsTapEnv data */
typedef struct elementsTapEnv elementsTapEnv;

/* Allocate and initialize a 'elementsTapEnv' from a 'rawElementsTapEnv', copying or hashing the data as needed.
 * Returns NULL if malloc fails (or if malloc cannot be called because we require an allocation larger than SIZE_MAX).
 *
 * Precondition: *rawEnv is well-formed (i.e. rawEnv->pathLen <= 128.)
 */
extern elementsTapEnv* simplicity_elements_mallocTapEnv(const rawElementsTapEnv* rawEnv);

/* Free a pointer to 'elementsTapEnv'.
 */
extern void simplicity_elements_freeTapEnv(elementsTapEnv* env);
#endif
