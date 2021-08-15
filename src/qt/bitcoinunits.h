// Copyright (c) 2011-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_BITCOINUNITS_H
#define BITCOIN_QT_BITCOINUNITS_H

#include <amount.h>

#include <QAbstractListModel>
#include <QString>

// U+2009 THIN SPACE = UTF-8 E2 80 89
#define REAL_THIN_SP_CP 0x2009
#define REAL_THIN_SP_UTF8 "\xE2\x80\x89"

// QMessageBox seems to have a bug whereby it doesn't display thin/hair spaces
// correctly.  Workaround is to display a space in a small font.  If you
// change this, please test that it doesn't cause the parent span to start
// wrapping.
#define HTML_HACK_SP "<span style='white-space: nowrap; font-size: 6pt'> </span>"

// Define THIN_SP_* variables to be our preferred type of thin space
#define THIN_SP_CP   REAL_THIN_SP_CP
#define THIN_SP_UTF8 REAL_THIN_SP_UTF8
#define THIN_SP_HTML HTML_HACK_SP

/** Bitcoin unit definitions. Encapsulates parsing and formatting
   and serves as list model for drop-down selection boxes.
*/
class BitcoinUnits: public QAbstractListModel
{
    Q_OBJECT

public:
    explicit BitcoinUnits(QObject *parent);

    /** Bitcoin units.
      @note Source: https://en.bitcoin.it/wiki/Units . Please add only sensible ones
     */
    enum Unit
    {
        BTC,
        mBTC,
        uBTC,
        SAT
    };

    enum class SeparatorStyle
    {
        NEVER,
        STANDARD,
        ALWAYS
    };

    //! @name Static API
    //! Unit conversion and formatting
    ///@{

    //! Get list of units, for drop-down box
    static QList<Unit> availableUnits();
    //! Is unit ID valid?
    static bool valid(int unit);
    //! Long name
    static QString longName(int unit);
    //! Short name
    static QString shortName(int unit);
    //! Longer description
    static QString description(int unit);
    //! Number of Satoshis (1e-8) per unit
    static qint64 factor(int unit);
    //! Number of decimals left
    static int decimals(int unit);
    //! Format as string
    static QString format(int unit, const CAmount& amount, bool plussign = false, SeparatorStyle separators = SeparatorStyle::STANDARD, bool justify = false);
    //! Format as string (with unit)
    static QString formatWithUnit(int unit, const CAmount& amount, bool plussign=false, SeparatorStyle separators=SeparatorStyle::STANDARD);
    //! Format as HTML string (with unit)
    static QString formatHtmlWithUnit(int unit, const CAmount& amount, bool plussign=false, SeparatorStyle separators=SeparatorStyle::STANDARD);
    //! Format as string (with unit) of fixed length to preserve privacy, if it is set.
    static QString formatWithPrivacy(int unit, const CAmount& amount, SeparatorStyle separators, bool privacy);
    //! Parse string to coin amount
    static bool parse(int unit, const QString &value, CAmount *val_out);
    //! Gets title for amount column including current display unit if optionsModel reference available */
    static QString getAmountColumnTitle(int unit);
    //! Gets title for delegated column including current display unit if optionsModel reference available */
    static QString getDelegatedColumnTitle(int unit);
    //! Format integer number as string (with and without separator)
    static QString formatInt(const int64_t& number, bool plussign=false, SeparatorStyle separators=SeparatorStyle::STANDARD);
    ///@}

    //! @name AbstractListModel implementation
    //! List model for unit drop-down selection box.
    ///@{
    enum RoleIndex {
        /** Unit identifier */
        UnitRole = Qt::UserRole
    };
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    ///@}

    static QString removeSpaces(QString text)
    {
        text.remove(' ');
        text.remove(QChar(THIN_SP_CP));
        return text;
    }

    //! Return maximum number of base units (Satoshis)
    static CAmount maxMoney();

private:
    QList<BitcoinUnits::Unit> unitlist;
};
typedef BitcoinUnits::Unit BitcoinUnit;

#endif // BITCOIN_QT_BITCOINUNITS_H
