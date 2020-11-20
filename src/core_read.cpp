// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <core_io.h>

#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <script/sign.h>
#include <serialize.h>
#include <streams.h>
#include <univalue.h>
#include <util/strencodings.h>
#include <version.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include <algorithm>
#include <string>

namespace {

opcodetype ParseOpCode(const std::string& s)
{
    static std::map<std::string, opcodetype> mapOpNames;

    if (mapOpNames.empty())
    {
        for (unsigned int op = 0; op <= MAX_OPCODE; op++)
        {
            // Allow OP_RESERVED to get into mapOpNames
            if (op < OP_NOP && op != OP_RESERVED)
                continue;

            std::string strName = GetOpName(static_cast<opcodetype>(op));
            if (strName == "OP_UNKNOWN")
                continue;
            mapOpNames[strName] = static_cast<opcodetype>(op);
            // Convenience: OP_ADD and just ADD are both recognized:
            if (strName.compare(0, 3, "OP_") == 0) {  // strName starts with "OP_"
                mapOpNames[strName.substr(3)] = static_cast<opcodetype>(op);
            }
        }
    }

    auto it = mapOpNames.find(s);
    if (it == mapOpNames.end()) throw std::runtime_error("script parse error: unknown opcode");
    return it->second;
}

} // namespace

CScript ParseScript(const std::string& s)
{
    CScript result;

    std::vector<std::string> words;
    boost::algorithm::split(words, s, boost::algorithm::is_any_of(" \t\n"), boost::algorithm::token_compress_on);

    for (std::vector<std::string>::const_iterator w = words.begin(); w != words.end(); ++w)
    {
        if (w->empty())
        {
            // Empty string, ignore. (boost::split given '' will return one word)
        }
        else if (std::all_of(w->begin(), w->end(), ::IsDigit) ||
            (w->front() == '-' && w->size() > 1 && std::all_of(w->begin()+1, w->end(), ::IsDigit)))
        {
            // Number
            int64_t n = atoi64(*w);

            //limit the range of numbers ParseScript accepts in decimal
            //since numbers outside -0xFFFFFFFF...0xFFFFFFFF are illegal in scripts
            if (n > int64_t{0xffffffff} || n < -1 * int64_t{0xffffffff}) {
                throw std::runtime_error("script parse error: decimal numeric value only allowed in the "
                                         "range -0xFFFFFFFF...0xFFFFFFFF");
            }

            result << n;
        }
        else if (w->substr(0,2) == "0x" && w->size() > 2 && IsHex(std::string(w->begin()+2, w->end())))
        {
            // Raw hex data, inserted NOT pushed onto stack:
            std::vector<unsigned char> raw = ParseHex(std::string(w->begin()+2, w->end()));
            result.insert(result.end(), raw.begin(), raw.end());
        }
        else if (w->size() >= 2 && w->front() == '\'' && w->back() == '\'')
        {
            // Single-quoted string, pushed as data. NOTE: this is poor-man's
            // parsing, spaces/tabs/newlines in single-quoted strings won't work.
            std::vector<unsigned char> value(w->begin()+1, w->end()-1);
            result << value;
        }
        else
        {
            // opcode, e.g. OP_ADD or ADD:
            result << ParseOpCode(*w);
        }
    }

    return result;
}

// Check that all of the input and output scripts of a transaction contains valid opcodes
static bool CheckTxScriptsSanity(const CMutableTransaction& tx)
{
    // Check input scripts for non-coinbase txs
    if (!CTransaction(tx).IsCoinBase()) {
        for (unsigned int i = 0; i < tx.vin.size(); i++) {
            if (!tx.vin[i].scriptSig.HasValidOps() || tx.vin[i].scriptSig.size() > MAX_SCRIPT_SIZE) {
                return false;
            }
        }
    }
    // Check output scripts
    for (unsigned int i = 0; i < tx.vout.size(); i++) {
        if (!tx.vout[i].scriptPubKey.HasValidOps() || tx.vout[i].scriptPubKey.size() > MAX_SCRIPT_SIZE) {
            return false;
        }
    }

    return true;
}

