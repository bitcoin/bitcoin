// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/tonalutils.h>

#include <QFont>
#include <QFontMetrics>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QString>

static const QList<QChar> tonal_digits{QChar(0xe8ef), QChar(0xe8ee), QChar(0xe8ed), QChar(0xe8ec), QChar(0xe8eb), QChar(0xe8ea), QChar(0xe8e9), '8', '7', '6', '5', '4', '3', '2', '1', '0'};

bool TonalUtils::font_supports_tonal(const QFont& font)
{
    const QFontMetrics fm(font);
    QString s = "000";
    const QSize sz = fm.size(0, s);
    for (const auto& c : tonal_digits) {
        if (!fm.inFont(c)) return false;
        s[0] = s[1] = s[2] = c;
        if (sz != fm.size(0, s)) return false;
    }
    return true;
}

#define RE_TONAL_DIGIT "[\\d\\xe8e0-\\xe8ef\\xe9d0-\\xe9df]"
static QRegularExpressionValidator tv(QRegularExpression("-?(?:" RE_TONAL_DIGIT "+\\.?|" RE_TONAL_DIGIT "*\\." RE_TONAL_DIGIT "+)"), nullptr);

QValidator::State TonalUtils::validate(QString&input, int&pos)
{
    return tv.validate(input, pos);
}

void TonalUtils::ConvertFromHex(QString&str)
{
    for (int i = 0; i < str.size(); ++i) {
        ushort c = str[i].unicode();
        if (c == '9') {
            str[i] = QChar(0xe8e9);
        } else if (c >= 'A' && c <= 'F') {
            str[i] = QChar(c + (0xe8ea - 'A'));
        } else if (c >= 'a' && c <= 'f') {
            str[i] = QChar(c + (0xe8ea - 'a'));
        }
    }
}

void TonalUtils::ConvertToHex(QString&str)
{
    for (int i = 0; i < str.size(); ++i) {
        ushort c = str[i].unicode();
        if (c == '9') {
            str[i] = 'a';
        } else if (c >= 0xe8e0 && c <= 0xe8e9) {  // UCSUR 0-9
            str[i] = QChar(c - (0xe8e0 - '0'));
        } else if (c >= 0xe8ea && c <= 0xe8ef) {  // UCSUR a-f
            str[i] = QChar(c - (0xe8ea - 'a'));
        } else if (c >= 0xe9d0 && c <= 0xe9d9) {
            str[i] = QChar(c - (0xe9d0 - '0'));
        } else if (c >= 0xe9da && c <= 0xe9df) {
            str[i] = QChar(c - 0xe999);
        }
    }
}
