/**
 * @file rpc.cpp
 *
 * This file contains RPC calls for data retrieval.
 */

#include "omnicore/rpc.h"

#include "omnicore/activation.h"
#include "omnicore/consensushash.h"
#include "omnicore/convert.h"
#include "omnicore/dex.h"
#include "omnicore/errors.h"
#include "omnicore/fetchwallettx.h"
#include "omnicore/log.h"
#include "omnicore/mdex.h"
#include "omnicore/notifications.h"
#include "omnicore/omnicore.h"
#include "omnicore/rpcrequirements.h"
#include "omnicore/rpctx.h"
#include "omnicore/rpctxobject.h"
#include "omnicore/rpcvalues.h"
#include "omnicore/rules.h"
#include "omnicore/sp.h"
#include "omnicore/tally.h"
#include "omnicore/tx.h"
#include "omnicore/utilsbitcoin.h"
#include "omnicore/version.h"

#include "amount.h"
#include "init.h"
#include "main.h"
#include "primitives/block.h"
#include "primitives/transaction.h"
#include "rpcserver.h"
#include "tinyformat.h"
#include "uint256.h"
#include "utilstrencodings.h"
#ifdef ENABLE_WALLET
#include "wallet.h"
#endif

#include "json/json_spirit_value.h"

#include <stdint.h>
#include <map>
#include <stdexcept>
#include <string>

using std::runtime_error;
using namespace json_spirit;
using namespace mastercore;

/**
 * Throws a JSONRPCError, depending on error code.
 */
void PopulateFailure(int error)
{
    switch (error) {
        case MP_TX_NOT_FOUND:
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available about transaction");
        case MP_TX_UNCONFIRMED:
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unconfirmed transactions are not supported");
        case MP_BLOCK_NOT_IN_CHAIN:
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Transaction not part of the active chain");
        case MP_CROWDSALE_WITHOUT_PROPERTY:
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Potential database corruption: \
                                                  \"Crowdsale Purchase\" without valid property identifier");
        case MP_INVALID_TX_IN_DB_FOUND:
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Potential database corruption: Invalid transaction found");
        case MP_TX_IS_NOT_MASTER_PROTOCOL:
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Not a Master Protocol transaction");
    }
    throw JSONRPCError(RPC_INTERNAL_ERROR, "Generic transaction population failure");
}

void PropertyToJSON(const CMPSPInfo::Entry& sProperty, Object& property_obj)
{
    property_obj.push_back(Pair("name", sProperty.name));
    property_obj.push_back(Pair("category", sProperty.category));
    property_obj.push_back(Pair("subcategory", sProperty.subcategory));
    property_obj.push_back(Pair("data", sProperty.data));
    property_obj.push_back(Pair("url", sProperty.url));
    property_obj.push_back(Pair("divisible", sProperty.isDivisible()));
}

void MetaDexObjectToJSON(const CMPMetaDEx& obj, Object& metadex_obj)
{
    bool propertyIdForSaleIsDivisible = isPropertyDivisible(obj.getProperty());
    bool propertyIdDesiredIsDivisible = isPropertyDivisible(obj.getDesProperty());
    // add data to JSON object
    metadex_obj.push_back(Pair("address", obj.getAddr()));
    metadex_obj.push_back(Pair("txid", obj.getHash().GetHex()));
    if (obj.getAction() == 4) metadex_obj.push_back(Pair("ecosystem", isTestEcosystemProperty(obj.getProperty()) ? "test" : "main"));
    metadex_obj.push_back(Pair("propertyidforsale", (uint64_t) obj.getProperty()));
    metadex_obj.push_back(Pair("propertyidforsaleisdivisible", propertyIdForSaleIsDivisible));
    metadex_obj.push_back(Pair("amountforsale", FormatMP(obj.getProperty(), obj.getAmountForSale())));
    metadex_obj.push_back(Pair("amountremaining", FormatMP(obj.getProperty(), obj.getAmountRemaining())));
    metadex_obj.push_back(Pair("propertyiddesired", (uint64_t) obj.getDesProperty()));
    metadex_obj.push_back(Pair("propertyiddesiredisdivisible", propertyIdDesiredIsDivisible));
    metadex_obj.push_back(Pair("amountdesired", FormatMP(obj.getDesProperty(), obj.getAmountDesired())));
    metadex_obj.push_back(Pair("amounttofill", FormatMP(obj.getDesProperty(), obj.getAmountToFill())));
    metadex_obj.push_back(Pair("action", (int) obj.getAction()));
    metadex_obj.push_back(Pair("block", obj.getBlock()));
    metadex_obj.push_back(Pair("blocktime", obj.getBlockTime()));
}

void MetaDexObjectsToJSON(std::vector<CMPMetaDEx>& vMetaDexObjs, Array& response)
{
    MetaDEx_compare compareByHeight;

    // sorts metadex objects based on block height and position in block
    std::sort (vMetaDexObjs.begin(), vMetaDexObjs.end(), compareByHeight);

    for (std::vector<CMPMetaDEx>::const_iterator it = vMetaDexObjs.begin(); it != vMetaDexObjs.end(); ++it) {
        Object metadex_obj;
        MetaDexObjectToJSON(*it, metadex_obj);

        response.push_back(metadex_obj);
    }
}

bool BalanceToJSON(const std::string& address, uint32_t property, Object& balance_obj, bool divisible)
{
    // confirmed balance minus unconfirmed, spent amounts
    int64_t nAvailable = getUserAvailableMPbalance(address, property);

    int64_t nReserved = 0;
    nReserved += getMPbalance(address, property, ACCEPT_RESERVE);
    nReserved += getMPbalance(address, property, METADEX_RESERVE);
    nReserved += getMPbalance(address, property, SELLOFFER_RESERVE);

    if (divisible) {
        balance_obj.push_back(Pair("balance", FormatDivisibleMP(nAvailable)));
        balance_obj.push_back(Pair("reserved", FormatDivisibleMP(nReserved)));
    } else {
        balance_obj.push_back(Pair("balance", FormatIndivisibleMP(nAvailable)));
        balance_obj.push_back(Pair("reserved", FormatIndivisibleMP(nReserved)));
    }

    if (nAvailable == 0 && nReserved == 0) {
        return false;
    } else {
        return true;
    }
}

// generate a list of seed blocks based on the data in LevelDB
Value omni_getseedblocks(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error(
            "omni_getseedblocks startblock endblock\n"
            "\nReturns a list of blocks containing Omni transactions for use in seed block filtering.\n"
            "\nWARNING: The Exodus crowdsale is not stored in LevelDB, thus this is currently only safe to use to generate seed blocks after block 255365."
            "\nArguments:\n"
            "1. startblock           (number, required) the first block to look for Omni transactions (inclusive)\n"
            "2. endblock             (number, required) the last block to look for Omni transactions (inclusive)\n"
            "\nResult:\n"
            "[                     (array of numbers) a list of seed blocks\n"
            "   nnnnnn,              (number) the block height of the seed block\n"
            "   ...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("omni_getseedblocks", "290000 300000")
            + HelpExampleRpc("omni_getseedblocks", "290000, 300000")
        );

    int startHeight = params[0].get_int();
    int endHeight = params[1].get_int();

    RequireHeightInChain(startHeight);
    RequireHeightInChain(endHeight);

    Array response;

    {
        LOCK(cs_tally);
        std::set<int> setSeedBlocks = p_txlistdb->GetSeedBlocks(startHeight, endHeight);
        for (std::set<int>::const_iterator it = setSeedBlocks.begin(); it != setSeedBlocks.end(); ++it) {
            response.push_back(*it);
        }
    }

    return response;
}

// obtain the payload for a transaction
Value omni_getpayload(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "omni_getpayload \"txid\"\n"
            "\nGet the payload for an Omni transaction.\n"
            "\nArguments:\n"
            "1. txid                 (string, required) the hash of the transaction to retrieve payload\n"
            "\nResult:\n"
            "{\n"
            "  \"payload\" : \"payloadmessage\",       (string) the decoded Omni payload message\n"
            "  \"payloadsize\" : n                     (number) the size of the payload\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("omni_getpayload", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
            + HelpExampleRpc("omni_getpayload", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
        );

    uint256 txid = ParseHashV(params[0], "txid");

    CTransaction tx;
    uint256 blockHash;
    if (!GetTransaction(txid, tx, blockHash, true)) {
        PopulateFailure(MP_TX_NOT_FOUND);
    }

    int blockTime = 0;
    int blockHeight = GetHeight();
    if (blockHash != 0) {
        CBlockIndex* pBlockIndex = GetBlockIndex(blockHash);
        if (NULL != pBlockIndex) {
            blockTime = pBlockIndex->nTime;
            blockHeight = pBlockIndex->nHeight;
        }
    }

    CMPTransaction mp_obj;
    int parseRC = ParseTransaction(tx, blockHeight, 0, mp_obj, blockTime);
    if (parseRC < 0) PopulateFailure(MP_TX_IS_NOT_MASTER_PROTOCOL);

    Object payloadObj;
    payloadObj.push_back(Pair("payload", mp_obj.getPayload()));
    payloadObj.push_back(Pair("payloadsize", mp_obj.getPayloadSize()));
    return payloadObj;
}

// determine whether to automatically commit transactions
Value omni_setautocommit(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "omni_setautocommit flag\n"
            "\nSets the global flag that determines whether transactions are automatically committed and broadcast.\n"
            "\nArguments:\n"
            "1. flag                 (boolean, required) the flag\n"
            "\nResult:\n"
            "true|false              (boolean) the updated flag status\n"
            "\nExamples:\n"
            + HelpExampleCli("omni_setautocommit", "false")
            + HelpExampleRpc("omni_setautocommit", "false")
        );

    LOCK(cs_tally);

    autoCommit = params[0].get_bool();
    return autoCommit;
}

