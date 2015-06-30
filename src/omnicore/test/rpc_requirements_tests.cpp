#include "omnicore/rpcrequirements.h"

#include "json/json_spirit_value.h"

#include <stdint.h>
#include <limits>
#include <string>

#include <boost/test/unit_test.hpp>

using json_spirit::Object;

BOOST_AUTO_TEST_SUITE(omnicore_rpc_requirements_tests)

BOOST_AUTO_TEST_CASE(rpcrequirements_universal)
{
    // The following checks should not fail, even with blank state and no previous
    // transactions.
    BOOST_CHECK_NO_THROW(RequireBalance("1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P", 1, 0));
    BOOST_CHECK_NO_THROW(RequirePrimaryToken(1));
    BOOST_CHECK_NO_THROW(RequirePrimaryToken(2));
    BOOST_CHECK_NO_THROW(RequirePropertyName("TestTokens"));
    BOOST_CHECK_NO_THROW(RequireExistingProperty(1));
    BOOST_CHECK_NO_THROW(RequireExistingProperty(2));
    BOOST_CHECK_NO_THROW(RequireSameEcosystem(0, 0));
    BOOST_CHECK_NO_THROW(RequireSameEcosystem(1, 3));
    BOOST_CHECK_NO_THROW(RequireSameEcosystem(3221225476U, 2));
    BOOST_CHECK_NO_THROW(RequireDifferentIds(100000, 100001));
    BOOST_CHECK_NO_THROW(RequireNoOtherDExOffer("137uFtQ5EgMsreg4FVvL3xuhjkYGToVPqs", 1));
    BOOST_CHECK_NO_THROW(RequireNoOtherDExOffer("332r9K9t581aGVyzMK8uoPQr7ieH26u6J3", 2));
    BOOST_CHECK_NO_THROW(RequireSaneReferenceAmount(0));
    BOOST_CHECK_NO_THROW(RequireSaneReferenceAmount(1));
    BOOST_CHECK_NO_THROW(RequireSaneReferenceAmount(1000000));
    BOOST_CHECK_NO_THROW(RequireHeightInChain(0));
}

BOOST_AUTO_TEST_CASE(rpcrequirements_universal_failure)
{
    // The following checks are in practise impossible to pass, even with blank state and
    // no previous transactions.
    // Some of the checks fail for unrelated reasons, such as the DEx payment window or the
    // fee check: the offers don't exist, which also raises exceptions.
    BOOST_CHECK_THROW(RequireBalance("1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P", 2, int64_t(9223372036854775807LL)), Object);
    BOOST_CHECK_THROW(RequirePrimaryToken(0), Object);
    BOOST_CHECK_THROW(RequirePrimaryToken(31), Object);
    BOOST_CHECK_THROW(RequirePropertyName(""), Object);
    BOOST_CHECK_THROW(RequireExistingProperty(uint32_t(2147483647)), Object); // last main identifier that may be created
    BOOST_CHECK_THROW(RequireExistingProperty(uint32_t(4294967295U)), Object); // last test identifier that may be created
    BOOST_CHECK_THROW(RequireSameEcosystem(2147483650U, 4294967295U), Object);
    BOOST_CHECK_THROW(RequireDifferentIds(4200000000U, 4200000000U), Object);
    BOOST_CHECK_THROW(RequireCrowdsale(uint32_t(2147483647)), Object);
    BOOST_CHECK_THROW(RequireCrowdsale(uint32_t(4294967295U)), Object);
    BOOST_CHECK_THROW(RequireActiveCrowdsale(uint32_t(2147483647)), Object);
    BOOST_CHECK_THROW(RequireActiveCrowdsale(uint32_t(4294967295U)), Object);
    BOOST_CHECK_THROW(RequireManagedProperty(uint32_t(2147483647)), Object);
    BOOST_CHECK_THROW(RequireManagedProperty(uint32_t(4294967295U)), Object);
    BOOST_CHECK_THROW(RequireTokenIssuer("1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa", uint32_t(2147483647)), Object);
    BOOST_CHECK_THROW(RequireSaneDExPaymentWindow("1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa", uint32_t(2147483647)), Object);
    BOOST_CHECK_THROW(RequireSaneDExFee("1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa", uint32_t(2147483647)), Object);
    BOOST_CHECK_THROW(RequireMatchingDExOffer("36fKL6YuaCKkWDkh4gce2E3PsucmDEyrR9", 3), Object);
    BOOST_CHECK_THROW(RequireMatchingDExOffer("1388enJtLbowR6LRQQUTjptWuChirfrhza", 7), Object);
    BOOST_CHECK_THROW(RequireSaneReferenceAmount(1000001), Object);
    BOOST_CHECK_THROW(RequireSaneReferenceAmount(int64_t(210000000000000L)), Object);
    BOOST_CHECK_THROW(RequireHeightInChain(-1), Object);
    BOOST_CHECK_THROW(RequireHeightInChain(std::numeric_limits<int>::max()), Object);
}


BOOST_AUTO_TEST_SUITE_END()
