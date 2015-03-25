//
// first & so far only Master protocol source file
// WARNING: Work In Progress -- major refactoring will be occurring often
//
// I am adding comments to aid with navigation and overall understanding of the design.
// this is the 'core' portion of the node+wallet: mastercored
// see 'qt' subdirectory for UI files
//
// remaining work, search for: TODO, FIXME
//

//
// global TODO: need locks on the maps in this file & balances (moneys[],reserved[] & raccept[]) !!!
//

#include "mastercore.h"

#include "mastercore_convert.h"
#include "mastercore_dex.h"
#include "mastercore_errors.h"
#include "mastercore_script.h"
#include "mastercore_sp.h"
#include "mastercore_tx.h"
#include "mastercore_version.h"

#include "base58.h"
#include "chainparams.h"
#include "coincontrol.h"
#include "init.h"
#include "primitives/block.h"
#include "primitives/transaction.h"
#include "script/script.h"
#include "script/standard.h"
#include "sync.h"
#include "tinyformat.h"
#include "uint256.h"
#include "util.h"
#include "utilstrencodings.h"
#include "utiltime.h"
#include "wallet.h"

#include <boost/algorithm/string.hpp>
#include <boost/exception/to_string.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/thread/mutex.hpp>

#include <openssl/sha.h>

#include "json/json_spirit_value.h"
#include "json/json_spirit_writer_template.h"

#include "leveldb/db.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>

using boost::algorithm::token_compress_on;
using boost::multiprecision::cpp_int;
using boost::multiprecision::int128_t;
using boost::to_string;

using json_spirit::Array;
using json_spirit::Pair;
using json_spirit::Object;
using json_spirit::write_string;

using leveldb::Iterator;
using leveldb::Slice;
using leveldb::Status;

using std::make_pair;
using std::map;
using std::ofstream;
using std::string;
using std::vector;

using namespace mastercore;


// comment out MY_HACK & others here - used for Unit Testing only !
// #define MY_HACK

static string exodus_address = "1EXoDusjGwvnjZUyKkxZ4UHEf77z6A5S4P";
static const string exodus_testnet = "mpexoDuSkGGqvqrkrjiFng38QPkJQVFyqv";
static const string getmoney_testnet = "moneyqMan7uh8FqdCA2BV5yZ8qVrc9ikLP";

// part of 'breakout' feature
static const int nBlockTop = 0;
// static const int nBlockTop = 271000;

static int nWaterlineBlock = 0;  //

uint64_t global_metadex_market;
uint64_t global_balance_money_maineco[100000];
uint64_t global_balance_reserved_maineco[100000];
uint64_t global_balance_money_testeco[100000];
uint64_t global_balance_reserved_testeco[100000];

string global_alert_message;

static uint64_t exodus_prev = 0;
static uint64_t exodus_balance;

static boost::filesystem::path MPPersistencePath;

const int msc_debug_parser_data = 0;
const int msc_debug_parser= 0;
const int msc_debug_verbose=0;
const int msc_debug_verbose2=0;
const int msc_debug_verbose3=0;
const int msc_debug_vin   = 0;
const int msc_debug_script= 0;
const int msc_debug_dex   = 1;
const int msc_debug_send  = 1;
const int msc_debug_tokens= 0;
const int msc_debug_spec  = 1;
const int msc_debug_exo   = 0;
const int msc_debug_tally = 1;
const int msc_debug_sp    = 1;
const int msc_debug_sto   = 1;
const int msc_debug_txdb  = 0;
const int msc_debug_tradedb = 1;
const int msc_debug_persistence = 0;
// disable all flags for metadex for the immediate tag, then turn back on 0 & 2 at least
int msc_debug_metadex1= 0;
int msc_debug_metadex2= 0;
int msc_debug_metadex3= 0;  // enable this to see the orderbook before & after each TX

const static int disable_Divs = 0;
const static int disableLevelDB = 0;

static int mastercoreInitialized = 0;

static int reorgRecoveryMode = 0;
static int reorgRecoveryMaxHeight = 0;

static bool bRawTX = false;

// TODO: there would be a block height for each TX version -- rework together with BLOCKHEIGHTRESTRICTIONS above
static const int txRestrictionsRules[][3] = {
  {MSC_TYPE_SIMPLE_SEND,              GENESIS_BLOCK,      MP_TX_PKT_V0},
  {MSC_TYPE_TRADE_OFFER,              MSC_DEX_BLOCK,      MP_TX_PKT_V1},
  {MSC_TYPE_ACCEPT_OFFER_BTC,         MSC_DEX_BLOCK,      MP_TX_PKT_V0},
  {MSC_TYPE_CREATE_PROPERTY_FIXED,    MSC_SP_BLOCK,       MP_TX_PKT_V0},
  {MSC_TYPE_CREATE_PROPERTY_VARIABLE, MSC_SP_BLOCK,       MP_TX_PKT_V1},
  {MSC_TYPE_CLOSE_CROWDSALE,          MSC_SP_BLOCK,       MP_TX_PKT_V0},
  {MSC_TYPE_SEND_TO_OWNERS,           MSC_STO_BLOCK,      MP_TX_PKT_V0},
  {MSC_TYPE_METADEX,                  MSC_METADEX_BLOCK,  MP_TX_PKT_V0},
  {MSC_TYPE_OFFER_ACCEPT_A_BET,       MSC_BET_BLOCK,      MP_TX_PKT_V0},
  {MSC_TYPE_CREATE_PROPERTY_MANUAL,   MSC_MANUALSP_BLOCK,      MP_TX_PKT_V0},
  {MSC_TYPE_GRANT_PROPERTY_TOKENS,    MSC_MANUALSP_BLOCK,      MP_TX_PKT_V0},
  {MSC_TYPE_REVOKE_PROPERTY_TOKENS,   MSC_MANUALSP_BLOCK,      MP_TX_PKT_V0},
  {MSC_TYPE_CHANGE_ISSUER_ADDRESS,    MSC_MANUALSP_BLOCK,      MP_TX_PKT_V0},

// end of array marker, in addition to sizeof/sizeof
  {-1,-1},
};

CMPTxList *mastercore::p_txlistdb;
CMPTradeList *mastercore::t_tradelistdb;
CMPSTOList *mastercore::s_stolistdb;

// a copy from main.cpp -- unfortunately that one is in a private namespace
int mastercore::GetHeight()
{
  if (0 < nBlockTop) return nBlockTop;

  LOCK(cs_main);
  return chainActive.Height();
}

uint32_t mastercore::GetLatestBlockTime()
{
    LOCK(cs_main);
    if (chainActive.Tip())
        return (int)(chainActive.Tip()->GetBlockTime());
    else
        return (int)(Params().GenesisBlock().nTime); // Genesis block's time of current network
}

CBlockIndex* mastercore::GetBlockIndex(const uint256& hash)
{
    CBlockIndex* pBlockIndex = NULL;
    LOCK(cs_main);
    BlockMap::const_iterator it = mapBlockIndex.find(hash);
    if (it != mapBlockIndex.end()) {
        pBlockIndex = it->second;
    }

    return pBlockIndex;
}


//--- CUT HERE --- mostly copied from util.h & util.cpp
// LogPrintf() has been broken a couple of times now
// by well-meaning people adding mutexes in the most straightforward way.
// It breaks because it may be called by global destructors during shutdown.
// Since the order of destruction of static/global objects is undefined,
// defining a mutex as a global object doesn't work (the mutex gets
// destroyed, and then some later destructor calls OutputDebugStringF,
// maybe indirectly, and you get a core dump at shutdown trying to lock
// the mutex).
static boost::once_flag mp_debugPrintInitFlag = BOOST_ONCE_INIT;
// We use boost::call_once() to make sure these are initialized in
// in a thread-safe manner the first time it is called:
static FILE* fileout = NULL;
static boost::mutex* mutexDebugLog = NULL;

static void mp_DebugPrintInit()
{
    assert(fileout == NULL);
    assert(mutexDebugLog == NULL);

    boost::filesystem::path pathDebug = GetDataDir() / LOG_FILENAME ;
    fileout = fopen(pathDebug.string().c_str(), "a");
    if (fileout) setbuf(fileout, NULL); // unbuffered

    mutexDebugLog = new boost::mutex();
}

int mp_LogPrintStr(const std::string &str)
{
    int ret = 0; // Returns total number of characters written
    if (fPrintToConsole)
    {
        // print to console
        ret = fwrite(str.data(), 1, str.size(), stdout);
    }
    else if (fPrintToDebugLog)
    {
        static bool fStartedNewLine = true;
        boost::call_once(&mp_DebugPrintInit, mp_debugPrintInitFlag);

        if (fileout == NULL)
            return ret;

        boost::mutex::scoped_lock scoped_lock(*mutexDebugLog);

        // reopen the log file, if requested
        if (fReopenDebugLog) {
            fReopenDebugLog = false;
            boost::filesystem::path pathDebug = GetDataDir() / LOG_FILENAME ;
            if (freopen(pathDebug.string().c_str(),"a",fileout) != NULL)
                setbuf(fileout, NULL); // unbuffered
        }

        // Debug print useful for profiling
        if (fLogTimestamps && fStartedNewLine)
            ret += fprintf(fileout, "%s ", DateTimeStrFormat("%Y-%m-%d %H:%M:%S", GetTime()).c_str());
        if (!str.empty() && str[str.size()-1] == '\n')
            fStartedNewLine = true;
        else
            fStartedNewLine = false;

        ret = fwrite(str.data(), 1, str.size(), fileout);
    }

    return ret;
}

// indicate whether persistence is enabled at this point, or not
// used to write/read files, for breakout mode, debugging, etc.
static bool readPersistence()
{
#ifdef  MY_HACK
  return false;
#else
  return true;
#endif
}

// indicate whether persistence is enabled at this point, or not
// used to write/read files, for breakout mode, debugging, etc.
static bool writePersistence(int block_now)
{
  // if too far away from the top -- do not write
  if (GetHeight() > (block_now + MAX_STATE_HISTORY)) return false;

  return true;
}

// copied from ShrinkDebugFile, util.cpp
static void shrinkDebugFile()
{
    // Scroll log if it's getting too big
    const int buffer_size = 8000000;  // 8MBytes
    boost::filesystem::path pathLog = GetDataDir() / LOG_FILENAME;
    FILE* file = fopen(pathLog.string().c_str(), "r");

    if (file && boost::filesystem::file_size(pathLog) > 50000000) // 50MBytes
    {
      // Restart the file with some of the end
      char *pch = new char[buffer_size];
      if (NULL != pch)
      {
        fseek(file, -buffer_size, SEEK_END);
        int nBytes = fread(pch, 1, buffer_size, file);
        fclose(file); file = NULL;

        file = fopen(pathLog.string().c_str(), "w");
        if (file)
        {
            fwrite(pch, 1, nBytes, file);
            fclose(file); file = NULL;
        }
        delete [] pch;
      }
    }
    else
    {
      if (NULL != file) fclose(file);
    }
}

string mastercore::strMPProperty(unsigned int i)
{
string str = "*unknown*";

  // test user-token
  if (0x80000000 & i)
  {
    str = strprintf("Test token: %d : 0x%08X", 0x7FFFFFFF & i, i);
  }
  else
  switch (i)
  {
    case OMNI_PROPERTY_BTC: str = "BTC"; break;
    case OMNI_PROPERTY_MSC: str = "MSC"; break;
    case OMNI_PROPERTY_TMSC: str = "TMSC"; break;
    default: str = strprintf("SP token: %d", i);
  }

  return str;
}

inline bool isNonMainNet()
{
    return Params().NetworkIDString() != "main";
}

inline bool TestNet()
{
    return Params().NetworkIDString() == "test";
}

inline bool RegTest()
{
    return Params().NetworkIDString() == "regtest";
}

string FormatPriceMP(double n)
{
  string str = strprintf("%lf", n);
  // clean up trailing zeros - good for RPC not so much for UI
  str.erase ( str.find_last_not_of('0') + 1, std::string::npos );
  if (str.length() > 0) { std::string::iterator it = str.end() - 1; if (*it == '.') { str.erase(it); } } //get rid of trailing dot if non decimal
return str;
}

string FormatDivisibleShortMP(int64_t n)
{
int64_t n_abs = (n > 0 ? n : -n);
int64_t quotient = n_abs/COIN;
int64_t remainder = n_abs%COIN;
string str = strprintf("%d.%08d", quotient, remainder);
// clean up trailing zeros - good for RPC not so much for UI
str.erase ( str.find_last_not_of('0') + 1, std::string::npos );
if (str.length() > 0) { std::string::iterator it = str.end() - 1; if (*it == '.') { str.erase(it); } } //get rid of trailing dot if non decimal
return str;
}

// mostly taken from Bitcoin's FormatMoney()
string FormatDivisibleMP(int64_t n, bool fSign)
{
// Note: not using straight sprintf here because we do NOT want
// localized number formatting.
int64_t n_abs = (n > 0 ? n : -n);
int64_t quotient = n_abs/COIN;
int64_t remainder = n_abs%COIN;
string str = strprintf("%d.%08d", quotient, remainder);

  if (!fSign) return str;

  if (n < 0)
      str.insert((unsigned int)0, 1, '-');
  else
      str.insert((unsigned int)0, 1, '+');
  return str;
}

std::string mastercore::FormatIndivisibleMP(int64_t n)
{
  string str = strprintf("%ld", n);
  return str;
}

std::string FormatMP(unsigned int property, int64_t n, bool fSign)
{
  if (isPropertyDivisible(property)) return FormatDivisibleMP(n, fSign);
  else return FormatIndivisibleMP(n);
}

string const CMPSPInfo::watermarkKey("watermark");

CCriticalSection cs_tally;

OfferMap mastercore::my_offers;
AcceptMap mastercore::my_accepts;

CMPSPInfo *mastercore::_my_sps;
CrowdMap mastercore::my_crowds;

PendingMap mastercore::my_pending;

static CMPPending *pendingDelete(const uint256 txid, bool bErase = false)
{
  if (msc_debug_verbose3) file_log("%s(%s)\n", __FUNCTION__, txid.GetHex().c_str());

  PendingMap::iterator it = my_pending.find(txid);

  if (it != my_pending.end())
  {
    // display
    CMPPending *p_pending = &(it->second);

    int64_t src_amount = getMPbalance(p_pending->src, p_pending->prop, PENDING);

    if (msc_debug_verbose3) file_log("%s()src= %ld, line %d, file: %s\n", __FUNCTION__, src_amount, __LINE__, __FILE__);

    if (src_amount)
    {
      update_tally_map(p_pending->src, p_pending->prop, p_pending->amount, PENDING);
    }

    if (bErase)
    {
      my_pending.erase(it);
    }
    else
    {
      return &(it->second);
    }
  }

  return (CMPPending *) NULL;
}

static int pendingAdd(const uint256 &txid, const string &FromAddress, unsigned int propId, int64_t Amount, int64_t type, const string &txDesc)
{
CMPPending pending;

  if (msc_debug_verbose3) file_log("%s(%s,%s,%u,%ld,%d, %s), line %d, file: %s\n", __FUNCTION__, txid.GetHex().c_str(), FromAddress.c_str(), propId, Amount, type, txDesc,__LINE__, __FILE__);

  // support for pending, 0-confirm
  if (update_tally_map(FromAddress, propId, -Amount, PENDING))
  {
    pending.src = FromAddress;
    pending.amount = Amount;
    pending.prop = propId;
    pending.desc = txDesc;
    pending.type = type;
    my_pending.insert(std::make_pair(txid, pending));
  }

  return 0;
}

// this is the master list of all amounts for all addresses for all properties, map is sorted by Bitcoin address
std::map<string, CMPTally> mastercore::mp_tally_map;

CMPTally *mastercore::getTally(const string & address)
{
  LOCK (cs_tally);

  map<string, CMPTally>::iterator it = mp_tally_map.find(address);

  if (it != mp_tally_map.end()) return &(it->second);

  return (CMPTally *) NULL;
}

// look at balance for an address
int64_t getMPbalance(const string &Address, unsigned int property, TallyType ttype)
{
uint64_t balance = 0;

  if (TALLY_TYPE_COUNT <= ttype) return 0;

  LOCK(cs_tally);

const map<string, CMPTally>::iterator my_it = mp_tally_map.find(Address);

  if (my_it != mp_tally_map.end())
  {
    balance = (my_it->second).getMoney(property, ttype);
  }

  return balance;
}

int64_t getUserAvailableMPbalance(const string &Address, unsigned int property)
{
int64_t money = getMPbalance(Address, property, BALANCE);
int64_t pending = getMPbalance(Address, property, PENDING);

  if (0 > pending)
  {
    return (money + pending); // show the decrease in money available
  }

  return money;
}

static bool isRangeOK(const uint64_t input)
{
  if (MAX_INT_8_BYTES < input) return false;

  return true;
}

// returns false if we are out of range and/or overflow
// call just before multiplying large numbers
bool isMultiplicationOK(const uint64_t a, const uint64_t b)
{
//  printf("%s(%lu, %lu): ", __FUNCTION__, a, b);

  if (!a || !b) return true;

  if (MAX_INT_8_BYTES < a) return false;
  if (MAX_INT_8_BYTES < b) return false;

  const uint64_t result = a*b;

  if (MAX_INT_8_BYTES < result) return false;

  if ((0 != a) && (result / a != b)) return false;

  return true;
}

bool mastercore::isTestEcosystemProperty(unsigned int property)
{
  if ((OMNI_PROPERTY_TMSC == property) || (TEST_ECO_PROPERTY_1 <= property)) return true;

  return false;
}

bool mastercore::isMainEcosystemProperty(unsigned int property)
{
  if ((OMNI_PROPERTY_BTC != property) && !isTestEcosystemProperty(property)) return true;

  return false;
}

bool mastercore::isMetaDExOfferActive(const uint256 txid, unsigned int propertyId)
{
  for (md_PropertiesMap::iterator my_it = metadex.begin(); my_it != metadex.end(); ++my_it)
  {
      if (my_it->first == propertyId) //at bear minimum only go deeper if it's the right property id
      {
           md_PricesMap & prices = my_it->second;
           for (md_PricesMap::iterator it = prices.begin(); it != prices.end(); ++it)
           {
                md_Set & indexes = (it->second);
                for (md_Set::iterator it = indexes.begin(); it != indexes.end(); ++it)
                {
                     CMPMetaDEx obj = *it;
                     if( obj.getHash().GetHex() == txid.GetHex() ) return true;
                }
           }
      }
  }
  return false;
}

std::string mastercore::getMasterCoreAlertString()
{
    return global_alert_message;
}

std::string mastercore::getTokenLabel(unsigned int propertyId)
{
    std::string tokenStr;
    if (propertyId < 3) {
        if(propertyId == 1) { tokenStr = " MSC"; } else { tokenStr = " TMSC"; }
    } else {
        tokenStr = " SPT#" + to_string(propertyId);
    }
    return tokenStr;
}

bool mastercore::parseAlertMessage(std::string rawAlertStr, int32_t *alertType, uint64_t *expiryValue, uint32_t *typeCheck, uint32_t *verCheck, std::string *alertMessage)
{
    std::vector<std::string> vstr;
    boost::split(vstr, rawAlertStr, boost::is_any_of(":"), token_compress_on);
    if (5 == vstr.size())
    {
        try {
            *alertType = boost::lexical_cast<int32_t>(vstr[0]);
            *expiryValue = boost::lexical_cast<uint64_t>(vstr[1]);
            *typeCheck = boost::lexical_cast<uint32_t>(vstr[2]);
            *verCheck = boost::lexical_cast<uint32_t>(vstr[3]);
        } catch (const boost::bad_lexical_cast &e) {
            file_log("DEBUG ALERT - error in converting values from global alert string\n");
            return false; //(something went wrong)
        }
        *alertMessage = vstr[4];
        if ((*alertType < 1) || (*alertType > 4) || (*expiryValue == 0)) {
            file_log("DEBUG ALERT ERROR - Something went wrong decoding the global alert string, values are not as expected.\n");
            return false;
        }
        return true;
    }
    return false;
}

std::string mastercore::getMasterCoreAlertTextOnly()
{
    std::vector<std::string> vstr;
    if(!global_alert_message.empty())
    {
        boost::split(vstr, global_alert_message, boost::is_any_of(":"), token_compress_on);
        // make sure there are 5 tokens, 5th is text message
        if (5 == vstr.size())
        {
            return vstr[4];
        }
        else
        {
            file_log("DEBUG ALERT ERROR - Something went wrong decoding the global alert string, there are not 5 tokens\n");
            return "";
        }
    }
    return "";
}

bool mastercore::checkExpiredAlerts(unsigned int curBlock, uint64_t curTime)
{
    //expire any alerts that need expiring
    int32_t alertType = 0;
    uint64_t expiryValue = 0;
    uint32_t typeCheck = 0;
    uint32_t verCheck = 0;
    string alertMessage;
    if(!global_alert_message.empty())
    {
        bool parseSuccess = parseAlertMessage(global_alert_message, &alertType, &expiryValue, &typeCheck, &verCheck, &alertMessage);
        if (parseSuccess)
        {
            switch (alertType)
            {
                 case 1: //Text based alert only expiring by block number, show alert in UI and getalert_MP call, ignores type check value (eg use 0)
                     if (curBlock > expiryValue)
                     {
                         //the alert has expired, clear the global alert string
                         file_log("DEBUG ALERT - Expiring alert string %s\n",global_alert_message.c_str());
                         global_alert_message = "";
                         return true;
                     }
                 break;
                 case 2: //Text based alert only expiring by block time, show alert in UI and getalert_MP call, ignores type check value (eg use 0)
                     if (curTime > expiryValue)
                     {
                         //the alert has expired, clear the global alert string
                         file_log("DEBUG ALERT - Expiring alert string %s\n",global_alert_message.c_str());
                         global_alert_message = "";
                         return true;
                     }
                 break;
                 case 3: //Text based alert only expiring by client version, show alert in UI and getalert_MP call, ignores type check value (eg use 0)
                     if (OMNICORE_VERSION > expiryValue)
                     {
                         //the alert has expired, clear the global alert string
                         file_log("DEBUG ALERT - Expiring alert string %s\n",global_alert_message.c_str());
                         global_alert_message = "";
                         return true;
                     }
                 break;
                 case 4: //Update alert, show upgrade alert in UI and getalert_MP call + use isTransactionTypeAllowed to verify client support and shutdown if not present
                     //check of the new tx type is supported at live block
                     bool txSupported = isTransactionTypeAllowed(expiryValue+1, OMNI_PROPERTY_MSC, typeCheck, verCheck);

                     //check if we are at/past the live blockheight
                     bool txLive = (chainActive.Height()>(int64_t)expiryValue);

                     //testnet allows all types of transactions, so override this here for testing
                     //txSupported = false; //testing
                     //txLive = true; //testing

                     if ((!txSupported) && (txLive))
                     {
                         // we know we have transactions live we don't understand
                         // can't be trusted to provide valid data, shutdown
                         file_log("DEBUG ALERT - Shutting down due to unsupported live TX - alert string %s\n",global_alert_message.c_str());
                         printf("DEBUG ALERT - Shutting down due to unsupported live TX - alert string %s\n",global_alert_message.c_str()); //echo to screen
                         if (!GetBoolArg("-overrideforcedshutdown", false)) AbortNode("Shutting down due to alert: " + getMasterCoreAlertTextOnly());
                         return false;
                     }

                     if ((!txSupported) && (!txLive))
                     {
                         // warn the user this version does not support the new coming TX and they will need to update
                         // we don't actually need to take any action here, we leave the alert string in place and simply don't expire it
                     }

                     if (txSupported)
                     {
                         // we can simply expire this update as this version supports the new coming TX
                         file_log("DEBUG ALERT - Expiring alert string %s\n",global_alert_message.c_str());
                         global_alert_message = "";
                         return true;
                     }
                 break;
            }
            return false;
        }
        return false;
    }
    else { return false; }
    return false;
}

