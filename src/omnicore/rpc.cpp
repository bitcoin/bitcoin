// RPC calls

#include "omnicore/rpc.h"

#include "omnicore/convert.h"
#include "omnicore/createpayload.h"
#include "omnicore/dex.h"
#include "omnicore/errors.h"
#include "omnicore/log.h"
#include "omnicore/mdex.h"
#include "omnicore/omnicore.h"
#include "omnicore/parse_string.h"
#include "omnicore/rpctx.h"
#include "omnicore/rpctxobject.h"
#include "omnicore/sp.h"
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
            "setautocommit\n"
            "\nSets the global flag that determines whether transactions are automatically committed and broadcast.\n"
            "\nResult:\n"
            "(boolean) The new flag status\n"
        );
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

          PrintToConsole("%34s => ", my_it->first);
          total += (my_it->second).print(extra2, bDivisible);
        }

        PrintToConsole("total for property %d  = %X is %s\n", extra2, extra2, FormatDivisibleMP(total));
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

          PrintToConsole("%34s => ", my_it->first);
          (my_it->second).print(extra2);

          (my_it->second).init();
          while (0 != (id = (my_it->second).next()))
          {
            PrintToConsole("Id: %u=0x%X ", id, id);
          }
          PrintToConsole("\n");
        }
      break;

    case 4:
      for(CrowdMap::const_iterator it = my_crowds.begin(); it != my_crowds.end(); ++it)
      {
        (it->second).print(it->first);
      }
      break;

    case 5:
      PrintToConsole("isMPinBlockRange(%d,%d)=%s\n", extra2, extra3, isMPinBlockRange(extra2, extra3, false) ? "YES":"NO");
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
  string rawHex;
  std::vector<unsigned char> data = ParseHex(hexTransaction);
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
        string address = my_it->first;
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

    PrintToLog("SIZE OF DB %lu\n", sp.historicalData.size() );
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

Value getorderbook_MP(const Array& params, bool fHelp)
{
   if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "getorderbook_MP property_id1 ( property_id2 )\n"
            "\nRequest active trade information from the MetaDEx\n"

            "\nArguments:\n"
            "1. propertyid           (int, required) filter orders on propertyid for sale\n"
            "2. propertyid           (int, optional) filter orders on propertyid desired\n"
        );

  uint32_t propertyIdForSale = 0, propertyIdDesired = 0;
  bool filterDesired = (params.size() == 2) ? true : false;

  propertyIdForSale = check_prop_valid(params[0].get_int64(), "Invalid property identifier (for sale)", "Property identifier does not exist (for sale)");
  if (filterDesired) {
    propertyIdDesired = check_prop_valid(params[1].get_int64(), "Invalid property identifier (desired)", "Property identifier does not exist (desired)");
  }

  std::vector<CMPMetaDEx> vecMetaDexObjects;
  for (md_PropertiesMap::iterator my_it = metadex.begin(); my_it != metadex.end(); ++my_it) {
      md_PricesMap & prices = my_it->second;
      for (md_PricesMap::iterator it = prices.begin(); it != prices.end(); ++it) {
          md_Set & indexes = (it->second);
          for (md_Set::iterator it = indexes.begin(); it != indexes.end(); ++it) {
              CMPMetaDEx obj = *it;
              if (obj.getProperty() == propertyIdForSale) {
                  if (!filterDesired || (obj.getProperty() == propertyIdForSale && obj.getDesProperty() == propertyIdDesired)) {
                      vecMetaDexObjects.push_back(obj);
                  }
              }
          }
      }
  }

  Array response;
  MetaDexObjectsToJSON(vecMetaDexObjects, response);
  return response;
}

