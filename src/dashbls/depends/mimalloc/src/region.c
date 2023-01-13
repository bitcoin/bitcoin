/* ----------------------------------------------------------------------------
Copyright (c) 2019-2020, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/

/* ----------------------------------------------------------------------------
This implements a layer between the raw OS memory (VirtualAlloc/mmap/sbrk/..)
and the segment and huge object allocation by mimalloc. There may be multiple
implementations of this (one could be the identity going directly to the OS,
another could be a simple cache etc), but the current one uses large "regions".
In contrast to the rest of mimalloc, the "regions" are shared between threads and
need to be accessed using atomic operations.
We need this memory layer between the raw OS calls because of:
1. on `sbrk` like systems (like WebAssembly) we need our own memory maps in order
   to reuse memory effectively.
2. It turns out that for large objects, between 1MiB and 32MiB (?), the cost of
   an OS allocation/free is still (much) too expensive relative to the accesses 
   in that object :-( (`malloc-large` tests this). This means we need a cheaper 
   way to reuse memory.
3. This layer allows for NUMA aware allocation.

Possible issues:
- (2) can potentially be addressed too with a small cache per thread which is much
  simpler. Generally though that requires shrinking of huge pages, and may overuse
  memory per thread. (and is not compatible with `sbrk`).
- Since the current regions are per-process, we need atomic operations to
  claim blocks which may be contended
- In the worst case, we need to search the whole region map (16KiB for 256GiB)
  linearly. At what point will direct OS calls be faster? Is there a way to
  do this better without adding too much complexity?
-----------------------------------------------------------------------------*/
#include "mimalloc.h"
#include "mimalloc-internal.h"
#include "mimalloc-atomic.h"

#include <string.h>  // memset

#include "bitmap.h"

// Internal raw OS interface
size_t  _mi_os_large_page_size(void);
bool    _mi_os_protect(void* addr, size_t size);
bool    _mi_os_unprotect(void* addr, size_t size);
bool    _mi_os_commit(void* p, size_t size, bool* is_zero, mi_stats_t* stats); // NOLINT(readability-redundant-declaration)
bool    _mi_os_decommit(void* p, size_t size, mi_stats_t* stats); // NOLINT(readability-redundant-declaration)
bool    _mi_os_reset(void* p, size_t size, mi_stats_t* stats);
bool    _mi_os_unreset(void* p, size_t size, bool* is_zero, mi_stats_t* stats);

// arena.c
mi_arena_id_t _mi_arena_id_none(void);
void    _mi_arena_free(void* p, size_t size, size_t memid, bool all_committed, mi_stats_t* stats);
void*   _mi_arena_alloc(size_t size, bool* commit, bool* large, bool* is_pinned, bool* is_zero, mi_arena_id_t req_arena_id, size_t* memid, mi_os_tld_t* tld);
void*   _mi_arena_alloc_aligned(size_t size, size_t alignment, bool* commit, bool* large, bool* is_pinned, bool* is_zero, mi_arena_id_t req_arena_id, size_t* memid, mi_os_tld_t* tld);



// Constants
#if (MI_INTPTR_SIZE==8)
#define MI_HEAP_REGION_MAX_SIZE    (256 * MI_GiB)  // 64KiB for the region map 
#elif (MI_INTPTR_SIZE==4)
#define MI_HEAP_REGION_MAX_SIZE    (3 * MI_GiB)    // ~ KiB for the region map
#else
#error "define the maximum heap space allowed for regions on this platform"
#endif

#define MI_SEGMENT_ALIGN          MI_SEGMENT_SIZE

#define MI_REGION_MAX_BLOCKS      MI_BITMAP_FIELD_BITS
#define MI_REGION_SIZE            (MI_SEGMENT_SIZE * MI_BITMAP_FIELD_BITS)    // 256MiB  (64MiB on 32 bits)
#define MI_REGION_MAX             (MI_HEAP_REGION_MAX_SIZE / MI_REGION_SIZE)  // 1024  (48 on 32 bits)
#define MI_REGION_MAX_OBJ_BLOCKS  (MI_REGION_MAX_BLOCKS/4)                    // 64MiB
#define MI_REGION_MAX_OBJ_SIZE    (MI_REGION_MAX_OBJ_BLOCKS*MI_SEGMENT_SIZE)  

