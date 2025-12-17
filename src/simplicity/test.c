#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#include <simplicity/elements/exec.h>
#include <simplicity/elements/cmr.h>
#include "ctx8Pruned.h"
#include "ctx8Unpruned.h"
#include "dag.h"
#include "deserialize.h"
#include "eval.h"
#include "hashBlock.h"
#include "rsort.h"
#include "sha256.h"
#include "schnorr0.h"
#include "schnorr6.h"
#include "regression4.h"
#include "typeSkipTest.h"
#include "simplicity_alloc.h"
#include "typeInference.h"
#include "elements/checkSigHashAllTx1.h"
#include "elements/primitive.h"

_Static_assert(CHAR_BIT == 8, "Buffers passed to fmemopen presume 8 bit chars");

/* Bitcoin's (old-school) sigop limit is 80,000.  Ecdsa signature verification takes approximately 50 microseconds,
 * meaning a block for of sigops would take 4 seconds to verify.
 * Post-taproot, there is a block size limit of 4,000,000 WU.
 * To verify the worst case block full of simplicity programs, in the same 4 second limit
 * we need to limit Simplicity programs so they take no more than 1 microsecond per WU to run.
 */
static const double secondsPerWU = 1 / 1000. / 1000.;
static int successes = 0;
static int failures = 0;

#ifdef TIMING_FLAG
static int timing_flag = 1;
#else
static int timing_flag = 0;
#endif

static void fprint_cmr(FILE* stream, const uint32_t* cmr) {
  fprintf(stream, "0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x",
          cmr[0], cmr[1], cmr[2], cmr[3], cmr[4], cmr[5], cmr[6], cmr[7]
         );
}

static void test_decodeUptoMaxInt(void) {
  printf("Test decodeUptoMaxInt\n");
  const unsigned char buf[] =
  { 0x4b, 0x86, 0x39, 0xe8, 0xdf, 0xc0, 0x38, 0x0f, 0x7f, 0xff, 0xff, 0x00
  , 0x00, 0x00, 0xf0, 0xe0, 0x00, 0x00, 0x00, 0x3c, 0x3b, 0xff, 0xff, 0xff
  , 0xff, 0x0f, 0x00, 0x00, 0x00, 0x00
  };
  const int32_t expected[] =
  { 1, 2, 3, 4, 5, 7, 8, 15, 16, 17
  , 0xffff, 0x10000, 0x40000000, 0x7fffffff, SIMPLICITY_ERR_DATA_OUT_OF_RANGE
  };

  bitstream stream = initializeBitstream(buf, sizeof(buf));
  for (size_t i = 0; i < sizeof(expected)/sizeof(expected[0]); ++i) {
    int32_t result = simplicity_decodeUptoMaxInt(&stream);
    if (expected[i] == result) {
      successes++;
    } else {
      failures++;
      printf("Unexpected result during parsing.  Expected %d and received %d\n", expected[i], result);
    }
  }
}

