// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <clientversion.h>
#include <core_io.h>
#include <hash.h>
#include <interfaces/chain.h>
#include <key_io.h>
#include <merkleblock.h>
#include <rpc/util.h>
#include <script/descriptor.h>
#include <script/script.h>
#include <script/standard.h>
#include <sync.h>
#include <uint256.h>
#include <util/bip32.h>
#include <util/fs.h>
#include <util/time.h>
#include <util/translation.h>
#include <wallet/rpc/util.h>
#include <wallet/wallet.h>
#include <wallet/imports.h>

#include <cstdint>
#include <fstream>
#include <tuple>
#include <string>

#include <univalue.h>



using interfaces::FoundBlock;

namespace wallet {
std::string static EncodeDumpString(const std::string &str) {
    std::stringstream ret;
    for (const unsigned char c : str) {
        if (c <= 32 || c >= 128 || c == '%') {
            ret << '%' << HexStr({&c, 1});
        } else {
            ret << c;
        }
    }
    return ret.str();
}

static std::string DecodeDumpString(const std::string &str) {
    std::stringstream ret;
    for (unsigned int pos = 0; pos < str.length(); pos++) {
        unsigned char c = str[pos];
        if (c == '%' && pos+2 < str.length()) {
            c = (((str[pos+1]>>6)*9+((str[pos+1]-'0')&15)) << 4) |
                ((str[pos+2]>>6)*9+((str[pos+2]-'0')&15));
            pos += 2;
        }
        ret << c;
    }
    return ret.str();
}

static bool GetWalletAddressesForKey(const LegacyScriptPubKeyMan* spk_man, const CWallet& wallet, const CKeyID& keyid, std::string& strAddr, std::string& strLabel) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet)
{
    bool fLabelFound = false;
    CKey key;
    spk_man->GetKey(keyid, key);
    for (const auto& dest : GetAllDestinationsForKey(key.GetPubKey())) {
        const auto* address_book_entry = wallet.FindAddressBookEntry(dest);
        if (address_book_entry) {
            if (!strAddr.empty()) {
                strAddr += ",";
            }
            strAddr += EncodeDestination(dest);
            strLabel = EncodeDumpString(address_book_entry->GetLabel());
            fLabelFound = true;
        }
    }
    if (!fLabelFound) {
        strAddr = EncodeDestination(GetDestinationForKey(key.GetPubKey(), wallet.m_default_address_type));
    }
    return fLabelFound;
}

static const int64_t TIMESTAMP_MIN = 0;

static void RescanWallet(CWallet& wallet, const WalletRescanReserver& reserver, int64_t time_begin = TIMESTAMP_MIN, bool update = true)
{
    int64_t scanned_time = wallet.RescanFromTime(time_begin, reserver, update);
    if (wallet.IsAbortingRescan()) {
        throw JSONRPCError(RPC_MISC_ERROR, "Rescan aborted by user.");
    } else if (scanned_time > time_begin) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Rescan was unable to fully rescan the blockchain. Some transactions may be missing.");
    }
}

static void EnsureBlockDataFromTime(const CWallet& wallet, int64_t timestamp)
{
    auto& chain{wallet.chain()};
    if (!chain.havePruned()) {
        return;
    }

    int height{0};
    const bool found{chain.findFirstBlockWithTimeAndHeight(timestamp - TIMESTAMP_WINDOW, 0, FoundBlock().height(height))};

    uint256 tip_hash{WITH_LOCK(wallet.cs_wallet, return wallet.GetLastBlockHash())};
    if (found && !chain.hasBlocks(tip_hash, height)) {
        throw JSONRPCError(RPC_WALLET_ERROR, strprintf("Pruned blocks from height %d required to import keys. Use RPC call getblockchaininfo to determine your pruned height.", height));
    }
}

RPCHelpMan importprivkey()
{
    return RPCHelpMan{"importprivkey",
                "\nAdds a private key (as returned by dumpprivkey) to your wallet. Requires a new wallet backup.\n"
                "Hint: use importmulti to import more than one private key.\n"
            "\nNote: This call can take over an hour to complete if rescan is true, during that time, other rpc calls\n"
            "may report that the imported key exists but related transactions are still missing, leading to temporarily incorrect/bogus balances and unspent outputs until rescan completes.\n"
            "The rescan parameter can be set to false if the key was never used to create transactions. If it is set to false,\n"
            "but the key was used to create transactions, rescanblockchain needs to be called with the appropriate block range.\n"
            "Note: Use \"getwalletinfo\" to query the scanning progress.\n"
            "Note: This command is only compatible with legacy wallets. Use \"importdescriptors\" with \"combo(X)\" for descriptor wallets.\n",
                {
                    {"privkey", RPCArg::Type::STR, RPCArg::Optional::NO, "The private key (see dumpprivkey)"},
                    {"label", RPCArg::Type::STR, RPCArg::DefaultHint{"current label if address exists, otherwise \"\""}, "An optional label"},
                    {"rescan", RPCArg::Type::BOOL, RPCArg::Default{true}, "Scan the chain and mempool for wallet transactions."},
                },
                RPCResult{RPCResult::Type::NONE, "", ""},
                RPCExamples{
            "\nDump a private key\n"
            + HelpExampleCli("dumpprivkey", "\"myaddress\"") +
            "\nImport the private key with rescan\n"
            + HelpExampleCli("importprivkey", "\"mykey\"") +
            "\nImport using a label and without rescan\n"
            + HelpExampleCli("importprivkey", "\"mykey\" \"testing\" false") +
            "\nImport using default blank label and without rescan\n"
            + HelpExampleCli("importprivkey", "\"mykey\" \"\" false") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("importprivkey", "\"mykey\", \"testing\", false")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;

    if (pwallet->IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Cannot import private keys to a wallet with private keys disabled");
    }

    EnsureLegacyScriptPubKeyMan(*pwallet, true);

    WalletRescanReserver reserver(*pwallet);
    bool fRescan = true;

    std::string strSecret = request.params[0].get_str();
    const std::string strLabel{LabelFromValue(request.params[1])};

    // Whether to perform rescan after import
    if (!request.params[2].isNull())
        fRescan = request.params[2].get_bool();

    if (fRescan && pwallet->chain().havePruned()) {
        // Exit early and print an error.
        // If a block is pruned after this check, we will import the key(s),
        // but fail the rescan with a generic error.
        throw JSONRPCError(RPC_WALLET_ERROR, "Rescan is disabled when blocks are pruned");
    }

    if (fRescan && !reserver.reserve()) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Wallet is currently rescanning. Abort existing rescan or wait.");
    }

    CKey key = DecodeSecret(strSecret);
    if (!key.IsValid()) throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key encoding");

    {
        LOCK(pwallet->cs_wallet);
        EnsureWalletIsUnlocked(*pwallet);

        try {
            ProcessPrivateKey(*pwallet, strLabel, key);
        } catch (const WalletError& e) {
            throw JSONRPCError(RPC_WALLET_ERROR, e.error);
        }
    }
    if (fRescan) {
        RescanWallet(*pwallet, reserver);
    }

    return UniValue::VNULL;
},
    };
}

