#include <boost/test/unit_test.hpp>
#include <test/setup_common.h>

BOOST_FIXTURE_TEST_SUITE(dummy_integration_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(test_integration_addr)
{
    const auto vbk_integration_endpoint = std::getenv("VBK_INTEGRATION_ENDPOINT");
    BOOST_REQUIRE_MESSAGE(vbk_integration_endpoint != nullptr,
                          "please set VBK_INTEGRATION_ENDPOINT environment variable to "
                          "host:port of the VeriBlock security service");
    BOOST_TEST_MESSAGE("VBK_INTEGRATION_ENDPOINT: " << vbk_integration_endpoint);
}

BOOST_AUTO_TEST_SUITE_END()
