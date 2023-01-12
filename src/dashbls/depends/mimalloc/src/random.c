/* ----------------------------------------------------------------------------
Copyright (c) 2019-2021, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE   // for syscall() on Linux
#endif

#include "mimalloc.h"
#include "mimalloc-internal.h"

#include <string.h> // memset

/* ----------------------------------------------------------------------------
We use our own PRNG to keep predictable performance of random number generation
and to avoid implementations that use a lock. We only use the OS provided
random source to initialize the initial seeds. Since we do not need ultimate
performance but we do rely on the security (for secret cookies in secure mode)
we use a cryptographically secure generator (chacha20).
-----------------------------------------------------------------------------*/

#define MI_CHACHA_ROUNDS (20)   // perhaps use 12 for better performance?


/* ----------------------------------------------------------------------------
Chacha20 implementation as the original algorithm with a 64-bit nonce
and counter: https://en.wikipedia.org/wiki/Salsa20
The input matrix has sixteen 32-bit values:
Position  0 to  3: constant key
Position  4 to 11: the key
Position 12 to 13: the counter.
Position 14 to 15: the nonce.

The implementation uses regular C code which compiles very well on modern compilers.
(gcc x64 has no register spills, and clang 6+ uses SSE instructions)
-----------------------------------------------------------------------------*/

static inline uint32_t rotl(uint32_t x, uint32_t shift) {
  return (x << shift) | (x >> (32 - shift));
}

static inline void qround(uint32_t x[16], size_t a, size_t b, size_t c, size_t d) {
  x[a] += x[b]; x[d] = rotl(x[d] ^ x[a], 16);
  x[c] += x[d]; x[b] = rotl(x[b] ^ x[c], 12);
  x[a] += x[b]; x[d] = rotl(x[d] ^ x[a], 8);
  x[c] += x[d]; x[b] = rotl(x[b] ^ x[c], 7);
}

static void chacha_block(mi_random_ctx_t* ctx)
{
  // scramble into `x`
  uint32_t x[16];
  for (size_t i = 0; i < 16; i++) {
    x[i] = ctx->input[i];
  }
  for (size_t i = 0; i < MI_CHACHA_ROUNDS; i += 2) {
    qround(x, 0, 4,  8, 12);
    qround(x, 1, 5,  9, 13);
    qround(x, 2, 6, 10, 14);
    qround(x, 3, 7, 11, 15);
    qround(x, 0, 5, 10, 15);
    qround(x, 1, 6, 11, 12);
    qround(x, 2, 7,  8, 13);
    qround(x, 3, 4,  9, 14);
  }

  // add scrambled data to the initial state
  for (size_t i = 0; i < 16; i++) {
    ctx->output[i] = x[i] + ctx->input[i];
  }
  ctx->output_available = 16;

  // increment the counter for the next round
  ctx->input[12] += 1;
  if (ctx->input[12] == 0) {
    ctx->input[13] += 1;
    if (ctx->input[13] == 0) {  // and keep increasing into the nonce
      ctx->input[14] += 1;
    }
  }
}

static uint32_t chacha_next32(mi_random_ctx_t* ctx) {
  if (ctx->output_available <= 0) {
    chacha_block(ctx);
    ctx->output_available = 16; // (assign again to suppress static analysis warning)
  }
  const uint32_t x = ctx->output[16 - ctx->output_available];
  ctx->output[16 - ctx->output_available] = 0; // reset once the data is handed out
  ctx->output_available--;
  return x;
}

static inline uint32_t read32(const uint8_t* p, size_t idx32) {
  const size_t i = 4*idx32;
  return ((uint32_t)p[i+0] | (uint32_t)p[i+1] << 8 | (uint32_t)p[i+2] << 16 | (uint32_t)p[i+3] << 24);
}

static void chacha_init(mi_random_ctx_t* ctx, const uint8_t key[32], uint64_t nonce)
{
  // since we only use chacha for randomness (and not encryption) we
  // do not _need_ to read 32-bit values as little endian but we do anyways
  // just for being compatible :-)
  memset(ctx, 0, sizeof(*ctx));
  for (size_t i = 0; i < 4; i++) {
    const uint8_t* sigma = (uint8_t*)"expand 32-byte k";
    ctx->input[i] = read32(sigma,i);
  }
  for (size_t i = 0; i < 8; i++) {
    ctx->input[i + 4] = read32(key,i);
  }
  ctx->input[12] = 0;
  ctx->input[13] = 0;
  ctx->input[14] = (uint32_t)nonce;
  ctx->input[15] = (uint32_t)(nonce >> 32);
}

static void chacha_split(mi_random_ctx_t* ctx, uint64_t nonce, mi_random_ctx_t* ctx_new) {
  memset(ctx_new, 0, sizeof(*ctx_new));
  _mi_memcpy(ctx_new->input, ctx->input, sizeof(ctx_new->input));
  ctx_new->input[12] = 0;
  ctx_new->input[13] = 0;
  ctx_new->input[14] = (uint32_t)nonce;
  ctx_new->input[15] = (uint32_t)(nonce >> 32);
  mi_assert_internal(ctx->input[14] != ctx_new->input[14] || ctx->input[15] != ctx_new->input[15]); // do not reuse nonces!
  chacha_block(ctx_new);
}


