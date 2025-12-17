/* This module defines the environment ('txEnv') for Simplicity evaluation for Elements.
 * It includes the transaction data and input index of the input whose Simplicity program is being executed.
 * It also includes the commitment Merkle root of the program being executed.
 */
#ifndef SIMPLICITY_ELEMENTS_TXENV_H
#define SIMPLICITY_ELEMENTS_TXENV_H

#include <stdbool.h>
#include "../sha256.h"

/* An Elements 'outpoint' consists of a transaction id and output index within that transaction.
 * The transaction id can be a either a transaction within the chain, or the transaction id from another chain in case of a peg-in.
 */
typedef struct outpoint {
  sha256_midstate txid;
  uint_fast32_t ix;
} outpoint;

/* In Elements, many fields can optionally be blinded and encoded as a point on the secp256k1 curve.
 * This prefix determines the parity of the y-coordinate of that point point, or indicates the value is explicit.
 * Sometimes values are entirely optional, in which case 'NONE' is a possibility.
 */
typedef enum confPrefix {
  NONE = 0,
  EXPLICIT = 1,
  EVEN_Y = 2,
  ODD_Y = 3
} confPrefix;

/* Returns true when the prefix indicates the associated value is a confidential value. */
static inline bool is_confidential(confPrefix prefix) {
  return EVEN_Y == prefix || ODD_Y == prefix;
}

/* A confidential (256-bit) hash.
 * When 'prefix' is either 'EVEN_Y' or 'ODD_Y' then 'data' contains the x-coordinate of the point on the secp256k1 curve
 * representing the blinded value.
 * When 'prefix' is 'EXPLICIT' then 'data' is the unblinded 256-bit hash.
 * When 'prefix' is 'NONE' the value is "NULL" and the 'data' field is unused.
 */
typedef struct confidential {
  sha256_midstate data;
  confPrefix prefix;
} confidential;

/* A confidential 64-bit value.
 * When 'prefix' is either 'EVEN_Y' or 'ODD_Y' then 'confidential' contains the x-coordinate of the point on the secp256k1 curve
 * representing the blinded value.
 * When 'prefix' is 'EXPLICIT' then 'explicit' is the unblinded 256-bit hash.
 * invariant: 'prefix' != 'NONE'
 */
typedef struct confAmount {
  union {
    sha256_midstate confidential;
    uint_fast64_t explicit;
  };
  confPrefix prefix;
} confAmount;

/* In Elements, a null-data scriptPubKey consists of an OP_RETURN followed by data only pushes (i.e. only opcodes less than OP_16).
 * This is an enumeration of all such data only push operation names.
 * OP_IMMEDIATE represents OP_0 and all the one-byte prefixes of data pushes up to 75 bytes.
 */
typedef enum opcodeType {
  OP_IMMEDIATE,
  OP_PUSHDATA,
  OP_PUSHDATA2,
  OP_PUSHDATA4,
  OP_1NEGATE,
  OP_RESERVED,
  OP_1,
  OP_2,
  OP_3,
  OP_4,
  OP_5,
  OP_6,
  OP_7,
  OP_8,
  OP_9,
  OP_10,
  OP_11,
  OP_12,
  OP_13,
  OP_14,
  OP_15,
  OP_16
} opcodeType;

/* In Elements, a null-data scriptPubKey consists of an OP_RETURN followed by data only pushes (i.e. only opcodes less than OP_16).
 * This is a structure represents a digest of all such operations.
 * 'code' represents the operation name.
 * If 'code' \in {OP_IMMEDIATE, OP_PUSHDATA, OP_PUSHDATA2, OP_PUSHDATA4} then 'dataHash' represents the SHA-256 hash of data pushed
 * by the push operation.
 */
typedef struct opcode {
  sha256_midstate dataHash;
  opcodeType code;
} opcode;

/* In Elements, a null-data scriptPubKey consists of an OP_RETURN followed by data only pushes (i.e. only opcodes less than OP_16).
 * This is an structure for an array of digets of null-data scriptPubKeys.
 * 'op' is an array of the (digests of) each data push.
 * Note that 'len' is 0 for a null-data scriptPubKey consisting of only an OP_RETURN.
 *
 * Invariant: opcode op[len] (or op == NULL and len == 0)
 */
typedef struct parsedNullData {
  const opcode* op;
  uint_fast32_t len;
} parsedNullData;

/* A structure representing data from one output from an Elements transaction.
 * 'surjectionProofHash' is the SHA-256 hash of the output's surjection proof.
 * 'rangeProofHash' is the SHA-256 hash of the output's range proof.
 * 'scriptPubKey' is the SHA-256 hash of the outputs scriptPubKey.
 * 'isNullData' is true if the output has a null-data scriptPubKey.
 */
typedef struct sigOutput {
  confidential asset;
  sha256_midstate surjectionProofHash;
  sha256_midstate rangeProofHash;
  confAmount amt;
  confidential nonce;
  sha256_midstate scriptPubKey;
  uint_fast64_t assetFee;
  parsedNullData pnd;
  bool isNullData;
  bool emptyScript;
} sigOutput;

/* The data held by an Elements unspent transaction output database.
 * This 'scriptPubKey' of the unspent transaction output, which in our application is digested as a SHA-256 hash.
 * This also includes the asset and amount of the output, each of which may or may not be blinded.
 */