// get total tokens for a property
// optionally counts the number of addresses who own that property: n_owners_total
int64_t mastercore::getTotalTokens(unsigned int propertyId, int64_t *n_owners_total)
{
int64_t prev = 0, owners = 0;

  LOCK(cs_tally);

  CMPSPInfo::Entry property;
  if (false == _my_sps->getSP(propertyId, property)) return 0; // property ID does not exist

  int64_t totalTokens = 0;
  bool fixedIssuance = property.fixed;

  if (!fixedIssuance || n_owners_total)
  {
      for(map<string, CMPTally>::iterator my_it = mp_tally_map.begin(); my_it != mp_tally_map.end(); ++my_it)
      {
          string address = (my_it->first).c_str();
          totalTokens += getMPbalance(address, propertyId, BALANCE);
          totalTokens += getMPbalance(address, propertyId, SELLOFFER_RESERVE);
          totalTokens += getMPbalance(address, propertyId, METADEX_RESERVE);
          if (propertyId<3) totalTokens += getMPbalance(address, propertyId, ACCEPT_RESERVE);

          if (prev != totalTokens)
          {
            prev = totalTokens;
            owners++;
          }
      }
  }

  if (fixedIssuance)
  {
      totalTokens = property.num_tokens; //only valid for TX50
  }

  if (n_owners_total) *n_owners_total = owners;

  return totalTokens;
}

// return true if everything is ok
bool mastercore::update_tally_map(string who, unsigned int which_property, int64_t amount, TallyType ttype)
{
bool bRet = false;
uint64_t before, after;

  if (0 == amount)
  {
    file_log("%s(%s, %u=0x%X, %+ld, ttype= %d) 0 FUNDS !\n", __FUNCTION__, who.c_str(), which_property, which_property, amount, ttype);
    return false;
  }

  LOCK(cs_tally);

  before = getMPbalance(who, which_property, ttype);

  map<string, CMPTally>::iterator my_it = mp_tally_map.find(who);
  if (my_it == mp_tally_map.end())
  {
    // insert an empty element
    my_it = (mp_tally_map.insert(std::make_pair(who,CMPTally()))).first;
  }

  CMPTally &tally = my_it->second;

  bRet = tally.updateMoney(which_property, amount, ttype);

  after = getMPbalance(who, which_property, ttype);
  if (!bRet) file_log("%s(%s, %u=0x%X, %+ld, ttype=%d) INSUFFICIENT FUNDS\n", __FUNCTION__, who.c_str(), which_property, which_property, amount, ttype);

  if (msc_debug_tally)
  {
    if ((exodus_address != who) || (exodus_address == who && msc_debug_exo))
    {
      file_log("%s(%s, %u=0x%X, %+ld, ttype=%d); before=%lu, after=%lu\n",
       __FUNCTION__, who.c_str(), which_property, which_property, amount, ttype, before, after);
    }
  }

  return bRet;
}

std::string p128(int128_t quantity)
{
    //printf("\nTest # was %s\n", boost::lexical_cast<std::string>(quantity).c_str() );
   return boost::lexical_cast<std::string>(quantity);
}
std::string p_arb(cpp_int quantity)
{
    //printf("\nTest # was %s\n", boost::lexical_cast<std::string>(quantity).c_str() );
   return boost::lexical_cast<std::string>(quantity);
}
//calculateFundraiser does token calculations per transaction
//calcluateFractional does calculations for missed tokens
void calculateFundraiser(unsigned short int propType, uint64_t amtTransfer, unsigned char bonusPerc, 
  uint64_t fundraiserSecs, uint64_t currentSecs, uint64_t numProps, unsigned char issuerPerc, uint64_t totalTokens, 
  std::pair<uint64_t, uint64_t>& tokens, bool &close_crowdsale )
{
  //uint64_t weeks_sec = 604800;
  int128_t weeks_sec_ = 604800L;
  //define weeks in seconds
  int128_t precision_ = 1000000000000L;
  //define precision for all non-bitcoin values (bonus percentages, for example)
  int128_t percentage_precision = 100L;
  //define precision for all percentages (10/100 = 10%)

  //uint64_t bonusSeconds = fundraiserSecs - currentSecs;
  //calcluate the bonusseconds
  //printf("\n bonus sec %lu\n", bonusSeconds);
  int128_t bonusSeconds_ = fundraiserSecs - currentSecs;

  //double weeks_d = bonusSeconds / (double) weeks_sec;
  //debugging
  
  int128_t weeks_ = (bonusSeconds_ / weeks_sec_) * precision_ + ( (bonusSeconds_ % weeks_sec_ ) * precision_) / weeks_sec_;
  //calculate the whole number of weeks to apply bonus

  //printf("\n weeks_d: %.8lf \n weeks: %s + (%s / %s) =~ %.8lf \n", weeks_d, p128(bonusSeconds_ / weeks_sec_).c_str(), p128(bonusSeconds_ % weeks_sec_).c_str(), p128(weeks_sec_).c_str(), boost::lexical_cast<double>(bonusSeconds_ / weeks_sec_) + boost::lexical_cast<double> (bonusSeconds_ % weeks_sec_) / boost::lexical_cast<double>(weeks_sec_) );
  //debugging lines

  //double ebPercentage_d = weeks_d * bonusPerc;
  //debugging lines

  int128_t ebPercentage_ = weeks_ * bonusPerc;
  //calculate the earlybird percentage to be applied

  //printf("\n ebPercentage_d: %.8lf \n ebPercentage: %s + (%s / %s ) =~ %.8lf \n", ebPercentage_d, p128(ebPercentage_ / precision_).c_str(), p128( (ebPercentage_) % precision_).c_str() , p128(precision_).c_str(), boost::lexical_cast<double>(ebPercentage_ / precision_) + boost::lexical_cast<double>(ebPercentage_ % precision_) / boost::lexical_cast<double>(precision_));
  //debugging
  
  //double bonusPercentage_d = ( ebPercentage_d / 100 ) + 1;
  //debugging

  int128_t bonusPercentage_ = (ebPercentage_ + (precision_ * percentage_precision) ) / percentage_precision; 
  //calcluate the bonus percentage to apply up to 'percentage_precision' number of digits

  //printf("\n bonusPercentage_d: %.18lf \n bonusPercentage: %s + (%s / %s) =~ %.11lf \n", bonusPercentage_d, p128(bonusPercentage_ / precision_).c_str(), p128(bonusPercentage_ % precision_).c_str(), p128(precision_).c_str(), boost::lexical_cast<double>(bonusPercentage_ / precision_) + boost::lexical_cast<double>(bonusPercentage_ % precision_) / boost::lexical_cast<double>(precision_));
  //debugging

  //double issuerPercentage_d = (double) (issuerPerc * 0.01);
  //debugging

  int128_t issuerPercentage_ = (int128_t)issuerPerc * precision_ / percentage_precision;

  //printf("\n issuerPercentage_d: %.8lf \n issuerPercentage: %s + (%s / %s) =~ %.8lf \n", issuerPercentage_d, p128(issuerPercentage_ / precision_ ).c_str(), p128(issuerPercentage_ % precision_).c_str(), p128( precision_ ).c_str(), boost::lexical_cast<double>(issuerPercentage_ / precision_) + boost::lexical_cast<double>(issuerPercentage_ % precision_) / boost::lexical_cast<double>(precision_));
  //debugging

  int128_t satoshi_precision_ = 100000000;
  //define the precision for bitcoin amounts (satoshi)
  //uint64_t createdTokens, createdTokens_decimal;
  //declare used variables for total created tokens

  //uint64_t issuerTokens, issuerTokens_decimal;
  //declare used variables for total issuer tokens

  //printf("\n NUMBER OF PROPERTIES %ld", numProps); 
  //printf("\n AMOUNT INVESTED: %ld BONUS PERCENTAGE: %.11f and %s", amtTransfer,bonusPercentage_d, p128(bonusPercentage_).c_str());
  
  //long double ct = ((amtTransfer/1e8) * (long double) numProps * bonusPercentage_d);

  //int128_t createdTokens_ = (int128_t)amtTransfer*(int128_t)numProps* bonusPercentage_;

  cpp_int createdTokens = boost::lexical_cast<cpp_int>((int128_t)amtTransfer*(int128_t)numProps)* boost::lexical_cast<cpp_int>(bonusPercentage_);

  //printf("\n CREATED TOKENS UINT %s \n", p_arb(createdTokens).c_str());

  //printf("\n CREATED TOKENS %.8Lf, %s + (%s / %s) ~= %.8lf",ct, p128(createdTokens_ / (precision_ * satoshi_precision_) ).c_str(), p128(createdTokens_ % (precision_ * satoshi_precision_) ).c_str() , p128( precision_*satoshi_precision_ ).c_str(), boost::lexical_cast<double>(createdTokens_ / (precision_ * satoshi_precision_) ) + boost::lexical_cast<double>(createdTokens_ % (precision_ * satoshi_precision_)) / boost::lexical_cast<double>(precision_*satoshi_precision_));

  //long double it = (uint64_t) ct * issuerPercentage_d;

  //int128_t issuerTokens_ = (createdTokens_ / (satoshi_precision_ * precision_ )) * (issuerPercentage_ / 100) * precision_;
  
  cpp_int issuerTokens = (createdTokens / (satoshi_precision_ * precision_ )) * (issuerPercentage_ / 100) * precision_;

  //printf("\n ISSUER TOKENS: %.8Lf, %s + (%s / %s ) ~= %.8lf \n",it, p128(issuerTokens_ / (precision_ * satoshi_precision_ * 100 ) ).c_str(), p128( issuerTokens_ % (precision_ * satoshi_precision_ * 100 ) ).c_str(), p128(precision_*satoshi_precision_*100).c_str(), boost::lexical_cast<double>(issuerTokens_ / (precision_ * satoshi_precision_ * 100))  + boost::lexical_cast<double>(issuerTokens_ % (satoshi_precision_*precision_*100) )/ boost::lexical_cast<double>(satoshi_precision_*precision_*100)); 
  
  //printf("\n UINT %s \n", p_arb(issuerTokens).c_str());
  //total tokens including remainders

  //printf("\n DIVISIBLE TOKENS (UI LAYER) CREATED: is ~= %.8lf, and %.8lf\n",(double)createdTokens + (double)createdTokens_decimal/(satoshi_precision *precision), (double) issuerTokens + (double)issuerTokens_decimal/(satoshi_precision*precision*percentage_precision) );
  //if (2 == propType)
    //printf("\n DIVISIBLE TOKENS (UI LAYER) CREATED: is ~= %.8lf, and %.8lf\n", (uint64_t) (boost::lexical_cast<double>(createdTokens_ / (precision_ * satoshi_precision_) ) + boost::lexical_cast<double>(createdTokens_ % (precision_ * satoshi_precision_)) / boost::lexical_cast<double>(precision_*satoshi_precision_) )/1e8, (uint64_t) (boost::lexical_cast<double>(issuerTokens_ / (precision_ * satoshi_precision_ * 100))  + boost::lexical_cast<double>(issuerTokens_ % (satoshi_precision_*precision_*100) )/ boost::lexical_cast<double>(satoshi_precision_*precision_*100)) / 1e8  );
  //else
    //printf("\n INDIVISIBLE TOKENS (UI LAYER) CREATED: is = %lu, and %lu\n", boost::lexical_cast<uint64_t>(createdTokens_ / (precision_ * satoshi_precision_ ) ), boost::lexical_cast<uint64_t>(issuerTokens_ / (precision_ * satoshi_precision_ * 100)));
  
  cpp_int createdTokens_int = createdTokens / (precision_ * satoshi_precision_);
  cpp_int issuerTokens_int = issuerTokens / (precision_ * satoshi_precision_ * 100 );
  cpp_int newTotalCreated = totalTokens + createdTokens_int  + issuerTokens_int;

  if ( newTotalCreated > MAX_INT_8_BYTES) {
    cpp_int maxCreatable = MAX_INT_8_BYTES - totalTokens;

    cpp_int created = createdTokens_int + issuerTokens_int;
    cpp_int ratio = (created * precision_ * satoshi_precision_) / maxCreatable;

    //printf("\n created %s, ratio %s, maxCreatable %s, totalTokens %s, createdTokens_int %s, issuerTokens_int %s \n", p_arb(created).c_str(), p_arb(ratio).c_str(), p_arb(maxCreatable).c_str(), p_arb(totalTokens).c_str(), p_arb(createdTokens_int).c_str(), p_arb(issuerTokens_int).c_str() );
    //debugging
  
    issuerTokens_int = (issuerTokens_int * precision_ * satoshi_precision_)/ratio;
    //calcluate the ratio of tokens for what we can create and apply it
    createdTokens_int = MAX_INT_8_BYTES - issuerTokens_int ;
    //give the rest to the user

    //printf("\n created %s, ratio %s, maxCreatable %s, totalTokens %s, createdTokens_int %s, issuerTokens_int %s \n", p_arb(created).c_str(), p_arb(ratio).c_str(), p_arb(maxCreatable).c_str(), p_arb(totalTokens).c_str(), p_arb(createdTokens_int).c_str(), p_arb(issuerTokens_int).c_str() );
    //debugging
    close_crowdsale = true; //close up the crowdsale after assigning all tokens
  }
  tokens = std::make_pair(boost::lexical_cast<uint64_t>(createdTokens_int) , boost::lexical_cast<uint64_t>(issuerTokens_int));
  //give tokens
}