// display the tally map & the offer/accept list(s)
Value mscrpc(const Array& params, bool fHelp)
{
    int extra = 0;
    int extra2 = 0, extra3 = 0;

    if (fHelp || params.size() > 3)
        throw runtime_error(
            "mscrpc\n"
            "\nReturns the number of blocks in the longest block chain.\n"
            "\nResult:\n"
            "n    (number) the current block count\n"
            "\nExamples:\n"
            + HelpExampleCli("mscrpc", "")
            + HelpExampleRpc("mscrpc", "")
        );

    if (0 < params.size()) extra = atoi(params[0].get_str());
    if (1 < params.size()) extra2 = atoi(params[1].get_str());
    if (2 < params.size()) extra3 = atoi(params[2].get_str());

    PrintToConsole("%s(extra=%d,extra2=%d,extra3=%d)\n", __FUNCTION__, extra, extra2, extra3);

    bool bDivisible = isPropertyDivisible(extra2);

    // various extra tests
    switch (extra) {
        case 0:
        {
            LOCK(cs_tally);
            int64_t total = 0;
            // display all balances
            for (std::map<std::string, CMPTally>::iterator my_it = mp_tally_map.begin(); my_it != mp_tally_map.end(); ++my_it) {
                PrintToConsole("%34s => ", my_it->first);
                total += (my_it->second).print(extra2, bDivisible);
            }
            PrintToConsole("total for property %d  = %X is %s\n", extra2, extra2, FormatDivisibleMP(total));
            break;
        }
        case 1:
        {
            LOCK(cs_tally);
            // display the whole CMPTxList (leveldb)
            p_txlistdb->printAll();
            p_txlistdb->printStats();
            break;
        }
        case 2:
        {
            LOCK(cs_tally);
            // display smart properties
            _my_sps->printAll();
            break;
        }
        case 3:
        {
            LOCK(cs_tally);
            uint32_t id = 0;
            // for each address display all currencies it holds
            for (std::map<std::string, CMPTally>::iterator my_it = mp_tally_map.begin(); my_it != mp_tally_map.end(); ++my_it) {
                PrintToConsole("%34s => ", my_it->first);
                (my_it->second).print(extra2);
                (my_it->second).init();
                while (0 != (id = (my_it->second).next())) {
                    PrintToConsole("Id: %u=0x%X ", id, id);
                }
                PrintToConsole("\n");
            }
            break;
        }
        case 4:
        {
            LOCK(cs_tally);
            for (CrowdMap::const_iterator it = my_crowds.begin(); it != my_crowds.end(); ++it) {
                (it->second).print(it->first);
            }
            break;
        }
        case 5:
        {
            LOCK(cs_tally);
            PrintToConsole("isMPinBlockRange(%d,%d)=%s\n", extra2, extra3, isMPinBlockRange(extra2, extra3, false) ? "YES" : "NO");
            break;
        }
        case 6:
        {
            LOCK(cs_tally);
            MetaDEx_debug_print(true, true);
            break;
        }
        case 7:
        {
            LOCK(cs_tally);
            // display the whole CMPTradeList (leveldb)
            t_tradelistdb->printAll();
            t_tradelistdb->printStats();
            break;
        }
        case 8:
        {
            LOCK(cs_tally);
            // display the STO receive list
            s_stolistdb->printAll();
            s_stolistdb->printStats();
            break;
        }
        case 9:
        {
            PrintToConsole("Locking cs_tally for %d milliseconds..\n", extra2);
            LOCK(cs_tally);
            MilliSleep(extra2);
            PrintToConsole("Unlocking cs_tally now\n");
            break;
        }
        case 10:
        {
            PrintToConsole("Locking cs_main for %d milliseconds..\n", extra2);
            LOCK(cs_main);
            MilliSleep(extra2);
            PrintToConsole("Unlocking cs_main now\n");
            break;
        }
#ifdef ENABLE_WALLET
        case 11:
        {
            PrintToConsole("Locking pwalletMain->cs_wallet for %d milliseconds..\n", extra2);
            LOCK(pwalletMain->cs_wallet);
            MilliSleep(extra2);
            PrintToConsole("Unlocking pwalletMain->cs_wallet now\n");
            break;
        }
#endif
        default:
            break;
    }

    return GetHeight();
}

// display an MP balance via RPC
Value omni_getbalance(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error(
            "omni_getbalance \"address\" propertyid\n"
            "\nReturns the token balance for a given address and property.\n"
            "\nArguments:\n"
            "1. address              (string, required) the address\n"
            "2. propertyid           (number, required) the property identifier\n"
            "\nResult:\n"
            "{\n"
            "  \"balance\" : \"n.nnnnnnnn\",   (string) the available balance of the address\n"
            "  \"reserved\" : \"n.nnnnnnnn\"   (string) the amount reserved by sell offers and accepts\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("omni_getbalance", "\"1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P\" 1")
            + HelpExampleRpc("omni_getbalance", "\"1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P\", 1")
        );

    std::string address = ParseAddress(params[0]);
    uint32_t propertyId = ParsePropertyId(params[1]);

    RequireExistingProperty(propertyId);

    Object balanceObj;
    BalanceToJSON(address, propertyId, balanceObj, isPropertyDivisible(propertyId));

    return balanceObj;
}

Value omni_sendrawtx(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 5)
        throw runtime_error(
            "omni_sendrawtx \"fromaddress\" \"rawtransaction\" ( \"referenceaddress\" \"redeemaddress\" \"referenceamount\" )\n"
            "\nBroadcasts a raw Omni Layer transaction.\n"
            "\nArguments:\n"
            "1. fromaddress          (string, required) the address to send from\n"
            "2. rawtransaction       (string, required) the hex-encoded raw transaction\n"
            "3. referenceaddress     (string, optional) a reference address (none by default)\n"
            "4. redeemaddress        (string, optional) an address that can spent the transaction dust (sender by default)\n"
            "5. referenceamount      (string, optional) a bitcoin amount that is sent to the receiver (minimal by default)\n"
            "\nResult:\n"
            "\"hash\"                  (string) the hex-encoded transaction hash\n"
            "\nExamples:\n"
            + HelpExampleCli("omni_sendrawtx", "\"1MCHESTptvd2LnNp7wmr2sGTpRomteAkq8\" \"000000000000000100000000017d7840\" \"1EqTta1Rt8ixAA32DuC29oukbsSWU62qAV\"")
            + HelpExampleRpc("omni_sendrawtx", "\"1MCHESTptvd2LnNp7wmr2sGTpRomteAkq8\", \"000000000000000100000000017d7840\", \"1EqTta1Rt8ixAA32DuC29oukbsSWU62qAV\"")
        );

    std::string fromAddress = ParseAddress(params[0]);
    std::vector<unsigned char> data = ParseHexV(params[1], "raw transaction");
    std::string toAddress = (params.size() > 2) ? ParseAddressOrEmpty(params[2]): "";
    std::string redeemAddress = (params.size() > 3) ? ParseAddressOrEmpty(params[3]): "";
    int64_t referenceAmount = (params.size() > 4) ? ParseAmount(params[4], true): 0;

    //some sanity checking of the data supplied?
    uint256 newTX;
    std::string rawHex;
    int result = ClassAgnosticWalletTXBuilder(fromAddress, toAddress, redeemAddress, referenceAmount, data, newTX, rawHex, autoCommit);

    // check error and return the txid (or raw hex depending on autocommit)
    if (result != 0) {
        throw JSONRPCError(result, error_str(result));
    } else {
        if (!autoCommit) {
            return rawHex;
        } else {
            return newTX.GetHex();
        }
    }
}

Value omni_getallbalancesforid(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "omni_getallbalancesforid propertyid\n"
            "\nReturns a list of token balances for a given currency or property identifier.\n"
            "\nArguments:\n"
            "1. propertyid           (number, required) the property identifier\n"
            "\nResult:\n"
            "[                           (array of JSON objects)\n"
            "  {\n"
            "    \"address\" : \"address\",      (string) the address\n"
            "    \"balance\" : \"n.nnnnnnnn\",   (string) the available balance of the address\n"
            "    \"reserved\" : \"n.nnnnnnnn\"   (string) the amount reserved by sell offers and accepts\n"
            "  },\n"
            "  ...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("omni_getallbalancesforid", "1")
            + HelpExampleRpc("omni_getallbalancesforid", "1")
        );

    uint32_t propertyId = ParsePropertyId(params[0]);

    RequireExistingProperty(propertyId);

    Array response;
    bool isDivisible = isPropertyDivisible(propertyId); // we want to check this BEFORE the loop

    LOCK(cs_tally);

    for (std::map<std::string, CMPTally>::iterator it = mp_tally_map.begin(); it != mp_tally_map.end(); ++it) {
        uint32_t id = 0;
        bool includeAddress = false;
        std::string address = it->first;
        (it->second).init();
        while (0 != (id = (it->second).next())) {
            if (id == propertyId) {
                includeAddress = true;
                break;
            }
        }
        if (!includeAddress) {
            continue; // ignore this address, has never transacted in this propertyId
        }
        Object balanceObj;
        balanceObj.push_back(Pair("address", address));
        bool nonEmptyBalance = BalanceToJSON(address, propertyId, balanceObj, isDivisible);

        if (nonEmptyBalance) {
            response.push_back(balanceObj);
        }
    }

    return response;
}

Value omni_getallbalancesforaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "omni_getallbalancesforaddress \"address\"\n"
            "\nReturns a list of all token balances for a given address.\n"
            "\nArguments:\n"
            "1. address              (string, required) the address\n"
            "\nResult:\n"
            "[                           (array of JSON objects)\n"
            "  {\n"
            "    \"propertyid\" : n,           (number) the property identifier\n"
            "    \"balance\" : \"n.nnnnnnnn\",   (string) the available balance of the address\n"
            "    \"reserved\" : \"n.nnnnnnnn\"   (string) the amount reserved by sell offers and accepts\n"
            "  },\n"
            "  ...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("omni_getallbalancesforaddress", "\"1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P\"")
            + HelpExampleRpc("omni_getallbalancesforaddress", "\"1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P\"")
        );

    std::string address = ParseAddress(params[0]);

    Array response;

    LOCK(cs_tally);

    CMPTally* addressTally = getTally(address);

    if (NULL == addressTally) { // addressTally object does not exist
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Address not found");
    }

    addressTally->init();

    uint32_t propertyId = 0;
    while (0 != (propertyId = addressTally->next())) {
        Object balanceObj;
        balanceObj.push_back(Pair("propertyid", (uint64_t) propertyId));
        bool nonEmptyBalance = BalanceToJSON(address, propertyId, balanceObj, isPropertyDivisible(propertyId));

        if (nonEmptyBalance) {
            response.push_back(balanceObj);
        }
    }

    return response;
}

