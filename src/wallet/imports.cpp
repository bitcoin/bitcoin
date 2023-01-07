// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/imports.h>

#include <key_io.h>
#include <set>
#include <string>
#include <util/check.h>
#include <vector>
#include <wallet/wallet.h>

namespace wallet {
struct ImportData {
    // Input data
    std::unique_ptr<CScript> redeemscript; //!< Provided redeemScript; will be moved to `import_scripts` if relevant.
    std::unique_ptr<CScript> witnessscript; //!< Provided witnessScript; will be moved to `import_scripts` if relevant.

    // Output data
    std::set<CScript> import_scripts;
    std::map<CKeyID, bool> used_keys; //!< Import these private keys if available (the value indicates whether if the key is required for solvability)
    std::map<CKeyID, std::pair<CPubKey, KeyOriginInfo>> key_origins;
};

enum class ScriptContext {
    TOP, //!< Top-level scriptPubKey
    P2SH, //!< P2SH redeemScript
    WITNESS_V0, //!< P2WSH witnessScript
};

// Analyse the provided scriptPubKey, determining which keys and which redeem scripts from the ImportData struct are needed to spend it, and mark them as used.
// Returns an error string, or the empty string for success.
static std::string RecurseImportData(const CScript &script, ImportData &import_data, const ScriptContext script_ctx) {
    // Use Solver to obtain script type and parsed pubkeys or hashes:
    std::vector<std::vector<unsigned char>> solverdata;
    TxoutType script_type = Solver(script, solverdata);

    switch (script_type) {
        case TxoutType::PUBKEY: {
            CPubKey pubkey(solverdata[0]);
            import_data.used_keys.emplace(pubkey.GetID(), false);
            return "";
        }
        case TxoutType::PUBKEYHASH: {
            CKeyID id = CKeyID(uint160(solverdata[0]));
            import_data.used_keys[id] = true;
            return "";
        }
        case TxoutType::SCRIPTHASH: {
            if (script_ctx == ScriptContext::P2SH)
                throw InvalidAddressOrKey("Trying to nest P2SH inside another P2SH");
            if (script_ctx == ScriptContext::WITNESS_V0)
                throw InvalidAddressOrKey("Trying to nest P2SH inside a P2WSH");
            CHECK_NONFATAL(script_ctx == ScriptContext::TOP);
            CScriptID id = CScriptID(uint160(solverdata[0]));
            auto subscript = std::move(
                    import_data.redeemscript); // Remove redeemscript from import_data to check for superfluous script later.
            if (!subscript) return "missing redeemscript";
            if (CScriptID(*subscript) != id) return "redeemScript does not match the scriptPubKey";
            import_data.import_scripts.emplace(*subscript);
            return RecurseImportData(*subscript, import_data, ScriptContext::P2SH);
        }
        case TxoutType::MULTISIG: {
            for (size_t i = 1; i + 1 < solverdata.size(); ++i) {
                CPubKey pubkey(solverdata[i]);
                import_data.used_keys.emplace(pubkey.GetID(), false);
            }
            return "";
        }
        case TxoutType::WITNESS_V0_SCRIPTHASH: {
            if (script_ctx == ScriptContext::WITNESS_V0)
                throw InvalidAddressOrKey("Trying to nest P2WSH inside another P2WSH");
            uint256 fullid(solverdata[0]);
            CScriptID id;
            CRIPEMD160().Write(fullid.begin(), fullid.size()).Finalize(id.begin());
            auto subscript = std::move(
                    import_data.witnessscript); // Remove redeemscript from import_data to check for superfluous script later.
            if (!subscript) return "missing witnessscript";
            if (CScriptID(*subscript) != id) return "witnessScript does not match the scriptPubKey or redeemScript";
            if (script_ctx == ScriptContext::TOP) {
                import_data.import_scripts.emplace(
                        script); // Special rule for IsMine: native P2WSH requires the TOP script imported (see script/ismine.cpp)
            }
            import_data.import_scripts.emplace(*subscript);
            return RecurseImportData(*subscript, import_data, ScriptContext::WITNESS_V0);
        }
        case TxoutType::WITNESS_V0_KEYHASH: {
            if (script_ctx == ScriptContext::WITNESS_V0)
                throw InvalidAddressOrKey("Trying to nest P2WPKH inside P2WSH");
            CKeyID id = CKeyID(uint160(solverdata[0]));
            import_data.used_keys[id] = true;
            if (script_ctx == ScriptContext::TOP) {
                import_data.import_scripts.emplace(
                        script); // Special rule for IsMine: native P2WPKH requires the TOP script imported (see script/ismine.cpp)
            }
            return "";
        }
        case TxoutType::NULL_DATA:
            return "unspendable script";
        case TxoutType::NONSTANDARD:
        case TxoutType::WITNESS_UNKNOWN:
        case TxoutType::WITNESS_V1_TAPROOT:
            return "unrecognized script";
    } // no default case, so the compiler can warn about missing cases
    NONFATAL_UNREACHABLE();
}

std::vector<std::string> ProcessImportLegacy(ImportData &import_data, std::map<CKeyID, CPubKey> &pubkey_map,
                                             std::map<CKeyID, CKey> &privkey_map,
                                             std::set<CScript> &script_pub_keys, bool &have_solving_data,
                                             const ImportMultiData &data, std::vector<CKeyID> &ordered_pubkeys) {
    std::vector<std::string> warnings;

    // Generate the script and destination for the scriptPubKey provided
    CScript script;
    if (!data.isScript) {
        CTxDestination dest = DecodeDestination(data.scriptPubKey);
        if (!IsValidDestination(dest)) {
            throw InvalidAddressOrKey("Invalid address \"" + data.scriptPubKey + "\"");
        }
        if (OutputTypeFromDestination(dest) == OutputType::BECH32M) {
            throw InvalidAddressOrKey("Bech32m addresses cannot be imported into legacy wallets");
        }
        script = GetScriptForDestination(dest);
    } else {
        if (!IsHex(data.scriptPubKey)) {
            throw InvalidAddressOrKey("Invalid scriptPubKey \"" + data.scriptPubKey + "\"");
        }
        std::vector<unsigned char> vData(ParseHex(data.scriptPubKey));
        script = CScript(vData.begin(), vData.end());
        CTxDestination dest;
        if (!ExtractDestination(script, dest) && !data.internal) {
            throw InvalidParameter("Internal must be set to true for nonstandard scriptPubKey imports.");
        }
    }
    script_pub_keys.emplace(script);

    // Parse all arguments
    if (data.redeem_script.size()) {
        if (!IsHex(data.redeem_script)) {
            throw InvalidAddressOrKey("Invalid redeem script \"" + data.redeem_script + "\": must be hex string");
        }
        auto parsed_redeemscript = ParseHex(data.redeem_script);
        import_data.redeemscript = std::make_unique<CScript>(parsed_redeemscript.begin(),
                                                             parsed_redeemscript.end());
    }
    if (data.witness_script.size()) {
        if (!IsHex(data.witness_script)) {
            throw InvalidAddressOrKey("Invalid witness script \"" + data.witness_script + "\": must be hex string");
        }
        auto parsed_witnessscript = ParseHex(data.witness_script);
        import_data.witnessscript = std::make_unique<CScript>(parsed_witnessscript.begin(),
                                                              parsed_witnessscript.end());
    }
    for (size_t i = 0; i < data.public_keys.size(); ++i) {
        const auto &str = data.public_keys[i];
        if (!IsHex(str)) {
            throw InvalidAddressOrKey("Pubkey \"" + str + "\" must be a hex string");
        }
        auto parsed_pubkey = ParseHex(str);
        CPubKey pubkey(parsed_pubkey);
        if (!pubkey.IsFullyValid()) {
            throw InvalidAddressOrKey("Pubkey \"" + str + "\" is not a valid public key");
        }
        pubkey_map.emplace(pubkey.GetID(), pubkey);
        ordered_pubkeys.push_back(pubkey.GetID());
    }
    for (size_t i = 0; i < data.private_keys.size(); ++i) {
        const auto &str = data.private_keys[i];
        CKey key = DecodeSecret(str);
        if (!key.IsValid()) {
            throw InvalidAddressOrKey("Invalid private key encoding " + str);
        }
        CPubKey pubkey = key.GetPubKey();
        CKeyID id = pubkey.GetID();
        if (pubkey_map.count(id)) {
            pubkey_map.erase(id);
        }
        privkey_map.emplace(id, key);
    }


    // Verify and process input data
    have_solving_data =
            import_data.redeemscript || import_data.witnessscript || pubkey_map.size() || privkey_map.size();
    if (have_solving_data) {
        // Match up data in import_data with the scriptPubKey in script.
        auto error = RecurseImportData(script, import_data, ScriptContext::TOP);

        // Verify whether the watchonly option corresponds to the availability of private keys.
        bool spendable = std::all_of(import_data.used_keys.begin(), import_data.used_keys.end(),
                                     [&](const std::pair<CKeyID, bool> &used_key) {
                                         return privkey_map.count(used_key.first) > 0;
                                     });
        if (!data.watch_only && !spendable) {
            warnings.push_back(
                    "Some private keys are missing, outputs will be considered watchonly. If this is intentional, specify the watchonly flag.");
        }
        if (data.watch_only && spendable) {
            warnings.push_back(
                    "All private keys are provided, outputs will be considered spendable. If this is intentional, do not specify the watchonly flag.");
        }

        // Check that all required keys for solvability are provided.
        if (error.empty()) {
            for (const auto &require_key: import_data.used_keys) {
                if (!require_key.second) continue; // Not a required key
                if (pubkey_map.count(require_key.first) == 0 && privkey_map.count(require_key.first) == 0) {
                    error = "some required keys are missing";
                }
            }
        }

        if (!error.empty()) {
            warnings.push_back("Importing as non-solvable: " + error +
                               ". If this is intentional, don't provide any keys, pubkeys, witnessscript, or redeemscript.");
            import_data = ImportData();
            pubkey_map.clear();
            privkey_map.clear();
            have_solving_data = false;
        } else {
            // RecurseImportData() removes any relevant redeemscript/witnessscript from import_data, so we can use that to discover if a superfluous one was provided.
            if (import_data.redeemscript) warnings.push_back("Ignoring redeemscript as this is not a P2SH script.");
            if (import_data.witnessscript)
                warnings.push_back("Ignoring witnessscript as this is not a (P2SH-)P2WSH script.");
            for (auto it = privkey_map.begin(); it != privkey_map.end();) {
                auto oldit = it++;
                if (import_data.used_keys.count(oldit->first) == 0) {
                    warnings.push_back("Ignoring irrelevant private key.");
                    privkey_map.erase(oldit);
                }
            }
            for (auto it = pubkey_map.begin(); it != pubkey_map.end();) {
                auto oldit = it++;
                auto key_data_it = import_data.used_keys.find(oldit->first);
                if (key_data_it == import_data.used_keys.end() || !key_data_it->second) {
                    warnings.push_back("Ignoring public key \"" + HexStr(oldit->first) +
                                       "\" as it doesn't appear inside P2PKH or P2WPKH.");
                    pubkey_map.erase(oldit);
                }
            }
        }
    }

    return warnings;
}

std::vector<std::string> ProcessImportDescriptor(ImportData &import_data, std::map<CKeyID, CPubKey> &pubkey_map,
                                                 std::map<CKeyID, CKey> &privkey_map,
                                                 std::set<CScript> &script_pub_keys, bool &have_solving_data,
                                                 const ImportMultiData &data,
                                                 std::vector<CKeyID> &ordered_pubkeys) {
    std::vector<std::string> warnings;

    have_solving_data = data.parsed_desc->IsSolvable();

    // Expand all descriptors to get public keys and scripts, and private keys if available.
    for (int i = data.range_start; i <= data.range_end; ++i) {
        FlatSigningProvider out_keys;
        std::vector<CScript> scripts_temp;
        data.parsed_desc->Expand(i, data.keys, scripts_temp, out_keys);
        std::copy(scripts_temp.begin(), scripts_temp.end(), std::inserter(script_pub_keys, script_pub_keys.end()));
        for (const auto &key_pair: out_keys.pubkeys) {
            ordered_pubkeys.push_back(key_pair.first);
        }

        for (const auto &x: out_keys.scripts) {
            import_data.import_scripts.emplace(x.second);
        }

        data.parsed_desc->ExpandPrivate(i, data.keys, out_keys);

        std::copy(out_keys.pubkeys.begin(), out_keys.pubkeys.end(), std::inserter(pubkey_map, pubkey_map.end()));
        std::copy(out_keys.keys.begin(), out_keys.keys.end(), std::inserter(privkey_map, privkey_map.end()));
        import_data.key_origins.insert(out_keys.origins.begin(), out_keys.origins.end());
    }

    for (size_t i = 0; i < data.private_keys.size(); ++i) {
        const auto &str = data.private_keys[i];
        CKey key = DecodeSecret(str);
        if (!key.IsValid()) {
            throw InvalidAddressOrKey("Invalid private key encoding");
        }
        CPubKey pubkey = key.GetPubKey();
        CKeyID id = pubkey.GetID();

        // Check if this private key corresponds to a public key from the descriptor
        if (!pubkey_map.count(id)) {
            warnings.push_back("Ignoring irrelevant private key.");
        } else {
            privkey_map.emplace(id, key);
        }
    }

    // Check if all the public keys have corresponding private keys in the import for spendability.
    // This does not take into account threshold multisigs which could be spendable without all keys.
    // Thus, threshold multisigs without all keys will be considered not spendable here, even if they are,
    // perhaps triggering a false warning message. This is consistent with the current wallet IsMine check.
    bool spendable = std::all_of(pubkey_map.begin(), pubkey_map.end(),
                                 [&](const std::pair<CKeyID, CPubKey> &used_key) {
                                     return privkey_map.count(used_key.first) > 0;
                                 }) && std::all_of(import_data.key_origins.begin(), import_data.key_origins.end(),
                                                   [&](const std::pair<CKeyID, std::pair<CPubKey, KeyOriginInfo>> &entry) {
                                                       return privkey_map.count(entry.first) > 0;
                                                   });
    if (!data.watch_only && !spendable) {
        warnings.push_back(
                "Some private keys are missing, outputs will be considered watchonly. If this is intentional, specify the watchonly flag.");
    }
    if (data.watch_only && spendable) {
        warnings.push_back(
                "All private keys are provided, outputs will be considered spendable. If this is intentional, do not specify the watchonly flag.");
    }

    return warnings;
}

bool ProcessImport(CWallet &wallet, const ImportMultiData &data, std::vector<std::string> &warnings,
                   const int64_t timestamp) {
    // Add to keypool only works with privkeys disabled
    if (data.keypool && !wallet.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
        throw InvalidParameter("Keys can only be imported to the keypool when private keys are disabled");
    }

    ImportData import_data;
    std::map<CKeyID, CPubKey> pubkey_map;
    std::map<CKeyID, CKey> privkey_map;
    std::set<CScript> script_pub_keys;
    std::vector<CKeyID> ordered_pubkeys;
    bool have_solving_data;

    if (!data.scriptPubKey.empty()) {
        warnings = ProcessImportLegacy(import_data, pubkey_map, privkey_map, script_pub_keys, have_solving_data,
                                       data, ordered_pubkeys);
    } else {
        warnings = ProcessImportDescriptor(import_data, pubkey_map, privkey_map, script_pub_keys, have_solving_data,
                                           data, ordered_pubkeys);
    }

    // If private keys are disabled, abort if private keys are being imported
    if (wallet.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS) && !privkey_map.empty()) {
        throw WalletError("Cannot import private keys to a wallet with private keys disabled");
    }

