
#include <boost/test/unit_test.hpp>

#include "uint256.h"
#include "filter.h"

using namespace std;

BOOST_AUTO_TEST_SUITE(filter_tests)

BOOST_AUTO_TEST_CASE(filter_test1)
{
    CBloomers bloomers;

    vector<uint160> hash;
    hash.push_back(0xaabbccdd11223344ULL);
    hash.push_back(0x9988776655443322ULL);

    vector<string> hashstr;
    int i;

    for (i = 0; i < 2; i++) {
        string s;
        s.assign(hash[i].begin(), hash[i].end());
        hashstr.push_back(s);
    }

    for (i = 0; i < 2; i++)
        bloomers.add("main", hashstr[i]);

    for (i = 0; i < 2; i++) {
        vector<string> rv;
        rv = bloomers.contains(hashstr[i]);

        BOOST_CHECK(rv.size() == 1);
        BOOST_CHECK(rv[0] == "main");
    }
}

BOOST_AUTO_TEST_SUITE_END()

