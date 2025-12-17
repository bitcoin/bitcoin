#include "deserialize.h"

#include <limits.h>
#include "limitations.h"
#include "simplicity_alloc.h"
#include "simplicity_assert.h"

/* Fetches 'len' 'uint32_t's from 'stream' into 'result'.
 * The bits in each 'uint32_t' are set from the MSB to the LSB and the 'uint32_t's of 'result' are set from 0 up to 'len'.
 * Returns 'SIMPLICITY_ERR_BITSTREAM_EOF' if not enough bits are available ('result' may be modified).
 * Returns 'SIMPLICITY_NO_ERROR' if successful.
 *
 * Precondition: uint32_t result[len];
 *               NULL != stream
 */
static simplicity_err getWord32Array(uint32_t* result, const size_t len, bitstream* stream) {
  for (size_t i = 0; i < len; ++i) {
    /* Due to error codes, readNBits cannot fetch 32 bits at once. Instead we fetch two groups of 16 bits. */
    int32_t bits16 = simplicity_readNBits(16, stream);
    if (bits16 < 0) return (simplicity_err)bits16;
    result[i] = (uint32_t)bits16 << 16;
    bits16 = simplicity_readNBits(16, stream);
    if (bits16 < 0) return (simplicity_err)bits16;
    result[i] |= (uint32_t)bits16;
  }
  return SIMPLICITY_NO_ERROR;
}

/* Fetches a 256-bit hash value from 'stream' into 'result'.
 * Returns 'SIMPLICITY_ERR_BITSTREAM_EOF' if not enough bits are available ('result' may be modified).
 * Returns 'SIMPLICITY_NO_ERROR' if successful.
 *
 * Precondition: NULL != result
 *               NULL != stream
 */
static simplicity_err getHash(sha256_midstate* result, bitstream* stream) {
  return getWord32Array(result->s, 8, stream);
}


/* Decode a single node of a Simplicity dag from 'stream' into 'dag'['i'].
 * Returns 'SIMPLICITY_ERR_FAIL_CODE' if the encoding of a fail expression is encountered
 *   (all fail subexpressions ought to have been pruned prior to serialization).
 * Returns 'SIMPLICITY_ERR_RESERVED_CODE' if a reserved codeword is encountered.
 * Returns 'SIMPLICITY_ERR_HIDDEN' if the decoded node has a HIDDEN child in a position where it is not allowed.
 * Returns 'SIMPLICITY_ERR_DATA_OUT_OF_RANGE' if the node's child isn't a reference to one of the preceding nodes.
 *                                            or some encoding for a non-existent jet is encountered
 *                                            or the size of a WORD encoding is greater than 2^31 bits.
 * Returns 'SIMPLICITY_ERR_BITSTRING_EOF' if not enough bits are available in the 'stream'.
 * In the above error cases, 'dag' may be modified.
 * Returns 'SIMPLICITY_NO_ERROR' if successful.
 *
 * Precondition: dag_node dag[i + 1];
 *               i < 2^31 - 1
 *               NULL != stream
 */
static simplicity_err decodeNode(dag_node* dag, simplicity_callback_decodeJet decodeJet, uint_fast32_t i, bitstream* stream) {
  int32_t bit = read1Bit(stream);
  if (bit < 0) return (simplicity_err)bit;
  dag[i] = (dag_node){0};
  if (bit) {
    bit = read1Bit(stream);
    if (bit < 0) return (simplicity_err)bit;
    if (bit) {
      return decodeJet(&dag[i], stream);
    } else {
      /* Decode WORD. */
      int32_t depth = simplicity_decodeUptoMaxInt(stream);
      if (depth < 0) return (simplicity_err)depth;
      if (32 < depth) return SIMPLICITY_ERR_DATA_OUT_OF_RANGE;
      {
        simplicity_err error = simplicity_readBitstring(&dag[i].compactValue, (size_t)1 << (depth - 1), stream);
        if (!IS_OK(error)) return error;
      }
      dag[i].tag = WORD;
      dag[i].targetIx = (size_t)depth;
      dag[i].cmr = simplicity_computeWordCMR(&dag[i].compactValue, (size_t)(depth - 1));
    }
  } else {
    int32_t code = simplicity_readNBits(2, stream);
    if (code < 0) return (simplicity_err)code;
    int32_t subcode = simplicity_readNBits(code < 3 ? 2 : 1, stream);
    if (subcode < 0) return (simplicity_err)subcode;
    for (int32_t j = 0; j < 2 - code; ++j) {
      int32_t ix = simplicity_decodeUptoMaxInt(stream);
      if (ix < 0) return (simplicity_err)ix;
      if (i < (uint_fast32_t)ix) return SIMPLICITY_ERR_DATA_OUT_OF_RANGE;
      dag[i].child[j] = i - (uint_fast32_t)ix;
    }
    switch (code) {
     case 0:
      switch (subcode) {
       case 0: dag[i].tag = COMP; break;
       case 1:
        dag[i].tag = (HIDDEN == dag[dag[i].child[0]].tag) ? ASSERTR
                   : (HIDDEN == dag[dag[i].child[1]].tag) ? ASSERTL
                   : CASE;
        break;
       case 2: dag[i].tag = PAIR; break;
       case 3: dag[i].tag = DISCONNECT; break;
      }
      break;
     case 1:
      switch (subcode) {
       case 0: dag[i].tag = INJL; break;
       case 1: dag[i].tag = INJR; break;
       case 2: dag[i].tag = TAKE; break;
       case 3: dag[i].tag = DROP; break;
      }
      break;
     case 2:
      switch (subcode) {
       case 0: dag[i].tag = IDEN; break;
       case 1: dag[i].tag = UNIT; break;
       case 2: return SIMPLICITY_ERR_FAIL_CODE;
       case 3: return SIMPLICITY_ERR_RESERVED_CODE;
      }
      break;
     case 3:
      switch (subcode) {
       case 0:
        dag[i].tag = HIDDEN;
        return getHash(&(dag[i].cmr), stream);
       case 1:
        dag[i].tag = WITNESS;
        break;
      }
      break;
    }

    /* Verify that there are no illegal HIDDEN children. */
    for (int32_t j = 0; j < 2 - code; ++j) {
       if (HIDDEN == dag[dag[i].child[j]].tag && dag[i].tag != (j ? ASSERTL : ASSERTR)) return SIMPLICITY_ERR_HIDDEN;
    }

    simplicity_computeCommitmentMerkleRoot(dag, i);
  }
  return SIMPLICITY_NO_ERROR;
}

