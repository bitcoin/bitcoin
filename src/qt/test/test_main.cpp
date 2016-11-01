// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <chainparams.h>
#include <qt/test/rpcnestedtests.h>
#include <util.h>
#include <qt/test/uritests.h>
#include <qt/test/compattests.h>

#ifdef ENABLE_WALLET
#include <qt/test/paymentservertests.h>
#include <qt/test/wallettests.h>
#endif

#include <QApplication>
#include <QObject>
#include <QTest>

#include <openssl/ssl.h>

#if defined(QT_STATICPLUGIN)
#include <QtPlugin>
#if QT_VERSION < 0x050000
Q_IMPORT_PLUGIN(qcncodecs)
Q_IMPORT_PLUGIN(qjpcodecs)
Q_IMPORT_PLUGIN(qtwcodecs)
Q_IMPORT_PLUGIN(qkrcodecs)
#else
#if defined(QT_QPA_PLATFORM_MINIMAL)
Q_IMPORT_PLUGIN(QMinimalIntegrationPlugin);
#endif
#if defined(QT_QPA_PLATFORM_XCB)
Q_IMPORT_PLUGIN(QXcbIntegrationPlugin);
#elif defined(QT_QPA_PLATFORM_WINDOWS)
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin);
#elif defined(QT_QPA_PLATFORM_COCOA)
Q_IMPORT_PLUGIN(QCocoaIntegrationPlugin);
#endif
#endif
#endif

extern void noui_connect();

// This is all you need to run all the tests
int main(int argc, char *argv[])
{
    SetupEnvironment();
    SetupNetworking();
    SelectParams(CBaseChainParams::MAIN);
    noui_connect();
    ClearDatadirCache();
    fs::path pathTemp = fs::temp_directory_path() / strprintf("test_bitcoin-qt_%lu_%i", (unsigned long)GetTime(), (int)GetRand(100000));
    fs::create_directories(pathTemp);
    gArgs.ForceSetArg("-datadir", pathTemp.string());

    bool fInvalid = false;

    // Prefer the "minimal" platform for the test instead of the normal default
    // platform ("xcb", "windows", or "cocoa") so tests can't unintentionally
    // interfere with any background GUIs and don't require extra resources.
    #if defined(WIN32)
        _putenv_s("QT_QPA_PLATFORM", "minimal");
    #else
        setenv("QT_QPA_PLATFORM", "minimal", 0);
    #endif

    // Don't remove this, it's needed to access
    // QApplication:: and QCoreApplication:: in the tests
    QApplication app(argc, argv);
    app.setApplicationName("Litecoin-Qt-test");

    SSL_library_init();

    URITests test1;
    if (QTest::qExec(&test1) != 0) {
        fInvalid = true;
    }
#ifdef ENABLE_WALLET
    PaymentServerTests test2;
    if (QTest::qExec(&test2) != 0) {
        fInvalid = true;
    }
#endif
    RPCNestedTests test3;
    if (QTest::qExec(&test3) != 0) {
        fInvalid = true;
    }
    CompatTests test4;
    if (QTest::qExec(&test4) != 0) {
        fInvalid = true;
    }
#ifdef ENABLE_WALLET
    WalletTests test5;
    if (QTest::qExec(&test5) != 0) {
        fInvalid = true;
    }
#endif

    fs::remove_all(pathTemp);

    return fInvalid;
}
