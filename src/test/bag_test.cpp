// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/bag.h>

#include <boost/test/unit_test.hpp>

#include <set>
#include <vector>

BOOST_AUTO_TEST_SUITE(bag_tests)

// Simple struct that keeps track of the number of copies, moves, etc.
// It has no default constructor.
struct CountCopyAndMoves {
    static int n_ctor;
    static int n_copy_ctor;
    static int n_copy_assignment;
    static int n_move_ctor;
    static int n_move_assignment;
    static int n_dtor;

    int m_x;

    static void ClearCounters()
    {
        n_ctor = 0;
        n_copy_ctor = 0;
        n_copy_assignment = 0;
        n_move_ctor = 0;
        n_move_assignment = 0;
        n_dtor = 0;
    }

    CountCopyAndMoves(int x) : m_x{x}
    {
        ++n_ctor;
    }

    CountCopyAndMoves(const CountCopyAndMoves& other)
        : m_x(other.m_x)
    {
        ++n_copy_ctor;
    }

    CountCopyAndMoves& operator=(const CountCopyAndMoves& other)
    {
        m_x = other.m_x;
        ++n_copy_assignment;
        return *this;
    }

    CountCopyAndMoves(CountCopyAndMoves&& other) noexcept
        : m_x(other.m_x)
    {
        other.m_x = -1;
        ++n_move_ctor;
    }

    CountCopyAndMoves& operator=(CountCopyAndMoves&& other) noexcept
    {
        m_x = other.m_x;
        other.m_x = -1;
        ++n_move_assignment;
        return *this;
    }

    ~CountCopyAndMoves()
    {
        ++n_dtor;
    }
};

int CountCopyAndMoves::n_ctor = 0;
int CountCopyAndMoves::n_copy_ctor = 0;
int CountCopyAndMoves::n_copy_assignment = 0;
int CountCopyAndMoves::n_move_ctor = 0;
int CountCopyAndMoves::n_move_assignment = 0;
int CountCopyAndMoves::n_dtor = 0;

BOOST_AUTO_TEST_CASE(bag_simple)
{
    CountCopyAndMoves::ClearCounters();

    Bag<CountCopyAndMoves> bag{};
    BOOST_TEST(bag.empty());

    {
        std::vector<CountCopyAndMoves> entries;
        entries.emplace_back(1);
        entries.emplace_back(2);
        bag.push(std::move(entries));
    }
    BOOST_TEST(bag.size() == 2);
    BOOST_TEST(!bag.empty());

    {
        std::vector<CountCopyAndMoves> data;
        data.emplace_back(123);
        bag.pop(1, data);
        BOOST_TEST(bag.size() == 1);
        BOOST_TEST(data.front().m_x != 123);
        std::set<int> taken_out{};
        taken_out.insert(data.front().m_x);

        bag.pop(100, data);
        BOOST_TEST(bag.empty());
        BOOST_TEST(data.size() == 1);
        taken_out.insert(data.front().m_x);

        BOOST_TEST((taken_out == std::set<int>{1, 2}));
    }

    BOOST_TEST(CountCopyAndMoves::n_ctor + CountCopyAndMoves::n_move_ctor == CountCopyAndMoves::n_dtor);
    BOOST_TEST(CountCopyAndMoves::n_copy_assignment == 0);
    BOOST_TEST(CountCopyAndMoves::n_copy_ctor == 0);
}


BOOST_AUTO_TEST_CASE(bag_larger)
{
    CountCopyAndMoves::ClearCounters();
    {
        Bag<CountCopyAndMoves> bag{};
        {
            std::vector<CountCopyAndMoves> data;
            data.reserve(100);
            for (int i = 0; i < 100; ++i) {
                data.emplace_back(i);
            }
            bag.push(std::move(data));
        }
        BOOST_TEST(CountCopyAndMoves::n_ctor + CountCopyAndMoves::n_move_ctor - CountCopyAndMoves::n_dtor == 100);
    }
    BOOST_TEST(CountCopyAndMoves::n_ctor + CountCopyAndMoves::n_move_ctor == CountCopyAndMoves::n_dtor);
    BOOST_TEST(CountCopyAndMoves::n_copy_assignment == 0);
    BOOST_TEST(CountCopyAndMoves::n_copy_ctor == 0);
}

BOOST_AUTO_TEST_SUITE_END()
