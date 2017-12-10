#ifndef OMNICORE_OMNICORE_H
#define OMNICORE_OMNICORE_H

class CBitcoinAddress;
class CBlockIndex;
class CCoinsView;
class CCoinsViewCache;
class CTransaction;

#include "omnicore/log.h"
#include "omnicore/persistence.h"
#include "omnicore/tally.h"

#include "sync.h"
#include "uint256.h"
#include "util.h"

#include <univalue.h>

#include <boost/filesystem/path.hpp>

#include "leveldb/status.h"

#include <stdint.h>

#include <map>
#include <string>
#include <vector>
#include <set>
#include <unordered_map>

using std::string;

int const MAX_STATE_HISTORY = 50;

#define TEST_ECO_PROPERTY_1 (0x80000003UL)

// increment this value to force a refresh of the state (similar to --startclean)
#define DB_VERSION 6

// could probably also use: int64_t maxInt64 = std::numeric_limits<int64_t>::max();
// maximum numeric values from the spec:
#define MAX_INT_8_BYTES (9223372036854775807UL)

// maximum size of string fields
#define SP_STRING_FIELD_LEN 256

// Omni Layer Transaction Class
#define NO_MARKER    0
#define OMNI_CLASS_A 1
#define OMNI_CLASS_B 2
#define OMNI_CLASS_C 3

// Omni Layer Transaction (Packet) Version
#define MP_TX_PKT_V0  0
#define MP_TX_PKT_V1  1

#define MIN_PAYLOAD_SIZE     5
#define PACKET_SIZE_CLASS_A 19
#define PACKET_SIZE         31
#define MAX_PACKETS        255

// Transaction types, from the spec
enum TransactionType {
  MSC_TYPE_SIMPLE_SEND                =  0,
  MSC_TYPE_RESTRICTED_SEND            =  2,
  MSC_TYPE_SEND_TO_OWNERS             =  3,
  MSC_TYPE_SEND_ALL                   =  4,
  MSC_TYPE_SAVINGS_MARK               = 10,
  MSC_TYPE_SAVINGS_COMPROMISED        = 11,
  MSC_TYPE_RATELIMITED_MARK           = 12,
  MSC_TYPE_AUTOMATIC_DISPENSARY       = 15,
  MSC_TYPE_TRADE_OFFER                = 20,
  MSC_TYPE_ACCEPT_OFFER_BTC           = 22,
  MSC_TYPE_METADEX_TRADE              = 25,
  MSC_TYPE_METADEX_CANCEL_PRICE       = 26,
  MSC_TYPE_METADEX_CANCEL_PAIR        = 27,
  MSC_TYPE_METADEX_CANCEL_ECOSYSTEM   = 28,
  MSC_TYPE_NOTIFICATION               = 31,
  MSC_TYPE_OFFER_ACCEPT_A_BET         = 40,
  MSC_TYPE_CREATE_PROPERTY_FIXED      = 50,
  MSC_TYPE_CREATE_PROPERTY_VARIABLE   = 51,
  MSC_TYPE_PROMOTE_PROPERTY           = 52,
  MSC_TYPE_CLOSE_CROWDSALE            = 53,
  MSC_TYPE_CREATE_PROPERTY_MANUAL     = 54,
  MSC_TYPE_GRANT_PROPERTY_TOKENS      = 55,
  MSC_TYPE_REVOKE_PROPERTY_TOKENS     = 56,
  MSC_TYPE_CHANGE_ISSUER_ADDRESS      = 70,
  MSC_TYPE_ENABLE_FREEZING            = 71,
  MSC_TYPE_DISABLE_FREEZING           = 72,
  MSC_TYPE_FREEZE_PROPERTY_TOKENS     = 185,
  MSC_TYPE_UNFREEZE_PROPERTY_TOKENS   = 186,
  OMNICORE_MESSAGE_TYPE_DEACTIVATION  = 65533,
  OMNICORE_MESSAGE_TYPE_ACTIVATION    = 65534,
  OMNICORE_MESSAGE_TYPE_ALERT         = 65535
};

#define MSC_PROPERTY_TYPE_INDIVISIBLE             1
#define MSC_PROPERTY_TYPE_DIVISIBLE               2
#define MSC_PROPERTY_TYPE_INDIVISIBLE_REPLACING   65
#define MSC_PROPERTY_TYPE_DIVISIBLE_REPLACING     66
#define MSC_PROPERTY_TYPE_INDIVISIBLE_APPENDING   129
#define MSC_PROPERTY_TYPE_DIVISIBLE_APPENDING     130

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
#define PKT_ERROR_CROWD       (-45000)
// Send To Owners
#define PKT_ERROR_STO         (-50000)
#define PKT_ERROR_SEND        (-60000)
#define PKT_ERROR_TRADEOFFER  (-70000)
#define PKT_ERROR_METADEX     (-80000)
#define METADEX_ERROR         (-81000)
#define PKT_ERROR_TOKENS      (-82000)
#define PKT_ERROR_SEND_ALL    (-83000)

