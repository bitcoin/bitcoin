// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CORE_WRITE_H
#define BITCOIN_CORE_WRITE_H

#include <amount.h>

#include <string>

class CScript;
class CTransaction;
class uint256;
class UniValue;

UniValue ValueFromAmount(const CAmount& amount);
std::string FormatScript(const CScript& script);
std::string EncodeHexTx(const CTransaction& tx, const int serializeFlags = 0);
std::string SighashToStr(unsigned char sighash_type);
void ScriptPubKeyToUniv(const CScript& scriptPubKey, UniValue& out, bool fIncludeHex);
std::string ScriptToAsmStr(const CScript& script, const bool fAttemptSighashDecode = false);
void ScriptToUniv(const CScript& script, UniValue& out, bool include_address);
void TxToUniv(const CTransaction& tx, const uint256& hashBlock, UniValue& entry, bool include_hex = true, int serialize_flags = 0);

#endif // BITCOIN_CORE_WRITE_H