    // Check whether we have any work to do
    for (const CScript &script: script_pub_keys) {
        if (wallet.IsMine(script) & ISMINE_SPENDABLE) {
            throw WalletError(
                    "The wallet already contains the private key for this address or script (\"" + HexStr(script) +
                    "\")");
        }
    }

    // All good, time to import
    wallet.MarkDirty();
    if (!wallet.ImportScripts(import_data.import_scripts, timestamp)) {
        throw WalletError("Error adding script to wallet");
    }
    if (!wallet.ImportPrivKeys(privkey_map, timestamp)) {
        throw WalletError("Error adding key to wallet");
    }
    if (!wallet.ImportPubKeys(ordered_pubkeys, pubkey_map, import_data.key_origins, data.keypool, data.internal,
                              timestamp)) {
        throw WalletError("Error adding address to wallet");
    }
    if (!wallet.ImportScriptPubKeys(data.label, script_pub_keys, have_solving_data, !data.internal, timestamp)) {
        throw WalletError("Error adding address to wallet");
    }

    return true;
}

bool ProcessDescriptorImport(CWallet &wallet, const ImportDescriptorData &data, std::vector<std::string> &warnings,
                             FlatSigningProvider &keys, bool range_exists, const int64_t timestamp) {
    // Active descriptors must be ranged
    if (data.active && !data.parsed_desc->IsRange()) {
        throw InvalidParameter("Active descriptors must be ranged");
    }

    // Ranged descriptors should not have a label
    if (range_exists && !data.label.empty()) {
        throw InvalidParameter("Ranged descriptors should not have a label");
    }

    // Internal addresses should not have a label either
    if (data.internal && !data.label.empty()) {
        throw InvalidParameter("Internal addresses should not have a label");
    }

    // Combo descriptor check
    if (data.active && !data.parsed_desc->IsSingleType()) {
        throw WalletError("Combo descriptors cannot be set to active");
    }

    // If the wallet disabled private keys, abort if private keys exist
    if (wallet.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS) && !keys.keys.empty()) {
        throw WalletError("Cannot import private keys to a wallet with private keys disabled");
    }

    // Need to ExpandPrivate to check if private keys are available for all pubkeys
    FlatSigningProvider expand_keys;
    std::vector<CScript> scripts;
    if (!data.parsed_desc->Expand(0, keys, scripts, expand_keys)) {
        throw WalletError(
                "Cannot expand descriptor. Probably because of hardened derivations without private keys provided");
    }
    data.parsed_desc->ExpandPrivate(0, keys, expand_keys);

    // Check if all private keys are provided
    bool have_all_privkeys = !expand_keys.keys.empty();
    for (const auto &entry: expand_keys.origins) {
        const CKeyID &key_id = entry.first;
        CKey key;
        if (!expand_keys.GetKey(key_id, key)) {
            have_all_privkeys = false;
            break;
        }
    }

    // If private keys are enabled, check some things.
    if (!wallet.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
        if (keys.keys.empty()) {
            throw WalletError(
                    "Cannot import descriptor without private keys to a wallet with private keys enabled");
        }
        if (!have_all_privkeys) {
            warnings.push_back(
                    "Not all private keys provided. Some wallet functionality may return unexpected errors");
        }
    }

    WalletDescriptor w_desc(data.parsed_desc, timestamp, data.range_start, data.range_end, data.next_index);

    // Check if the wallet already contains the descriptor
    std::string error;
    auto existing_spk_manager = wallet.GetDescriptorScriptPubKeyMan(w_desc);
    if (existing_spk_manager) {
        if (!existing_spk_manager->CanUpdateToWalletDescriptor(w_desc, error)) {
            throw InvalidParameter(error);
        }
    }

    // Add descriptor to the wallet
    auto spk_manager = wallet.AddWalletDescriptor(w_desc, keys, data.label, data.internal);
    if (spk_manager == nullptr) {
        throw WalletError(strprintf("Could not add descriptor '%s'", data.parsed_desc->ToString()));
    }

    // Set descriptor as active if necessary
    if (data.active) {
        if (!w_desc.descriptor->GetOutputType()) {
            warnings.push_back("Unknown output type, cannot set descriptor to active.");
        } else {
            wallet.AddActiveScriptPubKeyMan(spk_manager->GetID(), *w_desc.descriptor->GetOutputType(),
                                            data.internal);
        }
    } else {
        if (w_desc.descriptor->GetOutputType()) {
            wallet.DeactivateScriptPubKeyMan(spk_manager->GetID(), *w_desc.descriptor->GetOutputType(),
                                             data.internal);
        }
    }
    return true;
}

