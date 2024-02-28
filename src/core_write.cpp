// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <core_io.h>

#include <consensus/validation.h>
#include <key_io.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <script/standard.h>
#include <serialize.h>
#include <streams.h>
#include <univalue.h>
#include <util/strencodings.h>

#include <addressindex.h>
#include <spentindex.h>

#include <evo/assetlocktx.h>
#include <evo/cbtx.h>
#include <evo/mnhftx.h>
#include <evo/providertx.h>
#include <evo/specialtx.h>
#include <llmq/commitment.h>

UniValue ValueFromAmount(const CAmount& amount)
{
    bool sign = amount < 0;
    int64_t n_abs = (sign ? -amount : amount);
    int64_t quotient = n_abs / COIN;
    int64_t remainder = n_abs % COIN;
    return UniValue(UniValue::VNUM,
            strprintf("%s%d.%08d", sign ? "-" : "", quotient, remainder));
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
                std::string str(GetOpName(op));
                if (str.substr(0, 3) == std::string("OP_")) {
                    ret += str.substr(3, std::string::npos) + " ";
                    continue;
                }
            }
            if (vch.size() > 0) {
                ret += strprintf("0x%x 0x%x ", HexStr(std::vector<uint8_t>(it2, it - vch.size())),
                                               HexStr(std::vector<uint8_t>(it - vch.size(), it)));
            } else {
                ret += strprintf("0x%x ", HexStr(std::vector<uint8_t>(it2, it)));
            }
            continue;
        }
        ret += strprintf("0x%x ", HexStr(std::vector<uint8_t>(it2, script.end())));
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

/**
 * Create the assembly string representation of a CScript object.
 * @param[in] script    CScript object to convert into the asm string representation.
 * @param[in] fAttemptSighashDecode    Whether to attempt to decode sighash types on data within the script that matches the format
 *                                     of a signature. Only pass true for scripts you believe could contain signatures. For example,
 *                                     pass false, or omit the this argument (defaults to false), for scriptPubKeys.
 */
std::string ScriptToAsmStr(const CScript& script, const bool fAttemptSighashDecode)
{
    std::string str;
    opcodetype opcode;
    std::vector<unsigned char> vch;
    CScript::const_iterator pc = script.begin();
    while (pc < script.end()) {
        if (!str.empty()) {
            str += " ";
        }
        if (!script.GetOp(pc, opcode, vch)) {
            str += "[error]";
            return str;
        }
        if (0 <= opcode && opcode <= OP_PUSHDATA4) {
            if (vch.size() <= static_cast<std::vector<unsigned char>::size_type>(4)) {
                str += strprintf("%d", CScriptNum(vch, false).getint());
            } else {
                // the IsUnspendable check makes sure not to try to decode OP_RETURN data that may match the format of a signature
                if (fAttemptSighashDecode && !script.IsUnspendable()) {
                    std::string strSigHashDecode;
                    // goal: only attempt to decode a defined sighash type from data that looks like a signature within a scriptSig.
                    // this won't decode correctly formatted public keys in Pubkey or Multisig scripts due to
                    // the restrictions on the pubkey formats (see IsCompressedOrUncompressedPubKey) being incongruous with the
                    // checks in CheckSignatureEncoding.
                    if (CheckSignatureEncoding(vch, SCRIPT_VERIFY_STRICTENC, nullptr)) {
                        const unsigned char chSigHashType = vch.back();
                        const auto it = mapSigHashTypes.find(chSigHashType);
                        if (it != mapSigHashTypes.end()) {
                            strSigHashDecode = "[" + it->second + "]";
                            vch.pop_back(); // remove the sighash type byte. it will be replaced by the decode.
                        }
                    }
                    str += HexStr(vch) + strSigHashDecode;
                } else {
                    str += HexStr(vch);
                }
            }
        } else {
            str += GetOpName(opcode);
        }
    }
    return str;
}

std::string EncodeHexTx(const CTransaction& tx)
{
    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
    ssTx << tx;
    return HexStr(ssTx);
}

