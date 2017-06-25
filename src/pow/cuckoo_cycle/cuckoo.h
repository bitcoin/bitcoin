// Cuckoo Cycle, a memory-hard proof-of-work
// Copyright (c) 2013-2016 John Tromp

#ifndef BITCOIN_POW_CUCKOO_CYCLE_CUCKOO_H
#define BITCOIN_POW_CUCKOO_CYCLE_CUCKOO_H

#include <stdint.h> // for types uint32_t,uint64_t
#include <string.h> // for functions strlen, memset
#include "siphash.h"
#include "compat/endian.h"

#ifdef __APPLE__
#include "osx_barrier.h"
#endif

namespace powa {

namespace cuckoo_cycle {

// proof-of-work parameters
#ifndef SIZESHIFT
// the main parameter is the 2log of the graph size,
// which is the size in bits of the node identifiers
#define SIZESHIFT 28
#endif

// the graph size / number of nodes
static const u64 SIZE = 1ULL<<SIZESHIFT;
// number of nodes in one partition (eg. all even nodes)
static const u64 HALFSIZE = SIZE/2;
// used to mask siphash output
static const u64 NODEMASK = HALFSIZE-1;

// generate edge endpoint in cuckoo graph without partition bit
inline u64 _sipnode(siphash_keys *keys, u64 nonce, u32 uorv) {
  return siphash24(keys, 2*nonce + uorv) & NODEMASK;
}

// generate edge endpoint in cuckoo graph
inline u64 sipnode(siphash_keys *keys, u64 nonce, u32 uorv) {
  return _sipnode(keys, nonce, uorv) << 1 | uorv;
}

enum verify_code { POW_OK, POW_HEADER_LENGTH, POW_TOO_BIG, POW_TOO_SMALL, POW_NON_MATCHING, POW_BRANCH, POW_DEAD_END, POW_SHORT_CYCLE};
static const char *errstr[] = { "OK", "wrong header length", "proof too big", "proof too small", "endpoints don't match up", "branch in cycle", "cycle dead ends", "cycle too short"};

// verify that nonces are ascending and form a cycle in header-generated graph
inline int verify(u32* nonces, const char *headernonce, const u32 headerlen, const uint16_t proofsize) {
  if (headerlen != HEADERLEN)
    return POW_HEADER_LENGTH;
  siphash_keys keys;
  setheader(&keys, headernonce);
  u32* uvs = new u32[2*proofsize];
  u32 xor0=0,xor1=0;
  int err = POW_OK;
  for (u32 n = 0; err == POW_OK && n < proofsize; n++) {
    if (nonces[n] >= HALFSIZE) {
      err = POW_TOO_BIG;
    } else if (n && nonces[n] <= nonces[n-1]) {
      err = POW_TOO_SMALL;
    } else {
      xor0 ^= uvs[2*n  ] = sipnode(&keys, nonces[n], 0);
      xor1 ^= uvs[2*n+1] = sipnode(&keys, nonces[n], 1);
    }
  }
  if (err != POW_OK) {
    delete [] uvs;
    return err;
  }
  if (xor0|xor1) {                      // matching endpoints imply zero xors
    delete [] uvs;
    return POW_NON_MATCHING;
  }
  u32 n = 0, i = 0, j;
  do {                        // follow cycle
    for (u32 k = j = i; (k = (k+2) % (2*proofsize)) != i; ) {
      if (uvs[k] == uvs[i]) { // find other edge endpoint identical to one at i
        if (j != i) {          // already found one before
          delete [] uvs;
          return POW_BRANCH;
        }
        j = k;
      }
    }
    if (j == i) {
      // no matching endpoint
      delete [] uvs;
      return POW_DEAD_END;
    }
    i = j^1;
    n++;
  } while (i != 0);           // must cycle back to start or we would have found branch
  delete [] uvs;
  return n == proofsize ? POW_OK : POW_SHORT_CYCLE;
}

}  // namespace cuckoo_cycle

}  // namespace powa

#endif  // BITCOIN_POW_CUCKOO_CYCLE_CUCKOO_H