typedef struct utxo {
  sha256_midstate scriptPubKey;
  confidential asset;
  confAmount amt;
} utxo;

/* In Elements, a transaction input can optionally issue a new asset or reissue an existing asset.
 * This enumerates those possibilities.
 */
typedef enum issuanceType {
  NO_ISSUANCE = 0,
  NEW_ISSUANCE,
  REISSUANCE
} issuanceType;

/* In Elements, a transaction input can optionally issue a new asset or reissue an existing asset.
 * This structure contains data about such an issuance.
 * 'assetRangeProofHash' is the SHA-256 hash of the asset amount's range proof.
 * 'tokenRangeProofHash' is the SHA-256 hash of the token amount's range proof.
 *
 * Invariant: If 'type == NEW_ISSUANCE' then 'contractHash' and 'tokenAmt' are active;
 *            If 'type == REISSUANCE' then 'blindingNonce' is active;
 */
typedef struct assetIssuance {
  union {
    struct {
      sha256_midstate contractHash;
      confAmount tokenAmt;
    };
    struct {
      sha256_midstate blindingNonce;
    };
  };
  sha256_midstate entropy;
  sha256_midstate assetRangeProofHash;
  sha256_midstate tokenRangeProofHash;
  sha256_midstate assetId; /* Cached asset ID calculation. */
  sha256_midstate tokenId; /* Cached token ID calculation. */
  confAmount assetAmt;
  issuanceType type;
} assetIssuance;

/* A structure representing data from one input from an Elements transaction along with the utxo data of the output being redeemed.
 * When 'hasAnnex' then 'annexHash' is a cache of the hash of the input's segwit annex.
 * When 'isPegin' then the 'prevOutpoint' represents an outpoint of another chain
 * and 'pegin' contains the hash of the parent chain's genesis block.
 */
typedef struct sigInput {
  sha256_midstate annexHash;
  sha256_midstate pegin;
  sha256_midstate scriptSigHash;
  outpoint prevOutpoint;
  utxo txo;
  uint_fast32_t sequence;
  assetIssuance issuance;
  bool hasAnnex;
  bool isPegin;
} sigInput;

/* A structure representing data from an Elements transaction (along with the utxo data of the outputs being redeemed).
 * Includes a variety of cached hash values that are used in signature hash jets.
 */
typedef struct elementsTransaction {
  const sigInput* input;
  const sigOutput* output;
  const sigOutput* const * feeOutputs;
  sha256_midstate outputAssetAmountsHash;
  sha256_midstate outputNoncesHash;
  sha256_midstate outputScriptsHash;
  sha256_midstate outputRangeProofsHash;
  sha256_midstate outputSurjectionProofsHash;
  sha256_midstate outputsHash;
  sha256_midstate inputOutpointsHash;
  sha256_midstate inputAssetAmountsHash;
  sha256_midstate inputScriptsHash;
  sha256_midstate inputUTXOsHash;
  sha256_midstate inputSequencesHash;
  sha256_midstate inputAnnexesHash;
  sha256_midstate inputScriptSigsHash;
  sha256_midstate inputsHash;
  sha256_midstate issuanceAssetAmountsHash;
  sha256_midstate issuanceTokenAmountsHash;
  sha256_midstate issuanceRangeProofsHash;
  sha256_midstate issuanceBlindingEntropyHash;
  sha256_midstate issuancesHash;
  sha256_midstate txHash;
  sha256_midstate txid;
  uint_fast32_t numInputs;
  uint_fast32_t numOutputs;
  uint_fast32_t numFees;
  uint_fast32_t version;
  uint_fast32_t lockTime;
  /* lockDuration and lockDistance values are set even when the version is 0 or 1.
   * This is similar to lockTime whose value is also set, even when the transaction is final.
   */
  uint_fast16_t lockDistance;
  uint_fast16_t lockDuration; /* Units of 512 seconds */
  bool isFinal;
} elementsTransaction;

/* A structure representing taproot spending data from an Elements transaction.
 *
 * Invariant: pathLen <= 128
 *            sha256_midstate path[pathLen];
 */
typedef struct elementsTapEnv {
  const sha256_midstate *path;
  sha256_midstate tapLeafHash;
  sha256_midstate tappathHash;
  sha256_midstate tapEnvHash;
  sha256_midstate internalKey;
  sha256_midstate scriptCMR;
  unsigned char pathLen;
  unsigned char leafVersion;
} elementsTapEnv;

/* The 'txEnv' structure used by the Elements application of Simplicity.
 *
 * It includes
 * + the transaction data, which may be shared when Simplicity expressions are used for multiple inputs in the same transaction),
 * + the input index under consideration,
 * + the hash of the genesis block for the chain,
 */
typedef struct txEnv {
  const elementsTransaction* tx;
  const elementsTapEnv* taproot;
  sha256_midstate genesisHash;
  sha256_midstate sigAllHash;
  uint_fast32_t ix;
} txEnv;

/* Construct a txEnv structure from its components.
 * This function will precompute any cached values.
 *
 * Precondition: NULL != tx
 *               NULL != taproot
 *               NULL != genesisHash
 *               ix < tx->numInputs
 */
txEnv simplicity_elements_build_txEnv(const elementsTransaction* tx, const elementsTapEnv* taproot, const sha256_midstate* genesisHash, uint_fast32_t ix);

#endif
