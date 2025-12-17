#include <simplicity/elements/exec.h>

#include <stdalign.h>
#include <string.h>
#include "primitive.h"
#include "txEnv.h"
#include "../deserialize.h"
#include "../eval.h"
#include "../limitations.h"
#include "../simplicity_alloc.h"
#include "../simplicity_assert.h"
#include "../typeInference.h"

/* Deserialize a Simplicity 'program' with its 'witness' data and execute it in the environment of the 'ix'th input of 'tx' with `taproot`.
 *
 * If at any time malloc fails then '*error' is set to 'SIMPLICITY_ERR_MALLOC' and 'false' is returned,
 * meaning we were unable to determine the result of the simplicity program.
 * Otherwise, 'true' is returned indicating that the result was successfully computed and returned in the '*error' value.
 *
 * If deserialization, analysis, or execution fails, then '*error' is set to some simplicity_err.
 * In particular, if the cost analysis exceeds the budget, or exceeds BUDGET_MAX, then '*error' is set to 'SIMPLICITY_ERR_EXEC_BUDGET'.
 * On the other hand, if the cost analysis is less than or equal to minCost, then '*error' is set to 'SIMPLICITY_ERR_OVERWEIGHT'.
 *
 * Note that minCost and budget parameters are in WU, while the cost analysis will be performed in milliWU.
 * Thus the minCost and budget specify a half open interval (minCost, budget] of acceptable cost values in milliWU.
 * Setting minCost to 0 effectively disables the minCost check as every Simplicity program has a non-zero cost analysis.
 *
 * If 'amr != NULL' and the annotated Merkle root of the decoded expression doesn't match 'amr' then '*error' is set to 'SIMPLICITY_ERR_AMR'.
 *
 * Otherwise '*error' is set to 'SIMPLICITY_NO_ERROR'.
 *
 * If 'ihr != NULL' and '*error' is set to 'SIMPLICITY_NO_ERROR', then the identity hash of the root of the decoded expression is written to 'ihr'.
 * Otherwise if 'ihr != NULL'  and '*error' is not set to 'SIMPLCITY_NO_ERROR', then 'ihr' may or may not be written to.
 *
 * Precondition: NULL != error;
 *               NULL != ihr implies unsigned char ihr[32]
 *               NULL != tx;
 *               NULL != taproot;
 *               unsigned char genesisBlockHash[32]
 *               0 <= minCost <= budget;
 *               NULL != amr implies unsigned char amr[32]
 *               unsigned char program[program_len]
 *               unsigned char witness[witness_len]
 */
