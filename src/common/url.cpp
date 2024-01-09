// Copyright (c) 2015-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <common/url.h>

#include <event2/http.h>

#include <cstdlib>
#include <string>

std::string urlDecode(const std::string &urlEncoded) {
    std::string res;
    if (!urlEncoded.empty()) {
        char *decoded = evhttp_uridecode(urlEncoded.c_str(), false, nullptr);
        if (decoded) {
            res = std::string(decoded);
            free(decoded);
        }
    }
    return res;
}
