// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>

#include <algorithm>
#include <blsct/arith/elements.h>
#include <blsct/arith/g1point.h>
#include <blsct/arith/mcl_initializer.h>
#include <blsct/arith/scalar.h>
#include <boost/test/unit_test.hpp>
#include <set>
#include <streams.h>

BOOST_FIXTURE_TEST_SUITE(elements_tests, MclTestingSetup)

BOOST_AUTO_TEST_CASE(test_elements_constructors)
{
    {
        Scalars ss(std::vector<Scalar> { Scalar{1}, Scalar{2} });
        BOOST_CHECK(ss.Size() == 2);
        BOOST_CHECK(ss[0].GetUint64() == 1);
        BOOST_CHECK(ss[1].GetUint64() == 2);
    }
    {
        Scalar s(2);
        Scalars ss(3, s);
        BOOST_CHECK(ss.Size() == 3);
        BOOST_CHECK(ss[0].GetUint64() == 2);
        BOOST_CHECK(ss[1].GetUint64() == 2);
        BOOST_CHECK(ss[2].GetUint64() == 2);
    }
    {
        auto g = G1Point::GetBasePoint();
        G1Points g1s(2, g);
        BOOST_CHECK(g1s.Size() == 2);
        BOOST_CHECK(g1s[0] == g);
        BOOST_CHECK(g1s[1] == g);
    }
    {
        auto g = G1Point::GetBasePoint();
        auto g2 = g + g;
        G1Points g1s(std::vector<G1Point> { g, g2 });
        BOOST_CHECK(g1s.Size() == 2);
        BOOST_CHECK(g1s[0] == g);
        BOOST_CHECK(g1s[1] == g2);
    }
}

BOOST_AUTO_TEST_CASE(test_elements_size)
{
    {
        Scalars ss(std::vector<Scalar> { Scalar{1}, Scalar{2} });
        BOOST_CHECK(ss.Size() == 2);
    }
    {
        Scalars ss;
        BOOST_CHECK(ss.Size() == 0);
    }
    {
        auto g = G1Point::GetBasePoint();
        G1Points g1s(std::vector<G1Point> { g, g + g });
        BOOST_CHECK(g1s.Size() == 2);
    }
    {
        G1Points g1s;
        BOOST_CHECK(g1s.Size() == 0);
    }
}

BOOST_AUTO_TEST_CASE(test_elements_empty)
{
    {
        Scalars ss(std::vector<Scalar> { Scalar{1}, Scalar{2} });
        BOOST_CHECK(ss.Empty() == false);
    }
    {
        Scalars ss;
        BOOST_CHECK(ss.Empty() == true);
    }
    {
        auto g = G1Point::GetBasePoint();
        G1Points g1s(std::vector<G1Point> { g, g + g });
        BOOST_CHECK(g1s.Empty() == false);
    }
    {
        G1Points g1s;
        BOOST_CHECK(g1s.Empty() == true);
    }
}

BOOST_AUTO_TEST_CASE(test_elements_sum)
{
    {
        Scalars ss(std::vector<Scalar> { Scalar{1}, Scalar{2} });
        auto sum = ss.Sum();
        BOOST_CHECK_EQUAL(sum.GetUint64(), 3);
    }
    {
        Scalars ss;
        auto sum = ss.Sum();
        BOOST_CHECK_EQUAL(sum.GetUint64(), 0);
    }
    {
        auto g = G1Point::GetBasePoint();
        G1Points g1s(std::vector<G1Point> { g, g + g });
        auto sum = g1s.Sum();
        BOOST_CHECK(sum == (g * 3));
    }
    {
        G1Points g1s;
        auto sum = g1s.Sum();
        G1Point g;
        BOOST_CHECK(sum == g);
    }
}

BOOST_AUTO_TEST_CASE(test_elements_add)
{
    {
        Scalars ss;
        Scalar x(1);
        ss.Add(x);
        BOOST_CHECK(ss.Size() == 1);
        BOOST_CHECK(ss[0].GetUint64() == 1);
    }
    {
        G1Points g1s;
        auto g = G1Point::GetBasePoint();
        g1s.Add(g);
        BOOST_CHECK(g1s.Size() == 1);
        BOOST_CHECK(g1s[0] == g);
    }
}

