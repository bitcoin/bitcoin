// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/tonalutils.h>

#include <QFont>
#include <QFontMetrics>
#include <QRegExp>
#include <QRegExpValidator>
#include <QString>

static const QList<QChar> tonal_digits{0xe9df, 0xe9de, 0xe9dd, 0xe9dc, 0xe9db, 0xe9da, 0xe9d9, '8', '7', '6', '5', '4', '3', '2', '1', '0'};

namespace {

bool font_supports_tonal(const QFont& font)
{
    const QFontMetrics fm(font);
    QString s = "000";
    const QSize sz = fm.size(0, s);
    for (const auto& c : tonal_digits) {
        if (!fm.inFont(c)) return false;
        if (sz != fm.size(0, s)) return false;
    }
    return true;
}

} // anon namespace

bool TonalUtils::Supported()
{
    QFont default_font;
    if (font_supports_tonal(default_font)) return true;
    QFont last_resort_font(default_font.lastResortFamily());
    if (font_supports_tonal(last_resort_font)) return true;
    return false;
}

static QRegExpValidator tv(QRegExp("-?(?:[\\d\\xe9d9-\\xe9df]+\\.?|[\\d\\xe9d9-\\xe9df]*\\.[\\d\\xe9d9-\\xe9df]*)"), NULL);

QValidator::State TonalUtils::validate(QString&input, int&pos)
{
    return tv.validate(input, pos);
}

void TonalUtils::ConvertFromHex(QString&str)
{
    for (int i = 0; i < str.size(); ++i)
    {
        ushort c = str[i].unicode();
        if (c == '9')
            str[i] = 0xe9d9;
        else
        if (c >= 'A' && c <= 'F')
            str[i] = c + 0xe999;
        else
        if (c >= 'a' && c <= 'f')
            str[i] = c + 0xe979;
    }
}

void TonalUtils::ConvertToHex(QString&str)
{
    for (int i = 0; i < str.size(); ++i)
    {
        ushort c = str[i].unicode();
        if (c == 0xe9d9)
            str[i] = '9';
        else
        if (c == '9')
            str[i] = 'a';
        else
        if (c >= 0xe9da && c <= 0xe9df)
            str[i] = c - 0xe999;
    }
}
