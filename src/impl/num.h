// Copyright (c) 2013 Pieter Wuille
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef _SECP256K1_NUM_IMPL_H_
#define _SECP256K1_NUM_IMPL_H_

#include "../num.h"

#if defined(USE_NUM_GMP)
#include "num_gmp.h"
#elif defined(USE_NUM_OPENSSL)
#include "num_openssl.h"
#else
#error "Please select num implementation"
#endif

#endif