static void test_hashBlock(void) {
  printf("Test hashBlock\n");
  dag_node* dag;
  combinator_counters census;
  int_fast32_t len;
  simplicity_err error;
  {
    bitstream stream = initializeBitstream(hashBlock, sizeof_hashBlock);
    len = simplicity_decodeMallocDag(&dag, simplicity_elements_decodeJet, &census, &stream);
    if (!dag) {
      simplicity_assert(len < 0);
      error = (simplicity_err)len;
      failures++;
      printf("Error parsing dag: %d\n", error);
    } else {
      simplicity_assert(0 < len);
      error = simplicity_closeBitstream(&stream);
      if (!IS_OK(error)) {
        failures++;
        printf("Error closing dag stream for hashblock\n");
      }
    }
  }
  if (dag && IS_OK(error)) {
    successes++;

    if (0 == memcmp(hashBlock_cmr, dag[len-1].cmr.s, sizeof(uint32_t[8]))) {
      successes++;
    } else {
      failures++;
      printf("Unexpected CMR of hashblock\n");
    }

    type* type_dag;
    bitstream witness = initializeBitstream(hashBlock_witness, sizeof_hashBlock_witness);
    if (!IS_OK(simplicity_mallocTypeInference(&type_dag, simplicity_elements_mallocBoundVars, dag, (uint_fast32_t)len, &census)) || !type_dag ||
        type_dag[dag[len-1].sourceType].bitSize != 768 || type_dag[dag[len-1].targetType].bitSize != 256) {
      failures++;
      printf("Unexpected failure of type inference for hashblock\n");
    } else if (!IS_OK(simplicity_fillWitnessData(dag, type_dag, (uint_fast32_t)len, &witness))) {
      failures++;
      printf("Unexpected failure of fillWitnessData for hashblock\n");
    } else if (!IS_OK(simplicity_closeBitstream(&witness))) {
      failures++;
      printf("Unexpected failure of witness stream for hashblock\n");
    } else {
      {
        analyses analysis[len];
        simplicity_computeAnnotatedMerkleRoot(analysis, dag, type_dag, (uint_fast32_t)len);
        if (0 == memcmp(hashBlock_amr, analysis[len-1].annotatedMerkleRoot.s, sizeof(uint32_t[8]))) {
          successes++;
        } else {
          failures++;
          printf("Unexpected AMR of hashblock\n");
        }
      }
      {
        sha256_midstate ihr;
        if (IS_OK(simplicity_verifyNoDuplicateIdentityHashes(&ihr, dag, type_dag, (uint_fast32_t)len)) &&
            0 == memcmp(hashBlock_ihr, ihr.s, sizeof(uint32_t[8]))) {
          successes++;
        } else {
          failures++;
          printf("Unexpected IHR of hashblock\n");
        }
      }

      ubounded inputBitSize = type_dag[dag[len-1].sourceType].bitSize;
      ubounded outputBitSize = type_dag[dag[len-1].targetType].bitSize;
      UWORD input[ROUND_UWORD(inputBitSize)];
      UWORD output[ROUND_UWORD(outputBitSize)];
      { frameItem frame = initWriteFrame(inputBitSize, &input[ROUND_UWORD(inputBitSize)]);
        simplicity_assert(256+512 == inputBitSize);
        /* Set SHA-256's initial value. */
        write32s(&frame, (uint32_t[8])
            { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 }
          , 8);
        /* Set the block to be compressed to "abc" with padding. */
        write32s(&frame, (uint32_t[16]){ [0] = 0x61626380, [15] = 0x18 }, 16);
      }
      {
        ubounded cellsBound, UWORDBound, frameBound, costBound;
        if (IS_OK(simplicity_analyseBounds(&cellsBound, &UWORDBound, &frameBound, &costBound, UBOUNDED_MAX, 0, UBOUNDED_MAX, dag, type_dag, (uint_fast32_t)len))
            && hashBlock_cost == costBound) {
          successes++;
        } else {
          failures++;
          printf("Expected %d for cost, but got %d instead.\n", hashBlock_cost, costBound);
        }
      }
      simplicity_err err = simplicity_evalTCOExpression(CHECK_NONE, output, input, dag, type_dag, (uint_fast32_t)len, 0, NULL, NULL);
      if (IS_OK(err)) {
        /* The expected result is the value 'SHA256("abc")'. */
        const uint32_t expectedHash[8] = { 0xba7816bful, 0x8f01cfeaul, 0x414140deul, 0x5dae2223ul
                                         , 0xb00361a3ul, 0x96177a9cul, 0xb410ff61ul, 0xf20015adul };
        frameItem frame = initReadFrame(outputBitSize, &output[0]);
        uint32_t result[8];
        simplicity_assert(256 == outputBitSize);
        read32s(result, 8, &frame);
        if (0 == memcmp(expectedHash, result, sizeof(uint32_t[8]))) {
          successes++;
        } else {
          failures++;
          printf("Unexpected output of hashblock computation.\n");
        }
      } else {
        failures++;
        printf("Unexpected failure of hashblock evaluation: %d\n", err);
      }
    }
    simplicity_free(type_dag);
  }
  simplicity_free(dag);
}

