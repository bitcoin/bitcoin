/* ----------------------------------------------------------------------------
Copyright (c) 2018-2022, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE   // for realpath() on Linux
#endif

#include "mimalloc.h"
#include "mimalloc-internal.h"
#include "mimalloc-atomic.h"


#include <string.h>  // memset, strlen
#include <stdlib.h>  // malloc, exit

#define MI_IN_ALLOC_C
#include "alloc-override.c"
#undef MI_IN_ALLOC_C

// ------------------------------------------------------
// Allocation
// ------------------------------------------------------

// Fast allocation in a page: just pop from the free list.
// Fall back to generic allocation only if the list is empty.
extern inline void* _mi_page_malloc(mi_heap_t* heap, mi_page_t* page, size_t size, bool zero) mi_attr_noexcept {
  mi_assert_internal(page->xblock_size==0||mi_page_block_size(page) >= size);
  mi_block_t* const block = page->free;
  if mi_unlikely(block == NULL) {
    return _mi_malloc_generic(heap, size, zero); 
  }
  mi_assert_internal(block != NULL && _mi_ptr_page(block) == page);
  // pop from the free list
  page->used++;
  page->free = mi_block_next(page, block);
  mi_assert_internal(page->free == NULL || _mi_ptr_page(page->free) == page);

  // allow use of the block internally 
  // note: when tracking we need to avoid ever touching the MI_PADDING since
  // that is tracked by valgrind etc. as non-accessible (through the red-zone, see `mimalloc-track.h`)
  mi_track_mem_undefined(block, mi_page_usable_block_size(page));
  
  // zero the block? note: we need to zero the full block size (issue #63)
  if mi_unlikely(zero) {
    mi_assert_internal(page->xblock_size != 0); // do not call with zero'ing for huge blocks (see _mi_malloc_generic)
    const size_t zsize = (page->is_zero ? sizeof(block->next) + MI_PADDING_SIZE : page->xblock_size);
    _mi_memzero_aligned(block, zsize - MI_PADDING_SIZE);    
  }

#if (MI_DEBUG>0) && !MI_TRACK_ENABLED
  if (!page->is_zero && !zero) { memset(block, MI_DEBUG_UNINIT, mi_page_usable_block_size(page)); }
#elif (MI_SECURE!=0)
  if (!zero) { block->next = 0; } // don't leak internal data
#endif

#if (MI_STAT>0)
  const size_t bsize = mi_page_usable_block_size(page);
  if (bsize <= MI_MEDIUM_OBJ_SIZE_MAX) {
    mi_heap_stat_increase(heap, normal, bsize);
    mi_heap_stat_counter_increase(heap, normal_count, 1);
#if (MI_STAT>1)
    const size_t bin = _mi_bin(bsize);
    mi_heap_stat_increase(heap, normal_bins[bin], 1);
#endif
  }
#endif

#if (MI_PADDING > 0) && defined(MI_ENCODE_FREELIST) && !MI_TRACK_ENABLED
  mi_padding_t* const padding = (mi_padding_t*)((uint8_t*)block + mi_page_usable_block_size(page));
  ptrdiff_t delta = ((uint8_t*)padding - (uint8_t*)block - (size - MI_PADDING_SIZE));
  #if (MI_DEBUG>1)
  mi_assert_internal(delta >= 0 && mi_page_usable_block_size(page) >= (size - MI_PADDING_SIZE + delta));
  mi_track_mem_defined(padding,sizeof(mi_padding_t));  // note: re-enable since mi_page_usable_block_size may set noaccess
  #endif
  padding->canary = (uint32_t)(mi_ptr_encode(page,block,page->keys));
  padding->delta  = (uint32_t)(delta);
  uint8_t* fill = (uint8_t*)padding - delta;
  const size_t maxpad = (delta > MI_MAX_ALIGN_SIZE ? MI_MAX_ALIGN_SIZE : delta); // set at most N initial padding bytes
  for (size_t i = 0; i < maxpad; i++) { fill[i] = MI_DEBUG_PADDING; }
#endif

  return block;
}

static inline mi_decl_restrict void* mi_heap_malloc_small_zero(mi_heap_t* heap, size_t size, bool zero) mi_attr_noexcept {
  mi_assert(heap != NULL);
  mi_assert(heap->thread_id == 0 || heap->thread_id == _mi_thread_id()); // heaps are thread local
  mi_assert(size <= MI_SMALL_SIZE_MAX);
#if (MI_PADDING)
  if (size == 0) {
    size = sizeof(void*);
  }
#endif
  mi_page_t* page = _mi_heap_get_free_small_page(heap, size + MI_PADDING_SIZE);
  void* p = _mi_page_malloc(heap, page, size + MI_PADDING_SIZE, zero);
  mi_assert_internal(p == NULL || mi_usable_size(p) >= size);
#if MI_STAT>1
  if (p != NULL) {
    if (!mi_heap_is_initialized(heap)) { heap = mi_get_default_heap(); }
    mi_heap_stat_increase(heap, malloc, mi_usable_size(p));
  }
#endif
  mi_track_malloc(p,size,zero);
  return p;
}

// allocate a small block
mi_decl_nodiscard extern inline mi_decl_restrict void* mi_heap_malloc_small(mi_heap_t* heap, size_t size) mi_attr_noexcept {
  return mi_heap_malloc_small_zero(heap, size, false);
}

mi_decl_nodiscard extern inline mi_decl_restrict void* mi_malloc_small(size_t size) mi_attr_noexcept {
  return mi_heap_malloc_small(mi_get_default_heap(), size);
}

// The main allocation function
extern inline void* _mi_heap_malloc_zero(mi_heap_t* heap, size_t size, bool zero) mi_attr_noexcept {
  if mi_likely(size <= MI_SMALL_SIZE_MAX) {
    return mi_heap_malloc_small_zero(heap, size, zero);
  }
  else {
    mi_assert(heap!=NULL);
    mi_assert(heap->thread_id == 0 || heap->thread_id == _mi_thread_id());   // heaps are thread local
    void* const p = _mi_malloc_generic(heap, size + MI_PADDING_SIZE, zero);  // note: size can overflow but it is detected in malloc_generic
    mi_assert_internal(p == NULL || mi_usable_size(p) >= size);
    #if MI_STAT>1
    if (p != NULL) {
      if (!mi_heap_is_initialized(heap)) { heap = mi_get_default_heap(); }
      mi_heap_stat_increase(heap, malloc, mi_usable_size(p));
    }
    #endif
    mi_track_malloc(p,size,zero);
    return p;
  }
}

mi_decl_nodiscard extern inline mi_decl_restrict void* mi_heap_malloc(mi_heap_t* heap, size_t size) mi_attr_noexcept {
  return _mi_heap_malloc_zero(heap, size, false);
}

mi_decl_nodiscard extern inline mi_decl_restrict void* mi_malloc(size_t size) mi_attr_noexcept {
  return mi_heap_malloc(mi_get_default_heap(), size);
}

// zero initialized small block
mi_decl_nodiscard mi_decl_restrict void* mi_zalloc_small(size_t size) mi_attr_noexcept {
  return mi_heap_malloc_small_zero(mi_get_default_heap(), size, true);
}

mi_decl_nodiscard extern inline mi_decl_restrict void* mi_heap_zalloc(mi_heap_t* heap, size_t size) mi_attr_noexcept {
  return _mi_heap_malloc_zero(heap, size, true);
}

mi_decl_nodiscard mi_decl_restrict void* mi_zalloc(size_t size) mi_attr_noexcept {
  return mi_heap_zalloc(mi_get_default_heap(),size);
}


// ------------------------------------------------------
// Check for double free in secure and debug mode
// This is somewhat expensive so only enabled for secure mode 4
// ------------------------------------------------------

#if (MI_ENCODE_FREELIST && (MI_SECURE>=4 || MI_DEBUG!=0))
// linear check if the free list contains a specific element
static bool mi_list_contains(const mi_page_t* page, const mi_block_t* list, const mi_block_t* elem) {
  while (list != NULL) {
    if (elem==list) return true;
    list = mi_block_next(page, list);
  }
  return false;
}

static mi_decl_noinline bool mi_check_is_double_freex(const mi_page_t* page, const mi_block_t* block) {
  // The decoded value is in the same page (or NULL).
  // Walk the free lists to verify positively if it is already freed
  if (mi_list_contains(page, page->free, block) ||
      mi_list_contains(page, page->local_free, block) ||
      mi_list_contains(page, mi_page_thread_free(page), block))
  {
    _mi_error_message(EAGAIN, "double free detected of block %p with size %zu\n", block, mi_page_block_size(page));
    return true;
  }
  return false;
}

#define mi_track_page(page,access)  { size_t psize; void* pstart = _mi_page_start(_mi_page_segment(page),page,&psize); mi_track_mem_##access( pstart, psize); }

static inline bool mi_check_is_double_free(const mi_page_t* page, const mi_block_t* block) {
  bool is_double_free = false;
  mi_block_t* n = mi_block_nextx(page, block, page->keys); // pretend it is freed, and get the decoded first field
  if (((uintptr_t)n & (MI_INTPTR_SIZE-1))==0 &&  // quick check: aligned pointer?
      (n==NULL || mi_is_in_same_page(block, n))) // quick check: in same page or NULL?
  {
    // Suspicous: decoded value a in block is in the same page (or NULL) -- maybe a double free?
    // (continue in separate function to improve code generation)
    is_double_free = mi_check_is_double_freex(page, block);
  }
  return is_double_free;
}
#else
static inline bool mi_check_is_double_free(const mi_page_t* page, const mi_block_t* block) {
  MI_UNUSED(page);
  MI_UNUSED(block);
  return false;
}
#endif

// ---------------------------------------------------------------------------
// Check for heap block overflow by setting up padding at the end of the block
// ---------------------------------------------------------------------------

#if (MI_PADDING>0) && defined(MI_ENCODE_FREELIST) && !MI_TRACK_ENABLED
static bool mi_page_decode_padding(const mi_page_t* page, const mi_block_t* block, size_t* delta, size_t* bsize) {
  *bsize = mi_page_usable_block_size(page);
  const mi_padding_t* const padding = (mi_padding_t*)((uint8_t*)block + *bsize);
  mi_track_mem_defined(padding,sizeof(mi_padding_t));
  *delta = padding->delta;
  uint32_t canary = padding->canary;
  uintptr_t keys[2]; 
  keys[0] = page->keys[0];
  keys[1] = page->keys[1]; 
  bool ok = ((uint32_t)mi_ptr_encode(page,block,keys) == canary && *delta <= *bsize);
  mi_track_mem_noaccess(padding,sizeof(mi_padding_t));
  return ok;
}

// Return the exact usable size of a block.
static size_t mi_page_usable_size_of(const mi_page_t* page, const mi_block_t* block) {
  size_t bsize;
  size_t delta;
  bool ok = mi_page_decode_padding(page, block, &delta, &bsize);
  mi_assert_internal(ok); mi_assert_internal(delta <= bsize);  
  return (ok ? bsize - delta : 0);
}

static bool mi_verify_padding(const mi_page_t* page, const mi_block_t* block, size_t* size, size_t* wrong) {
  size_t bsize;
  size_t delta;
  bool ok = mi_page_decode_padding(page, block, &delta, &bsize);
  *size = *wrong = bsize;
  if (!ok) return false;
  mi_assert_internal(bsize >= delta);
  *size = bsize - delta;
  uint8_t* fill = (uint8_t*)block + bsize - delta;
  const size_t maxpad = (delta > MI_MAX_ALIGN_SIZE ? MI_MAX_ALIGN_SIZE : delta); // check at most the first N padding bytes
  mi_track_mem_defined(fill,maxpad);
  for (size_t i = 0; i < maxpad; i++) {
    if (fill[i] != MI_DEBUG_PADDING) {
      *wrong = bsize - delta + i;
      ok = false;
      break;
    }
  }
  mi_track_mem_noaccess(fill,maxpad);
  return ok;
}

static void mi_check_padding(const mi_page_t* page, const mi_block_t* block) {
  size_t size;
  size_t wrong;
  if (!mi_verify_padding(page,block,&size,&wrong)) {
    _mi_error_message(EFAULT, "buffer overflow in heap block %p of size %zu: write after %zu bytes\n", block, size, wrong );
  }
}

// When a non-thread-local block is freed, it becomes part of the thread delayed free
// list that is freed later by the owning heap. If the exact usable size is too small to
// contain the pointer for the delayed list, then shrink the padding (by decreasing delta)
// so it will later not trigger an overflow error in `mi_free_block`.
static void mi_padding_shrink(const mi_page_t* page, const mi_block_t* block, const size_t min_size) {
  size_t bsize;
  size_t delta;
  bool ok = mi_page_decode_padding(page, block, &delta, &bsize);
  mi_assert_internal(ok);
  if (!ok || (bsize - delta) >= min_size) return;  // usually already enough space
  mi_assert_internal(bsize >= min_size);
  if (bsize < min_size) return;  // should never happen
  size_t new_delta = (bsize - min_size);
  mi_assert_internal(new_delta < bsize);
  mi_padding_t* padding = (mi_padding_t*)((uint8_t*)block + bsize);
  padding->delta = (uint32_t)new_delta;
}
#else
static void mi_check_padding(const mi_page_t* page, const mi_block_t* block) {
  MI_UNUSED(page);
  MI_UNUSED(block);
}

static size_t mi_page_usable_size_of(const mi_page_t* page, const mi_block_t* block) {
  MI_UNUSED(block);
  return mi_page_usable_block_size(page);
}

static void mi_padding_shrink(const mi_page_t* page, const mi_block_t* block, const size_t min_size) {
  MI_UNUSED(page);
  MI_UNUSED(block);
  MI_UNUSED(min_size);
}
#endif

// only maintain stats for smaller objects if requested
#if (MI_STAT>0)
static void mi_stat_free(const mi_page_t* page, const mi_block_t* block) {
  #if (MI_STAT < 2)  
  MI_UNUSED(block);
  #endif
  mi_heap_t* const heap = mi_heap_get_default();
  const size_t bsize = mi_page_usable_block_size(page);
  #if (MI_STAT>1)
  const size_t usize = mi_page_usable_size_of(page, block);
  mi_heap_stat_decrease(heap, malloc, usize);
  #endif  
  if (bsize <= MI_MEDIUM_OBJ_SIZE_MAX) {
    mi_heap_stat_decrease(heap, normal, bsize);
    #if (MI_STAT > 1)
    mi_heap_stat_decrease(heap, normal_bins[_mi_bin(bsize)], 1);
    #endif
  }
  else if (bsize <= MI_LARGE_OBJ_SIZE_MAX) {
    mi_heap_stat_decrease(heap, large, bsize);
  }
  else {
    mi_heap_stat_decrease(heap, huge, bsize);
  }
}
#else
static void mi_stat_free(const mi_page_t* page, const mi_block_t* block) {
  MI_UNUSED(page); MI_UNUSED(block);
}
#endif

#if (MI_STAT>0)
// maintain stats for huge objects
static void mi_stat_huge_free(const mi_page_t* page) {
  mi_heap_t* const heap = mi_heap_get_default();
  const size_t bsize = mi_page_block_size(page); // to match stats in `page.c:mi_page_huge_alloc`
  if (bsize <= MI_LARGE_OBJ_SIZE_MAX) {
    mi_heap_stat_decrease(heap, large, bsize);
  }
  else {
    mi_heap_stat_decrease(heap, huge, bsize);
  }
}
#else
static void mi_stat_huge_free(const mi_page_t* page) {
  MI_UNUSED(page);
}
#endif

// ------------------------------------------------------
// Free
// ------------------------------------------------------

// multi-threaded free (or free in huge block)
static mi_decl_noinline void _mi_free_block_mt(mi_page_t* page, mi_block_t* block)
{
  // The padding check may access the non-thread-owned page for the key values.
  // that is safe as these are constant and the page won't be freed (as the block is not freed yet).
  mi_check_padding(page, block);
  mi_padding_shrink(page, block, sizeof(mi_block_t));       // for small size, ensure we can fit the delayed thread pointers without triggering overflow detection
  #if (MI_DEBUG!=0) && !MI_TRACK_ENABLED                    // note: when tracking, cannot use mi_usable_size with multi-threading
  memset(block, MI_DEBUG_FREED, mi_usable_size(block));
  #endif

  // huge page segments are always abandoned and can be freed immediately
  mi_segment_t* segment = _mi_page_segment(page);
  if (segment->kind==MI_SEGMENT_HUGE) {
    mi_stat_huge_free(page);
    _mi_segment_huge_page_free(segment, page, block);
    return;
  }

  // Try to put the block on either the page-local thread free list, or the heap delayed free list.
  mi_thread_free_t tfreex;
  bool use_delayed;
  mi_thread_free_t tfree = mi_atomic_load_relaxed(&page->xthread_free);
  do {
    use_delayed = (mi_tf_delayed(tfree) == MI_USE_DELAYED_FREE);
    if mi_unlikely(use_delayed) {
      // unlikely: this only happens on the first concurrent free in a page that is in the full list
      tfreex = mi_tf_set_delayed(tfree,MI_DELAYED_FREEING);
    }
    else {
      // usual: directly add to page thread_free list
      mi_block_set_next(page, block, mi_tf_block(tfree));
      tfreex = mi_tf_set_block(tfree,block);
    }
  } while (!mi_atomic_cas_weak_release(&page->xthread_free, &tfree, tfreex));

  if mi_unlikely(use_delayed) {
    // racy read on `heap`, but ok because MI_DELAYED_FREEING is set (see `mi_heap_delete` and `mi_heap_collect_abandon`)
    mi_heap_t* const heap = (mi_heap_t*)(mi_atomic_load_acquire(&page->xheap)); //mi_page_heap(page);
    mi_assert_internal(heap != NULL);
    if (heap != NULL) {
      // add to the delayed free list of this heap. (do this atomically as the lock only protects heap memory validity)
      mi_block_t* dfree = mi_atomic_load_ptr_relaxed(mi_block_t, &heap->thread_delayed_free);
      do {
        mi_block_set_nextx(heap,block,dfree, heap->keys);
      } while (!mi_atomic_cas_ptr_weak_release(mi_block_t,&heap->thread_delayed_free, &dfree, block));
    }

    // and reset the MI_DELAYED_FREEING flag
    tfree = mi_atomic_load_relaxed(&page->xthread_free);
    do {
      tfreex = tfree;
      mi_assert_internal(mi_tf_delayed(tfree) == MI_DELAYED_FREEING);
      tfreex = mi_tf_set_delayed(tfree,MI_NO_DELAYED_FREE);
    } while (!mi_atomic_cas_weak_release(&page->xthread_free, &tfree, tfreex));
  }
}

// regular free
static inline void _mi_free_block(mi_page_t* page, bool local, mi_block_t* block)
{
  // and push it on the free list
  //const size_t bsize = mi_page_block_size(page);
  if mi_likely(local) {
    // owning thread can free a block directly
    if mi_unlikely(mi_check_is_double_free(page, block)) return;
    mi_check_padding(page, block);
    #if (MI_DEBUG!=0) && !MI_TRACK_ENABLED
    memset(block, MI_DEBUG_FREED, mi_page_block_size(page));
    #endif
    mi_block_set_next(page, block, page->local_free);
    page->local_free = block;
    page->used--;
    if mi_unlikely(mi_page_all_free(page)) {
      _mi_page_retire(page);
    }
    else if mi_unlikely(mi_page_is_in_full(page)) {
      _mi_page_unfull(page);
    }
  }
  else {
    _mi_free_block_mt(page,block);
  }
}


// Adjust a block that was allocated aligned, to the actual start of the block in the page.
mi_block_t* _mi_page_ptr_unalign(const mi_segment_t* segment, const mi_page_t* page, const void* p) {
  mi_assert_internal(page!=NULL && p!=NULL);
  const size_t diff   = (uint8_t*)p - _mi_page_start(segment, page, NULL);
  const size_t adjust = (diff % mi_page_block_size(page));
  return (mi_block_t*)((uintptr_t)p - adjust);
}


static void mi_decl_noinline mi_free_generic(const mi_segment_t* segment, bool local, void* p) mi_attr_noexcept {
  mi_page_t* const page = _mi_segment_page_of(segment, p);
  mi_block_t* const block = (mi_page_has_aligned(page) ? _mi_page_ptr_unalign(segment, page, p) : (mi_block_t*)p);
  mi_stat_free(page, block);                 // stat_free may access the padding
  mi_track_free(p);
  _mi_free_block(page, local, block);  
}

// Get the segment data belonging to a pointer
// This is just a single `and` in assembly but does further checks in debug mode
// (and secure mode) if this was a valid pointer.
static inline mi_segment_t* mi_checked_ptr_segment(const void* p, const char* msg) 
{
  MI_UNUSED(msg);
#if (MI_DEBUG>0)
  if mi_unlikely(((uintptr_t)p & (MI_INTPTR_SIZE - 1)) != 0) {
    _mi_error_message(EINVAL, "%s: invalid (unaligned) pointer: %p\n", msg, p);
    return NULL;
  }
#endif

  mi_segment_t* const segment = _mi_ptr_segment(p);
  if mi_unlikely(segment == NULL) return NULL;  // checks also for (p==NULL)

#if (MI_DEBUG>0)
  if mi_unlikely(!mi_is_in_heap_region(p)) {
    _mi_warning_message("%s: pointer might not point to a valid heap region: %p\n"
      "(this may still be a valid very large allocation (over 64MiB))\n", msg, p);
    if mi_likely(_mi_ptr_cookie(segment) == segment->cookie) {
      _mi_warning_message("(yes, the previous pointer %p was valid after all)\n", p);
    }
  }
#endif
#if (MI_DEBUG>0 || MI_SECURE>=4)
  if mi_unlikely(_mi_ptr_cookie(segment) != segment->cookie) {
    _mi_error_message(EINVAL, "%s: pointer does not point to a valid heap space: %p\n", msg, p);
    return NULL;
  }
#endif
  return segment;
}

// Free a block 
void mi_free(void* p) mi_attr_noexcept
{
  mi_segment_t* const segment = mi_checked_ptr_segment(p,"mi_free");
  if mi_unlikely(segment == NULL) return; 

  mi_threadid_t tid = _mi_thread_id();
  mi_page_t* const page = _mi_segment_page_of(segment, p);
  
  if mi_likely(tid == mi_atomic_load_relaxed(&segment->thread_id) && page->flags.full_aligned == 0) {  // the thread id matches and it is not a full page, nor has aligned blocks
    // local, and not full or aligned
    mi_block_t* block = (mi_block_t*)(p);
    if mi_unlikely(mi_check_is_double_free(page,block)) return;
    mi_check_padding(page, block);
    mi_stat_free(page, block);
    #if (MI_DEBUG!=0) && !MI_TRACK_ENABLED
    memset(block, MI_DEBUG_FREED, mi_page_block_size(page));
    #endif
    mi_track_free(p);
    mi_block_set_next(page, block, page->local_free);
    page->local_free = block;
    if mi_unlikely(--page->used == 0) {   // using this expression generates better code than: page->used--; if (mi_page_all_free(page))    
      _mi_page_retire(page);
    }    
  }
  else {
    // non-local, aligned blocks, or a full page; use the more generic path
    // note: recalc page in generic to improve code generation
    mi_free_generic(segment, tid == segment->thread_id, p);
  }  
}

// return true if successful
bool _mi_free_delayed_block(mi_block_t* block) {
  // get segment and page
  const mi_segment_t* const segment = _mi_ptr_segment(block);
  mi_assert_internal(_mi_ptr_cookie(segment) == segment->cookie);
  mi_assert_internal(_mi_thread_id() == segment->thread_id);
  mi_page_t* const page = _mi_segment_page_of(segment, block);

  // Clear the no-delayed flag so delayed freeing is used again for this page.
  // This must be done before collecting the free lists on this page -- otherwise
  // some blocks may end up in the page `thread_free` list with no blocks in the
  // heap `thread_delayed_free` list which may cause the page to be never freed!
  // (it would only be freed if we happen to scan it in `mi_page_queue_find_free_ex`)
  if (!_mi_page_try_use_delayed_free(page, MI_USE_DELAYED_FREE, false /* dont overwrite never delayed */)) {
    return false;
  }

  // collect all other non-local frees to ensure up-to-date `used` count
  _mi_page_free_collect(page, false);

  // and free the block (possibly freeing the page as well since used is updated)
  _mi_free_block(page, true, block);
  return true;
}

