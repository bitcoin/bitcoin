// Copyright (c) 2014-2019 The Bitcoin Core developers
// Copyright (c) 2014-2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_NETWORKSTYLE_H
#define BITCOIN_QT_NETWORKSTYLE_H

#include <QIcon>
#include <QPixmap>
#include <QString>

/* Coin network-specific GUI style information */
class NetworkStyle
{
public:
    /** Get style associated with provided network id, or 0 if not known */
    static const NetworkStyle* instantiate(const std::string& networkId);

    const QString &getAppName() const { return appName; }
    const QIcon &getAppIcon() const { return appIcon; }
    const QPixmap &getSplashImage() const { return splashImage; }
    const QIcon &getTrayAndWindowIcon() const { return trayAndWindowIcon; }
    const QString &getTitleAddText() const { return titleAddText; }
    const QColor &getBadgeColor() const { return badgeColor; }

private:
    NetworkStyle(const QString &appName, const int iconColorHueShift, const int iconColorSaturationReduction, const char *titleAddText);

    QString appName;
    QIcon appIcon;
    QPixmap splashImage;
    QIcon trayAndWindowIcon;
    QString titleAddText;
    QColor badgeColor;

    void rotateColor(QColor& col, const int iconColorHueShift, const int iconColorSaturationReduction);
    void rotateColors(QImage& img, const int iconColorHueShift, const int iconColorSaturationReduction);
};

#endif // BITCOIN_QT_NETWORKSTYLE_H
