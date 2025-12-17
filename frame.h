/* This module provides functions writing initial data to and reading results from the frame used during evaluation
 * of Simplicity expressions.
 * These helper functions are also used for marshaling data to and from jets.
 */
#ifndef SIMPLICITY_FRAME_H
#define SIMPLICITY_FRAME_H

#include <stdbool.h>
#include "sha256.h"
#include "simplicity_assert.h"
#include "uword.h"

/* A Bit Machine frame contains an '.edge' pointer to a slice of an array of UWORDs to hold the frame's cells.
 * The '.offset' is used to represent the cursors position.
 * For a read frame, the '.edge' points to one-past-the-end of the slice of the UWORDs array for the frame's cells,
 * and the '.offset' value is equal to the frame's cursor position plus the amount of padding used in the frame.
 * For a write frame, the '.edge' points to the beginning of the slice of the UWORDs array for the frame's cells,
 * and the '.offset' value is equal to the total number of cells minus the frame's cursor position.
 */
typedef struct frameItem {
  UWORD* edge;
  size_t offset;
} frameItem;

/* Initialize a new read frame.
 * 'n' is the number of cells for the read frame.
 * 'from' is a pointer to the beginning of the new slice for the array of UWORDS to hold the frame's cells.
 *
 * Precondition: UWORD from[ROUND_UWORD(n)];
 */
static inline frameItem initReadFrame(size_t n, UWORD* from) {
  const size_t len = ROUND_UWORD(n);
/* '(1U * len) * UWORD_BIT - n' equals the number of padding bits in a frame of size 'n'.
 * Note that even if (len * UWORD_BIT) overflows,
 * (1) We have ensured an unsigned computation by multiplying by 1U so the behaviour is well-defined.
 * (2) after subtracting 'n' we are left with a value in the range 0 .. UWORD-BIT - 1.
 */
  return (frameItem){ .edge = from + len, .offset = (1U * len) * UWORD_BIT - n };
}

/* Initialize a new write frame.
 * 'n' is the number of cells for the write frame.
 * 'from' is a pointer to the one-past-the-end of the new slice for the array of UWORDS to hold the frame's cells.
 *
 * Precondition: UWORD (from - ROUND_UWORD(n))[ROUND_UWORD(n)];
 */
static inline frameItem initWriteFrame(size_t n, UWORD* from) {
  return (frameItem){ .edge = from - ROUND_UWORD(n), .offset = n };
}

/* Given a read frame, return the value of the cell at the cursor.
 *
 * Precondition: '*frame' is a valid read frame for 1 more cell.
 */
static inline bool peekBit(const frameItem* frame) {
  return 1 & (*(frame->edge - 1 - frame->offset / UWORD_BIT) >> (UWORD_BIT - (frame->offset % UWORD_BIT) - 1));
}

/* Given a read frame, return the value of the cell at the cursor and advance the cursor by one cell.
 *
 * Precondition: '*frame' is a valid read frame for 1 more cell.
 */
static inline bool readBit(frameItem* frame) {
  bool result = peekBit(frame);
  frame->offset++;
  return result;
}

/* Given a write frame, set its cursor's cell to 'bit' and advance the cursor by one cell.
 * Cells in front of the cursor's final position may be overwritten.
 *
 * The function returns the same value as bit.  This facilitates using 'writeBit' within an 'if' statement
 *
 *     if (writeBit(frame, bit)) { ... } else { ... }
 *
 * so that one can both decide conditions based on a Boolean value while at the same time writing to the frame the choice made.
 *
 * Precondition: '*frame' is a valid write frame for 1 more cell.
 */
static inline bool writeBit(frameItem* frame, bool bit) {
  simplicity_debug_assert(0 < frame->offset);
  frame->offset--;
  UWORD* dst_ptr = frame->edge + frame->offset / UWORD_BIT;
  if (bit) {
    *dst_ptr |= (UWORD)((UWORD)1 << (frame->offset % UWORD_BIT));
  } else {
    *dst_ptr = LSBclear(*dst_ptr, frame->offset % UWORD_BIT + 1);
  }
  return bit;
}

/* Given a read frame, advance the cursor by 'n' cells.
 *
 * Precondition: '*frame' is a valid read frame for 'n' more cells.
 */
static inline void forwardBits(frameItem* frame, size_t n) {
  frame->offset += n;
}

/* Given a write frame, advance the cursor by 'n' cells.
 *
 * Precondition: '*frame' is a valid write frame for 'n' more cells.
 */
static inline void skipBits(frameItem* frame, size_t n) {
  simplicity_debug_assert(n <= frame->offset);
  frame->offset -= n;
}

/* Given a read frame, the 'readN' function returns the value of the 'N' cells after the cursor and
 * advances the frame's cursor by 'N'.
 * The cell values are returned with the first cell in the MSB of the result and the last cell in the LSB of the result.
 *
 * Precondition: '*frame' is a valid read frame for 'N' more cells.
 */