// Bytes available in a block
mi_decl_noinline static size_t mi_page_usable_aligned_size_of(const mi_segment_t* segment, const mi_page_t* page, const void* p) mi_attr_noexcept {
  const mi_block_t* block = _mi_page_ptr_unalign(segment, page, p);
  const size_t size = mi_page_usable_size_of(page, block);
  const ptrdiff_t adjust = (uint8_t*)p - (uint8_t*)block;
  mi_assert_internal(adjust >= 0 && (size_t)adjust <= size);
  return (size - adjust);
}

static inline size_t _mi_usable_size(const void* p, const char* msg) mi_attr_noexcept {
  const mi_segment_t* const segment = mi_checked_ptr_segment(p, msg);
  if (segment==NULL) return 0;  // also returns 0 if `p == NULL`
  const mi_page_t* const page = _mi_segment_page_of(segment, p);  
  if mi_likely(!mi_page_has_aligned(page)) {
    const mi_block_t* block = (const mi_block_t*)p;
    return mi_page_usable_size_of(page, block);
  }
  else {
    // split out to separate routine for improved code generation
    return mi_page_usable_aligned_size_of(segment, page, p);
  }
}

mi_decl_nodiscard size_t mi_usable_size(const void* p) mi_attr_noexcept {
  return _mi_usable_size(p, "mi_usable_size");
}


