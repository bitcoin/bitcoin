// Copyright (c) 2011-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/bitcoinunits.h>
#include <qt/tonalutils.h>

#include <QStringList>

#include <cassert>

static constexpr auto MAX_DIGITS_BTC = 16;
static constexpr auto MAX_DIGITS_TBC = 13;

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
    unitlist.append(SAT);
    if (TonalUtils::Supported())
    {
        unitlist.append(bTBC);
        unitlist.append(sTBC);
        unitlist.append(TBC);
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
    case SAT:
    case bTBC:
    case sTBC:
    case TBC:
        return true;
    default:
        return false;
    }
}

QString BitcoinUnits::toSetting(int unit)
{
    switch(unit)
    {
    case BTC: return QString("0");
    case mBTC: return QString("1");
    case uBTC: return QString::fromUtf8("2");
    case SAT: return QString("3");
    case bTBC: return QString::fromUtf8("bTBC");
    case sTBC: return QString::fromUtf8("sTBC");
    case TBC: return QString("TBC");
    default: return QString("???");
    }
}

int BitcoinUnits::fromSetting(const QString& s)
{
    bool is_num;
    uint n = s.toUInt(&is_num);
    if (is_num) {
        if (n < 4) {
            // Decimal units
            return n;
        } else {
            // Tonal units (except bTBC, which conflicted with SAT)
            return (n - 3) + bTBC;
        }
    }
    if (s == "bTBC") return bTBC;
    if (s == "sTBC") return sTBC;
    if (s == "TBC")  return TBC;
    return BTC;
}

QString BitcoinUnits::longName(int unit)
{
    switch(unit)
    {
    case BTC: return QString("BTC");
    case mBTC: return QString("mBTC");
    case uBTC: return QString::fromUtf8("µBTC (bits)");
    case SAT: return QString("Satoshi (sat)");
    case bTBC: return QString::fromUtf8("ᵇTBC");
    case sTBC: return QString::fromUtf8("ˢTBC");
    case TBC: return QString("TBC");
    default: return QString("???");
    }
}

QString BitcoinUnits::shortName(int unit)
{
    switch(unit)
    {
    case uBTC: return QString::fromUtf8("bits");
    case SAT: return QString("sat");
    default: return longName(unit);
    }
}

QString BitcoinUnits::description(int unit)
{
    switch(unit)
    {
    case BTC: return QString("Bitcoins (decimal)");
    case mBTC: return QString("Milli-Bitcoins (1 / 1" THIN_SP_UTF8 "000)");
    case uBTC: return QString("Micro-Bitcoins (bits) (1 / 1" THIN_SP_UTF8 "000" THIN_SP_UTF8 "000)");
    case SAT: return QString("Satoshi (sat) (1 / 100" THIN_SP_UTF8 "000" THIN_SP_UTF8 "000)");
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
    case BTC: return 100000000;
    case mBTC: return 100000;
    case uBTC: return 100;
    case SAT: return 1;
    case bTBC: return 0x100000000LL;
    case sTBC: return 0x1000000;
    case TBC:  return 0x10000;
    default: return 100000000;
    }
}

