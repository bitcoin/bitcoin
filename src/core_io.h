// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CORE_IO_H
#define BITCOIN_CORE_IO_H

#include <consensus/amount.h>
#include <util/result.h>

#include <string>
#include <vector>
#include <optional>

class CBlock;
class CBlockHeader;
class CFeeRate;
class CScript;
class CTransaction;
struct CMutableTransaction;
class SigningProvider;
class uint256;
class UniValue;
class CTxUndo;

/**
 * Verbose level for block's transaction
 */
enum class TxVerbosity {
    SHOW_TXID,                //!< Only TXID for each block's transaction
    SHOW_DETAILS,             //!< Include TXID, inputs, outputs, and other common block's transaction information
    SHOW_DETAILS_AND_PREVOUT  //!< The same as previous option with information about prevouts if available
};

// core_read.cpp
CScript ParseScript(const std::string& s);
std::string ScriptToAsmStr(const CScript& script, const bool fAttemptSighashDecode = false);
[[nodiscard]] bool DecodeTx(CMutableTransaction& tx, const std::vector<unsigned char>& tx_data, bool try_no_witness, bool try_witness);
[[nodiscard]] bool DecodeHexTx(CMutableTransaction& tx, const std::string& hex_tx, bool try_no_witness = false, bool try_witness = true);
[[nodiscard]] bool DecodeHexBlk(CBlock&, const std::string& strHexBlk);
bool DecodeHexBlockHeader(CBlockHeader&, const std::string& hex_header);

[[nodiscard]] util::Result<int> SighashFromStr(const std::string& sighash);

// core_write.cpp
UniValue ValueFromAmount(const CAmount amount);
UniValue ValueFromFeeRate(const CFeeRate& fee_rate);
std::string FormatScript(const CScript& script);
std::string EncodeHexTx(const CTransaction& tx);
std::string SighashToStr(unsigned char sighash_type);
void ScriptToUniv(const CScript& script, UniValue& out, bool include_hex = true, bool include_address = false, const SigningProvider* provider = nullptr);
void TxToUniv(const CTransaction& tx, const uint256& block_hash, UniValue& entry, bool include_hex = true, const CTxUndo* txundo = nullptr, TxVerbosity verbosity = TxVerbosity::SHOW_DETAILS);

#endif // BITCOIN_CORE_IO_H
