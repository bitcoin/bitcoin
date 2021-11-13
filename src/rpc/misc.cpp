// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2014-2021 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <clientversion.h>
#include <consensus/consensus.h>
#include <core_io.h>
#include <evo/mnauth.h>
#include <httpserver.h>
#include <key_io.h>
#include <net.h>
#include <netbase.h>
#include <rpc/blockchain.h>
#include <rpc/server.h>
#include <rpc/util.h>
#include <script/descriptor.h>
#include <timedata.h>
#include <txmempool.h>
#include <util/system.h>
#include <util/strencodings.h>
#include <validation.h>
#include <util/validation.h>
#include <warnings.h>

#include <masternode/sync.h>
#include <spork.h>

#include <stdint.h>
#ifdef HAVE_MALLOC_INFO
#include <malloc.h>
#endif

#include <boost/algorithm/string.hpp>

#include <univalue.h>

static UniValue debug(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            RPCHelpMan{"debug",
                "Change debug category on the fly. Specify single category or use '+' to specify many.\n"
                "The valid debug categories are: " + ListLogCategories() + ".\n"
                "libevent logging is configured on startup and cannot be modified by this RPC during runtime.\n"
                "There are also a few meta-categories:\n"
                " - \"all\", \"1\" and \"\" activate all categories at once;\n"
                " - \"dash\" activates all Dash-specific categories at once;\n"
                " - \"none\" (or \"0\") deactivates all categories at once.\n"
                "Note: If specified category doesn't match any of the above, no error is thrown.\n",
                {
                    {"category", RPCArg::Type::STR, RPCArg::Optional::NO, "The name of the debug category to turn on."},
                },
                RPCResult {
            "  result               (string) \"Debug mode: \" followed by the specified category.\n"
                },
                RPCExamples {
                    HelpExampleCli("debug", "dash")
            + HelpExampleRpc("debug", "dash+net")
            }}.ToString());

    std::string strMode = request.params[0].get_str();
    LogInstance().DisableCategory(BCLog::ALL);

    std::vector<std::string> categories;
    boost::split(categories, strMode, boost::is_any_of("+"));

    if (std::find(categories.begin(), categories.end(), std::string("0")) == categories.end()) {
        for (const auto& cat : categories) {
            LogInstance().EnableCategory(cat);
        }
    }

    return "Debug mode: " + ListActiveLogCategoriesString();
}

static UniValue mnsync(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            RPCHelpMan{"mnsync",
                "Returns the sync status, updates to the next step or resets it entirely.\n",
                {
                    {"mode", RPCArg::Type::STR, RPCArg::Optional::NO, "[status|next|reset]"},
                },
                RPCResults{},
                RPCExamples{""}
            }.ToString());

    std::string strMode = request.params[0].get_str();

    if(strMode == "status") {
        UniValue objStatus(UniValue::VOBJ);
        objStatus.pushKV("AssetID", masternodeSync.GetAssetID());
        objStatus.pushKV("AssetName", masternodeSync.GetAssetName());
        objStatus.pushKV("AssetStartTime", masternodeSync.GetAssetStartTime());
        objStatus.pushKV("Attempt", masternodeSync.GetAttempt());
        objStatus.pushKV("IsBlockchainSynced", masternodeSync.IsBlockchainSynced());
        objStatus.pushKV("IsSynced", masternodeSync.IsSynced());
        return objStatus;
    }

    if(strMode == "next")
    {
        masternodeSync.SwitchToNextAsset(*g_connman);
        return "sync updated to " + masternodeSync.GetAssetName();
    }

    if(strMode == "reset")
    {
        masternodeSync.Reset(true);
        return "success";
    }
    return "failure";
}

/*
    Used for updating/reading spork settings on the network
*/
static UniValue spork(const JSONRPCRequest& request)
{
    if (request.params.size() == 1) {
        // basic mode, show info
        std:: string strCommand = request.params[0].get_str();
        if (strCommand == "show") {
            UniValue ret(UniValue::VOBJ);
            for (const auto& sporkDef : sporkDefs) {
                ret.pushKV(std::string(sporkDef.name), sporkManager.GetSporkValue(sporkDef.sporkId));
            }
            return ret;
        } else if(strCommand == "active"){
            UniValue ret(UniValue::VOBJ);
            for (const auto& sporkDef : sporkDefs) {
                ret.pushKV(std::string(sporkDef.name), sporkManager.IsSporkActive(sporkDef.sporkId));
            }
            return ret;
        }
    }

    if (request.fHelp || request.params.size() != 2) {
        // default help, for basic mode
        throw std::runtime_error(
            RPCHelpMan{"spork",
                "\nShows information about current state of sporks\n",
                {
                    {"command", RPCArg::Type::STR, RPCArg::Optional::NO, "'show' to show all current spork values, 'active' to show which sporks are active"},
                },
                RPCResults {
                    {"For 'show'",
            "{\n"
            "  \"SPORK_NAME\" : spork_value,    (number) The value of the specific spork with the name SPORK_NAME\n"
            "  ...\n"
            "}\n"
                    }, {"For 'active'",
            "{\n"
            "  \"SPORK_NAME\" : true|false,     (boolean) 'true' for time-based sporks if spork is active and 'false' otherwise\n"
            "  ...\n"
            "}\n"
                }},
                RPCExamples {
                    HelpExampleCli("spork", "show")
                    + HelpExampleRpc("spork", "\"show\"")
            }}.ToString());
    } else {
        // advanced mode, update spork values
        SporkId nSporkID = CSporkManager::GetSporkIDByName(request.params[0].get_str());
        if(nSporkID == SPORK_INVALID)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid spork name");

        if (!g_connman)
            throw JSONRPCError(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");

        // SPORK VALUE
        int64_t nValue = request.params[1].get_int64();

        //broadcast new spork
        if(sporkManager.UpdateSpork(nSporkID, nValue, *g_connman)){
            return "success";
        } else {
            throw std::runtime_error(
                RPCHelpMan{"spork",
                    "\nUpdate the value of the specific spork. Requires \"-sporkkey\" to be set to sign the message.\n",
                    {
                        {"name", RPCArg::Type::STR, RPCArg::Optional::NO, "The name of the spork to update"},
                        {"value", RPCArg::Type::NUM, RPCArg::Optional::NO, "The new desired value of the spork"},
                    },
                    RPCResult{
                "  result               (string) \"success\" if spork value was updated or this help otherwise\n"
                    },
                    RPCExamples{
                        HelpExampleCli("spork", "SPORK_2_INSTANTSEND_ENABLED 4070908800")
                + HelpExampleRpc("spork", "\"SPORK_2_INSTANTSEND_ENABLED\", 4070908800")
                    },
                }.ToString());
        }
    }

}

static UniValue validateaddress(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            RPCHelpMan{"validateaddress",
                "\nReturn information about the given dash address.\n",
                {
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The dash address to validate"},
                },
                RPCResult{
            "{\n"
            "  \"isvalid\" : true|false,       (boolean) If the address is valid or not. If not, this is the only property returned.\n"
            "  \"address\" : \"address\", (string) The dash address validated\n"
            "  \"scriptPubKey\" : \"hex\",       (string) The hex-encoded scriptPubKey generated by the address\n"
            "  \"isscript\" : true|false,      (boolean) If the key is a script\n"
            "}\n"
                },
                RPCExamples{
                    HelpExampleCli("validateaddress", "\"XwnLY9Tf7Zsef8gMGL2fhWA9ZmMjt4KPwg\"")
            + HelpExampleRpc("validateaddress", "\"XwnLY9Tf7Zsef8gMGL2fhWA9ZmMjt4KPwg\"")
                },
            }.ToString());

    CTxDestination dest = DecodeDestination(request.params[0].get_str());
    bool isValid = IsValidDestination(dest);

    UniValue ret(UniValue::VOBJ);
    ret.pushKV("isvalid", isValid);
    if (isValid)
    {
        std::string currentAddress = EncodeDestination(dest);
        ret.pushKV("address", currentAddress);

        CScript scriptPubKey = GetScriptForDestination(dest);
        ret.pushKV("scriptPubKey", HexStr(scriptPubKey));;

        UniValue detail = DescribeAddress(dest);
        ret.pushKVs(detail);
    }
    return ret;
}

