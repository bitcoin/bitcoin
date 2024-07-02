// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <core_io.h>

#include <common/system.h>
#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <consensus/validation.h>
#include <key_io.h>
#include <script/descriptor.h>
#include <script/script.h>
#include <script/solver.h>
#include <serialize.h>
#include <streams.h>
#include <undo.h>
#include <univalue.h>
#include <util/check.h>
#include <util/strencodings.h>

#include <map>
#include <string>
#include <vector>

UniValue ValueFromAmount(const CAmount amount)
{
    static_assert(COIN > 1);
    int64_t quotient = amount / COIN;
    int64_t remainder = amount % COIN;
    if (amount < 0) {
        quotient = -quotient;
        remainder = -remainder;
    }
    return UniValue(UniValue::VNUM,
            strprintf("%s%d.%08d", amount < 0 ? "-" : "", quotient, remainder));
}

std::string FormatScript(const CScript& script)
{
    std::string ret;
    CScript::const_iterator it = script.begin();
    opcodetype op;
    while (it != script.end()) {
        CScript::const_iterator it2 = it;
        std::vector<unsigned char> vch;
        if (script.GetOp(it, op, vch)) {
            if (op == OP_0) {
                ret += "0 ";
                continue;
            } else if ((op >= OP_1 && op <= OP_16) || op == OP_1NEGATE) {
                ret += strprintf("%i ", op - OP_1NEGATE - 1);
                continue;
            } else if (op >= OP_NOP && op <= OP_NOP10) {
                ret += strprintf("%s ", GetOpNameAsm(op));
                continue;
            }
            if (vch.size() > 0) {
                ret += strprintf("0x%x 0x%x ", HexStr(std::vector<uint8_t>(it2, it - vch.size())),
                                               HexStr(std::vector<uint8_t>(it - vch.size(), it)));
            } else {
                ret += strprintf("0x%x ", HexStr(std::vector<uint8_t>(it2, it)));
            }
            continue;
        }
        // Undecodable bytes
        ret += strprintf("UNPARSABLE:(%x) ", HexStr(std::vector<uint8_t>(it2, script.end())));
        break;
    }
    return ret.substr(0, ret.empty() ? ret.npos : ret.size() - 1);
}

const std::map<unsigned char, std::string> mapSigHashTypes = {
    {static_cast<unsigned char>(SIGHASH_ALL), std::string("ALL")},
    {static_cast<unsigned char>(SIGHASH_ALL|SIGHASH_ANYONECANPAY), std::string("ALL|ANYONECANPAY")},
    {static_cast<unsigned char>(SIGHASH_NONE), std::string("NONE")},
    {static_cast<unsigned char>(SIGHASH_NONE|SIGHASH_ANYONECANPAY), std::string("NONE|ANYONECANPAY")},
    {static_cast<unsigned char>(SIGHASH_SINGLE), std::string("SINGLE")},
    {static_cast<unsigned char>(SIGHASH_SINGLE|SIGHASH_ANYONECANPAY), std::string("SINGLE|ANYONECANPAY")},
};

std::string SighashToStr(unsigned char sighash_type)
{
    const auto& it = mapSigHashTypes.find(sighash_type);
    if (it == mapSigHashTypes.end()) return "";
    return it->second;
}

/*
 * Format pushed bytes into appropriate ASM format
 */
std::string FormatPushDataAsm(const std::vector<unsigned char>& vch, const opcodetype& opcode)
{
    // Use OPCODE<hex> for non-minimal pushes
    if (!CheckMinimalPush(vch, opcode)) {
        return GetOpNameAsm(opcode) + "<" + HexStr(vch) + ">";
    }

    // Use <hex> for minimal pushes > 5 bytes
    if (vch.size() > 5) {
        return "<" + HexStr(vch) + ">";
    }

    // Use decimal for minimally-encoded, minimal pushes <= 5 bytes
    // Note: OP_CLTV / OP_CSV accept 5-byte numbers
    try {
        CScriptNum n{vch, true, 5};
        return strprintf("%lld", n.GetInt64());
    }

    // Use OPCODE<hex> for non-minimally-encoded minimal pushes
    catch (scriptnum_error&) {
        return "<" + HexStr(vch) + ">";
    }
}

/*
 * Format an opcode and associated data into an ASM string
 */
std::string OpcodeToAsmString(const opcodetype& opcode, const std::vector<unsigned char>& vch)
{
    switch (opcode) {
        case OP_0:
            return "0";
        case OP_1NEGATE:
            return "-1";
        case OP_1:
        case OP_2:
        case OP_3:
        case OP_4:
        case OP_5:
        case OP_6:
        case OP_7:
        case OP_8:
        case OP_9:
        case OP_10:
        case OP_11:
        case OP_12:
        case OP_13:
        case OP_14:
        case OP_15:
        case OP_16:
            return strprintf("%i", opcode - OP_1NEGATE - 1);
        default:
            // Push opcodes 1-75
            if (0 < opcode && opcode <= OP_PUSHDATA4) {
                return FormatPushDataAsm(vch, opcode);
            }
            auto op_name = GetOpNameAsm(opcode);
            // Unknown opcodes
            if (op_name == "UNKNOWN") {
                return strprintf("UNKNOWN_%s", opcode);
            }
            // Other opcodes
            return op_name;
    }
}

/**
 * Create the assembly string representation of a CScript object.
 * @param[in] script    CScript object to convert into the asm string representation.
 */
std::string ScriptToAsmStr(const CScript& script)
{
    std::string str;
    opcodetype opcode;
    std::vector<unsigned char> vch;
    CScript::const_iterator pc = script.begin();
    while (pc < script.end()) {
        if (!str.empty()) {
            str += " ";
        }
        auto start = pc;
        if (!script.GetOp(pc, opcode, vch)) {
            std::vector<unsigned char> remaining_bytes(start, script.end());
            str += strprintf("UNPARSABLE(%s)", HexStr(remaining_bytes));
            break;
        }
        str += OpcodeToAsmString(opcode, vch);
    }
    return str;
}

