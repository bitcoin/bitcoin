// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/bitcoin.h>
#include <qt/test/optiontests.h>
#include <test/util/setup_common.h>
#include <util/system.h>

#include <QSettings>
#include <QTest>

#include <univalue.h>

//! Entry point for BitcoinApplication tests.
void OptionTests::optionTests()
{
    // Test regression https://github.com/bitcoin/bitcoin/issues/24457. Check
    // if setting an integer prune value causes an exception to be thrown in
    // the OptionsModel constructor.
    gArgs.LockSettings([&](util::Settings& settings) {
        settings.forced_settings.erase("prune");
        settings.rw_settings["prune"] = 3814;
    });
    gArgs.WriteSettingsFile();
    QVERIFY_EXCEPTION_THROWN(OptionsModel{}, std::runtime_error);
    gArgs.LockSettings([&](util::Settings& settings) {
        settings.rw_settings.erase("prune");
    });
    gArgs.WriteSettingsFile();
}
