// Copyright (c) 2013 Pieter Wuille
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef _SECP256K1_NUM_IMPL_H_
#define _SECP256K1_NUM_IMPL_H_

#if defined HAVE_CONFIG_H
#include "libsecp256k1-config.h"
#endif

#include "num.h"

#if defined(USE_NUM_GMP)
#include "num_gmp_impl.h"
#elif defined(USE_NUM_OPENSSL)
#include "num_openssl_impl.h"
#else
#error "Please select num implementation"
#endif

#endif
