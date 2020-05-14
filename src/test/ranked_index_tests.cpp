// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/ranked_index.hpp>

#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

namespace {

struct Tag1 {};
struct Tag2 {};
struct Tag3 {};
struct Tag4 {};

struct Data {
    int a;
    int* b;

    Data(int v, int* p) : a(v), b(p) {}

    friend bool operator<(const Data& p, const Data& q) { return std::tie(p.a, p.b) < std::tie(q.a, q.b); }
};

typedef boost::multi_index_container<
    Data,
    boost::multi_index::indexed_by<
        boost::multi_index::hashed_unique<
            boost::multi_index::tag<Tag1>,
            boost::multi_index::member<Data, int, &Data::a>
        >,
        boost::multi_index::ranked_non_unique<
            boost::multi_index::tag<Tag2>,
            boost::multi_index::identity<Data>
        >,
        boost::multi_index::ordered_non_unique<
            boost::multi_index::tag<Tag3>,
            boost::multi_index::member<Data, int*, &Data::b>
        >,
        boost::multi_index::ranked_unique<
            boost::multi_index::tag<Tag4>,
            boost::multi_index::member<Data, int, &Data::a>
        >
    >
> Index;

}

BOOST_FIXTURE_TEST_SUITE(ranked_index_tests, TestingSetup)

BOOST_AUTO_TEST_CASE(ranked_index_test)
{
    Index index;

    int v = 5;
    index.emplace(v, &v);

    BOOST_CHECK(index.get<Tag2>().nth(0)->a == v);
    BOOST_CHECK(index.get<Tag4>().nth(0)->a == v);
    BOOST_CHECK_EQUAL(index.get<Tag2>().find_rank(Data(v, &v)), 0U);
    BOOST_CHECK_EQUAL(index.get<Tag4>().find_rank(v), 0U);
    BOOST_CHECK_EQUAL(index.get<Tag2>().lower_bound_rank(Data(v, &v)), 0U);
    BOOST_CHECK_EQUAL(index.get<Tag4>().upper_bound_rank(v), 1U);
}

BOOST_AUTO_TEST_SUITE_END()
