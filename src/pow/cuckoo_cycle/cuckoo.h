// Cuckoo Cycle, a memory-hard proof-of-work
// Copyright (c) 2013-2016 John Tromp

#ifndef BITCOIN_POW_CUCKOO_CYCLE_CUCKOO_H
#define BITCOIN_POW_CUCKOO_CYCLE_CUCKOO_H

#include <stdint.h> // for types uint32_t,uint64_t
#include <string.h> // for functions strlen, memset
#include "siphash.h"
// both cuckoo.c and cuckoo_miner.h need htole32
#ifdef __APPLE__
#include "osx_barrier.h"
#include <machine/endian.h>
#include <libkern/OSByteOrder.h>
#define htole32(x) OSSwapHostToLittleInt32(x)
#else
#include <endian.h>
#endif

namespace powa {

namespace cuckoo_cycle {

// proof-of-work parameters
#ifndef SIZESHIFT
// the main parameter is the 2log of the graph size,
// which is the size in bits of the node identifiers
#define SIZESHIFT 20
#endif
#ifndef PROOFSIZE
// the next most important parameter is (even) length
// of the cycle to be found. a minimum of 12 is recommended
#define PROOFSIZE 42
#endif

// the graph size / number of nodes
static const u64 SIZE = 1ULL<<SIZESHIFT;
// number of nodes in one partition (eg. all even nodes)
static const u64 HALFSIZE = SIZE/2;
// used to mask siphash output
static const u64 NODEMASK = HALFSIZE-1;

// generate edge endpoint in cuckoo graph without partition bit
u64 _sipnode(siphash_keys *keys, u64 nonce, u32 uorv) {
  return siphash24(keys, 2*nonce + uorv) & NODEMASK;
}

// generate edge endpoint in cuckoo graph
u64 sipnode(siphash_keys *keys, u64 nonce, u32 uorv) {
  return _sipnode(keys, nonce, uorv) << 1 | uorv;
}

enum verify_code { POW_OK, POW_HEADER_LENGTH, POW_TOO_BIG, POW_TOO_SMALL, POW_NON_MATCHING, POW_BRANCH, POW_DEAD_END, POW_SHORT_CYCLE};
const char *errstr[] = { "OK", "wrong header length", "proof too big", "proof too small", "endpoints don't match up", "branch in cycle", "cycle dead ends", "cycle too short"};

// verify that nonces are ascending and form a cycle in header-generated graph
int verify(u32 nonces[PROOFSIZE], const char *headernonce, const u32 headerlen) {
  if (headerlen != HEADERLEN)
    return POW_HEADER_LENGTH;
  siphash_keys keys;
  setheader(&keys, headernonce);
  u32 uvs[2*PROOFSIZE];
  u32 xor0=0,xor1=0;
  for (u32 n = 0; n < PROOFSIZE; n++) {
    if (nonces[n] >= HALFSIZE)
      return POW_TOO_BIG;
    if (n && nonces[n] <= nonces[n-1])
      return POW_TOO_SMALL;
    xor0 ^= uvs[2*n  ] = sipnode(&keys, nonces[n], 0);
    xor1 ^= uvs[2*n+1] = sipnode(&keys, nonces[n], 1);
  }
  if (xor0|xor1)                        // matching endpoints imply zero xors
    return POW_NON_MATCHING;
  u32 n = 0, i = 0, j;
  do {                        // follow cycle
    for (u32 k = j = i; (k = (k+2) % (2*PROOFSIZE)) != i; ) {
      if (uvs[k] == uvs[i]) { // find other edge endpoint identical to one at i
        if (j != i)           // already found one before
          return POW_BRANCH;
        j = k;
      }
    } if (j == i) return POW_DEAD_END;  // no matching endpoint
    i = j^1;
    n++;
  } while (i != 0);           // must cycle back to start or we would have found branch
  return n == PROOFSIZE ? POW_OK : POW_SHORT_CYCLE;
}

}  // namespace cuckoo_cycle

}  // namespace powa

#endif  // BITCOIN_POW_CUCKOO_CYCLE_CUCKOO_H
