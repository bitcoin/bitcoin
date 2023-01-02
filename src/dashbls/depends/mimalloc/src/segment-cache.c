/* ----------------------------------------------------------------------------
Copyright (c) 2020, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/

/* ----------------------------------------------------------------------------
  Implements a cache of segments to avoid expensive OS calls and to reuse
  the commit_mask to optimize the commit/decommit calls.
  The full memory map of all segments is also implemented here.
-----------------------------------------------------------------------------*/
#include "mimalloc.h"
#include "mimalloc-internal.h"
#include "mimalloc-atomic.h"

#include "bitmap.h"  // atomic bitmap

//#define MI_CACHE_DISABLE 1    // define to completely disable the segment cache

#define MI_CACHE_FIELDS     (16)
#define MI_CACHE_MAX        (MI_BITMAP_FIELD_BITS*MI_CACHE_FIELDS)       // 1024 on 64-bit

#define BITS_SET()          MI_ATOMIC_VAR_INIT(UINTPTR_MAX)
#define MI_CACHE_BITS_SET   MI_INIT16(BITS_SET)                          // note: update if MI_CACHE_FIELDS changes

typedef struct mi_cache_slot_s {
  void*               p;
  size_t              memid;
  bool                is_pinned;
  mi_commit_mask_t    commit_mask;
  mi_commit_mask_t    decommit_mask;
  _Atomic(mi_msecs_t) expire;
} mi_cache_slot_t;

static mi_decl_cache_align mi_cache_slot_t cache[MI_CACHE_MAX];    // = 0

static mi_decl_cache_align mi_bitmap_field_t cache_available[MI_CACHE_FIELDS] = { MI_CACHE_BITS_SET };        // zero bit = available!
static mi_decl_cache_align mi_bitmap_field_t cache_available_large[MI_CACHE_FIELDS] = { MI_CACHE_BITS_SET };
static mi_decl_cache_align mi_bitmap_field_t cache_inuse[MI_CACHE_FIELDS];   // zero bit = free

static bool mi_cdecl mi_segment_cache_is_suitable(mi_bitmap_index_t bitidx, void* arg) {
  mi_arena_id_t req_arena_id = *((mi_arena_id_t*)arg);
  mi_cache_slot_t* slot = &cache[mi_bitmap_index_bit(bitidx)];
  return _mi_arena_memid_is_suitable(slot->memid, req_arena_id);
}

mi_decl_noinline void* _mi_segment_cache_pop(size_t size, mi_commit_mask_t* commit_mask, mi_commit_mask_t* decommit_mask, bool* large, bool* is_pinned, bool* is_zero, mi_arena_id_t _req_arena_id, size_t* memid, mi_os_tld_t* tld)
{
#ifdef MI_CACHE_DISABLE
  return NULL;
#else

  // only segment blocks
  if (size != MI_SEGMENT_SIZE) return NULL;

  // numa node determines start field
  const int numa_node = _mi_os_numa_node(tld);
  size_t start_field = 0;
  if (numa_node > 0) {
    start_field = (MI_CACHE_FIELDS / _mi_os_numa_node_count())*numa_node;
    if (start_field >= MI_CACHE_FIELDS) start_field = 0;
  }

  // find an available slot
  mi_bitmap_index_t bitidx = 0;
  bool claimed = false;
  mi_arena_id_t req_arena_id = _req_arena_id;
  mi_bitmap_pred_fun_t pred_fun = &mi_segment_cache_is_suitable;  // cannot pass NULL as the arena may be exclusive itself; todo: do not put exclusive arenas in the cache?
  
  if (*large) {  // large allowed?
    claimed = _mi_bitmap_try_find_from_claim_pred(cache_available_large, MI_CACHE_FIELDS, start_field, 1, pred_fun, &req_arena_id, &bitidx);
    if (claimed) *large = true;
  }
  if (!claimed) {
    claimed = _mi_bitmap_try_find_from_claim_pred (cache_available, MI_CACHE_FIELDS, start_field, 1, pred_fun, &req_arena_id, &bitidx);
    if (claimed) *large = false;
  }

  if (!claimed) return NULL;

  // found a slot
  mi_cache_slot_t* slot = &cache[mi_bitmap_index_bit(bitidx)];
  void* p = slot->p;
  *memid = slot->memid;
  *is_pinned = slot->is_pinned;
  *is_zero = false;
  *commit_mask = slot->commit_mask;     
  *decommit_mask = slot->decommit_mask;
  slot->p = NULL;
  mi_atomic_storei64_release(&slot->expire,(mi_msecs_t)0);
  
  // mark the slot as free again
  mi_assert_internal(_mi_bitmap_is_claimed(cache_inuse, MI_CACHE_FIELDS, 1, bitidx));
  _mi_bitmap_unclaim(cache_inuse, MI_CACHE_FIELDS, 1, bitidx);
  return p;
#endif
}

