// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_ARITH_RANGE_PROOF_RANGE_PROOF_H
#define NAVCOIN_BLSCT_ARITH_RANGE_PROOF_RANGE_PROOF_H

#include <optional>
#include <vector>

#include <blsct/arith/elements.h>
#include <blsct/range_proof/generators.h>
#include <blsct/range_proof/range_proof_with_transcript.h>
#include <consensus/amount.h>
#include <ctokens/tokenid.h>
#include <hash.h>

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

    static AmountRecoveryRequest<T> of(RangeProof<T>& proof, size_t& index, Point& nonce);
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
    bool is_completed; // done doesn't mean recovery success
    std::vector<RecoveredAmount<T>> amounts;

    static AmountRecoveryResult<T> failure();
};

// implementation of range proof algorithms described
// based on the paper: https://eprint.iacr.org/2017/1066.pdf
template <typename T>
class RangeProofLogic
{
private:
    using Scalar = typename T::Scalar;
    using Point = typename T::Point;
    using Scalars = Elements<Scalar>;
    using Points = Elements<Point>;

public:
    RangeProofLogic();

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
        const Generators<T>& gens,
        const size_t& max_mn) const;

    AmountRecoveryResult<T> RecoverAmounts(
        const std::vector<AmountRecoveryRequest<T>>& reqs,
        const TokenId& token_id) const;

    static void ValidateProofsBySizes(
        const std::vector<RangeProof<T>>& proofs);

private:
    Scalar GetUint64Max() const;

    Point GenerateBaseG1PointH(
        const Point& p,
        size_t index,
        TokenId token_id) const;

    // using pointers for Scalar and GeneratorsFactory to avoid default constructors to be called before mcl initialization
    // these variables are meant to be constant. do not make changes after initialization.
    static GeneratorsFactory<T>* m_gf;

    static Scalar* m_one;
    static Scalar* m_two;
    static Scalars* m_two_pows_64;
    static Scalar* m_inner_prod_1x2_pows_64;
    static Scalar* m_uint64_max;

    inline static std::mutex m_init_mutex;
    inline static bool m_is_initialized = false;
};

#endif // NAVCOIN_BLSCT_ARITH_RANGE_PROOF_RANGE_PROOF_H
