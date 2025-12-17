#include <simplicity/bitcoin/cmr.h>

#include "../cmr.h"
#include "primitive.h"

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
bool simplicity_bitcoin_computeCmr( simplicity_err* error, unsigned char* cmr
                                   , const unsigned char* program, size_t program_len) {
  return simplicity_computeCmr(error, cmr, simplicity_bitcoin_decodeJet, program, program_len);
}
