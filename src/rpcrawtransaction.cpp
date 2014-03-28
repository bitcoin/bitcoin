// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/assign/list_of.hpp>

#include "base58.h"
#include "bitcoinrpc.h"
#include "db.h"
#include "init.h"
#include "main.h"
#include "net.h"
#include "wallet.h"

using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace json_spirit;

//
// Utilities: convert hex-encoded Values
// (throws error if not hex).
//
uint256 ParseHashV(const Value& v, string strName)
{
    string strHex;
    if (v.type() == str_type)
        strHex = v.get_str();
    if (!IsHex(strHex)) // Note: IsHex("") is false
        throw JSONRPCError(RPC_INVALID_PARAMETER, strName+" must be hexadecimal string (not '"+strHex+"')");
    uint256 result;
    result.SetHex(strHex);
    return result;
}
uint256 ParseHashO(const Object& o, string strKey)
{
    return ParseHashV(find_value(o, strKey), strKey);
}
vector<unsigned char> ParseHexV(const Value& v, string strName)
{
    string strHex;
    if (v.type() == str_type)
        strHex = v.get_str();
    if (!IsHex(strHex))
        throw JSONRPCError(RPC_INVALID_PARAMETER, strName+" must be hexadecimal string (not '"+strHex+"')");
    return ParseHex(strHex);
}
vector<unsigned char> ParseHexO(const Object& o, string strKey)
{
    return ParseHexV(find_value(o, strKey), strKey);
}

void ScriptPubKeyToJSON(const CScript& scriptPubKey, Object& out)
{
    txnouttype type;
    vector<CTxDestination> addresses;
    int nRequired;

    out.push_back(Pair("asm", scriptPubKey.ToString()));
    out.push_back(Pair("hex", HexStr(scriptPubKey.begin(), scriptPubKey.end())));

    if (!ExtractDestinations(scriptPubKey, type, addresses, nRequired))
    {
        out.push_back(Pair("type", GetTxnOutputType(TX_NONSTANDARD)));
        return;
    }

    out.push_back(Pair("reqSigs", nRequired));
    out.push_back(Pair("type", GetTxnOutputType(type)));

    Array a;
    BOOST_FOREACH(const CTxDestination& addr, addresses)
        a.push_back(CBitcoinAddress(addr).ToString());
    out.push_back(Pair("addresses", a));
}

void TxToJSON(const CTransaction& tx, const uint256 hashBlock, Object& entry)
{
    entry.push_back(Pair("txid", tx.GetHash().GetHex()));
    entry.push_back(Pair("version", tx.nVersion));
    entry.push_back(Pair("locktime", (boost::int64_t)tx.nLockTime));
    Array vin;
    BOOST_FOREACH(const CTxIn& txin, tx.vin)
    {
        Object in;
        if (tx.IsCoinBase())
            in.push_back(Pair("coinbase", HexStr(txin.scriptSig.begin(), txin.scriptSig.end())));
        else
        {
            in.push_back(Pair("txid", txin.prevout.hash.GetHex()));
            in.push_back(Pair("vout", (boost::int64_t)txin.prevout.n));
            Object o;
            o.push_back(Pair("asm", txin.scriptSig.ToString()));
            o.push_back(Pair("hex", HexStr(txin.scriptSig.begin(), txin.scriptSig.end())));
            in.push_back(Pair("scriptSig", o));
        }
        in.push_back(Pair("sequence", (boost::int64_t)txin.nSequence));
        vin.push_back(in);
    }
    entry.push_back(Pair("vin", vin));
    Array vout;
    for (unsigned int i = 0; i < tx.vout.size(); i++)
    {
        const CTxOut& txout = tx.vout[i];
        Object out;
        out.push_back(Pair("value", ValueFromAmount(txout.nValue)));
        out.push_back(Pair("n", (boost::int64_t)i));
        Object o;
        ScriptPubKeyToJSON(txout.scriptPubKey, o);
        out.push_back(Pair("scriptPubKey", o));
        vout.push_back(out);
    }
    entry.push_back(Pair("vout", vout));

    if (hashBlock != 0)
    {
        entry.push_back(Pair("blockhash", hashBlock.GetHex()));
        map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(hashBlock);
        if (mi != mapBlockIndex.end() && (*mi).second)
        {
            CBlockIndex* pindex = (*mi).second;
            if (pindex->IsInMainChain())
            {
                entry.push_back(Pair("confirmations", 1 + nBestHeight - pindex->nHeight));
                entry.push_back(Pair("time", (boost::int64_t)pindex->nTime));
                entry.push_back(Pair("blocktime", (boost::int64_t)pindex->nTime));
            }
            else
                entry.push_back(Pair("confirmations", 0));
        }
    }
}

