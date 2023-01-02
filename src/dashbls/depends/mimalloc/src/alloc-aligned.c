/* ----------------------------------------------------------------------------
Copyright (c) 2018-2021, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/

#include "mimalloc.h"
#include "mimalloc-internal.h"

#include <string.h>  // memset

// ------------------------------------------------------
// Aligned Allocation
// ------------------------------------------------------

// Fallback primitive aligned allocation -- split out for better codegen
static mi_decl_noinline void* mi_heap_malloc_zero_aligned_at_fallback(mi_heap_t* const heap, const size_t size, const size_t alignment, const size_t offset, const bool zero) mi_attr_noexcept
{
  mi_assert_internal(size <= PTRDIFF_MAX);
  mi_assert_internal(alignment!=0 && _mi_is_power_of_two(alignment) && alignment <= MI_ALIGNMENT_MAX);

  const uintptr_t align_mask = alignment-1;  // for any x, `(x & align_mask) == (x % alignment)`
  const size_t padsize = size + MI_PADDING_SIZE;

  // use regular allocation if it is guaranteed to fit the alignment constraints
  if (offset==0 && alignment<=padsize && padsize<=MI_MAX_ALIGN_GUARANTEE && (padsize&align_mask)==0) {
    void* p = _mi_heap_malloc_zero(heap, size, zero);
    mi_assert_internal(p == NULL || ((uintptr_t)p % alignment) == 0);
    return p;
  }

  // otherwise over-allocate
  const size_t oversize = size + alignment - 1;
  void* p = _mi_heap_malloc_zero(heap, oversize, zero);
  if (p == NULL) return NULL;

  // .. and align within the allocation
  uintptr_t adjust = alignment - (((uintptr_t)p + offset) & align_mask);
  mi_assert_internal(adjust <= alignment);
  void* aligned_p = (adjust == alignment ? p : (void*)((uintptr_t)p + adjust));
  if (aligned_p != p) mi_page_set_has_aligned(_mi_ptr_page(p), true);
  mi_assert_internal(((uintptr_t)aligned_p + offset) % alignment == 0);
  mi_assert_internal(p == _mi_page_ptr_unalign(_mi_ptr_segment(aligned_p), _mi_ptr_page(aligned_p), aligned_p));

  #if MI_TRACK_ENABLED
  if (p != aligned_p) {
    mi_track_free(p);
    mi_track_malloc(aligned_p,size,zero);
  }
  else {
    mi_track_resize(aligned_p,oversize,size);
  }
  #endif
  return aligned_p;
}

// Primitive aligned allocation
static void* mi_heap_malloc_zero_aligned_at(mi_heap_t* const heap, const size_t size, const size_t alignment, const size_t offset, const bool zero) mi_attr_noexcept
{
  // note: we don't require `size > offset`, we just guarantee that the address at offset is aligned regardless of the allocated size.
  mi_assert(alignment > 0);
  if mi_unlikely(alignment==0 || !_mi_is_power_of_two(alignment)) { // require power-of-two (see <https://en.cppreference.com/w/c/memory/aligned_alloc>)
    #if MI_DEBUG > 0
    _mi_error_message(EOVERFLOW, "aligned allocation requires the alignment to be a power-of-two (size %zu, alignment %zu)\n", size, alignment);
    #endif
    return NULL;
  }
  if mi_unlikely(alignment > MI_ALIGNMENT_MAX) {  // we cannot align at a boundary larger than this (or otherwise we cannot find segment headers)
    #if MI_DEBUG > 0
    _mi_error_message(EOVERFLOW, "aligned allocation has a maximum alignment of %zu (size %zu, alignment %zu)\n", MI_ALIGNMENT_MAX, size, alignment);
    #endif
    return NULL;
  }
  if mi_unlikely(size > PTRDIFF_MAX) {          // we don't allocate more than PTRDIFF_MAX (see <https://sourceware.org/ml/libc-announce/2019/msg00001.html>)                                                    
    #if MI_DEBUG > 0
    _mi_error_message(EOVERFLOW, "aligned allocation request is too large (size %zu, alignment %zu)\n", size, alignment);
    #endif
    return NULL;
  }
  const uintptr_t align_mask = alignment-1;       // for any x, `(x & align_mask) == (x % alignment)`
  const size_t padsize = size + MI_PADDING_SIZE;  // note: cannot overflow due to earlier size > PTRDIFF_MAX check

  // try first if there happens to be a small block available with just the right alignment
  if mi_likely(padsize <= MI_SMALL_SIZE_MAX) {
    mi_page_t* page = _mi_heap_get_free_small_page(heap, padsize);
    const bool is_aligned = (((uintptr_t)page->free+offset) & align_mask)==0;
    if mi_likely(page->free != NULL && is_aligned)
    {
      #if MI_STAT>1
      mi_heap_stat_increase(heap, malloc, size);
      #endif
      void* p = _mi_page_malloc(heap, page, padsize, zero); // TODO: inline _mi_page_malloc
      mi_assert_internal(p != NULL);
      mi_assert_internal(((uintptr_t)p + offset) % alignment == 0);
      mi_track_malloc(p,size,zero);
      return p;
    }
  }
  // fallback
  return mi_heap_malloc_zero_aligned_at_fallback(heap, size, alignment, offset, zero);
}


// ------------------------------------------------------
// Optimized mi_heap_malloc_aligned / mi_malloc_aligned
// ------------------------------------------------------

mi_decl_nodiscard mi_decl_restrict void* mi_heap_malloc_aligned_at(mi_heap_t* heap, size_t size, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_heap_malloc_zero_aligned_at(heap, size, alignment, offset, false);
}

mi_decl_nodiscard mi_decl_restrict void* mi_heap_malloc_aligned(mi_heap_t* heap, size_t size, size_t alignment) mi_attr_noexcept {
  #if !MI_PADDING
  // without padding, any small sized allocation is naturally aligned (see also `_mi_segment_page_start`)
  if (!_mi_is_power_of_two(alignment)) return NULL;
  if mi_likely(_mi_is_power_of_two(size) && size >= alignment && size <= MI_SMALL_SIZE_MAX)
  #else
  // with padding, we can only guarantee this for fixed alignments
  if mi_likely((alignment == sizeof(void*) || (alignment == MI_MAX_ALIGN_SIZE && size > (MI_MAX_ALIGN_SIZE/2)))
                && size <= MI_SMALL_SIZE_MAX)
  #endif
  {
    // fast path for common alignment and size
    return mi_heap_malloc_small(heap, size);
  }
  else {
    return mi_heap_malloc_aligned_at(heap, size, alignment, 0);
  }
}

// ------------------------------------------------------
// Aligned Allocation
// ------------------------------------------------------

mi_decl_nodiscard mi_decl_restrict void* mi_heap_zalloc_aligned_at(mi_heap_t* heap, size_t size, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_heap_malloc_zero_aligned_at(heap, size, alignment, offset, true);
}

mi_decl_nodiscard mi_decl_restrict void* mi_heap_zalloc_aligned(mi_heap_t* heap, size_t size, size_t alignment) mi_attr_noexcept {
  return mi_heap_zalloc_aligned_at(heap, size, alignment, 0);
}

mi_decl_nodiscard mi_decl_restrict void* mi_heap_calloc_aligned_at(mi_heap_t* heap, size_t count, size_t size, size_t alignment, size_t offset) mi_attr_noexcept {
  size_t total;
  if (mi_count_size_overflow(count, size, &total)) return NULL;
  return mi_heap_zalloc_aligned_at(heap, total, alignment, offset);
}

mi_decl_nodiscard mi_decl_restrict void* mi_heap_calloc_aligned(mi_heap_t* heap, size_t count, size_t size, size_t alignment) mi_attr_noexcept {
  return mi_heap_calloc_aligned_at(heap,count,size,alignment,0);
}

mi_decl_nodiscard mi_decl_restrict void* mi_malloc_aligned_at(size_t size, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_heap_malloc_aligned_at(mi_get_default_heap(), size, alignment, offset);
}

mi_decl_nodiscard mi_decl_restrict void* mi_malloc_aligned(size_t size, size_t alignment) mi_attr_noexcept {
  return mi_heap_malloc_aligned(mi_get_default_heap(), size, alignment);
}

mi_decl_nodiscard mi_decl_restrict void* mi_zalloc_aligned_at(size_t size, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_heap_zalloc_aligned_at(mi_get_default_heap(), size, alignment, offset);
}

mi_decl_nodiscard mi_decl_restrict void* mi_zalloc_aligned(size_t size, size_t alignment) mi_attr_noexcept {
  return mi_heap_zalloc_aligned(mi_get_default_heap(), size, alignment);
}

mi_decl_nodiscard mi_decl_restrict void* mi_calloc_aligned_at(size_t count, size_t size, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_heap_calloc_aligned_at(mi_get_default_heap(), count, size, alignment, offset);
}

mi_decl_nodiscard mi_decl_restrict void* mi_calloc_aligned(size_t count, size_t size, size_t alignment) mi_attr_noexcept {
  return mi_heap_calloc_aligned(mi_get_default_heap(), count, size, alignment);
}


// ------------------------------------------------------
// Aligned re-allocation
// ------------------------------------------------------

static void* mi_heap_realloc_zero_aligned_at(mi_heap_t* heap, void* p, size_t newsize, size_t alignment, size_t offset, bool zero) mi_attr_noexcept {
  mi_assert(alignment > 0);
  if (alignment <= sizeof(uintptr_t)) return _mi_heap_realloc_zero(heap,p,newsize,zero);
  if (p == NULL) return mi_heap_malloc_zero_aligned_at(heap,newsize,alignment,offset,zero);
  size_t size = mi_usable_size(p);
  if (newsize <= size && newsize >= (size - (size / 2))
      && (((uintptr_t)p + offset) % alignment) == 0) {
    return p;  // reallocation still fits, is aligned and not more than 50% waste
  }
  else {
    void* newp = mi_heap_malloc_aligned_at(heap,newsize,alignment,offset);
    if (newp != NULL) {
      if (zero && newsize > size) {
        const mi_page_t* page = _mi_ptr_page(newp);
        if (page->is_zero) {
          // already zero initialized
          mi_assert_expensive(mi_mem_is_zero(newp,newsize));
        }
        else {
          // also set last word in the previous allocation to zero to ensure any padding is zero-initialized
          size_t start = (size >= sizeof(intptr_t) ? size - sizeof(intptr_t) : 0);
          memset((uint8_t*)newp + start, 0, newsize - start);
        }
      }
      _mi_memcpy_aligned(newp, p, (newsize > size ? size : newsize));
      mi_free(p); // only free if successful
    }
    return newp;
  }
}

static void* mi_heap_realloc_zero_aligned(mi_heap_t* heap, void* p, size_t newsize, size_t alignment, bool zero) mi_attr_noexcept {
  mi_assert(alignment > 0);
  if (alignment <= sizeof(uintptr_t)) return _mi_heap_realloc_zero(heap,p,newsize,zero);
  size_t offset = ((uintptr_t)p % alignment); // use offset of previous allocation (p can be NULL)
  return mi_heap_realloc_zero_aligned_at(heap,p,newsize,alignment,offset,zero);
}

mi_decl_nodiscard void* mi_heap_realloc_aligned_at(mi_heap_t* heap, void* p, size_t newsize, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_heap_realloc_zero_aligned_at(heap,p,newsize,alignment,offset,false);
}

mi_decl_nodiscard void* mi_heap_realloc_aligned(mi_heap_t* heap, void* p, size_t newsize, size_t alignment) mi_attr_noexcept {
  return mi_heap_realloc_zero_aligned(heap,p,newsize,alignment,false);
}

mi_decl_nodiscard void* mi_heap_rezalloc_aligned_at(mi_heap_t* heap, void* p, size_t newsize, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_heap_realloc_zero_aligned_at(heap, p, newsize, alignment, offset, true);
}

mi_decl_nodiscard void* mi_heap_rezalloc_aligned(mi_heap_t* heap, void* p, size_t newsize, size_t alignment) mi_attr_noexcept {
  return mi_heap_realloc_zero_aligned(heap, p, newsize, alignment, true);
}

mi_decl_nodiscard void* mi_heap_recalloc_aligned_at(mi_heap_t* heap, void* p, size_t newcount, size_t size, size_t alignment, size_t offset) mi_attr_noexcept {
  size_t total;
  if (mi_count_size_overflow(newcount, size, &total)) return NULL;
  return mi_heap_rezalloc_aligned_at(heap, p, total, alignment, offset);
}

mi_decl_nodiscard void* mi_heap_recalloc_aligned(mi_heap_t* heap, void* p, size_t newcount, size_t size, size_t alignment) mi_attr_noexcept {
  size_t total;
  if (mi_count_size_overflow(newcount, size, &total)) return NULL;
  return mi_heap_rezalloc_aligned(heap, p, total, alignment);
}

mi_decl_nodiscard void* mi_realloc_aligned_at(void* p, size_t newsize, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_heap_realloc_aligned_at(mi_get_default_heap(), p, newsize, alignment, offset);
}

mi_decl_nodiscard void* mi_realloc_aligned(void* p, size_t newsize, size_t alignment) mi_attr_noexcept {
  return mi_heap_realloc_aligned(mi_get_default_heap(), p, newsize, alignment);
}

mi_decl_nodiscard void* mi_rezalloc_aligned_at(void* p, size_t newsize, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_heap_rezalloc_aligned_at(mi_get_default_heap(), p, newsize, alignment, offset);
}

mi_decl_nodiscard void* mi_rezalloc_aligned(void* p, size_t newsize, size_t alignment) mi_attr_noexcept {
  return mi_heap_rezalloc_aligned(mi_get_default_heap(), p, newsize, alignment);
}

mi_decl_nodiscard void* mi_recalloc_aligned_at(void* p, size_t newcount, size_t size, size_t alignment, size_t offset) mi_attr_noexcept {
  return mi_heap_recalloc_aligned_at(mi_get_default_heap(), p, newcount, size, alignment, offset);
}

mi_decl_nodiscard void* mi_recalloc_aligned(void* p, size_t newcount, size_t size, size_t alignment) mi_attr_noexcept {
  return mi_heap_recalloc_aligned(mi_get_default_heap(), p, newcount, size, alignment);
}

