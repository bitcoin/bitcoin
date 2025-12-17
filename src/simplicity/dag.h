/* This module defines the structure for Simplicity DAGs, and functions for some analysis of that structure,
 * such as computing Merkle Roots.
 */
#ifndef SIMPLICITY_DAG_H
#define SIMPLICITY_DAG_H

#include <stddef.h>
#include <stdint.h>
#include <simplicity/errorCodes.h>
#include "bitstream.h"
#include "bitstring.h"
#include "bounded.h"
#include "jets.h"
#include "simplicity_assert.h"
#include "type.h"

/* An enumeration of the various kinds of Simplicity nodes. */
typedef enum tag_t
{ COMP
, CASE
, ASSERTL
, ASSERTR
, PAIR
, DISCONNECT
, INJL
, INJR
, TAKE
, DROP
, IDEN
, UNIT
, HIDDEN
, WITNESS
, JET
, WORD
} tag_t;

/* This structure is use to count the different kinds of combinators in a Simplicity DAG. */
typedef struct combinator_counters {
  size_t comp_cnt, case_cnt, pair_cnt, disconnect_cnt,
         injl_cnt, injr_cnt, take_cnt, drop_cnt;
} combinator_counters;

/* Given a tag for an expression, add it to the 'census'.
 */
static inline void enumerator(combinator_counters* census, tag_t tag) {
  if (!census) return;
  switch (tag) {
   case COMP: census->comp_cnt++; return;
   case ASSERTL:
   case ASSERTR: /* Assertions are counted as CASE combinators. */
   case CASE: census->case_cnt++; return;
   case PAIR: census->pair_cnt++; return;
   case DISCONNECT: census->disconnect_cnt++; return;
   case INJL: census->injl_cnt++; return;
   case INJR: census->injr_cnt++; return;
   case TAKE: census->take_cnt++; return;
   case DROP: census->drop_cnt++; return;

   /* These tags are not accounted for in the census. */
   case IDEN:
   case UNIT:
   case HIDDEN:
   case WITNESS:
   case JET:
   case WORD:
    return;
  }
}

/* Returns the number of children that a Simplicity combinator of the 'tag' kind has.
 */
static inline size_t numChildren(tag_t tag) {
  switch (tag) {
   case COMP:
   case ASSERTL:
   case ASSERTR:
   case CASE:
   case PAIR:
   case DISCONNECT:
    return 2;
   case INJL:
   case INJR:
   case TAKE:
   case DROP:
    return 1;
   case IDEN:
   case UNIT:
   case HIDDEN:
   case WITNESS:
   case JET:
   case WORD:
    return 0;
  }
}

/* Compute the CMR of a jet of scribe(v) : ONE |- TWO^(2^n) that outputs a given bitstring.
 *
 * Precondition: 2^n == value->len
 */
sha256_midstate simplicity_computeWordCMR(const bitstring* value, size_t n);

/* A node the the DAG of a Simplicity expression.
 * It consists of a 'tag' indicating the kind of expression the node represents.
 * The contents of a node depend on the kind of the expressions.
 * The node may have references to children, when it is a combinator kind of expression.
 *
 * Invariant: 'NULL != jet' when 'tag == JET';
 *            bitstring compactValue is active when tag == WITNESS and the node has witness data;
 *            bitstring compactValue is also active and has a length that is a power of 2 when tag == WORD;
 *            size_t sourceIx is active when tag == JET;
 *            unbounded cost is active when tag == JET;
 *            size_t child[numChildren(tag)] when tag \notin {HIDDEN, WITNESS, JET, WORD};
 */
typedef struct dag_node {
  jet_ptr jet;
  sha256_midstate cmr;
  union {
    uint_fast32_t aux; /* Used as scratch space for verifyCanonicalOrder. */
    struct {
       size_t sourceType, targetType;
    };
  };
  union {
    struct {
      size_t sourceIx;
    };
    struct {
      uint_fast32_t child[2];
    };
    bitstring compactValue;
  };
  size_t targetIx;
  /* cost is normalized so that the 'CheckSigVerify' jet has 50 000 milli weight units.  */
  ubounded cost; /* in milli weight units */
  tag_t tag;
} dag_node;

/* Inline functions for accessing the type annotations of combinators */
static inline size_t IDEN_A(const dag_node* dag, const type* type_dag, size_t i) {
  (void)type_dag;
  simplicity_debug_assert(IDEN == dag[i].tag);
  return dag[i].sourceType;
}

