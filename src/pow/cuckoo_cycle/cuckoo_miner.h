// Cuckoo Cycle, a memory-hard proof-of-work
// Copyright (c) 2013-2016 John Tromp
// The edge-trimming memory optimization is due to Dave Andersen
// http://da-data.blogspot.com/2014/03/a-public-review-of-cuckoo-cycle.html
// The use of prefetching was suggested by Alexander Peslyak (aka Solar Designer)
// define SINGLECYCLING to run cycle finding single threaded which runs slower
// but avoids losing cycles to race conditions (not worth it in my testing)

#ifndef BITCOIN_POW_CUCKOO_CYCLE_CUCKOO_MINER_H
#define BITCOIN_POW_CUCKOO_CYCLE_CUCKOO_MINER_H

#include "cuckoo.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <set>

#ifdef ATOMIC
#include <atomic>
#endif

namespace powa {

namespace cuckoo_cycle {

#ifdef ATOMIC
typedef std::atomic<u32> au32;
typedef std::atomic<u64> au64;
#else
typedef u32 au32;
typedef u64 au64;
#endif

#ifndef SIZEOF_TWICE_ATOM
#define SIZEOF_TWICE_ATOM 4
#endif
#if SIZEOF_TWICE_ATOM == 8
typedef au64 atwice;
typedef u64 uatwice;
#elif SIZEOF_TWICE_ATOM == 4
typedef au32 atwice;
typedef u32 uatwice;
#elif SIZEOF_TWICE_ATOM == 1
typedef unsigned char atwice;
typedef unsigned char uatwice;
#else
#error not implemented
#endif

#if SIZESHIFT <= 32
typedef u32 nonce_t;
typedef u32 node_t;
#else
typedef u64 nonce_t;
typedef u64 node_t;
#endif

// algorithm/performance parameters
#ifndef PART_BITS
// #bits used to partition edge set processing to save memory
// a value of 0 does no partitioning and is fastest
// a value of 1 partitions in two, making twice_set the
// same size as shrinkingset at about 33% slowdown
// higher values are not that interesting
#define PART_BITS 0
#endif

#ifndef NPREFETCH
// how many prefetches to queue up
// before accessing the memory
// must be a multiple of NSIPHASH
#define NPREFETCH 32
#endif

#ifndef IDXSHIFT
// we want sizeof(cuckoo_hash) == sizeof(twice_set), so
// CUCKOO_SIZE * sizeof(u64)   == 2 * ONCE_BITS / 32
// CUCKOO_SIZE * 2             == 2 * ONCE_BITS / 32
// (SIZE >> IDXSHIFT) * 2      == 2 * ONCE_BITS / 32
// SIZE >> IDXSHIFT            == HALFSIZE >> PART_BITS >> 5
// IDXSHIFT                    == 1 + PART_BITS + 5
#define IDXSHIFT (PART_BITS + 6)
#endif
// grow with cube root of size, hardly affected by trimming
const static u32 MAXCCPATHLEN = 8 << (SIZESHIFT/3);

const static u32 PART_MASK = (1 << PART_BITS) - 1;
const static u64 ONCE_BITS = HALFSIZE >> PART_BITS;
const static u64 TWICE_BYTES = (2 * ONCE_BITS) / 8;
const static u64 TWICE_ATOMS = TWICE_BYTES / sizeof(atwice);
const static u32 TWICE_PER_ATOM = sizeof(atwice) * 4;

class twice_set {
public:
  atwice *bits;

