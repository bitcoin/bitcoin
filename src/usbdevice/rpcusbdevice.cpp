// Copyright (c) 2018 The Particl Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <usbdevice/usbdevice.h>
#include <usbdevice/debugdevice.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <rpc/safemode.h>
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

#include <memory>




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
};

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
};

CUSBDevice *SelectDevice(std::vector<std::unique_ptr<CUSBDevice> > &vDevices)
{
    std::string sError;
    CUSBDevice *rv = SelectDevice(vDevices, sError);
    if (!rv)
        throw JSONRPCError(RPC_INTERNAL_ERROR, sError);
    return rv;
};

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

    std::vector<std::unique_ptr<CUSBDevice> > vDevices;
    ListDevices(vDevices);

    UniValue result(UniValue::VARR);

    for (size_t i = 0; i < vDevices.size(); ++i)
    {
        CUSBDevice *device = vDevices[i].get();
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("vendor", device->pType->cVendor);
        obj.pushKV("product", device->pType->cProduct);

        std::string sValue, sError;
        if (0 == device->GetFirmwareVersion(sValue, sError))
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

    std::vector<std::unique_ptr<CUSBDevice> > vDevices;
    CUSBDevice *pDevice = SelectDevice(vDevices);

    UniValue info(UniValue::VOBJ);
    std::string sError;
    if (0 != pDevice->GetInfo(info, sError))
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

    std::vector<std::unique_ptr<CUSBDevice> > vDevices;
    CUSBDevice *pDevice = SelectDevice(vDevices);

    std::string sError;
    CPubKey pk;
    if (0 != pDevice->GetPubKey(vPath, pk, sError))
        throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("GetPubKey failed %s.", sError));

    std::string sPath;
    if (0 != PathToString(vPath, sPath))
        sPath = "error";
    UniValue rv(UniValue::VOBJ);
    rv.pushKV("publickey", HexStr(pk));
    rv.pushKV("address", CBitcoinAddress(pk.GetID()).ToString());
    rv.pushKV("path", sPath);
    return rv;
};

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

    std::vector<std::unique_ptr<CUSBDevice> > vDevices;
    CUSBDevice *pDevice = SelectDevice(vDevices);

    std::string sError;
    CExtPubKey ekp;
    if (0 != pDevice->GetXPub(vPath, ekp, sError))
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

    std::vector<std::unique_ptr<CUSBDevice> > vDevices;
    CUSBDevice *pDevice = SelectDevice(vDevices);

    if (!request.params[1].isStr())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Bad message.");

    std::string sError, sMessage = request.params[1].get_str();
    std::vector<uint8_t> vchSig;
    if (0 != pDevice->SignMessage(vPath, sMessage, vchSig, sError))
        throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("SignMessage failed %s.", sError));

    return EncodeBase64(vchSig.data(), vchSig.size());
};

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
            + HelpExampleCli("devicesignrawtransaction", "\"myhex\"")
            + HelpExampleRpc("devicesignrawtransaction", "\"myhex\""));

#ifdef ENABLE_WALLET
    LOCK2(cs_main, pwallet ? &pwallet->cs_wallet : nullptr);
#else
    LOCK(cs_main);
