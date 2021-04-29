// Copyright (c) 2009-2020 The Bitcoin Core developers
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
#include <undo.h>
#include <univalue.h>
#include <util/check.h>
#include <util/system.h>
#include <util/strencodings.h>
// SYSCOIN
#include <services/asset.h>
#include <evo/cbtx.h>
#include <evo/providertx.h>
#include <evo/specialtx.h>
#include <llmq/quorums_commitment.h>
#include <math.h>
bool AssetAllocationTxToJSON(const CTransaction &tx, const uint256& hashBlock, UniValue &entry) {
    const uint256& txHash = tx.GetHash();
    entry.__pushKV("txtype", stringFromSyscoinTx(tx.nVersion));
    entry.__pushKV("txid", txHash.GetHex());
    entry.__pushKV("blockhash", hashBlock.GetHex());  
    UniValue oAssetAllocationReceiversArray(UniValue::VARR);
   
    for(const auto &it: tx.voutAssets) {
        CAmount nTotal = 0;
        UniValue oAssetAllocationReceiversObj(UniValue::VOBJ);
        const uint64_t &nAsset = it.key;
        const uint32_t &nBaseAsset = GetBaseAssetID(nAsset);
        oAssetAllocationReceiversObj.__pushKV("asset_guid", UniValue(nAsset).write());
        if(!it.vchNotarySig.empty()) {
            oAssetAllocationReceiversObj.__pushKV("notary_sig", HexStr(it.vchNotarySig));
        }
        UniValue oAssetAllocationReceiverOutputsArray(UniValue::VARR);
        for(const auto& voutAsset: it.values){
            nTotal += voutAsset.nValue;
            UniValue oAssetAllocationReceiverOutputObj(UniValue::VOBJ);
            oAssetAllocationReceiverOutputObj.__pushKV("n", voutAsset.n);
            oAssetAllocationReceiverOutputObj.__pushKV("amount", ValueFromAmount(voutAsset.nValue, nBaseAsset));
            oAssetAllocationReceiverOutputsArray.push_back(oAssetAllocationReceiverOutputObj);
        }
        oAssetAllocationReceiversObj.__pushKV("outputs", oAssetAllocationReceiverOutputsArray); 
        oAssetAllocationReceiversObj.__pushKV("total", ValueFromAmount(nTotal, nBaseAsset));
        oAssetAllocationReceiversArray.push_back(oAssetAllocationReceiversObj);
    }

    entry.__pushKV("allocations", oAssetAllocationReceiversArray);
    if(tx.nVersion == SYSCOIN_TX_VERSION_ALLOCATION_BURN_TO_ETHEREUM){
         CBurnSyscoin burnSyscoin(tx);
         entry.__pushKV("ethereum_destination", "0x" + HexStr(burnSyscoin.vchEthAddress));
    }
    return true;
}