bool ProcessPublicKey(CWallet &wallet, std::string strLabel, CPubKey pubKey) {
    std::set<CScript> script_pub_keys;
    for (const auto& dest : GetAllDestinationsForKey(pubKey)) {
        script_pub_keys.insert(GetScriptForDestination(dest));
    }

    wallet.MarkDirty();
    wallet.ImportScriptPubKeys(strLabel, script_pub_keys, /*have_solving_data=*/true, /*apply_label=*/true, /*timestamp=*/1);
    wallet.ImportPubKeys({pubKey.GetID()}, {{pubKey.GetID(), pubKey}} , /*key_origins=*/{}, /*add_keypool=*/false, /*internal=*/false, /*timestamp=*/1);
    return true;
}

bool ProcessPrivateKey(CWallet &wallet, std::string strLabel, CKey key) {
    CPubKey pubkey = key.GetPubKey();
    CHECK_NONFATAL(key.VerifyPubKey(pubkey));
    CKeyID vchAddress = pubkey.GetID();
    wallet.MarkDirty();

    // We don't know which corresponding address will be used;
    // label all new addresses, and label existing addresses if a
    // label was passed.
    for (const auto &dest: GetAllDestinationsForKey(pubkey)) {
        if (!strLabel.empty() || !wallet.FindAddressBookEntry(dest)) {
            wallet.SetAddressBook(dest, strLabel, AddressPurpose::RECEIVE);
        }
    }

    // Use timestamp of 1 to scan the whole chain
    if (!wallet.ImportPrivKeys({{vchAddress, key}}, 1)) {
        throw WalletError("Error adding key to wallet");
    }

    // Add the wpkh script for this key if possible
    if (pubkey.IsCompressed()) {
        wallet.ImportScripts({GetScriptForDestination(WitnessV0KeyHash(vchAddress))}, /*timestamp=*/0);
    }
    return true;
}

