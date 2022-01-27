// Copyright (c) 2015-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_REST_H
#define BITCOIN_REST_H

#include <string>

enum class RetFormat {
    UNDEF,
    BINARY,
    HEX,
    JSON,
};

RetFormat ParseDataFormat(std::string& param, const std::string& strReq);

#endif // BITCOIN_REST_H
