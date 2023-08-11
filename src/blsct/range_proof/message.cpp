// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/mcl/mcl.h>
#include <blsct/range_proof/message.h>
#include <blsct/range_proof/setup.h>
#include <string>
#include <vector>

namespace range_proof {

MsgWithAmt MsgWithAmt::of(const std::string& msg, const int64_t& amount)
{
    return MsgWithAmt {msg, amount};
}

template <typename T>
typename T::Scalar Message<T>::ComputeAlpha(
    const std::string& msg,
    const Scalar& vs0,
    const typename T::Point& nonce
) {
    // part of the message up to range_proof::Setup::m_message_1_max_size
    Scalar msg1(msg.size() > range_proof::Setup::message_1_max_size ?
        std::vector<uint8_t>(msg.begin(), msg.begin() + range_proof::Setup::message_1_max_size) :
        std::vector<uint8_t>(msg.begin(), msg.end())
    );

    // message followed by 64-bit vs0
    Scalar msg1_vs0 = (msg1 << range_proof::Setup::num_input_value_bits) | vs0;

    Scalar nonce_1 = nonce.GetHashWithSalt(1);
    Scalar alpha = nonce_1 + msg1_vs0;

    return alpha;
}

template <typename T>
typename T::Scalar Message<T>::ComputeTauX(
    const std::vector<uint8_t>& msg,
    const typename T::Scalar& x,
    const typename T::Scalar& z,
    const typename T::Point& nonce
) {
    // part of the message after range_proof::Setup::m_message_1_max_size
    Scalar msg2 = Scalar({msg.size() > range_proof::Setup::message_1_max_size ?
        std::vector<uint8_t>(msg.begin() + range_proof::Setup::message_1_max_size, msg.end()) :
        std::vector<uint8_t>()}
    );

    Scalar tau1 = nonce.GetHashWithSalt(3);
    Scalar tau2 = nonce.GetHashWithSalt(4);
    Scalar gamma_vs0 = nonce.GetHashWithSalt(100); // gamma for vs[0]

    Scalar tau_x =
        (tau2 * x.Square())
        + ((tau1 + msg2) * x)
        + (z.Square() * gamma_vs0)
        ;

    return tau_x;
}

template <typename T>
std::optional<MsgWithAmt> Message<T>::Recover(
    const typename T::Scalar& msg1_vs0,
    const typename T::Scalar& tau_x,
    const typename T::Scalar& x,
    const typename T::Scalar& z,
    const typename T::Scalar& uint64_max,
    const typename T::Point& H,
    const typename T::Point& G,
    const typename T::Point& exp_vs0_commitment,
    const typename T::Point& nonce
) {
    // recover Scalar values from nonce
    Scalar tau1 = nonce.GetHashWithSalt(3);
    Scalar tau2 = nonce.GetHashWithSalt(4);
    Scalar gamma_vs0 = nonce.GetHashWithSalt(100); // gamma for vs[0]

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

    return std::optional<MsgWithAmt> {MsgWithAmt::of(msg, amount)};
}
template std::optional<MsgWithAmt> Message<Mcl>::Recover(
    const Mcl::Scalar& msg1_vs0,
    const Mcl::Scalar& tau_x,
    const Mcl::Scalar& x,
    const Mcl::Scalar& z,
    const Mcl::Scalar& uint64_max,
    const Mcl::Point& H,
    const Mcl::Point& G,
    const Mcl::Point& exp_vs0_commitment,
    const Mcl::Point& nonce
);

} // namespace range_proof

