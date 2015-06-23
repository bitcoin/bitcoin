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
#include <set>

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
bool feeCheck(const string &address, size_t nDataSize);

/** Returns the Exodus address. */
const CBitcoinAddress ExodusAddress();

//! Used to indicate, whether to automatically commit created transactions
extern bool autoCommit;

//! Global lock for state objects
extern CCriticalSection cs_tally;

//! Balance record types
enum TallyType { BALANCE = 0, SELLOFFER_RESERVE = 1, ACCEPT_RESERVE = 2, PENDING = 3, METADEX_RESERVE = 4, TALLY_TYPE_COUNT };

/** Balance records of a single entity.
 */
class CMPTally
{
private:
    typedef struct {
        int64_t balance[TALLY_TYPE_COUNT];
    } BalanceRecord;

    //! Map of balance records
    typedef std::map<uint32_t, BalanceRecord> TokenMap;
    //! Balance records for different tokens
    TokenMap mp_token;
    //! Internal iterator pointing to a balance record
    TokenMap::iterator my_it;

    // TODO: lock outside of CMPTally
    //
    // There are usually many tally operations, and the
    // global tally map must be locked anyway, so locking
    // here is redundant and might be costly.

public:
    /** Resets the internal iterator. */
    uint32_t init()
    {
        uint32_t ret = 0;
        my_it = mp_token.begin();
        if (my_it != mp_token.end()) {
            ret = my_it->first;
        }
        return ret;
    }

    /** Advances the internal iterator. */
    uint32_t next()
    {
        uint32_t ret = 0;
        if (my_it != mp_token.end()) {
            ret = my_it->first;
            ++my_it;
        }
        return ret;
    }

    /** Updates the number of tokens for the given tally type. */
    bool updateMoney(uint32_t propertyId, int64_t amount, TallyType ttype)
    {
        if (TALLY_TYPE_COUNT <= ttype || amount == 0) {
            return false;
        }
        bool bRet = false;
        int64_t now64 = 0;

        LOCK(cs_tally);

        now64 = mp_token[propertyId].balance[ttype];

        if (PENDING != ttype && (now64 + amount) < 0) {
            // NOTE:
            // This check also serves as overflow protection!
        } else {
            now64 += amount;
            mp_token[propertyId].balance[ttype] = now64;

            bRet = true;
        }

        return bRet;
    }

    /** Creates an empty tally. */
    CMPTally()
    {
        my_it = mp_token.begin();
    }

    /** Returns the number of tokens for the given tally type. */
    int64_t getMoney(uint32_t propertyId, TallyType ttype) const
    {
        if (TALLY_TYPE_COUNT <= ttype) {
            return 0;
        }
        int64_t money = 0;

        LOCK(cs_tally);
        TokenMap::const_iterator it = mp_token.find(propertyId);

        if (it != mp_token.end()) {
            const BalanceRecord& record = it->second;
            money = record.balance[ttype];
        }

        return money;
    }

    /** Returns the number of available tokens. */
    int64_t getMoneyAvailable(uint32_t propertyId) const
    {
        LOCK(cs_tally);
        TokenMap::const_iterator it = mp_token.find(propertyId);

        if (it != mp_token.end()) {
            const BalanceRecord& record = it->second;
            if (record.balance[PENDING] < 0) {
                return record.balance[BALANCE] + record.balance[PENDING];
            } else {
                return record.balance[BALANCE];
            }
        }

        return 0;
    }

    /** Returns the number of reserved tokens. */
    int64_t getMoneyReserved(uint32_t propertyId) const
    {
        int64_t money = 0;

        LOCK(cs_tally);
        TokenMap::const_iterator it = mp_token.find(propertyId);

        if (it != mp_token.end()) {
            const BalanceRecord& record = it->second;
            money += record.balance[SELLOFFER_RESERVE];
            money += record.balance[ACCEPT_RESERVE];
            money += record.balance[METADEX_RESERVE];
        }

        return money;
    }

