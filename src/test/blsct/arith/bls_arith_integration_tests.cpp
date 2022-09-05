// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
#include <test/util/setup_common.h>

#include <algorithm>
#include <blsct/arith/elements.h>
#include <blsct/arith/g1point.h>
#include <blsct/arith/mcl_initializer.h>
#include <blsct/arith/scalar.h>
#include <boost/test/unit_test.hpp>
#include <set>
#include <streams.h>

BOOST_FIXTURE_TEST_SUITE(bls_arith_integration_tests, MclTestingSetup)

// gg^z == gg^(ones * z)
BOOST_AUTO_TEST_CASE(test_integration_gg_ones_times_z)
{
    auto z = Scalar::Rand(true);
    auto gg = G1Points(std::vector<G1Point>{
        G1Point::MapToG1("g1"),
        G1Point::MapToG1("g2")});
    auto r1 = gg * z;

    Scalar one(1);
    auto ones = Scalars::RepeatN(gg.Size(), one);
    auto r2 = gg * (ones * z);

    BOOST_CHECK(r1 == r2);
}

BOOST_AUTO_TEST_CASE(test_integration_offset_by_negation)
{
    {
        Scalar z(100);
        Scalar basis(12345);

        auto r1 = basis - z;
        auto r2 = basis + z.Negate();

        BOOST_CHECK(r1 == r2);
    }
    {
        Scalar z(100);
        Scalar basis(12345);
        auto g = G1Point::MapToG1("g");

        auto r1 = g * (basis - z);
        auto r2 = g * (basis + z.Negate());

        BOOST_CHECK(r1 == r2);
    }
}

// (66), (67) of the range proof excluding (h') part
BOOST_AUTO_TEST_CASE(test_integration_range_proof_66_67_excl_h_prime)
{
    auto n = 2;
    Scalar one(1);
    auto ones = Scalars::RepeatN(n, one);
    auto z = Scalar::Rand(true);

    auto alpha = Scalar::Rand(true);
    auto rho = Scalar::Rand(true);
    auto x = Scalar::Rand(true);
    auto mu = alpha + rho * x;

    auto gg = G1Points(std::vector<G1Point>{
        G1Point::MapToG1("g1"),
        G1Point::MapToG1("g2")});
    auto h = G1Point::MapToG1("h");

    Scalars al(std::vector<Scalar> {
        Scalar {1},
        Scalar {1}
    });
    auto sl = Scalars::RandVec(n);
    auto ll = al - (ones * z) + (sl * x);

    auto hmu_ggl = (h * mu) + (gg * ll).Sum();

    auto A = h * alpha + (gg * al).Sum();
    auto S = h * rho + (gg * sl).Sum();
    auto P = A + (S * x) + (gg * z.Negate()).Sum();

    BOOST_CHECK(P == hmu_ggl);
}

BOOST_AUTO_TEST_CASE(test_integration_rebasing_base_point)
{
    auto n = 2;

    Scalar one(1);
    auto one_n = Scalars::RepeatN(n, one);
    Scalar two(n);
    auto two_n = Scalars::FirstNPow(n, two);

    auto y = Scalar::Rand(true);
    auto z = Scalar::Rand(true);
    auto y_n = Scalars::FirstNPow(n, y);
    auto hh = G1Points(std::vector<G1Point> {
        G1Point::MapToG1("h1"),
        G1Point::MapToG1("h2")
    });
    {
        auto hhp = hh * y;
        auto lhs = hhp * two;
        auto rhs = hh * y * two;
        BOOST_CHECK(lhs == rhs);
    }
    {
        auto hhp = hh * y.Negate();
        auto lhs = hhp * two;
        auto rhs = hh * y.Negate() * two;
        BOOST_CHECK(lhs == rhs);
    }
    {
        auto hhp = G1Points(std::vector<G1Point> {
            hh[0],
            hh[1] * y.Invert()
        });
        auto y_pows_inv = Scalars::FirstNPow(n, y.Invert());
        auto lhs = hhp * (y_n * z + two_n * z.Square());
        auto rhs = hh * (one_n * z + two_n * z.Square() * y_pows_inv);
        BOOST_CHECK(lhs == rhs);
    }
}

