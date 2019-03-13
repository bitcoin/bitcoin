// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ADDRESSTYPE_H
#define BITCOIN_ADDRESSTYPE_H

#include <optional.h>

#include <string>

enum class AddressType {
    BASE58,
    BECH32
};

Optional<AddressType> ParseAddressType(const std::string& type);
const std::string& FormatAddressType(AddressType type);

#endif // BITCOIN_ADDRESSTYPE_H
