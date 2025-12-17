#if defined(__SHA__) && defined(__SSE4_1__) && (defined(__x86_64__) || defined(__amd64__) || defined(__i386__))

#include <cpuid.h>
#include <stdbool.h>
#include <immintrin.h>
#include "sha256.h"

static bool cpu_has_sha_ni(void) {
  const uint32_t sse4_flag = (uint32_t)1 << 19;
  const uint32_t sha_ni_flag = (uint32_t)1 << 29;
  bool sse4, sha_ni;
  uint32_t eax, ebx, ecx, edx;
  __cpuid(0, eax, ebx, ecx, edx);
  if (eax < 7) return false;
  __cpuid_count(1, 0, eax, ebx, ecx, edx);
  sse4 = ecx & sse4_flag;
  __cpuid_count(7, 0, eax, ebx, ecx, edx);
  sha_ni = ebx & sha_ni_flag;
  return sse4 && sha_ni;
}

/* sha256_compression_x86_sha_ni based on
   <https://github.com/bitcoin/bitcoin/blob/b1ba1b178f501daa1afdd91f9efec34e5ec1e294/src/crypto/sha256_x86_shani.cpp>
*/
/* Given a 256-bit 'midstate' and a 512-bit 'block', then 'midstate' becomes the value of the SHA-256 compression function ("added" to the original 'midstate' value).
 *
 * Precondition: uint32_t midstate[8];
 *               uint32_t block[16]
 */
