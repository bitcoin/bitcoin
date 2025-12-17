#include "jets.h"
#include "secp256k1/secp256k1.h"
#include "secp256k1/util.h"
#ifdef SECP256K1_WIDEMUL_INT128
# include "secp256k1/int128.h"
# include "secp256k1/int128_impl.h"
#else
# include "secp256k1/int128_struct.h"
# include "secp256k1/int128_struct_impl.h"
#endif

static void write128(frameItem* frame, const secp256k1_uint128* x) {
  simplicity_write64(frame, secp256k1_u128_hi_u64(x));
  simplicity_write64(frame, secp256k1_u128_to_u64(x));
}

/* verify : TWO |- ONE */
bool simplicity_verify(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; /* env is unused. */
  (void) dst; /* dst is unused. */
  return readBit(&src);
}

/* low_1 : ONE |- TWO */
bool simplicity_low_1(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; /* env is unused. */
  (void) src; /* src is unused. */
  writeBit(dst, 0);
  return true;
}

#define LOW_(bits)                                                            \
/* low_n : ONE |- TWO^n */                                                    \
bool simplicity_low_##bits(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                            \
  (void) src; /* src is unused. */                                            \
  simplicity_write##bits(dst, 0);                                             \
  return true;                                                                \
}
LOW_(8)
LOW_(16)
LOW_(32)
LOW_(64)

/* high_1 : ONE |- TWO */
bool simplicity_high_1(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; /* env is unused. */
  (void) src; /* src is unused. */
  writeBit(dst, 1);
  return true;
}

#define HIGH_(bits)                                                            \
/* high_n : ONE |- TWO^n */                                                    \
bool simplicity_high_##bits(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                             \
  (void) src; /* src is unused. */                                             \
  simplicity_write##bits(dst, UINT##bits##_MAX);                               \
  return true;                                                                 \
}
HIGH_(8)
HIGH_(16)
HIGH_(32)
HIGH_(64)

/* complement_1 : TWO |- TWO */
bool simplicity_complement_1(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; /* env is unused. */
  bool x = readBit(&src);
  writeBit(dst, !x);
  return true;
}

#define COMPLEMENT_(bits)                                                            \
/* complement_n : TWO^n |- TWO^n */                                                  \
bool simplicity_complement_##bits(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                                   \
  uint_fast##bits##_t x = simplicity_read##bits(&src);                               \
  simplicity_write##bits(dst, ~(1U*x));                                              \
  return true;                                                                       \
}
COMPLEMENT_(8)
COMPLEMENT_(16)
COMPLEMENT_(32)
COMPLEMENT_(64)

/* and_1 : TWO * TWO |- TWO */
bool simplicity_and_1(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; /* env is unused. */
  bool x = readBit(&src);
  bool y = readBit(&src);
  writeBit(dst, x && y);
  return true;
}

#define AND_(bits)                                                            \
/* and_n : TWO^n * TWO^n |- TWO^n */                                          \
bool simplicity_and_##bits(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                            \
  uint_fast##bits##_t x = simplicity_read##bits(&src);                        \
  uint_fast##bits##_t y = simplicity_read##bits(&src);                        \
  simplicity_write##bits(dst, x & y);                                         \
  return true;                                                                \
}
AND_(8)
AND_(16)
AND_(32)
AND_(64)

/* or_1 : TWO * TWO |- TWO */
bool simplicity_or_1(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; /* env is unused. */
  bool x = readBit(&src);
  bool y = readBit(&src);
  writeBit(dst, x || y);
  return true;
}

#define OR_(bits)                                                            \
/* or_n : TWO^n * TWO^n |- TWO^n */                                          \
bool simplicity_or_##bits(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                           \
  uint_fast##bits##_t x = simplicity_read##bits(&src);                       \
  uint_fast##bits##_t y = simplicity_read##bits(&src);                       \
  simplicity_write##bits(dst, x | y);                                        \
  return true;                                                               \
}
OR_(8)
OR_(16)
OR_(32)
OR_(64)

/* xor_1 : TWO * TWO |- TWO */
bool simplicity_xor_1(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; /* env is unused. */
  bool x = readBit(&src);
  bool y = readBit(&src);
  writeBit(dst, x ^ y);
  return true;
}

#define XOR_(bits)                                                            \
/* xor_n : TWO^n * TWO^n |- TWO^n */                                          \
bool simplicity_xor_##bits(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                            \
  uint_fast##bits##_t x = simplicity_read##bits(&src);                        \
  uint_fast##bits##_t y = simplicity_read##bits(&src);                        \
  simplicity_write##bits(dst, x ^ y);                                         \
  return true;                                                                \
}
XOR_(8)
XOR_(16)
XOR_(32)
XOR_(64)

/* maj_1 : TWO * TWO * TWO |- TWO */
bool simplicity_maj_1(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; /* env is unused. */
  bool x = readBit(&src);
  bool y = readBit(&src);
  bool z = readBit(&src);
  writeBit(dst, (x && y) || (y && z) || (z && x));
  return true;
}

#define MAJ_(bits)                                                            \
/* maj_n : TWO^n * TWO^n * TWO^n |- TWO^n */                                  \
bool simplicity_maj_##bits(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                            \
  uint_fast##bits##_t x = simplicity_read##bits(&src);                        \
  uint_fast##bits##_t y = simplicity_read##bits(&src);                        \
  uint_fast##bits##_t z = simplicity_read##bits(&src);                        \
  simplicity_write##bits(dst, (x&y) | (y&z) | (z&x));                         \
  return true;                                                                \
}
MAJ_(8)
MAJ_(16)
MAJ_(32)
MAJ_(64)

/* xor_xor_1 : TWO * TWO * TWO |- TWO */
bool simplicity_xor_xor_1(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; /* env is unused. */
  bool x = readBit(&src);
  bool y = readBit(&src);
  bool z = readBit(&src);
  writeBit(dst, x ^ y ^ z);
  return true;
}

#define XOR_XOR_(bits)                                                            \
/* xor_xor_n : TWO^n * TWO^n * TWO^n |- TWO^n */                                  \
bool simplicity_xor_xor_##bits(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                                \
  uint_fast##bits##_t x = simplicity_read##bits(&src);                            \
  uint_fast##bits##_t y = simplicity_read##bits(&src);                            \
  uint_fast##bits##_t z = simplicity_read##bits(&src);                            \
  simplicity_write##bits(dst, x ^ y ^ z);                                         \
  return true;                                                                    \
}
XOR_XOR_(8)
XOR_XOR_(16)
XOR_XOR_(32)
XOR_XOR_(64)

/* ch_1 : TWO * TWO * TWO |- TWO */
bool simplicity_ch_1(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; /* env is unused. */
  bool x = readBit(&src);
  bool y = readBit(&src);
  bool z = readBit(&src);
  writeBit(dst, x ? y : z);
  return true;
}

#define CH_(bits)                                                            \
/* ch_n : TWO^n * TWO^n * TWO^n |- TWO^n */                                  \
bool simplicity_ch_##bits(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                           \
  uint_fast##bits##_t x = simplicity_read##bits(&src);                       \
  uint_fast##bits##_t y = simplicity_read##bits(&src);                       \
  uint_fast##bits##_t z = simplicity_read##bits(&src);                       \
  simplicity_write##bits(dst, ((x&y) | ((~(1U*x))&z)));                      \
  return true;                                                               \
}
CH_(8)
CH_(16)
CH_(32)
CH_(64)

/* some_1 : TWO |- TWO */
bool simplicity_some_1(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; /* env is unused. */
  bool x = readBit(&src);
  writeBit(dst, x);
  return true;
}

