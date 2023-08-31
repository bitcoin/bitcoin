// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/mcl/mcl.h>
#include <blsct/range_proof/bulletproofs_plus/amount_recovery_request.h>
#include <blsct/range_proof/bulletproofs_plus/range_proof_with_transcript.h>

namespace bulletproofs_plus {

template <typename T>
AmountRecoveryRequest<T> AmountRecoveryRequest<T>::of(RangeProof<T>& proof, typename T::Point& nonce)
{
    auto proof_with_transcript = RangeProofWithTranscript<T>::Build(proof);

    AmountRecoveryRequest<T> req {
        1,
        proof.token_id,
        proof_with_transcript.y,
        proof_with_transcript.z,
        proof.alpha_hat,
        proof.tau_x,
        proof.Vs,
        proof.Ls,
        proof.Rs,
        proof_with_transcript.m,
        proof_with_transcript.n,
        proof_with_transcript.mn,
        nonce
    };
    return req;
}
template AmountRecoveryRequest<Mcl> AmountRecoveryRequest<Mcl>::of(RangeProof<Mcl>&, Mcl::Point&);

} // namespace bulletproofs_plus
