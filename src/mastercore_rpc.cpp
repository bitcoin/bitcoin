// RPC calls

#include "mastercore_rpc.h"

#include "mastercore.h"
#include "mastercore_convert.h"
#include "mastercore_dex.h"
#include "mastercore_errors.h"
#include "mastercore_log.h"
#include "mastercore_mdex.h"
#include "mastercore_parse_string.h"
#include "mastercore_sp.h"
#include "mastercore_tx.h"
#include "mastercore_version.h"

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

  printf("%s(extra=%d,extra2=%d,extra3=%d)\n", __FUNCTION__, extra, extra2, extra3);

  bool bDivisible = isPropertyDivisible(extra2);

  // various extra tests
  switch (extra)
  {
    case 0: // the old output
    {
    uint64_t total = 0;

        // display all balances
        for(map<string, CMPTally>::iterator my_it = mp_tally_map.begin(); my_it != mp_tally_map.end(); ++my_it)
        {
          // my_it->first = key
          // my_it->second = value

          printf("%34s => ", (my_it->first).c_str());
          total += (my_it->second).print(extra2, bDivisible);
        }

        printf("total for property %d  = %X is %s\n", extra2, extra2, FormatDivisibleMP(total).c_str());
      }
      break;

    case 1:
      // display the whole CMPTxList (leveldb)
      p_txlistdb->printAll();
      p_txlistdb->printStats();
      break;

    case 2:
        // display smart properties
        _my_sps->printAll();
      break;

    case 3:
        unsigned int id;
        // for each address display all currencies it holds
        for(map<string, CMPTally>::iterator my_it = mp_tally_map.begin(); my_it != mp_tally_map.end(); ++my_it)
        {
          // my_it->first = key
          // my_it->second = value

          printf("%34s => ", (my_it->first).c_str());
          (my_it->second).print(extra2);

          (my_it->second).init();
          while (0 != (id = (my_it->second).next()))
          {
            printf("Id: %u=0x%X ", id, id);
          }
          printf("\n");
        }
      break;

    case 4:
      for(CrowdMap::const_iterator it = my_crowds.begin(); it != my_crowds.end(); ++it)
      {
        (it->second).print(it->first);
      }
      break;

    case 5:
      printf("isMPinBlockRange(%d,%d)=%s\n", extra2, extra3, isMPinBlockRange(extra2, extra3, false) ? "YES":"NO");
      break;

    case 6:
      MetaDEx_debug_print(true, true);
      break;

    case 7:
      // display the whole CMPTradeList (leveldb)
      t_tradelistdb->printAll();
      t_tradelistdb->printStats();
      break;

    case 8:
      // display the STO receive list
      s_stolistdb->printAll();
      s_stolistdb->printStats();
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
            "\nReturns the Master Protocol balance for a given address and currency/property.\n"
            "\nResult:\n"
            "n    (numeric) The applicable balance for address:currency/propertyID pair\n"
            "\nExamples:\n"
            ">mastercored getbalance_MP 1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P 1\n"
        );
    std::string address = params[0].get_str();
    int64_t tmpPropertyId = params[1].get_int64();
    if ((1 > tmpPropertyId) || (4294967295 < tmpPropertyId)) // not safe to do conversion
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid property identifier");

    unsigned int propertyId = int(tmpPropertyId);
    CMPSPInfo::Entry sp;
    if (false == _my_sps->getSP(propertyId, sp)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Property identifier does not exist");
    }

    Object balance_obj;
    BalanceToJSON(address, propertyId, balance_obj, isPropertyDivisible(propertyId));

    return balance_obj;
}

// send a MP transaction via RPC - simple send
Value send_MP(const Array& params, bool fHelp)
{
if (fHelp || params.size() < 4 || params.size() > 6)
        throw runtime_error(
            "send_MP \"fromaddress\" \"toaddress\" propertyid \"amount\" ( \"redeemaddress\" \"referenceamount\" )\n"
            "\nCreates and broadcasts a simple send for a given amount and currency/property ID.\n"
            "\nParameters:\n"
            "FromAddress   : the address to send from\n"
            "ToAddress     : the address to send to\n"
            "PropertyID    : the id of the smart property to send\n"
            "Amount        : the amount to send\n"
            "RedeemAddress : (optional) the address that can redeem the bitcoin outputs. Defaults to FromAddress\n"
            "ReferenceAmount:(optional)\n"
            "Result:\n"
            "txid    (string) The transaction ID of the sent transaction\n"
            "\nExamples:\n"
            ">mastercored send_MP 1FromAddress 1ToAddress PropertyID Amount 1RedeemAddress\n"
        );

  std::string FromAddress = (params[0].get_str());
  std::string ToAddress = (params[1].get_str());
  std::string RedeemAddress = (params.size() > 4) ? (params[4].get_str()): "";

  int64_t tmpPropertyId = params[2].get_int64();
  if ((1 > tmpPropertyId) || (4294967295 < tmpPropertyId)) // not safe to do conversion
    throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid property identifier");
  unsigned int propertyId = int(tmpPropertyId);

  CMPSPInfo::Entry sp;
  if (false == _my_sps->getSP(propertyId, sp)) {
    throw JSONRPCError(RPC_INVALID_PARAMETER, "Property identifier does not exist");
  }

  bool divisible = sp.isDivisible();

  string strAmount = params[3].get_str();
  int64_t Amount = 0, additional = 0;
  Amount = StrToInt64(strAmount, divisible);

  if (0 >= Amount)
   throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount");

  std::string strAdditional = (params.size() > 5) ? (params[5].get_str()): "0";
  additional = StrToInt64(strAdditional, true);

  if ((0.01 * COIN) < additional)
   throw JSONRPCError(RPC_TYPE_ERROR, "Invalid reference amount");

  //some sanity checking of the data supplied?
  int code = 0;
  uint256 newTX = send_INTERNAL_1packet(FromAddress, ToAddress, RedeemAddress, propertyId, Amount, 0, 0, MSC_TYPE_SIMPLE_SEND, additional, &code);

  if (0 != code) throw JSONRPCError(code, error_str(code));

  //we need to do better than just returning a string of 0000000 here if we can't send the TX
  return newTX.GetHex();
}