Value omni_getproperty(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "omni_getproperty propertyid\n"
            "\nReturns details for about the tokens or smart property to lookup.\n"
            "\nArguments:\n"
            "1. propertyid           (number, required) the identifier of the tokens or property\n"
            "\nResult:\n"
            "{\n"
            "  \"propertyid\" : n,                (number) the identifier\n"
            "  \"name\" : \"name\",                 (string) the name of the tokens\n"
            "  \"category\" : \"category\",         (string) the category used for the tokens\n"
            "  \"subcategory\" : \"subcategory\",   (string) the subcategory used for the tokens\n"
            "  \"data\" : \"information\",          (string) additional information or a description\n"
            "  \"url\" : \"uri\",                   (string) an URI, for example pointing to a website\n"
            "  \"divisible\" : true|false,        (boolean) whether the tokens are divisible\n"
            "  \"issuer\" : \"address\",            (string) the Bitcoin address of the issuer on record\n"
            "  \"creationtxid\" : \"hash\",         (string) the hex-encoded creation transaction hash\n"
            "  \"fixedissuance\" : true|false,    (boolean) whether the token supply is fixed\n"
            "  \"totaltokens\" : \"n.nnnnnnnn\"     (string) the total number of tokens in existence\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("omni_getproperty", "3")
            + HelpExampleRpc("omni_getproperty", "3")
        );

    uint32_t propertyId = ParsePropertyId(params[0]);

    RequireExistingProperty(propertyId);

    CMPSPInfo::Entry sp;
    {
        LOCK(cs_tally);
        if (!_my_sps->getSP(propertyId, sp)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Property identifier does not exist");
        }
    }
    int64_t nTotalTokens = getTotalTokens(propertyId);
    std::string strCreationHash = sp.txid.GetHex();
    std::string strTotalTokens = FormatMP(propertyId, nTotalTokens);

    Object response;
    response.push_back(Pair("propertyid", (uint64_t) propertyId));
    PropertyToJSON(sp, response); // name, category, subcategory, data, url, divisible
    response.push_back(Pair("issuer", sp.issuer));
    response.push_back(Pair("creationtxid", strCreationHash));
    response.push_back(Pair("fixedissuance", sp.fixed));
    response.push_back(Pair("totaltokens", strTotalTokens));

    return response;
}

Value omni_listproperties(const Array& params, bool fHelp)
{
    if (fHelp)
        throw runtime_error(
            "omni_listproperties\n"
            "\nLists all tokens or smart properties.\n"
            "\nResult:\n"
            "[                                (array of JSON objects)\n"
            "  {\n"
            "    \"propertyid\" : n,                (number) the identifier of the tokens\n"
            "    \"name\" : \"name\",                 (string) the name of the tokens\n"
            "    \"category\" : \"category\",         (string) the category used for the tokens\n"
            "    \"subcategory\" : \"subcategory\",   (string) the subcategory used for the tokens\n"
            "    \"data\" : \"information\",          (string) additional information or a description\n"
            "    \"url\" : \"uri\",                   (string) an URI, for example pointing to a website\n"
            "    \"divisible\" : true|false         (boolean) whether the tokens are divisible\n"
            "  },\n"
            "  ...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("omni_listproperties", "")
            + HelpExampleRpc("omni_listproperties", "")
        );

    Array response;

    LOCK(cs_tally);

    uint32_t nextSPID = _my_sps->peekNextSPID(1);
    for (uint32_t propertyId = 1; propertyId < nextSPID; propertyId++) {
        CMPSPInfo::Entry sp;
        if (_my_sps->getSP(propertyId, sp)) {
            Object propertyObj;
            propertyObj.push_back(Pair("propertyid", (uint64_t) propertyId));
            PropertyToJSON(sp, propertyObj); // name, category, subcategory, data, url, divisible

            response.push_back(propertyObj);
        }
    }

    uint32_t nextTestSPID = _my_sps->peekNextSPID(2);
    for (uint32_t propertyId = TEST_ECO_PROPERTY_1; propertyId < nextTestSPID; propertyId++) {
        CMPSPInfo::Entry sp;
        if (_my_sps->getSP(propertyId, sp)) {
            Object propertyObj;
            propertyObj.push_back(Pair("propertyid", (uint64_t) propertyId));
            PropertyToJSON(sp, propertyObj); // name, category, subcategory, data, url, divisible

            response.push_back(propertyObj);
        }
    }

    return response;
}

Value omni_getcrowdsale(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "omni_getcrowdsale propertyid ( verbose )\n"
            "\nReturns information about a crowdsale.\n"
            "\nArguments:\n"
            "1. propertyid           (number, required) the identifier of the crowdsale\n"
            "2. verbose              (boolean, optional) list crowdsale participants (default: false)\n"
            "\nResult:\n"
            "{\n"
            "  \"propertyid\" : n,                     (number) the identifier of the crowdsale\n"
            "  \"name\" : \"name\",                      (string) the name of the tokens issued via the crowdsale\n"
            "  \"active\" : true|false,                (boolean) whether the crowdsale is still active\n"
            "  \"issuer\" : \"address\",                 (string) the Bitcoin address of the issuer on record\n"
            "  \"propertyiddesired\" : n,              (number) the identifier of the tokens eligible to participate in the crowdsale\n"
            "  \"tokensperunit\" : \"n.nnnnnnnn\",       (string) the amount of tokens granted per unit invested in the crowdsale\n"
            "  \"earlybonus\" : n,                     (number) an early bird bonus for participants in percent per week\n"
            "  \"percenttoissuer\" : n,                (number) a percentage of tokens that will be granted to the issuer\n"
            "  \"starttime\" : nnnnnnnnnn,             (number) the start time of the of the crowdsale as Unix timestamp\n"
            "  \"deadline\" : nnnnnnnnnn,              (number) the deadline of the crowdsale as Unix timestamp\n"
            "  \"amountraised\" : \"n.nnnnnnnn\",        (string) the amount of tokens invested by participants\n"
            "  \"tokensissued\" : \"n.nnnnnnnn\",        (string) the total number of tokens issued via the crowdsale\n"
            "  \"addedissuertokens\" : \"n.nnnnnnnn\",   (string) the amount of tokens granted to the issuer as bonus\n"
            "  \"closedearly\" : true|false,           (boolean) whether the crowdsale ended early (if not active)\n"
            "  \"maxtokens\" : true|false,             (boolean) whether the crowdsale ended early due to reaching the limit of max. issuable tokens (if not active)\n"
            "  \"endedtime\" : nnnnnnnnnn,             (number) the time when the crowdsale ended (if closed early)\n"
            "  \"closetx\" : \"hash\",                   (string) the hex-encoded hash of the transaction that closed the crowdsale (if closed manually)\n"
            "  \"participanttransactions\": [          (array of JSON objects) a list of crowdsale participations (if verbose=true)\n"
            "    {\n"
            "      \"txid\" : \"hash\",                      (string) the hex-encoded hash of participation transaction\n"
            "      \"amountsent\" : \"n.nnnnnnnn\",          (string) the amount of tokens invested by the participant\n"
            "      \"participanttokens\" : \"n.nnnnnnnn\",   (string) the tokens granted to the participant\n"
            "      \"issuertokens\" : \"n.nnnnnnnn\"         (string) the tokens granted to the issuer as bonus\n"
            "    },\n"
            "    ...\n"
            "  ]\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("omni_getcrowdsale", "3 true")
            + HelpExampleRpc("omni_getcrowdsale", "3, true")
        );

    uint32_t propertyId = ParsePropertyId(params[0]);
    bool showVerbose = (params.size() > 1) ? params[1].get_bool() : false;

    RequireExistingProperty(propertyId);
    RequireCrowdsale(propertyId);

    CMPSPInfo::Entry sp;
    {
        LOCK(cs_tally);
        if (!_my_sps->getSP(propertyId, sp)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Property identifier does not exist");
        }
    }

    const uint256& creationHash = sp.txid;

    CTransaction tx;
    uint256 hashBlock;
    if (!GetTransaction(creationHash, tx, hashBlock, true)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available about transaction");
    }

    Object response;
    bool active = isCrowdsaleActive(propertyId);
    std::map<uint256, std::vector<int64_t> > database;

    if (active) {
        bool crowdFound = false;

        LOCK(cs_tally);

        for (CrowdMap::const_iterator it = my_crowds.begin(); it != my_crowds.end(); ++it) {
            const CMPCrowd& crowd = it->second;
            if (propertyId == crowd.getPropertyId()) {
                crowdFound = true;
                database = crowd.getDatabase();
            }
        }
        if (!crowdFound) {
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Crowdsale is flagged active but cannot be retrieved");
        }
    } else {
        database = sp.historicalData;
    }

    int64_t tokensIssued = getTotalTokens(propertyId);
    const std::string& txidClosed = sp.txid_close.GetHex();

    int64_t startTime = -1;
    if (0 != hashBlock && GetBlockIndex(hashBlock)) {
        startTime = GetBlockIndex(hashBlock)->nTime;
    }

    // note the database is already deserialized here and there is minimal performance penalty to iterate recipients to calculate amountRaised
    int64_t amountRaised = 0;
    uint16_t propertyIdType = isPropertyDivisible(propertyId) ? MSC_PROPERTY_TYPE_DIVISIBLE : MSC_PROPERTY_TYPE_INDIVISIBLE;
    uint16_t desiredIdType = isPropertyDivisible(sp.property_desired) ? MSC_PROPERTY_TYPE_DIVISIBLE : MSC_PROPERTY_TYPE_INDIVISIBLE;
    std::map<std::string, Object> sortMap;
    for (std::map<uint256, std::vector<int64_t> >::const_iterator it = database.begin(); it != database.end(); it++) {
        Object participanttx;
        std::string txid = it->first.GetHex();
        amountRaised += it->second.at(0);
        participanttx.push_back(Pair("txid", txid));
        participanttx.push_back(Pair("amountsent", FormatByType(it->second.at(0), desiredIdType)));
        participanttx.push_back(Pair("participanttokens", FormatByType(it->second.at(2), propertyIdType)));
        participanttx.push_back(Pair("issuertokens", FormatByType(it->second.at(3), propertyIdType)));
        std::string sortKey = strprintf("%d-%s", it->second.at(1), txid);
        sortMap.insert(std::make_pair(sortKey, participanttx));
    }

    response.push_back(Pair("propertyid", (uint64_t) propertyId));
    response.push_back(Pair("name", sp.name));
    response.push_back(Pair("active", active));
    response.push_back(Pair("issuer", sp.issuer));
    response.push_back(Pair("propertyiddesired", (uint64_t) sp.property_desired));
    response.push_back(Pair("tokensperunit", FormatMP(propertyId, sp.num_tokens)));
    response.push_back(Pair("earlybonus", sp.early_bird));
    response.push_back(Pair("percenttoissuer", sp.percentage));
    response.push_back(Pair("starttime", startTime));
    response.push_back(Pair("deadline", sp.deadline));
    response.push_back(Pair("amountraised", FormatMP(sp.property_desired, amountRaised)));
    response.push_back(Pair("tokensissued", FormatMP(propertyId, tokensIssued)));
    response.push_back(Pair("addedissuertokens", FormatMP(propertyId, sp.missedTokens)));

    // TODO: return fields every time?
    if (!active) response.push_back(Pair("closedearly", sp.close_early));
    if (!active) response.push_back(Pair("maxtokens", sp.max_tokens));
    if (sp.close_early) response.push_back(Pair("endedtime", sp.timeclosed));
    if (sp.close_early && !sp.max_tokens) response.push_back(Pair("closetx", txidClosed));

    if (showVerbose) {
        Array participanttxs;
        for (std::map<std::string, Object>::iterator it = sortMap.begin(); it != sortMap.end(); ++it) {
            participanttxs.push_back(it->second);
        }
        response.push_back(Pair("participanttransactions", participanttxs));
    }

    return response;
}