RPCHelpMan importaddress()
{
    return RPCHelpMan{"importaddress",
            "\nAdds an address or script (in hex) that can be watched as if it were in your wallet but cannot be used to spend. Requires a new wallet backup.\n"
            "\nNote: This call can take over an hour to complete if rescan is true, during that time, other rpc calls\n"
            "may report that the imported address exists but related transactions are still missing, leading to temporarily incorrect/bogus balances and unspent outputs until rescan completes.\n"
            "The rescan parameter can be set to false if the key was never used to create transactions. If it is set to false,\n"
            "but the key was used to create transactions, rescanblockchain needs to be called with the appropriate block range.\n"
            "If you have the full public key, you should call importpubkey instead of this.\n"
            "Hint: use importmulti to import more than one address.\n"
            "\nNote: If you import a non-standard raw script in hex form, outputs sending to it will be treated\n"
            "as change, and not show up in many RPCs.\n"
            "Note: Use \"getwalletinfo\" to query the scanning progress.\n"
            "Note: This command is only compatible with legacy wallets. Use \"importdescriptors\" for descriptor wallets.\n",
                {
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The Bitcoin address (or hex-encoded script)"},
                    {"label", RPCArg::Type::STR, RPCArg::Default{""}, "An optional label"},
                    {"rescan", RPCArg::Type::BOOL, RPCArg::Default{true}, "Scan the chain and mempool for wallet transactions."},
                    {"p2sh", RPCArg::Type::BOOL, RPCArg::Default{false}, "Add the P2SH version of the script as well"},
                },
                RPCResult{RPCResult::Type::NONE, "", ""},
                RPCExamples{
            "\nImport an address with rescan\n"
            + HelpExampleCli("importaddress", "\"myaddress\"") +
            "\nImport using a label without rescan\n"
            + HelpExampleCli("importaddress", "\"myaddress\" \"testing\" false") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("importaddress", "\"myaddress\", \"testing\", false")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;

    EnsureLegacyScriptPubKeyMan(*pwallet, true);

    const std::string strLabel{LabelFromValue(request.params[1])};

    // Whether to perform rescan after import
    bool fRescan = true;
    if (!request.params[2].isNull())
        fRescan = request.params[2].get_bool();

    if (fRescan && pwallet->chain().havePruned()) {
        // Exit early and print an error.
        // If a block is pruned after this check, we will import the key(s),
        // but fail the rescan with a generic error.
        throw JSONRPCError(RPC_WALLET_ERROR, "Rescan is disabled when blocks are pruned");
    }

    WalletRescanReserver reserver(*pwallet);
    if (fRescan && !reserver.reserve()) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Wallet is currently rescanning. Abort existing rescan or wait.");
    }

    // Whether to import a p2sh version, too
    bool fP2SH = false;
    if (!request.params[3].isNull())
        fP2SH = request.params[3].get_bool();

    std::string strAddress = request.params[0].get_str();

    {
        LOCK(pwallet->cs_wallet);
        try {
            ProcessAddress(*pwallet, strAddress, strLabel, fP2SH);
        } catch (const InvalidAddressOrKey& e) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, e.error);
        }
    }
    if (fRescan)
    {
        RescanWallet(*pwallet, reserver);
        pwallet->ResubmitWalletTransactions(/*relay=*/false, /*force=*/true);
    }

    return UniValue::VNULL;
},
    };
}

RPCHelpMan importprunedfunds()
{
    return RPCHelpMan{"importprunedfunds",
                "\nImports funds without rescan. Corresponding address or script must previously be included in wallet. Aimed towards pruned wallets. The end-user is responsible to import additional transactions that subsequently spend the imported outputs or rescan after the point in the blockchain the transaction is included.\n",
                {
                    {"rawtransaction", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "A raw transaction in hex funding an already-existing address in wallet"},
                    {"txoutproof", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The hex output from gettxoutproof that contains the transaction"},
                },
                RPCResult{RPCResult::Type::NONE, "", ""},
                RPCExamples{""},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;

    CMutableTransaction tx;
    if (!DecodeHexTx(tx, request.params[0].get_str())) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed. Make sure the tx has at least one input.");
    }
    uint256 hashTx = tx.GetHash();

    DataStream ssMB{ParseHexV(request.params[1], "proof")};
    CMerkleBlock merkleBlock;
    ssMB >> merkleBlock;

    //Search partial merkle tree in proof for our transaction and index in valid block
    std::vector<uint256> vMatch;
    std::vector<unsigned int> vIndex;
    if (merkleBlock.txn.ExtractMatches(vMatch, vIndex) != merkleBlock.header.hashMerkleRoot) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Something wrong with merkleblock");
    }

    LOCK(pwallet->cs_wallet);
    int height;
    if (!pwallet->chain().findAncestorByHash(pwallet->GetLastBlockHash(), merkleBlock.header.GetHash(), FoundBlock().height(height))) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found in chain");
    }

    std::vector<uint256>::const_iterator it;
    if ((it = std::find(vMatch.begin(), vMatch.end(), hashTx)) == vMatch.end()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Transaction given doesn't exist in proof");
    }

    unsigned int txnIndex = vIndex[it - vMatch.begin()];

    CTransactionRef tx_ref = MakeTransactionRef(tx);
    if (pwallet->IsMine(*tx_ref)) {
        pwallet->AddToWallet(std::move(tx_ref), TxStateConfirmed{merkleBlock.header.GetHash(), height, static_cast<int>(txnIndex)});
        return UniValue::VNULL;
    }

    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No addresses in wallet correspond to included transaction");
},
    };
}

