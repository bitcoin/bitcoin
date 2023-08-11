// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_ARITH_RANGE_PROOF_BULLETPROOFS_PLUS_RANGE_PROOF_LOGIC_H
#define NAVCOIN_BLSCT_ARITH_RANGE_PROOF_BULLETPROOFS_PLUS_RANGE_PROOF_LOGIC_H

#include <optional>
#include <tuple>
#include <vector>

#include <blsct/arith/elements.h>
#include <blsct/range_proof/common.h>
#include <blsct/range_proof/bulletproofs_plus/amount_recovery_request.h>
#include <blsct/range_proof/bulletproofs_plus/amount_recovery_result.h>
#include <blsct/range_proof/bulletproofs_plus/range_proof_with_transcript.h>
#include <blsct/range_proof/bulletproofs_plus/recovered_amount.h>
#include <consensus/amount.h>
#include <ctokens/tokenid.h>
#include <hash.h>

namespace bulletproofs_plus {

// implementation of range proof algorithms described in:
// https://eprint.iacr.org/2020/735.pdf
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
        const TokenId& token_id
    );

    bool Verify(
        const std::vector<RangeProof<T>>& proofs,
        const TokenId& token_id
    );

    AmountRecoveryResult<T> RecoverAmounts(
        const std::vector<AmountRecoveryRequest<T>>& reqs,
        const TokenId& token_id
    );

#ifndef BOOST_UNIT_TEST
private:
#endif
    static Scalars Compute_D(
        const Scalars& z_asc_by_2_pows,
        const Scalars& two_pows,
        const Scalar& z_sq,
        const size_t& m
    );

    Point Compute_A_hat(
        const size_t& m,
        const size_t& mn,
        const Points& gs,
        const Points& hs,
        const Point& g,
        const Points& Vs,
        const Point& A,
        const Scalars& d,
        const Scalar& y,
        const Scalar& z,
        const Scalar& z_sq,
        const Scalar& y_to_mn_plus_1,
        const Scalars& y_inc_pow_mn,
        const Scalars& y_dec_pow_mn,
        const Scalars& z_asc_by_2_pows
    );

    // returns z^2, z^4, z^6, ...
    static Scalars ComputeZAscBy2Pows(
        const Scalar& z,
        const size_t& m
    );

    // returns tuple of:
    // - 1, 2, 4, ..., 2^n-1
    // - y, y^2, y^mn-1, ... and y^mn, y^mn-1, ...
    // - z^2, z^4, z^6, ...
    // - y^mn+1
    static std::tuple<Scalars, Scalars, Scalars, Scalars, Scalar> ComputePowers(
        const Scalar& y,
        const Scalar& z,
        const size_t& m,
        const size_t& n
    );

    static std::tuple<Scalars, Scalars, Scalars> ComputeVeriScalars(
        const Elements<typename T::Scalar>& es,
        const size_t& mn
    );

    static size_t GetNumLeadingZeros(const uint32_t& n);

    bool VerifyProofs(
        const std::vector<RangeProofWithTranscript<T>>& proof_transcripts,
        const TokenId& token_id,
        const size_t& max_mn
    );

    range_proof::Common<T> m_common;
};

} // namespace bulletproofs_plus

#endif // NAVCOIN_BLSCT_ARITH_RANGE_PROOF_BULLETPROOFS_PLUS_RANGE_PROOF_LOGIC_H
