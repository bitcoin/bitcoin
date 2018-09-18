// Copyright (c) 2017 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//#include <amount.h>
//#include <base58.h>
#include "assets/assets.h"
#include "assets/assetdb.h"
#include <map>
#include "tinyformat.h"
//#include <rpc/server.h>
//#include <script/standard.h>
//#include <utilstrencodings.h>

#include "amount.h"
#include "base58.h"
#include "chain.h"
#include "consensus/validation.h"
#include "core_io.h"
#include "httpserver.h"
#include "validation.h"
#include "net.h"
#include "policy/feerate.h"
#include "policy/fees.h"
#include "policy/policy.h"
#include "policy/rbf.h"
#include "rpc/mining.h"
#include "rpc/safemode.h"
#include "rpc/server.h"
#include "script/sign.h"
#include "timedata.h"
#include "util.h"
#include "utilmoneystr.h"
#include "wallet/coincontrol.h"
#include "wallet/feebumper.h"
#include "wallet/wallet.h"
#include "wallet/walletdb.h"

std::string AssetActivationWarning()
{
    return AreAssetsDeployed() ? "" : "\nTHIS COMMAND IS NOT YET ACTIVE!\nhttps://github.com/RavenProject/rips/blob/master/rip-0002.mediawiki\n";
}

std::string AssetTypeToString(AssetType& assetType)
{
    switch (assetType)
    {
        case AssetType::ROOT:          return "ROOT";
        case AssetType::SUB:           return "SUB";
        case AssetType::UNIQUE:        return "UNIQUE";
        case AssetType::OWNER:         return "OWNER";
        case AssetType::MSGCHANNEL:    return "MSGCHANNEL";
        case AssetType::VOTE:          return "VOTE";
        case AssetType::REISSUE:       return "REISSUE";
        case AssetType::INVALID:       return "INVALID";
        default:            return "UNKNOWN";
    }
}

UniValue UnitValueFromAmount(const CAmount& amount, const std::string asset_name)
{
    if (!passets)
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Asset cache isn't available.");

    uint8_t units = OWNER_UNITS;
    if (!IsAssetNameAnOwner(asset_name)) {
        CNewAsset assetData;
        if (!passets->GetAssetIfExists(asset_name, assetData))
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Couldn't load asset from cache: " + asset_name);

        units = assetData.units;
    }

    return ValueFromAmount(amount, units);
}

