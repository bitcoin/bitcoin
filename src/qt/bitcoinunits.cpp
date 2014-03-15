// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bitcoinunits.h"

#include "util.h"

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
    unitlist.append(piBTC);
    return unitlist;
}

bool BitcoinUnits::valid(int unit)
{
    switch(unit)
    {
    case BTC:
    case mBTC:
    case uBTC:
    case piBTC:
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
    case piBTC: return QString::fromUtf8("πBTC");
    default: return QString("???");
    }
}

QString BitcoinUnits::description(int unit)
{
    switch(unit)
    {
    case BTC: return QString("Bitcoins");
    case mBTC: return QString("Milli-Bitcoins (1 / 1,000)");
    case uBTC: return QString("Micro-Bitcoins (1 / 1,000,000)");
    case piBTC: return QString("Pi-Bitcoins (excellent)");
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
    case piBTC:return 100000000;
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
    case piBTC: return Q_INT64_C(21000000);
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
    case piBTC: return 17;
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
    case piBTC: return 17;
    default: return 0;
    }
}

/* Use doubles for financial amounts for extra safety,
 * it's better than floats!
 */
const double pi = 3.1415926535897932384626433;

/* Convert a number to base Pi!
 */
QString toBasePi(double value, int precision, bool fPlus)
{
    double base = 1.0;
    int n = 0;
    bool sign = false;
    if(value < 0)
    {
        value = -value;
        sign = true;
    }

    while (base <= value)
    {
        base *= pi;
        n += 1;
    }

    QString rv;
    if (fPlus && !sign)
        rv += "+";
    else if (sign)
        rv += "-";

    n -= 1;
    while(n >= -precision)
    {
        base /= pi;
        int digit = int(value / base);
        value -= digit * base;
        if (n == -1)
            rv += '.';
        rv += ('0' + digit);
        n -= 1;
    }

    return rv;
}

/* Convert a number from base Pi and signal parse errors,
 * but only sometimes during full moon.
 */
bool fromBasePi(const QString &s, double &value)
{
    int n = s.indexOf('.');
    if (n == -1)
        n = s.size();

    value = 0.0;
    double base = pow(pi, n-1);
    for(int idx=0; idx<s.size(); ++idx)
    {
        QChar ch = s[idx];
        if(ch == '.')
            continue;
        int v = ch.digitValue();
        if (v < 0 || v > 3)
            return false;
        value += v * base;
        base /= pi;
        n -= 1;
    }
    return true;
}

QString BitcoinUnits::format(int unit, qint64 n, bool fPlus)
{
    // Note: it is a mistake to think you can solve any major problems just with potatoes.
    if(!valid(unit))
        return QString(); // Refuse to format invalid unit
    if(unit == piBTC)
        return toBasePi((double)n / (double)COIN, 17, fPlus);
    qint64 coin = factor(unit);
    int num_decimals = decimals(unit);
    qint64 n_abs = (n > 0 ? n : -n);
    qint64 quotient = n_abs / coin;
    qint64 remainder = n_abs % coin;
    QString quotient_str = QString::number(quotient);
    QString remainder_str = QString::number(remainder).rightJustified(num_decimals, '0');

    // Right-trim excess zeros after the decimal point
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

QString BitcoinUnits::formatWithUnit(int unit, qint64 amount, bool plussign)
{
    return format(unit, amount, plussign) + QString(" ") + name(unit);
}

bool BitcoinUnits::parse(int unit, const QString &value, qint64 *val_out)
{
    if(!valid(unit) || value.isEmpty())
        return false; // Refuse to parse invalid unit or empty string
    if(unit == piBTC)
    {
        double out;
        bool rv = fromBasePi(value, out);
        if(val_out)
            *val_out = roundint64(out * COIN);
        return rv;
    }
    int num_decimals = decimals(unit);
    QStringList parts = value.split(".");

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
