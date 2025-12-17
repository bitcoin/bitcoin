#include "bitstream.h"

#include <limits.h>
#include "simplicity_assert.h"

/* Closes a bitstream by consuming all remaining bits.
 * Returns 'SIMPLICITY_ERR_BITSTREAM_TRAILING_BYTES' if CHAR_BIT or more bits remain in the stream.
 * Otherwise, returns 'SIMPLICITY_ERR_BITSTREAM_ILLEGAL_PADDING' if any remaining bits are non-zero.
 * Otherwise returns 'SIMPLICITY_NO_ERROR'.
 *
 * Precondition: NULL != stream
 */
simplicity_err simplicity_closeBitstream(bitstream* stream) {
  if (1 < stream->len) return SIMPLICITY_ERR_BITSTREAM_TRAILING_BYTES;        /* If there is more than one byte remaining. */
  if (1 == stream->len) {
    if (0 == stream->offset) return SIMPLICITY_ERR_BITSTREAM_TRAILING_BYTES;  /* If there is one byte remaining */
    if (0 != (*stream->arr & (UCHAR_MAX >> stream->offset))) {                /* If any of the unconsumed bits are non-zero */
      return SIMPLICITY_ERR_BITSTREAM_ILLEGAL_PADDING;
    }
  }
  /* Otherwise there are either 0 bits remaining or there are between 1 and CHAR_BITS-1 bits remaining and they are all zero. */
  *stream = (bitstream){0};
  return SIMPLICITY_NO_ERROR;
}

/* Fetches up to 31 bits from 'stream' as the 'n' least significant bits of return value.
 * The 'n' bits are set from the MSB to the LSB.
 * Returns 'SIMPLICITY_ERR_BITSTREAM_EOF' if not enough bits are available.
 *
 * Precondition: 0 <= n < 32
 *               NULL != stream
 */
int32_t simplicity_readNBits(int n, bitstream* stream) {
  simplicity_assert(0 <= n && n < 32);

  uint32_t result = 0;
  while (CHAR_BIT <= stream->offset + n) {
    if (!stream->len) return SIMPLICITY_ERR_BITSTREAM_EOF;
    n -= CHAR_BIT - stream->offset;
    result |= (uint32_t)(*stream->arr & (UCHAR_MAX >> stream->offset)) << n;
    stream->arr++; stream->len--; stream->offset = 0;
  }
  /* stream->offset + n < CHAR_BIT */
  if (n) {
    if (!stream->len) return SIMPLICITY_ERR_BITSTREAM_EOF;
    stream->offset += (unsigned char)n;
    result |= (*stream->arr >> (CHAR_BIT - stream->offset)) & ((UCHAR_MAX >> (CHAR_BIT - n)));
  }
  return (int32_t)result;
}
/* Decode an encoded bitstring up to length 1.
 * If successful returns the length of the bitstring and 'result' contains the decoded bits.
 * The decoded bitstring is stored in the LSBs of 'result', with the LSB being the last bit decoded.
 * Any remaining bits in 'result' are reset to 0.
 * If the decoded bitstring would be too long 'SIMPLICITY_ERR_DATA_OUT_OF_RANGE' is returned ('result' may be modified).
 * If more bits are needed than available in the 'stream', 'SIMPLICITY_ERR_BITSTRING_EOF' is returned ('result' may be modified).
 * If an I/O error occurs when reading from the 'stream', 'SIMPLICITY_ERR_BISTRING_ERROR' is returned ('result' may be modified).
 *
 * Precondition: NULL != result
 *               NULL != stream
 */
static int32_t decodeUpto1Bit(int32_t* result, bitstream* stream) {
  *result = read1Bit(stream);
  if (*result <= 0) return *result;

  *result = read1Bit(stream);
  if (*result < 0) return *result;
  if (0 != *result) return SIMPLICITY_ERR_DATA_OUT_OF_RANGE;

  *result = read1Bit(stream);
  if (*result < 0) return *result;
  return 1;
}

/* Decode an encoded number between 1 and 3 inclusive.
 * When successful returns the decoded result.
 * If the decoded value would be too large, 'SIMPLICITY_ERR_DATA_OUT_OF_RANGE' is returned.
 * If more bits are needed than available in the 'stream', 'SIMPLICITY_ERR_BITSTRING_EOF' is returned.
 * If an I/O error occurs when reading from the 'stream', 'SIMPLICITY_ERR_BISTRING_ERROR' is returned.
 *
 * Precondition: NULL != stream
 */
static int32_t decodeUpto3(bitstream* stream) {
  int32_t result;
  int32_t len = decodeUpto1Bit(&result, stream);
  if (len < 0) return len;
  result |= 1 << len;
  return result;
}

/* Decode an encoded bitstring up to length 3.
 * If successful returns the length of the bitstring and 'result' contains the decoded bits.
 * The decoded bitstring is stored in the LSBs of 'result', with the LSB being the last bit decoded.
 * Any remaining bits in 'result' are reset to 0.
 * If the decoded bitstring would be too long 'SIMPLICITY_ERR_DATA_OUT_OF_RANGE' is returned ('result' may be modified).
 * If more bits are needed than available in the 'stream', 'SIMPLICITY_ERR_BITSTRING_EOF' is returned ('result' may be modified).
 * If an I/O error occurs when reading from the 'stream', 'SIMPLICITY_ERR_BISTRING_ERROR' is returned ('result' may be modified).
 *
 * Precondition: NULL != result
 *               NULL != stream
 */