UniValue issue(const JSONRPCRequest& request)
{
    if (request.fHelp || !AreAssetsDeployed() || request.params.size() < 1 || request.params.size() > 8)
        throw std::runtime_error(
            "issue \"asset_name\" qty \"( to_address )\" \"( change_address )\" ( units ) ( reissuable ) ( has_ipfs ) \"( ipfs_hash )\"\n"
            + AssetActivationWarning() +
            "\nIssue an asset, subasset or unique asset.\n"
            "Asset name must not conflict with any existing asset.\n"
            "Unit as the number of decimals precision for the asset (0 for whole units (\"1\"), 8 for max precision (\"1.00000000\")\n"
            "Reissuable is true/false for whether additional units can be issued by the original issuer.\n"
            "If issuing a unique asset these values are required (and will be defaulted to): qty=1, units=0, reissuable=false.\n"

            "\nArguments:\n"
            "1. \"asset_name\"            (string, required) a unique name\n"
            "2. \"qty\"                   (numeric, optional, default=1) the number of units to be issued\n"
            "3. \"to_address\"            (string), optional, default=\"\"), address asset will be sent to, if it is empty, address will be generated for you\n"
            "4. \"change_address\"        (string), optional, default=\"\"), address the the rvn change will be sent to, if it is empty, change address will be generated for you\n"
            "5. \"units\"                 (integer, optional, default=0, min=0, max=8), the number of decimals precision for the asset (0 for whole units (\"1\"), 8 for max precision (\"1.00000000\")\n"
            "6. \"reissuable\"            (boolean, optional, default=true (false for unique assets)), whether future reissuance is allowed\n"
            "7. \"has_ipfs\"              (boolean, optional, default=false), whether ifps hash is going to be added to the asset\n"
            "8. \"ipfs_hash\"             (string, optional but required if has_ipfs = 1), an ipfs hash\n"

            "\nResult:\n"
            "\"txid\"                     (string) The transaction id\n"

            "\nExamples:\n"
            + HelpExampleCli("issue", "\"ASSET_NAME\" 1000")
            + HelpExampleCli("issue", "\"ASSET_NAME\" 1000 \"myaddress\"")
            + HelpExampleCli("issue", "\"ASSET_NAME\" 1000 \"myaddress\" \"changeaddress\" 4")
            + HelpExampleCli("issue", "\"ASSET_NAME\" 1000 \"myaddress\" \"changeaddress\" 2 true")
            + HelpExampleCli("issue", "\"ASSET_NAME/SUB_ASSET\" 1000 \"myaddress\" \"changeaddress\" 2 true")
            + HelpExampleCli("issue", "\"ASSET_NAME#uniquetag\"")
        );

    CWallet * const pwallet = GetWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return NullUniValue;
    }

    ObserveSafeMode();
    LOCK2(cs_main, pwallet->cs_wallet);

    EnsureWalletIsUnlocked(pwallet);

    // Check asset name and infer assetType
    std::string assetName = request.params[0].get_str();
    AssetType assetType;
    if (!IsAssetNameValid(assetName, assetType)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Invalid asset name: ") + assetName);
    }

    // Check assetType supported
    if (!(assetType == AssetType::ROOT || assetType == AssetType::SUB || assetType == AssetType::UNIQUE)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Unsupported asset type: ") + AssetTypeToString(assetType));
    }

    CAmount nAmount = COIN;
    if (request.params.size() > 1)
        nAmount = AmountFromValue(request.params[1]);

    std::string address = "";
    if (request.params.size() > 2)
        address = request.params[2].get_str();

    if (!address.empty()) {
        CTxDestination destination = DecodeDestination(address);
        if (!IsValidDestination(destination)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Raven address: ") + address);
        }
    } else {
        // Create a new address
        std::string strAccount;

        if (!pwallet->IsLocked()) {
            pwallet->TopUpKeyPool();
        }

        // Generate a new key that is added to wallet
        CPubKey newKey;
        if (!pwallet->GetKeyFromPool(newKey)) {
            throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, "Error: Keypool ran out, please call keypoolrefill first");
        }
        CKeyID keyID = newKey.GetID();

        pwallet->SetAddressBook(keyID, strAccount, "receive");

        address = EncodeDestination(keyID);
    }

    std::string changeAddress = "";
    if (request.params.size() > 3)
        changeAddress = request.params[3].get_str();
    if (!changeAddress.empty()) {
        CTxDestination destination = DecodeDestination(changeAddress);
        if (!IsValidDestination(destination)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                               std::string("Invalid Change Address: Invalid Raven address: ") + changeAddress);
        }
    }

    int units = 0;
    if (request.params.size() > 4)
        units = request.params[4].get_int();

    bool reissuable = assetType != AssetType::UNIQUE;
    if (request.params.size() > 5)
        reissuable = request.params[5].get_bool();

    bool has_ipfs = false;
    if (request.params.size() > 6)
        has_ipfs = request.params[6].get_bool();

    std::string ipfs_hash = "";
    if (request.params.size() > 7 && has_ipfs)
        ipfs_hash = request.params[7].get_str();

    // check for required unique asset params
    if (assetType == AssetType::UNIQUE && (nAmount != COIN || units != 0 || reissuable)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Invalid parameters for issuing a unique asset."));
    }

    CNewAsset asset(assetName, nAmount, units, reissuable ? 1 : 0, has_ipfs ? 1 : 0, DecodeIPFS(ipfs_hash));

    CReserveKey reservekey(pwallet);
    CWalletTx transaction;
    CAmount nRequiredFee;
    std::pair<int, std::string> error;

    CCoinControl crtl;
    crtl.destChange = DecodeDestination(changeAddress);

    // Create the Transaction
    if (!CreateAssetTransaction(pwallet, crtl, asset, address, error, transaction, reservekey, nRequiredFee))
        throw JSONRPCError(error.first, error.second);

    // Send the Transaction to the network
    std::string txid;
    if (!SendAssetTransaction(pwallet, transaction, reservekey, error, txid))
        throw JSONRPCError(error.first, error.second);

    UniValue result(UniValue::VARR);
    result.push_back(txid);
    return result;
}

