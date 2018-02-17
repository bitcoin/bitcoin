// Copyright (c) 2018 The Particl Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <usbdevice/usbdevice.h>
#include <rpc/server.h>
#include <utilstrencodings.h>
#include <base58.h>
#include <key/extkey.h>
#include <chainparams.h>

#include <univalue.h>

#ifdef ENABLE_WALLET
#include <wallet/hdwallet.h>
#include <wallet/rpchdwallet.h>
#endif
#include <validation.h>
#include <core_io.h>
#include <primitives/transaction.h>
#include <keystore.h>
#include <policy/policy.h>
#include <script/script.h>
#include <script/script_error.h>
#include <script/sign.h>
#include <script/standard.h>




static std::string GetDefaultPath()
{
    // Return path of default account: 44'/44'/0'
    std::vector<uint32_t> vPath;
    vPath.push_back(WithHardenedBit(44)); // purpose
    vPath.push_back(WithHardenedBit(Params().BIP44ID())); // coin
    vPath.push_back(WithHardenedBit(0)); // account
    std::string rv;
    if (0 == PathToString(vPath, rv, 'h'))
        return rv;
    return "";
}

static std::vector<uint32_t> GetPath(std::vector<uint32_t> &vPath, const UniValue &path, const UniValue &defaultpath)
{
    // Pass empty string as defaultpath to drop defaultpath and use path as full path
    std::string sPath;
    if (path.isStr())
    {
        sPath = path.get_str();
    } else
    if (path.isNum())
    {
        sPath = strprintf("%d", path.get_int());
    } else
    {
        throw JSONRPCError(RPC_INVALID_PARAMETER, _("Unknown \"path\" type."));
    };

    if (defaultpath.isNull())
    {
        sPath = GetDefaultPath() + "/" + sPath;
    } else
    if (path.isNum())
    {
        sPath = strprintf("%d", defaultpath.get_int()) + "/" + sPath;
    } else
    if (defaultpath.isStr())
    {
        if (!defaultpath.get_str().empty())
            sPath = defaultpath.get_str() + "/" + sPath;
    } else
    {
        throw JSONRPCError(RPC_INVALID_PARAMETER, _("Unknown \"defaultpath\" type."));
    };

    int rv;
    if ((rv = ExtractExtKeyPath(sPath, vPath)) != 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Bad path: %s.", ExtKeyGetString(rv)));

    return vPath;
}

UniValue listdevices(const JSONRPCRequest &request)
{
    if (request.fHelp || request.params.size() > 0)
        throw std::runtime_error(
            "listdevices\n"
            "list connected hardware devices.\n"
            "\nResult\n"
            "{\n"
            "  \"vendor\"           (string) USB vendor string.\n"
            "  \"product\"          (string) USB product string.\n"
            "  \"firmwareversion\"  (string) Detected firmware version of device, if possible.\n"
            "}\n"
            "\nExamples\n"
            + HelpExampleCli("listdevices", "")
            + HelpExampleRpc("listdevices", ""));

    std::vector<CUSBDevice> vDevices;
    ListDevices(vDevices);

    UniValue result(UniValue::VARR);

    for (auto &device : vDevices)
    {
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("vendor", device.pType->cVendor);
        obj.pushKV("product", device.pType->cProduct);

        std::string sValue, sError;
        if (0 == device.GetFirmwareVersion(sValue, sError))
            obj.pushKV("firmwareversion", sValue);
        else
            obj.pushKV("error", sError);

        result.push_back(obj);
    };

    return result;
};


UniValue getdeviceinfo(const JSONRPCRequest &request)
{
    if (request.fHelp || request.params.size() > 0)
        throw std::runtime_error(
            "getdeviceinfo\n"
            "Get information from connected hardware device.\n"
            "\nResult\n"
            "{\n"
            "}\n"
            "\nExamples\n"
            + HelpExampleCli("getdeviceinfo", "")
            + HelpExampleRpc("getdeviceinfo", ""));

    std::vector<CUSBDevice> vDevices;
    ListDevices(vDevices);

    if (vDevices.size() < 1)
        throw JSONRPCError(RPC_INTERNAL_ERROR, "No device found.");
    if (vDevices.size() > 1) // TODO: Select device
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Multiple devices found.");

    UniValue info(UniValue::VOBJ);
    std::string sError;
    if (0 != vDevices[0].GetInfo(info, sError))
        info.pushKV("error", sError);

    return info;
};

UniValue getdevicepublickey(const JSONRPCRequest &request)
{
    if (request.fHelp || request.params.size() < 1 || request.params.size() > 2)
        throw std::runtime_error(
            "getdevicepublickey \"path\" (\"accountpath\")\n"
            "Get the public key and address at \"path\" from a hardware device.\n"
            "\nArguments:\n"
            "1. \"path\"              (string, required) The path to derive the key from.\n"
            "                           The full path is \"accountpath\"/\"path\".\n"
            "2. \"accountpath\"       (string, optional) Account path, set to empty string to ignore (default=\""+GetDefaultPath()+"\").\n"
            "\nResult\n"
            "{\n"
            "  \"publickey\"        (string) The derived public key at \"path\".\n"
            "  \"address\"          (string) The address of \"publickey\".\n"
            "  \"path\"             (string) The full path of \"publickey\".\n"
            "}\n"
            "\nExamples\n"
            "Get the first public key of external chain:\n"
            + HelpExampleCli("getdevicepublickey", "\"0/0\"")
            + HelpExampleRpc("getdevicepublickey", "\"0/0\"")
            + "Get the first public key of internal chain of testnet account:\n"
            + HelpExampleCli("getdevicepublickey", "\"1/0\" \"44h/1h/0h\""));

    std::vector<uint32_t> vPath;
    GetPath(vPath, request.params[0], request.params[1]);

    std::vector<CUSBDevice> vDevices;
    ListDevices(vDevices);
    if (vDevices.size() < 1)
        throw JSONRPCError(RPC_INTERNAL_ERROR, "No device found.");
    if (vDevices.size() > 1) // TODO: Select device
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Multiple devices found.");

    std::string sError;
    CPubKey pk;
    if (0 != vDevices[0].GetPubKey(vPath, pk, sError))
        throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("GetPubKey failed %s.", sError));

    std::string sPath;
    if (0 != PathToString(vPath, sPath))
        sPath = "error";
    UniValue rv(UniValue::VOBJ);
    rv.pushKV("publickey", HexStr(pk));
    rv.pushKV("address", CBitcoinAddress(pk.GetID()).ToString());
    rv.pushKV("path", sPath);
    return rv;
}