#define SOME_(bits)                                                            \
/* some_n : TWO^n |- TWO */                                                    \
bool simplicity_some_##bits(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                             \
  uint_fast##bits##_t x = simplicity_read##bits(&src);                         \
  writeBit(dst, x != 0);                                                       \
  return true;                                                                 \
}
SOME_(8)
SOME_(16)
SOME_(32)
SOME_(64)

#define ALL_(bits)                                                            \
/* all_n : TWO^n |- TWO */                                                    \
bool simplicity_all_##bits(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                            \
  uint_fast##bits##_t x = simplicity_read##bits(&src);                        \
  writeBit(dst, x == UINT##bits##_MAX);                                       \
  return true;                                                                \
}
ALL_(8)
ALL_(16)
ALL_(32)
ALL_(64)

/* eq_1 : TWO * TWO |- TWO */
bool simplicity_eq_1(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; /* env is unused. */
  bool x = readBit(&src);
  bool y = readBit(&src);
  writeBit(dst, x == y);
  return true;
}

#define EQ_(bits)                                                            \
/* eq_n : TWO^n * TWO^n |- TWO */                                            \
bool simplicity_eq_##bits(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                           \
  uint_fast##bits##_t x = simplicity_read##bits(&src);                       \
  uint_fast##bits##_t y = simplicity_read##bits(&src);                       \
  writeBit(dst, x == y);                                                     \
  return true;                                                               \
}
EQ_(8)
EQ_(16)
EQ_(32)
EQ_(64)

/* eq_256 : TWO^256 * TWO^256 |- TWO */
bool simplicity_eq_256(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; /* env is unused. */
  uint32_t arr[16];
  read32s(arr, 16, &src);
  for (int i = 0; i < 8; ++i) {
    if (arr[i] != arr[i+8]) {
      writeBit(dst, false);
      return true;
    }
  }
  writeBit(dst, true);
  return true;
}

#define FULL_LEFT_SHIFT_(bitsN, bitsM)                                                                    \
/* full_left_shift_n_m : TWO^n * TWO^m |- TWO^m * TWO^n */                                                \
bool simplicity_full_left_shift_##bitsN##_##bitsM(frameItem* dst, frameItem src, const txEnv* env) {      \
  (void) env; /* env is unused. */                                                                        \
  static_assert(0 <= (bitsM) && (bitsM) <= (bitsN) && (bitsN) <= 64, "Bad arguments for bitsN or bitsM"); \
  simplicity_copyBits(dst, &src, (bitsN) + (bitsM));                                                      \
  return true;                                                                                            \
}
FULL_LEFT_SHIFT_(8,1)
FULL_LEFT_SHIFT_(8,2)
FULL_LEFT_SHIFT_(8,4)
FULL_LEFT_SHIFT_(16,1)
FULL_LEFT_SHIFT_(16,2)
FULL_LEFT_SHIFT_(16,4)
FULL_LEFT_SHIFT_(16,8)
FULL_LEFT_SHIFT_(32,1)
FULL_LEFT_SHIFT_(32,2)
FULL_LEFT_SHIFT_(32,4)
FULL_LEFT_SHIFT_(32,8)
FULL_LEFT_SHIFT_(32,16)
FULL_LEFT_SHIFT_(64,1)
FULL_LEFT_SHIFT_(64,2)
FULL_LEFT_SHIFT_(64,4)
FULL_LEFT_SHIFT_(64,8)
FULL_LEFT_SHIFT_(64,16)
FULL_LEFT_SHIFT_(64,32)

#define FULL_RIGHT_SHIFT_(bitsN, bitsM)                                                                   \
/* full_right_shift_n_m : TWO^m * TWO^n |- TWO^n * TWO^m */                                               \
bool simplicity_full_right_shift_##bitsN##_##bitsM(frameItem* dst, frameItem src, const txEnv* env) {     \
  (void) env; /* env is unused. */                                                                        \
  static_assert(0 <= (bitsM) && (bitsM) <= (bitsN) && (bitsN) <= 64, "Bad arguments for bitsN or bitsM"); \
  simplicity_copyBits(dst, &src, (bitsN) + (bitsM));                                                      \
  return true;                                                                                            \
}
FULL_RIGHT_SHIFT_(8,1)
FULL_RIGHT_SHIFT_(8,2)
FULL_RIGHT_SHIFT_(8,4)
FULL_RIGHT_SHIFT_(16,1)
FULL_RIGHT_SHIFT_(16,2)
FULL_RIGHT_SHIFT_(16,4)
FULL_RIGHT_SHIFT_(16,8)
FULL_RIGHT_SHIFT_(32,1)
FULL_RIGHT_SHIFT_(32,2)
FULL_RIGHT_SHIFT_(32,4)
FULL_RIGHT_SHIFT_(32,8)
FULL_RIGHT_SHIFT_(32,16)
FULL_RIGHT_SHIFT_(64,1)
FULL_RIGHT_SHIFT_(64,2)
FULL_RIGHT_SHIFT_(64,4)
FULL_RIGHT_SHIFT_(64,8)
FULL_RIGHT_SHIFT_(64,16)
FULL_RIGHT_SHIFT_(64,32)

#define LEFTMOST_(bitsN, bitsM)                                                                           \
/* leftmost_n_m : TWO^n |- TWO^m */                                                                       \
bool simplicity_leftmost_##bitsN##_##bitsM(frameItem* dst, frameItem src, const txEnv* env) {             \
  (void) env; /* env is unused. */                                                                        \
  static_assert(0 <= (bitsM) && (bitsM) <= (bitsN) && (bitsN) <= 64, "Bad arguments for bitsN or bitsM"); \
  simplicity_copyBits(dst, &src, (bitsM));                                                                \
  return true;                                                                                            \
}
LEFTMOST_(8,1)
LEFTMOST_(8,2)
LEFTMOST_(8,4)
LEFTMOST_(16,1)
LEFTMOST_(16,2)
LEFTMOST_(16,4)
LEFTMOST_(16,8)
LEFTMOST_(32,1)
LEFTMOST_(32,2)
LEFTMOST_(32,4)
LEFTMOST_(32,8)
LEFTMOST_(32,16)
LEFTMOST_(64,1)
LEFTMOST_(64,2)
LEFTMOST_(64,4)
LEFTMOST_(64,8)
LEFTMOST_(64,16)
LEFTMOST_(64,32)

#define RIGHTMOST_(bitsN, bitsM)                                                                          \
/* rightmost_n_m : TWO^n |- TWO^m */                                                                      \
bool simplicity_rightmost_##bitsN##_##bitsM(frameItem* dst, frameItem src, const txEnv* env) {            \
  (void) env; /* env is unused. */                                                                        \
  static_assert(0 <= (bitsM) && (bitsM) <= (bitsN) && (bitsN) <= 64, "Bad arguments for bitsN or bitsM"); \
  forwardBits(&src, (bitsN) - (bitsM));                                                                   \
  simplicity_copyBits(dst, &src, (bitsM));                                                                \
  return true;                                                                                            \
}
RIGHTMOST_(8,1)
RIGHTMOST_(8,2)
RIGHTMOST_(8,4)
RIGHTMOST_(16,1)
RIGHTMOST_(16,2)
RIGHTMOST_(16,4)
RIGHTMOST_(16,8)
RIGHTMOST_(32,1)
RIGHTMOST_(32,2)
RIGHTMOST_(32,4)
RIGHTMOST_(32,8)
RIGHTMOST_(32,16)
RIGHTMOST_(64,1)
RIGHTMOST_(64,2)
RIGHTMOST_(64,4)
RIGHTMOST_(64,8)
RIGHTMOST_(64,16)
RIGHTMOST_(64,32)

