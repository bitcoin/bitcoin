// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <init.h>
#include <qt/bitcoin.h>
#include <qt/guiutil.h>
#include <qt/guiutil_font.h>
#include <qt/test/optiontests.h>
#include <test/util/setup_common.h>
#include <util/system.h>

#include <QApplication>
#include <QFont>
#include <QLabel>
#include <QSettings>
#include <QTest>
#include <QWidget>

#include <univalue.h>

#include <fstream>

OptionTests::OptionTests(interfaces::Node& node) : m_node(node)
{
    gArgs.LockSettings([&](util::Settings& s) { m_previous_settings = s; });
}

void OptionTests::init()
{
    // reset args
    gArgs.LockSettings([&](util::Settings& s) { s = m_previous_settings; });
    gArgs.ClearPathCache();
}

void OptionTests::migrateSettings()
{
    // Set legacy QSettings and verify that they get cleared and migrated to
    // settings.json
    QSettings settings;
    settings.setValue("nDatabaseCache", 600);
    settings.setValue("nThreadsScriptVerif", 12);
    settings.setValue("fUseUPnP", false);
    settings.setValue("fListen", false);
    settings.setValue("bPrune", true);
    settings.setValue("nPruneSize", 3);
    settings.setValue("fUseProxy", true);
    settings.setValue("addrProxy", "proxy:123");
    settings.setValue("fUseSeparateProxyTor", true);
    settings.setValue("addrSeparateProxyTor", "onion:234");

    settings.sync();

    OptionsModel options{m_node};
    bilingual_str error;
    QVERIFY(options.Init(error));
    QVERIFY(!settings.contains("nDatabaseCache"));
    QVERIFY(!settings.contains("nThreadsScriptVerif"));
    QVERIFY(!settings.contains("fUseUPnP"));
    QVERIFY(!settings.contains("fListen"));
    QVERIFY(!settings.contains("bPrune"));
    QVERIFY(!settings.contains("nPruneSize"));
    QVERIFY(!settings.contains("fUseProxy"));
    QVERIFY(!settings.contains("addrProxy"));
    QVERIFY(!settings.contains("fUseSeparateProxyTor"));
    QVERIFY(!settings.contains("addrSeparateProxyTor"));

    std::ifstream file(gArgs.GetDataDirNet() / "settings.json");
    QCOMPARE(std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()).c_str(), "{\n"
        "    \"dbcache\": \"600\",\n"
        "    \"listen\": false,\n"
        "    \"onion\": \"onion:234\",\n"
        "    \"par\": \"12\",\n"
        "    \"proxy\": \"proxy:123\",\n"
        "    \"prune\": \"2861\"\n"
        "}\n");
}

void OptionTests::integerGetArgBug()
{
    // Test regression https://github.com/bitcoin/bitcoin/issues/24457. Ensure
    // that setting integer prune value doesn't cause an exception to be thrown
    // in the OptionsModel constructor
    gArgs.LockSettings([&](util::Settings& settings) {
        settings.forced_settings.erase("prune");
        settings.rw_settings["prune"] = 3814;
    });
    gArgs.WriteSettingsFile();
    bilingual_str error;
    QVERIFY(OptionsModel{m_node}.Init(error));
    gArgs.LockSettings([&](util::Settings& settings) {
        settings.rw_settings.erase("prune");
    });
    gArgs.WriteSettingsFile();
}

void OptionTests::parametersInteraction()
{
    // Test that the bug https://github.com/bitcoin-core/gui/issues/567 does not resurface.
    // It was fixed via https://github.com/bitcoin-core/gui/pull/568.
    // With fListen=false in ~/.config/Bitcoin/Bitcoin-Qt.conf and all else left as default,
    // bitcoin-qt should set both -listen and -listenonion to false and start successfully.
    gArgs.LockSettings([&](util::Settings& s) {
        s.forced_settings.erase("listen");
        s.forced_settings.erase("listenonion");
    });
    QVERIFY(!gArgs.IsArgSet("-listen"));
    QVERIFY(!gArgs.IsArgSet("-listenonion"));

    QSettings settings;
    settings.setValue("fListen", false);

    bilingual_str error;
    QVERIFY(OptionsModel{m_node}.Init(error));

    const bool expected{false};

    QVERIFY(gArgs.IsArgSet("-listen"));
    QCOMPARE(gArgs.GetBoolArg("-listen", !expected), expected);

    QVERIFY(gArgs.IsArgSet("-listenonion"));
    QCOMPARE(gArgs.GetBoolArg("-listenonion", !expected), expected);

    QVERIFY(AppInitParameterInteraction(gArgs));

    // cleanup
    settings.remove("fListen");
    QVERIFY(!settings.contains("fListen"));
    gArgs.ClearPathCache();
}