Value getrawtransaction(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "getrawtransaction <txid> [verbose=0]\n"
            "If verbose=0, returns a string that is\n"
            "serialized, hex-encoded data for <txid>.\n"
            "If verbose is non-zero, returns an Object\n"
            "with information about <txid>.");

    uint256 hash = ParseHashV(params[0], "parameter 1");

    bool fVerbose = false;
    if (params.size() > 1)
        fVerbose = (params[1].get_int() != 0);

    CTransaction tx;
    uint256 hashBlock = 0;
    if (!GetTransaction(hash, tx, hashBlock, true))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available about transaction");

    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
    ssTx << tx;
    string strHex = HexStr(ssTx.begin(), ssTx.end());

    if (!fVerbose)
        return strHex;

    Object result;
    result.push_back(Pair("hex", strHex));
    TxToJSON(tx, hashBlock, result);
    return result;
}

Value listunspent(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 3)
        throw runtime_error(
            "listunspent [minconf=1] [maxconf=9999999]  [\"address\",...]\n"
            "Returns array of unspent transaction outputs\n"
            "with between minconf and maxconf (inclusive) confirmations.\n"
            "Optionally filtered to only include txouts paid to specified addresses.\n"
            "Results are an array of Objects, each of which has:\n"
            "{txid, vout, scriptPubKey, amount, confirmations}");

    RPCTypeCheck(params, list_of(int_type)(int_type)(array_type));

    int nMinDepth = 1;
    if (params.size() > 0)
        nMinDepth = params[0].get_int();

    int nMaxDepth = 9999999;
    if (params.size() > 1)
        nMaxDepth = params[1].get_int();

    set<CBitcoinAddress> setAddress;
    if (params.size() > 2)
    {
        Array inputs = params[2].get_array();
        BOOST_FOREACH(Value& input, inputs)
        {
            CBitcoinAddress address(input.get_str());
            if (!address.IsValid())
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, string("Invalid Litecoin address: ")+input.get_str());
            if (setAddress.count(address))
                throw JSONRPCError(RPC_INVALID_PARAMETER, string("Invalid parameter, duplicated address: ")+input.get_str());
           setAddress.insert(address);
        }
    }

    Array results;
    vector<COutput> vecOutputs;
    assert(pwalletMain != NULL);
    pwalletMain->AvailableCoins(vecOutputs, false);
    BOOST_FOREACH(const COutput& out, vecOutputs)
    {
        if (out.nDepth < nMinDepth || out.nDepth > nMaxDepth)
            continue;

        if (setAddress.size())
        {
            CTxDestination address;
            if (!ExtractDestination(out.tx->vout[out.i].scriptPubKey, address))
                continue;

            if (!setAddress.count(address))
                continue;
        }

        int64 nValue = out.tx->vout[out.i].nValue;
        const CScript& pk = out.tx->vout[out.i].scriptPubKey;
        Object entry;
        entry.push_back(Pair("txid", out.tx->GetHash().GetHex()));
        entry.push_back(Pair("vout", out.i));
        CTxDestination address;
        if (ExtractDestination(out.tx->vout[out.i].scriptPubKey, address))
        {
            entry.push_back(Pair("address", CBitcoinAddress(address).ToString()));
            if (pwalletMain->mapAddressBook.count(address))
                entry.push_back(Pair("account", pwalletMain->mapAddressBook[address]));
        }
        entry.push_back(Pair("scriptPubKey", HexStr(pk.begin(), pk.end())));
        if (pk.IsPayToScriptHash())
        {
            CTxDestination address;
            if (ExtractDestination(pk, address))
            {
                const CScriptID& hash = boost::get<const CScriptID&>(address);
                CScript redeemScript;
                if (pwalletMain->GetCScript(hash, redeemScript))
                    entry.push_back(Pair("redeemScript", HexStr(redeemScript.begin(), redeemScript.end())));
            }
        }
        entry.push_back(Pair("amount",ValueFromAmount(nValue)));
        entry.push_back(Pair("confirmations",out.nDepth));
        results.push_back(entry);
    }

    return results;
}

