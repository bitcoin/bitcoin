// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <interfaces/node.h>
#include <qt/bitcoin.h>
#include <qt/test/apptests.h>
#include <qt/test/rpcnestedtests.h>
#include <qt/test/uritests.h>
#include <qt/test/compattests.h>
#include <test/util/setup_common.h>

#ifdef ENABLE_WALLET
#include <qt/test/addressbooktests.h>
#include <qt/test/wallettests.h>
#endif // ENABLE_WALLET

#include <QApplication>
#include <QObject>
#include <QTest>

#if defined(QT_STATICPLUGIN)
#include <QtPlugin>
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

const std::function<void(const std::string&)> G_TEST_LOG_FUN{};

// This is all you need to run all the tests
int main(int argc, char* argv[])
{
    // Initialize persistent globals with the testing setup state for sanity.
    // E.g. -datadir in gArgs is set to a temp directory dummy value (instead
    // of defaulting to the default datadir), or globalChainParams is set to
    // regtest params.
    //
    // All tests must use their own testing setup (if needed).
    {
        BasicTestingSetup dummy{CBaseChainParams::REGTEST};
    }

    NodeContext node_context;
    std::unique_ptr<interfaces::Node> node = interfaces::MakeNode(&node_context);

    bool fInvalid = false;

    // Prefer the "minimal" platform for the test instead of the normal default
    // platform ("xcb", "windows", or "cocoa") so tests can't unintentionally
    // interfere with any background GUIs and don't require extra resources.
    #if defined(WIN32)
        if (getenv("QT_QPA_PLATFORM") == nullptr) _putenv_s("QT_QPA_PLATFORM", "minimal");
    #else
        setenv("QT_QPA_PLATFORM", "minimal", /* overwrite */ 0);
    #endif

    // Don't remove this, it's needed to access
    // QApplication:: and QCoreApplication:: in the tests
    BitcoinApplication app(*node);
    app.setApplicationName("Bitcoin-Qt-test");

    node->setupServerArgs();            // Make gArgs available in the NodeContext
    node->context()->args->ClearArgs(); // Clear added args again
    AppTests app_tests(app);
    if (QTest::qExec(&app_tests) != 0) {
        fInvalid = true;
    }
    URITests test1;
    if (QTest::qExec(&test1) != 0) {
        fInvalid = true;
    }
    RPCNestedTests test3(*node);
    if (QTest::qExec(&test3) != 0) {
        fInvalid = true;
    }
    CompatTests test4;
    if (QTest::qExec(&test4) != 0) {
        fInvalid = true;
    }
#ifdef ENABLE_WALLET
    WalletTests test5(*node);
    if (QTest::qExec(&test5) != 0) {
        fInvalid = true;
    }
    AddressBookTests test6(*node);
    if (QTest::qExec(&test6) != 0) {
        fInvalid = true;
    }
#endif

    return fInvalid;
}