  twice_set() {
    bits = (atwice *)calloc(TWICE_ATOMS, sizeof(atwice));
    assert(bits != 0);
  }
  void clear() {
    assert(bits);
    memset(bits, 0, TWICE_ATOMS*sizeof(atwice));
  }
 void prefetch(node_t u) const {
#ifdef PREFETCH
    __builtin_prefetch((const void *)(&bits[u/TWICE_PER_ATOM]), /*READ=*/0, /*TEMPORAL=*/0);
#endif
  }
  void set(node_t u) {
    node_t idx = u/TWICE_PER_ATOM;
    uatwice bit = (uatwice)1 << (2 * (u%TWICE_PER_ATOM));
#ifdef ATOMIC
    uatwice old = std::atomic_fetch_or_explicit(&bits[idx], bit, std::memory_order_relaxed);
    if (old & bit) std::atomic_fetch_or_explicit(&bits[idx], bit<<1, std::memory_order_relaxed);
#else
    uatwice old = bits[idx];
    bits[idx] = old | (bit + (old & bit));
#endif
  }
  bool test(node_t u) const {
#ifdef ATOMIC
    return ((bits[u/TWICE_PER_ATOM].load(std::memory_order_relaxed)
            >> (2 * (u%TWICE_PER_ATOM))) & 2) != 0;
#else
    return (bits[u/TWICE_PER_ATOM] >> (2 * (u%TWICE_PER_ATOM)) & 2) != 0;
#endif
  }
  ~twice_set() {
    free(bits);
  }
};

// set that starts out full and gets reset by threads on disjoint words
class shrinkingset {
public:
  u64 *bits;
  u64 *cnt;
  u32 nthreads;

  shrinkingset(const u32 nt) {
    bits = (u64 *)malloc(HALFSIZE/8);
    cnt  = (u64 *)malloc(nt * sizeof(u64));
    nthreads = nt;
  }
  void clear() {
    memset(bits, 0, HALFSIZE/8);
    memset(cnt, 0, nthreads * sizeof(u64));
    cnt[0] = HALFSIZE;
  }
  u64 count() const {
    u64 sum = 0LL;
    for (u32 i=0; i<nthreads; i++)
      sum += cnt[i];
    return sum;
  }
  void reset(nonce_t n, u32 thread) {
    bits[n/64] |= 1LL << (n%64);
    cnt[thread]--;
  }
  bool test(node_t n) const {
    return !((bits[n/64] >> (n%64)) & 1LL);
  }
  u64 block(node_t n) const {
    return ~bits[n/64];
  }
};

const static u64 CUCKOO_SIZE = SIZE >> IDXSHIFT;
const static u64 CUCKOO_MASK = CUCKOO_SIZE - 1;
// number of (least significant) key bits that survives leftshift by SIZESHIFT
const static u32 KEYBITS = 64-SIZESHIFT;
const static u64 KEYMASK = (1LL << KEYBITS) - 1;
const static u64 MAXDRIFT = 1LL << (KEYBITS - IDXSHIFT);

class cuckoo_hash {
public:
  au64 *cuckoo;

  cuckoo_hash(void *recycle) {
    cuckoo = (au64 *)recycle;
    memset(cuckoo, 0, CUCKOO_SIZE*sizeof(au64));
  }
  void set(node_t u, node_t v) {
    u64 niew = (u64)u << SIZESHIFT | v;
    for (node_t ui = u >> IDXSHIFT; ; ui = (ui+1) & CUCKOO_MASK) {
#if !defined(SINGLECYCLING) && defined(ATOMIC)
      u64 old = 0;
      if (cuckoo[ui].compare_exchange_strong(old, niew, std::memory_order_relaxed))
        return;
      if ((old >> SIZESHIFT) == (u & KEYMASK)) {
        cuckoo[ui].store(niew, std::memory_order_relaxed);
        return;
      }
#else
      u64 old = cuckoo[ui];
      if (old == 0 || (old >> SIZESHIFT) == (u & KEYMASK)) {
        cuckoo[ui] = niew;
        return;
      }
#endif
    }
  }
  node_t operator[](node_t u) const {
    for (node_t ui = u >> IDXSHIFT; ; ui = (ui+1) & CUCKOO_MASK) {
#if !defined(SINGLECYCLING) && defined(ATOMIC)
      u64 cu = cuckoo[ui].load(std::memory_order_relaxed);
#else
      u64 cu = cuckoo[ui];
#endif
      if (!cu)
        return 0;
      if ((cu >> SIZESHIFT) == (u & KEYMASK)) {
        assert(((ui - (u >> IDXSHIFT)) & CUCKOO_MASK) < MAXDRIFT);
        return (node_t)(cu & (SIZE-1));
      }
    }
  }
};

class cuckoo_ctx {
public:
  siphash_keys sip_keys;
  shrinkingset *alive;
  twice_set *nonleaf;
  cuckoo_hash *cuckoo;
  nonce_t **sols;
  u32 maxsols;
  au32 nsols;
  u32 nthreads;
  u32 ntrims;
  uint16_t proofsize_min;
  uint16_t proofsize_max;
  pthread_barrier_t barry;

