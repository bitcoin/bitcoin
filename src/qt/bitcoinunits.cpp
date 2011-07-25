#include "bitcoinunits.h"

#include <QStringList>

QString BitcoinUnits::name(BitcoinUnits::Unit unit)
{
    switch(unit)
    {
    case BTC: return "BTC";
    case mBTC: return "mBTC";
    case uBTC: return "uBTC";
    default: return "???";
    }
}

QString BitcoinUnits::description(BitcoinUnits::Unit unit)
{
    switch(unit)
    {
    case BTC: return "Bitcoin";
    case mBTC: return "Milli-bitcoin";
    case uBTC: return "Micro-bitcoin";
    default: return "???";
    }
}

qint64 BitcoinUnits::factor(BitcoinUnits::Unit unit)
{
    switch(unit)
    {
    case BTC:  return 100000000;
    case mBTC: return 100000;
    case uBTC: return 100;
    default:   return 100000000;
    }
}

int BitcoinUnits::decimals(BitcoinUnits::Unit unit)
{
    switch(unit)
    {
    case BTC: return 8;
    case mBTC: return 5;
    case uBTC: return 2;
    default: return 0;
    }
}

QString BitcoinUnits::format(BitcoinUnits::Unit unit, qint64 n, bool fPlus)
{
    // Note: not using straight sprintf here because we do NOT want
    // localized number formatting.
    qint64 coin = factor(unit);
    int num_decimals = decimals(unit);
    qint64 n_abs = (n > 0 ? n : -n);
    qint64 quotient = n_abs / coin;
    qint64 remainder = n_abs % coin;
    QString quotient_str = QString::number(quotient);
    QString remainder_str = QString::number(remainder).rightJustified(num_decimals, '0');

    // Right-trim excess 0's after the decimal point
    int nTrim = 0;
    for (int i = remainder_str.size()-1; i>=2 && (remainder_str.at(i) == '0'); --i)
        ++nTrim;
    remainder_str.chop(nTrim);

    if (n < 0)
        quotient_str.insert(0, '-');
    else if (fPlus && n > 0)
        quotient_str.insert(0, '+');
    return quotient_str + QString(".") + remainder_str;
}

QString BitcoinUnits::formatWithUnit(BitcoinUnits::Unit unit, qint64 amount, bool plussign)
{
    return format(unit, amount, plussign) + QString(" ") + name(unit);
}

bool BitcoinUnits::parse(BitcoinUnits::Unit unit, const QString &value, qint64 *val_out)
{
    int num_decimals = decimals(unit);
    QStringList parts = value.split(".");
    if(parts.size() != 2 || parts.at(1).size() > num_decimals)
        return false; // Max num decimals
    bool ok = false;
    QString str = parts[0] + parts[1].leftJustified(num_decimals, '0');
    if(str.size()>18)
        return false; // Bounds check

    qint64 retvalue = str.toLongLong(&ok);
    if(val_out)
    {
        *val_out = retvalue;
    }
    return ok;
}
