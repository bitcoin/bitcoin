#include "cmr.h"

#include "limitations.h"
#include "simplicity_alloc.h"
#include "simplicity_assert.h"

/* Deserialize a Simplicity 'program' and compute its CMR.
 *
 * Caution: no typechecking is performed, only a well-formedness check.
 *
 * If at any time malloc fails then '*error' is set to 'SIMPLICITY_ERR_MALLOC' and 'false' is returned,
 * Otherwise, 'true' is returned indicating that the result was successfully computed and returned in the '*error' value.
 *
 * If the operation completes successfully then '*error' is set to 'SIMPLICITY_NO_ERROR', and the 'cmr' array is filled in with the program's computed CMR.
 *
 * Precondition: NULL != error;
 *               unsigned char cmr[32]
 *               unsigned char program[program_len]
 */
bool simplicity_computeCmr( simplicity_err* error, unsigned char* cmr, simplicity_callback_decodeJet decodeJet
                          , const unsigned char* program, size_t program_len) {
  simplicity_assert(NULL != error);
  simplicity_assert(NULL != cmr);
  simplicity_assert(NULL != program || 0 == program_len);

  bitstream stream = initializeBitstream(program, program_len);
  dag_node* dag = NULL;
  int_fast32_t dag_len = simplicity_decodeMallocDag(&dag, decodeJet, NULL, &stream);
  if (dag_len <= 0) {
    simplicity_assert(dag_len < 0);
    *error = (simplicity_err)dag_len;
  } else {
    simplicity_assert(NULL != dag);
    simplicity_assert((uint_fast32_t)dag_len <= DAG_LEN_MAX);
    *error = simplicity_closeBitstream(&stream);
    sha256_fromMidstate(cmr, dag[dag_len-1].cmr.s);
  }

  simplicity_free(dag);
  return IS_PERMANENT(*error);
}
