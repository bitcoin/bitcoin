// global Master Protocol header file
// globals consts (DEFINEs for now) should be here
//
// for now (?) class declarations go here -- work in progress; probably will get pulled out into a separate file, note: some declarations are still in the .cpp file

#ifndef OMNICORE_OMNICORE_H
#define OMNICORE_OMNICORE_H

class CBitcoinAddress;
class CBlockIndex;
class CTransaction;

#include "omnicore/log.h"
#include "omnicore/persistence.h"

#include "sync.h"
#include "uint256.h"
#include "util.h"

#include <boost/filesystem/path.hpp>

#include "leveldb/status.h"

#include "json/json_spirit_value.h"

#include <stdint.h>

#include <map>
#include <string>
#include <vector>

using json_spirit::Array;

using std::string;

int const MAX_STATE_HISTORY = 50;

#define TEST_ECO_PROPERTY_1 (0x80000003UL)

// could probably also use: int64_t maxInt64 = std::numeric_limits<int64_t>::max();
// maximum numeric values from the spec:
#define MAX_INT_8_BYTES (9223372036854775807UL)

// what should've been in the Exodus address for this block if none were spent
#define DEV_MSC_BLOCK_290629 (1743358325718)

#define SP_STRING_FIELD_LEN 256

// some boost formats
#define FORMAT_BOOST_TXINDEXKEY "index-tx-%s"
#define FORMAT_BOOST_SPKEY      "sp-%d"

// Omni Layer Transaction Class
#define OMNI_CLASS_A 1
#define OMNI_CLASS_B 2
#define OMNI_CLASS_C 3

// Maximum number of keys supported in Class B
#define CLASS_B_MAX_SENDABLE_PACKETS 2

// Master Protocol Transaction (Packet) Version
#define MP_TX_PKT_V0  0
#define MP_TX_PKT_V1  1

// Maximum outputs per BTC Transaction
#define MAX_BTC_OUTPUTS 16

#define MIN_PAYLOAD_SIZE     5
#define PACKET_SIZE_CLASS_A 19
#define PACKET_SIZE         31
#define MAX_PACKETS         64

// Transaction types, from the spec
enum TransactionType {
  MSC_TYPE_SIMPLE_SEND              =  0,
  MSC_TYPE_RESTRICTED_SEND          =  2,
  MSC_TYPE_SEND_TO_OWNERS           =  3,
  MSC_TYPE_SAVINGS_MARK             = 10,
  MSC_TYPE_SAVINGS_COMPROMISED      = 11,
  MSC_TYPE_RATELIMITED_MARK         = 12,
  MSC_TYPE_AUTOMATIC_DISPENSARY     = 15,
  MSC_TYPE_TRADE_OFFER              = 20,
  MSC_TYPE_ACCEPT_OFFER_BTC         = 22,
  MSC_TYPE_METADEX_TRADE            = 25,
  MSC_TYPE_METADEX_CANCEL_PRICE     = 26,
  MSC_TYPE_METADEX_CANCEL_PAIR      = 27,
  MSC_TYPE_METADEX_CANCEL_ECOSYSTEM = 28,
  MSC_TYPE_NOTIFICATION             = 31,
  MSC_TYPE_OFFER_ACCEPT_A_BET       = 40,
  MSC_TYPE_CREATE_PROPERTY_FIXED    = 50,
  MSC_TYPE_CREATE_PROPERTY_VARIABLE = 51,
  MSC_TYPE_PROMOTE_PROPERTY         = 52,
  MSC_TYPE_CLOSE_CROWDSALE          = 53,
  MSC_TYPE_CREATE_PROPERTY_MANUAL   = 54,
  MSC_TYPE_GRANT_PROPERTY_TOKENS    = 55,
  MSC_TYPE_REVOKE_PROPERTY_TOKENS   = 56,
  MSC_TYPE_CHANGE_ISSUER_ADDRESS    = 70,
  OMNICORE_MESSAGE_TYPE_ALERT     = 65535
};

#define MSC_PROPERTY_TYPE_INDIVISIBLE             1
#define MSC_PROPERTY_TYPE_DIVISIBLE               2
#define MSC_PROPERTY_TYPE_INDIVISIBLE_REPLACING   65
#define MSC_PROPERTY_TYPE_DIVISIBLE_REPLACING     66
#define MSC_PROPERTY_TYPE_INDIVISIBLE_APPENDING   129
#define MSC_PROPERTY_TYPE_DIVISIBLE_APPENDING     130