/* ----------------------------------------------------------------------------
Random interface
-----------------------------------------------------------------------------*/

#if MI_DEBUG>1
static bool mi_random_is_initialized(mi_random_ctx_t* ctx) {
  return (ctx != NULL && ctx->input[0] != 0);
}
#endif

void _mi_random_split(mi_random_ctx_t* ctx, mi_random_ctx_t* ctx_new) {
  mi_assert_internal(mi_random_is_initialized(ctx));
  mi_assert_internal(ctx != ctx_new);
  chacha_split(ctx, (uintptr_t)ctx_new /*nonce*/, ctx_new);
}

uintptr_t _mi_random_next(mi_random_ctx_t* ctx) {
  mi_assert_internal(mi_random_is_initialized(ctx));
  #if MI_INTPTR_SIZE <= 4
    return chacha_next32(ctx);
  #elif MI_INTPTR_SIZE == 8
    return (((uintptr_t)chacha_next32(ctx) << 32) | chacha_next32(ctx));
  #else
  # error "define mi_random_next for this platform"
  #endif
}


/* ----------------------------------------------------------------------------
To initialize a fresh random context we rely on the OS:
- Windows     : BCryptGenRandom (or RtlGenRandom)
- macOS       : CCRandomGenerateBytes, arc4random_buf
- bsd,wasi    : arc4random_buf
- Linux       : getrandom,/dev/urandom
If we cannot get good randomness, we fall back to weak randomness based on a timer and ASLR.
-----------------------------------------------------------------------------*/

#if defined(_WIN32)

#if defined(MI_USE_RTLGENRANDOM) || defined(__cplusplus)
// We prefer to use BCryptGenRandom instead of (the unofficial) RtlGenRandom but when using 
// dynamic overriding, we observed it can raise an exception when compiled with C++, and 
// sometimes deadlocks when also running under the VS debugger.
// In contrast, issue #623 implies that on Windows Server 2019 we need to use BCryptGenRandom.
// To be continued..
#pragma comment (lib,"advapi32.lib")
#define RtlGenRandom  SystemFunction036
#ifdef __cplusplus
extern "C" {
#endif
BOOLEAN NTAPI RtlGenRandom(PVOID RandomBuffer, ULONG RandomBufferLength);
#ifdef __cplusplus
}
#endif
static bool os_random_buf(void* buf, size_t buf_len) {
  return (RtlGenRandom(buf, (ULONG)buf_len) != 0);
}
#else
#pragma comment (lib,"bcrypt.lib")
#include <bcrypt.h>
static bool os_random_buf(void* buf, size_t buf_len) {
  return (BCryptGenRandom(NULL, (PUCHAR)buf, (ULONG)buf_len, BCRYPT_USE_SYSTEM_PREFERRED_RNG) >= 0);
}
#endif

#elif defined(__APPLE__)
#include <AvailabilityMacros.h>
#if defined(MAC_OS_X_VERSION_10_10) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_10
#include <CommonCrypto/CommonCryptoError.h>
#include <CommonCrypto/CommonRandom.h>
#endif
static bool os_random_buf(void* buf, size_t buf_len) {
  #if defined(MAC_OS_X_VERSION_10_15) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_15
    // We prefere CCRandomGenerateBytes as it returns an error code while arc4random_buf
    // may fail silently on macOS. See PR #390, and <https://opensource.apple.com/source/Libc/Libc-1439.40.11/gen/FreeBSD/arc4random.c.auto.html>      
    return (CCRandomGenerateBytes(buf, buf_len) == kCCSuccess);
  #else
    // fall back on older macOS
    arc4random_buf(buf, buf_len);
    return true;
  #endif
}

#elif defined(__ANDROID__) || defined(__DragonFly__) || \
      defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || \
      defined(__sun) // todo: what to use with __wasi__?
#include <stdlib.h>
static bool os_random_buf(void* buf, size_t buf_len) {
  arc4random_buf(buf, buf_len);
  return true;
}
#elif defined(__linux__) || defined(__HAIKU__)
#if defined(__linux__)
#include <sys/syscall.h>
#endif
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
static bool os_random_buf(void* buf, size_t buf_len) {
  // Modern Linux provides `getrandom` but different distributions either use `sys/random.h` or `linux/random.h`
  // and for the latter the actual `getrandom` call is not always defined.
  // (see <https://stackoverflow.com/questions/45237324/why-doesnt-getrandom-compile>)
  // We therefore use a syscall directly and fall back dynamically to /dev/urandom when needed.
#ifdef SYS_getrandom
  #ifndef GRND_NONBLOCK
  #define GRND_NONBLOCK (1)
  #endif
  static _Atomic(uintptr_t) no_getrandom; // = 0
  if (mi_atomic_load_acquire(&no_getrandom)==0) {
    ssize_t ret = syscall(SYS_getrandom, buf, buf_len, GRND_NONBLOCK);
    if (ret >= 0) return (buf_len == (size_t)ret);
    if (errno != ENOSYS) return false;
    mi_atomic_store_release(&no_getrandom, 1UL); // don't call again, and fall back to /dev/urandom
  }
#endif
  int flags = O_RDONLY;
  #if defined(O_CLOEXEC)
  flags |= O_CLOEXEC;
  #endif
  int fd = open("/dev/urandom", flags, 0);
  if (fd < 0) return false;
  size_t count = 0;
  while(count < buf_len) {
    ssize_t ret = read(fd, (char*)buf + count, buf_len - count);
    if (ret<=0) {
      if (errno!=EAGAIN && errno!=EINTR) break;
    }
    else {
      count += ret;
    }
  }
  close(fd);
  return (count==buf_len);
}
#else
static bool os_random_buf(void* buf, size_t buf_len) {
  return false;
}
#endif

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach/mach_time.h>
#else
#include <time.h>
#endif