// ------------------------------------------------------
// ensure explicit external inline definitions are emitted!
// ------------------------------------------------------

#ifdef __cplusplus
void* _mi_externs[] = {
  (void*)&_mi_page_malloc,
  (void*)&_mi_heap_malloc_zero,
  (void*)&mi_malloc,
  (void*)&mi_malloc_small,
  (void*)&mi_zalloc_small,
  (void*)&mi_heap_malloc,
  (void*)&mi_heap_zalloc,
  (void*)&mi_heap_malloc_small
};
#endif


// ------------------------------------------------------
// Allocation extensions
// ------------------------------------------------------

void mi_free_size(void* p, size_t size) mi_attr_noexcept {
  MI_UNUSED_RELEASE(size);
  mi_assert(p == NULL || size <= _mi_usable_size(p,"mi_free_size"));
  mi_free(p);
}

void mi_free_size_aligned(void* p, size_t size, size_t alignment) mi_attr_noexcept {
  MI_UNUSED_RELEASE(alignment);
  mi_assert(((uintptr_t)p % alignment) == 0);
  mi_free_size(p,size);
}

void mi_free_aligned(void* p, size_t alignment) mi_attr_noexcept {
  MI_UNUSED_RELEASE(alignment);
  mi_assert(((uintptr_t)p % alignment) == 0);
  mi_free(p);
}