BOOST_AUTO_TEST_CASE(test_integration_range_proof_66_67_only_h_prime)
{
    auto n = 2;

    Scalar two(2);
    auto two_n = Scalars::FirstNPow(n, two);
    Scalar one(1);
    auto one_n = Scalars::RepeatN(n, one);

    auto x = Scalar::Rand(true);
    auto y = Scalar::Rand(true);
    auto z = Scalar::Rand(true);
    auto y_n = Scalars::FirstNPow(n, y);

    Scalars ar(std::vector<Scalar>{
        Scalar{1},
        Scalar{1}});
    auto sr = Scalars::RandVec(n);
    auto zs = one_n * z;
    auto hh = G1Points(std::vector<G1Point>{
        G1Point::MapToG1("h1"),
        G1Point::MapToG1("h2")});
    auto a = hh * ar;
    auto s = hh * sr;

    auto hhp = hh * Scalars::FirstNPow(n, y.Invert());

    auto p = a + s * x + hhp * (y_n * z + two_n * z.Square());
    auto rr = y_n * (ar + zs + sr * x) + (two_n * z.Square());
    auto hhprr = hhp * rr;

    BOOST_CHECK(p == hhprr);
}

BOOST_AUTO_TEST_CASE(test_integration_range_proof_65_h_part_only)
{
    auto gamma = Scalar::Rand();
    auto x = Scalar::Rand(true);
    auto tau1 = Scalar::Rand(true);
    auto tau2 = Scalar::Rand(true);

    // RHS
    auto h = G1Point::MapToG1("h");
    auto v = h * gamma;
    auto z = Scalar::Rand(true);
    auto t1 = h * tau1;
    auto t2 = h * tau2;
    auto rhs =  v * z.Square() + t1 * x + t2 * x.Square();

    // LHS
    auto tauX = tau2 * x.Square() + tau1 * x + z.Square() * gamma;
    auto lhs = h * tauX;

    BOOST_CHECK(lhs == rhs);
}

BOOST_AUTO_TEST_CASE(test_integration_range_proof_65_g_part_only_excl_ts)
{
    auto n = 2;

    auto y = Scalar::Rand(true);
    auto z = Scalar::Rand(true);
    auto upsilon = 2;

    Scalar one(1);
    Scalar two(2);
    auto one_n = Scalars::FirstNPow(n, one);
    auto two_n = Scalars::FirstNPow(n, two);
    auto y_n = Scalars::FirstNPow(n, y);

    Scalars al(std::vector<Scalar>{
        Scalar{0},
        Scalar{1}});
    auto ar = al - one_n;
    auto sl = Scalars::RandVec(n);
    auto sr = Scalars::RandVec(n);

    auto l = al - one_n * z;  // (39)
    auto r = y_n * (ar + one_n * z) + two_n * z.Square(); // (39)
    auto t_hat = (l * r).Sum();

    auto l0 = al - one_n * z;
    auto r0 = y_n * (ar + one_n * z) + two_n * z.Square();
    auto lr_equiv = (l0 * r0).Sum();
    BOOST_CHECK(t_hat == lr_equiv);

    auto g = G1Point::MapToG1("g");

    auto v = g * upsilon;
    auto delta_yz =
        ((z - z.Square()) * (one_n * y_n).Sum())
        - (z.Cube() * (one_n * two_n).Sum());

    // LHS
    auto lhs = g * t_hat;

    // RHS
    auto rhs = v * z.Square() + g * delta_yz;

    BOOST_CHECK(lhs == rhs);
}

BOOST_AUTO_TEST_CASE(test_integration_range_proof_65_g_part_ts_only)
{
    auto n = 2;

    auto x = Scalar::Rand(true);
    auto y = Scalar::Rand(true);
    auto z = Scalar::Rand(true);

    Scalar one(1);
    Scalar two(2);
    auto one_n = Scalars::FirstNPow(n, one);
    auto two_n = Scalars::FirstNPow(n, two);
    auto y_n = Scalars::FirstNPow(n, y);

    Scalars al(std::vector<Scalar> {
        Scalar {0},
        Scalar {1}
    });
    auto ar = al - one_n;
    auto sl = Scalars::RandVec(n);
    auto sr = Scalars::RandVec(n);

    const auto &l1 = sl;
    auto r0 = y_n * (ar + one_n * z) + two_n * z.Square();
    auto r1 = y_n * sr;

    auto t_hat = (l1 * r0).Sum() * x + (l1 * r1).Sum() * x.Square();

    // t(x) = <l0, r0> + <l1, r0> * x + <l1, r1> * x^2
    auto t1 = (l1 * r0).Sum();
    auto t2 = (l1 * r1).Sum();

    auto g = G1Point::MapToG1("g");

    auto cap_t1 = g * t1;
    auto cap_t2 = g * t2;

    // LHS
    auto lhs = g * t_hat;

    // RHS
    auto rhs = cap_t1 * x + cap_t2 * x.Square();

    BOOST_CHECK(lhs == rhs);
}

