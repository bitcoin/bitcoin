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