static UniValue createmultisig(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 2 || request.params.size() > 2)
    {
        std::string msg =
            RPCHelpMan{"createmultisig",
                "\nCreates a multi-signature address with n signature of m keys required.\n"
                "It returns a json object with the address and redeemScript.\n",
                {
                    {"nrequired", RPCArg::Type::NUM, RPCArg::Optional::NO, "The number of required signatures out of the n keys."},
                    {"keys", RPCArg::Type::ARR, RPCArg::Optional::NO, "A json array of hex-encoded public keys.",
                        {
                            {"key", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, "The hex-encoded public key"},
                        }},
                },
                RPCResult{
            "{\n"
            "  \"address\":\"multisigaddress\",  (string) The value of the new multisig address.\n"
            "  \"redeemScript\":\"script\"       (string) The string value of the hex-encoded redemption script.\n"
            "}\n"
                },
                RPCExamples{
            "\nCreate a multisig address from 2 public keys\n"
            + HelpExampleCli("createmultisig", "2 \"[\\\"03789ed0bb717d88f7d321a368d905e7430207ebbd82bd342cf11ae157a7ace5fd\\\",\\\"03dbc6764b8884a92e871274b87583e6d5c2a58819473e17e107ef3f6aa5a61626\\\"]\"") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("createmultisig", "2, \"[\\\"03789ed0bb717d88f7d321a368d905e7430207ebbd82bd342cf11ae157a7ace5fd\\\",\\\"03dbc6764b8884a92e871274b87583e6d5c2a58819473e17e107ef3f6aa5a61626\\\"]\"")
                },
            }.ToString();
        throw std::runtime_error(msg);
    }

    int required = request.params[0].get_int();

    // Get the public keys
    const UniValue& keys = request.params[1].get_array();
    std::vector<CPubKey> pubkeys;
    for (unsigned int i = 0; i < keys.size(); ++i) {
        if (IsHex(keys[i].get_str()) && (keys[i].get_str().length() == 66 || keys[i].get_str().length() == 130)) {
            pubkeys.push_back(HexToPubKey(keys[i].get_str()));
        } else {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("Invalid public key: %s\n.", keys[i].get_str()));
        }
    }

    // Construct using pay-to-script-hash:
    CScript inner = CreateMultisigRedeemscript(required, pubkeys);
    CScriptID innerID(inner);

    UniValue result(UniValue::VOBJ);
    result.pushKV("address", EncodeDestination(innerID));
    result.pushKV("redeemScript", HexStr(inner));

    return result;
}

UniValue getdescriptorinfo(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1) {
        throw std::runtime_error(
            RPCHelpMan{"getdescriptorinfo",
            {"\nAnalyses a descriptor.\n"},
            {
                {"descriptor", RPCArg::Type::STR, RPCArg::Optional::NO, "The descriptor"},
            },
            RPCResult{
            "{\n"
            "  \"descriptor\" : \"desc\",         (string) The descriptor in canonical form, without private keys\n"
            "  \"checksum\" : \"chksum\",         (string) The checksum for the input descriptor\n"
            "  \"isrange\" : true|false,        (boolean) Whether the descriptor is ranged\n"
            "  \"issolvable\" : true|false,     (boolean) Whether the descriptor is solvable\n"
            "  \"hasprivatekeys\" : true|false, (boolean) Whether the input descriptor contained at least one private key\n"
            "}\n"
            },
            RPCExamples{
            "\nAnalyse a descriptor\n"
            + HelpExampleCli("getdescriptorinfo", "\"pkh([d34db33f/84h/0h/0h]0279be667ef9dcbbac55a06295Ce870b07029Bfcdb2dce28d959f2815b16f81798)\"")
            }
        }.ToString());
    }

    RPCTypeCheck(request.params, {UniValue::VSTR});

    FlatSigningProvider provider;
    auto desc = Parse(request.params[0].get_str(), provider);
    if (!desc) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("Invalid descriptor"));
    }

    UniValue result(UniValue::VOBJ);
    result.pushKV("descriptor", desc->ToString());
    result.pushKV("checksum", GetDescriptorChecksum(request.params[0].get_str()));
    result.pushKV("isrange", desc->IsRange());
    result.pushKV("issolvable", desc->IsSolvable());
    result.pushKV("hasprivatekeys", provider.keys.size() > 0);
    return result;
}