RPCHelpMan removeprunedfunds()
{
    return RPCHelpMan{"removeprunedfunds",
                "\nDeletes the specified transaction from the wallet. Meant for use with pruned wallets and as a companion to importprunedfunds. This will affect wallet balances.\n",
                {
                    {"txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "The hex-encoded id of the transaction you are deleting"},
                },
                RPCResult{RPCResult::Type::NONE, "", ""},
                RPCExamples{
                    HelpExampleCli("removeprunedfunds", "\"a8d0c0184dde994a09ec054286f1ce581bebf46446a512166eae7628734ea0a5\"") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("removeprunedfunds", "\"a8d0c0184dde994a09ec054286f1ce581bebf46446a512166eae7628734ea0a5\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;

    LOCK(pwallet->cs_wallet);

    uint256 hash(ParseHashV(request.params[0], "txid"));
    std::vector<uint256> vHash;
    vHash.push_back(hash);
    std::vector<uint256> vHashOut;

    if (pwallet->ZapSelectTx(vHash, vHashOut) != DBErrors::LOAD_OK) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Could not properly delete the transaction.");
    }

    if(vHashOut.empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Transaction does not exist in wallet.");
    }

    return UniValue::VNULL;
},
    };
}

RPCHelpMan importpubkey()
{
    return RPCHelpMan{"importpubkey",
                "\nAdds a public key (in hex) that can be watched as if it were in your wallet but cannot be used to spend. Requires a new wallet backup.\n"
                "Hint: use importmulti to import more than one public key.\n"
            "\nNote: This call can take over an hour to complete if rescan is true, during that time, other rpc calls\n"
            "may report that the imported pubkey exists but related transactions are still missing, leading to temporarily incorrect/bogus balances and unspent outputs until rescan completes.\n"
            "The rescan parameter can be set to false if the key was never used to create transactions. If it is set to false,\n"
            "but the key was used to create transactions, rescanblockchain needs to be called with the appropriate block range.\n"
            "Note: Use \"getwalletinfo\" to query the scanning progress.\n"
            "Note: This command is only compatible with legacy wallets. Use \"importdescriptors\" with \"combo(X)\" for descriptor wallets.\n",
                {
                    {"pubkey", RPCArg::Type::STR, RPCArg::Optional::NO, "The hex-encoded public key"},
                    {"label", RPCArg::Type::STR, RPCArg::Default{""}, "An optional label"},
                    {"rescan", RPCArg::Type::BOOL, RPCArg::Default{true}, "Scan the chain and mempool for wallet transactions."},
                },
                RPCResult{RPCResult::Type::NONE, "", ""},
                RPCExamples{
            "\nImport a public key with rescan\n"
            + HelpExampleCli("importpubkey", "\"mypubkey\"") +
            "\nImport using a label without rescan\n"
            + HelpExampleCli("importpubkey", "\"mypubkey\" \"testing\" false") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("importpubkey", "\"mypubkey\", \"testing\", false")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;

    EnsureLegacyScriptPubKeyMan(*pwallet, true);

    const std::string strLabel{LabelFromValue(request.params[1])};

    // Whether to perform rescan after import
    bool fRescan = true;
    if (!request.params[2].isNull())
        fRescan = request.params[2].get_bool();

    if (fRescan && pwallet->chain().havePruned()) {
        // Exit early and print an error.
        // If a block is pruned after this check, we will import the key(s),
        // but fail the rescan with a generic error.
        throw JSONRPCError(RPC_WALLET_ERROR, "Rescan is disabled when blocks are pruned");
    }

    WalletRescanReserver reserver(*pwallet);
    if (fRescan && !reserver.reserve()) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Wallet is currently rescanning. Abort existing rescan or wait.");
    }

    if (!IsHex(request.params[0].get_str()))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Pubkey must be a hex string");
    std::vector<unsigned char> data(ParseHex(request.params[0].get_str()));
    CPubKey pubKey(data);
    if (!pubKey.IsFullyValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Pubkey is not a valid public key");

    {
        LOCK(pwallet->cs_wallet);
        ProcessPublicKey(*pwallet, strLabel, pubKey);
    }
    if (fRescan)
    {
        RescanWallet(*pwallet, reserver);
        pwallet->ResubmitWalletTransactions(/*relay=*/false, /*force=*/true);
    }

    return UniValue::VNULL;
},
    };
}


RPCHelpMan importwallet()
{
    return RPCHelpMan{"importwallet",
                "\nImports keys from a wallet dump file (see dumpwallet). Requires a new wallet backup to include imported keys.\n"
                "Note: Blockchain and Mempool will be rescanned after a successful import. Use \"getwalletinfo\" to query the scanning progress.\n"
                "Note: This command is only compatible with legacy wallets.\n",
                {
                    {"filename", RPCArg::Type::STR, RPCArg::Optional::NO, "The wallet file"},
                },
                RPCResult{RPCResult::Type::NONE, "", ""},
                RPCExamples{
            "\nDump the wallet\n"
            + HelpExampleCli("dumpwallet", "\"test\"") +
            "\nImport the wallet\n"
            + HelpExampleCli("importwallet", "\"test\"") +
            "\nImport using the json rpc call\n"
            + HelpExampleRpc("importwallet", "\"test\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;

    EnsureLegacyScriptPubKeyMan(*pwallet, true);

    WalletRescanReserver reserver(*pwallet);
    if (!reserver.reserve()) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Wallet is currently rescanning. Abort existing rescan or wait.");
    }

    int64_t nTimeBegin = 0;
    bool fGood = true;
    {
        LOCK(pwallet->cs_wallet);

        EnsureWalletIsUnlocked(*pwallet);

        std::ifstream file;
        file.open(fs::u8path(request.params[0].get_str()), std::ios::in | std::ios::ate);
        if (!file.is_open()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot open wallet dump file");
        }
        CHECK_NONFATAL(pwallet->chain().findBlock(pwallet->GetLastBlockHash(), FoundBlock().time(nTimeBegin)));

        int64_t nFilesize = std::max((int64_t)1, (int64_t)file.tellg());
        file.seekg(0, file.beg);

        // Use uiInterface.ShowProgress instead of pwallet.ShowProgress because pwallet.ShowProgress has a cancel button tied to AbortRescan which
        // we don't want for this progress bar showing the import progress. uiInterface.ShowProgress does not have a cancel button.
        pwallet->chain().showProgress(strprintf("%s " + _("Importing…").translated, pwallet->GetDisplayName()), 0, false); // show progress dialog in GUI
        std::vector<std::tuple<CKey, int64_t, bool, std::string>> keys;
        std::vector<std::pair<CScript, int64_t>> scripts;
        while (file.good()) {
            pwallet->chain().showProgress("", std::max(1, std::min(50, (int)(((double)file.tellg() / (double)nFilesize) * 100))), false);
            std::string line;
            std::getline(file, line);
            if (line.empty() || line[0] == '#')
                continue;

            std::vector<std::string> vstr = SplitString(line, ' ');
            if (vstr.size() < 2)
                continue;
            CKey key = DecodeSecret(vstr[0]);
            if (key.IsValid()) {
                int64_t nTime = ParseISO8601DateTime(vstr[1]);
                std::string strLabel;
                bool fLabel = true;
                for (unsigned int nStr = 2; nStr < vstr.size(); nStr++) {
                    if (vstr[nStr].front() == '#')
                        break;
                    if (vstr[nStr] == "change=1")
                        fLabel = false;
                    if (vstr[nStr] == "reserve=1")
                        fLabel = false;
                    if (vstr[nStr].substr(0,6) == "label=") {
                        strLabel = DecodeDumpString(vstr[nStr].substr(6));
                        fLabel = true;
                    }
                }
                nTimeBegin = std::min(nTimeBegin, nTime);
                keys.push_back(std::make_tuple(key, nTime, fLabel, strLabel));
            } else if(IsHex(vstr[0])) {
                std::vector<unsigned char> vData(ParseHex(vstr[0]));
                CScript script = CScript(vData.begin(), vData.end());
                int64_t birth_time = ParseISO8601DateTime(vstr[1]);
                if (birth_time > 0) nTimeBegin = std::min(nTimeBegin, birth_time);
                scripts.push_back(std::pair<CScript, int64_t>(script, birth_time));
            }
        }
        file.close();
        EnsureBlockDataFromTime(*pwallet, nTimeBegin);
        // We now know whether we are importing private keys, so we can error if private keys are disabled
        if (keys.size() > 0 && pwallet->IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
            pwallet->chain().showProgress("", 100, false); // hide progress dialog in GUI
            throw JSONRPCError(RPC_WALLET_ERROR, "Importing wallets is disabled when private keys are disabled");
        }
        double total = (double)(keys.size() + scripts.size());
        double progress = 0;
        for (const auto& key_tuple : keys) {
            pwallet->chain().showProgress("", std::max(50, std::min(75, (int)((progress / total) * 100) + 50)), false);
            const CKey& key = std::get<0>(key_tuple);
            int64_t time = std::get<1>(key_tuple);
            bool has_label = std::get<2>(key_tuple);
            std::string label = std::get<3>(key_tuple);

            CPubKey pubkey = key.GetPubKey();
            CHECK_NONFATAL(key.VerifyPubKey(pubkey));
            CKeyID keyid = pubkey.GetID();

            pwallet->WalletLogPrintf("Importing %s...\n", EncodeDestination(PKHash(keyid)));

            if (!pwallet->ImportPrivKeys({{keyid, key}}, time)) {
                pwallet->WalletLogPrintf("Error importing key for %s\n", EncodeDestination(PKHash(keyid)));
                fGood = false;
                continue;
            }

            if (has_label)
                pwallet->SetAddressBook(PKHash(keyid), label, AddressPurpose::RECEIVE);
            progress++;
        }
        for (const auto& script_pair : scripts) {
            pwallet->chain().showProgress("", std::max(50, std::min(75, (int)((progress / total) * 100) + 50)), false);
            const CScript& script = script_pair.first;
            int64_t time = script_pair.second;

            if (!pwallet->ImportScripts({script}, time)) {
                pwallet->WalletLogPrintf("Error importing script %s\n", HexStr(script));
                fGood = false;
                continue;
            }

            progress++;
        }
        pwallet->chain().showProgress("", 100, false); // hide progress dialog in GUI
    }
    pwallet->chain().showProgress("", 100, false); // hide progress dialog in GUI
    RescanWallet(*pwallet, reserver, nTimeBegin, /*update=*/false);
    pwallet->MarkDirty();

    if (!fGood)
        throw JSONRPCError(RPC_WALLET_ERROR, "Error adding some keys/scripts to wallet");

    return UniValue::VNULL;
},
    };
}

RPCHelpMan dumpprivkey()
{
    return RPCHelpMan{"dumpprivkey",
                "\nReveals the private key corresponding to 'address'.\n"
                "Then the importprivkey can be used with this output\n"
                "Note: This command is only compatible with legacy wallets.\n",
                {
                    {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The bitcoin address for the private key"},
                },
                RPCResult{
                    RPCResult::Type::STR, "key", "The private key"
                },
                RPCExamples{
                    HelpExampleCli("dumpprivkey", "\"myaddress\"")
            + HelpExampleCli("importprivkey", "\"mykey\"")
            + HelpExampleRpc("dumpprivkey", "\"myaddress\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const std::shared_ptr<const CWallet> pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;

    const LegacyScriptPubKeyMan& spk_man = EnsureConstLegacyScriptPubKeyMan(*pwallet);

    LOCK2(pwallet->cs_wallet, spk_man.cs_KeyStore);

    EnsureWalletIsUnlocked(*pwallet);

    std::string strAddress = request.params[0].get_str();
    CTxDestination dest = DecodeDestination(strAddress);
    if (!IsValidDestination(dest)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Bitcoin address");
    }
    auto keyid = GetKeyForDestination(spk_man, dest);
    if (keyid.IsNull()) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Address does not refer to a key");
    }
    CKey vchSecret;
    if (!spk_man.GetKey(keyid, vchSecret)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Private key for address " + strAddress + " is not known");
    }
    return EncodeSecret(vchSecret);
},
    };
}


RPCHelpMan dumpwallet()
{
    return RPCHelpMan{"dumpwallet",
                "\nDumps all wallet keys in a human-readable format to a server-side file. This does not allow overwriting existing files.\n"
                "Imported scripts are included in the dumpfile, but corresponding BIP173 addresses, etc. may not be added automatically by importwallet.\n"
                "Note that if your wallet contains keys which are not derived from your HD seed (e.g. imported keys), these are not covered by\n"
                "only backing up the seed itself, and must be backed up too (e.g. ensure you back up the whole dumpfile).\n"
                "Note: This command is only compatible with legacy wallets.\n",
                {
                    {"filename", RPCArg::Type::STR, RPCArg::Optional::NO, "The filename with path (absolute path recommended)"},
                },
                RPCResult{
                    RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::STR, "filename", "The filename with full absolute path"},
                    }
                },
                RPCExamples{
                    HelpExampleCli("dumpwallet", "\"test\"")
            + HelpExampleRpc("dumpwallet", "\"test\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const std::shared_ptr<const CWallet> pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;

    const CWallet& wallet = *pwallet;
    const LegacyScriptPubKeyMan& spk_man = EnsureConstLegacyScriptPubKeyMan(wallet);

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    wallet.BlockUntilSyncedToCurrentChain();

    LOCK(wallet.cs_wallet);

    EnsureWalletIsUnlocked(wallet);

    fs::path filepath = fs::u8path(request.params[0].get_str());
    filepath = fs::absolute(filepath);

    /* Prevent arbitrary files from being overwritten. There have been reports
     * that users have overwritten wallet files this way:
     * https://github.com/bitcoin/bitcoin/issues/9934
     * It may also avoid other security issues.
     */
    if (fs::exists(filepath)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, filepath.u8string() + " already exists. If you are sure this is what you want, move it out of the way first");
    }

    std::ofstream file;
    file.open(filepath);
    if (!file.is_open())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot open wallet dump file");

    std::map<CKeyID, int64_t> mapKeyBirth;
    wallet.GetKeyBirthTimes(mapKeyBirth);

    int64_t block_time = 0;
    CHECK_NONFATAL(wallet.chain().findBlock(wallet.GetLastBlockHash(), FoundBlock().time(block_time)));

    // Note: To avoid a lock order issue, access to cs_main must be locked before cs_KeyStore.
    // So we do the two things in this function that lock cs_main first: GetKeyBirthTimes, and findBlock.
    LOCK(spk_man.cs_KeyStore);

    const std::map<CKeyID, int64_t>& mapKeyPool = spk_man.GetAllReserveKeys();
    std::set<CScriptID> scripts = spk_man.GetCScripts();

    // sort time/key pairs
    std::vector<std::pair<int64_t, CKeyID> > vKeyBirth;
    vKeyBirth.reserve(mapKeyBirth.size());
    for (const auto& entry : mapKeyBirth) {
        vKeyBirth.push_back(std::make_pair(entry.second, entry.first));
    }
    mapKeyBirth.clear();
    std::sort(vKeyBirth.begin(), vKeyBirth.end());

    // produce output
    file << strprintf("# Wallet dump created by %s %s\n", PACKAGE_NAME, FormatFullVersion());
    file << strprintf("# * Created on %s\n", FormatISO8601DateTime(GetTime()));
    file << strprintf("# * Best block at time of backup was %i (%s),\n", wallet.GetLastBlockHeight(), wallet.GetLastBlockHash().ToString());
    file << strprintf("#   mined on %s\n", FormatISO8601DateTime(block_time));
    file << "\n";

    // add the base58check encoded extended master if the wallet uses HD
    CKeyID seed_id = spk_man.GetHDChain().seed_id;
    if (!seed_id.IsNull())
    {
        CKey seed;
        if (spk_man.GetKey(seed_id, seed)) {
            CExtKey masterKey;
            masterKey.SetSeed(seed);

            file << "# extended private masterkey: " << EncodeExtKey(masterKey) << "\n\n";
        }
    }
    for (std::vector<std::pair<int64_t, CKeyID> >::const_iterator it = vKeyBirth.begin(); it != vKeyBirth.end(); it++) {
        const CKeyID &keyid = it->second;
        std::string strTime = FormatISO8601DateTime(it->first);
        std::string strAddr;
        std::string strLabel;
        CKey key;
        if (spk_man.GetKey(keyid, key)) {
            CKeyMetadata metadata;
            const auto it{spk_man.mapKeyMetadata.find(keyid)};
            if (it != spk_man.mapKeyMetadata.end()) metadata = it->second;
            file << strprintf("%s %s ", EncodeSecret(key), strTime);
            if (GetWalletAddressesForKey(&spk_man, wallet, keyid, strAddr, strLabel)) {
                file << strprintf("label=%s", strLabel);
            } else if (keyid == seed_id) {
                file << "hdseed=1";
            } else if (mapKeyPool.count(keyid)) {
                file << "reserve=1";
            } else if (metadata.hdKeypath == "s") {
                file << "inactivehdseed=1";
            } else {
                file << "change=1";
            }
            file << strprintf(" # addr=%s%s\n", strAddr, (metadata.has_key_origin ? " hdkeypath="+WriteHDKeypath(metadata.key_origin.path) : ""));
        }
    }
    file << "\n";
    for (const CScriptID &scriptid : scripts) {
        CScript script;
        std::string create_time = "0";
        std::string address = EncodeDestination(ScriptHash(scriptid));
        // get birth times for scripts with metadata
        auto it = spk_man.m_script_metadata.find(scriptid);
        if (it != spk_man.m_script_metadata.end()) {
            create_time = FormatISO8601DateTime(it->second.nCreateTime);
        }
        if(spk_man.GetCScript(scriptid, script)) {
            file << strprintf("%s %s script=1", HexStr(script), create_time);
            file << strprintf(" # addr=%s\n", address);
        }
    }
    file << "\n";
    file << "# End of dump\n";
    file.close();

    UniValue reply(UniValue::VOBJ);
    reply.pushKV("filename", filepath.u8string());

    return reply;
},
    };
}

