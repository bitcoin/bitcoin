/* This module provides function for running monomorphic type inference over Simplicity DAGs. */
#ifndef SIMPLICITY_TYPEINFERENCE_H
#define SIMPLICITY_TYPEINFERENCE_H

#include "dag.h"
#include "type.h"

/* Forward declarations for recursive data structures. */
typedef struct unification_cont unification_cont;
typedef struct unification_var unification_var;

/* 'unification_cont' is a stack element holding a pair of variables to be unified.
 * 'next' points to the rest of the stack or NULL if this is the bottom of the stack.
 */
struct unification_cont {
  unification_var* alpha;
  unification_var* beta;
  unification_cont* next;
};

/* We say that a value 'cont' of type 'unification_cont*' is a stack when
 * (a) 'NULL == cont', in which case we say 'cont' is an empty stack,
 * or
 * (b) 'NULL != cont', 'NULL != cont->alpha', 'NULL != cont->beta', and 'cont->next' is a stack,
 * in which case we say 'cont' is a non-empty stack.
 */

/* A binding for a bound unification variable.
 * 'kind' is the kind of Simplicity type for this binding.
 * When 'kind' is 'ONE' then this is a called "trivial" binding and 'arg' is not used.
 * When 'kind' is in { 'SUM', 'PRODUCT' } then this is called a "non-trivial" binding
 * and 'arg[0]' and 'arg[1]' are pointers to variables for the type's arguments.
 *
 * During freezing, the 'occursCheck' flag may be set to help detect occurs check failures (a.k.a cyclic types).
 * After freezing, 'frozen_ix' refers to the index within some 'type' array that holds the frozen version of this binding.
 *
 * When a binding is unused (e.g. when a unification_var has a non-NULL 'parent'), unification may activate 'cont' as scratch space;
 */
typedef union binding {
  struct {
    unification_var* arg[2];
    size_t frozen_ix;
    typeName kind;
    bool occursCheck;
  };
  unification_cont cont; /* unification uses this field as scratch space. */
} binding;

/* A unification variable.
 * When 'NULL == parent' then this variable is the representative of its equivalence class.
 * When 'NULL == parent' and '!isBound' this (and all equivalent variables) is a free unification variable.
 * When 'NULL == parent' and 'isBound' this (and all equivalent variables) is a bound unification variable
 *   with 'bound' holding its binding (and bound.kind is active).
 * When 'NULL != parent' then this variable is equivalent to '*parent' and 'isBound' and 'bound' are not used.
 *
 * During unification 'rank' is active and when 'NULL != parent' then 'rank < parent->rank'.
 * Also when 'rank' is active, there are at least 2^'rank' unification variables in this unification variable's equivalence class.
 *
 * After unification is completed, the freeze function may activate 'next' as scratch space.
 *
 * This structure is designed so that initializing it with '{0}' / implicit static initialization
 * produces a fresh free unification variable.
 */
struct unification_var {
  unification_var* parent;
  binding bound;
  union {
    int rank;
    unification_var* next; /* freezing uses this field as scratch space. */
  };
  bool isBound;
};

/* Allocate a fresh set of unification variables bound to at least all the types necessary
 * for all the jets that can be created by 'simplicity_callbac_decodeJet', and also the type 'TWO^256',
 * and also allocate space for 'extra_var_len' many unification variables.
 * Return the number of non-trivial bindings created.
 *
 * However, if malloc fails, then return 0.
 *
 * Precondition: NULL != bound_var;
 *               NULL != word256_ix;
 *               NULL != extra_var_start;
 *               extra_var_len <= 6*DAG_LEN_MAX;
 *
 * Postcondition: Either '*bound_var == NULL' and the function returns 0
 *                or 'unification_var (*bound_var)[*extra_var_start + extra_var_len]' is an array of unification variables
 *                   such that for any 'jet : A |- B' there is some 'i < *extra_var_start' and 'j < *extra_var_start' such that
 *                      '(*bound_var)[i]' is bound to 'A' and '(*bound_var)[j]' is bound to 'B'
 *                   and, '*word256_ix < *extra_var_start' and '(*bound_var)[*word256_ix]' is bound the type 'TWO^256'
 */
typedef size_t (*simplicity_callback_mallocBoundVars)(unification_var** bound_var, size_t* word256_ix, size_t* extra_var_start, size_t extra_var_len);

/* If the Simplicity DAG, 'dag', has a principal type (including constraints due to sharing of subexpressions),
 * then allocate a well-formed type DAG containing all the types needed for all the subexpressions of 'dag',
 * with all free type variables instantiated at ONE, and set '*type_dag' to this allocation,
 * and update the '.sourceType' and '.targetType' fields within each node of the 'dag' 'type_dag[dag[i].sourceType]'
 * and 'type_dag[dag[i].targetType]' are the inferred types of the Simplicity subexpression at dag[i].
 *
 * If malloc fails, returns 'SIMPLICITY_ERR_MALLOC'.
 * If the Simplicity DAG, 'dag', has no principal type (because it has a type error), then '*type_dag' is set to NULL,
 * and either 'SIMPLICITY_ERR_TYPE_INFERENCE_UNIFICATION' or 'SIMPLICITY_ERR_TYPE_INFERENCE_OCCURS_CHECK' is returned.
 * Otherwise 'SIMPLICITY_NO_ERROR' is returned.
 *
 * Precondition: NULL != type_dag;
 *               dag_node dag[len] is well-formed;
 *               '*census' contains a tally of the different tags that occur in 'dag'.
 *
 * Postcondition: if the return value is 'SIMPLICITY_NO_ERROR'
 *                then either NULL == '*type_dag'
 *                     or 'dag' is well-typed with '*type_dag' and without witness values
 *                if the return value is not 'SIMPLICITY_NO_ERROR' then 'NULL == *type_dag'
 */
simplicity_err simplicity_mallocTypeInference(type** type_dag, simplicity_callback_mallocBoundVars mallocBoundVars, dag_node* dag, const uint_fast32_t len, const combinator_counters* census);

#endif