  cuckoo_ctx(u32 n_threads, u32 n_trims, u32 max_sols, uint16_t proofsize_min_in, uint16_t proofsize_max_in) {
    proofsize_min = proofsize_min_in;
    proofsize_max = proofsize_max_in;
    nthreads = n_threads;
    alive = new shrinkingset(nthreads);
    cuckoo = 0;
    nonleaf = new twice_set;
    ntrims = n_trims;
    int err = pthread_barrier_init(&barry, NULL, nthreads);
    assert(err == 0);
    sols = (nonce_t**)calloc(maxsols = max_sols, sizeof(nonce_t*));
    for (u32 i = 0; i < max_sols; i++) sols[i] = (nonce_t*)calloc(proofsize_max + 1, sizeof(nonce_t));
    assert(sols != 0);
    nsols = 0;
  }
  void setheadernonce(char* headernonce, const u32 len, const u32 nonce) {
    ((u32 *)headernonce)[len/sizeof(u32)-1] = htole32(nonce); // place nonce at end
    setheader(&sip_keys, headernonce);
    alive->clear(); // set all edges to be alive
    nsols = 0;
  }
  ~cuckoo_ctx() {
    delete alive;
    delete nonleaf;
    delete cuckoo;
    for (u32 i = 0; i < maxsols; i++) free(sols[i]);
    free(sols);
  }
  void prefetch(const u64 *hashes, const u32 part) const {
    for (u32 i=0; i < NSIPHASH; i++) {
      u32 u = hashes[i] & NODEMASK;
      if ((u & PART_MASK) == part) {
        nonleaf->prefetch(u >> PART_MASK);
      }
    }
  }
  void node_deg(const u64 *hashes, const u32 nsiphash, const u32 part) const {
    for (u32 i=0; i < nsiphash; i++) {
      u32 u = hashes[i] & NODEMASK;
      if ((u & PART_MASK) == part) {
        nonleaf->set(u >>= PART_BITS);
      }
    }
  }
  void count_node_deg(const u32 id, const u32 uorv, const u32 part) {
    alignas(64) u64 indices[NSIPHASH];
    alignas(64) u64 hashes[NPREFETCH];

    memset(hashes, 0, NPREFETCH * sizeof(u64)); // allow many nonleaf->set(0) to reduce branching
    u32 nidx = 0;
    for (nonce_t block = id*64; block < HALFSIZE; block += nthreads*64) {
      u64 alive64 = alive->block(block);
      for (nonce_t nonce = block-1; alive64; ) { // -1 compensates for 1-based ffs
        u32 ffs = __builtin_ffsll(alive64);
        nonce += ffs; alive64 >>= ffs;
        indices[nidx++ % NSIPHASH] = 2*nonce + uorv;
        if (nidx % NSIPHASH == 0) {
          node_deg(hashes+nidx-NSIPHASH, NSIPHASH, part);
          siphash24xN(&sip_keys, indices, hashes+nidx-NSIPHASH);
          prefetch(hashes+nidx-NSIPHASH, part);
          nidx %= NPREFETCH;
        }
        if (ffs & 64) break; // can't shift by 64
      }
    }
    node_deg(hashes, NPREFETCH, part);
    if (nidx % NSIPHASH != 0) {
      siphash24xN(&sip_keys, indices, hashes+(nidx&-NSIPHASH));
      node_deg(hashes+(nidx&-NSIPHASH), nidx%NSIPHASH, part);
    }
  }
  void kill(const u64 *hashes, const u64 *indices, const u32 nsiphash,
             const u32 part, const u32 id) const {
    for (u32 i=0; i < nsiphash; i++) {
      u32 u = hashes[i] & NODEMASK;
      if ((u & PART_MASK) == part && !nonleaf->test(u >> PART_BITS)) {
        alive->reset(indices[i]/2, id);
      }
    }
  }
  void kill_leaf_edges(const u32 id, const u32 uorv, const u32 part) {
    alignas(64) u64 indices[NPREFETCH];
    alignas(64) u64 hashes[NPREFETCH];

    memset(hashes, 0, NPREFETCH * sizeof(u64)); // allow many nonleaf->test(0) to reduce branching
    u32 nidx = 0;
    for (nonce_t block = id*64; block < HALFSIZE; block += nthreads*64) {
      u64 alive64 = alive->block(block);
      for (nonce_t nonce = block-1; alive64; ) { // -1 compensates for 1-based ffs
        u32 ffs = __builtin_ffsll(alive64);
        nonce += ffs; alive64 >>= ffs;
        indices[nidx++] = 2*nonce + uorv;
        if (nidx % NSIPHASH == 0) {
          siphash24xN(&sip_keys, indices+nidx-NSIPHASH, hashes+nidx-NSIPHASH);
          prefetch(hashes+nidx-NSIPHASH, part);
          nidx %= NPREFETCH;
          kill(hashes+nidx, indices+nidx, NSIPHASH, part, id);
        }
        if (ffs & 64) break; // can't shift by 64
      }
    }
    const u32 pnsip = nidx & -NSIPHASH;
    if (pnsip != nidx) {
      siphash24xN(&sip_keys, indices+pnsip, hashes+pnsip);
    }
    kill(hashes, indices, nidx, part, id);
    const u32 nnsip = pnsip + NSIPHASH;
    kill(hashes+nnsip, indices+nnsip, NPREFETCH-nnsip, part, id);
  }
  void solution(node_t *us, u32 nu, node_t *vs, u32 nv, uint16_t proofsize) {
    typedef std::pair<node_t,node_t> edge;
    std::set<edge> cycle;
    u32 n = 0;
    cycle.insert(edge(*us, *vs));
    while (nu--)
      cycle.insert(edge(us[(nu+1)&~1], us[nu|1])); // u's in even position; v's in odd
    while (nv--)
      cycle.insert(edge(vs[nv|1], vs[(nv+1)&~1])); // u's in odd position; v's in even
  #ifdef ATOMIC
    u32 soli = std::atomic_fetch_add_explicit(&nsols, 1U, std::memory_order_relaxed);
  #else
    u32 soli = nsols++;
  #endif
    for (nonce_t block = 0; block < HALFSIZE; block += 64) {
      u64 alive64 = alive->block(block);
      for (nonce_t nonce = block-1; alive64; ) { // -1 compensates for 1-based ffs
        u32 ffs = __builtin_ffsll(alive64);
        nonce += ffs; alive64 >>= ffs;
        edge e(sipnode(&sip_keys, nonce, 0), sipnode(&sip_keys, nonce, 1));
        if (cycle.find(e) != cycle.end()) {
          sols[soli][n++] = nonce;
  #ifdef SHOWSOL
          printf("e(%x)=(%x,%x)%c", nonce, e.first, e.second, n == proofsize?'\n':' ');
  #endif
          if (proofsize_min > 2)
            cycle.erase(e);
        }
        if (ffs & 64) break; // can't shift by 64
      }
    }
    assert(n == proofsize);
    sols[soli][proofsize_max] = n; // store actual proofsize as last entry in solution
  }
};

typedef struct {
  u32 id;
  pthread_t thread;
  cuckoo_ctx *ctx;
} thread_ctx;

inline void barrier(pthread_barrier_t *barry) {
  int rc = pthread_barrier_wait(barry);
  if (rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
    printf("Could not wait on barrier\n");
    pthread_exit(NULL);
  }
}

inline u32 path(cuckoo_hash &cuckoo, node_t u, node_t *us) {
  u32 nu;
  for (nu = 0; u; u = cuckoo[u]) {
    if (nu >= MAXCCPATHLEN) {
      while (nu-- && us[nu] != u) ;
      // if (!~nu)
      //   printf("maximum path length exceeded\n");
      // else printf("illegal %4d-cycle\n", MAXCCPATHLEN-nu);
      pthread_exit(NULL);
    }
    us[nu++] = u;
  }
  return nu-1;
}

inline void *worker(void *vp) {
  thread_ctx *tp = (thread_ctx *)vp;
  cuckoo_ctx *ctx = tp->ctx;

  shrinkingset *alive = ctx->alive;
  u32 load = 100LL * HALFSIZE / CUCKOO_SIZE;
  // if (tp->id == 0)
  //   printf("initial load %d%%\n", load);
  for (u32 round=1; round <= ctx->ntrims; round++) {
    // if (tp->id == 0) printf("round %2d partition loads", round);
    for (u32 uorv = 0; uorv < 2; uorv++) {
      for (u32 part = 0; part <= PART_MASK; part++) {
        if (tp->id == 0)
          ctx->nonleaf->clear(); // clear all counts
        barrier(&ctx->barry);
        ctx->count_node_deg(tp->id,uorv,part);
        barrier(&ctx->barry);
        ctx->kill_leaf_edges(tp->id,uorv,part);
        barrier(&ctx->barry);
        // if (tp->id == 0) {
        //   u32 load = (u32)(100LL * alive->count() / CUCKOO_SIZE);
        //   printf(" %c%d %4d%%", "UV"[uorv], part, load);
        // }
      }
    }
    // if (tp->id == 0) printf("\n");
  }
  if (tp->id == 0) {
    load = (u32)(100LL * alive->count() / CUCKOO_SIZE);
    // printf("nonce %d: %d trims completed  final load %d%%\n", ctx->nonce, ctx->ntrims, load);
    if (load >= 90) {
      printf("overloaded! exiting...");
      pthread_exit(NULL);
    }
    ctx->cuckoo = new cuckoo_hash(ctx->nonleaf->bits);
  }
#ifdef SINGLECYCLING
  else pthread_exit(NULL);
#else
  barrier(&ctx->barry);
#endif
  cuckoo_hash &cuckoo = *ctx->cuckoo;
  node_t us[MAXCCPATHLEN], vs[MAXCCPATHLEN];
#ifdef SINGLECYCLING
  for (nonce_t block = 0; block < HALFSIZE; block += 64) {
#else
  for (nonce_t block = tp->id*64; block < HALFSIZE; block += ctx->nthreads*64) {
#endif
    u64 alive64 = alive->block(block);
    for (nonce_t nonce = block-1; alive64; ) { // -1 compensates for 1-based ffs
      u32 ffs = __builtin_ffsll(alive64);
      nonce += ffs; alive64 >>= ffs;
      node_t u0=sipnode(&ctx->sip_keys, nonce, 0), v0=sipnode(&ctx->sip_keys, nonce, 1);
      if (u0) {// ignore vertex 0 so it can be used as nil for cuckoo[]
        u32 nu = path(cuckoo, u0, us), nv = path(cuckoo, v0, vs);
        if (us[nu] == vs[nv]) {
          u32 min = nu < nv ? nu : nv;
          for (nu -= min, nv -= min; us[nu] != vs[nv]; nu++, nv++) ;
          u32 len = nu + nv + 1;
          // printf("%4d-cycle found at %d:%d%%\n", len, tp->id, (u32)(nonce*100LL/HALFSIZE));
          if (len >= ctx->proofsize_min && len <= ctx->proofsize_max && ctx->nsols < ctx->maxsols)
            ctx->solution(us, nu, vs, nv, len);
        } else if (nu < nv) {
          while (nu--)
            cuckoo.set(us[nu+1], us[nu]);
          cuckoo.set(u0, v0);
        } else {
          while (nv--)
            cuckoo.set(vs[nv+1], vs[nv]);
          cuckoo.set(v0, u0);
        }
      }
      if (ffs & 64) break; // can't shift by 64
    }
  }
  pthread_exit(NULL);
  return 0;
}

}  // namespace cuckoo_cycle

}  // namespace powa

#endif  // BITCOIN_POW_CUCKOO_CYCLE_CUCKOO_MINER_H