// block height (MainNet) with which the corresponding transaction is considered valid, per spec
enum BLOCKHEIGHTRESTRICTIONS {
// starting block for parsing on TestNet
  START_TESTNET_BLOCK=263000,
  START_REGTEST_BLOCK=5,
  MONEYMAN_REGTEST_BLOCK= 101, // new address to assign MSC & TMSC on RegTest
  MONEYMAN_TESTNET_BLOCK= 270775, // new address to assign MSC & TMSC on TestNet
  POST_EXODUS_BLOCK = 255366,
  MSC_DEX_BLOCK     = 290630,
  MSC_SP_BLOCK      = 297110,
  GENESIS_BLOCK     = 249498,
  LAST_EXODUS_BLOCK = 255365,
  MSC_STO_BLOCK     = 342650,
  MSC_METADEX_BLOCK = 999999,
  MSC_BET_BLOCK     = 999999,
  MSC_MANUALSP_BLOCK= 323230,
  P2SH_BLOCK        = 322000,
  OP_RETURN_BLOCK   = 999999
};

enum FILETYPES {
  FILETYPE_BALANCES = 0,
  FILETYPE_OFFERS,
  FILETYPE_ACCEPTS,
  FILETYPE_GLOBALS,
  FILETYPE_CROWDSALES,
  FILETYPE_MDEXORDERS,
  NUM_FILETYPES
};

#define PKT_RETURNED_OBJECT    (1000)

#define PKT_ERROR             ( -9000)
#define DEX_ERROR_SELLOFFER   (-10000)
#define DEX_ERROR_ACCEPT      (-20000)
#define DEX_ERROR_PAYMENT     (-30000)
// Smart Properties
#define PKT_ERROR_SP          (-40000)
// Send To Owners
#define PKT_ERROR_STO         (-50000)
#define PKT_ERROR_SEND        (-60000)
#define PKT_ERROR_TRADEOFFER  (-70000)
#define PKT_ERROR_METADEX     (-80000)
#define METADEX_ERROR         (-81000)
#define PKT_ERROR_TOKENS      (-82000)

#define OMNI_PROPERTY_BTC   0
#define OMNI_PROPERTY_MSC   1
#define OMNI_PROPERTY_TMSC  2

// forward declarations
std::string FormatDivisibleMP(int64_t n, bool fSign = false);
std::string FormatDivisibleShortMP(int64_t);
std::string FormatMP(unsigned int, int64_t n, bool fSign = false);
int64_t feeCheck(const string &address);

/** Returns the Exodus address. */
const CBitcoinAddress ExodusAddress();

/** Used to indicate, whether to automatically commit created transactions. */
extern bool autoCommit;

extern CCriticalSection cs_tally;

enum TallyType { BALANCE = 0, SELLOFFER_RESERVE = 1, ACCEPT_RESERVE = 2, PENDING = 3, METADEX_RESERVE = 4, TALLY_TYPE_COUNT };

class CMPTally
{
private:
typedef struct
{
  int64_t balance[TALLY_TYPE_COUNT];
} BalanceRecord;

  typedef std::map<unsigned int, BalanceRecord> TokenMap;
  TokenMap mp_token;
  TokenMap::iterator my_it;

  bool propertyExists(unsigned int which_property) const
  {
  const TokenMap::const_iterator it = mp_token.find(which_property);

    return (it != mp_token.end());
  }

public:

  unsigned int init()
  {
  unsigned int ret = 0;

    my_it = mp_token.begin();
    if (my_it != mp_token.end()) ret = my_it->first;

    return ret;
  }

  unsigned int next()
  {
  unsigned int ret;

    if (my_it == mp_token.end()) return 0;

    ret = my_it->first;

    ++my_it;

    return ret;
  }

  bool updateMoney(unsigned int which_property, int64_t amount, TallyType ttype)
  {
  bool bRet = false;
  int64_t now64;

    if (TALLY_TYPE_COUNT <= ttype) return false;

    LOCK(cs_tally);

    now64 = mp_token[which_property].balance[ttype];

    if ((PENDING != ttype) && (0>(now64 + amount)))
    {
    }
    else
    {
      now64 += amount;
      mp_token[which_property].balance[ttype] = now64;

      bRet = true;
    }

    return bRet;
  }