bool ProcessAddress(CWallet &wallet, std::string strAddress, std::string strLabel, bool fP2SH) {
    CTxDestination dest = DecodeDestination(strAddress);
    if (IsValidDestination(dest)) {
        if (fP2SH) {
            throw InvalidAddressOrKey("Cannot use the p2sh flag with an address - use a script instead");
        }
        if (OutputTypeFromDestination(dest) == OutputType::BECH32M) {
            throw InvalidAddressOrKey("Bech32m addresses cannot be imported into legacy wallets");
        }

        wallet.MarkDirty();
        wallet.ImportScriptPubKeys(strLabel, {GetScriptForDestination(dest)}, /*have_solving_data=*/false, /*apply_label=*/true, /*timestamp=*/1);
    } else if (IsHex(strAddress)) {
        std::vector<unsigned char> data(ParseHex(strAddress));
        CScript redeem_script(data.begin(), data.end());

        std::set<CScript> scripts = {redeem_script};
        wallet.ImportScripts(scripts, /*timestamp=*/0);

        if (fP2SH) {
            scripts.insert(GetScriptForDestination(ScriptHash(redeem_script)));
        }

        wallet.ImportScriptPubKeys(strLabel, scripts, /*have_solving_data=*/false, /*apply_label=*/true, /*timestamp=*/1);
    } else {
        throw InvalidAddressOrKey("Invalid Bitcoin address or script");
    }
    return true;
}
}