int BitcoinUnits::decimals(int unit)
{
    switch(unit)
    {
    case BTC: return 8;
    case mBTC: return 5;
    case uBTC: return 2;
    case SAT: return 0;
    case bTBC: return 8;
    case sTBC: return 6;
    case TBC: return 4;
    default: return 0;
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

qint64 BitcoinUnits::max_digits(int unit)
{
    switch(numsys(unit))
    {
    case TBC:
        return MAX_DIGITS_TBC;
    default:
        return MAX_DIGITS_BTC;
    }
}

qint64 BitcoinUnits::singlestep(int unit)
{
    switch(numsys(unit))
    {
    case TBC:
        return 0x10000;
    default:
        return 100000;
    }
}

QString BitcoinUnits::format(int unit, const CAmount& nIn, bool fPlus, SeparatorStyle separators, bool justify)
{
    // Note: not using straight sprintf here because we do NOT want
    // localized number formatting.
    if(!valid(unit))
        return QString(); // Refuse to format invalid unit
    qint64 n = (qint64)nIn;
    qint64 coin = factor(unit);
    int num_decimals = decimals(unit);
    qint64 n_abs = (n > 0 ? n : -n);
    qint64 quotient = n_abs / coin;
    int uradix = radix(unit);
    QString quotient_str = QString::number(quotient, uradix);
    if (justify) {
        quotient_str = quotient_str.rightJustified(max_digits(unit) - num_decimals, ' ');
    }

    QString remainder_str;
    if (num_decimals > 0) {
        const qint64 remainder = n_abs % coin;
        remainder_str = QString::number(remainder, uradix).rightJustified(num_decimals, '0');
    }

    switch (numsys(unit))
    {
    case BTC:
    {

    // Use SI-style thin space separators as these are locale independent and can't be
    // confused with the decimal marker.
    QChar thin_sp(THIN_SP_CP);
    int q_size = quotient_str.size();
    if (separators == SeparatorStyle::ALWAYS || (separators == SeparatorStyle::STANDARD && q_size > 4))
        for (int i = 3; i < q_size; i += 3)
            quotient_str.insert(q_size - i, thin_sp);

    break;
    }
    case TBC:
    {
        // Right-trim excess zeros after the decimal point
        static const QRegExp tail_zeros("0+$");
        remainder_str.remove(tail_zeros);
        TonalUtils::ConvertFromHex(quotient_str);
        TonalUtils::ConvertFromHex(remainder_str);
        break;
    }
    }

    if (n < 0)
        quotient_str.insert(0, '-');
    else if (fPlus && n > 0)
        quotient_str.insert(0, '+');

    if (!remainder_str.isEmpty())
        quotient_str += QString(".") + remainder_str;
    return quotient_str;
}


// NOTE: Using formatWithUnit in an HTML context risks wrapping
// quantities at the thousands separator. More subtly, it also results
// in a standard space rather than a thin space, due to a bug in Qt's
// XML whitespace canonicalisation
//
// Please take care to use formatHtmlWithUnit instead, when
// appropriate.

QString BitcoinUnits::formatWithUnit(int unit, const CAmount& amount, bool plussign, SeparatorStyle separators)
{
    return format(unit, amount, plussign, separators) + QString(" ") + shortName(unit);
}

QString BitcoinUnits::formatHtmlWithUnit(int unit, const CAmount& amount, bool plussign, SeparatorStyle separators)
{
    QString str(formatWithUnit(unit, amount, plussign, separators));
    str.replace(QChar(THIN_SP_CP), QString(THIN_SP_HTML));
    return QString("<span style='white-space: nowrap;'>%1</span>").arg(str);
}

QString BitcoinUnits::formatWithPrivacy(int unit, const CAmount& amount, SeparatorStyle separators, bool privacy)
{
    assert(amount >= 0);
    QString value;
    if (privacy) {
        value = format(unit, 0, false, separators, true).replace('0', '#');
    } else {
        value = format(unit, amount, false, separators, true);
    }
    return value + QString(" ") + shortName(unit);
}

bool BitcoinUnits::parse(int unit, const QString &value, CAmount *val_out)
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

    CAmount retvalue(str.toLongLong(&ok, radix(unit)));
    if(val_out)
    {
        *val_out = retvalue;
    }
    return ok;
}

QString BitcoinUnits::getAmountColumnTitle(int unit)
{
    QString amountTitle = QObject::tr("Amount");
    if (BitcoinUnits::valid(unit))
    {
        amountTitle += " ("+BitcoinUnits::shortName(unit) + ")";
    }
    return amountTitle;
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
            return QVariant(longName(unit));
        case Qt::ToolTipRole:
            return QVariant(description(unit));
        case UnitRole:
            return QVariant(static_cast<int>(unit));
        }
    }
    return QVariant();
}

CAmount BitcoinUnits::maxMoney()
{
    return MAX_MONEY;
}