bool AssetMintTxToJson(const CTransaction& tx, const uint256& txHash, const uint256& hashBlock, UniValue &entry) {
    CMintSyscoin mintSyscoin(tx);
    if (!mintSyscoin.IsNull()) {
        entry.__pushKV("txtype", "assetallocationmint");
        entry.__pushKV("txid", txHash.GetHex());
        entry.__pushKV("blockhash", hashBlock.GetHex());  
        UniValue oSPVProofObj(UniValue::VOBJ);
        oSPVProofObj.__pushKV("bridgetransferid", mintSyscoin.nBridgeTransferID);  
        oSPVProofObj.__pushKV("postx", mintSyscoin.posTx);
        oSPVProofObj.__pushKV("txroot", HexStr(mintSyscoin.vchTxRoot)); 
        oSPVProofObj.__pushKV("txparentnodes", HexStr(mintSyscoin.vchTxParentNodes)); 
        oSPVProofObj.__pushKV("txpath", HexStr(mintSyscoin.vchTxPath)); 
        oSPVProofObj.__pushKV("posReceipt", mintSyscoin.posReceipt);  
        oSPVProofObj.__pushKV("receiptroot", HexStr(mintSyscoin.vchReceiptRoot));  
        oSPVProofObj.__pushKV("receiptparentnodes", HexStr(mintSyscoin.vchReceiptParentNodes));  
        oSPVProofObj.__pushKV("ethblocknumber", mintSyscoin.nBlockNumber); 
        entry.__pushKV("spv_proof", oSPVProofObj);
        UniValue oAssetAllocationReceiversArray(UniValue::VARR);
        for(const auto &it: mintSyscoin.voutAssets) {
            CAmount nTotal = 0;
            UniValue oAssetAllocationReceiversObj(UniValue::VOBJ);
            const uint64_t &nAsset = it.key;
            const uint32_t &nBaseAsset = GetBaseAssetID(nAsset);
            oAssetAllocationReceiversObj.__pushKV("asset_guid", UniValue(nAsset).write());
            UniValue oAssetAllocationReceiverOutputsArray(UniValue::VARR);
            for(const auto& voutAsset: it.values){
                nTotal += voutAsset.nValue;
                UniValue oAssetAllocationReceiverOutputObj(UniValue::VOBJ);
                oAssetAllocationReceiverOutputObj.__pushKV("n", voutAsset.n);
                oAssetAllocationReceiverOutputObj.__pushKV("amount", ValueFromAmount(voutAsset.nValue, nBaseAsset));
                oAssetAllocationReceiverOutputsArray.push_back(oAssetAllocationReceiverOutputObj);
            }
            oAssetAllocationReceiversObj.__pushKV("outputs", oAssetAllocationReceiverOutputsArray); 
            oAssetAllocationReceiversObj.__pushKV("total", ValueFromAmount(nTotal, nBaseAsset));
            oAssetAllocationReceiversArray.push_back(oAssetAllocationReceiversObj);
        }
        entry.__pushKV("allocations", oAssetAllocationReceiversArray);
        return true;
    } 
    return false;
}
bool AssetTxToJSON(const CTransaction& tx, const uint256 &hashBlock, UniValue &entry) {
	CAsset asset(tx);
	if(asset.IsNull())
		return false;
    entry.__pushKV("txtype", stringFromSyscoinTx(tx.nVersion));
    entry.__pushKV("txid", tx.GetHash().GetHex());  
    entry.__pushKV("blockhash", hashBlock.GetHex());
    UniValue oAssetAllocationReceiversArray(UniValue::VARR);
    for(const auto &it: tx.voutAssets) {
        CAmount nTotal = 0;
        UniValue oAssetAllocationReceiversObj(UniValue::VOBJ);
        const uint64_t &nAsset = it.key;
        const uint32_t &nBaseAsset = GetBaseAssetID(nAsset);
        oAssetAllocationReceiversObj.__pushKV("asset_guid", UniValue(nAsset).write());
        UniValue oAssetAllocationReceiverOutputsArray(UniValue::VARR);
        for(const auto& voutAsset: it.values){
            nTotal += voutAsset.nValue;
            UniValue oAssetAllocationReceiverOutputObj(UniValue::VOBJ);
            oAssetAllocationReceiverOutputObj.__pushKV("n", voutAsset.n);
            oAssetAllocationReceiverOutputObj.__pushKV("amount", ValueFromAmount(voutAsset.nValue, nBaseAsset));
            oAssetAllocationReceiverOutputsArray.push_back(oAssetAllocationReceiverOutputObj);
        }
        oAssetAllocationReceiversObj.__pushKV("outputs", oAssetAllocationReceiverOutputsArray); 
        oAssetAllocationReceiversObj.__pushKV("total", ValueFromAmount(nTotal, nBaseAsset));
        oAssetAllocationReceiversArray.push_back(oAssetAllocationReceiversObj);
    }

    entry.__pushKV("allocations", oAssetAllocationReceiversArray); 
    if(asset.nUpdateMask & ASSET_INIT) {
		entry.__pushKV("symbol", DecodeBase64(asset.strSymbol));
        entry.__pushKV("max_supply", ValueFromAmount(asset.nMaxSupply, GetBaseAssetID(tx.voutAssets[0].key)));
		entry.__pushKV("precision", asset.nPrecision);
    }

	if(asset.nUpdateMask & ASSET_UPDATE_DATA) 
		entry.__pushKV("public_value", AssetPublicDataToJson(asset.strPubData));

	if(asset.nUpdateMask & ASSET_UPDATE_CONTRACT) 
		entry.__pushKV("contract", "0x" + HexStr(asset.vchContract));
    
    if(asset.nUpdateMask & ASSET_UPDATE_NOTARY_KEY) 
		entry.__pushKV("notary_address", asset.vchNotaryKeyID.empty()? "": EncodeDestination(WitnessV0KeyHash(uint160{asset.vchNotaryKeyID})));

    if(asset.nUpdateMask & ASSET_UPDATE_NOTARY_DETAILS) {
        UniValue value(UniValue::VOBJ);
        asset.notaryDetails.ToJson(value);
        entry.__pushKV("notary_details", value);
    }

    if(asset.nUpdateMask & ASSET_UPDATE_AUXFEE) {
        UniValue value(UniValue::VOBJ);
        asset.auxFeeDetails.ToJson(value, GetBaseAssetID(tx.voutAssets[0].key));
        entry.__pushKV("auxfee", value);
    }
    
    if(asset.nUpdateMask & ASSET_UPDATE_CAPABILITYFLAGS) 
		entry.__pushKV("updatecapability_flags", asset.nUpdateCapabilityFlags);

    entry.__pushKV("update_flags", asset.nUpdateMask);

    return true;
}
bool SysTxToJSON(const CTransaction& tx, const uint256 &hashBlock, UniValue& output) {
    bool found = false;
    if (IsAssetTx(tx.nVersion) && tx.nVersion != SYSCOIN_TX_VERSION_ASSET_SEND)
        found = AssetTxToJSON(tx, hashBlock, output);
    else if (IsAssetAllocationTx(tx.nVersion) || tx.nVersion == SYSCOIN_TX_VERSION_ASSET_SEND)
        found = AssetAllocationTxToJSON(tx, hashBlock, output);
    return found;
}

