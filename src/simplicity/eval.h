/* This module provides functions for evaluating Simplicity programs and expressions.
 */
#ifndef SIMPLICITY_EVAL_H
#define SIMPLICITY_EVAL_H

#include "bounded.h"
#include "dag.h"

typedef unsigned char flags_type;
#define CHECK_NONE 0
#define CHECK_EXEC 0x10
#define CHECK_CASE 0x60
#define CHECK_ALL ((flags_type)(-1))

/* Given a well-typed dag representing a Simplicity expression, compute the memory and CPU requirements for evaluation.
 *
 * If 'malloc' fails, then returns SIMPLICITY_ERR_MALLOC.
 * When maxCells < UBOUNDED_MAX, if the bounds on the number of cells needed for evaluation of 'dag' on an idealized Bit Machine exceeds maxCells,
 * then return SIMPLICITY_ERR_EXEC_MEMORY.
 * When maxCost < UBOUNDED_MAX, if the bounds on the dag's CPU cost exceeds 'maxCost', then return SIMPLICITY_ERR_EXEC_BUDGET.
 * If the bounds on the dag's CPU cost is less than or equal to 'minCost', then return SIMPLICITY_ERR_OVERWEIGHT.
 * Otherwise returns SIMPLICITY_NO_ERR.
 *
 * Precondition: NULL != cellsBound
 *               NULL != UWORDBound
 *               NULL != frameBound
 *               NULL != costBound
 *               dag_node dag[len] and 'dag' is well-typed with 'type_dag'.
 * Postcondition: if the result is 'SIMPLICITY_NO_ERR'
 *                then if maxCost < UBOUNDED_MAX then '*costBound' bounds the dag's CPU cost measured in milli weight units
 *                 and if maxCells < UBOUNDED_MAX then '*cellsBound' bounds the number of cells needed for evaluation of 'dag' on an idealized Bit Machine
 *                 and if maxCells < UBOUNDED_MAX then '*UWORDBound' bounds the number of UWORDs needed for the frames during evaluation of 'dag'
 *                 and if maxCells < UBOUNDED_MAX then '*frameBound' bounds the number of stack frames needed during execution of 'dag'.
 */
simplicity_err simplicity_analyseBounds( ubounded *cellsBound, ubounded *UWORDBound, ubounded *frameBound, ubounded *costBound
                                       , ubounded maxCells, ubounded minCost, ubounded maxCost, const dag_node* dag, const type* type_dag, const size_t len);

/* Run the Bit Machine on the well-typed Simplicity expression 'dag[len]' of type A |- B.
 * If bitSize(A) > 0, initialize the active read frame's data with 'input[ROUND_UWORD(bitSize(A))]'.
 *
 * If malloc fails, returns 'SIMPLICITY_ERR_MALLOC'.
 * When a budget is given, if static analysis results determines the bound on cpu requirements exceed the allowed budget, returns 'SIMPLICITY_ERR_EXEC_BUDGET'.
 * If static analysis results determines the bound on cpu requirements is less than or equal to the minCost, returns 'SIMPLICITY_ERR_OVERWEIGHT'.
 * If static analysis results determines the bound on memory allocation requirements exceed the allowed limits, returns 'SIMPLICITY_ERR_EXEC_MEMORY'.
 * If during execution some jet execution fails, returns 'SIMPLICITY_ERR_EXEC_JET'.
 * If during execution some 'assertr' or 'assertl' combinator fails, returns 'SIMPLICITY_ERR_EXEC_ASESRT'.
 *
 * Note that minCost and budget parameters are in WU, while the cost analysis will be performed in milliWU.
 * Thus the minCost and budget specify a half open interval (minCost, budget] of acceptable cost values in milliWU.
 * Setting minCost to 0 effectively disables the minCost check as every Simplicity program has a non-zero cost analysis.
 *
 * If none of the above conditions fail and bitSize(B) > 0, then a copy the final active write frame's data is written to 'output[roundWord(bitSize(B))]'.
 *
 * If 'anti_dos_checks' includes the 'CHECK_EXEC' flag, and not every non-HIDDEN dag node is executed, returns 'SIMPLICITY_ERR_ANTIDOS'
 * If 'anti_dos_checks' includes the 'CHECK_CASE' flag, and not every case node has both branches executed, returns 'SIMPLICITY_ERR_ANTIDOS'
 *
 * Otherwise 'SIMPLICITY_NO_ERROR' is returned.
 *
 * Precondition: dag_node dag[len] and 'dag' is well-typed with 'type_dag' for an expression of type A |- B;
 *               bitSize(A) == 0 or UWORD input[ROUND_UWORD(bitSize(A))];
 *               bitSize(B) == 0 or UWORD output[ROUND_UWORD(bitSize(B))];
 *               if NULL != budget then *budget <= BUDGET_MAX
 *               if NULL != budget then minCost <= *budget
 *               minCost <= BUDGET_MAX
 *               if 'dag[len]' represents a Simplicity expression with primitives then 'NULL != env';
 */
simplicity_err simplicity_evalTCOExpression( flags_type anti_dos_checks, UWORD* output, const UWORD* input
                                           , const dag_node* dag, type* type_dag, size_t len, ubounded minCost, const ubounded* budget, const txEnv* env
                                           );

/* Run the Bit Machine on the well-typed Simplicity program 'dag[len]'.
 *
 * If malloc fails, returns 'SIMPLICITY_ERR_MALLOC'.
 * When a budget is given, if static analysis results determines the bound on cpu requirements exceed the allowed budget, returns 'SIMPLICITY_ERR_EXEC_BUDGET'.
 * If static analysis results determines the bound on cpu requirements is less than or equal to the minCost, returns 'SIMPLICITY_ERR_OVERWEIGHT'.
 * If static analysis results determines the bound on memory allocation requirements exceed the allowed limits, returns 'SIMPLICITY_ERR_EXEC_MEMORY'.
 * If during execution some jet execution fails, returns 'SIMPLICITY_ERR_EXEC_JET'.
 * If during execution some 'assertr' or 'assertl' combinator fails, returns 'SIMPLICITY_ERR_EXEC_ASESRT'.
 *
 * Note that minCost and budget parameters are in WU, while the cost analysis will be performed in milliWU.
 * Thus the minCost and budget specify a half open interval (minCost, budget] of acceptable cost values in milliWU.
 * Setting minCost to 0 effectively disables the minCost check as every Simplicity program has a non-zero cost analysis.
 *
 * If not every non-HIDDEN dag node is executed, returns 'SIMPLICITY_ERR_ANTIDOS'
 * If not every case node has both branches executed, returns 'SIMPLICITY_ERR_ANTIDOS'
 *
 * Otherwise 'SIMPLICITY_NO_ERROR' is returned.
 *
 * Precondition: dag_node dag[len] and 'dag' is well-typed with 'type_dag' for an expression of type ONE |- ONE;
 *               if NULL != budget then *budget <= BUDGET_MAX
 *               if NULL != budget then minCost <= *budget
 *               minCost <= BUDGET_MAX
 *               if 'dag[len]' represents a Simplicity expression with primitives then 'NULL != env';
 */
static inline simplicity_err evalTCOProgram(const dag_node* dag, type* type_dag, size_t len, ubounded minCost, const ubounded* budget, const txEnv* env) {
  return simplicity_evalTCOExpression(CHECK_ALL, NULL, NULL, dag, type_dag, len, minCost, budget, env);
}
#endif