UniValue deriveaddresses(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.empty() || request.params.size() > 3) {
        throw std::runtime_error(
            RPCHelpMan{"deriveaddresses",
            "\nDerives one or more addresses corresponding to an output descriptor.\n"
            "Examples of output descriptors are:\n"
            "    pkh(<pubkey>)                        P2PKH outputs for the given pubkey\n"
            "    sh(multi(<n>,<pubkey>,<pubkey>,...)) P2SH-multisig outputs for the given threshold and pubkeys\n"
            "    raw(<hex script>)                    Outputs whose scriptPubKey equals the specified hex scripts\n"
            "\nIn the above, <pubkey> either refers to a fixed public key in hexadecimal notation, or to an xpub/xprv optionally followed by one\n"
            "or more path elements separated by \"/\", where \"h\" represents a hardened child key.\n"
            "For more information on output descriptors, see the documentation in the doc/descriptors.md file.\n",
                {
                    {"descriptor", RPCArg::Type::STR, RPCArg::Optional::NO, "The descriptor"},
                    {"begin", RPCArg::Type::NUM, /* default */ "", "If a ranged descriptor is used, this specifies the beginning of the range to import"},
                    {"end", RPCArg::Type::NUM, /* default */ "", "If a ranged descriptor is used, this specifies the end of the range to import"}
                },
                RPCResult{
            "\"address\"        (array) A json array of the derived addresses\n"
            "     [\n"
            "       ...\n"
            "     ]\n"
                },
                RPCExamples{
            "\nFirst three receive addresses\n"
            + HelpExampleCli("deriveaddresses", "\"pkh([d34db33f/84h/0h/0h]xpub6DJ2dNUysrn5Vt36jH2KLBT2i1auw1tTSSomg8PhqNiUtx8QX2SvC9nrHu81fT41fvDUnhMjEzQgXnQjKEu3oaqMSzhSrHMxyyoEAmUHQbY/0/*)#trd0mf0l\" 0 2")
                }
        }.ToString());
    }

    RPCTypeCheck(request.params, {UniValue::VSTR, UniValue::VNUM, UniValue::VNUM});
    const std::string desc_str = request.params[0].get_str();

    int range_begin = 0;
    int range_end = 0;

    if (request.params.size() >= 2) {
        if (request.params.size() == 2) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Missing range end parameter");
        }
        range_begin = request.params[1].get_int();
        range_end = request.params[2].get_int();
        if (range_begin < 0) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Range should be greater or equal than 0");
        }
        if (range_begin > range_end) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Range end should be equal to or greater than begin");
        }
    }

    FlatSigningProvider key_provider;
    auto desc = Parse(desc_str, key_provider, /* require_checksum = */ true);
    if (!desc) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("Invalid descriptor"));
    }

    if (!desc->IsRange() && request.params.size() > 1) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Range should not be specified for an un-ranged descriptor");
    }

    if (desc->IsRange() && request.params.size() == 1) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Range must be specified for a ranged descriptor");
    }

    UniValue addresses(UniValue::VARR);

    for (int i = range_begin; i <= range_end; ++i) {
        FlatSigningProvider provider;
        std::vector<CScript> scripts;
        if (!desc->Expand(i, key_provider, scripts, provider)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("Cannot derive script without private keys"));
        }

        for (const CScript &script : scripts) {
            CTxDestination dest;
            if (!ExtractDestination(script, dest)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("Descriptor does not have a corresponding address"));
            }

            addresses.push_back(EncodeDestination(dest));
        }
    }

    // This should not be possible, but an assert seems overkill:
    if (addresses.empty()) {
        throw JSONRPCError(RPC_MISC_ERROR, "Unexpected empty result");
    }

    return addresses;
}

static UniValue verifymessage(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 3)
        throw std::runtime_error(
            RPCHelpMan{"verifymessage",
                "\nVerify a signed message\n",
                {
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The dash address to use for the signature."},
                    {"signature", RPCArg::Type::STR, RPCArg::Optional::NO, "The signature provided by the signer in base 64 encoding (see signmessage)."},
                    {"message", RPCArg::Type::STR, RPCArg::Optional::NO, "The message that was signed."},
                },
                RPCResult{
            "true|false   (boolean) If the signature is verified or not.\n"
                },
                RPCExamples{
            "\nUnlock the wallet for 30 seconds\n"
            + HelpExampleCli("walletpassphrase", "\"mypassphrase\" 30") +
            "\nCreate the signature\n"
            + HelpExampleCli("signmessage", "\"XwnLY9Tf7Zsef8gMGL2fhWA9ZmMjt4KPwG\" \"my message\"") +
            "\nVerify the signature\n"
            + HelpExampleCli("verifymessage", "\"XwnLY9Tf7Zsef8gMGL2fhWA9ZmMjt4KPwG\" \"signature\" \"my message\"") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("verifymessage", "\"XwnLY9Tf7Zsef8gMGL2fhWA9ZmMjt4KPwG\", \"signature\", \"my message\"")
                },
            }.ToString());

    LOCK(cs_main);

    std::string strAddress  = request.params[0].get_str();
    std::string strSign     = request.params[1].get_str();
    std::string strMessage  = request.params[2].get_str();

    CTxDestination destination = DecodeDestination(strAddress);
    if (!IsValidDestination(destination)) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid address");
    }

    const CKeyID *keyID = boost::get<CKeyID>(&destination);
    if (!keyID) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Address does not refer to key");
    }

    bool fInvalid = false;
    std::vector<unsigned char> vchSig = DecodeBase64(strSign.c_str(), &fInvalid);

    if (fInvalid)
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Malformed base64 encoding");

    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMessage;

    CPubKey pubkey;
    if (!pubkey.RecoverCompact(ss.GetHash(), vchSig))
        return false;

    return (pubkey.GetID() == *keyID);
}