static int64_t GetImportTimestamp(const UniValue& data, int64_t now)
{
    if (data.exists("timestamp")) {
        const UniValue& timestamp = data["timestamp"];
        if (timestamp.isNum()) {
            return timestamp.getInt<int64_t>();
        } else if (timestamp.isStr() && timestamp.get_str() == "now") {
            return now;
        }
        throw JSONRPCError(RPC_TYPE_ERROR, strprintf("Expected number or \"now\" timestamp value for key. got type %s", uvTypeName(timestamp.type())));
    }
    throw JSONRPCError(RPC_TYPE_ERROR, "Missing required timestamp field for key");
}

RPCHelpMan importmulti()
{
    return RPCHelpMan{"importmulti",
                "\nImport addresses/scripts (with private or public keys, redeem script (P2SH)), optionally rescanning the blockchain from the earliest creation time of the imported scripts. Requires a new wallet backup.\n"
                "If an address/script is imported without all of the private keys required to spend from that address, it will be watchonly. The 'watchonly' option must be set to true in this case or a warning will be returned.\n"
                "Conversely, if all the private keys are provided and the address/script is spendable, the watchonly option must be set to false, or a warning will be returned.\n"
            "\nNote: This call can take over an hour to complete if rescan is true, during that time, other rpc calls\n"
            "may report that the imported keys, addresses or scripts exist but related transactions are still missing.\n"
            "The rescan parameter can be set to false if the key was never used to create transactions. If it is set to false,\n"
            "but the key was used to create transactions, rescanblockchain needs to be called with the appropriate block range.\n"
            "Note: Use \"getwalletinfo\" to query the scanning progress.\n"
            "Note: This command is only compatible with legacy wallets. Use \"importdescriptors\" for descriptor wallets.\n",
                {
                    {"requests", RPCArg::Type::ARR, RPCArg::Optional::NO, "Data to be imported",
                        {
                            {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "",
                                {
                                    {"desc", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Descriptor to import. If using descriptor, do not also provide address/scriptPubKey, scripts, or pubkeys"},
                                    {"scriptPubKey", RPCArg::Type::STR, RPCArg::Optional::NO, "Type of scriptPubKey (string for script, json for address). Should not be provided if using a descriptor",
                                        RPCArgOptions{.type_str={"\"<script>\" | { \"address\":\"<address>\" }", "string / json"}}
                                    },
                                    {"timestamp", RPCArg::Type::NUM, RPCArg::Optional::NO, "Creation time of the key expressed in " + UNIX_EPOCH_TIME + ",\n"
                                        "or the string \"now\" to substitute the current synced blockchain time. The timestamp of the oldest\n"
                                        "key will determine how far back blockchain rescans need to begin for missing wallet transactions.\n"
                                        "\"now\" can be specified to bypass scanning, for keys which are known to never have been used, and\n"
                                        "0 can be specified to scan the entire blockchain. Blocks up to 2 hours before the earliest key\n"
                                        "creation time of all keys being imported by the importmulti call will be scanned.",
                                        RPCArgOptions{.type_str={"timestamp | \"now\"", "integer / string"}}
                                    },
                                    {"redeemscript", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Allowed only if the scriptPubKey is a P2SH or P2SH-P2WSH address/scriptPubKey"},
                                    {"witnessscript", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "Allowed only if the scriptPubKey is a P2SH-P2WSH or P2WSH address/scriptPubKey"},
                                    {"pubkeys", RPCArg::Type::ARR, RPCArg::Default{UniValue::VARR}, "Array of strings giving pubkeys to import. They must occur in P2PKH or P2WPKH scripts. They are not required when the private key is also provided (see the \"keys\" argument).",
                                        {
                                            {"pubKey", RPCArg::Type::STR, RPCArg::Optional::OMITTED, ""},
                                        }
                                    },
                                    {"keys", RPCArg::Type::ARR, RPCArg::Default{UniValue::VARR}, "Array of strings giving private keys to import. The corresponding public keys must occur in the output or redeemscript.",
                                        {
                                            {"key", RPCArg::Type::STR, RPCArg::Optional::OMITTED, ""},
                                        }
                                    },
                                    {"range", RPCArg::Type::RANGE, RPCArg::Optional::OMITTED, "If a ranged descriptor is used, this specifies the end or the range (in the form [begin,end]) to import"},
                                    {"internal", RPCArg::Type::BOOL, RPCArg::Default{false}, "Stating whether matching outputs should be treated as not incoming payments (also known as change)"},
                                    {"watchonly", RPCArg::Type::BOOL, RPCArg::Default{false}, "Stating whether matching outputs should be considered watchonly."},
                                    {"label", RPCArg::Type::STR, RPCArg::Default{""}, "Label to assign to the address, only allowed with internal=false"},
                                    {"keypool", RPCArg::Type::BOOL, RPCArg::Default{false}, "Stating whether imported public keys should be added to the keypool for when users request new addresses. Only allowed when wallet private keys are disabled"},
                                },
                            },
                        },
                        RPCArgOptions{.oneline_description="\"requests\""}},
                    {"options", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "",
                        {
                            {"rescan", RPCArg::Type::BOOL, RPCArg::Default{true}, "Scan the chain and mempool for wallet transactions after all imports."},
                        },
                        RPCArgOptions{.oneline_description="\"options\""}},
                },
                RPCResult{
                    RPCResult::Type::ARR, "", "Response is an array with the same size as the input that has the execution result",
                    {
                        {RPCResult::Type::OBJ, "", "",
                        {
                            {RPCResult::Type::BOOL, "success", ""},
                            {RPCResult::Type::ARR, "warnings", /*optional=*/true, "",
                            {
                                {RPCResult::Type::STR, "", ""},
                            }},
                            {RPCResult::Type::OBJ, "error", /*optional=*/true, "",
                            {
                                {RPCResult::Type::ELISION, "", "JSONRPC error"},
                            }},
                        }},
                    }
                },
                RPCExamples{
                    HelpExampleCli("importmulti", "'[{ \"scriptPubKey\": { \"address\": \"<my address>\" }, \"timestamp\":1455191478 }, "
                                          "{ \"scriptPubKey\": { \"address\": \"<my 2nd address>\" }, \"label\": \"example 2\", \"timestamp\": 1455191480 }]'") +
                    HelpExampleCli("importmulti", "'[{ \"scriptPubKey\": { \"address\": \"<my address>\" }, \"timestamp\":1455191478 }]' '{ \"rescan\": false}'")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& mainRequest) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(mainRequest);
    if (!pwallet) return UniValue::VNULL;
    CWallet& wallet{*pwallet};

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    wallet.BlockUntilSyncedToCurrentChain();

    EnsureLegacyScriptPubKeyMan(*pwallet, true);

    const UniValue& requests = mainRequest.params[0];

    //Default options
    bool fRescan = true;

    if (!mainRequest.params[1].isNull()) {
        const UniValue& options = mainRequest.params[1];

        if (options.exists("rescan")) {
            fRescan = options["rescan"].get_bool();
        }
    }

    WalletRescanReserver reserver(*pwallet);
    if (fRescan && !reserver.reserve()) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Wallet is currently rescanning. Abort existing rescan or wait.");
    }

    int64_t now = 0;
    bool fRunScan = false;
    int64_t nLowestTimestamp = 0;
    UniValue response(UniValue::VARR);
    {
        LOCK(pwallet->cs_wallet);

        // Check all requests are watchonly
        bool is_watchonly{true};
        for (size_t i = 0; i < requests.size(); ++i) {
            const UniValue& request = requests[i];
            if (!request.exists("watchonly") || !request["watchonly"].get_bool()) {
                is_watchonly = false;
                break;
            }
        }
        // Wallet does not need to be unlocked if all requests are watchonly
        if (!is_watchonly) EnsureWalletIsUnlocked(wallet);

        // Verify all timestamps are present before importing any keys.
        CHECK_NONFATAL(pwallet->chain().findBlock(pwallet->GetLastBlockHash(), FoundBlock().time(nLowestTimestamp).mtpTime(now)));
        for (const UniValue& data : requests.getValues()) {
            GetImportTimestamp(data, now);
        }

        const int64_t minimumTimestamp = 1;

        for (const UniValue& data : requests.getValues()) {
            const int64_t timestamp = std::max(GetImportTimestamp(data, now), minimumTimestamp);
            ImportMultiData multi_data;
            std::vector<std::string> warnings;
            UniValue result(UniValue::VOBJ);
            try {
                multi_data.internal = data.exists("internal") ? data["internal"].get_bool() : false;
                // Internal addresses should not have a label
                if (multi_data.internal && data.exists("label")) {
                    throw InvalidParameter("Internal addresses should not have a label");
                }
                multi_data.label = LabelFromValue(data["label"]);
                multi_data.keypool = data.exists("keypool") ? data["keypool"].get_bool() : false;
                multi_data.watch_only = data.exists("watchonly") ? data["watchonly"].get_bool() : false;
                if (data.exists("keys"))
                    for (size_t i = 0; i < data["keys"].get_array().size(); ++i)
                        multi_data.private_keys.push_back(data["keys"].get_array()[i].get_str());

                if (data.exists("scriptPubKey") && data.exists("desc")) {
                    throw InvalidParameter("Both a descriptor and a scriptPubKey should not be provided.");
                } else if (data.exists("scriptPubKey")) {
                    // First ensure scriptPubKey has either a script or JSON with "address" string
                    const UniValue& scriptPubKey = data["scriptPubKey"];
                    multi_data.isScript = scriptPubKey.getType() == UniValue::VSTR;
                    if (!multi_data.isScript && !(scriptPubKey.getType() == UniValue::VOBJ && scriptPubKey.exists("address"))) {
                        throw InvalidParameter("scriptPubKey must be string with script or JSON with address string");
                    }
                    multi_data.scriptPubKey = multi_data.isScript ? scriptPubKey.get_str() : scriptPubKey["address"].get_str();

                    // Optional fields.
                    multi_data.redeem_script = data.exists("redeemscript") ? data["redeemscript"].get_str() : "";
                    multi_data.witness_script = data.exists("witnessscript") ? data["witnessscript"].get_str() : "";
                    if (data.exists("pubkeys"))
                        for (size_t i = 0; i < data["pubkeys"].get_array().size(); ++i)
                            multi_data.public_keys.push_back(data["pubkeys"].get_array()[i].get_str());

                    if (data.exists("range")) {
                        throw InvalidParameter("Range should not be specified for a non-descriptor import");
                    }
                } else if (data.exists("desc")) {
                    std::string error;
                    multi_data.parsed_desc = Parse(data["desc"].get_str(), multi_data.keys, error, true);
                    if (!multi_data.parsed_desc) {
                        throw InvalidAddressOrKey(error);
                    }
                    if (multi_data.parsed_desc->GetOutputType() == OutputType::BECH32M) {
                        throw InvalidAddressOrKey("Bech32m descriptors cannot be imported into legacy wallets");
                    }

                    if (!multi_data.parsed_desc->IsRange() && data.exists("range")) {
                        throw InvalidParameter("Range should not be specified for an un-ranged descriptor");
                    } else if (multi_data.parsed_desc->IsRange()) {
                        if (!data.exists("range")) {
                            throw InvalidParameter("Descriptor is ranged, please specify the range");
                        }
                        std::tie(multi_data.range_start, multi_data.range_end) = ParseDescriptorRange(data["range"]);
                    }
                } else {
                    throw InvalidParameter("Either a descriptor or scriptPubKey must be provided.");
                }

                ProcessImport(*pwallet, multi_data, warnings, timestamp);
                result.pushKV("success", UniValue(true));
            } catch (const UniValue& e) {
                result.pushKV("success", UniValue(false));
                result.pushKV("error", e);
            } catch (const MiscError& e) {
                result.pushKV("success", UniValue(false));
                result.pushKV("error", JSONRPCError(RPC_MISC_ERROR, e.error));
            } catch (const WalletError& e) {
                result.pushKV("success", UniValue(false));
                result.pushKV("error", JSONRPCError(RPC_WALLET_ERROR, e.error));
            } catch (const InvalidAddressOrKey& e) {
                result.pushKV("success", UniValue(false));
                result.pushKV("error", JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, e.error));
            } catch (const InvalidParameter& e) {
                result.pushKV("success", UniValue(false));
                result.pushKV("error", JSONRPCError(RPC_INVALID_PARAMETER, e.error));
            } catch (...) {
                result.pushKV("success", UniValue(false));
                result.pushKV("error", JSONRPCError(RPC_MISC_ERROR, "Missing required fields"));
            }
            if (warnings.size()) {
                UniValue warning(UniValue::VARR);
                for (const auto& w: warnings)
                    warning.push_back(w);
                PushWarnings(warning, result);
            }

            response.push_back(result);

            if (!fRescan) {
                continue;
            }

            // If at least one request was successful then allow rescan.
            if (result["success"].get_bool()) {
                fRunScan = true;
            }

            // Get the lowest timestamp.
            if (timestamp < nLowestTimestamp) {
                nLowestTimestamp = timestamp;
            }
        }
    }
    if (fRescan && fRunScan && requests.size()) {
        int64_t scannedTime = pwallet->RescanFromTime(nLowestTimestamp, reserver, /*update=*/true);
        pwallet->ResubmitWalletTransactions(/*relay=*/false, /*force=*/true);

        if (pwallet->IsAbortingRescan()) {
            throw JSONRPCError(RPC_MISC_ERROR, "Rescan aborted by user.");
        }
        if (scannedTime > nLowestTimestamp) {
            std::vector<UniValue> results = response.getValues();
            response.clear();
            response.setArray();
            size_t i = 0;
            for (const UniValue& request : requests.getValues()) {
                // If key creation date is within the successfully scanned
                // range, or if the import result already has an error set, let
                // the result stand unmodified. Otherwise replace the result
                // with an error message.
                if (scannedTime <= GetImportTimestamp(request, now) || results.at(i).exists("error")) {
                    response.push_back(results.at(i));
                } else {
                    UniValue result = UniValue(UniValue::VOBJ);
                    result.pushKV("success", UniValue(false));
                    result.pushKV(
                        "error",
                        JSONRPCError(
                            RPC_MISC_ERROR,
                            strprintf("Rescan failed for key with creation timestamp %d. There was an error reading a "
                                      "block from time %d, which is after or within %d seconds of key creation, and "
                                      "could contain transactions pertaining to the key. As a result, transactions "
                                      "and coins using this key may not appear in the wallet. This error could be "
                                      "caused by pruning or data corruption (see bitcoind log for details) and could "
                                      "be dealt with by downloading and rescanning the relevant blocks (see -reindex "
                                      "option and rescanblockchain RPC).",
                                GetImportTimestamp(request, now), scannedTime - TIMESTAMP_WINDOW - 1, TIMESTAMP_WINDOW)));
                    response.push_back(std::move(result));
                }
                ++i;
            }
        }
    }

    return response;
},
    };
}

