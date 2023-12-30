// Copyright (c) 2023 The Navcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/arith/mcl/mcl.h>
#include <blsct/building_block/fiat_shamir.h>
#include <blsct/building_block/imp_inner_prod_arg.h>
#include <blsct/building_block/lazy_points.h>
#include <blsct/building_block/g_h_gi_hi_zero_verifier.h>
#include <blsct/common.h>
#include <blsct/set_mem_proof/set_mem_proof_prover.h>
#include <stdexcept>
#include <cmath>
#include <hash.h>
#include <streams.h>
#include <version.h>
#include <util/strencodings.h>

template <typename T>
const typename SetMemProofProver<T>::Scalar& SetMemProofProver<T>::One()
{
    using Scalar = typename T::Scalar;
    static Scalar* x = nullptr;
    if (x == nullptr) {
        x = new Scalar(1);
    }
    return *x;
}
template
const typename SetMemProofProver<Mcl>::Scalar& SetMemProofProver<Mcl>::One();

template <typename T>
typename T::Scalar SetMemProofProver<T>::ComputeX(
    const SetMemProofSetup<T>& setup,
    const Scalar& omega,
    const Scalar& y,
    const Scalar& z,
    const Point& T1,
    const Point& T2
) {
    CDataStream st(SER_DISK, PROTOCOL_VERSION);
    st << omega << y << z << T1 << T2;
    auto vec = blsct::Common::CDataStreamToVector(st);
    Scalar x = setup.H1(vec);
    return x;
}
template
typename Mcl::Scalar SetMemProofProver<Mcl>::ComputeX(
    const SetMemProofSetup<Mcl>& setup,
    const Scalar& omega,
    const Scalar& y,
    const Scalar& z,
    const Point& T1,
    const Point& T2
);


template <typename T>
CHashWriter SetMemProofProver<T>::GenInitialFiatShamir(
    const Points& Ys,
    const Point& A1,
    const Point& A2,
    const Point& S1,
    const Point& S2,
    const Point& S3,
    const Point& phi,
    const Scalar& eta
) {
    CHashWriter fiat_shamir(0, 0);
    fiat_shamir << Ys << A1 << A2 << S1 << S2 << S3 << phi << eta;
    return fiat_shamir;
}
template
CHashWriter SetMemProofProver<Mcl>::GenInitialFiatShamir(
    const Points& Ys,
    const Point& A1,
    const Point& A2,
    const Point& S1,
    const Point& S2,
    const Point& S3,
    const Point& phi,
    const Scalar& eta
);

template <typename T>
typename SetMemProofProver<T>::Points SetMemProofProver<T>::ExtendYs(
    const SetMemProofSetup<T>& setup,
    const Points& Ys_src,
    const size_t& new_size
) {
    if (Ys_src.Size() > new_size) {
        throw std::runtime_error(std::string(__func__) + ": Not expecting new_size < current_size");
    }
    std::string padding_prefix = "SET_MEMBERSHIP_DUMMY";
    std::vector<uint8_t> msg(padding_prefix.begin(), padding_prefix.end());
    size_t prefix_len = msg.size();
    msg.resize(prefix_len + sizeof(size_t));

    Points Ys = Ys_src;
    std::vector<uint8_t> i_buf(sizeof(size_t));

    for (size_t i=Ys_src.Size(); i<new_size; ++i) {
        std::memcpy(i_buf.data(), &i, sizeof(i));
        std::copy(i_buf.begin(), i_buf.end(), &msg[prefix_len]);
        Point padding_point = setup.H5(msg);
        Ys.Add(padding_point);
    }
    return Ys;
}
template
typename SetMemProofProver<Mcl>::Points SetMemProofProver<Mcl>::ExtendYs(
    const SetMemProofSetup<Mcl>& setup,
    const Points& Ys_src,
    const size_t& new_size
);