static UniValue signmessagewithprivkey(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 2)
        throw std::runtime_error(
            RPCHelpMan{"signmessagewithprivkey",
                "\nSign a message with the private key of an address\n",
                {
                    {"privkey", RPCArg::Type::STR, RPCArg::Optional::NO, "The private key to sign the message with."},
                    {"message", RPCArg::Type::STR, RPCArg::Optional::NO, "The message to create a signature of."},
                },
                RPCResult{
            "\"signature\"          (string) The signature of the message encoded in base 64\n"
                },
                RPCExamples{
            "\nCreate the signature\n"
            + HelpExampleCli("signmessagewithprivkey", "\"privkey\" \"my message\"") +
            "\nVerify the signature\n"
            + HelpExampleCli("verifymessage", "\"XwnLY9Tf7Zsef8gMGL2fhWA9ZmMjt4KPwG\" \"signature\" \"my message\"") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("signmessagewithprivkey", "\"privkey\", \"my message\"")
                },
            }.ToString());

    std::string strPrivkey = request.params[0].get_str();
    std::string strMessage = request.params[1].get_str();

    CKey key = DecodeSecret(strPrivkey);
    if (!key.IsValid()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key");
    }

    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMessage;

    std::vector<unsigned char> vchSig;
    if (!key.SignCompact(ss.GetHash(), vchSig))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Sign failed");

    return EncodeBase64(vchSig);
}

static UniValue setmocktime(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            RPCHelpMan{"setmocktime",
                "\nSet the local time to given timestamp (-regtest only)\n",
                {
                    {"timestamp", RPCArg::Type::NUM, RPCArg::Optional::NO, "Unix seconds-since-epoch timestamp\n"
            "   Pass 0 to go back to using the system time."},
                },
                RPCResults{},
                RPCExamples{""},
            }.ToString()
        );

    if (!Params().MineBlocksOnDemand())
        throw std::runtime_error("setmocktime for regression testing (-regtest mode) only");

    // For now, don't change mocktime if we're in the middle of validation, as
    // this could have an effect on mempool time-based eviction, as well as
    // IsCurrentForFeeEstimation() and IsInitialBlockDownload().
    // TODO: figure out the right way to synchronize around mocktime, and
    // ensure all call sites of GetTime() are accessing this safely.
    LOCK(cs_main);

    RPCTypeCheck(request.params, {UniValue::VNUM});
    SetMockTime(request.params[0].get_int64());

    return NullUniValue;
}

static UniValue mnauth(const JSONRPCRequest& request)
{
    if (request.fHelp || (request.params.size() != 3))
        throw std::runtime_error(
            RPCHelpMan{"mnauth",
                "\nOverride MNAUTH processing results for the specified node with a user provided data (-regtest only).\n",
                {
                    {"nodeId", RPCArg::Type::NUM, RPCArg::Optional::NO, "Internal peer id of the node the mock data gets added to."},
                    {"proTxHash", RPCArg::Type::STR, RPCArg::Optional::NO, "The authenticated proTxHash as hex string."},
                    {"publicKey", RPCArg::Type::STR, RPCArg::Optional::NO, "The authenticated public key as hex string."},
                },
                RPCResults{},
                RPCExamples{""},
            }.ToString());

    if (!Params().MineBlocksOnDemand())
        throw std::runtime_error("mnauth for regression testing (-regtest mode) only");

    int nodeId = ParseInt64V(request.params[0], "nodeId");
    uint256 proTxHash = ParseHashV(request.params[1], "proTxHash");
    if (proTxHash.IsNull()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "proTxHash invalid");
    }
    CBLSPublicKey publicKey;
    publicKey.SetHexStr(request.params[2].get_str());
    if (!publicKey.IsValid()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "publicKey invalid");
    }

    bool fSuccess = g_connman->ForNode(nodeId, CConnman::AllNodes, [&](CNode* pNode){
        pNode->SetVerifiedProRegTxHash(proTxHash);
        pNode->SetVerifiedPubKeyHash(publicKey.GetHash());
        return true;
    });

    return fSuccess;
}

static bool getAddressFromIndex(const int &type, const uint160 &hash, std::string &address)
{
    if (type == 2) {
        address = EncodeDestination(CScriptID(hash));
    } else if (type == 1) {
        address = EncodeDestination(CKeyID(hash));
    } else {
        return false;
    }
    return true;
}

static bool getIndexKey(const std::string& str, uint160& hashBytes, int& type)
{
    CTxDestination dest = DecodeDestination(str);
    if (!IsValidDestination(dest)) {
        type = 0;
        return false;
    }
    const CKeyID *keyID = boost::get<CKeyID>(&dest);
    const CScriptID *scriptID = boost::get<CScriptID>(&dest);
    type = keyID ? 1 : 2;
    hashBytes = keyID ? *keyID : *scriptID;
    return true;
}

static bool getAddressesFromParams(const UniValue& params, std::vector<std::pair<uint160, int> > &addresses)
{
    if (params[0].isStr()) {
        uint160 hashBytes;
        int type = 0;
        if (!getIndexKey(params[0].get_str(), hashBytes, type)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
        }
        addresses.push_back(std::make_pair(hashBytes, type));
    } else if (params[0].isObject()) {

        UniValue addressValues = find_value(params[0].get_obj(), "addresses");
        if (!addressValues.isArray()) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Addresses is expected to be an array");
        }

        std::vector<UniValue> values = addressValues.getValues();

        for (std::vector<UniValue>::iterator it = values.begin(); it != values.end(); ++it) {

            uint160 hashBytes;
            int type = 0;
            if (!getIndexKey(it->get_str(), hashBytes, type)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
            }
            addresses.push_back(std::make_pair(hashBytes, type));
        }
    } else {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    return true;
}

static bool heightSort(std::pair<CAddressUnspentKey, CAddressUnspentValue> a,
                std::pair<CAddressUnspentKey, CAddressUnspentValue> b) {
    return a.second.blockHeight < b.second.blockHeight;
}

static bool timestampSort(std::pair<CMempoolAddressDeltaKey, CMempoolAddressDelta> a,
                   std::pair<CMempoolAddressDeltaKey, CMempoolAddressDelta> b) {
    return a.second.time < b.second.time;
}

