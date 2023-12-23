// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/mcl/mcl.h>
#include <blsct/arith/mcl/mcl_g1point.h>
#include <blsct/arith/mcl/mcl_scalar.h>
#include <blsct/building_block/fiat_shamir.h>
#include <blsct/building_block/weighted_inner_prod_arg.h>
#include <blsct/building_block/lazy_points.h>
#include <blsct/building_block/g_h_gi_hi_zero_verifier.h>
#include <blsct/common.h>
#include <blsct/range_proof/bulletproofs_plus/range_proof_logic.h>
#include <blsct/range_proof/common.h>
#include <blsct/range_proof/msg_amt_cipher.h>
#include <tinyformat.h>

// Bulletproofs+ implementation based on
// - https://eprint.iacr.org/2020/735.pdf
// - https://github.com/KyoohyungHan/BulletProofsPlus/tree/master

namespace bulletproofs_plus {

template <typename T>
Elements<typename T::Scalar> RangeProofLogic<T>::Compute_D(
    const Scalars& z_asc_by_2_pows,
    const Scalars& two_pows,
    const Scalar& z_sq,
    const size_t& m
) {
    Scalars d;
    for (size_t i=0; i<m; ++i) {
        for (size_t j=0; j<two_pows.Size(); ++j) {
            Scalar x = z_asc_by_2_pows[i] * two_pows[j];
            d.Add(x);
        }
    }
    return d;
}
template
Elements<Mcl::Scalar> RangeProofLogic<Mcl>::Compute_D(
    const Elements<Mcl::Scalar>& z_asc_by_2_pows,
    const Elements<Mcl::Scalar>& two_pows,
    const Mcl::Scalar& z_sq,
    const size_t& m
);

template <typename T>
typename T::Point RangeProofLogic<T>::Compute_A_hat(
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
    const Scalars& y_asc_pows_mn,
    const Scalars& y_desc_pows_mn,
    const Scalars& z_asc_by_2_pows
) {
    Scalars ones = Scalars::RepeatN(m_common.One(), mn);
    Scalars neg_zs = Scalars::RepeatN(z.Negate(), mn);
    Scalars zs = Scalars::RepeatN(z, mn);

    Scalars v_exp;
    for (size_t i=0; i<m; ++i) {
        v_exp.Add(z_asc_by_2_pows[i] * y_to_mn_plus_1);
    }

    Scalar g_exp =
        y_asc_pows_mn.Sum() * z
        + (d.Sum() * y_to_mn_plus_1 * z).Negate()
        + (y_asc_pows_mn.Sum() * z_sq).Negate()
        ;

    LazyPoints<T> lp;
    lp.Add(A);
    lp.Add(gs, neg_zs);
    lp.Add(hs, d * y_desc_pows_mn + zs);
    lp.Add(g, g_exp);
    lp.Add(Vs, v_exp);

    return lp.Sum();
}

template <typename T>
Elements<typename T::Scalar> RangeProofLogic<T>::ComputeZAscBy2Pows(
    const Scalar& z,
    const size_t& m
) {
    Scalar z_sq = z.Square();
    Scalars z_asc_by_2_pows;
    {
        Scalar z_pow = z_sq;
        for (size_t i=0; i<m; ++i) {
            z_asc_by_2_pows.Add(z_pow);
            z_pow = z_pow * z_sq;
        }
    }
    return z_asc_by_2_pows;
}
template
Elements<Mcl::Scalar> RangeProofLogic<Mcl>::ComputeZAscBy2Pows(
    const Scalar& z,
    const size_t& m
);

template <typename T>
std::tuple<
    Elements<typename T::Scalar>,
    Elements<typename T::Scalar>,
    Elements<typename T::Scalar>,
    Elements<typename T::Scalar>,
    typename T::Scalar
> RangeProofLogic<T>::ComputePowers(
    const Scalar& y,
    const Scalar& z,
    const size_t& m,
    const size_t& n
) {
    Scalars two_pows;
    {
        Scalar two(2);
        two_pows = Scalars::FirstNPow(two, n);
    }
    size_t mn = m * n;

    // y->^mn
    Scalars y_asc_pows_mn = Scalars::FirstNPow(y, mn, 1);

    // y<-^mn
    Scalars y_desc_pows_mn = y_asc_pows_mn.Reverse();

    Scalar y_to_mn_plus_1 = y.Pow(mn + 1);

    return {
        two_pows,
        y_asc_pows_mn,
        y_desc_pows_mn,
        ComputeZAscBy2Pows(z, m),
        y_to_mn_plus_1
    };
}

template <typename T>
size_t RangeProofLogic<T>::GetNumLeadingZeros(const uint32_t& n) {
    size_t count = 0;
    uint32_t mask = 1U << 31;

    if (n == 0) return 0;

    while (mask != 0) {
        if ((n & mask) != 0) return count;
        mask >>= 1;
        ++count;
    }
    return count;
}
template
size_t RangeProofLogic<Mcl>::GetNumLeadingZeros(const uint32_t& n);

template <typename T>
std::tuple<
    Elements<typename T::Scalar>,
    Elements<typename T::Scalar>,
    Elements<typename T::Scalar>
> RangeProofLogic<T>::ComputeVeriScalars(
    const Elements<typename T::Scalar>& es,
    const size_t& mn
) {
    // inv_es_prod: 1/e_1 * 1/e_2, ..., 1/e_k
    Scalar inv_es_prod = es.Invert().Product();

    // e_i^2
    Scalars e_squares = es.Square();

    // e_i^-2
    Scalars inv_e_squares = es.Invert().Square();

    // s_vec
    uint32_t log_mn = std::log2(mn);

    Scalars s_vec;
    s_vec.Add(inv_es_prod);
    for (size_t i=1; i<mn; ++i) {
        uint32_t log_i = 31 - RangeProofLogic<T>::GetNumLeadingZeros(i);
        uint32_t k = 1 << log_i;
        size_t e_idx = log_mn - 1 - log_i;
        Scalar u_log_i_sq = e_squares[e_idx];
        s_vec.Add(s_vec[i - k] * u_log_i_sq);
    }

    return {
        e_squares,
        inv_e_squares,
        s_vec
    };
}

template <typename T>
RangeProof<T> RangeProofLogic<T>::Prove(
    Elements<typename T::Scalar>& vs,
    typename T::Point& nonce,
    const std::vector<uint8_t>& message,
    const TokenId& token_id
) {
    using Scalar = typename T::Scalar;
    using Scalars = Elements<Scalar>;

    range_proof::Common<T>::ValidateParameters(vs, message);

    const size_t num_input_values_power_of_2 =
        blsct::Common::GetFirstPowerOf2GreaterOrEqTo(vs.Size());

    RangeProof<T> proof;
    proof.token_id = token_id;

    const size_t m = num_input_values_power_of_2;
    const size_t n = range_proof::Setup::num_input_value_bits;
    const size_t mn = m * n;

    // generate gammas
    Scalars gammas;
    for (size_t i = 0; i < num_input_values_power_of_2; ++i) {
        auto hash = nonce.GetHashWithSalt(100 + i);
        gammas.Add(hash);
    }

    // make the number of input values a power of 2 w/ 0s if needed
    while (vs.Size() < num_input_values_power_of_2) {
        vs.Add(Scalar(0));
    }

    // get generators for the token_id
    range_proof::Generators<T> gens = m_common.Gf().GetInstance(token_id);
    auto gs = gens.GetGiSubset(mn);
    auto hs = gens.GetHiSubset(mn);
    auto h = gens.H;
    auto g = gens.G;

    // This hash is updated for Fiat-Shamir throughout the proof
    HashWriter fiat_shamir{};

retry: // hasher is not cleared so that different hash will be obtained upon retry

    // Calculate value commitments directly form the input values
    for (size_t i = 0; i < vs.Size(); ++i) {
        auto V = (g * vs[i]) + (h * gammas[i]);
        proof.Vs.Add(V);
        fiat_shamir << V;
    }

    GEN_FIAT_SHAMIR_VAR(y, fiat_shamir, retry);
    GEN_FIAT_SHAMIR_VAR(z, fiat_shamir, retry);

    // Commitment to aL and aR (obfuscated with alpha)
    Scalar nonce_alpha = nonce.GetHashWithSalt(1);
    Scalar alpha = range_proof::MsgAmtCipher<T>::ComputeAlpha(message, vs[0], nonce_alpha);

    Scalar tau1 = nonce.GetHashWithSalt(2);
    Scalar tau2 = nonce.GetHashWithSalt(3);
    Scalars z_pows_from_2 = Scalars::FirstNPow(z, gammas.Size(), 2);

    //proof.tau_x = (tau2 * y.Square()) + ((tau1 + msg2) * y) + (z_pows_from_2 * gammas).Sum();
    proof.tau_x = range_proof::MsgAmtCipher<T>::ComputeTauX(
        message,
        y,
        z,
        tau1,
        tau2,
        z_pows_from_2,
        gammas
    );

    // Values to be obfuscated are encoded in binary and flattened to a single vector aL
    // only the first 64 bits of each Scalar<S> is picked up
    Scalars aL;                  // ** size of aL can be shorter than concat_input_values_in_bits
    for (Scalar& v : vs.m_vec) { // for each input value
        for (size_t i = 0; i < range_proof::Setup::num_input_value_bits; ++i) {
            aL.Add(v.GetSeriBit(i) ? 1 : 0);
        }
    }
    // pad 0 bits at the end if aL.size < concat_input_values_in_bits
    while (aL.Size() < mn) {
        aL.Add(0);
    }

    auto one_value_concat_bits = Scalars::RepeatN(m_common.One(), mn);

    // aR is aL - 1
    Scalars aR = aL - one_value_concat_bits;
    proof.A = (LazyPoints<T>(gs, aL) + LazyPoints<T>(hs, aR) + LazyPoint<T>(h, alpha)).Sum();
    fiat_shamir << proof.A;

    auto [
        two_pows,
        y_asc_pows_mn,
        y_desc_pows_mn,
        z_asc_by_2_pows,
        y_to_mn_plus_1
    ] = RangeProofLogic<T>::ComputePowers(y, z, m, n);

    Scalar z_sq = z.Square();

    Scalars d = Compute_D(
        z_asc_by_2_pows,
        two_pows,
        z_sq,
        m
    );

    // A_hat
    Point A_hat = Compute_A_hat(
        m,
        mn,
        gs,
        hs,
        g,
        proof.Vs,
        proof.A,
        d,
        y,
        z,
        z_sq,
        y_to_mn_plus_1,
        y_asc_pows_mn,
        y_desc_pows_mn,
        z_asc_by_2_pows
    );

    Scalars ones = Scalars::RepeatN(m_common.One(), mn);

    // a_hat_L
    Scalars a_hat_L = aL - ones * z;

    // a_hat_R
    Scalars a_hat_R = aR + d * y_desc_pows_mn + ones * z;

    // alpha_hat
    proof.alpha_hat = alpha + (z_asc_by_2_pows * gammas).Sum() * y_to_mn_plus_1;

    {
        auto res = WeightedInnerProdArg::Run<Mcl>(
            mn,
            y,
            gs,
            hs,
            g,
            h,
            A_hat, // P
            a_hat_L,
            a_hat_R,
            proof.alpha_hat,
            fiat_shamir
        );
        if (res == std::nullopt) goto retry;

        proof.Ls = res.value().Ls;
        proof.Rs = res.value().Rs;
        proof.A_wip = res.value().A;
        proof.B = res.value().B;
        proof.r_prime = res.value().r_prime;
        proof.s_prime = res.value().s_prime;
        proof.delta_prime = res.value().delta_prime;
    }
    return proof;
}
template RangeProof<Mcl> RangeProofLogic<Mcl>::Prove(
    Elements<Mcl::Scalar>&,
    Mcl::Point&,
    const std::vector<uint8_t>&,
    const TokenId&
);

template <typename T>
bool RangeProofLogic<T>::VerifyProofs(
    const std::vector<RangeProofWithTranscript<T>>& proof_transcripts,
    const size_t& max_mn
) {
    using Scalar = typename T::Scalar;
    using Scalars = Elements<Scalar>;

    for (const RangeProofWithTranscript<T>& pt : proof_transcripts) {
        range_proof::Generators<T> gens = m_common.Gf().GetInstance(pt.proof.token_id);

        auto gs = gens.GetGiSubset(pt.mn);
        auto hs = gens.GetHiSubset(pt.mn);
        auto h = gens.H;
        auto g = gens.G;

        auto [
            two_pows,
            y_asc_pows_mn,
            y_desc_pows_mn,
            z_asc_by_2_pows,
            y_to_mn_plus_1
        ] = RangeProofLogic<T>::ComputePowers(pt.y, pt.z, pt.m, pt.n);

        // Compute: z^2 * 1, ..., z^2 * 2^n-1, z^4 * 1, ..., z^4 * 2^n-1, z^6 * 1, ...
        Scalars z_times_two_pows;
        {
            for (auto z_pow: z_asc_by_2_pows.m_vec) {
                for (auto two_pow: two_pows.m_vec) {
                    z_times_two_pows.Add(z_pow * two_pow);
                }
            }
        }

        // Compute scalars for verification
        auto [
            e_squares,
            e_inv_squares,
            s_vec
        ] = RangeProofLogic<T>::ComputeVeriScalars(pt.es, pt.mn);

        Scalars s_prime_vec = s_vec.Reverse();
        Scalar inv_final_e = pt.e_last_round.Invert();
        Scalar final_e_sq = pt.e_last_round.Square();
        Scalar inv_final_e_sq = final_e_sq.Invert();
        Scalar r_prime_inv_final_e_y = pt.proof.r_prime * inv_final_e * pt.y;
        Scalar s_prime_inv_final_e = pt.proof.s_prime * inv_final_e;
        Scalars inv_y_asc_pows_mn = Scalars::FirstNPow(pt.y.Invert(), pt.mn, 1); // skip first 1

        // Compute generator exponents
        Scalars gs_exp;
        {
            Scalar minus_z = pt.z.Negate();
            for (size_t i=0; i<pt.mn; ++i) {
                Scalar s = s_vec[i];
                Scalar inv_y_pow = inv_y_asc_pows_mn[i];
                gs_exp.Add(minus_z + s.Negate() * inv_y_pow * r_prime_inv_final_e_y);
            }
        }

        Scalars hs_exp;
        {
            Scalar neg_s_prime_inv_final_e = s_prime_inv_final_e.Negate();
            for (size_t i=0; i<pt.mn; ++i) {
                Scalar rev_s = s_vec[pt.mn - 1 - i];
                Scalar z_times_two_pow = z_times_two_pows[i];
                Scalar y_pow_desc = y_desc_pows_mn[i];
                hs_exp.Add(
                    neg_s_prime_inv_final_e * rev_s + z_times_two_pow * y_pow_desc + pt.z
                );
            }
        }

        Scalar g_exp =
            pt.proof.r_prime.Negate()
            * pt.proof.s_prime
            * pt.y
            * inv_final_e_sq
            + (
                y_asc_pows_mn.Sum() * (pt.z + pt.z.Square().Negate())
                + y_to_mn_plus_1.Negate() * pt.z * two_pows.Sum() * z_asc_by_2_pows.Sum()
            );
        Scalar h_exp = pt.proof.delta_prime.Negate() * inv_final_e_sq;

        Scalars vs_exp;
        {
            for (auto x: z_asc_by_2_pows.m_vec) {
                vs_exp.Add(x * y_to_mn_plus_1);
            }
        }

        LazyPoints<T> lp;
        lp.Add(pt.proof.A);
        lp.Add(pt.proof.A_wip, pt.e_last_round.Invert());
        lp.Add(pt.proof.B, pt.e_last_round.Square().Invert());
        lp.Add(g, g_exp);
        lp.Add(h, h_exp);
        lp.Add(pt.proof.Ls, static_cast<const Scalars&>(e_squares));
        lp.Add(pt.proof.Rs, e_inv_squares);
        lp.Add(gs, gs_exp);
        lp.Add(hs, hs_exp);
        lp.Add(pt.proof.Vs, vs_exp);

        if (!lp.Sum().IsZero()) return false;
    }

    return true;
}
template bool RangeProofLogic<Mcl>::VerifyProofs(
    const std::vector<RangeProofWithTranscript<Mcl>>&,
    const size_t&
);

template <typename T>
bool RangeProofLogic<T>::Verify(
    const std::vector<RangeProof<T>>& proofs
) {
    range_proof::Common<T>::ValidateProofsBySizes(proofs);

    std::vector<RangeProofWithTranscript<T>> proof_transcripts;
    size_t max_num_rounds = 0;

    for (const RangeProof<T>& proof: proofs) {
        // update max # of rounds and sum of all V bits
        max_num_rounds = std::max(max_num_rounds, proof.Ls.Size());

        // derive transcript from the proof
        auto proof_transcript = RangeProofWithTranscript<T>::Build(proof);
        proof_transcripts.push_back(proof_transcript);
    }

    const size_t max_mn = 1ull << max_num_rounds;

    return VerifyProofs(
        proof_transcripts,
        max_mn
    );
}
template bool RangeProofLogic<Mcl>::Verify(
    const std::vector<RangeProof<Mcl>>&
);

template <typename T>
AmountRecoveryResult<T> RangeProofLogic<T>::RecoverAmounts(
    const std::vector<AmountRecoveryRequest<T>>& reqs
) {
    using Scalar = typename T::Scalar;
    using Point = typename T::Point;

    // will contain result of successful requests only
    std::vector<range_proof::RecoveredData<T>> xs;

    for (const AmountRecoveryRequest<T>& req: reqs) {
        range_proof::Generators<T> gens = m_common.Gf().GetInstance(req.token_id);
        Point g = gens.G;
        Point h = gens.H;

        // failure if sizes of Ls and Rs differ or Vs is empty
        auto Ls_Rs_valid = req.Ls.Size() > 0 && req.Ls.Size() == req.Rs.Size();
        if (req.Vs.Size() == 0 || !Ls_Rs_valid) {
            return AmountRecoveryResult<T>::failure();
        }
        // recovery can only be done when the number of value commitment is 1
        if (req.Vs.Size() != 1) {
            continue;
        }

        Scalar gamma_vs0 = req.nonce.GetHashWithSalt(100); // gamma for vs[0]

        auto [
            _two_pows, // unused
            _y_asc_pows_mn, // unused
            _y_desc_pows_mn,  // unused
            z_asc_by_2_pows,
            y_to_mn_plus_1
        ] = RangeProofLogic<T>::ComputePowers(req.y, req.z, req.m, req.n);

        // msg1 and vs[0] are embedded to alpha_hat as follows:
        // - msg1_vs0 = (msg1 << 64 | 64-bit vs[0])
        // - alpha = nonce_1 + msg1_vs0
        // - alpha_hat = alpha + alpha_hat_sum
        // so,
        // alpha_hat = nonce_1 + msg1_vs0 + alpha_hat_sum
        // msg1_vs0 = alpha_hat - nonce_1 - alpha_hat_sum
        Scalar msg1_vs0;
        {
            Scalar alpha_hat_sum;
            {
                Scalars gammas;
                gammas.Add(gamma_vs0);
                alpha_hat_sum = (z_asc_by_2_pows * gammas).Sum() * y_to_mn_plus_1;
            }
            Scalar nonce_1 = req.nonce.GetHashWithSalt(1);
            msg1_vs0 = req.alpha_hat - nonce_1 - alpha_hat_sum;
        }
        Scalar tau1 = req.nonce.GetHashWithSalt(2);
        Scalar tau2 = req.nonce.GetHashWithSalt(3);

        auto maybe_msg_amt = range_proof::MsgAmtCipher<T>::Decrypt(
            msg1_vs0,
            gamma_vs0,
            tau1,
            tau2,
            req.tau_x,
            req.y,
            req.z,
            m_common.Uint64Max(),
            h,
            g,
            req.Vs[0]
        );
        if (maybe_msg_amt == std::nullopt) {
            continue;
        }
        auto msg_amt = maybe_msg_amt.value();

        auto x = range_proof::RecoveredData<T>(
            req.id,
            msg_amt.amount,
            req.nonce.GetHashWithSalt(100), // gamma for vs[0]
            msg_amt.msg
        );
        xs.push_back(x);

        Scalar msg2_scalar = ((req.tau_x - (tau2 * req.y.Square()) - (req.z.Square() * gamma_vs0)) * req.y.Invert()) - tau1;
        std::vector<uint8_t> msg2 = msg2_scalar.GetVch(true);
    }
    return {
        true,
        xs
    };
}
template AmountRecoveryResult<Mcl> RangeProofLogic<Mcl>::RecoverAmounts(
    const std::vector<AmountRecoveryRequest<Mcl>>&
);

} // namespace bulletproofs_plus