UniValue getdevicexpub(const JSONRPCRequest &request)
{
    if (request.fHelp || request.params.size() < 1 || request.params.size() > 2)
        throw std::runtime_error(
            "getdevicexpub \"path\" (\"accountpath\")\n"
            "Get the extended public key at \"path\" from a hardware device.\n"
            "\nArguments:\n"
            "1. \"path\"              (string, required) The path to derive the key from.\n"
            "                           The full path is \"accountpath\"/\"path\".\n"
            "2. \"accountpath\"       (string, optional) Account path, set to empty string to ignore (default=\""+GetDefaultPath()+"\").\n"
            "\nResult\n"
            "\"address\"              (string) The particl extended public key\n"
            "\nExamples\n"
            + HelpExampleCli("getdevicexpub", "\"0\"")
            + HelpExampleRpc("getdevicexpub", "\"0\""));

    std::vector<uint32_t> vPath;
    GetPath(vPath, request.params[0], request.params[1]);

    std::vector<CUSBDevice> vDevices;
    ListDevices(vDevices);
    if (vDevices.size() < 1)
        throw JSONRPCError(RPC_INTERNAL_ERROR, "No device found.");
    if (vDevices.size() > 1) // TODO: Select device
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Multiple devices found.");

    std::string sError;
    CExtPubKey ekp;
    if (0 != vDevices[0].GetXPub(vPath, ekp, sError))
        throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("GetXPub failed %s.", sError));

    return CBitcoinExtPubKey(ekp).ToString();
};

