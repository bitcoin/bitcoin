// Copyright (c) 2014-2019 The Bitcoin Core developers
// Copyright (c) 2014-2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/networkstyle.h>

#include <qt/guiconstants.h>
#include <qt/guiutil.h>

#include <chainparams.h>
#include <tinyformat.h>
#include <util/system.h>

#include <chainparamsbase.h>

#include <QApplication>

static const struct {
    const char *networkId;
    const char *appName;
    const int iconColorHueShift;
    const int iconColorSaturationReduction;
    const std::string titleAddText;
} network_styles[] = {
    {"main", QAPP_APP_NAME_DEFAULT, 0, 0, ""},
    {"test", QAPP_APP_NAME_TESTNET, 190, 20},
    {"devnet", QAPP_APP_NAME_DEVNET, 190, 20, "[devnet: %s]"},
    {"regtest", QAPP_APP_NAME_REGTEST, 160, 30}
};

void NetworkStyle::rotateColor(QColor& col, const int iconColorHueShift, const int iconColorSaturationReduction)
{
    int h, s, l, a;
    col.getHsl(&h, &s, &l, &a);

    // rotate color on RGB color circle
    h += iconColorHueShift;
    // change saturation value
    s -= iconColorSaturationReduction;
    s = std::max(s, 0);

    col.setHsl(h,s,l,a);
}

void NetworkStyle::rotateColors(QImage& img, const int iconColorHueShift, const int iconColorSaturationReduction)
{
    // traverse though lines
    for(int y=0;y<img.height();y++)
    {
        QRgb *scL = reinterpret_cast< QRgb *>( img.scanLine( y ) );

        // loop through pixels
        for(int x=0;x<img.width();x++)
        {
            QColor col;
            col.setRgba(scL[x]);
            rotateColor(col, iconColorHueShift, iconColorSaturationReduction);
            scL[x] = col.rgba();
        }
    }
}

// titleAddText needs to be const char* for tr()
NetworkStyle::NetworkStyle(const QString &_appName, const int iconColorHueShift, const int iconColorSaturationReduction, const char *_titleAddText):
    appName(_appName),
    titleAddText(qApp->translate("SplashScreen", _titleAddText)),
    badgeColor(QColor(0, 141, 228)) // default badge color is the original Dash's blue, regardless of the current theme
{
    // Allow for separate UI settings for testnets
    QApplication::setApplicationName(appName);
    // load pixmap
    QPixmap appIconPixmap(":/icons/dash");

    if(iconColorHueShift != 0 && iconColorSaturationReduction != 0)
    {
        // generate QImage from QPixmap
        QImage appIconImg = appIconPixmap.toImage();
        rotateColors(appIconImg, iconColorHueShift, iconColorSaturationReduction);
        //convert back to QPixmap
        appIconPixmap.convertFromImage(appIconImg);
        // tweak badge color
        rotateColor(badgeColor, iconColorHueShift, iconColorSaturationReduction);
    }

    appIcon             = QIcon(appIconPixmap);
    trayAndWindowIcon   = QIcon(appIconPixmap.scaled(QSize(256,256)));
    splashImage         = QPixmap(":/images/splash");
}

const NetworkStyle* NetworkStyle::instantiate(const std::string& networkId)
{
    std::string titleAddText = networkId == CBaseChainParams::MAIN ? "" : strprintf("[%s]", networkId);
    for (const auto& network_style : network_styles)
    {
        if (networkId == network_style.networkId)
        {
            std::string appName = network_style.appName;
            std::string titleAddText = network_style.titleAddText;

            if (networkId == CBaseChainParams::DEVNET.c_str()) {
                appName = strprintf(appName, gArgs.GetDevNetName());
                titleAddText = strprintf(titleAddText, gArgs.GetDevNetName());
            }

            return new NetworkStyle(
                    appName.c_str(),
                    network_style.iconColorHueShift,
                    network_style.iconColorSaturationReduction,
                    titleAddText.c_str());
        }
    }
    return nullptr;
}
