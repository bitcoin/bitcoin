// Copyright (c) 2016 BitPay, Inc.
// Copyright (c) 2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <addressindex.h>

#include <key_io.h>
#include <hash.h>
#include <script/script.h>

template <typename T1>
inline std::vector<uint8_t> TrimScriptP2PKH(const T1& input) {
    return std::vector<uint8_t>(input.begin() + 3, input.begin() + 23);
};

template <typename T1>
inline std::vector<uint8_t> TrimScriptP2SH(const T1& input) {
    return std::vector<uint8_t>(input.begin() + 2, input.begin() + 22);
};

template <typename T1>
inline std::vector<uint8_t> TrimScriptP2PK(const T1& input) {
    return std::vector<uint8_t>(input.begin() + 1, input.end() - 1);
};

bool AddressBytesFromScript(const CScript& script, AddressType& address_type, uint160& address_bytes) {
    if (script.IsPayToScriptHash()) {
        address_type  = AddressType::P2SH;
        address_bytes = uint160(TrimScriptP2SH(script));
    } else if (script.IsPayToPublicKeyHash()) {
        address_type  = AddressType::P2PK_OR_P2PKH;
        address_bytes = uint160(TrimScriptP2PKH(script));
    } else if (script.IsPayToPublicKey()) {
        address_type  = AddressType::P2PK_OR_P2PKH;
        address_bytes = Hash160(TrimScriptP2PK(script));
    } else {
        address_type  = AddressType::UNKNOWN;
        address_bytes.SetNull();
        return false;
    }
    return true;
}