UniValue devicesignmessage(const JSONRPCRequest &request)
{
    if (request.fHelp || request.params.size() < 2 || request.params.size() > 3)
        throw std::runtime_error(
            "devicesignmessage \"path\" \"message\" (\"accountpath\")\n"
            "Sign a message with the key at \"path\" on a hardware device.\n"
            "\nArguments:\n"
            "1. \"path\"            (string, required) The path to the key to sign with.\n"
            "                           The full path is \"accountpath\"/\"path\".\n"
            "2. \"message\"         (string, required) The message to create a signature for.\n"
            "3. \"accountpath\"     (string, optional) Account path, set to empty string to ignore (default=\""+GetDefaultPath()+"\").\n"
            "\nResult\n"
            "\"signature\"          (string) The signature of the message encoded in base 64\n"
            "\nExamples\n"
            "Sign with the first key of external chain:\n"
            + HelpExampleCli("devicesignmessage", "\"0/0\" \"my message\"")
            + HelpExampleRpc("devicesignmessage", "\"0/0\", \"my message\""));

    std::vector<uint32_t> vPath;
    GetPath(vPath, request.params[0], request.params[2]);

    std::vector<CUSBDevice> vDevices;
    ListDevices(vDevices);
    if (vDevices.size() < 1)
        throw JSONRPCError(RPC_INTERNAL_ERROR, "No device found.");
    if (vDevices.size() > 1) // TODO: Select device
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Multiple devices found.");

    if (!request.params[1].isStr())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Bad message.");

    std::string sError, sMessage = request.params[1].get_str();
    std::vector<uint8_t> vchSig;
    if (0 != vDevices[0].SignMessage(vPath, sMessage, vchSig, sError))
        throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("SignMessage failed %s.", sError));

    return EncodeBase64(vchSig.data(), vchSig.size());
};


/** Pushes a JSON object for script verification or signing errors to vErrorsRet. */
// TODO: deduplicate
static void TxInErrorToJSON(const CTxIn& txin, UniValue& vErrorsRet, const std::string& strMessage)
{
    UniValue entry(UniValue::VOBJ);
    entry.push_back(Pair("txid", txin.prevout.hash.ToString()));
    entry.push_back(Pair("vout", (uint64_t)txin.prevout.n));
    UniValue witness(UniValue::VARR);
    for (unsigned int i = 0; i < txin.scriptWitness.stack.size(); i++) {
        witness.push_back(HexStr(txin.scriptWitness.stack[i].begin(), txin.scriptWitness.stack[i].end()));
    }
    entry.push_back(Pair("witness", witness));
    entry.push_back(Pair("scriptSig", HexStr(txin.scriptSig.begin(), txin.scriptSig.end())));
    entry.push_back(Pair("sequence", (uint64_t)txin.nSequence));
    entry.push_back(Pair("error", strMessage));
    vErrorsRet.push_back(entry);
}

