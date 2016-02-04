#include <boost/test/unit_test.hpp>

#include <boost/atomic.hpp>

BOOST_AUTO_TEST_SUITE(atomic_tests)

BOOST_AUTO_TEST_CASE(is_lock_free)
{
    boost::atomic<int> i;
    std::cout << i.is_lock_free() << '\n';
    BOOST_CHECK(i.is_lock_free());
}

BOOST_AUTO_TEST_SUITE_END()