Value omni_getactivecrowdsales(const Array& params, bool fHelp)
{
    if (fHelp)
        throw runtime_error(
            "omni_getactivecrowdsales\n"
            "\nLists currently active crowdsales.\n"
            "\nResult:\n"
            "[                                 (array of JSON objects)\n"
            "  {\n"
            "    \"propertyid\" : n,                 (number) the identifier of the crowdsale\n"
            "    \"name\" : \"name\",                  (string) the name of the tokens issued via the crowdsale\n"
            "    \"issuer\" : \"address\",             (string) the Bitcoin address of the issuer on record\n"
            "    \"propertyiddesired\" : n,          (number) the identifier of the tokens eligible to participate in the crowdsale\n"
            "    \"tokensperunit\" : \"n.nnnnnnnn\",   (string) the amount of tokens granted per unit invested in the crowdsale\n"
            "    \"earlybonus\" : n,                 (number) an early bird bonus for participants in percent per week\n"
            "    \"percenttoissuer\" : n,            (number) a percentage of tokens that will be granted to the issuer\n"
            "    \"starttime\" : nnnnnnnnnn,         (number) the start time of the of the crowdsale as Unix timestamp\n"
            "    \"deadline\" : nnnnnnnnnn           (number) the deadline of the crowdsale as Unix timestamp\n"
            "  },\n"
            "  ...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("omni_getactivecrowdsales", "")
            + HelpExampleRpc("omni_getactivecrowdsales", "")
        );

    Array response;

    LOCK2(cs_main, cs_tally);

    for (CrowdMap::const_iterator it = my_crowds.begin(); it != my_crowds.end(); ++it) {
        const CMPCrowd& crowd = it->second;
        uint32_t propertyId = crowd.getPropertyId();

        CMPSPInfo::Entry sp;
        if (!_my_sps->getSP(propertyId, sp)) {
            continue;
        }

        const uint256& creationHash = sp.txid;

        CTransaction tx;
        uint256 hashBlock;
        if (!GetTransaction(creationHash, tx, hashBlock, true)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available about transaction");
        }

        int64_t startTime = -1;
        if (0 != hashBlock && GetBlockIndex(hashBlock)) {
            startTime = GetBlockIndex(hashBlock)->nTime;
        }

        Object responseObj;
        responseObj.push_back(Pair("propertyid", (uint64_t) propertyId));
        responseObj.push_back(Pair("name", sp.name));
        responseObj.push_back(Pair("issuer", sp.issuer));
        responseObj.push_back(Pair("propertyiddesired", (uint64_t) sp.property_desired));
        responseObj.push_back(Pair("tokensperunit", FormatMP(propertyId, sp.num_tokens)));
        responseObj.push_back(Pair("earlybonus", sp.early_bird));
        responseObj.push_back(Pair("percenttoissuer", sp.percentage));
        responseObj.push_back(Pair("starttime", startTime));
        responseObj.push_back(Pair("deadline", sp.deadline));
        response.push_back(responseObj);
    }

    return response;
}

Value omni_getgrants(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "omni_getgrants propertyid\n"
            "\nReturns information about granted and revoked units of managed tokens.\n"
            "\nArguments:\n"
            "1. propertyid           (number, required) the identifier of the managed tokens to lookup\n"
            "\nResult:\n"
            "{\n"
            "  \"propertyid\" : n,               (number) the identifier of the managed tokens\n"
            "  \"name\" : \"name\",                (string) the name of the tokens\n"
            "  \"issuer\" : \"address\",           (string) the Bitcoin address of the issuer on record\n"
            "  \"creationtxid\" : \"hash\",        (string) the hex-encoded creation transaction hash\n"
            "  \"totaltokens\" : \"n.nnnnnnnn\",   (string) the total number of tokens in existence\n"
            "  \"issuances\": [                  (array of JSON objects) a list of the granted and revoked tokens\n"
            "    {\n"
            "      \"txid\" : \"hash\",                (string) the hash of the transaction that granted tokens\n"
            "      \"grant\" : \"n.nnnnnnnn\"          (string) the number of tokens granted by this transaction\n"
            "    },\n"
            "    {\n"
            "      \"txid\" : \"hash\",                (string) the hash of the transaction that revoked tokens\n"
            "      \"grant\" : \"n.nnnnnnnn\"          (string) the number of tokens revoked by this transaction\n"
            "    },\n"
            "    ...\n"
            "  ]\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("omni_getgrants", "31")
            + HelpExampleRpc("omni_getgrants", "31")
        );

    uint32_t propertyId = ParsePropertyId(params[0]);

    RequireExistingProperty(propertyId);
    RequireManagedProperty(propertyId);

    CMPSPInfo::Entry sp;
    {
        LOCK(cs_tally);
        if (false == _my_sps->getSP(propertyId, sp)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Property identifier does not exist");
        }
    }
    Object response;
    const uint256& creationHash = sp.txid;
    int64_t totalTokens = getTotalTokens(propertyId);

    // TODO: sort by height?

    Array issuancetxs;
    std::map<uint256, std::vector<int64_t> >::const_iterator it;
    for (it = sp.historicalData.begin(); it != sp.historicalData.end(); it++) {
        const std::string& txid = it->first.GetHex();
        int64_t grantedTokens = it->second.at(0);
        int64_t revokedTokens = it->second.at(1);

        if (grantedTokens > 0) {
            Object granttx;
            granttx.push_back(Pair("txid", txid));
            granttx.push_back(Pair("grant", FormatMP(propertyId, grantedTokens)));
            issuancetxs.push_back(granttx);
        }

        if (revokedTokens > 0) {
            Object revoketx;
            revoketx.push_back(Pair("txid", txid));
            revoketx.push_back(Pair("revoke", FormatMP(propertyId, revokedTokens)));
            issuancetxs.push_back(revoketx);
        }
    }

    response.push_back(Pair("propertyid", (uint64_t) propertyId));
    response.push_back(Pair("name", sp.name));
    response.push_back(Pair("issuer", sp.issuer));
    response.push_back(Pair("creationtxid", creationHash.GetHex()));
    response.push_back(Pair("totaltokens", FormatMP(propertyId, totalTokens)));
    response.push_back(Pair("issuances", issuancetxs));

    return response;
}

Value omni_getorderbook(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "omni_getorderbook propertyid ( propertyid )\n"
            "\nList active offers on the distributed token exchange.\n"
            "\nArguments:\n"
            "1. propertyid           (number, required) filter orders by property identifier for sale\n"
            "2. propertyid           (number, optional) filter orders by property identifier desired\n"
            "\nResult:\n"
            "[                                              (array of JSON objects)\n"
            "  {\n"
            "    \"address\" : \"address\",                         (string) the Bitcoin address of the trader\n"
            "    \"txid\" : \"hash\",                               (string) the hex-encoded hash of the transaction of the order\n"
            "    \"ecosystem\" : \"main\"|\"test\",                   (string) the ecosytem in which the order was made (if \"cancel-ecosystem\")\n"
            "    \"propertyidforsale\" : n,                       (number) the identifier of the tokens put up for sale\n"
            "    \"propertyidforsaleisdivisible\" : true|false,   (boolean) whether the tokens for sale are divisible\n"
            "    \"amountforsale\" : \"n.nnnnnnnn\",                (string) the amount of tokens initially offered\n"
            "    \"amountremaining\" : \"n.nnnnnnnn\",              (string) the amount of tokens still up for sale\n"
            "    \"propertyiddesired\" : n,                       (number) the identifier of the tokens desired in exchange\n"
            "    \"propertyiddesiredisdivisible\" : true|false,   (boolean) whether the desired tokens are divisible\n"
            "    \"amountdesired\" : \"n.nnnnnnnn\",                (string) the amount of tokens initially desired\n"
            "    \"amounttofill\" : \"n.nnnnnnnn\",                 (string) the amount of tokens still needed to fill the offer completely\n"
            "    \"action\" : n,                                  (number) the action of the transaction: (1) \"trade\", (2) \"cancel-price\", (3) \"cancel-pair\", (4) \"cancel-ecosystem\"\n"
            "    \"block\" : nnnnnn,                              (number) the index of the block that contains the transaction\n"
            "    \"blocktime\" : nnnnnnnnnn                       (number) the timestamp of the block that contains the transaction\n"
            "  },\n"
            "  ...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("omni_getorderbook", "2")
            + HelpExampleRpc("omni_getorderbook", "2")
        );

    bool filterDesired = (params.size() > 1);
    uint32_t propertyIdForSale = ParsePropertyId(params[0]);
    uint32_t propertyIdDesired = 0;

    RequireExistingProperty(propertyIdForSale);

    if (filterDesired) {
        propertyIdDesired = ParsePropertyId(params[1]);

        RequireExistingProperty(propertyIdDesired);
        RequireSameEcosystem(propertyIdForSale, propertyIdDesired);
        RequireDifferentIds(propertyIdForSale, propertyIdDesired);
    }

    std::vector<CMPMetaDEx> vecMetaDexObjects;
    {
        LOCK(cs_tally);
        for (md_PropertiesMap::const_iterator my_it = metadex.begin(); my_it != metadex.end(); ++my_it) {
            const md_PricesMap& prices = my_it->second;
            for (md_PricesMap::const_iterator it = prices.begin(); it != prices.end(); ++it) {
                const md_Set& indexes = it->second;
                for (md_Set::const_iterator it = indexes.begin(); it != indexes.end(); ++it) {
                    const CMPMetaDEx& obj = *it;
                    if (obj.getProperty() != propertyIdForSale) continue;
                    if (!filterDesired || obj.getDesProperty() == propertyIdDesired) vecMetaDexObjects.push_back(obj);
                }
            }
        }
    }

    Array response;
    MetaDexObjectsToJSON(vecMetaDexObjects, response);
    return response;
}