#define LEFT_PAD_LOW_1_(bitsM)                                                            \
/* left_pad_low_1_m : TWO |- TWO^m */                                                     \
bool simplicity_left_pad_low_1_##bitsM(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                                        \
  bool bit = readBit(&src);                                                               \
  simplicity_write##bitsM(dst, bit);                                                      \
  return true;                                                                            \
}
LEFT_PAD_LOW_1_(8)
LEFT_PAD_LOW_1_(16)
LEFT_PAD_LOW_1_(32)
LEFT_PAD_LOW_1_(64)

#define LEFT_PAD_LOW_(bitsN, bitsM)                                                               \
/* left_pad_low_n_m : TWO^n |- TWO^m */                                                           \
bool simplicity_left_pad_low_##bitsN##_##bitsM(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                                                \
  static_assert(0 < (bitsN) && (bitsN) <= 64, "bitsN is out of range");                           \
  static_assert(0 < (bitsM) && (bitsM) <= 64, "bitsM is out of range");                           \
  static_assert(0 == (bitsM) % (bitsN), "bitsM is not a multiple of bitsN");                      \
  for(int i = 0; i < (bitsM)/(bitsN) - 1; ++i) { simplicity_write##bitsN(dst, 0); }               \
  simplicity_copyBits(dst, &src, (bitsN));                                                        \
  return true;                                                                                    \
}
LEFT_PAD_LOW_(8,16)
LEFT_PAD_LOW_(8,32)
LEFT_PAD_LOW_(8,64)
LEFT_PAD_LOW_(16,32)
LEFT_PAD_LOW_(16,64)
LEFT_PAD_LOW_(32,64)

#define LEFT_PAD_HIGH_1_(bitsM)                                                            \
/* left_pad_high_1_m : TWO |- TWO^m */                                                     \
bool simplicity_left_pad_high_1_##bitsM(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                                         \
  static_assert(0 < (bitsM) && (bitsM) <= 64, "bitsM is out of range");                    \
  for(int i = 0; i < (bitsM) - 1; ++i) { writeBit(dst, true); }                            \
  simplicity_copyBits(dst, &src, 1);                                                       \
  return true;                                                                             \
}
LEFT_PAD_HIGH_1_(8)
LEFT_PAD_HIGH_1_(16)
LEFT_PAD_HIGH_1_(32)
LEFT_PAD_HIGH_1_(64)

#define LEFT_PAD_HIGH_(bitsN, bitsM)                                                                \
/* left_pad_high_n_m : TWO^n |- TWO^m */                                                            \
bool simplicity_left_pad_high_##bitsN##_##bitsM(frameItem* dst, frameItem src, const txEnv* env) {  \
  (void) env; /* env is unused. */                                                                  \
  static_assert(0 < (bitsN) && (bitsN) <= 64, "bitsN is out of range");                             \
  static_assert(0 < (bitsM) && (bitsM) <= 64, "bitsM is out of range");                             \
  static_assert(0 == (bitsM) % (bitsN), "bitsM is not a multiple of bitsN");                        \
  for(int i = 0; i < (bitsM)/(bitsN) - 1; ++i) { simplicity_write##bitsN(dst, UINT##bitsN##_MAX); } \
  simplicity_copyBits(dst, &src, (bitsN));                                                          \
  return true;                                                                                      \
}
LEFT_PAD_HIGH_(8,16)
LEFT_PAD_HIGH_(8,32)
LEFT_PAD_HIGH_(8,64)
LEFT_PAD_HIGH_(16,32)
LEFT_PAD_HIGH_(16,64)
LEFT_PAD_HIGH_(32,64)

#define LEFT_EXTEND_1_(bitsM)                                                            \
/* left_extend_1_m : TWO |- TWO^m */                                                     \
bool simplicity_left_extend_1_##bitsM(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                                       \
  bool bit = readBit(&src);                                                              \
  simplicity_write##bitsM(dst, bit ? UINT##bitsM##_MAX : 0);                             \
  return true;                                                                           \
}
LEFT_EXTEND_1_(8)
LEFT_EXTEND_1_(16)
LEFT_EXTEND_1_(32)
LEFT_EXTEND_1_(64)

#define LEFT_EXTEND_(bitsN, bitsM)                                                                            \
/* left_extend_n_m : TWO^n |- TWO^m */                                                                        \
bool simplicity_left_extend_##bitsN##_##bitsM(frameItem* dst, frameItem src, const txEnv* env) {              \
  (void) env; /* env is unused. */                                                                            \
  static_assert(0 < (bitsN) && (bitsN) <= 64, "bitsN is out of range");                                       \
  static_assert(0 < (bitsM) && (bitsM) <= 64, "bitsM is out of range");                                       \
  static_assert(0 == (bitsM) % (bitsN), "bitsM is not a multiple of bitsN");                                  \
  uint_fast##bitsN##_t input = simplicity_read##bitsN(&src);                                                  \
  bool msb = input >> ((bitsN) - 1);                                                                          \
  for(int i = 0; i < (bitsM)/(bitsN) - 1; ++i) { simplicity_write##bitsN(dst, msb ? UINT##bitsN##_MAX : 0); } \
  simplicity_write##bitsN(dst, input);                                                                        \
  return true;                                                                                                \
}
LEFT_EXTEND_(8,16)
LEFT_EXTEND_(8,32)
LEFT_EXTEND_(8,64)
LEFT_EXTEND_(16,32)
LEFT_EXTEND_(16,64)
LEFT_EXTEND_(32,64)

#define RIGHT_PAD_LOW_1_(bitsM)                                                                     \
/* right_pad_low_1_m : TWO |- TWO^m */                                                              \
bool simplicity_right_pad_low_1_##bitsM(frameItem* dst, frameItem src, const txEnv* env) {          \
  (void) env; /* env is unused. */                                                                  \
  static_assert(0 < (bitsM) && (bitsM) <= 64, "bitsM is out of range");                             \
  bool bit = readBit(&src);                                                                         \
  simplicity_write##bitsM(dst, (uint_fast##bitsM##_t)((uint_fast##bitsM##_t)bit << ((bitsM) - 1))); \
  return true;                                                                                      \
}
RIGHT_PAD_LOW_1_(8)
RIGHT_PAD_LOW_1_(16)
RIGHT_PAD_LOW_1_(32)
RIGHT_PAD_LOW_1_(64)

#define RIGHT_PAD_LOW_(bitsN, bitsM)                                                               \
/* right_pad_low_n_m : TWO^n |- TWO^m */                                                           \
bool simplicity_right_pad_low_##bitsN##_##bitsM(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                                                 \
  static_assert(0 < (bitsN) && (bitsN) <= 64, "bitsN is out of range");                            \
  static_assert(0 < (bitsM) && (bitsM) <= 64, "bitsM is out of range");                            \
  static_assert(0 == (bitsM) % (bitsN), "bitsM is not a multiple of bitsN");                       \
  simplicity_copyBits(dst, &src, (bitsN));                                                         \
  for(int i = 0; i < (bitsM)/(bitsN) - 1; ++i) { simplicity_write##bitsN(dst, 0); }                \
  return true;                                                                                     \
}
RIGHT_PAD_LOW_(8,16)
RIGHT_PAD_LOW_(8,32)
RIGHT_PAD_LOW_(8,64)
RIGHT_PAD_LOW_(16,32)
RIGHT_PAD_LOW_(16,64)
RIGHT_PAD_LOW_(32,64)

