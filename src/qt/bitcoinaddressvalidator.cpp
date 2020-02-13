// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/bitcoinaddressvalidator.h>

#include <key_io.h>

/* Base58 characters are:
     "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz"

  This is:
  - All numbers except for '0'
  - All upper-case letters except for 'I' and 'O'
  - All lower-case letters except for 'l'
*/

BitcoinAddressEntryValidator::BitcoinAddressEntryValidator(QObject *parent) :
    QValidator(parent)
{
}

QValidator::State BitcoinAddressEntryValidator::validate(QString &input, int &pos) const
{
    // Empty address is "intermediate" input
    if (input.isEmpty())
        return QValidator::Intermediate;

    // Correction
    for (int idx = 0; idx < input.size();)
    {
        bool removeChar = false;
        QChar ch = input.at(idx);
        // Corrections made are very conservative on purpose, to avoid
        // users unexpectedly getting away with typos that would normally
        // be detected, and thus sending to the wrong address.
        switch(ch.unicode())
        {
        // Qt categorizes these as "Other_Format" not "Separator_Space"
        case 0x200B: // ZERO WIDTH SPACE
        case 0xFEFF: // ZERO WIDTH NO-BREAK SPACE
            removeChar = true;
            break;
        default:
            break;
        }

        // Remove whitespace
        if (ch.isSpace())
            removeChar = true;

        // To next character
        if (removeChar)
            input.remove(idx, 1);
        else
            ++idx;
    }

    // Validation
    for (int idx = 0; idx < input.size(); ++idx)
    {
        int ch = input.at(idx).unicode();

        if (((ch >= '0' && ch<='9') ||
            (ch >= 'a' && ch<='z') ||
            (ch >= 'A' && ch<='Z')) &&
            ch != 'I' && ch != 'O') // Characters invalid in both Base58 and Bech32
        {
            // Alphanumeric and not a 'forbidden' character
        }
        else
        {
            pos = idx;
            return QValidator::Invalid;
        }
    }

    return QValidator::Acceptable;
}

BitcoinAddressCheckValidator::BitcoinAddressCheckValidator(QObject *parent) :
    BitcoinAddressEntryValidator(parent)
{
}

QValidator::State BitcoinAddressCheckValidator::validate(QString &input, int &pos) const
{
    QValidator::State state = BitcoinAddressEntryValidator::validate(input, pos);
    if (state != QValidator::Acceptable) return state;

    // Validate the passed Bitcoin address
    if (IsValidDestinationString(input.toStdString())) {
        return QValidator::Acceptable;
    }

    std::vector<int> error_locations;
    std::string res = LocateErrorsInDestinationString(input.toStdString(), error_locations);
    if (!(res.empty() || error_locations.empty())) pos = error_locations.at(0);

    return QValidator::Invalid;
}
