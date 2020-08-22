// Copyright (c) 2014-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/networkstyle.h>

#include <qt/guiconstants.h>

#include <chainparamsbase.h>
#include <tinyformat.h>

#include <QApplication>

static const struct {
    const char *networkId;
    const char *appName;
    const char* icon_file;
} network_styles[] = {
    {"main", QAPP_APP_NAME_DEFAULT, ":/svg/bitcoin"},
    {"test", QAPP_APP_NAME_TESTNET, ":/svg/bitcoin-test"},
    {"regtest", QAPP_APP_NAME_REGTEST, ":/svg/bitcoin-regtest"}
};
static const unsigned network_styles_count = sizeof(network_styles)/sizeof(*network_styles);

// titleAddText needs to be const char* for tr()
NetworkStyle::NetworkStyle(const QString& _appName, const QString& icon_file, const char* _titleAddText):
    appName(_appName),
    titleAddText(qApp->translate("SplashScreen", _titleAddText))
{
    appIcon             = QIcon(icon_file);
    trayAndWindowIcon   = QIcon(icon_file);
}

const NetworkStyle* NetworkStyle::instantiate(const std::string& networkId)
{
    std::string titleAddText = networkId == CBaseChainParams::MAIN ? "" : strprintf("[%s]", networkId);
    for (unsigned x=0; x<network_styles_count; ++x)
    {
        if (networkId == network_styles[x].networkId)
        {
            return new NetworkStyle(
                    network_styles[x].appName,
                    network_styles[x].icon_file,
                    titleAddText.c_str());
        }
    }
    return nullptr;
}
