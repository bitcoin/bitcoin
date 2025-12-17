/* This module provides functions for initializing and reading from a stream of bits from a 'FILE'. */
#ifndef SIMPLICITY_BITSTREAM_H
#define SIMPLICITY_BITSTREAM_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <simplicity/errorCodes.h>
#include "bitstring.h"

/* :TODO: consider adding an 'invalid' state that can be set when parsing has failed and should not be resumed. */
/* Datatype representing a bit stream.
 * Bits are streamed from MSB to LSB.
 *
 * Invariant: unsigned char arr[len]
 *            0 <= offset < CHAR_BIT
 *            0 == len implies 0 == offset
 */
typedef struct bitstream {
  const unsigned char *arr;  /* Underlying byte array */
  size_t len;                /* Length of arr (in bytes) */
  unsigned char offset;      /* Number of bits parsed from the beginning of arr */
} bitstream;

/* Initialize a bit stream, 'stream', from a given byte array.
 * Precondition: unsigned char arr[len];
 */
static inline bitstream initializeBitstream(const unsigned char* arr, size_t len) {
  return (bitstream){ .arr = arr, .len = len };
}

/* Closes a bitstream by consuming all remaining bits.
 * Returns 'SIMPLICITY_ERR_BITSTREAM_TRAILING_BYTES' if CHAR_BIT or more bits remain in the stream.
 * Otherwise, returns 'SIMPLICITY_ERR_BITSTREAM_ILLEGAL_PADDING' if any remaining bits are non-zero.
 * Otherwise returns 'SIMPLICITY_NO_ERROR'.
 *
 * Precondition: NULL != stream
 */
simplicity_err simplicity_closeBitstream(bitstream* stream);

/* Fetches up to 31 bits from 'stream' as the 'n' least significant bits of return value.
 * The 'n' bits are set from the MSB to the LSB.
 * Returns 'SIMPLICITY_ERR_BITSTREAM_EOF' if not enough bits are available.
 *
 * Precondition: 0 <= n < 32
 *               NULL != stream
 */
int32_t simplicity_readNBits(int n, bitstream* stream);

/* Returns one bit from 'stream', 0 or 1.
 * Returns 'SIMPLICITY_ERR_BITSTREAM_EOF' if no bits are available.
 *
 * Precondition: NULL != stream
 */
static inline int32_t read1Bit(bitstream* stream) {
  return simplicity_readNBits(1, stream);
}

/* Decode an encoded number between 1 and 2^31 - 1 inclusive.
 * When successful returns the decoded result.
 * If the decoded value would be too large, 'SIMPLICITY_ERR_DATA_OUT_OF_RANGE' is returned.
 * If more bits are needed than available in the 'stream', 'SIMPLICITY_ERR_BITSTRING_EOF' is returned.
 * If an I/O error occurs when reading from the 'stream', 'SIMPLICITY_ERR_BISTRING_ERROR' is returned.
 *
 * Precondition: NULL != stream
 */
int32_t simplicity_decodeUptoMaxInt(bitstream* stream);

/* Fills a 'bitstring' containing 'n' bits from 'stream'.
 * Returns 'SIMPLICITY_ERR_BITSTREAM_EOF' if not enough bits are available.
 * If successful, '*result' is set to a bitstring with 'n' bits read from 'stream' and 'SIMPLICITY_NO_ERROR' is returned.
 *
 * If an error is returned '*result' might be modified.
 *
 * Precondition: NULL != result
 *               n <= 2^31
 *               NULL != stream
 */
simplicity_err simplicity_readBitstring(bitstring* result, size_t n, bitstream* stream);
#endif