UniValue issueunique(const JSONRPCRequest& request)
{
    if (request.fHelp || !AreAssetsDeployed() || request.params.size() < 2 || request.params.size() > 3)
        throw std::runtime_error(
                "issueunique \"root_name\" [asset_tags] ( [ipfs_hashes] ) \"( to_address )\" \"( change_address )\"\n"
                + AssetActivationWarning() +
                "\nIssue unique asset(s).\n"
                "root_name must be an asset you own.\n"
                "An asset will be created for each element of asset_tags.\n"
                "If provided ipfs_hashes must be the same length as asset_tags.\n"
                "Five (5) RVN will be burned for each asset created.\n"

                "\nArguments:\n"
                "1. \"root_name\"             (string, required) name of the asset the unique asset(s) are being issued under\n"
                "2. \"asset_tags\"            (array, required) the unique tag for each asset which is to be issued\n"
                "3. \"ipfs_hashes\"           (array, optional) ipfs hashes corresponding to each supplied tag (should be same size as \"asset_tags\")\n"
                "4. \"to_address\"            (string, optional, default=\"\"), address assets will be sent to, if it is empty, address will be generated for you\n"
                "5. \"change_address\"        (string, optional, default=\"\"), address the the rvn change will be sent to, if it is empty, change address will be generated for you\n"

                "\nResult:\n"
                "\"txid\"                     (string) The transaction id\n"

                "\nExamples:\n"
                + HelpExampleCli("issueunique", "\"MY_ASSET\" [\"primo\",\"secundo\"]")
                + HelpExampleCli("issueunique", "\"MY_ASSET\" [\"primo\",\"secundo\"] [\"first_hash\",\"second_hash\"]")
        );

    CWallet * const pwallet = GetWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return NullUniValue;
    }

    ObserveSafeMode();
    LOCK2(cs_main, pwallet->cs_wallet);

    EnsureWalletIsUnlocked(pwallet);


    const std::string rootName = request.params[0].get_str();
    AssetType assetType;
    if (!IsAssetNameValid(rootName, assetType)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Invalid asset name: ") + rootName);
    }
    if (assetType != AssetType::ROOT && assetType != AssetType::SUB) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Root asset must be a regular top-level or sub-asset."));
    }

    const UniValue& assetTags = request.params[1];
    if (!assetTags.isArray() || assetTags.size() < 1) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Asset tags must be a non-empty array."));
    }

    const UniValue& ipfsHashes = request.params[2];
    if (!ipfsHashes.isNull()) {
        if (!ipfsHashes.isArray() || ipfsHashes.size() != assetTags.size()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("If provided, IPFS hashes must be an array of the same size as the asset tags array."));
        }
    }

    std::string address = "";
    if (request.params.size() > 3)
        address = request.params[3].get_str();

    if (!address.empty()) {
        CTxDestination destination = DecodeDestination(address);
        if (!IsValidDestination(destination)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Raven address: ") + address);
        }
    } else {
        // Create a new address
        std::string strAccount;

        if (!pwallet->IsLocked()) {
            pwallet->TopUpKeyPool();
        }

        // Generate a new key that is added to wallet
        CPubKey newKey;
        if (!pwallet->GetKeyFromPool(newKey)) {
            throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, "Error: Keypool ran out, please call keypoolrefill first");
        }
        CKeyID keyID = newKey.GetID();

        pwallet->SetAddressBook(keyID, strAccount, "receive");

        address = EncodeDestination(keyID);
    }

    std::string changeAddress = "";
    if (request.params.size() > 4)
        changeAddress = request.params[4].get_str();
    if (!changeAddress.empty()) {
        CTxDestination destination = DecodeDestination(changeAddress);
        if (!IsValidDestination(destination)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                               std::string("Invalid Change Address: Invalid Raven address: ") + changeAddress);
        }
    }

    std::vector<CNewAsset> assets;
    for (int i = 0; i < (int)assetTags.size(); i++) {
        std::string tag = assetTags[i].get_str();

        if (!IsUniqueTagValid(tag)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Unique asset tag is invalid: " + tag));
        }

        std::string assetName = GetUniqueAssetName(rootName, tag);
        CNewAsset asset;

        if (ipfsHashes.isNull())
        {
            asset = CNewAsset(assetName, UNIQUE_ASSET_AMOUNT, UNIQUE_ASSET_UNITS, UNIQUE_ASSETS_REISSUABLE, 0, "");
        }
        else
        {
            asset = CNewAsset(assetName, UNIQUE_ASSET_AMOUNT, UNIQUE_ASSET_UNITS, UNIQUE_ASSETS_REISSUABLE, 1,
                              DecodeIPFS(ipfsHashes[i].get_str()));
        }

        assets.push_back(asset);
    }

    CReserveKey reservekey(pwallet);
    CWalletTx transaction;
    CAmount nRequiredFee;
    std::pair<int, std::string> error;

    CCoinControl crtl;

    crtl.destChange = DecodeDestination(changeAddress);

    // Create the Transaction
    if (!CreateAssetTransaction(pwallet, crtl, assets, address, error, transaction, reservekey, nRequiredFee))
        throw JSONRPCError(error.first, error.second);

    // Send the Transaction to the network
    std::string txid;
    if (!SendAssetTransaction(pwallet, transaction, reservekey, error, txid))
        throw JSONRPCError(error.first, error.second);

    UniValue result(UniValue::VARR);
    result.push_back(txid);
    return result;
}