/* Decode a Simplicity DAG consisting of 'len' nodes from 'stream' into 'dag'.
 * Returns 'SIMPLICITY_ERR_DATA_OUT_OF_RANGE' if some node's child isn't a reference to one of the preceding nodes.
 * Returns 'SIMPLICITY_ERR_FAIL_CODE' if the encoding of a fail expression is encountered
 *   (all fail subexpressions ought to have been pruned prior to deserialization).
 * Returns 'SIMPLICITY_ERR_RESERVED_CODE' if a reserved codeword is encountered.
 * Returns 'SIMPLICITY_ERR_HIDDEN' if the decoded node has a HIDDEN child in a position where it is not allowed.
 * Returns 'SIMPLICITY_ERR_BITSTRING_EOF' if not enough bits are available in the 'stream'.
 * In the above error cases, 'dag' may be modified.
 * Returns 'SIMPLICITY_NO_ERROR' if successful.
 *
 * Precondition: dag_node dag[len];
 *               len < 2^31
 *               NULL != stream
 */
static simplicity_err decodeDag(dag_node* dag, simplicity_callback_decodeJet decodeJet, const uint_fast32_t len, combinator_counters* census, bitstream* stream) {
  for (uint_fast32_t i = 0; i < len; ++i) {
    simplicity_err error = decodeNode(dag, decodeJet, i, stream);
    if (!IS_OK(error)) return error;

    enumerator(census, dag[i].tag);
  }
  return SIMPLICITY_NO_ERROR;
}

/* Decode a length-prefixed Simplicity DAG from 'stream'.
 * Returns 'SIMPLICITY_ERR_DATA_OUT_OF_RANGE' the length prefix's value is too large.
 * Returns 'SIMPLICITY_ERR_DATA_OUT_OF_RANGE' if some node's child isn't a reference to one of the preceding nodes.
 * Returns 'SIMPLICITY_ERR_FAIL_CODE' if the encoding of a fail expression is encountered
 *  (all fail subexpressions ought to have been pruned prior to deserialization).
 * Returns 'SIMPLICITY_ERR_RESERVED_CODE' if a reserved codeword is encountered.
 * Returns 'SIMPLICITY_ERR_HIDDEN' if the decoded node has a HIDDEN child in a position where it is not allowed.
 * Returns 'SIMPLICITY_ERR_HIDDEN_ROOT' if the root of the DAG is a HIDDEN node.
 * Returns 'SIMPLICITY_ERR_BITSTRING_EOF' if not enough bits are available in the 'stream'.
 * Returns 'SIMPLICITY_ERR_DATA_OUT_OF_ORDER' if nodes are not serialized in the canonical order.
 * Returns 'SIMPLICITY_ERR_MALLOC' if malloc fails.
 * In the above error cases, '*dag' is set to NULL.
 * If successful, returns a positive value equal to the length of an allocated array of (*dag).
 *
 * Precondition: NULL != dag
 *               NULL != stream
 *
 * Postcondition: if the return value of the function is positive
 *                  then (dag_node (*dag)[return_value] and '*dag' is a well-formed dag without witness data);
 *                '*census' contains a tally of the different tags that occur in 'dag' when the return value
 *                          of the function is positive and when NULL != census;
 *                NULL == *dag when the return value is negative.
 */
int_fast32_t simplicity_decodeMallocDag(dag_node** dag, simplicity_callback_decodeJet decodeJet, combinator_counters* census, bitstream* stream) {
  *dag = NULL;
  int32_t dagLen = simplicity_decodeUptoMaxInt(stream);
  if (dagLen <= 0) return dagLen;
  static_assert(DAG_LEN_MAX <= (uint32_t)INT32_MAX, "DAG_LEN_MAX exceeds supported parsing range.");
  if (DAG_LEN_MAX < (uint32_t)dagLen) return SIMPLICITY_ERR_DATA_OUT_OF_RANGE;
  static_assert(DAG_LEN_MAX <= SIZE_MAX / sizeof(dag_node), "dag array too large.");
  static_assert(1 <= DAG_LEN_MAX, "DAG_LEN_MAX is zero.");
  static_assert(DAG_LEN_MAX - 1 <= UINT32_MAX, "dag array index does not fit in uint32_t.");
  *dag = simplicity_malloc((size_t)dagLen * sizeof(dag_node));
  if (!*dag) return SIMPLICITY_ERR_MALLOC;

  if (census) *census = (combinator_counters){0};
  simplicity_err error = decodeDag(*dag, decodeJet, (uint_fast32_t)dagLen, census, stream);

  if (IS_OK(error)) {
    error = HIDDEN == (*dag)[dagLen - 1].tag
          ? SIMPLICITY_ERR_HIDDEN_ROOT
          : simplicity_verifyCanonicalOrder(*dag, (uint_fast32_t)(dagLen));
  }

  if (IS_OK(error)) {
    return dagLen;
  } else {
    simplicity_free(*dag);
    *dag = NULL;
    return (int_fast32_t)error;
  }
}
