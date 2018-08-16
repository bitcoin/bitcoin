// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <key_io.h>
#include <rpc/server.h>
#include <validation.h>
#include <script/script.h>
#include <script/standard.h>
#include <sync.h>
#include <util.h>
#include <utiltime.h>
#include <wallet/wallet.h>
#include <merkleblock.h>
#include <core_io.h>

#include <wallet/rpcwallet.h>

#include <fstream>
#include <stdint.h>

#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <univalue.h>


int64_t static DecodeDumpTime(const std::string &str) {
    static const boost::posix_time::ptime epoch = boost::posix_time::from_time_t(0);
    static const std::locale loc(std::locale::classic(),
        new boost::posix_time::time_input_facet("%Y-%m-%dT%H:%M:%SZ"));
    std::istringstream iss(str);
    iss.imbue(loc);
    boost::posix_time::ptime ptime(boost::date_time::not_a_date_time);
    iss >> ptime;
    if (ptime.is_not_a_date_time())
        return 0;
    return (ptime - epoch).total_seconds();
}

std::string static EncodeDumpString(const std::string &str) {
    std::stringstream ret;
    for (unsigned char c : str) {
        if (c <= 32 || c >= 128 || c == '%') {
            ret << '%' << HexStr(&c, &c + 1);
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

static bool GetWalletAddressesForKey(CWallet * const pwallet, const CKeyID &keyid, std::string &strAddr, std::string &strLabel)
{
    bool fLabelFound = false;
    CKey key;
    pwallet->GetKey(keyid, key);
    for (const auto& dest : GetAllDestinationsForKey(key.GetPubKey())) {
        if (pwallet->mapAddressBook.count(dest)) {
            if (!strAddr.empty()) {
                strAddr += ",";
            }
            strAddr += EncodeDestination(dest);
            strLabel = EncodeDumpString(pwallet->mapAddressBook[dest].name);
            fLabelFound = true;
        }
    }
    if (!fLabelFound) {
        strAddr = EncodeDestination(GetDestinationForKey(key.GetPubKey(), pwallet->m_default_address_type));
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

UniValue importprivkey(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return NullUniValue;
    }

    if (request.fHelp || request.params.size() < 1 || request.params.size() > 3)
        throw std::runtime_error(
            "importprivkey \"privkey\" ( \"label\" ) ( rescan )\n"
            "\nAdds a private key (as returned by dumpprivkey) to your wallet. Requires a new wallet backup.\n"
            "Hint: use importmulti to import more than one private key.\n"
            "\nArguments:\n"
            "1. \"privkey\"          (string, required) The private key (see dumpprivkey)\n"
            "2. \"label\"            (string, optional, default=\"\") An optional label\n"
            "3. rescan               (boolean, optional, default=true) Rescan the wallet for transactions\n"
            "\nNote: This call can take over an hour to complete if rescan is true, during that time, other rpc calls\n"
            "may report that the imported key exists but related transactions are still missing, leading to temporarily incorrect/bogus balances and unspent outputs until rescan completes.\n"
            "\nExamples:\n"
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
        );


    WalletRescanReserver reserver(pwallet);
    bool fRescan = true;
    {
        LOCK2(cs_main, pwallet->cs_wallet);

        EnsureWalletIsUnlocked(pwallet);

        std::string strSecret = request.params[0].get_str();
        std::string strLabel = "";
        if (!request.params[1].isNull())
            strLabel = request.params[1].get_str();

        // Whether to perform rescan after import
        if (!request.params[2].isNull())
            fRescan = request.params[2].get_bool();

        if (fRescan && fPruneMode)
            throw JSONRPCError(RPC_WALLET_ERROR, "Rescan is disabled in pruned mode");

        if (fRescan && !reserver.reserve()) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Wallet is currently rescanning. Abort existing rescan or wait.");
        }

        CKey key = DecodeSecret(strSecret);
        if (!key.IsValid()) throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key encoding");

        CPubKey pubkey = key.GetPubKey();
        assert(key.VerifyPubKey(pubkey));
        CKeyID vchAddress = pubkey.GetID();
        {
            pwallet->MarkDirty();
            // We don't know which corresponding address will be used; label them all
            for (const auto& dest : GetAllDestinationsForKey(pubkey)) {
                pwallet->SetAddressBook(dest, strLabel, "receive");
            }

            // Don't throw error in case a key is already there
            if (pwallet->HaveKey(vchAddress)) {
                return NullUniValue;
            }

            // whenever a key is imported, we need to scan the whole chain
            pwallet->UpdateTimeFirstKey(1);
            pwallet->mapKeyMetadata[vchAddress].nCreateTime = 1;

            if (!pwallet->AddKeyPubKey(key, pubkey)) {
                throw JSONRPCError(RPC_WALLET_ERROR, "Error adding key to wallet");
            }
            pwallet->LearnAllRelatedScripts(pubkey);
        }
    }
    if (fRescan) {
        RescanWallet(*pwallet, reserver);
    }

    return NullUniValue;
}

UniValue abortrescan(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return NullUniValue;
    }

    if (request.fHelp || request.params.size() > 0)
        throw std::runtime_error(
            "abortrescan\n"
            "\nStops current wallet rescan triggered by an RPC call, e.g. by an importprivkey call.\n"
            "\nExamples:\n"
            "\nImport a private key\n"
            + HelpExampleCli("importprivkey", "\"mykey\"") +
            "\nAbort the running wallet rescan\n"
            + HelpExampleCli("abortrescan", "") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("abortrescan", "")
        );

    if (!pwallet->IsScanning() || pwallet->IsAbortingRescan()) return false;
    pwallet->AbortRescan();
    return true;
}

static void ImportAddress(CWallet*, const CTxDestination& dest, const std::string& strLabel);
static void ImportScript(CWallet* const pwallet, const CScript& script, const std::string& strLabel, bool isRedeemScript) EXCLUSIVE_LOCKS_REQUIRED(pwallet->cs_wallet)
{
    if (!isRedeemScript && ::IsMine(*pwallet, script) == ISMINE_SPENDABLE) {
        throw JSONRPCError(RPC_WALLET_ERROR, "The wallet already contains the private key for this address or script");
    }

    pwallet->MarkDirty();

    if (!pwallet->HaveWatchOnly(script) && !pwallet->AddWatchOnly(script, 0 /* nCreateTime */)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Error adding address to wallet");
    }

    if (isRedeemScript) {
        const CScriptID id(script);
        if (!pwallet->HaveCScript(id) && !pwallet->AddCScript(script)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Error adding p2sh redeemScript to wallet");
        }
        ImportAddress(pwallet, id, strLabel);
    } else {
        CTxDestination destination;
        if (ExtractDestination(script, destination)) {
            pwallet->SetAddressBook(destination, strLabel, "receive");
        }
    }
}

static void ImportAddress(CWallet* const pwallet, const CTxDestination& dest, const std::string& strLabel) EXCLUSIVE_LOCKS_REQUIRED(pwallet->cs_wallet)
{
    CScript script = GetScriptForDestination(dest);
    ImportScript(pwallet, script, strLabel, false);
    // add to address book or update label
    if (IsValidDestination(dest))
        pwallet->SetAddressBook(dest, strLabel, "receive");
}

UniValue importaddress(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return NullUniValue;
    }

    if (request.fHelp || request.params.size() < 1 || request.params.size() > 4)
        throw std::runtime_error(
            "importaddress \"address\" ( \"label\" rescan p2sh )\n"
            "\nAdds an address or script (in hex) that can be watched as if it were in your wallet but cannot be used to spend. Requires a new wallet backup.\n"
            "\nArguments:\n"
            "1. \"address\"          (string, required) The Bitcoin address (or hex-encoded script)\n"
            "2. \"label\"            (string, optional, default=\"\") An optional label\n"
            "3. rescan               (boolean, optional, default=true) Rescan the wallet for transactions\n"
            "4. p2sh                 (boolean, optional, default=false) Add the P2SH version of the script as well\n"
            "\nNote: This call can take over an hour to complete if rescan is true, during that time, other rpc calls\n"
            "may report that the imported address exists but related transactions are still missing, leading to temporarily incorrect/bogus balances and unspent outputs until rescan completes.\n"
            "If you have the full public key, you should call importpubkey instead of this.\n"
            "\nNote: If you import a non-standard raw script in hex form, outputs sending to it will be treated\n"
            "as change, and not show up in many RPCs.\n"
            "\nExamples:\n"
            "\nImport an address with rescan\n"
            + HelpExampleCli("importaddress", "\"myaddress\"") +
            "\nImport using a label without rescan\n"
            + HelpExampleCli("importaddress", "\"myaddress\" \"testing\" false") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("importaddress", "\"myaddress\", \"testing\", false")
        );


    std::string strLabel;
    if (!request.params[1].isNull())
        strLabel = request.params[1].get_str();

    // Whether to perform rescan after import
    bool fRescan = true;
    if (!request.params[2].isNull())
        fRescan = request.params[2].get_bool();

    if (fRescan && fPruneMode)
        throw JSONRPCError(RPC_WALLET_ERROR, "Rescan is disabled in pruned mode");

    WalletRescanReserver reserver(pwallet);
    if (fRescan && !reserver.reserve()) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Wallet is currently rescanning. Abort existing rescan or wait.");
    }

    // Whether to import a p2sh version, too
    bool fP2SH = false;
    if (!request.params[3].isNull())
        fP2SH = request.params[3].get_bool();

    {
        LOCK2(cs_main, pwallet->cs_wallet);

        CTxDestination dest = DecodeDestination(request.params[0].get_str());
        if (IsValidDestination(dest)) {
            if (fP2SH) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Cannot use the p2sh flag with an address - use a script instead");
            }
            ImportAddress(pwallet, dest, strLabel);
        } else if (IsHex(request.params[0].get_str())) {
            std::vector<unsigned char> data(ParseHex(request.params[0].get_str()));
            ImportScript(pwallet, CScript(data.begin(), data.end()), strLabel, fP2SH);
        } else {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Bitcoin address or script");
        }
    }
    if (fRescan)
    {
        RescanWallet(*pwallet, reserver);
        pwallet->ReacceptWalletTransactions();
    }

    return NullUniValue;
}