UniValue listassetbalancesbyaddress(const JSONRPCRequest& request)
{
    if (request.fHelp || !AreAssetsDeployed() || request.params.size() < 1)
        throw std::runtime_error(
            "listassetbalancesbyaddress \"address\"\n"
            + AssetActivationWarning() +
            "\nReturns a list of all asset balances for an address.\n"

            "\nArguments:\n"
            "1. \"address\"               (string, required) a raven address\n"

            "\nResult:\n"
            "{\n"
            "  (asset_name) : (quantity),\n"
            "  ...\n"
            "}\n"

            "\nExamples:\n"
            + HelpExampleCli("listassetbalancesbyaddress", "\"myaddress\"")
        );

    std::string address = request.params[0].get_str();
    CTxDestination destination = DecodeDestination(address);
    if (!IsValidDestination(destination)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Raven address: ") + address);
    }

    LOCK(cs_main);
    UniValue result(UniValue::VOBJ);

    if (!passets)
        return NullUniValue;

    for (auto it : passets->mapAssetsAddressAmount) {
        if (address.compare(it.first.second) == 0) {
            result.push_back(Pair(it.first.first, UnitValueFromAmount(it.second, it.first.first)));
        }
    }

    return result;
}

UniValue getassetdata(const JSONRPCRequest& request)
{
    if (request.fHelp || !AreAssetsDeployed() || request.params.size() != 1)
        throw std::runtime_error(
                "getassetdata \"asset_name\"\n"
                + AssetActivationWarning() +
                "\nReturns assets metadata if that asset exists\n"

                "\nArguments:\n"
                "1. \"asset_name\"               (string, required) the name of the asset\n"

                "\nResult:\n"
                "{\n"
                "  name: (string),\n"
                "  amount: (number),\n"
                "  units: (number),\n"
                "  reissuable: (number),\n"
                "  has_ipfs: (number),\n"
                "  ipfs_hash: (hash) (only if has_ipfs = 1)\n"
                "}\n"

                "\nExamples:\n"
                + HelpExampleCli("getallassets", "\"ASSET_NAME\"")
        );


    std::string asset_name = request.params[0].get_str();

    LOCK(cs_main);
    UniValue result (UniValue::VOBJ);

    if (passets) {
        CNewAsset asset;
        if (!passets->GetAssetIfExists(asset_name, asset))
            return NullUniValue;

        result.push_back(Pair("name", asset.strName));
        result.push_back(Pair("amount", UnitValueFromAmount(asset.nAmount, asset.strName)));
        result.push_back(Pair("units", asset.units));
        result.push_back(Pair("reissuable", asset.nReissuable));
        result.push_back(Pair("has_ipfs", asset.nHasIPFS));
        if (asset.nHasIPFS)
            result.push_back(Pair("ipfs_hash", EncodeIPFS(asset.strIPFSHash)));

        return result;
    }

    return NullUniValue;
}

template <class Iter, class Incr>
void safe_advance(Iter& curr, const Iter& end, Incr n)
{
    size_t remaining(std::distance(curr, end));
    if (remaining < n)
    {
        n = remaining;
    }
    std::advance(curr, n);
};