static inline size_t UNIT_A(const dag_node* dag, const type* type_dag, size_t i) {
  (void)type_dag;
  simplicity_debug_assert(UNIT == dag[i].tag);
  return dag[i].sourceType;
}

static inline size_t COMP_A(const dag_node* dag, const type* type_dag, size_t i) {
  (void)type_dag;
  simplicity_debug_assert(COMP == dag[i].tag);
  return dag[i].sourceType;
}

static inline size_t COMP_B(const dag_node* dag, const type* type_dag, size_t i) {
  (void)type_dag;
  simplicity_debug_assert(COMP == dag[i].tag);
  return dag[dag[i].child[1]].sourceType;
}

static inline size_t COMP_C(const dag_node* dag, const type* type_dag, size_t i) {
  (void)type_dag;
  simplicity_debug_assert(COMP == dag[i].tag);
  return dag[i].targetType;
}

static inline size_t CASE_A(const dag_node* dag, const type* type_dag, size_t i) {
  simplicity_debug_assert(CASE == dag[i].tag || ASSERTL == dag[i].tag || ASSERTR == dag[i].tag);
  simplicity_debug_assert(PRODUCT == type_dag[dag[i].sourceType].kind);
  simplicity_debug_assert(SUM == type_dag[type_dag[dag[i].sourceType].typeArg[0]].kind);
  return type_dag[type_dag[dag[i].sourceType].typeArg[0]].typeArg[0];
}

static inline size_t CASE_B(const dag_node* dag, const type* type_dag, size_t i) {
  simplicity_debug_assert(CASE == dag[i].tag || ASSERTL == dag[i].tag || ASSERTR == dag[i].tag);
  simplicity_debug_assert(PRODUCT == type_dag[dag[i].sourceType].kind);
  simplicity_debug_assert(SUM == type_dag[type_dag[dag[i].sourceType].typeArg[0]].kind);
  return type_dag[type_dag[dag[i].sourceType].typeArg[0]].typeArg[1];
}

static inline size_t CASE_C(const dag_node* dag, const type* type_dag, size_t i) {
  simplicity_debug_assert(CASE == dag[i].tag || ASSERTL == dag[i].tag || ASSERTR == dag[i].tag);
  simplicity_debug_assert(PRODUCT == type_dag[dag[i].sourceType].kind);
  return type_dag[dag[i].sourceType].typeArg[1];
}

static inline size_t CASE_D(const dag_node* dag, const type* type_dag, size_t i) {
  (void)type_dag;
  simplicity_debug_assert(CASE == dag[i].tag || ASSERTL == dag[i].tag || ASSERTR == dag[i].tag);
  return dag[i].targetType;
}

static inline size_t PAIR_A(const dag_node* dag, const type* type_dag, size_t i) {
  (void)type_dag;
  simplicity_debug_assert(PAIR == dag[i].tag);
  return dag[i].sourceType;
}

static inline size_t PAIR_B(const dag_node* dag, const type* type_dag, size_t i) {
  simplicity_debug_assert(PAIR == dag[i].tag);
  simplicity_debug_assert(PRODUCT == type_dag[dag[i].targetType].kind);
  return type_dag[dag[i].targetType].typeArg[0];
}

static inline size_t PAIR_C(const dag_node* dag, const type* type_dag, size_t i) {
  simplicity_debug_assert(PAIR == dag[i].tag);
  simplicity_debug_assert(PRODUCT == type_dag[dag[i].targetType].kind);
  return type_dag[dag[i].targetType].typeArg[1];
}

static inline size_t DISCONNECT_A(const dag_node* dag, const type* type_dag, size_t i) {
  (void)type_dag;
  simplicity_debug_assert(DISCONNECT == dag[i].tag);
  return dag[i].sourceType;
}

static inline size_t DISCONNECT_B(const dag_node* dag, const type* type_dag, size_t i) {
  simplicity_debug_assert(DISCONNECT == dag[i].tag);
  simplicity_debug_assert(PRODUCT == type_dag[dag[i].targetType].kind);
  return type_dag[dag[i].targetType].typeArg[0];
}

