#ifndef _SECP256K1_NUM_
#define _SECP256K1_NUM_

#if defined(USE_NUM_GMP)
#include "num_gmp.h"
#elif defined(USE_NUM_OPENSSL)
#include "num_openssl.h"
#else
#error "Please select num implementation"
#endif

#endif
