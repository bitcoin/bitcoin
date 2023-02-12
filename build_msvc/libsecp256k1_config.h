/**********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                             *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef BITCOIN_LIBSECP256K1_CONFIG_H
#define BITCOIN_LIBSECP256K1_CONFIG_H

#undef USE_ASM_X86_64

#define ECMULT_GEN_PREC_BITS 4
#define ECMULT_WINDOW_SIZE 15

#endif // BITCOIN_LIBSECP256K1_CONFIG_H