#define RIGHT_PAD_HIGH_1_(bitsM)                                                            \
/* right_pad_high_1_m : TWO |- TWO^m */                                                     \
bool simplicity_right_pad_high_1_##bitsM(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                                          \
  static_assert(0 < (bitsM) && (bitsM) <= 64, "bitsM is out of range");                     \
  simplicity_copyBits(dst, &src, 1);                                                        \
  for(int i = 0; i < (bitsM) - 1; ++i) { writeBit(dst, true); }                             \
  return true;                                                                              \
}
RIGHT_PAD_HIGH_1_(8)
RIGHT_PAD_HIGH_1_(16)
RIGHT_PAD_HIGH_1_(32)
RIGHT_PAD_HIGH_1_(64)

#define RIGHT_PAD_HIGH_(bitsN, bitsM)                                                               \
/* right_pad_high_n_m : TWO^n |- TWO^m */                                                           \
bool simplicity_right_pad_high_##bitsN##_##bitsM(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                                                  \
  static_assert(0 < (bitsN) && (bitsN) <= 64, "bitsN is out of range");                             \
  static_assert(0 < (bitsM) && (bitsM) <= 64, "bitsM is out of range");                             \
  static_assert(0 == (bitsM) % (bitsN), "bitsM is not a multiple of bitsN");                        \
  simplicity_copyBits(dst, &src, (bitsN));                                                          \
  for(int i = 0; i < (bitsM)/(bitsN) - 1; ++i) { simplicity_write##bitsN(dst, UINT##bitsN##_MAX); } \
  return true;                                                                                      \
}
RIGHT_PAD_HIGH_(8,16)
RIGHT_PAD_HIGH_(8,32)
RIGHT_PAD_HIGH_(8,64)
RIGHT_PAD_HIGH_(16,32)
RIGHT_PAD_HIGH_(16,64)
RIGHT_PAD_HIGH_(32,64)

#define RIGHT_EXTEND_(bitsN, bitsM)                                                                           \
/* right_extend_n_m : TWO^n |- TWO^m */                                                                       \
bool simplicity_right_extend_##bitsN##_##bitsM(frameItem* dst, frameItem src, const txEnv* env) {             \
  (void) env; /* env is unused. */                                                                            \
  static_assert(0 < (bitsN) && (bitsN) <= 64, "bitsN is out of range");                                       \
  static_assert(0 < (bitsM) && (bitsM) <= 64, "bitsM is out of range");                                       \
  static_assert(0 == (bitsM) % (bitsN), "bitsM is not a multiple of bitsN");                                  \
  uint_fast##bitsN##_t input = simplicity_read##bitsN(&src);                                                  \
  bool lsb = input & 1;                                                                                       \
  simplicity_write##bitsN(dst, input);                                                                        \
  for(int i = 0; i < (bitsM)/(bitsN) - 1; ++i) { simplicity_write##bitsN(dst, lsb ? UINT##bitsN##_MAX : 0); } \
  return true;                                                                                                \
}
RIGHT_EXTEND_(8,16)
RIGHT_EXTEND_(8,32)
RIGHT_EXTEND_(8,64)
RIGHT_EXTEND_(16,32)
RIGHT_EXTEND_(16,64)
RIGHT_EXTEND_(32,64)

#define LEFT_SHIFT_(log, bits)                                                            \
static inline void left_shift_helper_##bits(bool with, frameItem* dst, frameItem *src) {  \
  static_assert(log <= 8, "Only log parameter up to 8 is supported.");                    \
  uint_fast8_t amt = simplicity_read##log(src);                                           \
  uint_fast##bits##_t output = simplicity_read##bits(src);                                \
  if (with) output = UINT##bits##_MAX ^ output;                                           \
  if (amt < bits) {                                                                       \
    output = (uint_fast##bits##_t)((1U * output) << amt);                                 \
  } else {                                                                                \
    output = 0;                                                                           \
  }                                                                                       \
  if (with) output = UINT##bits##_MAX ^ output;                                           \
  simplicity_write##bits(dst, output);                                                    \
}                                                                                         \
                                                                                          \
/* left_shift_with_n : TWO * TWO^l * TWO^n |- TWO^n */                                    \
bool simplicity_left_shift_with_##bits(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                                        \
  bool with = readBit(&src);                                                              \
  left_shift_helper_##bits(with, dst, &src);                                              \
  return true;                                                                            \
}                                                                                         \
                                                                                          \
/* left_shift_n : TWO^l * TWO^n |- TWO^n */                                               \
bool simplicity_left_shift_##bits(frameItem* dst, frameItem src, const txEnv* env) {      \
  (void) env; /* env is unused. */                                                        \
  left_shift_helper_##bits(0, dst, &src);                                                 \
  return true;                                                                            \
}
LEFT_SHIFT_(4,8)
LEFT_SHIFT_(4,16)
LEFT_SHIFT_(8,32)
LEFT_SHIFT_(8,64)

#define RIGHT_SHIFT_(log, bits)                                                            \
static inline void right_shift_helper_##bits(bool with, frameItem* dst, frameItem *src) {  \
  static_assert(log <= 8, "Only log parameter up to 8 is supported.");                     \
  uint_fast8_t amt = simplicity_read##log(src);                                            \
  uint_fast##bits##_t output = simplicity_read##bits(src);                                 \
  if (with) output = UINT##bits##_MAX ^ output;                                            \
  if (amt < bits) {                                                                        \
    output = (uint_fast##bits##_t)(output >> amt);                                         \
  } else {                                                                                 \
    output = 0;                                                                            \
  }                                                                                        \
  if (with) output = UINT##bits##_MAX ^ output;                                            \
  simplicity_write##bits(dst, output);                                                     \
}                                                                                          \
                                                                                           \
/* right_shift_with_n : TWO * TWO^l * TWO^n |- TWO^n */                                    \
bool simplicity_right_shift_with_##bits(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                                         \
  bool with = readBit(&src);                                                               \
  right_shift_helper_##bits(with, dst, &src);                                              \
  return true;                                                                             \
}                                                                                          \
                                                                                           \
/* right_shift_n : TWO^l * TWO^n |- TWO^n */                                               \
bool simplicity_right_shift_##bits(frameItem* dst, frameItem src, const txEnv* env) {      \
  (void) env; /* env is unused. */                                                         \
  right_shift_helper_##bits(0, dst, &src);                                                 \
  return true;                                                                             \
}
RIGHT_SHIFT_(4,8)
RIGHT_SHIFT_(4,16)
RIGHT_SHIFT_(8,32)
RIGHT_SHIFT_(8,64)

#define ROTATE_(log, bits)                                                                     \
/* Precondition: 0 <= amt < bits */                                                            \
static inline uint_fast##bits##_t rotate_##bits(uint_fast##bits##_t value, uint_fast8_t amt) { \
  if (amt) {                                                                                   \
    return (uint_fast##bits##_t)((1U * value << amt) | value >> (bits - amt));                 \
  } else {                                                                                     \
    return value;                                                                              \
  }                                                                                            \
}                                                                                              \
                                                                                               \
/* left_rotate_n : TWO^l * TWO^n |- TWO^n */                                                   \
bool simplicity_left_rotate_##bits(frameItem* dst, frameItem src, const txEnv* env) {          \
  (void) env; /* env is unused. */                                                             \
  uint_fast8_t amt = simplicity_read##log(&src) % bits;                                        \
  uint_fast##bits##_t input = simplicity_read##bits(&src);                                     \
  simplicity_write##bits(dst, rotate_##bits(input, amt));                                      \
  return true;                                                                                 \
}                                                                                              \
                                                                                               \
