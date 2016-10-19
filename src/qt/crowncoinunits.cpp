<<<<<<< HEAD:src/qt/bitcoinunits.cpp
// Copyright (c) 2011-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bitcoinunits.h"
#include "chainparams.h"
#include "primitives/transaction.h"
=======
// Copyright (c) 2011-2013 The Crowncoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "crowncoinunits.h"
>>>>>>> origin/dirty-merge-dash-0.11.0:src/qt/crowncoinunits.cpp

#include <QSettings>
#include <QStringList>

CrowncoinUnits::CrowncoinUnits(QObject *parent):
        QAbstractListModel(parent),
        unitlist(availableUnits())
{
}

QList<CrowncoinUnits::Unit> CrowncoinUnits::availableUnits()
{
<<<<<<< HEAD:src/qt/bitcoinunits.cpp
    QList<BitcoinUnits::Unit> unitlist;
    unitlist.append(CRW);
    unitlist.append(mCRW);
    unitlist.append(uCRW);
    unitlist.append(duffs);
=======
    QList<CrowncoinUnits::Unit> unitlist;
    unitlist.append(CRW);
    unitlist.append(mCRW);
    unitlist.append(uCRW);
>>>>>>> origin/dirty-merge-dash-0.11.0:src/qt/crowncoinunits.cpp
    return unitlist;
}

bool CrowncoinUnits::valid(int unit)
{
    switch(unit)
    {
<<<<<<< HEAD:src/qt/bitcoinunits.cpp
    case CRW:
    case mCRW:
    case uCRW:
    case duffs:
=======
    case CRW:
    case mCRW:
    case uCRW:
>>>>>>> origin/dirty-merge-dash-0.11.0:src/qt/crowncoinunits.cpp
        return true;
    default:
        return false;
    }
}

<<<<<<< HEAD:src/qt/bitcoinunits.cpp
QString BitcoinUnits::id(int unit)
{
    switch(unit)
    {
        case CRW: return QString("dash");
        case mCRW: return QString("mdash");
        case uCRW: return QString::fromUtf8("udash");
        case duffs: return QString("duffs");
        default: return QString("???");
    }
}

QString BitcoinUnits::name(int unit)
=======
QString CrowncoinUnits::name(int unit)
{
    switch(unit)
    {
    case CRW: return QString("CRW");
    case mCRW: return QString("mCRW");
    case uCRW: return QString::fromUtf8("μCRW");
    default: return QString("???");
    }
}

QString CrowncoinUnits::description(int unit)
>>>>>>> origin/dirty-merge-dash-0.11.0:src/qt/crowncoinunits.cpp
{
    if(Params().NetworkID() == CBaseChainParams::MAIN)
    {
<<<<<<< HEAD:src/qt/bitcoinunits.cpp
        switch(unit)
        {
            case CRW: return QString("CRW");
            case mCRW: return QString("mCRW");
            case uCRW: return QString::fromUtf8("μCRW");
            case duffs: return QString("duffs");
            default: return QString("???");
        }
    }
    else
    {
        switch(unit)
        {
            case CRW: return QString("tCRW");
            case mCRW: return QString("mtCRW");
            case uCRW: return QString::fromUtf8("μtCRW");
            case duffs: return QString("tduffs");
            default: return QString("???");
        }
    }
}

QString BitcoinUnits::description(int unit)
=======
    case CRW: return QString("Crowncoins");
    case mCRW: return QString("Milli-Crowncoins (1 / 1,000)");
    case uCRW: return QString("Micro-Crowncoins (1 / 1,000,000)");
    default: return QString("???");
    }
}

qint64 CrowncoinUnits::factor(int unit)
{
    switch(unit)
    {
    case CRW:  return 100000000;
    case mCRW: return 100000;
    case uCRW: return 100;
    default:   return 100000000;
    }
}

qint64 CrowncoinUnits::maxAmount(int unit)
>>>>>>> origin/dirty-merge-dash-0.11.0:src/qt/crowncoinunits.cpp
{
    if(Params().NetworkID() == CBaseChainParams::MAIN)
    {
        switch(unit)
        {
            case CRW: return QString("Crown");
            case mCRW: return QString("Milli-Crown (1 / 1" THIN_SP_UTF8 "000)");
            case uCRW: return QString("Micro-Crown (1 / 1" THIN_SP_UTF8 "000" THIN_SP_UTF8 "000)");
            case duffs: return QString("Ten Nano-Crown (1 / 100" THIN_SP_UTF8 "000" THIN_SP_UTF8 "000)");
            default: return QString("???");
        }
    }
    else
    {
<<<<<<< HEAD:src/qt/bitcoinunits.cpp
        switch(unit)
        {
            case CRW: return QString("TestCrowns");
            case mCRW: return QString("Milli-TestCrown (1 / 1" THIN_SP_UTF8 "000)");
            case uCRW: return QString("Micro-TestCrown (1 / 1" THIN_SP_UTF8 "000" THIN_SP_UTF8 "000)");
            case duffs: return QString("Ten Nano-TestCrown (1 / 100" THIN_SP_UTF8 "000" THIN_SP_UTF8 "000)");
            default: return QString("???");
        }
    }
}

qint64 BitcoinUnits::factor(int unit)
{
    switch(unit)
    {
    case CRW:  return 100000000;
    case mCRW: return 100000;
    case uCRW: return 100;
    case duffs: return 1;
    default:   return 100000000;
=======
    case CRW:  return Q_INT64_C(42000000);
    case mCRW: return Q_INT64_C(42000000000);
    case uCRW: return Q_INT64_C(42000000000000);
    default:   return 0;
    }
}