BOOST_AUTO_TEST_CASE(test_elements_confirm_sizes_match)
{
    {
        Scalars s1(std::vector<Scalar> { Scalar{1} });
        Scalars s2(std::vector<Scalar>{ Scalar{1}, Scalar{2} });
        BOOST_CHECK_THROW(s1.ConfirmSizesMatch(s2.Size()), std::runtime_error);
    }
    {
        Scalars s1(std::vector<Scalar> { Scalar{2}, Scalar{3} });
        Scalars s2(std::vector<Scalar>{ Scalar{1}, Scalar{2} });
        BOOST_CHECK_NO_THROW(s1.ConfirmSizesMatch(s2.Size()));
    }
    {
        auto g = G1Point::GetBasePoint();
        G1Points gg(std::vector<G1Point> { g, g + g });
        G1Points hh(std::vector<G1Point>{ g });
        BOOST_CHECK_THROW(gg.ConfirmSizesMatch(hh.Size()), std::runtime_error);
    }
    {
        auto g = G1Point::GetBasePoint();
        G1Points gg(std::vector<G1Point> { g, g + g });
        G1Points hh(std::vector<G1Point>{ g, g * 3 });
        BOOST_CHECK_NO_THROW(gg.ConfirmSizesMatch(hh.Size()));
    }
}

BOOST_AUTO_TEST_CASE(test_elements_operator_mul_scalars)
{
    // G1Points ^ Scalars -> G1Points
    {
        Scalars ss(std::vector<Scalar> { Scalar{2}, Scalar{3} });
        auto g = G1Point::GetBasePoint();
        G1Points gg(std::vector<G1Point> { g, g + g });
        auto hh = gg * ss;

        auto h1 = g * Scalar(2);
        auto h2 = (g + g) * Scalar(3);
        G1Points ii(std::vector<G1Point> { h1, h2 });

        BOOST_CHECK(hh == ii);
    }
    // Scalars ^ Scalars -> Scalars
    {
        Scalars ss(std::vector<Scalar> { Scalar{2}, Scalar{3} });
        Scalars tt(std::vector<Scalar>{ Scalar{3}, Scalar{4} });
        auto uu = ss * tt;

        Scalars vv(std::vector<Scalar> { Scalar{6}, Scalar{12} });

        BOOST_CHECK(uu == vv);
    }
}

BOOST_AUTO_TEST_CASE(test_elements_operator_mul_scalar)
{
    // Scalars * Scalar -> Scalars
    {
        Scalars ss(std::vector<Scalar> { Scalar{2}, Scalar{3} });
        Scalar z(5);
        auto r1 = ss * z;

        auto zz = Scalars::RepeatN(z, ss.Size());
        auto r2 = ss * zz;

        BOOST_CHECK(r1 == r2);
    }
    // G1Points * Scalar -> G1Points
    {
        auto g = G1Point::GetBasePoint();
        G1Points gg(std::vector<G1Point> { g, g + g });
        Scalar z(3);
        auto r1 = gg * z;

        auto zz = Scalars::RepeatN(z, gg.Size());
        auto r2 = gg * zz;

        BOOST_CHECK(r1 == r2);
    }
}

BOOST_AUTO_TEST_CASE(test_elements_operator_add)
{
    {
        Scalars ss(std::vector<Scalar> { Scalar{2}, Scalar{3} });
        Scalars tt(std::vector<Scalar>{ Scalar{3}, Scalar{4} });
        auto uu = ss + tt;

        Scalars vv(std::vector<Scalar> { Scalar{5}, Scalar{7} });
        BOOST_CHECK(uu == vv);
    }
    {
        auto g = G1Point::GetBasePoint();
        G1Points gg(std::vector<G1Point> { g, g + g });
        G1Points hh(std::vector<G1Point>{ g + g, g });
        auto ii = gg + hh;

        G1Points jj(std::vector<G1Point> { g + g + g, g + g + g });
        BOOST_CHECK(ii == jj);
    }
}

BOOST_AUTO_TEST_CASE(test_elements_operator_sub)
{
    {
        Scalars ss(std::vector<Scalar> { Scalar{7}, Scalar{6} });
        Scalars tt(std::vector<Scalar> { Scalar{3}, Scalar{4} });
        auto uu = ss - tt;

        Scalars vv(std::vector<Scalar> { Scalar{4}, Scalar{2} });
        BOOST_CHECK(uu == vv);
    }
    {
        auto g = G1Point::GetBasePoint();
        G1Points gg(std::vector<G1Point> { g + g + g, g + g + g + g });
        G1Points hh(std::vector<G1Point> { g, g });
        auto ii = gg - hh;

        G1Points jj(std::vector<G1Point> { g + g, g + g + g });
        BOOST_CHECK(ii == jj);
    }
}

