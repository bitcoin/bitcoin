#include "bitcoinaddressvalidator.h"

/* Base58 characters are:
     "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz"

  This is:
  - All numbers except for '0'
  - All uppercase letters except for 'I' and 'O'
  - All lowercase letters except for 'l'

  User friendly Base58 input can map
  - 'l' and 'I' to '1'
  - '0' and 'O' to 'o'
*/

BitcoinAddressValidator::BitcoinAddressValidator(QObject *parent) :
    QValidator(parent)
{
}

QValidator::State BitcoinAddressValidator::validate(QString &input, int &pos) const
{
    // Correction
    for(int idx=0; idx<input.size();)
    {
        bool removeChar = false;
        QChar ch = input.at(idx);
        // Transform characters that are visually close
        switch(ch.unicode())
        {
        case 'l':
        case 'I':
            input[idx] = QChar('1');
            break;
        case '0':
        case 'O':
            input[idx] = QChar('o');
            break;
        // Qt categorizes these as "Other_Format" not "Separator_Space"
        case 0x200B: // ZERO WIDTH SPACE
        case 0xFEFF: // ZERO WIDTH NO-BREAK SPACE
            removeChar = true;
            break;
        default:
            break;
        }
        // Remove whitespace
        if(ch.isSpace())
            removeChar = true;
        // To next character
        if(removeChar)
            input.remove(idx, 1);
        else
            ++idx;
    }

    // Validation
    QValidator::State state = QValidator::Acceptable;
    for(int idx=0; idx<input.size(); ++idx)
    {
        int ch = input.at(idx).unicode();

        if(((ch >= '0' && ch<='9') ||
           (ch >= 'a' && ch<='z') ||
           (ch >= 'A' && ch<='Z')) &&
           ch != 'l' && ch != 'I' && ch != '0' && ch != 'O')
        {
            // Alphanumeric and not a 'forbidden' character
        }
        else
        {
            state = QValidator::Invalid;
        }
    }

    // Empty address is "intermediate" input
    if(input.isEmpty())
    {
        state = QValidator::Intermediate;
    }

    return state;
}