Value omni_gettradehistoryforaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 3)
        throw runtime_error(
            "omni_gettradehistoryforaddress \"address\" ( count propertyid )\n"
            "\nRetrieves the history of orders on the distributed exchange for the supplied address.\n"
            "\nArguments:\n"
            "1. address              (string, required) address to retrieve history for\n"
            "2. count                (number, optional) number of orders to retrieve (default: 10)\n"
            "3. propertyid           (number, optional) filter by property identifier transacted (default: no filter)\n"
            "\nResult:\n"
            "[                                              (array of JSON objects)\n"
            "  {\n"
            "    \"txid\" : \"hash\",                               (string) the hex-encoded hash of the transaction of the order\n"
            "    \"sendingaddress\" : \"address\",                  (string) the Bitcoin address of the trader\n"
            "    \"ismine\" : true|false,                         (boolean) whether the order involes an address in the wallet\n"
            "    \"confirmations\" : nnnnnnnnnn,                  (number) the number of transaction confirmations\n"
            "    \"fee\" : \"n.nnnnnnnn\",                          (string) the transaction fee in bitcoins\n"
            "    \"blocktime\" : nnnnnnnnnn,                      (number) the timestamp of the block that contains the transaction\n"
            "    \"valid\" : true|false,                          (boolean) whether the transaction is valid\n"
            "    \"version\" : n,                                 (number) the transaction version\n"
            "    \"type_int\" : n,                                (number) the transaction type as number\n"
            "    \"type\" : \"type\",                               (string) the transaction type as string\n"
            "    \"propertyidforsale\" : n,                       (number) the identifier of the tokens put up for sale\n"
            "    \"propertyidforsaleisdivisible\" : true|false,   (boolean) whether the tokens for sale are divisible\n"
            "    \"amountforsale\" : \"n.nnnnnnnn\",                (string) the amount of tokens initially offered\n"
            "    \"propertyiddesired\" : n,                       (number) the identifier of the tokens desired in exchange\n"
            "    \"propertyiddesiredisdivisible\" : true|false,   (boolean) whether the desired tokens are divisible\n"
            "    \"amountdesired\" : \"n.nnnnnnnn\",                (string) the amount of tokens initially desired\n"
            "    \"unitprice\" : \"n.nnnnnnnnnnn...\"               (string) the unit price nominated in OMNI or TOMNI\n"
            "    \"status\" : \"status\"                            (string) the status of the order (\"open\", \"cancelled\", \"filled\", ...)\n"
            "    \"canceltxid\" : \"hash\",                         (string) the hash of the transaction that cancelled the order (if cancelled)\n"
            "    \"matches\": [                                   (array of JSON objects) a list of matched orders and executed trades\n"
            "      {\n"
            "        \"txid\" : \"hash\",                               (string) the hash of the transaction that was matched against\n"
            "        \"block\" : nnnnnn,                              (number) the index of the block that contains this transaction\n"
            "        \"address\" : \"address\",                         (string) the Bitcoin address of the other trader\n"
            "        \"amountsold\" : \"n.nnnnnnnn\",                   (string) the number of tokens sold in this trade\n"
            "        \"amountreceived\" : \"n.nnnnnnnn\"                (string) the number of tokens traded in exchange\n"
            "      },\n"
            "      ...\n"
            "    ]\n"
            "  },\n"
            "  ...\n"
            "]\n"
            "\nNote:\n"
            "The documentation only covers the output for a trade, but there are also cancel transactions with different properties.\n"
            "\nExamples:\n"
            + HelpExampleCli("omni_gettradehistoryforaddress", "\"1MCHESTptvd2LnNp7wmr2sGTpRomteAkq8\"")
            + HelpExampleRpc("omni_gettradehistoryforaddress", "\"1MCHESTptvd2LnNp7wmr2sGTpRomteAkq8\"")
        );

    std::string address = ParseAddress(params[0]);
    uint64_t count = (params.size() > 1) ? params[1].get_uint64() : 10;
    uint32_t propertyId = 0;

    if (params.size() > 2) {
        propertyId = ParsePropertyId(params[2]);
        RequireExistingProperty(propertyId);
    }

    // Obtain a sorted vector of txids for the address trade history
    std::vector<uint256> vecTransactions;
    {
        LOCK(cs_tally);
        t_tradelistdb->getTradesForAddress(address, vecTransactions, propertyId);
    }

    // Populate the address trade history into JSON objects until we have processed count transactions
    Array response;
    uint32_t processed = 0;
    for(std::vector<uint256>::iterator it = vecTransactions.begin(); it != vecTransactions.end(); ++it) {
        Object txobj;
        int populateResult = populateRPCTransactionObject(*it, txobj, "", true);
        if (0 == populateResult) {
            response.push_back(txobj);
            processed++;
            if (processed >= count) break;
        }
    }

    return response;
}

Value omni_gettradehistoryforpair(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 3)
        throw runtime_error(
            "omni_gettradehistoryforpair propertyid propertyid ( count )\n"
            "\nRetrieves the history of trades on the distributed token exchange for the specified market.\n"
            "\nArguments:\n"
            "1. propertyid           (number, required) the first side of the traded pair\n"
            "2. propertyid           (number, required) the second side of the traded pair\n"
            "3. count                (number, optional) number of trades to retrieve (default: 10)\n"
            "\nResult:\n"
            "[                                      (array of JSON objects)\n"
            "  {\n"
            "    \"block\" : nnnnnn,                      (number) the index of the block that contains the trade match\n"
            "    \"unitprice\" : \"n.nnnnnnnnnnn...\" ,     (string) the unit price used to execute this trade (received/sold)\n"
            "    \"inverseprice\" : \"n.nnnnnnnnnnn...\",   (string) the inverse unit price (sold/received)\n"
            "    \"sellertxid\" : \"hash\",                 (string) the hash of the transaction of the seller\n"
            "    \"address\" : \"address\",                 (string) the Bitcoin address of the seller\n"
            "    \"amountsold\" : \"n.nnnnnnnn\",           (string) the number of tokens sold in this trade\n"
            "    \"amountreceived\" : \"n.nnnnnnnn\",       (string) the number of tokens traded in exchange\n"
            "    \"matchingtxid\" : \"hash\",               (string) the hash of the transaction that was matched against\n"
            "    \"matchingaddress\" : \"address\"          (string) the Bitcoin address of the other party of this trade\n"
            "  },\n"
            "  ...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("omni_gettradehistoryforpair", "1 12 500")
            + HelpExampleRpc("omni_gettradehistoryforpair", "1, 12, 500")
        );

    // obtain property identifiers for pair & check valid parameters
    uint32_t propertyIdSideA = ParsePropertyId(params[0]);
    uint32_t propertyIdSideB = ParsePropertyId(params[1]);
    uint64_t count = (params.size() > 2) ? params[2].get_uint64() : 10;

    RequireExistingProperty(propertyIdSideA);
    RequireExistingProperty(propertyIdSideB);
    RequireSameEcosystem(propertyIdSideA, propertyIdSideB);
    RequireDifferentIds(propertyIdSideA, propertyIdSideB);

    // request pair trade history from trade db
    Array response;
    LOCK(cs_tally);
    t_tradelistdb->getTradesForPair(propertyIdSideA, propertyIdSideB, response, count);
    return response;
}