Value createrawtransaction(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error(
            "createrawtransaction [{\"txid\":txid,\"vout\":n},...] {address:amount,...}\n"
            "Create a transaction spending given inputs\n"
            "(array of objects containing transaction id and output number),\n"
            "sending to given address(es).\n"
            "Returns hex-encoded raw transaction.\n"
            "Note that the transaction's inputs are not signed, and\n"
            "it is not stored in the wallet or transmitted to the network.");

    RPCTypeCheck(params, list_of(array_type)(obj_type));

    Array inputs = params[0].get_array();
    Object sendTo = params[1].get_obj();

    CTransaction rawTx;

    BOOST_FOREACH(const Value& input, inputs)
    {
        const Object& o = input.get_obj();

        uint256 txid = ParseHashO(o, "txid");

        const Value& vout_v = find_value(o, "vout");
        if (vout_v.type() != int_type)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, missing vout key");
        int nOutput = vout_v.get_int();
        if (nOutput < 0)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, vout must be positive");

        CTxIn in(COutPoint(txid, nOutput));
        rawTx.vin.push_back(in);
    }

    set<CBitcoinAddress> setAddress;
    BOOST_FOREACH(const Pair& s, sendTo)
    {
        CBitcoinAddress address(s.name_);
        if (!address.IsValid())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, string("Invalid Litecoin address: ")+s.name_);

        if (setAddress.count(address))
            throw JSONRPCError(RPC_INVALID_PARAMETER, string("Invalid parameter, duplicated address: ")+s.name_);
        setAddress.insert(address);

        CScript scriptPubKey;
        scriptPubKey.SetDestination(address.Get());
        int64 nAmount = AmountFromValue(s.value_);

        CTxOut out(nAmount, scriptPubKey);
        rawTx.vout.push_back(out);
    }

    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << rawTx;
    return HexStr(ss.begin(), ss.end());
}

Value decoderawtransaction(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "decoderawtransaction <hex string>\n"
            "Return a JSON object representing the serialized, hex-encoded transaction.");

    vector<unsigned char> txData(ParseHexV(params[0], "argument"));
    CDataStream ssData(txData, SER_NETWORK, PROTOCOL_VERSION);
    CTransaction tx;
    try {
        ssData >> tx;
    }
    catch (std::exception &e) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed");
    }

    Object result;
    TxToJSON(tx, 0, result);

    return result;
}