template <typename T>
SetMemProof<T> SetMemProofProver<T>::Prove(
    const SetMemProofSetup<T>& setup,
    const Points& Ys_src,
    const Point& sigma,
    const Scalar& m,
    const Scalar& f,
    const Scalar& eta
) {
    size_t n = blsct::Common::GetFirstPowerOf2GreaterOrEqTo(Ys_src.Size());
    if (n > setup.N) {
        throw std::runtime_error(std::string(__func__) + ": # of commitments exceeds the setup maximum");
    }
    Points Ys = ExtendYs(setup, Ys_src, n);

    // Prepare Index
    Scalars bL;
    for (auto Y_i: Ys.m_vec) {
        auto v = Y_i == sigma ? Scalar(1) : Scalar(0);
        bL.Add(v);
    }

    // bL o bR = 0^n, bL - bR = 1^n, <bL, 1^n> = 1
    Scalars ones = Scalars::RepeatN(Scalar(1), n);
    Scalars bR = bL - ones;

    // Commit 1
    Point h2 = setup.H5(Ys.GetVch());
    Point h3 = setup.H6(eta.GetVch());
    Point g2 = setup.H7(eta.GetVch());

    // generate random scalars
    Scalar alpha = Scalar::Rand(true);
    Scalar beta = Scalar::Rand(true);
    Scalar rho = Scalar::Rand(true);
    Scalar r_alpha = Scalar::Rand(true);
    Scalar r_tau = Scalar::Rand(true);
    Scalar r_beta = Scalar::Rand(true);

    Scalars sL;
    for (size_t i=0; i<n; ++i) {
        sL.Add(Scalar::Rand(true));
    }
    Scalars sR;
    for (size_t i=0; i<n; ++i) {
        sR.Add(Scalar::Rand(true));
    }

    Point A1 = h2 * alpha + (Ys * bL).Sum();
    Point A2 = h2 * beta + (setup.hs.To(n) * bR).Sum();
    Point S1 = h2 * r_alpha + setup.h * r_beta + setup.g * r_tau;
    Point S2 = h2 * rho + (Ys * sL).Sum() + (setup.hs.To(n) * sR).Sum();
    Point S3 = h3 * r_beta + g2 * r_tau;

    // Set element image
    Point phi = h3 * m + g2 * f;

    // Challenge 1
    auto fiat_shamir = GenInitialFiatShamir(
        Ys, A1, A2, S1, S2, S3, phi, eta
    );

retry: // retrying without generating fiat_shamir again to get different hashes
    GEN_FIAT_SHAMIR_VAR(y, fiat_shamir, retry);
    GEN_FIAT_SHAMIR_VAR(z, fiat_shamir, retry);
    GEN_FIAT_SHAMIR_VAR(omega, fiat_shamir, retry);

    // Commonly used constants
    Scalars y_to_n = Scalars::FirstNPow(y, n);
    Scalar z_sq = z.Square();

    // Commit 2
    Scalars l0 = bL - (ones * z);
    Scalars l1 = sL;
    Scalars r0 = y_to_n * (bR * omega + ones * (omega * z)) + (ones * z_sq);
    Scalars r1 = y_to_n * sR;
    Scalar t1 = (l0 * r1).Sum() + (l1 * r0).Sum();
    Scalar t2 = (l1 * r1).Sum();

    Scalar tau_1 = Scalar::Rand(true);
    Scalar tau_2 = Scalar::Rand(true);

    Point T1 = setup.g * t1 + setup.h * tau_1;
    Point T2 = setup.g * t2 + setup.h * tau_2;

    // Challenge 2
    Scalar x = ComputeX(setup, omega, y, z, T1, T2);

    // Response
    Scalar tau_x = tau_1 * x + tau_2 * x.Square();
    Scalar mu = alpha + beta * omega + rho * x;
    Scalar z_alpha = r_alpha + alpha * x;
    Scalar z_tau = r_tau + f * x;
    Scalar z_beta = r_beta + m * x;

    Scalars l = l0 + l1 * x;
    Scalars r = r0 + r1 * x;
    Scalar t = (l * r).Sum();

    Points Hi = setup.hs.To(Ys.Size());

    GEN_FIAT_SHAMIR_VAR(c_factor, fiat_shamir, retry);

    auto iipa_res = ImpInnerProdArg::Run<T>(
        n,
        Ys, Hi, setup.g,
        l, r,
        c_factor, y,
        fiat_shamir
    );
    if (iipa_res == std::nullopt) goto retry;

    auto proof = SetMemProof<T>(
        phi, A1,
        A2, S1, S2, S3, T1, T2,
        tau_x, mu, z_alpha, z_tau, z_beta,
        t,
        iipa_res.value().Ls,
        iipa_res.value().Rs,
        iipa_res.value().a,
        iipa_res.value().b,
        omega
    );
    return proof;
}
template
SetMemProof<Mcl> SetMemProofProver<Mcl>::Prove(
    const SetMemProofSetup<Mcl>& setup,
    const Points& Ys_src,
    const Point& sigma,
    const Scalar& m,
    const Scalar& f,
    const Scalar& eta
);

