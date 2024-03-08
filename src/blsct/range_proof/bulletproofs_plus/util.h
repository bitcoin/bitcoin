// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_RANGE_PROOF_BULLETPROOFS_PLUS_UTIL_H
#define NAVCOIN_BLSCT_RANGE_PROOF_BULLETPROOFS_PLUS_UTIL_H

#include <blsct/arith/elements.h>
#include <blsct/arith/mcl/mcl.h>
#include <blsct/arith/mcl/mcl_scalar.h>
#include <blsct/building_block/fiat_shamir.h>
#include <hash.h>

namespace bulletproofs_plus {

template <typename T>
struct Util {
    static Elements<typename T::Scalar> GetYPows(
        const typename T::Scalar& y,
        const size_t& n,
        HashWriter& fiat_shamir
    );
};

} // namespace bulletproofs_plus

#endif // NAVCOIN_BLSCT_RANGE_PROOF_BULLETPROOFS_PLUS_UTIL_H