int CrowncoinUnits::amountDigits(int unit)
{
    switch(unit)
    {
    case CRW: return 8; // 42,000,000 (# digits, without commas)
    case mCRW: return 11; // 42,000,000,000
    case uCRW: return 14; // 42,000,000,000,000
    default: return 0;
>>>>>>> origin/dirty-merge-dash-0.11.0:src/qt/crowncoinunits.cpp
    }
}

int CrowncoinUnits::decimals(int unit)
{
    switch(unit)
    {
<<<<<<< HEAD:src/qt/bitcoinunits.cpp
    case CRW: return 8;
    case mCRW: return 5;
    case uCRW: return 2;
    case duffs: return 0;
=======
    case CRW: return 8;
    case mCRW: return 5;
    case uCRW: return 2;
>>>>>>> origin/dirty-merge-dash-0.11.0:src/qt/crowncoinunits.cpp
    default: return 0;
    }
}

<<<<<<< HEAD:src/qt/bitcoinunits.cpp
QString BitcoinUnits::format(int unit, const CAmount& nIn, bool fPlus, SeparatorStyle separators)
=======
QString CrowncoinUnits::format(int unit, qint64 n, bool fPlus)
>>>>>>> origin/dirty-merge-dash-0.11.0:src/qt/crowncoinunits.cpp
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
    qint64 remainder = n_abs % coin;
    QString quotient_str = QString::number(quotient);
    QString remainder_str = QString::number(remainder).rightJustified(num_decimals, '0');

    // Use SI-style thin space separators as these are locale independent and can't be
    // confused with the decimal marker.
    QChar thin_sp(THIN_SP_CP);
    int q_size = quotient_str.size();
    if (separators == separatorAlways || (separators == separatorStandard && q_size > 4))
        for (int i = 3; i < q_size; i += 3)
            quotient_str.insert(q_size - i, thin_sp);

    if (n < 0)
        quotient_str.insert(0, '-');
    else if (fPlus && n > 0)
        quotient_str.insert(0, '+');

    if (num_decimals <= 0)
        return quotient_str;

    return quotient_str + QString(".") + remainder_str;
}

<<<<<<< HEAD:src/qt/bitcoinunits.cpp

// TODO: Review all remaining calls to BitcoinUnits::formatWithUnit to
// TODO: determine whether the output is used in a plain text context
// TODO: or an HTML context (and replace with
// TODO: BtcoinUnits::formatHtmlWithUnit in the latter case). Hopefully
// TODO: there aren't instances where the result could be used in
// TODO: either context.

// NOTE: Using formatWithUnit in an HTML context risks wrapping
// quantities at the thousands separator. More subtly, it also results
// in a standard space rather than a thin space, due to a bug in Qt's
// XML whitespace canonicalisation
//
// Please take care to use formatHtmlWithUnit instead, when
// appropriate.

QString BitcoinUnits::formatWithUnit(int unit, const CAmount& amount, bool plussign, SeparatorStyle separators)
=======
QString CrowncoinUnits::formatWithUnit(int unit, qint64 amount, bool plussign)
>>>>>>> origin/dirty-merge-dash-0.11.0:src/qt/crowncoinunits.cpp
{
    return format(unit, amount, plussign, separators) + QString(" ") + name(unit);
}

QString BitcoinUnits::formatHtmlWithUnit(int unit, const CAmount& amount, bool plussign, SeparatorStyle separators)
{
    QString str(formatWithUnit(unit, amount, plussign, separators));
    str.replace(QChar(THIN_SP_CP), QString(THIN_SP_HTML));
    return QString("<span style='white-space: nowrap;'>%1</span>").arg(str);
}

QString BitcoinUnits::floorWithUnit(int unit, const CAmount& amount, bool plussign, SeparatorStyle separators)
{
    QSettings settings;
    int digits = settings.value("digits").toInt();

    QString result = format(unit, amount, plussign, separators);
    if(decimals(unit) > digits) result.chop(decimals(unit) - digits);

    return result + QString(" ") + name(unit);
}

<<<<<<< HEAD:src/qt/bitcoinunits.cpp
QString BitcoinUnits::floorHtmlWithUnit(int unit, const CAmount& amount, bool plussign, SeparatorStyle separators)
{
    QString str(floorWithUnit(unit, amount, plussign, separators));
    str.replace(QChar(THIN_SP_CP), QString(THIN_SP_HTML));
    return QString("<span style='white-space: nowrap;'>%1</span>").arg(str);
}

bool BitcoinUnits::parse(int unit, const QString &value, CAmount *val_out)
=======
bool CrowncoinUnits::parse(int unit, const QString &value, qint64 *val_out)
>>>>>>> origin/dirty-merge-dash-0.11.0:src/qt/crowncoinunits.cpp
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
    CAmount retvalue(str.toLongLong(&ok));
    if(val_out)
    {
        *val_out = retvalue;
    }
    return ok;
}

<<<<<<< HEAD:src/qt/bitcoinunits.cpp
QString BitcoinUnits::getAmountColumnTitle(int unit)
{
    QString amountTitle = QObject::tr("Amount");
    if (BitcoinUnits::valid(unit))
    {
        amountTitle += " ("+BitcoinUnits::name(unit) + ")";
    }
    return amountTitle;
}

int BitcoinUnits::rowCount(const QModelIndex &parent) const
=======
int CrowncoinUnits::rowCount(const QModelIndex &parent) const
>>>>>>> origin/dirty-merge-dash-0.11.0:src/qt/crowncoinunits.cpp
{
    Q_UNUSED(parent);
    return unitlist.size();
}

QVariant CrowncoinUnits::data(const QModelIndex &index, int role) const
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

CAmount BitcoinUnits::maxMoney()
{
    return MAX_MONEY;
}