BOOST_AUTO_TEST_CASE(test_integration_range_proof_65_g_part_only)
{
    auto n = 2;

    auto x = Scalar::Rand(true);
    auto y = Scalar::Rand(true);
    auto z = Scalar::Rand(true);
    auto upsilon = 2;

    Scalar one(1);
    Scalar two(2);
    auto one_n = Scalars::FirstNPow(n, one);
    auto two_n = Scalars::FirstNPow(n, two);
    auto y_n = Scalars::FirstNPow(n, y);

    Scalars al(std::vector<Scalar> {
        Scalar {0},
        Scalar {1}
    });
    auto ar = al - one_n;
    auto sl = Scalars::RandVec(n);
    auto sr = Scalars::RandVec(n);

    auto l0 = (al - one_n * z);
    const auto &l1 = sl;
    auto r0 = y_n * (ar + one_n * z) + two_n * z.Square();
    auto r1 = y_n * sr;

    // LHS

    // t_hat = <l,r> = t(x) = <l0, r0> + <l1, r0> * x + <l1, r1> * x^2
    auto t0 = (l0 * r0).Sum();
    auto t1 = (l1 * r0).Sum();
    auto t2 = (l1 * r1).Sum();
    auto t_hat = t0 + t1 * x + t2 * x.Square();

    auto g = G1Point::MapToG1("g");

    auto lhs = g * t_hat;

    // RHS
    auto cap_t1 = g * t1;
    auto cap_t2 = g * t2;

    auto v = g * upsilon;
    auto delta_yz =
        ((z - z.Square()) * (one_n * y_n).Sum())
        - (z.Cube() * (one_n * two_n).Sum());

    auto rhs = v * z.Square() + g * delta_yz + cap_t1 * x + cap_t2 * x.Square();

    BOOST_CHECK(lhs == rhs);
}

// Prover and verifier know:
// g, h, u, P
//
// For a given P, prover proves that it has vectors a, b s.t.
// P = g^a h^b u^<a,b>
bool InnerProductArgument(
    const size_t& n,
    const G1Points& gg, const G1Points& hh,
    const G1Point& u, const G1Point& p,
    const Scalars& a, const Scalars& b
)
{
    if (n == 1) {
        auto c = (a * b).Sum();
        return p == (gg * a).Sum() + (hh * b).Sum() + u * c;
    } else {
        auto np = n / 2;

        auto cl = (a.To(np) * b.From(np)).Sum();
        auto cr = (a.From(np) * b.To(np)).Sum();

        auto l = (gg.From(np) * a.To(np)).Sum() + (hh.To(np) * b.From(np)).Sum() + u * cl;
        auto r = (gg.To(np) * a.From(np)).Sum() + (hh.From(np) * b.To(np)).Sum() + u * cr;

        auto x = Scalar::Rand(true);

        auto ggp = (gg.To(np) * x.Invert()) + (gg.From(np) * x);
        auto hhp = (hh.To(np) * x) + (hh.From(np) * x.Invert());

        auto pp = l * x.Square() + p + (r * x.Square().Invert());

        auto ap = a.To(np) * x + a.From(np) * x.Invert();
        auto bp = b.To(np) * x.Invert() + b.From(np) * x;

        return InnerProductArgument(np, ggp, hhp, u, pp, ap, bp);
    }
}

BOOST_AUTO_TEST_CASE(test_integration_inner_product_argument)
{
    auto n = 2;

    auto gg = G1Points(std::vector{
        G1Point::MapToG1("g1"),
        G1Point::MapToG1("g2")});
    auto hh = G1Points(std::vector{
        G1Point::MapToG1("h1"),
        G1Point::MapToG1("h2")});
    auto u = G1Point::MapToG1("u");

    // a, b are Scalar vectors
    Scalars a(std::vector<Scalar> { Scalar {2}, Scalar {3} });
    Scalars b(std::vector<Scalar> { Scalar {5}, Scalar {7} });

    auto p = (gg * a).Sum() + (hh * b).Sum() + u * (a * b).Sum();

    auto res = InnerProductArgument(
        n,
        gg, hh,
        u, p,
        a, b
    );
    BOOST_CHECK_EQUAL(res, true);
}

