// Copyright (c) 2011-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <core_io.h>
#include <key_io.h>
#include <rpc/util.h>
#include <util/bip32.h>
#include <util/translation.h>
#include <wallet/receive.h>
#include <wallet/rpc/util.h>
#include <wallet/wallet.h>

#include <univalue.h>

namespace wallet {
RPCHelpMan getnewaddress()
{
    return RPCHelpMan{"getnewaddress",
        "\nReturns a new Dash address for receiving payments.\n"
        "If 'label' is specified, it is added to the address book \n"
        "so payments received with the address will be associated with 'label'.\n",
        {
            {"label", RPCArg::Type::STR, RPCArg::Default{""}, "The label name for the address to be linked to. It can also be set to the empty string \"\" to represent the default label. The label does not need to exist, it will be created if there is no label by the given name."},
        },
        RPCResult{
    RPCResult::Type::STR, "address", "The new Dash address"
        },
        RPCExamples{
            HelpExampleCli("getnewaddress", "")
    + HelpExampleRpc("getnewaddress", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    LOCK(pwallet->cs_wallet);

    if (!pwallet->CanGetAddresses()) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Error: This wallet has no available keys");
    }

    // Parse the label first so we don't generate a key if there's an error
    std::string label;
    if (!request.params[0].isNull())
        label = LabelFromValue(request.params[0]);

    auto op_dest = pwallet->GetNewDestination(label);
    if (!op_dest) {
        throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, util::ErrorString(op_dest).original);
    }
    return EncodeDestination(*op_dest);
},
    };
}

RPCHelpMan getrawchangeaddress()
{
    return RPCHelpMan{"getrawchangeaddress",
        "\nReturns a new Dash address, for receiving change.\n"
        "This is for use with raw transactions, NOT normal use.\n",
        {},
        RPCResult{
            RPCResult::Type::STR, "address", "The address"
        },
        RPCExamples{
            HelpExampleCli("getrawchangeaddress", "")
    + HelpExampleRpc("getrawchangeaddress", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    LOCK(pwallet->cs_wallet);

    if (!pwallet->CanGetAddresses(true)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Error: This wallet has no available keys");
    }

    auto op_dest = pwallet->GetNewChangeDestination();
    if (!op_dest) {
        throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, util::ErrorString(op_dest).original);
    }
    return EncodeDestination(*op_dest);
},
    };
}

