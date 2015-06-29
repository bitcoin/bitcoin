// RPC calls

#include "omnicore/rpc.h"

#include "omnicore/convert.h"
#include "omnicore/createpayload.h"
#include "omnicore/dex.h"
#include "omnicore/errors.h"
#include "omnicore/log.h"
#include "omnicore/mdex.h"
#include "omnicore/notifications.h"
#include "omnicore/omnicore.h"
#include "omnicore/parse_string.h"
#include "omnicore/rpcrequirements.h"
#include "omnicore/rpctx.h"
#include "omnicore/rpcvalues.h"
#include "omnicore/sp.h"
#include "omnicore/tally.h"
#include "omnicore/tx.h"
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
#include "wallet.h"

#include <boost/algorithm/string.hpp>
#include <boost/exception/to_string.hpp>
#include <boost/lexical_cast.hpp>

#include "json/json_spirit_value.h"

#include <stdint.h>

#include <map>
#include <stdexcept>
#include <string>

using boost::algorithm::token_compress_on;
using boost::to_string;

using std::map;
using std::runtime_error;
using std::string;
using std::vector;

using namespace json_spirit;
using namespace mastercore;


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
    // temp commentary - Z rewrite to be more similar to existing calls

    bool propertyIdForSaleIsDivisible = isPropertyDivisible(obj.getProperty());
    bool propertyIdDesiredIsDivisible = isPropertyDivisible(obj.getDesProperty());
    std::string strAmountForSale = FormatMP(propertyIdForSaleIsDivisible, obj.getAmountForSale());
    std::string strAmountRemaining = FormatMP(propertyIdForSaleIsDivisible, obj.getAmountRemaining());
    std::string strAmountDesired = FormatMP(propertyIdDesiredIsDivisible, obj.getAmountDesired());
    // add data to JSON object
    metadex_obj.push_back(Pair("address", obj.getAddr()));
    metadex_obj.push_back(Pair("txid", obj.getHash().GetHex()));
    if (obj.getAction() == 4) metadex_obj.push_back(Pair("ecosystem", isTestEcosystemProperty(obj.getProperty()) ? "Test" : "Main"));
    metadex_obj.push_back(Pair("propertyidforsale", (uint64_t) obj.getProperty()));
    metadex_obj.push_back(Pair("propertyidforsaleisdivisible", propertyIdForSaleIsDivisible));
    metadex_obj.push_back(Pair("amountforsale", strAmountForSale));
    metadex_obj.push_back(Pair("amountremaining", strAmountRemaining));
    metadex_obj.push_back(Pair("propertyiddesired", (uint64_t) obj.getDesProperty()));
    metadex_obj.push_back(Pair("propertyiddesiredisdivisible", propertyIdDesiredIsDivisible));
    metadex_obj.push_back(Pair("amountdesired", strAmountDesired));
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

void BalanceToJSON(const std::string& address, uint32_t property, Object& balance_obj, bool divisible)
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
}

// determine whether to automatically commit transactions
Value setautocommit_OMNI(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "setautocommit flag\n"
            "\nSets the global flag that determines whether transactions are automatically committed and broadcast.\n"
            "\nArguments:\n"
            "1. flag    (boolean, required) the flag\n"
            "\nResult:\n"
            "(boolean) the new flag status\n"
            "\nExamples:\n"
            + HelpExampleCli("setautocommit", "false")
            + HelpExampleRpc("setautocommit", "false")
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
            "n    (numeric) The current block count\n"
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
        case 11:
        {
            PrintToConsole("Locking pwalletMain->cs_wallet for %d milliseconds..\n", extra2);
            LOCK(pwalletMain->cs_wallet);
            MilliSleep(extra2);
            PrintToConsole("Unlocking pwalletMain->cs_wallet now\n");
            break;
        }
        default:
            break;
    }

    return GetHeight();
}

// display an MP balance via RPC
Value getbalance_MP(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error(
            "getbalance_MP \"address\" propertyid\n"
            "\nReturns the token balance for a given address and property.\n"
            "\nArguments:\n"
            "1. address           (string, required) the address\n"
            "2. propertyid        (number, required) the property identifier\n"
            "\nResult:\n"
            "{\n"
            "  \"balance\" : \"x.xxxxxxxx\",   (string) the available balance of the address\n"
            "  \"reserved\" : \"x.xxxxxxxx\"   (string) the amount reserved by sell offers and accepts\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getbalance_MP", "\"1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P\" 1")
            + HelpExampleRpc("getbalance_MP", "\"1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P\", 1")
        );

    std::string address = ParseAddress(params[0]);
    uint32_t propertyId = ParsePropertyId(params[1]);

    RequireExistingProperty(propertyId);

    Object balanceObj;
    BalanceToJSON(address, propertyId, balanceObj, isPropertyDivisible(propertyId));

    return balanceObj;
}

Value sendrawtx_MP(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 5)
        throw runtime_error(
            "sendrawtx_MP \"fromaddress\" \"rawtransaction\" ( \"referenceaddress\" \"redeemaddress\" \"referenceamount\" )\n"
            "\nBroadcasts a raw Omni Layer transaction.\n"
            "\nArguments:\n"
            "1. fromaddress       (string, required) the address to send from\n"
            "2. rawtransaction    (string, required) the hex-encoded raw transaction\n"
            "3. referenceaddress  (string, optional) a reference address (empty by default)\n"
            "4. redeemaddress     (string, optional) an address that can spent the transaction dust (sender by default)\n"
            "5. referenceamount   (string, optional) a bitcoin amount that is sent to the receiver (minimal by default)\n"
            "\nResult:\n"
            "\"hash\"               (string) The hex-encoded transaction hash\n"
            "\nExamples:\n"
            + HelpExampleCli("sendrawtx_MP", "\"1MCHESTptvd2LnNp7wmr2sGTpRomteAkq8\" \"000000000000000100000000017d7840\" \"1EqTta1Rt8ixAA32DuC29oukbsSWU62qAV\"")
            + HelpExampleRpc("sendrawtx_MP", "\"1MCHESTptvd2LnNp7wmr2sGTpRomteAkq8\", \"000000000000000100000000017d7840\", \"1EqTta1Rt8ixAA32DuC29oukbsSWU62qAV\"")
        );

    std::string fromAddress = ParseAddress(params[0]);
    std::vector<unsigned char> data = ParseHexV(params[1], "raw transaction");
    std::string toAddress = (params.size() > 2 && !ParseText(params[2]).empty()) ? ParseAddress(params[2]): "";
    std::string redeemAddress = (params.size() > 3 && !ParseText(params[3]).empty()) ? ParseAddress(params[3]): "";
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

Value getallbalancesforid_MP(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getallbalancesforid_MP propertyid\n"
            "\nReturns a list of token balances for a given currency or property identifier.\n"
            "\nArguments:\n"
            "1. propertyid        (number, required) the property identifier\n"
            "\nResult:\n"
            "[                             (array of JSON objects)\n"
            "  {\n"
            "    \"address\" : \"address\",      (string) the address\n"
            "    \"balance\" : \"x.xxxxxxxx\",   (string) the available balance of the address\n"
            "    \"reserved\" : \"x.xxxxxxxx\"   (string) the amount reserved by sell offers and accepts\n"
            "  },\n"
            "  ...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("getallbalancesforid_MP", "1")
            + HelpExampleRpc("getallbalancesforid_MP", "1")
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
        BalanceToJSON(address, propertyId, balanceObj, isDivisible);

        response.push_back(balanceObj);
    }

    return response;
}

Value getallbalancesforaddress_MP(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getallbalancesforaddress_MP \"address\"\n"
            "\nReturns a list of all token balances for a given address.\n"
            "\nArguments:\n"
            "1. address           (string, required) the address\n"
            "\nResult:\n"
            "[                             (array of JSON objects)\n"
            "  {\n"
            "    \"propertyid\" : x,           (number) the property identifier\n"
            "    \"balance\" : \"x.xxxxxxxx\",   (string) the available balance of the address\n"
            "    \"reserved\" : \"x.xxxxxxxx\"   (string) the amount reserved by sell offers and accepts\n"
            "  },\n"
            "  ...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("getallbalancesforaddress_MP", "1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P")
            + HelpExampleRpc("getallbalancesforaddress_MP", "1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P")
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
        BalanceToJSON(address, propertyId, balanceObj, isPropertyDivisible(propertyId));

        response.push_back(balanceObj);
    }

    return response;
}

