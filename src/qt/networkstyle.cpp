// Copyright (c) 2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "networkstyle.h"

#include "guiconstants.h"

#include <QApplication>

static const struct {
    const char *networkId;
    const char *appName;
    const int iconColorHueShift;
    const int iconColorSaturationReduction;
    const char *titleAddText;
} network_styles[] = {
    {"main", QAPP_APP_NAME_DEFAULT, 0, 0, ""},
    {"test", QAPP_APP_NAME_TESTNET, 70, 30, QT_TRANSLATE_NOOP("SplashScreen", "[testnet]")},
    {"regtest", QAPP_APP_NAME_TESTNET, 70, 30, "[regtest]"}
};
static const unsigned network_styles_count = sizeof(network_styles)/sizeof(*network_styles);

// titleAddText needs to be const char* for tr()
NetworkStyle::NetworkStyle(const QString &appName, const int iconColorHueShift, const int iconColorSaturationReduction, const char *titleAddText):
    appName(appName),
    iconColorHueShift(iconColorHueShift),
    iconColorSaturationReduction(iconColorSaturationReduction),
    titleAddText(qApp->translate("SplashScreen", titleAddText))
{
}

const NetworkStyle *NetworkStyle::instantiate(const QString &networkId)
{
    for (unsigned x=0; x<network_styles_count; ++x)
    {
        if (networkId == network_styles[x].networkId)
        {
            return new NetworkStyle(
                    network_styles[x].appName,
                    network_styles[x].iconColorHueShift,
                    network_styles[x].iconColorSaturationReduction,
                    network_styles[x].titleAddText);
        }
    }
    return 0;
}

QIcon NetworkStyle::getAppIcon() const
{
    return getAppIcon(QSize(256,256));
}

QIcon NetworkStyle::getAppIcon(const QSize size) const
{
    // load pixmap
    QPixmap pixmap(":/icons/bitcoin");

    if(pixmap.size().width() != size.width() && pixmap.size().height() != size.height())
    {
        QPixmap scaledPixmap = pixmap.scaled(size, Qt::KeepAspectRatio);
        if(!scaledPixmap.isNull())
        {
            pixmap = scaledPixmap;
        }
    }

    if(iconColorHueShift != 0 && iconColorSaturationReduction != 0)
    {
        // copy the pixmap because on linux the original pixmap will be affected
        pixmap = pixmap.copy();

        // generate QImage from QPixmap
        QImage img = pixmap.toImage();

        int h,s,l,a;

        // traverse though lines
        for(int y=0;y<img.height();y++)
        {
            QRgb *scL = reinterpret_cast< QRgb *>( img.scanLine( y ) );

            // loop through pixels
            for(int x=0;x<img.width();x++)
            {
                // preserve alpha because QColor::getHsl doesen't return the alpha value
                a = qAlpha(scL[x]);
                QColor col(scL[x]);

                // get hue value
                col.getHsl(&h,&s,&l);

                // rotate color on RGB color circle
                // 70° should end up with the typical "testnet" green
                h+=iconColorHueShift;

                // change saturation value
                if(s>iconColorSaturationReduction)
                {
                    s -= iconColorSaturationReduction;
                }
                col.setHsl(h,s,l,a);

                // set the pixel
                scL[x] = col.rgba();
            }
        }

        //convert back to QPixmap
        pixmap.convertFromImage(img);
    }

    QIcon icon(pixmap);
    return icon;
}
