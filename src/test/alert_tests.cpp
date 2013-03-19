//
// Unit tests for alert system
//

#include <boost/foreach.hpp>
#include <boost/test/unit_test.hpp>

#include "alert.h"
#include "serialize.h"
#include "util.h"

BOOST_AUTO_TEST_SUITE(Alert_tests)

#if 0
//
// alertTests contains 7 alerts, generated with this code:
// (SignAndSave code not shown, alert signing key is secret)
//
{
    CAlert alert;
    alert.nRelayUntil   = 60;
    alert.nExpiration   = 24 * 60 * 60;
    alert.nID           = 1;
    alert.nCancel       = 0;   // cancels previous messages up to this ID number
    alert.nMinVer       = 0;  // These versions are protocol versions
    alert.nMaxVer       = 70001;
    alert.nPriority     = 1;
    alert.strComment    = "Alert comment";
    alert.strStatusBar  = "Alert 1";

    SignAndSave(alert, "test/alertTests");

    alert.setSubVer.insert(std::string("/Satoshi:0.1.0/"));
    alert.strStatusBar  = "Alert 1 for Satoshi 0.1.0";
    SignAndSave(alert, "test/alertTests");

    alert.setSubVer.insert(std::string("/Satoshi:0.2.0/"));
    alert.strStatusBar  = "Alert 1 for Satoshi 0.1.0, 0.2.0";
    SignAndSave(alert, "test/alertTests");

    alert.setSubVer.clear();
    alert.nID = 2;
    alert.nCancel = 1;
    alert.strStatusBar  = "Alert 2, cancels 1";
    SignAndSave(alert, "test/alertTests");

    alert.nExpiration += 60;
    SignAndSave(alert, "test/alertTests");

    alert.nMinVer = 11;
    alert.nMaxVer = 22;
    SignAndSave(alert, "test/alertTests");

    alert.strStatusBar  = "Alert 2 for Satoshi 0.1.0";
    alert.setSubVer.insert(std::string("/Satoshi:0.1.0/"));
    SignAndSave(alert, "test/alertTests");
}
#endif


std::vector<CAlert>
read_alerts(const std::string& filename)
{
    std::vector<CAlert> result;

    namespace fs = boost::filesystem;
    fs::path testFile = fs::current_path() / "test" / "data" / filename;
#ifdef TEST_DATA_DIR
    if (!fs::exists(testFile))
    {
        testFile = fs::path(BOOST_PP_STRINGIZE(TEST_DATA_DIR)) / filename;
    }
#endif
    FILE* fp = fopen(testFile.string().c_str(), "rb");
    if (!fp) return result;


    CAutoFile filein = CAutoFile(fp, SER_DISK, CLIENT_VERSION);
    if (!filein) return result;

    try {
        while (!feof(filein))
        {
            CAlert alert;
            filein >> alert;
            result.push_back(alert);
        }
    }
    catch (std::exception) { }

    return result;
}

BOOST_AUTO_TEST_CASE(AlertApplies)
{
    SetMockTime(11);

    std::vector<CAlert> alerts = read_alerts("alertTests");

    BOOST_FOREACH(const CAlert& alert, alerts)
    {
        BOOST_CHECK(alert.CheckSignature());
    }
    // Matches:
    BOOST_CHECK(alerts[0].AppliesTo(1, ""));
    BOOST_CHECK(alerts[0].AppliesTo(70001, ""));
    BOOST_CHECK(alerts[0].AppliesTo(1, "/Satoshi:11.11.11/"));

    BOOST_CHECK(alerts[1].AppliesTo(1, "/Satoshi:0.1.0/"));
    BOOST_CHECK(alerts[1].AppliesTo(70001, "/Satoshi:0.1.0/"));

    BOOST_CHECK(alerts[2].AppliesTo(1, "/Satoshi:0.1.0/"));
    BOOST_CHECK(alerts[2].AppliesTo(1, "/Satoshi:0.2.0/"));

    // Don't match:
    BOOST_CHECK(!alerts[0].AppliesTo(-1, ""));
    BOOST_CHECK(!alerts[0].AppliesTo(70002, ""));

    BOOST_CHECK(!alerts[1].AppliesTo(1, ""));
    BOOST_CHECK(!alerts[1].AppliesTo(1, "Satoshi:0.1.0"));
    BOOST_CHECK(!alerts[1].AppliesTo(1, "/Satoshi:0.1.0"));
    BOOST_CHECK(!alerts[1].AppliesTo(1, "Satoshi:0.1.0/"));
    BOOST_CHECK(!alerts[1].AppliesTo(-1, "/Satoshi:0.1.0/"));
    BOOST_CHECK(!alerts[1].AppliesTo(70002, "/Satoshi:0.1.0/"));
    BOOST_CHECK(!alerts[1].AppliesTo(1, "/Satoshi:0.2.0/"));

    BOOST_CHECK(!alerts[2].AppliesTo(1, "/Satoshi:0.3.0/"));

    SetMockTime(0);
}

BOOST_AUTO_TEST_SUITE_END()