static UniValue getaddressmempool(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            RPCHelpMan{"getaddressmempool",
                "\nReturns all mempool deltas for an address (requires addressindex to be enabled).\n",
                {
                    {"addresses", RPCArg::Type::ARR, /* default */ "", "",
                        {
                            {"address", RPCArg::Type::STR, /* default */ "", "The base58check encoded address"},
                        },
                    },
                },
                RPCResult{
            "[\n"
            "  {\n"
            "    \"address\"  (string) The base58check encoded address\n"
            "    \"txid\"  (string) The related txid\n"
            "    \"index\"  (number) The related input or output index\n"
            "    \"satoshis\"  (number) The difference of duffs\n"
            "    \"timestamp\"  (number) The time the transaction entered the mempool (seconds)\n"
            "    \"prevtxid\"  (string) The previous txid (if spending)\n"
            "    \"prevout\"  (string) The previous transaction output index (if spending)\n"
            "  }\n"
            "]\n"
                },
                RPCExamples{
                    HelpExampleCli("getaddressmempool", "'{\"addresses\": [\"XwnLY9Tf7Zsef8gMGL2fhWA9ZmMjt4KPwg\"]}'")
            + HelpExampleRpc("getaddressmempool", "{\"addresses\": [\"XwnLY9Tf7Zsef8gMGL2fhWA9ZmMjt4KPwg\"]}")
                },
            }.ToString());

    std::vector<std::pair<uint160, int> > addresses;

    if (!getAddressesFromParams(request.params, addresses)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    std::vector<std::pair<CMempoolAddressDeltaKey, CMempoolAddressDelta> > indexes;

    if (!mempool.getAddressIndex(addresses, indexes)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
    }

    std::sort(indexes.begin(), indexes.end(), timestampSort);

    UniValue result(UniValue::VARR);

    for (std::vector<std::pair<CMempoolAddressDeltaKey, CMempoolAddressDelta> >::iterator it = indexes.begin();
         it != indexes.end(); it++) {

        std::string address;
        if (!getAddressFromIndex(it->first.type, it->first.addressBytes, address)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unknown address type");
        }

        UniValue delta(UniValue::VOBJ);
        delta.pushKV("address", address);
        delta.pushKV("txid", it->first.txhash.GetHex());
        delta.pushKV("index", (int)it->first.index);
        delta.pushKV("satoshis", it->second.amount);
        delta.pushKV("timestamp", it->second.time);
        if (it->second.amount < 0) {
            delta.pushKV("prevtxid", it->second.prevhash.GetHex());
            delta.pushKV("prevout", (int)it->second.prevout);
        }
        result.push_back(delta);
    }

    return result;
}

static UniValue getaddressutxos(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            RPCHelpMan{"getaddressutxos",
                "\nReturns all unspent outputs for an address (requires addressindex to be enabled).\n",
                {
                    {"addresses", RPCArg::Type::ARR, /* default */ "", "",
                        {
                            {"address", RPCArg::Type::STR, /* default */ "", "The base58check encoded address"},
                        },
                    },
                },
                RPCResult{
            "[\n"
            "  {\n"
            "    \"address\"  (string) The address base58check encoded\n"
            "    \"txid\"  (string) The output txid\n"
            "    \"outputIndex\"  (number) The output index\n"
            "    \"script\"  (string) The script hex-encoded\n"
            "    \"satoshis\"  (number) The number of duffs of the output\n"
            "    \"height\"  (number) The block height\n"
            "  }\n"
            "]\n"
                },
                RPCExamples{
                    HelpExampleCli("getaddressutxos", "'{\"addresses\": [\"XwnLY9Tf7Zsef8gMGL2fhWA9ZmMjt4KPwg\"]}'")
            + HelpExampleRpc("getaddressutxos", "{\"addresses\": [\"XwnLY9Tf7Zsef8gMGL2fhWA9ZmMjt4KPwg\"]}")
                },
            }.ToString());

    std::vector<std::pair<uint160, int> > addresses;

    if (!getAddressesFromParams(request.params, addresses)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > unspentOutputs;

    for (std::vector<std::pair<uint160, int> >::iterator it = addresses.begin(); it != addresses.end(); it++) {
        if (!GetAddressUnspent((*it).first, (*it).second, unspentOutputs)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
        }
    }

    std::sort(unspentOutputs.begin(), unspentOutputs.end(), heightSort);

    UniValue result(UniValue::VARR);

    for (std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> >::const_iterator it=unspentOutputs.begin(); it!=unspentOutputs.end(); it++) {
        UniValue output(UniValue::VOBJ);
        std::string address;
        if (!getAddressFromIndex(it->first.type, it->first.hashBytes, address)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unknown address type");
        }

        output.pushKV("address", address);
        output.pushKV("txid", it->first.txhash.GetHex());
        output.pushKV("outputIndex", (int)it->first.index);
        output.pushKV("script", HexStr(it->second.script));
        output.pushKV("satoshis", it->second.satoshis);
        output.pushKV("height", it->second.blockHeight);
        result.push_back(output);
    }

    return result;
}