static void test_program(char* name, const unsigned char* program, size_t program_len, const unsigned char* witness, size_t witness_len,
                         simplicity_err expectedResult, const uint32_t* expectedCMR,
                         const uint32_t* expectedIHR, const uint32_t* expectedAMR, const ubounded *expectedCost) {
  printf("Test %s\n", name);
  dag_node* dag;
  combinator_counters census;
  int_fast32_t len;
  simplicity_err error;
  {
    bitstream stream = initializeBitstream(program, program_len);
    len = simplicity_decodeMallocDag(&dag, simplicity_elements_decodeJet, &census, &stream);
    if (!dag) {
      simplicity_assert(len < 0);
      error = (simplicity_err)len;
      failures++;
      printf("Error parsing dag: %d\n", error);
    } else {
      simplicity_assert(0 < len);
      error = simplicity_closeBitstream(&stream);
      if (!IS_OK(error)) {
        if (expectedResult == error) {
          successes++;
        } else {
          failures++;
          printf("Error closing dag stream: %d\n", error);
        }
      }
    }
  }
  if (dag && IS_OK(error)) {
    successes++;

    if (expectedCMR) {
      if (0 == memcmp(expectedCMR, dag[len-1].cmr.s, sizeof(uint32_t[8]))) {
        successes++;
      } else {
        failures++;
        printf("Unexpected CMR. Expected\n{");
        fprint_cmr(stdout, expectedCMR);
        printf("}, but received\n{");
        fprint_cmr(stdout, dag[len-1].cmr.s);
        printf("}\n");
      }
    }
    type* type_dag;
    bitstream witness_stream = initializeBitstream(witness, witness_len);
    if (!IS_OK(simplicity_mallocTypeInference(&type_dag, simplicity_elements_mallocBoundVars, dag, (uint_fast32_t)len, &census)) || !type_dag ||
        dag[len-1].sourceType != 0 || dag[len-1].targetType != 0) {
      failures++;
      printf("Unexpected failure of type inference.\n");
    } else if (!IS_OK(simplicity_fillWitnessData(dag, type_dag, (uint_fast32_t)len, &witness_stream))) {
      failures++;
      printf("Unexpected failure of fillWitnessData.\n");
    } else if (!IS_OK(simplicity_closeBitstream(&witness_stream))) {
      failures++;
      printf("Unexpected failure closing witness_stream\n");
    } else {
      if (expectedAMR) {
        analyses analysis[len];
        simplicity_computeAnnotatedMerkleRoot(analysis, dag, type_dag, (uint_fast32_t)len);
        if (0 == memcmp(expectedAMR, analysis[len-1].annotatedMerkleRoot.s, sizeof(uint32_t[8]))) {
          successes++;
        } else {
          failures++;
          printf("Unexpected AMR.\n");
        }
      }
      {
        sha256_midstate ihr;
        if (IS_OK(simplicity_verifyNoDuplicateIdentityHashes(&ihr, dag, type_dag, (uint_fast32_t)len)) &&
            (!expectedIHR || 0 == memcmp(expectedIHR, ihr.s, sizeof(uint32_t[8])))) {
          successes++;
        } else {
          failures++;
          printf("Unexpected IHR.\n");
        }
      }
      if (expectedCost) {
        ubounded cellsBound, UWORDBound, frameBound, costBound;
        if (IS_OK(simplicity_analyseBounds(&cellsBound, &UWORDBound, &frameBound, &costBound, UBOUNDED_MAX, 0, UBOUNDED_MAX, dag, type_dag, (uint_fast32_t)len))
           && 0 < costBound
           && *expectedCost == costBound) {
          successes++;
        } else {
          failures++;
          printf("Expected %u for cost, but got %u instead.\n", *expectedCost, costBound);
        }
        /* Analysis should pass when computed bounds are used. */
        if (IS_OK(simplicity_analyseBounds(&cellsBound, &UWORDBound, &frameBound, &costBound, cellsBound, costBound-1, costBound, dag, type_dag, (uint_fast32_t)len))
           && *expectedCost == costBound) {
          successes++;
        } else {
          failures++;
          printf("Analysis with computed bounds failed.\n");
        }
        /* if cellsBound is non-zero, analysis should fail when smaller cellsBound is used. */
        if (0 < cellsBound) {
          if (SIMPLICITY_ERR_EXEC_MEMORY == simplicity_analyseBounds(&cellsBound, &UWORDBound, &frameBound, &costBound, cellsBound-1, 0, UBOUNDED_MAX, dag, type_dag, (uint_fast32_t)len)) {
            successes++;
          } else {
            failures++;
            printf("Analysis with too small cells bounds failed. \n");
          }
        }
        /* Analysis should fail when smaller costBound is used. */
        if (0 < *expectedCost &&
            SIMPLICITY_ERR_EXEC_BUDGET == simplicity_analyseBounds(&cellsBound, &UWORDBound, &frameBound, &costBound, UBOUNDED_MAX, 0, *expectedCost-1, dag, type_dag, (uint_fast32_t)len)
           ) {
          successes++;
        } else {
          failures++;
          printf("Analysis with too small cost bounds failed.\n");
        }
        /* Analysis should fail when overweight. */
        if (0 < *expectedCost &&
            SIMPLICITY_ERR_OVERWEIGHT == simplicity_analyseBounds(&cellsBound, &UWORDBound, &frameBound, &costBound, UBOUNDED_MAX, *expectedCost, UBOUNDED_MAX, dag, type_dag, (uint_fast32_t)len)
           ) {
          successes++;
        } else {
          failures++;
          printf("Analysis with too large minCost failed.\n");
        }
      }
      simplicity_err actualResult = evalTCOProgram(dag, type_dag, (uint_fast32_t)len, 0, NULL, NULL);
      if (expectedResult == actualResult) {
        successes++;
      } else {
        failures++;
        printf("Expected %d from evaluation, but got %d instead.\n", expectedResult, actualResult);
      }
    }
    simplicity_free(type_dag);
  }
  simplicity_free(dag);
}

static void test_occursCheck(void) {
  printf("Test occursCheck\n");
  /* The untyped Simplicity term (case (drop iden) iden) ought to cause an occurs check failure. */
  const unsigned char buf[] = { 0xc1, 0x07, 0x20, 0x30 };
  dag_node* dag;
  combinator_counters census;
  int_fast32_t len;
  {
    bitstream stream = initializeBitstream(buf, sizeof(buf));
    len = simplicity_decodeMallocDag(&dag, simplicity_elements_decodeJet, &census, &stream);
  }
  if (!dag) {
    simplicity_assert(len < 0);
    printf("Error parsing dag: %" PRIdFAST32 "\n", len);
  } else {
    type* type_dag;
    simplicity_assert(0 < len);
    if (SIMPLICITY_ERR_TYPE_INFERENCE_OCCURS_CHECK == simplicity_mallocTypeInference(&type_dag, simplicity_elements_mallocBoundVars, dag, (uint_fast32_t)len, &census) &&
        !type_dag) {
      successes++;
    } else {
      printf("Unexpected occurs check success\n");
      failures++;
    }
    simplicity_free(type_dag);
  }
  simplicity_free(dag);
}

