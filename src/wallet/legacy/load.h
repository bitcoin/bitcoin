// Copyright (c) 2026-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_LEGACY_LOAD_H
#define BITCOIN_WALLET_LEGACY_LOAD_H

#include <wallet/legacy/migration.h>

namespace wallet {
namespace legacy_migration {

std::shared_ptr<LegacyWallet> LoadWalletForMigration(const std::string& name, bilingual_str& error);

} // namespace legacy_migration
} // namespace wallet
#endif // BITCOIN_WALLET_LEGACY_LOAD_H