// send a MP transaction via RPC - simple send
Value sendtoowners_MP(const Array& params, bool fHelp)
{
if (fHelp || params.size() < 3 || params.size() > 4)
        throw runtime_error(
            "sendtoowners_MP \"fromaddress\" propertyid \"amount\" ( \"redeemaddress\" )\n"
            "\nCreates and broadcasts a send-to-owners transaction for a given amount and currency/property ID.\n"
            "\nParameters:\n"
            "FromAddress   : the address to send from\n"
            "PropertyID    : the id of the smart property to send\n"
            "Amount (string): the amount to send\n"
            "RedeemAddress : (optional) the address that can redeem the bitcoin outputs. Defaults to FromAddress\n"
            "\nResult:\n"
            "txid    (string) The transaction ID of the sent transaction\n"
            "\nExamples:\n"
            ">mastercored send_MP 1FromAddress PropertyID Amount 1RedeemAddress\n"
        );

  std::string FromAddress = (params[0].get_str());
  std::string RedeemAddress = (params.size() > 3) ? (params[3].get_str()): "";

  int64_t tmpPropertyId = params[1].get_int64();
  if ((1 > tmpPropertyId) || (4294967295 < tmpPropertyId)) // not safe to do conversion
    throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid property identifier");

  unsigned int propertyId = int(tmpPropertyId);

  CMPSPInfo::Entry sp;
  if (false == _my_sps->getSP(propertyId, sp)) {
    throw JSONRPCError(RPC_INVALID_PARAMETER, "Property identifier does not exist");
  }

  bool divisible = sp.isDivisible();

//  printf("%s(), params3='%s' line %d, file: %s\n", __FUNCTION__, params[3].get_str().c_str(), __LINE__, __FILE__);

  string strAmount = params[2].get_str();
  int64_t Amount = 0;
  Amount = StrToInt64(strAmount, divisible);

  if (0 >= Amount)
    throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount");

  // printf("%s() %40.25lf, %lu, line %d, file: %s\n", __FUNCTION__, tmpAmount, Amount, __LINE__, __FILE__);

  //some sanity checking of the data supplied?
  int code = 0;
  uint256 newTX = send_INTERNAL_1packet(FromAddress, "", RedeemAddress, propertyId, Amount, 0, 0, MSC_TYPE_SEND_TO_OWNERS, 0, &code);

  if (0 != code)
    throw JSONRPCError(code, error_str(code));

  //we need to do better than just returning a string of 0000000 here if we can't send the TX
  return newTX.GetHex();
}

Value sendrawtx_MP(const Array& params, bool fHelp)
{
if (fHelp || params.size() < 2 || params.size() > 5)
        throw runtime_error(
            "sendrawtx_MP \"fromaddress\" \"hexstring\" ( \"toaddress\" \"redeemaddress\" \"referenceamount\" )\n"
            "\nCreates and broadcasts a raw Master protocol transaction.\n"
            "\nParameters:\n"
            "FromAddress   : the address to send from\n"
            "RawTX         : the hex-encoded raw transaction\n"
            "ToAddress     : the address to send to.  This should be empty: (\"\") for transaction\n"
            "                types that do not use a reference/to address\n"
            "RedeemAddress : (optional) the address that can redeem the bitcoin outputs. Defaults to FromAddress\n"
            "ReferenceAmount:(optional)\n"
            "\nResult:\n"
            "txid    (string) The transaction ID of the sent transaction\n"
            "\nExamples:\n"
            ">mastercored sendrawtx_MP 1FromAddress <tx bytes hex> 1ToAddress 1RedeemAddress\n"
        );

  std::string fromAddress = (params[0].get_str());
  std::string hexTransaction = (params[1].get_str());
  std::string toAddress = (params.size() > 2) ? (params[2].get_str()): "";
  std::string redeemAddress = (params.size() > 3) ? (params[3].get_str()): "";
  
  int64_t referenceAmount = 0;

  if (params.size() > 4)
      referenceAmount = StrToInt64(params[4].get_str(), true);

  //some sanity checking of the data supplied?
  uint256 newTX;
  std::vector<unsigned char> data = ParseHex(hexTransaction);
  int rc = ClassB_send(fromAddress, toAddress, redeemAddress, data, newTX, referenceAmount);

  if (0 != rc)
      throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("error code= %i", rc));

  //we need to do better than just returning a string of 0000000 here if we can't send the TX
  return newTX.GetHex();
}

