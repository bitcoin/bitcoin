/* This module defines some constants used for error codes when processing Simplicity.
 * Errors can either indicate a transient or a permanent failure.
 */
#ifndef SIMPLICITY_ERRORCODES_H
#define SIMPLICITY_ERRORCODES_H

#include <stdbool.h>

/* By convention, odd error codes are transient failures (i.e. out of memory)
 * while even error codes are permanent failures (i.e. unexpected end of file or parsing error, etc.)
 */
typedef enum {
  SIMPLICITY_NO_ERROR = 0,
  SIMPLICITY_ERR_MALLOC = -1,
  SIMPLICITY_ERR_NOT_YET_IMPLEMENTED = -3,
  SIMPLICITY_ERR_DATA_OUT_OF_RANGE = -2,
  SIMPLICITY_ERR_DATA_OUT_OF_ORDER = -4,
  SIMPLICITY_ERR_FAIL_CODE = -6,
  SIMPLICITY_ERR_RESERVED_CODE = -8,
  SIMPLICITY_ERR_HIDDEN = -10,
  SIMPLICITY_ERR_BITSTREAM_EOF = -12,
  SIMPLICITY_ERR_BITSTREAM_TRAILING_BYTES = -14,
  SIMPLICITY_ERR_BITSTREAM_ILLEGAL_PADDING = -16,
  SIMPLICITY_ERR_TYPE_INFERENCE_UNIFICATION = -18,
  SIMPLICITY_ERR_TYPE_INFERENCE_OCCURS_CHECK = -20,
  SIMPLICITY_ERR_TYPE_INFERENCE_NOT_PROGRAM = -22,
  SIMPLICITY_ERR_WITNESS_EOF = -24,
  SIMPLICITY_ERR_WITNESS_TRAILING_BYTES = -26,
  SIMPLICITY_ERR_WITNESS_ILLEGAL_PADDING = -28,
  SIMPLICITY_ERR_UNSHARED_SUBEXPRESSION = -30,
  SIMPLICITY_ERR_CMR = -32,
  SIMPLICITY_ERR_EXEC_BUDGET = -34,
  SIMPLICITY_ERR_EXEC_MEMORY = -36,
  SIMPLICITY_ERR_EXEC_JET = -38,
  SIMPLICITY_ERR_EXEC_ASSERT = -40,
  SIMPLICITY_ERR_ANTIDOS = -42,
  SIMPLICITY_ERR_HIDDEN_ROOT = -44,
  SIMPLICITY_ERR_AMR = -46,
  SIMPLICITY_ERR_OVERWEIGHT = -48,
} simplicity_err;

/* Check if failure is permanent (or success which is always permanent). */
static inline bool IS_PERMANENT(simplicity_err err) {
  return !(err & 1);
}

/* Check if no failure. */
static inline bool IS_OK(simplicity_err err) {
  return SIMPLICITY_NO_ERROR == err;
}

static inline const char * SIMPLICITY_ERR_MSG(simplicity_err err) {
  switch (err) {
  case SIMPLICITY_NO_ERROR:
    return "No error";
  case SIMPLICITY_ERR_MALLOC:
    return "Memory allocation failed";
  case SIMPLICITY_ERR_NOT_YET_IMPLEMENTED:
    return "Incomplete implementation (this should not occur)";
  case SIMPLICITY_ERR_DATA_OUT_OF_RANGE:
    return "Value out of range";
  case SIMPLICITY_ERR_DATA_OUT_OF_ORDER:
    return "Non-canonical order";
  case SIMPLICITY_ERR_FAIL_CODE:
    return "Program has FAIL node";
  case SIMPLICITY_ERR_RESERVED_CODE:
    return "Program has reserved codeword";
  case SIMPLICITY_ERR_HIDDEN:
    return "Program has node with a HIDDEN child in a position where it is not allowed";
  case SIMPLICITY_ERR_BITSTREAM_EOF:
    return "Unexpected end of bitstream";
  case SIMPLICITY_ERR_BITSTREAM_TRAILING_BYTES:
    return "Trailing bytes after final byte of program";
  case SIMPLICITY_ERR_BITSTREAM_ILLEGAL_PADDING:
    return "Illegal padding in final byte of program";
  case SIMPLICITY_ERR_TYPE_INFERENCE_UNIFICATION:
    return "Unification failure";
  case SIMPLICITY_ERR_TYPE_INFERENCE_OCCURS_CHECK:
    return "Occurs check failure";
  case SIMPLICITY_ERR_TYPE_INFERENCE_NOT_PROGRAM:
    return "Expression not unit to unit";
  case SIMPLICITY_ERR_WITNESS_EOF:
    return "Unexpected end of witness block";
  case SIMPLICITY_ERR_WITNESS_TRAILING_BYTES:
    return "Trailing bytes after final byte of witness";
  case SIMPLICITY_ERR_WITNESS_ILLEGAL_PADDING:
    return "Illegal padding in final byte of witness";
  case SIMPLICITY_ERR_UNSHARED_SUBEXPRESSION:
    return "Subexpression not properly shared";
  case SIMPLICITY_ERR_CMR:
    return "Program's CMR does not match";
  case SIMPLICITY_ERR_EXEC_BUDGET:
    return "Program's execution cost could exceed budget";
  case SIMPLICITY_ERR_EXEC_MEMORY:
    return "Program's memory cost could exceed limit";
  case SIMPLICITY_ERR_EXEC_JET:
    return "Assertion failed inside jet";
  case SIMPLICITY_ERR_EXEC_ASSERT:
    return "Assertion failed";
  case SIMPLICITY_ERR_ANTIDOS:
    return "Anti-DOS check failed";
  case SIMPLICITY_ERR_HIDDEN_ROOT:
    return "Program's root is HIDDEN";
  case SIMPLICITY_ERR_AMR:
    return "Program's AMR does not match";
  case SIMPLICITY_ERR_OVERWEIGHT:
    return "Program's budget is too large";
  default:
    return "Unknown error code";
  }
}

#endif