bool DecodeSyscoinRawtransaction(const CTransaction& rawTx, const uint256 &hashBlock, UniValue& output) {
    bool found = false;
    if(IsSyscoinMintTx(rawTx.nVersion)) {
        found = AssetMintTxToJson(rawTx, rawTx.GetHash(), hashBlock, output);
    }
    else if (IsAssetTx(rawTx.nVersion) || IsAssetAllocationTx(rawTx.nVersion)) {
        found = SysTxToJSON(rawTx, hashBlock, output);
    }
    
    return found;
}
// SYSCOIN
UniValue ValueFromAmount(const CAmount amount, const uint32_t &nBaseAsset)
{
    uint8_t nPrecision = 8;
    if(nBaseAsset > 0) {
        if(!GetAssetPrecision(nBaseAsset, nPrecision)) {
            nPrecision = 0;
        }
    }
    int64_t quotient = amount;
    int64_t divByAmount = 1;
    int64_t remainder = 0;
    std::string strPrecision = "0";
    if (nPrecision > 0) {
        divByAmount = powf(10.0, nPrecision);
        quotient = amount / divByAmount;
        remainder = amount % divByAmount;
        strPrecision = itostr(nPrecision);
    }
    if (amount < 0) {
        quotient = -quotient;
        remainder = -remainder;
    }

    return UniValue(UniValue::VNUM,
        strprintf("%s%d.%0" + strPrecision + "d", amount < 0 ? "-" : "", quotient, remainder));
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

std::string EncodeHexTx(const CTransaction& tx, const int serializeFlags)
{
    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION | serializeFlags);
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

// TODO: from v23 ("addresses" and "reqSigs" deprecated) this method should be refactored to remove the `include_addresses` option
// this method can also be combined with `ScriptToUniv` as they will overlap
void ScriptPubKeyToUniv(const CScript& scriptPubKey,
                        UniValue& out, bool fIncludeHex, bool include_addresses)
{
    TxoutType type;
    CTxDestination address;
    std::vector<CTxDestination> addresses;
    int nRequired;

    out.pushKV("asm", ScriptToAsmStr(scriptPubKey));
    if (fIncludeHex)
        out.pushKV("hex", HexStr(scriptPubKey));

    if (!ExtractDestinations(scriptPubKey, type, addresses, nRequired) || type == TxoutType::PUBKEY) {
        out.pushKV("type", GetTxnOutputType(type));
        return;
    }

    if (ExtractDestination(scriptPubKey, address)) {
        out.pushKV("address", EncodeDestination(address));
    }
    out.pushKV("type", GetTxnOutputType(type));

    if (include_addresses) {
        UniValue a(UniValue::VARR);
        for (const CTxDestination& addr : addresses) {
            a.push_back(EncodeDestination(addr));
        }
        out.pushKV("addresses", a);
        out.pushKV("reqSigs", nRequired);
    }
}

void TxToUniv(const CTransaction& tx, const uint256& hashBlock, bool include_addresses, UniValue& entry, bool include_hex, int serialize_flags, const CTxUndo* txundo)
{
    entry.pushKV("txid", tx.GetHash().GetHex());
    entry.pushKV("hash", tx.GetWitnessHash().GetHex());
    // Transaction version is actually unsigned in consensus checks, just signed in memory,
    // so cast to unsigned before giving it to the user.
    entry.pushKV("version", static_cast<int64_t>(static_cast<uint32_t>(tx.nVersion)));
    entry.pushKV("size", (int)::GetSerializeSize(tx, PROTOCOL_VERSION));
    entry.pushKV("vsize", (GetTransactionWeight(tx) + WITNESS_SCALE_FACTOR - 1) / WITNESS_SCALE_FACTOR);
    entry.pushKV("weight", GetTransactionWeight(tx));
    entry.pushKV("locktime", (int64_t)tx.nLockTime);

    UniValue vin{UniValue::VARR};

    // If available, use Undo data to calculate the fee. Note that txundo == nullptr
    // for coinbase transactions and for transactions where undo data is unavailable.
    const bool calculate_fee = txundo != nullptr;
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
            o.pushKV("asm", ScriptToAsmStr(txin.scriptSig, true));
            o.pushKV("hex", HexStr(txin.scriptSig));
            in.pushKV("scriptSig", o);
        }
        if (!tx.vin[i].scriptWitness.IsNull()) {
            UniValue txinwitness(UniValue::VARR);
            for (const auto& item : tx.vin[i].scriptWitness.stack) {
                txinwitness.push_back(HexStr(item));
            }
            in.pushKV("txinwitness", txinwitness);
        }
        if (calculate_fee) {
            const CTxOut& prev_txout = txundo->vprevout[i].out;
            amt_total_in += prev_txout.nValue;
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
        out.pushKV("n", (int64_t)i);

        UniValue o(UniValue::VOBJ);
        ScriptPubKeyToUniv(txout.scriptPubKey, o, true, include_addresses);
        out.pushKV("scriptPubKey", o);
        vout.push_back(out);

        if (calculate_fee) {
            amt_total_out += txout.nValue;
        }
    }
    entry.pushKV("vout", vout);
    // SYSCOIN
    if (tx.nVersion == SYSCOIN_TX_VERSION_MN_REGISTER) {
        CProRegTx proTx;
        if (GetTxPayload(tx, proTx)) {
            UniValue obj;
            proTx.ToJson(obj);
            entry.pushKV("proRegTx", obj);
        }
    } else if (tx.nVersion == SYSCOIN_TX_VERSION_MN_UPDATE_SERVICE) {
        CProUpServTx proTx;
        if (GetTxPayload(tx, proTx)) {
            UniValue obj;
            proTx.ToJson(obj);
            entry.pushKV("proUpServTx", obj);
        }
    } else if (tx.nVersion == SYSCOIN_TX_VERSION_MN_UPDATE_REGISTRAR) {
        CProUpRegTx proTx;
        if (GetTxPayload(tx, proTx)) {
            UniValue obj;
            proTx.ToJson(obj);
            entry.pushKV("proUpRegTx", obj);
        }
    } else if (tx.nVersion == SYSCOIN_TX_VERSION_MN_UPDATE_REVOKE) {
        CProUpRevTx proTx;
        if (GetTxPayload(tx, proTx)) {
            UniValue obj;
            proTx.ToJson(obj);
            entry.pushKV("proUpRevTx", obj);
        }
    } else if (tx.nVersion == SYSCOIN_TX_VERSION_MN_COINBASE) {
        CCbTx cbTx;
        if (GetTxPayload(tx, cbTx)) {
            UniValue obj;
            cbTx.ToJson(obj);
            entry.pushKV("cbTx", obj);
        }
    } else if (tx.nVersion == SYSCOIN_TX_VERSION_MN_QUORUM_COMMITMENT) {
        llmq::CFinalCommitmentTxPayload qcTx;
        if (GetTxPayload(tx, qcTx)) {
            UniValue obj;
            UniValue cbObj;
            qcTx.cbTx.ToJson(cbObj);
            entry.pushKV("cbTx", cbObj);
            qcTx.ToJson(obj);
            entry.pushKV("qcTx", obj);
        }
    }
    UniValue output(UniValue::VOBJ);
    if(DecodeSyscoinRawtransaction(tx, hashBlock, output))
        entry.pushKV("systx", output);

    if (calculate_fee) {
        const CAmount fee = amt_total_in - amt_total_out;
        CHECK_NONFATAL(MoneyRange(fee));
        entry.pushKV("fee", ValueFromAmount(fee));
    }

    if (!hashBlock.IsNull())
        entry.pushKV("blockhash", hashBlock.GetHex());

    if (include_hex) {
        entry.pushKV("hex", EncodeHexTx(tx, serialize_flags)); // The hex-encoded transaction. Used the name "hex" to be consistent with the verbose output of "getrawtransaction".
    }
}
