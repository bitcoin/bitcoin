// Copyright (c) 2013-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Unit tests for alert system

#include "alert.h"
#include "chain.h"
#include "chainparams.h"
#include "clientversion.h"
#include "data/alertTests.raw.h"
#include "serialize.h"
#include "streams.h"
#include "utilstrencodings.h"

#include "test/testutil.h"
#include "test/test_dash.h"

#include <fstream>

#include <boost/filesystem/operations.hpp>
#include <boost/test/unit_test.hpp>

//
// Sign a CAlert and serialize it
//
bool SignAndSave(CAlert &alert)
{
    // Sign
    if(!alert.Sign())
    {
        printf("SignAndSave() : could not sign alert:\n%s", alert.ToString().c_str());
        return false;
    }

    std::string strFilePath = "src/test/data/alertTests.raw";
    // open output file and associate it with CAutoFile
    FILE *file = fopen(strFilePath.c_str(), "ab+");
    CAutoFile fileout(file, SER_DISK, CLIENT_VERSION);
    if (fileout.IsNull())
        return error("%s: Failed to open file %s", __func__, strFilePath);

    try {
        fileout << alert;
    }
    catch (std::exception &e) {
        return error("%s: Serialize or I/O error - %s", __func__, e.what());
    }
    fileout.fclose();
    return true;
}

//
// alertTests contains 8 alerts, generated with this code
//
void GenerateAlertTests()
{
    CAlert alert;
    alert.nRelayUntil   = 60;
    alert.nExpiration   = 24 * 60 * 60;
    alert.nID           = 1;
    alert.nCancel       = 0;   // cancels previous messages up to this ID number
    alert.nMinVer       = 0;  // These versions are protocol versions
    alert.nMaxVer       = 999001;
    alert.nPriority     = 1;
    alert.strComment    = "Alert comment";
    alert.strStatusBar  = "Alert 1";

    SignAndSave(alert);

    alert.setSubVer.insert(std::string("/Satoshi:0.1.0/"));
    alert.strStatusBar  = "Alert 1 for Satoshi 0.1.0";
    SignAndSave(alert);

    alert.setSubVer.insert(std::string("/Satoshi:0.2.0/"));
    alert.strStatusBar  = "Alert 1 for Satoshi 0.1.0, 0.2.0";
    SignAndSave(alert);

    alert.setSubVer.clear();
    ++alert.nID;
    alert.nCancel = 1;
    alert.nPriority = 100;
    alert.strStatusBar  = "Alert 2, cancels 1";
    SignAndSave(alert);

    alert.nExpiration += 60;
    ++alert.nID;
    SignAndSave(alert);

    ++alert.nID;
    alert.nMinVer = 11;
    alert.nMaxVer = 22;
    SignAndSave(alert);

    ++alert.nID;
    alert.strStatusBar  = "Alert 2 for Satoshi 0.1.0";
    alert.setSubVer.insert(std::string("/Satoshi:0.1.0/"));
    SignAndSave(alert);

    ++alert.nID;
    alert.nMinVer = 0;
    alert.nMaxVer = 999999;
    alert.strStatusBar  = "Evil Alert'; /bin/ls; echo '";
    alert.setSubVer.clear();
    SignAndSave(alert);
}

struct ReadAlerts : public TestingSetup
{
    ReadAlerts()
    {
        std::vector<unsigned char> vch(raw_tests::alertTests, raw_tests::alertTests + sizeof(raw_tests::alertTests));
        CDataStream stream(vch, SER_DISK, CLIENT_VERSION);
        try {
            while (!stream.eof())
            {
                CAlert alert;
                stream >> alert;
                alerts.push_back(alert);
            }
        }
        catch (const std::exception&) { }
    }
    ~ReadAlerts() { }

    static std::vector<std::string> read_lines(boost::filesystem::path filepath)
    {
        std::vector<std::string> result;

        std::ifstream f(filepath.string().c_str());
        std::string line;
        while (std::getline(f,line))
            result.push_back(line);

        return result;
    }

    std::vector<CAlert> alerts;
};

BOOST_FIXTURE_TEST_SUITE(Alert_tests, ReadAlerts)