#endif
    RPCTypeCheck(request.params, {UniValue::VSTR, UniValue::VARR, UniValue::VARR, UniValue::VSTR, UniValue::VSTR}, true);

    CMutableTransaction mtx;
    if (!DecodeHexTx(mtx, request.params[0].get_str(), true))
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed");

    std::vector<std::unique_ptr<CUSBDevice> > vDevices;
    CUSBDevice *pDevice = SelectDevice(vDevices);

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
            if (0 != pDevice->GetPubKey(pathkey.vPath, pathkey.pk, sError))
                throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("Device GetPubKey failed %s.", sError));

            tempKeystore.AddKey(pathkey);
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

    // Prepare transaction
    if (0 != pDevice->Open())
        throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("Failed to open dongle."));
    pDevice->PrepareTransaction(&txConst, view);
    if (!pDevice->sError.empty())
    {
        pDevice->Close();
        UniValue entry(UniValue::VOBJ);
        entry.push_back(Pair("error", pDevice->sError));
        vErrors.push_back(entry);
        UniValue result(UniValue::VOBJ);
        result.push_back(Pair("hex", EncodeHexTx(mtx)));
        if (!vErrors.empty()) {
            result.push_back(Pair("errors", vErrors));
        }
        return result;
    };


    // Sign what we can:
    for (unsigned int i = 0; i < mtx.vin.size(); i++) {
        CTxIn& txin = mtx.vin[i];
        const Coin& coin = view.AccessCoin(txin.prevout);
        if (coin.IsSpent()) {
            TxInErrorToJSON(txin, vErrors, "Input not found or already spent");
            continue;
        }
        if (coin.nType != OUTPUT_STANDARD)
        {
            pDevice->Close();
            throw JSONRPCError(RPC_MISC_ERROR, "TODO: make work for !StandardOutput");
        };

        std::vector<uint8_t> vchAmount(8);
        SignatureData sigdata;
        CScript prevPubKey = coin.out.scriptPubKey;
        CAmount amount = coin.out.nValue;
        memcpy(&vchAmount[0], &amount, 8);

        // Only sign SIGHASH_SINGLE if there's a corresponding output:
        if (!fHashSingle || (i < mtx.GetNumVOuts()))
        {
            pDevice->sError.clear();
            ProduceSignature(DeviceSignatureCreator(pDevice, &keystore, &txConst, i, vchAmount, nHashType), prevPubKey, sigdata);

            if (!pDevice->sError.empty())
            {
                UniValue entry(UniValue::VOBJ);
                entry.push_back(Pair("error", pDevice->sError));
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

    pDevice->Close();

    bool fComplete = vErrors.empty();

    UniValue result(UniValue::VOBJ);
    result.push_back(Pair("hex", EncodeHexTx(mtx)));
    result.push_back(Pair("complete", fComplete));
    if (!vErrors.empty()) {
        result.push_back(Pair("errors", vErrors));
    }

    return result;
};

#ifdef ENABLE_WALLET
UniValue initaccountfromdevice(const JSONRPCRequest &request)
{
    CHDWallet *pwallet = GetHDWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() > 5)
        throw std::runtime_error(
            "initaccountfromdevice (\"label\" \"path\" makedefault scan_chain_from)\n"
            "Initialise an extended key account from a hardware device.\n"
            + HelpRequiringPassphrase(pwallet) +
            "\nArguments:\n"
            "1. \"label\"             (string, optional) A label for the account.\n"
            "2. \"path\"              (string, optional) The path to derive the key from (default=\""+GetDefaultPath()+"\").\n"
            "                           The full path is \"accountpath\"/\"path\".\n"
            "3. makedefault           (bool, optional) Make the new account the default account for the wallet (default=true).\n"
            "4. scan_chain_from       (int, optional) Timestamp, scan the chain for incoming txns only on blocks after time (default=0).\n"
            "5. initstealthchain      (bool, optional) Prepare the account to generate stealthaddresses (default=true).\n"
            "                           The hardware device will need to sign a fake transaction to use as the seed for the scan chain.\n"
            "\nResult\n"
            "{\n"
            "  \"extkey\"           (string) The derived extended public key at \"path\".\n"
            "  \"path\"             (string) The full path used to derive the account.\n"
            "}\n"
            "\nExamples\n"
            + HelpExampleCli("initaccountfromdevice", "\"new_acc\" \"44h/1h/0h\" false")
            + HelpExampleRpc("initaccountfromdevice", "\"new_acc\""));

    ObserveSafeMode();

    RPCTypeCheck(request.params, {UniValue::VSTR, UniValue::VSTR, UniValue::VBOOL, UniValue::VNUM}, true);

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();

    EnsureWalletIsUnlocked(pwallet);

    std::vector<std::unique_ptr<CUSBDevice> > vDevices;
    CUSBDevice *pDevice = SelectDevice(vDevices);

    std::string sLabel;
    if (request.params[0].isStr())
        sLabel = request.params[0].get_str();

    std::vector<uint32_t> vPath;
    if (request.params[1].isStr())
    {
        GetPath(vPath, request.params[1], NullUniValue);
    } else
    {
        std::string sPath = GetDefaultPath();
        int rv;
        if ((rv = ExtractExtKeyPath(sPath, vPath)) != 0)
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Bad path: %s.", ExtKeyGetString(rv)));
    };

    bool fMakeDefault = request.params[2].isBool() ? request.params[2].get_bool() : true;
    int64_t nScanFrom = request.params[3].isNum() ? request.params[3].get_int64() : 0;
    bool fInitStealth = request.params[4].isBool() ? request.params[4].get_bool() : true;

    WalletRescanReserver reserver(pwallet);
    if (!reserver.reserve()) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Wallet is currently rescanning. Abort existing rescan or wait.");
    }

    std::string sError;
    CExtPubKey ekp;
    if (0 != pDevice->GetXPub(vPath, ekp, sError))
        throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("GetXPub failed %s.", sError));

    CKeyID idAccount = ekp.pubkey.GetID();

    {
        LOCK(pwallet->cs_wallet);
        CHDWalletDB wdb(pwallet->GetDBHandle(), "r+");

        CStoredExtKey checkSEA;
        if (wdb.ReadExtKey(idAccount, checkSEA))
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Account already exists in db.");


        CStoredExtKey *sekAccount = new CStoredExtKey();
        sekAccount->nFlags |= EAF_ACTIVE | EAF_IN_ACCOUNT | EAF_HARDWARE_DEVICE;
        sekAccount->kp = ekp;
        sekAccount->SetPath(vPath); // EKVT_PATH

        std::vector<uint8_t> vData;
        CExtKeyAccount *sea = new CExtKeyAccount();
        sea->sLabel = sLabel;
        sea->nFlags |= EAF_ACTIVE | EAF_HARDWARE_DEVICE;
        sea->mapValue[EKVT_CREATED_AT] = SetCompressedInt64(vData, GetTime());
        vData.clear();
        PushUInt32(vData, pDevice->pType->nVendorId);
        PushUInt32(vData, pDevice->pType->nProductId);
        sea->mapValue[EKVT_HARDWARE_DEVICE] = vData;
        sea->InsertChain(sekAccount);

        CExtPubKey epExternal, epInternal;
        uint32_t nExternal, nInternal;
        if (sekAccount->DeriveNextKey(epExternal, nExternal, false) != 0
            || sekAccount->DeriveNextKey(epInternal, nInternal, false) != 0)
        {
            sea->FreeChains();
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Could not derive account chain keys.");
        };

        std::vector<uint32_t> vChainPath;
        vChainPath.push_back(vPath.back()); // make relative to key before account
        CStoredExtKey *sekExternal = new CStoredExtKey();
        sekExternal->kp = epExternal;
        vChainPath.push_back(nExternal);
        sekExternal->SetPath(vChainPath);
        sekExternal->nFlags |= EAF_ACTIVE | EAF_RECEIVE_ON | EAF_IN_ACCOUNT | EAF_HARDWARE_DEVICE;
        sekExternal->mapValue[EKVT_KEY_TYPE] = SetChar(vData, EKT_EXTERNAL);
        sea->InsertChain(sekExternal);
        sea->nActiveExternal = sea->NumChains();

        CStoredExtKey *sekInternal = new CStoredExtKey();
        sekInternal->kp = epInternal;
        vChainPath.pop_back();
        vChainPath.push_back(nInternal);
        sekInternal->SetPath(vChainPath);
        sekInternal->nFlags |= EAF_ACTIVE | EAF_RECEIVE_ON | EAF_IN_ACCOUNT | EAF_HARDWARE_DEVICE;
        sekInternal->mapValue[EKVT_KEY_TYPE] = SetChar(vData, EKT_INTERNAL);
        sea->InsertChain(sekInternal);
        sea->nActiveInternal = sea->NumChains();

        if (fInitStealth)
        {
            // Generate a chain to use for generating scan secrets
            // Use a signed message as the seed so the scan chain is deterministic.
            // In this way if the wallet is locked it's not possible to regenerate the private key to the scan chain.

            std::string msg = "Scan chain secret seed";
            std::vector<uint8_t> vchSig;
            std::vector<uint32_t> vSigPath;
            if (!pwallet->GetFullChainPath(sea, sea->nActiveExternal, vSigPath))
            {
                sea->FreeChains();
                throw JSONRPCError(RPC_INTERNAL_ERROR, "GetFullChainPath failed.");
            };
            vSigPath.push_back(0);
            if (0 != pDevice->SignMessage(vSigPath, msg, vchSig, sError))
            {
                sea->FreeChains();
                throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("Could not generate scan chain seed from signed message %s.", sError));
            };
            vchSig.insert(vchSig.end(), sekExternal->kp.pubkey.begin(), sekExternal->kp.pubkey.end());

            CExtKey evStealthScan;
            evStealthScan.SetMaster(vchSig.data(), vchSig.size());

            CStoredExtKey *sekStealthScan = new CStoredExtKey();
            sekStealthScan->kp = evStealthScan;
            vSigPath.clear();
            //sekStealthSpend->SetPath(vSigPath);
            sekStealthScan->nFlags |= EAF_ACTIVE | EAF_IN_ACCOUNT;
            sekStealthScan->mapValue[EKVT_KEY_TYPE] = SetChar(vData, EKT_STEALTH_SCAN);
            sea->InsertChain(sekStealthScan);
            uint32_t nStealthScanChain = sea->NumChains();


            // Step over hardened chain 1 (stealth v1 chain)
            sekAccount->SetCounter(1, true);

            CExtPubKey epStealthSpend;
            uint32_t nStealthSpend;
            vPath.push_back(WithHardenedBit(CHAIN_NO_STEALTH_SPEND));
            if (0 != pDevice->GetXPub(vPath, epStealthSpend, sError))
            {
                sea->FreeChains();
                throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("GetXPub failed %s.", sError));
            };
            vPath.pop_back();

            CStoredExtKey *sekStealthSpend = new CStoredExtKey();
            sekStealthSpend->kp = epStealthSpend;
            vChainPath.pop_back();
            vChainPath.push_back(nStealthSpend);
            sekStealthSpend->SetPath(vChainPath);
            sekStealthSpend->nFlags |= EAF_ACTIVE | EAF_IN_ACCOUNT | EAF_HARDWARE_DEVICE;
            sekStealthSpend->mapValue[EKVT_KEY_TYPE] = SetChar(vData, EKT_STEALTH_SPEND);
            sea->InsertChain(sekStealthSpend);
            uint32_t nStealthSpendChain = sea->NumChains();

            sea->mapValue[EKVT_STEALTH_SCAN_CHAIN] = SetCompressedInt64(vData, nStealthScanChain);
            sea->mapValue[EKVT_STEALTH_SPEND_CHAIN] = SetCompressedInt64(vData, nStealthSpendChain);
        };

        if (!wdb.TxnBegin())
            throw std::runtime_error("TxnBegin failed.");

        if (0 != pwallet->ExtKeySaveAccountToDB(&wdb, idAccount, sea))
        {
            sea->FreeChains();
            wdb.TxnAbort();
            throw JSONRPCError(RPC_INTERNAL_ERROR, "DB Write failed.");
        };

        if (0 != pwallet->ExtKeyAddAccountToMaps(idAccount, sea))
        {
            sea->FreeChains();
            wdb.TxnAbort();
            throw JSONRPCError(RPC_INTERNAL_ERROR, "ExtKeyAddAccountToMaps failed.");
        };

        CKeyID idOldDefault = pwallet->idDefaultAccount;
        if (fMakeDefault)
        {
            CKeyID idNewDefaultAccount = sea->GetID();
            int rv;
            if (0 != (rv = pwallet->ExtKeySetDefaultAccount(&wdb, idNewDefaultAccount)))
            {
                pwallet->ExtKeyRemoveAccountFromMapsAndFree(sea);
                wdb.TxnAbort();
                throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("ExtKeySetDefaultAccount failed, %s.", ExtKeyGetString(rv)));
            };
        };

        if (!wdb.TxnCommit())
        {
            pwallet->idDefaultAccount = idOldDefault;
            pwallet->ExtKeyRemoveAccountFromMapsAndFree(sea);
            throw JSONRPCError(RPC_INTERNAL_ERROR, "TxnCommit failed.");
        };
    } // pwallet->cs_wallet

    pwallet->RescanFromTime(nScanFrom, reserver, true /* update */);
    pwallet->MarkDirty();
    pwallet->ReacceptWalletTransactions();

    std::string sPath;
    if (0 != PathToString(vPath, sPath))
        sPath = "error";
    UniValue result(UniValue::VOBJ);

    result.pushKV("extkey", CBitcoinExtPubKey(ekp).ToString());
    result.pushKV("path", sPath);
    result.pushKV("scanfrom", nScanFrom);

    return result;
};