static void test_elements(void) {
  unsigned char cmr[32], amr[32];
  sha256_fromMidstate(cmr, elementsCheckSigHashAllTx1_cmr);
  sha256_fromMidstate(amr, elementsCheckSigHashAllTx1_amr);

  unsigned char genesisHash[32] = {0x0f, 0x91, 0x88, 0xf1, 0x3c, 0xb7, 0xb2, 0xc7, 0x1f, 0x2a, 0x33, 0x5e, 0x3a, 0x4f, 0xc3, 0x28, 0xbf, 0x5b, 0xeb, 0x43, 0x60, 0x12, 0xaf, 0xca, 0x59, 0x0b, 0x1a, 0x11, 0x46, 0x6e, 0x22, 0x06};
  rawElementsTapEnv rawTaproot = (rawElementsTapEnv)
    { .controlBlock = (unsigned char [33]){0xbe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3b, 0x78, 0xce, 0x56, 0x3f, 0x89, 0xa0, 0xed, 0x94, 0x14, 0xf5, 0xaa, 0x28, 0xad, 0x0d, 0x96, 0xd6, 0x79, 0x5f, 0x9c, 0x63}
    , .pathLen = 0
    , .scriptCMR = cmr
    };
  elementsTapEnv* taproot = simplicity_elements_mallocTapEnv(&rawTaproot);

  printf("Test elements\n");
  {
    rawElementsTransaction testTx1 = (rawElementsTransaction)
      { .txid = (unsigned char[32]){0xdb, 0x9a, 0x3d, 0xe0, 0xb6, 0xb8, 0xcc, 0x74, 0x1e, 0x4d, 0x6c, 0x8f, 0x19, 0xce, 0x75, 0xec, 0x0d, 0xfd, 0x01, 0x02, 0xdb, 0x9c, 0xb5, 0xcd, 0x27, 0xa4, 0x1a, 0x66, 0x91, 0x66, 0x3a, 0x07}
      , .input = (rawElementsInput[])
                 { { .annex = NULL
                   , .prevTxid = (unsigned char[32]){0xeb, 0x04, 0xb6, 0x8e, 0x9a, 0x26, 0xd1, 0x16, 0x04, 0x6c, 0x76, 0xe8, 0xff, 0x47, 0x33, 0x2f, 0xb7, 0x1d, 0xda, 0x90, 0xff, 0x4b, 0xef, 0x53, 0x70, 0xf2, 0x52, 0x26, 0xd3, 0xbc, 0x09, 0xfc}
                   , .prevIx = 0
                   , .sequence = 0xfffffffe
                   , .issuance = {0}
                   , .scriptSig = {0}
                   , .txo = { .asset = (unsigned char[33]){0x01, 0x23, 0x0f, 0x4f, 0x5d, 0x4b, 0x7c, 0x6f, 0xa8, 0x45, 0x80, 0x6e, 0xe4, 0xf6, 0x77, 0x13, 0x45, 0x9e, 0x1b, 0x69, 0xe8, 0xe6, 0x0f, 0xce, 0xe2, 0xe4, 0x94, 0x0c, 0x7a, 0x0d, 0x5d, 0xe1, 0xb2}
                            , .value = (unsigned char[9]){0x01, 0x00, 0x00, 0x00, 0x02, 0x54, 0x0b, 0xe4, 0x00}
                            , .scriptPubKey = {0}
                 } }        }
      , .output = (rawElementsOutput[])
                  { { .asset = (unsigned char[33]){0x01, 0x23, 0x0f, 0x4f, 0x5d, 0x4b, 0x7c, 0x6f, 0xa8, 0x45, 0x80, 0x6e, 0xe4, 0xf6, 0x77, 0x13, 0x45, 0x9e, 0x1b, 0x69, 0xe8, 0xe6, 0x0f, 0xce, 0xe2, 0xe4, 0x94, 0x0c, 0x7a, 0x0d, 0x5d, 0xe1, 0xb2}
                    , .value = (unsigned char[9]){0x01, 0x00, 0x00, 0x00, 0x02, 0x54, 0x0b, 0xd7, 0x1c}
                    , .nonce = NULL
                    , .scriptPubKey = { .buf = (unsigned char [26]){0x19, 0x76, 0xa9, 0x14, 0x48, 0x63, 0x3e, 0x2c, 0x0e, 0xe9, 0x49, 0x5d, 0xd3, 0xf9, 0xc4, 0x37, 0x32, 0xc4, 0x7f, 0x47, 0x02, 0xa3, 0x62, 0xc8, 0x88, 0xac}
                                      , .len = 26
                                      }
                    }
                  , { .asset = (unsigned char[33]){0x01, 0x23, 0x0f, 0x4f, 0x5d, 0x4b, 0x7c, 0x6f, 0xa8, 0x45, 0x80, 0x6e, 0xe4, 0xf6, 0x77, 0x13, 0x45, 0x9e, 0x1b, 0x69, 0xe8, 0xe6, 0x0f, 0xce, 0xe2, 0xe4, 0x94, 0x0c, 0x7a, 0x0d, 0x5d, 0xe1, 0xb2}
                    , .value = (unsigned char[9]){0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0xe4}
                    , .nonce = NULL
                    , .scriptPubKey = {0}
                  } }
      , .numInputs = 1
      , .numOutputs = 2
      , .version = 0x00000002
      , .lockTime = 0x00000000
      };
    elementsTransaction* tx1 = simplicity_elements_mallocTransaction(&testTx1);
    if (tx1) {
      successes++;
      simplicity_err execResult;
      {
        unsigned char cmrResult[32];
        if (simplicity_elements_computeCmr(&execResult, cmrResult, elementsCheckSigHashAllTx1, sizeof_elementsCheckSigHashAllTx1) && IS_OK(execResult)) {
          if (0 == memcmp(cmrResult, cmr, sizeof(unsigned char[8]))) {
            successes++;
          } else {
            failures++;
            printf("Unexpected CMR of elementsCheckSigHashAllTx1\n");
          }
        } else {
          failures++;
          printf("simplicity_elements_computeCmr of elementsCheckSigHashAllTx1 unexpectedly produced %d.\n", execResult);
        }
      }
      {
        unsigned char ihrResult[32];
        if (simplicity_elements_execSimplicity(&execResult, ihrResult, tx1, 0, taproot, genesisHash, 0, (elementsCheckSigHashAllTx1_cost + 999)/1000, amr, elementsCheckSigHashAllTx1, sizeof_elementsCheckSigHashAllTx1, elementsCheckSigHashAllTx1_witness, sizeof_elementsCheckSigHashAllTx1_witness) && IS_OK(execResult)) {
          sha256_midstate ihr;
          sha256_toMidstate(ihr.s, ihrResult);
          if (0 == memcmp(ihr.s, elementsCheckSigHashAllTx1_ihr, sizeof(uint32_t[8]))) {
            successes++;
          } else {
            failures++;
            printf("Unexpected IHR of elementsCheckSigHashAllTx1\n");
          }
        } else {
          failures++;
          printf("execSimplicity of elementsCheckSigHashAllTx1 on tx1 unexpectedly produced %d.\n", execResult);
        }
        if (elementsCheckSigHashAllTx1_cost){
          /* test the same transaction without adequate budget. */
          simplicity_assert(elementsCheckSigHashAllTx1_cost);
          if (simplicity_elements_execSimplicity(&execResult, ihrResult, tx1, 0, taproot, genesisHash, 0, (elementsCheckSigHashAllTx1_cost - 1)/1000, amr, elementsCheckSigHashAllTx1, sizeof_elementsCheckSigHashAllTx1, elementsCheckSigHashAllTx1_witness, sizeof_elementsCheckSigHashAllTx1_witness) && SIMPLICITY_ERR_EXEC_BUDGET == execResult) {
            successes++;
          } else {
            failures++;
            printf("execSimplicity of elementsCheckSigHashAllTx1 on tx1 unexpectedly produced %d.\n", execResult);
          }
        }
      }
      {
        /* test the same transaction with a erroneous signature. */
        unsigned char brokenSig[sizeof_elementsCheckSigHashAllTx1_witness];
        memcpy(brokenSig, elementsCheckSigHashAllTx1_witness, sizeof_elementsCheckSigHashAllTx1_witness);
        brokenSig[sizeof_elementsCheckSigHashAllTx1_witness - 1] ^= 0x80;
        if (simplicity_elements_execSimplicity(&execResult, NULL, tx1, 0, taproot, genesisHash, 0, BUDGET_MAX, NULL, elementsCheckSigHashAllTx1, sizeof_elementsCheckSigHashAllTx1, brokenSig, sizeof_elementsCheckSigHashAllTx1_witness) && SIMPLICITY_ERR_EXEC_JET == execResult) {
          successes++;
        } else {
          failures++;
          printf("execSimplicity of brokenSig on tx1 unexpectedly produced %d.\n", execResult);
        }
      }
    } else {
      printf("mallocTransaction(&rawTx1) failed\n");
      failures++;
    }
    simplicity_elements_freeTransaction(tx1);
  }
  /* test a modified transaction with the same signature. */
  {
    rawElementsTransaction testTx2 = (rawElementsTransaction)
      { .txid = (unsigned char[32]){0xdb, 0x9a, 0x3d, 0xe0, 0xb6, 0xb8, 0xcc, 0x74, 0x1e, 0x4d, 0x6c, 0x8f, 0x19, 0xce, 0x75, 0xec, 0x0d, 0xfd, 0x01, 0x02, 0xdb, 0x9c, 0xb5, 0xcd, 0x27, 0xa4, 0x1a, 0x66, 0x91, 0x66, 0x3a, 0x07}
      , .input = (rawElementsInput[])
                 { { .prevTxid = (unsigned char[32]){0xeb, 0x04, 0xb6, 0x8e, 0x9a, 0x26, 0xd1, 0x16, 0x04, 0x6c, 0x76, 0xe8, 0xff, 0x47, 0x33, 0x2f, 0xb7, 0x1d, 0xda, 0x90, 0xff, 0x4b, 0xef, 0x53, 0x70, 0xf2, 0x52, 0x26, 0xd3, 0xbc, 0x09, 0xfc}
                   , .prevIx = 0
                   , .sequence = 0xffffffff /* Here is the modification. */
                   , .issuance = {0}
                   , .txo = { .asset = (unsigned char[33]){0x01, 0x23, 0x0f, 0x4f, 0x5d, 0x4b, 0x7c, 0x6f, 0xa8, 0x45, 0x80, 0x6e, 0xe4, 0xf6, 0x77, 0x13, 0x45, 0x9e, 0x1b, 0x69, 0xe8, 0xe6, 0x0f, 0xce, 0xe2, 0xe4, 0x94, 0x0c, 0x7a, 0x0d, 0x5d, 0xe1, 0xb2}
                            , .value = (unsigned char[9]){0x01, 0x00, 0x00, 0x00, 0x02, 0x54, 0x0b, 0xe4, 0x00}
                            , .scriptPubKey = {0}
                 } }        }
      , .output = (rawElementsOutput[])
                  { { .asset = (unsigned char[33]){0x01, 0x23, 0x0f, 0x4f, 0x5d, 0x4b, 0x7c, 0x6f, 0xa8, 0x45, 0x80, 0x6e, 0xe4, 0xf6, 0x77, 0x13, 0x45, 0x9e, 0x1b, 0x69, 0xe8, 0xe6, 0x0f, 0xce, 0xe2, 0xe4, 0x94, 0x0c, 0x7a, 0x0d, 0x5d, 0xe1, 0xb2}
                    , .value = (unsigned char[9]){0x01, 0x00, 0x00, 0x00, 0x02, 0x54, 0x0b, 0xd7, 0x1c}
                    , .nonce = NULL
                    , .scriptPubKey = { .buf = (unsigned char [26]){0x19, 0x76, 0xa9, 0x14, 0x48, 0x63, 0x3e, 0x2c, 0x0e, 0xe9, 0x49, 0x5d, 0xd3, 0xf9, 0xc4, 0x37, 0x32, 0xc4, 0x7f, 0x47, 0x02, 0xa3, 0x62, 0xc8, 0x88, 0xac}
                                      , .len = 26
                                      }
                    }
                  , { .asset = (unsigned char[33]){0x01, 0x23, 0x0f, 0x4f, 0x5d, 0x4b, 0x7c, 0x6f, 0xa8, 0x45, 0x80, 0x6e, 0xe4, 0xf6, 0x77, 0x13, 0x45, 0x9e, 0x1b, 0x69, 0xe8, 0xe6, 0x0f, 0xce, 0xe2, 0xe4, 0x94, 0x0c, 0x7a, 0x0d, 0x5d, 0xe1, 0xb2}
                    , .value = (unsigned char[9]){0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0xe4}
                    , .nonce = NULL
                    , .scriptPubKey = {0}
                  } }
      , .numInputs = 1
      , .numOutputs = 2
      , .version = 0x00000002
      , .lockTime = 0x00000000
      };
    elementsTransaction* tx2 = simplicity_elements_mallocTransaction(&testTx2);
    if (tx2) {
      successes++;
      simplicity_err execResult;
      {
        if (simplicity_elements_execSimplicity(&execResult, NULL, tx2, 0, taproot, genesisHash, 0, BUDGET_MAX, NULL, elementsCheckSigHashAllTx1, sizeof_elementsCheckSigHashAllTx1, elementsCheckSigHashAllTx1_witness, sizeof_elementsCheckSigHashAllTx1_witness) && SIMPLICITY_ERR_EXEC_JET == execResult) {
          successes++;
        } else {
          failures++;
          printf("execSimplicity of elementsCheckSigHashAllTx1 on tx2 unexpectedly produced %d.\n", execResult);
        }
      }
    } else {
      printf("mallocTransaction(&testTx2) failed\n");
      failures++;
    }
    simplicity_elements_freeTransaction(tx2);
  }
  simplicity_elements_freeTapEnv(taproot);
}

