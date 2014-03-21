#ifndef BITCOINUNITSTESTS_H
#define BITCOINUNITSTESTS_H

#include <QObject>
#include <QTest>

class BitcoinUnitsTests : public QObject
{
    Q_OBJECT

private slots:
    void formatTests();
    void parseTests();
};

#endif // BITCOINUNITSTESTS_H