static mi_decl_noinline void mi_commit_mask_decommit(mi_commit_mask_t* cmask, void* p, size_t total, mi_stats_t* stats)
{
  if (mi_commit_mask_is_empty(cmask)) {
    // nothing
  }
  else if (mi_commit_mask_is_full(cmask)) {
    _mi_os_decommit(p, total, stats);
  }
  else {
    // todo: one call to decommit the whole at once?
    mi_assert_internal((total%MI_COMMIT_MASK_BITS)==0);
    size_t part = total/MI_COMMIT_MASK_BITS;
    size_t idx;
    size_t count;    
    mi_commit_mask_foreach(cmask, idx, count) {
      void*  start = (uint8_t*)p + (idx*part);
      size_t size = count*part;
      _mi_os_decommit(start, size, stats);
    }
    mi_commit_mask_foreach_end()
  }
  mi_commit_mask_create_empty(cmask);
}

#define MI_MAX_PURGE_PER_PUSH  (4)

static mi_decl_noinline void mi_segment_cache_purge(bool force, mi_os_tld_t* tld)
{
  MI_UNUSED(tld);
  if (!mi_option_is_enabled(mi_option_allow_decommit)) return;
  mi_msecs_t now = _mi_clock_now();
  size_t purged = 0;
  const size_t max_visits = (force ? MI_CACHE_MAX /* visit all */ : MI_CACHE_FIELDS /* probe at most N (=16) slots */);
  size_t idx              = (force ? 0 : _mi_random_shuffle((uintptr_t)now) % MI_CACHE_MAX /* random start */ );
  for (size_t visited = 0; visited < max_visits; visited++,idx++) {  // visit N slots
    if (idx >= MI_CACHE_MAX) idx = 0; // wrap
    mi_cache_slot_t* slot = &cache[idx];
    mi_msecs_t expire = mi_atomic_loadi64_relaxed(&slot->expire);
    if (expire != 0 && (force || now >= expire)) {  // racy read
      // seems expired, first claim it from available
      purged++;
      mi_bitmap_index_t bitidx = mi_bitmap_index_create_from_bit(idx);
      if (_mi_bitmap_claim(cache_available, MI_CACHE_FIELDS, 1, bitidx, NULL)) {
        // was available, we claimed it
        expire = mi_atomic_loadi64_acquire(&slot->expire);
        if (expire != 0 && (force || now >= expire)) {  // safe read
          // still expired, decommit it
          mi_atomic_storei64_relaxed(&slot->expire,(mi_msecs_t)0);
          mi_assert_internal(!mi_commit_mask_is_empty(&slot->commit_mask) && _mi_bitmap_is_claimed(cache_available_large, MI_CACHE_FIELDS, 1, bitidx));
          _mi_abandoned_await_readers();  // wait until safe to decommit
          // decommit committed parts
          // TODO: instead of decommit, we could also free to the OS?
          mi_commit_mask_decommit(&slot->commit_mask, slot->p, MI_SEGMENT_SIZE, tld->stats);
          mi_commit_mask_create_empty(&slot->decommit_mask);
        }
        _mi_bitmap_unclaim(cache_available, MI_CACHE_FIELDS, 1, bitidx); // make it available again for a pop
      }
      if (!force && purged > MI_MAX_PURGE_PER_PUSH) break;  // bound to no more than N purge tries per push
    }
  }
}

void _mi_segment_cache_collect(bool force, mi_os_tld_t* tld) {
  mi_segment_cache_purge(force, tld );
}

