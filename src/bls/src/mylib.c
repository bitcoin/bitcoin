#include <stdlib.h>

#ifdef __wasm__
	#define BLS_DLL_API __attribute__((visibility("default")))
#else
	#define BLS_DLL_API
#endif

BLS_DLL_API int strcmp(const char *s, const char *t)
{
	for (;;) {
		char c1 = *s++;
		char c2 = *t++;
		if (c1 < c2) return -1;
		if (c1 > c2) return 1;
		if (c1 == '\0') return 0;
	}
}

BLS_DLL_API void* memset(void *p, int v, size_t n)
{
	char *s = (char*)p;
	for (size_t i = 0; i < n; i++) {
		s[i] = (char)v;
	}
	return p;
}

BLS_DLL_API void* memcpy(void *dst, const void *src, size_t n)
{
	char *d = (char *)dst;
	const char *s = (const char *)src;
	for (size_t i = 0; i < n; i++) {
		d[i] = s[i];
	}
	return dst;
}

BLS_DLL_API size_t strlen(const char *s)
{
	size_t n = 0;
	for (;;) {
		if (s[n] == '\0') return n;
		n++;
	}
}

BLS_DLL_API int atexit(void (*f)(void))
{
	(void)f;
	return 0;
}

BLS_DLL_API size_t getEndOffset()
{
	static char end[] = "DATA_END";
	return (size_t)end;
}

// to get stack address (temporary)
BLS_DLL_API size_t getStackAddress(int x)
{
	if (x == 0) x = 32;
	char buf[x];
	return (size_t)buf;
}