UniValue importprunedfunds(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return NullUniValue;
    }

    if (request.fHelp || request.params.size() != 2)
        throw std::runtime_error(
            "importprunedfunds\n"
            "\nImports funds without rescan. Corresponding address or script must previously be included in wallet. Aimed towards pruned wallets. The end-user is responsible to import additional transactions that subsequently spend the imported outputs or rescan after the point in the blockchain the transaction is included.\n"
            "\nArguments:\n"
            "1. \"rawtransaction\" (string, required) A raw transaction in hex funding an already-existing address in wallet\n"
            "2. \"txoutproof\"     (string, required) The hex output from gettxoutproof that contains the transaction\n"
        );

    CMutableTransaction tx;
    if (!DecodeHexTx(tx, request.params[0].get_str()))
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "TX decode failed");
    uint256 hashTx = tx.GetHash();
    CWalletTx wtx(pwallet, MakeTransactionRef(std::move(tx)));

    CDataStream ssMB(ParseHexV(request.params[1], "proof"), SER_NETWORK, PROTOCOL_VERSION);
    CMerkleBlock merkleBlock;
    ssMB >> merkleBlock;

    //Search partial merkle tree in proof for our transaction and index in valid block
    std::vector<uint256> vMatch;
    std::vector<unsigned int> vIndex;
    unsigned int txnIndex = 0;
    if (merkleBlock.txn.ExtractMatches(vMatch, vIndex) == merkleBlock.header.hashMerkleRoot) {

        LOCK(cs_main);
        const CBlockIndex* pindex = LookupBlockIndex(merkleBlock.header.GetHash());
        if (!pindex || !chainActive.Contains(pindex)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found in chain");
        }

        std::vector<uint256>::const_iterator it;
        if ((it = std::find(vMatch.begin(), vMatch.end(), hashTx))==vMatch.end()) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Transaction given doesn't exist in proof");
        }

        txnIndex = vIndex[it - vMatch.begin()];
    }
    else {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Something wrong with merkleblock");
    }

    wtx.nIndex = txnIndex;
    wtx.hashBlock = merkleBlock.header.GetHash();

    LOCK2(cs_main, pwallet->cs_wallet);

    if (pwallet->IsMine(*wtx.tx)) {
        pwallet->AddToWallet(wtx, false);
        return NullUniValue;
    }

    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No addresses in wallet correspond to included transaction");
}

