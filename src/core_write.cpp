// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <core_io.h>

#include <consensus/consensus.h>
#include <consensus/validation.h>
#include <key_io.h>
#include <script/script.h>
#include <script/standard.h>
#include <serialize.h>
#include <streams.h>
#include <univalue.h>
#include <util.h>
#include <utilmoneystr.h>
#include <utilstrencodings.h>
#include <spentindex.h>
#include <blind.h>

//extern bool GetSpentIndex(CSpentIndexKey &key, CSpentIndexValue &value);
bool (*pCoreWriteGetSpentIndex)(CSpentIndexKey &key, CSpentIndexValue &value) = nullptr; // HACK, alternative is to move GetSpentIndex into common lib

void SetCoreWriteGetSpentIndex(bool (*function)(CSpentIndexKey&, CSpentIndexValue&))
{
    pCoreWriteGetSpentIndex = function;
};

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
                ret += strprintf("0x%x 0x%x ", HexStr(it2, it - vch.size()), HexStr(it - vch.size(), it));
            } else {
                ret += strprintf("0x%x ", HexStr(it2, it));
            }
            continue;
        }
        ret += strprintf("0x%x ", HexStr(it2, script.end()));
        break;
    }
    return ret.substr(0, ret.size() - 1);
}

const std::map<unsigned char, std::string> mapSigHashTypes = {
    {static_cast<unsigned char>(SIGHASH_ALL), std::string("ALL")},
    {static_cast<unsigned char>(SIGHASH_ALL|SIGHASH_ANYONECANPAY), std::string("ALL|ANYONECANPAY")},
    {static_cast<unsigned char>(SIGHASH_NONE), std::string("NONE")},
    {static_cast<unsigned char>(SIGHASH_NONE|SIGHASH_ANYONECANPAY), std::string("NONE|ANYONECANPAY")},
    {static_cast<unsigned char>(SIGHASH_SINGLE), std::string("SINGLE")},
    {static_cast<unsigned char>(SIGHASH_SINGLE|SIGHASH_ANYONECANPAY), std::string("SINGLE|ANYONECANPAY")},
};

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
                        if (mapSigHashTypes.count(chSigHashType)) {
                            strSigHashDecode = "[" + mapSigHashTypes.find(chSigHashType)->second + "]";
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

std::string EncodeHexTx(const CTransaction& tx, const int serializeFlags)
{
    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION | serializeFlags);
    ssTx << tx;
    return HexStr(ssTx.begin(), ssTx.end());
}