RPCHelpMan importdescriptors()
{
    return RPCHelpMan{"importdescriptors",
                "\nImport descriptors. This will trigger a rescan of the blockchain based on the earliest timestamp of all descriptors being imported. Requires a new wallet backup.\n"
            "\nNote: This call can take over an hour to complete if using an early timestamp; during that time, other rpc calls\n"
            "may report that the imported keys, addresses or scripts exist but related transactions are still missing.\n"
            "The rescan is significantly faster if block filters are available (using startup option \"-blockfilterindex=1\").\n",
                {
                    {"requests", RPCArg::Type::ARR, RPCArg::Optional::NO, "Data to be imported",
                        {
                            {"", RPCArg::Type::OBJ, RPCArg::Optional::OMITTED, "",
                                {
                                    {"desc", RPCArg::Type::STR, RPCArg::Optional::NO, "Descriptor to import."},
                                    {"active", RPCArg::Type::BOOL, RPCArg::Default{false}, "Set this descriptor to be the active descriptor for the corresponding output type/externality"},
                                    {"range", RPCArg::Type::RANGE, RPCArg::Optional::OMITTED, "If a ranged descriptor is used, this specifies the end or the range (in the form [begin,end]) to import"},
                                    {"next_index", RPCArg::Type::NUM, RPCArg::Optional::OMITTED, "If a ranged descriptor is set to active, this specifies the next index to generate addresses from"},
                                    {"timestamp", RPCArg::Type::NUM, RPCArg::Optional::NO, "Time from which to start rescanning the blockchain for this descriptor, in " + UNIX_EPOCH_TIME + "\n"
                                        "Use the string \"now\" to substitute the current synced blockchain time.\n"
                                        "\"now\" can be specified to bypass scanning, for outputs which are known to never have been used, and\n"
                                        "0 can be specified to scan the entire blockchain. Blocks up to 2 hours before the earliest timestamp\n"
                                        "of all descriptors being imported will be scanned as well as the mempool.",
                                        RPCArgOptions{.type_str={"timestamp | \"now\"", "integer / string"}}
                                    },
                                    {"internal", RPCArg::Type::BOOL, RPCArg::Default{false}, "Whether matching outputs should be treated as not incoming payments (e.g. change)"},
                                    {"label", RPCArg::Type::STR, RPCArg::Default{""}, "Label to assign to the address, only allowed with internal=false. Disabled for ranged descriptors"},
                                },
                            },
                        },
                        RPCArgOptions{.oneline_description="\"requests\""}},
                },
                RPCResult{
                    RPCResult::Type::ARR, "", "Response is an array with the same size as the input that has the execution result",
                    {
                        {RPCResult::Type::OBJ, "", "",
                        {
                            {RPCResult::Type::BOOL, "success", ""},
                            {RPCResult::Type::ARR, "warnings", /*optional=*/true, "",
                            {
                                {RPCResult::Type::STR, "", ""},
                            }},
                            {RPCResult::Type::OBJ, "error", /*optional=*/true, "",
                            {
                                {RPCResult::Type::ELISION, "", "JSONRPC error"},
                            }},
                        }},
                    }
                },
                RPCExamples{
                    HelpExampleCli("importdescriptors", "'[{ \"desc\": \"<my descriptor>\", \"timestamp\":1455191478, \"internal\": true }, "
                                          "{ \"desc\": \"<my descriptor 2>\", \"label\": \"example 2\", \"timestamp\": 1455191480 }]'") +
                    HelpExampleCli("importdescriptors", "'[{ \"desc\": \"<my descriptor>\", \"timestamp\":1455191478, \"active\": true, \"range\": [0,100], \"label\": \"<my bech32 wallet>\" }]'")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& main_request) -> UniValue
{
    std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(main_request);
    if (!pwallet) return UniValue::VNULL;
    CWallet& wallet{*pwallet};

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    wallet.BlockUntilSyncedToCurrentChain();

    //  Make sure wallet is a descriptor wallet
    if (!pwallet->IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "importdescriptors is not available for non-descriptor wallets");
    }

    WalletRescanReserver reserver(*pwallet);
    if (!reserver.reserve(/*with_passphrase=*/true)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Wallet is currently rescanning. Abort existing rescan or wait.");
    }

    // Ensure that the wallet is not locked for the remainder of this RPC, as
    // the passphrase is used to top up the keypool.
    LOCK(pwallet->m_relock_mutex);

    const UniValue& requests = main_request.params[0];
    const int64_t minimum_timestamp = 1;
    int64_t now = 0;
    int64_t lowest_timestamp = 0;
    bool rescan = false;
    UniValue response(UniValue::VARR);
    {
        LOCK(pwallet->cs_wallet);
        EnsureWalletIsUnlocked(*pwallet);

        CHECK_NONFATAL(pwallet->chain().findBlock(pwallet->GetLastBlockHash(), FoundBlock().time(lowest_timestamp).mtpTime(now)));

        // Get all timestamps and extract the lowest timestamp
        for (const UniValue& data : requests.getValues()) {
            // This throws an error if "timestamp" doesn't exist
            const int64_t timestamp = std::max(GetImportTimestamp(data, now), minimum_timestamp);
            ImportDescriptorData descriptor_data;
            std::vector<std::string> warnings;
            UniValue result(UniValue::VOBJ);
            try {
                if (!data.exists("desc")) {
                    throw InvalidParameter("Descriptor not found.");
                }

                descriptor_data.active = data.exists("active") ? data["active"].get_bool() : false;
                descriptor_data.internal = data.exists("internal") ? data["internal"].get_bool() : false;
                descriptor_data.label = LabelFromValue(data["label"]);

                // Parse descriptor string
                FlatSigningProvider keys;
                std::string error;
                descriptor_data.parsed_desc = Parse(data["desc"].get_str(), keys, error, /* require_checksum = */ true);
                if (!descriptor_data.parsed_desc) {
                    throw InvalidAddressOrKey(error);
                }

                // Range check
                descriptor_data.range_start = 0, descriptor_data.range_end = 1, descriptor_data.next_index = 0;
                if (!descriptor_data.parsed_desc->IsRange() && data.exists("range")) {
                    throw InvalidParameter("Range should not be specified for an un-ranged descriptor");
                } else if (descriptor_data.parsed_desc->IsRange()) {
                    if (data.exists("range")) {
                        auto range = ParseDescriptorRange(data["range"]);
                        descriptor_data.range_start = range.first;
                        descriptor_data.range_end = range.second + 1; // Specified range end is inclusive, but we need range end as exclusive
                    } else {
                        warnings.push_back("Range not given, using default keypool range");
                        descriptor_data.range_start = 0;
                        descriptor_data.range_end = wallet.m_keypool_size;
                    }
                    descriptor_data.next_index = descriptor_data.range_start;

                    if (data.exists("next_index")) {
                        descriptor_data.next_index = data["next_index"].getInt<int64_t>();
                        // bound checks
                        if (descriptor_data.next_index < descriptor_data.range_start || descriptor_data.next_index >= descriptor_data.range_end) {
                            throw InvalidParameter("next_index is out of range");
                        }
                    }
                }

                bool range_exists = data.exists("range");
                ProcessDescriptorImport(*pwallet, descriptor_data, warnings, keys, range_exists, timestamp);
                result.pushKV("success", UniValue(true));
            } catch (const UniValue& e) {
                result.pushKV("success", UniValue(false));
                result.pushKV("error", e);
            } catch (const MiscError& e) {
                result.pushKV("success", UniValue(false));
                result.pushKV("error", JSONRPCError(RPC_MISC_ERROR, e.error));
            } catch (const WalletError& e) {
                result.pushKV("success", UniValue(false));
                result.pushKV("error", JSONRPCError(RPC_WALLET_ERROR, e.error));
            } catch (const InvalidAddressOrKey& e) {
                result.pushKV("success", UniValue(false));
                result.pushKV("error", JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, e.error));
            } catch (const InvalidParameter& e) {
                result.pushKV("success", UniValue(false));
                result.pushKV("error", JSONRPCError(RPC_INVALID_PARAMETER, e.error));
            }

            if (warnings.size()) {
                UniValue warning(UniValue::VARR);
                for (const auto& w: warnings)
                    warning.push_back(w);
                PushWarnings(warning, result);
            }

            response.push_back(result);

            if (lowest_timestamp > timestamp ) {
                lowest_timestamp = timestamp;
            }

            // If we know the chain tip, and at least one request was successful then allow rescan
            if (!rescan && result["success"].get_bool()) {
                rescan = true;
            }
        }
        pwallet->ConnectScriptPubKeyManNotifiers();
    }

    // Rescan the blockchain using the lowest timestamp
    if (rescan) {
        int64_t scanned_time = pwallet->RescanFromTime(lowest_timestamp, reserver, /*update=*/true);
        pwallet->ResubmitWalletTransactions(/*relay=*/false, /*force=*/true);

        if (pwallet->IsAbortingRescan()) {
            throw JSONRPCError(RPC_MISC_ERROR, "Rescan aborted by user.");
        }

        if (scanned_time > lowest_timestamp) {
            std::vector<UniValue> results = response.getValues();
            response.clear();
            response.setArray();

            // Compose the response
            for (unsigned int i = 0; i < requests.size(); ++i) {
                const UniValue& request = requests.getValues().at(i);

                // If the descriptor timestamp is within the successfully scanned
                // range, or if the import result already has an error set, let
                // the result stand unmodified. Otherwise replace the result
                // with an error message.
                if (scanned_time <= GetImportTimestamp(request, now) || results.at(i).exists("error")) {
                    response.push_back(results.at(i));
                } else {
                    UniValue result = UniValue(UniValue::VOBJ);
                    result.pushKV("success", UniValue(false));
                    result.pushKV(
                        "error",
                        JSONRPCError(
                            RPC_MISC_ERROR,
                            strprintf("Rescan failed for descriptor with timestamp %d. There was an error reading a "
                                      "block from time %d, which is after or within %d seconds of key creation, and "
                                      "could contain transactions pertaining to the desc. As a result, transactions "
                                      "and coins using this desc may not appear in the wallet. This error could be "
                                      "caused by pruning or data corruption (see bitcoind log for details) and could "
                                      "be dealt with by downloading and rescanning the relevant blocks (see -reindex "
                                      "option and rescanblockchain RPC).",
                                GetImportTimestamp(request, now), scanned_time - TIMESTAMP_WINDOW - 1, TIMESTAMP_WINDOW)));
                    response.push_back(std::move(result));
                }
            }
        }
    }

    return response;
},
    };
}