Value omni_getactivedexsells(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "omni_getactivedexsells ( address )\n"
            "\nReturns currently active offers on the distributed exchange.\n"
            "\nArguments:\n"
            "1. address              (string, optional) address filter (default: include any)\n"
            "\nResult:\n"
            "[                                   (array of JSON objects)\n"
            "  {\n"
            "    \"txid\" : \"hash\",                    (string) the hash of the transaction of this offer\n"
            "    \"propertyid\" : n,                   (number) the identifier of the tokens for sale\n"
            "    \"seller\" : \"address\",               (string) the Bitcoin address of the seller\n"
            "    \"amountavailable\" : \"n.nnnnnnnn\",   (string) the number of tokens still listed for sale and currently available\n"
            "    \"bitcoindesired\" : \"n.nnnnnnnn\",    (string) the number of bitcoins desired in exchange\n"
            "    \"unitprice\" : \"n.nnnnnnnn\" ,        (string) the unit price (BTC/token)\n"
            "    \"timelimit\" : nn,                   (number) the time limit in blocks a buyer has to pay following a successful accept\n"
            "    \"minimumfee\" : \"n.nnnnnnnn\",        (string) the minimum mining fee a buyer has to pay to accept this offer\n"
            "    \"amountaccepted\" : \"n.nnnnnnnn\",    (string) the number of tokens currently reserved for pending \"accept\" orders\n"
            "    \"accepts\": [                        (array of JSON objects) a list of pending \"accept\" orders\n"
            "      {\n"
            "        \"buyer\" : \"address\",                (string) the Bitcoin address of the buyer\n"
            "        \"block\" : nnnnnn,                   (number) the index of the block that contains the \"accept\" order\n"
            "        \"blocksleft\" : nn,                  (number) the number of blocks left to pay\n"
            "        \"amount\" : \"n.nnnnnnnn\"             (string) the amount of tokens accepted and reserved\n"
            "        \"amounttopay\" : \"n.nnnnnnnn\"        (string) the amount in bitcoins needed finalize the trade\n"
            "      },\n"
            "      ...\n"
            "    ]\n"
            "  },\n"
            "  ...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("omni_getactivedexsells", "")
            + HelpExampleRpc("omni_getactivedexsells", "")
        );

    std::string addressFilter;

    if (params.size() > 0) {
        addressFilter = ParseAddressOrEmpty(params[0]);
    }

    Array response;

    LOCK(cs_tally);

    for (OfferMap::iterator it = my_offers.begin(); it != my_offers.end(); ++it) {
        const CMPOffer& selloffer = it->second;
        const std::string& sellCombo = it->first;
        std::string seller = sellCombo.substr(0, sellCombo.size() - 2);

        // filtering
        if (!addressFilter.empty() && seller != addressFilter) continue;

        std::string txid = selloffer.getHash().GetHex();
        uint32_t propertyId = selloffer.getProperty();
        int64_t minFee = selloffer.getMinFee();
        uint8_t timeLimit = selloffer.getBlockTimeLimit();
        int64_t sellOfferAmount = selloffer.getOfferAmountOriginal(); //badly named - "Original" implies off the wire, but is amended amount
        int64_t sellBitcoinDesired = selloffer.getBTCDesiredOriginal(); //badly named - "Original" implies off the wire, but is amended amount
        int64_t amountAvailable = getMPbalance(seller, propertyId, SELLOFFER_RESERVE);
        int64_t amountAccepted = getMPbalance(seller, propertyId, ACCEPT_RESERVE);

        // TODO: no math, and especially no rounding here (!)
        // TODO: no math, and especially no rounding here (!)
        // TODO: no math, and especially no rounding here (!)

        // calculate unit price and updated amount of bitcoin desired
        double unitPriceFloat = 0.0;
        if ((sellOfferAmount > 0) && (sellBitcoinDesired > 0)) {
            unitPriceFloat = (double) sellBitcoinDesired / (double) sellOfferAmount; // divide by zero protection
        }
        int64_t unitPrice = rounduint64(unitPriceFloat * COIN);
        int64_t bitcoinDesired = calculateDesiredBTC(sellOfferAmount, sellBitcoinDesired, amountAvailable);

        Object responseObj;
        responseObj.push_back(Pair("txid", txid));
        responseObj.push_back(Pair("propertyid", (uint64_t) propertyId));
        responseObj.push_back(Pair("seller", seller));
        responseObj.push_back(Pair("amountavailable", FormatDivisibleMP(amountAvailable)));
        responseObj.push_back(Pair("bitcoindesired", FormatDivisibleMP(bitcoinDesired)));
        responseObj.push_back(Pair("unitprice", FormatDivisibleMP(unitPrice)));
        responseObj.push_back(Pair("timelimit", timeLimit));
        responseObj.push_back(Pair("minimumfee", FormatDivisibleMP(minFee)));

        // display info about accepts related to sell
        responseObj.push_back(Pair("amountaccepted", FormatDivisibleMP(amountAccepted)));
        Array acceptsMatched;
        for (AcceptMap::const_iterator ait = my_accepts.begin(); ait != my_accepts.end(); ++ait) {
            Object matchedAccept;
            const CMPAccept& accept = ait->second;
            const std::string& acceptCombo = ait->first;

            // does this accept match the sell?
            if (accept.getHash() == selloffer.getHash()) {
                // split acceptCombo out to get the buyer address
                std::string buyer = acceptCombo.substr((acceptCombo.find("+") + 1), (acceptCombo.size()-(acceptCombo.find("+") + 1)));
                int blockOfAccept = accept.getAcceptBlock();
                int blocksLeftToPay = (blockOfAccept + selloffer.getBlockTimeLimit()) - GetHeight();
                int64_t amountAccepted = accept.getAcceptAmountRemaining();
                // TODO: don't recalculate!
                int64_t amountToPayInBTC = calculateDesiredBTC(accept.getOfferAmountOriginal(), accept.getBTCDesiredOriginal(), amountAccepted);
                matchedAccept.push_back(Pair("buyer", buyer));
                matchedAccept.push_back(Pair("block", blockOfAccept));
                matchedAccept.push_back(Pair("blocksleft", blocksLeftToPay));
                matchedAccept.push_back(Pair("amount", FormatDivisibleMP(amountAccepted)));
                matchedAccept.push_back(Pair("amounttopay", FormatDivisibleMP(amountToPayInBTC)));
                acceptsMatched.push_back(matchedAccept);
            }
        }
        responseObj.push_back(Pair("accepts", acceptsMatched));

        // add sell object into response array
        response.push_back(responseObj);
    }

    return response;
}

Value omni_listblocktransactions(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "omni_listblocktransactions index\n"
            "\nLists all Omni transactions in a block.\n"
            "\nArguments:\n"
            "1. index                (number, required) the block height or block index\n"
            "\nResult:\n"
            "[                       (array of string)\n"
            "  \"hash\",                 (string) the hash of the transaction\n"
            "  ...\n"
            "]\n"

            "\nExamples:\n"
            + HelpExampleCli("omni_listblocktransactions", "279007")
            + HelpExampleRpc("omni_listblocktransactions", "279007")
        );

    int blockHeight = params[0].get_int();

    RequireHeightInChain(blockHeight);

    // next let's obtain the block for this height
    CBlock block;
    {
        LOCK(cs_main);
        CBlockIndex* pBlockIndex = chainActive[blockHeight];

        if (!ReadBlockFromDisk(block, pBlockIndex)) {
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Failed to read block from disk");
        }
    }

    Array response;

    // now we want to loop through each of the transactions in the block and run against CMPTxList::exists
    // those that return positive add to our response array

    LOCK(cs_tally);

    BOOST_FOREACH(const CTransaction&tx, block.vtx) {
        if (p_txlistdb->exists(tx.GetHash())) {
            // later we can add a verbose flag to decode here, but for now callers can send returned txids into gettransaction_MP
            // add the txid into the response as it's an MP transaction
            response.push_back(tx.GetHash().GetHex());
        }
    }

    return response;
}

Value omni_gettransaction(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "omni_gettransaction \"txid\"\n"
            "\nGet detailed information about an Omni transaction.\n"
            "\nArguments:\n"
            "1. txid                 (string, required) the hash of the transaction to lookup\n"
            "\nResult:\n"
            "{\n"
            "  \"txid\" : \"hash\",                  (string) the hex-encoded hash of the transaction\n"
            "  \"sendingaddress\" : \"address\",     (string) the Bitcoin address of the sender\n"
            "  \"referenceaddress\" : \"address\",   (string) a Bitcoin address used as reference (if any)\n"
            "  \"ismine\" : true|false,            (boolean) whether the transaction involes an address in the wallet\n"
            "  \"confirmations\" : nnnnnnnnnn,     (number) the number of transaction confirmations\n"
            "  \"fee\" : \"n.nnnnnnnn\",             (string) the transaction fee in bitcoins\n"
            "  \"blocktime\" : nnnnnnnnnn,         (number) the timestamp of the block that contains the transaction\n"
            "  \"valid\" : true|false,             (boolean) whether the transaction is valid\n"
            "  \"version\" : n,                    (number) the transaction version\n"
            "  \"type_int\" : n,                   (number) the transaction type as number\n"
            "  \"type\" : \"type\",                  (string) the transaction type as string\n"
            "  [...]                             (mixed) other transaction type specific properties\n"
            "}\n"
            "\nbExamples:\n"
            + HelpExampleCli("omni_gettransaction", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
            + HelpExampleRpc("omni_gettransaction", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
        );

    uint256 hash = ParseHashV(params[0], "txid");

    Object txobj;
    int populateResult = populateRPCTransactionObject(hash, txobj);
    if (populateResult != 0) PopulateFailure(populateResult);

    return txobj;
}

Value omni_listtransactions(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 5)
        throw runtime_error(
            "omni_listtransactions ( \"address\" count skip startblock endblock )\n"
            "\nList wallet transactions, optionally filtered by an address and block boundaries.\n"
            "\nArguments:\n"
            "1. address              (string, optional) address filter (default: \"*\")\n"
            "2. count                (number, optional) show at most n transactions (default: 10)\n"
            "3. skip                 (number, optional) skip the first n transactions (default: 0)\n"
            "4. startblock           (number, optional) first block to begin the search (default: 0)\n"
            "5. endblock             (number, optional) last block to include in the search (default: 999999)\n"
            "\nResult:\n"
            "[                                 (array of JSON objects)\n"
            "  {\n"
            "    \"txid\" : \"hash\",                  (string) the hex-encoded hash of the transaction\n"
            "    \"sendingaddress\" : \"address\",     (string) the Bitcoin address of the sender\n"
            "    \"referenceaddress\" : \"address\",   (string) a Bitcoin address used as reference (if any)\n"
            "    \"ismine\" : true|false,            (boolean) whether the transaction involes an address in the wallet\n"
            "    \"confirmations\" : nnnnnnnnnn,     (number) the number of transaction confirmations\n"
            "    \"fee\" : \"n.nnnnnnnn\",             (string) the transaction fee in bitcoins\n"
            "    \"blocktime\" : nnnnnnnnnn,         (number) the timestamp of the block that contains the transaction\n"
            "    \"valid\" : true|false,             (boolean) whether the transaction is valid\n"
            "    \"version\" : n,                    (number) the transaction version\n"
            "    \"type_int\" : n,                   (number) the transaction type as number\n"
            "    \"type\" : \"type\",                  (string) the transaction type as string\n"
            "    [...]                             (mixed) other transaction type specific properties\n"
            "  },\n"
            "  ...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("omni_listtransactions", "")
            + HelpExampleRpc("omni_listtransactions", "")
        );

    // obtains parameters - default all wallet addresses & last 10 transactions
    std::string addressParam;
    if (params.size() > 0) {
        if (("*" != params[0].get_str()) && ("" != params[0].get_str())) addressParam = params[0].get_str();
    }
    int64_t nCount = 10;
    if (params.size() > 1) nCount = params[1].get_int64();
    if (nCount < 0) throw JSONRPCError(RPC_INVALID_PARAMETER, "Negative count");
    int64_t nFrom = 0;
    if (params.size() > 2) nFrom = params[2].get_int64();
    if (nFrom < 0) throw JSONRPCError(RPC_INVALID_PARAMETER, "Negative from");
    int64_t nStartBlock = 0;
    if (params.size() > 3) nStartBlock = params[3].get_int64();
    if (nStartBlock < 0) throw JSONRPCError(RPC_INVALID_PARAMETER, "Negative start block");
    int64_t nEndBlock = 999999;
    if (params.size() > 4) nEndBlock = params[4].get_int64();
    if (nEndBlock < 0) throw JSONRPCError(RPC_INVALID_PARAMETER, "Negative end block");

    // obtain a sorted list of Omni layer wallet transactions (including STO receipts and pending)
    std::map<std::string,uint256> walletTransactions = FetchWalletOmniTransactions(nFrom+nCount, nStartBlock, nEndBlock);

    // reverse iterate over (now ordered) transactions and populate RPC objects for each one
    Array response;
    for (std::map<std::string,uint256>::reverse_iterator it = walletTransactions.rbegin(); it != walletTransactions.rend(); it++) {
        uint256 txHash = it->second;
        Object txobj;
        int populateResult = populateRPCTransactionObject(txHash, txobj, addressParam);
        if (0 == populateResult) response.push_back(txobj);
    }

    // cut on nFrom and nCount
    if (nFrom > (int)response.size()) nFrom = response.size();
    if ((nFrom + nCount) > (int)response.size()) nCount = response.size() - nFrom;
    Array::iterator first = response.begin();
    std::advance(first, nFrom);
    Array::iterator last = response.begin();
    std::advance(last, nFrom+nCount);
    if (last != response.end()) response.erase(last, response.end());
    if (first != response.begin()) response.erase(response.begin(), first);
    std::reverse(response.begin(), response.end());

    return response;
}