    /** Compares the tally with another tally and returns true, if they are equal. */
    bool operator==(const CMPTally& rhs) const
    {
        LOCK(cs_tally);

        if (mp_token.size() != rhs.mp_token.size()) {
            return false;
        }
        TokenMap::const_iterator pc1 = mp_token.begin();
        TokenMap::const_iterator pc2 = rhs.mp_token.begin();

        for (unsigned int i = 0; i < mp_token.size(); ++i) {
            if (pc1->first != pc2->first) {
                return false;
            }
            const BalanceRecord& record1 = pc1->second;
            const BalanceRecord& record2 = pc2->second;

            for (int ttype = 0; ttype < TALLY_TYPE_COUNT; ++ttype) {
                if (record1.balance[ttype] != record2.balance[ttype]) {
                    return false;
                }
            }
            ++pc1;
            ++pc2;
        }

        assert(pc1 == mp_token.end());
        assert(pc2 == rhs.mp_token.end());

        return true;
    }

    /** Compares the tally with another tally and returns true, if they are not equal. */
    bool operator!=(const CMPTally& rhs) const
    {
        return !operator==(rhs);
    }

    /** Prints a balance record to the console. */
    int64_t print(uint32_t propertyId = OMNI_PROPERTY_MSC, bool bDivisible = true) const
    {
        int64_t balance = 0;
        int64_t selloffer_reserve = 0;
        int64_t accept_reserve = 0;
        int64_t pending = 0;
        int64_t metadex_reserve = 0;

        TokenMap::const_iterator it = mp_token.find(propertyId);

        if (it != mp_token.end()) {
            const BalanceRecord& record = it->second;
            balance = record.balance[BALANCE];
            selloffer_reserve = record.balance[SELLOFFER_RESERVE];
            accept_reserve = record.balance[ACCEPT_RESERVE];
            pending = record.balance[PENDING];
            metadex_reserve = record.balance[METADEX_RESERVE];
        }

        if (bDivisible) {
            PrintToConsole("%22s [ SO_RESERVE= %22s, ACCEPT_RESERVE= %22s, METADEX_RESERVE= %22s ] %22s\n",
                    FormatDivisibleMP(balance, true), FormatDivisibleMP(selloffer_reserve, true),
                    FormatDivisibleMP(accept_reserve, true), FormatDivisibleMP(metadex_reserve, true),
                    FormatDivisibleMP(pending, true));
        } else {
            PrintToConsole("%14d [ SO_RESERVE= %14d, ACCEPT_RESERVE= %14d, METADEX_RESERVE= %14d ] %14d\n",
                    balance, selloffer_reserve, accept_reserve, metadex_reserve, pending);
        }

        return (balance + selloffer_reserve + accept_reserve + metadex_reserve);
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

//! Available balances of wallet properties
extern std::map<uint32_t, int64_t> global_balance_money;
//! Reserved balances of wallet propertiess
extern std::map<uint32_t, int64_t> global_balance_reserved;
//! Vector containing a list of properties relative to the wallet
extern std::set<uint32_t> global_wallet_property_list;


int64_t getMPbalance(const string &Address, uint32_t property, TallyType ttype);
int64_t getUserAvailableMPbalance(const string &Address, unsigned int property);
int IsMyAddress(const std::string &address);

string getLabel(const string &address);

/** Global handler to initialize Omni Core. */
int mastercore_init();

/** Global handler to shut down Omni Core. */
int mastercore_shutdown();

/** Global handler to total wallet balances. */
void CheckWalletUpdate();

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

char *c_strMasterProtocolTXType(int i);

bool isTransactionTypeAllowed(int txBlock, unsigned int txProperty, unsigned int txType, unsigned short version, bool bAllowNullProperty = false);

bool getValidMPTX(const uint256 &txid, int *block = NULL, unsigned int *type = NULL, uint64_t *nAmended = NULL);

bool update_tally_map(string who, unsigned int which_currency, int64_t amount, TallyType ttype);

std::string getTokenLabel(unsigned int propertyId);
}


#endif // OMNICORE_OMNICORE_H