mi_decl_nodiscard extern inline mi_decl_restrict void* mi_heap_calloc(mi_heap_t* heap, size_t count, size_t size) mi_attr_noexcept {
  size_t total;
  if (mi_count_size_overflow(count,size,&total)) return NULL;
  return mi_heap_zalloc(heap,total);
}

mi_decl_nodiscard mi_decl_restrict void* mi_calloc(size_t count, size_t size) mi_attr_noexcept {
  return mi_heap_calloc(mi_get_default_heap(),count,size);
}

// Uninitialized `calloc`
mi_decl_nodiscard extern mi_decl_restrict void* mi_heap_mallocn(mi_heap_t* heap, size_t count, size_t size) mi_attr_noexcept {
  size_t total;
  if (mi_count_size_overflow(count, size, &total)) return NULL;
  return mi_heap_malloc(heap, total);
}

mi_decl_nodiscard mi_decl_restrict void* mi_mallocn(size_t count, size_t size) mi_attr_noexcept {
  return mi_heap_mallocn(mi_get_default_heap(),count,size);
}

// Expand (or shrink) in place (or fail)
void* mi_expand(void* p, size_t newsize) mi_attr_noexcept {
  #if MI_PADDING
  // we do not shrink/expand with padding enabled 
  MI_UNUSED(p); MI_UNUSED(newsize);
  return NULL;
  #else
  if (p == NULL) return NULL;
  const size_t size = _mi_usable_size(p,"mi_expand");
  if (newsize > size) return NULL;
  return p; // it fits
  #endif
}

