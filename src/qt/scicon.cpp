// Copyright (c) 2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "scicon.h"

#include <QApplication>
#include <QColor>
#include <QIcon>
#include <QImage>
#include <QPalette>
#include <QPixmap>

static void MakeSingleColorImage(QImage& img, const QColor& colorbase)
{
    img = img.convertToFormat(QImage::Format_ARGB32);
    for (int x = img.width(); x--; )
    {
        for (int y = img.height(); y--; )
        {
            const QRgb rgb = img.pixel(x, y);
            img.setPixel(x, y, qRgba(colorbase.red(), colorbase.green(), colorbase.blue(), qAlpha(rgb)));
        }
    }
}

QImage SingleColorImage(const QString& filename, const QColor& colorbase)
{
    QImage img(filename);
    MakeSingleColorImage(img, colorbase);
    return img;
}

QIcon SingleColorIcon(const QIcon& ico, const QColor& colorbase)
{
    QIcon new_ico;
    QSize sz;
    Q_FOREACH(sz, ico.availableSizes())
    {
        QImage img(ico.pixmap(sz).toImage());
        MakeSingleColorImage(img, colorbase);
        new_ico.addPixmap(QPixmap::fromImage(img));
    }
    return new_ico;
}

QIcon SingleColorIcon(const QString& filename, const QColor& colorbase)
{
    return QIcon(QPixmap::fromImage(SingleColorImage(filename, colorbase)));
}

QColor SingleColor()
{
    const QColor colorHighlightBg(QApplication::palette().color(QPalette::Highlight));
    const QColor colorHighlightFg(QApplication::palette().color(QPalette::HighlightedText));
    const QColor colorText(QApplication::palette().color(QPalette::WindowText));
    const int colorTextLightness = colorText.lightness();
    QColor colorbase;
    if (abs(colorHighlightBg.lightness() - colorTextLightness) < abs(colorHighlightFg.lightness() - colorTextLightness))
        colorbase = colorHighlightBg;
    else
        colorbase = colorHighlightFg;
    return colorbase;
}

QIcon SingleColorIcon(const QString& filename)
{
    return SingleColorIcon(filename, SingleColor());
}

static QColor TextColor()
{
    return QColor(QApplication::palette().color(QPalette::WindowText));
}

QIcon TextColorIcon(const QString& filename)
{
    return SingleColorIcon(filename, TextColor());
}

QIcon TextColorIcon(const QIcon& ico)
{
    return SingleColorIcon(ico, TextColor());
}