Value signrawtransaction(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 4)
        throw runtime_error(
            "signrawtransaction <hex string> [{\"txid\":txid,\"vout\":n,\"scriptPubKey\":hex,\"redeemScript\":hex},...] [<privatekey1>,...] [sighashtype=\"ALL\"]\n"
            "Sign inputs for raw transaction (serialized, hex-encoded).\n"
            "Second optional argument (may be null) is an array of previous transaction outputs that\n"
            "this transaction depends on but may not yet be in the block chain.\n"
            "Third optional argument (may be null) is an array of base58-encoded private\n"
            "keys that, if given, will be the only keys used to sign the transaction.\n"
            "Fourth optional argument is a string that is one of six values; ALL, NONE, SINGLE or\n"
            "ALL|ANYONECANPAY, NONE|ANYONECANPAY, SINGLE|ANYONECANPAY.\n"
            "Returns json object with keys:\n"
            "  hex : raw transaction with signature(s) (hex-encoded string)\n"
            "  complete : 1 if transaction has a complete set of signature (0 if not)"
            + HelpRequiringPassphrase());

    RPCTypeCheck(params, list_of(str_type)(array_type)(array_type)(str_type), true);

    vector<unsigned char> txData(ParseHexV(params[0], "argument 1"));
    CDataStream ssData(txData, SER_NETWORK, PROTOCOL_VERSION);
    vector<CTransaction> txVariants;
    while (!ssData.empty())
    {
        try {
            CTransaction tx;
            ssData >> tx;
            txVariants.push_back(tx);
        }
        catch (std::exception &e) {
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed");
        }
    }

    if (txVariants.empty())
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Missing transaction");

    // mergedTx will end up with all the signatures; it
    // starts as a clone of the rawtx:
    CTransaction mergedTx(txVariants[0]);
    bool fComplete = true;

    // Fetch previous transactions (inputs):
    CCoinsView viewDummy;
    CCoinsViewCache view(viewDummy);
    {
        LOCK(mempool.cs);
        CCoinsViewCache &viewChain = *pcoinsTip;
        CCoinsViewMemPool viewMempool(viewChain, mempool);
        view.SetBackend(viewMempool); // temporarily switch cache backend to db+mempool view

        BOOST_FOREACH(const CTxIn& txin, mergedTx.vin) {
            const uint256& prevHash = txin.prevout.hash;
            CCoins coins;
            view.GetCoins(prevHash, coins); // this is certainly allowed to fail
        }

        view.SetBackend(viewDummy); // switch back to avoid locking mempool for too long
    }

    bool fGivenKeys = false;
    CBasicKeyStore tempKeystore;
    if (params.size() > 2 && params[2].type() != null_type)
    {
        fGivenKeys = true;
        Array keys = params[2].get_array();
        BOOST_FOREACH(Value k, keys)
        {
            CBitcoinSecret vchSecret;
            bool fGood = vchSecret.SetString(k.get_str());
            if (!fGood)
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key");
            CKey key = vchSecret.GetKey();
            tempKeystore.AddKey(key);
        }
    }
    else
        EnsureWalletIsUnlocked();

    // Add previous txouts given in the RPC call:
    if (params.size() > 1 && params[1].type() != null_type)
    {
        Array prevTxs = params[1].get_array();
        BOOST_FOREACH(Value& p, prevTxs)
        {
            if (p.type() != obj_type)
                throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "expected object with {\"txid'\",\"vout\",\"scriptPubKey\"}");

            Object prevOut = p.get_obj();

            RPCTypeCheck(prevOut, map_list_of("txid", str_type)("vout", int_type)("scriptPubKey", str_type));

            uint256 txid = ParseHashO(prevOut, "txid");

            int nOut = find_value(prevOut, "vout").get_int();
            if (nOut < 0)
                throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "vout must be positive");

            vector<unsigned char> pkData(ParseHexO(prevOut, "scriptPubKey"));
            CScript scriptPubKey(pkData.begin(), pkData.end());

            CCoins coins;
            if (view.GetCoins(txid, coins)) {
                if (coins.IsAvailable(nOut) && coins.vout[nOut].scriptPubKey != scriptPubKey) {
                    string err("Previous output scriptPubKey mismatch:\n");
                    err = err + coins.vout[nOut].scriptPubKey.ToString() + "\nvs:\n"+
                        scriptPubKey.ToString();
                    throw JSONRPCError(RPC_DESERIALIZATION_ERROR, err);
                }
                // what todo if txid is known, but the actual output isn't?
            }
            if ((unsigned int)nOut >= coins.vout.size())
                coins.vout.resize(nOut+1);
            coins.vout[nOut].scriptPubKey = scriptPubKey;
            coins.vout[nOut].nValue = 0; // we don't know the actual output value
            view.SetCoins(txid, coins);

            // if redeemScript given and not using the local wallet (private keys
            // given), add redeemScript to the tempKeystore so it can be signed:
            if (fGivenKeys && scriptPubKey.IsPayToScriptHash())
            {
                RPCTypeCheck(prevOut, map_list_of("txid", str_type)("vout", int_type)("scriptPubKey", str_type)("redeemScript",str_type));
                Value v = find_value(prevOut, "redeemScript");
                if (!(v == Value::null))
                {
                    vector<unsigned char> rsData(ParseHexV(v, "redeemScript"));
                    CScript redeemScript(rsData.begin(), rsData.end());
                    tempKeystore.AddCScript(redeemScript);
                }
            }
        }
    }

    const CKeyStore& keystore = ((fGivenKeys || !pwalletMain) ? tempKeystore : *pwalletMain);

    int nHashType = SIGHASH_ALL;
    if (params.size() > 3 && params[3].type() != null_type)
    {
        static map<string, int> mapSigHashValues =
            boost::assign::map_list_of
            (string("ALL"), int(SIGHASH_ALL))
            (string("ALL|ANYONECANPAY"), int(SIGHASH_ALL|SIGHASH_ANYONECANPAY))
            (string("NONE"), int(SIGHASH_NONE))
            (string("NONE|ANYONECANPAY"), int(SIGHASH_NONE|SIGHASH_ANYONECANPAY))
            (string("SINGLE"), int(SIGHASH_SINGLE))
            (string("SINGLE|ANYONECANPAY"), int(SIGHASH_SINGLE|SIGHASH_ANYONECANPAY))
            ;
        string strHashType = params[3].get_str();
        if (mapSigHashValues.count(strHashType))
            nHashType = mapSigHashValues[strHashType];
        else
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid sighash param");
    }

    bool fHashSingle = ((nHashType & ~SIGHASH_ANYONECANPAY) == SIGHASH_SINGLE);

    // Sign what we can:
    for (unsigned int i = 0; i < mergedTx.vin.size(); i++)
    {
        CTxIn& txin = mergedTx.vin[i];
        CCoins coins;
        if (!view.GetCoins(txin.prevout.hash, coins) || !coins.IsAvailable(txin.prevout.n))
        {
            fComplete = false;
            continue;
        }
        const CScript& prevPubKey = coins.vout[txin.prevout.n].scriptPubKey;

        txin.scriptSig.clear();
        // Only sign SIGHASH_SINGLE if there's a corresponding output:
        if (!fHashSingle || (i < mergedTx.vout.size()))
            SignSignature(keystore, prevPubKey, mergedTx, i, nHashType);

        // ... and merge in other signatures:
        BOOST_FOREACH(const CTransaction& txv, txVariants)
        {
            txin.scriptSig = CombineSignatures(prevPubKey, mergedTx, i, txin.scriptSig, txv.vin[i].scriptSig);
        }
        if (!VerifyScript(txin.scriptSig, prevPubKey, mergedTx, i, SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_STRICTENC, 0))
            fComplete = false;
    }

    Object result;
    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
    ssTx << mergedTx;
    result.push_back(Pair("hex", HexStr(ssTx.begin(), ssTx.end())));
    result.push_back(Pair("complete", fComplete));

    return result;
}