UniValue devicesignrawtransaction(const JSONRPCRequest &request)
{
    #ifdef ENABLE_WALLET
    CHDWallet *pwallet = GetHDWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;
    #endif

    if (request.fHelp || request.params.size() < 1 || request.params.size() > 5)
        throw std::runtime_error(
            "devicesignrawtransaction \"hexstring\" ( [{\"txid\":\"id\",\"vout\":n,\"scriptPubKey\":\"hex\",\"redeemScript\":\"hex\"},...] [\"path1\",...] sighashtype, \"accountpath\" )\n"
            "\nSign inputs for raw transaction (serialized, hex-encoded).\n"
            "The second optional argument (may be null) is an array of previous transaction outputs that\n"
            "this transaction depends on but may not yet be in the block chain.\n"
            "The third optional argument (may be null) is an array of bip44 paths\n"
            "that, if given, will be the only keys derived to sign the transaction.\n"
            "\nArguments:\n"
            "1. \"hexstring\"     (string, required) The transaction hex string\n"
            "2. \"prevtxs\"       (string, optional) An json array of previous dependent transaction outputs\n"
            "     [               (json array of json objects, or 'null' if none provided)\n"
            "       {\n"
            "         \"txid\":\"id\",             (string, required) The transaction id\n"
            "         \"vout\":n,                  (numeric, required) The output number\n"
            "         \"scriptPubKey\": \"hex\",   (string, required) script key\n"
            "         \"redeemScript\": \"hex\",   (string, required for P2SH or P2WSH) redeem script\n"
            "         \"amount\": value            (numeric, required) The amount spent\n"
            "       }\n"
            "       ,...\n"
            "    ]\n"
            "3. \"paths\"     (string, optional) A json array of key paths for signing\n"
            "    [                  (json array of strings, or 'null' if none provided)\n"
            "      \"path\"   (string) bip44 path\n"
            "      ,...\n"
            "    ]\n"
            "4. \"sighashtype\"     (string, optional, default=ALL) The signature hash type. Must be one of\n"
            "       \"ALL\"\n"
            "       \"NONE\"\n"
            "       \"SINGLE\"\n"
            "       \"ALL|ANYONECANPAY\"\n"
            "       \"NONE|ANYONECANPAY\"\n"
            "       \"SINGLE|ANYONECANPAY\"\n"
            "5. \"accountpath\"     (string, optional) Account path, set to empty string to ignore (default=\""+GetDefaultPath()+"\").\n"
            "\nResult\n"
            "{\n"
            "  \"hex\" : \"value\",           (string) The hex-encoded raw transaction with signature(s)\n"
            "  \"complete\" : true|false,   (boolean) If the transaction has a complete set of signatures\n"
            "  \"errors\" : [                 (json array of objects) Script verification errors (if there are any)\n"
            "    {\n"
            "      \"txid\" : \"hash\",           (string) The hash of the referenced, previous transaction\n"
            "      \"vout\" : n,                (numeric) The index of the output to spent and used as input\n"
            "      \"scriptSig\" : \"hex\",       (string) The hex-encoded signature script\n"
            "      \"sequence\" : n,            (numeric) Script sequence number\n"
            "      \"error\" : \"text\"           (string) Verification or signing error related to the input\n"
            "    }\n"
            "    ,...\n"
            "  ]\n"
            "}\n"
            "\nExamples\n"
);

#ifdef ENABLE_WALLET
    LOCK2(cs_main, pwallet ? &pwallet->cs_wallet : nullptr);
#else
    LOCK(cs_main);
#endif
    RPCTypeCheck(request.params, {UniValue::VSTR, UniValue::VARR, UniValue::VARR, UniValue::VSTR, UniValue::VSTR}, true);

    CMutableTransaction mtx;
    if (!DecodeHexTx(mtx, request.params[0].get_str(), true))
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed");

    std::vector<CUSBDevice> vDevices;
    ListDevices(vDevices);
    if (vDevices.size() < 1)
        throw JSONRPCError(RPC_INTERNAL_ERROR, "No device found.");
    if (vDevices.size() > 1) // TODO: Select device
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Multiple devices found.");

    // TODO check root id on wallet and device match


    // Fetch previous transactions (inputs):
    CCoinsView viewDummy;
    CCoinsViewCache view(&viewDummy);
    {
        LOCK(mempool.cs);
        CCoinsViewCache &viewChain = *pcoinsTip;
        CCoinsViewMemPool viewMempool(&viewChain, mempool);
        view.SetBackend(viewMempool); // temporarily switch cache backend to db+mempool view

        for (const CTxIn& txin : mtx.vin) {
            view.AccessCoin(txin.prevout); // Load entries from viewChain into view; can fail.
        }

        view.SetBackend(viewDummy); // switch back to avoid locking mempool for too long
    }


    bool fGivenKeys = false;
    CPathKeyStore tempKeystore;
    if (!request.params[2].isNull()) {
        fGivenKeys = true;
        UniValue paths = request.params[2].get_array();
        for (unsigned int idx = 0; idx < paths.size(); idx++) {
            CPathKey pathkey;
            GetPath(pathkey.vPath, paths[idx], request.params[4]);

            std::string sError;
            PathToString(pathkey.vPath, sError);

            if (0 != vDevices[0].GetPubKey(pathkey.vPath, pathkey.pk, sError))
                throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("Device GetPubKey failed %s.", sError));

            tempKeystore.AddKey(pathkey);
            /*
            CBitcoinSecret vchSecret;
            bool fGood = vchSecret.SetString(k.get_str());
            if (!fGood)
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key");
            CKey key = vchSecret.GetKey();
            if (!key.IsValid())
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Private key outside allowed range");
            tempKeystore.AddKey(key);
            */
        }
    }

    // Add previous txouts given in the RPC call:
    if (!request.params[1].isNull()) {
        UniValue prevTxs = request.params[1].get_array();
        for (unsigned int idx = 0; idx < prevTxs.size(); idx++) {
            const UniValue& p = prevTxs[idx];
            if (!p.isObject())
                throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "expected object with {\"txid'\",\"vout\",\"scriptPubKey\"}");

            UniValue prevOut = p.get_obj();

            RPCTypeCheckObj(prevOut,
                {
                    {"txid", UniValueType(UniValue::VSTR)},
                    {"vout", UniValueType(UniValue::VNUM)},
                    {"scriptPubKey", UniValueType(UniValue::VSTR)},
                });

            uint256 txid = ParseHashO(prevOut, "txid");

            int nOut = find_value(prevOut, "vout").get_int();
            if (nOut < 0)
                throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "vout must be positive");

            COutPoint out(txid, nOut);
            std::vector<unsigned char> pkData(ParseHexO(prevOut, "scriptPubKey"));
            CScript scriptPubKey(pkData.begin(), pkData.end());

            {
            const Coin& coin = view.AccessCoin(out);

            if (coin.nType != OUTPUT_STANDARD)
                throw JSONRPCError(RPC_MISC_ERROR, "TODO: make work for !StandardOutput");

            if (!coin.IsSpent() && coin.out.scriptPubKey != scriptPubKey) {
                std::string err("Previous output scriptPubKey mismatch:\n");
                err = err + ScriptToAsmStr(coin.out.scriptPubKey) + "\nvs:\n"+
                    ScriptToAsmStr(scriptPubKey);
                throw JSONRPCError(RPC_DESERIALIZATION_ERROR, err);
            }
            Coin newcoin;
            newcoin.out.scriptPubKey = scriptPubKey;
            newcoin.out.nValue = 0;
            if (prevOut.exists("amount")) {
                newcoin.out.nValue = AmountFromValue(find_value(prevOut, "amount"));
            }
            newcoin.nHeight = 1;
            view.AddCoin(out, std::move(newcoin), true);
            }

            // if redeemScript given and not using the local wallet (private keys
            // given), add redeemScript to the tempKeystore so it can be signed:
            if (fGivenKeys && (scriptPubKey.IsPayToScriptHashAny())) {
                RPCTypeCheckObj(prevOut,
                    {
                        {"txid", UniValueType(UniValue::VSTR)},
                        {"vout", UniValueType(UniValue::VNUM)},
                        {"scriptPubKey", UniValueType(UniValue::VSTR)},
                        {"redeemScript", UniValueType(UniValue::VSTR)},
                    });
                UniValue v = find_value(prevOut, "redeemScript");
                if (!v.isNull()) {
                    std::vector<unsigned char> rsData(ParseHexV(v, "redeemScript"));
                    CScript redeemScript(rsData.begin(), rsData.end());
                    tempKeystore.AddCScript(redeemScript);
                }
            }
        }
    }