bool RangeProof(
    size_t n, G1Point V, Scalar gamma,
    G1Point g, G1Point h,
    G1Points gg, G1Points hh,
    Scalars al,
    bool use_inner_product_argument
)
{
    // On input upsilon and gamma, prover computes
    Scalar one(1);
    Scalar two(2);
    auto one_n = Scalars::FirstNPow(n, one);
    auto two_n = Scalars::FirstNPow(n, two);

    auto ar = al - one_n;
    auto alpha = Scalar::Rand();
    auto a = (h * alpha) + (gg * al).Sum() + (hh * ar).Sum();

    auto sl = Scalars::RandVec(n);
    auto sr = Scalars::RandVec(n);
    auto rho = Scalar::Rand();
    auto s = (h * rho) + (gg * sl).Sum() + (hh * sr).Sum();

    // Prover sends a,s to verifier

    // Verifier selects challenge points y,z and send to prover
    auto y = Scalar::Rand(true);
    auto z = Scalar::Rand(true);

    // Define vector ploynomials l(x), r(x) and t(x)
    // t(x) = <l(x),r(x)> = <l0, r0> + (<l1, r0> + <l0, r1>) * x + <l1, r1> * x^2
    auto y_n = Scalars::FirstNPow(n, y);
    auto l0 = al - one_n * z;
    const auto &l1 = sl;
    auto r0 = y_n * (ar + one_n * z) + two_n * z.Square();
    auto r1 = y_n * sr;

    auto t0 = (l0 * r0).Sum();
    auto t1 = (l1 * r0).Sum() + (l0 * r1).Sum();
    auto t2 = (l1 * r1).Sum();

    // Prover computes
    auto tau1 = Scalar::Rand(true);
    auto tau2 = Scalar::Rand(true);
    auto cap_t1 = g * t1 + h * tau1;
    auto cap_t2 = g * t2 + h * tau2;

    // Prover sends cap_t1,cal_t2 to verifier

    // Verifier select random challenge x and send to prover
    auto x = Scalar::Rand(true);

    // Prover computes

    // t_hat = <l,r> = t(x)
    auto t_hat = t0 + t1 * x + t2 * x.Square();
    auto tau_x = tau2 * x.Square() + tau1 * x + z.Square() * gamma;
    auto mu = alpha + rho * x;

    // Prover sends l,r,t_hat,tau_x,mu to verifier

    // (64)
    auto hhp = hh * Scalars::FirstNPow(n, y.Invert());

    // (65)
    auto delta_yz =
        ((z - z.Square()) * (one_n * y_n).Sum())
        - (z.Cube() * (one_n * two_n).Sum());

    auto lhs_65 = g * t_hat + h * tau_x;
    auto rhs_65 = V * z.Square() + g * delta_yz + cap_t1 * x + cap_t2 * x.Square();
    if (lhs_65 != rhs_65) return false;

    // (66), (67)
    auto l = (al - one_n * z) + sl * x;
    auto r = y_n * (ar + one_n * z + sr * x) + two_n * z.Square();

    auto p =
        a + (s * x) - (gg * (one_n * z)).Sum() + (hhp * (y_n * z + two_n * z.Square())).Sum();

    if (use_inner_product_argument) {
        auto u = G1Point::Rand();
        auto pp = p + h * mu.Negate() + u * (l * r).Sum();
        return InnerProductArgument(n, gg, hhp, u, pp, l, r);
    } else {
        auto rhs_66_67 = h * mu + (gg * l).Sum() + (hhp * r).Sum();
        if (p != rhs_66_67) return false;

        // (68)
        auto rhs_68 = (l * r).Sum();

        return t_hat == rhs_68;
    }
}

BOOST_AUTO_TEST_CASE(test_integration_range_proof)
{
    auto gamma = Scalar::Rand();
    Scalars al(std::vector<Scalar> {
        Scalar {1},
        Scalar {0},
        Scalar {0},
        Scalar {1}
    });
    size_t n = al.Size();
    Scalar upsilon(9);

    auto g = G1Point::MapToG1("g");
    auto h = G1Point::MapToG1("h");

    auto gg = G1Points(std::vector<G1Point> {
        G1Point::MapToG1("g1"),
        G1Point::MapToG1("g2"),
        G1Point::MapToG1("g3"),
        G1Point::MapToG1("g4")
    });
    auto hh = G1Points(std::vector<G1Point> {
        G1Point::MapToG1("h1"),
        G1Point::MapToG1("h2"),
        G1Point::MapToG1("h3"),
        G1Point::MapToG1("h4")
    });

    auto v = h * gamma + g * upsilon;

    for (auto i = 0; i < 2; ++i) {
        auto testCaseBool = i != 0;
        auto res = RangeProof(
            n, v, gamma,
            g, h,
            gg, hh,
            al,
            testCaseBool
        );
        BOOST_CHECK(res == true);
    }
}

BOOST_AUTO_TEST_SUITE_END()
