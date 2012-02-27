#include <boost/test/unit_test.hpp>

using namespace std;

#include "mruset.h"
#include "util.h"

#define NUM_TESTS 16
#define MAX_SIZE 100

class mrutester
{
private:
    mruset<int> mru;
    std::set<int> set;

public:
    mrutester() { mru.max_size(MAX_SIZE); }
    int size() const { return set.size(); }

    void insert(int n)
    {
        mru.insert(n);
        set.insert(n);
        BOOST_CHECK(mru == set);
    }
};

BOOST_AUTO_TEST_SUITE(mruset_tests)

// Test that an mruset behaves like a set, as long as no more than MAX_SIZE elements are in it
BOOST_AUTO_TEST_CASE(mruset_like_set)
{

    for (int nTest=0; nTest<NUM_TESTS; nTest++)
    {
        mrutester tester;
        while (tester.size() < MAX_SIZE)
            tester.insert(GetRandInt(2 * MAX_SIZE));
    }

}

// Test that an mruset's size never exceeds its max_size
BOOST_AUTO_TEST_CASE(mruset_limited_size)
{
    for (int nTest=0; nTest<NUM_TESTS; nTest++)
    {
        mruset<int> mru(MAX_SIZE);
        for (int nAction=0; nAction<3*MAX_SIZE; nAction++)
        {
            int n = GetRandInt(2 * MAX_SIZE);
            mru.insert(n);
            BOOST_CHECK(mru.size() <= MAX_SIZE);
        }
    }
}

// 16-bit permutation function
int static permute(int n)
{
    // hexadecimals of pi; verified to be linearly independent
    static const int table[16] = {0x243F, 0x6A88, 0x85A3, 0x08D3, 0x1319, 0x8A2E, 0x0370, 0x7344,
                                  0xA409, 0x3822, 0x299F, 0x31D0, 0x082E, 0xFA98, 0xEC4E, 0x6C89};

    int ret = 0;
    for (int bit=0; bit<16; bit++)
         if (n & (1<<bit))
             ret ^= table[bit];

    return ret;
}

// Test that an mruset acts like a moving window, if no duplcate elements are added
BOOST_AUTO_TEST_CASE(mruset_window)
{
    mruset<int> mru(MAX_SIZE);
    for (int n=0; n<10*MAX_SIZE; n++)
    {
        mru.insert(permute(n));

        set<int> tester;
        for (int m=max(0,n-MAX_SIZE+1); m<=n; m++)
            tester.insert(permute(m));

        BOOST_CHECK(mru == tester);
    }
}

BOOST_AUTO_TEST_SUITE_END()
