#ifndef BITCOINUNITS_H
#define BITCOINUNITS_H

#include <QString>

// Bitcoin unit definitions
class BitcoinUnits
{
public:
    enum Unit
    {
        BTC,
        mBTC,
        uBTC
    };

    // Short name
    static QString name(Unit unit);
    // Longer description
    static QString description(Unit unit);
    // Number of satoshis / unit
    static qint64 factor(Unit unit);
    // Number of decimals left
    static int decimals(Unit unit);
    // Format as string
    static QString format(Unit unit, qint64 amount, bool plussign=false);
    // Format as string (with unit)
    static QString formatWithUnit(Unit unit, qint64 amount, bool plussign=false);
    // Parse string to coin amount
    static bool parse(Unit unit, const QString &value, qint64 *val_out);

};

#endif // BITCOINUNITS_H