template <typename T>
bool SetMemProofProver<T>::Verify(
    const SetMemProofSetup<T>& setup,
    const Points& Ys_src,
    const Scalar& eta,
    const SetMemProof<T>& proof
) {
    using LazyPoint = LazyPoint<T>;

    if (proof.Ls.Size() != proof.Rs.Size()) return false;

    size_t n = blsct::Common::GetFirstPowerOf2GreaterOrEqTo(Ys_src.Size());
    if (n > setup.N) {
        throw std::runtime_error(std::string(__func__) + ": # of commitments exceeds the setup maximum");
    }
    Points Ys = ExtendYs(setup, Ys_src, n);

    auto fiat_shamir = GenInitialFiatShamir(
        Ys, proof.A1, proof.A2, proof.S1,
        proof.S2, proof.S3, proof.phi, eta
    );
    Point h2 = setup.H5(Ys.GetVch());
    Point h3 = setup.H6(eta.GetVch());
    Point g2 = setup.H7(eta.GetVch());

retry:
    GEN_FIAT_SHAMIR_VAR(y, fiat_shamir, retry);
    GEN_FIAT_SHAMIR_VAR(z, fiat_shamir, retry);
    GEN_FIAT_SHAMIR_VAR(omega, fiat_shamir, retry);

    Scalar y_inv = y.Invert();
    Scalars y_to_n = Scalars::FirstNPow(y, n);
    Scalars y_inv_to_n = Scalars::FirstNPow(y_inv, n);
    Scalar z_sq = z.Square();
    Points h_primes = setup.hs.To(n) * y_inv_to_n;
    Scalar x = ComputeX(setup, omega, y, z, proof.T1, proof.T2);

    G_H_Gi_Hi_ZeroVerifier<T> verifier(n);

    //////// (18)
    {
        // g^t * h^tau_x = g^(z^2 + omega * (z-z^2)<1^n,y^n> - z^3<1^n,1^n>) * T1^x * T2^(x^2)
        // g^(t - z^2 - omega * (z-z^2)<1^n,y^n> + z^3<1^n,1^n>) * h^tau_x = T1^x * T2^(x^2)

        // LHS
        verifier.AddNegativeH(proof.tau_x); // LHS (18)

        // z^2 + omega * (z-z^2)<1^n,y^n> - z^3<1^n,1^n> = t0
        Scalar t0 =
            z_sq
            + proof.omega * (z - z_sq) * y_to_n.Sum()
            - z.Cube() * n; // n = <1^n, 1^n>

        // g part of LHS with t0 exp on RHS moved to LHS
        verifier.AddNegativeG(proof.t - t0);

        // T1^x and T2^(x^2) in RHS
        verifier.AddPoint(LazyPoint(proof.T1, x));          // T1^x
        verifier.AddPoint(LazyPoint(proof.T2, x.Square())); // T2^(x^2)
    }

    //////// (19): refer to ./verifying_equations.md for the details
    {
        verifier.AddPoint(LazyPoint(proof.A1, One()));
        verifier.AddPoint(LazyPoint(proof.A2, proof.omega));
        verifier.AddPoint(LazyPoint(proof.S2, x));
        verifier.AddPoint(LazyPoint(h2, proof.mu.Negate()));

        GEN_FIAT_SHAMIR_VAR(c_factor, fiat_shamir, retry);
        size_t num_rounds = std::log2(n);

        Scalars xs;
        {
            auto maybe_xs = ImpInnerProdArg::GenAllRoundXs<T>(proof.Ls, proof.Rs, fiat_shamir);
            if (!maybe_xs.has_value()) goto retry;
            xs = maybe_xs.value();
        }
        auto x_invs = xs.Invert();
        auto gen_exps = ImpInnerProdArg::GenGeneratorExponents<T>(num_rounds, xs);

        ImpInnerProdArg::LoopWithYPows<T>(n, y,
            [&](const size_t& i, const Scalar& y_pow, const Scalar& y_inv_pow) {
                verifier.SetGiExp(i, (proof.a * gen_exps[i]).Negate() - z);
                verifier.SetHiExp(i,
                    (proof.b * y_inv_pow * gen_exps[n - 1 - i]).Negate()
                    + (proof.omega * z * y_pow + z_sq) * y_inv_pow
                );
            }
        );

        for (size_t i=0; i<num_rounds; ++i) {
            verifier.AddPoint(LazyPoint(proof.Ls[i], xs[i].Square()));
            verifier.AddPoint(LazyPoint(proof.Rs[i], x_invs[i].Square()));
        }

        verifier.AddPositiveG((proof.t - proof.a * proof.b) * c_factor);
    }

    //////// (20)
    {
        // LHS
        verifier.AddPoint(LazyPoint(h2, proof.z_alpha.Negate()));
        verifier.AddNegativeH(proof.z_beta);
        verifier.AddNegativeG(proof.z_tau);

        // RHS
        verifier.AddPoint(LazyPoint(proof.S1, One()));
        verifier.AddPoint(LazyPoint(proof.A1, x));
    }

    //////// (21)
    {
        // LHS
        verifier.AddPoint(LazyPoint(h3, proof.z_beta.Negate()));
        verifier.AddPoint(LazyPoint(g2, proof.z_tau.Negate()));

        // RHS
        verifier.AddPoint(LazyPoint(proof.S3, One()));
        verifier.AddPoint(LazyPoint(proof.phi, x));
    }

    return verifier.Verify(setup.g, setup.h, Ys, setup.hs.To(n));
}
template
bool SetMemProofProver<Mcl>::Verify(
    const SetMemProofSetup<Mcl>& setup,
    const Points& Ys_src,
    const Scalar& eta,
    const SetMemProof<Mcl>& proof
);