void ScriptToUniv(const CScript& script, UniValue& out, bool include_address)
{
    out.pushKV("asm", ScriptToAsmStr(script));
    out.pushKV("hex", HexStr(script));

    std::vector<std::vector<unsigned char>> solns;
    TxoutType type = Solver(script, solns);
    out.pushKV("type", GetTxnOutputType(type));

    CTxDestination address;
    if (include_address && ExtractDestination(script, address) && type != TxoutType::PUBKEY) {
        out.pushKV("address", EncodeDestination(address));
    }
}

void ScriptPubKeyToUniv(const CScript& scriptPubKey,
                        UniValue& out, bool fIncludeHex)
{
    TxoutType type;
    std::vector<CTxDestination> addresses;
    int nRequired;

    out.pushKV("asm", ScriptToAsmStr(scriptPubKey));
    if (fIncludeHex)
        out.pushKV("hex", HexStr(scriptPubKey));

    if (!ExtractDestinations(scriptPubKey, type, addresses, nRequired) || type == TxoutType::PUBKEY) {
        out.pushKV("type", GetTxnOutputType(type));
        return;
    }

    out.pushKV("reqSigs", nRequired);
    out.pushKV("type", GetTxnOutputType(type));

    UniValue a(UniValue::VARR);
    for (const CTxDestination& addr : addresses) {
        a.push_back(EncodeDestination(addr));
    }
    out.pushKV("addresses", a);
}

