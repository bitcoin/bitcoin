// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/bitcoinunits.h>
#include <qt/tonalutils.h>

#include <consensus/amount.h>

#include <QRegularExpression>
#include <QStringList>

#include <cassert>

static constexpr auto MAX_DIGITS_BTC = 16;
static constexpr auto MAX_DIGITS_TBC = 13;

BitcoinUnits::BitcoinUnits(QObject *parent):
        QAbstractListModel(parent),
        unitlist(availableUnits())
{
}

QList<BitcoinUnit> BitcoinUnits::availableUnits()
{
    QList<BitcoinUnit> unitlist;
    unitlist.append(Unit::BTC);
    unitlist.append(Unit::mBTC);
    unitlist.append(Unit::uBTC);
    unitlist.append(Unit::SAT);
    if (TonalUtils::Supported())
    {
        unitlist.append(Unit::TBC);
    }
    return unitlist;
}

QString BitcoinUnits::longName(Unit unit)
{
    switch (unit) {
    case Unit::BTC: return QString("BTC");
    case Unit::mBTC: return QString("mBTC");
    case Unit::uBTC: return QString::fromUtf8("µBTC (bits)");
    case Unit::SAT: return QString("Satoshi (sat)");
    case Unit::bTBC: return QString::fromUtf8("ᵇTBC");
    case Unit::sTBC: return QString::fromUtf8("ˢTBC");
    case Unit::TBC: return QString("TBC");
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

QString BitcoinUnits::shortName(Unit unit)
{
    switch (unit) {
    case Unit::BTC: return longName(unit);
    case Unit::mBTC: return longName(unit);
    case Unit::uBTC: return QString("bits");
    case Unit::SAT: return QString("sat");
    case Unit::bTBC: return QString::fromUtf8("ᵇTBC");
    case Unit::sTBC: return QString::fromUtf8("ˢTBC");
    case Unit::TBC: return QString("TBC");
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

QString BitcoinUnits::description(Unit unit)
{
    switch (unit) {
    case Unit::BTC: return QString("Bitcoins (decimal)");
    case Unit::mBTC: return QString("Milli-Bitcoins (1 / 1" THIN_SP_UTF8 "000)");
    case Unit::uBTC: return QString("Micro-Bitcoins (bits) (1 / 1" THIN_SP_UTF8 "000" THIN_SP_UTF8 "000)");
    case Unit::SAT: return QString("Satoshi (sat) (1 / 100" THIN_SP_UTF8 "000" THIN_SP_UTF8 "000)");
    case Unit::bTBC: return QString("Bong-Bitcoins (1,0000 tonal)");
    case Unit::sTBC: return QString("San-Bitcoins (100 tonal)");
    case Unit::TBC: return QString("Bitcoins (tonal)");
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

qint64 BitcoinUnits::factor(Unit unit)
{
    switch (unit) {
    case Unit::BTC: return 100'000'000;
    case Unit::mBTC: return 100'000;
    case Unit::uBTC: return 100;
    case Unit::SAT: return 1;
    case Unit::bTBC: return 0x100000000LL;
    case Unit::sTBC: return 0x1000000;
    case Unit::TBC:  return 0x10000;
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

int BitcoinUnits::decimals(Unit unit)
{
    switch (unit) {
    case Unit::BTC: return 8;
    case Unit::mBTC: return 5;
    case Unit::uBTC: return 2;
    case Unit::SAT: return 0;
    case Unit::bTBC: return 8;
    case Unit::sTBC: return 6;
    case Unit::TBC: return 4;
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

int BitcoinUnits::radix(Unit unit)
{
    switch (unit) {
    case Unit::bTBC:
    case Unit::sTBC:
    case Unit::TBC:
        return 0x10;
    default:
        return 10;
    }
}

BitcoinUnit BitcoinUnits::numsys(Unit unit)
{
    switch (unit) {
    case Unit::bTBC:
    case Unit::sTBC:
    case Unit::TBC:
        return Unit::TBC;
    default:
        return Unit::BTC;
    }
}

qint64 BitcoinUnits::max_digits(Unit unit)
{
    switch (numsys(unit)) {
    case Unit::TBC:
        return MAX_DIGITS_TBC;
    default:
        return MAX_DIGITS_BTC;
    }
}

qint64 BitcoinUnits::singlestep(Unit unit)
{
    switch (numsys(unit)) {
    case Unit::TBC:
        return 0x10000;
    default:
        return 100000;
    }
}

QString BitcoinUnits::format(Unit unit, const CAmount& nIn, bool fPlus, SeparatorStyle separators, bool justify)
{
    // Note: not using straight sprintf here because we do NOT want
    // localized number formatting.
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

    switch (numsys(unit)) {
    case Unit::BTC:
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
    case Unit::TBC:
    {
        // Right-trim excess zeros after the decimal point
        static const QRegularExpression tail_zeros("0+$");
        remainder_str.remove(tail_zeros);
        TonalUtils::ConvertFromHex(quotient_str);
        TonalUtils::ConvertFromHex(remainder_str);
        break;
    }
    default: assert(false);
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

QString BitcoinUnits::formatWithUnit(Unit unit, const CAmount& amount, bool plussign, SeparatorStyle separators)
{
    return format(unit, amount, plussign, separators) + QString(" ") + shortName(unit);
}

QString BitcoinUnits::formatHtmlWithUnit(Unit unit, const CAmount& amount, bool plussign, SeparatorStyle separators)
{
    QString str(formatWithUnit(unit, amount, plussign, separators));
    str.replace(QChar(THIN_SP_CP), QString(THIN_SP_HTML));
    return QString("<span style='white-space: nowrap;'>%1</span>").arg(str);
}

QString BitcoinUnits::formatWithPrivacy(Unit unit, const CAmount& amount, SeparatorStyle separators, bool privacy)
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

bool BitcoinUnits::parse(Unit unit, const QString& value, CAmount* val_out)
{
    if (value.isEmpty()) {
        return false; // Refuse to parse invalid unit or empty string
    }
    int num_decimals = decimals(unit);

    // Ignore spaces and thin spaces when parsing
    QStringList parts = removeSpaces(value).split(".");

    if(parts.size() > 2)
    {
        return false; // More than one dot
    }
    const QString& whole = parts[0];
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

    Unit unumsys = numsys(unit);
    if (unumsys == Unit::TBC) {
        if (str.size() > 15)
            return false; // Longer numbers may exceed 63 bits
        TonalUtils::ConvertToHex(str);
    } else
    if(str.size() > 18)
    {
        return false; // Longer numbers will exceed 63 bits
    }

    CAmount retvalue(str.toLongLong(&ok, radix(unit)));
    if(val_out)
    {
        *val_out = retvalue;
    }
    return ok;
}

QString BitcoinUnits::getAmountColumnTitle(Unit unit)
{
    return QObject::tr("Amount") + " (" + shortName(unit) + ")";
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
            return QVariant::fromValue(unit);
        }
    }
    return QVariant();
}

CAmount BitcoinUnits::maxMoney()
{
    return MAX_MONEY;
}

std::variant<qint8, QString> BitcoinUnits::ToSetting(BitcoinUnit unit)
{
    switch (unit) {
    case BitcoinUnit::BTC:  return qint8{0};
    case BitcoinUnit::mBTC: return qint8{1};
    case BitcoinUnit::uBTC: return qint8{2};
    case BitcoinUnit::SAT:  return qint8{3};
    case BitcoinUnit::bTBC: return QString("bTBC");
    case BitcoinUnit::sTBC: return QString("sTBC");
    case BitcoinUnit::TBC:  return QString("TBC");
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

namespace {
BitcoinUnit FromQint8(qint8 num)
{
    switch (num) {
    case 0: return BitcoinUnit::BTC;
    case 1: return BitcoinUnit::mBTC;
    case 2: return BitcoinUnit::uBTC;
    case 3: return BitcoinUnit::SAT;
    }
    return BitcoinUnit::BTC;
}
} // namespace

BitcoinUnit BitcoinUnits::FromSetting(const QString& s, BitcoinUnit def)
{
    if (s == "0") return BitcoinUnit::BTC;
    if (s == "1") return BitcoinUnit::mBTC;
    if (s == "2") return BitcoinUnit::uBTC;
    if (s == "3") return BitcoinUnit::SAT;
    if (s == "4") return BitcoinUnit::sTBC;
    if (s == "5") return BitcoinUnit::TBC;
    if (s == "bTBC") return BitcoinUnit::bTBC;
    if (s == "sTBC") return BitcoinUnit::sTBC;
    if (s == "TBC")  return BitcoinUnit::TBC;
    return def;
}

QDataStream& operator<<(QDataStream& out, const BitcoinUnit& unit)
{
    auto setting_val = BitcoinUnits::ToSetting(unit);
    if (const QString* setting_str = std::get_if<QString>(&setting_val)) {
        return out << qint8{0} << *setting_str;
    } else {
        return out << std::get<qint8>(setting_val);
    }
}

QDataStream& operator>>(QDataStream& in, BitcoinUnit& unit)
{
    qint8 input;
    in >> input;
    unit = FromQint8(input);
    if (!in.atEnd()) {
        QString setting_str;
        in >> setting_str;
        unit = BitcoinUnits::FromSetting(setting_str, unit);
    }
    return in;
}