// Region info 
typedef union mi_region_info_u {
  size_t value;      
  struct {
    bool  valid;        // initialized?
    bool  is_large:1;   // allocated in fixed large/huge OS pages
    bool  is_pinned:1;  // pinned memory cannot be decommitted
    short numa_node;    // the associated NUMA node (where -1 means no associated node)
  } x;
} mi_region_info_t;


// A region owns a chunk of REGION_SIZE (256MiB) (virtual) memory with
// a bit map with one bit per MI_SEGMENT_SIZE (4MiB) block.
typedef struct mem_region_s {
  _Atomic(size_t)           info;        // mi_region_info_t.value
  _Atomic(void*)            start;       // start of the memory area 
  mi_bitmap_field_t         in_use;      // bit per in-use block
  mi_bitmap_field_t         dirty;       // track if non-zero per block
  mi_bitmap_field_t         commit;      // track if committed per block
  mi_bitmap_field_t         reset;       // track if reset per block
  _Atomic(size_t)           arena_memid; // if allocated from a (huge page) arena
  _Atomic(size_t)           padding;     // round to 8 fields (needs to be atomic for msvc, see issue #508)
} mem_region_t;

// The region map
static mem_region_t regions[MI_REGION_MAX];

// Allocated regions
static _Atomic(size_t) regions_count; // = 0;        


/* ----------------------------------------------------------------------------
Utility functions
-----------------------------------------------------------------------------*/

// Blocks (of 4MiB) needed for the given size.
static size_t mi_region_block_count(size_t size) {
  return _mi_divide_up(size, MI_SEGMENT_SIZE);
}

/*
// Return a rounded commit/reset size such that we don't fragment large OS pages into small ones.
static size_t mi_good_commit_size(size_t size) {
  if (size > (SIZE_MAX - _mi_os_large_page_size())) return size;
  return _mi_align_up(size, _mi_os_large_page_size());
}
*/

// Return if a pointer points into a region reserved by us.
mi_decl_nodiscard bool mi_is_in_heap_region(const void* p) mi_attr_noexcept {
  if (p==NULL) return false;
  size_t count = mi_atomic_load_relaxed(&regions_count);
  for (size_t i = 0; i < count; i++) {
    uint8_t* start = (uint8_t*)mi_atomic_load_ptr_relaxed(uint8_t, &regions[i].start);
    if (start != NULL && (uint8_t*)p >= start && (uint8_t*)p < start + MI_REGION_SIZE) return true;
  }
  return false;
}


static void* mi_region_blocks_start(const mem_region_t* region, mi_bitmap_index_t bit_idx) {
  uint8_t* start = (uint8_t*)mi_atomic_load_ptr_acquire(uint8_t, &((mem_region_t*)region)->start);
  mi_assert_internal(start != NULL);
  return (start + (bit_idx * MI_SEGMENT_SIZE));  
}

static size_t mi_memid_create(mem_region_t* region, mi_bitmap_index_t bit_idx) {
  mi_assert_internal(bit_idx < MI_BITMAP_FIELD_BITS);
  size_t idx = region - regions;
  mi_assert_internal(&regions[idx] == region);
  return (idx*MI_BITMAP_FIELD_BITS + bit_idx)<<1;
}

static size_t mi_memid_create_from_arena(size_t arena_memid) {
  return (arena_memid << 1) | 1;
}


static bool mi_memid_is_arena(size_t id, mem_region_t** region, mi_bitmap_index_t* bit_idx, size_t* arena_memid) {
  if ((id&1)==1) {
    if (arena_memid != NULL) *arena_memid = (id>>1);
    return true;
  }
  else {
    size_t idx = (id >> 1) / MI_BITMAP_FIELD_BITS;
    *bit_idx   = (mi_bitmap_index_t)(id>>1) % MI_BITMAP_FIELD_BITS;
    *region    = &regions[idx];
    return false;
  }
}


