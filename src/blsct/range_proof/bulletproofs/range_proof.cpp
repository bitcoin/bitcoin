// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/range_proof/bulletproofs/range_proof.h>
#include <blsct/common.h>
#include <variant>

namespace bulletproofs {

template <typename T>
bool RangeProof<T>::operator==(const RangeProof<T>& other) const
{
    using P = range_proof::ProofBase<T>;
    auto this_parent = static_cast<const P&>(*this);
    auto other_parent = static_cast<const P&>(other);

    bool seed_matches = false;
    if (std::holds_alternative<TokenId>(seed) && std::holds_alternative<TokenId>(other.seed)) {
        seed_matches = std::get<TokenId>(seed) == std::get<TokenId>(other.seed);
    } else if (std::holds_alternative<blsct::Message>(seed) && std::holds_alternative<blsct::Message>(other.seed)) {
        seed_matches = std::get<blsct::Message>(seed) == std::get<blsct::Message>(other.seed);
    }

    return
        this_parent == other_parent &&
        seed_matches &&
        A == other.A &&
        S == other.S &&
        T1 == other.T1 &&
        T2 == other.T2 &&
        mu == other.mu &&
        tau_x == other.tau_x &&
        a == other.a &&
        b == other.b &&
        t_hat == other.t_hat;
}
template
bool RangeProof<Mcl>::operator==(const RangeProof<Mcl>& other) const;

template <typename T>
bool RangeProof<T>::operator!=(const RangeProof<T>& other) const
{
    return !operator==(other);
}
template
bool RangeProof<Mcl>::operator!=(const RangeProof<Mcl>& other) const;

} // namespace bulletproofs
