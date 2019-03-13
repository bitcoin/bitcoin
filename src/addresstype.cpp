// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addresstype.h>

static const std::string ADDRESS_TYPE_STRING_BASE58 = "legacy";
static const std::string ADDRESS_TYPE_STRING_BECH32 = "bech32";

Optional<AddressType> ParseAddressType(const std::string& type)
{
    if (type == ADDRESS_TYPE_STRING_BASE58) {
        return AddressType::BASE58;
    } else if (type == ADDRESS_TYPE_STRING_BECH32) {
        return AddressType::BECH32;
    }
    return nullopt;
}

const std::string& FormatAddressType(AddressType type)
{
    switch (type) {
    case AddressType::BASE58: return ADDRESS_TYPE_STRING_BASE58;
    case AddressType::BECH32: return ADDRESS_TYPE_STRING_BECH32;
    }
}