void ScriptPubKeyToUniv(const CScript& scriptPubKey,
                        UniValue& out, bool fIncludeHex)
{
    txnouttype type;
    std::vector<CTxDestination> addresses;
    int nRequired;

    out.pushKV("asm", ScriptToAsmStr(scriptPubKey));
    if (fIncludeHex)
        out.pushKV("hex", HexStr(scriptPubKey.begin(), scriptPubKey.end()));

    if (!ExtractDestinations(scriptPubKey, type, addresses, nRequired)) {
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

    if (HasIsCoinstakeOp(scriptPubKey))
    {
        CScript scriptCS;
        if (GetCoinstakeScriptPath(scriptPubKey, scriptCS))
        if (ExtractDestinations(scriptCS, type, addresses, nRequired)) {
            UniValue a(UniValue::VARR);
            for (const CTxDestination& addr : addresses) {
                a.push_back(EncodeDestination(addr));
            };
            out.pushKV("stakeaddresses", a);
        };
    };
}

void AddRangeproof(const std::vector<uint8_t> &vRangeproof, UniValue &entry)
{
    entry.push_back(Pair("rangeproof", HexStr(vRangeproof.begin(), vRangeproof.end())));

    if (vRangeproof.size() > 0)
    {
        int exponent, mantissa;
        CAmount min_value, max_value;
        if (0 == GetRangeProofInfo(vRangeproof, exponent, mantissa, min_value, max_value))
        {
            entry.push_back(Pair("rp_exponent", exponent));
            entry.push_back(Pair("rp_mantissa", mantissa));
            entry.push_back(Pair("rp_min_value", ValueFromAmount(min_value)));
            entry.push_back(Pair("rp_max_value", ValueFromAmount(max_value)));
        };
    };
}

void OutputToJSON(uint256 &txid, int i,
    const CTxOutBase *baseOut, UniValue &entry)
{
    bool fCanSpend = false;
    switch (baseOut->GetType())
    {
        case OUTPUT_STANDARD:
            {
            fCanSpend = true;
            entry.push_back(Pair("type", "standard"));
            CTxOutStandard *s = (CTxOutStandard*) baseOut;
            entry.push_back(Pair("value", ValueFromAmount(s->nValue)));
            entry.push_back(Pair("valueSat", s->nValue));
            UniValue o(UniValue::VOBJ);
            ScriptPubKeyToUniv(s->scriptPubKey, o, true);
            entry.push_back(Pair("scriptPubKey", o));
            }
            break;
        case OUTPUT_DATA:
            {
            CTxOutData *s = (CTxOutData*) baseOut;
            entry.push_back(Pair("type", "data"));
            entry.push_back(Pair("data_hex", HexStr(s->vData.begin(), s->vData.end())));
            CAmount nValue;
            if (s->GetCTFee(nValue))
                entry.push_back(Pair("ct_fee", ValueFromAmount(nValue)));
            if (s->GetDevFundCfwd(nValue))
                entry.push_back(Pair("dev_fund_cfwd", ValueFromAmount(nValue)));
            }
            break;
        case OUTPUT_CT:
            {
            fCanSpend = true;
            CTxOutCT *s = (CTxOutCT*) baseOut;
            entry.push_back(Pair("type", "blind"));
            entry.push_back(Pair("valueCommitment", HexStr(&s->commitment.data[0], &s->commitment.data[0]+33)));
            UniValue o(UniValue::VOBJ);
            ScriptPubKeyToUniv(s->scriptPubKey, o, true);
            entry.push_back(Pair("scriptPubKey", o));
            entry.push_back(Pair("data_hex", HexStr(s->vData.begin(), s->vData.end())));

            AddRangeproof(s->vRangeproof, entry);
            }
            break;
        case OUTPUT_RINGCT:
            {
            CTxOutRingCT *s = (CTxOutRingCT*) baseOut;
            entry.push_back(Pair("type", "anon"));
            entry.push_back(Pair("pubkey", HexStr(s->pk.begin(), s->pk.end())));
            entry.push_back(Pair("valueCommitment", HexStr(&s->commitment.data[0], &s->commitment.data[0]+33)));
            entry.push_back(Pair("data_hex", HexStr(s->vData.begin(), s->vData.end())));

            AddRangeproof(s->vRangeproof, entry);
            }
            break;
        default:
            entry.push_back(Pair("type", "unknown"));
            break;
    };

    if (fCanSpend)
    {
        // Add spent information if spentindex is enabled
        CSpentIndexValue spentInfo;
        CSpentIndexKey spentKey(txid, i);
        if (pCoreWriteGetSpentIndex && pCoreWriteGetSpentIndex(spentKey, spentInfo))
        {
            entry.push_back(Pair("spentTxId", spentInfo.txid.GetHex()));
            entry.push_back(Pair("spentIndex", (int)spentInfo.inputIndex));
            entry.push_back(Pair("spentHeight", spentInfo.blockHeight));
        };
    };
};

void TxToUniv(const CTransaction& tx, const uint256& hashBlock, UniValue& entry, bool include_hex, int serialize_flags)
{
    uint256 txid = tx.GetHash();
    entry.pushKV("txid", txid.GetHex());
    entry.pushKV("hash", tx.GetWitnessHash().GetHex());
    entry.pushKV("version", tx.nVersion);
    entry.pushKV("size", (int)::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION));
    entry.pushKV("vsize", (GetTransactionWeight(tx) + WITNESS_SCALE_FACTOR - 1) / WITNESS_SCALE_FACTOR);
    entry.pushKV("weight", GetTransactionWeight(tx));
    entry.pushKV("locktime", (int64_t)tx.nLockTime);

    UniValue vin(UniValue::VARR);
    for (unsigned int i = 0; i < tx.vin.size(); i++) {
        const CTxIn& txin = tx.vin[i];
        UniValue in(UniValue::VOBJ);
        if (tx.IsCoinBase())
            in.pushKV("coinbase", HexStr(txin.scriptSig.begin(), txin.scriptSig.end()));
          {
            if (txin.IsAnonInput())
            {
                in.push_back(Pair("type", "anon"));
                uint32_t nSigInputs, nSigRingSize;
                txin.GetAnonInfo(nSigInputs, nSigRingSize);
                in.push_back(Pair("num_inputs", (int)nSigInputs));
                in.push_back(Pair("ring_size", (int)nSigRingSize));
            } else{
            in.pushKV("txid", txin.prevout.hash.GetHex());
            in.pushKV("vout", (int64_t)txin.prevout.n);
            UniValue o(UniValue::VOBJ);
            o.pushKV("asm", ScriptToAsmStr(txin.scriptSig, true));
            o.pushKV("hex", HexStr(txin.scriptSig.begin(), txin.scriptSig.end()));
            in.pushKV("scriptSig", o);
            }
            if (!tx.vin[i].scriptWitness.IsNull()) {
                UniValue txinwitness(UniValue::VARR);
                for (const auto& item : tx.vin[i].scriptWitness.stack) {
                    txinwitness.push_back(HexStr(item.begin(), item.end()));
                }
                in.pushKV("txinwitness", txinwitness);
            }
        }
        in.pushKV("sequence", (int64_t)txin.nSequence);
        vin.push_back(in);
    }
    entry.pushKV("vin", vin);

    UniValue vout(UniValue::VARR);
    for (unsigned int i = 0; i < tx.vpout.size(); i++)
    {
        UniValue out(UniValue::VOBJ);
        out.push_back(Pair("n", (int64_t)i));
        OutputToJSON(txid, i, tx.vpout[i].get(), out);
        vout.push_back(out);
    }

    if (!tx.IsParticlVersion())
    for (unsigned int i = 0; i < tx.vout.size(); i++) {
        const CTxOut& txout = tx.vout[i];

        UniValue out(UniValue::VOBJ);

        out.pushKV("value", ValueFromAmount(txout.nValue));
        out.pushKV("n", (int64_t)i);

        UniValue o(UniValue::VOBJ);
        ScriptPubKeyToUniv(txout.scriptPubKey, o, true);
        out.pushKV("scriptPubKey", o);
        vout.push_back(out);
    }

    entry.pushKV("vout", vout);

    if (!hashBlock.IsNull())
        entry.pushKV("blockhash", hashBlock.GetHex());

    if (include_hex) {
        entry.pushKV("hex", EncodeHexTx(tx, serialize_flags)); // The hex-encoded transaction. Used the name "hex" to be consistent with the verbose output of "getrawtransaction".
    }
}