Value getallbalancesforid_MP(const Array& params, bool fHelp)
{
   if (fHelp || params.size() != 1)
        throw runtime_error(
            "getallbalancesforid_MP propertyid\n"
            "\nGet a list of balances for a given currency or property identifier.\n"
            "\nArguments:\n"
            "1. propertyid    (int, required) The property identifier\n"
            "\nResult:\n"
            "{\n"
            "  \"address\" : \"1Address\",  (string) The address\n"
            "  \"balance\" : \"x.xxx\",     (string) The available balance of the address\n"
            "  \"reserved\" : \"x.xxx\",    (string) The amount reserved by sell offers and accepts\n"
            "}\n"

            "\nbExamples\n"
            + HelpExampleCli("getallbalancesforid_MP", "1")
            + HelpExampleRpc("getallbalancesforid_MP", "1")
        );

    int64_t tmpPropertyId = params[0].get_int64();
    if ((1 > tmpPropertyId) || (4294967295 < tmpPropertyId)) // not safe to do conversion
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid property identifier");

    unsigned int propertyId = int(tmpPropertyId);
    CMPSPInfo::Entry sp;
    if (false == _my_sps->getSP(propertyId, sp)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Property identifier does not exist");
    }

    Array response;
    bool divisible = isPropertyDivisible(propertyId); // we want to check this BEFORE the loop

    for(map<string, CMPTally>::iterator my_it = mp_tally_map.begin(); my_it != mp_tally_map.end(); ++my_it)
    {
        unsigned int id;
        bool includeAddress=false;
        string address = (my_it->first).c_str();
        (my_it->second).init();
        while (0 != (id = (my_it->second).next()))
        {
           if(id==propertyId) { includeAddress=true; break; }
        }

        if (!includeAddress) continue; //ignore this address, has never transacted in this propertyId

        Object balance_obj;
        balance_obj.push_back(Pair("address", address));
        BalanceToJSON(address, propertyId, balance_obj, divisible);

        response.push_back(balance_obj);
    }
return response;
}

Value getallbalancesforaddress_MP(const Array& params, bool fHelp)
{
   string address;

   if (fHelp || params.size() != 1)
        throw runtime_error(
            "getallbalancesforaddress_MP \"address\"\n"
            "\nGet a list of all balances for a given address.\n"
            "\nArguments:\n"
            "1. address    (string, required) The address\n"
            "\nResult:\n"
            "{\n"
            "  \"propertyid\" : x,        (numeric) the property id\n"
            "  \"balance\" : \"x.xxx\",     (string) The available balance of the address\n"
            "  \"reserved\" : \"x.xxx\",    (string) The amount reserved by sell offers and accepts\n"
            "}\n"

            "\nbExamples\n"
            + HelpExampleCli("getallbalancesforaddress_MP", "address")
            + HelpExampleRpc("getallbalancesforaddress_MP", "address")
        );

    address = params[0].get_str();
    if (address.empty())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid address");

    Array response;

    CMPTally *addressTally=getTally(address);

    if (NULL == addressTally) // addressTally object does not exist
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Address not found");

    addressTally->init();

    uint64_t propertyId; // avoid issues with json spirit at uint32
    while (0 != (propertyId = addressTally->next()))
    {
        Object balance_obj;
        balance_obj.push_back(Pair("propertyid", propertyId));
        BalanceToJSON(address, propertyId, balance_obj, isPropertyDivisible(propertyId));

        response.push_back(balance_obj);
    }

    return response;
}