UniValue listmyassets(const JSONRPCRequest &request)
{
    if (request.fHelp || !AreAssetsDeployed() || request.params.size() > 4)
        throw std::runtime_error(
                "listmyassets \"( asset )\" ( verbose ) ( count ) ( start )\n"
                + AssetActivationWarning() +
                "\nReturns a list of all asset that are owned by this wallet\n"

                "\nArguments:\n"
                "1. \"asset\"                    (string, optional, default=\"*\") filters results -- must be an asset name or a partial asset name followed by '*' ('*' matches all trailing characters)\n"
                "2. \"verbose\"                  (boolean, optional, default=false) when false results only contain balances -- when true results include outpoints\n"
                "3. \"count\"                    (integer, optional, default=ALL) truncates results to include only the first _count_ assets found\n"
                "4. \"start\"                    (integer, optional, default=0) results skip over the first _start_ assets found (if negative it skips back from the end)\n"

                "\nResult (verbose=false):\n"
                "{\n"
                "  (asset_name): balance,\n"
                "  ...\n"
                "}\n"

                "\nResult (verbose=true):\n"
                "{\n"
                "  (asset_name):\n"
                "    {\n"
                "      \"balance\": balance,\n"
                "      \"outpoints\":\n"
                "        [\n"
                "          {\n"
                "            \"txid\": txid,\n"
                "            \"vout\": vout,\n"
                "            \"amount\": amount\n"
                "          }\n"
                "          {...}, {...}\n"
                "        ]\n"
                "    }\n"
                "}\n"
                "{...}, {...}\n"

                "\nExamples:\n"
                + HelpExampleRpc("listmyassets", "")
                + HelpExampleCli("listmyassets", "ASSET")
                + HelpExampleCli("listmyassets", "\"ASSET*\" true 10 20")
        );

    CWallet * const pwallet = GetWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return NullUniValue;
    }

    ObserveSafeMode();
    LOCK2(cs_main, pwallet->cs_wallet);

    if (!passets)
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Asset cache unavailable.");

    std::string filter = "*";
    if (request.params.size() > 0)
        filter = request.params[0].get_str();

    if (filter == "")
        filter = "*";

    bool verbose = false;
    if (request.params.size() > 1)
        verbose = request.params[1].get_bool();

    size_t count = INT_MAX;
    if (request.params.size() > 2) {
        if (request.params[2].get_int() < 1)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "count must be greater than 1.");
        count = request.params[2].get_int();
    }

    long start = 0;
    if (request.params.size() > 3) {
        start = request.params[3].get_int();
    }

    // retrieve balances
    std::map<std::string, CAmount> balances;
    if (filter == "*") {
        if (!GetMyAssetBalances(*passets, balances))
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Couldn't get asset balances. For all assets");
    }
    else if (filter.back() == '*') {
        std::vector<std::string> assetNames;
        filter.pop_back();
        if (!GetMyOwnedAssets(*passets, filter, assetNames))
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Couldn't get owned assets.");
        if (!GetMyAssetBalances(*passets, assetNames, balances))
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Couldn't get asset balances.");
    }
    else {
        if (!IsAssetNameValid(filter))
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid asset name.");
        CAmount balance;
        if (!GetMyAssetBalance(*passets, filter, balance)) {
            throw JSONRPCError(RPC_INTERNAL_ERROR, std::string("Couldn't get asset balances. For asset : ") + filter);
        }
        balances[filter] = balance;
    }

    // pagination setup
    auto bal = balances.begin();
    if (start >= 0)
        safe_advance(bal, balances.end(), (size_t)start);
    else
        safe_advance(bal, balances.end(), balances.size() + start);
    auto end = bal;
    safe_advance(end, balances.end(), count);

    // generate output
    UniValue result(UniValue::VOBJ);
    if (verbose) {
        for (; bal != end && bal != balances.end(); bal++) {
            UniValue asset(UniValue::VOBJ);
            asset.push_back(Pair("balance", UnitValueFromAmount(bal->second, bal->first)));

            UniValue outpoints(UniValue::VARR);
            for (auto const& out : passets->mapMyUnspentAssets[bal->first]) {
                UniValue tempOut(UniValue::VOBJ);
                tempOut.push_back(Pair("txid", out.hash.GetHex()));
                tempOut.push_back(Pair("vout", (int)out.n));

                //
                // get amount for this outpoint
                CAmount txAmount = 0;
                auto it = pwallet->mapWallet.find(out.hash);
                if (it == pwallet->mapWallet.end()) {
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid or non-wallet transaction id");
                }
                const CWalletTx& wtx = it->second;
                CTxOut txOut = wtx.tx->vout[out.n];
                std::string strAddress;
                if (CheckIssueDataTx(txOut)) {
                    CNewAsset asset;
                    if (!AssetFromScript(txOut.scriptPubKey, asset, strAddress))
                        throw JSONRPCError(RPC_INTERNAL_ERROR, "Couldn't get asset from script.");
                    txAmount = asset.nAmount;
                }
                else if (CheckReissueDataTx(txOut)) {
                    CReissueAsset asset;
                    if (!ReissueAssetFromScript(txOut.scriptPubKey, asset, strAddress))
                        throw JSONRPCError(RPC_INTERNAL_ERROR, "Couldn't get asset from script.");
                    txAmount = asset.nAmount;
                }
                else if (CheckTransferOwnerTx(txOut)) {
                    CAssetTransfer asset;
                    if (!TransferAssetFromScript(txOut.scriptPubKey, asset, strAddress))
                        throw JSONRPCError(RPC_INTERNAL_ERROR, "Couldn't get asset from script.");
                    txAmount = asset.nAmount;
                }
                else if (CheckOwnerDataTx(txOut)) {
                    std::string assetName;
                    if (!OwnerAssetFromScript(txOut.scriptPubKey, assetName, strAddress))
                        throw JSONRPCError(RPC_INTERNAL_ERROR, "Couldn't get asset from script.");
                    txAmount = OWNER_ASSET_AMOUNT;
                }
                tempOut.push_back(Pair("amount", UnitValueFromAmount(txAmount, bal->first)));
                //
                //

                outpoints.push_back(tempOut);
            }
            asset.push_back(Pair("outpoints", outpoints));
            result.push_back(Pair(bal->first, asset));
        }
    }
    else {
        for (; bal != end && bal != balances.end(); bal++) {
            result.push_back(Pair(bal->first, UnitValueFromAmount(bal->second, bal->first)));
        }
    }
    return result;
}