#ifdef ENABLE_WALLET
    const CKeyStore& keystore = ((fGivenKeys || !pwallet) ? (CKeyStore&)tempKeystore : (CKeyStore&)*pwallet);
#else
    const CKeyStore& keystore = tempKeystore;
#endif

    int nHashType = SIGHASH_ALL;
    if (!request.params[3].isNull()) {
        static std::map<std::string, int> mapSigHashValues = {
            {std::string("ALL"), int(SIGHASH_ALL)},
            {std::string("ALL|ANYONECANPAY"), int(SIGHASH_ALL|SIGHASH_ANYONECANPAY)},
            {std::string("NONE"), int(SIGHASH_NONE)},
            {std::string("NONE|ANYONECANPAY"), int(SIGHASH_NONE|SIGHASH_ANYONECANPAY)},
            {std::string("SINGLE"), int(SIGHASH_SINGLE)},
            {std::string("SINGLE|ANYONECANPAY"), int(SIGHASH_SINGLE|SIGHASH_ANYONECANPAY)},
        };
        std::string strHashType = request.params[3].get_str();
        if (mapSigHashValues.count(strHashType))
            nHashType = mapSigHashValues[strHashType];
        else
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid sighash param");
    }

    bool fHashSingle = ((nHashType & ~SIGHASH_ANYONECANPAY) == SIGHASH_SINGLE);

    // Script verification errors
    UniValue vErrors(UniValue::VARR);

    // Use CTransaction for the constant parts of the
    // transaction to avoid rehashing.
    const CTransaction txConst(mtx);
    // Sign what we can:
    for (unsigned int i = 0; i < mtx.vin.size(); i++) {
        CTxIn& txin = mtx.vin[i];
        const Coin& coin = view.AccessCoin(txin.prevout);
        if (coin.IsSpent()) {
            TxInErrorToJSON(txin, vErrors, "Input not found or already spent");
            continue;
        }
        if (coin.nType != OUTPUT_STANDARD)
            throw JSONRPCError(RPC_MISC_ERROR, "TODO: make work for !StandardOutput");

        std::vector<uint8_t> vchAmount(8);
        SignatureData sigdata;
        CScript prevPubKey = coin.out.scriptPubKey;
        CAmount amount = coin.out.nValue;
        memcpy(&vchAmount[0], &amount, 8);

        // Only sign SIGHASH_SINGLE if there's a corresponding output:
        if (!fHashSingle || (i < mtx.GetNumVOuts()))
        {
            CTransaction tx(mtx);
            vDevices[0].sError.clear();
            ProduceSignature(DeviceSignatureCreator(&vDevices[0], &keystore, &tx, i, vchAmount, nHashType), prevPubKey, sigdata);

            if (!vDevices[0].sError.empty())
            {
                UniValue entry(UniValue::VOBJ);
                entry.push_back(Pair("error", vDevices[0].sError));
                vErrors.push_back(entry);
            };
        };

        sigdata = CombineSignatures(prevPubKey, TransactionSignatureChecker(&txConst, i, vchAmount), sigdata, DataFromTransaction(mtx, i));
        UpdateTransaction(mtx, i, sigdata);

        ScriptError serror = SCRIPT_ERR_OK;
        if (!VerifyScript(txin.scriptSig, prevPubKey, &txin.scriptWitness, STANDARD_SCRIPT_VERIFY_FLAGS, TransactionSignatureChecker(&txConst, i, vchAmount), &serror)) {
            if (serror == SCRIPT_ERR_INVALID_STACK_OPERATION) {
                // Unable to sign input and verification failed (possible attempt to partially sign).
                TxInErrorToJSON(txin, vErrors, "Unable to sign input, invalid stack size (possibly missing key)");
            } else {
                TxInErrorToJSON(txin, vErrors, ScriptErrorString(serror));
            }
        }
    }
    bool fComplete = vErrors.empty();

    UniValue result(UniValue::VOBJ);
    result.push_back(Pair("hex", EncodeHexTx(mtx)));
    result.push_back(Pair("complete", fComplete));
    if (!vErrors.empty()) {
        result.push_back(Pair("errors", vErrors));
    }

    return result;
};


static const CRPCCommand commands[] =
{ //  category              name                        actor (function)           argNames
  //  --------------------- ------------------------    -----------------------    ----------
    { "usbdevice",          "listdevices",              &listdevices,              {} },
    { "usbdevice",          "getdeviceinfo",            &getdeviceinfo,            {} },
    { "usbdevice",          "getdevicepublickey",       &getdevicepublickey,       {"path","accountpath"} },
    { "usbdevice",          "getdevicexpub",            &getdevicexpub,            {"path","accountpath"} },
    { "usbdevice",          "devicesignmessage",        &devicesignmessage,        {"path","message","accountpath"} },
    { "usbdevice",          "devicesignrawtransaction", &devicesignrawtransaction, {"hexstring","prevtxs","privkeypaths","sighashtype","accountpath"} }, /* uses wallet if enabled */
};

void RegisterUSBDeviceRPC(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
