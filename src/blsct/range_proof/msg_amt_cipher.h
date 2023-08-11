// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_RANGE_PROOF_RANGE_PROOF_MSG_AMT_CIPHER_H
#define NAVCOIN_BLSCT_RANGE_PROOF_RANGE_PROOF_MSG_AMT_CIPHER_H

#include <blsct/arith/elements.h>

#include <cstdint>
#include <optional>
#include <vector>

namespace range_proof {

struct MsgAmt
{
    std::string msg;
    int64_t amount;

    static MsgAmt of(const std::string& msg, const int64_t& amount);
};

template <typename T>
struct MsgAmtCipher
{
    using Point = typename T::Point;
    using Scalar = typename T::Scalar;
    using Scalars = Elements<typename T::Scalar>;

    // msg1: first 23 bytes of the msg
    // msg2: remaining part of the msg after msg1 if the message is longer than 23-byte long. msg2 can be empty.

    static Scalar RetrieveMsg2(
        const std::vector<uint8_t>& msg
    );

    static Scalar ComputeAlpha(
        const std::vector<uint8_t>& msg,
        const Scalar& vs0,
        const Scalar& nonce_alpha
    );

    static Scalar ComputeTauX(
        const std::vector<uint8_t>& msg,
        const Scalar& x,
        const Scalar& z,
        const Scalar& tau1,
        const Scalar& tau2,
        const Scalars& z_pows_from_2,
        const Scalars& gammas
    );

    static std::optional<MsgAmt> Decrypt(
        const Scalar& msg1_vs0,
        const Scalar& gamma_vs0,
        const Scalar& tau1,
        const Scalar& tau2,
        const Scalar& tau_x,
        const Scalar& x,
        const Scalar& z,
        const Scalar& uint64_max,
        const Point& H,
        const Point& G,
        const Point& exp_vs0_commitment
    );
};

} // namespace range_proof

#endif // NAVCOIN_BLSCT_RANGE_PROOF_RANGE_PROOF_MSG_AMT_CIPHER_H