  // the constructor -- create an empty tally for an address
  CMPTally()
  {
    my_it = mp_token.begin();
  }

  int64_t print(int which_property = OMNI_PROPERTY_MSC, bool bDivisible = true)
  {
  int64_t money = 0;
  int64_t so_r = 0;
  int64_t a_r = 0;
  int64_t pending = 0;

    if (propertyExists(which_property))
    {
      money = mp_token[which_property].balance[BALANCE];
      so_r = mp_token[which_property].balance[SELLOFFER_RESERVE];
      a_r = mp_token[which_property].balance[ACCEPT_RESERVE];
      pending = mp_token[which_property].balance[PENDING];
    }

    if (bDivisible)
    {
      PrintToConsole("%22s [SO_RESERVE= %22s , ACCEPT_RESERVE= %22s ] %22s\n",
       FormatDivisibleMP(money, true), FormatDivisibleMP(so_r, true), FormatDivisibleMP(a_r, true), FormatDivisibleMP(pending, true));
    }
    else
    {
      PrintToConsole("%14lu [SO_RESERVE= %14lu , ACCEPT_RESERVE= %14lu ] %14ld\n", money, so_r, a_r, pending);
    }

    return (money + so_r + a_r);
  }

  int64_t getMoney(unsigned int which_property, TallyType ttype) const
  {
    if (TALLY_TYPE_COUNT <= ttype) return 0;

    int64_t money = 0;

    LOCK(cs_tally);
    TokenMap::const_iterator it = mp_token.find(which_property);

    if (it != mp_token.end()) {
        const BalanceRecord& record = it->second;
        money = record.balance[ttype];
    }

    return money;
  }
};

/** LevelDB based storage for STO recipients.
 */
class CMPSTOList : public CDBBase
{
public:
    CMPSTOList(const boost::filesystem::path& path, bool fWipe)
    {
        leveldb::Status status = Open(path, fWipe);
        PrintToConsole("Loading send-to-owners database: %s\n", status.ToString());
    }

    virtual ~CMPSTOList()
    {
        if (msc_debug_persistence) PrintToLog("CMPSTOList closed\n");
    }

    void getRecipients(const uint256 txid, string filterAddress, Array *recipientArray, uint64_t *total, uint64_t *stoFee);
    std::string getMySTOReceipts(string filterAddress);
    int deleteAboveBlock(int blockNum);
    void printStats();
    void printAll();
    bool exists(string address);
    void recordSTOReceive(std::string, const uint256&, int, unsigned int, uint64_t);
};

/** LevelDB based storage for the trade history. Trades are listed with key "txid1+txid2".
 */
class CMPTradeList : public CDBBase
{
public:
    CMPTradeList(const boost::filesystem::path& path, bool fWipe)
    {
        leveldb::Status status = Open(path, fWipe);
        PrintToConsole("Loading trades database: %s\n", status.ToString());
    }

    virtual ~CMPTradeList()
    {
        if (msc_debug_persistence) PrintToLog("CMPTradeList closed\n");
    }

    void recordTrade(const uint256 txid1, const uint256 txid2, string address1, string address2, unsigned int prop1, unsigned int prop2, uint64_t amount1, uint64_t amount2, int blockNum);
    int deleteAboveBlock(int blockNum);
    bool exists(const uint256 &txid);
    void printStats();
    void printAll();
    bool getMatchingTrades(const uint256 txid, unsigned int propertyId, Array *tradeArray, int64_t *totalSold, int64_t *totalBought);
    int getMPTradeCountTotal();
};

/** LevelDB based storage for transactions, with txid as key and validity bit, and other data as value.
 */
class CMPTxList : public CDBBase
{
public:
    CMPTxList(const boost::filesystem::path& path, bool fWipe)
    {
        leveldb::Status status = Open(path, fWipe);
        PrintToConsole("Loading transactions database: %s\n", status.ToString());
    }

    virtual ~CMPTxList()
    {
        if (msc_debug_persistence) PrintToLog("CMPTxList closed\n");
    }

