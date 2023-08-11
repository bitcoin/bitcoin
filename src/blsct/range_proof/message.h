// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_RANGE_PROOF_RANGE_PROOF_MESSAGE_H
#define NAVCOIN_BLSCT_RANGE_PROOF_RANGE_PROOF_MESSAGE_H

#include <blsct/arith/elements.h>

#include <cstdint>
#include <optional>
#include <string>

namespace range_proof {

struct MsgWithAmt
{
    std::string msg;
    int64_t amount;

    static MsgWithAmt of(const std::string& msg, const int64_t& amount);
};

template <typename T>
struct Message
{
    using Point = typename T::Point;
    using Scalar = typename T::Scalar;
    using Scalars = Elements<typename T::Scalar>;

    static Scalar ComputeAlpha(
        const std::string& msg,
        const Scalar& vs0,
        const Point& nonce
    );

    static Scalar ComputeTauX(
        const std::vector<uint8_t>& msg,
        const Scalar& x,
        const Scalar& z,
        const Point& nonce
    );

    static std::optional<MsgWithAmt> Recover(
        const Scalar& msg1_vs0,
        const Scalar& tau_x,
        const Scalar& x,
        const Scalar& z,
        const Scalar& uint64_max,
        const Point& H,
        const Point& G,
        const Point& exp_vs0_commitment,
        const Point& nonce
    );
};

} // namespace range_proof

#endif // NAVCOIN_BLSCT_RANGE_PROOF_RANGE_PROOF_MESSAGE_H
