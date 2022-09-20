// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_ARITH_RANGE_PROOF_ENRICHED_PROOF_H
#define NAVCOIN_BLSCT_ARITH_RANGE_PROOF_ENRICHED_PROOF_H

#include <blsct/arith/elements.h>
#include <blsct/arith/scalar.h>
#include <blsct/arith/range_proof/proof.h>

class EnrichedProof
{
public:
    EnrichedProof(
        const Proof& proof,
        const Scalar& x,
        const Scalar& y,
        const Scalar& z,
        const Scalar& x_ip,
        const Scalars& ws,
        const size_t& log_m,
        const size_t& inv_offset
    ): proof{proof}, x{x}, y{y}, z{z}, x_ip{x_ip}, ws{ws},
        log_m{log_m}, inv_offset{inv_offset} {}

    static EnrichedProof Build(
      const Proof& proof,
      const size_t& inv_offset,
      const size_t& rounds,   // TODO move to Config?
      const size_t& log_m
    );

    const Proof proof;
    const Scalar x;
    const Scalar y;
    const Scalar z;
    const Scalar x_ip;
    const Scalars ws;
    const size_t log_m;
    const size_t inv_offset;
};

#endif // NAVCOIN_BLSCT_ARITH_RANGE_PROOF_ENRICHED_PROOF_H