UniValue removeprunedfunds(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return NullUniValue;
    }

    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "removeprunedfunds \"txid\"\n"
            "\nDeletes the specified transaction from the wallet. Meant for use with pruned wallets and as a companion to importprunedfunds. This will affect wallet balances.\n"
            "\nArguments:\n"
            "1. \"txid\"           (string, required) The hex-encoded id of the transaction you are deleting\n"
            "\nExamples:\n"
            + HelpExampleCli("removeprunedfunds", "\"a8d0c0184dde994a09ec054286f1ce581bebf46446a512166eae7628734ea0a5\"") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("removeprunedfunds", "\"a8d0c0184dde994a09ec054286f1ce581bebf46446a512166eae7628734ea0a5\"")
        );

    LOCK2(cs_main, pwallet->cs_wallet);

    uint256 hash;
    hash.SetHex(request.params[0].get_str());
    std::vector<uint256> vHash;
    vHash.push_back(hash);
    std::vector<uint256> vHashOut;

    if (pwallet->ZapSelectTx(vHash, vHashOut) != DBErrors::LOAD_OK) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Could not properly delete the transaction.");
    }

    if(vHashOut.empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Transaction does not exist in wallet.");
    }

    return NullUniValue;
}

UniValue importpubkey(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return NullUniValue;
    }

    if (request.fHelp || request.params.size() < 1 || request.params.size() > 3)
        throw std::runtime_error(
            "importpubkey \"pubkey\" ( \"label\" rescan )\n"
            "\nAdds a public key (in hex) that can be watched as if it were in your wallet but cannot be used to spend. Requires a new wallet backup.\n"
            "\nArguments:\n"
            "1. \"pubkey\"           (string, required) The hex-encoded public key\n"
            "2. \"label\"            (string, optional, default=\"\") An optional label\n"
            "3. rescan               (boolean, optional, default=true) Rescan the wallet for transactions\n"
            "\nNote: This call can take over an hour to complete if rescan is true, during that time, other rpc calls\n"
            "may report that the imported pubkey exists but related transactions are still missing, leading to temporarily incorrect/bogus balances and unspent outputs until rescan completes.\n"
            "\nExamples:\n"
            "\nImport a public key with rescan\n"
            + HelpExampleCli("importpubkey", "\"mypubkey\"") +
            "\nImport using a label without rescan\n"
            + HelpExampleCli("importpubkey", "\"mypubkey\" \"testing\" false") +
            "\nAs a JSON-RPC call\n"
            + HelpExampleRpc("importpubkey", "\"mypubkey\", \"testing\", false")
        );


    std::string strLabel;
    if (!request.params[1].isNull())
        strLabel = request.params[1].get_str();

    // Whether to perform rescan after import
    bool fRescan = true;
    if (!request.params[2].isNull())
        fRescan = request.params[2].get_bool();

    if (fRescan && fPruneMode)
        throw JSONRPCError(RPC_WALLET_ERROR, "Rescan is disabled in pruned mode");

    WalletRescanReserver reserver(pwallet);
    if (fRescan && !reserver.reserve()) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Wallet is currently rescanning. Abort existing rescan or wait.");
    }

    if (!IsHex(request.params[0].get_str()))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Pubkey must be a hex string");
    std::vector<unsigned char> data(ParseHex(request.params[0].get_str()));
    CPubKey pubKey(data.begin(), data.end());
    if (!pubKey.IsFullyValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Pubkey is not a valid public key");

    {
        LOCK2(cs_main, pwallet->cs_wallet);

        for (const auto& dest : GetAllDestinationsForKey(pubKey)) {
            ImportAddress(pwallet, dest, strLabel);
        }
        ImportScript(pwallet, GetScriptForRawPubKey(pubKey), strLabel, false);
        pwallet->LearnAllRelatedScripts(pubKey);
    }
    if (fRescan)
    {
        RescanWallet(*pwallet, reserver);
        pwallet->ReacceptWalletTransactions();
    }

    return NullUniValue;
}


