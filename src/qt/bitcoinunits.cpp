// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bitcoinunits.h"

#include <QStringList>
#include <QLocale>

BitcoinUnits::BitcoinUnits(QObject *parent):
        QAbstractListModel(parent),
        unitlist(availableUnits())
{
}

QList<BitcoinUnits::Unit> BitcoinUnits::availableUnits()
{
    QList<BitcoinUnits::Unit> unitlist;
    unitlist.append(BTC);
    unitlist.append(mBTC);
    unitlist.append(uBTC);
    return unitlist;
}

bool BitcoinUnits::valid(int unit)
{
    switch(unit)
    {
    case BTC:
    case mBTC:
    case uBTC:
        return true;
    default:
        return false;
    }
}

QString BitcoinUnits::name(int unit)
{
    switch(unit)
    {
    case BTC: return QString("BTC");
    case mBTC: return QString("mBTC");
    case uBTC: return QChar(0x03BC) + QString("BTC");
    default: return QString("???");
    }
}

QString BitcoinUnits::description(int unit)
{
    switch(unit)
    {
    case BTC: return tr("Bitcoins");
    case mBTC: return tr("Milli-Bitcoins (1 / 1,000)");
    case uBTC: return tr("Micro-Bitcoins (1 / 1,000,000)");
    default: return QString("???");
    }
}

qint64 BitcoinUnits::factor(int unit)
{
    switch(unit)
    {
    case BTC:  return 100000000;
    case mBTC: return 100000;
    case uBTC: return 100;
    default:   return 100000000;
    }
}

qint64 BitcoinUnits::maxAmount(int unit)
{
    switch(unit)
    {
    case BTC:  return Q_INT64_C(21000000);
    case mBTC: return Q_INT64_C(21000000000);
    case uBTC: return Q_INT64_C(21000000000000);
    default:   return 0;
    }
}

int BitcoinUnits::amountDigits(int unit)
{
    switch(unit)
    {
    case BTC: return 8; // 21,000,000 (# digits, without commas)
    case mBTC: return 11; // 21,000,000,000
    case uBTC: return 14; // 21,000,000,000,000
    default: return 0;
    }
}

int BitcoinUnits::decimals(int unit)
{
    switch(unit)
    {
    case BTC: return 8;
    case mBTC: return 5;
    case uBTC: return 2;
    default: return 0;
    }
}

QString BitcoinUnits::format(int unit, qint64 n, bool fPlus, bool fTrim, const QLocale &locale_in)
{
    // Note: not using straight sprintf here because we do NOT want
    // localized number formatting.
    if(!valid(unit))
        return QString(); // Refuse to format invalid unit
    QLocale locale(locale_in);
    qint64 coin = factor(unit);
    int num_decimals = decimals(unit);

    qint64 n_abs = (n > 0 ? n : -n);
    qint64 quotient = n_abs / coin;
    qint64 remainder = n_abs % coin;
    // Quotient has group (decimal) separators if locale has this enabled
    QString quotient_str = locale.toString(quotient);
    // Remainder does not have group separators
    locale.setNumberOptions(QLocale::OmitGroupSeparator | QLocale::RejectGroupSeparator);
    QString remainder_str = locale.toString(remainder).rightJustified(num_decimals, '0');

    if(fTrim)
    {
        // Right-trim excess zeros after the decimal point
        int nTrim = 0;
        for (int i = remainder_str.size()-1; i>=2 && (remainder_str.at(i) == '0'); --i)
            ++nTrim;
        remainder_str.chop(nTrim);
    }

    if (n < 0)
        quotient_str.insert(0, '-');
    else if (fPlus && n >= 0)
        quotient_str.insert(0, '+');
    return quotient_str + locale.decimalPoint() + remainder_str;
}

QString BitcoinUnits::formatWithUnit(int unit, qint64 amount, bool plussign, bool trim, const QLocale &locale)
{
    return format(unit, amount, plussign, trim) + QString(" ") + name(unit);
}
bool BitcoinUnits::parse(int unit, const QString &value, qint64 *val_out, const QLocale &locale_in)
{
    if(!valid(unit) || value.isEmpty())
        return false; // Refuse to parse invalid unit or empty string

    QLocale locale(locale_in);
    qint64 coin = factor(unit);
    int num_decimals = decimals(unit);
    QStringList parts = value.split(locale.decimalPoint());
    bool ok = false;

    if(parts.size() > 2)
        return false; // More than one decimal point

    // Parse whole part (may include locale-specific group separators)
#if QT_VERSION < 0x050000
    qint64 whole = locale.toLongLong(parts[0], &ok, 10);
#else
    qint64 whole = locale.toLongLong(parts[0], &ok);
#endif
    if(!ok)
        return false; // Parse error
    if(whole > maxAmount(unit) || whole < 0)
        return false; // Overflow or underflow

    // Parse decimals part (if present, may not include group separators)
    qint64 decimals = 0;
    if(parts.size() > 1)
    {
        if(parts[1].size() > num_decimals)
            return false; // Exceeds max precision
        locale.setNumberOptions(QLocale::OmitGroupSeparator | QLocale::RejectGroupSeparator);
#if QT_VERSION < 0x050000
        decimals = locale.toLongLong(parts[1].leftJustified(num_decimals, '0'), &ok, 10);
#else
        decimals = locale.toLongLong(parts[1].leftJustified(num_decimals, '0'), &ok);
#endif
        if(!ok || decimals < 0)
            return false; // Parse error
    }

    if(val_out)
    {
        *val_out = whole * coin + decimals;
    }
    return ok;
}

int BitcoinUnits::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return unitlist.size();
}

QVariant BitcoinUnits::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if(row >= 0 && row < unitlist.size())
    {
        Unit unit = unitlist.at(row);
        switch(role)
        {
        case Qt::EditRole:
        case Qt::DisplayRole:
            return QVariant(name(unit));
        case Qt::ToolTipRole:
            return QVariant(description(unit));
        case UnitRole:
            return QVariant(static_cast<int>(unit));
        }
    }
    return QVariant();
}