RPCHelpMan setlabel()
{
    return RPCHelpMan{"setlabel",
        "\nSets the label associated with the given address.\n",
        {
            {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The Dash address to be associated with a label."},
            {"label", RPCArg::Type::STR, RPCArg::Optional::NO, "The label to assign to the address."},
        },
        RPCResult{RPCResult::Type::NONE, "", ""},
        RPCExamples{
            HelpExampleCli("setlabel", "\"" + EXAMPLE_ADDRESS[0] + "\" \"tabby\"")
    + HelpExampleRpc("setlabel", "\"" + EXAMPLE_ADDRESS[0] + "\", \"tabby\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    LOCK(pwallet->cs_wallet);

    CTxDestination dest = DecodeDestination(request.params[0].get_str());
    if (!IsValidDestination(dest)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Dash address");
    }

    std::string label = LabelFromValue(request.params[1]);

    if (pwallet->IsMine(dest)) {
        pwallet->SetAddressBook(dest, label, "receive");
    } else {
        pwallet->SetAddressBook(dest, label, "send");
    }

    return NullUniValue;
},
    };
}

RPCHelpMan listaddressgroupings()
{
    return RPCHelpMan{"listaddressgroupings",
        "\nLists groups of addresses which have had their common ownership\n"
        "made public by common use as inputs or as the resulting change\n"
        "in past transactions\n",
        {},
        RPCResult{
            RPCResult::Type::ARR, "", "",
            {
                {RPCResult::Type::ARR, "", "",
                {
                    {RPCResult::Type::ARR_FIXED, "", "",
                    {
                        {RPCResult::Type::STR, "address", "The Dash address"},
                        {RPCResult::Type::STR_AMOUNT, "amount", "The amount in " + CURRENCY_UNIT},
                        {RPCResult::Type::STR, "label", /*optional=*/true, "The label"},
                    }},
                }},
            }},
        RPCExamples{
            HelpExampleCli("listaddressgroupings", "")
    + HelpExampleRpc("listaddressgroupings", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const std::shared_ptr<const CWallet> pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();

    LOCK(pwallet->cs_wallet);

    UniValue jsonGroupings(UniValue::VARR);
    std::map<CTxDestination, CAmount> balances = GetAddressBalances(*pwallet);
    for (const std::set<CTxDestination>& grouping : GetAddressGroupings(*pwallet)) {
        UniValue jsonGrouping(UniValue::VARR);
        for (const CTxDestination& address : grouping)
        {
            UniValue addressInfo(UniValue::VARR);
            addressInfo.push_back(EncodeDestination(address));
            addressInfo.push_back(ValueFromAmount(balances[address]));
            {
                const auto* address_book_entry = pwallet->FindAddressBookEntry(address);
                if (address_book_entry) {
                    addressInfo.push_back(address_book_entry->GetLabel());
                }
            }
            jsonGrouping.push_back(addressInfo);
        }
        jsonGroupings.push_back(jsonGrouping);
    }
    return jsonGroupings;
},
    };
}

RPCHelpMan addmultisigaddress()
{
    return RPCHelpMan{"addmultisigaddress",
        "\nAdd an nrequired-to-sign multisignature address to the wallet. Requires a new wallet backup.\n"
        "Each key is a Dash address or hex-encoded public key.\n"
        "This functionality is only intended for use with non-watchonly addresses.\n"
        "See `importaddress` for watchonly p2sh address support.\n"
        "If 'label' is specified, assign address to that label.\n"
        "Note: This command is only compatible with legacy wallets.\n",
        {
            {"nrequired", RPCArg::Type::NUM, RPCArg::Optional::NO, "The number of required signatures out of the n keys or addresses."},
            {"keys", RPCArg::Type::ARR, RPCArg::Optional::NO, "The Dash addresses or hex-encoded public keys",
                {
                    {"key", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Dash address or hex-encoded public key"},
                },
                },
            {"label", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "A label to assign the addresses to."},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "address", "The value of the new multisig address"},
                {RPCResult::Type::STR_HEX, "redeemScript", "The string value of the hex-encoded redemption script"},
                {RPCResult::Type::STR, "descriptor", "The descriptor for this multisig."},
            }
        },
        RPCExamples{
    "\nAdd a multisig address from 2 addresses\n"
    + HelpExampleCli("addmultisigaddress", "2 \"[\\\"" + EXAMPLE_ADDRESS[0] + "\\\",\\\"" + EXAMPLE_ADDRESS[1] + "\\\"]\"") +
    "\nAs a JSON-RPC call\n"
    + HelpExampleRpc("addmultisigaddress", "2, \"[\\\"" + EXAMPLE_ADDRESS[0] + "\\\",\\\"" + EXAMPLE_ADDRESS[1] + "\\\"]\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    LegacyScriptPubKeyMan& spk_man = EnsureLegacyScriptPubKeyMan(*pwallet);

    LOCK2(pwallet->cs_wallet, spk_man.cs_KeyStore);

    std::string label;
    if (!request.params[2].isNull())
        label = LabelFromValue(request.params[2]);

    int required = request.params[0].getInt<int>();

    // Get the public keys
    const UniValue& keys_or_addrs = request.params[1].get_array();
    std::vector<CPubKey> pubkeys;
    for (unsigned int i = 0; i < keys_or_addrs.size(); ++i) {
        if (IsHex(keys_or_addrs[i].get_str()) && (keys_or_addrs[i].get_str().length() == 66 || keys_or_addrs[i].get_str().length() == 130)) {
            pubkeys.push_back(HexToPubKey(keys_or_addrs[i].get_str()));
        } else {
            pubkeys.push_back(AddrToPubKey(spk_man, keys_or_addrs[i].get_str()));
        }
    }

    // Construct using pay-to-script-hash:
    CScript inner;
    CTxDestination dest = AddAndGetMultisigDestination(required, pubkeys, spk_man, inner);
    pwallet->SetAddressBook(dest, label, "send");

    // Make the descriptor
    std::unique_ptr<Descriptor> descriptor = InferDescriptor(GetScriptForDestination(dest), spk_man);

    UniValue result(UniValue::VOBJ);
    result.pushKV("address", EncodeDestination(dest));
    result.pushKV("redeemScript", HexStr(inner));
    result.pushKV("descriptor", descriptor->ToString());
    return result;
},
    };
}

RPCHelpMan keypoolrefill()
{
    return RPCHelpMan{"keypoolrefill",
        "\nFills the keypool."+
                HELP_REQUIRING_PASSPHRASE,
        {
            {"newsize", RPCArg::Type::NUM, RPCArg::DefaultHint{strprintf("%u, or as set by -keypool", DEFAULT_KEYPOOL_SIZE)}, "The new keypool size"},
        },
        RPCResult{RPCResult::Type::NONE, "", ""},
        RPCExamples{
            HelpExampleCli("keypoolrefill", "")
    + HelpExampleRpc("keypoolrefill", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    if (pwallet->IsLegacy() && pwallet->IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Error: Private keys are disabled for this wallet");
    }

    LOCK(pwallet->cs_wallet);

    // 0 is interpreted by TopUpKeyPool() as the default keypool size given by -keypool
    unsigned int kpSize = 0;
    if (!request.params[0].isNull()) {
        if (request.params[0].getInt<int>() < 0)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, expected valid size.");
        kpSize = (unsigned int)request.params[0].getInt<int>();
    }

    EnsureWalletIsUnlocked(*pwallet);
    pwallet->TopUpKeyPool(kpSize);

    if (pwallet->GetKeyPoolSize() < (pwallet->IsHDEnabled() ? kpSize * 2 : kpSize)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Error refreshing keypool.");
    }

    return NullUniValue;
},
    };
}

RPCHelpMan newkeypool()
{
    return RPCHelpMan{"newkeypool",
                "\nEntirely clears and refills the keypool.\n"
                "WARNING: On non-HD wallets, this will require a new backup immediately, to include the new keys.\n"
                "When restoring a backup of an HD wallet created before the newkeypool command is run, funds received to\n"
                "new addresses may not appear automatically. They have not been lost, but the wallet may not find them.\n"
                "This can be fixed by running the newkeypool command on the backup and then rescanning, so the wallet\n"
                "re-generates the required keys.\n"
                "Note: This command is only compatible with legacy wallets.\n" +
            HELP_REQUIRING_PASSPHRASE,
                {},
                RPCResult{RPCResult::Type::NONE, "", ""},
                RPCExamples{
            HelpExampleCli("newkeypool", "")
            + HelpExampleRpc("newkeypool", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    LOCK(pwallet->cs_wallet);

    LegacyScriptPubKeyMan& spk_man = EnsureLegacyScriptPubKeyMan(*pwallet, true);
    spk_man.NewKeyPool();

    return NullUniValue;
},
    };
}

class DescribeWalletAddressVisitor
{
public:
    const SigningProvider * const provider;

    void ProcessSubScript(const CScript& subscript, UniValue& obj) const
    {
        // Always present: script type and redeemscript
        std::vector<std::vector<unsigned char>> solutions_data;
        TxoutType whichType = Solver(subscript, solutions_data);
        obj.pushKV("script", GetTxnOutputType(whichType));
        obj.pushKV("hex", HexStr(subscript));

        CTxDestination embedded;
        if (ExtractDestination(subscript, embedded)) {
            // Only when the script corresponds to an address.
            UniValue subobj(UniValue::VOBJ);
            UniValue detail = DescribeAddress(embedded);
            subobj.pushKVs(detail);
            UniValue wallet_detail = std::visit(*this, embedded);
            subobj.pushKVs(wallet_detail);
            subobj.pushKV("address", EncodeDestination(embedded));
            subobj.pushKV("scriptPubKey", HexStr(subscript));
            // Always report the pubkey at the top level, so that `getnewaddress()['pubkey']` always works.
            if (subobj.exists("pubkey")) obj.pushKV("pubkey", subobj["pubkey"]);
            obj.pushKV("embedded", std::move(subobj));
        } else if (whichType == TxoutType::MULTISIG) {
            // Also report some information on multisig scripts (which do not have a corresponding address).
            UniValue pubkeys(UniValue::VARR);
            UniValue addresses(UniValue::VARR);
            for (size_t i = 1; i < solutions_data.size() - 1; ++i) {
                CPubKey pubkey(solutions_data[i]);
                pubkeys.push_back(HexStr(pubkey));
                addresses.push_back(EncodeDestination(PKHash(pubkey)));
            }
            obj.pushKV("pubkeys", std::move(pubkeys));
            obj.pushKV("addresses", std::move(addresses));
            obj.pushKV("sigsrequired", solutions_data[0][0]);
        }
    }

    explicit DescribeWalletAddressVisitor(const SigningProvider * const _provider) : provider(_provider) {}

    UniValue operator()(const CNoDestination &dest) const { return UniValue(UniValue::VOBJ); }

    UniValue operator()(const PKHash& pkhash) const {
        CKeyID keyID{ToKeyID(pkhash)};
        UniValue obj(UniValue::VOBJ);
        CPubKey vchPubKey;
        if (provider && provider->GetPubKey(keyID, vchPubKey)) {
            obj.pushKV("pubkey", HexStr(vchPubKey));
            obj.pushKV("iscompressed", vchPubKey.IsCompressed());
        }
        return obj;
    }

    UniValue operator()(const ScriptHash& scriptHash) const {
        CScriptID scriptID(scriptHash);
        UniValue obj(UniValue::VOBJ);
        CScript subscript;
        if (provider && provider->GetCScript(scriptID, subscript)) {
            ProcessSubScript(subscript, obj);
        }
        return obj;
    }
};

static UniValue DescribeWalletAddress(const CWallet& wallet, const CTxDestination& dest)
{
    UniValue ret(UniValue::VOBJ);
    UniValue detail = DescribeAddress(dest);
    CScript script = GetScriptForDestination(dest);
    std::unique_ptr<SigningProvider> provider = nullptr;
    provider = wallet.GetSolvingProvider(script);
    ret.pushKVs(detail);
    ret.pushKVs(std::visit(DescribeWalletAddressVisitor(provider.get()), dest));
    return ret;
}

RPCHelpMan getaddressinfo()
{
    return RPCHelpMan{"getaddressinfo",
        "\nReturn information about the given Dash address.\n"
        "Some of the information will only be present if the address is in the active wallet.\n",
        {
            {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The Dash address for which to get information."},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "address", "The Dash address validated."},
                {RPCResult::Type::STR_HEX, "scriptPubKey", "The hex-encoded scriptPubKey generated by the address."},
                {RPCResult::Type::BOOL, "ismine", "If the address is yours."},
                {RPCResult::Type::BOOL, "iswatchonly", "If the address is watchonly."},
                {RPCResult::Type::BOOL, "solvable", "If we know how to spend coins sent to this address, ignoring the possible lack of private keys."},
                {RPCResult::Type::STR, "desc", /*optional=*/true, "A descriptor for spending coins sent to this address (only when solvable)."},
                {RPCResult::Type::STR, "parent_desc", /*optional=*/true, "The descriptor used to derive this address if this is a descriptor wallet"},
                {RPCResult::Type::BOOL, "isscript", "If the key is a script."},
                {RPCResult::Type::BOOL, "ischange", "If the address was used for change output."},
                {RPCResult::Type::STR, "script", /*optional=*/true, "The output script type. Only if isscript is true and the redeemscript is known. Possible types: nonstandard, pubkey, pubkeyhash, scripthash, multisig, nulldata"},
                {RPCResult::Type::STR_HEX, "hex", /*optional=*/true, "The redeemscript for the p2sh address."},
                {RPCResult::Type::ARR, "pubkeys", /*optional=*/true, "Array of pubkeys associated with the known redeemscript (only if script is multisig).",
                {
                    {RPCResult::Type::STR, "pubkey", ""},
                }},
                {RPCResult::Type::ARR, "addresses", /*optional=*/true, "Array of addresses associated with the known redeemscript (only if script is multisig).",
                {
                    {RPCResult::Type::STR, "address", ""},
                }},
                {RPCResult::Type::NUM, "sigsrequired", /*optional=*/true, "The number of signatures required to spend multisig output (only if script is multisig)."},
                {RPCResult::Type::STR_HEX, "pubkey", /*optional=*/true, "The hex value of the raw public key, for single-key addresses."},
                {RPCResult::Type::OBJ, "embedded", /*optional=*/true, "Information about the address embedded in P2SH, if relevant and known.",
                {
                    {RPCResult::Type::ELISION, "", "Includes all getaddressinfo output fields for the embedded address, excluding metadata (timestamp, hdkeypath, hdseedid)\n"
                    "and relation to the wallet (ismine, iswatchonly)."},
                }},
                {RPCResult::Type::BOOL, "iscompressed", /*optional=*/true, "If the pubkey is compressed."},
                {RPCResult::Type::NUM_TIME, "timestamp", /*optional=*/true, "The creation time of the key, if available, expressed in " + UNIX_EPOCH_TIME + "."},
                {RPCResult::Type::STR_HEX, "hdchainid", /*optional=*/true, "The ID of the HD chain."},
                {RPCResult::Type::STR, "hdkeypath", /*optional=*/true, "The HD keypath, if the key is HD and available."},
                {RPCResult::Type::STR_HEX, "hdseedid", /*optional=*/true, "The Hash160 of the HD seed."},
                {RPCResult::Type::STR_HEX, "hdmasterfingerprint", /*optional=*/true, "The fingerprint of the master key."},
                {RPCResult::Type::ARR, "labels", "An array of labels associated with the address. Currently limited to one label but returned as an array to keep the API stable if multiple labels are enabled in the future.",
                {
                    {RPCResult::Type::STR, "label name", "Label name (defaults to \"\")."},
                }},
            }
        },
        RPCExamples{
            HelpExampleCli("getaddressinfo", EXAMPLE_ADDRESS[0])
    + HelpExampleRpc("getaddressinfo", EXAMPLE_ADDRESS[0])
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const std::shared_ptr<const CWallet> pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    LOCK(pwallet->cs_wallet);

    std::string error_msg;
    CTxDestination dest = DecodeDestination(request.params[0].get_str(), error_msg);

    // Make sure the destination is valid
    if (!IsValidDestination(dest)) {
        // Set generic error message in case 'DecodeDestination' didn't set it
        if (error_msg.empty()) error_msg = "Invalid address";

        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, error_msg);
    }

    UniValue ret(UniValue::VOBJ);

    std::string currentAddress = EncodeDestination(dest);
    ret.pushKV("address", currentAddress);

    CScript scriptPubKey = GetScriptForDestination(dest);
    ret.pushKV("scriptPubKey", HexStr(scriptPubKey));

    std::unique_ptr<SigningProvider> provider = pwallet->GetSolvingProvider(scriptPubKey);

    isminetype mine = pwallet->IsMine(dest);
    ret.pushKV("ismine", bool(mine & ISMINE_SPENDABLE));

    bool solvable = provider && IsSolvable(*provider, scriptPubKey);
    ret.pushKV("solvable", solvable);

    if (solvable) {
        ret.pushKV("desc", InferDescriptor(scriptPubKey, *provider)->ToString());
    }

    DescriptorScriptPubKeyMan* desc_spk_man = dynamic_cast<DescriptorScriptPubKeyMan*>(pwallet->GetScriptPubKeyMan(scriptPubKey));
    if (desc_spk_man) {
        std::string desc_str;
        if (desc_spk_man->GetDescriptorString(desc_str, /*priv=*/false)) {
            ret.pushKV("parent_desc", desc_str);
        }
    }

    ret.pushKV("iswatchonly", bool(mine & ISMINE_WATCH_ONLY));

    UniValue detail = DescribeWalletAddress(*pwallet, dest);
    ret.pushKVs(detail);

    ret.pushKV("ischange", ScriptIsChange(*pwallet, scriptPubKey));

    ScriptPubKeyMan* spk_man = pwallet->GetScriptPubKeyMan(scriptPubKey);
    if (spk_man) {
        if (const std::unique_ptr<CKeyMetadata> meta = spk_man->GetMetadata(dest)) {
            ret.pushKV("timestamp", meta->nCreateTime);
            CHDChain hdChainCurrent;
            LegacyScriptPubKeyMan* legacy_spk_man = pwallet->GetLegacyScriptPubKeyMan();
            if (legacy_spk_man != nullptr) {
                const PKHash *pkhash = std::get_if<PKHash>(&dest);
                // TODO: refactor to use hd_seed_id from `meta`
                if (pkhash && legacy_spk_man->HaveHDKey(ToKeyID(*pkhash), hdChainCurrent)) {
                    ret.pushKV("hdchainid", hdChainCurrent.GetID().GetHex());
                }
            }
            if (meta->has_key_origin) {
                ret.pushKV("hdkeypath", WriteHDKeypath(meta->key_origin.path));
                ret.pushKV("hdmasterfingerprint", HexStr(meta->key_origin.fingerprint));
            }
        }
    }

    // Return a `labels` array containing the label associated with the address,
    // equivalent to the `label` field above. Currently only one label can be
    // associated with an address, but we return an array so the API remains
    // stable if we allow multiple labels to be associated with an address in
    // the future.
    UniValue labels(UniValue::VARR);
    const auto* address_book_entry = pwallet->FindAddressBookEntry(dest);
    if (address_book_entry) {
        labels.push_back(address_book_entry->GetLabel());
    }
    ret.pushKV("labels", std::move(labels));

    return ret;
},
    };
}

RPCHelpMan getaddressesbylabel()
{
    return RPCHelpMan{"getaddressesbylabel",
        "\nReturns the list of addresses assigned the specified label.\n",
        {
            {"label", RPCArg::Type::STR, RPCArg::Optional::NO, "The label."},
        },
        RPCResult{
            RPCResult::Type::OBJ_DYN, "", "json object with addresses as keys",
            {
                {RPCResult::Type::OBJ, "address", "json object with information about address",
                {
                    {RPCResult::Type::STR, "purpose", "Purpose of address (\"send\" for sending address, \"receive\" for receiving address)"},
                }},
            }
        },
        RPCExamples{
            HelpExampleCli("getaddressesbylabel", "\"tabby\"")
    + HelpExampleRpc("getaddressesbylabel", "\"tabby\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const std::shared_ptr<const CWallet> pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    LOCK(pwallet->cs_wallet);

    std::string label = LabelFromValue(request.params[0]);

    // Find all addresses that have the given label
    UniValue ret(UniValue::VOBJ);
    std::set<std::string> addresses;
    pwallet->ForEachAddrBookEntry([&](const CTxDestination& _dest, const std::string& _label, const std::string& _purpose, bool _is_change) {
        if (_is_change) return;
        if (_label == label) {
            std::string address = EncodeDestination(_dest);
            // CWallet::m_address_book is not expected to contain duplicate
            // address strings, but build a separate set as a precaution just in
            // case it does.
            bool unique = addresses.emplace(address).second;
            CHECK_NONFATAL(unique);
            // UniValue::pushKV checks if the key exists in O(N)
            // and since duplicate addresses are unexpected (checked with
            // std::set in O(log(N))), UniValue::__pushKV is used instead,
            // which currently is O(1).
            UniValue value(UniValue::VOBJ);
            value.pushKV("purpose", _purpose);
            ret.__pushKV(address, value);
        }
    });

    if (ret.empty()) {
        throw JSONRPCError(RPC_WALLET_INVALID_LABEL_NAME, std::string("No addresses with label " + label));
    }

    return ret;
},
    };
}

RPCHelpMan listlabels()
{
    return RPCHelpMan{"listlabels",
        "\nReturns the list of all labels, or labels that are assigned to addresses with a specific purpose.\n",
        {
            {"purpose", RPCArg::Type::STR, RPCArg::Optional::OMITTED_NAMED_ARG, "Address purpose to list labels for ('send','receive'). An empty string is the same as not providing this argument."},
        },
        RPCResult{
            RPCResult::Type::ARR, "", "",
            {
                {RPCResult::Type::STR, "label", "Label name"},
            }
        },
        RPCExamples{
    "\nList all labels\n"
    + HelpExampleCli("listlabels", "") +
    "\nList labels that have receiving addresses\n"
    + HelpExampleCli("listlabels", "receive") +
    "\nList labels that have sending addresses\n"
    + HelpExampleCli("listlabels", "send") +
    "\nAs a JSON-RPC call\n"
    + HelpExampleRpc("listlabels", "receive")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const std::shared_ptr<const CWallet> pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return NullUniValue;

    LOCK(pwallet->cs_wallet);

    std::string purpose;
    if (!request.params[0].isNull()) {
        purpose = request.params[0].get_str();
    }

    // Add to a set to sort by label name, then insert into Univalue array
    std::set<std::string> label_set = pwallet->ListAddrBookLabels(purpose);

    UniValue ret(UniValue::VARR);
    for (const std::string& name : label_set) {
        ret.push_back(name);
    }

    return ret;
},
    };
}
} // namespace wallet