/* ----------------------------------------------------------------------------
  Allocate a region is allocated from the OS (or an arena)
-----------------------------------------------------------------------------*/

static bool mi_region_try_alloc_os(size_t blocks, bool commit, bool allow_large, mem_region_t** region, mi_bitmap_index_t* bit_idx, mi_os_tld_t* tld)
{
  // not out of regions yet?
  if (mi_atomic_load_relaxed(&regions_count) >= MI_REGION_MAX - 1) return false;

  // try to allocate a fresh region from the OS
  bool region_commit = (commit && mi_option_is_enabled(mi_option_eager_region_commit));
  bool region_large = (commit && allow_large);
  bool is_zero = false;
  bool is_pinned = false;
  size_t arena_memid = 0;
  void* const start = _mi_arena_alloc_aligned(MI_REGION_SIZE, MI_SEGMENT_ALIGN, &region_commit, &region_large, &is_pinned, &is_zero, _mi_arena_id_none(),  & arena_memid, tld);
  if (start == NULL) return false;
  mi_assert_internal(!(region_large && !allow_large));
  mi_assert_internal(!region_large || region_commit);

  // claim a fresh slot
  const size_t idx = mi_atomic_increment_acq_rel(&regions_count);
  if (idx >= MI_REGION_MAX) {
    mi_atomic_decrement_acq_rel(&regions_count);
    _mi_arena_free(start, MI_REGION_SIZE, arena_memid, region_commit, tld->stats);
    _mi_warning_message("maximum regions used: %zu GiB (perhaps recompile with a larger setting for MI_HEAP_REGION_MAX_SIZE)", _mi_divide_up(MI_HEAP_REGION_MAX_SIZE, MI_GiB));
    return false;
  }

  // allocated, initialize and claim the initial blocks
  mem_region_t* r = &regions[idx];
  r->arena_memid  = arena_memid;
  mi_atomic_store_release(&r->in_use, (size_t)0);
  mi_atomic_store_release(&r->dirty, (is_zero ? 0 : MI_BITMAP_FIELD_FULL));
  mi_atomic_store_release(&r->commit, (region_commit ? MI_BITMAP_FIELD_FULL : 0));
  mi_atomic_store_release(&r->reset, (size_t)0);
  *bit_idx = 0;
  _mi_bitmap_claim(&r->in_use, 1, blocks, *bit_idx, NULL);
  mi_atomic_store_ptr_release(void,&r->start, start);

  // and share it 
  mi_region_info_t info;
  info.value = 0;                        // initialize the full union to zero
  info.x.valid = true;
  info.x.is_large = region_large;
  info.x.is_pinned = is_pinned;
  info.x.numa_node = (short)_mi_os_numa_node(tld);
  mi_atomic_store_release(&r->info, info.value); // now make it available to others
  *region = r;
  return true;
}

/* ----------------------------------------------------------------------------
  Try to claim blocks in suitable regions
-----------------------------------------------------------------------------*/

static bool mi_region_is_suitable(const mem_region_t* region, int numa_node, bool allow_large ) {
  // initialized at all?
  mi_region_info_t info;
  info.value = mi_atomic_load_relaxed(&((mem_region_t*)region)->info);
  if (info.value==0) return false;

  // numa correct
  if (numa_node >= 0) {  // use negative numa node to always succeed
    int rnode = info.x.numa_node;
    if (rnode >= 0 && rnode != numa_node) return false;
  }

  // check allow-large
  if (!allow_large && info.x.is_large) return false;

  return true;
}


