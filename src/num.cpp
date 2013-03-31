#if defined(USE_NUM_GMP)
#include "num_gmp.cpp"
#elif defined(USE_NUM_OPENSSL)
#include "num_openssl.cpp"
#else
#error "Please select num implementation"
#endif
