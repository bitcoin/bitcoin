#ifdef __SSE2__
#include "yescrypt-simd.c"
#else
#include "yescrypt-opt.c"
#endif
