/* This module implements the 'primitive.h' interface for the Bitcoin application of Simplicity.
 */
#include "primitive.h"

#include "bitcoinJets.h"
#include "../limitations.h"
#include "../simplicity_alloc.h"
#include "../simplicity_assert.h"

/* An enumeration of all the types we need to construct to specify the input and output types of all jets created by 'decodeJet'. */
enum TypeNamesForJets {
#include "primitiveEnumTy.inc"
  NumberOfTypeNames
};

/* Allocate a fresh set of unification variables bound to at least all the types necessary
 * for all the jets that can be created by 'decodeJet', and also the type 'TWO^256',
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
size_t simplicity_bitcoin_mallocBoundVars(unification_var** bound_var, size_t* word256_ix, size_t* extra_var_start, size_t extra_var_len) {
  static_assert(1 <= NumberOfTypeNames, "Missing TypeNamesForJets.");
  static_assert(NumberOfTypeNames <= NUMBER_OF_TYPENAMES_MAX, "Too many TypeNamesForJets.");
  static_assert(DAG_LEN_MAX <= (SIZE_MAX - NumberOfTypeNames) / 6, "NumberOfTypeNames + 6*DAG_LEN_MAX doesn't fit in size_t");
  static_assert(NumberOfTypeNames + 6*DAG_LEN_MAX <= SIZE_MAX/sizeof(unification_var) , "bound_var array too large");
  static_assert(NumberOfTypeNames + 6*DAG_LEN_MAX - 1 <= UINT32_MAX, "bound_var array index doesn't fit in uint32_t");
  simplicity_assert(extra_var_len <= 6*DAG_LEN_MAX);
  *bound_var = simplicity_malloc((NumberOfTypeNames + extra_var_len) * sizeof(unification_var));
  if (!(*bound_var)) return 0;
#include "primitiveInitTy.inc"
  *word256_ix = ty_w256;
  *extra_var_start = NumberOfTypeNames;

  /* 'ty_u' is a trivial binding, so we made 'NumberOfTypeNames - 1' non-trivial bindings. */
  return NumberOfTypeNames - 1;
};

/* An enumeration of the names of Bitcoin specific jets and primitives. */
typedef enum jetName
{
#include "primitiveEnumJet.inc"
  NUMBER_OF_JET_NAMES
} jetName;

/* Decode an Bitcoin specific jet name from 'stream' into 'result'.
 * All jets begin with a bit prefix of '1' which needs to have already been consumed from the 'stream'.
 * Returns 'SIMPLICITY_ERR_DATA_OUT_OF_RANGE' if the stream's prefix doesn't match any valid code for a jet.
 * Returns 'SIMPLICITY_ERR_BITSTRING_EOF' if not enough bits are available in the 'stream'.
 * In the above error cases, 'result' may be modified.
 * Returns 'SIMPLICITY_NO_ERROR' if successful.
 *
 * Precondition: NULL != result
 *               NULL != stream
 */
static simplicity_err decodePrimitive(jetName* result, bitstream* stream) {
  int32_t bit = read1Bit(stream);
  if (bit < 0) return (simplicity_err)bit;
  if (!bit) {
    /* Core jets */
#include "../decodeCoreJets.inc"
    return SIMPLICITY_ERR_DATA_OUT_OF_RANGE;
  } else {
    /* Bitcoin jets */
#include "decodeBitcoinJets.inc"
    return SIMPLICITY_ERR_DATA_OUT_OF_RANGE;
  }
}

/* Return a copy of the Simplicity node corresponding to the given Bitcoin specific jet 'name'. */
static dag_node jetNode(jetName name) {
  static const dag_node jet_node[] = {
    #include "primitiveJetNode.inc"
  };

  return jet_node[name];
}

/* Decode a Bitcoin specific jet from 'stream' into 'node'.
 * All jets begin with a bit prefix of '1' which needs to have already been consumed from the 'stream'.
 * Returns 'SIMPLICITY_ERR_DATA_OUT_OF_RANGE' if the stream's prefix doesn't match any valid code for a jet.
 * Returns 'SIMPLICITY_ERR_BITSTRING_EOF' if not enough bits are available in the 'stream'.
 * In the above error cases, 'dag' may be modified.
 * Returns 'SIMPLICITY_NO_ERR' if successful.
 *
 * Precondition: NULL != node
 *               NULL != stream
 */
simplicity_err simplicity_bitcoin_decodeJet(dag_node* node, bitstream* stream) {
  jetName name;
  simplicity_err error = decodePrimitive(&name, stream);
  if (!IS_OK(error)) return error;
  *node = jetNode(name);
  return SIMPLICITY_NO_ERROR;
}