UniValue listaddressesbyasset(const JSONRPCRequest &request)
{
    if (request.fHelp || !AreAssetsDeployed() || request.params.size() != 1)
        throw std::runtime_error(
                "listaddressesbyasset \"asset_name\"\n"
                + AssetActivationWarning() +
                "\nReturns a list of all address that own the given asset (with balances)"

                "\nArguments:\n"
                "1. \"asset_name\"               (string, required) name of asset\n"

                "\nResult:\n"
                "[ "
                "  (address): balance,\n"
                "  ...\n"
                "]\n"

                "\nExamples:\n"
                + HelpExampleCli("getassetsaddresses", "ASSET_NAME")
                + HelpExampleCli("getassetsaddresses", "ASSET_NAME")
        );

    LOCK(cs_main);

    std::string asset_name = request.params[0].get_str();

    if (!passets)
        return NullUniValue;

    if (!passets->mapAssetsAddresses.count(asset_name))
        return NullUniValue;

    UniValue addresses(UniValue::VOBJ);

    auto setAddresses = passets->mapAssetsAddresses.at(asset_name);
    for (auto it : setAddresses) {
        auto pair = std::make_pair(asset_name, it);

        if (GetBestAssetAddressAmount(*passets, asset_name, it))
            addresses.push_back(Pair(it, UnitValueFromAmount(passets->mapAssetsAddressAmount.at(pair), asset_name)));
    }

    return addresses;
}

UniValue transfer(const JSONRPCRequest& request)
{
    if (request.fHelp || !AreAssetsDeployed() || request.params.size() != 3)
        throw std::runtime_error(
                "transfer \"asset_name\" qty \"to_address\"\n"
                + AssetActivationWarning() +
                "\nTransfers a quantity of an owned asset to a given address"

                "\nArguments:\n"
                "1. \"asset_name\"               (string, required) name of asset\n"
                "2. \"qty\"                      (numeric, required) number of assets you want to send to the address\n"
                "3. \"to_address\"               (string, required) address to send the asset to\n"

                "\nResult:\n"
                "txid"
                "[ \n"
                "txid\n"
                "]\n"

                "\nExamples:\n"
                + HelpExampleCli("transfer", "\"ASSET_NAME\" 20 \"address\"")
                + HelpExampleCli("transfer", "\"ASSET_NAME\" 20 \"address\"")
        );

    CWallet * const pwallet = GetWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return NullUniValue;
    }

    ObserveSafeMode();
    LOCK2(cs_main, pwallet->cs_wallet);

    EnsureWalletIsUnlocked(pwallet);

    std::string asset_name = request.params[0].get_str();

    CAmount nAmount = AmountFromValue(request.params[1]);

    std::string address = request.params[2].get_str();

    std::pair<int, std::string> error;
    std::vector< std::pair<CAssetTransfer, std::string> >vTransfers;

    vTransfers.emplace_back(std::make_pair(CAssetTransfer(asset_name, nAmount), address));
    CReserveKey reservekey(pwallet);
    CWalletTx transaction;
    CAmount nRequiredFee;

    CCoinControl ctrl;

    // Create the Transaction
    if (!CreateTransferAssetTransaction(pwallet, ctrl, vTransfers, "", error, transaction, reservekey, nRequiredFee))
        throw JSONRPCError(error.first, error.second);

    // Send the Transaction to the network
    std::string txid;
    if (!SendAssetTransaction(pwallet, transaction, reservekey, error, txid))
        throw JSONRPCError(error.first, error.second);

    // Display the transaction id
    UniValue result(UniValue::VARR);
    result.push_back(txid);
    return result;
}