// certain transaction types are not live on the network until some specific block height
// certain transactions will be unknown to the client, i.e. "black holes" based on their version
// the Restrictions array is as such: type, block-allowed-in, top-version-allowed
bool mastercore::isTransactionTypeAllowed(int txBlock, unsigned int txProperty, unsigned int txType, unsigned short version, bool bAllowNullProperty)
{
bool bAllowed = false;
bool bBlackHole = false;
unsigned int type;
int block_FirstAllowed;
unsigned short version_TopAllowed;

  // bitcoin as property is never allowed, unless explicitly stated otherwise
  if ((OMNI_PROPERTY_BTC == txProperty) && !bAllowNullProperty) return false;

  // everything is always allowed on Bitcoin's TestNet or with TMSC/TestEcosystem on MainNet
  if ((isNonMainNet()) || isTestEcosystemProperty(txProperty))
  {
    bAllowed = true;
  }

  for (unsigned int i = 0; i < sizeof(txRestrictionsRules)/sizeof(txRestrictionsRules[0]); i++)
  {
    type = txRestrictionsRules[i][0];
    block_FirstAllowed = txRestrictionsRules[i][1];
    version_TopAllowed = txRestrictionsRules[i][2];

    if (txType != type) continue;

    if (version_TopAllowed < version)
    {
      file_log("Black Hole identified !!! %d, %u, %u, %u\n", txBlock, txProperty, txType, version);

      bBlackHole = true;

      // TODO: what else?
      // ...
    }

    if (0 > block_FirstAllowed) break;  // array contains a negative -- nothing's allowed or done parsing

    if (block_FirstAllowed <= txBlock) bAllowed = true;
  }

  return bAllowed && !bBlackHole;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// some old TODOs
//  6) verify large-number calculations (especially divisions & multiplications)
//  9) build in consesus checks with the masterchain.info & masterchest.info -- possibly run them automatically, daily (?)
// 10) need a locking mechanism between Core & Qt -- to retrieve the tally, for instance, this and similar to this: LOCK(wallet->cs_wallet);
//

uint64_t calculate_and_update_devmsc(unsigned int nTime)
{
//do nothing if before end of fundraiser 
if (nTime < 1377993874) return -9919;

// taken mainly from msc_validate.py: def get_available_reward(height, c)
uint64_t devmsc = 0;
int64_t exodus_delta;
// spec constants:
const uint64_t all_reward = 5631623576222;
const double seconds_in_one_year = 31556926;
const double seconds_passed = nTime - 1377993874; // exodus bootstrap deadline
const double years = seconds_passed/seconds_in_one_year;
const double part_available = 1 - pow(0.5, years);
const double available_reward=all_reward * part_available;

  devmsc = rounduint64(available_reward);
  exodus_delta = devmsc - exodus_prev;

  if (msc_debug_exo) file_log("devmsc=%lu, exodus_prev=%lu, exodus_delta=%ld\n", devmsc, exodus_prev, exodus_delta);

  // skip if a block's timestamp is older than that of a previous one!
  if (0>exodus_delta) return 0;

  update_tally_map(exodus_address, OMNI_PROPERTY_MSC, exodus_delta, BALANCE);
  exodus_prev = devmsc;

  return devmsc;
}

// TODO: optimize efficiency -- iterate only over wallet's addresses in the future
// NOTE: if we loop over wallet addresses we miss tokens that may be in change addresses (since mapAddressBook does not
//       include change addresses).  with current transaction load, about 0.02 - 0.06 seconds is spent on this function
int mastercore::set_wallet_totals()
{
  //concerned about efficiency here, time how long this takes, averaging 0.02-0.04s on my system
  //timer t;
  int my_addresses_count = 0;
  int64_t propertyId;
  unsigned int nextSPID = _my_sps->peekNextSPID(1); // real eco
  unsigned int nextTestSPID = _my_sps->peekNextSPID(2); // test eco

  //zero bals
  for (propertyId = 1; propertyId<nextSPID; propertyId++) //main eco
  {
    global_balance_money_maineco[propertyId] = 0;
    global_balance_reserved_maineco[propertyId] = 0;
  }
  for (propertyId = TEST_ECO_PROPERTY_1; propertyId<nextTestSPID; propertyId++) //test eco
  {
    global_balance_money_testeco[propertyId-2147483647] = 0;
    global_balance_reserved_testeco[propertyId-2147483647] = 0;
  }

  for(map<string, CMPTally>::iterator my_it = mp_tally_map.begin(); my_it != mp_tally_map.end(); ++my_it)
  {
    if (IsMyAddress(my_it->first))
    {
       for (propertyId = 1; propertyId<nextSPID; propertyId++) //main eco
       {
              //global_balance_money_maineco[propertyId] += getMPbalance(my_it->first, propertyId, BALANCE);
              global_balance_money_maineco[propertyId] += getUserAvailableMPbalance(my_it->first, propertyId);
              global_balance_reserved_maineco[propertyId] += getMPbalance(my_it->first, propertyId, SELLOFFER_RESERVE);
              global_balance_reserved_maineco[propertyId] += getMPbalance(my_it->first, propertyId, METADEX_RESERVE);
              if (propertyId < 3) global_balance_reserved_maineco[propertyId] += getMPbalance(my_it->first, propertyId, ACCEPT_RESERVE);
       }
       for (propertyId = TEST_ECO_PROPERTY_1; propertyId<nextTestSPID; propertyId++) //test eco
       {
              //global_balance_money_testeco[propertyId-2147483647] += getMPbalance(my_it->first, propertyId, BALANCE);
              global_balance_money_testeco[propertyId-2147483647] += getUserAvailableMPbalance(my_it->first, propertyId);
              global_balance_reserved_testeco[propertyId-2147483647] += getMPbalance(my_it->first, propertyId, SELLOFFER_RESERVE);
              global_balance_reserved_testeco[propertyId-2147483647] += getMPbalance(my_it->first, propertyId, METADEX_RESERVE);
       }
    }
  }
  //printf("Global MSC totals: MSC_total= %lu, MSC_RESERVED_total= %lu\n", global_balance_money_maineco[1], global_balance_reserved_maineco[1]);
  //std::cout << t.elapsed() << std::endl;
  return (my_addresses_count);
}

static void prepareObfuscatedHashes(const string &address, string (&ObfsHashes)[1+MAX_SHA256_OBFUSCATION_TIMES])
{
unsigned char sha_input[128];
unsigned char sha_result[128];
vector<unsigned char> vec_chars;

  strcpy((char *)sha_input, address.c_str());
  // do only as many re-hashes as there are mastercoin packets, 255 per spec
  for (unsigned int j = 1; j<=MAX_SHA256_OBFUSCATION_TIMES;j++)
  {
    SHA256(sha_input, strlen((const char *)sha_input), sha_result);

      vec_chars.resize(32);
      memcpy(&vec_chars[0], &sha_result[0], 32);
      ObfsHashes[j] = HexStr(vec_chars);
      boost::to_upper(ObfsHashes[j]); // uppercase per spec

      if (msc_debug_verbose2) if (5>j) file_log("%d: sha256 hex: %s\n", j, ObfsHashes[j].c_str());
      strcpy((char *)sha_input, ObfsHashes[j].c_str());
  }
}

static bool getOutputType(const CScript& scriptPubKey, txnouttype& whichTypeRet)
{
vector<vector<unsigned char> > vSolutions;

  if (!Solver(scriptPubKey, whichTypeRet, vSolutions)) return false;

  return true;
}


int TXExodusFundraiser(const CTransaction &wtx, const string &sender, int64_t ExodusHighestValue, int nBlock, unsigned int nTime)
{
  if ((nBlock >= GENESIS_BLOCK && nBlock <= LAST_EXODUS_BLOCK) || (isNonMainNet()))
  { //Exodus Fundraiser start/end blocks
    //printf("transaction: %s\n", wtx.ToString().c_str() );
    int deadline_timeleft=1377993600-nTime;
    double bonus= 1 + std::max( 0.10 * deadline_timeleft / (60 * 60 * 24 * 7), 0.0 );

    if (isNonMainNet())
    {
      bonus = 1;

      if (sender == exodus_address) return 1; // sending from Exodus should not be fundraising anything
    }

    uint64_t msc_tot= round( 100 * ExodusHighestValue * bonus ); 
    if (msc_debug_exo) file_log("Exodus Fundraiser tx detected, tx %s generated %lu.%08lu\n",wtx.GetHash().ToString().c_str(), msc_tot / COIN, msc_tot % COIN);
 
    update_tally_map(sender, OMNI_PROPERTY_MSC, msc_tot, BALANCE);
    update_tally_map(sender, OMNI_PROPERTY_TMSC, msc_tot, BALANCE);

    return 0;
  }
  return -1;
}

static bool isAllowedOutputType(int whichType, int nBlock)
{
  int p2shAllowed = 0;

  if (P2SH_BLOCK <= nBlock || isNonMainNet()) {
    p2shAllowed = 1;
  }
  // validTypes:
  // 1) Pay to pubkey hash
  // 2) Pay to Script Hash (IFF p2sh is allowed)
  if ((TX_PUBKEYHASH == whichType) || (p2shAllowed && (TX_SCRIPTHASH == whichType))) {
    return true;
  } else {
    return false;
  }
}

// idx is position within the block, 0-based
// int msc_tx_push(const CTransaction &wtx, int nBlock, unsigned int idx)
// INPUT: bRPConly -- set to true to avoid moving funds; to be called from various RPC calls like this
// RETURNS: 0 if parsed a MP TX
// RETURNS: < 0 if a non-MP-TX or invalid
// RETURNS: >0 if 1 or more payments have been made
int parseTransaction(bool bRPConly, const CTransaction &wtx, int nBlock, unsigned int idx, CMPTransaction *mp_tx, unsigned int nTime)
{
string strSender;
// class A: data & address storage -- combine them into a structure or something
vector<string>script_data;
vector<string>address_data;
// vector<uint64_t>value_data;
vector<int64_t>value_data;
int64_t ExodusValues[MAX_BTC_OUTPUTS]= { 0 };
int64_t TestNetMoneyValues[MAX_BTC_OUTPUTS] = { 0 };  // new way to get funded on TestNet, send TBTC to moneyman address
string strReference;
unsigned char single_pkt[MAX_PACKETS * PACKET_SIZE];
unsigned int packet_size = 0;
int fMultisig = 0;
int marker_count = 0, getmoney_count = 0;
// class B: multisig data storage
vector<string>multisig_script_data;
uint64_t inAll = 0;
uint64_t outAll = 0;
uint64_t txFee = 0;

            mp_tx->Set(wtx.GetHash(), nBlock, idx, nTime);

            // quickly go through the outputs & ensure there is a marker (a send to the Exodus address)
            for (unsigned int i = 0; i < wtx.vout.size(); i++)
            {
            CTxDestination dest;
            string strAddress;

              outAll += wtx.vout[i].nValue;

              if (ExtractDestination(wtx.vout[i].scriptPubKey, dest))
              {
                strAddress = CBitcoinAddress(dest).ToString();

                if (exodus_address == strAddress)
                {
                  ExodusValues[marker_count++] = wtx.vout[i].nValue;
                }
                else if (isNonMainNet() && (getmoney_testnet == strAddress))
                {
                  TestNetMoneyValues[getmoney_count++] = wtx.vout[i].nValue;
                }
              }
            }
            if ((isNonMainNet() && getmoney_count))
            {
            }
            else if (!marker_count)
            {
              return -1;
            }

            file_log("____________________________________________________________________________________________________________________________________\n");
            file_log("%s(block=%d, %s idx= %d); txid: %s\n", __FUNCTION__, nBlock, DateTimeStrFormat("%Y-%m-%d %H:%M:%S", nTime).c_str(),
             idx, wtx.GetHash().GetHex().c_str());

            // now save output addresses & scripts for later use
            // also determine if there is a multisig in there, if so = Class B
            for (unsigned int i = 0; i < wtx.vout.size(); i++)
            {
            CTxDestination dest;
            string strAddress;

              if (ExtractDestination(wtx.vout[i].scriptPubKey, dest))
              {
                txnouttype whichType;
                bool validType = false;
                if (!getOutputType(wtx.vout[i].scriptPubKey, whichType)) validType=false;
                if (isAllowedOutputType(whichType, nBlock)) validType=true;

                strAddress = CBitcoinAddress(dest).ToString();

                if ((exodus_address != strAddress) && (validType))
                {
                  if (msc_debug_parser_data) file_log("saving address_data #%d: %s:%s\n", i, strAddress.c_str(), wtx.vout[i].scriptPubKey.ToString().c_str());

                  // saving for Class A processing or reference
                  GetScriptPushes(wtx.vout[i].scriptPubKey, script_data);
                  address_data.push_back(strAddress);
                  value_data.push_back(wtx.vout[i].nValue);
                }
              }
              else
              {
              // a multisig ?
              txnouttype type;
              std::vector<CTxDestination> vDest;
              int nRequired;

                if (ExtractDestinations(wtx.vout[i].scriptPubKey, type, vDest, nRequired))
                {
                  ++fMultisig;
                }
              }
            }

            if (msc_debug_parser_data)
            {
              file_log("address_data.size=%lu\n", address_data.size());
              file_log("script_data.size=%lu\n", script_data.size());
              file_log("value_data.size=%lu\n", value_data.size());
            }

            int inputs_errors = 0;  // several types of erroroneous MP TX inputs
            map <string, uint64_t> inputs_sum_of_values;
            // now go through inputs & identify the sender, collect input amounts
            // go through inputs, find the largest per Mastercoin protocol, the Sender
            for (unsigned int i = 0; i < wtx.vin.size(); i++)
            {
            CTxDestination address;

            if (msc_debug_vin) file_log("vin=%d:%s\n", i, wtx.vin[i].scriptSig.ToString().c_str());

            CTransaction txPrev;
            uint256 hashBlock;
            if (!GetTransaction(wtx.vin[i].prevout.hash, txPrev, hashBlock, true))  // get the vin's previous transaction 
            {
              return -101;
            }

            unsigned int n = wtx.vin[i].prevout.n;

            CTxDestination source;

            uint64_t nValue = txPrev.vout[n].nValue;
            txnouttype whichType;

              inAll += nValue;

              if (ExtractDestination(txPrev.vout[n].scriptPubKey, source))  // extract the destination of the previous transaction's vout[n]
              {
                // we only allow pay-to-pubkeyhash, pay-to-scripthash & probably pay-to-pubkey (?)
                {
                  if (!getOutputType(txPrev.vout[n].scriptPubKey, whichType)) ++inputs_errors;
                  if (!isAllowedOutputType(whichType, nBlock)) ++inputs_errors;

                  if (inputs_errors) break;
                }

                CBitcoinAddress addressSource(source);              // convert this to an address

                inputs_sum_of_values[addressSource.ToString()] += nValue;
              }
              else ++inputs_errors;

              if (msc_debug_vin) file_log("vin=%d:%s\n", i, wtx.vin[i].ToString().c_str());
            } // end of inputs for loop

            txFee = inAll - outAll; // this is the fee paid to miners for this TX

            if (inputs_errors)  // not a valid MP TX
            {
              return -101;
            }

            // largest by sum of values
            uint64_t nMax = 0;
            for(map<string, uint64_t>::iterator my_it = inputs_sum_of_values.begin(); my_it != inputs_sum_of_values.end(); ++my_it)
            {
            uint64_t nTemp = my_it->second;

                if (nTemp > nMax)
                {
                  strSender = my_it->first;
                  if (msc_debug_exo) file_log("looking for The Sender: %s , nMax=%lu, nTemp=%lu\n", strSender.c_str(), nMax, nTemp);
                  nMax = nTemp;
                }
            }

            if (!strSender.empty())
            {
              if (msc_debug_verbose) file_log("The Sender: %s : His Input Sum of Values= %lu.%08lu ; fee= %lu.%08lu\n",
               strSender.c_str(), nMax / COIN, nMax % COIN, txFee/COIN, txFee%COIN);
            }
            else
            {
              file_log("The sender is still EMPTY !!! txid: %s\n", wtx.GetHash().GetHex().c_str());
              return -5;
            }
            
            //This calculates exodus fundraiser for each tx within a given block
            int64_t BTC_amount = ExodusValues[0];
            if (isNonMainNet())
            {
              if (MONEYMAN_TESTNET_BLOCK <= nBlock) BTC_amount = TestNetMoneyValues[0];
            }

            if (RegTest()) 
            { 
              if (MONEYMAN_REGTEST_BLOCK <= nBlock) BTC_amount = TestNetMoneyValues[0];
            }

//            file_log("%s() amount = %ld , nBlock = %d, line %d, file: %s\n", __FUNCTION__, BTC_amount, nBlock, __LINE__, __FILE__);

            if (0 < BTC_amount && !bRPConly) (void) TXExodusFundraiser(wtx, strSender, BTC_amount, nBlock, nTime);

            // go through the outputs
            for (unsigned int i = 0; i < wtx.vout.size(); i++)
            {
            CTxDestination address;

              // if TRUE -- non-multisig
              if (ExtractDestination(wtx.vout[i].scriptPubKey, address))
              {
              }
              else
              {
                // probably a multisig -- get them

        txnouttype type;
        std::vector<CTxDestination> vDest;
        int nRequired;

        // CScript is a std::vector
        if (msc_debug_script) file_log("scriptPubKey: %s\n", HexStr(wtx.vout[i].scriptPubKey));

        if (ExtractDestinations(wtx.vout[i].scriptPubKey, type, vDest, nRequired))
        {
          if (msc_debug_script) file_log(" >> multisig: ");
            BOOST_FOREACH(const CTxDestination &dest, vDest)
            {
            CBitcoinAddress address = CBitcoinAddress(dest);
            CKeyID keyID;

            if (!address.GetKeyID(keyID))
            {
            // TODO: add an error handler
            }

              // base_uint is a superclass of dest, size(), GetHex() is the same as ToString()
//              file_log("%s size=%d (%s); ", address.ToString().c_str(), keyID.size(), keyID.GetHex().c_str());
              if (msc_debug_script) file_log("%s ; ", address.ToString().c_str());

            }
          if (msc_debug_script) file_log("\n");

          // ignore first public key, as it should belong to the sender
          // and it be used to avoid the creation of unspendable dust
          GetScriptPushes(wtx.vout[i].scriptPubKey, multisig_script_data, true);
        }
              }
            } // end of the outputs' for loop

          string strObfuscatedHashes[1+MAX_SHA256_OBFUSCATION_TIMES];
          prepareObfuscatedHashes(strSender, strObfuscatedHashes);

          unsigned char packets[MAX_PACKETS][32];
          int mdata_count = 0;  // multisig data count
          if (!fMultisig)
          {
              // ---------------------------------- Class A parsing ---------------------------

              // Init vars
              string strScriptData;
              string strDataAddress;
              string strRefAddress;
              unsigned char dataAddressSeq = 0xFF;
              unsigned char seq = 0xFF;
              int64_t dataAddressValue = 0;

              // Step 1, locate the data packet
              for (unsigned k = 0; k<script_data.size();k++) // loop through outputs
              {
                  txnouttype whichType;
                  if (!getOutputType(wtx.vout[k].scriptPubKey, whichType)) break; // unable to determine type, ignore output
                  if (!isAllowedOutputType(whichType, nBlock)) break;
                  string strSub = script_data[k].substr(2,16); // retrieve bytes 1-9 of packet for peek & decode comparison
                  seq = (ParseHex(script_data[k].substr(0,2)))[0]; // retrieve sequence number

                  if (("0000000000000001" == strSub) || ("0000000000000002" == strSub)) // peek & decode comparison
                  {
                      if (strScriptData.empty()) // confirm we have not already located a data address
                      {
                          strScriptData = script_data[k].substr(2*1,2*PACKET_SIZE_CLASS_A); // populate data packet
                          strDataAddress = address_data[k]; // record data address
                          dataAddressSeq = seq; // record data address seq num for reference matching
                          dataAddressValue = value_data[k]; // record data address amount for reference matching
                          if (msc_debug_parser_data) file_log("Data Address located - data[%d]:%s: %s (%lu.%08lu)\n", k, script_data[k].c_str(), address_data[k].c_str(), value_data[k] / COIN, value_data[k] % COIN);
                      }
                      else
                      {
                          // invalidate - Class A cannot be more than one data packet - possible collision, treat as default (BTC payment)
                          strDataAddress = ""; //empty strScriptData to block further parsing
                          if (msc_debug_parser_data) file_log("Multiple Data Addresses found (collision?) Class A invalidated, defaulting to BTC payment\n");
                          break;
                      }
                  }
              }

              // Step 2, see if we can locate an address with a seqnum +1 of DataAddressSeq
              if (!strDataAddress.empty()) // verify Step 1, we should now have a valid data packet, if so continue parsing
              {
                  unsigned char expectedRefAddressSeq = dataAddressSeq + 1;
                  for (unsigned k = 0; k<script_data.size();k++) // loop through outputs
                  {
                      txnouttype whichType;
                      if (!getOutputType(wtx.vout[k].scriptPubKey, whichType)) break; // unable to determine type, ignore output
                      if (!isAllowedOutputType(whichType, nBlock)) break;

                      seq = (ParseHex(script_data[k].substr(0,2)))[0]; // retrieve sequence number

                      if ((address_data[k] != strDataAddress) && (address_data[k] != exodus_address) && (expectedRefAddressSeq == seq)) // found reference address with matching sequence number
                      {
                          if (strRefAddress.empty()) // confirm we have not already located a reference address
                          {
                              strRefAddress = address_data[k]; // set ref address
                              if (msc_debug_parser_data) file_log("Reference Address located via seqnum - data[%d]:%s: %s (%lu.%08lu)\n", k, script_data[k].c_str(), address_data[k].c_str(), value_data[k] / COIN, value_data[k] % COIN);
                          }
                          else
                          {
                              // can't trust sequence numbers to provide reference address, there is a collision with >1 address with expected seqnum
                              strRefAddress = ""; // blank ref address
                              if (msc_debug_parser_data) file_log("Reference Address sequence number collision, will fall back to evaluating matching output amounts\n");
                              break;
                          }
                      }
                  }
                  // Step 3, if we still don't have a reference address, see if we can locate an address with matching output amounts
                  if (strRefAddress.empty())
                  {
                      for (unsigned k = 0; k<script_data.size();k++) // loop through outputs
                      {
                          txnouttype whichType;
                          if (!getOutputType(wtx.vout[k].scriptPubKey, whichType)) break; // unable to determine type, ignore output
                          if (!isAllowedOutputType(whichType, nBlock)) break;

                          if ((address_data[k] != strDataAddress) && (address_data[k] != exodus_address) && (dataAddressValue == value_data[k])) // this output matches data output, check if matches exodus output
                          {
                              for (int exodus_idx=0;exodus_idx<marker_count;exodus_idx++)
                              {
                                  if (value_data[k] == ExodusValues[exodus_idx]) //this output matches data address value and exodus address value, choose as ref
                                  {
                                       if (strRefAddress.empty())
                                       {
                                           strRefAddress = address_data[k];
                                           if (msc_debug_parser_data) file_log("Reference Address located via matching amounts - data[%d]:%s: %s (%lu.%08lu)\n", k, script_data[k].c_str(), address_data[k].c_str(), value_data[k] / COIN, value_data[k] % COIN);
                                       }
                                       else
                                       {
                                           strRefAddress = "";
                                           if (msc_debug_parser_data) file_log("Reference Address collision, multiple potential candidates. Class A invalidated, defaulting to BTC payment\n");
                                           break;
                                       }
                                  }
                              }
                          }
                      }
                  }
              } // end if (!strDataAddress.empty())

              // Populate expected var strReference with chosen address (if not empty)
              if (!strRefAddress.empty()) strReference=strRefAddress;

              // Last validation step, if strRefAddress is empty, blank strDataAddress so we default to BTC payment
              if (strRefAddress.empty()) strDataAddress="";

              // -------------------------------- End Class A parsing -------------------------

              if (strDataAddress.empty()) // an empty Data Address here means it is not Class A valid and should be defaulted to a BTC payment
              {
              // this must be the BTC payment - validate (?)
//              if (msc_debug_verbose) file_log("\n================BLOCK: %d======\ntxid: %s\n", nBlock, wtx.GetHash().GetHex().c_str());
              file_log("!! sender: %s , receiver: %s\n", strSender.c_str(), strReference.c_str());
              file_log("!! this may be the BTC payment for an offer !!\n");

              // TODO collect all payments made to non-itself & non-exodus and their amounts -- these may be purchases!!!

              int count = 0;
              // go through the outputs, once again...
              {
                for (unsigned int i = 0; i < wtx.vout.size(); i++)
                {
                CTxDestination dest;

                  if (ExtractDestination(wtx.vout[i].scriptPubKey, dest))
                  {
                  const string strAddress = CBitcoinAddress(dest).ToString();

                    if (exodus_address == strAddress) continue;
                    file_log("payment #%d %s %11.8lf\n", count, strAddress.c_str(), (double)wtx.vout[i].nValue/(double)COIN);

                    // check everything & pay BTC for the property we are buying here...
                    if (bRPConly) count = 55555;  // no real way to validate a payment during simple RPC call
                    else if (0 == DEx_payment(wtx.GetHash(), i, strAddress, strSender, wtx.vout[i].nValue, nBlock)) ++count;
                  }
                }
              }
              return count ? count : -5678; // return count -- the actual number of payments within this TX or error if none were made
            }
            else
            {
            // valid Class A packet almost ready
              if (msc_debug_parser_data) file_log("valid Class A:from=%s:to=%s:data=%s\n", strSender.c_str(), strReference.c_str(), strScriptData.c_str());
              packet_size = PACKET_SIZE_CLASS_A;
              memcpy(single_pkt, &ParseHex(strScriptData)[0], packet_size);
            }
          }
          else // if (fMultisig)
          {
            unsigned int k = 0;
            // gotta find the Reference - Z rewrite - scrappy & inefficient, can be optimized

            if (msc_debug_parser_data) file_log("Beginning reference identification\n");

            bool referenceFound = false; // bool to hold whether we've found the reference yet
            bool changeRemoved = false; // bool to hold whether we've ignored the first output to sender as change
            unsigned int potentialReferenceOutputs = 0; // int to hold number of potential reference outputs

            // how many potential reference outputs do we have, if just one select it right here
            BOOST_FOREACH(const string &addr, address_data)
            {
                // keep Michael's original debug info & k int as used elsewhere
                if (msc_debug_parser_data) file_log("ref? data[%d]:%s: %s (%lu.%08lu)\n",
                 k, script_data[k].c_str(), addr.c_str(), value_data[k] / COIN, value_data[k] % COIN);
                ++k;

                if (addr != exodus_address)
                {
                        ++potentialReferenceOutputs;
                        if (1 == potentialReferenceOutputs)
                        {
                                strReference = addr;
                                referenceFound = true;
                                if (msc_debug_parser_data) file_log("Single reference potentially id'd as follows: %s \n", strReference.c_str());
                        }
                        else //as soon as potentialReferenceOutputs > 1 we need to go fishing
                        {
                                strReference = ""; // avoid leaving strReference populated for sanity
                                referenceFound = false;
                                if (msc_debug_parser_data) file_log("More than one potential reference candidate, blanking strReference, need to go fishing\n");
                        }
                }
            }

            // do we have a reference now? or do we need to dig deeper
            if (!referenceFound) // multiple possible reference addresses
            {
                if (msc_debug_parser_data) file_log("Reference has not been found yet, going fishing\n");

                BOOST_FOREACH(const string &addr, address_data)
                {
                        // !!!! address_data is ordered by vout (i think - please confirm that's correct analysis?)
                        if (addr != exodus_address) // removed strSender restriction, not to spec
                        {
                                if ((addr == strSender) && (!changeRemoved))
                                {
                                        // per spec ignore first output to sender as change if multiple possible ref addresses
                                        changeRemoved = true;
                                        if (msc_debug_parser_data) file_log("Removed change\n");
                                }
                                else
                                {
                                        // this may be set several times, but last time will be highest vout
                                        strReference = addr;
                                        if (msc_debug_parser_data) file_log("Resetting strReference as follows: %s \n ", strReference.c_str());
                                }
                        }
                }
            }

          if (msc_debug_parser_data) file_log("Ending reference identification\n");
          if (msc_debug_parser_data) file_log("Final decision on reference identification is: %s\n", strReference.c_str());

          if (msc_debug_parser) file_log("%s(), line %d, file: %s\n", __FUNCTION__, __LINE__, __FILE__);
          // multisig , Class B; get the data packets that are found here
          for (unsigned int k = 0; k<multisig_script_data.size();k++)
          {
            if (msc_debug_parser) file_log("%s(), line %d, file: %s\n", __FUNCTION__, __LINE__, __FILE__);
            CPubKey key(ParseHex(multisig_script_data[k]));
            CKeyID keyID = key.GetID();
            string strAddress = CBitcoinAddress(keyID).ToString();
            char *c_addr_type = (char *)"";
            string strPacket;

            if (msc_debug_parser) file_log("%s(), line %d, file: %s\n", __FUNCTION__, __LINE__, __FILE__);
            {
              // this is a data packet, must deobfuscate now
              vector<unsigned char>hash = ParseHex(strObfuscatedHashes[mdata_count+1]);      
              vector<unsigned char>packet = ParseHex(multisig_script_data[k].substr(2*1,2*PACKET_SIZE));

              for (unsigned int i=0;i<packet.size();i++)
              {
                packet[i] ^= hash[i];
              }

                memcpy(&packets[mdata_count], &packet[0], PACKET_SIZE);
                strPacket = HexStr(packet.begin(),packet.end(), false);
                ++mdata_count;

                if (MAX_PACKETS <= mdata_count)
                {
                  file_log("increase MAX_PACKETS ! mdata_count= %d\n", mdata_count);
                  return -222;
                }
            }

          if (msc_debug_parser_data) file_log("multisig_data[%d]:%s: %s%s\n", k, multisig_script_data[k].c_str(), strAddress.c_str(), c_addr_type);

            if (!strPacket.empty())
            {
              if (msc_debug_parser) file_log("packet #%d: %s\n", mdata_count, strPacket.c_str());
            }
          if (msc_debug_parser) file_log("%s(), line %d, file: %s\n", __FUNCTION__, __LINE__, __FILE__);
          }

          packet_size = mdata_count * (PACKET_SIZE - 1);

          if (sizeof(single_pkt)<packet_size)
          {
            return -111;
          }

          if (msc_debug_parser) file_log("%s(), line %d, file: %s\n", __FUNCTION__, __LINE__, __FILE__);
          } // end of if (fMultisig)
          if (msc_debug_parser) file_log("%s(), line %d, file: %s\n", __FUNCTION__, __LINE__, __FILE__);

            // now decode mastercoin packets
            for (int m=0;m<mdata_count;m++)
            {
              if (msc_debug_parser) file_log("m=%d: %s\n", m, HexStr(packets[m], PACKET_SIZE + packets[m], false).c_str());

              // check to ensure the sequence numbers are sequential and begin with 01 !
              if (1+m != packets[m][0])
              {
                if (msc_debug_spec) file_log("Error: non-sequential seqnum ! expected=%d, got=%d\n", 1+m, packets[m][0]);
              }

              // now ignoring sequence numbers for Class B packets
              memcpy(m*(PACKET_SIZE-1)+single_pkt, 1+packets[m], PACKET_SIZE-1);
            }

  if (msc_debug_verbose) file_log("single_pkt: %s\n", HexStr(single_pkt, packet_size + single_pkt, false).c_str());

  mp_tx->Set(strSender, strReference, 0, wtx.GetHash(), nBlock, idx, (unsigned char *)&single_pkt, packet_size, fMultisig, (inAll-outAll));  

  return 0;
}


// parse blocks, potential right from Mastercoin's Exodus
int msc_initial_scan(int nHeight)
{
int n_total = 0, n_found = 0;
const int max_block = GetHeight();

  // this function is useless if there are not enough blocks in the blockchain yet!
  if ((0 >= nHeight) || (max_block < nHeight)) return -1;

  printf("starting block= %d, max_block= %d\n", nHeight, max_block);

  CBlock block;
  for (int blockNum = nHeight;blockNum<=max_block;blockNum++)
  {
    CBlockIndex* pblockindex = chainActive[blockNum];
    string strBlockHash = pblockindex->GetBlockHash().GetHex();

    if (msc_debug_exo) file_log("%s(%d; max=%d):%s, line %d, file: %s\n",
     __FUNCTION__, blockNum, max_block, strBlockHash.c_str(), __LINE__, __FILE__);

    ReadBlockFromDisk(block, pblockindex);

    int tx_count = 0;
    mastercore_handler_block_begin(blockNum, pblockindex);
    BOOST_FOREACH(const CTransaction&tx, block.vtx)
    {
      if (0 == mastercore_handler_tx(tx, blockNum, tx_count, pblockindex)) n_found++;

      ++tx_count;
    }
    
    n_total += tx_count;

    mastercore_handler_block_end(blockNum, pblockindex, n_found);
  }

  printf("\n");
  for(map<string, CMPTally>::iterator my_it = mp_tally_map.begin(); my_it != mp_tally_map.end(); ++my_it)
  {
    // my_it->first = key
    // my_it->second = value

    printf("%34s => ", (my_it->first).c_str());
    (my_it->second).print();
  }

  printf("starting block= %d, max_block= %d\n", nHeight, max_block);
  printf("n_total= %d, n_found= %d\n", n_total, n_found);

  return 0;
}

int input_msc_balances_string(const string &s)
{
  std::vector<std::string> addrData;
  boost::split(addrData, s, boost::is_any_of("="), token_compress_on);
  if (addrData.size() != 2) {
    return -1;
  }

  string strAddress = addrData[0];

  // split the tuples of properties
  std::vector<std::string> vProperties;
  boost::split(vProperties, addrData[1], boost::is_any_of(";"), token_compress_on);

  std::vector<std::string>::const_iterator iter;
  for (iter = vProperties.begin(); iter != vProperties.end(); ++iter) {
    if ((*iter).empty()) {
      continue;
    }

    std::vector<std::string> curData;
    boost::split(curData, *iter, boost::is_any_of(","), token_compress_on);
    if (curData.size() < 1) {
      // malformed property entry
       return -1;
    }

    size_t delimPos = curData[0].find(':');
    int property = OMNI_PROPERTY_MSC;
    uint64_t balance = 0, sellReserved = 0, acceptReserved = 0;

    if (delimPos != curData[0].npos) {
      property = atoi(curData[0].substr(0,delimPos));
      balance = boost::lexical_cast<boost::uint64_t>(curData[0].substr(delimPos + 1, curData[0].npos));
    } else {
      balance = boost::lexical_cast<boost::uint64_t>(curData[0]);
    }

    if (curData.size() >= 2) {
      sellReserved = boost::lexical_cast<boost::uint64_t>(curData[1]);
    }

    if (curData.size() >= 3) {
      acceptReserved = boost::lexical_cast<boost::uint64_t>(curData[2]);
    }

    if (balance == 0 && sellReserved == 0 && acceptReserved == 0) {
      continue;
    }

    if (balance) update_tally_map(strAddress, property, balance, BALANCE);
    if (sellReserved) update_tally_map(strAddress, property, sellReserved, SELLOFFER_RESERVE);
    if (acceptReserved) update_tally_map(strAddress, property, acceptReserved, ACCEPT_RESERVE);

    // FIXME
    const uint64_t metadexReserve = 0;
    if (metadexReserve) update_tally_map(strAddress, property, sellReserved, METADEX_RESERVE);
  }

  return 1;
}

// seller-address, offer_block, amount, property, desired BTC , property_desired, fee, blocktimelimit
// 13z1JFtDMGTYQvtMq5gs4LmCztK3rmEZga,299076,76375000,1,6415500,0,10000,6
int input_mp_offers_string(const string &s)
{
  int offerBlock;
  uint64_t amountOriginal, btcDesired, minFee, left_forsale;
  unsigned int prop, prop_desired;
  unsigned char blocktimelimit;
  std::vector<std::string> vstr;
  boost::split(vstr, s, boost::is_any_of(" ,="), token_compress_on);
  string sellerAddr;
  string txidStr;
  int i = 0;

  if ((10 != vstr.size()) && (9 != vstr.size())) return -1;

  sellerAddr = vstr[i++];
  offerBlock = atoi(vstr[i++]);
  amountOriginal = boost::lexical_cast<uint64_t>(vstr[i++]);
  prop = boost::lexical_cast<unsigned int>(vstr[i++]);
  btcDesired = boost::lexical_cast<uint64_t>(vstr[i++]); // metadex: amount of desired property
  prop_desired = boost::lexical_cast<unsigned int>(vstr[i++]);
  minFee = boost::lexical_cast<uint64_t>(vstr[i++]);  // metadex: subaction
  blocktimelimit = atoi(vstr[i++]); // metadex: index of tx in block
  txidStr = vstr[i++];

  if (OMNI_PROPERTY_BTC == prop_desired)
  {
  const string combo = STR_SELLOFFER_ADDR_PROP_COMBO(sellerAddr);
  CMPOffer newOffer(offerBlock, amountOriginal, prop, btcDesired, minFee, blocktimelimit, uint256(txidStr));

    if (my_offers.insert(std::make_pair(combo, newOffer)).second)
    {
      return 0;
    }
    else
    {
      return -1;
    }
  }
  else
  {
    assert(10 == vstr.size());

    left_forsale = boost::lexical_cast<uint64_t>(vstr[i++]);

    CMPMetaDEx new_mdex(sellerAddr, offerBlock, prop, amountOriginal, prop_desired, 
    btcDesired, uint256(txidStr), blocktimelimit, (unsigned char) minFee, left_forsale );

    XDOUBLE neworder_price = (XDOUBLE)amountOriginal / (XDOUBLE)btcDesired;

    if (0 >= neworder_price) return METADEX_ERROR -66;

    md_PricesMap temp_prices, *p_prices = get_Prices(prop);
    md_Set temp_indexes, *p_indexes = NULL;

    std::pair<md_Set::iterator,bool> ret;

    if (p_prices) p_indexes = get_Indexes(p_prices, neworder_price);

    if (!p_indexes) p_indexes = &temp_indexes;
    {
      ret = p_indexes->insert(new_mdex);

      if (false == ret.second) return -1;
    }

    if (!p_prices) p_prices = &temp_prices;

    (*p_prices)[neworder_price] = *p_indexes;

    metadex[prop] = *p_prices;
  }

  return 0;
}

// seller-address, property, buyer-address, amount, fee, block
// 13z1JFtDMGTYQvtMq5gs4LmCztK3rmEZga,1, 148EFCFXbk2LrUhEHDfs9y3A5dJ4tttKVd,100000,11000,299126
// 13z1JFtDMGTYQvtMq5gs4LmCztK3rmEZga,1,1Md8GwMtWpiobRnjRabMT98EW6Jh4rEUNy,50000000,11000,299132
int input_mp_accepts_string(const string &s)
{
  int nBlock;
  unsigned char blocktimelimit;
  std::vector<std::string> vstr;
  boost::split(vstr, s, boost::is_any_of(" ,="), token_compress_on);
  uint64_t amountRemaining, amountOriginal, offerOriginal, btcDesired;
  unsigned int prop;
  string sellerAddr, buyerAddr, txidStr;
  int i = 0;

  if (10 != vstr.size()) return -1;

  sellerAddr = vstr[i++];
  prop = boost::lexical_cast<unsigned int>(vstr[i++]);
  buyerAddr = vstr[i++];
  nBlock = atoi(vstr[i++]);
  amountRemaining = boost::lexical_cast<uint64_t>(vstr[i++]);
  amountOriginal = boost::lexical_cast<uint64_t>(vstr[i++]);
  blocktimelimit = atoi(vstr[i++]);
  offerOriginal = boost::lexical_cast<uint64_t>(vstr[i++]);
  btcDesired = boost::lexical_cast<uint64_t>(vstr[i++]);
  txidStr = vstr[i++];

  const string combo = STR_ACCEPT_ADDR_PROP_ADDR_COMBO(sellerAddr, buyerAddr);
  CMPAccept newAccept(amountOriginal, amountRemaining, nBlock, blocktimelimit, prop, offerOriginal, btcDesired, uint256(txidStr));
  if (my_accepts.insert(std::make_pair(combo, newAccept)).second) {
    return 0;
  } else {
    return -1;
  }
}

// exodus_prev
int input_globals_state_string(const string &s)
{
  uint64_t exodusPrev;
  unsigned int nextSPID, nextTestSPID;
  std::vector<std::string> vstr;
  boost::split(vstr, s, boost::is_any_of(" ,="), token_compress_on);
  if (3 != vstr.size()) return -1;

  int i = 0;
  exodusPrev = boost::lexical_cast<uint64_t>(vstr[i++]);
  nextSPID = boost::lexical_cast<unsigned int>(vstr[i++]);
  nextTestSPID = boost::lexical_cast<unsigned int>(vstr[i++]);

  exodus_prev = exodusPrev;
  _my_sps->init(nextSPID, nextTestSPID);
  return 0;
}

// addr,propertyId,nValue,property_desired,deadline,early_bird,percentage,txid
int input_mp_crowdsale_string(const string &s)
{
  string sellerAddr;
  unsigned int propertyId;
  uint64_t nValue;
  unsigned int property_desired;
  uint64_t deadline;
  unsigned char early_bird;
  unsigned char percentage;
  uint64_t u_created;
  uint64_t i_created;

  std::vector<std::string> vstr;
  boost::split(vstr, s, boost::is_any_of(" ,"), token_compress_on);
  unsigned int i = 0;

  if (9 > vstr.size()) return -1;

  sellerAddr = vstr[i++];
  propertyId = atoi(vstr[i++]);
  nValue = boost::lexical_cast<uint64_t>(vstr[i++]);
  property_desired = atoi(vstr[i++]);
  deadline = boost::lexical_cast<uint64_t>(vstr[i++]);
  early_bird = (unsigned char)atoi(vstr[i++]);
  percentage = (unsigned char)atoi(vstr[i++]);
  u_created = boost::lexical_cast<uint64_t>(vstr[i++]);
  i_created = boost::lexical_cast<uint64_t>(vstr[i++]);

  CMPCrowd newCrowdsale(propertyId,nValue,property_desired,deadline,early_bird,percentage,u_created,i_created);

  // load the remaining as database pairs
  while (i < vstr.size()) {
    std::vector<std::string> entryData;
    boost::split(entryData, vstr[i++], boost::is_any_of("="), token_compress_on);
    if ( 2 != entryData.size()) return -1;

    std::vector<std::string> valueData;
    boost::split(valueData, entryData[1], boost::is_any_of(";"), token_compress_on);

    std::vector<uint64_t> vals;
    std::vector<std::string>::const_iterator iter;
    for (iter = valueData.begin(); iter != valueData.end(); ++iter) {
      vals.push_back(boost::lexical_cast<uint64_t>(*iter));
    }

    newCrowdsale.insertDatabase(entryData[0], vals);
  }


  if (my_crowds.insert(std::make_pair(sellerAddr, newCrowdsale)).second) {
    return 0;
  } else {
    return -1;
  }

  return 0;
}

static int msc_file_load(const string &filename, int what, bool verifyHash = false)
{
  int lines = 0;
  int (*inputLineFunc)(const string &) = NULL;

  SHA256_CTX shaCtx;
  SHA256_Init(&shaCtx);

  switch (what)
  {
    case FILETYPE_BALANCES:
      mp_tally_map.clear();
      inputLineFunc = input_msc_balances_string;
      break;

    case FILETYPE_OFFERS:
      my_offers.clear();

      // FIXME
      // memory leak ... gotta unallocate inner layers first....
      // TODO
      // ...
      metadex.clear();

      inputLineFunc = input_mp_offers_string;
      break;

    case FILETYPE_ACCEPTS:
      my_accepts.clear();
      inputLineFunc = input_mp_accepts_string;
      break;

    case FILETYPE_GLOBALS:
      inputLineFunc = input_globals_state_string;
      break;

    case FILETYPE_CROWDSALES:
      my_crowds.clear();
      inputLineFunc = input_mp_crowdsale_string;
      break;

    default:
      return -1;
  }

  if (msc_debug_persistence)
  {
    LogPrintf("Loading %s ... \n", filename);
    file_log("%s(%s), line %d, file: %s\n", __FUNCTION__, filename.c_str(), __LINE__, __FILE__);
  }

  std::ifstream file;
  file.open(filename.c_str());
  if (!file.is_open())
  {
    if (msc_debug_persistence) LogPrintf("%s(%s): file not found, line %d, file: %s\n", __FUNCTION__, filename.c_str(), __LINE__, __FILE__);
    return -1;
  }

  int res = 0;

  std::string fileHash;
  while (file.good())
  {
    std::string line;
    std::getline(file, line);
    if (line.empty() || line[0] == '#') continue;

    // remove \r if the file came from Windows
    line.erase( std::remove( line.begin(), line.end(), '\r' ), line.end() ) ;

    // record and skip hashes in the file
    if (line[0] == '!') {
      fileHash = line.substr(1);
      continue;
    }

    // update hash?
    if (verifyHash) {
      SHA256_Update(&shaCtx, line.c_str(), line.length());
    }

    if (inputLineFunc) {
      if (inputLineFunc(line) < 0) {
        res = -1;
        break;
      }
    }

    ++lines;
  }

  file.close();

  if (verifyHash && res == 0) {
    // generate and wite the double hash of all the contents written
    uint256 hash1;
    SHA256_Final((unsigned char*)&hash1, &shaCtx);
    uint256 hash2;
    SHA256((unsigned char*)&hash1, sizeof(hash1), (unsigned char*)&hash2);

    if (false == boost::iequals(hash2.ToString(), fileHash)) {
      file_log("File %s loaded, but failed hash validation!\n", filename.c_str());
      res = -1;
    }
  }

  file_log("%s(%s), loaded lines= %d, res= %d\n", __FUNCTION__, filename.c_str(), lines, res);
  LogPrintf("%s(): file: %s , loaded lines= %d, res= %d\n", __FUNCTION__, filename, lines, res);

  return res;
}

static char const * const statePrefix[NUM_FILETYPES] = {
    "balances",
    "offers",
    "accepts",
    "globals",
    "crowdsales",
};

// returns the height of the state loaded
static int load_most_relevant_state()
{
  int res = -1;
  // check the SP database and roll it back to its latest valid state
  // according to the active chain
  uint256 spWatermark;
  if (0 > _my_sps->getWatermark(spWatermark)) {
    //trigger a full reparse, if the SP database has no watermark
    return -1;
  }

  CBlockIndex const *spBlockIndex = GetBlockIndex(spWatermark);
  if (NULL == spBlockIndex) {
    //trigger a full reparse, if the watermark isn't a real block
    return -1;
  }

  while (NULL != spBlockIndex && false == chainActive.Contains(spBlockIndex)) {
    int remainingSPs = _my_sps->popBlock(*spBlockIndex->phashBlock);
    if (remainingSPs < 0) {
      // trigger a full reparse, if the levelDB cannot roll back
      return -1;
    } /*else if (remainingSPs == 0) {
      // potential optimization here?
    }*/
    spBlockIndex = spBlockIndex->pprev;
    if (spBlockIndex != NULL) {
        _my_sps->setWatermark(*spBlockIndex->phashBlock);
    }
  }

  // prepare a set of available files by block hash pruning any that are
  // not in the active chain
  std::set<uint256> persistedBlocks;
  boost::filesystem::directory_iterator dIter(MPPersistencePath);
  boost::filesystem::directory_iterator endIter;
  for (; dIter != endIter; ++dIter) {
    if (false == boost::filesystem::is_regular_file(dIter->status()) || dIter->path().empty()) {
      // skip funny business
      continue;
    }

    std::string fName = (*--dIter->path().end()).string();
    std::vector<std::string> vstr;
    boost::split(vstr, fName, boost::is_any_of("-."), token_compress_on);
    if (  vstr.size() == 3 &&
          boost::equals(vstr[2], "dat")) {
      uint256 blockHash;
      blockHash.SetHex(vstr[1]);
      CBlockIndex *pBlockIndex = GetBlockIndex(blockHash);
      if (pBlockIndex == NULL || false == chainActive.Contains(pBlockIndex)) {
        continue;
      }

      // this is a valid block in the active chain, store it
      persistedBlocks.insert(blockHash);
    }
  }

  // using the SP's watermark after its fixed-up as the tip
  // walk backwards until we find a valid and full set of persisted state files
  // for each block we discard, roll back the SP database
  CBlockIndex const *curTip = spBlockIndex;
  while (NULL != curTip && persistedBlocks.size() > 0) {
    if (persistedBlocks.find(*spBlockIndex->phashBlock) != persistedBlocks.end()) {
      int success = -1;
      for (int i = 0; i < NUM_FILETYPES; ++i) {
        const string filename = (MPPersistencePath / (boost::format("%s-%s.dat") % statePrefix[i] % curTip->GetBlockHash().ToString()).str().c_str()).string();
        success = msc_file_load(filename, i, true);
        if (success < 0) {
          break;
        }
      }

      if (success >= 0) {
        res = curTip->nHeight;
        break;
      }

      // remove this from the persistedBlock Set
      persistedBlocks.erase(*spBlockIndex->phashBlock);
    }

    // go to the previous block
    if (0 > _my_sps->popBlock(*curTip->phashBlock)) {
      // trigger a full reparse, if the levelDB cannot roll back
      return -1;
    }
    curTip = curTip->pprev;
    if (curTip != NULL) {
        _my_sps->setWatermark(*curTip->phashBlock);
    }
  }

  if (persistedBlocks.size() == 0) {
    // trigger a reparse if we exhausted the persistence files without success
    return -1;
  }

  // return the height of the block we settled at
  return res;
}

static int write_msc_balances(ofstream &file, SHA256_CTX *shaCtx)
{
  LOCK(cs_tally);

  map<string, CMPTally>::iterator iter;
  for (iter = mp_tally_map.begin(); iter != mp_tally_map.end(); ++iter) {
    bool emptyWallet = true;

    string lineOut = (*iter).first;
    lineOut.append("=");
    CMPTally &curAddr = (*iter).second;
    curAddr.init();
    unsigned int prop = 0;
    while (0 != (prop = curAddr.next())) {
      uint64_t balance = (*iter).second.getMoney(prop, BALANCE);
      uint64_t sellReserved = (*iter).second.getMoney(prop, SELLOFFER_RESERVE);
      uint64_t acceptReserved = (*iter).second.getMoney(prop, ACCEPT_RESERVE);
      const uint64_t metadexReserve = (*iter).second.getMoney(prop, METADEX_RESERVE);

      // we don't allow 0 balances to read in, so if we don't write them
      // it makes things match up better between persisted state and processed state
      if ( 0 == balance && 0 == sellReserved && 0 == acceptReserved  && 0 == metadexReserve ) {
        continue;
      }

      emptyWallet = false;

      lineOut.append((boost::format("%d:%d,%d,%d,%d;")
          % prop
          % balance
          % sellReserved
          % acceptReserved
          % metadexReserve).str());

    }

    if (false == emptyWallet) {
      // add the line to the hash
      SHA256_Update(shaCtx, lineOut.c_str(), lineOut.length());

      // write the line
      file << lineOut << endl;
    }
  }

  return 0;
}

static int write_mp_offers(ofstream &file, SHA256_CTX *shaCtx)
{
  OfferMap::const_iterator iter;
  for (iter = my_offers.begin(); iter != my_offers.end(); ++iter) {
    // decompose the key for address
    std::vector<std::string> vstr;
    boost::split(vstr, (*iter).first, boost::is_any_of("-"), token_compress_on);
    CMPOffer const &offer = (*iter).second;
    offer.saveOffer(file, shaCtx, vstr[0]);
  }


  return 0;
}

static int write_mp_metadex(ofstream &file, SHA256_CTX *shaCtx)
{
  for (md_PropertiesMap::iterator my_it = metadex.begin(); my_it != metadex.end(); ++my_it)
  {
    md_PricesMap & prices = my_it->second;
    for (md_PricesMap::iterator it = prices.begin(); it != prices.end(); ++it)
    {
      md_Set & indexes = (it->second);
      for (md_Set::iterator it = indexes.begin(); it != indexes.end(); ++it)
      {
        CMPMetaDEx meta = *it;
        meta.saveOffer(file, shaCtx);
      }
    }
  }

  return 0;
}

static int write_mp_accepts(ofstream &file, SHA256_CTX *shaCtx)
{
  AcceptMap::const_iterator iter;
  for (iter = my_accepts.begin(); iter != my_accepts.end(); ++iter) {
    // decompose the key for address
    std::vector<std::string> vstr;
    boost::split(vstr, (*iter).first, boost::is_any_of("-+"), token_compress_on);
    CMPAccept const &accept = (*iter).second;
    accept.saveAccept(file, shaCtx, vstr[0], vstr[1]);
  }

  return 0;
}

static int write_globals_state(ofstream &file, SHA256_CTX *shaCtx)
{
  unsigned int nextSPID = _my_sps->peekNextSPID(OMNI_PROPERTY_MSC);
  unsigned int nextTestSPID = _my_sps->peekNextSPID(OMNI_PROPERTY_TMSC);
  string lineOut = (boost::format("%d,%d,%d")
    % exodus_prev
    % nextSPID
    % nextTestSPID
    ).str();

  // add the line to the hash
  SHA256_Update(shaCtx, lineOut.c_str(), lineOut.length());

  // write the line
  file << lineOut << endl;

  return 0;
}

static int write_mp_crowdsales(ofstream &file, SHA256_CTX *shaCtx)
{
  CrowdMap::const_iterator iter;
  for (iter = my_crowds.begin(); iter != my_crowds.end(); ++iter) {
    // decompose the key for address
    CMPCrowd const &crowd = (*iter).second;
    crowd.saveCrowdSale(file, shaCtx, (*iter).first);
  }

  return 0;
}

static int write_state_file( CBlockIndex const *pBlockIndex, int what )
{
  const char *blockHash = pBlockIndex->GetBlockHash().ToString().c_str();
  boost::filesystem::path balancePath = MPPersistencePath / (boost::format("%s-%s.dat") % statePrefix[what] % blockHash).str();
  ofstream file;
  file.open(balancePath.string().c_str());

  SHA256_CTX shaCtx;
  SHA256_Init(&shaCtx);

  int result = 0;

  switch(what) {
  case FILETYPE_BALANCES:
    result = write_msc_balances(file, &shaCtx);
    break;

  case FILETYPE_OFFERS:
    result = write_mp_offers(file, &shaCtx);
    result += write_mp_metadex(file, &shaCtx);
    break;

  case FILETYPE_ACCEPTS:
    result = write_mp_accepts(file, &shaCtx);
    break;

  case FILETYPE_GLOBALS:
    result = write_globals_state(file, &shaCtx);
    break;

  case FILETYPE_CROWDSALES:
      result = write_mp_crowdsales(file, &shaCtx);
      break;
  }

  // generate and wite the double hash of all the contents written
  uint256 hash1;
  SHA256_Final((unsigned char*)&hash1, &shaCtx);
  uint256 hash2;
  SHA256((unsigned char*)&hash1, sizeof(hash1), (unsigned char*)&hash2);
  file << "!" << hash2.ToString() << endl;

  file.flush();
  file.close();
  return result;
}

static bool is_state_prefix( std::string const &str )
{
  for (int i = 0; i < NUM_FILETYPES; ++i) {
    if (boost::equals(str,  statePrefix[i])) {
      return true;
    }
  }

  return false;
}

static void prune_state_files( CBlockIndex const *topIndex )
{
  // build a set of blockHashes for which we have any state files
  std::set<uint256> statefulBlockHashes;

  boost::filesystem::directory_iterator dIter(MPPersistencePath);
  boost::filesystem::directory_iterator endIter;
  for (; dIter != endIter; ++dIter) {
    std::string fName = dIter->path().empty() ? "<invalid>" : (*--dIter->path().end()).string();
    if (false == boost::filesystem::is_regular_file(dIter->status())) {
      // skip funny business
      file_log("Non-regular file found in persistence directory : %s\n", fName.c_str());
      continue;
    }

    std::vector<std::string> vstr;
    boost::split(vstr, fName, boost::is_any_of("-."), token_compress_on);
    if (  vstr.size() == 3 &&
          is_state_prefix(vstr[0]) &&
          boost::equals(vstr[2], "dat")) {
      uint256 blockHash;
      blockHash.SetHex(vstr[1]);
      statefulBlockHashes.insert(blockHash);
    } else {
      file_log("None state file found in persistence directory : %s\n", fName.c_str());
    }
  }

  // for each blockHash in the set, determine the distance from the given block
  std::set<uint256>::const_iterator iter;
  for (iter = statefulBlockHashes.begin(); iter != statefulBlockHashes.end(); ++iter) {
    // look up the CBlockIndex for height info
    CBlockIndex const *curIndex = GetBlockIndex(*iter);

    // if we have nothing int the index, or this block is too old..
    if (NULL == curIndex || (topIndex->nHeight - curIndex->nHeight) > MAX_STATE_HISTORY ) {
     if (msc_debug_persistence)
     {
      if (curIndex) {
        file_log("State from Block:%s is no longer need, removing files (age-from-tip: %d)\n", (*iter).ToString().c_str(), topIndex->nHeight - curIndex->nHeight);
      } else {
        file_log("State from Block:%s is no longer need, removing files (not in index)\n", (*iter).ToString().c_str());
      }
     }

      // destroy the associated files!
      const char *blockHashStr = (*iter).ToString().c_str();
      for (int i = 0; i < NUM_FILETYPES; ++i) {
        boost::filesystem::remove(MPPersistencePath / (boost::format("%s-%s.dat") % statePrefix[i] % blockHashStr).str());
      }
    }
  }
}

int mastercore_save_state( CBlockIndex const *pBlockIndex )
{
  // write the new state as of the given block
  write_state_file(pBlockIndex, FILETYPE_BALANCES);
  write_state_file(pBlockIndex, FILETYPE_OFFERS);
  write_state_file(pBlockIndex, FILETYPE_ACCEPTS);
  write_state_file(pBlockIndex, FILETYPE_GLOBALS);
  write_state_file(pBlockIndex, FILETYPE_CROWDSALES);

  // clean-up the directory
  prune_state_files(pBlockIndex);

  _my_sps->setWatermark(*pBlockIndex->phashBlock);

  return 0;
}

static void clear_all_state() {
  mp_tally_map.clear();
  my_offers.clear();
  my_accepts.clear();
  my_crowds.clear();
  _my_sps->clear();
  exodus_prev = 0;
}

// called from init.cpp of Bitcoin Core
int mastercore_init()
{
  if (mastercoreInitialized) {
    // nothing to do
    return 0;
  }

  printf("%s()%s, line %d, file: %s\n", __FUNCTION__, isNonMainNet() ? "TESTNET":"", __LINE__, __FILE__);

  shrinkDebugFile();

  file_log("\n%s OMNICORE INIT, build date: " __DATE__ " " __TIME__ "\n\n", DateTimeStrFormat("%Y-%m-%d %H:%M:%S", GetTime()).c_str());

  if (isNonMainNet())
  {
    exodus_address = exodus_testnet;
  }
  //If interested in changing regtest address do so here and uncomment
  /*if (RegTest())
  {
    exodus_address = exodus_testnet;
  }*/

  // check for --startclean option and delete MP_ folders if present
  if (GetBoolArg("-startclean", false))
  {
      file_log("Process was started with --startclean option, attempting to clear persistence files...");
      try
      {
          boost::filesystem::path persistPath = GetDataDir() / "MP_persist";
          boost::filesystem::path txlistPath = GetDataDir() / "MP_txlist";
          boost::filesystem::path tradePath = GetDataDir() / "MP_tradelist";
          boost::filesystem::path spPath = GetDataDir() / "MP_spinfo";
          boost::filesystem::path stoPath = GetDataDir() / "MP_stolist";
          if (boost::filesystem::exists(persistPath)) boost::filesystem::remove_all(persistPath);
          if (boost::filesystem::exists(txlistPath)) boost::filesystem::remove_all(txlistPath);
          if (boost::filesystem::exists(tradePath)) boost::filesystem::remove_all(tradePath);
          if (boost::filesystem::exists(spPath)) boost::filesystem::remove_all(spPath);
          if (boost::filesystem::exists(stoPath)) boost::filesystem::remove_all(stoPath);
          file_log("Success clearing persistence files (did not raise any exceptions).");
      }
      catch(boost::filesystem::filesystem_error const & e)
      {
          file_log("Exception deleting folders for --startclean option.\n");
          printf("Exception deleting folders for --startclean option.\n");
      }
  }

  t_tradelistdb = new CMPTradeList(GetDataDir() / "MP_tradelist", 1<<20, false, fReindex);
  s_stolistdb = new CMPSTOList(GetDataDir() / "MP_stolist", 1<<20, false, fReindex);
  p_txlistdb = new CMPTxList(GetDataDir() / "MP_txlist", 1<<20, false, fReindex);
  _my_sps = new CMPSPInfo(GetDataDir() / "MP_spinfo");
  MPPersistencePath = GetDataDir() / "MP_persist";
  boost::filesystem::create_directories(MPPersistencePath);

  // legacy code, setting to pre-genesis-block
  static int snapshotHeight = (GENESIS_BLOCK - 1);
  static const uint64_t snapshotDevMSC = 0;

  if (isNonMainNet()) snapshotHeight = START_TESTNET_BLOCK - 1;

  if (RegTest()) snapshotHeight = START_REGTEST_BLOCK - 1;

  ++mastercoreInitialized;

  if (readPersistence())
  {
    nWaterlineBlock = load_most_relevant_state();
    if (nWaterlineBlock < 0) {
      // persistence says we reparse!, nuke some stuff in case the partial loads left stale bits
      clear_all_state();
    }

    if (nWaterlineBlock < snapshotHeight)
    {
      nWaterlineBlock = snapshotHeight;
      exodus_prev=snapshotDevMSC;
    }

    // advance the waterline so that we start on the next unaccounted for block
    nWaterlineBlock += 1;
  }
  else
  {
  // my old way
    nWaterlineBlock = GENESIS_BLOCK - 1;  // the DEX block

    if (TestNet()) nWaterlineBlock = START_TESTNET_BLOCK; //testnet3

    if (RegTest()) nWaterlineBlock = START_REGTEST_BLOCK; //testnet3

#ifdef  MY_HACK
//    nWaterlineBlock = MSC_DEX_BLOCK-3;
//    if (isNonMainNet()) nWaterlineBlock = 272700;

#if 0
    if (isNonMainNet()) nWaterlineBlock = 272790;

    update_tally_map("mfaiZGBkY4mBqt3PHPD2qWgbaafGa7vR64" , 1 , 500000, BALANCE);
    update_tally_map("mxaYwMv2Brbs7CW9r5aYuEr1jKTSDXg1TH" , 1 , 100000, BALANCE);

    update_tally_map("mfaiZGBkY4mBqt3PHPD2qWgbaafGa7vR64" , 2 , 500000, BALANCE);
    update_tally_map("mxaYwMv2Brbs7CW9r5aYuEr1jKTSDXg1TH" , 2 , 100000, BALANCE);

    update_tally_map("mfaiZGBkY4mBqt3PHPD2qWgbaafGa7vR64" , 3 , 500000, BALANCE);
    update_tally_map("mxaYwMv2Brbs7CW9r5aYuEr1jKTSDXg1TH" , 3 , 100000, BALANCE);

    update_tally_map("mfaiZGBkY4mBqt3PHPD2qWgbaafGa7vR64" , 2147483652 , 500000, BALANCE);
    update_tally_map("mxaYwMv2Brbs7CW9r5aYuEr1jKTSDXg1TH" , 2147483652 , 100000, BALANCE);

    update_tally_map("mfaiZGBkY4mBqt3PHPD2qWgbaafGa7vR64" , 2147483660 , 500000, BALANCE);
    update_tally_map("mxaYwMv2Brbs7CW9r5aYuEr1jKTSDXg1TH" , 2147483660 , 100000, BALANCE);

    update_tally_map("mfaiZGBkY4mBqt3PHPD2qWgbaafGa7vR64" , 2147483661 , 500000, BALANCE);
    update_tally_map("mxaYwMv2Brbs7CW9r5aYuEr1jKTSDXg1TH" , 2147483661 , 100000, BALANCE);
#endif
#endif
  }

  // collect the real Exodus balances available at the snapshot time
  // redundant? do we need to show it both pre-parse and post-parse?  if so let's label the printfs accordingly
  exodus_balance = getMPbalance(exodus_address, OMNI_PROPERTY_MSC, BALANCE);
  printf("[Snapshot] Exodus balance: %s\n", FormatDivisibleMP(exodus_balance).c_str());

  // check out levelDB for the most recently stored alert and load it into global_alert_message then check if expired
  (void) p_txlistdb->setLastAlert(nWaterlineBlock);
  // initial scan
  (void) msc_initial_scan(nWaterlineBlock);

  // display Exodus balance
  exodus_balance = getMPbalance(exodus_address, OMNI_PROPERTY_MSC, BALANCE);
  printf("[Initialized] Exodus balance: %s\n", FormatDivisibleMP(exodus_balance).c_str());

  return 0;
}

int mastercore_shutdown()
{
  printf("%s(), line %d, file: %s\n", __FUNCTION__, __LINE__, __FILE__);

  if (p_txlistdb)
  {
    delete p_txlistdb; p_txlistdb = NULL;
  }
  if (t_tradelistdb)
  {
    delete t_tradelistdb; t_tradelistdb = NULL;
  }
  if (s_stolistdb)
  {
    delete s_stolistdb; s_stolistdb = NULL;
  }
  file_log("\n%s OMNICORE SHUTDOWN, build date: " __DATE__ " " __TIME__ "\n\n", DateTimeStrFormat("%Y-%m-%d %H:%M:%S", GetTime()).c_str());

  if (_my_sps)
  {
    delete _my_sps; _my_sps = NULL;
  }

  return 0;
}

// this is called for every new transaction that comes in (actually in block parsing loop)
int mastercore_handler_tx(const CTransaction &tx, int nBlock, unsigned int idx, CBlockIndex const * pBlockIndex)
{
  if (!mastercoreInitialized) {
    mastercore_init();
  }

  // clear pending, if any
  // NOTE1: Every incoming TX is checked, not just MP-ones because:
  //  if for some reason the incoming TX doesn't pass our parser validation steps successfuly, I'd still want to clear pending amounts for that TX.
  // NOTE2: Plus I wanna clear the amount before that TX is parsed by our protocol, in case we ever consider pending amounts in internal calculations.
  (void) pendingDelete(tx.GetHash(), true);

CMPTransaction mp_obj;
// save the augmented offer or accept amount into the database as well (expecting them to be numerically lower than that in the blockchain)
int interp_ret = -555555, pop_ret;

  if (nBlock < nWaterlineBlock) return -1;  // we do not care about parsing blocks prior to our waterline (empty blockchain defense)

  pop_ret = parseTransaction(false, tx, nBlock, idx, &mp_obj, pBlockIndex->GetBlockTime() );
  if (0 == pop_ret)
  {
  // true MP transaction, validity (such as insufficient funds, or offer not found) is determined elsewhere

    interp_ret = mp_obj.interpretPacket();
    if (interp_ret) file_log("!!! interpretPacket() returned %d !!!\n", interp_ret);

    // of course only MP-related TXs get recorded
    if (!disableLevelDB)
    {
    bool bValid = (0 <= interp_ret);

      p_txlistdb->recordTX(tx.GetHash(), bValid, nBlock, mp_obj.getType(), mp_obj.getNewAmount());
    }
  }

  return interp_ret;
}

// IsMine wrapper to determine whether the address is in our local wallet
bool IsMyAddress(const std::string &address) 
{
  if (!pwalletMain) return false;

  const CBitcoinAddress& mscaddress = address;

  CTxDestination lookupaddress = mscaddress.Get(); 

  return (IsMine(*pwalletMain, lookupaddress));
}

// gets a label for a Bitcoin address from the wallet, mainly to the UI (used in demo)
string getLabel(const string &address)
{
CWallet *wallet = pwalletMain;

  if (wallet)
   {
        LOCK(wallet->cs_wallet);
        CBitcoinAddress address_parsed(address);
        std::map<CTxDestination, CAddressBookData>::iterator mi = wallet->mapAddressBook.find(address_parsed.Get());
        if (mi != wallet->mapAddressBook.end())
        {
            return (mi->second.name);
        }
    }

  return string();
}

//
// Determines minimum output amount to be spent by an output based on
// scriptPubKey size in relation to the minimum relay fee.
// 
// This method is very related with IsDust(nMinRelayTxFee) in core.h.
//
int64_t GetDustLimit(const CScript& scriptPubKey)
{
    // The total size is based on a typical scriptSig size of 148 byte,
    // 8 byte accounted for the size of output value and the serialized
    // size of scriptPubKey.
    size_t nSize = ::GetSerializeSize(scriptPubKey, SER_DISK, 0) + 156;

    // The minimum relay fee dictates a threshold value under which a
    // transaction won't be relayed.
    int64_t nRelayTxFee = (::minRelayTxFee).GetFee(nSize);

    // A transaction is considered as "dust", if less than 1/3 of the
    // minimum fee required to relay a transaction is spent by one of
    // it's outputs. The minimum relay fee is defined per 1000 byte.
    int64_t nDustLimit = nRelayTxFee * 3;

    return nDustLimit;
}

static int64_t selectCoins(const string &FromAddress, CCoinControl &coinControl, int64_t additional)
{
  CWallet *wallet = pwalletMain;
  int64_t n_max = (COIN * (20 * (0.0001))); // assume 20KBytes max TX size at 0.0001 per kilobyte
  // FUTURE: remove n_max and try 1st smallest input, then 2 smallest inputs etc. -- i.e. move Coin Control selection closer to CreateTransaction
  int64_t n_total = 0;  // total output funds collected

  // if referenceamount is set it is needed to be accounted for here too
  if (0 < additional) n_max += additional;

  LOCK2(cs_main, wallet->cs_wallet);

    string sAddress = "";

    // iterate over the wallet
    for (map<uint256, CWalletTx>::iterator it = wallet->mapWallet.begin();
        it != wallet->mapWallet.end(); ++it) {
      const uint256& wtxid = it->first;
      const CWalletTx* pcoin = &(*it).second;
      bool bIsMine;
      bool bIsSpent;

      if (pcoin->IsTrusted()) {
        const int64_t nAvailable = pcoin->GetAvailableCredit();

        if (!nAvailable)
          continue;

        for (unsigned int i = 0; i < pcoin->vout.size(); i++) {
          CTxDestination dest;

          if (!ExtractDestination(pcoin->vout[i].scriptPubKey, dest))
            continue;

          bIsMine = IsMine(*wallet, dest);
          bIsSpent = wallet->IsSpent(wtxid, i);

          if (!bIsMine || bIsSpent)
            continue;

          int64_t n = bIsSpent ? 0 : pcoin->vout[i].nValue;

          sAddress = CBitcoinAddress(dest).ToString();
          if (msc_debug_tokens)
            file_log("%s:IsMine()=%s:IsSpent()=%s:%s: i=%d, nValue= %lu\n",
                sAddress.c_str(), bIsMine ? "yes" : "NO",
                bIsSpent ? "YES" : "no", wtxid.ToString().c_str(), i, n);

          // only use funds from the Sender's address for our MP transaction
          // TODO: may want to a little more selective here, i.e. use smallest possible (~0.1 BTC), but large amounts lead to faster confirmations !
          if (FromAddress == sAddress) {
            COutPoint outpt(wtxid, i);
            coinControl.Select(outpt);

            n_total += n;

            if (n_max <= n_total)
              break;
          }
        } // for pcoin end
      }

      if (n_max <= n_total)
        break;
    } // for iterate over the wallet end

// return 0;
return n_total;
}

int64_t feeCheck(const string &address)
{
    // check the supplied address against selectCoins to determine if sufficient fees for send
    CCoinControl coinControl;
    return selectCoins(address, coinControl, 0);
}

//
// Do we care if this is true: pubkeys[i].IsCompressed() ???
// returns 0 if everything is OK, the transaction was sent
int mastercore::ClassB_send(const string &senderAddress, const string &receiverAddress, const string &redemptionAddress, const vector<unsigned char> &data, uint256 & txid, int64_t referenceamount)
{
CWallet *wallet = pwalletMain;
CCoinControl coinControl;
vector< pair<CScript, int64_t> > vecSend;

  // pick inputs for this transaction
  if (0 > selectCoins(senderAddress, coinControl, referenceamount))
  {
    return MP_INPUTS_INVALID;
  }

  txid = 0;

  // determine the redeeming public key for our multisig outputs
  // partially copied from _createmultisig()
  CBitcoinAddress address;
  CPubKey redeemingPubKey;
  if (false == redemptionAddress.empty()) {
    address.SetString(redemptionAddress);
  } else {
    address.SetString(senderAddress);
  }

  // validate that the redemption Address is good
  if (wallet && address.IsValid())
  {
    if (address.IsScript())
    {
      file_log("%s() ERROR: Redemption Address must be specified !\n", __FUNCTION__);
      return MP_REDEMP_ILLEGAL;
    }
    else
    {
      CKeyID keyID;

      if (!address.GetKeyID(keyID))
        return MP_REDEMP_BAD_KEYID;

      if (!bRawTX)
      {
      if (!wallet->GetPubKey(keyID, redeemingPubKey))
        return MP_REDEMP_FETCH_ERR_PUBKEY;

      if (!redeemingPubKey.IsFullyValid())
        return MP_REDEMP_INVALID_PUBKEY;
      }
     }
  }
  else return MP_REDEMP_BAD_VALIDATION;

  int nRemainingBytes = data.size();
  int nNextByte = 0;
  unsigned char seqNum = 1;
  string strObfuscatedHashes[1+MAX_SHA256_OBFUSCATION_TIMES];
  prepareObfuscatedHashes(senderAddress, strObfuscatedHashes);

  while (nRemainingBytes > 0) {
    int nKeys = 1; //assume one key of data since we have data remaining
    if (nRemainingBytes > (PACKET_SIZE - 1)) {
      // we have enough data for 2 keys in this output
      nKeys += 1;
    }

    vector<CPubKey> keys;
    // always include the redeeming pubkey
    keys.push_back(redeemingPubKey);

    int i;
    for (i = 0; i < nKeys; i++)
    {
      // add sequence number
      vector<unsigned char> fakeKey;
      fakeKey.push_back(seqNum);

      // add up to 30 bytes of data
      int numBytes = nRemainingBytes < (PACKET_SIZE - 1) ? nRemainingBytes: (PACKET_SIZE - 1);
      fakeKey.insert(fakeKey.end(), data.begin() + nNextByte, data.begin() + nNextByte + numBytes);
      nNextByte += numBytes;
      nRemainingBytes -= numBytes;

      // pad to 31 total bytes with zeros
      while (fakeKey.size() < PACKET_SIZE) {
        fakeKey.push_back(0);
      }

      // xor in the obfuscation
      int j;
      vector<unsigned char>hash = ParseHex(strObfuscatedHashes[seqNum]);
      for (j = 0; j < PACKET_SIZE; j++) {
        fakeKey[j] = fakeKey[j] ^ hash[j];
      }

      // prepend the 2
      fakeKey.insert(fakeKey.begin(), 2);

      // fix up the ecdsa code point
      CPubKey pubKey;
      fakeKey.resize(33);
      unsigned char random_byte = (unsigned char)(GetRand(256));
      for (j = 0; i < 256 ; j++)
      {
        fakeKey[32] = random_byte;

        pubKey = CPubKey(fakeKey);
        file_log("pubKey check: %s\n", (HexStr(pubKey.begin(), pubKey.end()).c_str()));

        if (pubKey.IsFullyValid()) break;

        ++random_byte; // cycle 256 times, if we must to find a valid ECDSA point
      }

      keys.push_back(pubKey);
      seqNum++;
    }

    CScript multisig_output = GetScriptForMultisig(1, keys);
    vecSend.push_back(make_pair(multisig_output, GetDustLimit(multisig_output)));
  }

  CWalletTx wtxNew;
  int64_t nFeeRet = 0;
  std::string strFailReason;
  CReserveKey reserveKey(wallet);

  CBitcoinAddress addr = CBitcoinAddress(senderAddress);  // change goes back to us
  coinControl.destChange = addr.Get();

  if (!wallet) return MP_ERR_WALLET_ACCESS;

  // add the the reference/recepient/receiver ouput if needed
  if (!receiverAddress.empty())
  {
    // Send To Owners is the first use case where the receiver is empty
    CScript scriptPubKey = GetScriptForDestination(CBitcoinAddress(receiverAddress).Get());
    vecSend.push_back(make_pair(scriptPubKey, 0 < referenceamount ? referenceamount : GetDustLimit(scriptPubKey)));
  }

  // add the marker output
  CScript scriptPubKey = GetScriptForDestination(CBitcoinAddress(exodus_address).Get());
  vecSend.push_back(make_pair(scriptPubKey, GetDustLimit(scriptPubKey)));

  // selected in the parent function, i.e.: ensure we are only using the address passed in as the Sender
  if (!coinControl.HasSelected()) return MP_ERR_INPUTSELECT_FAIL;

  LOCK(wallet->cs_wallet);

  // the fee will be computed by Bitcoin Core, need an override (?)
  // TODO: look at Bitcoin Core's global: nTransactionFee (?)
  if (!wallet->CreateTransaction(vecSend, wtxNew, reserveKey, nFeeRet, strFailReason, &coinControl)) return MP_ERR_CREATE_TX;

  if (bRawTX)
  {
    CTransaction tx = (CTransaction) wtxNew;
    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
    ssTx << tx;
    string strHex = HexStr(ssTx.begin(), ssTx.end());
    printf("RawTX:\n%s\n\n", strHex.c_str());

    return 0;
  }

  file_log("%s():%s; nFeeRet = %lu, line %d, file: %s\n", __FUNCTION__, wtxNew.ToString().c_str(), nFeeRet, __LINE__, __FILE__);

  if (!wallet->CommitTransaction(wtxNew, reserveKey)) return MP_ERR_COMMIT_TX;

  txid = wtxNew.GetHash();

  return 0;
}

// WIP: expanding the function to a general-purpose one, but still sending 1 packet only for now (30-31 bytes)
uint256 mastercore::send_INTERNAL_1packet(const string &FromAddress, const string &ToAddress, const string &RedeemAddress, unsigned int PropertyID, uint64_t Amount, unsigned int PropertyID_2, uint64_t Amount_2, unsigned int TransactionType, int64_t additional, int *error_code)
{
const int64_t iAvailable = getMPbalance(FromAddress, PropertyID, BALANCE);
const int64_t iUserAvailable = getUserAvailableMPbalance(FromAddress, PropertyID);
int rc = MP_INSUF_FUNDS_BPENDI;
uint256 txid = 0;
const int64_t amount = Amount;
const unsigned int prop = PropertyID;

  if (msc_debug_send) file_log("%s(From: %s , To: %s , Property= %u, Amount= %lu, Available= %ld, Pending= %ld)\n",
   __FUNCTION__, FromAddress.c_str(), ToAddress.c_str(), PropertyID, Amount, iAvailable, iUserAvailable);

  if (!isRangeOK(Amount))
  {
    rc = MP_INPUT_NOT_IN_RANGE;
    if (error_code) *error_code = rc;

    return 0;
  }

  bool bCancel_checkBypass = false;
  //If doing a METADEX CANCEL, use following flag to bypass funds checks
  if((TransactionType == MSC_TYPE_METADEX) && ((additional == CMPTransaction::CANCEL_AT_PRICE) || (additional == CMPTransaction::CANCEL_ALL_FOR_PAIR) || (additional == CMPTransaction::CANCEL_EVERYTHING)))
  {
    bCancel_checkBypass = true;
  } 

  // make sure this address has enough MP property available!
  if ((((uint64_t)iAvailable < Amount) || (0 == Amount)) && !bCancel_checkBypass)
  {
    LogPrintf("%s(): aborted -- not enough MP property (%lu < %lu)\n", __FUNCTION__, iAvailable, Amount);
    if (msc_debug_send) file_log("%s(): aborted -- not enough MP property (%lu < %lu)\n", __FUNCTION__, iAvailable, Amount);

    if (error_code) *error_code = rc;

    return 0;
  }

  // check once more, this time considering PENDING amount reduction
  // make sure this address has enough MP property available!
  if (((iUserAvailable < (int64_t)Amount) || (0 == Amount)) && !bCancel_checkBypass)
  {
    LogPrintf("%s(): aborted -- not enough MP property with PENDING reduction (%lu < %lu)\n", __FUNCTION__, iUserAvailable, Amount);
    if (msc_debug_send) file_log("%s(): aborted -- not enough MP property with PENDING reduction (%lu < %lu)\n", __FUNCTION__, iUserAvailable, Amount);

    rc = MP_INSUF_FUNDS_APENDI;
    if (error_code) *error_code = rc;

    return 0;
  }

  int64_t rawTransactionType = TransactionType;
  int64_t rawPropertyID = PropertyID;
  int64_t rawPropertyID_2 = PropertyID_2;
  int64_t rawAmount = Amount;
  int64_t rawAmount_2 = Amount_2;
  vector<unsigned char> data;
  swapByteOrder32(TransactionType);
  swapByteOrder32(PropertyID);
  swapByteOrder64(Amount);

  PUSH_BACK_BYTES(data, TransactionType);
  PUSH_BACK_BYTES(data, PropertyID);
  PUSH_BACK_BYTES(data, Amount);
  
  if (PropertyID_2 != 0 || bCancel_checkBypass == true) // for trade_MP
  {
    //use additional to pass action byte
    unsigned char action = boost::lexical_cast<int>(additional); 
    //zero out additional for trade_MP
    additional = 0;

    swapByteOrder32(PropertyID_2);
    swapByteOrder64(Amount_2);

    PUSH_BACK_BYTES(data, PropertyID_2);
    PUSH_BACK_BYTES(data, Amount_2);
    PUSH_BACK_BYTES(data, action);
  }

  rc = ClassB_send(FromAddress, ToAddress, RedeemAddress, data, txid, additional);
  if (msc_debug_send) file_log("ClassB_send returned %d\n", rc);

  if (error_code) *error_code = rc;

  if (0 == rc)
  {
      // only simple sends and metadex pending needed at moment
      Object txobj;
      txobj.push_back(Pair("txid", txid.GetHex()));
      txobj.push_back(Pair("sendingaddress", FromAddress));
      if (rawTransactionType == MSC_TYPE_SIMPLE_SEND) txobj.push_back(Pair("referenceaddress", ToAddress));
      txobj.push_back(Pair("confirmations", 0));
      // txobj->push_back(Pair("fee", ValueFromAmount(nFee)));
      txobj.push_back(Pair("version", (int64_t)0)); //we only send v0 currently so all pending v0
      txobj.push_back(Pair("type_int", (int64_t)rawTransactionType));
      bool divisible = false;
      bool desiredDivisible = false;
      string amountStr;
      string amountDStr;
      switch (rawTransactionType)
      {
          case 0: //simple send
              txobj.push_back(Pair("type", "Simple send"));
              txobj.push_back(Pair("propertyid", rawPropertyID));
              divisible = isPropertyDivisible(rawPropertyID);
              txobj.push_back(Pair("divisible", divisible));
              if (divisible) { amountStr = FormatDivisibleMP(rawAmount); } else { amountStr = FormatIndivisibleMP(rawAmount); }
              txobj.push_back(Pair("amount", amountStr));
          break;
          case 21: //metadex sell
              txobj.push_back(Pair("type", "MetaDEx token trade"));
              divisible = isPropertyDivisible(rawPropertyID);
              desiredDivisible = isPropertyDivisible(rawPropertyID_2);
              if (divisible) { amountStr = FormatDivisibleMP(rawAmount); } else { amountStr = FormatIndivisibleMP(rawAmount); }
              if (desiredDivisible) { amountDStr = FormatDivisibleMP(rawAmount_2); } else { amountDStr = FormatIndivisibleMP(rawAmount_2); }
              txobj.push_back(Pair("amountoffered", amountStr));
              txobj.push_back(Pair("propertyoffered", rawPropertyID));
              txobj.push_back(Pair("propertyofferedisdivisible", divisible));
              txobj.push_back(Pair("amountdesired", amountDStr));
              txobj.push_back(Pair("propertydesired", rawPropertyID_2));
              txobj.push_back(Pair("propertydesiredisdivisible", desiredDivisible));
              txobj.push_back(Pair("action", additional));
          break;
      }
      string txDesc = write_string(Value(txobj), false);
      (void) pendingAdd(txid, FromAddress, prop, amount, rawTransactionType, txDesc);
  }

  return txid;
}

int CMPTxList::setLastAlert(int blockHeight)
{
    if (blockHeight > chainActive.Height()) blockHeight = chainActive.Height();
    if (!pdb) return 0;
    Slice skey, svalue;
    readoptions.fill_cache = false;
    Iterator* it = pdb->NewIterator(readoptions);
    string lastAlertTxid;
    string lastAlertData;
    string itData;
    int64_t lastAlertBlock = 0;
    for(it->SeekToFirst(); it->Valid(); it->Next())
    {
       skey = it->key();
       svalue = it->value();
       string itData = svalue.ToString().c_str();
       std::vector<std::string> vstr;
       boost::split(vstr, itData, boost::is_any_of(":"), token_compress_on);
       // we expect 5 tokens
       if (4 == vstr.size())
       {
           if (atoi(vstr[2]) == OMNICORE_MESSAGE_TYPE_ALERT) // is it an alert?
           {
               if (atoi(vstr[0]) == 1) // is it valid?
               {
                   if ( (atoi(vstr[1]) > lastAlertBlock) && (atoi(vstr[1]) < blockHeight) ) // is it the most recent and within our parsed range?
                   {
                      lastAlertTxid = skey.ToString().c_str();
                      lastAlertData = svalue.ToString().c_str();
                      lastAlertBlock = atoi(vstr[1]);
                   }
               }
           }
       }
    }
    delete it;

    // if lastAlertTxid is not empty, load the alert and see if it's still valid - if so, copy to global_alert_message
    if(lastAlertTxid.empty())
    {
        file_log("DEBUG ALERT No alerts found to load\n");
    }
    else
    {
        file_log("DEBUG ALERT Loading lastAlertTxid %s\n", lastAlertTxid);

        // reparse lastAlertTxid
        uint256 hash;
        hash.SetHex(lastAlertTxid);
        CTransaction wtx;
        uint256 blockHash = 0;
        if (!GetTransaction(hash, wtx, blockHash, true)) //can't find transaction
        {
            file_log("DEBUG ALERT Unable to load lastAlertTxid, transaction not found\n");
        }
        else // note reparsing here is unavoidable because we've only loaded a txid and have no other alert info stored
        {
            CMPTransaction mp_obj;
            int parseRC = parseTransaction(true, wtx, blockHeight, 0, &mp_obj);
            string new_global_alert_message;
            if (0 <= parseRC)
            {
                if (0<=mp_obj.step1())
                {
                    if(65535 == mp_obj.getType())
                    {
                        if (0 == mp_obj.step2_Alert(&new_global_alert_message))
                        {
                            global_alert_message = new_global_alert_message;
                            // check if expired
                            CBlockIndex* mpBlockIndex = chainActive[blockHeight];
                            (void) checkExpiredAlerts(blockHeight, mpBlockIndex->GetBlockTime());
                        }
                    }
                }
            }
        }
    }
    return 0;
}

uint256 CMPTxList::findMetaDExCancel(const uint256 txid)
{
  std::vector<std::string> vstr;
  string txidStr = txid.ToString();
  Slice skey, svalue;
  readoptions.fill_cache = false;
  uint256 cancelTxid;
  Iterator* it = pdb->NewIterator(readoptions);
  for(it->SeekToFirst(); it->Valid(); it->Next())
  {
      skey = it->key();
      svalue = it->value();
      string svalueStr = svalue.ToString().c_str();
      boost::split(vstr, svalueStr, boost::is_any_of(":"), token_compress_on);
      // obtain the existing affected tx count
      if (3 <= vstr.size())
      {
          if (vstr[0] == txidStr) { delete it; cancelTxid.SetHex(skey.ToString().c_str()); return cancelTxid; }
      }
  }

  delete it;
  return 0;
}

int CMPTxList::getNumberOfMetaDExCancels(const uint256 txid)
{
    if (!pdb) return 0;
    int numberOfCancels = 0;
    std::vector<std::string> vstr;
    string strValue;
    Status status = pdb->Get(readoptions, txid.ToString() + "-C", &strValue);
    if (status.ok())
    {
        // parse the string returned
        boost::split(vstr, strValue, boost::is_any_of(":"), token_compress_on);
        // obtain the number of cancels
        if (4 <= vstr.size())
        {
            numberOfCancels = atoi(vstr[3]);
        }
    }
    return numberOfCancels;
}

int CMPTxList::getNumberOfPurchases(const uint256 txid)
{
    if (!pdb) return 0;
    int numberOfPurchases = 0;
    std::vector<std::string> vstr;
    string strValue;
    Status status = pdb->Get(readoptions, txid.ToString(), &strValue);
    if (status.ok())
    {
        // parse the string returned
        boost::split(vstr, strValue, boost::is_any_of(":"), token_compress_on);
        // obtain the number of purchases
        if (4 <= vstr.size())
        {
            numberOfPurchases = atoi(vstr[3]);
        }
    }
    return numberOfPurchases;
}

int CMPTxList::getMPTransactionCountTotal()
{
    int count = 0;
    Slice skey, svalue;
    readoptions.fill_cache = false;
    Iterator* it = pdb->NewIterator(readoptions);
    for(it->SeekToFirst(); it->Valid(); it->Next())
    {
        skey = it->key();
        if (skey.ToString().length() == 64) { ++count; } //extra entries for cancels and purchases are more than 64 chars long
    }
    delete it;
    return count;
}

int CMPTxList::getMPTransactionCountBlock(int block)
{
    int count = 0;
    Slice skey, svalue;
    readoptions.fill_cache = false;
    Iterator* it = pdb->NewIterator(readoptions);
    for(it->SeekToFirst(); it->Valid(); it->Next())
    {
        skey = it->key();
        svalue = it->value();
        if (skey.ToString().length() == 64)
        {
            string strValue = svalue.ToString().c_str();
            std::vector<std::string> vstr;
            boost::split(vstr, strValue, boost::is_any_of(":"), token_compress_on);
            if (4 == vstr.size())
            {
                if (atoi(vstr[1]) == block) { ++count; }
            }
        }
    }
    delete it;
    return count;
}

string CMPTxList::getKeyValue(string key)
{
    if (!pdb) return "";
    string strValue;
    Status status = pdb->Get(readoptions, key, &strValue);
    if (status.ok()) { return strValue; } else { return ""; }
}

bool CMPTxList::getPurchaseDetails(const uint256 txid, int purchaseNumber, string *buyer, string *seller, uint64_t *vout, uint64_t *propertyId, uint64_t *nValue)
{
    if (!pdb) return 0;
    std::vector<std::string> vstr;
    string strValue;
    Status status = pdb->Get(readoptions, txid.ToString()+"-"+to_string(purchaseNumber), &strValue);
    if (status.ok())
    {
        // parse the string returned
        boost::split(vstr, strValue, boost::is_any_of(":"), token_compress_on);
        // obtain the requisite details
        if (5 == vstr.size())
        {
            *vout = atoi(vstr[0]);
            *buyer = vstr[1];
            *seller = vstr[2];
            *propertyId = atoi(vstr[3]);
            *nValue = boost::lexical_cast<boost::uint64_t>(vstr[4]);;
            return true;
        }
    }
    return false;
}

void CMPTxList::recordMetaDExCancelTX(const uint256 &txidMaster, const uint256 &txidSub, bool fValid, int nBlock, unsigned int propertyId, uint64_t nValue)
{
  if (!pdb) return;

       // Prep - setup vars
       unsigned int type = 99992104;
       unsigned int refNumber = 1;
       uint64_t existingAffectedTXCount = 0;
       string txidMasterStr = txidMaster.ToString() + "-C";

       // Step 1 - Check TXList to see if this cancel TXID exists
       // Step 2a - If doesn't exist leave number of affected txs & ref set to 1
       // Step 2b - If does exist add +1 to existing ref and set this ref as new number of affected
       std::vector<std::string> vstr;
       string strValue;
       Status status = pdb->Get(readoptions, txidMasterStr, &strValue);
       if (status.ok())
       {
           // parse the string returned
           boost::split(vstr, strValue, boost::is_any_of(":"), token_compress_on);

           // obtain the existing affected tx count
           if (4 <= vstr.size())
           {
               existingAffectedTXCount = atoi(vstr[3]);
               refNumber = existingAffectedTXCount + 1;
           }
       }

       // Step 3 - Create new/update master record for cancel tx in TXList
       const string key = txidMasterStr;
       const string value = strprintf("%u:%d:%u:%lu", fValid ? 1:0, nBlock, type, refNumber);
       file_log("METADEXCANCELDEBUG : Writing master record %s(%s, valid=%s, block= %d, type= %d, number of affected transactions= %d)\n", __FUNCTION__, txidMaster.ToString().c_str(), fValid ? "YES":"NO", nBlock, type, refNumber);
       if (pdb)
       {
           status = pdb->Put(writeoptions, key, value);
           file_log("METADEXCANCELDEBUG : %s(): %s, line %d, file: %s\n", __FUNCTION__, status.ToString().c_str(), __LINE__, __FILE__);
       }

       // Step 4 - Write sub-record with cancel details
       const string txidStr = txidMaster.ToString() + "-C";
       const string subKey = STR_REF_SUBKEY_TXID_REF_COMBO(txidStr);
       const string subValue = strprintf("%s:%d:%lu", txidSub.ToString(), propertyId, nValue);
       Status subStatus;
       file_log("METADEXCANCELDEBUG : Writing sub-record %s with value %s\n", subKey.c_str(), subValue.c_str());
       if (pdb)
       {
           subStatus = pdb->Put(writeoptions, subKey, subValue);
           file_log("METADEXCANCELDEBUG : %s(): %s, line %d, file: %s\n", __FUNCTION__, subStatus.ToString().c_str(), __LINE__, __FILE__);
       }
}

void CMPTxList::recordPaymentTX(const uint256 &txid, bool fValid, int nBlock, unsigned int vout, unsigned int propertyId, uint64_t nValue, string buyer, string seller)
{
  if (!pdb) return;

       // Prep - setup vars
       unsigned int type = 99999999;
       uint64_t numberOfPayments = 1;
       unsigned int paymentNumber = 1;
       uint64_t existingNumberOfPayments = 0;

       // Step 1 - Check TXList to see if this payment TXID exists
       bool paymentEntryExists = p_txlistdb->exists(txid);

       // Step 2a - If doesn't exist leave number of payments & paymentNumber set to 1
       // Step 2b - If does exist add +1 to existing number of payments and set this paymentNumber as new numberOfPayments
       if (paymentEntryExists)
       {
           //retrieve old numberOfPayments
           std::vector<std::string> vstr;
           string strValue;
           Status status = pdb->Get(readoptions, txid.ToString(), &strValue);
           if (status.ok())
           {
               // parse the string returned
               boost::split(vstr, strValue, boost::is_any_of(":"), token_compress_on);

               // obtain the existing number of payments
               if (4 <= vstr.size())
               {
                   existingNumberOfPayments = atoi(vstr[3]);
                   paymentNumber = existingNumberOfPayments + 1;
                   numberOfPayments = existingNumberOfPayments + 1;
               }
           }
       }

       // Step 3 - Create new/update master record for payment tx in TXList
       const string key = txid.ToString();
       const string value = strprintf("%u:%d:%u:%lu", fValid ? 1:0, nBlock, type, numberOfPayments); 
       Status status;
       file_log("DEXPAYDEBUG : Writing master record %s(%s, valid=%s, block= %d, type= %d, number of payments= %lu)\n", __FUNCTION__, txid.ToString().c_str(), fValid ? "YES":"NO", nBlock, type, numberOfPayments);
       if (pdb)
       {
           status = pdb->Put(writeoptions, key, value);
           file_log("DEXPAYDEBUG : %s(): %s, line %d, file: %s\n", __FUNCTION__, status.ToString().c_str(), __LINE__, __FILE__);
       }

       // Step 4 - Write sub-record with payment details
       const string txidStr = txid.ToString();
       const string subKey = STR_PAYMENT_SUBKEY_TXID_PAYMENT_COMBO(txidStr);
       const string subValue = strprintf("%d:%s:%s:%d:%lu", vout, buyer, seller, propertyId, nValue);
       Status subStatus;
       file_log("DEXPAYDEBUG : Writing sub-record %s with value %s\n", subKey.c_str(), subValue.c_str());
       if (pdb)
       {
           subStatus = pdb->Put(writeoptions, subKey, subValue);
           file_log("DEXPAYDEBUG : %s(): %s, line %d, file: %s\n", __FUNCTION__, subStatus.ToString().c_str(), __LINE__, __FILE__);
       }
}

void CMPTxList::recordTX(const uint256 &txid, bool fValid, int nBlock, unsigned int type, uint64_t nValue)
{
  if (!pdb) return;

  // overwrite detection, we should never be overwriting a tx, as that means we have redone something a second time
  // reorgs delete all txs from levelDB above reorg_chain_height
  if (p_txlistdb->exists(txid)) file_log("LEVELDB TX OVERWRITE DETECTION - %s\n", txid.ToString().c_str());

const string key = txid.ToString();
const string value = strprintf("%u:%d:%u:%lu", fValid ? 1:0, nBlock, type, nValue);
Status status;

  file_log("%s(%s, valid=%s, block= %d, type= %d, value= %lu)\n",
   __FUNCTION__, txid.ToString().c_str(), fValid ? "YES":"NO", nBlock, type, nValue);

  if (pdb)
  {
    status = pdb->Put(writeoptions, key, value);
    ++nWritten;
    if (msc_debug_txdb) file_log("%s(): %s, line %d, file: %s\n", __FUNCTION__, status.ToString().c_str(), __LINE__, __FILE__);
  }
}

bool CMPTxList::exists(const uint256 &txid)
{
  if (!pdb) return false;

string strValue;
Status status = pdb->Get(readoptions, txid.ToString(), &strValue);

  if (!status.ok())
  {
    if (status.IsNotFound()) return false;
  }

  return true;
}

bool CMPTxList::getTX(const uint256 &txid, string &value)
{
Status status = pdb->Get(readoptions, txid.ToString(), &value);

  ++nRead;

  if (status.ok())
  {
    return true;
  }

  return false;
}

void CMPTxList::printStats()
{
  file_log("CMPTxList stats: nWritten= %d , nRead= %d\n", nWritten, nRead);
}

void CMPTxList::printAll()
{
int count = 0;
Slice skey, svalue;

  readoptions.fill_cache = false;

  Iterator* it = pdb->NewIterator(readoptions);

  for(it->SeekToFirst(); it->Valid(); it->Next())
  {
    skey = it->key();
    svalue = it->value();
    ++count;
    printf("entry #%8d= %s:%s\n", count, skey.ToString().c_str(), svalue.ToString().c_str());
  }

  delete it;
}

// figure out if there was at least 1 Master Protocol transaction within the block range, or a block if starting equals ending
// block numbers are inclusive
// pass in bDeleteFound = true to erase each entry found within the block range
bool CMPTxList::isMPinBlockRange(int starting_block, int ending_block, bool bDeleteFound)
{
leveldb::Slice skey, svalue;
unsigned int count = 0;
std::vector<std::string> vstr;
int block;
unsigned int n_found = 0;

  leveldb::Iterator* it = pdb->NewIterator(iteroptions);

  for(it->SeekToFirst(); it->Valid(); it->Next())
  {
    skey = it->key();
    svalue = it->value();

    ++count;

//    printf("%5u:%s=%s\n", count, skey.ToString().c_str(), svalue.ToString().c_str());

    string strvalue = it->value().ToString();

    // parse the string returned, find the validity flag/bit & other parameters
    boost::split(vstr, strvalue, boost::is_any_of(":"), token_compress_on);

    // only care about the block number/height here
    if (2 <= vstr.size())
    {
      block = atoi(vstr[1]);

      if ((starting_block <= block) && (block <= ending_block))
      {
        ++n_found;
        file_log("%s() DELETING: %s=%s\n", __FUNCTION__, skey.ToString().c_str(), svalue.ToString().c_str());
        if (bDeleteFound) pdb->Delete(writeoptions, skey);
      }
    }
  }

  printf("%s(%d, %d); n_found= %d\n", __FUNCTION__, starting_block, ending_block, n_found);

  delete it;

  return (n_found);
}

// MPSTOList here
std::string CMPSTOList::getMySTOReceipts(string filterAddress)
{
  if (!sdb) return "";
  string mySTOReceipts = "";

  Slice skey, svalue;
  readoptions.fill_cache = false;
  Iterator* it = sdb->NewIterator(readoptions);
  for(it->SeekToFirst(); it->Valid(); it->Next())
  {
      skey = it->key();
      string recipientAddress = skey.ToString();
      if(!IsMyAddress(recipientAddress)) continue; // not ours, not interested
      if((!filterAddress.empty()) && (filterAddress != recipientAddress)) continue; // not the filtered address
      // ours, get info
      svalue = it->value();
      string strValue = svalue.ToString();
      // break into individual receipts
      std::vector<std::string> vstr;
      boost::split(vstr, strValue, boost::is_any_of(","), token_compress_on);
      for(uint32_t i = 0; i<vstr.size(); i++)
      {
          // add to array
          std::vector<std::string> svstr;
          boost::split(svstr, vstr[i], boost::is_any_of(":"), token_compress_on);
          if(4 == svstr.size())
          {
              size_t txidMatch = mySTOReceipts.find(svstr[0]);
              if(txidMatch==std::string::npos) mySTOReceipts += svstr[0]+":"+svstr[1]+":"+recipientAddress+":"+svstr[2]+",";
          }
      }
  }
  delete it;
  return mySTOReceipts;
}

void CMPSTOList::getRecipients(const uint256 txid, string filterAddress, Array *recipientArray, uint64_t *total, uint64_t *stoFee)
{
  if (!sdb) return;

  bool filter = true; //default
  bool filterByWallet = true; //default
  bool filterByAddress = false; //default

  if (filterAddress == "*") filter = false;
  if ((filterAddress != "") && (filterAddress != "*")) { filterByWallet = false; filterByAddress = true; }

  // iterate through SDB, dropping all records where key is not filterAddress (if filtering)
  int count = 0;

  // ugly way to do this, really we should store the fee used but for now since we know it is
  // always num_addresses * 0.00000001 MSC we can recalculate it on the fly
  *stoFee = 0;

  Slice skey, svalue;
  readoptions.fill_cache = false;
  Iterator* it = sdb->NewIterator(readoptions);
  for(it->SeekToFirst(); it->Valid(); it->Next())
  {
      skey = it->key();
      string recipientAddress = skey.ToString();
      svalue = it->value();
      string strValue = svalue.ToString();
      // see if txid is in the data
      size_t txidMatch = strValue.find(txid.ToString());
      if(txidMatch!=std::string::npos)
      {
          ++*stoFee;
          // the txid exists inside the data, this address was a recipient of this STO, check filter and add the details
          if(filter)
          {
              if( ( (filterByAddress) && (filterAddress == recipientAddress) ) || ( (filterByWallet) && (IsMyAddress(recipientAddress)) ) )
              { } else { continue; } // move on if no filter match (but counter still increased for fee)
          }
          std::vector<std::string> vstr;
          boost::split(vstr, strValue, boost::is_any_of(","), token_compress_on);
          for(uint32_t i = 0; i<vstr.size(); i++)
          {
              std::vector<std::string> svstr;
              boost::split(svstr, vstr[i], boost::is_any_of(":"), token_compress_on);
              if(4 == svstr.size())
              {
                  if(svstr[0] == txid.ToString())
                  {
                      //add data to array
                      uint64_t amount = 0;
                      uint64_t propertyId = 0;
                      try
                      {
                          amount = boost::lexical_cast<uint64_t>(svstr[3]);
                          propertyId = boost::lexical_cast<uint64_t>(svstr[2]);
                      } catch (const boost::bad_lexical_cast &e)
                      {
                          file_log("DEBUG STO - error in converting values from leveldb\n");
                          delete it;
                          return; //(something went wrong)
                      }
                      Object recipient;
                      recipient.push_back(Pair("address", recipientAddress));
                      if(isPropertyDivisible(propertyId))
                      {
                         recipient.push_back(Pair("amount", FormatDivisibleMP(amount)));
                      }
                      else
                      {
                         recipient.push_back(Pair("amount", FormatIndivisibleMP(amount)));
                      }
                      *total += amount;
                      recipientArray->push_back(recipient);
                      ++count;
                  }
              }
          }
      }
  }

  delete it;
  return;
}

bool CMPSTOList::exists(string address)
{
  if (!sdb) return false;

  string strValue;
  Status status = sdb->Get(readoptions, address, &strValue);

  if (!status.ok())
  {
    if (status.IsNotFound()) return false;
  }

  return true;
}

void CMPSTOList::recordSTOReceive(string address, const uint256 &txid, int nBlock, unsigned int propertyId, uint64_t amount)
{
  if (!sdb) return;

  bool addressExists = s_stolistdb->exists(address);
  if (addressExists)
  {
      //retrieve existing record
      std::vector<std::string> vstr;
      string strValue;
      Status status = sdb->Get(readoptions, address, &strValue);
      if (status.ok())
      {
          // add details to record
          // see if we are overwriting (check)
          size_t txidMatch = strValue.find(txid.ToString());
          if(txidMatch!=std::string::npos) file_log("STODEBUG : Duplicating entry for %s : %s\n",address,txid.ToString());

          const string key = address;
          const string newValue = strprintf("%s:%d:%u:%lu,", txid.ToString(), nBlock, propertyId, amount);
          strValue += newValue;
          // write updated record
          Status status;
          if (sdb)
          {
              status = sdb->Put(writeoptions, key, strValue);
              file_log("STODBDEBUG : %s(): %s, line %d, file: %s\n", __FUNCTION__, status.ToString().c_str(), __LINE__, __FILE__);
          }
      }
  }
  else
  {
      const string key = address;
      const string value = strprintf("%s:%d:%u:%lu,", txid.ToString(), nBlock, propertyId, amount);
      Status status;
      if (sdb)
      {
          status = sdb->Put(writeoptions, key, value);
          file_log("STODBDEBUG : %s(): %s, line %d, file: %s\n", __FUNCTION__, status.ToString().c_str(), __LINE__, __FILE__);
      }
  }
}

void CMPSTOList::printAll()
{
  int count = 0;
  Slice skey, svalue;

  readoptions.fill_cache = false;

  Iterator* it = sdb->NewIterator(readoptions);

  for(it->SeekToFirst(); it->Valid(); it->Next())
  {
    skey = it->key();
    svalue = it->value();
    ++count;
    printf("entry #%8d= %s:%s\n", count, skey.ToString().c_str(), svalue.ToString().c_str());
  }

  delete it;
}

void CMPSTOList::printStats()
{
  file_log("CMPSTOList stats: tWritten= %d , tRead= %d\n", sWritten, sRead);
}

// delete any STO receipts after blockNum
int CMPSTOList::deleteAboveBlock(int blockNum)
{
  leveldb::Slice skey, svalue;
  unsigned int count = 0;
  std::vector<std::string> vstr;
  unsigned int n_found = 0;
  leveldb::Iterator* it = sdb->NewIterator(iteroptions);
  for(it->SeekToFirst(); it->Valid(); it->Next())
  {
    skey = it->key();
    string address = skey.ToString();
    svalue = it->value();
    ++count;
    string strvalue = it->value().ToString();
    boost::split(vstr, strvalue, boost::is_any_of(","), token_compress_on);
    string newValue = "";
    bool needsUpdate = false;
    for(uint32_t i = 0; i<vstr.size(); i++)
    {
        std::vector<std::string> svstr;
        boost::split(svstr, vstr[i], boost::is_any_of(":"), token_compress_on);
        if(4 == svstr.size())
        {
            if(atoi(svstr[1]) <= blockNum) { newValue += vstr[i]; } else { needsUpdate = true; } // add back to new key
        }
    }

    if(needsUpdate)
    {
        ++n_found;
        const string key = address;
        // write updated record
        Status status;
        if (sdb)
        {
            status = sdb->Put(writeoptions, key, newValue);
            file_log("DEBUG STO - rewriting STO data after reorg\n");
            file_log("STODBDEBUG : %s(): %s, line %d, file: %s\n", __FUNCTION__, status.ToString().c_str(), __LINE__, __FILE__);
        }
    }
  }

  printf("%s(%d); stodb n_found= %d\n", __FUNCTION__, blockNum, n_found);

  delete it;

  return (n_found);
}

// MPTradeList here
bool CMPTradeList::getMatchingTrades(const uint256 txid, unsigned int propertyId, Array *tradeArray, uint64_t *totalSold, uint64_t *totalBought)
{
  if (!tdb) return false;
  leveldb::Slice skey, svalue;
  unsigned int count = 0;
  std::vector<std::string> vstr;
  string txidStr = txid.ToString();
  leveldb::Iterator* it = tdb->NewIterator(iteroptions);
  for(it->SeekToFirst(); it->Valid(); it->Next())
  {
      skey = it->key();
      svalue = it->value();
      string strkey = it->key().ToString();
      size_t txidMatch = strkey.find(txidStr);
      if(txidMatch!=std::string::npos)
      {
          // we have a matching trade involving this txid
          // get the txid of the match
          string matchTxid;
          // make sure string is correct length
          if (strkey.length() == 129)
          {
              if (txidMatch==0) { matchTxid = strkey.substr(65,64); } else { matchTxid = strkey.substr(0,64); }

              string strvalue = it->value().ToString();
              boost::split(vstr, strvalue, boost::is_any_of(":"), token_compress_on);
              if (7 == vstr.size()) // ensure there are the expected amount of tokens
              {
                  string address;
                  string address1 = vstr[0];
                  string address2 = vstr[1];
                  unsigned int prop1 = 0;
                  unsigned int prop2 = 0;
                  uint64_t uAmount1 = 0;
                  uint64_t uAmount2 = 0;
                  uint64_t nBought = 0;
                  uint64_t nSold = 0;
                  string amountBought;
                  string amountSold;
                  string amount1;
                  string amount2;
                  prop1 = boost::lexical_cast<unsigned int>(vstr[2]);
                  prop2 = boost::lexical_cast<unsigned int>(vstr[3]);
                  uAmount1 = boost::lexical_cast<uint64_t>(vstr[4]);
                  uAmount2 = boost::lexical_cast<uint64_t>(vstr[5]);
                  if(isPropertyDivisible(prop1))
                     { amount1 = FormatDivisibleMP(uAmount1); }
                  else
                     { amount1 = FormatIndivisibleMP(uAmount1); }
                  if(isPropertyDivisible(prop2))
                     { amount2 = FormatDivisibleMP(uAmount2); }
                  else
                     { amount2 = FormatIndivisibleMP(uAmount2); }

                  // correct orientation of trade
                  if (prop1 == propertyId)
                  {
                      address = address1;
                      amountBought = amount2;
                      amountSold = amount1;
                      nBought = uAmount2;
                      nSold = uAmount1;
                  }
                  else
                  {
                      address = address2;
                      amountBought = amount1;
                      amountSold = amount2;
                      nBought = uAmount1;
                      nSold = uAmount2;
                  }
                  int blockNum = atoi(vstr[6]);
                  Object trade;
                  trade.push_back(Pair("txid", matchTxid));
                  trade.push_back(Pair("address", address));
                  trade.push_back(Pair("block", blockNum));
                  trade.push_back(Pair("amountsold", amountSold));
                  trade.push_back(Pair("amountbought", amountBought));
                  tradeArray->push_back(trade);
                  ++count;
                  *totalBought += nBought;
                  *totalSold += nSold;
              }
          }
      }
  }
  delete it;
  if (count) { return true; } else { return false; }
}

void CMPTradeList::recordTrade(const uint256 txid1, const uint256 txid2, string address1, string address2, unsigned int prop1, unsigned int prop2, uint64_t amount1, uint64_t amount2, int blockNum)
{
  if (!tdb) return;
  const string key = txid1.ToString() + "+" + txid2.ToString();
  const string value = strprintf("%s:%s:%u:%u:%lu:%lu:%d", address1, address2, prop1, prop2, amount1, amount2, blockNum);
  Status status;
  if (tdb)
  {
    status = tdb->Put(writeoptions, key, value);
    ++tWritten;
    if (msc_debug_tradedb) file_log("%s(): %s\n", __FUNCTION__, status.ToString().c_str());
  }
}

// delete any trades after blockNum
int CMPTradeList::deleteAboveBlock(int blockNum)
{
  leveldb::Slice skey, svalue;
  unsigned int count = 0;
  std::vector<std::string> vstr;
  int block;
  unsigned int n_found = 0;
  leveldb::Iterator* it = tdb->NewIterator(iteroptions);
  for(it->SeekToFirst(); it->Valid(); it->Next())
  {
    skey = it->key();
    svalue = it->value();
    ++count;
    string strvalue = it->value().ToString();
    boost::split(vstr, strvalue, boost::is_any_of(":"), token_compress_on);
    // only care about the block number/height here
    if (7 == vstr.size())
    {
      block = atoi(vstr[6]);

      if (block >= blockNum)
      {
        ++n_found;
        file_log("%s() DELETING FROM TRADEDB: %s=%s\n", __FUNCTION__, skey.ToString().c_str(), svalue.ToString().c_str());
        tdb->Delete(writeoptions, skey);
      }
    }
  }

  printf("%s(%d); tradedb n_found= %d\n", __FUNCTION__, blockNum, n_found);

  delete it;

  return (n_found);
}

void CMPTradeList::printStats()
{
  file_log("CMPTradeList stats: tWritten= %d , tRead= %d\n", tWritten, tRead);
}

int CMPTradeList::getMPTradeCountTotal()
{
    int count = 0;
    Slice skey, svalue;
    readoptions.fill_cache = false;
    Iterator* it = tdb->NewIterator(readoptions);
    for(it->SeekToFirst(); it->Valid(); it->Next())
    {
        ++count;
    }
    delete it;
    return count;
}

void CMPTradeList::printAll()
{
  int count = 0;
  Slice skey, svalue;

  readoptions.fill_cache = false;

  Iterator* it = tdb->NewIterator(readoptions);

  for(it->SeekToFirst(); it->Valid(); it->Next())
  {
    skey = it->key();
    svalue = it->value();
    ++count;
    printf("entry #%8d= %s:%s\n", count, skey.ToString().c_str(), svalue.ToString().c_str());
  }

  delete it;
}

// global wrapper, block numbers are inclusive, if ending_block is 0 top of the chain will be used
bool mastercore::isMPinBlockRange(int starting_block, int ending_block, bool bDeleteFound)
{
  if (!p_txlistdb) return false;

  if (0 == ending_block) ending_block = GetHeight(); // will scan 'til the end

  return p_txlistdb->isMPinBlockRange(starting_block, ending_block, bDeleteFound);
}

// call it like so (variable # of parameters):
// int block = 0;
// ...
// uint64_t nNew = 0;
//
// if (getValidMPTX(txid, &block, &type, &nNew)) // if true -- the TX is a valid MP TX
//
bool mastercore::getValidMPTX(const uint256 &txid, int *block, unsigned int *type, uint64_t *nAmended)
{
string result;
int validity = 0;

  if (msc_debug_txdb) file_log("%s()\n", __FUNCTION__);

  if (!p_txlistdb) return false;

  if (!p_txlistdb->getTX(txid, result)) return false;

  // parse the string returned, find the validity flag/bit & other parameters
  std::vector<std::string> vstr;
  boost::split(vstr, result, boost::is_any_of(":"), token_compress_on);

  file_log("%s() size=%lu : %s\n", __FUNCTION__, vstr.size(), result.c_str());

  if (1 <= vstr.size()) validity = atoi(vstr[0]);

  if (block)
  {
    if (2 <= vstr.size()) *block = atoi(vstr[1]);
    else *block = 0;
  }

  if (type)
  {
    if (3 <= vstr.size()) *type = atoi(vstr[2]);
    else *type = 0;
  }

  if (nAmended)
  {
    if (4 <= vstr.size()) *nAmended = boost::lexical_cast<boost::uint64_t>(vstr[3]);
    else nAmended = 0;
  }

  p_txlistdb->printStats();

  if ((int)0 == validity) return false;

  return true;
}

int mastercore_handler_block_begin(int nBlockPrev, CBlockIndex const * pBlockIndex) {
  if (reorgRecoveryMode > 0) {
    reorgRecoveryMode = 0;  // clear reorgRecovery here as this is likely re-entrant

    // reset states
    if(!readPersistence()) clear_all_state();
    p_txlistdb->isMPinBlockRange(pBlockIndex->nHeight, reorgRecoveryMaxHeight, true);
    t_tradelistdb->deleteAboveBlock(pBlockIndex->nHeight);
    s_stolistdb->deleteAboveBlock(pBlockIndex->nHeight);
    reorgRecoveryMaxHeight = 0;


    nWaterlineBlock = GENESIS_BLOCK - 1;
    if (isNonMainNet()) nWaterlineBlock = START_TESTNET_BLOCK - 1;
    if (RegTest()) nWaterlineBlock = START_REGTEST_BLOCK - 1;


    if(readPersistence()) {
      int best_state_block = load_most_relevant_state();
      if (best_state_block < 0) {
        // unable to recover easily, remove stale stale state bits and reparse from the beginning.
        clear_all_state();
      } else {
        nWaterlineBlock = best_state_block;
      }
    }

    if (nWaterlineBlock < nBlockPrev) {
      // scan from the block after the best active block to catch up to the active chain
      msc_initial_scan(nWaterlineBlock + 1);
    }
  }

  if (0 < nBlockTop)
    if (nBlockTop < nBlockPrev + 1)
      return 0;

  (void) eraseExpiredCrowdsale(pBlockIndex);

  return 0;
}

// called once per block, after the block has been processed
// TODO: consolidate into *handler_block_begin() << need to adjust Accept expiry check.............
// it performs cleanup and other functions
int mastercore_handler_block_end(int nBlockNow, CBlockIndex const * pBlockIndex,
    unsigned int countMP) {
  if (!mastercoreInitialized) {
    mastercore_init();
  }

  if (0 < nBlockTop)
    if (nBlockTop < nBlockNow)
      return 0;

// for every new received block must do:
// 1) remove expired entries from the accept list (per spec accept entries are valid until their blocklimit expiration; because the customer can keep paying BTC for the offer in several installments)
// 2) update the amount in the Exodus address
  uint64_t devmsc = 0;
  unsigned int how_many_erased = eraseExpiredAccepts(nBlockNow);

  if (how_many_erased)
    file_log("%s(%d); erased %u accepts this block, line %d, file: %s\n",
        __FUNCTION__, how_many_erased, nBlockNow, __LINE__, __FILE__);

  // calculate devmsc as of this block and update the Exodus' balance
  devmsc = calculate_and_update_devmsc(pBlockIndex->GetBlockTime());

  if (msc_debug_exo)
    file_log("devmsc for block %d: %lu, Exodus balance: %lu\n", nBlockNow,
        devmsc, getMPbalance(exodus_address, OMNI_PROPERTY_MSC, BALANCE));

  // get the total MSC for this wallet, for QT display
  (void) set_wallet_totals();

  // check the alert status, do we need to do anything else here?
  (void) checkExpiredAlerts(nBlockNow, pBlockIndex->GetBlockTime());

  // force an update of the UI once per processed block containing Omni transactions
  if (countMP > 0)  // there were Omni transactions in this block
  {
    uiInterface.OmniStateChanged();
  }

  // save out the state after this block
  if (writePersistence(nBlockNow))
    mastercore_save_state(pBlockIndex);

  return 0;
}

int mastercore_handler_disc_begin(int nBlockNow, CBlockIndex const * pBlockIndex)
{
    reorgRecoveryMode = 1;
    reorgRecoveryMaxHeight = (pBlockIndex->nHeight > reorgRecoveryMaxHeight) ? pBlockIndex->nHeight: reorgRecoveryMaxHeight;
    return 0;
}

int mastercore_handler_disc_end(int nBlockNow, CBlockIndex const * pBlockIndex) {
    return 0;
}

const std::string ExodusAddress()
{
  return string(exodus_address);
}

const std::string NotificationAddress()
{
static const string addr = "1MpNote1jsHkbQLwEmgoMr29EoUC1nyxxV";

  if (isNonMainNet()) {}; // TODO pick a notification address for TestNet

  return addr;
}

 // the 31-byte packet & the packet #
 // int interpretPacket(int blocknow, unsigned char pkt[], int size)
 //
 // RETURNS:  0 if the packet is fully valid
 // RETURNS: <0 if the packet is invalid
 // RETURNS: >0 the only known case today is: return PKT_RETURNED_OBJECT
 //
 // 
 // the following functions may augment the amount in question (nValue):
 // DEx_offerCreate()
 // DEx_offerUpdate()
 // DEx_acceptCreate()
 // DEx_payment() -- DOES NOT fit nicely into the model, as it currently is NOT a MP TX (not even in leveldb) -- gotta rethink
 //
 // optional: provide the pointer to the CMPOffer object, it will get filled in
 // verify that it does via if (MSC_TYPE_TRADE_OFFER == mp_obj.getType())
 //
int CMPTransaction::interpretPacket(CMPOffer *obj_o, CMPMetaDEx *mdex_o)
{
int rc = PKT_ERROR;
int step_rc;
std::string new_global_alert_message;

  if (0>step1()) return -98765;

  if ((obj_o) && (MSC_TYPE_TRADE_OFFER != type)) return -777; // can't fill in the Offer object !
  if ((mdex_o) && (MSC_TYPE_METADEX != type)) return -778; // can't fill in the MetaDEx object !

  // further processing for complex types
  // TODO: version may play a role here !
  switch(type)
  {
    case MSC_TYPE_SIMPLE_SEND:
      step_rc = step2_Value();
      if (0>step_rc) return step_rc;

      rc = logicMath_SimpleSend();
      break;

    case MSC_TYPE_TRADE_OFFER:
      step_rc = step2_Value();
      if (0>step_rc) return step_rc;

      rc = logicMath_TradeOffer(obj_o);
      break;

    case MSC_TYPE_ACCEPT_OFFER_BTC:
      step_rc = step2_Value();
      if (0>step_rc) return step_rc;

      rc = logicMath_AcceptOffer_BTC();
      break;

    case MSC_TYPE_CREATE_PROPERTY_FIXED:
    {
      const char *p = step2_SmartProperty(step_rc);
      if (0>step_rc) return step_rc;
      if (!p) return (PKT_ERROR_SP -11);

      step_rc = step3_sp_fixed(p);
      if (0>step_rc) return step_rc;

      if (0 == step_rc)
      {
        CMPSPInfo::Entry newSP;
        newSP.issuer = sender;
        newSP.txid = txid;
        newSP.prop_type = prop_type;
        newSP.num_tokens = nValue;
        newSP.category.assign(category);
        newSP.subcategory.assign(subcategory);
        newSP.name.assign(name);
        newSP.url.assign(url);
        newSP.data.assign(data);
        newSP.fixed = true;
        newSP.creation_block = newSP.update_block = chainActive[block]->GetBlockHash();

        const unsigned int id = _my_sps->putSP(ecosystem, newSP);
        update_tally_map(sender, id, nValue, BALANCE);
      }
      rc = 0;
      break;
    }

    case MSC_TYPE_CREATE_PROPERTY_VARIABLE:
    {
      const char *p = step2_SmartProperty(step_rc);
      if (0>step_rc) return step_rc;
      if (!p) return (PKT_ERROR_SP -12);

      step_rc = step3_sp_variable(p);
      if (0>step_rc) return step_rc;

      // check if one exists for this address already !
      if (NULL != getCrowd(sender)) return (PKT_ERROR_SP -20);

      // must check that the desired property exists in our universe
      if (false == _my_sps->hasSP(property)) return (PKT_ERROR_SP -30);

      if (0 == step_rc)
      {
        CMPSPInfo::Entry newSP;
        newSP.issuer = sender;
        newSP.txid = txid;
        newSP.prop_type = prop_type;
        newSP.num_tokens = nValue;
        newSP.category.assign(category);
        newSP.subcategory.assign(subcategory);
        newSP.name.assign(name);
        newSP.url.assign(url);
        newSP.data.assign(data);
        newSP.fixed = false;
        newSP.property_desired = property;
        newSP.deadline = deadline;
        newSP.early_bird = early_bird;
        newSP.percentage = percentage;
        newSP.creation_block = newSP.update_block = chainActive[block]->GetBlockHash();

        const unsigned int id = _my_sps->putSP(ecosystem, newSP);
        my_crowds.insert(std::make_pair(sender, CMPCrowd(id, nValue, property, deadline, early_bird, percentage, 0, 0)));
        file_log("CREATED CROWDSALE id: %u value: %lu property: %u\n", id, nValue, property);  
      }
      rc = 0;
      break;
    }

    case MSC_TYPE_CLOSE_CROWDSALE:
    {
    CrowdMap::iterator it = my_crowds.find(sender);

      if (it != my_crowds.end())
      {
        // retrieve the property id from the incoming packet
        memcpy(&property, &pkt[4], 4);
        swapByteOrder32(property);

        if (msc_debug_sp) file_log("%s() trying to ERASE CROWDSALE for propid= %u=%X\n", __FUNCTION__, property, property);

        // ensure we are closing the crowdsale which we opened by checking the property
        if ((it->second).getPropertyId() != property)
        {
          rc = (PKT_ERROR_SP -606);
          break;
        }

        dumpCrowdsaleInfo(it->first, it->second);

        // Begin calculate Fractional 

        CMPCrowd &crowd = it->second;
        
        CMPSPInfo::Entry sp;
        _my_sps->getSP(crowd.getPropertyId(), sp);

        //file_log("\nValues going into calculateFractional(): hexid %s earlyBird %d deadline %lu numProps %lu issuerPerc %d, issuerCreated %ld \n", sp.txid.GetHex().c_str(), sp.early_bird, sp.deadline, sp.num_tokens, sp.percentage, crowd.getIssuerCreated());

        double missedTokens = calculateFractional(sp.prop_type,
                            sp.early_bird,
                            sp.deadline,
                            sp.num_tokens,
                            sp.percentage,
                            crowd.getDatabase(),
                            crowd.getIssuerCreated());

        //file_log("\nValues coming out of calculateFractional(): Total tokens, Tokens created, Tokens for issuer, amountMissed: issuer %s %ld %ld %ld %f\n",sp.issuer.c_str(), crowd.getUserCreated() + crowd.getIssuerCreated(), crowd.getUserCreated(), crowd.getIssuerCreated(), missedTokens);
        sp.historicalData = crowd.getDatabase();
        sp.update_block = chainActive[block]->GetBlockHash();
        sp.close_early = 1;
        sp.timeclosed = blockTime;
        sp.txid_close = txid;
        sp.missedTokens = (int64_t) missedTokens;
        _my_sps->updateSP(crowd.getPropertyId() , sp);
        
        update_tally_map(sp.issuer, crowd.getPropertyId(), missedTokens, BALANCE);
        //End

        my_crowds.erase(it);

        rc = 0;
      }
      break;
    }

    case MSC_TYPE_CREATE_PROPERTY_MANUAL:
    {
      const char *p = step2_SmartProperty(step_rc);
      if (0>step_rc) return step_rc;
      if (!p) return (PKT_ERROR_SP -11);

      if (0 == step_rc)
      {
        CMPSPInfo::Entry newSP;
        newSP.issuer = sender;
        newSP.txid = txid;
        newSP.prop_type = prop_type;
        newSP.category.assign(category);
        newSP.subcategory.assign(subcategory);
        newSP.name.assign(name);
        newSP.url.assign(url);
        newSP.data.assign(data);
        newSP.fixed = false;
        newSP.manual = true;

        const unsigned int id = _my_sps->putSP(ecosystem, newSP);
        file_log("CREATED MANUAL PROPERTY id: %u admin: %s \n", id, sender.c_str());
      }
      rc = 0;
      break;
    }

    case MSC_TYPE_GRANT_PROPERTY_TOKENS:
      step_rc = step2_Value();
      if (0>step_rc) return step_rc;

      rc = logicMath_GrantTokens();
      break;

    case MSC_TYPE_REVOKE_PROPERTY_TOKENS:
      step_rc = step2_Value();
      if (0>step_rc) return step_rc;

      rc = logicMath_RevokeTokens();
      break;

    case MSC_TYPE_SEND_TO_OWNERS:
    if (disable_Divs) break;
    else
    {
      step_rc = step2_Value();
      if (0>step_rc) return step_rc;

      boost::filesystem::path pathOwners = GetDataDir() / OWNERS_FILENAME;
      FILE *fp = fopen(pathOwners.string().c_str(), "a");

      if (fp)
      {
        printInfo(fp);
      }
      else
      {
        file_log("\nPROBLEM writing %s, errno= %d\n", OWNERS_FILENAME, errno);
      }

      rc = logicMath_SendToOwners(fp);

      if (fp) fclose(fp);
    }
    break;

    case MSC_TYPE_METADEX:
#ifdef  MY_HACK
//      if (304500 > block) return -31337;
//      if (305100 > block) return -31337;

//      if (304930 > block) return -31337;
//      if (307057 > block) return -31337;

//      if (307234 > block) return -31337;
//      if (307607 > block) return -31337;

      if (307057 > block) return -31337;
#endif
      step_rc = step2_Value();
      if (0>step_rc) return step_rc;

      rc = logicMath_MetaDEx(mdex_o);
      break;

    case MSC_TYPE_CHANGE_ISSUER_ADDRESS:
      // parse the property from the packet
      memcpy(&property, &pkt[4], 4);
      swapByteOrder32(property);

      rc = logicMath_ChangeIssuer();
      break;

    case MSC_TYPE_SAVINGS_MARK:
      rc = logicMath_SavingsMark();
      break;

    case MSC_TYPE_SAVINGS_COMPROMISED:
      rc = logicMath_SavingsCompromised();
      break;

    case OMNICORE_MESSAGE_TYPE_ALERT:
      // check the packet version is also FF
      if ((int)version == 65535)
      {
          rc = step2_Alert(&new_global_alert_message);
          if (rc == 0) global_alert_message = new_global_alert_message;
          // end of block handler will expire any old alerts
      }
      break;

    default:
      return (PKT_ERROR -100);
  }

  return rc;
}

int CMPTransaction::logicMath_SimpleSend()
{
int rc = PKT_ERROR_SEND -1000;
int invalid = 0;  // unused

      if (!isTransactionTypeAllowed(block, property, type, version)) return (PKT_ERROR_SEND -22);

      if (sender.empty()) ++invalid;
      // special case: if can't find the receiver -- assume sending to itself !
      // may also be true for BTC payments........
      // TODO: think about this..........
      if (receiver.empty())
      {
        receiver = sender;
      }
      if (receiver.empty()) ++invalid;

      // insufficient funds check & return
      if (!update_tally_map(sender, property, - nValue, BALANCE))
      {
        return (PKT_ERROR -111);
      }

      update_tally_map(receiver, property, nValue, BALANCE);

      // is there a crowdsale running from this recepient ?
      {
      CMPCrowd *crowd;

        crowd = getCrowd(receiver);

        if (crowd && (crowd->getCurrDes() == property) )
        {
          CMPSPInfo::Entry sp;
          bool spFound = _my_sps->getSP(crowd->getPropertyId(), sp);

          file_log("INVESTMENT SEND to Crowdsale Issuer: %s\n", receiver.c_str());
          
          if (spFound)
          {
            //init this struct
            std::pair <uint64_t,uint64_t> tokens;
            //pass this in by reference to determine if max_tokens has been reached
            bool close_crowdsale = false; 
            //get txid
            string sp_txid =  sp.txid.GetHex().c_str();

            //Units going into the calculateFundraiser function must
            //match the unit of the fundraiser's property_type.
            //By default this means Satoshis in and satoshis out.
            //In the condition that your fundraiser is Divisible,
            //but you are accepting indivisible tokens, you must
            //account for 1.0 Div != 1 Indiv but actually 1.0 Div == 100000000 Indiv.
            //The unit must be shifted or your values will be incorrect,
            //that is what we check for below.
            if ( !(isPropertyDivisible(property)) ) {
              nValue = nValue * 1e8;
            }

            //file_log("\nValues going into calculateFundraiser(): hexid %s nValue %lu earlyBird %d deadline %lu blockTime %ld numProps %lu issuerPerc %d \n", txid.GetHex().c_str(), nValue, sp.early_bird, sp.deadline, (uint64_t) blockTime, sp.num_tokens, sp.percentage);

            // calc tokens per this fundraise
            calculateFundraiser(sp.prop_type,         //u short
                                nValue,               // u int 64
                                sp.early_bird,        // u char
                                sp.deadline,          // u int 64
                                (uint64_t) blockTime, // int 64
                                sp.num_tokens,      // u int 64
                                sp.percentage,        // u char
                                getTotalTokens(crowd->getPropertyId()),
                                tokens,
                                close_crowdsale);

            //file_log(mp_fp,"\n before incrementing global tokens user: %ld issuer: %ld\n", crowd->getUserCreated(), crowd->getIssuerCreated());
            
            //getIssuerCreated() is passed into calcluateFractional() at close
            //getUserCreated() is a convenient way to get user created during a crowdsale
            crowd->incTokensUserCreated(tokens.first);
            crowd->incTokensIssuerCreated(tokens.second);
            
            //file_log(mp_fp,"\n after incrementing global tokens user: %ld issuer: %ld\n", crowd->getUserCreated(), crowd->getIssuerCreated());
            
            //init data to pass to txFundraiserData
            uint64_t txdata[] = { (uint64_t) nValue, (uint64_t) blockTime, (uint64_t) tokens.first, (uint64_t) tokens.second };
            
            std::vector<uint64_t> txDataVec(txdata, txdata + sizeof(txdata)/sizeof(txdata[0]) );

            //insert data
            crowd->insertDatabase(txid.GetHex().c_str(), txDataVec  );

            //file_log(mp_fp,"\nValues coming out of calculateFundraiser(): hex %s: Tokens created, Tokens for issuer: %ld %ld\n",txid.GetHex().c_str(), tokens.first, tokens.second);

            //update sender/rec
            update_tally_map(sender, crowd->getPropertyId(), tokens.first, BALANCE);
            update_tally_map(receiver, crowd->getPropertyId(), tokens.second, BALANCE);

            // close crowdsale if we hit MAX_TOKENS
            if( close_crowdsale ) {
              eraseMaxedCrowdsale(receiver, blockTime, block);
            }
          }
        }
      }

      rc = 0;

    return rc;
}

int CMPTransaction::logicMath_SendToOwners(FILE *fhandle)
{
int rc = PKT_ERROR_STO -1000;

      if (!isTransactionTypeAllowed(block, property, type, version)) return (PKT_ERROR_STO -888);

      // totalTokens will be 0 for non-existing property
      int64_t totalTokens = getTotalTokens(property);

      file_log("\t    Total Tokens: %s\n", FormatMP(property, totalTokens).c_str());

      if (0 >= totalTokens)
      {
        return (PKT_ERROR_STO -2);
      }

      // does the sender have enough of the property he's trying to "Send To Owners" ?
      if (getMPbalance(sender, property, BALANCE) < (int64_t)nValue)
      {
        return (PKT_ERROR_STO -3);
      }

      totalTokens = 0;

      typedef std::set<pair<int64_t, string>, SendToOwners_compare> OwnerAddrType;
      OwnerAddrType OwnerAddrSet;

      {
        for(map<string, CMPTally>::reverse_iterator my_it = mp_tally_map.rbegin(); my_it != mp_tally_map.rend(); ++my_it)
        {
          const string address = (my_it->first).c_str();

          // do not count the sender
          if (address == sender) continue;

          int64_t tokens = 0;

          tokens += getMPbalance(address, property, BALANCE);
          tokens += getMPbalance(address, property, SELLOFFER_RESERVE);
          tokens += getMPbalance(address, property, METADEX_RESERVE);
          tokens += getMPbalance(address, property, ACCEPT_RESERVE);

          if (tokens)
          {
            OwnerAddrSet.insert(make_pair(tokens, address));
            totalTokens += tokens;
          }
        }
      }

      file_log("  Excluding Sender: %s\n", FormatMP(property, totalTokens).c_str());

      // determine which property the fee will be paid in
      const unsigned int feeProperty = isTestEcosystemProperty(property) ? OMNI_PROPERTY_TMSC : OMNI_PROPERTY_MSC;

    uint64_t n_owners = 0;
    rc = 0; // almost good, the for-loop will set the error code if any

    for (unsigned int itern=0;itern<=1;itern++)  // iteration number, loop executes twice - first time the dry run to collect n_owners
    { // two-iteration loop START
      // split up what was taken and distribute between all holders
      uint64_t sent_so_far = 0;
      for(OwnerAddrType::reverse_iterator my_it = OwnerAddrSet.rbegin(); my_it != OwnerAddrSet.rend(); ++my_it)
      { // owners loop
      const string address = my_it->second;

        int128_t owns = my_it->first;
        int128_t temp = owns * int128_t(nValue);
        int128_t piece = 1 + ((temp - 1) / int128_t(totalTokens));

        uint64_t will_really_receive = 0;
        uint64_t should_receive = piece.convert_to<uint64_t>();

        // ensure that much is still available
        if ((nValue - sent_so_far) < should_receive)
        {
          will_really_receive = nValue - sent_so_far;
        }
        else
        {
          will_really_receive = should_receive;
        }

        sent_so_far += will_really_receive;

        if (itern)
        {
        // real execution of the loop
          if (!update_tally_map(sender, property, - will_really_receive, BALANCE))
          {
            return (PKT_ERROR_STO -1);
          }

          update_tally_map(address, property, will_really_receive, BALANCE);

          // add to stodb
          s_stolistdb->recordSTOReceive(address, txid, block, property, will_really_receive);

          if (sent_so_far >= nValue)
          {
            file_log("SendToOwners: DONE HERE : those who could get paid got paid, SOME DID NOT, but that's ok\n");
            break; // done here, everybody who could get paid got paid
          }
        }
        else
        {
          // dry run code
          if (will_really_receive > 0) ++n_owners;

          if (msc_debug_sto)
            file_log("%14lu = %s, temp= %38s, should_get= %19lu, will_really_get= %14lu, sent_so_far= %14lu\n",
            owns, address.c_str(), temp.str(), should_receive, will_really_receive, sent_so_far);

          // record the detailed info as needed
          if (fhandle) fprintf(fhandle, "%s = %s\n", address.c_str(), FormatMP(property, will_really_receive).c_str());
        }
      } // owners loop

      if (!itern) // first dummy iteration
      { //  if first ITERATION BEGIN
        // make sure we found some owners
        if (0 >= n_owners)
        {
          return (PKT_ERROR_STO -4);
        }

        file_log("\t          Owners: %lu\n", n_owners);

        const int64_t nXferFee = TRANSFER_FEE_PER_OWNER * n_owners;
        file_log("\t    Transfer fee: %lu.%08lu %s\n", nXferFee/COIN, nXferFee%COIN, strMPProperty(feeProperty).c_str());

        // enough coins to pay the fee?
        if (getMPbalance(sender, feeProperty, BALANCE) < nXferFee)
        {
          return (PKT_ERROR_STO -5);
        }

        // special case check, only if distributing MSC or TMSC -- the property the fee will be paid in
        if (feeProperty == property)
        {
          if (getMPbalance(sender, feeProperty, BALANCE) < (int64_t)(nValue + nXferFee))
          {
            return (PKT_ERROR_STO -55);
          }
        }

        // burn MSC or TMSC here: take the transfer fee away from the sender
        if (!update_tally_map(sender, feeProperty, - nXferFee, BALANCE))
        {
          // impossible to reach this, the check was done just before (the check is not necessary since update_tally_map checks balances too)
          return (PKT_ERROR_STO -500);
        }
      } //  if first ITERATION END

      // sent_so_far must equal nValue here
      if (sent_so_far != nValue)
      {
        file_log("sent_so_far= %14lu, nValue= %14lu, n_owners= %lu\n", sent_so_far, nValue, n_owners);

        // rc = ???
      }
    } // two-iteration loop END

    return rc;
}