UniValue importwallet(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return NullUniValue;
    }

    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "importwallet \"filename\"\n"
            "\nImports keys from a wallet dump file (see dumpwallet). Requires a new wallet backup to include imported keys.\n"
            "\nArguments:\n"
            "1. \"filename\"    (string, required) The wallet file\n"
            "\nExamples:\n"
            "\nDump the wallet\n"
            + HelpExampleCli("dumpwallet", "\"test\"") +
            "\nImport the wallet\n"
            + HelpExampleCli("importwallet", "\"test\"") +
            "\nImport using the json rpc call\n"
            + HelpExampleRpc("importwallet", "\"test\"")
        );

    if (fPruneMode)
        throw JSONRPCError(RPC_WALLET_ERROR, "Importing wallets is disabled in pruned mode");

    WalletRescanReserver reserver(pwallet);
    if (!reserver.reserve()) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Wallet is currently rescanning. Abort existing rescan or wait.");
    }

    int64_t nTimeBegin = 0;
    bool fGood = true;
    {
        LOCK2(cs_main, pwallet->cs_wallet);

        EnsureWalletIsUnlocked(pwallet);

        std::ifstream file;
        file.open(request.params[0].get_str().c_str(), std::ios::in | std::ios::ate);
        if (!file.is_open()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot open wallet dump file");
        }
        nTimeBegin = chainActive.Tip()->GetBlockTime();

        int64_t nFilesize = std::max((int64_t)1, (int64_t)file.tellg());
        file.seekg(0, file.beg);

        // Use uiInterface.ShowProgress instead of pwallet.ShowProgress because pwallet.ShowProgress has a cancel button tied to AbortRescan which
        // we don't want for this progress bar showing the import progress. uiInterface.ShowProgress does not have a cancel button.
        uiInterface.ShowProgress(strprintf("%s " + _("Importing..."), pwallet->GetDisplayName()), 0, false); // show progress dialog in GUI
        while (file.good()) {
            uiInterface.ShowProgress("", std::max(1, std::min(99, (int)(((double)file.tellg() / (double)nFilesize) * 100))), false);
            std::string line;
            std::getline(file, line);
            if (line.empty() || line[0] == '#')
                continue;

            std::vector<std::string> vstr;
            boost::split(vstr, line, boost::is_any_of(" "));
            if (vstr.size() < 2)
                continue;
            CKey key = DecodeSecret(vstr[0]);
            if (key.IsValid()) {
                CPubKey pubkey = key.GetPubKey();
                assert(key.VerifyPubKey(pubkey));
                CKeyID keyid = pubkey.GetID();
                if (pwallet->HaveKey(keyid)) {
                    pwallet->WalletLogPrintf("Skipping import of %s (key already present)\n", EncodeDestination(keyid));
                    continue;
                }
                int64_t nTime = DecodeDumpTime(vstr[1]);
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
                pwallet->WalletLogPrintf("Importing %s...\n", EncodeDestination(keyid));
                if (!pwallet->AddKeyPubKey(key, pubkey)) {
                    fGood = false;
                    continue;
                }
                pwallet->mapKeyMetadata[keyid].nCreateTime = nTime;
                if (fLabel)
                    pwallet->SetAddressBook(keyid, strLabel, "receive");
                nTimeBegin = std::min(nTimeBegin, nTime);
            } else if(IsHex(vstr[0])) {
               std::vector<unsigned char> vData(ParseHex(vstr[0]));
               CScript script = CScript(vData.begin(), vData.end());
               CScriptID id(script);
               if (pwallet->HaveCScript(id)) {
                   pwallet->WalletLogPrintf("Skipping import of %s (script already present)\n", vstr[0]);
                   continue;
               }
               if(!pwallet->AddCScript(script)) {
                   pwallet->WalletLogPrintf("Error importing script %s\n", vstr[0]);
                   fGood = false;
                   continue;
               }
               int64_t birth_time = DecodeDumpTime(vstr[1]);
               if (birth_time > 0) {
                   pwallet->m_script_metadata[id].nCreateTime = birth_time;
                   nTimeBegin = std::min(nTimeBegin, birth_time);
               }
            }
        }
        file.close();
        uiInterface.ShowProgress("", 100, false); // hide progress dialog in GUI
        pwallet->UpdateTimeFirstKey(nTimeBegin);
    }
    uiInterface.ShowProgress("", 100, false); // hide progress dialog in GUI
    RescanWallet(*pwallet, reserver, nTimeBegin, false /* update */);
    pwallet->MarkDirty();

    if (!fGood)
        throw JSONRPCError(RPC_WALLET_ERROR, "Error adding some keys/scripts to wallet");

    return NullUniValue;
}

UniValue dumpprivkey(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return NullUniValue;
    }

    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "dumpprivkey \"address\"\n"
            "\nReveals the private key corresponding to 'address'.\n"
            "Then the importprivkey can be used with this output\n"
            "\nArguments:\n"
            "1. \"address\"   (string, required) The bitcoin address for the private key\n"
            "\nResult:\n"
            "\"key\"                (string) The private key\n"
            "\nExamples:\n"
            + HelpExampleCli("dumpprivkey", "\"myaddress\"")
            + HelpExampleCli("importprivkey", "\"mykey\"")
            + HelpExampleRpc("dumpprivkey", "\"myaddress\"")
        );

    LOCK2(cs_main, pwallet->cs_wallet);

    EnsureWalletIsUnlocked(pwallet);

    std::string strAddress = request.params[0].get_str();
    CTxDestination dest = DecodeDestination(strAddress);
    if (!IsValidDestination(dest)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Bitcoin address");
    }
    auto keyid = GetKeyForDestination(*pwallet, dest);
    if (keyid.IsNull()) {
        throw JSONRPCError(RPC_TYPE_ERROR, "Address does not refer to a key");
    }
    CKey vchSecret;
    if (!pwallet->GetKey(keyid, vchSecret)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Private key for address " + strAddress + " is not known");
    }
    return EncodeSecret(vchSecret);
}


