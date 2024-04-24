// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/mcl/mcl.h>
#include <blsct/range_proof/bulletproofs/amount_recovery_request.h>
#include <blsct/range_proof/bulletproofs/range_proof_with_transcript.h>
#include <blsct/range_proof/bulletproofs/range_proof.h>

#include <stdexcept>
#include <variant>

namespace bulletproofs {

template <typename T>
AmountRecoveryRequest<T> AmountRecoveryRequest<T>::of(const RangeProofWithSeed<T>& proof, const range_proof::GammaSeed<T>& nonce)
{
    auto proof_with_transcript = RangeProofWithTranscript<T>::Build(proof);

    AmountRecoveryRequest<T> req{
        1,
        proof.seed,
        proof_with_transcript.x,
        proof_with_transcript.z,
        proof.Vs,
        proof.Ls,
        proof.Rs,
        proof.mu,
        proof.tau_x,
        nonce,
        0};
    return req;
}
template AmountRecoveryRequest<Mcl> AmountRecoveryRequest<Mcl>::of(const RangeProofWithSeed<Mcl>&, const range_proof::GammaSeed<Mcl>&);

} // namespace bulletproofs