Value getproperty_MP(const Array& params, bool fHelp)
{
   if (fHelp || params.size() != 1)
        throw runtime_error(
            "getproperty_MP propertyid\n"
            "\nGet details for a property identifier.\n"
            "\nArguments:\n"
            "1. propertyid    (int, required) The property identifier\n"
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

            "\nbExamples\n"
            + HelpExampleCli("getproperty_MP", "3")
            + HelpExampleRpc("getproperty_MP", "3")
        );

    int64_t tmpPropertyId = params[0].get_int64();
    if ((1 > tmpPropertyId) || (4294967295 < tmpPropertyId)) // not safe to do conversion
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid property identifier");

    unsigned int propertyId = int(tmpPropertyId);
    CMPSPInfo::Entry sp;
    if (false == _my_sps->getSP(propertyId, sp)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Property identifier does not exist");
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

            "\nbExamples\n"
            + HelpExampleCli("listproperties_MP", "")
            + HelpExampleRpc("listproperties_MP", "")
        );

    Array response;

    int64_t propertyId;
    unsigned int nextSPID = _my_sps->peekNextSPID(1);
    for (propertyId = 1; propertyId<nextSPID; propertyId++)
    {
        CMPSPInfo::Entry sp;
        if (false != _my_sps->getSP(propertyId, sp))
        {
            Object property_obj;
            property_obj.push_back(Pair("propertyid", propertyId));
            PropertyToJSON(sp, property_obj); // name, category, subcategory, data, url, divisible

            response.push_back(property_obj);
        }
    }

    unsigned int nextTestSPID = _my_sps->peekNextSPID(2);
    for (propertyId = TEST_ECO_PROPERTY_1; propertyId<nextTestSPID; propertyId++)
    {
        CMPSPInfo::Entry sp;
        if (false != _my_sps->getSP(propertyId, sp))
        {
            Object property_obj;
            property_obj.push_back(Pair("propertyid", propertyId));
            PropertyToJSON(sp, property_obj); // name, category, subcategory, data, url, divisible

            response.push_back(property_obj);
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
            "1. propertyid    (int, required) The property identifier\n"
            "\nResult:\n"
            "{\n"
            "  \"name\" : \"PropertyName\",     (string) the property name\n"
            "  \"active\" : false,     (boolean) whether the crowdsale is active\n"
            "  \"issuer\" : \"1Address\",     (string) the issuer address\n"
            "  \"creationtxid\" : \"txid\",     (string) the transaction that created the crowdsale\n"
            "  \"propertyiddesired\" : x,     (numeric) the property ID desired\n"
            "  \"tokensperunit\" : x,     (numeric) the number of tokens awarded per unit\n"
            "  \"earlybonus\" : x,     (numeric) the percentage per week early bonus applied\n"
            "  \"percenttoissuer\" : x,     (numeric) the percentage awarded to the issuer\n"
            "  \"starttime\" : xxx,     (numeric) the start time of the crowdsale\n"
            "  \"deadline\" : xxx,     (numeric) the time the crowdsale will automatically end\n"
            "  \"closedearly\" : false,     (boolean) whether the crowdsale was ended early\n"
            "  \"maxtokens\" : false,     (boolean) whether the crowdsale was ended early due to maxing the token count\n"
            "  \"endedtime\" : xxx,     (numeric) the time the crowdsale ended\n"
            "  \"closetx\" : \"txid\",     (string) the transaction that closed the crowdsale\n"
            "}\n"

            "\nbExamples\n"
            + HelpExampleCli("getcrowdsale_MP", "3")
            + HelpExampleRpc("getcrowdsale_MP", "3")
        );

    int64_t tmpPropertyId = params[0].get_int64();
    if ((1 > tmpPropertyId) || (4294967295 < tmpPropertyId)) // not safe to do conversion
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid property identifier");

    unsigned int propertyId = int(tmpPropertyId);
    CMPSPInfo::Entry sp;
    if (false == _my_sps->getSP(propertyId, sp)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Property identifier does not exist");
    }

    bool showVerbose = false;
    if (params.size() > 1) showVerbose = params[1].get_bool();

    bool fixedIssuance = sp.fixed;
    bool manualIssuance = sp.manual;
    if (fixedIssuance || manualIssuance) // property was not a variable issuance
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Property was not created with a crowdsale");

    uint256 creationHash = sp.txid;

    CTransaction tx;
    uint256 hashBlock = 0;
    if (!GetTransaction(creationHash, tx, hashBlock, true))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available about transaction");

    if ((0 == hashBlock) || (NULL == GetBlockIndex(hashBlock)))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unconfirmed transactions are not supported");

    Object response;

    bool active = false;
    active = isCrowdsaleActive(propertyId);
    string propertyName = sp.name;
    int64_t startTime = GetBlockIndex(hashBlock)->nTime;
    int64_t deadline = sp.deadline;
    uint8_t earlyBonus = sp.early_bird;
    uint8_t percentToIssuer = sp.percentage;
    bool closeEarly = sp.close_early;
    bool maxTokens = sp.max_tokens;
    int64_t timeClosed = sp.timeclosed;
    string txidClosed = sp.txid_close.GetHex();
    string issuer = sp.issuer;
    int64_t amountRaised = 0;
    int64_t tokensIssued = getTotalTokens(propertyId);
    int64_t missedTokens = sp.missedTokens;
    int64_t tokensPerUnit = sp.num_tokens;
    int64_t propertyIdDesired = sp.property_desired;
    std::map<std::string, std::vector<uint64_t> > database;

    if (active)
    {
          bool crowdFound = false;
          for(CrowdMap::const_iterator it = my_crowds.begin(); it != my_crowds.end(); ++it)
          {
              CMPCrowd crowd = it->second;
              int64_t tmpPropertyId = crowd.getPropertyId();
              if (tmpPropertyId == propertyId)
              {
                  crowdFound = true;
                  database = crowd.getDatabase();
              }
          }
          if (!crowdFound)
                  throw JSONRPCError(RPC_INTERNAL_ERROR, "Crowdsale is flagged active but cannot be retrieved");
    }
    else
    {
        database = sp.historicalData;
    }

    file_log("SIZE OF DB %lu\n", sp.historicalData.size() );
    //bool closedEarly = false; //this needs to wait for dead crowdsale persistence
    //int64_t endedTime = 0; //this needs to wait for dead crowdsale persistence

    Array participanttxs;
    std::map<std::string, std::vector<uint64_t> >::const_iterator it;
    for(it = database.begin(); it != database.end(); it++)
    {
        Object participanttx;

        string txid = it->first; //uint256 txid = it->first;
        int64_t userTokens = it->second.at(2);
        int64_t issuerTokens = it->second.at(3);
        int64_t amountSent = it->second.at(0);

        amountRaised += amountSent;
        participanttx.push_back(Pair("txid", txid)); //.GetHex()).c_str();
        participanttx.push_back(Pair("amountsent", FormatMP(propertyIdDesired, amountSent)));
        participanttx.push_back(Pair("participanttokens", FormatMP(propertyId, userTokens)));
        participanttx.push_back(Pair("issuertokens", FormatMP(propertyId, issuerTokens)));        
        participanttxs.push_back(participanttx);
    }

    response.push_back(Pair("name", propertyName));
    response.push_back(Pair("active", active));
    response.push_back(Pair("issuer", issuer));
    response.push_back(Pair("propertyiddesired", propertyIdDesired));
    response.push_back(Pair("tokensperunit", FormatMP(propertyId, tokensPerUnit)));
    response.push_back(Pair("earlybonus", earlyBonus));
    response.push_back(Pair("percenttoissuer", percentToIssuer));
    response.push_back(Pair("starttime", startTime));
    response.push_back(Pair("deadline", deadline));
    response.push_back(Pair("amountraised", FormatMP(propertyIdDesired, amountRaised)));
    response.push_back(Pair("tokensissued", FormatMP(propertyId, tokensIssued)));
    response.push_back(Pair("addedissuertokens", FormatMP(propertyId, missedTokens)));

    if (!active) response.push_back(Pair("closedearly", closeEarly));
    if (!active) response.push_back(Pair("maxtokens", maxTokens));
    if (closeEarly) response.push_back(Pair("endedtime", timeClosed));
    if (closeEarly && !maxTokens) response.push_back(Pair("closetx", txidClosed));
    
    // array of txids contributing to crowdsale here if needed
    if (showVerbose)
    {
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
            "  \"propertyiddesired\" : x,     (numeric) the property ID desired\n"
            "  \"tokensperunit\" : x,     (numeric) the number of tokens awarded per unit\n"
            "  \"earlybonus\" : x,     (numeric) the percentage per week early bonus applied\n"
            "  \"percenttoissuer\" : x,     (numeric) the percentage awarded to the issuer\n"
            "  \"starttime\" : xxx,     (numeric) the start time of the crowdsale\n"
            "  \"deadline\" : xxx,     (numeric) the time the crowdsale will automatically end\n"
            "}\n"

            "\nbExamples\n"
            + HelpExampleCli("getactivecrowdsales_MP", "")
            + HelpExampleRpc("getactivecrowdsales_MP", "")
        );

      Array response;

      for(CrowdMap::const_iterator it = my_crowds.begin(); it != my_crowds.end(); ++it)
      {
          CMPCrowd crowd = it->second;
          CMPSPInfo::Entry sp;
          bool spFound = _my_sps->getSP(crowd.getPropertyId(), sp);
          int64_t propertyId = crowd.getPropertyId();
          if (spFound)
          {
              Object responseObj;

              uint256 creationHash = sp.txid;

              CTransaction tx;
              uint256 hashBlock = 0;
              if (!GetTransaction(creationHash, tx, hashBlock, true))
                  throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available about transaction");

              if ((0 == hashBlock) || (NULL == GetBlockIndex(hashBlock)))
                  throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unconfirmed transactions are not supported");

              string propertyName = sp.name;
              int64_t startTime = GetBlockIndex(hashBlock)->nTime;
              int64_t deadline = sp.deadline;
              uint8_t earlyBonus = sp.early_bird;
              uint8_t percentToIssuer = sp.percentage;
              string issuer = sp.issuer;
              int64_t tokensPerUnit = sp.num_tokens;
              int64_t propertyIdDesired = sp.property_desired;

              responseObj.push_back(Pair("propertyid", propertyId));
              responseObj.push_back(Pair("name", propertyName));
              responseObj.push_back(Pair("issuer", issuer));
              responseObj.push_back(Pair("propertyiddesired", propertyIdDesired));
              responseObj.push_back(Pair("tokensperunit", FormatMP(propertyId, tokensPerUnit)));
              responseObj.push_back(Pair("earlybonus", earlyBonus));
              responseObj.push_back(Pair("percenttoissuer", percentToIssuer));
              responseObj.push_back(Pair("starttime", startTime));
              responseObj.push_back(Pair("deadline", deadline));

              response.push_back(responseObj);
          }
      }

return response;
}

Value getgrants_MP(const Array& params, bool fHelp)
{
   if (fHelp || params.size() != 1 )
        throw runtime_error(
            "getgrants_MP propertyid\n"
            "\nGet information about grants and revokes for a property identifier.\n"
            "\nArguments:\n"
            "1. propertyid    (int, required) The property identifier\n"
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

            "\nbExamples\n"
            + HelpExampleCli("getgrants_MP", "3")
            + HelpExampleRpc("getgrants_MP", "3")
        );

    int64_t tmpPropertyId = params[0].get_int64();
    if ((1 > tmpPropertyId) || (4294967295 < tmpPropertyId)) // not safe to do conversion
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid property identifier");

    unsigned int propertyId = int(tmpPropertyId);
    CMPSPInfo::Entry sp;
    if (false == _my_sps->getSP(propertyId, sp)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Property identifier does not exist");
    }

    bool fixedIssuance = sp.fixed;
    bool manualIssuance = sp.manual;
    if (fixedIssuance || !manualIssuance) // property was not a manual issuance
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Property was not created with a manual issuance");

    uint256 creationHash = sp.txid;

    Object response;

//    bool active = isCrowdsaleActive(propertyId);  // UNUSED WARNING
//    bool divisible = sp.isDivisible(); // UNUSED WARNING
    string propertyName = sp.name;
    string issuer = sp.issuer;
    int64_t totalTokens = getTotalTokens(propertyId);

    Array issuancetxs;
    std::map<std::string, std::vector<uint64_t> >::const_iterator it;
    for(it = sp.historicalData.begin(); it != sp.historicalData.end(); it++)
    {

        string txid = it->first; //uint256 txid = it->first;
        int64_t grantedTokens = it->second.at(0);
        int64_t revokedTokens = it->second.at(1);
        if (grantedTokens > 0){
          Object granttx;
          granttx.push_back(Pair("txid", txid));
          granttx.push_back(Pair("grant", FormatMP(propertyId, grantedTokens)));
          issuancetxs.push_back(granttx);
        }

        if (revokedTokens > 0){
          Object revoketx;
          revoketx.push_back(Pair("txid", txid));
          revoketx.push_back(Pair("revoke", FormatMP(propertyId, revokedTokens)));
          issuancetxs.push_back(revoketx);
        }
    }

    response.push_back(Pair("name", propertyName));
    response.push_back(Pair("issuer", issuer));
    response.push_back(Pair("creationtxid", creationHash.GetHex()));
    response.push_back(Pair("totaltokens", FormatMP(propertyId, totalTokens)));
    response.push_back(Pair("issuances", issuancetxs));
    return response;
}

int check_prop_valid(int64_t tmpPropId, string error, string exist_error ) {
  CMPSPInfo::Entry sp;

  if ((1 > tmpPropId) || (4294967295 < tmpPropId)) 
    throw JSONRPCError(RPC_INVALID_PARAMETER, error);
  if (false == _my_sps->getSP(tmpPropId, sp)) 
    throw JSONRPCError(RPC_INVALID_PARAMETER, exist_error);

  return tmpPropId;
}

Value trade_MP(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 6)
        throw runtime_error(
            "trade_MP \"address\" \"amountforsale\" propertyid1 \"amountdesired\" propertid2 action ( \"redeemaddress\" )\n"
            "\nPlace or cancel an offer on the distributed token exchange.\n"

            "\nArguments:\n"
            "1. address           (string, required) address that is making the sale\n"
            "2. amount_for_sale   (string, required) amount owned to put up on sale\n"
            "3. property_id1      (int, required) property owned to put up on sale\n"
            "4. amount_desired    (string, required) amount wanted/willing to purchase\n"
            "5. property_id2      (int, required) property wanted/willing to purchase\n"
            "6. action            (int, required) decision to either start a new (1), cancel_price (2), cancel_pair (3), or cancel_all (4) for an offer\n"
            "7. redeem_address    (optional) the address that can redeem the bitcoin outputs, defaults to sender\n"
        );

    std::string fromAddress = params[0].get_str();
    std::string redeemAddress = (params.size() > 6) ? (params[6].get_str()) : "";

    uint32_t propertyIdSale = static_cast<uint32_t>(params[2].get_int64());
    uint32_t propertyIdWant = static_cast<uint32_t>(params[4].get_int64());

    int64_t amountSale = 0;
    int64_t amountWant = 0;
    int64_t action = params[5].get_int64();
    
    if (action <= 0 || CMPTransaction::CANCEL_EVERYTHING < action)
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid action (1, 2, 3, 4 only)");

    // CANCEL-EVERYTHING doesn't require additional checks or setup
    // CANCEL-AT-PRICE and CANCEL-ALL-FOR-PAIR have property values
    if (action <= CMPTransaction::CANCEL_ALL_FOR_PAIR)
    {
        check_prop_valid(propertyIdSale,
                "Invalid property identifier (Sale)",
                "Property identifier does not exist (Sale)");

        check_prop_valid(propertyIdWant,
                "Invalid property identifier (Want)",
                "Property identifier does not exist (Want)");

        if (isTestEcosystemProperty(propertyIdSale) != isTestEcosystemProperty(propertyIdWant))
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Properties must be in the same ecosystem (Main/Test)");

        if (propertyIdSale == propertyIdWant)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Properties must be different");
    }

    // CANCEL-AT-PRICE has also price information
    if (action <= CMPTransaction::CANCEL_AT_PRICE)
    {
        CMPSPInfo::Entry sp;
        bool divisible_sale = false, divisible_want = false;

        _my_sps->getSP(propertyIdSale, sp);
        divisible_sale = sp.isDivisible();

        _my_sps->getSP(propertyIdWant, sp);
        divisible_want = sp.isDivisible();

        std::string strAmountSale = params[1].get_str();        
        amountSale = StrToInt64(strAmountSale, divisible_sale);

        std::string strAmountWant = params[3].get_str();        
        amountWant = StrToInt64(strAmountWant, divisible_want);

        if (0 >= amountSale)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount (Sale)");

        if (0 >= amountWant)
            throw JSONRPCError(RPC_TYPE_ERROR, "Invalid amount (Want)");
    }

    int code = 0;    
    uint256 newTX = send_INTERNAL_1packet(fromAddress, "", redeemAddress, propertyIdSale, amountSale, propertyIdWant, amountWant, MSC_TYPE_METADEX, action, &code);
    if (0 != code) throw JSONRPCError(code, error_str(code));

    // we need to do better than just returning a string of 0000000 here if we can't send the TX
    return newTX.GetHex();
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

  uint64_t timestamp = (params.size() > 0) ? params[0].get_int64() : GetLatestBlockTime() - 1209600; //2 weeks 

  bool filter_by_one = (params.size() > 1) ? true : false;
  bool filter_by_both = (params.size() > 2) ? true : false;

  if( filter_by_one ) {
    propertyIdSaleFilter = check_prop_valid( params[1].get_int64() , "Invalid property identifier (Sale)", "Property identifier does not exist (Sale)"); 
  }

  if ( filter_by_both ) {
    propertyIdWantFilter = check_prop_valid( params[2].get_int64() , "Invalid property identifier (Want)", "Property identifier does not exist (Want)"); 
  }
  
  std::vector<CMPMetaDEx> vMetaDexObjects;

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
  
  Array response;
  MetaDexObjectsToJSON(vMetaDexObjects, response);
  
  return response;
}