uint_fast8_t simplicity_read4(frameItem* frame);
uint_fast8_t simplicity_read8(frameItem* frame);
uint_fast16_t simplicity_read16(frameItem* frame);
uint_fast32_t simplicity_read32(frameItem* frame);
uint_fast64_t simplicity_read64(frameItem* frame);

/* Given a write frame, the 'writeN' function sets the value of the 'N' cells after the cursor and
 * advances the frame's cursor by 'N'.
 * The first cell is set to the value of the MSB of 'x' and the last cell is set to the LSB of 'x'.
 * Cells in front of the cursor's final position may be overwritten.
 *
 * Precondition: '*frame' is a valid write frame for 'N' more cells.
 */
void simplicity_write8(frameItem* frame, uint_fast8_t x);
void simplicity_write16(frameItem* frame, uint_fast16_t x);
void simplicity_write32(frameItem* frame, uint_fast32_t x);
void simplicity_write64(frameItem* frame, uint_fast64_t x);

static inline void read8s(unsigned char* x, size_t n, frameItem* frame) {
  for(; n; --n) *(x++) = (unsigned char)simplicity_read8(frame);
}

static inline void write8s(frameItem* frame, const unsigned char* x, size_t n) {
  for(; n; --n) simplicity_write8(frame, *(x++));
}

static inline void read32s(uint32_t* x, size_t n, frameItem* frame) {
  for(; n; --n) *(x++) = (uint32_t)simplicity_read32(frame);
}

static inline void write32s(frameItem* frame, const uint32_t* x, size_t n) {
  for(; n; --n) simplicity_write32(frame, *(x++));
}

/* Read bytes from a Simplicity buffer of type (TWO^8)^<2^(n+1) into 'buf'.
 * Set 'len' to the number of bytes read from the buffer.
 * Advance the 'src' frame to the end of the buffer type.
 *
 * The notation X^<2 is notation for the type (S X)
 * The notation X^<(2*n) is notation for the type S (X^n) * X^<n
 *
 * Precondition: unsigned char buf[2^(n+1)-1];
 *               NULL != len;
 *               '*src' is a valid read frame for 8*(2^(n+1)-1)+n+1 more cells;
 *               0 <= n < 16
 */
void simplicity_read_buffer8(unsigned char* buf, size_t* len, frameItem* src, int n);

/* Write 'len' bytes to a Simplicity buffer of type (TWO^8)^<2^(n+1) from 'buf'.
 * Advance the 'dst' frame to the end of the buffer type.
 *
 * The notation X^<2 is notation for the type (S X)
 * The notation X^<(2*n) is notation for the type S (X^n) * X^<n
 *
 * Precondition: '*dst' is a valid write frame for 8*(2^(n+1)-1)+n+1 more cells;
 *               unsigned char buf[len];
 *               len < 2^(n+1);
 *               0 <= n < 16;
 */
void simplicity_write_buffer8(frameItem* dst, const unsigned char* buf, size_t len, int n);

/* Read data from a Simplicity CTX8 type (TWO^8)^<2^64 * TWO^64 * TWO^256 and fill in a sha256_context value.
 * Advance the 'src' frame to the end of the CTX8 type.
 * Returns false if the context's counter is too large (i.e. the compression count is greater than or equal to 2^55).
 *
 * The notation X^<2 is notation for the type (S X)
 * The notation X^<(2*n) is notation for the type S (X^n) * X^<n
 *
 * Precondition: NULL != ctx->output;
 *               '*src' is a valid read frame for 838 more cells;
 */
bool simplicity_read_sha256_context(sha256_context* ctx, frameItem* src);

/* Write data to a Simplicity CTX8 type (TWO^8)^<2^64 * TWO^64 * TWO^256 from a sha256_context value.
 * Advance the 'dst' frame to the end of the CTX8 type.
 * Returns false if the ctx had overflowed.
 *
 * The notation X^<2 is notation for the type (S X)
 * The notation X^<(2*n) is notation for the type S (X^n) * X^<n
 *
 * Precondition: '*dst' is a valid write frame for 838 more cells;
 *               NULL != ctx->output;
 *               ctx->counter < 2^61;
 */
bool simplicity_write_sha256_context(frameItem* dst, const sha256_context* ctx);

/* Given a write frame and a read frame, copy 'n' cells from after the read frame's cursor to after the write frame's cursor,
 * and then advance the write frame's cursor by 'n'.
 * Cells in front of the '*dst's cursor's final position may be overwritten.
 *
 * Precondition: '*dst' is a valid write frame for 'n' more cells;
 *               '*src' is a valid read frame for 'n' more cells;
 */
void simplicity_copyBits(frameItem* dst, const frameItem* src, size_t n);
#endif
