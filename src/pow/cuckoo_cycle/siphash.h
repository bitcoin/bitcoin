#ifndef BITCOIN_POW_CUCKOO_CYCLE_SIPHASH_H
#define BITCOIN_POW_CUCKOO_CYCLE_SIPHASH_H
#include <stdint.h> // for types uint32_t,uint64_t
#include "hash.h"

namespace powa {

namespace cuckoo_cycle {

// length of header hashed into siphash key
#ifndef HEADERLEN
#define HEADERLEN 80
#endif

// save some keystrokes since i'm a lazy typer
typedef uint32_t u32;
typedef uint64_t u64;

// siphash uses a pair of 64-bit keys,
typedef struct {
  u64 k0;
  u64 k1;
} siphash_keys;

#define U8TO64_LE(p) \
  (((u64)((p)[0])      ) | ((u64)((p)[1]) <<  8) | \
   ((u64)((p)[2]) << 16) | ((u64)((p)[3]) << 24) | \
   ((u64)((p)[4]) << 32) | ((u64)((p)[5]) << 40) | \
   ((u64)((p)[6]) << 48) | ((u64)((p)[7]) << 56))

#ifndef SHA256
#define SHA256(d, n, md) CSHA256().Write(d, n).Finalize(md)
#endif

// derive siphash key from fixed length header
inline void setheader(siphash_keys *keys, const char *header) {
  unsigned char hdrkey[32];
  SHA256((unsigned char *)header, HEADERLEN, hdrkey);
  keys->k0 = U8TO64_LE(hdrkey);
  keys->k1 = U8TO64_LE(hdrkey+8);
}

#define ROTL(x,b) (u64)( ((x) << (b)) | ( (x) >> (64 - (b))) )
#define SIPROUND \
  do { \
    v0 += v1; v2 += v3; v1 = ROTL(v1,13); \
    v3 = ROTL(v3,16); v1 ^= v0; v3 ^= v2; \
    v0 = ROTL(v0,32); v2 += v1; v0 += v3; \
    v1 = ROTL(v1,17);   v3 = ROTL(v3,21); \
    v1 ^= v2; v3 ^= v0; v2 = ROTL(v2,32); \
  } while(0)

// SipHash-2-4 specialized to precomputed key and 8 byte nonces
inline u64 siphash24(const siphash_keys *keys, const u64 nonce) {
  u64 v0 = keys->k0 ^ 0x736f6d6570736575ULL, v1 = keys->k1 ^ 0x646f72616e646f6dULL,
      v2 = keys->k0 ^ 0x6c7967656e657261ULL, v3 = keys->k1 ^ 0x7465646279746573ULL ^ nonce;
  SIPROUND; SIPROUND;
  v0 ^= nonce;
  v2 ^= 0xff;
  SIPROUND; SIPROUND; SIPROUND; SIPROUND;
  return (v0 ^ v1) ^ (v2  ^ v3);
}

#define NSIPHASH 1

inline void siphash24xN(const siphash_keys *keys, const u64 *indices, u64 * hashes) {
  *hashes = siphash24(keys, *indices);
}

}  // namespace cuckoo_cycle

}  // namespace powa

#endif // BITCOIN_POW_CUCKOO_CYCLE_SIPHASH_H
