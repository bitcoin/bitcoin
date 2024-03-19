// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/mcl/mcl.h>
#include <blsct/range_proof/msg_amt_cipher.h>
#include <blsct/range_proof/setup.h>
#include <iostream>
#include <string>
#include <util/strencodings.h>

namespace range_proof {

MsgAmt MsgAmt::of(const std::string& msg, const int64_t& amount)
{
    return MsgAmt {msg, amount};
}

template <typename T>
typename T::Scalar MsgAmtCipher<T>::RetrieveMsg2(
    const std::vector<uint8_t>& msg
) {
    if (msg.size() > range_proof::Setup::message_1_max_size) {
        auto vec = std::vector<uint8_t>(
            msg.begin() + range_proof::Setup::message_1_max_size,
            msg.end()
        );
        return Scalar(vec);

    } else {
        return Scalar(0);
    }
}
template
Mcl::Scalar MsgAmtCipher<Mcl>::RetrieveMsg2(
    const std::vector<uint8_t>& msg
);

template <typename T>
typename T::Scalar MsgAmtCipher<T>::ComputeAlpha(
    const std::vector<uint8_t>& msg,
    const Scalar& vs0,
    const Scalar& nonce_alpha
) {
    // extract msg1
    Scalar msg1(msg.size() > range_proof::Setup::message_1_max_size ?
        std::vector<uint8_t>(msg.begin(), msg.begin() + range_proof::Setup::message_1_max_size) :
        msg
    );

    // msg1 followed by 64-bit vs0
    Scalar msg1_vs0 = (msg1 << range_proof::Setup::num_input_value_bits) | vs0;

    Scalar alpha = nonce_alpha + msg1_vs0;

    return alpha;
}
template Mcl::Scalar MsgAmtCipher<Mcl>::ComputeAlpha(
    const std::vector<uint8_t>& msg,
    const Mcl::Scalar& vs0,
    const Mcl::Scalar& nonce_alpha
);

template <typename T>
typename T::Scalar MsgAmtCipher<T>::ComputeTauX(
    const std::vector<uint8_t>& msg,
    const Scalar& x,
    const Scalar& z,
    const Scalar& tau1,
    const Scalar& tau2,
    const Scalars& z_pows_from_2,
    const Scalars& gammas
) {
    Scalar msg2 = RetrieveMsg2(msg);

    Scalar tau_x =
        (tau2 * x.Square())
        + ((tau1 + msg2) * x)
        + (z_pows_from_2 * gammas).Sum()
        ;

    return tau_x;
}
template Mcl::Scalar MsgAmtCipher<Mcl>::ComputeTauX(
    const std::vector<uint8_t>& msg,
    const Scalar& x,
    const Scalar& z,
    const Scalar& tau1,
    const Scalar& tau2,
    const Scalars& z_pows_from_2,
    const Scalars& gammas
);

template <typename T>
std::optional<MsgAmt> MsgAmtCipher<T>::Decrypt(
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
) {
    // lower 64 bits of msg1_vs0 is vs0
    Scalar vs0 = msg1_vs0 & uint64_max;

    // failure if commitment created from recoverted amount doesn't match
    Point act_vs0_commitment = (H * gamma_vs0) + (G * vs0);
    if (act_vs0_commitment != exp_vs0_commitment) {
        return std::nullopt;
    }

    // msg1 starts from the 65th bit
    std::vector<uint8_t> msg1 = (msg1_vs0 >> 64).GetVch(true);

    // tau_x = tau2 * x^2 + (tau1 + msg2) * x + z^2 * gamma
    //
    // solving this equation for msg2, we get:
    //
    // tau_x - tau2 * x^2 - z^2 * gamma = (tau1 + msg2) * x
    // tau1 + msg2 = (tau_x - tau2 * x^2 - z^2 * gamma) * x^-1
    // msg2 = (tau_x - tau2 * x^2 - z^2 * gamma) * x^-1 - tau1
    //
    Scalar msg2_scalar =
        ((tau_x - (tau2 * x.Square()) - (z.Square() * gamma_vs0)) * x.Invert()) - tau1;

    std::vector<uint8_t> msg2 = msg2_scalar.GetVch(true);

    std::string msg =
        std::string(msg1.begin(), msg1.end())
        + std::string(msg2.begin(), msg2.end());

    int64_t amount = (int64_t) vs0.GetUint64();

    return std::optional<MsgAmt> {MsgAmt::of(msg, amount)};
}
template std::optional<MsgAmt> MsgAmtCipher<Mcl>::Decrypt(
    const Mcl::Scalar& msg1_vs0,
    const Mcl::Scalar& gamma_vs0,
    const Mcl::Scalar& tau1,
    const Mcl::Scalar& tau2,
    const Mcl::Scalar& tau_x,
    const Mcl::Scalar& x,
    const Mcl::Scalar& z,
    const Mcl::Scalar& uint64_max,
    const Mcl::Point& H,
    const Mcl::Point& G,
    const Mcl::Point& exp_vs0_commitment
);

} // namespace range_proof

