// Copyright (c) 2014 The Bitcoin Core developers
// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "networkstyle.h"

#include "guiconstants.h"
#include "guiutil.h"

#include "chainparams.h"
#include "tinyformat.h"

#include <QApplication>

static const struct {
    const char *networkId;
    const char *appName;
    const int iconColorHueShift;
    const int iconColorSaturationReduction;
    const std::string titleAddText;
} network_styles[] = {
    {"main", QAPP_APP_NAME_DEFAULT, 0, 0, ""},
    {"test", QAPP_APP_NAME_TESTNET, 190, 20, QT_TRANSLATE_NOOP("SplashScreen", "[testnet]")},
    {"dev", QAPP_APP_NAME_DEVNET, 190, 20, "[devnet: %s]"},
    {"regtest", QAPP_APP_NAME_TESTNET, 160, 30, "[regtest]"}
};
static const unsigned network_styles_count = sizeof(network_styles)/sizeof(*network_styles);

void NetworkStyle::rotateColors(QImage& img, const int iconColorHueShift, const int iconColorSaturationReduction) {
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
            // 70° should end up with the typical "testnet" green (in bitcoin)
            h+=iconColorHueShift;

            // change saturation value
            s -= iconColorSaturationReduction;
            s = std::max(s, 0);

            col.setHsl(h,s,l,a);

            // set the pixel
            scL[x] = col.rgba();
        }
    }
}

// titleAddText needs to be const char* for tr()
NetworkStyle::NetworkStyle(const QString &_appName, const int iconColorHueShift, const int iconColorSaturationReduction, const char *_titleAddText):
    appName(_appName),
    titleAddText(qApp->translate("SplashScreen", _titleAddText))
{
    // Allow for separate UI settings for testnets
    QApplication::setApplicationName(appName);
    // Make sure settings migrated properly
    GUIUtil::migrateQtSettings();
    // Grab theme from settings
    QString theme = GUIUtil::getThemeName();
    // load pixmap
    QPixmap appIconPixmap(":/icons/bitcoin");
    QPixmap splashImagePixmap(":/images/" + theme + "/splash");

    if(iconColorHueShift != 0 && iconColorSaturationReduction != 0)
    {
        // generate QImage from QPixmap
        QImage appIconImg = appIconPixmap.toImage();
        QImage splashImageImg = splashImagePixmap.toImage();

        rotateColors(appIconImg, iconColorHueShift, iconColorSaturationReduction);
        rotateColors(splashImageImg, iconColorHueShift, iconColorSaturationReduction);

        //convert back to QPixmap
#if QT_VERSION >= 0x040700
        appIconPixmap.convertFromImage(appIconImg);
        splashImagePixmap.convertFromImage(splashImageImg);
#else
        appIconPixmap = QPixmap::fromImage(appIconImg);
        splashImagePixmap = QPixmap::fromImage(splashImageImg);
#endif
    }

    appIcon             = QIcon(appIconPixmap);
    trayAndWindowIcon   = QIcon(appIconPixmap.scaled(QSize(256,256)));
    splashImage         = splashImagePixmap;
}

const NetworkStyle *NetworkStyle::instantiate(const QString &networkId)
{
    for (unsigned x=0; x<network_styles_count; ++x)
    {
        if (networkId == network_styles[x].networkId)
        {
            std::string appName = network_styles[x].appName;
            std::string titleAddText = network_styles[x].titleAddText;

            if (networkId == QString(CBaseChainParams::DEVNET.c_str())) {
                appName = strprintf(appName, GetDevNetName());
                titleAddText = strprintf(titleAddText, GetDevNetName());
            }

            return new NetworkStyle(
                    appName.c_str(),
                    network_styles[x].iconColorHueShift,
                    network_styles[x].iconColorSaturationReduction,
                    titleAddText.c_str());
        }
    }
    return 0;
}