UniValue reissue(const JSONRPCRequest& request)
{
    if (request.fHelp || !AreAssetsDeployed() || request.params.size() > 7 || request.params.size() < 3)
        throw std::runtime_error(
                "reissue \"asset_name\" qty \"to_address\" \"change_address\" ( reissuable ) ( new_unit) \"( new_ipfs )\" \n"
                + AssetActivationWarning() +
                "\nReissues a quantity of an asset to an owned address if you own the Owner Token"
                "\nCan change the reissuable flag during reissuance"
                "\nCan change the ipfs hash during reissuance"

                "\nArguments:\n"
                "1. \"asset_name\"               (string, required) name of asset that is being reissued\n"
                "2. \"qty\"                      (numeric, required) number of assets to reissue\n"
                "3. \"to_address\"               (string, required) address to send the asset to\n"
                "4. \"change_address\"           (string, optional) address that the change of the transaction will be sent to\n"
                "5. \"reissuable\"               (boolean, optional, default=true), whether future reissuance is allowed\n"
                "6. \"new_unit\"                 (numeric, optional, default=-1), the new units that will be associated with the asset\n"
                "7. \"new_ifps\"                 (string, optional, default=\"\"), whether to update the current ipfshash\n"

                "\nResult:\n"
                "\"txid\"                     (string) The transaction id\n"

                "\nExamples:\n"
                + HelpExampleCli("reissue", "\"ASSET_NAME\" 20 \"address\"")
                + HelpExampleCli("reissue", "\"ASSET_NAME\" 20 \"address\" \"change_address\" \"true\" 8 \"Qmd286K6pohQcTKYqnS1YhWrCiS4gz7Xi34sdwMe9USZ7u\"")
        );

    CWallet * const pwallet = GetWalletForJSONRPCRequest(request);
    if (!EnsureWalletIsAvailable(pwallet, request.fHelp)) {
        return NullUniValue;
    }

    ObserveSafeMode();
    LOCK2(cs_main, pwallet->cs_wallet);

    // To send a transaction the wallet must be unlocked
    EnsureWalletIsUnlocked(pwallet);

    // Get that paramaters
    std::string asset_name = request.params[0].get_str();
    CAmount nAmount = AmountFromValue(request.params[1]);
    std::string address = request.params[2].get_str();

    std::string changeAddress =  "";
    if (request.params.size() > 3)
        changeAddress = request.params[3].get_str();

    bool reissuable = true;
    if (request.params.size() > 4) {
        reissuable = request.params[4].get_bool();
    }

    int newUnits = -1;
    if (request.params.size() > 5) {
        newUnits = request.params[5].get_int();
    }

    std::string newipfs = "";
    if (request.params.size() > 6) {
        newipfs = request.params[6].get_str();
        if (newipfs.length() != 46)
            throw JSONRPCError(RPC_INVALID_PARAMS, std::string("Invalid IPFS hash (must be 46 characters"));
    }

    CReissueAsset reissueAsset(asset_name, nAmount, newUnits, reissuable, DecodeIPFS(newipfs));

    std::pair<int, std::string> error;
    CReserveKey reservekey(pwallet);
    CWalletTx transaction;
    CAmount nRequiredFee;

    CCoinControl crtl;
    crtl.destChange = DecodeDestination(changeAddress);

    // Create the Transaction
    if (!CreateReissueAssetTransaction(pwallet, crtl, reissueAsset, address, error, transaction, reservekey, nRequiredFee))
        throw JSONRPCError(error.first, error.second);

    // Send the Transaction to the network
    std::string txid;
    if (!SendAssetTransaction(pwallet, transaction, reservekey, error, txid))
        throw JSONRPCError(error.first, error.second);

    UniValue result(UniValue::VARR);
    result.push_back(txid);
    return result;
}