BOOST_AUTO_TEST_CASE(test_elements_operator_assign)
{
    {
        Scalars a(std::vector<Scalar> { Scalar{2}, Scalar{3} });
        Scalars b;
        b = a;
        BOOST_CHECK(b.Size() == 2);
        BOOST_CHECK(b[0].GetUint64() == 2);
        BOOST_CHECK(b[1].GetUint64() == 3);
    }
    {
        auto g = G1Point::GetBasePoint();
        auto g2 = g + g;
        G1Points gs(std::vector<G1Point> { g, g2 });
        G1Points gs2;
        gs2 = gs;
        BOOST_CHECK(gs2.Size() == 2);
        BOOST_CHECK(gs2[0] == g);
        BOOST_CHECK(gs2[1] == g2);
    }
}

BOOST_AUTO_TEST_CASE(test_elements_operator_eq)
{
    {
        Scalars ss(std::vector<Scalar> { Scalar{2}, Scalar{3} });
        Scalars tt(std::vector<Scalar> { Scalar{2}, Scalar{3} });
        auto b = ss == tt;
        BOOST_CHECK(b);
    }
    {
        auto g = G1Point::GetBasePoint();
        G1Points gg(std::vector<G1Point> { g, g + g });
        G1Points hh(std::vector<G1Point> { g, g + g });
        auto b = gg == hh;
        BOOST_CHECK(b);
    }
}

BOOST_AUTO_TEST_CASE(test_elements_operator_ne)
{
    {
        Scalars ss(std::vector<Scalar> { Scalar{2}, Scalar{3} });
        Scalars tt(std::vector<Scalar> { Scalar{1}, Scalar{3} });
        auto b = ss != tt;
        BOOST_CHECK(b);
    }
    {
        auto g = G1Point::GetBasePoint();
        G1Points gg(std::vector<G1Point> { g, g + g });
        G1Points hh(std::vector<G1Point>{g * 10, g + g});
        auto b = gg != hh;
        BOOST_CHECK(b);
    }
}

BOOST_AUTO_TEST_CASE(test_elements_from)
{
    Scalars ss(std::vector<Scalar> { Scalar{1}, Scalar{2}, Scalar{3} });
    {
        auto tt = ss.From(0);
        BOOST_CHECK(ss == tt);
    }
    {
        auto tt = ss.From(1);
        Scalars uu(std::vector<Scalar> { Scalar{2}, Scalar{3} });
        BOOST_CHECK(tt == uu);
    }
    {
        auto tt = ss.From(2);
        Scalars uu(std::vector<Scalar> { Scalar{3} });
        BOOST_CHECK(tt == uu);
    }
    {
        BOOST_CHECK_THROW(ss.From(3), std::runtime_error);
    }

    auto g = G1Point::GetBasePoint();
    G1Points gg(std::vector<G1Point> { g, g + g, g + g + g });
    {
        auto hh = gg.From(0);
        BOOST_CHECK(gg == hh);
    }
    {
        auto hh = gg.From(1);
        G1Points ii(std::vector<G1Point> { g + g, g + g + g });
        BOOST_CHECK(hh == ii);
    }
    {
        auto hh = gg.From(2);
        G1Points ii(std::vector<G1Point> { g + g + g });
        BOOST_CHECK(hh == ii);
    }
    {
        BOOST_CHECK_THROW(gg.From(3), std::runtime_error);
    }
}

