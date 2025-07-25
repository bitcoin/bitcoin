// Copyright (c) 2014-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_NETWORKSTYLE_H
#define BITCOIN_QT_NETWORKSTYLE_H

#include <util/chaintype.h>

#include <QIcon>
#include <QPixmap>
#include <QString>

/* Coin network-specific GUI style information */
class NetworkStyle
{
public:
    /** Get style associated with provided network id, or 0 if not known */
    static const NetworkStyle* instantiate(ChainType networkId);

    const QString &getAppName() const { return appName; }
    const QIcon &getAppIcon() const { return appIcon; }
    const QIcon &getTrayAndWindowIcon() const { return trayAndWindowIcon; }
    const QString &getTitleAddText() const { return titleAddText; }

private:
    NetworkStyle(const QString& appName, int iconColorHueShift, int iconColorSaturationReduction, const char* titleAddText);

    QString appName;
    QIcon appIcon;
    QIcon trayAndWindowIcon;
    QString titleAddText;
};

#endif // BITCOIN_QT_NETWORKSTYLE_H
