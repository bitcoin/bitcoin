// Copyright (c) 2015-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_QT_PLATFORMSTYLE_H
#define SYSCOIN_QT_PLATFORMSTYLE_H

#include <QIcon>
#include <QPixmap>
#include <QString>

/* Coin network-specific GUI style information */
class PlatformStyle
{
public:
    /** Get style associated with provided platform name, or 0 if not known */
    static const PlatformStyle *instantiate(const QString &platformId);

    const QString &getName() const { return name; }

    bool getImagesOnButtons() const { return imagesOnButtons; }
    bool getUseExtraSpacing() const { return useExtraSpacing; }

    QColor TextColor() const;
    QColor SingleColor() const;

    /** Colorize an image (given filename) with the icon color */
    QImage SingleColorImage(const QString& filename) const;

    /** Colorize an icon (given filename) with the icon color */
    QIcon SingleColorIcon(const QString& filename) const;

    /** Colorize an icon (given object) with the icon color */
    QIcon SingleColorIcon(const QIcon& icon) const;

    /** Colorize an icon (given object) with the text color */
    QIcon TextColorIcon(const QIcon& icon) const;

private:
    PlatformStyle(const QString &name, bool imagesOnButtons, bool colorizeIcons, bool useExtraSpacing);

    QString name;
    bool imagesOnButtons;
    bool colorizeIcons;
    bool useExtraSpacing;
};

#endif // SYSCOIN_QT_PLATFORMSTYLE_H

