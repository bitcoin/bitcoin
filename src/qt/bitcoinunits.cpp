// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bitcoinunits.h"

#include <QStringList>

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
    case uBTC: return QString::fromUtf8("μBTC");
    default: return QString("???");
    }
}

QString BitcoinUnits::description(int unit)
{
    switch(unit)
    {
    case BTC: return QString("Bitcoins");
    case mBTC: return QString("Milli-Bitcoins (1 / 1" THIN_SP_UTF8 "000)");
    case uBTC: return QString("Micro-Bitcoins (1 / 1" THIN_SP_UTF8 "000" THIN_SP_UTF8 "000)");
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

QString BitcoinUnits::format(int unit, qint64 n, bool fPlus, SeparatorStyle separators, bool fAlign)
{
    // Note: not using straight sprintf here because we do NOT want
    // localized number formatting.
    if(!valid(unit))
        return QString(); // Refuse to format invalid unit
    qint64 coin = factor(unit);
    int num_decimals = decimals(unit);
    qint64 n_abs = (n > 0 ? n : -n);
    qint64 quotient = n_abs / coin;
    qint64 remainder = n_abs % coin;
    QString quotient_str = QString::number(quotient);
    QString remainder_str = QString::number(remainder).rightJustified(num_decimals, '0');

    // Use SI-stule separators as these are locale indendent and can't be
    // confused with the decimal marker.  Rule is to use a thin space every
    // three digits on *both* sides of the decimal point - but only if there
    // are five or more digits
    QChar thin_sp(THIN_SP_CP);
    int q_size = quotient_str.size();
    if (separators == separatorAlways || (separators == separatorStandard && q_size > 4))
        for (int i = 3; i < q_size; i += 3)
            quotient_str.insert(q_size - i, thin_sp);

    int r_size = remainder_str.size();
    if (separators == separatorAlways || (separators == separatorStandard && r_size > 4))
        for (int i = 3, adj = 0; i < r_size ; i += 3, adj++)
            remainder_str.insert(i + adj, thin_sp);

    if (n < 0)
        quotient_str.insert(0, '-');
    else if (fPlus && n > 0)
        quotient_str.insert(0, '+');
    return quotient_str + QString(".") + remainder_str;
}

QString BitcoinUnits::formatWithUnit(int unit, qint64 amount, bool plussign, SeparatorStyle separators, bool fAlign)
{
    return format(unit, amount, plussign, separators, fAlign) + QString(" ") + name(unit);
}

QString BitcoinUnits::formatHtmlWithUnit(int unit, qint64 amount, bool plussign, SeparatorStyle separators)
{
    QString str(formatWithUnit(unit, amount, plussign, separators));
    str.replace(QChar(THIN_SP_CP), QString(THIN_SP_HTML));
    return QString("<span style='white-space: nowrap;'>%1</span>").arg(str);
}


bool BitcoinUnits::parse(int unit, const QString &value, qint64 *val_out)
{
    if(!valid(unit) || value.isEmpty())
        return false; // Refuse to parse invalid unit or empty string
    int num_decimals = decimals(unit);

    // Ignore spaces and thin spaces when parsing
    QStringList parts = removeSpaces(value).split(".");

    if(parts.size() > 2)
    {
        return false; // More than one dot
    }
    QString whole = parts[0];
    QString decimals;

    if(parts.size() > 1)
    {
        decimals = parts[1];
    }
    if(decimals.size() > num_decimals)
    {
        return false; // Exceeds max precision
    }
    bool ok = false;
    QString str = whole + decimals.leftJustified(num_decimals, '0');

    if(str.size() > 18)
    {
        return false; // Longer numbers will exceed 63 bits
    }
    qint64 retvalue = str.toLongLong(&ok);
    if(val_out)
    {
        *val_out = retvalue;
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