UniValue devicegetnewstealthaddress(const JSONRPCRequest &request)
{
    CHDWallet *pwallet = GetHDWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp))
        return NullUniValue;

    if (request.fHelp || request.params.size() > 4)
        throw std::runtime_error(
            "devicegetnewstealthaddress [label] [num_prefix_bits] [prefix_num] [bech32]\n"
            "Returns a new Particl stealth address for receiving payments."
            + HelpRequiringPassphrase(pwallet) +
            "\nArguments:\n"
            "1. \"label\"             (string, optional) If specified the key is added to the address book.\n"
            "2. num_prefix_bits     (int, optional) If specified and > 0, the stealth address is created with a prefix.\n"
            "3. prefix_num          (int, optional) If prefix_num is not specified the prefix will be selected deterministically.\n"
            "           prefix_num can be specified in base2, 10 or 16, for base 2 prefix_num must begin with 0b, 0x for base16.\n"
            "           A 32bit integer will be created from prefix_num and the least significant num_prefix_bits will become the prefix.\n"
            "           A stealth address created without a prefix will scan all incoming stealth transactions, irrespective of transaction prefixes.\n"
            "           Stealth addresses with prefixes will scan only incoming stealth transactions with a matching prefix.\n"
            "4. bech32              (bool, optional) Use Bech32 encoding, default true.\n"
            "\nResult:\n"
            "\"address\"              (string) The new particl stealth address\n"
            "\nExamples:\n"
            + HelpExampleCli("devicegetnewstealthaddress", "\"lblTestSxAddrPrefix\" 3 \"0b101\"")
            + HelpExampleRpc("devicegetnewstealthaddress", "\"lblTestSxAddrPrefix\", 3, \"0b101\""));

    EnsureWalletIsUnlocked(pwallet);


    std::string sError, sLabel;
    if (request.params.size() > 0)
        sLabel = request.params[0].get_str();

    uint32_t num_prefix_bits = 0;
    if (request.params.size() > 1)
    {
        std::string sTemp = request.params[1].get_str();
        char *pend;
        errno = 0;
        num_prefix_bits = strtoul(sTemp.c_str(), &pend, 10);
        if (errno != 0 || !pend || *pend != '\0')
            throw JSONRPCError(RPC_INVALID_PARAMETER, _("num_prefix_bits invalid number."));
    };

    if (num_prefix_bits > 32)
        throw JSONRPCError(RPC_INVALID_PARAMETER, _("num_prefix_bits must be <= 32."));

    std::string sPrefix_num;
    if (request.params.size() > 2)
        sPrefix_num = request.params[2].get_str();

    bool fBech32 = request.params.size() > 3 ? request.params[3].get_bool() : true;


    CEKAStealthKey akStealth;
    CStealthAddress sxAddr;
    {
        LOCK(pwallet->cs_wallet);

        ExtKeyAccountMap::iterator mi = pwallet->mapExtAccounts.find(pwallet->idDefaultAccount);
        if (mi == pwallet->mapExtAccounts.end())
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Unknown account.");

        CExtKeyAccount *sea = mi->second;
        uint64_t nScanChain, nSpendChain;
        CStoredExtKey *sekScan = nullptr, *sekSpend = nullptr;
        mapEKValue_t::iterator mvi = sea->mapValue.find(EKVT_STEALTH_SCAN_CHAIN);
        if (mvi != sea->mapValue.end())
        {
            GetCompressedInt64(mvi->second, nScanChain);
            sekScan = sea->GetChain(nScanChain);
        };
        if (!sekScan)
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Unknown stealth scan chain.");

        mvi = sea->mapValue.find(EKVT_STEALTH_SPEND_CHAIN);
        if (mvi != sea->mapValue.end())
        {
            GetCompressedInt64(mvi->second, nSpendChain);
            sekSpend = sea->GetChain(nSpendChain);
        };
        if (!sekSpend)
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Unknown stealth spend chain.");


        uint32_t nSpendGenerated = sekSpend->nHGenerated;

        std::vector<uint32_t> vSpendPath;
        if (!pwallet->GetFullChainPath(sea, nSpendChain, vSpendPath))
            throw JSONRPCError(RPC_INTERNAL_ERROR, "GetFullChainPath failed.");
        vSpendPath.push_back(WithHardenedBit(nSpendGenerated));

        std::vector<std::unique_ptr<CUSBDevice> > vDevices;
        CUSBDevice *pDevice = SelectDevice(vDevices);

        CPubKey pkSpend;
        if (0 != pDevice->GetPubKey(vSpendPath, pkSpend, sError))
            throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("Device GetPubKey failed %s.", sError));

        sekSpend->nHGenerated = nSpendGenerated+1;


        CKey kScan;
        uint32_t nScanOut;
        if (0 != sekScan->DeriveNextKey(kScan, nScanOut, true))
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Derive failed.");

        uint32_t nPrefixBits = num_prefix_bits;
        uint32_t nPrefix = 0;
        const char *pPrefix = sPrefix_num.empty() ? nullptr : sPrefix_num.c_str();
        if (pPrefix)
        {
            if (!ExtractStealthPrefix(pPrefix, nPrefix))
                throw JSONRPCError(RPC_INTERNAL_ERROR, "ExtractStealthPrefix failed.");
        } else
        if (nPrefixBits > 0)
        {
            // If pPrefix is null, set nPrefix from the hash of kScan
            uint8_t tmp32[32];
            CSHA256().Write(kScan.begin(), 32).Finalize(tmp32);
            memcpy(&nPrefix, tmp32, 4);
        };

        uint32_t nMask = SetStealthMask(nPrefixBits);
        nPrefix = nPrefix & nMask;
        akStealth = CEKAStealthKey(nScanChain, nScanOut, kScan, nSpendChain, WithHardenedBit(nSpendGenerated), pkSpend, nPrefixBits, nPrefix);
        akStealth.sLabel = sLabel;

        if (0 != akStealth.SetSxAddr(sxAddr))
            throw JSONRPCError(RPC_INTERNAL_ERROR, "SetSxAddr failed.");


        CKeyID idAccount = sea->GetID();
        CHDWalletDB wdb(pwallet->GetDBHandle(), "r+");

        if (!wdb.TxnBegin())
            throw JSONRPCError(RPC_INTERNAL_ERROR, "TxnBegin failed.");

        if (0 != pwallet->SaveStealthAddress(&wdb, sea, akStealth, fBech32))
        {
            wdb.TxnAbort();
            pwallet->ExtKeyRemoveAccountFromMapsAndFree(idAccount);
            pwallet->ExtKeyLoadAccount(&wdb, idAccount);
            throw JSONRPCError(RPC_INTERNAL_ERROR, "SaveStealthAddress failed.");
        };

        if (!wdb.TxnCommit())
            throw JSONRPCError(RPC_INTERNAL_ERROR, "TxnCommit failed.");
    }

    pwallet->AddressBookChangedNotify(sxAddr, CT_NEW);

    return sxAddr.ToString(fBech32);
};
#endif

static const CRPCCommand commands[] =
{ //  category              name                            actor (function)            argNames
  //  --------------------- ------------------------        -----------------------     ----------
    { "usbdevice",          "listdevices",                  &listdevices,               {} },
    { "usbdevice",          "getdeviceinfo",                &getdeviceinfo,             {} },
    { "usbdevice",          "getdevicepublickey",           &getdevicepublickey,        {"path","accountpath"} },
    { "usbdevice",          "getdevicexpub",                &getdevicexpub,             {"path","accountpath"} },
    { "usbdevice",          "devicesignmessage",            &devicesignmessage,         {"path","message","accountpath"} },
    { "usbdevice",          "devicesignrawtransaction",     &devicesignrawtransaction,  {"hexstring","prevtxs","privkeypaths","sighashtype","accountpath"} }, /* uses wallet if enabled */
#ifdef ENABLE_WALLET
    { "usbdevice",          "initaccountfromdevice",        &initaccountfromdevice,     {"label","path","makedefault","scan_chain_from","initstealthchain"} },
    { "usbdevice",          "devicegetnewstealthaddress",   &devicegetnewstealthaddress,{"label","num_prefix_bits","prefix_num","bech32"} },
#endif
};

void RegisterUSBDeviceRPC(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