/* right_rotate_n : TWO^l * TWO^n |- TWO^n */                                                  \
bool simplicity_right_rotate_##bits(frameItem* dst, frameItem src, const txEnv* env) {         \
  static_assert(bits <= UINT8_MAX, "'bits' is too large.");                                    \
  (void) env; /* env is unused. */                                                             \
  uint_fast8_t amt = simplicity_read##log(&src) % bits;                                        \
  uint_fast##bits##_t input = simplicity_read##bits(&src);                                     \
  simplicity_write##bits(dst, rotate_##bits(input, (uint_fast8_t)((bits - amt) % bits)));      \
  return true;                                                                                 \
}
ROTATE_(4,8)
ROTATE_(4,16)
ROTATE_(8,32)
ROTATE_(8,64)

#define ONE_(bits)                                                            \
/* one_n : ONE |- TWO^n */                                                    \
bool simplicity_one_##bits(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                            \
  (void) src; /* src is unused. */                                            \
  simplicity_write##bits(dst, 1);                                             \
  return true;                                                                \
}
ONE_(8)
ONE_(16)
ONE_(32)
ONE_(64)

#define ADD_(bits)                                                            \
/* add_n : TWO^n * TWO^n |- TWO * TWO^n */                                    \
bool simplicity_add_##bits(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                            \
  uint_fast##bits##_t x = simplicity_read##bits(&src);                        \
  uint_fast##bits##_t y = simplicity_read##bits(&src);                        \
  writeBit(dst, 1U * UINT##bits##_MAX - y < x);                               \
  simplicity_write##bits(dst, (uint_fast##bits##_t)(1U * x + y));             \
  return true;                                                                \
}
ADD_(8)
ADD_(16)
ADD_(32)
ADD_(64)

#define FULL_ADD_(bits)                                                                   \
/* full_add_n : TWO * TWO^n * TWO^n |- TWO * TWO^n */                                     \
bool simplicity_full_add_##bits(frameItem* dst, frameItem src, const txEnv* env) {        \
  (void) env; /* env is unused. */                                                        \
  bool z = readBit(&src);                                                                 \
  uint_fast##bits##_t x = simplicity_read##bits(&src);                                    \
  uint_fast##bits##_t y = simplicity_read##bits(&src);                                    \
  writeBit(dst, 1U * UINT##bits##_MAX - y < x || 1U * UINT##bits##_MAX - z < 1U * x + y); \
  simplicity_write##bits(dst, (uint_fast##bits##_t)(1U * x + y + z));                     \
  return true;                                                                            \
}
FULL_ADD_(8)
FULL_ADD_(16)
FULL_ADD_(32)
FULL_ADD_(64)

#define FULL_INCREMENT_(bits)                                                            \
/* full_increment_n : TWO * TWO^n |- TWO * TWO^n */                                      \
bool simplicity_full_increment_##bits(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                                       \
  bool z = readBit(&src);                                                                \
  uint_fast##bits##_t x = simplicity_read##bits(&src);                                   \
  writeBit(dst, 1U * UINT##bits##_MAX - z < x);                                          \
  simplicity_write##bits(dst, (uint_fast##bits##_t)(1U * x + z));                        \
  return true;                                                                           \
}
FULL_INCREMENT_(8)
FULL_INCREMENT_(16)
FULL_INCREMENT_(32)
FULL_INCREMENT_(64)

#define INCREMENT_(bits)                                                            \
/* increment_n : TWO^n |- TWO * TWO^n */                                            \
bool simplicity_increment_##bits(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                                  \
  uint_fast##bits##_t x = simplicity_read##bits(&src);                              \
  writeBit(dst, 1U * UINT##bits##_MAX - 1 < x);                                     \
  simplicity_write##bits(dst, (uint_fast##bits##_t)(1U * x + 1));                   \
  return true;                                                                      \
}
INCREMENT_(8)
INCREMENT_(16)
INCREMENT_(32)
INCREMENT_(64)

#define SUBTRACT_(bits)                                                            \
/* subtract_n : TWO^n * TWO^n |- TWO * TWO^n */                                    \
bool simplicity_subtract_##bits(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                                 \
  uint_fast##bits##_t x = simplicity_read##bits(&src);                             \
  uint_fast##bits##_t y = simplicity_read##bits(&src);                             \
  writeBit(dst, x < y);                                                            \
  simplicity_write##bits(dst, (uint_fast##bits##_t)(1U * x - y));                  \
  return true;                                                                     \
}
SUBTRACT_(8)
SUBTRACT_(16)
SUBTRACT_(32)
SUBTRACT_(64)

#define NEGATE_(bits)                                                            \
/* negate_n : TWO^n |- TWO * TWO^n */                                            \
bool simplicity_negate_##bits(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                               \
  uint_fast##bits##_t x = simplicity_read##bits(&src);                           \
  writeBit(dst, x != 0);                                                         \
  simplicity_write##bits(dst, (uint_fast##bits##_t)(- (1U * x)));                \
  return true;                                                                   \
}
NEGATE_(8)
NEGATE_(16)
NEGATE_(32)
NEGATE_(64)

#define FULL_DECREMENT_(bits)                                                            \
/* full_decrement_n : TWO * TWO^n |- TWO * TWO^n */                                      \
bool simplicity_full_decrement_##bits(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                                       \
  bool z = readBit(&src);                                                                \
  uint_fast##bits##_t x = simplicity_read##bits(&src);                                   \
  writeBit(dst, 1U * x < 1U * z);                                                        \
  simplicity_write##bits(dst, (uint_fast##bits##_t)(1U * x - z));                        \
  return true;                                                                           \
}
FULL_DECREMENT_(8)
FULL_DECREMENT_(16)
FULL_DECREMENT_(32)
FULL_DECREMENT_(64)

#define DECREMENT_(bits)                                                            \
/* decrement_n : TWO^n |- TWO * TWO^n */                                            \
bool simplicity_decrement_##bits(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                                  \
  uint_fast##bits##_t x = simplicity_read##bits(&src);                              \
  writeBit(dst, x < 1);                                                             \
  simplicity_write##bits(dst, (uint_fast##bits##_t)(1U * x - 1));                   \
  return true;                                                                      \
}
DECREMENT_(8)
DECREMENT_(16)
DECREMENT_(32)
DECREMENT_(64)

#define FULL_SUBTRACT_(bits)                                                            \
/* full_subtract_n : TWO * TWO^n * TWO^n |- TWO * TWO^n */                              \
bool simplicity_full_subtract_##bits(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                                      \
  bool z = readBit(&src);                                                               \
  uint_fast##bits##_t x = simplicity_read##bits(&src);                                  \
  uint_fast##bits##_t y = simplicity_read##bits(&src);                                  \
  writeBit(dst, 1U * x < 1U * y || 1U * x - y < 1U * z);                                \
  simplicity_write##bits(dst, (uint_fast##bits##_t)(1U * x - y - z));                   \
  return true;                                                                          \
}
FULL_SUBTRACT_(8)
FULL_SUBTRACT_(16)
FULL_SUBTRACT_(32)
FULL_SUBTRACT_(64)

#define MULTIPLY_(bits,bitsx2)                                                     \
bool simplicity_multiply_##bits(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                                 \
  uint_fast##bitsx2##_t x = simplicity_read##bits(&src);                           \
  uint_fast##bitsx2##_t y = simplicity_read##bits(&src);                           \
  simplicity_write##bitsx2(dst, x * y);                                            \
  return true;                                                                     \
}
MULTIPLY_(8, 16)
MULTIPLY_(16, 32)
MULTIPLY_(32, 64)