mi_decl_noinline bool _mi_segment_cache_push(void* start, size_t size, size_t memid, const mi_commit_mask_t* commit_mask, const mi_commit_mask_t* decommit_mask, bool is_large, bool is_pinned, mi_os_tld_t* tld)
{
#ifdef MI_CACHE_DISABLE
  return false;
#else

  // only for normal segment blocks
  if (size != MI_SEGMENT_SIZE || ((uintptr_t)start % MI_SEGMENT_ALIGN) != 0) return false;

  // numa node determines start field
  int numa_node = _mi_os_numa_node(NULL);
  size_t start_field = 0;
  if (numa_node > 0) {
    start_field = (MI_CACHE_FIELDS / _mi_os_numa_node_count())*numa_node;
    if (start_field >= MI_CACHE_FIELDS) start_field = 0;
  }

  // purge expired entries
  mi_segment_cache_purge(false /* force? */, tld);

  // find an available slot
  mi_bitmap_index_t bitidx;
  bool claimed = _mi_bitmap_try_find_from_claim(cache_inuse, MI_CACHE_FIELDS, start_field, 1, &bitidx);
  if (!claimed) return false;

  mi_assert_internal(_mi_bitmap_is_claimed(cache_available, MI_CACHE_FIELDS, 1, bitidx));
  mi_assert_internal(_mi_bitmap_is_claimed(cache_available_large, MI_CACHE_FIELDS, 1, bitidx));
#if MI_DEBUG>1
  if (is_pinned || is_large) {
    mi_assert_internal(mi_commit_mask_is_full(commit_mask));
  }
#endif

  // set the slot
  mi_cache_slot_t* slot = &cache[mi_bitmap_index_bit(bitidx)];
  slot->p = start;
  slot->memid = memid;
  slot->is_pinned = is_pinned;
  mi_atomic_storei64_relaxed(&slot->expire,(mi_msecs_t)0);
  slot->commit_mask = *commit_mask;
  slot->decommit_mask = *decommit_mask;
  if (!mi_commit_mask_is_empty(commit_mask) && !is_large && !is_pinned && mi_option_is_enabled(mi_option_allow_decommit)) {
    long delay = mi_option_get(mi_option_segment_decommit_delay);
    if (delay == 0) {
      _mi_abandoned_await_readers(); // wait until safe to decommit
      mi_commit_mask_decommit(&slot->commit_mask, start, MI_SEGMENT_SIZE, tld->stats);
      mi_commit_mask_create_empty(&slot->decommit_mask);
    }
    else {
      mi_atomic_storei64_release(&slot->expire, _mi_clock_now() + delay);
    }
  }

  // make it available
  _mi_bitmap_unclaim((is_large ? cache_available_large : cache_available), MI_CACHE_FIELDS, 1, bitidx);
  return true;
#endif
}


/* -----------------------------------------------------------
  The following functions are to reliably find the segment or
  block that encompasses any pointer p (or NULL if it is not
  in any of our segments).
  We maintain a bitmap of all memory with 1 bit per MI_SEGMENT_SIZE (64MiB)
  set to 1 if it contains the segment meta data.
----------------------------------------------------------- */


#if (MI_INTPTR_SIZE==8)
#define MI_MAX_ADDRESS    ((size_t)20 << 40)  // 20TB
#else
#define MI_MAX_ADDRESS    ((size_t)2 << 30)   // 2Gb
#endif

#define MI_SEGMENT_MAP_BITS  (MI_MAX_ADDRESS / MI_SEGMENT_SIZE)
#define MI_SEGMENT_MAP_SIZE  (MI_SEGMENT_MAP_BITS / 8)
#define MI_SEGMENT_MAP_WSIZE (MI_SEGMENT_MAP_SIZE / MI_INTPTR_SIZE)

static _Atomic(uintptr_t) mi_segment_map[MI_SEGMENT_MAP_WSIZE + 1];  // 2KiB per TB with 64MiB segments

static size_t mi_segment_map_index_of(const mi_segment_t* segment, size_t* bitidx) {
  mi_assert_internal(_mi_ptr_segment(segment) == segment); // is it aligned on MI_SEGMENT_SIZE?
  if ((uintptr_t)segment >= MI_MAX_ADDRESS) {
    *bitidx = 0;
    return MI_SEGMENT_MAP_WSIZE;
  }
  else {
    const uintptr_t segindex = ((uintptr_t)segment) / MI_SEGMENT_SIZE;
    *bitidx = segindex % MI_INTPTR_BITS;
    const size_t mapindex = segindex / MI_INTPTR_BITS;
    mi_assert_internal(mapindex < MI_SEGMENT_MAP_WSIZE);
    return mapindex;
  }
}

void _mi_segment_map_allocated_at(const mi_segment_t* segment) {
  size_t bitidx;
  size_t index = mi_segment_map_index_of(segment, &bitidx);
  mi_assert_internal(index <= MI_SEGMENT_MAP_WSIZE);
  if (index==MI_SEGMENT_MAP_WSIZE) return;
  uintptr_t mask = mi_atomic_load_relaxed(&mi_segment_map[index]);
  uintptr_t newmask;
  do {
    newmask = (mask | ((uintptr_t)1 << bitidx));
  } while (!mi_atomic_cas_weak_release(&mi_segment_map[index], &mask, newmask));
}

