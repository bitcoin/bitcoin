// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/mcl/mcl_g1point.h>
#include <blsct/arith/mcl/mcl_scalar.h>
#include <blsct/arith/mcl/mcl.h>
#include <blsct/building_block/fiat_shamir.h>
#include <blsct/building_block/weighted_inner_prod_arg.h>
#include <blsct/range_proof/bulletproofs_plus/range_proof_with_transcript.h>
#include <blsct/range_proof/bulletproofs_plus/util.h>
#include <blsct/range_proof/common.h>
#include <blsct/range_proof/setup.h>
#include <blsct/common.h>
#include <hash.h>
#include <cmath>

namespace bulletproofs_plus {

template <typename T>
RangeProofWithTranscript<T> RangeProofWithTranscript<T>::Build(const RangeProof<T>& proof) {
    using Scalar = typename T::Scalar;

    // build transcript in the same way the prove function builds it
    HashWriter fiat_shamir{};
    Scalars es;

    size_t m = blsct::Common::GetFirstPowerOf2GreaterOrEqTo(proof.Vs.Size());
    size_t n = range_proof::Setup::num_input_value_bits;
    size_t mn = m * n;

retry:
    es.Clear();

    for (size_t i = 0; i < proof.Vs.Size(); ++i) {
        fiat_shamir << proof.Vs[i];
    }

    GEN_FIAT_SHAMIR_VAR(y, fiat_shamir, retry);
    GEN_FIAT_SHAMIR_VAR(z, fiat_shamir, retry);

    fiat_shamir << proof.A;

    // to update hasher to expected state. generated values are not used
    static_cast<void>(Util<T>::GetYPows(y, mn, fiat_shamir));

    auto num_rounds = range_proof::Common<T>::GetNumRoundsExclLast(proof.Vs.Size()) + 1;

    size_t i = 0;
    while (true) {
        if (i == num_rounds - 1) {
            fiat_shamir << proof.A_wip;
            fiat_shamir << proof.B;
            GEN_FIAT_SHAMIR_VAR(e_last_round, fiat_shamir, retry);

            return RangeProofWithTranscript<T>(
                proof,
                y,
                z,
                e_last_round,
                es,
                m,
                n,
                mn
            );
        }

        fiat_shamir << proof.Ls[i];
        fiat_shamir << proof.Rs[i];

        // GEN_FIAT_SHAMIR_VAR(e, fiat_shamir, retry);
        Scalar e(7);
        es.Add(e);

        ++i;
    }
}
template RangeProofWithTranscript<Mcl> RangeProofWithTranscript<Mcl>::Build(const RangeProof<Mcl>&);

} // namespace bulletproofs_plus