bool simplicity_multiply_64(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; /* env is unused. */
  uint_fast64_t x = simplicity_read64(&src);
  uint_fast64_t y = simplicity_read64(&src);
  secp256k1_uint128 r;
  secp256k1_u128_mul(&r, x, y);
  write128(dst, &r);
  return true;
}

#define FULL_MULTIPLY_(bits,bitsx2)                                                     \
bool simplicity_full_multiply_##bits(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                                      \
  uint_fast##bitsx2##_t x = simplicity_read##bits(&src);                                \
  uint_fast##bitsx2##_t y = simplicity_read##bits(&src);                                \
  uint_fast##bitsx2##_t z = simplicity_read##bits(&src);                                \
  uint_fast##bitsx2##_t w = simplicity_read##bits(&src);                                \
  simplicity_write##bitsx2(dst, x * y + z + w);                                         \
  return true;                                                                          \
}
FULL_MULTIPLY_(8, 16)
FULL_MULTIPLY_(16, 32)
FULL_MULTIPLY_(32, 64)

bool simplicity_full_multiply_64(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; /* env is unused. */
  uint_fast64_t x = simplicity_read64(&src);
  uint_fast64_t y = simplicity_read64(&src);
  uint_fast64_t z = simplicity_read64(&src);
  uint_fast64_t w = simplicity_read64(&src);
  secp256k1_uint128 r;
  secp256k1_u128_mul(&r, x, y);
  secp256k1_u128_accum_u64(&r, z);
  secp256k1_u128_accum_u64(&r, w);
  write128(dst, &r);
  return true;
}

#define IS_ZERO_(bits)                                                            \
/* is_zero_n : TWO^n |- TWO */                                                    \
bool simplicity_is_zero_##bits(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                                \
  uint_fast##bits##_t x = simplicity_read##bits(&src);                            \
  writeBit(dst, x == 0);                                                          \
  return true;                                                                    \
}
IS_ZERO_(8)
IS_ZERO_(16)
IS_ZERO_(32)
IS_ZERO_(64)

#define IS_ONE_(bits)                                                            \
/* is_one_n : TWO^n |- TWO */                                                    \
bool simplicity_is_one_##bits(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                               \
  uint_fast##bits##_t x = simplicity_read##bits(&src);                           \
  writeBit(dst, x == 1);                                                         \
  return true;                                                                   \
}
IS_ONE_(8)
IS_ONE_(16)
IS_ONE_(32)
IS_ONE_(64)

#define LE_(bits)                                                            \
/* le_n : TWO^n * TWO^n |- TWO */                                            \
bool simplicity_le_##bits(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                           \
  uint_fast##bits##_t x = simplicity_read##bits(&src);                       \
  uint_fast##bits##_t y = simplicity_read##bits(&src);                       \
  writeBit(dst, x <= y);                                                     \
  return true;                                                               \
}
LE_(8)
LE_(16)
LE_(32)
LE_(64)

#define LT_(bits)                                                            \
/* lt_n : TWO^n * TWO^n |- TWO */                                            \
bool simplicity_lt_##bits(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                           \
  uint_fast##bits##_t x = simplicity_read##bits(&src);                       \
  uint_fast##bits##_t y = simplicity_read##bits(&src);                       \
  writeBit(dst, x < y);                                                      \
  return true;                                                               \
}
LT_(8)
LT_(16)
LT_(32)
LT_(64)

#define MIN_(bits)                                                            \
/* min_n : TWO^n * TWO^n |- TWO^n */                                          \
bool simplicity_min_##bits(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                            \
  uint_fast##bits##_t x = simplicity_read##bits(&src);                        \
  uint_fast##bits##_t y = simplicity_read##bits(&src);                        \
  simplicity_write##bits(dst, x < y ? x : y);                                 \
  return true;                                                                \
}
MIN_(8)
MIN_(16)
MIN_(32)
MIN_(64)

#define MAX_(bits)                                                            \
/* max_n : TWO^n * TWO^n |- TWO^n */                                          \
bool simplicity_max_##bits(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                            \
  uint_fast##bits##_t x = simplicity_read##bits(&src);                        \
  uint_fast##bits##_t y = simplicity_read##bits(&src);                        \
  simplicity_write##bits(dst, x < y ? y : x);                                 \
  return true;                                                                \
}
MAX_(8)
MAX_(16)
MAX_(32)
MAX_(64)

#define MEDIAN_(bits)                                                            \
/* median_n : TWO^n * TWO^n * TWO^n |- TWO^n */                                  \
bool simplicity_median_##bits(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                               \
  uint_fast##bits##_t x = simplicity_read##bits(&src);                           \
  uint_fast##bits##_t y = simplicity_read##bits(&src);                           \
  uint_fast##bits##_t z = simplicity_read##bits(&src);                           \
  simplicity_write##bits(dst, x < y                                              \
                 ? (y < z ? y : (z < x ? x : z))                                 \
                 : (x < z ? x : (z < y ? y : z)));                               \
  return true;                                                                   \
}
MEDIAN_(8)
MEDIAN_(16)
MEDIAN_(32)
MEDIAN_(64)

#define DIV_MOD_(bits)                                                            \
/* div_mod_n : TWO^n * TWO^n |- TWO^n * TWO^n */                                  \
bool simplicity_div_mod_##bits(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                                \
  uint_fast##bits##_t x = simplicity_read##bits(&src);                            \
  uint_fast##bits##_t y = simplicity_read##bits(&src);                            \
  simplicity_write##bits(dst, 0 == y ? 0 : x / y);                                \
  simplicity_write##bits(dst, 0 == y ? x : x % y);                                \
  return true;                                                                    \
}
DIV_MOD_(8)
DIV_MOD_(16)
DIV_MOD_(32)
DIV_MOD_(64)

#define DIVIDE_(bits)                                                            \
/* divide_n : TWO^n * TWO^n |- TWO^n */                                          \
bool simplicity_divide_##bits(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                               \
  uint_fast##bits##_t x = simplicity_read##bits(&src);                           \
  uint_fast##bits##_t y = simplicity_read##bits(&src);                           \
  simplicity_write##bits(dst, 0 == y ? 0 : x / y);                               \
  return true;                                                                   \
}
DIVIDE_(8)
DIVIDE_(16)
DIVIDE_(32)
DIVIDE_(64)

#define MODULO_(bits)                                                            \
/* modulo_n : TWO^n * TWO^n |- TWO^n */                                          \
bool simplicity_modulo_##bits(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                               \
  uint_fast##bits##_t x = simplicity_read##bits(&src);                           \
  uint_fast##bits##_t y = simplicity_read##bits(&src);                           \
  simplicity_write##bits(dst, 0 == y ? x : x % y);                               \
  return true;                                                                   \
}
MODULO_(8)
MODULO_(16)
MODULO_(32)
MODULO_(64)

#define DIVIDES_(bits)                                                            \
/* divides_n : TWO^n * TWO^n |- TWO */                                            \
bool simplicity_divides_##bits(frameItem* dst, frameItem src, const txEnv* env) { \
  (void) env; /* env is unused. */                                                \
  uint_fast##bits##_t x = simplicity_read##bits(&src);                            \
  uint_fast##bits##_t y = simplicity_read##bits(&src);                            \
  writeBit(dst, 0 == (0 == x ? y : y % x));                                       \
  return true;                                                                    \
}
DIVIDES_(8)
DIVIDES_(16)
DIVIDES_(32)
DIVIDES_(64)

