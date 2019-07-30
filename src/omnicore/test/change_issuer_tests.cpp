#include <omnicore/dbspinfo.h>

#include <test/test_bitcoin.h>

#include <boost/test/unit_test.hpp>

#include <string>
#include <stdint.h>

BOOST_FIXTURE_TEST_SUITE(omnicore_change_issuer_tests, BasicTestingSetup)

/**
 * Even if no updateIssuer was called, it falls back to the currently
 * set issuer.
 */
BOOST_AUTO_TEST_CASE(fallback_issuer)
{
    CMPSPInfo::Entry e;
    e.issuer = "ABC";

    BOOST_CHECK_EQUAL(e.getIssuer(0), "ABC");
    BOOST_CHECK_EQUAL(e.getIssuer(1), "ABC");
    BOOST_CHECK_EQUAL(e.getIssuer(574202), "ABC");
}

/**
 * There are two issuer changes in different blocks.
 */
BOOST_AUTO_TEST_CASE(two_issuer_changes)
{

    CMPSPInfo::Entry e;
    e.issuer = "JUNK";

    e.updateIssuer(123, 5, "First");
    BOOST_CHECK_EQUAL(e.getIssuer(123), "First");
    BOOST_CHECK_EQUAL(e.getIssuer(574202), "First");

    e.updateIssuer(9999, 77, "Second");
    BOOST_CHECK_EQUAL(e.getIssuer(123), "First");
    BOOST_CHECK_EQUAL(e.getIssuer(9999), "Second");
    BOOST_CHECK_EQUAL(e.getIssuer(574202), "Second");
}

/**
 * There are multiple issuer changes per block.
 */
BOOST_AUTO_TEST_CASE(multiple_per_block)
{

    CMPSPInfo::Entry e;
    e.issuer = "JUNK";

    e.updateIssuer(123, 5, "One");
    BOOST_CHECK_EQUAL(e.getIssuer(123), "One");
    BOOST_CHECK_EQUAL(e.getIssuer(574202), "One");

    e.updateIssuer(126, 3, "Two");
    BOOST_CHECK_EQUAL(e.getIssuer(123), "One");
    BOOST_CHECK_EQUAL(e.getIssuer(126), "Two");
    BOOST_CHECK_EQUAL(e.getIssuer(99999), "Two");
    
    e.updateIssuer(126, 55, "Three");
    BOOST_CHECK_EQUAL(e.getIssuer(123), "One");
    BOOST_CHECK_EQUAL(e.getIssuer(126), "Three");
    BOOST_CHECK_EQUAL(e.getIssuer(127), "Three");

    // Position within block 126 is lower than "Three"
    e.updateIssuer(126, 4, "Four");
    BOOST_CHECK_EQUAL(e.getIssuer(123), "One");
    BOOST_CHECK_EQUAL(e.getIssuer(126), "Three");
    BOOST_CHECK_EQUAL(e.getIssuer(574202), "Three");

    e.updateIssuer(99999, 2, "Five");
    BOOST_CHECK_EQUAL(e.getIssuer(123), "One");
    BOOST_CHECK_EQUAL(e.getIssuer(126), "Three");
    BOOST_CHECK_EQUAL(e.getIssuer(574202), "Five");
}

BOOST_AUTO_TEST_SUITE_END()