static bool DecodeTx(CMutableTransaction& tx, const std::vector<unsigned char>& tx_data, bool try_no_witness, bool try_witness)
{
    if (try_witness) {
        CDataStream ssData(tx_data, SER_NETWORK, PROTOCOL_VERSION);
        try {
            ssData >> tx;
            // If transaction looks sane, we don't try other mode even if requested
            if (ssData.empty() && (!try_no_witness || CheckTxScriptsSanity(tx))) {
                return true;
            }
        } catch (const std::exception&) {
            // Fall through.
        }
    }

    if (try_no_witness) {
        CDataStream ssData(tx_data, SER_NETWORK, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS);
        try {
            ssData >> tx;
            if (ssData.empty()) {
                return true;
            }
        } catch (const std::exception&) {
            // Fall through.
        }
    }

    return false;
}

bool DecodeHexTx(CMutableTransaction& tx, const std::string& hex_tx, bool try_no_witness, bool try_witness)
{
    if (!IsHex(hex_tx)) {
        return false;
    }

    std::vector<unsigned char> txData(ParseHex(hex_tx));
    return DecodeTx(tx, txData, try_no_witness, try_witness);
}

bool DecodeHexBlockHeader(CBlockHeader& header, const std::string& hex_header)
{
    if (!IsHex(hex_header)) return false;

    const std::vector<unsigned char> header_data{ParseHex(hex_header)};
    CDataStream ser_header(header_data, SER_NETWORK, PROTOCOL_VERSION);
    try {
        ser_header >> header;
    } catch (const std::exception&) {
        return false;
    }
    return true;
}

bool DecodeHexBlk(CBlock& block, const std::string& strHexBlk)
{
    if (!IsHex(strHexBlk))
        return false;

    std::vector<unsigned char> blockData(ParseHex(strHexBlk));
    CDataStream ssBlock(blockData, SER_NETWORK, PROTOCOL_VERSION);
    try {
        ssBlock >> block;
    }
    catch (const std::exception&) {
        return false;
    }

    return true;
}

bool ParseHashStr(const std::string& strHex, uint256& result)
{
    if ((strHex.size() != 64) || !IsHex(strHex))
        return false;

    result.SetHex(strHex);
    return true;
}

std::vector<unsigned char> ParseHexUV(const UniValue& v, const std::string& strName)
{
    std::string strHex;
    if (v.isStr())
        strHex = v.getValStr();
    if (!IsHex(strHex))
        throw std::runtime_error(strName + " must be hexadecimal string (not '" + strHex + "')");
    return ParseHex(strHex);
}

int ParseSighashString(const UniValue& sighash)
{
    int hash_type = SIGHASH_ALL;
    if (!sighash.isNull()) {
        static std::map<std::string, int> map_sighash_values = {
            {std::string("ALL"), int(SIGHASH_ALL)},
            {std::string("ALL|ANYONECANPAY"), int(SIGHASH_ALL|SIGHASH_ANYONECANPAY)},
            {std::string("NONE"), int(SIGHASH_NONE)},
            {std::string("NONE|ANYONECANPAY"), int(SIGHASH_NONE|SIGHASH_ANYONECANPAY)},
            {std::string("SINGLE"), int(SIGHASH_SINGLE)},
            {std::string("SINGLE|ANYONECANPAY"), int(SIGHASH_SINGLE|SIGHASH_ANYONECANPAY)},
        };
        std::string strHashType = sighash.get_str();
        const auto& it = map_sighash_values.find(strHashType);
        if (it != map_sighash_values.end()) {
            hash_type = it->second;
        } else {
            throw std::runtime_error(strHashType + " is not a valid sighash parameter.");
        }
    }
    return hash_type;
}