/* Implements the 3n/2n division algorithm for n=32 bits.
 * For more details see "Fast Recursive Division" by Christoph Burnikel and Joachim Ziegler, MPI-I-98-1-022, Oct. 1998.
 *
 * Given a 96 bit (unsigned) value A and a 64 bit value B, set *q and *r to the quotient and remainder of A divided by B.
 *
 * ah is passed the high 64 bits of A, and al is passed the low 32 bits of A.
 * We say that A = [ah;al] where [ah;al] denotes ah * 2^32 + al.
 *
 * The value of B is passed in as b.
 *
 * This algorithm requires certain preconditons:
 * 1. A < [B;0]
 * 2. 2^63 <= B.
 *
 * Preconditon 1 ensures that the quotient fits in 32-bits.
 *
 * We define the quotient Q and remainder R by the equation:
 *   A == Q * B + R where 0 <= R < B.
 *
 * The algorithm works by estimating the value of Q by
 *   estQ = (A >> 32) / (B >> 32).
 *
 * Preconditon 2 ensures that this estimate is close to the true value of Q.  In fact Q <= estQ <= Q + 2 (see proof below)
 *
 * There is a corresponding estR value satisfying the equation estR = A - estQ * B.
 * This estR is one of {R, R - B, R - 2B}.
 * Therefore if estR is non-negative, then estR is equal to the true R value, and hence estQ is equal to the true Q value.
 *
 * If estR is negative we decrement our estimated value of Q and repeatedly try again until the computed remainder is non-negative.
 *
 * Lemma 1: Q <= estQ.
 * A - estQ * B == [ah;al] - estQ * [bh;bl]
 *              == [ah - estQ * bh; al - estQ * bl]
 *              == [ah % bh; al] - estQ * bl
 *              <= [ah % bh; al]
 *              <  [(ah % bh) + 1; 0]
 *              <= [bh; 0]
 *              <= B
 * Therefore A - (estQ + 1)*B < 0, and hence Q == A/B < estQ + 1.
 *
 * Lemma 2: estQ < [1;2] (== 2^32 + 2).
 * First note that ah - [bh;0] < [1;0] because
 * ah < B (by precondition 1)
 *    < [bh+1;0]
 *    == [bh;0] + [1;0]
 *
 * ah - [1;2]*bh == ah - [bh;2*bh]
 *               < [1;0] - [0;2*bh]
 *               <= 0 (by precondition 2.)
 * Therefore estQ == ah / bh < [1;2].
 *
 * Lemma 3: estQ - 2 <= Q.
 * A - (estQ - 2)*B == 2*B + estR
 *                  == 2*B + [ah % bh; al] - estQ * bl
 *                  >= 2*B - estQ * bl
 *                  == 2*[bh;bl] - estQ * bl
 *                  == [2*bh;0] + 2*bl - estQ * bl
 *                  == [2*bh;0] - (estQ - 2) * bl
 *                  >  [2*bh;0] - [1;0] * bl (by lemma 2.)
 *                  == [2*bh - bl;0]
 *                  >= 0 (by precondition 2.)
 * Therefore Q = A/B >= estQ - 2.
 */
static void div_mod_96_64(uint_fast32_t *q, uint_fast64_t *r,
                          uint_fast64_t ah, uint_fast32_t al,
                          uint_fast64_t b) {
  simplicity_debug_assert(ah < b);
  simplicity_debug_assert(0x8000000000000000u <= b);
  uint_fast64_t bh = b >> 32;
  uint_fast64_t bl = b & 0xffffffffu;
  /* B == b == [bh;bl] */
  uint_fast64_t estQ = ah / bh;

  /* Precondition 1 guarantees Q is 32-bits, if estQ is greater than UINT32_MAX, then reduce our initial estimated quotient to UINT32_MAX. */
  *q = estQ <= UINT32_MAX ? (uint_fast32_t)estQ : UINT32_MAX;

  /* *q * bh <= estQ * bh <= ah */
  uint_fast64_t rh = ah - 1u * *q * bh;
  uint_fast64_t d = 1u * *q * bl;

  /* Test if our estimated remainder, A - *q * B is negative.
   * A - *q * B  == [ah;al] - *q * [bh;bl]
   *             == [ah - *q * bh;al - *q * bl]
   *             == [rh;al] - d
   *
   * This value is negative when [rh;al] < d.
   * Note that d is 64 bit and thus if rh is greater than UINT32_MAX, then this value cannot be negative.
   */
  /* This loop is executed at most twice. */
  while (rh <= UINT32_MAX && 0x100000000u*rh + al < d) {
    /* Our estimated remainder, A - *q * B is negative. */
    /* 0 < d == *q * bl and hence 0 < *q, so this decrement does not underflow. */
    (*q)--;
    rh += bh;
    /* rh == ah - *q * bh */
    d -= bl;
    /* d == *q * bl */;
  }
  /* *q is the correct quotient. */

  /* Compute the remainder.
   * The computation below is performed modulo 2^64.
   * However since we know the remainder lies within [0,b) which is within [0,2^64),
   * the computation must end up with the correct result even if overflow/underflow occurs.
   */
  *r = 0x100000000u*rh + al - d;
}

/* div_mod_128_64 : TWO^128 * TWO^64 |- TWO^64 * TWO^64 */
bool simplicity_div_mod_128_64(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; /* env is unused. */
  uint_fast32_t qh, ql;
  uint_fast64_t r;
  uint_fast64_t ah = simplicity_read64(&src);
  uint_fast32_t am = simplicity_read32(&src);
  uint_fast32_t al = simplicity_read32(&src);
  uint_fast64_t b = simplicity_read64(&src);

  /* div2n1n is only defined when 2^(n-1) <= b and when the quotient q < 2^n. */
  if (0x8000000000000000 <= b && ah < b) {
    /* We compute the division and remainder of A = [[ah;am];al] by B = b by long division with 32-bit "digits"
     *
     * 1.       Q
     *      ______
     *    BB) AAAA
     *        XXX
     *        ---
     *         RR
     *
     * First divide the high 3 "digit"s (96-bits) of A by the two "digit"s (64-bits) of B,
     * returning the first "digit" (high 32-bits) of the quotient, and an intermediate remainder consisiting of 2 "digit"s (64-bits).
     */
    div_mod_96_64(&qh, &r, ah, am, b);
    simplicity_debug_assert(r < b);
    /* 2.       QQ
     *      ______
     *    BB) AAA|
     *        XXX|
     *        ---|
     *         RRA
     *         XXX
     *         ---
     *          RR
     *
     * Then append the last "digit" of A to the intermediate remainder and divide that value (96_bits) by the two "digit"s (64-bits) of B,
     * returning the second "digit" (low 32-bits) of the quotient, and the final remainder consisiting of 2 "digit"s (64-bits).
     */
    div_mod_96_64(&ql, &r, r, al, b);
    simplicity_write32(dst, qh);
    simplicity_write32(dst, ql);
    simplicity_write64(dst, r);
  } else {
    /* Set all the bits in the output when the input is out of bounds. */
    simplicity_write64(dst, (uint64_t)(-1));
    simplicity_write64(dst, (uint64_t)(-1));
  }
  return true;
}

/* sha_256_iv : ONE |- TWO^256 */
bool simplicity_sha_256_iv(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; /* env is unused. */
  (void) src; /* env is unused. */

  uint32_t iv[8];
  sha256_iv(iv);

  write32s(dst, iv, 8);
  return true;
}

/* sha_256_block : TWO^256 * TWO^512 |- TWO^256 */
bool simplicity_sha_256_block(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; /* env is unused. */
  uint32_t h[8];
  uint32_t block[16];
  read32s(h, 8, &src);
  read32s(block, 16, &src);
  simplicity_sha256_compression(h, block);
  write32s(dst, h, 8);
  return true;
}

