// Copyright (c) 2014-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/networkstyle.h>

#include <qt/guiconstants.h>

#include <chainparamsbase.h>
#include <tinyformat.h>

#include <QApplication>

static const struct {
    const char* network_id;
    const char* app_name;
    const char* icon_file;
} network_styles[] = {
    {"main", QAPP_APP_NAME_DEFAULT, ":/svg/bitcoin"},
    {"test", QAPP_APP_NAME_TESTNET, ":/svg/bitcoin-test"},
    {"regtest", QAPP_APP_NAME_REGTEST, ":/svg/bitcoin-regtest"}
};
static const unsigned network_styles_count = sizeof(network_styles) / sizeof(*network_styles);

// title_add_text needs to be const char* for tr()
NetworkStyle::NetworkStyle(const QString& app_name, const QString& icon_file, const char* title_add_text)
    : m_app_name(app_name), m_icon(icon_file), m_title_add_text(qApp->translate("SplashScreen", title_add_text))
{
}

const NetworkStyle* NetworkStyle::instantiate(const std::string& network_id)
{
    std::string title_add_text = network_id == CBaseChainParams::MAIN ? "" : strprintf("[%s]", network_id);
    for (unsigned x = 0; x < network_styles_count; ++x) {
        if (network_id == network_styles[x].network_id) {
            return new NetworkStyle(
                network_styles[x].app_name,
                network_styles[x].icon_file,
                title_add_text.c_str());
        }
    }
    return nullptr;
}