static UniValue getaddressdeltas(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1 || !request.params[0].isObject())
        throw std::runtime_error(
            RPCHelpMan{"getaddressdeltas",
                "\nReturns all changes for an address (requires addressindex to be enabled).\n",
                {
                    {"addresses", RPCArg::Type::ARR, /* default */ "", "",
                        {
                            {"address", RPCArg::Type::STR, /* default */ "", "The base58check encoded address"},
                        },
                    },
                },
                RPCResult{
            "[\n"
            "  {\n"
            "    \"satoshis\"  (number) The difference of duffs\n"
            "    \"txid\"  (string) The related txid\n"
            "    \"index\"  (number) The related input or output index\n"
            "    \"blockindex\"  (number) The related block index\n"
            "    \"height\"  (number) The block height\n"
            "    \"address\"  (string) The base58check encoded address\n"
            "  }\n"
            "]\n"
                },
                RPCExamples{
                    HelpExampleCli("getaddressdeltas", "'{\"addresses\": [\"XwnLY9Tf7Zsef8gMGL2fhWA9ZmMjt4KPwg\"]}'")
            + HelpExampleRpc("getaddressdeltas", "{\"addresses\": [\"XwnLY9Tf7Zsef8gMGL2fhWA9ZmMjt4KPwg\"]}")
                },
            }.ToString());


    UniValue startValue = find_value(request.params[0].get_obj(), "start");
    UniValue endValue = find_value(request.params[0].get_obj(), "end");

    int start = 0;
    int end = 0;

    if (startValue.isNum() && endValue.isNum()) {
        start = startValue.get_int();
        end = endValue.get_int();
        if (end < start) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "End value is expected to be greater than start");
        }
    }

    std::vector<std::pair<uint160, int> > addresses;

    if (!getAddressesFromParams(request.params, addresses)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    std::vector<std::pair<CAddressIndexKey, CAmount> > addressIndex;

    for (std::vector<std::pair<uint160, int> >::iterator it = addresses.begin(); it != addresses.end(); it++) {
        if (start > 0 && end > 0) {
            if (!GetAddressIndex((*it).first, (*it).second, addressIndex, start, end)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
            }
        } else {
            if (!GetAddressIndex((*it).first, (*it).second, addressIndex)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
            }
        }
    }

    UniValue result(UniValue::VARR);

    for (std::vector<std::pair<CAddressIndexKey, CAmount> >::const_iterator it=addressIndex.begin(); it!=addressIndex.end(); it++) {
        std::string address;
        if (!getAddressFromIndex(it->first.type, it->first.hashBytes, address)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unknown address type");
        }

        UniValue delta(UniValue::VOBJ);
        delta.pushKV("satoshis", it->second);
        delta.pushKV("txid", it->first.txhash.GetHex());
        delta.pushKV("index", (int)it->first.index);
        delta.pushKV("blockindex", (int)it->first.txindex);
        delta.pushKV("height", it->first.blockHeight);
        delta.pushKV("address", address);
        result.push_back(delta);
    }

    return result;
}

static UniValue getaddressbalance(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            RPCHelpMan{"getaddressbalance",
                "\nReturns the balance for an address(es) (requires addressindex to be enabled).\n",
                {
                    {"addresses", RPCArg::Type::ARR, /* default */ "", "",
                        {
                            {"address", RPCArg::Type::STR, /* default */ "", "The base58check encoded address"},
                        },
                    },
                },
                RPCResult{
            "{\n"
            "  \"balance\": xxxxx,              (numeric) The current total balance in duffs\n"
            "  \"balance_immature\": xxxxx,     (numeric) The current immature balance in duffs\n"
            "  \"balance_spendable\": xxxxx,    (numeric) The current spendable balance in duffs\n"
            "  \"received\": xxxxx              (numeric) The total number of duffs received (including change)\n"
            "}\n"
                },
                RPCExamples{
                    HelpExampleCli("getaddressbalance", "'{\"addresses\": [\"XwnLY9Tf7Zsef8gMGL2fhWA9ZmMjt4KPwg\"]}'")
            + HelpExampleRpc("getaddressbalance", "{\"addresses\": [\"XwnLY9Tf7Zsef8gMGL2fhWA9ZmMjt4KPwg\"]}")
                },
            }.ToString());

    std::vector<std::pair<uint160, int> > addresses;

    if (!getAddressesFromParams(request.params, addresses)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    std::vector<std::pair<CAddressIndexKey, CAmount> > addressIndex;

    for (std::vector<std::pair<uint160, int> >::iterator it = addresses.begin(); it != addresses.end(); it++) {
        if (!GetAddressIndex((*it).first, (*it).second, addressIndex)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
        }
    }

    int nHeight = WITH_LOCK(cs_main, return ::ChainActive().Height());

    CAmount balance = 0;
    CAmount balance_spendable = 0;
    CAmount balance_immature = 0;
    CAmount received = 0;

    for (std::vector<std::pair<CAddressIndexKey, CAmount> >::const_iterator it=addressIndex.begin(); it!=addressIndex.end(); it++) {
        if (it->second > 0) {
            received += it->second;
        }
        if (it->first.txindex == 0 && nHeight - it->first.blockHeight < COINBASE_MATURITY) {
            balance_immature += it->second;
        } else {
            balance_spendable += it->second;
        }
        balance += it->second;
    }

    UniValue result(UniValue::VOBJ);
    result.pushKV("balance", balance);
    result.pushKV("balance_immature", balance_immature);
    result.pushKV("balance_spendable", balance_spendable);
    result.pushKV("received", received);

    return result;

}

static UniValue getaddresstxids(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            RPCHelpMan{"getaddresstxids",
                "\nReturns the txids for an address(es) (requires addressindex to be enabled).\n",
                {
                    {"addresses", RPCArg::Type::ARR, /* default */ "", "",
                        {
                            {"address", RPCArg::Type::STR, /* default */ "", "The base58check encoded address"},
                        },
                    },
                },
                RPCResult{
            "[\n"
            "  \"transactionid\"  (string) The transaction id\n"
            "  ,...\n"
            "]\n"
                },
                RPCExamples{
                    HelpExampleCli("getaddresstxids", "'{\"addresses\": [\"XwnLY9Tf7Zsef8gMGL2fhWA9ZmMjt4KPwg\"]}'")
            + HelpExampleRpc("getaddresstxids", "{\"addresses\": [\"XwnLY9Tf7Zsef8gMGL2fhWA9ZmMjt4KPwg\"]}")
                },
            }.ToString());

    std::vector<std::pair<uint160, int> > addresses;

    if (!getAddressesFromParams(request.params, addresses)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    int start = 0;
    int end = 0;
    if (request.params[0].isObject()) {
        UniValue startValue = find_value(request.params[0].get_obj(), "start");
        UniValue endValue = find_value(request.params[0].get_obj(), "end");
        if (startValue.isNum() && endValue.isNum()) {
            start = startValue.get_int();
            end = endValue.get_int();
        }
    }

    std::vector<std::pair<CAddressIndexKey, CAmount> > addressIndex;

    for (std::vector<std::pair<uint160, int> >::iterator it = addresses.begin(); it != addresses.end(); it++) {
        if (start > 0 && end > 0) {
            if (!GetAddressIndex((*it).first, (*it).second, addressIndex, start, end)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
            }
        } else {
            if (!GetAddressIndex((*it).first, (*it).second, addressIndex)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
            }
        }
    }

    std::set<std::pair<int, std::string> > txids;
    UniValue result(UniValue::VARR);

    for (std::vector<std::pair<CAddressIndexKey, CAmount> >::const_iterator it=addressIndex.begin(); it!=addressIndex.end(); it++) {
        int height = it->first.blockHeight;
        std::string txid = it->first.txhash.GetHex();

        if (addresses.size() > 1) {
            txids.insert(std::make_pair(height, txid));
        } else {
            if (txids.insert(std::make_pair(height, txid)).second) {
                result.push_back(txid);
            }
        }
    }

    if (addresses.size() > 1) {
        for (std::set<std::pair<int, std::string> >::const_iterator it=txids.begin(); it!=txids.end(); it++) {
            result.push_back(it->second);
        }
    }

    return result;

}