Value getactivedexsells_MP(const Array& params, bool fHelp)
{
   if (fHelp)
        throw runtime_error(
            "getactivedexsells_MP\n"
            "\nGet currently active offers on the distributed exchange.\n"
            "\nResult:\n"
            "{\n"
            "}\n"

            "\nbExamples\n"
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

  // firstly let's get the block height given in the param
  int blockHeight = params[0].get_int();
  if (blockHeight < 0 || blockHeight > GetHeight())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Block height out of range");

  // next let's obtain the block for this height
  CBlockIndex* mpBlockIndex = chainActive[blockHeight];
  CBlock mpBlock;

  // now let's read this block in from disk so we can loop its transactions
  if(!ReadBlockFromDisk(mpBlock, mpBlockIndex))
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Failed to read block from disk");

  // create an array to hold our response
  Array response;

  // now we want to loop through each of the transactions in the block and run against CMPTxList::exists
  // those that return positive add to our response array
  BOOST_FOREACH(const CTransaction&tx, mpBlock.vtx)
  {
       bool mptx = p_txlistdb->exists(tx.GetHash());
       if (mptx)
       {
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
    if (p_txlistdb->exists(wtxid))
    {
        //transaction is in levelDB, so we can attempt to parse it
        int parseRC = parseTransaction(true, wtx, blockHeight, 0, &mp_obj);
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
                p_txlistdb->getPurchaseDetails(wtxid,1,&tmpBuyer,&tmpSeller,&tmpVout,&tmpPropertyId,&tmpNValue);
                txobj->push_back(Pair("sendingaddress", tmpBuyer));
                // get the details of sub records for payment(s) in the tx and push into an array
                Array purchases;
                int numberOfPurchases=p_txlistdb->getNumberOfPurchases(wtxid);
                if (0<numberOfPurchases)
                {
                    for(int purchaseNumber = 1; purchaseNumber <= numberOfPurchases; purchaseNumber++)
                    {
                        Object purchaseObj;
                        string buyer;
                        string seller;
                        uint64_t vout;
                        uint64_t nValue;
                        p_txlistdb->getPurchaseDetails(wtxid,purchaseNumber,&buyer,&seller,&vout,&propertyId,&nValue);
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
                    valid=getValidMPTX(wtxid, &tmpblock, &tmptype, &amountNew);
                    //populate based on type of tx
                    switch (MPTxTypeInt)
                    {
                        case MSC_TYPE_METADEX:
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
                                     XDOUBLE tempUnitPrice = 0;
                                     if ((propertyId == OMNI_PROPERTY_MSC) || (propertyId == OMNI_PROPERTY_TMSC)) {
                                         tempUnitPrice = temp_metadexoffer.inversePrice();
                                         if (!mdex_propertyWanted_Div) tempUnitPrice = tempUnitPrice/COIN;
                                     } else {
                                         tempUnitPrice = temp_metadexoffer.effectivePrice();
                                         if (!mdex_propertyId_Div) tempUnitPrice = tempUnitPrice/COIN;
                                     }
                                     mdex_unitPriceStr = tempUnitPrice.str(DISPLAY_PRECISION_LEN, std::ios_base::fixed);
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
                                propertyId = _my_sps->findSPByTX(wtxid); // propertyId of created property (if valid)
                                amount = getTotalTokens(propertyId);
                                propertyName = mp_obj.getSPName();
                            }
                        break;
                        case MSC_TYPE_CREATE_PROPERTY_VARIABLE:
                            mp_obj.step2_SmartProperty(rc);
                            if (0 == rc)
                            {
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
        if (MSC_TYPE_METADEX != MPTxTypeInt) txobj->push_back(Pair("propertyid", propertyId));
        if ((MSC_TYPE_CREATE_PROPERTY_VARIABLE == MPTxTypeInt) || (MSC_TYPE_CREATE_PROPERTY_FIXED == MPTxTypeInt) || (MSC_TYPE_CREATE_PROPERTY_MANUAL == MPTxTypeInt))
        {
            txobj->push_back(Pair("propertyname", propertyName));
        }
        if (MSC_TYPE_METADEX != MPTxTypeInt) txobj->push_back(Pair("divisible", divisible));
        if (MSC_TYPE_METADEX != MPTxTypeInt) txobj->push_back(Pair("amount", FormatMP(propertyId, amount)));
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
        if (MSC_TYPE_METADEX == MPTxTypeInt)
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
    // get STO receipts affecting me
    string mySTOReceipts = s_stolistdb->getMySTOReceipts(addressParam);
    std::vector<std::string> vecReceipts;
    boost::split(vecReceipts, mySTOReceipts, boost::is_any_of(","), token_compress_on);
    int64_t lastTXBlock = 999999;

    // rewrite to use original listtransactions methodology from core
    LOCK(wallet->cs_wallet);
    std::list<CAccountingEntry> acentries;
    CWallet::TxItems txOrdered = pwalletMain->OrderedTxItems(acentries, "*");

    // iterate backwards until we have nCount items to return:
    for (CWallet::TxItems::reverse_iterator it = txOrdered.rbegin(); it != txOrdered.rend(); ++it)
    {
        CWalletTx *const pwtx = (*it).second.first;
        if (pwtx != 0)
        {
            // get the height of the transaction and check it's within the chosen parameters
            uint256 blockHash = pwtx->hashBlock;
            if ((0 == blockHash) || (NULL == GetBlockIndex(blockHash))) continue;
            CBlockIndex* pBlockIndex = GetBlockIndex(blockHash);
            if (NULL == pBlockIndex) continue;
            int blockHeight = pBlockIndex->nHeight;
            if ((blockHeight < nStartBlock) || (blockHeight > nEndBlock)) continue; // ignore it if not within our range

            // look for an STO receipt to see if we need to insert it
            for(uint32_t i = 0; i<vecReceipts.size(); i++)
            {
                std::vector<std::string> svstr;
                boost::split(svstr, vecReceipts[i], boost::is_any_of(":"), token_compress_on);
                if(4 == svstr.size()) // make sure expected num items
                {
                    if((atoi(svstr[1]) < lastTXBlock) && (atoi(svstr[1]) > blockHeight))
                    {
                        // STO receipt insert here - add STO receipt to response array
                        uint256 hash;
                        hash.SetHex(svstr[0]);
                        Object txobj;
                        int populateResult = -1;
                        populateResult = populateRPCTransactionObject(hash, &txobj);
                        if (0 == populateResult)
                        {
                            Array receiveArray;
                            uint64_t tmpAmount = 0;
                            uint64_t stoFee = 0;
                            s_stolistdb->getRecipients(hash, addressParam, &receiveArray, &tmpAmount, &stoFee);
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

            // populateRPCTransactionObject will throw us a non-0 rc if this isn't a MP transaction, speeds up search by orders of magnitude
            uint256 hash = pwtx->GetHash();
            Object txobj;

            // make a request to new RPC populator function to populate a transaction object (if it is a MP transaction)
            int populateResult = -1;
            if (addressFilter)
            {
                populateResult = populateRPCTransactionObject(hash, &txobj, addressParam); // pass in an address filter
            }
            else
            {
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
    Object infoResponse;
    // other bits of info we want to report should be included here

    // provide the mastercore and bitcoin version and if available commit id
    infoResponse.push_back(Pair("mastercoreversion", OmniCoreVersion()));
    infoResponse.push_back(Pair("bitcoincoreversion", BitcoinCoreVersion()));
    infoResponse.push_back(Pair("commitinfo", BuildCommit()));

    // provide the current block details
    uint64_t block = chainActive.Height();
    uint64_t blockTime = chainActive[chainActive.Height()]->GetBlockTime();
    int64_t blockMPTransactions = p_txlistdb->getMPTransactionCountBlock(block);
    int64_t totalMPTransactions = p_txlistdb->getMPTransactionCountTotal();
    int64_t totalMPTrades = t_tradelistdb->getMPTradeCountTotal();
    infoResponse.push_back(Pair("block", block));
    infoResponse.push_back(Pair("blocktime", blockTime));
    infoResponse.push_back(Pair("blocktransactions", blockMPTransactions));

    // provide the number of trades completed
    infoResponse.push_back(Pair("totaltrades", totalMPTrades));
    // provide the number of transactions parsed
    infoResponse.push_back(Pair("totaltransactions", totalMPTransactions));

    // handle alerts
    Object alertResponse;
    string global_alert_message = getMasterCoreAlertString();
    int32_t alertType = 0;
    uint64_t expiryValue = 0;
    uint32_t typeCheck = 0;
    uint32_t verCheck = 0;
    string alertTypeStr;
    string alertMessage;

    if(!global_alert_message.empty())
    {
        bool parseSuccess = parseAlertMessage(global_alert_message, &alertType, &expiryValue, &typeCheck, &verCheck, &alertMessage);
        if (parseSuccess)
        {
            switch (alertType)
            {
                case 0: alertTypeStr = "error"; break;
                case 1: alertTypeStr = "textalertexpiringbyblock"; break;
                case 2: alertTypeStr = "textalertexpiringbytime"; break;
                case 3: alertTypeStr = "textalertexpiringbyversion"; break;
                case 4: alertTypeStr = "updatealerttxcheck"; break;
            }
            alertResponse.push_back(Pair("alerttype", alertTypeStr));
            alertResponse.push_back(Pair("expiryvalue", FormatIndivisibleMP(expiryValue)));
            if (alertType == 4) { alertResponse.push_back(Pair("typecheck",  FormatIndivisibleMP(typeCheck))); alertResponse.push_back(Pair("vercheck",  FormatIndivisibleMP(verCheck))); }
            alertResponse.push_back(Pair("alertmessage", alertMessage.c_str()));
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
    int parseRC = parseTransaction(true, wtx, 0, 0, &mp_obj);
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
        s_stolistdb->getRecipients(hash, filterAddress, &receiveArray, &tmpAmount, &stoFee);
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
    int parseRC = parseTransaction(true, wtx, 0, 0, &mp_obj);
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
    uint64_t amountForSale = mp_obj.getAmount();

    // create array of matches
    Array tradeArray;
    uint64_t totalBought = 0;
    uint64_t totalSold = 0;
    t_tradelistdb->getMatchingTrades(hash, propertyId, &tradeArray, &totalSold, &totalBought);

    // get action byte
    int actionByte = 0;
    if (0 <= mp_obj.interpretPacket(NULL,&temp_metadexoffer)) { actionByte = (int)temp_metadexoffer.getAction(); }

    // everything seems ok, now add status and add array of matches to the object
    // work out status
    bool orderOpen = isMetaDExOfferActive(hash, propertyId);
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