static sha256_midstate hashint(uint_fast32_t n) {
  sha256_midstate result;
  sha256_context ctx = sha256_init(result.s);
  sha256_u32le(&ctx, n);
  sha256_finalize(&ctx);

  return result;
}

static sha256_midstate rsort_no_duplicates(uint_fast32_t i) {
  return hashint(i);
}

static sha256_midstate rsort_all_duplicates(uint_fast32_t i) {
  (void)i;
  return hashint(0);
}

static sha256_midstate rsort_one_duplicate(uint_fast32_t i) {
  return hashint(i ? i : 1);
}

/* Tests a worst-case conditions for stack usage in rsort. */
static sha256_midstate rsort_diagonal(uint_fast32_t i) {
  sha256_midstate result = {0};
  unsigned char *alias = (unsigned char *)(result.s);
  for (uint_fast32_t j = 0; j < sizeof(result.s); ++j) {
    alias[j] = j == i ? 0 : 0xff;
  }
  return result;
}

static void test_hasDuplicates(const char* name, int expected, sha256_midstate (*f)(uint_fast32_t), uint_fast32_t n) {
  sha256_midstate hashes[n];

  printf("Test %s\n", name);
  for(uint_fast32_t i = 0; i < n; ++i) {
    hashes[i] = f(i);
  }

  int actual = simplicity_hasDuplicates(hashes, n);
  if (expected == actual) {
    successes++;
  } else if (actual < 0) {
    failures++;
    printf("Unexpected failure of hasDuplicates\n");
  } else if (0 == expected) {
    failures++;
    printf("Expected no duplicate but found some.\n");
  } else {
    failures++;
    printf("Expected duplicates but found none.\n");
  }
}