static inline size_t DISCONNECT_C(const dag_node* dag, const type* type_dag, size_t i) {
  (void)type_dag;
  simplicity_debug_assert(DISCONNECT == dag[i].tag);
  return dag[dag[i].child[1]].sourceType;
}

static inline size_t DISCONNECT_D(const dag_node* dag, const type* type_dag, size_t i) {
  simplicity_debug_assert(DISCONNECT == dag[i].tag);
  simplicity_debug_assert(PRODUCT == type_dag[dag[i].targetType].kind);
  return type_dag[dag[i].targetType].typeArg[1];
}

static inline size_t DISCONNECT_W256A(const dag_node* dag, const type* type_dag, size_t i) {
  (void)type_dag;
  simplicity_debug_assert(DISCONNECT == dag[i].tag);
  return dag[dag[i].child[0]].sourceType;
}

static inline size_t DISCONNECT_BC(const dag_node* dag, const type* type_dag, size_t i) {
  (void)type_dag;
  simplicity_debug_assert(DISCONNECT == dag[i].tag);
  return dag[dag[i].child[0]].targetType;
}

static inline size_t INJ_A(const dag_node* dag, const type* type_dag, size_t i) {
  (void)type_dag;
  simplicity_debug_assert(INJL == dag[i].tag || INJR == dag[i].tag);
  return dag[i].sourceType;
}

static inline size_t INJ_B(const dag_node* dag, const type* type_dag, size_t i) {
  simplicity_debug_assert(INJL == dag[i].tag || INJR == dag[i].tag);
  simplicity_debug_assert(SUM == type_dag[dag[i].targetType].kind);
  return type_dag[dag[i].targetType].typeArg[0];
}

static inline size_t INJ_C(const dag_node* dag, const type* type_dag, size_t i) {
  simplicity_debug_assert(INJL == dag[i].tag || INJR == dag[i].tag);
  simplicity_debug_assert(SUM == type_dag[dag[i].targetType].kind);
  return type_dag[dag[i].targetType].typeArg[1];
}

static inline size_t PROJ_A(const dag_node* dag, const type* type_dag, size_t i) {
  simplicity_debug_assert(TAKE == dag[i].tag || DROP == dag[i].tag);
  simplicity_debug_assert(PRODUCT == type_dag[dag[i].sourceType].kind);
  return type_dag[dag[i].sourceType].typeArg[0];
}

static inline size_t PROJ_B(const dag_node* dag, const type* type_dag, size_t i) {
  simplicity_debug_assert(TAKE == dag[i].tag || DROP == dag[i].tag);
  simplicity_debug_assert(PRODUCT == type_dag[dag[i].sourceType].kind);
  return type_dag[dag[i].sourceType].typeArg[1];
}

static inline size_t PROJ_C(const dag_node* dag, const type* type_dag, size_t i) {
  (void)type_dag;
  simplicity_debug_assert(TAKE == dag[i].tag || DROP == dag[i].tag);
  return dag[i].targetType;
}

static inline size_t WITNESS_A(const dag_node* dag, const type* type_dag, size_t i) {
  (void)type_dag;
  simplicity_debug_assert(WITNESS == dag[i].tag);
  return dag[i].sourceType;
}

static inline size_t WITNESS_B(const dag_node* dag, const type* type_dag, size_t i) {
  (void)type_dag;
  simplicity_debug_assert(WITNESS == dag[i].tag);
  return dag[i].targetType;
}

/* A well-formed Simplicity DAG is an array of 'dag_node's,
 *
 *     dag_node dag[len],
 *
 * such that
 *
 *     1 <= len <= DAG_LEN_MAX
 *
 * and for all 'i', 0 <= 'i' < 'len' and for all 'j', 0 <= 'j' < 'numChildren(dag[i].tag)',
 *
 *     dag[i].child[j] < i
 *
 * and for all 'i', 0 <= 'i' < 'len',
 *
 *     if dag[dag[i].child[0]].tag == HIDDEN then dag[i].tag == ASSERTR
 *
 *     and
 *
 *     if dag[dag[i].child[1]].tag == HIDDEN then dag[i].tag == ASSERTL
 *
 * Note that a well-formed Simplicity DAG is not necessarily a well-typed Simplicity DAG.
 */

/* A structure of static analyses for a particular node of a Simplicity DAG.
 * 'annotatedMerkleRoot' is the a Merkle root to includes the type annotations of all subexpressions.
 */