UniValue dumpwallet(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    CWallet* const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return NullUniValue;
    }

    if (request.fHelp || request.params.size() != 1)
        throw std::runtime_error(
            "dumpwallet \"filename\"\n"
            "\nDumps all wallet keys in a human-readable format to a server-side file. This does not allow overwriting existing files.\n"
            "Imported scripts are included in the dumpfile, but corresponding BIP173 addresses, etc. may not be added automatically by importwallet.\n"
            "Note that if your wallet contains keys which are not derived from your HD seed (e.g. imported keys), these are not covered by\n"
            "only backing up the seed itself, and must be backed up too (e.g. ensure you back up the whole dumpfile).\n"
            "\nArguments:\n"
            "1. \"filename\"    (string, required) The filename with path (either absolute or relative to bitcoind)\n"
            "\nResult:\n"
            "{                           (json object)\n"
            "  \"filename\" : {        (string) The filename with full absolute path\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("dumpwallet", "\"test\"")
            + HelpExampleRpc("dumpwallet", "\"test\"")
        );

    LOCK2(cs_main, pwallet->cs_wallet);

    EnsureWalletIsUnlocked(pwallet);

    fs::path filepath = request.params[0].get_str();
    filepath = fs::absolute(filepath);

    /* Prevent arbitrary files from being overwritten. There have been reports
     * that users have overwritten wallet files this way:
     * https://github.com/bitcoin/bitcoin/issues/9934
     * It may also avoid other security issues.
     */
    if (fs::exists(filepath)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, filepath.string() + " already exists. If you are sure this is what you want, move it out of the way first");
    }

    std::ofstream file;
    file.open(filepath.string().c_str());
    if (!file.is_open())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot open wallet dump file");

    std::map<CTxDestination, int64_t> mapKeyBirth;
    const std::map<CKeyID, int64_t>& mapKeyPool = pwallet->GetAllReserveKeys();
    pwallet->GetKeyBirthTimes(mapKeyBirth);

    std::set<CScriptID> scripts = pwallet->GetCScripts();
    // TODO: include scripts in GetKeyBirthTimes() output instead of separate

    // sort time/key pairs
    std::vector<std::pair<int64_t, CKeyID> > vKeyBirth;
    for (const auto& entry : mapKeyBirth) {
        if (const CKeyID* keyID = boost::get<CKeyID>(&entry.first)) { // set and test
            vKeyBirth.push_back(std::make_pair(entry.second, *keyID));
        }
    }
    mapKeyBirth.clear();
    std::sort(vKeyBirth.begin(), vKeyBirth.end());

    // produce output
    file << strprintf("# Wallet dump created by Bitcoin %s\n", CLIENT_BUILD);
    file << strprintf("# * Created on %s\n", FormatISO8601DateTime(GetTime()));
    file << strprintf("# * Best block at time of backup was %i (%s),\n", chainActive.Height(), chainActive.Tip()->GetBlockHash().ToString());
    file << strprintf("#   mined on %s\n", FormatISO8601DateTime(chainActive.Tip()->GetBlockTime()));
    file << "\n";

    // add the base58check encoded extended master if the wallet uses HD
    CKeyID seed_id = pwallet->GetHDChain().seed_id;
    if (!seed_id.IsNull())
    {
        CKey seed;
        if (pwallet->GetKey(seed_id, seed)) {
            CExtKey masterKey;
            masterKey.SetSeed(seed.begin(), seed.size());

            file << "# extended private masterkey: " << EncodeExtKey(masterKey) << "\n\n";
        }
    }
    for (std::vector<std::pair<int64_t, CKeyID> >::const_iterator it = vKeyBirth.begin(); it != vKeyBirth.end(); it++) {
        const CKeyID &keyid = it->second;
        std::string strTime = FormatISO8601DateTime(it->first);
        std::string strAddr;
        std::string strLabel;
        CKey key;
        if (pwallet->GetKey(keyid, key)) {
            file << strprintf("%s %s ", EncodeSecret(key), strTime);
            if (GetWalletAddressesForKey(pwallet, keyid, strAddr, strLabel)) {
               file << strprintf("label=%s", strLabel);
            } else if (keyid == seed_id) {
                file << "hdseed=1";
            } else if (mapKeyPool.count(keyid)) {
                file << "reserve=1";
            } else if (pwallet->mapKeyMetadata[keyid].hdKeypath == "s") {
                file << "inactivehdseed=1";
            } else {
                file << "change=1";
            }
            file << strprintf(" # addr=%s%s\n", strAddr, (pwallet->mapKeyMetadata[keyid].hdKeypath.size() > 0 ? " hdkeypath="+pwallet->mapKeyMetadata[keyid].hdKeypath : ""));
        }
    }
    file << "\n";
    for (const CScriptID &scriptid : scripts) {
        CScript script;
        std::string create_time = "0";
        std::string address = EncodeDestination(scriptid);
        // get birth times for scripts with metadata
        auto it = pwallet->m_script_metadata.find(scriptid);
        if (it != pwallet->m_script_metadata.end()) {
            create_time = FormatISO8601DateTime(it->second.nCreateTime);
        }
        if(pwallet->GetCScript(scriptid, script)) {
            file << strprintf("%s %s script=1", HexStr(script.begin(), script.end()), create_time);
            file << strprintf(" # addr=%s\n", address);
        }
    }
    file << "\n";
    file << "# End of dump\n";
    file.close();

    UniValue reply(UniValue::VOBJ);
    reply.pushKV("filename", filepath.string());

    return reply;
}


