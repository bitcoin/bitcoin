#include "dag.h"

#include <stdbool.h>
#include "bounded.h"
#include "precomputed.h"
#include "rsort.h"
#include "sha256.h"
#include "simplicity_alloc.h"
#include "uword.h"

/* Given a tag for a node, return the SHA-256 hash of its associated CMR tag.
 * This is the "initial value" for computing the commitment Merkle root for that expression.
 *
 * Precondition: 'tag' \notin {HIDDEN, JET, WORD}
 */
static sha256_midstate cmrIV(tag_t tag) {
  switch (tag) {
   case COMP: return cmr_compIV;
   case ASSERTL:
   case ASSERTR:
   case CASE: return cmr_caseIV;
   case PAIR: return cmr_pairIV;
   case DISCONNECT: return cmr_disconnectIV;
   case INJL: return cmr_injlIV;
   case INJR: return cmr_injrIV;
   case TAKE: return cmr_takeIV;
   case DROP: return cmr_dropIV;
   case IDEN: return cmr_idenIV;
   case UNIT: return cmr_unitIV;
   case WITNESS: return cmr_witnessIV;
   /* Precondition violated. */
   case WORD:
   case HIDDEN:
   case JET:
    break;
  }
  SIMPLICITY_UNREACHABLE;
}

/* Given a tag for a node, return the SHA-256 hash of its associated IMR tag.
 * This is the "initial value" for computing the commitment Merkle root for that expression.
 *
 * Precondition: 'tag' \notin {HIDDEN, JET, WORD}
 */
static sha256_midstate imrIV(tag_t tag) {
  return DISCONNECT == tag ? imr_disconnectIV
       : WITNESS == tag ? imr_witnessIV
       : cmrIV(tag);
}

/* Given a tag for a node, return the SHA-256 hash of its associated AMR tag.
 * This is the "initial value" for computing the annotated Merkle root for that expression.
 *
 * Precondition: 'tag' \notin {HIDDEN, JET, WORD}
 */
static sha256_midstate amrIV(tag_t tag) {
  switch (tag) {
   case COMP: return amr_compIV;
   case ASSERTL: return amr_assertlIV;
   case ASSERTR: return amr_assertrIV;
   case CASE: return amr_caseIV;
   case PAIR: return amr_pairIV;
   case DISCONNECT: return amr_disconnectIV;
   case INJL: return amr_injlIV;
   case INJR: return amr_injrIV;
   case TAKE: return amr_takeIV;
   case DROP: return amr_dropIV;
   case IDEN: return amr_idenIV;
   case UNIT: return amr_unitIV;
   case WITNESS: return amr_witnessIV;
   /* Precondition violated. */
   case HIDDEN:
   case JET:
   case WORD:
    break;
  }
  SIMPLICITY_UNREACHABLE;
}

/* Given the identity hash of a jet specification, return the CMR for a jet that implements that specification.
 *
 * Precondition: uint32_t ih[8]
 */
static sha256_midstate mkJetCMR(uint32_t *ih, uint_fast64_t weight) {
  sha256_midstate result = jetIV;
  uint32_t block[16] = {0};
  block[6] = (uint32_t)(weight >> 32);
  block[7] = (uint32_t)weight;
  memcpy(&block[8], ih, sizeof(uint32_t[8]));
  simplicity_sha256_compression(result.s, block);

  return result;
}

/* Compute the CMR of a jet of scribe(v) : ONE |- TWO^(2^n) that outputs a given bitstring.
 *
 * Precondition: 2^n == value->len
 */
