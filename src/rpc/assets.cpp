// Copyright (c) 2017 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <amount.h>
#include <base58.h>
#include <assets/assets.h>
#include <rpc/server.h>
#include <script/standard.h>
#include <utilstrencodings.h>

#include <univalue.h>

//issue(to_address, asset_name, qty, units=1, reissuable=false)
//Issue an asset with unique name. Unit as 1 for whole units, or 0.00000001 for satoshi-like units. Qty should be whole number. Reissuable is true/false for whether additional units can be issued by the original issuer.
UniValue issue(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 3 || request.params.size() > 5)
        throw std::runtime_error(
            "issue \"to_address\" \"asset_name\" \"qty\" ( units ) ( reissuable )\n"
            "\nIssue an asset with unique name.\n"
            "Unit as 1 for whole units, or 0.00000001 for satoshi-like units.\n"
            "Qty should be whole number.\n"
            "Reissuable is true/false for whether additional units can be issued by the original issuer.\n"

            "\nArguments:\n"
            "1. \"to_address\"            (string, required) a raven address\n"
            "2. \"asset_name\"            (string, required) a unique name\n"
            "3. \"qty\"                   (integer, required) the number of units to be issued\n"
            "4. \"units\"                 (string, optional, default=\"1\") the atomic unit size (1, 0.1, ... ,0.00000001)\n"
            "5. \"reissuable\"            (boolean, optional, default=false) whether future reissuance is allowed\n"

            "\nResult:\n"
            "\"txid\"                     (string) The transaction id\n"

            "\nExamples:\n"
            + HelpExampleCli("issue", "\"myaddress\" \"myassetname\" 1000")
            + HelpExampleCli("issue", "\"myaddress\" \"myassetname\" 1000 \"0.0001\"")
            + HelpExampleCli("issue", "\"myaddress\" \"myassetname\" 1000 \"0.01\" true")
        );

    std::string to_address_ = request.params[0].get_str();
    CTxDestination destination = DecodeDestination(to_address_);
    if (!IsValidDestination(destination)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Raven address: ") + to_address_);
    }

    //TODO: Validate asset name (follows rules, unique, not reserved, etc.).  This is a stub.
    std::string asset_name_ = request.params[1].get_str();
    if (!IsAssetNameValid(asset_name_)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Invalid asset name: ") + asset_name_);
    }

    int qty_ = request.params[2].get_int();
    if (qty_ < 1) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Invalid asset quantity: ") + std::to_string(qty_));
    }

    CAmount units = COIN;
    if (!request.params[3].isNull()) {
        std::string units_ = request.params[3].get_str();
        units = AmountFromValue(units_);
        if (!IsAssetUnitsValid(units)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Invalid asset units: ") + units_);
        }
    }

    UniValue result(UniValue::VARR);
    return result;
}

//getaddressbalances(address, minconf=1)
//Returns a list of all the asset balances for address in this node’s wallet, with at least minconf confirmations.
UniValue getaddressbalances(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 1)
        throw std::runtime_error(
            "getaddressbalances \"address\" ( minconf )\n"
            "\nReturns a list of all the asset balances for address in this node's wallet, with at least minconf confirmations.\n"

            "\nArguments:\n"
            "1. \"address\"               (string, required) a raven address\n"
            "2. \"minconf\"               (integer, optional, default=1) the minimum required confirmations\n"

            "\nResult:\n"
            "TBD\n"

            "\nExamples:\n"
            + HelpExampleCli("getaddressbalances", "\"myaddress\"")
            + HelpExampleCli("getaddressbalances", "\"myaddress\" 5")
        );

    std::string address_ = request.params[0].get_str();
    CTxDestination destination = DecodeDestination(address_);
    if (!IsValidDestination(destination)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Raven address: ") + address_);
    }

    int minconf = 1;
    if (!request.params[1].isNull()) {
        minconf = request.params[1].get_int();
        if (minconf < 1) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, std::string("Invalid minconf: ") + std::to_string(minconf));
        }
    }

    UniValue result(UniValue::VARR);
    return result;
}

//issuefrom(from_address, to_address, qty, units, units=1, reissuable=false)
//Issue an asset with unique name from a specific address -- allows control of which address/private_key is used to issue the asset. Unit as 1 for whole units, or 0.00000001 for satoshi-like units. Qty should be whole number. Reissuable is true/false for whether additional units can be issued by the original issuer.

//issuemore(to_address, asset_name, qty)
//Issue more of a specific asset. This is only allowed by the original issuer of the asset and if the reissuable flag was set to true at the time of original issuance.

//makeuniqueasset(address, asset_name, unique_id)
//Creates a unique asset from a pool of assets with a specific name. Example: If the asset name is SOFTLICENSE, then this could make unique assets like SOFTLICENSE:38293 and SOFTLICENSE:48382 This would be called once per unique asset needed.

//listassets(assets=*, verbose=false, count=MAX, start=0)
//This lists assets that have already been created. It does not distinguish unique assets.

//listuniqueassets(asset)
//This lists the assets that have been made unique, and the address that owns the asset.

//sendasset(to_address, asset, amount)
//This sends assets from one asset holder to another.

//sendassetfrom(from_address, to_address, asset, amount)
//This sends asset from one asset holder to another, but allows specifying which address to send from, so that if a wallet that has multiple addresses holding a given asset, the send can disambiguate the address from which to send.

//getassettransaction(asset, txid)
//This returns details for a specific asset transaction.

//listassettransactions(asset, verbose=false, count=100, start=0)
//This returns a list of transactions for a given asset.

//reward(from_address, asset, amount, except=[])
//Sends RVN to holders of the the specified asset. The Raven is split pro-rata to holders of the asset. Any remainder that cannot be evenly divided to the satoshi (1/100,000,000 RVN) level will be added to the mining fee. ​except​ is a list of addresses to exclude from the distribution - used so that you could exclude treasury shares that do not participate in the reward.

//send_asset(from_address, from_asset, to_asset, amount, except=[])
//Sends an asset to holders of the the specified to_asset. This can be used to send a voting token to holders of an asset. Combined with a messaging protocol explaining the vote, it could act as a distributed voting system.

static const CRPCCommand commands[] =
{ //  category    name                      actor (function)         argNames
  //  ----------- ------------------------  -----------------------  ----------
    { "assets",   "issue",                  &issue,                  {"to_address","asset_name","qty","units","reissuable"} },
    { "assets",   "getaddressbalances",     &getaddressbalances,     {"address", "minconf"} },
};

void RegisterAssetRPCCommands(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