static UniValue ProcessImport(CWallet * const pwallet, const UniValue& data, const int64_t timestamp) EXCLUSIVE_LOCKS_REQUIRED(pwallet->cs_wallet)
{
    try {
        bool success = false;

        // Required fields.
        const UniValue& scriptPubKey = data["scriptPubKey"];

        // Should have script or JSON with "address".
        if (!(scriptPubKey.getType() == UniValue::VOBJ && scriptPubKey.exists("address")) && !(scriptPubKey.getType() == UniValue::VSTR)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid scriptPubKey");
        }

        // Optional fields.
        const std::string& strRedeemScript = data.exists("redeemscript") ? data["redeemscript"].get_str() : "";
        const UniValue& pubKeys = data.exists("pubkeys") ? data["pubkeys"].get_array() : UniValue();
        const UniValue& keys = data.exists("keys") ? data["keys"].get_array() : UniValue();
        const bool internal = data.exists("internal") ? data["internal"].get_bool() : false;
        const bool watchOnly = data.exists("watchonly") ? data["watchonly"].get_bool() : false;
        const std::string& label = data.exists("label") && !internal ? data["label"].get_str() : "";

        bool isScript = scriptPubKey.getType() == UniValue::VSTR;
        bool isP2SH = strRedeemScript.length() > 0;
        const std::string& output = isScript ? scriptPubKey.get_str() : scriptPubKey["address"].get_str();

        // Parse the output.
        CScript script;
        CTxDestination dest;

        if (!isScript) {
            dest = DecodeDestination(output);
            if (!IsValidDestination(dest)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
            }
            script = GetScriptForDestination(dest);
        } else {
            if (!IsHex(output)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid scriptPubKey");
            }

            std::vector<unsigned char> vData(ParseHex(output));
            script = CScript(vData.begin(), vData.end());
        }

        // Watchonly and private keys
        if (watchOnly && keys.size()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Incompatibility found between watchonly and keys");
        }

        // Internal + Label
        if (internal && data.exists("label")) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Incompatibility found between internal and label");
        }

        // Not having Internal + Script
        if (!internal && isScript) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Internal must be set for hex scriptPubKey");
        }

        // Keys / PubKeys size check.
        if (!isP2SH && (keys.size() > 1 || pubKeys.size() > 1)) { // Address / scriptPubKey
            throw JSONRPCError(RPC_INVALID_PARAMETER, "More than private key given for one address");
        }

        // Invalid P2SH redeemScript
        if (isP2SH && !IsHex(strRedeemScript)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid redeem script");
        }

        // Process. //

        // P2SH
        if (isP2SH) {
            // Import redeem script.
            std::vector<unsigned char> vData(ParseHex(strRedeemScript));
            CScript redeemScript = CScript(vData.begin(), vData.end());

            // Invalid P2SH address
            if (!script.IsPayToScriptHash()) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid P2SH address / script");
            }

            pwallet->MarkDirty();

            if (!pwallet->AddWatchOnly(redeemScript, timestamp)) {
                throw JSONRPCError(RPC_WALLET_ERROR, "Error adding address to wallet");
            }

            CScriptID redeem_id(redeemScript);
            if (!pwallet->HaveCScript(redeem_id) && !pwallet->AddCScript(redeemScript)) {
                throw JSONRPCError(RPC_WALLET_ERROR, "Error adding p2sh redeemScript to wallet");
            }

            CScript redeemDestination = GetScriptForDestination(redeem_id);

            if (::IsMine(*pwallet, redeemDestination) == ISMINE_SPENDABLE) {
                throw JSONRPCError(RPC_WALLET_ERROR, "The wallet already contains the private key for this address or script");
            }

            pwallet->MarkDirty();

            if (!pwallet->AddWatchOnly(redeemDestination, timestamp)) {
                throw JSONRPCError(RPC_WALLET_ERROR, "Error adding address to wallet");
            }

            // add to address book or update label
            if (IsValidDestination(dest)) {
                pwallet->SetAddressBook(dest, label, "receive");
            }

            // Import private keys.
            if (keys.size()) {
                for (size_t i = 0; i < keys.size(); i++) {
                    const std::string& privkey = keys[i].get_str();

                    CKey key = DecodeSecret(privkey);

                    if (!key.IsValid()) {
                        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key encoding");
                    }

                    CPubKey pubkey = key.GetPubKey();
                    assert(key.VerifyPubKey(pubkey));

                    CKeyID vchAddress = pubkey.GetID();
                    pwallet->MarkDirty();
                    pwallet->SetAddressBook(vchAddress, label, "receive");

                    if (pwallet->HaveKey(vchAddress)) {
                        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Already have this key");
                    }

                    pwallet->mapKeyMetadata[vchAddress].nCreateTime = timestamp;

                    if (!pwallet->AddKeyPubKey(key, pubkey)) {
                        throw JSONRPCError(RPC_WALLET_ERROR, "Error adding key to wallet");
                    }

                    pwallet->UpdateTimeFirstKey(timestamp);
                }
            }

            success = true;
        } else {
            // Import public keys.
            if (pubKeys.size() && keys.size() == 0) {
                const std::string& strPubKey = pubKeys[0].get_str();

                if (!IsHex(strPubKey)) {
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Pubkey must be a hex string");
                }

                std::vector<unsigned char> vData(ParseHex(strPubKey));
                CPubKey pubKey(vData.begin(), vData.end());

                if (!pubKey.IsFullyValid()) {
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Pubkey is not a valid public key");
                }

                CTxDestination pubkey_dest = pubKey.GetID();

                // Consistency check.
                if (!isScript && !(pubkey_dest == dest)) {
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Consistency check failed");
                }

                // Consistency check.
                if (isScript) {
                    CTxDestination destination;

                    if (ExtractDestination(script, destination)) {
                        if (!(destination == pubkey_dest)) {
                            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Consistency check failed");
                        }
                    }
                }

                CScript pubKeyScript = GetScriptForDestination(pubkey_dest);

                if (::IsMine(*pwallet, pubKeyScript) == ISMINE_SPENDABLE) {
                    throw JSONRPCError(RPC_WALLET_ERROR, "The wallet already contains the private key for this address or script");
                }

                pwallet->MarkDirty();

                if (!pwallet->AddWatchOnly(pubKeyScript, timestamp)) {
                    throw JSONRPCError(RPC_WALLET_ERROR, "Error adding address to wallet");
                }

                // add to address book or update label
                if (IsValidDestination(pubkey_dest)) {
                    pwallet->SetAddressBook(pubkey_dest, label, "receive");
                }

                // TODO Is this necessary?
                CScript scriptRawPubKey = GetScriptForRawPubKey(pubKey);

                if (::IsMine(*pwallet, scriptRawPubKey) == ISMINE_SPENDABLE) {
                    throw JSONRPCError(RPC_WALLET_ERROR, "The wallet already contains the private key for this address or script");
                }

                pwallet->MarkDirty();

                if (!pwallet->AddWatchOnly(scriptRawPubKey, timestamp)) {
                    throw JSONRPCError(RPC_WALLET_ERROR, "Error adding address to wallet");
                }

                success = true;
            }

            // Import private keys.
            if (keys.size()) {
                const std::string& strPrivkey = keys[0].get_str();

                // Checks.
                CKey key = DecodeSecret(strPrivkey);

                if (!key.IsValid()) {
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key encoding");
                }

                CPubKey pubKey = key.GetPubKey();
                assert(key.VerifyPubKey(pubKey));

                CTxDestination pubkey_dest = pubKey.GetID();

                // Consistency check.
                if (!isScript && !(pubkey_dest == dest)) {
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Consistency check failed");
                }

                // Consistency check.
                if (isScript) {
                    CTxDestination destination;

                    if (ExtractDestination(script, destination)) {
                        if (!(destination == pubkey_dest)) {
                            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Consistency check failed");
                        }
                    }
                }

                CKeyID vchAddress = pubKey.GetID();
                pwallet->MarkDirty();
                pwallet->SetAddressBook(vchAddress, label, "receive");

                if (pwallet->HaveKey(vchAddress)) {
                    throw JSONRPCError(RPC_WALLET_ERROR, "The wallet already contains the private key for this address or script");
                }

                pwallet->mapKeyMetadata[vchAddress].nCreateTime = timestamp;

                if (!pwallet->AddKeyPubKey(key, pubKey)) {
                    throw JSONRPCError(RPC_WALLET_ERROR, "Error adding key to wallet");
                }

                pwallet->UpdateTimeFirstKey(timestamp);

                success = true;
            }

            // Import scriptPubKey only.
            if (pubKeys.size() == 0 && keys.size() == 0) {
                if (::IsMine(*pwallet, script) == ISMINE_SPENDABLE) {
                    throw JSONRPCError(RPC_WALLET_ERROR, "The wallet already contains the private key for this address or script");
                }

                pwallet->MarkDirty();

                if (!pwallet->AddWatchOnly(script, timestamp)) {
                    throw JSONRPCError(RPC_WALLET_ERROR, "Error adding address to wallet");
                }

                if (scriptPubKey.getType() == UniValue::VOBJ) {
                    // add to address book or update label
                    if (IsValidDestination(dest)) {
                        pwallet->SetAddressBook(dest, label, "receive");
                    }
                }

                success = true;
            }
        }

        UniValue result = UniValue(UniValue::VOBJ);
        result.pushKV("success", UniValue(success));
        return result;
    } catch (const UniValue& e) {
        UniValue result = UniValue(UniValue::VOBJ);
        result.pushKV("success", UniValue(false));
        result.pushKV("error", e);
        return result;
    } catch (...) {
        UniValue result = UniValue(UniValue::VOBJ);
        result.pushKV("success", UniValue(false));
        result.pushKV("error", JSONRPCError(RPC_MISC_ERROR, "Missing required fields"));
        return result;
    }
}

