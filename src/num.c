#if defined(USE_NUM_GMP)
#include "num_gmp.c"
#elif defined(USE_NUM_OPENSSL)
#include "num_openssl.c"
#else
#error "Please select num implementation"
#endif
