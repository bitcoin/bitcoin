// Copyright (c) 2014-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_NETWORKSTYLE_H
#define BITCOIN_QT_NETWORKSTYLE_H

#include <QIcon>
#include <QString>

#include <string>

/* Coin network-specific GUI style information */
class NetworkStyle
{
public:
    /** Get style associated with provided network id, or 0 if not known */
    static const NetworkStyle* instantiate(const std::string& networkId);

    const QString& appName() const { return m_app_name; }
    const QIcon& icon() const { return m_icon; }
    const QString& titleAddText() const { return m_title_add_text; }

private:
    NetworkStyle(const QString& app_name, const QString& icon_file, const char* title_add_text);

    const QString m_app_name;
    const QIcon m_icon;
    const QString m_title_add_text;
};

#endif // BITCOIN_QT_NETWORKSTYLE_H
