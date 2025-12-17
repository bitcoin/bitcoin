#ifndef SIMPLICITY_ELEMENTS_EXEC_H
#define SIMPLICITY_ELEMENTS_EXEC_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <simplicity/errorCodes.h>
#include <simplicity/elements/env.h>

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
                                              , const unsigned char* witness, size_t witness_len);
#endif
