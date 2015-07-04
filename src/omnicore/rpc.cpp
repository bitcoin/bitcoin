// RPC calls

#include "omnicore/rpc.h"

#include "omnicore/convert.h"
#include "omnicore/createpayload.h"
#include "omnicore/dex.h"
#include "omnicore/errors.h"
#include "omnicore/fetchwallettx.h"
#include "omnicore/log.h"
#include "omnicore/mdex.h"
#include "omnicore/notifications.h"
#include "omnicore/omnicore.h"
#include "omnicore/parse_string.h"
#include "omnicore/rpcrequirements.h"
#include "omnicore/rpctx.h"
#include "omnicore/rpctxobject.h"
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
    if (obj.getAction() == 4) metadex_obj.push_back(Pair("ecosystem", isTestEcosystemProperty(obj.getProperty()) ? "Test" : "Main"));
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
            "\"hash\"               (string) the hex-encoded transaction hash\n"
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
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "getcrowdsale_MP propertyid ( verbose )\n"
            "\nGet information about a crowdsale for a property identifier.\n"
            "\nArguments:\n"
            "1. propertyid        (number, required) the property identifier\n"
            "2. verbose           (boolean, optional) list crowdsale participants (default: false)\n"
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
            + HelpExampleCli("getcrowdsale_MP", "3 true")
            + HelpExampleRpc("getcrowdsale_MP", "3, true")
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

    int64_t amountRaised = 0;
    int64_t tokensIssued = getTotalTokens(propertyId);
    const std::string& txidClosed = sp.txid_close.GetHex();

    int64_t startTime = -1;
    if (0 != hashBlock && GetBlockIndex(hashBlock)) {
        startTime = GetBlockIndex(hashBlock)->nTime;
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
        Array participanttxs;
        std::map<uint256, std::vector<int64_t> >::const_iterator it;
        for (it = database.begin(); it != database.end(); it++) {
            Object participanttx;

            std::string txid = it->first.GetHex();
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

Value getorderbook_MP(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "getorderbook_MP propertyid ( propertyid )\n"
            "\nRequest active trade information from the MetaDEx.\n"
            "\nArguments:\n"
            "1. propertyid           (number, required) filter orders by propertyid for sale\n"
            "2. propertyid           (number, optional) filter orders by propertyid desired\n"
            "\nResult:\n"
            "[                             (array of MetaDEx trades)\n"
            "  ...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("getorderbook_MP", "1 12")
            + HelpExampleRpc("getorderbook_MP", "1, 12")
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

Value gettradehistoryforaddress_OMNI(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 3)
        throw runtime_error(
            "gettradehistoryforaddress_MP \"address\" ( count propertyid )\n"
            "\nRetrieves MetaDEx trade history for the supplied address\n"
            "\nArguments:\n"
            "1. address          (string, required) address to retrieve history for\n"
            "2. count            (number, optional) number of trades to retrieve (default: 10)\n"
            "3. propertyid       (number, optional) filter by propertyid transacted (default: no filter)\n"
            "\nResult:\n"
            "[                             (array of MetaDEx trades including matches)\n"
            "  ...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("gettradehistoryforaddress_OMNI", "\"1MCHESTptvd2LnNp7wmr2sGTpRomteAkq8\"")
            + HelpExampleRpc("gettradehistoryforaddress_OMNI", "\"1MCHESTptvd2LnNp7wmr2sGTpRomteAkq8\"")
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

Value gettradehistoryforpair_OMNI(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 2 || params.size() > 3)
        throw runtime_error(
            "gettradehistoryforpair_OMNI propertyid propertyid ( count )\n"
            "\nAllows user to retrieve MetaDEx trade history for the specified market\n"
            "\nArguments:\n"
            "1. propertyid           (number, required) the first side of the pair\n"
            "2. propertyid           (number, required) the second side of the pair\n"
            "3. count                (number, optional) number of trades to retrieve (default: 10)\n"
            "\nResult:\n"
            "[                             (array of MetaDEx trades)\n"
            "  ...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("gettradehistoryforpair_OMNI", "1 12 500")
            + HelpExampleRpc("gettradehistoryforpair_OMNI", "1, 12, 500")
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

Value getactivedexsells_MP(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "getactivedexsells_MP ( address )\n"
            "\nReturns currently active offers on the distributed exchange.\n"
            "\nArguments:\n"
            "1. address          (string, optional) address filter (default: include any)\n"
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

    std::string addressFilter;

    if (params.size() > 0) {
        addressFilter = ParseAddress(params[0]);
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

        //unit price & updated bitcoin desired calcs
        double unitPriceFloat = 0;
        if ((sellOfferAmount > 0) && (sellBitcoinDesired > 0)) unitPriceFloat = (double) sellBitcoinDesired / (double) sellOfferAmount; //divide by zero protection
        uint64_t unitPrice = rounduint64(unitPriceFloat * COIN);
        uint64_t bitcoinDesired = rounduint64(amountAvailable * unitPriceFloat);

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
                int acceptBlock = accept.getAcceptBlock();
                int64_t acceptAmount = accept.getAcceptAmountRemaining();
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

Value gettransaction_MP(const Array& params, bool fHelp)
{
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

    uint256 hash(params[0].get_str());

    Object txobj;
    int populateResult = populateRPCTransactionObject(hash, txobj);
    if (populateResult != 0) PopulateFailure(populateResult);

    return txobj;
}

Value listtransactions_MP(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 5)
        throw runtime_error(
            "listtransactions_MP ( \"address\" count skip startblock endblock )\n"
            "\nList wallet transactions, optionally filtered by an address and block boundaries.\n"
            "\nArguments:\n"
            "1. address           (string, optional) address filter (default: \"*\")\n"
            "2. count             (number, optional) show at most n transactions (default: 10)\n"
            "3. skip              (number, optional) skip the first n transactions (default: 0)\n"
            "4. startblock        (number, optional) first block to begin the search (default: 0)\n"
            "5. endblock          (number, optional) last block to include in the search (default: 999999)\n"
            "\nResult:\n"
            "[                             (array of transactions)\n"
            "  ...\n"
            "]\n"
            + HelpExampleCli("listtransactions_MP", "")
            + HelpExampleRpc("listtransactions_MP", "")
        );

    // obtains parameters - default all wallet addresses & last 10 transactions
    string addressParam = "";
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

    uint256 hash(params[0].get_str());
    std::string filterAddress;
    if (params.size() > 1) filterAddress = ParseAddress(params[1]);

    Object txobj;
    int populateResult = populateRPCTransactionObject(hash, txobj, "", true, filterAddress);
    if (populateResult != 0) PopulateFailure(populateResult);

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

    uint256 hash(params[0].get_str());

    Object txobj;
    int populateResult = populateRPCTransactionObject(hash, txobj, "", true);
    if (populateResult != 0) PopulateFailure(populateResult);

    return txobj;
}