static int64_t GetImportTimestamp(const UniValue& data, int64_t now)
{
    if (data.exists("timestamp")) {
        const UniValue& timestamp = data["timestamp"];
        if (timestamp.isNum()) {
            return timestamp.get_int64();
        } else if (timestamp.isStr() && timestamp.get_str() == "now") {
            return now;
        }
        throw JSONRPCError(RPC_TYPE_ERROR, strprintf("Expected number or \"now\" timestamp value for key. got type %s", uvTypeName(timestamp.type())));
    }
    throw JSONRPCError(RPC_TYPE_ERROR, "Missing required timestamp field for key");
}

UniValue importmulti(const JSONRPCRequest& mainRequest)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(mainRequest);
    CWallet* const pwallet = wallet.get();
    if (!EnsureWalletIsAvailable(pwallet, mainRequest.fHelp)) {
        return NullUniValue;
    }

    // clang-format off
    if (mainRequest.fHelp || mainRequest.params.size() < 1 || mainRequest.params.size() > 2)
        throw std::runtime_error(
            "importmulti \"requests\" ( \"options\" )\n\n"
            "Import addresses/scripts (with private or public keys, redeem script (P2SH)), rescanning all addresses in one-shot-only (rescan can be disabled via options). Requires a new wallet backup.\n\n"
            "Arguments:\n"
            "1. requests     (array, required) Data to be imported\n"
            "  [     (array of json objects)\n"
            "    {\n"
            "      \"scriptPubKey\": \"<script>\" | { \"address\":\"<address>\" }, (string / json, required) Type of scriptPubKey (string for script, json for address)\n"
            "      \"timestamp\": timestamp | \"now\"                        , (integer / string, required) Creation time of the key in seconds since epoch (Jan 1 1970 GMT),\n"
            "                                                              or the string \"now\" to substitute the current synced blockchain time. The timestamp of the oldest\n"
            "                                                              key will determine how far back blockchain rescans need to begin for missing wallet transactions.\n"
            "                                                              \"now\" can be specified to bypass scanning, for keys which are known to never have been used, and\n"
            "                                                              0 can be specified to scan the entire blockchain. Blocks up to 2 hours before the earliest key\n"
            "                                                              creation time of all keys being imported by the importmulti call will be scanned.\n"
            "      \"redeemscript\": \"<script>\"                            , (string, optional) Allowed only if the scriptPubKey is a P2SH address or a P2SH scriptPubKey\n"
            "      \"pubkeys\": [\"<pubKey>\", ... ]                         , (array, optional) Array of strings giving pubkeys that must occur in the output or redeemscript\n"
            "      \"keys\": [\"<key>\", ... ]                               , (array, optional) Array of strings giving private keys whose corresponding public keys must occur in the output or redeemscript\n"
            "      \"internal\": <true>                                    , (boolean, optional, default: false) Stating whether matching outputs should be treated as not incoming payments\n"
            "      \"watchonly\": <true>                                   , (boolean, optional, default: false) Stating whether matching outputs should be considered watched even when they're not spendable, only allowed if keys are empty\n"
            "      \"label\": <label>                                      , (string, optional, default: '') Label to assign to the address (aka account name, for now), only allowed with internal=false\n"
            "    }\n"
            "  ,...\n"
            "  ]\n"
            "2. options                 (json, optional)\n"
            "  {\n"
            "     \"rescan\": <false>,         (boolean, optional, default: true) Stating if should rescan the blockchain after all imports\n"
            "  }\n"
            "\nNote: This call can take over an hour to complete if rescan is true, during that time, other rpc calls\n"
            "may report that the imported keys, addresses or scripts exists but related transactions are still missing.\n"
            "\nExamples:\n" +
            HelpExampleCli("importmulti", "'[{ \"scriptPubKey\": { \"address\": \"<my address>\" }, \"timestamp\":1455191478 }, "
                                          "{ \"scriptPubKey\": { \"address\": \"<my 2nd address>\" }, \"label\": \"example 2\", \"timestamp\": 1455191480 }]'") +
            HelpExampleCli("importmulti", "'[{ \"scriptPubKey\": { \"address\": \"<my address>\" }, \"timestamp\":1455191478 }]' '{ \"rescan\": false}'") +

            "\nResponse is an array with the same size as the input that has the execution result :\n"
            "  [{ \"success\": true } , { \"success\": false, \"error\": { \"code\": -1, \"message\": \"Internal Server Error\"} }, ... ]\n");

    // clang-format on

    RPCTypeCheck(mainRequest.params, {UniValue::VARR, UniValue::VOBJ});

    const UniValue& requests = mainRequest.params[0];

    //Default options
    bool fRescan = true;

    if (!mainRequest.params[1].isNull()) {
        const UniValue& options = mainRequest.params[1];

        if (options.exists("rescan")) {
            fRescan = options["rescan"].get_bool();
        }
    }

    WalletRescanReserver reserver(pwallet);
    if (fRescan && !reserver.reserve()) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Wallet is currently rescanning. Abort existing rescan or wait.");
    }

    int64_t now = 0;
    bool fRunScan = false;
    int64_t nLowestTimestamp = 0;
    UniValue response(UniValue::VARR);
    {
        LOCK2(cs_main, pwallet->cs_wallet);
        EnsureWalletIsUnlocked(pwallet);

        // Verify all timestamps are present before importing any keys.
        now = chainActive.Tip() ? chainActive.Tip()->GetMedianTimePast() : 0;
        for (const UniValue& data : requests.getValues()) {
            GetImportTimestamp(data, now);
        }

        const int64_t minimumTimestamp = 1;

        if (fRescan && chainActive.Tip()) {
            nLowestTimestamp = chainActive.Tip()->GetBlockTime();
        } else {
            fRescan = false;
        }

        for (const UniValue& data : requests.getValues()) {
            const int64_t timestamp = std::max(GetImportTimestamp(data, now), minimumTimestamp);
            const UniValue result = ProcessImport(pwallet, data, timestamp);
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
        int64_t scannedTime = pwallet->RescanFromTime(nLowestTimestamp, reserver, true /* update */);
        pwallet->ReacceptWalletTransactions();

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
                                      "and -rescan options).",
                                GetImportTimestamp(request, now), scannedTime - TIMESTAMP_WINDOW - 1, TIMESTAMP_WINDOW)));
                    response.push_back(std::move(result));
                }
                ++i;
            }
        }
    }

    return response;
}