Value omni_listpendingtransactions(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "omni_listpendingtransactions ( \"address\" )\n"
            "\nReturns a list of unconfirmed Omni transactions, pending in the memory pool.\n"
            "\nAn optional filter can be provided to only include transactions which involve the given address.\n"
            "\nNote: the validity of pending transactions is uncertain, and the state of the memory pool may "
            "change at any moment. It is recommended to check transactions after confirmation, and pending "
            "transactions should be considered as invalid.\n"
            "\nArguments:\n"
            "1. address              (string, optional) address filter (default: \"\" for no filter)\n"
            "\nResult:\n"
            "[                                 (array of JSON objects)\n"
            "  {\n"
            "    \"txid\" : \"hash\",                  (string) the hex-encoded hash of the transaction\n"
            "    \"sendingaddress\" : \"address\",     (string) the Bitcoin address of the sender\n"
            "    \"referenceaddress\" : \"address\",   (string) a Bitcoin address used as reference (if any)\n"
            "    \"ismine\" : true|false,            (boolean) whether the transaction involes an address in the wallet\n"
            "    \"fee\" : \"n.nnnnnnnn\",             (string) the transaction fee in bitcoins\n"
            "    \"version\" : n,                    (number) the transaction version\n"
            "    \"type_int\" : n,                   (number) the transaction type as number\n"
            "    \"type\" : \"type\",                  (string) the transaction type as string\n"
            "    [...]                             (mixed) other transaction type specific properties\n"
            "  },\n"
            "  ...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("omni_listpendingtransactions", "")
            + HelpExampleRpc("omni_listpendingtransactions", "")
        );

    std::string filterAddress;
    if (params.size() > 0) {
        filterAddress = ParseAddressOrEmpty(params[0]);
    }

    std::vector<uint256> vTxid;
    mempool.queryHashes(vTxid);

    Array result;
    BOOST_FOREACH(const uint256& hash, vTxid) {
        Object txObj;
        if (populateRPCTransactionObject(hash, txObj, filterAddress) == 0) {
            result.push_back(txObj);
        }
    }

    return result;
}

Value omni_getinfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "omni_getinfo\n"
            "Returns various state information of the client and protocol.\n"
            "\nResult:\n"
            "{\n"
            "  \"omnicoreversion_int\" : xxxxxxx,       (number) client version as integer\n"
            "  \"omnicoreversion\" : \"x.x.x.x-xxx\",     (string) client version\n"
            "  \"mastercoreversion\" : \"x.x.x.x-xxx\",   (string) client version (DEPRECIATED)\n"
            "  \"bitcoincoreversion\" : \"x.x.x\",        (string) Bitcoin Core version\n"
            "  \"commitinfo\" : \"xxxxxxx\",              (string) build commit identifier\n"
            "  \"block\" : nnnnnn,                      (number) index of the last processed block\n"
            "  \"blocktime\" : nnnnnnnnnn,              (number) timestamp of the last processed block\n"
            "  \"blocktransactions\" : nnnn,            (number) Omni transactions found in the last processed block\n"
            "  \"totaltransactions\" : nnnnnnnn,        (number) Omni transactions processed in total\n"
            "  \"alerts\" : [                           (array of JSON objects) active protocol alert (if any)\n"
            "    {\n"
            "      \"alerttypeint\" : n,                    (number) alert type as integer\n"
            "      \"alerttype\" : \"xxx\",                   (string) alert type\n"
            "      \"alertexpiry\" : \"nnnnnnnnnn\",          (string) expiration criteria\n"
            "      \"alertmessage\" : \"xxx\"                 (string) information about the alert\n"
            "    },\n"
            "    ...\n"
            "  ]\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("omni_getinfo", "")
            + HelpExampleRpc("omni_getinfo", "")
        );

    Object infoResponse;

    // provide the mastercore and bitcoin version and if available commit id
    infoResponse.push_back(Pair("omnicoreversion_int", OMNICORE_VERSION));
    infoResponse.push_back(Pair("omnicoreversion", OmniCoreVersion()));
    infoResponse.push_back(Pair("mastercoreversion", OmniCoreVersion()));
    infoResponse.push_back(Pair("bitcoincoreversion", BitcoinCoreVersion()));
    infoResponse.push_back(Pair("commitinfo", BuildCommit()));

    // provide the current block details
    int block = GetHeight();
    int64_t blockTime = GetLatestBlockTime();

    LOCK(cs_tally);

    int blockMPTransactions = p_txlistdb->getMPTransactionCountBlock(block);
    int totalMPTransactions = p_txlistdb->getMPTransactionCountTotal();
    int totalMPTrades = t_tradelistdb->getMPTradeCountTotal();
    infoResponse.push_back(Pair("block", block));
    infoResponse.push_back(Pair("blocktime", blockTime));
    infoResponse.push_back(Pair("blocktransactions", blockMPTransactions));

    // provide the number of trades completed
    infoResponse.push_back(Pair("totaltrades", totalMPTrades));
    // provide the number of transactions parsed
    infoResponse.push_back(Pair("totaltransactions", totalMPTransactions));

    // handle alerts
    Array alerts;
    std::vector<AlertData> omniAlerts = GetOmniCoreAlerts();
    for (std::vector<AlertData>::iterator it = omniAlerts.begin(); it != omniAlerts.end(); it++) {
        AlertData alert = *it;
        Object alertResponse;
        std::string alertTypeStr;
        switch (alert.alert_type) {
            case 1: alertTypeStr = "alertexpiringbyblock";
            break;
            case 2: alertTypeStr = "alertexpiringbyblocktime";
            break;
            case 3: alertTypeStr = "alertexpiringbyclientversion";
            break;
            default: alertTypeStr = "error";
        }
        alertResponse.push_back(Pair("alerttypeint", alert.alert_type));
        alertResponse.push_back(Pair("alerttype", alertTypeStr));
        alertResponse.push_back(Pair("alertexpiry", FormatIndivisibleMP(alert.alert_expiry)));
        alertResponse.push_back(Pair("alertmessage", alert.alert_message));
        alerts.push_back(alertResponse);
    }
    infoResponse.push_back(Pair("alerts", alerts));

    return infoResponse;
}

Value omni_getactivations(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "omni_getactivations\n"
            "Returns pending and completed feature activations.\n"
            "\nResult:\n"
            "{\n"
            "  \"pendingactivations\": [       (array of JSON objects) a list of pending feature activations\n"
            "    {\n"
            "      \"featureid\" : n,              (number) the id of the feature\n"
            "      \"featurename\" : \"xxxxxxxx\",   (string) the name of the feature\n"
            "      \"activationblock\" : n,        (number) the block the feature will be activated\n"
            "      \"minimumversion\" : n          (number) the minimum client version needed to support this feature\n"
            "    },\n"
            "    ...\n"
            "  ]\n"
            "  \"completedactivations\": [     (array of JSON objects) a list of completed feature activations\n"
            "    {\n"
            "      \"featureid\" : n,              (number) the id of the feature\n"
            "      \"featurename\" : \"xxxxxxxx\",   (string) the name of the feature\n"
            "      \"activationblock\" : n,        (number) the block the feature will be activated\n"
            "      \"minimumversion\" : n          (number) the minimum client version needed to support this feature\n"
            "    },\n"
            "    ...\n"
            "  ]\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("omni_getactivations", "")
            + HelpExampleRpc("omni_getactivations", "")
        );

    Object response;

    Array arrayPendingActivations;
    std::vector<FeatureActivation> vecPendingActivations = GetPendingActivations();
    for (std::vector<FeatureActivation>::iterator it = vecPendingActivations.begin(); it != vecPendingActivations.end(); ++it) {
        Object actObj;
        FeatureActivation pendingAct = *it;
        actObj.push_back(Pair("featureid", pendingAct.featureId));
        actObj.push_back(Pair("featurename", pendingAct.featureName));
        actObj.push_back(Pair("activationblock", pendingAct.activationBlock));
        actObj.push_back(Pair("minimumversion", (uint64_t)pendingAct.minClientVersion));
        arrayPendingActivations.push_back(actObj);
    }

    Array arrayCompletedActivations;
    std::vector<FeatureActivation> vecCompletedActivations = GetCompletedActivations();
    for (std::vector<FeatureActivation>::iterator it = vecCompletedActivations.begin(); it != vecCompletedActivations.end(); ++it) {
        Object actObj;
        FeatureActivation completedAct = *it;
        actObj.push_back(Pair("featureid", completedAct.featureId));
        actObj.push_back(Pair("featurename", completedAct.featureName));
        actObj.push_back(Pair("activationblock", completedAct.activationBlock));
        actObj.push_back(Pair("minimumversion", (uint64_t)completedAct.minClientVersion));
        arrayCompletedActivations.push_back(actObj);
    }

    response.push_back(Pair("pendingactivations", arrayPendingActivations));
    response.push_back(Pair("completedactivations", arrayCompletedActivations));

    return response;
}

