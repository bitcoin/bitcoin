// Copyright 2008 The CRC32C Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "./crc32c_sse42.h"

// In a separate source file to allow this accelerated CRC32C function to be
// compiled with the appropriate compiler flags to enable SSE4.2 instructions.

// This implementation is loosely based on Intel Pub 323405 from April 2011,
// "Fast CRC Computation for iSCSI Polynomial Using CRC32 Instruction".

#include <cstddef>
#include <cstdint>

#include "./crc32c_internal.h"
#include "./crc32c_prefetch.h"
#include "./crc32c_read_le.h"
#include "./crc32c_round_up.h"
#ifdef CRC32C_HAVE_CONFIG_H
#include "crc32c/crc32c_config.h"
#endif

#if HAVE_SSE42 && (defined(_M_X64) || defined(__x86_64__))

#if defined(_MSC_VER)
#include <intrin.h>
#else  // !defined(_MSC_VER)
#include <nmmintrin.h>
#endif  // defined(_MSC_VER)

namespace crc32c {

namespace {

constexpr const ptrdiff_t kGroups = 3;
constexpr const ptrdiff_t kBlock0Size = 16 * 1024 / kGroups / 64 * 64;
constexpr const ptrdiff_t kBlock1Size = 4 * 1024 / kGroups / 8 * 8;
constexpr const ptrdiff_t kBlock2Size = 1024 / kGroups / 8 * 8;

const uint32_t kBlock0SkipTable[8][16] = {
    {0x00000000, 0xff770459, 0xfb027e43, 0x04757a1a, 0xf3e88a77, 0x0c9f8e2e,
     0x08eaf434, 0xf79df06d, 0xe23d621f, 0x1d4a6646, 0x193f1c5c, 0xe6481805,
     0x11d5e868, 0xeea2ec31, 0xead7962b, 0x15a09272},
    {0x00000000, 0xc196b2cf, 0x86c1136f, 0x4757a1a0, 0x086e502f, 0xc9f8e2e0,
     0x8eaf4340, 0x4f39f18f, 0x10dca05e, 0xd14a1291, 0x961db331, 0x578b01fe,
     0x18b2f071, 0xd92442be, 0x9e73e31e, 0x5fe551d1},
    {0x00000000, 0x21b940bc, 0x43728178, 0x62cbc1c4, 0x86e502f0, 0xa75c424c,
     0xc5978388, 0xe42ec334, 0x08267311, 0x299f33ad, 0x4b54f269, 0x6aedb2d5,
     0x8ec371e1, 0xaf7a315d, 0xcdb1f099, 0xec08b025},
    {0x00000000, 0x104ce622, 0x2099cc44, 0x30d52a66, 0x41339888, 0x517f7eaa,
     0x61aa54cc, 0x71e6b2ee, 0x82673110, 0x922bd732, 0xa2fefd54, 0xb2b21b76,
     0xc354a998, 0xd3184fba, 0xe3cd65dc, 0xf38183fe},
    {0x00000000, 0x012214d1, 0x024429a2, 0x03663d73, 0x04885344, 0x05aa4795,
     0x06cc7ae6, 0x07ee6e37, 0x0910a688, 0x0832b259, 0x0b548f2a, 0x0a769bfb,
     0x0d98f5cc, 0x0cbae11d, 0x0fdcdc6e, 0x0efec8bf},
    {0x00000000, 0x12214d10, 0x24429a20, 0x3663d730, 0x48853440, 0x5aa47950,
     0x6cc7ae60, 0x7ee6e370, 0x910a6880, 0x832b2590, 0xb548f2a0, 0xa769bfb0,
     0xd98f5cc0, 0xcbae11d0, 0xfdcdc6e0, 0xefec8bf0},
    {0x00000000, 0x27f8a7f1, 0x4ff14fe2, 0x6809e813, 0x9fe29fc4, 0xb81a3835,
     0xd013d026, 0xf7eb77d7, 0x3a294979, 0x1dd1ee88, 0x75d8069b, 0x5220a16a,
     0xa5cbd6bd, 0x8233714c, 0xea3a995f, 0xcdc23eae},
    {0x00000000, 0x745292f2, 0xe8a525e4, 0x9cf7b716, 0xd4a63d39, 0xa0f4afcb,
     0x3c0318dd, 0x48518a2f, 0xaca00c83, 0xd8f29e71, 0x44052967, 0x3057bb95,
     0x780631ba, 0x0c54a348, 0x90a3145e, 0xe4f186ac},
};
const uint32_t kBlock1SkipTable[8][16] = {
    {0x00000000, 0x79113270, 0xf22264e0, 0x8b335690, 0xe1a8bf31, 0x98b98d41,
     0x138adbd1, 0x6a9be9a1, 0xc6bd0893, 0xbfac3ae3, 0x349f6c73, 0x4d8e5e03,
     0x2715b7a2, 0x5e0485d2, 0xd537d342, 0xac26e132},
    {0x00000000, 0x889667d7, 0x14c0b95f, 0x9c56de88, 0x298172be, 0xa1171569,
     0x3d41cbe1, 0xb5d7ac36, 0x5302e57c, 0xdb9482ab, 0x47c25c23, 0xcf543bf4,
     0x7a8397c2, 0xf215f015, 0x6e432e9d, 0xe6d5494a},
    {0x00000000, 0xa605caf8, 0x49e7e301, 0xefe229f9, 0x93cfc602, 0x35ca0cfa,
     0xda282503, 0x7c2deffb, 0x2273faf5, 0x8476300d, 0x6b9419f4, 0xcd91d30c,
     0xb1bc3cf7, 0x17b9f60f, 0xf85bdff6, 0x5e5e150e},
    {0x00000000, 0x44e7f5ea, 0x89cfebd4, 0xcd281e3e, 0x1673a159, 0x529454b3,
     0x9fbc4a8d, 0xdb5bbf67, 0x2ce742b2, 0x6800b758, 0xa528a966, 0xe1cf5c8c,
     0x3a94e3eb, 0x7e731601, 0xb35b083f, 0xf7bcfdd5},
    {0x00000000, 0x59ce8564, 0xb39d0ac8, 0xea538fac, 0x62d66361, 0x3b18e605,
     0xd14b69a9, 0x8885eccd, 0xc5acc6c2, 0x9c6243a6, 0x7631cc0a, 0x2fff496e,
     0xa77aa5a3, 0xfeb420c7, 0x14e7af6b, 0x4d292a0f},
    {0x00000000, 0x8eb5fb75, 0x1887801b, 0x96327b6e, 0x310f0036, 0xbfbafb43,
     0x2988802d, 0xa73d7b58, 0x621e006c, 0xecabfb19, 0x7a998077, 0xf42c7b02,
     0x5311005a, 0xdda4fb2f, 0x4b968041, 0xc5237b34},
    {0x00000000, 0xc43c00d8, 0x8d947741, 0x49a87799, 0x1ec49873, 0xdaf898ab,
     0x9350ef32, 0x576cefea, 0x3d8930e6, 0xf9b5303e, 0xb01d47a7, 0x7421477f,
     0x234da895, 0xe771a84d, 0xaed9dfd4, 0x6ae5df0c},
    {0x00000000, 0x7b1261cc, 0xf624c398, 0x8d36a254, 0xe9a5f1c1, 0x92b7900d,
     0x1f813259, 0x64935395, 0xd6a79573, 0xadb5f4bf, 0x208356eb, 0x5b913727,
     0x3f0264b2, 0x4410057e, 0xc926a72a, 0xb234c6e6},
};
const uint32_t kBlock2SkipTable[8][16] = {
    {0x00000000, 0x8f158014, 0x1bc776d9, 0x94d2f6cd, 0x378eedb2, 0xb89b6da6,
     0x2c499b6b, 0xa35c1b7f, 0x6f1ddb64, 0xe0085b70, 0x74daadbd, 0xfbcf2da9,
     0x589336d6, 0xd786b6c2, 0x4354400f, 0xcc41c01b},
    {0x00000000, 0xde3bb6c8, 0xb99b1b61, 0x67a0ada9, 0x76da4033, 0xa8e1f6fb,
     0xcf415b52, 0x117aed9a, 0xedb48066, 0x338f36ae, 0x542f9b07, 0x8a142dcf,
     0x9b6ec055, 0x4555769d, 0x22f5db34, 0xfcce6dfc},
    {0x00000000, 0xde85763d, 0xb8e69a8b, 0x6663ecb6, 0x742143e7, 0xaaa435da,
     0xccc7d96c, 0x1242af51, 0xe84287ce, 0x36c7f1f3, 0x50a41d45, 0x8e216b78,
     0x9c63c429, 0x42e6b214, 0x24855ea2, 0xfa00289f},
    {0x00000000, 0xd569796d, 0xaf3e842b, 0x7a57fd46, 0x5b917ea7, 0x8ef807ca,
     0xf4affa8c, 0x21c683e1, 0xb722fd4e, 0x624b8423, 0x181c7965, 0xcd750008,
     0xecb383e9, 0x39dafa84, 0x438d07c2, 0x96e47eaf},
    {0x00000000, 0x6ba98c6d, 0xd75318da, 0xbcfa94b7, 0xab4a4745, 0xc0e3cb28,
     0x7c195f9f, 0x17b0d3f2, 0x5378f87b, 0x38d17416, 0x842be0a1, 0xef826ccc,
     0xf832bf3e, 0x939b3353, 0x2f61a7e4, 0x44c82b89},
    {0x00000000, 0xa6f1f0f6, 0x480f971d, 0xeefe67eb, 0x901f2e3a, 0x36eedecc,
     0xd810b927, 0x7ee149d1, 0x25d22a85, 0x8323da73, 0x6dddbd98, 0xcb2c4d6e,
     0xb5cd04bf, 0x133cf449, 0xfdc293a2, 0x5b336354},
    {0x00000000, 0x4ba4550a, 0x9748aa14, 0xdcecff1e, 0x2b7d22d9, 0x60d977d3,
     0xbc3588cd, 0xf791ddc7, 0x56fa45b2, 0x1d5e10b8, 0xc1b2efa6, 0x8a16baac,
     0x7d87676b, 0x36233261, 0xeacfcd7f, 0xa16b9875},
    {0x00000000, 0xadf48b64, 0x5e056039, 0xf3f1eb5d, 0xbc0ac072, 0x11fe4b16,
     0xe20fa04b, 0x4ffb2b2f, 0x7df9f615, 0xd00d7d71, 0x23fc962c, 0x8e081d48,
     0xc1f33667, 0x6c07bd03, 0x9ff6565e, 0x3202dd3a},
};

constexpr const ptrdiff_t kPrefetchHorizon = 256;

}  // namespace

uint32_t ExtendSse42(uint32_t crc, const uint8_t* data, size_t size) {
  const uint8_t* p = data;
  const uint8_t* e = data + size;
  uint32_t l = crc ^ kCRC32Xor;

#define STEP1                  \
  do {                         \
    l = _mm_crc32_u8(l, *p++); \
  } while (0)

#define STEP4(crc)                             \
  do {                                         \
    crc = _mm_crc32_u32(crc, ReadUint32LE(p)); \
    p += 4;                                    \
  } while (0)

#define STEP8(crc, data)                          \
  do {                                            \
    crc = _mm_crc32_u64(crc, ReadUint64LE(data)); \
    data += 8;                                    \
  } while (0)

#define STEP8BY3(crc0, crc1, crc2, p0, p1, p2) \
  do {                                         \
    STEP8(crc0, p0);                           \
    STEP8(crc1, p1);                           \
    STEP8(crc2, p2);                           \
  } while (0)

#define STEP8X3(crc0, crc1, crc2, bs)                     \
  do {                                                    \
    crc0 = _mm_crc32_u64(crc0, ReadUint64LE(p));          \
    crc1 = _mm_crc32_u64(crc1, ReadUint64LE(p + bs));     \
    crc2 = _mm_crc32_u64(crc2, ReadUint64LE(p + 2 * bs)); \
    p += 8;                                               \
  } while (0)

#define SKIP_BLOCK(crc, tab)                                      \
  do {                                                            \
    crc = tab[0][crc & 0xf] ^ tab[1][(crc >> 4) & 0xf] ^          \
          tab[2][(crc >> 8) & 0xf] ^ tab[3][(crc >> 12) & 0xf] ^  \
          tab[4][(crc >> 16) & 0xf] ^ tab[5][(crc >> 20) & 0xf] ^ \
          tab[6][(crc >> 24) & 0xf] ^ tab[7][(crc >> 28) & 0xf];  \
  } while (0)

  // Point x at first 8-byte aligned byte in the buffer. This might be past the
  // end of the buffer.
  const uint8_t* x = RoundUp<8>(p);
  if (x <= e) {
    // Process bytes p is 8-byte aligned.
    while (p != x) {
      STEP1;
    }
  }

  // Process the data in predetermined block sizes with tables for quickly
  // combining the checksum. Experimentally it's better to use larger block
  // sizes where possible so use a hierarchy of decreasing block sizes.
  uint64_t l64 = l;
  while ((e - p) >= kGroups * kBlock0Size) {
    uint64_t l641 = 0;
    uint64_t l642 = 0;
    for (int i = 0; i < kBlock0Size; i += 8 * 8) {
      // Prefetch ahead to hide latency.
      RequestPrefetch(p + kPrefetchHorizon);
      RequestPrefetch(p + kBlock0Size + kPrefetchHorizon);
      RequestPrefetch(p + 2 * kBlock0Size + kPrefetchHorizon);

      // Process 64 bytes at a time.
      STEP8X3(l64, l641, l642, kBlock0Size);
      STEP8X3(l64, l641, l642, kBlock0Size);
      STEP8X3(l64, l641, l642, kBlock0Size);
      STEP8X3(l64, l641, l642, kBlock0Size);
      STEP8X3(l64, l641, l642, kBlock0Size);
      STEP8X3(l64, l641, l642, kBlock0Size);
      STEP8X3(l64, l641, l642, kBlock0Size);
      STEP8X3(l64, l641, l642, kBlock0Size);
    }

    // Combine results.
    SKIP_BLOCK(l64, kBlock0SkipTable);
    l64 ^= l641;
    SKIP_BLOCK(l64, kBlock0SkipTable);
    l64 ^= l642;
    p += (kGroups - 1) * kBlock0Size;
  }
  while ((e - p) >= kGroups * kBlock1Size) {
    uint64_t l641 = 0;
    uint64_t l642 = 0;
    for (int i = 0; i < kBlock1Size; i += 8) {
      STEP8X3(l64, l641, l642, kBlock1Size);
    }
    SKIP_BLOCK(l64, kBlock1SkipTable);
    l64 ^= l641;
    SKIP_BLOCK(l64, kBlock1SkipTable);
    l64 ^= l642;
    p += (kGroups - 1) * kBlock1Size;
  }
  while ((e - p) >= kGroups * kBlock2Size) {
    uint64_t l641 = 0;
    uint64_t l642 = 0;
    for (int i = 0; i < kBlock2Size; i += 8) {
      STEP8X3(l64, l641, l642, kBlock2Size);
    }
    SKIP_BLOCK(l64, kBlock2SkipTable);
    l64 ^= l641;
    SKIP_BLOCK(l64, kBlock2SkipTable);
    l64 ^= l642;
    p += (kGroups - 1) * kBlock2Size;
  }

  // Process bytes 16 at a time
  while ((e - p) >= 16) {
    STEP8(l64, p);
    STEP8(l64, p);
  }

  l = static_cast<uint32_t>(l64);
  // Process the last few bytes.
  while (p != e) {
    STEP1;
  }
#undef SKIP_BLOCK
#undef STEP8X3
#undef STEP8BY3
#undef STEP8
#undef STEP4
#undef STEP1

  return l ^ kCRC32Xor;
}

}  // namespace crc32c

#endif  // HAVE_SSE42 && (defined(_M_X64) || defined(__x86_64__))
