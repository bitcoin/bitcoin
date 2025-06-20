// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_DUMP_H
#define BITCOIN_WALLET_DUMP_H

#include <fs.h>

#include <string>
#include <vector>

class CWallet;

struct bilingual_str;
class ArgsManager;

bool DumpWallet(const ArgsManager& args, CWallet& wallet, bilingual_str& error);
bool CreateFromDump(const ArgsManager& args, const std::string& name, const fs::path& wallet_path, bilingual_str& error, std::vector<bilingual_str>& warnings);

#endif // BITCOIN_WALLET_DUMP_H
