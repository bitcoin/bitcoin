// Copyright (c) 2018-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/bitcoin.h>

#include <util/translation.h>
#include <util/url.h>

#include <QCoreApplication>

#include <functional>
#include <string>
#include <consensus/dynengine.h>
#include <primitives/dynnft_manager.h>
#include <rpc/webserver.h>

CDynHash* g_hashFunction;
CDynEngine* g_dynEngine;
CContractManager* g_contractMgr;
CNFTManager* g_nftMgr;
CWebServer* g_webServer;

bool IS_TESTNET;

/** Translate string to current locale using Qt. */
extern const std::function<std::string(const char*)> G_TRANSLATION_FUN = [](const char* psz) {
    return QCoreApplication::translate("bitcoin-core", psz).toStdString();
};
UrlDecodeFn* const URL_DECODE = urlDecode;

int main(int argc, char* argv[]) {

    IS_TESTNET = false;

    return GuiMain(argc, argv);
}