void TxToUniv(const CTransaction& tx, const uint256& hashBlock, UniValue& entry, bool include_hex, const CSpentIndexTxInfo* ptxSpentInfo)
{
    uint256 txid = tx.GetHash();
    entry.pushKV("txid", txid.GetHex());
    // Transaction version is actually unsigned in consensus checks, just signed in memory,
    // so cast to unsigned before giving it to the user.
    entry.pushKV("version", static_cast<int64_t>(static_cast<uint16_t>(tx.nVersion)));
    entry.pushKV("type", tx.nType);
    entry.pushKV("size", (int)::GetSerializeSize(tx, PROTOCOL_VERSION));
    entry.pushKV("locktime", (int64_t)tx.nLockTime);

    UniValue vin(UniValue::VARR);
    for (const CTxIn& txin : tx.vin) {
        UniValue in(UniValue::VOBJ);
        if (tx.IsCoinBase())
            in.pushKV("coinbase", HexStr(txin.scriptSig));
        else {
            in.pushKV("txid", txin.prevout.hash.GetHex());
            in.pushKV("vout", (int64_t)txin.prevout.n);
            UniValue o(UniValue::VOBJ);
            o.pushKV("asm", ScriptToAsmStr(txin.scriptSig, true));
            o.pushKV("hex", HexStr(txin.scriptSig));
            in.pushKV("scriptSig", o);

            // Add address and value info if spentindex enabled
            if (ptxSpentInfo != nullptr) {
                CSpentIndexKey spentKey(txin.prevout.hash, txin.prevout.n);
                auto it = ptxSpentInfo->mSpentInfo.find(spentKey);
                if (it != ptxSpentInfo->mSpentInfo.end()) {
                    auto spentInfo = it->second;
                    in.pushKV("value", ValueFromAmount(spentInfo.m_amount));
                    in.pushKV("valueSat", spentInfo.m_amount);
                    if (spentInfo.m_address_type == AddressType::P2PK_OR_P2PKH) {
                        in.pushKV("address", EncodeDestination(PKHash(spentInfo.m_address_bytes)));
                    } else if (spentInfo.m_address_type == AddressType::P2SH) {
                        in.pushKV("address", EncodeDestination(ScriptHash(spentInfo.m_address_bytes)));
                    }
                }
            }
        }
        in.pushKV("sequence", (int64_t)txin.nSequence);
        vin.push_back(in);
    }
    entry.pushKV("vin", vin);

    UniValue vout(UniValue::VARR);
    for (unsigned int i = 0; i < tx.vout.size(); i++) {
        const CTxOut& txout = tx.vout[i];

        UniValue out(UniValue::VOBJ);

        out.pushKV("value", ValueFromAmount(txout.nValue));
        out.pushKV("valueSat", txout.nValue);
        out.pushKV("n", (int64_t)i);

        UniValue o(UniValue::VOBJ);
        ScriptPubKeyToUniv(txout.scriptPubKey, o, true);
        out.pushKV("scriptPubKey", o);

        // Add spent information if spentindex is enabled
        if (ptxSpentInfo != nullptr) {
            CSpentIndexKey spentKey(txid, i);
            auto it = ptxSpentInfo->mSpentInfo.find(spentKey);
            if (it != ptxSpentInfo->mSpentInfo.end()) {
                auto spentInfo = it->second;
                out.pushKV("spentTxId", spentInfo.m_tx_hash.GetHex());
                out.pushKV("spentIndex", (int)spentInfo.m_tx_index);
                out.pushKV("spentHeight", spentInfo.m_block_height);
            }
        }
        vout.push_back(out);
    }
    entry.pushKV("vout", vout);

    if (!tx.vExtraPayload.empty()) {
        entry.pushKV("extraPayloadSize", (int)tx.vExtraPayload.size());
        entry.pushKV("extraPayload", HexStr(tx.vExtraPayload));
    }

    if (tx.nType == TRANSACTION_PROVIDER_REGISTER) {
        if (const auto opt_proTx = GetTxPayload<CProRegTx>(tx)) {
            entry.pushKV("proRegTx", opt_proTx->ToJson());
        }
    } else if (tx.nType == TRANSACTION_PROVIDER_UPDATE_SERVICE) {
        if (const auto opt_proTx = GetTxPayload<CProUpServTx>(tx)) {
            entry.pushKV("proUpServTx", opt_proTx->ToJson());
        }
    } else if (tx.nType == TRANSACTION_PROVIDER_UPDATE_REGISTRAR) {
        if (const auto opt_proTx = GetTxPayload<CProUpRegTx>(tx)) {
            entry.pushKV("proUpRegTx", opt_proTx->ToJson());
        }
    } else if (tx.nType == TRANSACTION_PROVIDER_UPDATE_REVOKE) {
        if (const auto opt_proTx = GetTxPayload<CProUpRevTx>(tx)) {
            entry.pushKV("proUpRevTx", opt_proTx->ToJson());
        }
    } else if (tx.nType == TRANSACTION_COINBASE) {
        if (const auto opt_cbTx = GetTxPayload<CCbTx>(tx)) {
            entry.pushKV("cbTx", opt_cbTx->ToJson());
        }
    } else if (tx.nType == TRANSACTION_QUORUM_COMMITMENT) {
        if (const auto opt_qcTx = GetTxPayload<llmq::CFinalCommitmentTxPayload>(tx)) {
            entry.pushKV("qcTx", opt_qcTx->ToJson());
        }
    } else if (tx.nType == TRANSACTION_MNHF_SIGNAL) {
        if (const auto opt_mnhfTx = GetTxPayload<MNHFTxPayload>(tx)) {
            entry.pushKV("mnhfTx", opt_mnhfTx->ToJson());
        }
    } else if (tx.nType == TRANSACTION_ASSET_LOCK) {
        if (const auto opt_assetLockTx = GetTxPayload<CAssetLockPayload>(tx)) {
            entry.pushKV("assetLockTx", opt_assetLockTx->ToJson());
        }
    } else if (tx.nType == TRANSACTION_ASSET_UNLOCK) {
        if (const auto opt_assetUnlockTx = GetTxPayload<CAssetUnlockPayload>(tx)) {
            entry.pushKV("assetUnlockTx", opt_assetUnlockTx->ToJson());
        }
    }

    if (!hashBlock.IsNull())
        entry.pushKV("blockhash", hashBlock.GetHex());

    if (include_hex) {
        entry.pushKV("hex", EncodeHexTx(tx)); // The hex-encoded transaction. Used the name "hex" to be consistent with the verbose output of "getrawtransaction".
    }
}