void _mi_segment_map_freed_at(const mi_segment_t* segment) {
  size_t bitidx;
  size_t index = mi_segment_map_index_of(segment, &bitidx);
  mi_assert_internal(index <= MI_SEGMENT_MAP_WSIZE);
  if (index == MI_SEGMENT_MAP_WSIZE) return;
  uintptr_t mask = mi_atomic_load_relaxed(&mi_segment_map[index]);
  uintptr_t newmask;
  do {
    newmask = (mask & ~((uintptr_t)1 << bitidx));
  } while (!mi_atomic_cas_weak_release(&mi_segment_map[index], &mask, newmask));
}

// Determine the segment belonging to a pointer or NULL if it is not in a valid segment.
static mi_segment_t* _mi_segment_of(const void* p) {
  mi_segment_t* segment = _mi_ptr_segment(p);
  if (segment == NULL) return NULL; 
  size_t bitidx;
  size_t index = mi_segment_map_index_of(segment, &bitidx);
  // fast path: for any pointer to valid small/medium/large object or first MI_SEGMENT_SIZE in huge
  const uintptr_t mask = mi_atomic_load_relaxed(&mi_segment_map[index]);
  if mi_likely((mask & ((uintptr_t)1 << bitidx)) != 0) {
    return segment; // yes, allocated by us
  }
  if (index==MI_SEGMENT_MAP_WSIZE) return NULL;

  // TODO: maintain max/min allocated range for efficiency for more efficient rejection of invalid pointers?

  // search downwards for the first segment in case it is an interior pointer
  // could be slow but searches in MI_INTPTR_SIZE * MI_SEGMENT_SIZE (512MiB) steps trough
  // valid huge objects
  // note: we could maintain a lowest index to speed up the path for invalid pointers?
  size_t lobitidx;
  size_t loindex;
  uintptr_t lobits = mask & (((uintptr_t)1 << bitidx) - 1);
  if (lobits != 0) {
    loindex = index;
    lobitidx = mi_bsr(lobits);    // lobits != 0
  }
  else if (index == 0) {
    return NULL;
  }
  else {
    mi_assert_internal(index > 0);
    uintptr_t lomask = mask;
    loindex = index;
    do {
      loindex--;  
      lomask = mi_atomic_load_relaxed(&mi_segment_map[loindex]);      
    } while (lomask != 0 && loindex > 0);
    if (lomask == 0) return NULL;
    lobitidx = mi_bsr(lomask);    // lomask != 0
  }
  mi_assert_internal(loindex < MI_SEGMENT_MAP_WSIZE);
  // take difference as the addresses could be larger than the MAX_ADDRESS space.
  size_t diff = (((index - loindex) * (8*MI_INTPTR_SIZE)) + bitidx - lobitidx) * MI_SEGMENT_SIZE;
  segment = (mi_segment_t*)((uint8_t*)segment - diff);

  if (segment == NULL) return NULL;
  mi_assert_internal((void*)segment < p);
  bool cookie_ok = (_mi_ptr_cookie(segment) == segment->cookie);
  mi_assert_internal(cookie_ok);
  if mi_unlikely(!cookie_ok) return NULL;
  if (((uint8_t*)segment + mi_segment_size(segment)) <= (uint8_t*)p) return NULL; // outside the range
  mi_assert_internal(p >= (void*)segment && (uint8_t*)p < (uint8_t*)segment + mi_segment_size(segment));
  return segment;
}

// Is this a valid pointer in our heap?
static bool  mi_is_valid_pointer(const void* p) {
  return (_mi_segment_of(p) != NULL);
}

mi_decl_nodiscard mi_decl_export bool mi_is_in_heap_region(const void* p) mi_attr_noexcept {
  return mi_is_valid_pointer(p);
}

/*
// Return the full segment range belonging to a pointer
static void* mi_segment_range_of(const void* p, size_t* size) {
  mi_segment_t* segment = _mi_segment_of(p);
  if (segment == NULL) {
    if (size != NULL) *size = 0;
    return NULL;
  }
  else {
    if (size != NULL) *size = segment->segment_size;
    return segment;
  }
  mi_assert_expensive(page == NULL || mi_segment_is_valid(_mi_page_segment(page),tld));
  mi_assert_internal(page == NULL || (mi_segment_page_size(_mi_page_segment(page)) - (MI_SECURE == 0 ? 0 : _mi_os_page_size())) >= block_size);
  mi_reset_delayed(tld);
  mi_assert_internal(page == NULL || mi_page_not_in_queue(page, tld));
  return page;
}
*/