void OptionTests::extractFilter()
{
    QString filter = QString("Partially Signed Transaction (Binary) (*.psbt)");
    QCOMPARE(GUIUtil::ExtractFirstSuffixFromFilter(filter), "psbt");

    filter = QString("Image (*.png *.jpg)");
    QCOMPARE(GUIUtil::ExtractFirstSuffixFromFilter(filter), "png");
}

void OptionTests::effectivePointSize()
{
    using GUIUtil::internal::effectivePointSize;

    // A point-sized font reports its size directly, fractions included, whatever the DPI.
    QFont point_font;
    point_font.setPointSizeF(12.5);
    QCOMPARE(effectivePointSize(point_font, 96).value_or(0), 12.5);
    QCOMPARE(effectivePointSize(point_font, 72).value_or(0), 12.5);

    // A pixel-sized font carries no point size and must be converted using the target DPI.
    QFont pixel_font;
    pixel_font.setPixelSize(17);
    QVERIFY(pixel_font.pointSizeF() <= 0);
    QCOMPARE(effectivePointSize(pixel_font, 96).value_or(0), 17 * 72.0 / 96);
    QCOMPARE(effectivePointSize(pixel_font, 144).value_or(0), 8.5);
    // At 72 DPI points and pixels coincide; pinned so the identity is deliberate rather
    // than an accident of whichever DPI the host happens to report.
    QCOMPARE(effectivePointSize(pixel_font, 72).value_or(0), 17.0);

    // A non-positive DPI cannot yield a conversion factor, so the pixel size is unusable
    // even though it is valid. QWidget::logicalDpiY() is not guaranteed to be positive.
    QVERIFY(!effectivePointSize(pixel_font, 0).has_value());
    QVERIFY(!effectivePointSize(pixel_font, -1).has_value());

    // The remaining branch -- neither size usable -- is guarded but not asserted here: Qt
    // rejects non-positive sizes in the setters, and once a QGuiApplication exists (as it
    // does in this binary) every QFont is handed a valid default point size. The state is
    // still reachable in production, e.g. a font engine that populates no size at all, so
    // the helper compares against 0 rather than trusting any particular sentinel.
}

void OptionTests::updateFontsWithPixelSizedWidget()
{
    if (QApplication::platformName() == "minimal") {
        QSKIP("AppTests cannot initialize fonts with the 'minimal' platform plugin.");
    }

    // updateFonts() is a no-op until loadFonts() has run, and loadFonts() is process-global,
    // non-idempotent state owned by AppTests. Treat missing initialization as a failure on
    // supported platforms so the regression test cannot pass without exercising updateFonts().
    QVERIFY2(GUIUtil::fontsLoaded(),
             "GUIUtil::loadFonts() must succeed in AppTests::appTests() before OptionTests run.");

    QWidget host;
    QLabel* label{new QLabel(&host)};
    QFont pixel_font{label->font()};
    pixel_font.setPixelSize(17);
    label->setFont(pixel_font);
    QVERIFY(label->font().pointSizeF() <= 0);

    // The pre-fix code asserted pointSize() > 0 here and aborted the process. The widget must
    // now be swept normally and end up with a usable point size. The exact value depends on
    // the host DPI, so the arithmetic is pinned in effectivePointSize() above instead.
    GUIUtil::updateFonts();
    const double scaled_size{label->font().pointSizeF()};
    QVERIFY(scaled_size > 0);

    // The size is cached per widget on the first sweep, so repeated passes must not compound
    // it -- the defect that makes the cache load-bearing rather than an optimisation.
    GUIUtil::updateFonts();
    QCOMPARE(label->font().pointSizeF(), scaled_size);
}