std::string EncodeHexTx(const CTransaction& tx)
{
    DataStream ssTx;
    ssTx << TX_WITH_WITNESS(tx);
    return HexStr(ssTx);
}

void ScriptToUniv(const CScript& script, UniValue& out, bool include_hex, bool include_address, const SigningProvider* provider)
{
    CTxDestination address;

    out.pushKV("asm", ScriptToAsmStr(script));
    if (include_address) {
        out.pushKV("desc", InferDescriptor(script, provider ? *provider : DUMMY_SIGNING_PROVIDER)->ToString());
    }
    if (include_hex) {
        out.pushKV("hex", HexStr(script));
    }

    std::vector<std::vector<unsigned char>> solns;
    const TxoutType type{Solver(script, solns)};

    if (include_address && ExtractDestination(script, address) && type != TxoutType::PUBKEY) {
        out.pushKV("address", EncodeDestination(address));
    }
    out.pushKV("type", GetTxnOutputType(type));
}

void TxToUniv(const CTransaction& tx, const uint256& block_hash, UniValue& entry, bool include_hex, const CTxUndo* txundo, TxVerbosity verbosity)
{
    CHECK_NONFATAL(verbosity >= TxVerbosity::SHOW_DETAILS);

    entry.pushKV("txid", tx.GetHash().GetHex());
    entry.pushKV("hash", tx.GetWitnessHash().GetHex());
    entry.pushKV("version", tx.version);
    entry.pushKV("size", tx.GetTotalSize());
    entry.pushKV("vsize", (GetTransactionWeight(tx) + WITNESS_SCALE_FACTOR - 1) / WITNESS_SCALE_FACTOR);
    entry.pushKV("weight", GetTransactionWeight(tx));
    entry.pushKV("locktime", (int64_t)tx.nLockTime);

    UniValue vin{UniValue::VARR};

    // If available, use Undo data to calculate the fee. Note that txundo == nullptr
    // for coinbase transactions and for transactions where undo data is unavailable.
    const bool have_undo = txundo != nullptr;
    CAmount amt_total_in = 0;
    CAmount amt_total_out = 0;

    for (unsigned int i = 0; i < tx.vin.size(); i++) {
        const CTxIn& txin = tx.vin[i];
        UniValue in(UniValue::VOBJ);
        if (tx.IsCoinBase()) {
            in.pushKV("coinbase", HexStr(txin.scriptSig));
        } else {
            in.pushKV("txid", txin.prevout.hash.GetHex());
            in.pushKV("vout", (int64_t)txin.prevout.n);
            UniValue o(UniValue::VOBJ);
            o.pushKV("asm", ScriptToAsmStr(txin.scriptSig));
            o.pushKV("hex", HexStr(txin.scriptSig));
            in.pushKV("scriptSig", std::move(o));
        }
        if (!tx.vin[i].scriptWitness.IsNull()) {
            UniValue txinwitness(UniValue::VARR);
            for (const auto& item : tx.vin[i].scriptWitness.stack) {
                txinwitness.push_back(HexStr(item));
            }
            in.pushKV("txinwitness", std::move(txinwitness));
        }
        if (have_undo) {
            const Coin& prev_coin = txundo->vprevout[i];
            const CTxOut& prev_txout = prev_coin.out;

            amt_total_in += prev_txout.nValue;

            if (verbosity == TxVerbosity::SHOW_DETAILS_AND_PREVOUT) {
                UniValue o_script_pub_key(UniValue::VOBJ);
                ScriptToUniv(prev_txout.scriptPubKey, /*out=*/o_script_pub_key, /*include_hex=*/true, /*include_address=*/true);

                UniValue p(UniValue::VOBJ);
                p.pushKV("generated", bool(prev_coin.fCoinBase));
                p.pushKV("height", uint64_t(prev_coin.nHeight));
                p.pushKV("value", ValueFromAmount(prev_txout.nValue));
                p.pushKV("scriptPubKey", std::move(o_script_pub_key));
                in.pushKV("prevout", std::move(p));
            }
        }
        in.pushKV("sequence", (int64_t)txin.nSequence);
        vin.push_back(std::move(in));
    }
    entry.pushKV("vin", std::move(vin));

    UniValue vout(UniValue::VARR);
    for (unsigned int i = 0; i < tx.vout.size(); i++) {
        const CTxOut& txout = tx.vout[i];

        UniValue out(UniValue::VOBJ);

        out.pushKV("value", ValueFromAmount(txout.nValue));
        out.pushKV("n", (int64_t)i);

        UniValue o(UniValue::VOBJ);
        ScriptToUniv(txout.scriptPubKey, /*out=*/o, /*include_hex=*/true, /*include_address=*/true);
        out.pushKV("scriptPubKey", std::move(o));
        vout.push_back(std::move(out));

        if (have_undo) {
            amt_total_out += txout.nValue;
        }
    }
    entry.pushKV("vout", std::move(vout));

    if (have_undo) {
        const CAmount fee = amt_total_in - amt_total_out;
        CHECK_NONFATAL(MoneyRange(fee));
        entry.pushKV("fee", ValueFromAmount(fee));
    }

    if (!block_hash.IsNull()) {
        entry.pushKV("blockhash", block_hash.GetHex());
    }

    if (include_hex) {
        entry.pushKV("hex", EncodeHexTx(tx)); // The hex-encoded transaction. Used the name "hex" to be consistent with the verbose output of "getrawtransaction".
    }
}