static bool mi_region_try_claim(int numa_node, size_t blocks, bool allow_large, mem_region_t** region, mi_bitmap_index_t* bit_idx, mi_os_tld_t* tld)
{
  // try all regions for a free slot  
  const size_t count = mi_atomic_load_relaxed(&regions_count); // monotonic, so ok to be relaxed
  size_t idx = tld->region_idx; // Or start at 0 to reuse low addresses? Starting at 0 seems to increase latency though
  for (size_t visited = 0; visited < count; visited++, idx++) {
    if (idx >= count) idx = 0;  // wrap around
    mem_region_t* r = &regions[idx];
    // if this region suits our demand (numa node matches, large OS page matches)
    if (mi_region_is_suitable(r, numa_node, allow_large)) {
      // then try to atomically claim a segment(s) in this region
      if (_mi_bitmap_try_find_claim_field(&r->in_use, 0, blocks, bit_idx)) {
        tld->region_idx = idx;    // remember the last found position
        *region = r;
        return true;
      }
    }
  }
  return false;
}


static void* mi_region_try_alloc(size_t blocks, bool* commit, bool* large, bool* is_pinned, bool* is_zero, size_t* memid, mi_os_tld_t* tld)
{
  mi_assert_internal(blocks <= MI_BITMAP_FIELD_BITS);
  mem_region_t* region;
  mi_bitmap_index_t bit_idx;
  const int numa_node = (_mi_os_numa_node_count() <= 1 ? -1 : _mi_os_numa_node(tld));
  // try to claim in existing regions
  if (!mi_region_try_claim(numa_node, blocks, *large, &region, &bit_idx, tld)) {
    // otherwise try to allocate a fresh region and claim in there
    if (!mi_region_try_alloc_os(blocks, *commit, *large, &region, &bit_idx, tld)) {
      // out of regions or memory
      return NULL;
    }
  }
  
  // ------------------------------------------------
  // found a region and claimed `blocks` at `bit_idx`, initialize them now
  mi_assert_internal(region != NULL);
  mi_assert_internal(_mi_bitmap_is_claimed(&region->in_use, 1, blocks, bit_idx));

  mi_region_info_t info;
  info.value = mi_atomic_load_acquire(&region->info);
  uint8_t* start = (uint8_t*)mi_atomic_load_ptr_acquire(uint8_t,&region->start);
  mi_assert_internal(!(info.x.is_large && !*large));
  mi_assert_internal(start != NULL);

  *is_zero   = _mi_bitmap_claim(&region->dirty, 1, blocks, bit_idx, NULL);  
  *large     = info.x.is_large;
  *is_pinned = info.x.is_pinned;
  *memid     = mi_memid_create(region, bit_idx);
  void* p = start + (mi_bitmap_index_bit_in_field(bit_idx) * MI_SEGMENT_SIZE);

  // commit
  if (*commit) {
    // ensure commit
    bool any_uncommitted;
    _mi_bitmap_claim(&region->commit, 1, blocks, bit_idx, &any_uncommitted);
    if (any_uncommitted) {
      mi_assert_internal(!info.x.is_large && !info.x.is_pinned);
      bool commit_zero = false;
      if (!_mi_mem_commit(p, blocks * MI_SEGMENT_SIZE, &commit_zero, tld)) {
        // failed to commit! unclaim and return
        mi_bitmap_unclaim(&region->in_use, 1, blocks, bit_idx);
        return NULL;
      }
      if (commit_zero) *is_zero = true;      
    }
  }
  else {
    // no need to commit, but check if already fully committed
    *commit = _mi_bitmap_is_claimed(&region->commit, 1, blocks, bit_idx);
  }  
  mi_assert_internal(!*commit || _mi_bitmap_is_claimed(&region->commit, 1, blocks, bit_idx));

  // unreset reset blocks
  if (_mi_bitmap_is_any_claimed(&region->reset, 1, blocks, bit_idx)) {
    // some blocks are still reset
    mi_assert_internal(!info.x.is_large && !info.x.is_pinned);
    mi_assert_internal(!mi_option_is_enabled(mi_option_eager_commit) || *commit || mi_option_get(mi_option_eager_commit_delay) > 0); 
    mi_bitmap_unclaim(&region->reset, 1, blocks, bit_idx);
    if (*commit || !mi_option_is_enabled(mi_option_reset_decommits)) { // only if needed
      bool reset_zero = false;
      _mi_mem_unreset(p, blocks * MI_SEGMENT_SIZE, &reset_zero, tld);
      if (reset_zero) *is_zero = true;
    }
  }
  mi_assert_internal(!_mi_bitmap_is_any_claimed(&region->reset, 1, blocks, bit_idx));
  
  #if (MI_DEBUG>=2) && !MI_TRACK_ENABLED
  if (*commit) { ((uint8_t*)p)[0] = 0; }
  #endif
  
  // and return the allocation  
  mi_assert_internal(p != NULL);  
  return p;
}


