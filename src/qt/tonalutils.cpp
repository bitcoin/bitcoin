// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/tonalutils.h>

#include <QFont>
#include <QFontMetrics>
#include <QRegExp>
#include <QRegExpValidator>
#include <QString>

bool TonalUtils::Supported()
{
    QFontMetrics fm = QFontMetrics(QFont());
    return fm.inFont(0xe9d9);
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
