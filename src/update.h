// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UPDATE_H
#define BITCOIN_UPDATE_H

#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <curl/curl.h>
#include <clientversion.h>
#include <key_io.h>
#include <outputtype.h>
#include <script/descriptor.h>
#include <util/system.h>
#include <util/strencodings.h>
#include <util/validation.h>
#include <stdlib.h>
#include <time.h>
#include <regex>

class UpdateManager {
public:
    void addSource(std::string url);
    void addKey(std::string pubKey);
    std::string checkUpdate();
    UpdateManager();
private:
    std::vector<std::string> sources;
    std::vector<std::string> keys;
    std::string download(std::string url);
    bool verify(std::string strSign, std::string strMessage);
};

#endif