typedef struct analyses {
  sha256_midstate annotatedMerkleRoot;
} analyses;

/* Given a well-formed dag[i + 1], such that for all 'j', 0 <= 'j' < 'i',
 * 'dag[j].cmr' is the CMR of the subexpression denoted by the slice
 *
 *     (dag_nodes[j + 1])dag,
 *
 * then we set the value of 'dag[i].cmr' to be the CMR of the subexpression denoted by 'dag'.
 *
 * Precondition: dag_node dag[i + 1] and 'dag' is well-formed.
 *               dag[i].'tag' \notin {HIDDEN, JET, WORD}
 */
void simplicity_computeCommitmentMerkleRoot(dag_node* dag, uint_fast32_t i);

/* Given a well-typed dag representing a Simplicity expression, compute the annotated Merkle roots of all subexpressions.
 * For all 'i', 0 <= 'i' < 'len', 'analysis[i].annotatedMerkleRoot' will be the AMR of the subexpression denoted by the slice
 *
 *     (dag_nodes[i + 1])dag.
 *
 * The AMR of the overall expression will be 'analysis[len - 1].annotatedMerkleRoot'.
 *
 * Precondition: analyses analysis[len];
 *               dag_node dag[len] and 'dag' has witness data and is well-typed with 'type_dag'.
 * Postconditon: analyses analysis[len] contains the annotated Merkle roots of each subexpressions of 'dag'.
 */
void simplicity_computeAnnotatedMerkleRoot(analyses* analysis, const dag_node* dag, const type* type_dag, uint_fast32_t len);

/* Verifies that the 'dag' is in canonical order, meaning that nodes under the left branches have lower indices than nodes under
 * right branches, with the exception that nodes under right branches may (cross-)reference identical nodes that already occur under
 * left branches.
 *
 * Returns 'SIMPLICITY_NO_ERROR' if the 'dag' is in canonical order, and returns 'SIMPLICITY_ERR_DATA_OUT_OF_ORDER' if it is not.
 *
 * May modify dag[i].aux values and invalidate dag[i].sourceType and dag[i].targetType.
 * This function should only be used prior to calling 'mallocTypeInference'.
 *
 * Precondition: dag_node dag[len] and 'dag' is well-formed.
 */
simplicity_err simplicity_verifyCanonicalOrder(dag_node* dag, const uint_fast32_t len);

/* This function fills in the 'WITNESS' nodes of a 'dag' with the data from 'witness'.
 * For each 'WITNESS' : A |- B expression in 'dag', the bits from the 'witness' bitstring are decoded in turn
 * to construct a compact representation of a witness value of type B.
 * If there are not enough bits, then 'SIMPLICITY_ERR_WITNESS_EOF' is returned.
 * If a witness node would end up with more than CELLS_MAX bits, then 'SIMPLICITY_ERR_EXEC_MEMORY' is returned.
 * Otherwise, returns 'SIMPLICITY_NO_ERROR'.
 *
 * Precondition: dag_node dag[len] and 'dag' without witness data and is well-typed with 'type_dag';
 *               witness is a valid bitstream;
 *
 * Postcondition: dag_node dag[len] and 'dag' has witness data and is well-typed with 'type_dag'
 *                  when the result is 'SIMPLICITY_NO_ERROR';
 */
simplicity_err simplicity_fillWitnessData(dag_node* dag, type* type_dag, const uint_fast32_t len, bitstream *witness);

/* Verifies that identity hash of every subexpression in a well-typed 'dag' with witnesses are all unique,
 * including that each hidden root hash for every 'HIDDEN' node is unique.
 *
 * if 'ihr' is not NULL, then '*ihr' is set to the identity hash of the root of the 'dag'.
 *
 * If malloc fails, returns 'SIMPLICITY_ERR_MALLOC'.
 * If all the identity hahes (and hidden roots) are all unique, returns 'SIMPLICITY_NO_ERROR'.
 * Otherwise returns 'SIMPLICITY_ERR_UNSHARED_SUBEXPRESSION'.
 *
 * Precondition: dag_node dag[len] and 'dag' is well-typed with 'type_dag' and contains witnesses.
 */
simplicity_err simplicity_verifyNoDuplicateIdentityHashes(sha256_midstate* ihr, const dag_node* dag, const type* type_dag, const uint_fast32_t dag_len);

#endif