void* _mi_heap_realloc_zero(mi_heap_t* heap, void* p, size_t newsize, bool zero) mi_attr_noexcept {
  // if p == NULL then behave as malloc.
  // else if size == 0 then reallocate to a zero-sized block (and don't return NULL, just as mi_malloc(0)).
  // (this means that returning NULL always indicates an error, and `p` will not have been freed in that case.)
  const size_t size = _mi_usable_size(p,"mi_realloc"); // also works if p == NULL (with size 0)
  if mi_unlikely(newsize <= size && newsize >= (size / 2) && newsize > 0) {  // note: newsize must be > 0 or otherwise we return NULL for realloc(NULL,0)
    // todo: adjust potential padding to reflect the new size?
    mi_track_free(p);
    mi_track_malloc(p,newsize,true);
    return p;  // reallocation still fits and not more than 50% waste
  }
  void* newp = mi_heap_malloc(heap,newsize);
  if mi_likely(newp != NULL) {
    if (zero && newsize > size) {
      // also set last word in the previous allocation to zero to ensure any padding is zero-initialized
      const size_t start = (size >= sizeof(intptr_t) ? size - sizeof(intptr_t) : 0);
      memset((uint8_t*)newp + start, 0, newsize - start);
    }
    if mi_likely(p != NULL) {
      if mi_likely(_mi_is_aligned(p, sizeof(uintptr_t))) {  // a client may pass in an arbitrary pointer `p`..
        const size_t copysize = (newsize > size ? size : newsize);
        mi_track_mem_defined(p,copysize);  // _mi_useable_size may be too large for byte precise memory tracking..
        _mi_memcpy_aligned(newp, p, copysize);
      }
      mi_free(p); // only free the original pointer if successful
    }
  }
  return newp;
}

