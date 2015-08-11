#include "omnicore/notifications.h"
#include "omnicore/version.h"

#include "util.h"
#include "tinyformat.h"

#include <stdint.h>
#include <map>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

using namespace mastercore;

// Is only temporarily modified and restored after each test
extern std::map<std::string, std::string> mapArgs;
extern std::map<std::string, std::vector<std::string> > mapMultiArgs;

BOOST_AUTO_TEST_SUITE(omnicore_alert_tests)

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
/*
BOOST_AUTO_TEST_CASE(alert_parsing)
{
    int32_t alertType = 0;
    uint64_t expiryValue = 0;
    uint32_t typeCheck = 0;
    uint32_t verCheck = 0;
    std::string alertMessage;

    // Malformed alerts
    BOOST_CHECK(!ParseAlertMessage("", alertType, expiryValue, typeCheck, verCheck, alertMessage));
    BOOST_CHECK(!ParseAlertMessage("test", alertType, expiryValue, typeCheck, verCheck, alertMessage));
    BOOST_CHECK(!ParseAlertMessage("x:x:x:x:x", alertType, expiryValue, typeCheck, verCheck, alertMessage));
    BOOST_CHECK(!ParseAlertMessage("1:1:0:0:x:x", alertType, expiryValue, typeCheck, verCheck, alertMessage));
    BOOST_CHECK(!ParseAlertMessage("0:1:0:0:x", alertType, expiryValue, typeCheck, verCheck, alertMessage));
    BOOST_CHECK(!ParseAlertMessage("5:1:0:0:x", alertType, expiryValue, typeCheck, verCheck, alertMessage));
    BOOST_CHECK(!ParseAlertMessage("2:0:0:0:x", alertType, expiryValue, typeCheck, verCheck, alertMessage));

    // Valid alerts
    BOOST_CHECK(ParseAlertMessage("1:1:0:0:x", alertType, expiryValue, typeCheck, verCheck, alertMessage));
    BOOST_CHECK(ParseAlertMessage("2:1:0:0:x", alertType, expiryValue, typeCheck, verCheck, alertMessage));
    BOOST_CHECK(ParseAlertMessage("3:1:0:0:x", alertType, expiryValue, typeCheck, verCheck, alertMessage));
    BOOST_CHECK(ParseAlertMessage("4:1:1:1:x", alertType, expiryValue, typeCheck, verCheck, alertMessage));

    // Confrim correct values
    std::string testAlert = "4:350000:99:0:Transaction type 99v0 is not supported";
    BOOST_CHECK(ParseAlertMessage(testAlert, alertType, expiryValue, typeCheck, verCheck, alertMessage));
    BOOST_CHECK_EQUAL(alertType, 4);
    BOOST_CHECK_EQUAL(expiryValue, 350000);
    BOOST_CHECK_EQUAL(typeCheck, 99);
    BOOST_CHECK_EQUAL(verCheck, 0);
    BOOST_CHECK_EQUAL(alertMessage, "Transaction type 99v0 is not supported");
}

BOOST_AUTO_TEST_CASE(alert_expiration_by_block)
{
    std::string testAlertMessage = "This alert expires after block 350000";
    std::string testAlertRaw = "1:350000:0:0:" + testAlertMessage;

    SetOmniCoreAlert(testAlertRaw);
    BOOST_CHECK_EQUAL(GetOmniCoreAlertString(), testAlertRaw);
    BOOST_CHECK_EQUAL(GetOmniCoreAlertTextOnly(), testAlertMessage);
    BOOST_CHECK(!CheckExpiredAlerts(0, 0));
    BOOST_CHECK(!CheckExpiredAlerts(350000, 0));

    // Expire the the alert
    BOOST_CHECK(CheckExpiredAlerts(350001, 0));
    BOOST_CHECK(GetOmniCoreAlertString().empty());
    BOOST_CHECK(GetOmniCoreAlertTextOnly().empty());
}

BOOST_AUTO_TEST_CASE(alert_expiration_by_timestamp)
{
    std::string testAlertMessage = "This alert expires after timestamp 2147483648";
    std::string testAlertRaw = "2:2147483648:0:0:" + testAlertMessage;

    SetOmniCoreAlert(testAlertRaw);
    BOOST_CHECK_EQUAL(GetOmniCoreAlertString(), testAlertRaw);
    BOOST_CHECK_EQUAL(GetOmniCoreAlertTextOnly(), testAlertMessage);
    BOOST_CHECK(!CheckExpiredAlerts(0, 0));
    BOOST_CHECK(!CheckExpiredAlerts(2147483648U, 0));
    BOOST_CHECK(!CheckExpiredAlerts(350000, 2147483648U));

    // Expire the the alert
    BOOST_CHECK(CheckExpiredAlerts(350001, 2147483649U));
    BOOST_CHECK(GetOmniCoreAlertString().empty());
    BOOST_CHECK(GetOmniCoreAlertTextOnly().empty());
}

BOOST_AUTO_TEST_CASE(alert_expiration_by_version)
{
    // Set an old version and and expire the alert
    std::string testAlertMessage = "This alert is already expired";
    std::string testAlertRaw = strprintf("3:%d:0:0:%s", OMNICORE_VERSION - 1, testAlertMessage);
    SetOmniCoreAlert(testAlertRaw);
    BOOST_CHECK(CheckExpiredAlerts(358867, 1433110322));
    BOOST_CHECK(GetOmniCoreAlertString().empty());
    BOOST_CHECK(GetOmniCoreAlertTextOnly().empty());

    // Target the current version
    testAlertMessage = strprintf("This alert is visible to version %s", OmniCoreVersion());
    testAlertRaw = strprintf("3:%d:0:0:%s", OMNICORE_VERSION, testAlertMessage);
    SetOmniCoreAlert(testAlertRaw);
    BOOST_CHECK(!CheckExpiredAlerts(358867, 1433110322));
    BOOST_CHECK_EQUAL(GetOmniCoreAlertString(), testAlertRaw);
    BOOST_CHECK_EQUAL(GetOmniCoreAlertTextOnly(), testAlertMessage);

    // Clear alert
    SetOmniCoreAlert("");
    BOOST_CHECK(GetOmniCoreAlertString().empty());
    BOOST_CHECK(GetOmniCoreAlertTextOnly().empty());
}
*/

BOOST_AUTO_TEST_SUITE_END()