extern bool simplicity_elements_execSimplicity( simplicity_err* error, unsigned char* ihr
                                              , const elementsTransaction* tx, uint_fast32_t ix, const elementsTapEnv* taproot
                                              , const unsigned char* genesisBlockHash
                                              , int64_t minCost, int64_t budget
                                              , const unsigned char* amr
                                              , const unsigned char* program, size_t program_len
                                              , const unsigned char* witness, size_t witness_len) {
  simplicity_assert(NULL != error);
  simplicity_assert(NULL != tx);
  simplicity_assert(NULL != taproot);
  simplicity_assert(NULL != genesisBlockHash);
  simplicity_assert(0 <= minCost);
  simplicity_assert(minCost <= budget);
  simplicity_assert(NULL != program || 0 == program_len);
  simplicity_assert(NULL != witness || 0 == witness_len);

  combinator_counters census;
  dag_node* dag = NULL;
  int_fast32_t dag_len;
  sha256_midstate amr_hash, genesis_hash;

  if (amr) sha256_toMidstate(amr_hash.s, amr);
  sha256_toMidstate(genesis_hash.s, genesisBlockHash);

  {
    bitstream stream = initializeBitstream(program, program_len);
    dag_len = simplicity_decodeMallocDag(&dag, simplicity_elements_decodeJet, &census, &stream);
    if (dag_len <= 0) {
      simplicity_assert(dag_len < 0);
      *error = (simplicity_err)dag_len;
      return IS_PERMANENT(*error);
    }
    simplicity_assert(NULL != dag);
    simplicity_assert((uint_fast32_t)dag_len <= DAG_LEN_MAX);
    *error = simplicity_closeBitstream(&stream);
  }

  if (IS_OK(*error)) {
    if (0 != memcmp(taproot->scriptCMR.s, dag[dag_len-1].cmr.s, sizeof(uint32_t[8]))) {
      *error = SIMPLICITY_ERR_CMR;
    }
  }

  if (IS_OK(*error)) {
    type* type_dag = NULL;
    *error = simplicity_mallocTypeInference(&type_dag, simplicity_elements_mallocBoundVars, dag, (uint_fast32_t)dag_len, &census);
    if (IS_OK(*error)) {
      simplicity_assert(NULL != type_dag);
      if (0 != dag[dag_len-1].sourceType || 0 != dag[dag_len-1].targetType) {
        *error = SIMPLICITY_ERR_TYPE_INFERENCE_NOT_PROGRAM;
      }
    }
    if (IS_OK(*error)) {
      bitstream witness_stream = initializeBitstream(witness, witness_len);
      *error = simplicity_fillWitnessData(dag, type_dag, (uint_fast32_t)dag_len, &witness_stream);
      if (IS_OK(*error)) {
        *error = simplicity_closeBitstream(&witness_stream);
        if (SIMPLICITY_ERR_BITSTREAM_TRAILING_BYTES == *error) *error = SIMPLICITY_ERR_WITNESS_TRAILING_BYTES;
        if (SIMPLICITY_ERR_BITSTREAM_ILLEGAL_PADDING == *error) *error = SIMPLICITY_ERR_WITNESS_ILLEGAL_PADDING;
      }
    }
    if (IS_OK(*error)) {
      sha256_midstate ihr_buf;
      *error = simplicity_verifyNoDuplicateIdentityHashes(&ihr_buf, dag, type_dag, (uint_fast32_t)dag_len);
      if (IS_OK(*error) && ihr) sha256_fromMidstate(ihr, ihr_buf.s);
    }
    if (IS_OK(*error) && amr) {
      static_assert(DAG_LEN_MAX <= SIZE_MAX / sizeof(analyses), "analysis array too large.");
      static_assert(1 <= DAG_LEN_MAX, "DAG_LEN_MAX is zero.");
      static_assert(DAG_LEN_MAX - 1 <= UINT32_MAX, "analysis array index does nto fit in uint32_t.");
      analyses *analysis = simplicity_malloc((size_t)dag_len * sizeof(analyses));
      if (analysis) {
        simplicity_computeAnnotatedMerkleRoot(analysis, dag, type_dag, (uint_fast32_t)dag_len);
        if (0 != memcmp(amr_hash.s, analysis[dag_len-1].annotatedMerkleRoot.s, sizeof(uint32_t[8]))) {
          *error = SIMPLICITY_ERR_AMR;
        }
      } else {
        /* malloc failed which counts as a transient error. */
        *error = SIMPLICITY_ERR_MALLOC;
      }
      simplicity_free(analysis);
    }
    if (IS_OK(*error)) {
      txEnv env = simplicity_elements_build_txEnv(tx, taproot, &genesis_hash, ix);
      static_assert(BUDGET_MAX <= UBOUNDED_MAX, "BUDGET_MAX doesn't fit in ubounded.");
      *error = evalTCOProgram( dag, type_dag, (size_t)dag_len
                             , minCost <= BUDGET_MAX ? (ubounded)minCost : BUDGET_MAX
                             , &(ubounded){budget <= BUDGET_MAX ? (ubounded)budget : BUDGET_MAX}
                             , &env);
    }
    simplicity_free(type_dag);
  }

  simplicity_free(dag);
  return IS_PERMANENT(*error);
}