mi_decl_nodiscard void* mi_heap_realloc(mi_heap_t* heap, void* p, size_t newsize) mi_attr_noexcept {
  return _mi_heap_realloc_zero(heap, p, newsize, false);  
}

mi_decl_nodiscard void* mi_heap_reallocn(mi_heap_t* heap, void* p, size_t count, size_t size) mi_attr_noexcept {
  size_t total;
  if (mi_count_size_overflow(count, size, &total)) return NULL;
  return mi_heap_realloc(heap, p, total);
}


// Reallocate but free `p` on errors
mi_decl_nodiscard void* mi_heap_reallocf(mi_heap_t* heap, void* p, size_t newsize) mi_attr_noexcept {
  void* newp = mi_heap_realloc(heap, p, newsize);
  if (newp==NULL && p!=NULL) mi_free(p);
  return newp;
}

mi_decl_nodiscard void* mi_heap_rezalloc(mi_heap_t* heap, void* p, size_t newsize) mi_attr_noexcept {
  return _mi_heap_realloc_zero(heap, p, newsize, true);
}

mi_decl_nodiscard void* mi_heap_recalloc(mi_heap_t* heap, void* p, size_t count, size_t size) mi_attr_noexcept {
  size_t total;
  if (mi_count_size_overflow(count, size, &total)) return NULL;
  return mi_heap_rezalloc(heap, p, total);
}