UniValue listassets(const JSONRPCRequest& request)
{
    if (request.fHelp || !AreAssetsDeployed() || request.params.size() > 4)
        throw std::runtime_error(
                "listassets \"( asset )\" ( verbose ) ( count ) ( start )\n"
                + AssetActivationWarning() +
                "\nReturns a list of all asset that are owned by this wallet\n"
                "\nThis could be a slow/expensive operation as it reads from the database\n"
                "\nAssets come back in timestamp order (oldest to newest)\n"

                "\nArguments:\n"
                "1. \"asset\"                    (string, optional, default=\"*\") filters results -- must be an asset name or a partial asset name followed by '*' ('*' matches all trailing characters)\n"
                "2. \"verbose\"                  (boolean, optional, default=false) when false result is just a list of asset names -- when true results are asset name mapped to metadata\n"
                "3. \"count\"                    (integer, optional, default=ALL) truncates results to include only the first _count_ assets found\n"
                "4. \"start\"                    (integer, optional, default=0) results skip over the first _start_ assets found (if negative it skips back from the end)\n"

                "\nResult (verbose=false):\n"
                "[\n"
                "  asset_name,\n"
                "  ...\n"
                "]\n"

                "\nResult (verbose=true):\n"
                "{\n"
                "  (asset_name):\n"
                "    {\n"
                "      amount: (number),\n"
                "      units: (number),\n"
                "      reissuable: (number),\n"
                "      has_ipfs: (number),\n"
                "      ipfs_hash: (hash) (only if has_ipfs = 1)\n"
                "    },\n"
                "  {...}, {...}\n"
                "}\n"

                "\nExamples:\n"
                + HelpExampleRpc("listassets", "")
                + HelpExampleCli("listassets", "ASSET")
                + HelpExampleCli("listassets", "\"ASSET*\" true 10 20")
        );

    ObserveSafeMode();

    if (!passetsdb)
        throw JSONRPCError(RPC_INTERNAL_ERROR, "asset db unavailable.");

    std::string filter = "*";
    if (request.params.size() > 0)
        filter = request.params[0].get_str();

    if (filter == "")
        filter = "*";

    bool verbose = false;
    if (request.params.size() > 1)
        verbose = request.params[1].get_bool();

    size_t count = INT_MAX;
    if (request.params.size() > 2) {
        if (request.params[2].get_int() < 1)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "count must be greater than 1.");
        count = request.params[2].get_int();
    }

    long start = 0;
    if (request.params.size() > 3) {
        start = request.params[3].get_int();
    }

    std::vector<CNewAsset> assets;
    if (!passetsdb->AssetDir(assets, filter, count, start))
        throw JSONRPCError(RPC_INTERNAL_ERROR, "couldn't retrieve asset directory.");

    UniValue result;
    result = verbose ? UniValue(UniValue::VOBJ) : UniValue(UniValue::VARR);

    for (auto asset : assets) {
        if (verbose) {
            UniValue detail(UniValue::VOBJ);
            detail.push_back(Pair("name", asset.strName));
            detail.push_back(Pair("amount", UnitValueFromAmount(asset.nAmount, asset.strName)));
            detail.push_back(Pair("units", asset.units));
            detail.push_back(Pair("reissuable", asset.nReissuable));
            detail.push_back(Pair("has_ipfs", asset.nHasIPFS));
            if (asset.nHasIPFS)
                detail.push_back(Pair("ipfs_hash", EncodeIPFS(asset.strIPFSHash)));
            result.push_back(Pair(asset.strName, detail));
        } else {
            result.push_back(asset.strName);
        }
    }

    return result;
}

static const CRPCCommand commands[] =
{ //  category    name                          actor (function)             argNames
  //  ----------- ------------------------      -----------------------      ----------
    { "assets",   "issue",                      &issue,                      {"asset_name","qty","to_address","change_address","units","reissuable","has_ipfs","ipfs_hash"} },
    { "assets",   "issueunique",                &issueunique,                {"root_name", "asset_tags", "ipfs_hashes", "to_address", "change_address"}},
    { "assets",   "listassetbalancesbyaddress", &listassetbalancesbyaddress, {"address"} },
    { "assets",   "getassetdata",               &getassetdata,               {"asset_name"}},
    { "assets",   "listmyassets",               &listmyassets,               {"asset", "verbose", "count", "start"}},
    { "assets",   "listaddressesbyasset",       &listaddressesbyasset,       {"asset_name"}},
    { "assets",   "transfer",                   &transfer,                   {"asset_name", "qty", "to_address"}},
    { "assets",   "reissue",                    &reissue,                    {"asset_name", "qty", "to_address", "change_address", "reissuable", "new_ipfs"}},
    { "assets",   "listassets",                 &listassets,                 {"asset", "verbose", "count", "start"}}
};

void RegisterAssetRPCCommands(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
