#ifndef SIMPLICITY_BITCOIN_ENV_H
#define SIMPLICITY_BITCOIN_ENV_H

#include <stdbool.h>
#include <stdint.h>

/* This section builds the 'rawBitcoinTransaction' structure which is the transaction data needed to build a Bitcoin 'txEnv' environment
 * for evaluating Simplicity expressions within.
 * The 'rawBitcoinTransaction' is copied into an opaque 'bitcoinTransaction' structure that can be reused within evaluating Simplicity on multiple
 * inputs within the same transaction.
 */

/* A type for an unparsed buffer
 *
 * Invariant: if 0 < len then unsigned char buf[len]
 */
typedef struct rawBitcoinBuffer {
  const unsigned char* buf;
  uint32_t len;
} rawBitcoinBuffer;

/* A structure representing data for one output from a Bitcoin transaction.
 */
typedef struct rawBitcoinOutput {
  uint64_t value;
  rawBitcoinBuffer scriptPubKey;
} rawBitcoinOutput;

/* A structure representing data for one input from a Bitcoin transaction, including its taproot annex,
 * plus the TXO data of the output being redeemed.
 *
 * Invariant: unsigned char prevTxid[32];
 */
typedef struct rawInput {
  const rawBitcoinBuffer* annex;
  const unsigned char* prevTxid;
  rawBitcoinOutput txo;
  rawBitcoinBuffer scriptSig;
  uint32_t prevIx;
  uint32_t sequence;
} rawBitcoinInput;

/* A structure representing data for a Bitcoin transaction, including the TXO data of each output being redeemed.
 *
 * Invariant: unsigned char txid[32];
 *            rawBitcoinInput input[numInputs];
 *            rawBitcoinOutput output[numOutputs];
 */
typedef struct rawBitcoinTransaction {
  const unsigned char* txid; /* While in theory we could recompute the txid ourselves, it is easier and safer for it to be provided. */
  const rawBitcoinInput* input;
  const rawBitcoinOutput* output;
  uint32_t numInputs;
  uint32_t numOutputs;
  uint32_t version;
  uint32_t lockTime;
} rawBitcoinTransaction;

/* A forward declaration for the structure containing a copy (and digest) of the rawTransaction data */
typedef struct bitcoinTransaction bitcoinTransaction;

/* Allocate and initialize a 'bitcoinTransaction' from a 'rawBitcoinTransaction', copying or hashing the data as needed.
 * Returns NULL if malloc fails (or if malloc cannot be called because we require an allocation larger than SIZE_MAX).
 *
 * Precondition: NULL != rawTx
 */
extern bitcoinTransaction* simplicity_bitcoin_mallocTransaction(const rawBitcoinTransaction* rawTx);

/* Free a pointer to 'bitcoinTransaction'.
 */
extern void simplicity_bitcoin_freeTransaction(bitcoinTransaction* tx);

/* A structure representing taproot spending data for a Bitcoin transaction.
 *
 * Invariant: pathLen <= 128;
 *            unsigned char controlBlock[33+pathLen*32];
 *            unsigned char scriptCMR[32];
 */
typedef struct rawBitcoinTapEnv {
  const unsigned char* controlBlock;
  const unsigned char* scriptCMR;
  unsigned char pathLen;
} rawBitcoinTapEnv;

/* A forward declaration for the structure containing a copy (and digest) of the rawBitcoinTapEnv data */
typedef struct bitcoinTapEnv bitcoinTapEnv;

/* Allocate and initialize a 'bitcoinTapEnv' from a 'rawBitcoinTapEnv', copying or hashing the data as needed.
 * Returns NULL if malloc fails (or if malloc cannot be called because we require an allocation larger than SIZE_MAX).
 *
 * Precondition: *rawEnv is well-formed (i.e. rawEnv->pathLen <= 128.)
 */
extern bitcoinTapEnv* simplicity_bitcoin_mallocTapEnv(const rawBitcoinTapEnv* rawEnv);

/* Free a pointer to 'bitcoinTapEnv'.
 */
extern void simplicity_bitcoin_freeTapEnv(bitcoinTapEnv* env);
#endif