sha256_midstate simplicity_computeWordCMR(const bitstring* value, size_t n) {
  /* 'stack' is an array of 30 hashes consisting of 8 'uint32_t's each. */
  uint32_t stack[8*30] = {0};
  uint32_t *stack_ptr = stack;
  sha256_midstate ih = identityIV;
  simplicity_assert(n < 32);
  simplicity_assert((size_t)1 << n == value->len);
  /* Pass 1: Compute the CMR for the expression that writes 'value'.
   * This expression consists of deeply nested PAIRs of expressions that write one bit each.
   */
  /* stack[0..7] (8 bytes) is kept as all zeros for later.
   * We start the stack_ptr at the second item.
   */
  if (n < 3) {
    stack_ptr += 8;
    size_t i;
    switch(n) {
     case 0: i = getBit(value, 0); break;
     case 1: i = 2 + ((1U * getBit(value, 0) << 1) | getBit(value, 1)); break;
     case 2: i = 6 + ((1U * getBit(value, 0) << 3) | (1U * getBit(value, 1) << 2) | (1U * getBit(value, 2) << 1) | getBit(value, 3)); break;
    }
    memcpy(stack_ptr, &word_cmr[i], sizeof(uint32_t[8]));
  } else {
    for (size_t i = 0; i < value->len >> 3; ++i) {
      /* stack_ptr == stack + 8*<count of the number of set bits in the value i> */
      stack_ptr += 8;
      memcpy(stack_ptr, &word_cmr[22 + getByte(value, 8*i)], sizeof(uint32_t[8]));
      /* This inner for loop runs in amortized constant time. */
      for (size_t j = i; j & 1; j = j >> 1) {
        sha256_midstate pair = cmrIV(PAIR);
        stack_ptr -= 8;
        simplicity_sha256_compression(pair.s, stack_ptr);
        memcpy(stack_ptr, pair.s, sizeof(uint32_t[8]));
      }
    }
  }
  /* value->len is a power of 2.*/
  simplicity_assert(stack_ptr == stack + 8);

  /* Pass 2: Compute the identity hash for the expression by adding the type roots of ONE and TWO^(2^n) to the CMR. */
  simplicity_sha256_compression(ih.s, stack);
  memcpy(&stack[0], word_type_root[0].s, sizeof(uint32_t[8]));
  memcpy(&stack[8], word_type_root[n+1].s, sizeof(uint32_t[8]));
  simplicity_sha256_compression(ih.s, stack);

  /* Pass 3: Compute the jet's CMR from the specificion's identity hash. */
  return mkJetCMR(ih.s, ((uint_fast64_t)1 << n));
}

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
void simplicity_computeCommitmentMerkleRoot(dag_node* dag, const uint_fast32_t i) {
  uint32_t block[16] = {0};
  size_t j = 8;

  simplicity_assert(HIDDEN != dag[i].tag);
  simplicity_assert(JET != dag[i].tag);
  simplicity_assert(WORD != dag[i].tag);

  dag[i].cmr = cmrIV(dag[i].tag);

  /* Hash the child sub-expression's CMRs (if there are any children). */
  switch (dag[i].tag) {
   case COMP:
   case ASSERTL:
   case ASSERTR:
   case CASE:
   case PAIR:
    memcpy(block + j, dag[dag[i].child[1]].cmr.s, sizeof(uint32_t[8]));
    j = 0;
    /*@fallthrough@*/
   case DISCONNECT: /* Only the first child is used in the CMR. */
   case INJL:
   case INJR:
   case TAKE:
   case DROP:
    memcpy(block + j, dag[dag[i].child[0]].cmr.s, sizeof(uint32_t[8]));
    simplicity_sha256_compression(dag[i].cmr.s, block);
   case IDEN:
   case UNIT:
   case WITNESS:
   case HIDDEN:
   case JET:
   case WORD:
    break;
  }
}

/* Computes the identity hash roots of every subexpression in a well-typed 'dag' with witnesses.
 * 'ihr[i]' is set to the identity hash of the root of the subexpression 'dag[i]'.
 * When 'HIDDEN == dag[i].tag', then 'ihr[i]' is instead set to a hidden root hash for that hidden node.
 *
 * Precondition: sha256_midstate ihr[len];
 *               dag_node dag[len] and 'dag' is well-typed with 'type_dag' and contains witnesses.
 */
