/**********************************************************************
 * Copyright (c) 2016 Andrew Poelstra                                 *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _SECP256K1_SURJECTION_H_
#define _SECP256K1_SURJECTION_H_

#include "group.h"
#include "scalar.h"

SECP256K1_INLINE static int secp256k1_surjection_genmessage(unsigned char *msg32, secp256k1_ge *ephemeral_input_tags, size_t n_input_tags, secp256k1_ge *ephemeral_output_tag);

SECP256K1_INLINE static int secp256k1_surjection_genrand(secp256k1_scalar *s, size_t ns, const secp256k1_scalar *blinding_key);

SECP256K1_INLINE static int secp256k1_surjection_compute_public_keys(secp256k1_gej *pubkeys, size_t n_pubkeys, const secp256k1_ge *input_tags, size_t n_input_tags, const unsigned char *used_tags, const secp256k1_ge *output_tag, size_t input_index, size_t *ring_input_index);

#endif
