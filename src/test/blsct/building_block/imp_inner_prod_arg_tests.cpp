// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <blsct/building_block/imp_inner_prod_arg.h>
#include <blsct/range_proof/setup.h>
#include <blsct/arith/mcl/mcl.h>
#include <blsct/arith/elements.h>
#include <hash.h>
#include <test/util/setup_common.h>
#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(imp_inner_prod_arg_tests, BasicTestingSetup)

using Scalar = Mcl::Scalar;
using Point = Mcl::Point;
using Scalars = Elements<Scalar>;
using Points = Elements<Point>;

BOOST_AUTO_TEST_CASE(test_gen_generator_exponents)
{
    Scalar one = Scalar(1);
    Scalar two = Scalar(2);
    Scalar three = Scalar(3);
    Scalar four = Scalar(4);

    Scalars xs, x_invs;
    xs.Add(one);
    xs.Add(two);
    xs.Add(three);
    xs.Add(four);

    auto res = ImpInnerProdArg::GenGeneratorExponents<Mcl>(4, xs);

    BOOST_CHECK(res[0].GetString(16) == "6f1915afb28c42ba866cc45d093b19afc595bd2d8aa91829b555555460000001");
    BOOST_CHECK(res[1].GetString(16) == "26a48d1bb889d46d66689d580335f2ac713f36abaaaa1eaa5555555500000001");
    BOOST_CHECK(res[2].GetString(16) == "48748893fa026e4d200427050605270354568681dffef97f5fffffff60000001");
    BOOST_CHECK(res[3].GetString(16) == "6");
    BOOST_CHECK(res[4].GetString(16) == "609b60c54d5893118005895c0806deaf1b1e08ad2aa94ca9d555555480000001");
    BOOST_CHECK(res[5].GetString(16) == "26a48d1bb889d46d66689d580335f2ac713f36abaaaa1eaa5555555500000003");
    BOOST_CHECK(res[6].GetString(16) == "39f6d3a994cebea4199cec0404d0ec02a9ded2017fff2dff7fffffff80000002");
    BOOST_CHECK(res[7].GetString(16) == "18");
    BOOST_CHECK(res[8].GetString(16) == "6f1915afb28c42ba866cc45d093b19afc595bd2d8aa91829b555555460000001");
    BOOST_CHECK(res[9].GetString(16) == "26a48d1bb889d46d66689d580335f2ac713f36abaaaa1eaa5555555500000001");
    BOOST_CHECK(res[10].GetString(16) == "48748893fa026e4d200427050605270354568681dffef97f5fffffff60000001");
    BOOST_CHECK(res[11].GetString(16) == "6");
    BOOST_CHECK(res[12].GetString(16) == "609b60c54d5893118005895c0806deaf1b1e08ad2aa94ca9d555555480000001");
    BOOST_CHECK(res[13].GetString(16) == "26a48d1bb889d46d66689d580335f2ac713f36abaaaa1eaa5555555500000003");
    BOOST_CHECK(res[14].GetString(16) == "39f6d3a994cebea4199cec0404d0ec02a9ded2017fff2dff7fffffff80000002");
    BOOST_CHECK(res[15].GetString(16) == "18");
}

BOOST_AUTO_TEST_CASE(test_exec_ypow_loop)
{
    Scalar y(2);
    std::vector<Scalar> act;
    ImpInnerProdArg::LoopWithYPows<Mcl>(3, y,
        [&](const size_t& i, const Scalar& y_pow, const Scalar& y_inv_pow) {
            act.emplace_back(Scalar(i));
            act.emplace_back(y_pow);
            act.emplace_back(y_inv_pow);
        }
    );
    Scalar zero = Scalar(0);
    Scalar one = Scalar(1);
    Scalar two = Scalar(2);
    Scalar four = Scalar(4);
    Scalar one_half = two.Invert();
    Scalar one_fourth = four.Invert();
    std::vector<Scalar> exp {
        zero,
        one,
        one,
        one,
        two,
        one_half,
        two,
        four,
        one_fourth
    };
    BOOST_CHECK(act == exp);
}

BOOST_AUTO_TEST_CASE(test_gen_all_round_xs_xinvs)
{
    HashWriter fiat_shamir{};

    Points Ls, Rs;
    Point g = Point::GetBasePoint();
    Ls.Add(g);
    Ls.Add(g + g);
    Rs.Add(g + g + g);
    Rs.Add(g + g + g + g);

    auto res = ImpInnerProdArg::GenAllRoundXs<Mcl>(2, Ls, Rs, fiat_shamir).value();
    BOOST_CHECK(res.Size() == 2);
    BOOST_CHECK(res[0].GetString(16) == "1549ffc50ba69bf258b57da9e829cf787d7996fb9b6f779667a3d83544f8fac3");
    BOOST_CHECK(res[1].GetString(16) == "198816319c5d3178b6569166c76e75c956e3382487a95fed771d4312686b6e8");
}

BOOST_AUTO_TEST_SUITE_END()
