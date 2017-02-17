#ifndef ALIGNED_MALLOC_H
#define ALIGNED_MALLOC_H

#if (__x86_64__)
#include <mm_malloc.h>

inline void* aligned_malloc(size_t size) {
	return _mm_malloc(size, 32);
}

inline void aligned_free(void* ptr) {
	_mm_free(ptr);
}

#else

inline void* aligned_malloc(size_t size) {
	return malloc(size);
}

inline void aligned_free(void* ptr) {
	return free(ptr);
}

#endif


#endif // ALIGNED_MALLOC_H