/* sha_256_ctx_8_init : ONE |- CTX8
 * where
 * CTX8 = (TWO^8)^<64 * TWO^64 * TWO^256
 */
bool simplicity_sha_256_ctx_8_init(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; /* env is unused. */
  (void) src; /* env is unused. */

  uint32_t iv[8];
  sha256_context ctx = sha256_init(iv);

  return simplicity_write_sha256_context(dst, &ctx);
}

/* sha_256_ctx_8_add_n : CTX8 * (TWO^8)^n |- CTX8
 * where
 * n is a power of 2 less than or equal to 512
 * and
 * CTX8 = (TWO^8)^<64 * TWO^64 * TWO^256
 */
static bool sha_256_ctx_8_add_n(frameItem* dst, frameItem *src, size_t n) {
  simplicity_debug_assert(0 < n && n <= 512 && (n & (n - 1)) == 0);
  sha256_midstate midstate;
  unsigned char buf[512];
  sha256_context ctx = {.output = midstate.s};

  if (!simplicity_read_sha256_context(&ctx, src)) return false;
  read8s(buf, n, src);
  sha256_uchars(&ctx, buf, n);
  return simplicity_write_sha256_context(dst, &ctx);
}

/* sha_256_ctx_8_add_1 : CTX8 * TWO^8 |- CTX8
 * where
 * CTX8 = (TWO^8)^<64 * TWO^64 * TWO^256
 */
bool simplicity_sha_256_ctx_8_add_1(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; /* env is unused. */
  return sha_256_ctx_8_add_n(dst, &src, 1);
}

/* sha_256_ctx_8_add_2 : CTX8 * (TWO^8)^2 |- CTX8
 * where
 * CTX8 = (TWO^8)^<64 * TWO^64 * TWO^256
 */
bool simplicity_sha_256_ctx_8_add_2(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; /* env is unused. */
  return sha_256_ctx_8_add_n(dst, &src, 2);
}

/* sha_256_ctx_8_add_4 : CTX8 * (TWO^8)^4 |- CTX8
 * where
 * CTX8 = (TWO^8)^<64 * TWO^64 * TWO^256
 */
bool simplicity_sha_256_ctx_8_add_4(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; /* env is unused. */
  return sha_256_ctx_8_add_n(dst, &src, 4);
}

/* sha_256_ctx_8_add_8 : CTX8 * (TWO^8)^8 |- CTX8
 * where
 * CTX8 = (TWO^8)^<64 * TWO^64 * TWO^256
 */
bool simplicity_sha_256_ctx_8_add_8(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; /* env is unused. */
  return sha_256_ctx_8_add_n(dst, &src, 8);
}

/* sha_256_ctx_8_add_16 : CTX8 * (TWO^8)^16 |- CTX8
 * where
 * CTX8 = (TWO^8)^<64 * TWO^64 * TWO^256
 */
bool simplicity_sha_256_ctx_8_add_16(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; /* env is unused. */
  return sha_256_ctx_8_add_n(dst, &src, 16);
}

/* sha_256_ctx_8_add_32 : CTX8 * (TWO^8)^32 |- CTX8
 * where
 * CTX8 = (TWO^8)^<64 * TWO^64 * TWO^256
 */
bool simplicity_sha_256_ctx_8_add_32(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; /* env is unused. */
  return sha_256_ctx_8_add_n(dst, &src, 32);
}

/* sha_256_ctx_8_add_64 : CTX8 * (TWO^8)^64 |- CTX8
 * where
 * CTX8 = (TWO^8)^<64 * TWO^64 * TWO^256
 */
bool simplicity_sha_256_ctx_8_add_64(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; /* env is unused. */
  return sha_256_ctx_8_add_n(dst, &src, 64);
}

/* sha_256_ctx_8_add_128 : CTX8 * (TWO^8)^128 |- CTX8
 * where
 * CTX8 = (TWO^8)^<64 * TWO^64 * TWO^256
 */
bool simplicity_sha_256_ctx_8_add_128(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; /* env is unused. */
  return sha_256_ctx_8_add_n(dst, &src, 128);
}

/* sha_256_ctx_8_add_256 : CTX8 * (TWO^8)^256 |- CTX8
 * where
 * CTX8 = (TWO^8)^<64 * TWO^64 * TWO^256
 */
bool simplicity_sha_256_ctx_8_add_256(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; /* env is unused. */
  return sha_256_ctx_8_add_n(dst, &src, 256);
}

/* sha_256_ctx_8_add_512 : CTX8 * (TWO^8)^512 |- CTX8
 * where
 * CTX8 = (TWO^8)^<64 * TWO^64 * TWO^256
 */
bool simplicity_sha_256_ctx_8_add_512(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; /* env is unused. */
  return sha_256_ctx_8_add_n(dst, &src, 512);
}

/* sha_256_ctx_8_add_buffer_511 : CTX8 * (TWO^8)^<512 |- CTX8
 * where
 * CTX8 = (TWO^8)^<64 * TWO^64 * TWO^256
 */
bool simplicity_sha_256_ctx_8_add_buffer_511(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; /* env is unused. */
  sha256_midstate midstate;
  unsigned char buf[511];
  size_t buf_len;
  sha256_context ctx = {.output = midstate.s};

  if (!simplicity_read_sha256_context(&ctx, &src)) return false;

  simplicity_read_buffer8(buf, &buf_len, &src, 8);
  sha256_uchars(&ctx, buf, buf_len);
  return simplicity_write_sha256_context(dst, &ctx);
}

/* sha_256_ctx_8_finalize : CTX8 |- TWO^256
 * where
 * CTX8 = (TWO^8)^<64 * TWO^64 * TWO^256
 */
bool simplicity_sha_256_ctx_8_finalize(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; /* env is unused. */
  sha256_midstate midstate;
  sha256_context ctx = {.output = midstate.s};

  if (!simplicity_read_sha256_context(&ctx, &src)) return false;

  sha256_finalize(&ctx);
  write32s(dst, midstate.s, 8);
  return true;
}

/* parse_sequence : TWO^32 |- TWO^32 + TWO^32 */
bool simplicity_parse_lock(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; /* env is unused. */
  uint_fast32_t nLockTime = simplicity_read32(&src);
  writeBit(dst, 500000000U <= nLockTime);
  simplicity_write32(dst, nLockTime);
  return true;
}

/* parse_sequence : TWO^32 |- S (TWO^16 + TWO^16) */
bool simplicity_parse_sequence(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; /* env is unused. */
  uint_fast32_t nSequence = simplicity_read32(&src);
  if (writeBit(dst, nSequence < ((uint_fast32_t)1 << 31))) {
    writeBit(dst, nSequence & ((uint_fast32_t)1 << 22));
    simplicity_write16(dst, nSequence & 0xffff);
  } else {
    skipBits(dst, 17);
  }
  return true;
}

/* simplicity_tapdata_init : ONE |- CTX8 */
bool simplicity_tapdata_init(frameItem* dst, frameItem src, const txEnv* env) {
  (void) env; /* env is unused. */
  (void) src; /* env is unused. */

  sha256_midstate tapleafTag;
  {
    static const unsigned char tagName[] = "TapData";
    sha256_context ctx = sha256_init(tapleafTag.s);
    sha256_uchars(&ctx, tagName, sizeof(tagName) - 1);
    sha256_finalize(&ctx);
  }
  uint32_t iv[8];
  sha256_context ctx = sha256_init(iv);
  sha256_hash(&ctx, &tapleafTag);
  sha256_hash(&ctx, &tapleafTag);

  return simplicity_write_sha256_context(dst, &ctx);
}
