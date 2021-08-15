// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_BITCOINADDRESSTYPES_H
#define BITCOIN_QT_BITCOINADDRESSTYPES_H

#include <amount.h>

#include <QAbstractListModel>
#include <QString>

// U+2009 THIN SPACE = UTF-8 E2 80 89
#define REAL_THIN_SP_CP 0x2009
#define REAL_THIN_SP_UTF8 "\xE2\x80\x89"
#define REAL_THIN_SP_HTML "&thinsp;"

// U+200A HAIR SPACE = UTF-8 E2 80 8A
#define HAIR_SP_CP 0x200A
#define HAIR_SP_UTF8 "\xE2\x80\x8A"
#define HAIR_SP_HTML "&#8202;"

// U+2006 SIX-PER-EM SPACE = UTF-8 E2 80 86
#define SIXPEREM_SP_CP 0x2006
#define SIXPEREM_SP_UTF8 "\xE2\x80\x86"
#define SIXPEREM_SP_HTML "&#8198;"

// U+2007 FIGURE SPACE = UTF-8 E2 80 87
#define FIGURE_SP_CP 0x2007
#define FIGURE_SP_UTF8 "\xE2\x80\x87"
#define FIGURE_SP_HTML "&#8199;"

// QMessageBox seems to have a bug whereby it doesn't display thin/hair spaces
// correctly.  Workaround is to display a space in a small font.  If you
// change this, please test that it doesn't cause the parent span to start
// wrapping.
#define HTML_HACK_SP "<span style='white-space: nowrap; font-size: 6pt'> </span>"

// Define THIN_SP_* variables to be our preferred type of thin space
#define THIN_SP_CP   REAL_THIN_SP_CP
#define THIN_SP_UTF8 REAL_THIN_SP_UTF8
#define THIN_SP_HTML HTML_HACK_SP

/** Bitcoin adress type definitions. Encapsulates parsing
   and serves as list model for drop-down selection boxes.
*/
class BitcoinAddressTypes: public QAbstractListModel
{
    Q_OBJECT

public:
    explicit BitcoinAddressTypes(QObject *parent);

    enum AddressType
    {
        LEGACY,
        NESTED_SEGWIT,
        NATIVE_SEGWIT
    };

    //! @name Static API
    //! Address type conversion
    ///@{

    //! Get list of address types, for drop-down box
    static QList<AddressType> availableAddressTypes();
    //! Is address type ID valid?
    static bool valid(int addressType);
    //! Long name
    static QString longName(int addressType);
    //! Short name
    static QString shortName(int addressType);
    //! Longer description
    static QString description(int addressType);
    ///@}

    //! @name AbstractListModel implementation
    //! List model for address type drop-down selection box.
    ///@{
    enum RoleIndex {
        /** Address type identifier */
        AddressTypeRole = Qt::UserRole
    };
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    ///@}
private:
    QList<BitcoinAddressTypes::AddressType> addressTypeList;
};
typedef BitcoinAddressTypes::AddressType BitcoinAddressType;

#endif // BITCOIN_QT_BITCOINADDRESSTYPES_H
