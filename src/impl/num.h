#ifndef _SECP256K1_NUM_IMPL_H_
#define _SECP256K1_NUM_IMPL_H_

#include "../num.h"

#if defined(USE_NUM_GMPN)
#include "num_gmpn.h"
#elif defined(USE_NUM_GMP)
#include "num_gmp.h"
#elif defined(USE_NUM_OPENSSL)
#include "num_openssl.h"
#else
#error "Please select num implementation"
#endif

#endif