uintptr_t _mi_os_random_weak(uintptr_t extra_seed) {
  uintptr_t x = (uintptr_t)&_mi_os_random_weak ^ extra_seed; // ASLR makes the address random
  
  #if defined(_WIN32)
    LARGE_INTEGER pcount;
    QueryPerformanceCounter(&pcount);
    x ^= (uintptr_t)(pcount.QuadPart);
  #elif defined(__APPLE__)
    x ^= (uintptr_t)mach_absolute_time();
  #else
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    x ^= (uintptr_t)time.tv_sec;
    x ^= (uintptr_t)time.tv_nsec;
  #endif
  // and do a few randomization steps
  uintptr_t max = ((x ^ (x >> 17)) & 0x0F) + 1;
  for (uintptr_t i = 0; i < max; i++) {
    x = _mi_random_shuffle(x);
  }
  mi_assert_internal(x != 0);
  return x;
}

void _mi_random_init(mi_random_ctx_t* ctx) {
  uint8_t key[32];
  if (!os_random_buf(key, sizeof(key))) {
    // if we fail to get random data from the OS, we fall back to a
    // weak random source based on the current time
    #if !defined(__wasi__)
    _mi_warning_message("unable to use secure randomness\n");
    #endif
    uintptr_t x = _mi_os_random_weak(0);
    for (size_t i = 0; i < 8; i++) {  // key is eight 32-bit words.
      x = _mi_random_shuffle(x);
      ((uint32_t*)key)[i] = (uint32_t)x;
    }
  }
  chacha_init(ctx, key, (uintptr_t)ctx /*nonce*/ );
}

/* --------------------------------------------------------
test vectors from <https://tools.ietf.org/html/rfc8439>
----------------------------------------------------------- */
/*
static bool array_equals(uint32_t* x, uint32_t* y, size_t n) {
  for (size_t i = 0; i < n; i++) {
    if (x[i] != y[i]) return false;
  }
  return true;
}
static void chacha_test(void)
{
  uint32_t x[4] = { 0x11111111, 0x01020304, 0x9b8d6f43, 0x01234567 };
  uint32_t x_out[4] = { 0xea2a92f4, 0xcb1cf8ce, 0x4581472e, 0x5881c4bb };
  qround(x, 0, 1, 2, 3);
  mi_assert_internal(array_equals(x, x_out, 4));

  uint32_t y[16] = {
       0x879531e0,  0xc5ecf37d,  0x516461b1,  0xc9a62f8a,
       0x44c20ef3,  0x3390af7f,  0xd9fc690b,  0x2a5f714c,
       0x53372767,  0xb00a5631,  0x974c541a,  0x359e9963,
       0x5c971061,  0x3d631689,  0x2098d9d6,  0x91dbd320 };
  uint32_t y_out[16] = {
       0x879531e0,  0xc5ecf37d,  0xbdb886dc,  0xc9a62f8a,
       0x44c20ef3,  0x3390af7f,  0xd9fc690b,  0xcfacafd2,
       0xe46bea80,  0xb00a5631,  0x974c541a,  0x359e9963,
       0x5c971061,  0xccc07c79,  0x2098d9d6,  0x91dbd320 };
  qround(y, 2, 7, 8, 13);
  mi_assert_internal(array_equals(y, y_out, 16));

  mi_random_ctx_t r = {
    { 0x61707865, 0x3320646e, 0x79622d32, 0x6b206574,
      0x03020100, 0x07060504, 0x0b0a0908, 0x0f0e0d0c,
      0x13121110, 0x17161514, 0x1b1a1918, 0x1f1e1d1c,
      0x00000001, 0x09000000, 0x4a000000, 0x00000000 },
    {0},
    0
  };
  uint32_t r_out[16] = {
       0xe4e7f110, 0x15593bd1, 0x1fdd0f50, 0xc47120a3,
       0xc7f4d1c7, 0x0368c033, 0x9aaa2204, 0x4e6cd4c3,
       0x466482d2, 0x09aa9f07, 0x05d7c214, 0xa2028bd9,
       0xd19c12b5, 0xb94e16de, 0xe883d0cb, 0x4e3c50a2 };
  chacha_block(&r);
  mi_assert_internal(array_equals(r.output, r_out, 16));
}
*/