Value sendrawtransaction(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "sendrawtransaction <hex string> [allowhighfees=false]\n"
            "Submits raw transaction (serialized, hex-encoded) to local node and network.");

    // parse hex string from parameter
    vector<unsigned char> txData(ParseHexV(params[0], "parameter"));
    CDataStream ssData(txData, SER_NETWORK, PROTOCOL_VERSION);
    CTransaction tx;

    bool fOverrideFees = false;
    if (params.size() > 1)
        fOverrideFees = params[1].get_bool();

    // deserialize binary data stream
    try {
        ssData >> tx;
    }
    catch (std::exception &e) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed");
    }
    uint256 hashTx = tx.GetHash();

    bool fHave = false;
    CCoinsViewCache &view = *pcoinsTip;
    CCoins existingCoins;
    {
        fHave = view.GetCoins(hashTx, existingCoins);
        if (!fHave) {
            // push to local node
            CValidationState state;
            if (!tx.AcceptToMemoryPool(state, true, false, NULL, !fOverrideFees))
                throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX rejected"); // TODO: report validation state
        }
    }
    if (fHave) {
        if (existingCoins.nHeight < 1000000000)
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "transaction already in block chain");
        // Not in block, but already in the memory pool; will drop
        // through to re-relay it.
    } else {
        SyncWithWallets(hashTx, tx, NULL, true);
    }
    RelayTransaction(tx, hashTx);

    return hashTx.GetHex();
}

Value getnormalizedtxid(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getnormalizedtxid <hex string>\n"
            "Return the normalized transaction ID.");

    // parse hex string from parameter
    vector<unsigned char> txData(ParseHexV(params[0], "parameter"));
    CDataStream ssData(txData, SER_NETWORK, PROTOCOL_VERSION);
    CTransaction tx;

    // deserialize binary data stream
    try {
        ssData >> tx;
    }
    catch (std::exception &e) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed");
    }
    uint256 hashNormalized = tx.GetNormalizedHash();

    return hashNormalized.GetHex();
}