static void regression_tests(void) {
  {
    /* word("2^23 zero bits") ; unit */
    size_t sizeof_regression3 = ((size_t)1 << 20) + 4;
    unsigned char *regression3 = simplicity_calloc(sizeof_regression3, 1);
    clock_t start, end;
    double diff, bound;
    const uint32_t cmr[] = {
      0x872d12eeu, 0x631ae2e7u, 0xffb8b06au, 0xc54ef77fu, 0x693adbffu, 0xb229e760u, 0x111b8fd9u, 0x13d88b7au
    };
    simplicity_assert(regression3);
    regression3[0] = 0xb7; regression3[1] = 0x08;
    regression3[sizeof_regression3 - 2] = 0x48; regression3[sizeof_regression3 - 1] = 0x20;
    start = clock();
    test_program("regression3", regression3, sizeof_regression3, NULL, 0, SIMPLICITY_ERR_EXEC_MEMORY, cmr, NULL, NULL, NULL);
    end = clock();
    diff = (double)(end - start) / CLOCKS_PER_SEC;
    bound = (double)(sizeof_regression3) * secondsPerWU;
    printf("cpu_time_used by regression3: %f s.  (Should be less than %f s.)\n", diff, bound);
    if (timing_flag) {
      if (diff <= bound) {
        successes++;
      } else {
        failures++;
        printf("regression3 took too long.\n");
      }
    }
    simplicity_free(regression3);
  }
  {
    clock_t start, end;
    double diff, bound;
    start = clock();
    test_program("regression4", regression4, sizeof_regression4, NULL, 0, SIMPLICITY_NO_ERROR, NULL, NULL, NULL, NULL);
    end = clock();
    diff = (double)(end - start) / CLOCKS_PER_SEC;
    bound = (double)(sizeof_regression4) * secondsPerWU;
    printf("cpu_time_used by regression4: %f s.  (Should be less than %f s.)\n", diff, bound);
    if (timing_flag) {
      if (diff <= bound) {
        successes++;
      } else {
        failures++;
        printf("regression4 took too long.\n");
      }
    }
  }
}

