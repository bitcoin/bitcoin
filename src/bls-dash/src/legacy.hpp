// Copyright (c) 2021 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SRC_LEGACY_HPP_
#define SRC_LEGACY_HPP_

#include <ios>

#include <relic_conf.h>

#if defined GMP && ARITH == GMP
#include <gmp.h>
#endif

extern "C" {
#include <relic.h>
}

/**
 * Maps a byte array to a point in an elliptic curve over a quadratic extension.
 *
 * Called ep2_map() in old relic version, _legacy prefix is to avoid duplicated symbols
 *
 * @param[out] p			- the result.
 * @param[in] msg			- the byte array to map.
 * @param[in] len			- the array length in bytes.
 */
void ep2_map_legacy(ep2_t p, const uint8_t *msg, int len);

#endif  // #define SRC_LEGACY_HPP_