static UniValue getspentinfo(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1 || !request.params[0].isObject())
        throw std::runtime_error(
            RPCHelpMan{"getspentinfo",
                "\nReturns the txid and index where an output is spent.\n",
                {
                    {"request", RPCArg::Type::OBJ, /* default */ "", "",
                        {
                            {"txid", RPCArg::Type::STR_HEX, /* default */ "", "The hex string of the txid"},
                            {"index", RPCArg::Type::NUM, /* default */ "", "The start block height"},
                        },
                    },
                },
                RPCResult{
            "{\n"
            "  \"txid\"  (string) The transaction id\n"
            "  \"index\"  (number) The spending input index\n"
            "  ,...\n"
            "}\n"
                },
                RPCExamples{
                    HelpExampleCli("getspentinfo", "'{\"txid\": \"0437cd7f8525ceed2324359c2d0ba26006d92d856a9c20fa0241106ee5a597c9\", \"index\": 0}'")
            + HelpExampleRpc("getspentinfo", "{\"txid\": \"0437cd7f8525ceed2324359c2d0ba26006d92d856a9c20fa0241106ee5a597c9\", \"index\": 0}")
                },
            }.ToString());

    UniValue txidValue = find_value(request.params[0].get_obj(), "txid");
    UniValue indexValue = find_value(request.params[0].get_obj(), "index");

    if (!txidValue.isStr() || !indexValue.isNum()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid txid or index");
    }

    uint256 txid = ParseHashV(txidValue, "txid");
    int outputIndex = indexValue.get_int();

    CSpentIndexKey key(txid, outputIndex);
    CSpentIndexValue value;

    if (!GetSpentIndex(key, value)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unable to get spent info");
    }

    UniValue obj(UniValue::VOBJ);
    obj.pushKV("txid", value.txid.GetHex());
    obj.pushKV("index", (int)value.inputIndex);
    obj.pushKV("height", value.blockHeight);

    return obj;
}

static UniValue RPCLockedMemoryInfo()
{
    LockedPool::Stats stats = LockedPoolManager::Instance().stats();
    UniValue obj(UniValue::VOBJ);
    obj.pushKV("used", uint64_t(stats.used));
    obj.pushKV("free", uint64_t(stats.free));
    obj.pushKV("total", uint64_t(stats.total));
    obj.pushKV("locked", uint64_t(stats.locked));
    obj.pushKV("chunks_used", uint64_t(stats.chunks_used));
    obj.pushKV("chunks_free", uint64_t(stats.chunks_free));
    return obj;
}

#ifdef HAVE_MALLOC_INFO
static std::string RPCMallocInfo()
{
    char *ptr = nullptr;
    size_t size = 0;
    FILE *f = open_memstream(&ptr, &size);
    if (f) {
        malloc_info(0, f);
        fclose(f);
        if (ptr) {
            std::string rv(ptr, size);
            free(ptr);
            return rv;
        }
    }
    return "";
}
#endif

static UniValue getmemoryinfo(const JSONRPCRequest& request)
{
    /* Please, avoid using the word "pool" here in the RPC interface or help,
     * as users will undoubtedly confuse it with the other "memory pool"
     */
    if (request.fHelp || request.params.size() > 1)
        throw std::runtime_error(
            RPCHelpMan{"getmemoryinfo",
                "Returns an object containing information about memory usage.\n",
                {
                    {"mode", RPCArg::Type::STR, /* default */ "\"stats\"", "determines what kind of information is returned.\n"
            "  - \"stats\" returns general statistics about memory usage in the daemon.\n"
            "  - \"mallocinfo\" returns an XML string describing low-level heap state (only available if compiled with glibc 2.10+)."},
                },
                {
                    RPCResult{"mode \"stats\"",
            "{\n"
            "  \"locked\": {               (json object) Information about locked memory manager\n"
            "    \"used\": xxxxx,          (numeric) Number of bytes used\n"
            "    \"free\": xxxxx,          (numeric) Number of bytes available in current arenas\n"
            "    \"total\": xxxxxxx,       (numeric) Total number of bytes managed\n"
            "    \"locked\": xxxxxx,       (numeric) Amount of bytes that succeeded locking. If this number is smaller than total, locking pages failed at some point and key data could be swapped to disk.\n"
            "    \"chunks_used\": xxxxx,   (numeric) Number allocated chunks\n"
            "    \"chunks_free\": xxxxx,   (numeric) Number unused chunks\n"
            "  }\n"
            "}\n"
                    },
                    RPCResult{"mode \"mallocinfo\"",
            "\"<malloc version=\"1\">...\"\n"
                    },
                },
                RPCExamples{
                    HelpExampleCli("getmemoryinfo", "")
            + HelpExampleRpc("getmemoryinfo", "")
                },
            }.ToString());

    std::string mode = request.params[0].isNull() ? "stats" : request.params[0].get_str();
    if (mode == "stats") {
        UniValue obj(UniValue::VOBJ);
        obj.pushKV("locked", RPCLockedMemoryInfo());
        return obj;
    } else if (mode == "mallocinfo") {
#ifdef HAVE_MALLOC_INFO
        return RPCMallocInfo();
#else
        throw JSONRPCError(RPC_INVALID_PARAMETER, "mallocinfo is only available when compiled with glibc 2.10+");
#endif
    } else {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "unknown mode " + mode);
    }
}

static void EnableOrDisableLogCategories(UniValue cats, bool enable) {
    cats = cats.get_array();
    for (unsigned int i = 0; i < cats.size(); ++i) {
        std::string cat = cats[i].get_str();

        bool success;
        if (enable) {
            success = LogInstance().EnableCategory(cat);
        } else {
            success = LogInstance().DisableCategory(cat);
        }

        if (!success) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "unknown logging category " + cat);
        }
    }
}

