#include "omnicore/notifications.h"
#include "omnicore/version.h"

#include "util.h"
#include "test/test_bitcoin.h"
#include "tinyformat.h"

#include <boost/test/unit_test.hpp>

#include <stdint.h>
#include <map>
#include <string>
#include <vector>

using namespace mastercore;

// Is only temporarily modified and restored after each test
extern std::map<std::string, std::string> mapArgs;
extern std::map<std::string, std::vector<std::string> > mapMultiArgs;

BOOST_FIXTURE_TEST_SUITE(omnicore_alert_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(alert_positive_authorization)
{
    // Confirm authorized sources for mainnet
    BOOST_CHECK(CheckAlertAuthorization("16Zwbujf1h3v1DotKcn9XXt1m7FZn2o4mj")); // Craig   <craig@omni.foundation>
    BOOST_CHECK(CheckAlertAuthorization("1MicH2Vu4YVSvREvxW1zAx2XKo2GQomeXY")); // Michael <michael@omni.foundation>
    BOOST_CHECK(CheckAlertAuthorization("1zAtHRASgdHvZDfHs6xJquMghga4eG7gy"));  // Zathras <zathras@omni.foundation>
    BOOST_CHECK(CheckAlertAuthorization("1dexX7zmPen1yBz2H9ZF62AK5TGGqGTZH"));  // dexX7   <dexx@bitwatch.co>
    BOOST_CHECK(CheckAlertAuthorization("1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P")); // Exodus (who has access?)

    // Confirm authorized sources for testnet
    BOOST_CHECK(CheckAlertAuthorization("mpDex4kSX4iscrmiEQ8fBiPoyeTH55z23j")); // Michael <michael@omni.foundation>
    BOOST_CHECK(CheckAlertAuthorization("mpZATHupfCLqet5N1YL48ByCM1ZBfddbGJ")); // Zathras <zathras@omni.foundation>
    BOOST_CHECK(CheckAlertAuthorization("mk5SSx4kdexENHzLxk9FLhQdbbBexHUFTW")); // dexX7   <dexx@bitwatch.co>
}

BOOST_AUTO_TEST_CASE(alert_unauthorized_source)
{
    // Confirm unauthorized sources are not accepted
    BOOST_CHECK(!CheckAlertAuthorization("1JwSSubhmg6iPtRjtyqhUYYH7bZg3Lfy1T"));
}

BOOST_AUTO_TEST_CASE(alert_manual_sources)
{
    std::map<std::string, std::string> mapArgsOriginal = mapArgs;
    std::map<std::string, std::vector<std::string> > mapMultiArgsOriginal = mapMultiArgs;

    mapArgs["-omnialertallowsender"] = "";
    mapArgs["-omnialertignoresender"] = "";

    // Add 1JwSSu as allowed source for alerts
    mapMultiArgs["-omnialertallowsender"].push_back("1JwSSubhmg6iPtRjtyqhUYYH7bZg3Lfy1T");
    BOOST_CHECK(CheckAlertAuthorization("1JwSSubhmg6iPtRjtyqhUYYH7bZg3Lfy1T"));

    // Then ignore some sources explicitly
    mapMultiArgs["-omnialertignoresender"].push_back("1JwSSubhmg6iPtRjtyqhUYYH7bZg3Lfy1T");
    mapMultiArgs["-omnialertignoresender"].push_back("1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P");
    BOOST_CHECK(CheckAlertAuthorization("1zAtHRASgdHvZDfHs6xJquMghga4eG7gy")); // should still be authorized
    BOOST_CHECK(!CheckAlertAuthorization("1JwSSubhmg6iPtRjtyqhUYYH7bZg3Lfy1T"));
    BOOST_CHECK(!CheckAlertAuthorization("1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P"));

    mapMultiArgs = mapMultiArgsOriginal;
    mapArgs = mapArgsOriginal;
}

BOOST_AUTO_TEST_CASE(alert_authorize_any_source)
{
    std::map<std::string, std::string> mapArgsOriginal = mapArgs;
    std::map<std::string, std::vector<std::string> > mapMultiArgsOriginal = mapMultiArgs;

    mapArgs["-omnialertallowsender"] = "";

    // Allow any source (e.g. for tests!)
    mapMultiArgs["-omnialertallowsender"].push_back("any");
    BOOST_CHECK(CheckAlertAuthorization("1JwSSubhmg6iPtRjtyqhUYYH7bZg3Lfy1T"));
    BOOST_CHECK(CheckAlertAuthorization("137uFtQ5EgMsreg4FVvL3xuhjkYGToVPqs"));
    BOOST_CHECK(CheckAlertAuthorization("1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P"));

    mapMultiArgs = mapMultiArgsOriginal;
    mapArgs = mapArgsOriginal;
}

BOOST_AUTO_TEST_SUITE_END()
