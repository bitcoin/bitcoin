// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(HAVE_CONFIG_H)
#include <config/syscoin-config.h>
#endif

#include <interfaces/init.h>
#include <interfaces/node.h>
#include <qt/syscoin.h>
#include <qt/test/apptests.h>
#include <qt/test/optiontests.h>
#include <qt/test/rpcnestedtests.h>
#include <qt/test/uritests.h>
#include <test/util/setup_common.h>

#ifdef ENABLE_WALLET
#include <qt/test/addressbooktests.h>
#include <qt/test/wallettests.h>
#endif // ENABLE_WALLET

#include <QApplication>
#include <QDebug>
#include <QObject>
#include <QTest>

#include <functional>

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
#elif defined(QT_QPA_PLATFORM_ANDROID)
Q_IMPORT_PLUGIN(QAndroidPlatformIntegrationPlugin)
#endif
#endif

const std::function<void(const std::string&)> G_TEST_LOG_FUN{};

const std::function<std::vector<const char*>()> G_TEST_COMMAND_LINE_ARGUMENTS{};

// This is all you need to run all the tests
int main(int argc, char* argv[])
{
    // Initialize persistent globals with the testing setup state for sanity.
    // E.g. -datadir in gArgs is set to a temp directory dummy value (instead
    // of defaulting to the default datadir), or globalChainParams is set to
    // regtest params.
    //
    // All tests must use their own testing setup (if needed).
    fs::create_directories([] {
        BasicTestingSetup dummy{CBaseChainParams::REGTEST};
        return gArgs.GetDataDirNet() / "blocks";
    }());

    std::unique_ptr<interfaces::Init> init = interfaces::MakeGuiInit(argc, argv);
    gArgs.ForceSetArg("-listen", "0");
    gArgs.ForceSetArg("-listenonion", "0");
    gArgs.ForceSetArg("-discover", "0");
    gArgs.ForceSetArg("-dnsseed", "0");
    gArgs.ForceSetArg("-fixedseeds", "0");
    gArgs.ForceSetArg("-upnp", "0");
    gArgs.ForceSetArg("-natpmp", "0");

    // Prefer the "minimal" platform for the test instead of the normal default
    // platform ("xcb", "windows", or "cocoa") so tests can't unintentionally
    // interfere with any background GUIs and don't require extra resources.
    #if defined(WIN32)
        if (getenv("QT_QPA_PLATFORM") == nullptr) _putenv_s("QT_QPA_PLATFORM", "minimal");
    #else
        setenv("QT_QPA_PLATFORM", "minimal", 0 /* overwrite */);
    #endif

    SyscoinApplication app;
    app.setApplicationName("Syscoin-Qt-test");
    app.createNode(*init);

    int num_test_failures{0};

    AppTests app_tests(app);
    num_test_failures += QTest::qExec(&app_tests);

    OptionTests options_tests(app.node());
    num_test_failures += QTest::qExec(&options_tests);

    URITests test1;
    num_test_failures += QTest::qExec(&test1);

    RPCNestedTests test3(app.node());
    num_test_failures += QTest::qExec(&test3);

#ifdef ENABLE_WALLET
    WalletTests test5(app.node());
    num_test_failures += QTest::qExec(&test5);

    AddressBookTests test6(app.node());
    num_test_failures += QTest::qExec(&test6);
#endif

    if (num_test_failures) {
        qWarning("\nFailed tests: %d\n", num_test_failures);
    } else {
        qDebug("\nAll tests passed.\n");
    }
    return num_test_failures;
}