#define OMNI_PROPERTY_BTC   0
#define OMNI_PROPERTY_MSC   1
#define OMNI_PROPERTY_TMSC  2

// forward declarations
std::string FormatDivisibleMP(int64_t amount, bool fSign = false);
std::string FormatDivisibleShortMP(int64_t amount);
std::string FormatMP(uint32_t propertyId, int64_t amount, bool fSign = false);
std::string FormatShortMP(uint32_t propertyId, int64_t amount);
std::string FormatByType(int64_t amount, uint16_t propertyType);

/** Returns the Exodus address. */
const CBitcoinAddress ExodusAddress();

/** Returns the Exodus crowdsale address. */
const CBitcoinAddress ExodusCrowdsaleAddress(int nBlock = 0);

/** Returns the marker for class C transactions. */
const std::vector<unsigned char> GetOmMarker();

//! Used to indicate, whether to automatically commit created transactions
extern bool autoCommit;

//! Global lock for state objects
extern CCriticalSection cs_tally;

/** LevelDB based storage for storing Omni transaction data.  This will become the new master database, holding serialized Omni transactions.
 *  Note, intention is to consolidate and clean up data storage
 */
class COmniTransactionDB : public CDBBase
{
public:
    COmniTransactionDB(const boost::filesystem::path& path, bool fWipe)
    {
        leveldb::Status status = Open(path, fWipe);
        PrintToConsole("Loading master transactions database: %s\n", status.ToString());
    }

    virtual ~COmniTransactionDB()
    {
        if (msc_debug_persistence) PrintToLog("COmniTransactionDB closed\n");
    }

    /* These functions would be expanded upon to store a serialized version of the transaction and associated state data
     *
     * void RecordTransaction(const uint256& txid, uint32_t posInBlock, various, other, data);
     * int FetchTransactionPosition(const uint256& txid);
     * bool FetchTransactionValidity(const uint256& txid);
     *
     * and so on...
     */
    void RecordTransaction(const uint256& txid, uint32_t posInBlock, int processingResult);
    std::vector<std::string> FetchTransactionDetails(const uint256& txid);
    uint32_t FetchTransactionPosition(const uint256& txid);
    std::string FetchInvalidReason(const uint256& txid);
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

    void getRecipients(const uint256 txid, string filterAddress, UniValue *recipientArray, uint64_t *total, uint64_t *numRecipients);
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

    void recordMatchedTrade(const uint256 txid1, const uint256 txid2, string address1, string address2, unsigned int prop1, unsigned int prop2, uint64_t amount1, uint64_t amount2, int blockNum, int64_t fee);
    void recordNewTrade(const uint256& txid, const std::string& address, uint32_t propertyIdForSale, uint32_t propertyIdDesired, int blockNum, int blockIndex);
    int deleteAboveBlock(int blockNum);
    bool exists(const uint256 &txid);
    void printStats();
    void printAll();
    bool getMatchingTrades(const uint256& txid, uint32_t propertyId, UniValue& tradeArray, int64_t& totalSold, int64_t& totalBought);
    void getTradesForAddress(std::string address, std::vector<uint256>& vecTransactions, uint32_t propertyIdFilter = 0);
    void getTradesForPair(uint32_t propertyIdSideA, uint32_t propertyIdSideB, UniValue& response, uint64_t count);
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
        PrintToConsole("Loading tx meta-info database: %s\n", status.ToString());
    }

    virtual ~CMPTxList()
    {
        if (msc_debug_persistence) PrintToLog("CMPTxList closed\n");
    }

    void recordTX(const uint256 &txid, bool fValid, int nBlock, unsigned int type, uint64_t nValue);
    void recordPaymentTX(const uint256 &txid, bool fValid, int nBlock, unsigned int vout, unsigned int propertyId, uint64_t nValue, string buyer, string seller);
    void recordMetaDExCancelTX(const uint256 &txidMaster, const uint256 &txidSub, bool fValid, int nBlock, unsigned int propertyId, uint64_t nValue);
    /** Records a "send all" sub record. */
    void recordSendAllSubRecord(const uint256& txid, int subRecordNumber, uint32_t propertyId, int64_t nvalue);

    string getKeyValue(string key);
    uint256 findMetaDExCancel(const uint256 txid);
    /** Returns the number of sub records. */
    int getNumberOfSubRecords(const uint256& txid);
    int getNumberOfMetaDExCancels(const uint256 txid);
    bool getPurchaseDetails(const uint256 txid, int purchaseNumber, string *buyer, string *seller, uint64_t *vout, uint64_t *propertyId, uint64_t *nValue);
    /** Retrieves details about a "send all" record. */
    bool getSendAllDetails(const uint256& txid, int subSend, uint32_t& propertyId, int64_t& amount);
    int getMPTransactionCountTotal();
    int getMPTransactionCountBlock(int block);

    int getDBVersion();
    int setDBVersion();

    bool exists(const uint256 &txid);
    bool getTX(const uint256 &txid, string &value);

    std::set<int> GetSeedBlocks(int startHeight, int endHeight);
    void LoadAlerts(int blockHeight);
    void LoadActivations(int blockHeight);
    bool LoadFreezeState(int blockHeight);
    bool CheckForFreezeTxs(int blockHeight);

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

