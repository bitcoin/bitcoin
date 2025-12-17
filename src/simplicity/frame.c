#include "frame.h"

#define READ_(bits,size)                                                    \
/* Given a read frame, return the value of the bits cells after the cursor and advance the frame's cursor by bits##.          \
 * The cell values are returned with the first cell in the MSB of the result and the last cell in the LSB of the result.      \
 *                                                                                                                            \
 * Precondition: '*frame' is a valid read frame for bits more cells.                                                          \
 */                                                                                                                           \
uint_fast##size##_t simplicity_read##bits(frameItem* frame) {                                                                 \
  static_assert(bits <= size, "Return type too small to hold the requested number of bits.");                                 \
  uint_fast##size##_t result = 0;                                                                                             \
  /* Pointers to the UWORD of the read frame that contains the frame's cursor (or is immediately after the cursor). */        \
  UWORD* frame_ptr = frame->edge - 1 - frame->offset / UWORD_BIT;                                                             \
  /* The specific bit within the above UWORD that is immediately in front of the cursor.                                      \
   * That bit is specifically '1 << (frame_shift - 1)'.                                                                       \
   */                                                                                                                         \
  size_t frame_shift = UWORD_BIT - (frame->offset % UWORD_BIT);                                                               \
  size_t n = bits;                                                                                                            \
  if (frame_shift < n) {                                                                                                      \
    /* We may only want part of the LSBs from the read frame's current UWORD.                                                 \
     * Copy that data to 'x', and move the frame_ptr to the frame's next UWORD.                                               \
     */                                                                                                                       \
    result |= (uint_fast##size##_t)((uint_fast##size##_t)LSBkeep(*frame_ptr, frame_shift) << (n - frame_shift));              \
    frame->offset += frame_shift;                                                                                             \
    n -= frame_shift;                                                                                                         \
    frame_shift = UWORD_BIT;                                                                                                  \
    frame_ptr--;                                                                                                              \
    while (UWORD_BIT < n) {                                                                                                   \
      /* Copy the entire read frame's current UWORD to 'x', and move the frame_ptr to the frame's next UWORD. */              \
      result |= (uint_fast##size##_t)((uint_fast##size##_t)(*frame_ptr) << (n - UWORD_BIT));                                  \
      frame->offset += UWORD_BIT;                                                                                             \
      n -= UWORD_BIT;                                                                                                         \
      frame_ptr--;                                                                                                            \
    }                                                                                                                         \
  }                                                                                                                           \
  /* We may only want part of the bits from the middle of the read frame's current UWORD.                                     \
   * Copy that data to fill the remainder of 'x'.                                                                             \
   */                                                                                                                         \
  result |= (uint_fast##size##_t)(LSBkeep((UWORD)(*frame_ptr >> (frame_shift - n)), n));                                      \
  frame->offset += n;                                                                                                         \
  return result;                                                                                                              \
}
READ_(4,8)
READ_(8,8)
READ_(16,16)
READ_(32,32)
READ_(64,64)

#define WRITE_(bits)                                                                                                          \
/* Given a write frame, set the value of the bits cells after the cursor and advance the frame's cursor by bits##.            \
 * The first cell is set to the value of the MSB of 'x' and the last cell is set to the LSB of 'x'.                           \
 * Cells in front of the cursor's final position may be overwritten.                                                          \
 *                                                                                                                            \
 * Precondition: '*frame' is a valid write frame for bits more cells.                                                         \
 */                                                                                                                           \
void simplicity_write##bits(frameItem* frame, uint_fast##bits##_t x) {                                                        \
  /* Pointers to the UWORD of the write frame that contains the frame's cursor (or is immediately after the cursor). */       \
  UWORD* frame_ptr = frame->edge + (frame->offset - 1) / UWORD_BIT;                                                           \
  /* The specific bit within the above UWORD that is immediately in front of the cursor.                                      \
   * That bit is specifically '1 << (frame_shift - 1)'.                                                                       \
   */                                                                                                                         \
  size_t frame_shift = (frame->offset - 1) % UWORD_BIT + 1;                                                                   \
  size_t n = bits;                                                                                                            \
  if (frame_shift < n) {                                                                                                      \
    /* The write frame's current UWORD may be partially filled.                                                               \
     * Fill the rest of it with data from 'x', and move the frame_ptr to the frame's next UWORD.                              \
     */                                                                                                                       \
    *frame_ptr = LSBclear(*frame_ptr, frame_shift) | LSBkeep((UWORD)(x >> (n - frame_shift)), frame_shift);                   \
    frame->offset -= frame_shift;                                                                                             \
    n -= frame_shift;                                                                                                         \
    frame_shift = UWORD_BIT;                                                                                                  \
    frame_ptr--;                                                                                                              \
    while (UWORD_BIT < n) {                                                                                                   \
      /* Fill the write frame's entire current UWORD with data from 'x', and move the frame_ptr to the frame's next UWORD. */ \
      *frame_ptr = (UWORD)(x >> (n - UWORD_BIT));                                                                             \
      frame->offset -= UWORD_BIT;                                                                                             \
      n -= UWORD_BIT;                                                                                                         \
      frame_ptr--;                                                                                                            \
    }                                                                                                                         \
  }                                                                                                                           \
  /* The current write frame's UWORD may be partially filled.                                                                 \
   * Fill the UWORD with the last of the data from 'x', which may or may not be enough to fill the rest of the UWORD.         \
   */                                                                                                                         \
  *frame_ptr = (UWORD)(LSBclear(*frame_ptr, frame_shift) | (LSBkeep((UWORD)x, n) << (frame_shift - n)));                      \
  frame->offset -= n;                                                                                                         \
}
WRITE_(8)
WRITE_(16)
WRITE_(32)
WRITE_(64)

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
void simplicity_read_buffer8(unsigned char* buf, size_t* len, frameItem* src, int n) {
  simplicity_debug_assert(0 <= n && n < 16);
  *len = 0;

  for (size_t i = (size_t)1 << n; 0 < i; i /= 2) {
    if (readBit(src)) {
      read8s(buf, i, src);
      buf += i; *len += i;
    } else {
      forwardBits(src, i*8);
    }
  }
}

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
void simplicity_write_buffer8(frameItem* dst, const unsigned char* buf, size_t len, int n) {
  simplicity_debug_assert(0 <= n && n < 16);
  simplicity_debug_assert(len < ((size_t)1<<(n+1)));
  for (size_t i = (size_t)1 << n; 0 < i; i /= 2) {
    if (writeBit(dst, i <= len)) {
      write8s(dst, buf, i);
      buf += i; len -= i;
    } else {
      skipBits(dst, i*8);
    }
  }
}

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
bool simplicity_read_sha256_context(sha256_context* ctx, frameItem* src) {
  size_t len;
  uint_fast64_t compressionCount;

  simplicity_read_buffer8(ctx->block, &len, src, 5);
  compressionCount = simplicity_read64(src);
  ctx->counter = ((compressionCount*1U) << 6) + len;
  read32s(ctx->output, 8, src);
  ctx->overflow = (sha256_max_counter >> 6) <= compressionCount;
  return !ctx->overflow;
}

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
bool simplicity_write_sha256_context(frameItem* dst, const sha256_context* ctx) {
  simplicity_write_buffer8(dst, ctx->block, ctx->counter % 64, 5);
  simplicity_write64(dst, ctx->counter >> 6);
  write32s(dst, ctx->output, 8);
  return !ctx->overflow;
}

/* Given a write frame and a read frame, copy 'n' cells from after the read frame's cursor to after the write frame's cursor,
 * Cells in within the write frame beyond 'n' cells after the write frame's cursor may also be overwritten.
 *
 * Precondition: '*dst' is a valid write frame for 'n' more cells;
 *               '*src' is a valid read frame for 'n' more cells;
 *               '0 < n';
 */
static void copyBitsHelper(const frameItem* dst, const frameItem *src, size_t n) {
  /* Pointers to the UWORDs of the read and write frame that contain the frame's cursor. */
  UWORD* src_ptr = src->edge - 1 - src->offset / UWORD_BIT;
  UWORD* dst_ptr = dst->edge + (dst->offset - 1) / UWORD_BIT;
  /* The specific bit within those UWORDs that is immediately in front of their respective cursors.
   * That bit is specifically '1 << (src_shift - 1)' for the read frame,
   * and '1 << (dst_shift - 1)' for the write frame, unless 'dst_shift == 0', in which case it is '1 << (UWORD_BIT - 1)'.
   */
  size_t src_shift = UWORD_BIT - (src->offset % UWORD_BIT);
  size_t dst_shift = dst->offset % UWORD_BIT;
  if (dst_shift) {
    /* The write frame's current UWORD is partially filled.
     * Fill the rest of it without overwriting the existing data.
     */
    *dst_ptr = LSBclear(*dst_ptr, dst_shift);

    if (src_shift < dst_shift) {
      /* The read frame's current UWORD doesn't have enough data to entirely fill the rest of the write frame's current UWORD.
       * Fill as much as we can and move src_ptr to the read frame's next UWORD.
       */
      *dst_ptr |= (UWORD)(LSBkeep(*src_ptr, src_shift) << (dst_shift - src_shift));
      if (n <= src_shift) return;
      n -= src_shift;
      dst_shift -= src_shift;
      src_ptr--;
      src_shift = UWORD_BIT;
    }

    /* Fill the rest of the write frame's current UWORD and move dst_ptr to the write frame's next UWORD. */
    *dst_ptr |= LSBkeep((UWORD)(*src_ptr >> (src_shift - dst_shift)), dst_shift);
    if (n <= dst_shift) return;
    n -= dst_shift;
    src_shift -= dst_shift;
    dst_ptr--;
  }
  /* The next cell in the write frame to be filled begins at the boundary of a UWORD. */

  /* :TODO: Use static analysis to limit the total amount of copied memory. */
  if (0 == src_shift % UWORD_BIT) {
    /* The next cell in the read frame to be filled also begins at the boundary of a UWORD.
     * We can use 'memcpy' to copy data in bulk.
     */
    size_t m = ROUND_UWORD(n);
    /* If we went through the previous 'if (dst_shift)' block then 'src_shift == 0' and we need to decrement src_ptr.
     * If we did not go through the previous 'if (dst_shift)' block then 'src_shift == UWORD_BIT'
     * and we do not need to decrement src_ptr.
     * We have folded this conditional decrement into the equation applied to 'src_ptr' below.
     */
    memcpy(dst_ptr - (m - 1), src_ptr - (m - src_shift / UWORD_BIT), m * sizeof(UWORD));
  } else {
    while(1) {
      /* Fill the write frame's UWORD by copying the LSBs of the read frame's current UWORD
       * to the MSBs of the write frame's current UWORD,
       * and copy the MSBs of the read frame's next UWORD to the LSBs of the write frame's current UWORD.
       * Then move both the src_ptr and dst_ptr to their next UWORDs.
       */
      *dst_ptr = (UWORD)(LSBkeep(*src_ptr, src_shift) << (UWORD_BIT - src_shift));
      if (n <= src_shift) return;

      *dst_ptr |= (UWORD)(*(src_ptr - 1) >> src_shift);
      if (n <= UWORD_BIT) return;
      n -= UWORD_BIT;
      dst_ptr--; src_ptr--;
    }
  }
}

/* Given a write frame and a read frame, copy 'n' cells from after the read frame's cursor to after the write frame's cursor,
 * and then advance the write frame's cursor by 'n'.
 * Cells in front of the '*dst's cursor's final position may be overwritten.
 *
 * Precondition: '*dst' is a valid write frame for 'n' more cells;
 *               '*src' is a valid read frame for 'n' more cells;
 */
void simplicity_copyBits(frameItem* dst, const frameItem* src, size_t n) {
  if (0 == n) return;
  copyBitsHelper(dst, src, n);
  dst->offset -= n;
}
