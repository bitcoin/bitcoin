#include "omnicore/version.h"

#include "config/bitcoin-config.h"

#include <string>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(omnicore_version_tests)

BOOST_AUTO_TEST_CASE(version_comparison)
{
    BOOST_CHECK(OMNICORE_VERSION > 900300); // Master Core v0.0.9.3
}

/**
 * The following tests are very unhandy, because any version bump
 * breaks the tests.
 *
 * TODO: might be removed completely.
 */

BOOST_AUTO_TEST_CASE(version_string)
{
    BOOST_CHECK_EQUAL(OmniCoreVersion(), "0.0.10-rel");
}

BOOST_AUTO_TEST_CASE(version_number)
{
    BOOST_CHECK_EQUAL(OMNICORE_VERSION, 1000000);
}

BOOST_AUTO_TEST_CASE(config_package_version)
{
    // the package version is used in the file names:
    BOOST_CHECK_EQUAL(PACKAGE_VERSION, "0.0.10.0-rel");
}


BOOST_AUTO_TEST_SUITE_END()