    void recordTX(const uint256 &txid, bool fValid, int nBlock, unsigned int type, uint64_t nValue);
    void recordPaymentTX(const uint256 &txid, bool fValid, int nBlock, unsigned int vout, unsigned int propertyId, uint64_t nValue, string buyer, string seller);
    void recordMetaDExCancelTX(const uint256 &txidMaster, const uint256 &txidSub, bool fValid, int nBlock, unsigned int propertyId, uint64_t nValue);

    string getKeyValue(string key);
    uint256 findMetaDExCancel(const uint256 txid);
    int getNumberOfPurchases(const uint256 txid);
    int getNumberOfMetaDExCancels(const uint256 txid);
    bool getPurchaseDetails(const uint256 txid, int purchaseNumber, string *buyer, string *seller, uint64_t *vout, uint64_t *propertyId, uint64_t *nValue);
    int getMPTransactionCountTotal();
    int getMPTransactionCountBlock(int block);

    bool exists(const uint256 &txid);
    bool getTX(const uint256 &txid, string &value);

    int setLastAlert(int blockHeight);

    void printStats();
    void printAll();

    bool isMPinBlockRange(int, int, bool);
};

extern uint64_t global_metadex_market;
//! Available balances of wallet properties in the main ecosystem
extern std::map<uint32_t, int64_t> global_balance_money_maineco;
//! Reserved balances of wallet properties in the main ecosystem
extern std::map<uint32_t, int64_t> global_balance_reserved_maineco;
//! Available balances of wallet properties in the test ecosystem
extern std::map<uint32_t, int64_t> global_balance_money_testeco;
//! Reserved balances of wallet properties in the test ecosystem
extern std::map<uint32_t, int64_t> global_balance_reserved_testeco;

int64_t getMPbalance(const string &Address, unsigned int property, TallyType ttype);
int64_t getUserAvailableMPbalance(const string &Address, unsigned int property);
bool IsMyAddress(const std::string &address);
bool IsMyAddressSpendable(const std::string &address);

string getLabel(const string &address);

/** Global handler to initialize Omni Core. */
int mastercore_init();

/** Global handler to shut down Omni Core. */
int mastercore_shutdown();

int mastercore_handler_disc_begin(int nBlockNow, CBlockIndex const * pBlockIndex);
int mastercore_handler_disc_end(int nBlockNow, CBlockIndex const * pBlockIndex);
int mastercore_handler_block_begin(int nBlockNow, CBlockIndex const * pBlockIndex);
int mastercore_handler_block_end(int nBlockNow, CBlockIndex const * pBlockIndex, unsigned int);
int mastercore_handler_tx(const CTransaction &tx, int nBlock, unsigned int idx, CBlockIndex const *pBlockIndex );
int mastercore_save_state( CBlockIndex const *pBlockIndex );

namespace mastercore
{
extern std::map<string, CMPTally> mp_tally_map;
extern CMPTxList *p_txlistdb;
extern CMPTradeList *t_tradelistdb;
extern CMPSTOList *s_stolistdb;

string strMPProperty(unsigned int i);

int GetHeight(void);
uint32_t GetLatestBlockTime(void);
CBlockIndex* GetBlockIndex(const uint256& hash);

bool isMPinBlockRange(int starting_block, int ending_block, bool bDeleteFound);

std::string FormatIndivisibleMP(int64_t n);

int ClassAgnosticWalletTXBuilder(const string &senderAddress, const string &receiverAddress, const string &redemptionAddress,
                 int64_t referenceAmount, const std::vector<unsigned char> &data, uint256 & txid, string &rawHex, bool commit);

bool isTestEcosystemProperty(unsigned int property);
bool isMainEcosystemProperty(unsigned int property);
uint32_t GetNextPropertyId(bool maineco); // maybe move into sp

CMPTally *getTally(const string & address);

int64_t getTotalTokens(unsigned int propertyId, int64_t *n_owners_total = NULL);
int set_wallet_totals();

char *c_strMasterProtocolTXType(int i);

bool isTransactionTypeAllowed(int txBlock, unsigned int txProperty, unsigned int txType, unsigned short version, bool bAllowNullProperty = false);

bool getValidMPTX(const uint256 &txid, int *block = NULL, unsigned int *type = NULL, uint64_t *nAmended = NULL);

bool update_tally_map(string who, unsigned int which_currency, int64_t amount, TallyType ttype);

std::string getTokenLabel(unsigned int propertyId);
}


#endif // OMNICORE_OMNICORE_H
