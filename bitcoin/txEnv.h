/* This module defines the environment ('txEnv') for Simplicity evaluation for Bitcoin.
 * It includes the transaction data and input index of the input whose Simplicity program is being executed.
 * It also includes the commitment Merkle root of the program being executed.
 */
#ifndef SIMPLICITY_BITCOIN_TXENV_H
#define SIMPLICITY_BITCOIN_TXENV_H

#include <stdbool.h>
#include "../sha256.h"

/* An Bitcoin 'outpoint' consists of a transaction id and output index within that transaction.
 */
typedef struct outpoint {
  sha256_midstate txid;
  uint_fast32_t ix;
} outpoint;

/* A structure representing data from one output from a Bitcoin transaction.
 * 'scriptPubKey' is the SHA-256 hash of the outputs scriptPubKey.
 */
typedef struct sigOutput {
  uint_fast64_t value;
  sha256_midstate scriptPubKey;
} sigOutput;

/* A structure representing data from one input from a Bitcoin transaction along with the utxo data of the output being redeemed.
 * When 'hasAnnex' then 'annexHash' is a cache of the hash of the input's segwit annex.
 */
typedef struct sigInput {
  sha256_midstate annexHash;
  sha256_midstate scriptSigHash;
  outpoint prevOutpoint;
  sigOutput txo;
  uint_fast32_t sequence;
  bool hasAnnex;
} sigInput;

/* A structure representing data from a Bitcoin transaction (along with the utxo data of the outputs being redeemed).
 * Includes a variety of cached hash values that are used in signature hash jets.
 */
typedef struct bitcoinTransaction {
  const sigInput* input;
  const sigOutput* output;
  sha256_midstate outputValuesHash;
  sha256_midstate outputScriptsHash;
  sha256_midstate outputsHash;
  sha256_midstate inputOutpointsHash;
  sha256_midstate inputValuesHash;
  sha256_midstate inputScriptsHash;
  sha256_midstate inputUTXOsHash;
  sha256_midstate inputSequencesHash;
  sha256_midstate inputAnnexesHash;
  sha256_midstate inputScriptSigsHash;
  sha256_midstate inputsHash;
  sha256_midstate txHash;
  sha256_midstate txid;
  uint_fast64_t totalInputValue;
  uint_fast64_t totalOutputValue;
  uint_fast32_t numInputs;
  uint_fast32_t numOutputs;
  uint_fast32_t version;
  uint_fast32_t lockTime;
  /* lockDuration and lockDistance values are set even when the version is 0 or 1.
   * This is similar to lockTime whose value is also set, even when the transaction is final.
   */
  uint_fast16_t lockDistance;
  uint_fast16_t lockDuration; /* Units of 512 seconds */
  bool isFinal;
} bitcoinTransaction;

/* A structure representing taproot spending data from a Bitcoin transaction.
 *
 * Invariant: pathLen <= 128
 *            sha256_midstate path[pathLen];
 */
typedef struct bitcoinTapEnv {
  const sha256_midstate *path;
  sha256_midstate tapLeafHash;
  sha256_midstate tappathHash;
  sha256_midstate tapEnvHash;
  sha256_midstate internalKey;
  sha256_midstate scriptCMR;
  unsigned char pathLen;
  unsigned char leafVersion;
} bitcoinTapEnv;

/* The 'txEnv' structure used by the Bitcoin application of Simplicity.
 *
 * It includes
 * + the transaction data, which may be shared when Simplicity expressions are used for multiple inputs in the same transaction),
 * + the input index under consideration,
 */
typedef struct txEnv {
  const bitcoinTransaction* tx;
  const bitcoinTapEnv* taproot;
  sha256_midstate sigAllHash;
  uint_fast32_t ix;
} txEnv;

/* Construct a txEnv structure from its components.
 * This function will precompute any cached values.
 *
 * Precondition: NULL != tx
 *               NULL != taproot
 *               ix < tx->numInputs
 */
txEnv simplicity_bitcoin_build_txEnv(const bitcoinTransaction* tx, const bitcoinTapEnv* taproot, uint_fast32_t ix);

#endif