Value omni_getsto(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "omni_getsto \"txid\" \"recipientfilter\"\n"
            "\nGet information and recipients of a send-to-owners transaction.\n"
            "\nArguments:\n"
            "1. txid                 (string, required) the hash of the transaction to lookup\n"
            "2. recipientfilter      (string, optional) a filter for recipients (wallet by default, \"*\" for all)\n"
            "\nResult:\n"
            "{\n"
            "  \"txid\" : \"hash\",                (string) the hex-encoded hash of the transaction\n"
            "  \"sendingaddress\" : \"address\",   (string) the Bitcoin address of the sender\n"
            "  \"ismine\" : true|false,          (boolean) whether the transaction involes an address in the wallet\n"
            "  \"confirmations\" : nnnnnnnnnn,   (number) the number of transaction confirmations\n"
            "  \"fee\" : \"n.nnnnnnnn\",           (string) the transaction fee in bitcoins\n"
            "  \"blocktime\" : nnnnnnnnnn,       (number) the timestamp of the block that contains the transaction\n"
            "  \"valid\" : true|false,           (boolean) whether the transaction is valid\n"
            "  \"version\" : n,                  (number) the transaction version\n"
            "  \"type_int\" : n,                 (number) the transaction type as number\n"
            "  \"type\" : \"type\",                (string) the transaction type as string\n"
            "  \"propertyid\" : n,               (number) the identifier of sent tokens\n"
            "  \"divisible\" : true|false,       (boolean) whether the sent tokens are divisible\n"
            "  \"amount\" : \"n.nnnnnnnn\",        (string) the number of tokens sent to owners\n"
            "  \"totalstofee\" : \"n.nnnnnnnn\",   (string) the fee paid by the sender, nominated in OMNI or TOMNI\n"
            "  \"recipients\": [                 (array of JSON objects) a list of recipients\n"
            "    {\n"
            "      \"address\" : \"address\",          (string) the Bitcoin address of the recipient\n"
            "      \"amount\" : \"n.nnnnnnnn\"         (string) the number of tokens sent to this recipient\n"
            "    },\n"
            "    ...\n"
            "  ]\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("omni_getsto", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\" \"*\"")
            + HelpExampleRpc("omni_getsto", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\", \"*\"")
        );

    uint256 hash = ParseHashV(params[0], "txid");
    std::string filterAddress;
    if (params.size() > 1) filterAddress = ParseAddressOrWildcard(params[1]);

    Object txobj;
    int populateResult = populateRPCTransactionObject(hash, txobj, "", true, filterAddress);
    if (populateResult != 0) PopulateFailure(populateResult);

    return txobj;
}

Value omni_gettrade(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "omni_gettrade \"txid\"\n"
            "\nGet detailed information and trade matches for orders on the distributed token exchange.\n"
            "\nArguments:\n"
            "1. txid                 (string, required) the hash of the order to lookup\n"
            "\nResult:\n"
            "{\n"
            "  \"txid\" : \"hash\",                               (string) the hex-encoded hash of the transaction of the order\n"
            "  \"sendingaddress\" : \"address\",                  (string) the Bitcoin address of the trader\n"
            "  \"ismine\" : true|false,                         (boolean) whether the order involes an address in the wallet\n"
            "  \"confirmations\" : nnnnnnnnnn,                  (number) the number of transaction confirmations\n"
            "  \"fee\" : \"n.nnnnnnnn\",                          (string) the transaction fee in bitcoins\n"
            "  \"blocktime\" : nnnnnnnnnn,                      (number) the timestamp of the block that contains the transaction\n"
            "  \"valid\" : true|false,                          (boolean) whether the transaction is valid\n"
            "  \"version\" : n,                                 (number) the transaction version\n"
            "  \"type_int\" : n,                                (number) the transaction type as number\n"
            "  \"type\" : \"type\",                               (string) the transaction type as string\n"
            "  \"propertyidforsale\" : n,                       (number) the identifier of the tokens put up for sale\n"
            "  \"propertyidforsaleisdivisible\" : true|false,   (boolean) whether the tokens for sale are divisible\n"
            "  \"amountforsale\" : \"n.nnnnnnnn\",                (string) the amount of tokens initially offered\n"
            "  \"propertyiddesired\" : n,                       (number) the identifier of the tokens desired in exchange\n"
            "  \"propertyiddesiredisdivisible\" : true|false,   (boolean) whether the desired tokens are divisible\n"
            "  \"amountdesired\" : \"n.nnnnnnnn\",                (string) the amount of tokens initially desired\n"
            "  \"unitprice\" : \"n.nnnnnnnnnnn...\"               (string) the unit price nominated in OMNI or TOMNI\n"
            "  \"status\" : \"status\"                            (string) the status of the order (\"open\", \"cancelled\", \"filled\", ...)\n"
            "  \"canceltxid\" : \"hash\",                         (string) the hash of the transaction that cancelled the order (if cancelled)\n"
            "  \"matches\": [                                   (array of JSON objects) a list of matched orders and executed trades\n"
            "    {\n"
            "      \"txid\" : \"hash\",                               (string) the hash of the transaction that was matched against\n"
            "      \"block\" : nnnnnn,                              (number) the index of the block that contains this transaction\n"
            "      \"address\" : \"address\",                         (string) the Bitcoin address of the other trader\n"
            "      \"amountsold\" : \"n.nnnnnnnn\",                   (string) the number of tokens sold in this trade\n"
            "      \"amountreceived\" : \"n.nnnnnnnn\"                (string) the number of tokens traded in exchange\n"
            "    },\n"
            "    ...\n"
            "  ]\n"
            "}\n"
            "\nNote:\n"
            "The documentation only covers the output for a trade, but there are also cancel transactions with different properties.\n"
            "\nExamples:\n"
            + HelpExampleCli("omni_gettrade", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
            + HelpExampleRpc("omni_gettrade", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
        );

    uint256 hash = ParseHashV(params[0], "txid");

    Object txobj;
    int populateResult = populateRPCTransactionObject(hash, txobj, "", true);
    if (populateResult != 0) PopulateFailure(populateResult);

    return txobj;
}

Value omni_getcurrentconsensushash(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "omni_getcurrentconsensushash\n"
            "\nReturns the consensus hash for all balances for the current block.\n"
            "\nResult:\n"
            "{\n"
            "  \"block\" : nnnnnn,          (number) the index of the block this consensus hash applies to\n"
            "  \"blockhash\" : \"hash\",      (string) the hash of the corresponding block\n"
            "  \"consensushash\" : \"hash\"   (string) the consensus hash for the block\n"
            "}\n"

            "\nExamples:\n"
            + HelpExampleCli("omni_getcurrentconsensushash", "")
            + HelpExampleRpc("omni_getcurrentconsensushash", "")
        );

    LOCK(cs_main); // TODO - will this ensure we don't take in a new block in the couple of ms it takes to calculate the consensus hash?

    int block = GetHeight();

    CBlockIndex* pblockindex = chainActive[block];
    uint256 blockHash = pblockindex->GetBlockHash();

    uint256 consensusHash = GetConsensusHash();

    Object response;
    response.push_back(Pair("block", block));
    response.push_back(Pair("blockhash", blockHash.GetHex()));
    response.push_back(Pair("consensushash", consensusHash.GetHex()));

    return response;
}

Value omni_getmetadexhash(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "omni_getmetadexhash propertyId\n"
            "\nReturns a hash of the current state of the MetaDEx (default) or orderbook.\n"
            "\nArguments:\n"
            "1. propertyid                  (number, optional) hash orderbook (only trades selling propertyid)\n"
            "\nResult:\n"
            "{\n"
            "  \"block\" : nnnnnn,          (number) the index of the block this hash applies to\n"
            "  \"blockhash\" : \"hash\",    (string) the hash of the corresponding block\n"
            "  \"propertyid\" : nnnnnn,     (number) the market this hash applies to (or 0 for all markets)\n"
            "  \"metadexhash\" : \"hash\"   (string) the hash for the state of the MetaDEx/orderbook\n"
            "}\n"

            "\nExamples:\n"
            + HelpExampleCli("omni_getmetadexhash", "3")
            + HelpExampleRpc("omni_getmetadexhash", "3")
        );

    LOCK(cs_main);

    uint32_t propertyId = 0;
    if (params.size() > 0) {
        propertyId = ParsePropertyId(params[0]);
        RequireExistingProperty(propertyId);
    }

    int block = GetHeight();
    CBlockIndex* pblockindex = chainActive[block];
    uint256 blockHash = pblockindex->GetBlockHash();

    uint256 metadexHash = GetMetaDExHash(propertyId);

    Object response;
    response.push_back(Pair("block", block));
    response.push_back(Pair("blockhash", blockHash.GetHex()));
    response.push_back(Pair("propertyid", (uint64_t)propertyId));
    response.push_back(Pair("metadexhash", metadexHash.GetHex()));

    return response;
}