/* ----------------------------------------------------------------------------
 Allocation
-----------------------------------------------------------------------------*/

// Allocate `size` memory aligned at `alignment`. Return non NULL on success, with a given memory `id`.
// (`id` is abstract, but `id = idx*MI_REGION_MAP_BITS + bitidx`)
void* _mi_mem_alloc_aligned(size_t size, size_t alignment, bool* commit, bool* large, bool* is_pinned, bool* is_zero, size_t* memid, mi_os_tld_t* tld)
{
  mi_assert_internal(memid != NULL && tld != NULL);
  mi_assert_internal(size > 0);
  *memid = 0;
  *is_zero = false;
  *is_pinned = false;
  bool default_large = false;
  if (large==NULL) large = &default_large;  // ensure `large != NULL`  
  if (size == 0) return NULL;
  size = _mi_align_up(size, _mi_os_page_size());

  // allocate from regions if possible
  void* p = NULL;
  size_t arena_memid;
  const size_t blocks = mi_region_block_count(size);
  if (blocks <= MI_REGION_MAX_OBJ_BLOCKS && alignment <= MI_SEGMENT_ALIGN) {
    p = mi_region_try_alloc(blocks, commit, large, is_pinned, is_zero, memid, tld);    
    if (p == NULL) {
      _mi_warning_message("unable to allocate from region: size %zu\n", size);
    }
  }
  if (p == NULL) {
    // and otherwise fall back to the OS
    p = _mi_arena_alloc_aligned(size, alignment, commit, large, is_pinned, is_zero, _mi_arena_id_none(),  & arena_memid, tld);
    *memid = mi_memid_create_from_arena(arena_memid);
  }

  if (p != NULL) {
    mi_assert_internal((uintptr_t)p % alignment == 0);
    #if (MI_DEBUG>=2) && !MI_TRACK_ENABLED
    if (*commit) { ((uint8_t*)p)[0] = 0; } // ensure the memory is committed
    #endif
  }
  return p;
}



/* ----------------------------------------------------------------------------
Free
-----------------------------------------------------------------------------*/