mi_decl_nodiscard void* mi_realloc(void* p, size_t newsize) mi_attr_noexcept {
  return mi_heap_realloc(mi_get_default_heap(),p,newsize);
}

mi_decl_nodiscard void* mi_reallocn(void* p, size_t count, size_t size) mi_attr_noexcept {
  return mi_heap_reallocn(mi_get_default_heap(),p,count,size);
}

// Reallocate but free `p` on errors
mi_decl_nodiscard void* mi_reallocf(void* p, size_t newsize) mi_attr_noexcept {
  return mi_heap_reallocf(mi_get_default_heap(),p,newsize);
}

mi_decl_nodiscard void* mi_rezalloc(void* p, size_t newsize) mi_attr_noexcept {
  return mi_heap_rezalloc(mi_get_default_heap(), p, newsize);
}

mi_decl_nodiscard void* mi_recalloc(void* p, size_t count, size_t size) mi_attr_noexcept {
  return mi_heap_recalloc(mi_get_default_heap(), p, count, size);
}



// ------------------------------------------------------
// strdup, strndup, and realpath
// ------------------------------------------------------

// `strdup` using mi_malloc
mi_decl_nodiscard mi_decl_restrict char* mi_heap_strdup(mi_heap_t* heap, const char* s) mi_attr_noexcept {
  if (s == NULL) return NULL;
  size_t n = strlen(s);
  char* t = (char*)mi_heap_malloc(heap,n+1);
  if (t != NULL) _mi_memcpy(t, s, n + 1);
  return t;
}

mi_decl_nodiscard mi_decl_restrict char* mi_strdup(const char* s) mi_attr_noexcept {
  return mi_heap_strdup(mi_get_default_heap(), s);
}

// `strndup` using mi_malloc
mi_decl_nodiscard mi_decl_restrict char* mi_heap_strndup(mi_heap_t* heap, const char* s, size_t n) mi_attr_noexcept {
  if (s == NULL) return NULL;
  const char* end = (const char*)memchr(s, 0, n);  // find end of string in the first `n` characters (returns NULL if not found)
  const size_t m = (end != NULL ? (size_t)(end - s) : n);  // `m` is the minimum of `n` or the end-of-string
  mi_assert_internal(m <= n);
  char* t = (char*)mi_heap_malloc(heap, m+1);
  if (t == NULL) return NULL;
  _mi_memcpy(t, s, m);
  t[m] = 0;
  return t;
}

mi_decl_nodiscard mi_decl_restrict char* mi_strndup(const char* s, size_t n) mi_attr_noexcept {
  return mi_heap_strndup(mi_get_default_heap(),s,n);
}

#ifndef __wasi__
// `realpath` using mi_malloc
#ifdef _WIN32
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif
#include <windows.h>
mi_decl_nodiscard mi_decl_restrict char* mi_heap_realpath(mi_heap_t* heap, const char* fname, char* resolved_name) mi_attr_noexcept {
  // todo: use GetFullPathNameW to allow longer file names
  char buf[PATH_MAX];
  DWORD res = GetFullPathNameA(fname, PATH_MAX, (resolved_name == NULL ? buf : resolved_name), NULL);
  if (res == 0) {
    errno = GetLastError(); return NULL;
  }
  else if (res > PATH_MAX) {
    errno = EINVAL; return NULL;
  }
  else if (resolved_name != NULL) {
    return resolved_name;
  }
  else {
    return mi_heap_strndup(heap, buf, PATH_MAX);
  }
}
#else
#include <unistd.h>  // pathconf
static size_t mi_path_max(void) {
  static size_t path_max = 0;
  if (path_max <= 0) {
    long m = pathconf("/",_PC_PATH_MAX);
    if (m <= 0) path_max = 4096;      // guess
    else if (m < 256) path_max = 256; // at least 256
    else path_max = m;
  }
  return path_max;
}

