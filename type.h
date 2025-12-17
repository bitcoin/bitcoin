/* This module defines the structure for Simplicity type DAGs and computes type Merkle roots. */
#ifndef SIMPLICITY_TYPE_H
#define SIMPLICITY_TYPE_H

#include <stddef.h>
#include "bounded.h"
#include "sha256.h"
#include "simplicity_assert.h"

typedef enum typeName
  { ONE, SUM, PRODUCT }
typeName;

/* A structure for Simplicity type DAGs. */
/* :TODO: when 'back' is used as "mutable" scratch space, concurrent evaluation of Simplicity expressions that share
 * subexpressions and typing information is not possible.
 * Consider instead implementing static analysis to determine a maximum stack size needed for traversing types in an expression
 * in order to remove uses of 'back'.
 */
typedef struct type {
  size_t typeArg[2];
  union {
    size_t skip; /* Used by 'typeSkip'. */
    size_t back; /* Sometimes used as scratch space when traversing types. */
  };
  sha256_midstate typeMerkleRoot;
  ubounded bitSize;
  typeName kind;
} type;

/* We say a Simplicity type is trivial if it is the 'ONE' type, or the 'PRODUCT' of two trivial types.
 * A Simplicity type is a trivial type if and only if its bitSize is 0.
 */

/* A well-formed type DAG is an array of 'type's,
 *
 *     type type_dag[n],
 *
 * such that
 *
 *     0 < n
 *
 * and
 *
 *     type_dag[0].kind == ONE.
 *
 * and for all 'i', 1 <= 'i' < 'n' and for all 'j', 0 <= 'j' < 2.
 *
 *     type_dag[i].kind \in { SUM, PRODUCT } and type_dag[i].typeArg[j] < i.
 */

/* Given a well-formed 'type_dag', compute the bitSizes, skips, and type Merkle roots of all subexpressions.
 * For all 'i', 0 <= 'i' < 'len',
 *   'type_dag[i].typeMerkleRoot' will be the TMR
 *   and 'type_dag[i].bitSize' will be the bitSize of the subexpression denoted by the slice
 *
 *     (type_dag[i + 1])type_dag.
 *
 *   and when 'type_dag[i]' represents a non-trivial 'PRODUCT' type, where one of the two type arguments a trivial type.
 *       then 'type_dag[i].skip' is the index of the largest subexpression of 'type_dag[i]' such that
 *        either 'type_dag[type_dag[i].skip]' is a 'SUM' type
 *            or 'type_dag[type_dag[i].skip]' is a 'PRODUCT' type of two non-trivial types.
 *
 * Precondition: type type_dag[len] and 'type_dag' is well-formed.
 */
void simplicity_computeTypeAnalyses(type* type_dag, size_t len);

/* Return the index of the largest subexpression of 'type_dag[i]' (possibly 'i' itself) that is either
 * (1) a 'SUM' type;
 * (2) a 'PRODUCT' type of two non-trivial type arguments.
 *
 * If there is no such subexpression at all, i.e. when 'type_dag[i]' is a trivial type, return 0
 * (which is the index of the 'ONE' type).
 *
 * This functions is used for fast traversals of type expressions, skipping over trivial subexpressions,
 * as this function runs in in constant time.
 */
static inline size_t typeSkip(size_t i, const type* type_dag) {
  if (0 == type_dag[i].bitSize) return 0;
  if (PRODUCT == type_dag[i].kind &&
      (0 == type_dag[type_dag[i].typeArg[0]].bitSize || 0 == type_dag[type_dag[i].typeArg[1]].bitSize)) {
    return type_dag[i].skip;
  }
  return i;
}

/* Precondition: type type_dag[i] and 'type_dag' is well-formed.
 *               if type_dag[i] is a non-trival 'PRODUCT', then both of its two type arguments are non-trival.
 * Postconditon: value == type_dag[i]
 */
static inline void setTypeBack(size_t i, type* type_dag, size_t value) {
  /* .back cannot be used if .skip is in use.
     Specifically it cannot be a non-trivial 'PRODUCT' type where one of its two type arguments is a trivial type.
   */
  simplicity_assert((PRODUCT != type_dag[i].kind ||
                     0 == type_dag[i].bitSize ||
                     (0 != type_dag[type_dag[i].typeArg[0]].bitSize &&
                      0 != type_dag[type_dag[i].typeArg[1]].bitSize)));
  type_dag[i].back = value;
}
#endif
