#ifndef _TYPES_H_
#define _TYPES_H_

#undef unlikely
#undef likely
#if defined(__GNUC__) && (__GNUC__ > 2) && defined(__OPTIMIZE__)
#define unlikely(expr) (__builtin_expect(!!(expr), 0))
#define likely(expr) (__builtin_expect(!!(expr), 1))
#else
#define unlikely(expr) (expr)
#define likely(expr) (expr)
#endif

typedef unsigned char BitSequence;
typedef unsigned long long DataLength;
typedef enum {SUCCESS = 0, FAIL = 1, BAD_HASHLEN = 2} HashReturn;

#endif
