#include "bitcoinunits.h"

#include <QStringList>

#include "tonalutils.h"

BitcoinUnits::BitcoinUnits(QObject *parent):
        QAbstractListModel(parent),
        unitlist(availableUnits())
{
}

QList<BitcoinUnits::Unit> BitcoinUnits::availableUnits()
{
    static QList<BitcoinUnits::Unit> unitlist;
    if (unitlist.empty())
    {
        unitlist.append(BTC);
        unitlist.append(mBTC);
        unitlist.append(uBTC);
        if (TonalUtils::Supported())
        {
            unitlist.append(bTBC);
            unitlist.append(sTBC);
            unitlist.append(TBC);
        }
    }
    return unitlist;
}

bool BitcoinUnits::valid(int unit)
{
    switch(unit)
    {
    case BTC:
    case mBTC:
    case uBTC:
    case bTBC:
    case sTBC:
    case TBC:
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
    case bTBC: return QString::fromUtf8("ᵇTBC");
    case sTBC: return QString::fromUtf8("ˢTBC");
    case TBC: return QString("TBC");
    default: return QString("???");
    }
}

QString BitcoinUnits::description(int unit)
{
    switch(unit)
    {
    case BTC: return QString("Bitcoins (decimal)");
    case mBTC: return QString("Milli-Bitcoins (1 / 1,000)");
    case uBTC: return QString("Micro-Bitcoins (1 / 1,000,000)");
    case bTBC: return QString("Bong-Bitcoins (1,0000 tonal)");
    case sTBC: return QString("San-Bitcoins (100 tonal)");
    case TBC: return QString("Bitcoins (tonal)");
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
    case bTBC: return 0x100000000;
    case sTBC: return 0x1000000;
    case TBC:  return 0x10000;
    default:   return 100000000;
    }
}

int BitcoinUnits::amountDigits(int unit)
{
    switch(unit)
    {
    case BTC: return 8; // 21,000,000 (# digits, without commas)
    case mBTC: return 11; // 21,000,000,000
    case uBTC: return 14; // 21,000,000,000,000
    case bTBC: return 6; // 49,63
    case sTBC: return 8; // 49,6384
    case TBC: return 10; // 49,63,8448
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
    case bTBC: return 8;
    case sTBC: return 6;
    case TBC: return 4;
    default: return 0;
    }
}

int BitcoinUnits::minPlaces(int unit)
{
    switch(unit)
    {
    case bTBC:
    case sTBC:
    case TBC:
        return -1;
    default:
        return 2;
    }
}

int BitcoinUnits::radix(int unit)
{
    switch(unit)
    {
    case bTBC:
    case sTBC:
    case TBC:
        return 0x10;
    default:
        return 10;
    }
}

int BitcoinUnits::numsys(int unit)
{
    switch(unit)
    {
    case bTBC:
    case sTBC:
    case TBC:
        return TBC;
    default:
        return BTC;
    }
}

QString BitcoinUnits::format(int unit, qint64 n, bool fPlus)
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
    int uradix = radix(unit);
    QString s = QString::number(quotient, uradix) + "." + QString::number(remainder, uradix).rightJustified(num_decimals, '0');

    // Right-trim excess 0's after the decimal point
    int nTrim = 0;
    int nTrimStop = s.size() - (decimals(unit) - minPlaces(unit));
    for (int i = s.size()-1; i>=nTrimStop && (s.at(i) == '0'); --i)
        ++nTrim;
    if (s.at(nTrimStop) == '.' && nTrimStop + nTrim + 1 == s.size())
        ++nTrim;
    s.chop(nTrim);

    int unumsys = numsys(unit);
    if (unumsys == TBC)
        TonalUtils::ConvertFromHex(s);

    if (n < 0)
        s.insert(0, '-');
    else if (fPlus && n > 0)
        s.insert(0, '+');
    return s;
}

QString BitcoinUnits::formatWithUnit(int unit, qint64 amount, bool plussign)
{
    return format(unit, amount, plussign) + QString(" ") + name(unit);
}

bool BitcoinUnits::parse(int unit, const QString &value, qint64 *val_out)
{
    if(!valid(unit) || value.isEmpty())
        return false; // Refuse to parse invalid unit or empty string
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

    int unumsys = numsys(unit);
    if (unumsys == TBC)
    {
        if (str.size() > 15)
            return false; // Longer numbers may exceed 63 bits
        TonalUtils::ConvertToHex(str);
    }
    else
    {
    if(str.size() > 18)
    {
        return false; // Longer numbers will exceed 63 bits
    }
    }

    qint64 retvalue = str.toLongLong(&ok, radix(unit));
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