// Steps to generate alert tests:
// - update alerts in GenerateAlertTests() (optional)
// - enable code below (#if 1)
// - replace "fffffffffffffffffffffffffffffffffffffffffffffffffff" with the actual MAINNET privkey
// - recompile and run "/path/to/test_dash -t Alert_test"
//
// NOTE: make sure to disable code and remove alert privkey when you're done!
//
#if 0
BOOST_AUTO_TEST_CASE(GenerateAlerts)
{
    SoftSetArg("-alertkey", "fffffffffffffffffffffffffffffffffffffffffffffffffff");
    GenerateAlertTests();
}
#endif

BOOST_AUTO_TEST_CASE(AlertApplies)
{
    SetMockTime(11);
    const std::vector<unsigned char>& alertKey = Params(CBaseChainParams::MAIN).AlertKey();

    for (const auto& alert : alerts)
    {
        BOOST_CHECK(alert.CheckSignature(alertKey));
    }

    BOOST_CHECK(alerts.size() >= 3);

    // Matches:
    BOOST_CHECK(alerts[0].AppliesTo(1, ""));
    BOOST_CHECK(alerts[0].AppliesTo(999001, ""));
    BOOST_CHECK(alerts[0].AppliesTo(1, "/Satoshi:11.11.11/"));

    BOOST_CHECK(alerts[1].AppliesTo(1, "/Satoshi:0.1.0/"));
    BOOST_CHECK(alerts[1].AppliesTo(999001, "/Satoshi:0.1.0/"));

    BOOST_CHECK(alerts[2].AppliesTo(1, "/Satoshi:0.1.0/"));
    BOOST_CHECK(alerts[2].AppliesTo(1, "/Satoshi:0.2.0/"));

    // Don't match:
    BOOST_CHECK(!alerts[0].AppliesTo(-1, ""));
    BOOST_CHECK(!alerts[0].AppliesTo(999002, ""));

    BOOST_CHECK(!alerts[1].AppliesTo(1, ""));
    BOOST_CHECK(!alerts[1].AppliesTo(1, "Satoshi:0.1.0"));
    BOOST_CHECK(!alerts[1].AppliesTo(1, "/Satoshi:0.1.0"));
    BOOST_CHECK(!alerts[1].AppliesTo(1, "Satoshi:0.1.0/"));
    BOOST_CHECK(!alerts[1].AppliesTo(-1, "/Satoshi:0.1.0/"));
    BOOST_CHECK(!alerts[1].AppliesTo(999002, "/Satoshi:0.1.0/"));
    BOOST_CHECK(!alerts[1].AppliesTo(1, "/Satoshi:0.2.0/"));

    BOOST_CHECK(!alerts[2].AppliesTo(1, "/Satoshi:0.3.0/"));

    SetMockTime(0);
}


BOOST_AUTO_TEST_CASE(AlertNotify)
{
    SetMockTime(11);
    const std::vector<unsigned char>& alertKey = Params(CBaseChainParams::MAIN).AlertKey();

    boost::filesystem::path temp = GetTempPath() /
        boost::filesystem::unique_path("alertnotify-%%%%.txt");

    ForceSetArg("-alertnotify", std::string("echo %s >> ") + temp.string());

    for (const auto& alert : alerts)
        alert.ProcessAlert(alertKey, false);

    std::vector<std::string> r = read_lines(temp);
    BOOST_CHECK_EQUAL(r.size(), 4u);

// Windows built-in echo semantics are different than posixy shells. Quotes and
// whitespace are printed literally.

#ifndef WIN32
    BOOST_CHECK_EQUAL(r[0], "Alert 1");
    BOOST_CHECK_EQUAL(r[1], "Alert 2, cancels 1");
    BOOST_CHECK_EQUAL(r[2], "Alert 2, cancels 1");
    BOOST_CHECK_EQUAL(r[3], "Evil Alert; /bin/ls; echo "); // single-quotes should be removed
#else
    BOOST_CHECK_EQUAL(r[0], "'Alert 1' ");
    BOOST_CHECK_EQUAL(r[1], "'Alert 2, cancels 1' ");
    BOOST_CHECK_EQUAL(r[2], "'Alert 2, cancels 1' ");
    BOOST_CHECK_EQUAL(r[3], "'Evil Alert; /bin/ls; echo ' ");
#endif
    boost::filesystem::remove(temp);

    SetMockTime(0);
}

BOOST_AUTO_TEST_SUITE_END()
