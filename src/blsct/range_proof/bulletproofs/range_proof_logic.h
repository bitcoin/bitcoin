// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_ARITH_RANGE_PROOF_BULLETPROOFS_RANGE_PROOF_LOGIC_H
#define NAVCOIN_BLSCT_ARITH_RANGE_PROOF_BULLETPROOFS_RANGE_PROOF_LOGIC_H

#include <optional>
#include <vector>

#include <blsct/arith/elements.h>
#include <blsct/range_proof/common.h>
#include <blsct/range_proof/bulletproofs/range_proof_with_transcript.h>
#include <consensus/amount.h>
#include <ctokens/tokenid.h>
#include <hash.h>

namespace bulletproofs {

template <typename T>
struct AmountRecoveryRequest {
    using Scalar = typename T::Scalar;
    using Point = typename T::Point;
    using Points = Elements<Point>;

    size_t id;
    Scalar x;
    Scalar z;
    Points Vs;
    Points Ls;
    Points Rs;
    Scalar mu;
    Scalar tau_x;
    Point nonce;

    static AmountRecoveryRequest<T> of(RangeProof<T>& proof, Point& nonce);
};

template <typename T>
struct RecoveredAmount {
    using Scalar = typename T::Scalar;

    RecoveredAmount(
        const size_t& id,
        const CAmount& amount,
        const Scalar& gamma,
        const std::string& message) : id{id}, amount{amount}, gamma{gamma}, message{message} {}

    size_t id;
    CAmount amount;
    Scalar gamma;
    std::string message;
};

template <typename T>
struct AmountRecoveryResult {
    bool is_completed; // does not mean recovery success
    std::vector<RecoveredAmount<T>> amounts;

    static AmountRecoveryResult<T> failure();
};

// implementation of range proof algorithms described in:
// https://eprint.iacr.org/2017/1066.pdf
template <typename T>
class RangeProofLogic
{
public:
    using Scalar = typename T::Scalar;
    using Point = typename T::Point;
    using Scalars = Elements<Scalar>;
    using Points = Elements<Point>;

    RangeProof<T> Prove(
        Scalars& vs,
        Point& nonce,
        const std::vector<uint8_t>& message,
        const TokenId& token_id) const;

    bool Verify(
        const std::vector<RangeProof<T>>& proofs,
        const TokenId& token_id) const;

    bool VerifyProofs(
        const std::vector<RangeProofWithTranscript<T>>& proof_transcripts,
        const range_proof::Generators<T>& gens,
        const size_t& max_mn) const;

    AmountRecoveryResult<T> RecoverAmounts(
        const std::vector<AmountRecoveryRequest<T>>& reqs,
        const TokenId& token_id) const;

private:
    range_proof::Common<T> m_common;
};

} // namespace bulletproofs

#endif // NAVCOIN_BLSCT_ARITH_RANGE_PROOF_BULLETPROOFS_RANGE_PROOF_LOGIC_H