static int32_t decodeUpto3Bits(int32_t* result, bitstream* stream) {
  int32_t bit = read1Bit(stream);
  if (bit < 0) return bit;

  *result = 0;
  if (0 == bit) {
    return 0;
  } else {
    int32_t n = decodeUpto3(stream);
    if (0 <= n) {
      *result = simplicity_readNBits(n, stream);
      if (*result < 0) return *result;
    }
    return n;
  }
}

/* Decode an encoded number between 1 and 15 inclusive.
 * When successful returns the decoded result.
 * If the decoded value would be too large, 'SIMPLICITY_ERR_DATA_OUT_OF_RANGE' is returned.
 * If more bits are needed than available in the 'stream', 'SIMPLICITY_ERR_BITSTRING_EOF' is returned.
 * If an I/O error occurs when reading from the 'stream', 'SIMPLICITY_ERR_BISTRING_ERROR' is returned.
 *
 * Precondition: NULL != stream
 */
static int32_t decodeUpto15(bitstream* stream) {
  int32_t result;
  int32_t len = decodeUpto3Bits(&result, stream);
  if (len < 0) return len;
  result |= 1 << len;
  return result;
}

/* Decode an encoded bitstring up to length 15.
 * If successful returns the length of the bitstring and 'result' contains the decoded bits.
 * The decoded bitstring is stored in the LSBs of 'result', with the LSB being the last bit decoded.
 * Any remaining bits in 'result' are reset to 0.
 * If the decoded bitstring would be too long 'SIMPLICITY_ERR_DATA_OUT_OF_RANGE' is returned ('result' may be modified).
 * If more bits are needed than available in the 'stream', 'SIMPLICITY_ERR_BITSTRING_EOF' is returned ('result' may be modified).
 * If an I/O error occurs when reading from the 'stream', 'SIMPLICITY_ERR_BISTRING_ERROR' is returned ('result' may be modified).
 *
 * Precondition: NULL != result
 *               NULL != stream
 */
static int32_t decodeUpto15Bits(int32_t* result, bitstream* stream) {
  int32_t bit = read1Bit(stream);
  if (bit < 0) return bit;

  *result = 0;
  if (0 == bit) {
    return 0;
  } else {
    int32_t n = decodeUpto15(stream);
    if (0 <= n) {
      *result = simplicity_readNBits(n, stream);
      if (*result < 0) return *result;
    }
    return n;
  }
}

/* Decode an encoded number between 1 and 65535 inclusive.
 * When successful returns the decoded result.
 * If the decoded value would be too large, 'SIMPLICITY_ERR_DATA_OUT_OF_RANGE' is returned.
 * If more bits are needed than available in the 'stream', 'SIMPLICITY_ERR_BITSTRING_EOF' is returned.
 * If an I/O error occurs when reading from the 'stream', 'SIMPLICITY_ERR_BISTRING_ERROR' is returned.
 *
 * Precondition: NULL != stream
 */
static int32_t decodeUpto65535(bitstream* stream) {
  int32_t result;
  int32_t len = decodeUpto15Bits(&result, stream);
  if (len < 0) return len;
  result |= 1 << len;
  return result;
}

/* Decode an encoded number between 1 and 2^31 - 1 inclusive.
 * When successful returns the decoded result.
 * If the decoded value would be too large, 'SIMPLICITY_ERR_DATA_OUT_OF_RANGE' is returned.
 * If more bits are needed than available in the 'stream', 'SIMPLICITY_ERR_BITSTRING_EOF' is returned.
 * If an I/O error occurs when reading from the 'stream', 'SIMPLICITY_ERR_BISTRING_ERROR' is returned.
 *
 * Precondition: NULL != stream
 */
int32_t simplicity_decodeUptoMaxInt(bitstream* stream) {
  int32_t bit = read1Bit(stream);
  if (bit < 0) return bit;
  if (0 == bit) {
    return 1;
  } else {
    int32_t n = decodeUpto65535(stream);
    if (n < 0) return n;
    if (30 < n) return SIMPLICITY_ERR_DATA_OUT_OF_RANGE;
    {
      int32_t result = simplicity_readNBits(n, stream);
      if (result < 0) return result;
      return ((1 << n) | result);
    }
  }
}

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
simplicity_err simplicity_readBitstring(bitstring* result, size_t n, bitstream* stream) {
  static_assert(0x80000000u + 2*(CHAR_BIT - 1) <= SIZE_MAX, "size_t needs to be at least 32-bits");
  simplicity_assert(n <= 0x80000000u);
  size_t total_offset = n + stream->offset;
  /* |= stream->len * CHAR_BIT < total_offset iff stream->len < (total_offset + (CHAR_BIT - 1)) / CHAR_BIT */
  if (stream->len < (total_offset + (CHAR_BIT - 1)) / CHAR_BIT) return SIMPLICITY_ERR_BITSTREAM_EOF;
  /* total_offset <= stream->len * CHAR_BIT */
  *result = (bitstring)
          { .arr = stream->arr
          , .offset = stream->offset
          , .len = n
          };
  {
    size_t delta = total_offset / CHAR_BIT;
    stream->arr += delta; stream->len -= delta;
    stream->offset = total_offset % CHAR_BIT;
    /* Note that if 0 == stream->len then 0 == stream->offset. */
  }
  return SIMPLICITY_NO_ERROR;
}
