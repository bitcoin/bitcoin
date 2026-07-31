// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#ifndef BITCOIN_WALLET_BIP39_H
#define BITCOIN_WALLET_BIP39_H

#include <key.h>
#include <util/expected.h>

#include <string_view>

namespace wallet::bip39 {

enum class Error {
    InvalidWordCount,
    UnknownWord,
    InvalidChecksum,
    InvalidPassphrase,
    InvalidSeed,
};

/**
 * Decode an English BIP39 mnemonic and derive its BIP32 master key.
 *
 * ASCII whitespace surrounding and separating mnemonic words is normalized to
 * one space. The passphrase is used verbatim and must contain only ASCII bytes.
 */
[[nodiscard]] util::Expected<CExtKey, Error> DecodeMnemonic(
    std::string_view mnemonic,
    std::string_view passphrase);

} // namespace wallet::bip39

#endif // BITCOIN_WALLET_BIP39_H