// DISCUSS: assume purpose of this call is to allow requesting trades since last polled
//          is this necessary or wanted?
//          Additionally use of a blocktime is not advisable since blocktimes are non-sequential
// SUGGEST: this call is removed completely, and if the functionality is required we simply append an optional
//          block height parameter to getorderbook_MP since this call does same thing
Value gettradessince_MP(const Array& params, bool fHelp)
{
   if (fHelp)
        throw runtime_error(
            "gettradessince_MP\n"
            "\nRequest last known orders from order book\n"
            
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

/* TODO: point for discussion - "trade history" is ambiguous; providing trade history for a
 * pair is likely to be more useful than trade history for an address.  Regardless the existing
 * function did neither (only showing open trades, not trade history) so has been rewritten.
 *
 * Perhaps consider renaming this to gettradehistoryforaddress_OMNI to match existing naming
 * (eg getallbalancesforaddress_MP) then look at addition of new gettradehistoryforpair_OMNI
 * call.
 */
Value gettradehistoryforaddress_OMNI(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 3)
        throw runtime_error(
            "gettradehistory_MP\n"
            "\nAllows user to retrieve MetaDEx trade history for the supplied address\n"
            "\nArguments:\n"
            "1. address          (string, required) address to retrieve history for\n"
            "2. count            (int, optional) number of trades to retrieve (default: 10)\n"
            "3. propertyid       (int, optional) filter by propertyid transacted\n"
        );

    Array response;

    string address = params[0].get_str();
    int64_t count = 10;
    if (params.size() > 1) {
        count = params[1].get_int64();
        if (count < 1) // TODO: do we need a maximum here?
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid value for count parameter");
    }
    uint32_t propertyId = 0;
    if (params.size() > 2) {
        int64_t tmpPropertyId = params[2].get_int64();
        if (tmpPropertyId < 1 || tmpPropertyId > 4294967295) // not safe to do conversion
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid property identifier");
        propertyId = boost::lexical_cast<uint32_t>(tmpPropertyId);
        CMPSPInfo::Entry sp;
        if (false == _my_sps->getSP(propertyId, sp))
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Property identifier does not exist");
    }

    // to provide a proper trade history for an address we must utilize a two part methodology:
    // 1) check the metadex maps for open trades
    // 2) check the trades database for closed trades

    // vector vecTransactions will hold the txids from both stage 1 and stage 2 to avoid duplicates
    std::vector<uint256> vecTransactions;

    // stage 1
    for (md_PropertiesMap::iterator my_it = metadex.begin(); my_it != metadex.end(); ++my_it) {
        md_PricesMap & prices = my_it->second;
        for (md_PricesMap::iterator it = prices.begin(); it != prices.end(); ++it) {
            md_Set & indexes = (it->second);
            for (md_Set::iterator it = indexes.begin(); it != indexes.end(); ++it) {
                CMPMetaDEx obj = *it;
                if (obj.getAddr() == address) {
                    if (propertyId != 0 && propertyId != obj.getProperty() && propertyId != obj.getDesProperty()) continue;
                    vecTransactions.push_back(obj.getHash());
                }
            }
        }
    }

    // stage 2
    t_tradelistdb->getTradesForAddress(address, &vecTransactions, propertyId);

    // populate
    for(std::vector<uint256>::iterator it = vecTransactions.begin(); it != vecTransactions.end(); ++it) {
        Object txobj;
        int populateResult = populateRPCTransactionObject(*it, txobj, "", true);
        if (0 == populateResult) response.push_back(txobj);
    }

    // TODO: sort response array on confirmations attribute and crop response to (count) most recent transactions
    return response;
}

Value gettradehistoryformarket_OMNI(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "gettradehistory_MP\n"
            "\nAllows user to retrieve MetaDEx trade history for the specified market\n"
            "\nArguments:\n"
            "1. propertyid           (int, required) metadex market\n"
            "3. count                (int, optional) number of trades to retrieve (default: 10)\n"
        );

    Array response;

    uint32_t market = 0;
    int64_t tmpMarket = params[0].get_int64();

    if (tmpMarket < 1 || tmpMarket > 4294967295)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid property identifier");
    market = boost::lexical_cast<uint32_t>(tmpMarket);

    if (!_my_sps->hasSP(market))
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Specified market does not exist");

    int64_t count = 10;
    if (params.size() > 2) count = params[2].get_int64();
    if (count < 1) // TODO: do we need a maximum here?
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid value for count parameter");

    // metadex maps are irrelevant for this call, we are looking for completed trades which are stored in tradelistdb

    // this call will apply the concept of "market", since one side of the pair must always be MSC/TMSC we can simplify with buy/sell concepts
    // "buyer" is always considered the party that is selling MSC
    // "seller" is always considered the party that is selling SPT
    t_tradelistdb->getTradesForMarket(market, response);

    // TODO: sort response array on confirmations attribute and crop response to (count) most recent transactions
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
    int populateResult = populateRPCTransactionObject(hash, txobj);
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

    // if 0 params consider all addresses in wallet, otherwise first param is filter address
    addressFilter = false;
    if (params.size() > 0) {
        // allow setting "" or "*" to use nCount and nFrom params with all addresses in wallet
        if ( ("*" != params[0].get_str()) && ("" != params[0].get_str()) ) {
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

            // ### START STO CHECK ### (was a STO received between last and next wallet transactions)
            for(uint32_t i = 0; i<vecReceipts.size(); i++) {
                std::vector<std::string> svstr;
                boost::split(svstr, vecReceipts[i], boost::is_any_of(":"), token_compress_on);
                if (svstr.size() != 4) {
                    PrintToLog("STODB Error - number of tokens is not as expected (%s)\n", vecReceipts[i]);
                    continue;
                }
                if (atoi(svstr[1]) < lastTXBlock && atoi(svstr[1]) > blockHeight) { // STO receipt found - insert into response
                    uint256 hash;
                    hash.SetHex(svstr[0]);
                    Object txobj;
                    int populateResult = -1;
                    populateResult = populateRPCTransactionObject(hash, txobj); // don't spam listtransactions output with STO extended details
                    if (0 == populateResult) response.push_back(txobj);
                }
            }
            // ### END STO CHECK ###

            // populateRPCTransactionObject will throw us a non-0 rc if this isn't a MP transaction, speeds up search by orders of magnitude
            uint256 hash = pwtx->GetHash();
            Object txobj;

            // make a request to new RPC populator function to populate a transaction object (if it is a MP transaction)
            int populateResult = -1;
            if (addressFilter) {
                populateResult = populateRPCTransactionObject(hash, txobj, addressParam); // pass in an address filter
            } else {
                populateResult = populateRPCTransactionObject(hash, txobj); // no address filter
            }
            if (0 == populateResult) response.push_back(txobj); // add the transaction object to the response array if we get a 0 rc
            lastTXBlock = blockHeight;

            // don't burn time doing more work than we need to
            if ((int)response.size() >= (nCount+nFrom)) break;
        }
    }

    // sort array here and cut on nFrom and nCount
    if (nFrom > (int)response.size()) nFrom = response.size();
    if ((nFrom + nCount) > (int)response.size()) nCount = response.size() - nFrom;
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

    Object txobj;
    uint256 hash;
    hash.SetHex(params[0].get_str());
    string filterAddress = "";
    if (params.size() == 2) filterAddress=params[1].get_str();

    // make a request to RPC populator function to populate a transaction object
    int populateResult = populateRPCTransactionObject(hash, txobj, "", true, filterAddress);
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

    uint256 hash;
    hash.SetHex(params[0].get_str());
    Object txobj;

    // make a request to RPC populator function to populate a transaction object
    int populateResult = populateRPCTransactionObject(hash, txobj, "", true);
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

    // return object
    return txobj;
}

