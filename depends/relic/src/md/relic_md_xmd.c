/*
 * RELIC is an Efficient LIbrary for Cryptography
 * Copyright (c) 2020 RELIC Authors
 *
 * This file is part of RELIC. RELIC is legal property of its developers,
 * whose names are not listed here. Please refer to the COPYRIGHT file
 * for contact information.
 *
 * RELIC is free software; you can redistribute it and/or modify it under the
 * terms of the version 2.1 (or later) of the GNU Lesser General Public License
 * as published by the Free Software Foundation; or version 2.0 of the Apache
 * License as published by the Apache Software Foundation. See the LICENSE files
 * for more details.
 *
 * RELIC is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the LICENSE files for more details.
 *
 * You should have received a copy of the GNU Lesser General Public or the
 * Apache License along with RELIC. If not, see <https://www.gnu.org/licenses/>
 * or <https://www.apache.org/licenses/>.
 */

/**
 * @file
 *
 * Implementation of an extensible-output function from a Merkle-Damgaard hash.
 *
 * @ingroup md
 */

#include <string.h>

#include "relic_conf.h"
#include "relic_core.h"
#include "sha.h"

/*============================================================================*/
/* Private definitions                                                        */
/*============================================================================*/

/**
 * Helper: check result of a SHA call.
 */
#define _check_md(EXPR)                                                        \
	do {                                                                       \
		if ((EXPR) != shaSuccess) {                                            \
			RLC_THROW(ERR_NO_VALID);                                           \
			return;                                                            \
		}                                                                      \
	} while (0)

/**
 * Construct an extensible-output function based on HASH, which must be
 * one of SHA224, SHA256, SHA384, or SHA512.
 */
#define make_md_xmd(HASH, HNAME)                                               \
	_make_md_xmd(md_xmd_##HNAME, HASH##_Message_Block_Size, HASH##HashSize,    \
				 HASH##Context, HASH##Reset, HASH##Input, HASH##Result)

/**
 * Helper for make_md_xmd
 */
#define _make_md_xmd(HName, HBlockSize, HHashSize, HContext, HReset, HInput, HResult)                             \
	void HName(uint8_t *buf, int buf_len, const uint8_t *in, int in_len, const uint8_t *dst, int dst_len) {       \
		const unsigned ell = (buf_len + HHashSize - 1) / HHashSize;                                               \
		if (buf_len < 0 || ell > 255 || dst_len > 255) {                                                          \
			RLC_THROW(ERR_NO_VALID);                                                                              \
			return;                                                                                               \
		}                                                                                                         \
                                                                                                                  \
		/* info needed for hashing: zero padding and some lengths */                                              \
		const uint8_t Z_pad[HBlockSize] = {                                                                       \
			0,                                                                                                    \
		};                                                                                                        \
		const uint8_t l_i_b_0_str[] = {buf_len >> 8, buf_len & 0xff, 0, dst_len};                                 \
		const uint8_t *dstlen_str = l_i_b_0_str + 3;                                                              \
                                                                                                                  \
		/* now compute b_0 */                                                                                     \
		uint8_t b_0[HHashSize];                                                                                   \
		HContext ctx;                                                                                             \
		_check_md(HReset(&ctx));                                                                                  \
		_check_md(HInput(&ctx, Z_pad, HBlockSize)); /* Z_pad */                                                   \
		_check_md(HInput(&ctx, in, in_len));        /* msg */                                                     \
		_check_md(HInput(&ctx, l_i_b_0_str, 3));    /* l_i_b_str || I2OSP(0, 1) */                                \
		_check_md(HInput(&ctx, dst, dst_len));      /* DST */                                                     \
		_check_md(HInput(&ctx, dstlen_str, 1));     /* I2OSP(len(dst), 1) */                                      \
		_check_md(HResult(&ctx, b_0));              /* finalize computation */                                    \
                                                                                                                  \
		/* now compute b_i */                                                                                     \
		uint8_t b_i[HHashSize + 1] = {                                                                            \
			0,                                                                                                    \
		};                                                                                                        \
		for (unsigned i = 1; i <= ell; ++i) {                                                                     \
			/* compute b_0 XOR b_(i-1) */                                                                         \
			for (unsigned j = 0; j < HHashSize; ++j) {                                                            \
				b_i[j] = b_0[j] ^ b_i[j];                                                                         \
			}                                                                                                     \
			b_i[HHashSize] = i;                                                                                   \
                                                                                                                  \
			_check_md(HReset(&ctx));                                                                              \
			_check_md(HInput(&ctx, b_i, HHashSize + 1)); /* b_0 ^ b_(i-1) || I2OSP(i, 1) */                       \
			_check_md(HInput(&ctx, dst, dst_len));       /* DST */                                                \
			_check_md(HInput(&ctx, dstlen_str, 1));      /* I2OSP(len(dst), 1) */                                 \
			_check_md(HResult(&ctx, b_i));               /* finalize computation */                               \
                                                                                                                  \
			/* copy into output buffer */                                                                         \
			const int rem_after = buf_len - i * HHashSize;                                                        \
			const int copy_len = HHashSize + (rem_after < 0 ? rem_after : 0);                                     \
			memcpy(buf + (i - 1) * HHashSize, b_i, copy_len);                                                     \
		}                                                                                                         \
	}

/*============================================================================*/
/* Public definitions                                                         */
/*============================================================================*/

#if MD_MAP == SH224 || !defined(STRIP)
make_md_xmd(SHA224, sh224)
#endif

#if MD_MAP == SH256 || !defined(STRIP)
make_md_xmd(SHA256, sh256)
#endif

#if MD_MAP == SH384 || !defined(STRIP)
make_md_xmd(SHA384, sh384)
#endif

#if MD_MAP == SH512 || !defined(STRIP)
make_md_xmd(SHA512, sh512)
#endif