int64_t getMPbalance(const std::string& address, uint32_t propertyId, TallyType ttype);
int64_t getUserAvailableMPbalance(const std::string& address, uint32_t propertyId);
int64_t getUserFrozenMPbalance(const std::string& address, uint32_t propertyId);

/** Global handler to initialize Omni Core. */
int mastercore_init();

/** Global handler to shut down Omni Core. */
int mastercore_shutdown();

/** Global handler to total wallet balances. */
void CheckWalletUpdate(bool forceUpdate = false);

/** Used to notify that the number of tokens for a property has changed. */
void NotifyTotalTokensChanged(uint32_t propertyId, int block);

int mastercore_handler_disc_begin(int nBlockNow, CBlockIndex const * pBlockIndex);
int mastercore_handler_disc_end(int nBlockNow, CBlockIndex const * pBlockIndex);
int mastercore_handler_block_begin(int nBlockNow, CBlockIndex const * pBlockIndex);
int mastercore_handler_block_end(int nBlockNow, CBlockIndex const * pBlockIndex, unsigned int);
bool mastercore_handler_tx(const CTransaction& tx, int nBlock, unsigned int idx, const CBlockIndex* pBlockIndex);
int mastercore_save_state( CBlockIndex const *pBlockIndex );

namespace mastercore
{
extern std::unordered_map<std::string, CMPTally> mp_tally_map;
extern CMPTxList *p_txlistdb;
extern CMPTradeList *t_tradelistdb;
extern CMPSTOList *s_stolistdb;
extern COmniTransactionDB *p_OmniTXDB;

// TODO: move, rename
extern CCoinsView viewDummy;
extern CCoinsViewCache view;
//! Guards coins view cache
extern CCriticalSection cs_tx_cache;

std::string strMPProperty(uint32_t propertyId);

bool isMPinBlockRange(int starting_block, int ending_block, bool bDeleteFound);

std::string FormatIndivisibleMP(int64_t n);

int WalletTxBuilder(const std::string& senderAddress, const std::string& receiverAddress, const std::string& redemptionAddress,
                 int64_t referenceAmount, const std::vector<unsigned char>& data, uint256& txid, std::string& rawHex, bool commit);

bool isTestEcosystemProperty(uint32_t propertyId);
bool isMainEcosystemProperty(uint32_t propertyId);
uint32_t GetNextPropertyId(bool maineco); // maybe move into sp

CMPTally* getTally(const std::string& address);

int64_t getTotalTokens(uint32_t propertyId, int64_t* n_owners_total = NULL);

std::string strTransactionType(uint16_t txType);

/** Returns the encoding class, used to embed a payload. */
int GetEncodingClass(const CTransaction& tx, int nBlock);

/** Determines, whether it is valid to use a Class C transaction for a given payload size. */
bool UseEncodingClassC(size_t nDataSize);

bool getValidMPTX(const uint256 &txid, int *block = NULL, unsigned int *type = NULL, uint64_t *nAmended = NULL);

bool update_tally_map(const std::string& who, uint32_t propertyId, int64_t amount, TallyType ttype);

std::string getTokenLabel(uint32_t propertyId);

/**
    NOTE: The following functions are only permitted for properties
          managed by a central issuer that have enabled freezing.
 **/
/** Adds an address and property to the frozenMap **/
void freezeAddress(const std::string& address, uint32_t propertyId);
/** Removes an address and property from the frozenMap **/
void unfreezeAddress(const std::string& address, uint32_t propertyId);
/** Checks whether an address and property are frozen **/
bool isAddressFrozen(const std::string& address, uint32_t propertyId);
/** Adds a property to the freezingEnabledMap **/
void enableFreezing(uint32_t propertyId, int liveBlock);
/** Removes a property from the freezingEnabledMap **/
void disableFreezing(uint32_t propertyId);
/** Checks whether a property has freezing enabled **/
bool isFreezingEnabled(uint32_t propertyId, int block);
/** Clears the freeze state in the event of a reorg **/
void ClearFreezeState();
/** Prints the freeze state **/
void PrintFreezeState();

}

#endif // OMNICORE_OMNICORE_H