BOOST_AUTO_TEST_CASE(test_elements_first_n_pow)
{
    {
        Scalar k(3);
        auto pows = Scalars::FirstNPow(k, 3);
        Scalar p1(1);
        Scalar p2(3);
        Scalar p3(9);
        BOOST_CHECK(pows.Size() == 3);
        BOOST_CHECK(pows[0] == p1);
        BOOST_CHECK(pows[1] == p2);
        BOOST_CHECK(pows[2] == p3);
    }
    {
        Scalar k(3);
        auto pows = Scalars::FirstNPow(k, 3, 2);
        Scalar p1(9);
        Scalar p2(27);
        Scalar p3(81);
        BOOST_CHECK(pows.Size() == 3);
        BOOST_CHECK(pows[0] == p1);
        BOOST_CHECK(pows[1] == p2);
        BOOST_CHECK(pows[2] == p3);
    }
    {
        Scalar k(3);
        auto pows = Scalars::FirstNPow(k, 3);
        auto invPows = Scalars::FirstNPow(k.Invert(), 3);
        auto r = pows * invPows;
        Scalar one(1);
        BOOST_CHECK(r[0] == one);
        BOOST_CHECK(r[1] == one);
        BOOST_CHECK(r[2] == one);
    }
    {
        Scalar one(1);
        for(size_t i=0; i<100; ++i) {
            Scalar k(i);
            auto pows = Scalars::FirstNPow(k, 1);
            BOOST_CHECK(pows.Size() == 1);
            BOOST_CHECK(pows[0] == one);
        }
    }
}

BOOST_AUTO_TEST_CASE(test_elements_repeat_n)
{
    Scalar k(3);
    auto pows = Scalars::RepeatN(k, 3);
    BOOST_CHECK(pows.Size() == 3);
    BOOST_CHECK(pows[0] == k);
    BOOST_CHECK(pows[1] == k);
    BOOST_CHECK(pows[2] == k);
}

BOOST_AUTO_TEST_CASE(test_elements_rand_vec)
{
    auto xs = Scalars::RandVec(3);
    BOOST_CHECK(xs.Size() == 3);
}

BOOST_AUTO_TEST_CASE(test_elements_to)
{
    Scalars ss(std::vector<Scalar> { Scalar{1}, Scalar{2}, Scalar{3} });
    {
        auto tt = ss.To(0);
        BOOST_CHECK(tt.Size() == 0);
    }
    {
        auto tt = ss.To(1);
        Scalars uu(std::vector<Scalar> { Scalar{1} });
        BOOST_CHECK(tt == uu);
    }
    {
        auto tt = ss.To(2);
        Scalars uu(std::vector<Scalar> { Scalar{1}, Scalar{2} });
        BOOST_CHECK(tt == uu);
    }
    {
        auto tt = ss.To(3);
        BOOST_CHECK(tt == ss);
    }
    {
        BOOST_CHECK_THROW(ss.To(4), std::runtime_error);
    }

    auto g = G1Point::GetBasePoint();
    G1Points gg(std::vector<G1Point> { g, g + g, g + g + g });
    {
        auto hh = gg.To(0);
        BOOST_CHECK(hh.Size() == 0);
    }
    {
        auto hh = gg.To(1);
        G1Points ii(std::vector<G1Point> { g });
        BOOST_CHECK(hh == ii);
    }
    {
        auto hh = gg.To(2);
        G1Points ii(std::vector<G1Point> { g, g + g });
        BOOST_CHECK(hh == ii);
    }
    {
        auto hh = gg.To(3);
        G1Points ii(std::vector<G1Point> { g, g + g, g + g+ g });
        BOOST_CHECK(hh == ii);
    }
    {
        BOOST_CHECK_THROW(gg.To(4), std::runtime_error);
    }
}

BOOST_AUTO_TEST_CASE(test_elements_mulvec_elements)
{
    auto p1 = G1Point::GetBasePoint();
    auto p2 = p1.Double();
    G1Points ps(std::vector<G1Point> { p1, p2 });

    Scalar s1(2), s2(3);
    Scalars ss(std::vector<Scalar> { s1, s2 });

    // p should be G^2 + (G+G)^3 = G^8
    auto p = ps.MulVec(ss);
    auto q = G1Point::GetBasePoint() * 8;

    BOOST_CHECK(p == q);
}

BOOST_AUTO_TEST_CASE(test_elements_invert)
{
    {
        Scalars ss(std::vector<Scalar> { Scalar{1}, Scalar{2} });
        auto ss_inv = ss.Invert();
        BOOST_CHECK(ss_inv[0] == ss[0].Invert());
        BOOST_CHECK(ss_inv[1] == ss[1].Invert());
    }
}

BOOST_AUTO_TEST_CASE(test_elements_negate)
{
    {
        Scalars ss(std::vector<Scalar> { Scalar{1}, Scalar{2} });
        auto ss_inv = ss.Negate();
        BOOST_CHECK(ss_inv[0] == ss[0].Negate());
        BOOST_CHECK(ss_inv[1] == ss[1].Negate());
    }
}

BOOST_AUTO_TEST_SUITE_END()
