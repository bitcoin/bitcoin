#include "bitcoinunitstests.h"

#include "bitcoinunits.h"

#include <QUrl>

void BitcoinUnitsTests::parseTests()
{
    qint64 value = 0;

    /// Tests with en_US locale
    QLocale locale1("en_US");
    QVERIFY(BitcoinUnits::parse(BitcoinUnits::BTC, "0", &value, locale1));
    QCOMPARE(value, 0LL);
    QVERIFY(BitcoinUnits::parse(BitcoinUnits::BTC, "1", &value, locale1));
    QCOMPARE(value, 100000000LL);
    QVERIFY(BitcoinUnits::parse(BitcoinUnits::BTC, "1.0", &value, locale1));
    QCOMPARE(value, 100000000LL);
    QVERIFY(BitcoinUnits::parse(BitcoinUnits::uBTC, "1,000,000.0", &value, locale1));
    QCOMPARE(value, 100000000LL);
    QVERIFY(BitcoinUnits::parse(BitcoinUnits::uBTC, "1,000.0", &value, locale1));
    QCOMPARE(value, 100000LL);
    QVERIFY(BitcoinUnits::parse(BitcoinUnits::mBTC, "1,000.0", &value, locale1));
    QCOMPARE(value, 100000000LL);
    QVERIFY(BitcoinUnits::parse(BitcoinUnits::BTC, "0.00000001", &value, locale1));
    QCOMPARE(value, 1LL);
    QVERIFY(BitcoinUnits::parse(BitcoinUnits::mBTC, "0.00001", &value, locale1));
    QCOMPARE(value, 1LL);
    QVERIFY(BitcoinUnits::parse(BitcoinUnits::uBTC, "0.01", &value, locale1));
    QCOMPARE(value, 1LL);
    // Fail: group separator in wrong place
    QVERIFY(!BitcoinUnits::parse(BitcoinUnits::BTC, "0,00", &value, locale1));
    // Fail: group separator in decimals
    QVERIFY(!BitcoinUnits::parse(BitcoinUnits::BTC, "0.0,000", &value, locale1));
    // Fail: multiple decimal separators
    QVERIFY(!BitcoinUnits::parse(BitcoinUnits::BTC, "0.000.000", &value, locale1));

    /// Tests with nl_NL locale
    QLocale locale2("nl_NL");
    QVERIFY(BitcoinUnits::parse(BitcoinUnits::BTC, "1", &value, locale2));
    QCOMPARE(value, 100000000LL);
    QVERIFY(BitcoinUnits::parse(BitcoinUnits::BTC, "1,0", &value, locale2));
    QCOMPARE(value, 100000000LL);
    QVERIFY(BitcoinUnits::parse(BitcoinUnits::uBTC, "1.000.000,0", &value, locale2));
    QCOMPARE(value, 100000000LL);
    // Fail: multiple decimal separators
    QVERIFY(!BitcoinUnits::parse(BitcoinUnits::BTC, "0,000,000", &value, locale2));

    /// Tests with de_CH locale
    QLocale locale3("de_CH");
    QVERIFY(BitcoinUnits::parse(BitcoinUnits::uBTC, "123'456.78", &value, locale3));
    QCOMPARE(value, 12345678LL);
    // Fail: multiple decimal separators
    QVERIFY(!BitcoinUnits::parse(BitcoinUnits::BTC, "0.000.000", &value, locale3));

    /// Tests with c locale
    QLocale locale4(QLocale::c());
    locale4.setNumberOptions(QLocale::OmitGroupSeparator | QLocale::RejectGroupSeparator);
    QVERIFY(BitcoinUnits::parse(BitcoinUnits::BTC, "1000.00000000", &value, locale4));
    QCOMPARE(value, 100000000000LL);
    // Fail: group separator
    QVERIFY(!BitcoinUnits::parse(BitcoinUnits::BTC, "1,000.00", &value, locale4));
    // Fail: too many decimals
    QVERIFY(!BitcoinUnits::parse(BitcoinUnits::BTC, "1000.000000000", &value, locale4));
    // Fail: overflow
    QVERIFY(!BitcoinUnits::parse(BitcoinUnits::BTC, "21000001.0", &value, locale4));
    QVERIFY(!BitcoinUnits::parse(BitcoinUnits::BTC, "92233720368547758090.0", &value, locale4));
    // Fail: underflow
    QVERIFY(!BitcoinUnits::parse(BitcoinUnits::BTC, "-1000000.0", &value, locale4));
    // Fail: sign in decimals
    QVERIFY(!BitcoinUnits::parse(BitcoinUnits::BTC, "0.-1000000", &value, locale4));
}

void BitcoinUnitsTests::formatTests()
{
    /// Tests with en_US locale
    QLocale locale1("en_US");
    QCOMPARE(BitcoinUnits::format(BitcoinUnits::BTC, 0, false, false, locale1), QString("0.00000000"));
    QCOMPARE(BitcoinUnits::format(BitcoinUnits::BTC, 0, false, true, locale1), QString("0.00"));
    QCOMPARE(BitcoinUnits::format(BitcoinUnits::mBTC, 0, false, false, locale1), QString("0.00000"));
    QCOMPARE(BitcoinUnits::format(BitcoinUnits::uBTC, 0, false, false, locale1), QString("0.00"));
    QCOMPARE(BitcoinUnits::format(BitcoinUnits::uBTC, 0, true, false, locale1), QString("+0.00"));
    QCOMPARE(BitcoinUnits::format(BitcoinUnits::uBTC, 100000000, false, true, locale1), QString("1,000,000.00"));
    QCOMPARE(BitcoinUnits::format(BitcoinUnits::uBTC, 100000000, true, true, locale1), QString("+1,000,000.00"));

    QCOMPARE(BitcoinUnits::formatWithUnit(BitcoinUnits::BTC, 100000000, false, true, locale1), QString("1.00 BTC"));
    QCOMPARE(BitcoinUnits::formatWithUnit(BitcoinUnits::mBTC, 100000000, false, true, locale1), QString("1,000.00 mBTC"));
    QCOMPARE(BitcoinUnits::formatWithUnit(BitcoinUnits::uBTC, 100000000, false, true, locale1), QString("1,000,000.00 μBTC"));

    /// Tests with nl_NL locale
    QLocale locale2("nl_NL");
    QCOMPARE(BitcoinUnits::format(BitcoinUnits::uBTC, 100000000, false, true, locale2), QString("1.000.000,00"));

    /// Tests with de_CH locale
    QLocale locale3("de_CH");
    QCOMPARE(BitcoinUnits::format(BitcoinUnits::uBTC, 100000000, false, true, locale3), QString("1'000'000.00"));

    /// Tests with c locale (with and without group separators)
    QLocale locale4(QLocale::c());
    locale4.setNumberOptions(QLocale::OmitGroupSeparator | QLocale::RejectGroupSeparator);
    QCOMPARE(BitcoinUnits::format(BitcoinUnits::uBTC, 100000000, false, true, QLocale::c()), QString("1,000,000.00"));
    QCOMPARE(BitcoinUnits::format(BitcoinUnits::uBTC, 100000000, false, true, locale4), QString("1000000.00"));
}

