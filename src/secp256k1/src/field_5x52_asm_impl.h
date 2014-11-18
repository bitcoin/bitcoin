/**********************************************************************
 * Copyright (c) 2013 Pieter Wuille                                   *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _SECP256K1_FIELD_INNER5X52_IMPL_H_
#define _SECP256K1_FIELD_INNER5X52_IMPL_H_

void __attribute__ ((sysv_abi)) secp256k1_fe_mul_inner(const uint64_t *a, const uint64_t *b, uint64_t *r);
void __attribute__ ((sysv_abi)) secp256k1_fe_sqr_inner(const uint64_t *a, uint64_t *r);

#endif