// Free previously allocated memory with a given id.
void _mi_mem_free(void* p, size_t size, size_t id, bool full_commit, bool any_reset, mi_os_tld_t* tld) {
  mi_assert_internal(size > 0 && tld != NULL);
  if (p==NULL) return;
  if (size==0) return;
  size = _mi_align_up(size, _mi_os_page_size());

  size_t arena_memid = 0;
  mi_bitmap_index_t bit_idx;
  mem_region_t* region;
  if (mi_memid_is_arena(id,&region,&bit_idx,&arena_memid)) {
   // was a direct arena allocation, pass through
    _mi_arena_free(p, size, arena_memid, full_commit, tld->stats);
  }
  else {
    // allocated in a region
    mi_assert_internal(size <= MI_REGION_MAX_OBJ_SIZE); if (size > MI_REGION_MAX_OBJ_SIZE) return;
    const size_t blocks = mi_region_block_count(size);
    mi_assert_internal(blocks + bit_idx <= MI_BITMAP_FIELD_BITS);
    mi_region_info_t info;
    info.value = mi_atomic_load_acquire(&region->info);
    mi_assert_internal(info.value != 0);
    void* blocks_start = mi_region_blocks_start(region, bit_idx);
    mi_assert_internal(blocks_start == p); // not a pointer in our area?
    mi_assert_internal(bit_idx + blocks <= MI_BITMAP_FIELD_BITS);
    if (blocks_start != p || bit_idx + blocks > MI_BITMAP_FIELD_BITS) return; // or `abort`?

    // committed?
    if (full_commit && (size % MI_SEGMENT_SIZE) == 0) {
      _mi_bitmap_claim(&region->commit, 1, blocks, bit_idx, NULL);
    }

    if (any_reset) {
      // set the is_reset bits if any pages were reset
      _mi_bitmap_claim(&region->reset, 1, blocks, bit_idx, NULL);
    }

    // reset the blocks to reduce the working set.
    if (!info.x.is_large && !info.x.is_pinned && mi_option_is_enabled(mi_option_segment_reset) 
       && (mi_option_is_enabled(mi_option_eager_commit) ||
           mi_option_is_enabled(mi_option_reset_decommits))) // cannot reset halfway committed segments, use only `option_page_reset` instead            
    {
      bool any_unreset;
      _mi_bitmap_claim(&region->reset, 1, blocks, bit_idx, &any_unreset);
      if (any_unreset) {
        _mi_abandoned_await_readers(); // ensure no more pending write (in case reset = decommit)
        _mi_mem_reset(p, blocks * MI_SEGMENT_SIZE, tld);
      }
    }    

    // and unclaim
    bool all_unclaimed = mi_bitmap_unclaim(&region->in_use, 1, blocks, bit_idx);
    mi_assert_internal(all_unclaimed); MI_UNUSED(all_unclaimed);
  }
}


/* ----------------------------------------------------------------------------
  collection
-----------------------------------------------------------------------------*/
void _mi_mem_collect(mi_os_tld_t* tld) {
  // free every region that has no segments in use.
  size_t rcount = mi_atomic_load_relaxed(&regions_count);
  for (size_t i = 0; i < rcount; i++) {
    mem_region_t* region = &regions[i];
    if (mi_atomic_load_relaxed(&region->info) != 0) {
      // if no segments used, try to claim the whole region
      size_t m = mi_atomic_load_relaxed(&region->in_use);
      while (m == 0 && !mi_atomic_cas_weak_release(&region->in_use, &m, MI_BITMAP_FIELD_FULL)) { /* nothing */ };
      if (m == 0) {
        // on success, free the whole region
        uint8_t* start = (uint8_t*)mi_atomic_load_ptr_acquire(uint8_t,&regions[i].start);
        size_t arena_memid = mi_atomic_load_relaxed(&regions[i].arena_memid);
        size_t commit = mi_atomic_load_relaxed(&regions[i].commit);
        memset((void*)&regions[i], 0, sizeof(mem_region_t));  // cast to void* to avoid atomic warning
        // and release the whole region
        mi_atomic_store_release(&region->info, (size_t)0);
        if (start != NULL) { // && !_mi_os_is_huge_reserved(start)) {         
          _mi_abandoned_await_readers(); // ensure no pending reads
          _mi_arena_free(start, MI_REGION_SIZE, arena_memid, (~commit == 0), tld->stats);
        }
      }
    }
  }
}


/* ----------------------------------------------------------------------------
  Other
-----------------------------------------------------------------------------*/

bool _mi_mem_reset(void* p, size_t size, mi_os_tld_t* tld) {
  return _mi_os_reset(p, size, tld->stats);
}

bool _mi_mem_unreset(void* p, size_t size, bool* is_zero, mi_os_tld_t* tld) {
  return _mi_os_unreset(p, size, is_zero, tld->stats);
}

bool _mi_mem_commit(void* p, size_t size, bool* is_zero, mi_os_tld_t* tld) {
  return _mi_os_commit(p, size, is_zero, tld->stats);
}

bool _mi_mem_decommit(void* p, size_t size, mi_os_tld_t* tld) {
  return _mi_os_decommit(p, size, tld->stats);
}

bool _mi_mem_protect(void* p, size_t size) {
  return _mi_os_protect(p, size);
}

bool _mi_mem_unprotect(void* p, size_t size) {
  return _mi_os_unprotect(p, size);
}