static void computeIdentityHashRoots(sha256_midstate* ihr, const dag_node* dag, const type* type_dag, const uint_fast32_t len) {
  /* Pass 1 */
  for (size_t i = 0; i < len; ++i) {
    uint32_t block[16] = {0};
    size_t j = 8;

    /* For jets, the first pass identity Merkle root is the same as their commitment Merkle root. */
    ihr[i] = HIDDEN == dag[i].tag ? dag[i].cmr
           : JET == dag[i].tag ? dag[i].cmr
           : WORD == dag[i].tag ? dag[i].cmr
           : imrIV(dag[i].tag);
    switch (dag[i].tag) {
     case WITNESS:
      simplicity_sha256_bitstring(block, &dag[i].compactValue);
      memcpy(block + 8, type_dag[WITNESS_B(dag, type_dag, i)].typeMerkleRoot.s, sizeof(uint32_t[8]));
      simplicity_sha256_compression(ihr[i].s, block);
      break;
     case COMP:
     case ASSERTL:
     case ASSERTR:
     case CASE:
     case PAIR:
     case DISCONNECT:
      memcpy(block + j, ihr[dag[i].child[1]].s, sizeof(uint32_t[8]));
      j = 0;
      /*@fallthrough@*/
     case INJL:
     case INJR:
     case TAKE:
     case DROP:
      memcpy(block + j, ihr[dag[i].child[0]].s, sizeof(uint32_t[8]));
      simplicity_sha256_compression(ihr[i].s, block);
     case IDEN:
     case UNIT:
     case HIDDEN:
     case JET:
     case WORD:
      break;
    }
  }

  /* Pass 2 */
  for (size_t i = 0; i < len; ++i) {
    uint32_t block[16] = {0};

    if (HIDDEN == dag[i].tag) {
      memcpy(block + 8, ihr[i].s, sizeof(uint32_t[8]));
      ihr[i] = hiddenIV;
      simplicity_sha256_compression(ihr[i].s, block);
    } else {
      memcpy(block + 8, ihr[i].s, sizeof(uint32_t[8]));
      ihr[i] = identityIV;
      simplicity_sha256_compression(ihr[i].s, block);
      memcpy(block, type_dag[dag[i].sourceType].typeMerkleRoot.s, sizeof(uint32_t[8]));
      memcpy(block + 8, type_dag[dag[i].targetType].typeMerkleRoot.s, sizeof(uint32_t[8]));
      simplicity_sha256_compression(ihr[i].s, block);
    }
  }
}

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
void simplicity_computeAnnotatedMerkleRoot(analyses* analysis, const dag_node* dag, const type* type_dag, const uint_fast32_t len) {
  for (uint_fast32_t i = 0; i < len; ++i) {
    uint32_t block[16] = {0};

    /* For jets, their annotated Merkle root is the same as their commitment Merkle root. */
    analysis[i].annotatedMerkleRoot = HIDDEN == dag[i].tag ? dag[i].cmr
                                    : JET == dag[i].tag ? dag[i].cmr
                                    : WORD == dag[i].tag ? dag[i].cmr
                                    : amrIV(dag[i].tag);
    switch (dag[i].tag) {
     case ASSERTL:
     case ASSERTR:
     case CASE:
      memcpy(block, type_dag[CASE_A(dag, type_dag, i)].typeMerkleRoot.s, sizeof(uint32_t[8]));
      memcpy(block + 8,
             type_dag[CASE_B(dag, type_dag, i)].typeMerkleRoot.s, sizeof(uint32_t[8]));
      simplicity_sha256_compression(analysis[i].annotatedMerkleRoot.s, block);
      memcpy(block, type_dag[CASE_C(dag, type_dag, i)].typeMerkleRoot.s, sizeof(uint32_t[8]));
      memcpy(block + 8, type_dag[CASE_D(dag, type_dag, i)].typeMerkleRoot.s, sizeof(uint32_t[8]));
      simplicity_sha256_compression(analysis[i].annotatedMerkleRoot.s, block);
      memcpy(block, analysis[dag[i].child[0]].annotatedMerkleRoot.s, sizeof(uint32_t[8]));
      memcpy(block + 8, analysis[dag[i].child[1]].annotatedMerkleRoot.s, sizeof(uint32_t[8]));
      simplicity_sha256_compression(analysis[i].annotatedMerkleRoot.s, block);
      break;
     case DISCONNECT:
      memcpy(block, type_dag[DISCONNECT_A(dag, type_dag, i)].typeMerkleRoot.s, sizeof(uint32_t[8]));
      memcpy(block + 8, type_dag[DISCONNECT_B(dag, type_dag, i)].typeMerkleRoot.s, sizeof(uint32_t[8]));
      simplicity_sha256_compression(analysis[i].annotatedMerkleRoot.s, block);
      memcpy(block, type_dag[DISCONNECT_C(dag, type_dag, i)].typeMerkleRoot.s, sizeof(uint32_t[8]));
      memcpy(block + 8, type_dag[DISCONNECT_D(dag, type_dag, i)].typeMerkleRoot.s, sizeof(uint32_t[8]));
      simplicity_sha256_compression(analysis[i].annotatedMerkleRoot.s, block);
      memcpy(block, analysis[dag[i].child[0]].annotatedMerkleRoot.s, sizeof(uint32_t[8]));
      memcpy(block + 8, analysis[dag[i].child[1]].annotatedMerkleRoot.s, sizeof(uint32_t[8]));
      simplicity_sha256_compression(analysis[i].annotatedMerkleRoot.s, block);
      break;
     case COMP:
      memcpy(block + 8, type_dag[COMP_A(dag, type_dag, i)].typeMerkleRoot.s, sizeof(uint32_t[8]));
      simplicity_sha256_compression(analysis[i].annotatedMerkleRoot.s, block);
      memcpy(block, type_dag[COMP_B(dag, type_dag, i)].typeMerkleRoot.s, sizeof(uint32_t[8]));
      memcpy(block + 8, type_dag[COMP_C(dag, type_dag, i)].typeMerkleRoot.s, sizeof(uint32_t[8]));
      simplicity_sha256_compression(analysis[i].annotatedMerkleRoot.s, block);
      memcpy(block, analysis[dag[i].child[0]].annotatedMerkleRoot.s, sizeof(uint32_t[8]));
      memcpy(block + 8, analysis[dag[i].child[1]].annotatedMerkleRoot.s, sizeof(uint32_t[8]));
      simplicity_sha256_compression(analysis[i].annotatedMerkleRoot.s, block);
      break;
     case PAIR:
      memcpy(block + 8, type_dag[PAIR_A(dag, type_dag, i)].typeMerkleRoot.s, sizeof(uint32_t[8]));
      simplicity_sha256_compression(analysis[i].annotatedMerkleRoot.s, block);
      memcpy(block, type_dag[PAIR_B(dag, type_dag, i)].typeMerkleRoot.s, sizeof(uint32_t[8]));
      memcpy(block + 8, type_dag[PAIR_C(dag, type_dag, i)].typeMerkleRoot.s, sizeof(uint32_t[8]));
      simplicity_sha256_compression(analysis[i].annotatedMerkleRoot.s, block);
      memcpy(block, analysis[dag[i].child[0]].annotatedMerkleRoot.s, sizeof(uint32_t[8]));
      memcpy(block + 8, analysis[dag[i].child[1]].annotatedMerkleRoot.s, sizeof(uint32_t[8]));
      simplicity_sha256_compression(analysis[i].annotatedMerkleRoot.s, block);
      break;
     case INJL:
     case INJR:
      memcpy(block, type_dag[INJ_A(dag, type_dag, i)].typeMerkleRoot.s, sizeof(uint32_t[8]));
      memcpy(block + 8, type_dag[INJ_B(dag, type_dag, i)].typeMerkleRoot.s, sizeof(uint32_t[8]));
      simplicity_sha256_compression(analysis[i].annotatedMerkleRoot.s, block);
      memcpy(block, type_dag[INJ_C(dag, type_dag, i)].typeMerkleRoot.s, sizeof(uint32_t[8]));
      memcpy(block + 8, analysis[dag[i].child[0]].annotatedMerkleRoot.s, sizeof(uint32_t[8]));
      simplicity_sha256_compression(analysis[i].annotatedMerkleRoot.s, block);
      break;
     case TAKE:
     case DROP:
      memcpy(block, type_dag[PROJ_A(dag, type_dag, i)].typeMerkleRoot.s, sizeof(uint32_t[8]));
      memcpy(block + 8, type_dag[PROJ_B(dag, type_dag, i)].typeMerkleRoot.s, sizeof(uint32_t[8]));
      simplicity_sha256_compression(analysis[i].annotatedMerkleRoot.s, block);
      memcpy(block, type_dag[PROJ_C(dag, type_dag, i)].typeMerkleRoot.s, sizeof(uint32_t[8]));
      memcpy(block + 8, analysis[dag[i].child[0]].annotatedMerkleRoot.s, sizeof(uint32_t[8]));
      simplicity_sha256_compression(analysis[i].annotatedMerkleRoot.s, block);
      break;
     case IDEN:
      memcpy(block + 8, type_dag[IDEN_A(dag, type_dag, i)].typeMerkleRoot.s, sizeof(uint32_t[8]));
      simplicity_sha256_compression(analysis[i].annotatedMerkleRoot.s, block);
      break;
     case UNIT:
      memcpy(block + 8, type_dag[UNIT_A(dag, type_dag, i)].typeMerkleRoot.s, sizeof(uint32_t[8]));
      simplicity_sha256_compression(analysis[i].annotatedMerkleRoot.s, block);
      break;
     case WITNESS:
      memcpy(block + 8, type_dag[WITNESS_A(dag, type_dag, i)].typeMerkleRoot.s, sizeof(uint32_t[8]));
      simplicity_sha256_compression(analysis[i].annotatedMerkleRoot.s, block);
      memcpy(block, type_dag[WITNESS_B(dag, type_dag, i)].typeMerkleRoot.s, sizeof(uint32_t[8]));
      simplicity_sha256_bitstring(block + 8, &dag[i].compactValue);
      simplicity_sha256_compression(analysis[i].annotatedMerkleRoot.s, block);
      break;
     case HIDDEN:
     case JET:
     case WORD:
      break;
    }
  }
}

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
simplicity_err simplicity_verifyCanonicalOrder(dag_node* dag, const uint_fast32_t len) {
  uint_fast32_t bottom = 0;
  uint_fast32_t top = len-1; /* Underflow is checked below. */

  if (!len) {
    simplicity_assert(false); /* A well-formed dag has non-zero length */
    return SIMPLICITY_NO_ERROR; /* However, an empty dag is technically in canonical order */
  }

  /* We use dag[i].aux as a "stack" to manage the traversal of the DAG. */
  dag[top].aux = len; /* We will set top to 'len' to indicate we are finished. */

  /* Each time any particular 'top' value is revisited in this loop, bottom has increased to be strictly larger than the last 'child'
     value examined.  Therefore we will make further progress in the loop the next time around.
     By this reasoning any given 'top' value will be visited no more than numChildren(dag[top].tag) + 1 <= 3 times.
     Thus this loop iterates at most O('len') times.
   */
  while (top < len) {
    /* We determine canonical order by iterating through the dag in canonical (pre-)order,
       incrementing 'bottom' each time we encounter a node that is (correctly) placed at the 'bottom' index.
       We take advantage of the precondition that the dag is well-formed to know in advance that any children
       of a node have index strictly less than the node itself.
     */

    /* Check first child. */
    uint_fast32_t child = dag[top].child[0];
    switch (dag[top].tag) {
     case ASSERTL:
     case ASSERTR:
     case CASE:
     case DISCONNECT:
     case COMP:
     case PAIR:
     case INJL:
     case INJR:
     case TAKE:
     case DROP:
      if (bottom < child) {
        dag[child].aux = top;
        top = child;
        continue;
      }
      if (bottom == child) bottom++;
     case IDEN:
     case UNIT:
     case WITNESS:
     case HIDDEN:
     case JET:
     case WORD:
      break;
    }

    /* Check second child. */
    child = dag[top].child[1];
    switch (dag[top].tag) {
     case ASSERTL:
     case ASSERTR:
     case CASE:
     case DISCONNECT:
     case COMP:
     case PAIR:
      if (bottom < child) {
        dag[child].aux = top;
        top = child;
        continue;
      }
      if (bottom == child) bottom++;
     case INJL:
     case INJR:
     case TAKE:
     case DROP:
     case IDEN:
     case UNIT:
     case WITNESS:
     case HIDDEN:
     case JET:
     case WORD:
      break;
    }

    /* Check current node. */
    if (bottom < top) return SIMPLICITY_ERR_DATA_OUT_OF_ORDER;
    if (bottom == top) bottom++;
    /* top < bottom */
    top = dag[top].aux; /* Return. */
  }
  simplicity_assert(bottom == top && top == len);

  return SIMPLICITY_NO_ERROR;
}

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
simplicity_err simplicity_fillWitnessData(dag_node* dag, type* type_dag, const uint_fast32_t len, bitstream *witness) {
  static_assert(CELLS_MAX <= 0x80000000, "CELLS_MAX is too large.");
  for (uint_fast32_t i = 0; i < len; ++i) {
    if (WITNESS == dag[i].tag) {
      if (CELLS_MAX < type_dag[WITNESS_B(dag, type_dag, i)].bitSize) return SIMPLICITY_ERR_EXEC_MEMORY;
      if (witness->len <= 0) {
        /* There is no data in the witness. */
        dag[i].compactValue = (bitstring){0};
        /* This is fine as long as the witness type is trivial */
        if (type_dag[WITNESS_B(dag, type_dag, i)].bitSize) return SIMPLICITY_ERR_WITNESS_EOF;
      } else {
        dag[i].compactValue = (bitstring)
          { .arr = witness->arr
          , .offset = witness->offset
          , .len = 0 /* The value of .len will computed within the while loop. */
          };

        /* Traverse the witness type to parse the witness's compact representation as a bit string. */
        size_t cur = typeSkip(WITNESS_B(dag, type_dag, i), type_dag);
        bool calling = true;
        setTypeBack(cur, type_dag, 0);
        while (cur) {
          if (SUM == type_dag[cur].kind) {
            /* Parse one bit and traverse the left type or the right type depending on the value of the bit parsed. */
            simplicity_assert(calling);
            int32_t bit = read1Bit(witness);
            if (bit < 0) return SIMPLICITY_ERR_WITNESS_EOF;
            dag[i].compactValue.len++;
            size_t next = typeSkip(type_dag[cur].typeArg[bit], type_dag);
            if (next) {
              setTypeBack(next, type_dag, type_dag[cur].back);
              cur = next;
            } else {
              cur = type_dag[cur].back;
              calling = false;
            }
          } else {
            simplicity_assert(PRODUCT == type_dag[cur].kind);
            size_t next;
            if (calling) {
              next = typeSkip(type_dag[cur].typeArg[0], type_dag);
              /* Note: Because we are using 'typeSkip' we have an invarant on 'cur' such that whenever type_dag[cur].kind == PRODUCT,
                 then it is a product of two non-trival types.  This implies that 'next' cannot actually be 0. */
              if (next) {
                /* Traverse the first element of the product type, if it has any data. */
                setTypeBack(next, type_dag, cur);
                cur = next;
                continue;
              }
            }
            next = typeSkip(type_dag[cur].typeArg[1], type_dag);
            /* Note: Because we are using 'typeSkip' we have an invarant on 'cur' such that whenever type_dag[cur].kind == PRODUCT,
               then it is a product of two non-trival types.  This implies that 'next' cannot actually be 0. */
            if (next) {
              /* Traverse the second element of the product type, if it has any data. */
              setTypeBack(next, type_dag, type_dag[cur].back);
              cur = next;
              calling = true;
            } else {
              cur = type_dag[cur].back;
              calling = false;
            }
          }
        }

        /* Note: Above we use 'typeSkip' to skip over long chains of products against trivial types
         * This avoids a potential DOS vulnerability where a DAG of deeply nested products of unit types with sharing is traversed,
         * taking exponential time.
         * While traversing still could take exponential time in terms of the size of the type's dag,
         * at least one bit of witness data is required per PRODUCT type encountered.
         * This ought to limit the total number of times through the above loop to no more that 3 * dag[i].witness.len.
         */
      }
    }
  }
  return SIMPLICITY_NO_ERROR;
}

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
simplicity_err simplicity_verifyNoDuplicateIdentityHashes(sha256_midstate* ihr, const dag_node* dag, const type* type_dag, const uint_fast32_t dag_len) {
  simplicity_assert(0 < dag_len);
  simplicity_assert(dag_len <= DAG_LEN_MAX);
  sha256_midstate* ih_buf = simplicity_malloc((size_t)dag_len * sizeof(sha256_midstate));
  if (!ih_buf) return SIMPLICITY_ERR_MALLOC;

  computeIdentityHashRoots(ih_buf, dag, type_dag, dag_len);

  if (ihr) *ihr = ih_buf[dag_len-1];

  int result = simplicity_hasDuplicates(ih_buf, dag_len);

  simplicity_free(ih_buf);

  switch (result) {
  case -1: return SIMPLICITY_ERR_MALLOC;
  case 0: return SIMPLICITY_NO_ERROR;
  default: return SIMPLICITY_ERR_UNSHARED_SUBEXPRESSION;
  }
}
