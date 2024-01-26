// Copyright (c) 2022 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NAVCOIN_BLSCT_ARITH_RANGE_PROOF_BULLETPROOFS_RANGE_PROOF_LOGIC_H
#define NAVCOIN_BLSCT_ARITH_RANGE_PROOF_BULLETPROOFS_RANGE_PROOF_LOGIC_H

#include <optional>
#include <vector>

#include <blsct/arith/elements.h>
#include <blsct/range_proof/common.h>
#include <blsct/range_proof/bulletproofs/amount_recovery_request.h>
#include <blsct/range_proof/bulletproofs/amount_recovery_result.h>
#include <blsct/range_proof/bulletproofs/range_proof_with_transcript.h>
#include <blsct/range_proof/recovered_data.h>
#include <consensus/amount.h>
#include <ctokens/tokenid.h>
#include <hash.h>

namespace bulletproofs {

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
        Scalars vs,
        Point& nonce,
        const std::vector<uint8_t>& message,
        const TokenId& token_id,
        const Scalar& minValue = 0) const;

    bool Verify(
        const std::vector<RangeProof<T>>& proofs
    ) const;

    AmountRecoveryResult<T> RecoverAmounts(
        const std::vector<AmountRecoveryRequest<T>>& reqs
    ) const;

private:
    bool VerifyProofs(
        const std::vector<RangeProofWithTranscript<T>>& proof_transcripts,
        const size_t& max_mn
    ) const;

    range_proof::Common<T> m_common;
};

} // namespace bulletproofs

#endif // NAVCOIN_BLSCT_ARITH_RANGE_PROOF_BULLETPROOFS_RANGE_PROOF_LOGIC_H