static UniValue logging(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() > 2) {
        throw std::runtime_error(
            RPCHelpMan{"logging",
            "Gets and sets the logging configuration.\n"
            "When called without an argument, returns the list of categories with status that are currently being debug logged or not.\n"
            "When called with arguments, adds or removes categories from debug logging and return the lists above.\n"
            "The arguments are evaluated in order \"include\", \"exclude\".\n"
            "If an item is both included and excluded, it will thus end up being excluded.\n"
            "The valid logging categories are: " + ListLogCategories() + "\n"
            "In addition, the following are available as category names with special meanings:\n"
            "  - \"all\",  \"1\" : represent all logging categories.\n"
            "  - \"dash\" activates all Dash-specific categories at once.\n"
            "To deactivate all categories at once you can specify \"all\" in <exclude>.\n"
            "  - \"none\", \"0\" : even if other logging categories are specified, ignore all of them.\n"
            ,
                {
                    {"include", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "A json array of categories to add debug logging",
                        {
                            {"include_category", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "the valid logging category"},
                        }},
                    {"exclude", RPCArg::Type::ARR, RPCArg::Optional::OMITTED_NAMED_ARG, "A json array of categories to remove debug logging",
                        {
                            {"exclude_category", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "the valid logging category"},
                        }},
                },
                RPCResult{
            "{                   (json object where keys are the logging categories, and values indicates its status\n"
            "  \"category\": true|false,  (bool) if being debug logged or not. false:inactive, true:active\n"
            "  ...\n"
            "}\n"
                },
                RPCExamples{
                    HelpExampleCli("logging", "\"[\\\"all\\\"]\" \"[\\\"http\\\"]\"")
            + HelpExampleRpc("logging", "[\"all\"], \"[libevent]\"")
                },
            }.ToString());
    }

    uint64_t original_log_categories = LogInstance().GetCategoryMask();
    if (request.params[0].isArray()) {
        EnableOrDisableLogCategories(request.params[0], true);
    }
    if (request.params[1].isArray()) {
        EnableOrDisableLogCategories(request.params[1], false);
    }
    uint64_t updated_log_categories = LogInstance().GetCategoryMask();
    uint64_t changed_log_categories = original_log_categories ^ updated_log_categories;

    // Update libevent logging if BCLog::LIBEVENT has changed.
    // If the library version doesn't allow it, UpdateHTTPServerLogging() returns false,
    // in which case we should clear the BCLog::LIBEVENT flag.
    // Throw an error if the user has explicitly asked to change only the libevent
    // flag and it failed.
    if (changed_log_categories & BCLog::LIBEVENT) {
        if (!UpdateHTTPServerLogging(LogInstance().WillLogCategory(BCLog::LIBEVENT))) {
            LogInstance().DisableCategory(BCLog::LIBEVENT);
            if (changed_log_categories == BCLog::LIBEVENT) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "libevent logging cannot be updated when using libevent before v2.1.1.");
            }
        }
    }

    UniValue result(UniValue::VOBJ);
    std::vector<CLogCategoryActive> vLogCatActive = ListActiveLogCategories();
    for (const auto& logCatActive : vLogCatActive) {
        result.pushKV(logCatActive.category, logCatActive.active);
    }

    return result;
}

static UniValue echo(const JSONRPCRequest& request)
{
    if (request.fHelp)
        throw std::runtime_error(
            RPCHelpMan{"echo|echojson ...",
                "\nSimply echo back the input arguments. This command is for testing.\n"
                "\nThe difference between echo and echojson is that echojson has argument conversion enabled in the client-side table in "
                "dash-cli and the GUI. There is no server-side difference.",
                {},
                RPCResults{},
                RPCExamples{""},
            }.ToString()
        );

    return request.params;
}

// clang-format off
static const CRPCCommand commands[] =
{ //  category              name                      actor (function)         argNames
  //  --------------------- ------------------------  -----------------------  ----------
    { "control",            "debug",                  &debug,                  {} },
    { "control",            "getmemoryinfo",          &getmemoryinfo,          {"mode"} },
    { "control",            "logging",                &logging,                {"include", "exclude"}},
    { "util",               "validateaddress",        &validateaddress,        {"address"} },
    { "util",               "createmultisig",         &createmultisig,         {"nrequired","keys"} },
    { "util",               "deriveaddresses",        &deriveaddresses,        {"descriptor", "begin", "end"} },
    { "util",               "getdescriptorinfo",      &getdescriptorinfo,      {"descriptor"} },
    { "util",               "verifymessage",          &verifymessage,          {"address","signature","message"} },
    { "util",               "signmessagewithprivkey", &signmessagewithprivkey, {"privkey","message"} },
    { "blockchain",         "getspentinfo",           &getspentinfo,           {"json"} },

    /* Address index */
    { "addressindex",       "getaddressmempool",      &getaddressmempool,      {"addresses"}  },
    { "addressindex",       "getaddressutxos",        &getaddressutxos,        {"addresses"} },
    { "addressindex",       "getaddressdeltas",       &getaddressdeltas,       {"addresses"} },
    { "addressindex",       "getaddresstxids",        &getaddresstxids,        {"addresses"} },
    { "addressindex",       "getaddressbalance",      &getaddressbalance,      {"addresses"} },

    /* Dash features */
    { "dash",               "mnsync",                 &mnsync,                 {} },
    { "dash",               "spork",                  &spork,                  {"arg0","value"} },

    /* Not shown in help */
    { "hidden",             "setmocktime",            &setmocktime,            {"timestamp"}},
    { "hidden",             "echo",                   &echo,                   {"arg0","arg1","arg2","arg3","arg4","arg5","arg6","arg7","arg8","arg9"}},
    { "hidden",             "echojson",               &echo,                   {"arg0","arg1","arg2","arg3","arg4","arg5","arg6","arg7","arg8","arg9"}},
    { "hidden",             "mnauth",                 &mnauth,                 {"nodeId", "proTxHash", "publicKey"}},
};
// clang-format on

void RegisterMiscRPCCommands(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