static void sha256_compression_x86_sha_ni(uint32_t* midstate, const uint32_t* block) {
  __m128i msg, t1, t2, m0, m1, m2, m3, s0, s1, so0, so1;

  /* Load state */
  s0 = _mm_loadu_si128((const __m128i*)(midstate + 0));
  s1 = _mm_loadu_si128((const __m128i*)(midstate + 4));
  t1 = _mm_shuffle_epi32(s0, 0xB1);
  t2 = _mm_shuffle_epi32(s1, 0x1B);
  s0 = _mm_alignr_epi8(t1, t2, 0x08);
  s1 = _mm_blend_epi16(t2, t1, 0xF0);

  /* Remember old state */
  so0 = s0;
  so1 = s1;

  /* Rounds 0 - 3 */
  m0 = _mm_shuffle_epi32(_mm_loadu_si128((const __m128i*)(block + 0)), 0xE4);
  msg = _mm_add_epi32(m0, _mm_set_epi64x(0xE9B5DBA5B5C0FBCFULL, 0x71374491428A2F98ULL));
  s1 = _mm_sha256rnds2_epu32(s1, s0, msg);
  s0 = _mm_sha256rnds2_epu32(s0, s1, _mm_shuffle_epi32(msg, 0x0e));

  /* Rounds 4 - 7 */
  m1 = _mm_shuffle_epi32(_mm_loadu_si128((const __m128i*)(block + 4)), 0xE4);
  msg = _mm_add_epi32(m1, _mm_set_epi64x(0xAB1C5ED5923F82A4ULL, 0x59F111F13956C25BULL));
  s1 = _mm_sha256rnds2_epu32(s1, s0, msg);
  s0 = _mm_sha256rnds2_epu32(s0, s1, _mm_shuffle_epi32(msg, 0x0e));
  m0 = _mm_sha256msg1_epu32(m0, m1);

  /* Rounds 8 - 11 */
  m2 = _mm_shuffle_epi32(_mm_loadu_si128((const __m128i*)(block + 8)), 0xE4);
  msg = _mm_add_epi32(m2, _mm_set_epi64x(0x550C7DC3243185BEULL, 0x12835B01D807AA98ULL));
  s1 = _mm_sha256rnds2_epu32(s1, s0, msg);
  s0 = _mm_sha256rnds2_epu32(s0, s1, _mm_shuffle_epi32(msg, 0x0e));
  m1 = _mm_sha256msg1_epu32(m1, m2);

  /* Rounds 12 - 15 */
  m3 = _mm_shuffle_epi32(_mm_loadu_si128((const __m128i*)(block + 12)), 0xE4);
  msg = _mm_add_epi32(m3, _mm_set_epi64x(0xC19BF1749BDC06A7ULL, 0x80DEB1FE72BE5D74ULL));
  s1 = _mm_sha256rnds2_epu32(s1, s0, msg);
  s0 = _mm_sha256rnds2_epu32(s0, s1, _mm_shuffle_epi32(msg, 0x0e));
  m0 = _mm_sha256msg2_epu32(_mm_add_epi32(m0, _mm_alignr_epi8(m3, m2, 4)), m3);
  m2 = _mm_sha256msg1_epu32(m2, m3);

  /* Rounds 16 - 19 */
  msg = _mm_add_epi32(m0, _mm_set_epi64x(0x240CA1CC0FC19DC6ULL, 0xEFBE4786E49B69C1ULL));
  s1 = _mm_sha256rnds2_epu32(s1, s0, msg);
  s0 = _mm_sha256rnds2_epu32(s0, s1, _mm_shuffle_epi32(msg, 0x0e));
  m1 = _mm_sha256msg2_epu32(_mm_add_epi32(m1, _mm_alignr_epi8(m0, m3, 4)), m0);
  m3 = _mm_sha256msg1_epu32(m3, m0);

  /* Rounds 20 - 23 */
  msg = _mm_add_epi32(m1, _mm_set_epi64x(0x76F988DA5CB0A9DCULL, 0x4A7484AA2DE92C6FULL));
  s1 = _mm_sha256rnds2_epu32(s1, s0, msg);
  s0 = _mm_sha256rnds2_epu32(s0, s1, _mm_shuffle_epi32(msg, 0x0e));
  m2 = _mm_sha256msg2_epu32(_mm_add_epi32(m2, _mm_alignr_epi8(m1, m0, 4)), m1);
  m0 = _mm_sha256msg1_epu32(m0, m1);

  /* Rounds 24 - 27 */
  msg = _mm_add_epi32(m2, _mm_set_epi64x(0xBF597FC7B00327C8ULL, 0xA831C66D983E5152ULL));
  s1 = _mm_sha256rnds2_epu32(s1, s0, msg);
  s0 = _mm_sha256rnds2_epu32(s0, s1, _mm_shuffle_epi32(msg, 0x0e));
  m3 = _mm_sha256msg2_epu32(_mm_add_epi32(m3, _mm_alignr_epi8(m2, m1, 4)), m2);
  m1 = _mm_sha256msg1_epu32(m1, m2);

  /* Rounds 28 - 31 */
  msg = _mm_add_epi32(m3, _mm_set_epi64x(0x1429296706CA6351ULL, 0xD5A79147C6E00BF3ULL));
  s1 = _mm_sha256rnds2_epu32(s1, s0, msg);
  s0 = _mm_sha256rnds2_epu32(s0, s1, _mm_shuffle_epi32(msg, 0x0e));
  m0 = _mm_sha256msg2_epu32(_mm_add_epi32(m0, _mm_alignr_epi8(m3, m2, 4)), m3);
  m2 = _mm_sha256msg1_epu32(m2, m3);

  /* Rounds 32 - 35 */
  msg = _mm_add_epi32(m0, _mm_set_epi64x(0x53380D134D2C6DFCULL, 0x2E1B213827B70A85ULL));
  s1 = _mm_sha256rnds2_epu32(s1, s0, msg);
  s0 = _mm_sha256rnds2_epu32(s0, s1, _mm_shuffle_epi32(msg, 0x0e));
  m1 = _mm_sha256msg2_epu32(_mm_add_epi32(m1, _mm_alignr_epi8(m0, m3, 4)), m0);
  m3 = _mm_sha256msg1_epu32(m3, m0);

  /* Rounds 36 - 39 */
  msg = _mm_add_epi32(m1, _mm_set_epi64x(0x92722C8581C2C92EULL, 0x766A0ABB650A7354ULL));
  s1 = _mm_sha256rnds2_epu32(s1, s0, msg);
  s0 = _mm_sha256rnds2_epu32(s0, s1, _mm_shuffle_epi32(msg, 0x0e));
  m2 = _mm_sha256msg2_epu32(_mm_add_epi32(m2, _mm_alignr_epi8(m1, m0, 4)), m1);
  m0 = _mm_sha256msg1_epu32(m0, m1);

  /* Rounds 40 - 43 */
  msg = _mm_add_epi32(m2, _mm_set_epi64x(0xC76C51A3C24B8B70ULL, 0xA81A664BA2BFE8A1ULL));
  s1 = _mm_sha256rnds2_epu32(s1, s0, msg);
  s0 = _mm_sha256rnds2_epu32(s0, s1, _mm_shuffle_epi32(msg, 0x0e));
  m3 = _mm_sha256msg2_epu32(_mm_add_epi32(m3, _mm_alignr_epi8(m2, m1, 4)), m2);
  m1 = _mm_sha256msg1_epu32(m1, m2);

  /* Rounds 44 - 47 */
  msg = _mm_add_epi32(m3, _mm_set_epi64x(0x106AA070F40E3585ULL, 0xD6990624D192E819ULL));
  s1 = _mm_sha256rnds2_epu32(s1, s0, msg);
  s0 = _mm_sha256rnds2_epu32(s0, s1, _mm_shuffle_epi32(msg, 0x0e));
  m0 = _mm_sha256msg2_epu32(_mm_add_epi32(m0, _mm_alignr_epi8(m3, m2, 4)), m3);
  m2 = _mm_sha256msg1_epu32(m2, m3);

  /* Rounds 48 - 51 */
  msg = _mm_add_epi32(m0, _mm_set_epi64x(0x34B0BCB52748774CULL, 0x1E376C0819A4C116ULL));
  s1 = _mm_sha256rnds2_epu32(s1, s0, msg);
  s0 = _mm_sha256rnds2_epu32(s0, s1, _mm_shuffle_epi32(msg, 0x0e));
  m1 = _mm_sha256msg2_epu32(_mm_add_epi32(m1, _mm_alignr_epi8(m0, m3, 4)), m0);
  m3 = _mm_sha256msg1_epu32(m3, m0);

  /* Rounds 52 - 55 */
  msg = _mm_add_epi32(m1, _mm_set_epi64x(0x682E6FF35B9CCA4FULL, 0x4ED8AA4A391C0CB3ULL));
  s1 = _mm_sha256rnds2_epu32(s1, s0, msg);
  s0 = _mm_sha256rnds2_epu32(s0, s1, _mm_shuffle_epi32(msg, 0x0e));
  m2 = _mm_sha256msg2_epu32(_mm_add_epi32(m2, _mm_alignr_epi8(m1, m0, 4)), m1);

  /* Rounds 56 - 59 */
  msg = _mm_add_epi32(m2, _mm_set_epi64x(0x8CC7020884C87814ULL, 0x78A5636F748F82EEULL));
  s1 = _mm_sha256rnds2_epu32(s1, s0, msg);
  s0 = _mm_sha256rnds2_epu32(s0, s1, _mm_shuffle_epi32(msg, 0x0e));
  m3 = _mm_sha256msg2_epu32(_mm_add_epi32(m3, _mm_alignr_epi8(m2, m1, 4)), m2);

  /* Rounds 60 - 63 */
  msg = _mm_add_epi32(m3, _mm_set_epi64x(0xC67178F2BEF9A3F7ULL, 0xA4506CEB90BEFFFAULL));
  s1 = _mm_sha256rnds2_epu32(s1, s0, msg);
  s0 = _mm_sha256rnds2_epu32(s0, s1, _mm_shuffle_epi32(msg, 0x0e));

  /* Combine with old state */
  s0 = _mm_add_epi32(s0, so0);
  s1 = _mm_add_epi32(s1, so1);

  /* Store state */
  t1 = _mm_shuffle_epi32(s0, 0x1B);
  t2 = _mm_shuffle_epi32(s1, 0xB1);
  s0 = _mm_blend_epi16(t1, t2, 0xF0);
  s1 = _mm_alignr_epi8(t2, t1, 0x08);
  _mm_storeu_si128((__m128i*)(midstate + 0), s0);
  _mm_storeu_si128((__m128i*)(midstate + 4), s1);
}

/* This function inspects the CPU's capabilities and enables the use of some optimized instructions, if they are known and available.
 */
extern void simplicity_sha256_cpu_optimize_not_thread_safe(void) __attribute__((constructor));
extern void simplicity_sha256_cpu_optimize_not_thread_safe(void) {
  if (cpu_has_sha_ni()) {
    simplicity_sha256_compression = sha256_compression_x86_sha_ni;
  }
}

#endif