RPCHelpMan listdescriptors()
{
    return RPCHelpMan{
        "listdescriptors",
        "\nList descriptors imported into a descriptor-enabled wallet.\n",
        {
            {"private", RPCArg::Type::BOOL, RPCArg::Default{false}, "Show private descriptors."}
        },
        RPCResult{RPCResult::Type::OBJ, "", "", {
            {RPCResult::Type::STR, "wallet_name", "Name of wallet this operation was performed on"},
            {RPCResult::Type::ARR, "descriptors", "Array of descriptor objects (sorted by descriptor string representation)",
            {
                {RPCResult::Type::OBJ, "", "", {
                    {RPCResult::Type::STR, "desc", "Descriptor string representation"},
                    {RPCResult::Type::NUM, "timestamp", "The creation time of the descriptor"},
                    {RPCResult::Type::BOOL, "active", "Whether this descriptor is currently used to generate new addresses"},
                    {RPCResult::Type::BOOL, "internal", /*optional=*/true, "True if this descriptor is used to generate change addresses. False if this descriptor is used to generate receiving addresses; defined only for active descriptors"},
                    {RPCResult::Type::ARR_FIXED, "range", /*optional=*/true, "Defined only for ranged descriptors", {
                        {RPCResult::Type::NUM, "", "Range start inclusive"},
                        {RPCResult::Type::NUM, "", "Range end inclusive"},
                    }},
                    {RPCResult::Type::NUM, "next", /*optional=*/true, "Same as next_index field. Kept for compatibility reason."},
                    {RPCResult::Type::NUM, "next_index", /*optional=*/true, "The next index to generate addresses from; defined only for ranged descriptors"},
                }},
            }}
        }},
        RPCExamples{
            HelpExampleCli("listdescriptors", "") + HelpExampleRpc("listdescriptors", "")
            + HelpExampleCli("listdescriptors", "true") + HelpExampleRpc("listdescriptors", "true")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const std::shared_ptr<const CWallet> wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return UniValue::VNULL;

    if (!wallet->IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "listdescriptors is not available for non-descriptor wallets");
    }

    const bool priv = !request.params[0].isNull() && request.params[0].get_bool();
    if (priv) {
        EnsureWalletIsUnlocked(*wallet);
    }

    LOCK(wallet->cs_wallet);

    const auto active_spk_mans = wallet->GetActiveScriptPubKeyMans();

    struct WalletDescInfo {
        std::string descriptor;
        uint64_t creation_time;
        bool active;
        std::optional<bool> internal;
        std::optional<std::pair<int64_t,int64_t>> range;
        int64_t next_index;
    };

    std::vector<WalletDescInfo> wallet_descriptors;
    for (const auto& spk_man : wallet->GetAllScriptPubKeyMans()) {
        const auto desc_spk_man = dynamic_cast<DescriptorScriptPubKeyMan*>(spk_man);
        if (!desc_spk_man) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Unexpected ScriptPubKey manager type.");
        }
        LOCK(desc_spk_man->cs_desc_man);
        const auto& wallet_descriptor = desc_spk_man->GetWalletDescriptor();
        std::string descriptor;
        if (!desc_spk_man->GetDescriptorString(descriptor, priv)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Can't get descriptor string.");
        }
        const bool is_range = wallet_descriptor.descriptor->IsRange();
        wallet_descriptors.push_back({
            descriptor,
            wallet_descriptor.creation_time,
            active_spk_mans.count(desc_spk_man) != 0,
            wallet->IsInternalScriptPubKeyMan(desc_spk_man),
            is_range ? std::optional(std::make_pair(wallet_descriptor.range_start, wallet_descriptor.range_end)) : std::nullopt,
            wallet_descriptor.next_index
        });
    }

    std::sort(wallet_descriptors.begin(), wallet_descriptors.end(), [](const auto& a, const auto& b) {
        return a.descriptor < b.descriptor;
    });

    UniValue descriptors(UniValue::VARR);
    for (const WalletDescInfo& info : wallet_descriptors) {
        UniValue spk(UniValue::VOBJ);
        spk.pushKV("desc", info.descriptor);
        spk.pushKV("timestamp", info.creation_time);
        spk.pushKV("active", info.active);
        if (info.internal.has_value()) {
            spk.pushKV("internal", info.internal.value());
        }
        if (info.range.has_value()) {
            UniValue range(UniValue::VARR);
            range.push_back(info.range->first);
            range.push_back(info.range->second - 1);
            spk.pushKV("range", range);
            spk.pushKV("next", info.next_index);
            spk.pushKV("next_index", info.next_index);
        }
        descriptors.push_back(spk);
    }

    UniValue response(UniValue::VOBJ);
    response.pushKV("wallet_name", wallet->GetName());
    response.pushKV("descriptors", descriptors);

    return response;
},
    };
}