char* mi_heap_realpath(mi_heap_t* heap, const char* fname, char* resolved_name) mi_attr_noexcept {
  if (resolved_name != NULL) {
    return realpath(fname,resolved_name);
  }
  else {
    size_t n  = mi_path_max();
    char* buf = (char*)mi_malloc(n+1);
    if (buf==NULL) return NULL;
    char* rname  = realpath(fname,buf);
    char* result = mi_heap_strndup(heap,rname,n); // ok if `rname==NULL`
    mi_free(buf);
    return result;
  }
}
#endif

mi_decl_nodiscard mi_decl_restrict char* mi_realpath(const char* fname, char* resolved_name) mi_attr_noexcept {
  return mi_heap_realpath(mi_get_default_heap(),fname,resolved_name);
}
#endif

/*-------------------------------------------------------
C++ new and new_aligned
The standard requires calling into `get_new_handler` and
throwing the bad_alloc exception on failure. If we compile
with a C++ compiler we can implement this precisely. If we
use a C compiler we cannot throw a `bad_alloc` exception
but we call `exit` instead (i.e. not returning).
-------------------------------------------------------*/

#ifdef __cplusplus
#include <new>
static bool mi_try_new_handler(bool nothrow) {
  #if defined(_MSC_VER) || (__cplusplus >= 201103L)
    std::new_handler h = std::get_new_handler();
  #else
    std::new_handler h = std::set_new_handler();
    std::set_new_handler(h);
  #endif  
  if (h==NULL) {
    _mi_error_message(ENOMEM, "out of memory in 'new'");      
    if (!nothrow) {
      throw std::bad_alloc();
    }
    return false;
  }
  else {
    h();
    return true;
  }
}
#else
typedef void (*std_new_handler_t)(void);

#if (defined(__GNUC__) || (defined(__clang__) && !defined(_MSC_VER)))  // exclude clang-cl, see issue #631
std_new_handler_t __attribute__((weak)) _ZSt15get_new_handlerv(void) {
  return NULL;
}
static std_new_handler_t mi_get_new_handler(void) {
  return _ZSt15get_new_handlerv();
}
#else
// note: on windows we could dynamically link to `?get_new_handler@std@@YAP6AXXZXZ`.
static std_new_handler_t mi_get_new_handler() {
  return NULL;
}
#endif

static bool mi_try_new_handler(bool nothrow) {
  std_new_handler_t h = mi_get_new_handler();
  if (h==NULL) {
    _mi_error_message(ENOMEM, "out of memory in 'new'");       
    if (!nothrow) {
      abort();  // cannot throw in plain C, use abort
    }
    return false;
  }
  else {
    h();
    return true;
  }
}
#endif

static mi_decl_noinline void* mi_try_new(size_t size, bool nothrow ) {
  void* p = NULL;
  while(p == NULL && mi_try_new_handler(nothrow)) {
    p = mi_malloc(size);
  }
  return p;
}

mi_decl_nodiscard mi_decl_restrict void* mi_new(size_t size) {
  void* p = mi_malloc(size);
  if mi_unlikely(p == NULL) return mi_try_new(size,false);
  return p;
}

mi_decl_nodiscard mi_decl_restrict void* mi_new_nothrow(size_t size) mi_attr_noexcept {
  void* p = mi_malloc(size);
  if mi_unlikely(p == NULL) return mi_try_new(size, true);
  return p;
}

mi_decl_nodiscard mi_decl_restrict void* mi_new_aligned(size_t size, size_t alignment) {
  void* p;
  do {
    p = mi_malloc_aligned(size, alignment);
  }
  while(p == NULL && mi_try_new_handler(false));
  return p;
}

mi_decl_nodiscard mi_decl_restrict void* mi_new_aligned_nothrow(size_t size, size_t alignment) mi_attr_noexcept {
  void* p;
  do {
    p = mi_malloc_aligned(size, alignment);
  }
  while(p == NULL && mi_try_new_handler(true));
  return p;
}

mi_decl_nodiscard mi_decl_restrict void* mi_new_n(size_t count, size_t size) {
  size_t total;
  if mi_unlikely(mi_count_size_overflow(count, size, &total)) {
    mi_try_new_handler(false);  // on overflow we invoke the try_new_handler once to potentially throw std::bad_alloc
    return NULL;
  }
  else {
    return mi_new(total);
  }
}

mi_decl_nodiscard void* mi_new_realloc(void* p, size_t newsize) {
  void* q;
  do {
    q = mi_realloc(p, newsize);
  } while (q == NULL && mi_try_new_handler(false));
  return q;
}

mi_decl_nodiscard void* mi_new_reallocn(void* p, size_t newcount, size_t size) {
  size_t total;
  if mi_unlikely(mi_count_size_overflow(newcount, size, &total)) {
    mi_try_new_handler(false);  // on overflow we invoke the try_new_handler once to potentially throw std::bad_alloc
    return NULL;
  }
  else {
    return mi_new_realloc(p, total);
  }
}
