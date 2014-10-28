// Copyright (c) 2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "alert.h"
#include "util.h"
#include "utilstrencodings.h"
#include "key.h"

#include <boost/foreach.hpp>

//
// alertTests contains 7 alerts, generated with this code:
//
std::vector<CAlert> GenAlertTests()
{
    std::vector<CAlert> alerts;
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

    alerts.push_back(alert);

    alert.setSubVer.insert(std::string("/Satoshi:0.1.0/"));
    alert.strStatusBar  = "Alert 1 for Satoshi 0.1.0";
    alerts.push_back(alert);

    alert.setSubVer.insert(std::string("/Satoshi:0.2.0/"));
    alert.strStatusBar  = "Alert 1 for Satoshi 0.1.0, 0.2.0";
    alerts.push_back(alert);

    alert.setSubVer.clear();
    ++alert.nID;
    alert.nCancel = 1;
    alert.nPriority = 100;
    alert.strStatusBar  = "Alert 2, cancels 1";
    alerts.push_back(alert);

    alert.nExpiration += 60;
    ++alert.nID;
    alerts.push_back(alert);

    ++alert.nID;
    alert.nMinVer = 11;
    alert.nMaxVer = 22;
    alerts.push_back(alert);

    ++alert.nID;
    alert.strStatusBar  = "Alert 2 for Satoshi 0.1.0";
    alert.setSubVer.insert(std::string("/Satoshi:0.1.0/"));
    alerts.push_back(alert);

    ++alert.nID;
    alert.nMinVer = 0;
    alert.nMaxVer = 999999;
    alert.strStatusBar  = "Evil Alert'; /bin/ls; echo '";
    alert.setSubVer.clear();
    alerts.push_back(alert);

    return alerts;
}

void SignAlert(CAlert &alert, const CKey &key, const CPubKey &pubkey)
{
    CDataStream sMsg(SER_NETWORK, CLIENT_VERSION);
    sMsg << *(CUnsignedAlert*)&alert;
    alert.vchMsg = std::vector<unsigned char>(sMsg.begin(), sMsg.end());
    if (!key.Sign(Hash(alert.vchMsg.begin(), alert.vchMsg.end()), alert.vchSig))
    {
        fprintf(stderr, "%s: Signing failed\n", __func__);
        exit(1);
    }
    if (!pubkey.Verify(Hash(alert.vchMsg.begin(), alert.vchMsg.end()), alert.vchSig))
    {
        fprintf(stderr, "%s: Verification failed\n", __func__);
        exit(1);
    }
}

std::vector<CAlert> GenKey(std::string &outPriv, const std::string &networkName)
{
    CKey key;
    key.MakeNewKey(true);

    CPrivKey privkey = key.GetPrivKey();
    CPubKey pubkey = key.GetPubKey();

    outPriv += "const char* psz" + networkName + "PrivKey = \"";
    outPriv += HexStr(privkey);
    outPriv += "\";\n";

    outPriv += "// Add the following to chainparams.cpp constructor for " + networkName + "\n";
    outPriv += "// vAlertPubKey = ParseHex(\"";
    outPriv += HexStr(pubkey);
    outPriv += "\");\n";

    /// Generate and sign test vectors
    std::vector<CAlert> alerts = GenAlertTests();
    BOOST_FOREACH(CAlert &alert, alerts)
        SignAlert(alert, key, pubkey);

    return alerts;
}

void WriteTests(const boost::filesystem::path &testsFileName, const std::vector<CAlert> &alerts)
{
    CAutoFile fileout(fopen(testsFileName.string().c_str(), "wb"), SER_DISK, CLIENT_VERSION);
    if (!fileout)
    {
        fprintf(stderr, "Could not write test data to %s, provide valid -testoutdir\n", testsFileName.string().c_str());
        exit(1);
    }
    fileout << alerts;
}

int GenAlertKey(int argc, char *argv[])
{
    ParseParameters(argc, argv);

    // Generate alertKeys.h
    std::string outPriv;

    outPriv = "// Private alert key header - CONFIDENTIAL\n\n";
    std::vector<CAlert> testsMainNet = GenKey(outPriv, "MainNet");
    outPriv += "\n";
    std::vector<CAlert> testsTestNet = GenKey(outPriv, "TestNet");

    boost::filesystem::path testoutdir = GetArg("-testoutdir", "test/data");

    WriteTests(testoutdir / "alertTestsMainNet.raw", testsMainNet);
    WriteTests(testoutdir / "alertTestsTestNet.raw", testsTestNet);

    printf("%s\n", outPriv.c_str());
    return 0;
}

int main(int argc, char* argv[])
{
    SetupEnvironment();
    int ret = EXIT_FAILURE;
    try {
        ret = GenAlertKey(argc, argv);
    }
    catch (std::exception& e) {
        PrintExceptionContinue(&e, "GenAlertKey()");
    } catch (...) {
        PrintExceptionContinue(NULL, "GenAlertKey()");
    }
    return ret;
}