RPCHelpMan backupwallet()
{
    return RPCHelpMan{"backupwallet",
                "\nSafely copies current wallet file to destination, which can be a directory or a path with filename.\n",
                {
                    {"destination", RPCArg::Type::STR, RPCArg::Optional::NO, "The destination directory or file"},
                },
                RPCResult{RPCResult::Type::NONE, "", ""},
                RPCExamples{
                    HelpExampleCli("backupwallet", "\"backup.dat\"")
            + HelpExampleRpc("backupwallet", "\"backup.dat\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const std::shared_ptr<const CWallet> pwallet = GetWalletForJSONRPCRequest(request);
    if (!pwallet) return UniValue::VNULL;

    // Make sure the results are valid at least up to the most recent block
    // the user could have gotten from another RPC command prior to now
    pwallet->BlockUntilSyncedToCurrentChain();

    LOCK(pwallet->cs_wallet);

    std::string strDest = request.params[0].get_str();
    if (!pwallet->BackupWallet(strDest)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Error: Wallet backup failed!");
    }

    return UniValue::VNULL;
},
    };
}


RPCHelpMan restorewallet()
{
    return RPCHelpMan{
        "restorewallet",
        "\nRestore and loads a wallet from backup.\n"
        "\nThe rescan is significantly faster if a descriptor wallet is restored"
        "\nand block filters are available (using startup option \"-blockfilterindex=1\").\n",
        {
            {"wallet_name", RPCArg::Type::STR, RPCArg::Optional::NO, "The name that will be applied to the restored wallet"},
            {"backup_file", RPCArg::Type::STR, RPCArg::Optional::NO, "The backup file that will be used to restore the wallet."},
            {"load_on_startup", RPCArg::Type::BOOL, RPCArg::Optional::OMITTED, "Save wallet name to persistent settings and load on startup. True to add wallet to startup list, false to remove, null to leave unchanged."},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "name", "The wallet name if restored successfully."},
                {RPCResult::Type::STR, "warning", /*optional=*/true, "Warning messages, if any, related to restoring the wallet. Multiple messages will be delimited by newlines. (DEPRECATED, returned only if config option -deprecatedrpc=walletwarningfield is passed.)"},
                {RPCResult::Type::ARR, "warnings", /*optional=*/true, "Warning messages, if any, related to restoring the wallet.",
                {
                    {RPCResult::Type::STR, "", ""},
                }},
            }
        },
        RPCExamples{
            HelpExampleCli("restorewallet", "\"testwallet\" \"home\\backups\\backup-file.bak\"")
            + HelpExampleRpc("restorewallet", "\"testwallet\" \"home\\backups\\backup-file.bak\"")
            + HelpExampleCliNamed("restorewallet", {{"wallet_name", "testwallet"}, {"backup_file", "home\\backups\\backup-file.bak\""}, {"load_on_startup", true}})
            + HelpExampleRpcNamed("restorewallet", {{"wallet_name", "testwallet"}, {"backup_file", "home\\backups\\backup-file.bak\""}, {"load_on_startup", true}})
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{

    WalletContext& context = EnsureWalletContext(request.context);

    auto backup_file = fs::u8path(request.params[1].get_str());

    std::string wallet_name = request.params[0].get_str();

    std::optional<bool> load_on_start = request.params[2].isNull() ? std::nullopt : std::optional<bool>(request.params[2].get_bool());

    DatabaseStatus status;
    bilingual_str error;
    std::vector<bilingual_str> warnings;

    const std::shared_ptr<CWallet> wallet = RestoreWallet(context, backup_file, wallet_name, load_on_start, status, error, warnings);

    HandleWalletError(wallet, status, error);

    UniValue obj(UniValue::VOBJ);
    obj.pushKV("name", wallet->GetName());
    if (wallet->chain().rpcEnableDeprecated("walletwarningfield")) {
        obj.pushKV("warning", Join(warnings, Untranslated("\n")).original);
    }
    PushWarnings(warnings, obj);

    return obj;

},
    };
}
} // namespace wallet