static void iden8mebi_test(void) {
  /* iden composed with itself 2^23 times. */
  const unsigned char iden8mebi[23] = {0xe1, 0x08};
  const ubounded expectedCost = 1677721500; /* in milliWU */
  clock_t start, end;
  double diff, bound;
  start = clock();
  test_program("iden8mebi", iden8mebi, sizeof(iden8mebi), NULL, 0, SIMPLICITY_NO_ERROR, NULL, NULL, NULL, &expectedCost);
  end = clock();
  diff = (double)(end - start) / CLOCKS_PER_SEC;
  bound = (double)expectedCost / 1000. * secondsPerWU;

  printf("cpu_time_used by iden8mebi: %f s.  (Should be less than %f s.)\n", diff, bound);
  if (timing_flag) {
    if (diff <= bound) {
      successes++;
    } else {
      failures++;
      printf("iden8mebi took too long.\n");
    }
  }
}
static void exactBudget_test(void) {
  /* Core Simplicity program with a cost that is exactly 410000 milliWU */
  const unsigned char program[] = {
    0xe0, 0x09, 0x40, 0x81, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x81, 0x02,
    0x05, 0xb4, 0x6d, 0xa0, 0x80
  };
  const ubounded expectedCost = 410; /* in WU */

  printf("Test exactBudget\n");

  dag_node* dag;
  type* type_dag;
  combinator_counters census;
  int_fast32_t len;
  simplicity_err error, expected;
  sha256_midstate ihr;
  {
    bitstream stream = initializeBitstream(program, sizeof(program));
    len = simplicity_decodeMallocDag(&dag, simplicity_elements_decodeJet, &census, &stream);
    simplicity_assert(dag);
    simplicity_assert(0 < len);
    error = simplicity_closeBitstream(&stream);
    simplicity_assert(IS_OK(error));
  }
  error = simplicity_mallocTypeInference(&type_dag, simplicity_elements_mallocBoundVars, dag, (uint_fast32_t)len, &census);
  simplicity_assert(IS_OK(error));
  simplicity_assert(type_dag);
  simplicity_assert(!dag[len-1].sourceType);
  simplicity_assert(!dag[len-1].targetType);
  {
    bitstream stream = initializeBitstream(NULL, 0);
    error = simplicity_fillWitnessData(dag, type_dag, (uint_fast32_t)len, &stream);
    simplicity_assert(IS_OK(error));
  }
  error = simplicity_verifyNoDuplicateIdentityHashes(&ihr, dag, type_dag, (uint_fast32_t)len);
  simplicity_assert(IS_OK(error));
  {
    ubounded cellsBound, UWORDBound, frameBound, costBound;
    error = simplicity_analyseBounds(&cellsBound, &UWORDBound, &frameBound, &costBound, UBOUNDED_MAX, 0, UBOUNDED_MAX, dag, type_dag, (uint_fast32_t)len);
    simplicity_assert(IS_OK(error));
    simplicity_assert(1000*expectedCost == costBound);
  }
  error = evalTCOProgram(dag, type_dag, (uint_fast32_t)len, expectedCost, &expectedCost, NULL);
  expected = SIMPLICITY_ERR_OVERWEIGHT;
  if (expected == error) {
    successes++;
  } else {
    failures++;
    printf("Expected %d from evaluation, but got %d instead.\n", expected, error);
  }
  error = evalTCOProgram(dag, type_dag, (uint_fast32_t)len, expectedCost-1, &(ubounded){expectedCost-1}, NULL);
  expected = SIMPLICITY_ERR_EXEC_BUDGET;
  if (expected == error) {
    successes++;
  } else {
    failures++;
    printf("Expected %d from evaluation, but got %d instead.\n", expected, error);
  }
  error = evalTCOProgram(dag, type_dag, (uint_fast32_t)len, expectedCost, &(ubounded){expectedCost+1}, NULL);
  expected = SIMPLICITY_ERR_OVERWEIGHT;
  if (expected == error) {
    successes++;
  } else {
    failures++;
    printf("Expected %d from evaluation, but got %d instead.\n", expected, error);
  }
  error = evalTCOProgram(dag, type_dag, (uint_fast32_t)len, expectedCost-1, &expectedCost, NULL);
  expected = SIMPLICITY_NO_ERROR;
  if (expected == error) {
    successes++;
  } else {
    failures++;
    printf("Expected %d from evaluation, but got %d instead.\n", expected, error);
  }
  simplicity_free(type_dag);
  simplicity_free(dag);
}

