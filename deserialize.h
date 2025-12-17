/* This module provides functions for deserializing Simplicity's bit-wise prefix coding. */
#ifndef SIMPLICITY_DESERIALIZE_H
#define SIMPLICITY_DESERIALIZE_H

#include <simplicity/errorCodes.h>
#include "bitstream.h"
#include "dag.h"

/* Decode an application specific jet from 'stream' into 'node'.
 * All jets begin with a bit prefix of '1' which needs to have already been consumed from the 'stream'.
 * Returns 'SIMPLICITY_ERR_DATA_OUT_OF_RANGE' if the stream's prefix doesn't match any valid code for a jet.
 * Returns 'SIMPLICITY_ERR_BITSTRING_EOF' if not enough bits are available in the 'stream'.
 * In the above error cases, 'dag' may be modified.
 * Returns 'SIMPLICITY_NO_ERROR' if successful.
 *
 * Precondition: NULL != node
 *               NULL != stream
 */
typedef simplicity_err (*simplicity_callback_decodeJet)(dag_node* node, bitstream* stream);

/* Decode a length-prefixed Simplicity DAG from 'stream'.
 * Returns 'SIMPLICITY_ERR_DATA_OUT_OF_RANGE' the length prefix's value is too large.
 * Returns 'SIMPLICITY_ERR_DATA_OUT_OF_RANGE' if some node's child isn't a reference to one of the preceding nodes.
 * Returns 'SIMPLICITY_ERR_FAIL_CODE' if the encoding of a fail expression is encountered
 *  (all fail subexpressions ought to have been pruned prior to deserialization).
 * Returns 'SIMPLICITY_ERR_RESERVED_CODE' if a reserved codeword is encountered.
 * Returns 'SIMPLICITY_ERR_HIDDEN' if the decoded node has a HIDDEN child in a position where it is not allowed.
 * Returns 'SIMPLICITY_ERR_HIDDEN_ROOT' if the root of the DAG is a HIDDEN node.
 * Returns 'SIMPLICITY_ERR_BITSTRING_EOF' if not enough bits are available in the 'stream'.
 * Returns 'SIMPLICITY_ERR_MALLOC' if malloc fails.
 * In the above error cases, '*dag' is set to NULL.
 * If successful, returns a positive value equal to the length of an allocated array of (*dag).
 *
 * Precondition: NULL != dag
 *               NULL != stream
 *
 * Postcondition: if the return value of the function is positive
 *                  then (dag_node (*dag)[return_value] and '*dag' is a well-formed dag without witness data);
 *                '*census' contains a tally of the different tags that occur in 'dag' when the return value
 *                          of the function is positive and when NULL != census;
 *                NULL == *dag when the return value is negative.
 */
int_fast32_t simplicity_decodeMallocDag(dag_node** dag, simplicity_callback_decodeJet decodeJet, combinator_counters* census, bitstream* stream);

#endif