Value getproperty_MP(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getproperty_MP propertyid\n"
            "\nReturns details for a property identifier.\n"
            "\nArguments:\n"
            "1. propertyid        (number, required) the property identifier\n"
            "\nResult:\n"
            "{\n"
            "  \"name\" : \"PropertyName\",     (string) the property name\n"
            "  \"category\" : \"PropertyCategory\",     (string) the property category\n"
            "  \"subcategory\" : \"PropertySubCategory\",     (string) the property subcategory\n"
            "  \"data\" : \"PropertyData\",     (string) the property data\n"
            "  \"url\" : \"PropertyURL\",     (string) the property URL\n"
            "  \"divisible\" : false,     (boolean) whether the property is divisible\n"
            "  \"issuer\" : \"1Address\",     (string) the property issuer address\n"
            "  \"issueancetype\" : \"Fixed\",     (string) the property method of issuance\n"
            "  \"totaltokens\" : x     (string) the total number of tokens in existence\n"
            "}\n"
            "\nExamples\n"
            + HelpExampleCli("getproperty_MP", "3")
            + HelpExampleRpc("getproperty_MP", "3")
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

Value listproperties_MP(const Array& params, bool fHelp)
{
    if (fHelp)
        throw runtime_error(
            "listproperties_MP\n"
            "\nLists all smart properties.\n"
            "\nResult:\n"
            "{\n"
            "  \"name\" : \"PropertyName\",     (string) the property name\n"
            "  \"category\" : \"PropertyCategory\",     (string) the property category\n"
            "  \"subcategory\" : \"PropertySubCategory\",     (string) the property subcategory\n"
            "  \"data\" : \"PropertyData\",     (string) the property data\n"
            "  \"url\" : \"PropertyURL\",     (string) the property URL\n"
            "  \"divisible\" : false,     (boolean) whether the property is divisible\n"
            "}\n"
            "\nExamples\n"
            + HelpExampleCli("listproperties_MP", "")
            + HelpExampleRpc("listproperties_MP", "")
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

Value getcrowdsale_MP(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 )
        throw runtime_error(
            "getcrowdsale_MP propertyid\n"
            "\nGet information about a crowdsale for a property identifier.\n"
            "\nArguments:\n"
            "1. propertyid        (number, required) the property identifier\n"
            "\nResult:\n"
            "{\n"
            "  \"name\" : \"PropertyName\",     (string) the property name\n"
            "  \"active\" : false,     (boolean) whether the crowdsale is active\n"
            "  \"issuer\" : \"1Address\",     (string) the issuer address\n"
            "  \"creationtxid\" : \"txid\",     (string) the transaction that created the crowdsale\n"
            "  \"propertyiddesired\" : x,     (number) the property identifier desired\n"
            "  \"tokensperunit\" : x,     (number) the number of tokens awarded per unit\n"
            "  \"earlybonus\" : x,     (number) the percentage per week early bonus applied\n"
            "  \"percenttoissuer\" : x,     (number) the percentage awarded to the issuer\n"
            "  \"starttime\" : xxx,     (number) the start time of the crowdsale\n"
            "  \"deadline\" : xxx,     (number) the time the crowdsale will automatically end\n"
            "  \"closedearly\" : false,     (boolean) whether the crowdsale was ended early\n"
            "  \"maxtokens\" : false,     (boolean) whether the crowdsale was ended early due to maxing the token count\n"
            "  \"endedtime\" : xxx,     (number) the time the crowdsale ended\n"
            "  \"closetx\" : \"txid\",     (string) the transaction that closed the crowdsale\n"
            "}\n"
            "\nExamples\n"
            + HelpExampleCli("getcrowdsale_MP", "3")
            + HelpExampleRpc("getcrowdsale_MP", "3")
        );

    uint32_t propertyId = ParsePropertyId(params[0]);

    RequireExistingProperty(propertyId);
    RequireCrowdsale(propertyId);

    LOCK(cs_tally);

    CMPSPInfo::Entry sp;
    if (!_my_sps->getSP(propertyId, sp)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Property identifier does not exist");
    }

    bool showVerbose = false;
    if (params.size() > 1) showVerbose = params[1].get_bool();

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

    int64_t amountRaised = 0;
    int64_t tokensIssued = getTotalTokens(propertyId);
    const std::string& txidClosed = sp.txid_close.GetHex();

    int64_t startTime = -1;
    if (0 != hashBlock && GetBlockIndex(hashBlock)) {
        startTime = GetBlockIndex(hashBlock)->nTime;
    }

    Array participanttxs;
    std::map<uint256, std::vector<int64_t> >::const_iterator it;
    for (it = database.begin(); it != database.end(); it++) {
        Object participanttx;

        const std::string& txid = it->first.GetHex();
        int64_t userTokens = it->second.at(2);
        int64_t issuerTokens = it->second.at(3);
        int64_t amountSent = it->second.at(0);

        amountRaised += amountSent;
        participanttx.push_back(Pair("txid", txid));
        participanttx.push_back(Pair("amountsent", FormatMP(sp.property_desired, amountSent)));
        participanttx.push_back(Pair("participanttokens", FormatMP(propertyId, userTokens)));
        participanttx.push_back(Pair("issuertokens", FormatMP(propertyId, issuerTokens)));
        participanttxs.push_back(participanttx);
    }

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

    // array of txids contributing to crowdsale here if needed
    if (showVerbose) {
        response.push_back(Pair("participanttransactions", participanttxs));
    }

    return response;
}

Value getactivecrowdsales_MP(const Array& params, bool fHelp)
{
    if (fHelp)
        throw runtime_error(
            "getactivecrowdsales_MP\n"
            "\nGet active crowdsales.\n"
            "\nResult:\n"
            "{\n"
            "  \"name\" : \"PropertyName\",     (string) the property name\n"
            "  \"issuer\" : \"1Address\",     (string) the issuer address\n"
            "  \"creationtxid\" : \"txid\",     (string) the transaction that created the crowdsale\n"
            "  \"propertyiddesired\" : x,     (number) the property identifier desired\n"
            "  \"tokensperunit\" : x,     (number) the number of tokens awarded per unit\n"
            "  \"earlybonus\" : x,     (number) the percentage per week early bonus applied\n"
            "  \"percenttoissuer\" : x,     (number) the percentage awarded to the issuer\n"
            "  \"starttime\" : xxx,     (number) the start time of the crowdsale\n"
            "  \"deadline\" : xxx,     (number) the time the crowdsale will automatically end\n"
            "}\n"
            "\nExamples\n"
            + HelpExampleCli("getactivecrowdsales_MP", "")
            + HelpExampleRpc("getactivecrowdsales_MP", "")
        );

    Array response;

    LOCK(cs_tally);

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

Value getgrants_MP(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getgrants_MP propertyid\n"
            "\nGet information about grants and revokes for a property identifier.\n"
            "\nArguments:\n"
            "1. propertyid        (number, required) the property identifier\n"
            "\nResult:\n"
            "{\n"
            "  \"name\" : \"PropertyName\",   (string) the property name\n"
            "  \"issuer\" : \"1Address\",     (string) the issuer address\n"
            "  \"creationtxid\" : \"txid\",   (string) the transaction that created the crowdsale\n"
            "  \"totaltokens\" : \"1234\",    (string) the number of tokens in circulation\n"
            "  \"issuances\" : [\n"
            "                    {\n"
            "                      \"txid\" : \"txid\",   (string) the transaction that granted/revoked tokens\n"
            "                      \"grant\" : \"1234\",  (string) the number of tokens granted\n"
            "                    },\n"
            "                    {\n"
            "                      \"txid\" : \"txid\",   (string) the transaction that granted/revoked tokens\n"
            "                      \"revoke\" : \"1234\", (string) the number of tokens revoked\n"
            "                    },\n"
            "                    ...\n"
            "                  ]\n"
            "}\n"
            "\nExamples\n"
            + HelpExampleCli("getgrants_MP", "31")
            + HelpExampleRpc("getgrants_MP", "31")
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

    response.push_back(Pair("name", sp.name));
    response.push_back(Pair("issuer", sp.issuer));
    response.push_back(Pair("creationtxid", creationHash.GetHex()));
    response.push_back(Pair("totaltokens", FormatMP(propertyId, totalTokens)));
    response.push_back(Pair("issuances", issuancetxs));

    return response;
}

int check_prop_valid(int64_t tmpPropId, string error, string exist_error ) {
  CMPSPInfo::Entry sp;
  LOCK(cs_tally);

  if ((1 > tmpPropId) || (4294967295LL < tmpPropId))
    throw JSONRPCError(RPC_INVALID_PARAMETER, error);
  if (false == _my_sps->getSP(tmpPropId, sp)) 
    throw JSONRPCError(RPC_INVALID_PARAMETER, exist_error);

  return tmpPropId;
}

Value getorderbook_MP(const Array& params, bool fHelp)
{
   if (fHelp || params.size() < 1)
        throw runtime_error(
            "getorderbook_MP property_id1 ( property_id2 )\n"
            "\nAllows user to request active order information from the order book\n"
            
            "\nArguments:\n"
            "1. property_id1            (int, required) amount owned to up on sale\n"
            "2. property_id2         (int, optional) property owned to put up on sale\n"
        );

  unsigned int propertyIdSaleFilter = 0, propertyIdWantFilter = 0;

  bool filter_by_desired = (params.size() == 2) ? true : false;

  propertyIdSaleFilter = check_prop_valid( params[0].get_int64() , "Invalid property identifier (Sale)", "Property identifier does not exist (Sale)"); 

  if ( filter_by_desired ) {
    propertyIdWantFilter = check_prop_valid( params[1].get_int64() , "Invalid property identifier (Want)", "Property identifier does not exist (Want)"); 
  }

  //for each address
  //get property pair and total order amount at a price
  std::vector<CMPMetaDEx> vMetaDexObjects;

  {
  LOCK(cs_tally);

  for (md_PropertiesMap::iterator my_it = metadex.begin(); my_it != metadex.end(); ++my_it)
  {
    md_PricesMap & prices = my_it->second;
    for (md_PricesMap::iterator it = prices.begin(); it != prices.end(); ++it)
    {
      md_Set & indexes = (it->second);
      for (md_Set::iterator it = indexes.begin(); it != indexes.end(); ++it)
      {
        CMPMetaDEx obj = *it;

        //this filter, the first part is filtering by two currencies, the second part is filtering by the first only
        bool filter = ( filter_by_desired && ( obj.getProperty() == propertyIdSaleFilter ) && ( obj.getDesProperty() == propertyIdWantFilter ) ) || ( !filter_by_desired && ( obj.getProperty() == propertyIdSaleFilter ) );

        if ( filter  ) {
            vMetaDexObjects.push_back(obj);
        }
      }
    }
  }

  }
  
  Array response;
  MetaDexObjectsToJSON(vMetaDexObjects, response);
  
  return response;
}
 
Value gettradessince_MP(const Array& params, bool fHelp)
{
   if (fHelp)
        throw runtime_error(
            "gettradessince_MP\n"
            "\nAllows user to request last known orders from order book\n"
            
            "\nArguments:\n"
            "1. timestamp               (int, optional, default=[" + strprintf("%s",GetLatestBlockTime() - 1209600) + "]) starting from the timestamp, orders to show"
            "2. property_id1            (int, optional) filter orders by property_id1 on either side of the trade \n"
            "3. property_id2            (int, optional) filter orders by property_id1 and property_id2\n"
        );

  unsigned int propertyIdSaleFilter = 0, propertyIdWantFilter = 0;

  int64_t timestamp = (params.size() > 0) ? params[0].get_int64() : GetLatestBlockTime() - 1209600; // 2 weeks

  bool filter_by_one = (params.size() > 1) ? true : false;
  bool filter_by_both = (params.size() > 2) ? true : false;

  if( filter_by_one ) {
    propertyIdSaleFilter = check_prop_valid( params[1].get_int64() , "Invalid property identifier (Sale)", "Property identifier does not exist (Sale)"); 
  }

  if ( filter_by_both ) {
    propertyIdWantFilter = check_prop_valid( params[2].get_int64() , "Invalid property identifier (Want)", "Property identifier does not exist (Want)"); 
  }
  
  std::vector<CMPMetaDEx> vMetaDexObjects;

  {
  LOCK(cs_tally);

  for (md_PropertiesMap::iterator my_it = metadex.begin(); my_it != metadex.end(); ++my_it)
  {
    md_PricesMap & prices = my_it->second;
    for (md_PricesMap::iterator it = prices.begin(); it != prices.end(); ++it)
    {
      md_Set & indexes = (it->second);
      for (md_Set::iterator it = indexes.begin(); it != indexes.end(); ++it)
      {
          CMPMetaDEx obj = *it;

          bool filter = 1;

          if( filter_by_one || filter_by_both ) {
          //this filter, the first part is filtering by two currencies, the second part is filtering by the first only
          filter = ( filter_by_both && ( obj.getProperty() == propertyIdSaleFilter ) && ( obj.getDesProperty() == propertyIdWantFilter ) ) || ( !filter_by_both && ( obj.getProperty() == propertyIdSaleFilter ) );
          }

          if ( filter &&  (obj.getBlockTime() >= timestamp)) {
            vMetaDexObjects.push_back(obj);
          }
      }
    }
  }

  }

  Array response;
  MetaDexObjectsToJSON(vMetaDexObjects, response);
  
  return response;
}

Value getopenorders_MP(const Array& params, bool fHelp)
{
   if (fHelp)
        throw runtime_error(
            "getorderbook_MP\n"
            "\nAllows user to request active order information from the order book\n"
            
            "\nArguments:\n"
            "1. property_id1            (int, required) amount owned to up on sale\n"
            "2. property_id2         (int, optional) property owned to put up on sale\n"
            
            "\nResult:\n"
            "[                (array of string)\n"
            "  \"hash\"         (string) Transaction id\n"            
            "  ,...\n"
            "]\n"

            "\nbExamples\n"
            //+ HelpExampleCli("trade_MP", "50", "3", "500", "5" )
            //+ HelpExampleRpc("trade_MP", "50", "3", "500", "5" )
        );
  return "\nNot Implemented";
}

Value gettradehistory_MP(const Array& params, bool fHelp)
{
   if (fHelp || params.size() < 1)
        throw runtime_error(
            "gettradehistory_MP\n"
            "\nAllows user to retreive trade history for some address on the Metadex\n"
            
            "\nArguments:\n"
            "1. address          (string, required) address to query history on\n"
            "2. number            (int, optional ) number of trades to retreive\n"
            "3. property_id         (int, optional) filter by propertyid on one side\n"
        );

  string address = params[0].get_str();
  unsigned int number_trades = (params.size() == 2 ? (unsigned int) params[1].get_int64() : 512);
  unsigned int propertyIdFilter = 0;

  bool filter_by_one = (params.size() == 3) ? true : false;

  if( filter_by_one ) {
    propertyIdFilter = check_prop_valid( params[2].get_int64() , "Invalid property identifier (Sale)", "Property identifier does not exist (Sale)"); 
  }
  
  std::vector<CMPMetaDEx> vMetaDexObjects;

  {
  LOCK(cs_tally);

  for (md_PropertiesMap::iterator my_it = metadex.begin(); my_it != metadex.end(); ++my_it)
  {
    md_PricesMap & prices = my_it->second;
    for (md_PricesMap::iterator it = prices.begin(); it != prices.end(); ++it)
    {
      md_Set & indexes = (it->second);
      for (md_Set::iterator it = indexes.begin(); it != indexes.end(); ++it)
      {
          CMPMetaDEx obj = *it;

          bool filter = 1;

          //this filter, the first part is filtering by two currencies, the second part is filtering by the first only
          filter = (obj.getAddr() == address) && 
                   ( ( ( filter_by_one && (obj.getProperty() == propertyIdFilter) == 1 ) ) || !filter_by_one ) && 
                   (vMetaDexObjects.size() < number_trades);
          if ( filter ) {
            vMetaDexObjects.push_back(obj);
          }
      }
    }
  }

  }

  Array response;
  MetaDexObjectsToJSON(vMetaDexObjects, response);
  
  return response;
}

Value getactivedexsells_MP(const Array& params, bool fHelp)
{
   if (fHelp)
        throw runtime_error(
            "getactivedexsells_MP\n"
            "\nReturns currently active offers on the distributed exchange.\n"
            "\nResult:\n"
            "[                             (array of JSON objects)\n"
            "  {\n"
            "  },\n"
            "  ...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("getactivedexsells_MP", "")
            + HelpExampleRpc("getactivedexsells_MP", "")
        );

      //if 0 params list all sells, otherwise first param is filter address
      bool addressFilter = false;
      string addressParam;

      if (params.size() > 0)
      {
          addressParam = params[0].get_str();
          addressFilter = true;
      }

      Array response;

      LOCK(cs_tally);

      for(OfferMap::iterator it = my_offers.begin(); it != my_offers.end(); ++it)
      {
          CMPOffer selloffer = it->second;
          string sellCombo = it->first;
          string seller = sellCombo.substr(0, sellCombo.size()-2);

          //filtering
          if ((addressFilter) && (seller != addressParam)) continue;

          uint256 sellHash = selloffer.getHash();
          string txid = sellHash.GetHex();
          uint64_t propertyId = selloffer.getProperty();
          uint64_t minFee = selloffer.getMinFee();
          unsigned char timeLimit = selloffer.getBlockTimeLimit();
          uint64_t sellOfferAmount = selloffer.getOfferAmountOriginal(); //badly named - "Original" implies off the wire, but is amended amount
          uint64_t sellBitcoinDesired = selloffer.getBTCDesiredOriginal(); //badly named - "Original" implies off the wire, but is amended amount
          uint64_t amountAvailable = getMPbalance(seller, propertyId, SELLOFFER_RESERVE);
          uint64_t amountAccepted = getMPbalance(seller, propertyId, ACCEPT_RESERVE);

          //unit price & updated bitcoin desired calcs
          double unitPriceFloat = 0;
          if ((sellOfferAmount>0) && (sellBitcoinDesired > 0)) unitPriceFloat = (double)sellBitcoinDesired/(double)sellOfferAmount; //divide by zero protection
          uint64_t unitPrice = rounduint64(unitPriceFloat * COIN);
          uint64_t bitcoinDesired = rounduint64(amountAvailable*unitPriceFloat);

          Object responseObj;

          responseObj.push_back(Pair("txid", txid));
          responseObj.push_back(Pair("propertyid", propertyId));
          responseObj.push_back(Pair("seller", seller));
          responseObj.push_back(Pair("amountavailable", FormatDivisibleMP(amountAvailable)));
          responseObj.push_back(Pair("bitcoindesired", FormatDivisibleMP(bitcoinDesired)));
          responseObj.push_back(Pair("unitprice", FormatDivisibleMP(unitPrice)));
          responseObj.push_back(Pair("timelimit", timeLimit));
          responseObj.push_back(Pair("minimumfee", FormatDivisibleMP(minFee)));

          // display info about accepts related to sell
          responseObj.push_back(Pair("amountaccepted", FormatDivisibleMP(amountAccepted)));
          Array acceptsMatched;
          for(AcceptMap::iterator ait = my_accepts.begin(); ait != my_accepts.end(); ++ait)
          {
              Object matchedAccept;

              CMPAccept accept = ait->second;
              string acceptCombo = ait->first;
              uint256 matchedHash = accept.getHash();
              // does this accept match the sell?
              if (matchedHash == sellHash)
              {
                  //split acceptCombo out to get the buyer address
                  string buyer = acceptCombo.substr((acceptCombo.find("+")+1),(acceptCombo.size()-(acceptCombo.find("+")+1)));
                  uint64_t acceptBlock = accept.getAcceptBlock();
                  uint64_t acceptAmount = accept.getAcceptAmountRemaining();
                  matchedAccept.push_back(Pair("buyer", buyer));
                  matchedAccept.push_back(Pair("block", acceptBlock));
                  matchedAccept.push_back(Pair("amount", FormatDivisibleMP(acceptAmount)));
                  acceptsMatched.push_back(matchedAccept);
              }
          }
          responseObj.push_back(Pair("accepts", acceptsMatched));

          // add sell object into response array
          response.push_back(responseObj);
      }

return response;
}

Value listblocktransactions_MP(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "listblocktransactions_MP index\n"
            "\nReturns all Master Protocol transactions in a block.\n"
            "\nArguments:\n"
            "1. index         (numeric, required) The block height or index\n"
            "\nResult:\n"
            "[                (array of string)\n"
            "  \"hash\"         (string) Transaction id\n"
            "  ,...\n"
            "]\n"

            "\nExamples\n"
            + HelpExampleCli("listblocktransactions_MP", "279007")
            + HelpExampleRpc("listblocktransactions_MP", "279007")
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

// this function standardizes the RPC output for gettransaction_MP and listtransaction_MP into a central function
int populateRPCTransactionObject(const uint256& txid, Object *txobj, std::string filterAddress = "")
{
    //uint256 hash;
    //hash.SetHex(params[0].get_str());

    CTransaction wtx;
    uint256 blockHash = 0;
    if (!GetTransaction(txid, wtx, blockHash, true)) { return MP_TX_NOT_FOUND; }

    CMPTransaction mp_obj;
    uint256 wtxid = txid;
    bool bIsMine = false;
    bool isMPTx = false;
    uint64_t nFee = 0;
    string MPTxType;
    unsigned int MPTxTypeInt;
    string selectedAddress;
    string senderAddress;
    string refAddress;
    bool valid = false;
    bool showReference = false;
    uint64_t propertyId = 0;  //using 64 instead of 32 here as json::sprint chokes on 32 - research why
    bool divisible = false;
    uint64_t amount = 0;
    string result;
    uint64_t sell_minfee = 0;
    unsigned char sell_timelimit = 0;
    unsigned char sell_subaction = 0;
    uint64_t sell_btcdesired = 0;
    int rc=0;
    bool crowdPurchase = false;
    int64_t crowdPropertyId = 0;
    int64_t crowdTokens = 0;
    int64_t issuerTokens = 0;
    bool crowdDivisible = false;
    string crowdName;
    string propertyName;
    bool mdex_propertyId_Div = false;
    uint64_t mdex_propertyWanted = 0;
    uint64_t mdex_amountWanted = 0;
    uint64_t mdex_amountRemaining = 0;
    bool mdex_propertyWanted_Div = false;
    unsigned int mdex_action = 0;
    string mdex_unitPriceStr;
    string mdex_actionStr;

    if (0 == blockHash) { return MP_TX_UNCONFIRMED; }

    CBlockIndex* pBlockIndex = GetBlockIndex(blockHash);
    if (NULL == pBlockIndex) { return MP_BLOCK_NOT_IN_CHAIN; }

    int blockHeight = pBlockIndex->nHeight;
    int confirmations =  1 + GetHeight() - pBlockIndex->nHeight;
    int64_t blockTime = pBlockIndex->nTime;

    mp_obj.SetNull();
    CMPOffer temp_offer;
    CMPMetaDEx temp_metadexoffer;

    // replace initial MP detection with levelDB lookup instead of parse, this is much faster especially in calls like list/search
    bool fExists = false;
    {
        LOCK(cs_tally);
        fExists = p_txlistdb->exists(wtxid);
    }
    if (fExists)
    {
        //transaction is in levelDB, so we can attempt to parse it
        int parseRC = ParseTransaction(wtx, blockHeight, 0, mp_obj);
        if (0 <= parseRC) //negative RC means no MP content/badly encoded TX, we shouldn't see this if TX in levelDB but check for sanity
        {
            // do we have a non-zero RC, if so it's a payment, handle differently
            if (0 < parseRC)
            {
                // handle as payment TX - this doesn't fit nicely into the kind of output for a MP tx so we'll do this seperately
                // add generic TX data to the output
                //Object txobj;
                txobj->push_back(Pair("txid", wtxid.GetHex()));
                txobj->push_back(Pair("confirmations", confirmations));
                txobj->push_back(Pair("blocktime", blockTime));
                txobj->push_back(Pair("type", "DEx Purchase"));
                // always min one subrecord, grab sender from first sub so we can display in parent as per Adam feedback
                string tmpBuyer;
                string tmpSeller;
                uint64_t tmpVout;
                uint64_t tmpNValue;
                uint64_t tmpPropertyId;
                {
                    LOCK(cs_tally);
                    p_txlistdb->getPurchaseDetails(wtxid,1,&tmpBuyer,&tmpSeller,&tmpVout,&tmpPropertyId,&tmpNValue);
                }
                txobj->push_back(Pair("sendingaddress", tmpBuyer));
                // get the details of sub records for payment(s) in the tx and push into an array
                Array purchases;
                int numberOfPurchases = 0;
                {
                    LOCK(cs_tally);
                    numberOfPurchases = p_txlistdb->getNumberOfPurchases(wtxid);
                }
                if (0<numberOfPurchases)
                {
                    for(int purchaseNumber = 1; purchaseNumber <= numberOfPurchases; purchaseNumber++)
                    {
                        Object purchaseObj;
                        string buyer;
                        string seller;
                        uint64_t vout;
                        uint64_t nValue;
                        {
                            LOCK(cs_tally);
                            p_txlistdb->getPurchaseDetails(wtxid,purchaseNumber,&buyer,&seller,&vout,&propertyId,&nValue);
                        }
                        bIsMine = false;
                        bIsMine = IsMyAddress(buyer);
                        if (!bIsMine)
                        {
                            bIsMine = IsMyAddress(seller);
                        }
                        if (!filterAddress.empty()) if ((buyer != filterAddress) && (seller != filterAddress)) return -1; // return negative rc if filtering & no match
                        uint64_t amountPaid = wtx.vout[vout].nValue;
                        purchaseObj.push_back(Pair("vout", vout));
                        purchaseObj.push_back(Pair("amountpaid", FormatDivisibleMP(amountPaid)));
                        purchaseObj.push_back(Pair("ismine", bIsMine));
                        purchaseObj.push_back(Pair("referenceaddress", seller));
                        purchaseObj.push_back(Pair("propertyid", propertyId));
                        purchaseObj.push_back(Pair("amountbought", FormatDivisibleMP(nValue)));
                        purchaseObj.push_back(Pair("valid", true)); //only valid purchases are stored, anything else is regular BTC tx
                        purchases.push_back(purchaseObj);
                    }
                }
                txobj->push_back(Pair("purchases", purchases));
                // return the object
                return 0;
            }
            else
            {
                // otherwise RC was 0, a valid MP transaction so far
                if (0<=mp_obj.step1())
                {
                    MPTxType = mp_obj.getTypeString();
                    MPTxTypeInt = mp_obj.getType();
                    senderAddress = mp_obj.getSender();
                    refAddress = mp_obj.getReceiver();
                    if (!filterAddress.empty()) if ((senderAddress != filterAddress) && (refAddress != filterAddress)) return -1; // return negative rc if filtering & no match
                    isMPTx = true;
                    nFee = mp_obj.getFeePaid();
                    int tmpblock=0;
                    uint32_t tmptype=0;
                    uint64_t amountNew=0;
                    {
                        LOCK(cs_tally);
                        valid=getValidMPTX(wtxid, &tmpblock, &tmptype, &amountNew);
                    }
                    //populate based on type of tx
                    switch (MPTxTypeInt)
                    {
                        case MSC_TYPE_METADEX_TRADE:
                        case MSC_TYPE_METADEX_CANCEL_PRICE:
                        case MSC_TYPE_METADEX_CANCEL_PAIR:
                        case MSC_TYPE_METADEX_CANCEL_ECOSYSTEM:
                             if (0 == mp_obj.step2_Value())
                             {
                                 propertyId = mp_obj.getProperty();
                                 amount = mp_obj.getAmount();
                                 mdex_propertyId_Div = isPropertyDivisible(propertyId);
                                 if (0 <= mp_obj.interpretPacket(NULL,&temp_metadexoffer))
                                 {
                                     mdex_propertyWanted = temp_metadexoffer.getDesProperty();
                                     mdex_propertyWanted_Div = isPropertyDivisible(mdex_propertyWanted);
                                     mdex_amountWanted = temp_metadexoffer.getAmountDesired();
                                     mdex_amountRemaining = temp_metadexoffer.getAmountRemaining();
                                     mdex_action = temp_metadexoffer.getAction();
                                     // unit price display adjustment based on divisibility and always showing prices in MSC/TMSC
                                     rational_t tempUnitPrice(int128_t(0));
                                     if ((propertyId == OMNI_PROPERTY_MSC) || (propertyId == OMNI_PROPERTY_TMSC)) {
                                         tempUnitPrice = temp_metadexoffer.inversePrice();
                                         if (!mdex_propertyWanted_Div) tempUnitPrice = tempUnitPrice/COIN;
                                     } else {
                                         tempUnitPrice = temp_metadexoffer.unitPrice();
                                         if (!mdex_propertyId_Div) tempUnitPrice = tempUnitPrice/COIN;
                                     }
                                     mdex_unitPriceStr = xToString(tempUnitPrice);
                                     if(1 == mdex_action) mdex_actionStr = "new sell";
                                     if(2 == mdex_action) mdex_actionStr = "cancel price";
                                     if(3 == mdex_action) mdex_actionStr = "cancel pair";
                                     if(4 == mdex_action) mdex_actionStr = "cancel all";
                                 }
                             }

                        break;
                        case MSC_TYPE_CHANGE_ISSUER_ADDRESS:
                             if (0 == mp_obj.step2_Value())
                             {
                                showReference = true;
                                propertyId = mp_obj.getProperty();
                                amount = mp_obj.getAmount();
                             }
                        break;
                        case MSC_TYPE_GRANT_PROPERTY_TOKENS:
                             if (0 == mp_obj.step2_Value())
                             {
                                showReference = true;
                                propertyId = mp_obj.getProperty();
                                amount = mp_obj.getAmount();
                             }
                        break;
                        case MSC_TYPE_REVOKE_PROPERTY_TOKENS:
                             if (0 == mp_obj.step2_Value())
                             {
                                propertyId = mp_obj.getProperty();
                                amount = mp_obj.getAmount();
                             }
                        break;
                        case MSC_TYPE_CREATE_PROPERTY_FIXED:
                            mp_obj.step2_SmartProperty(rc);
                            if (0 == rc)
                            {
                                LOCK(cs_tally);
                                propertyId = _my_sps->findSPByTX(wtxid); // propertyId of created property (if valid)
                                amount = getTotalTokens(propertyId);
                                propertyName = mp_obj.getSPName();
                            }
                        break;
                        case MSC_TYPE_CREATE_PROPERTY_VARIABLE:
                            mp_obj.step2_SmartProperty(rc);
                            if (0 == rc)
                            {
                                LOCK(cs_tally);
                                propertyId = _my_sps->findSPByTX(wtxid); // propertyId of created property (if valid)
                                amount = 0; // crowdsale txs always create zero tokens
                                propertyName = mp_obj.getSPName();
                            }
                        break;
                        case MSC_TYPE_CREATE_PROPERTY_MANUAL:
                            //propertyId = _my_sps->findSPByTX(wtxid); // propertyId of created property (if valid)
                            //amount = 0; // issuance of a managed token does not create tokens
                            mp_obj.step2_SmartProperty(rc);
                            if (0 == rc)
                            {
                                LOCK(cs_tally);
                                propertyId = _my_sps->findSPByTX(wtxid); // propertyId of created property (if valid)
                                amount = 0; // crowdsale txs always create zero tokens
                                propertyName = mp_obj.getSPName();
                            }
                        break;
                        case MSC_TYPE_SIMPLE_SEND:
                            if (0 == mp_obj.step2_Value())
                            {
                                propertyId = mp_obj.getProperty();
                                amount = mp_obj.getAmount();
                                showReference = true;

                                LOCK(cs_tally);
                                //check crowdsale invest?
                                crowdPurchase = isCrowdsalePurchase(wtxid, refAddress, &crowdPropertyId, &crowdTokens, &issuerTokens);
                                if (crowdPurchase)
                                {
                                    MPTxType = "Crowdsale Purchase";
                                    CMPSPInfo::Entry sp;
                                    if (false == _my_sps->getSP(crowdPropertyId, sp)) { return MP_CROWDSALE_WITHOUT_PROPERTY; }

                                    crowdName = sp.name;
                                    crowdDivisible = sp.isDivisible();
                                }
                            }
                        break;
                        case MSC_TYPE_TRADE_OFFER:
                            if (0 == mp_obj.step2_Value())
                            {
                                propertyId = mp_obj.getProperty();
                                amount = mp_obj.getAmount();
                            }
                            if (0 <= mp_obj.interpretPacket(&temp_offer))
                            {
                                sell_minfee = temp_offer.getMinFee();
                                sell_timelimit = temp_offer.getBlockTimeLimit();
                                sell_subaction = temp_offer.getSubaction();
                                sell_btcdesired = temp_offer.getBTCDesiredOriginal();
                                if (3 < sell_subaction) sell_subaction=0; // case where subaction byte >3, to have been allowed must be a v0 sell, flip byte to 0
                                // for now must mark all v0 sells as new until we can store subaction for v0 sells
                                if ((0 == sell_subaction) && (0 < amount)) sell_subaction=1; // case where subaction byte=0, must be a v0 sell, infer action from amount
                                if ((0 == sell_subaction) && (0 == amount)) sell_subaction=3; // case where subaction byte=0, must be a v0 sell, infer action from amount
                            }
                            if ((valid) && (amountNew>0)) amount=amountNew; //amount has been amended, update
                        break;
                        case MSC_TYPE_ACCEPT_OFFER_BTC:
                            if (0 == mp_obj.step2_Value())
                            {
                                propertyId = mp_obj.getProperty();
                                amount = mp_obj.getAmount();
                                showReference = true;
                            }
                            if ((valid) && (amountNew>0)) amount=amountNew; //amount has been amended, update
                        break;
                        case MSC_TYPE_CLOSE_CROWDSALE:
                            if (0 == mp_obj.step2_Value())
                            {
                                propertyId = mp_obj.getProperty();
                            }
                        break;
                        case  MSC_TYPE_SEND_TO_OWNERS:
                            if (0 == mp_obj.step2_Value())
                            {
                                propertyId = mp_obj.getProperty();
                                amount = mp_obj.getAmount();
                            }
                        break;
                    } // end switch
                    divisible=isPropertyDivisible(propertyId);
                } // end step 1 if
                else
                {
                    return -3336; // "Not a Master Protocol transaction"
                }
            } // end payment check if
        } //negative RC check
        else
        {
            return MP_INVALID_TX_IN_DB_FOUND;
        }
    }
    else
    {
        return MP_TX_IS_NOT_MASTER_PROTOCOL;
    }

    if (isMPTx)
    {
        // test sender and reference against ismine to determine which address is ours
        // if both ours (eg sending to another address in wallet) use reference
        bIsMine = IsMyAddress(senderAddress);
        if (!bIsMine)
        {
            bIsMine = IsMyAddress(refAddress);
        }
        txobj->push_back(Pair("txid", wtxid.GetHex()));
        txobj->push_back(Pair("sendingaddress", senderAddress));
        if (showReference) txobj->push_back(Pair("referenceaddress", refAddress));
        txobj->push_back(Pair("ismine", bIsMine));
        txobj->push_back(Pair("confirmations", confirmations));
        txobj->push_back(Pair("fee", ValueFromAmount(nFee)));
        txobj->push_back(Pair("blocktime", blockTime));
        txobj->push_back(Pair("version", (int64_t)mp_obj.getVersion()));
        txobj->push_back(Pair("type_int", (int64_t)mp_obj.getType()));
        txobj->push_back(Pair("type", MPTxType));
        if (MSC_TYPE_METADEX_TRADE != MPTxTypeInt
                && MSC_TYPE_METADEX_CANCEL_PRICE != MPTxTypeInt
                && MSC_TYPE_METADEX_CANCEL_PAIR != MPTxTypeInt
                && MSC_TYPE_METADEX_CANCEL_ECOSYSTEM != MPTxTypeInt)
        {
            txobj->push_back(Pair("propertyid", propertyId));
        }
        if ((MSC_TYPE_CREATE_PROPERTY_VARIABLE == MPTxTypeInt) || (MSC_TYPE_CREATE_PROPERTY_FIXED == MPTxTypeInt) || (MSC_TYPE_CREATE_PROPERTY_MANUAL == MPTxTypeInt))
        {
            txobj->push_back(Pair("propertyname", propertyName));
        }
        if (MSC_TYPE_METADEX_TRADE != MPTxTypeInt
                && MSC_TYPE_METADEX_CANCEL_PRICE != MPTxTypeInt
                && MSC_TYPE_METADEX_CANCEL_PAIR != MPTxTypeInt
                && MSC_TYPE_METADEX_CANCEL_ECOSYSTEM != MPTxTypeInt)
        {
            txobj->push_back(Pair("divisible", divisible));
        }
        if (MSC_TYPE_METADEX_TRADE != MPTxTypeInt
                && MSC_TYPE_METADEX_CANCEL_PRICE != MPTxTypeInt
                && MSC_TYPE_METADEX_CANCEL_PAIR != MPTxTypeInt
                && MSC_TYPE_METADEX_CANCEL_ECOSYSTEM != MPTxTypeInt)
        {
            txobj->push_back(Pair("amount", FormatMP(propertyId, amount)));
        }
        if (crowdPurchase)
        {
            txobj->push_back(Pair("purchasedpropertyid", crowdPropertyId));
            txobj->push_back(Pair("purchasedpropertyname", crowdName));
            txobj->push_back(Pair("purchasedpropertydivisible", crowdDivisible));
            txobj->push_back(Pair("purchasedtokens", FormatMP(crowdPropertyId, crowdTokens)));
            txobj->push_back(Pair("issuertokens", FormatMP(crowdPropertyId, issuerTokens)));
        }
        if (MSC_TYPE_TRADE_OFFER == MPTxTypeInt)
        {
            txobj->push_back(Pair("feerequired", ValueFromAmount(sell_minfee)));
            txobj->push_back(Pair("timelimit", sell_timelimit));
            if (1 == sell_subaction) txobj->push_back(Pair("subaction", "New"));
            if (2 == sell_subaction) txobj->push_back(Pair("subaction", "Update"));
            if (3 == sell_subaction) txobj->push_back(Pair("subaction", "Cancel"));
            txobj->push_back(Pair("bitcoindesired", ValueFromAmount(sell_btcdesired)));
        }
        if (MSC_TYPE_METADEX_TRADE == MPTxTypeInt
                || MSC_TYPE_METADEX_CANCEL_PRICE == MPTxTypeInt
                || MSC_TYPE_METADEX_CANCEL_PAIR == MPTxTypeInt
                || MSC_TYPE_METADEX_CANCEL_ECOSYSTEM == MPTxTypeInt)
        {
            txobj->push_back(Pair("propertyidforsale", propertyId));
            txobj->push_back(Pair("propertyidforsaleisdivisible", mdex_propertyId_Div));
            txobj->push_back(Pair("amountforsale", FormatMP(propertyId, amount)));
            txobj->push_back(Pair("amountremaining", FormatMP(propertyId, mdex_amountRemaining)));
            txobj->push_back(Pair("propertyiddesired", mdex_propertyWanted));
            txobj->push_back(Pair("propertyiddesiredisdivisible", mdex_propertyWanted_Div));
            txobj->push_back(Pair("amountdesired", FormatMP(mdex_propertyWanted, mdex_amountWanted)));
            txobj->push_back(Pair("unitprice", mdex_unitPriceStr));
            txobj->push_back(Pair("action", mdex_actionStr));
            if (mdex_actionStr == "cancel all") txobj->push_back(Pair("ecosystem",
                isTestEcosystemProperty(propertyId) ? "Test" : "Main"));
        }
        txobj->push_back(Pair("valid", valid));
    }
    return 0;
}

Value gettransaction_MP(const Array& params, bool fHelp)
{
    // note this call has been refactored to use the singular populateRPCTransactionObject function

    if (fHelp || params.size() != 1)
        throw runtime_error(
            "gettransaction_MP \"txid\"\n"
            "\nGet detailed information about a Master Protocol transaction <txid>\n"
            "\nArguments:\n"
            "1. \"txid\"    (string, required) The transaction id\n"
            "\nResult:\n"
            "{\n"
            "  \"txid\" : \"transactionid\",   (string) The transaction id\n"
            "  \"sendingaddress\" : \"sender\",    (string) The sending address\n"
            "  \"referenceaddress\" : \"receiving address\",    (string) The receiving address (if applicable)\n"
            "  \"ismine\" : true/false\",    (boolean) Whether the transaction involes an address in the wallet\n"
            "  \"confirmations\" : n,     (numeric) The number of confirmations\n"
            "  \"blocktime\" : ttt,       (numeric) The time in seconds since epoch (1 Jan 1970 GMT)\n"
            "  \"type\" : \"transaction type\",    (string) The type of transaction\n"
            "  \"propertyid\" : \"property id\",    (numeric) The ID of the property transacted\n"
            "  \"propertyname\" : \"property name\",    (string) The name of the property (for creation transactions)\n"
            "  \"divisible\" : true/false\",    (boolean) Whether the property is divisible\n"
            "  \"amount\" : \"x.xxx\",     (string) The transaction amount\n"
            "  \"valid\" : true/false\",    (boolean) Whether the transaction is valid\n"
            "}\n"

            "\nbExamples\n"
            + HelpExampleCli("gettransaction_MP", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
            + HelpExampleRpc("gettransaction_MP", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
        );

    uint256 hash;
    hash.SetHex(params[0].get_str());
    Object txobj;

    // make a request to new RPC populator function to populate a transaction object
    int populateResult = populateRPCTransactionObject(hash, &txobj);
    // check the response, throw any error codes if false
    if (0>populateResult)
    {
        // TODO: consider throwing other error codes, check back with Bitcoin Core
        switch (populateResult)
        {
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
    }
    // everything seems ok, return the object
    return txobj;
}

Value listtransactions_MP(const Array& params, bool fHelp)
{
    // note this call has been refactored to use the singular populateRPCTransactionObject function
    // note address filtering has been readded to this call, and searchtransactions_MP has been removed since performance hit no longer an issue

    if (fHelp || params.size() > 5)
        throw runtime_error(
            "listtransactions_MP ( \"address\" count skip startblock endblock )\n" //todo increase verbosity in help
            "\nList wallet transactions filtered on counts and block boundaries\n"
            + HelpExampleCli("listtransactions_MP", "")
            + HelpExampleRpc("listtransactions_MP", "")
        );

    CWallet *wallet = pwalletMain;
    string sAddress = "";
    string addressParam = "";
    bool addressFilter;

    //if 0 params consider all addresses in wallet, otherwise first param is filter address
    addressFilter = false;
    if (params.size() > 0)
    {
        // allow setting "" or "*" to use nCount and nFrom params with all addresses in wallet
        if ( ("*" != params[0].get_str()) && ("" != params[0].get_str()) )
        {
            addressParam = params[0].get_str();
            addressFilter = true;
        }
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

    Array response; //prep an array to hold our output

    // STO has no inbound transaction, so we need to use an insert methodology here
    std::string mySTOReceipts;
    {
        LOCK(cs_tally);
        mySTOReceipts = s_stolistdb->getMySTOReceipts(addressParam);
    }
    std::vector<std::string> vecReceipts;
    boost::split(vecReceipts, mySTOReceipts, boost::is_any_of(","), token_compress_on);
    int64_t lastTXBlock = 999999;

    std::list<CAccountingEntry> acentries;
    CWallet::TxItems txOrdered;
    {
        LOCK(pwalletMain->cs_wallet);
        txOrdered = pwalletMain->OrderedTxItems(acentries, "*");
    }

    // iterate backwards until we have nCount items to return:
    for (CWallet::TxItems::reverse_iterator it = txOrdered.rbegin(); it != txOrdered.rend(); ++it) {
        CWalletTx *const pwtx = (*it).second.first;
        if (pwtx != 0) {
            // get the height of the transaction and check it's within the chosen parameters
            uint256 blockHash = pwtx->hashBlock;
            if ((0 == blockHash) || (NULL == GetBlockIndex(blockHash))) continue;
            CBlockIndex* pBlockIndex = GetBlockIndex(blockHash);
            if (NULL == pBlockIndex) continue;
            int blockHeight = pBlockIndex->nHeight;
            if ((blockHeight < nStartBlock) || (blockHeight > nEndBlock)) continue; // ignore it if not within our range

            // look for an STO receipt to see if we need to insert it
            for(uint32_t i = 0; i<vecReceipts.size(); i++) {
                std::vector<std::string> svstr;
                boost::split(svstr, vecReceipts[i], boost::is_any_of(":"), token_compress_on);
                if(4 == svstr.size()) { // make sure expected num items
                    if((atoi(svstr[1]) < lastTXBlock) && (atoi(svstr[1]) > blockHeight)) {
                        // STO receipt insert here - add STO receipt to response array
                        uint256 hash;
                        hash.SetHex(svstr[0]);
                        Object txobj;
                        int populateResult = -1;
                        populateResult = populateRPCTransactionObject(hash, &txobj);
                        if (0 == populateResult) {
                            Array receiveArray;
                            uint64_t tmpAmount = 0;
                            uint64_t stoFee = 0;
                            {
                                LOCK(cs_tally);
                                s_stolistdb->getRecipients(hash, addressParam, &receiveArray, &tmpAmount, &stoFee);
                            }
                            // add matches array and stofee to object
                            txobj.push_back(Pair("totalstofee", FormatDivisibleMP(stoFee))); // fee always MSC so always divisible
                            txobj.push_back(Pair("recipients", receiveArray));
                            response.push_back(txobj); // add the transaction object to the response array
                        }
                        // don't burn time doing more work than we need to
                        if ((int)response.size() >= (nCount+nFrom)) break;
                    }
                }
            }

            uint256 hash = pwtx->GetHash();
            Object txobj;

            // make a request to new RPC populator function to populate a transaction object (if it is a MP transaction)
            int populateResult = -1;
            if (addressFilter) {
                populateResult = populateRPCTransactionObject(hash, &txobj, addressParam); // pass in an address filter
            } else {
                populateResult = populateRPCTransactionObject(hash, &txobj); // no address filter
            }
            if (0 == populateResult) response.push_back(txobj); // add the transaction object to the response array if we get a 0 rc
            lastTXBlock = blockHeight;

            // don't burn time doing more work than we need to
            if ((int)response.size() >= (nCount+nFrom)) break;
        }
    }
    // sort array here and cut on nFrom and nCount
    if (nFrom > (int)response.size())
        nFrom = response.size();
    if ((nFrom + nCount) > (int)response.size())
        nCount = response.size() - nFrom;
    Array::iterator first = response.begin();
    std::advance(first, nFrom);
    Array::iterator last = response.begin();
    std::advance(last, nFrom+nCount);

    if (last != response.end()) response.erase(last, response.end());
    if (first != response.begin()) response.erase(response.begin(), first);

    std::reverse(response.begin(), response.end()); // return oldest to newest?
    return response;   // return response array for JSON serialization
}

Value getinfo_MP(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getinfo_MP\n"
            "Returns various state information of the client and protocol.\n"
            "\nResult:\n"
            "{\n"
            "  \"mastercoreversion\" : \"x.x.x.x-xxx\",   (string) client version\n"
            "  \"bitcoincoreversion\" : \"x.x.x\",        (string) Bitcoin Core version\n"
            "  \"commitinfo\" : \"xxxxxxx\",              (string) build commit identifier\n"
            "  \"block\" : x,                           (number) number of blocks processed\n"
            "  \"blocktime\" : xxxxxxxxxx,              (number) timestamp of the last processed block\n"
            "  \"blocktransactions\" : x,               (number) meta transactions found in the last processed block\n"
            "  \"totaltransactions\" : x,               (number) meta transactions processed in total\n"
            "  \"alert\" : {                            (object) any active protocol alert\n"
            "    \"alerttype\" : \"xxx\"                    (string) alert type\n"
            "    \"expiryvalue\" : \"x\"                    (string) block when the alert expires\n"
            "    \"alertmessage\" : \"xxx\"                 (string) information about the alert event\n"
            "  }\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getinfo", "")
            + HelpExampleRpc("getinfo", "")
        );

    Object infoResponse;
    // other bits of info we want to report should be included here

    // provide the mastercore and bitcoin version and if available commit id
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
    Object alertResponse;
    std::string global_alert_message = GetOmniCoreAlertString();

    if (!global_alert_message.empty()) {
        int32_t alertType = 0;
        uint64_t expiryValue = 0;
        uint32_t typeCheck = 0;
        uint32_t verCheck = 0;
        std::string alertTypeStr;
        std::string alertMessage;
        bool parseSuccess = ParseAlertMessage(global_alert_message, alertType, expiryValue, typeCheck, verCheck, alertMessage);
        if (parseSuccess) {
            switch (alertType) {
                case 0: alertTypeStr = "error";
                    break;
                case 1: alertTypeStr = "textalertexpiringbyblock";
                    break;
                case 2: alertTypeStr = "textalertexpiringbytime";
                    break;
                case 3: alertTypeStr = "textalertexpiringbyversion";
                    break;
                case 4: alertTypeStr = "updatealerttxcheck";
                    break;
            }
            alertResponse.push_back(Pair("alerttype", alertTypeStr));
            alertResponse.push_back(Pair("expiryvalue", FormatIndivisibleMP(expiryValue)));
            if (alertType == 4) {
                alertResponse.push_back(Pair("typecheck", FormatIndivisibleMP(typeCheck)));
                alertResponse.push_back(Pair("vercheck", FormatIndivisibleMP(verCheck)));
            }
            alertResponse.push_back(Pair("alertmessage", alertMessage));
        }
    }
    infoResponse.push_back(Pair("alert", alertResponse));

    return infoResponse;
}

Value getsto_MP(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "getsto_MP \"txid\" \"recipientfilter\"\n"
            "\nGet information and recipients of send to owners transaction <txid>\n"
            "\nArguments:\n"
            "1. \"txid\"    (string, required) The transaction id\n"
            "2. \"recipientfilter\"    (string, optional) The recipient address filter (wallet by default, \"*\" for all)\n"
            "\nResult:\n"
            "{\n"
            "  \"txid\" : \"transactionid\",   (string) The transaction id\n"
            + HelpExampleCli("getsto_MP", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
            + HelpExampleRpc("getsto_MP", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
        );

    uint256 hash;
    hash.SetHex(params[0].get_str());
    string filterAddress = "";
    if (params.size() == 2) filterAddress=params[1].get_str();
    Object txobj;
    CTransaction wtx;
    uint256 blockHash = 0;
    if (!GetTransaction(hash, wtx, blockHash, true)) { return MP_TX_NOT_FOUND; }
    uint64_t propertyId = 0;

    CMPTransaction mp_obj;
    int parseRC = ParseTransaction(wtx, 0, 0, mp_obj);
    if (0 <= parseRC) //negative RC means no MP content/badly encoded TX, we shouldn't see this if TX in levelDB but check for safety
    {
        if (0<=mp_obj.step1())
        {
            if (0 == mp_obj.step2_Value())
            {
                propertyId = mp_obj.getProperty();
            }
        }
        if (propertyId == 0) // something went wrong, couldn't decode property ID - bad packet?
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Not a Master Protocol transaction");

        // make a request to new RPC populator function to populate a transaction object
        int populateResult = populateRPCTransactionObject(hash, &txobj);
        // check the response, throw any error codes if false
        if (0>populateResult)
        {
            // TODO: consider throwing other error codes, check back with Bitcoin Core
            switch (populateResult)
            {
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
        }
        // create array of recipients
        Array receiveArray;
        uint64_t tmpAmount = 0;
        uint64_t stoFee = 0;
        {
            LOCK(cs_tally);
            s_stolistdb->getRecipients(hash, filterAddress, &receiveArray, &tmpAmount, &stoFee);
        }
        // add matches array and stofee to object
        txobj.push_back(Pair("totalstofee", FormatDivisibleMP(stoFee))); // fee always MSC so always divisible
        txobj.push_back(Pair("recipients", receiveArray));
    }
    // return object
    return txobj;
}

Value gettrade_MP(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "gettrade_MP \"txid\"\n"
            "\nGet detailed information and trade matches for a Master Protocol MetaDEx trade offer <txid>\n"
            "\nArguments:\n"
            "1. \"txid\"    (string, required) The transaction id\n"
            "\nResult:\n"
            "{\n"
            "  \"txid\" : \"transactionid\",   (string) The transaction id\n"
            + HelpExampleCli("gettrade_MP", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
            + HelpExampleRpc("gettrade_MP", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
        );

    // Prepare a few vars
    uint256 hash;
    Object tradeobj;
    Object txobj;
    CMPMetaDEx temp_metadexoffer;
    CTransaction wtx;
    uint256 blockHash = 0;
    string senderAddress;
    unsigned int propertyId = 0;
    CMPTransaction mp_obj;

    // Obtain the transaction
    hash.SetHex(params[0].get_str());
    if (!GetTransaction(hash, wtx, blockHash, true)) { return MP_TX_NOT_FOUND; }

    // Ensure it can be parsed OK
    int parseRC = ParseTransaction(wtx, 0, 0, mp_obj);
    if (0 <= parseRC) { //negative RC means no MP content/badly encoded TX, we shouldn't see this if TX in levelDB but check for sanity
        if (0<=mp_obj.step1()) {
            senderAddress = mp_obj.getSender();
            if (0 == mp_obj.step2_Value()) propertyId = mp_obj.getProperty();
        }
    }
    if ((0 > parseRC) || (propertyId == 0) || (senderAddress.empty())) // something went wrong, couldn't decode - bad packet?
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Not a Master Protocol transaction");

    // make a request to RPC populator function to populate a transaction object
    int populateResult = populateRPCTransactionObject(hash, &txobj);

    // check the response, throw any error codes if false
    if (0>populateResult) {
        // TODO: consider throwing other error codes, check back with Bitcoin Core
        switch (populateResult) {
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
    }

    // get the amount for sale in this sell offer to see if filled
    int64_t amountForSale = mp_obj.getAmount();

    // create array of matches
    Array tradeArray;
    int64_t totalBought = 0;
    int64_t totalSold = 0;
    {
        LOCK(cs_tally);
        t_tradelistdb->getMatchingTrades(hash, propertyId, &tradeArray, &totalSold, &totalBought);
    }
    // get action byte
    int actionByte = 0;
    if (0 <= mp_obj.interpretPacket(NULL,&temp_metadexoffer)) { actionByte = (int)temp_metadexoffer.getAction(); }

    // everything seems ok, now add status and add array of matches to the object
    // work out status
    bool orderOpen = MetaDEx_isOpen(hash, propertyId);
    bool partialFilled = false;
    bool filled = false;
    string statusText;
    if(totalSold>0) partialFilled = true;
    if(totalSold>=amountForSale) filled = true;
    statusText = "unknown";
    if((!orderOpen) && (!partialFilled)) statusText = "cancelled"; // offers that are closed but not filled must have been cancelled
    if((!orderOpen) && (partialFilled)) statusText = "cancelled part filled"; // offers that are closed but not filled must have been cancelled
    if((!orderOpen) && (filled)) statusText = "filled"; // filled offers are closed
    if((orderOpen) && (!partialFilled)) statusText = "open"; // offer exists but no matches yet
    if((orderOpen) && (partialFilled)) statusText = "open part filled"; // offer exists, some matches but not filled yet
    if(actionByte==1) txobj.push_back(Pair("status", statusText)); // no status for cancel txs

    LOCK(cs_tally);

    // add cancels array to object and set status as cancelled only if cancel type
    if(actionByte != 1) {
        Array cancelArray;
        int numberOfCancels = p_txlistdb->getNumberOfMetaDExCancels(hash);
        if (0<numberOfCancels) {
            for(int refNumber = 1; refNumber <= numberOfCancels; refNumber++) {
                Object cancelTx;
                string strValue = p_txlistdb->getKeyValue(hash.ToString() + "-C" + to_string(refNumber));
                if (!strValue.empty()) {
                    std::vector<std::string> vstr;
                    boost::split(vstr, strValue, boost::is_any_of(":"), token_compress_on);
                    if (3 <= vstr.size()) {
                        uint64_t propId = boost::lexical_cast<uint64_t>(vstr[1]);
                        uint64_t amountUnreserved = boost::lexical_cast<uint64_t>(vstr[2]);
                        cancelTx.push_back(Pair("txid", vstr[0]));
                        cancelTx.push_back(Pair("propertyid", propId));
                        cancelTx.push_back(Pair("amountunreserved", FormatMP(propId, amountUnreserved)));
                        cancelArray.push_back(cancelTx);
                    }
                }
            }
        }
        txobj.push_back(Pair("cancelledtransactions", cancelArray));
    } else {
        // if cancelled, show cancellation txid
        if((statusText == "cancelled") || (statusText == "cancelled part filled")) { txobj.push_back(Pair("canceltxid", p_txlistdb->findMetaDExCancel(hash).GetHex())); }
        // add matches array to object
        txobj.push_back(Pair("matches", tradeArray)); // only action 1 offers can have matches
    }

    // return object
    return txobj;
}