int main(int argc, char **argv) {
  while (true) {
    static struct option long_options[] = {
        /* These options set a flag. */
        {"timing", no_argument, &timing_flag, 1},
        {"no-timing", no_argument, &timing_flag, 0},
        {0, 0, 0, 0}
      };
    int opt_result = getopt_long(argc, argv, "", long_options, NULL);
    if (-1 == opt_result) break;
    if (0 == opt_result) continue;
    exit(EXIT_FAILURE);
  }
  if (simplicity_sha256_compression_is_optimized()) {
    printf("Sha optimization enabled.\n");
    if (timing_flag) {
      printf("Timings are checked.\n");
    } else {
      printf("Timings not checked.\n");
    }
  } else {
    printf("Sha optimization disabled.\n");
    if (timing_flag) {
      printf("Timings cannot be checked.\n");
    } else {
      printf("Timings not checked.\n");
    }
    timing_flag = 0;
  }
  test_decodeUptoMaxInt();
  test_hashBlock();
  test_occursCheck();

  test_hasDuplicates("hasDuplicates no duplicates testcase", 0, rsort_no_duplicates, 10000);
  test_hasDuplicates("hasDuplicates all duplicates testcase", 1, rsort_all_duplicates, 10000);
  test_hasDuplicates("hasDuplicates one duplicate testcase", 1, rsort_one_duplicate, 10000);
  test_hasDuplicates("hasDuplicates diagonal testcase", 0, rsort_diagonal, 33);

  test_program("ctx8Pruned", ctx8Pruned, sizeof_ctx8Pruned, ctx8Pruned_witness, sizeof_ctx8Pruned_witness, SIMPLICITY_NO_ERROR, ctx8Pruned_cmr, ctx8Pruned_ihr, ctx8Pruned_amr, &ctx8Pruned_cost);
  test_program("ctx8Unpruned", ctx8Unpruned, sizeof_ctx8Unpruned, ctx8Unpruned_witness, sizeof_ctx8Unpruned_witness, SIMPLICITY_ERR_ANTIDOS, ctx8Unpruned_cmr, ctx8Unpruned_ihr, ctx8Unpruned_amr, &ctx8Unpruned_cost);
  if (0 == memcmp(ctx8Pruned_cmr, ctx8Unpruned_cmr, sizeof(uint32_t[8]))) {
    successes++;
  } else {
    failures++;
    printf("Pruned and Unpruned CMRs are not the same.\n");
  }
  test_program("schnorr0", schnorr0, sizeof_schnorr0, schnorr0_witness, sizeof_schnorr0_witness, SIMPLICITY_NO_ERROR, schnorr0_cmr, schnorr0_ihr, schnorr0_amr, &schnorr0_cost);
  test_program("schnorr6", schnorr6, sizeof_schnorr6, schnorr6_witness, sizeof_schnorr6_witness, SIMPLICITY_ERR_EXEC_JET, schnorr6_cmr, schnorr6_ihr, schnorr6_amr, &schnorr0_cost);
  test_program("typeSkipTest", typeSkipTest, sizeof_typeSkipTest, typeSkipTest_witness, sizeof_typeSkipTest_witness, SIMPLICITY_NO_ERROR, NULL, NULL, NULL, NULL);
  test_elements();
  exactBudget_test();
  regression_tests();
  iden8mebi_test();

  printf("Successes: %d\n", successes);
  printf("Failures: %d\n", failures);
  return (0 == failures) ? EXIT_SUCCESS : EXIT_FAILURE;
}
