// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_WALLET_H
#define BITCOIN_WALLET_WALLET_H

#include <addresstype.h>
#include <consensus/amount.h>
#include <interfaces/chain.h>
#include <interfaces/handler.h>
#include <kernel/cs_main.h>
#include <logging.h>
#include <outputtype.h>
#include <policy/feerate.h>
#include <primitives/transaction.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <support/allocators/secure.h>
#include <sync.h>
#include <tinyformat.h>
#include <uint256.h>
#include <util/fs.h>
#include <util/hasher.h>
#include <util/result.h>
#include <util/string.h>
#include <util/time.h>
#include <util/ui_change_type.h>
#include <wallet/crypter.h>
#include <wallet/db.h>
#include <wallet/scriptpubkeyman.h>
#include <wallet/transaction.h>
#include <wallet/types.h>
#include <wallet/walletutil.h>

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/signals2/signal.hpp>

class CKey;
class CKeyID;
class CPubKey;
class Coin;
class SigningProvider;
enum class MemPoolRemovalReason;
enum class SigningResult;
namespace common {
enum class PSBTError;
} // namespace common
namespace interfaces {
class Wallet;
}
namespace wallet {
class CWallet;
class WalletBatch;
enum class DBErrors : int;
} // namespace wallet
struct CBlockLocator;
struct CExtKey;
struct FlatSigningProvider;
struct KeyOriginInfo;
struct PartiallySignedTransaction;
struct SignatureData;

using LoadWalletFn = std::function<void(std::unique_ptr<interfaces::Wallet> wallet)>;

struct bilingual_str;

namespace wallet {
struct WalletContext;

//! Explicitly unload and delete the wallet.
//! Blocks the current thread after signaling the unload intent so that all
//! wallet pointer owners release the wallet.
//! Note that, when blocking is not required, the wallet is implicitly unloaded
//! by the shared pointer deleter.
void UnloadWallet(std::shared_ptr<CWallet>&& wallet);

bool AddWallet(WalletContext& context, const std::shared_ptr<CWallet>& wallet);
bool RemoveWallet(WalletContext& context, const std::shared_ptr<CWallet>& wallet, std::optional<bool> load_on_start, std::vector<bilingual_str>& warnings);
bool RemoveWallet(WalletContext& context, const std::shared_ptr<CWallet>& wallet, std::optional<bool> load_on_start);
std::vector<std::shared_ptr<CWallet>> GetWallets(WalletContext& context);
std::shared_ptr<CWallet> GetDefaultWallet(WalletContext& context, size_t& count);
std::shared_ptr<CWallet> GetWallet(WalletContext& context, const std::string& name);
std::shared_ptr<CWallet> LoadWallet(WalletContext& context, const std::string& name, std::optional<bool> load_on_start, const DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error, std::vector<bilingual_str>& warnings);
std::shared_ptr<CWallet> CreateWallet(WalletContext& context, const std::string& name, std::optional<bool> load_on_start, DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error, std::vector<bilingual_str>& warnings);
std::shared_ptr<CWallet> RestoreWallet(WalletContext& context, const fs::path& backup_file, const std::string& wallet_name, std::optional<bool> load_on_start, DatabaseStatus& status, bilingual_str& error, std::vector<bilingual_str>& warnings);
std::unique_ptr<interfaces::Handler> HandleLoadWallet(WalletContext& context, LoadWalletFn load_wallet);
void NotifyWalletLoaded(WalletContext& context, const std::shared_ptr<CWallet>& wallet);
std::unique_ptr<WalletDatabase> MakeWalletDatabase(const std::string& name, const DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error);

//! -paytxfee default
constexpr CAmount DEFAULT_PAY_TX_FEE = 0;
//! -fallbackfee default
static const CAmount DEFAULT_FALLBACK_FEE = 0;
//! -discardfee default
static const CAmount DEFAULT_DISCARD_FEE = 10000;
//! -mintxfee default
static const CAmount DEFAULT_TRANSACTION_MINFEE = 1000;
//! -consolidatefeerate default
static const CAmount DEFAULT_CONSOLIDATE_FEERATE{10000}; // 10 sat/vbyte
/**
 * maximum fee increase allowed to do partial spend avoidance, even for nodes with this feature disabled by default
 *
 * A value of -1 disables this feature completely.
 * A value of 0 (current default) means to attempt to do partial spend avoidance, and use its results if the fees remain *unchanged*
 * A value > 0 means to do partial spend avoidance if the fee difference against a regular coin selection instance is in the range [0..value].
 */
static const CAmount DEFAULT_MAX_AVOIDPARTIALSPEND_FEE = 0;
//! discourage APS fee higher than this amount
constexpr CAmount HIGH_APS_FEE{COIN / 10000};
//! minimum recommended increment for replacement txs
static const CAmount WALLET_INCREMENTAL_RELAY_FEE = 5000;
//! Default for -spendzeroconfchange
static const bool DEFAULT_SPEND_ZEROCONF_CHANGE = true;
//! Default for -walletrejectlongchains
static const bool DEFAULT_WALLET_REJECT_LONG_CHAINS{true};
//! -txconfirmtarget default
static const unsigned int DEFAULT_TX_CONFIRM_TARGET = 6;
//! -walletrbf default
static const bool DEFAULT_WALLET_RBF = true;
static const bool DEFAULT_WALLETBROADCAST = true;
static const bool DEFAULT_DISABLE_WALLET = false;
static const bool DEFAULT_WALLETCROSSCHAIN = false;
//! -maxtxfee default
constexpr CAmount DEFAULT_TRANSACTION_MAXFEE{COIN / 10};
//! Discourage users to set fees higher than this amount (in satoshis) per kB
constexpr CAmount HIGH_TX_FEE_PER_KB{COIN / 100};
//! -maxtxfee will warn if called with a higher fee than this amount (in satoshis)
constexpr CAmount HIGH_MAX_TX_FEE{100 * HIGH_TX_FEE_PER_KB};
//! Pre-calculated constants for input size estimation in *virtual size*
static constexpr size_t DUMMY_NESTED_P2WPKH_INPUT_SIZE = 91;

class CCoinControl;

//! Default for -addresstype
constexpr OutputType DEFAULT_ADDRESS_TYPE{OutputType::BECH32};

static constexpr uint64_t KNOWN_WALLET_FLAGS =
        WALLET_FLAG_AVOID_REUSE
    |   WALLET_FLAG_BLANK_WALLET
    |   WALLET_FLAG_KEY_ORIGIN_METADATA
    |   WALLET_FLAG_LAST_HARDENED_XPUB_CACHED
    |   WALLET_FLAG_DISABLE_PRIVATE_KEYS
    |   WALLET_FLAG_DESCRIPTORS
    |   WALLET_FLAG_EXTERNAL_SIGNER;

static constexpr uint64_t MUTABLE_WALLET_FLAGS =
        WALLET_FLAG_AVOID_REUSE;

static const std::map<std::string,WalletFlags> WALLET_FLAG_MAP{
    {"avoid_reuse", WALLET_FLAG_AVOID_REUSE},
    {"blank", WALLET_FLAG_BLANK_WALLET},
    {"key_origin_metadata", WALLET_FLAG_KEY_ORIGIN_METADATA},
    {"last_hardened_xpub_cached", WALLET_FLAG_LAST_HARDENED_XPUB_CACHED},
    {"disable_private_keys", WALLET_FLAG_DISABLE_PRIVATE_KEYS},
    {"descriptor_wallet", WALLET_FLAG_DESCRIPTORS},
    {"external_signer", WALLET_FLAG_EXTERNAL_SIGNER}
};

/** A wrapper to reserve an address from a wallet
 *
 * ReserveDestination is used to reserve an address.
 * It is currently only used inside of CreateTransaction.
 *
 * Instantiating a ReserveDestination does not reserve an address. To do so,
 * GetReservedDestination() needs to be called on the object. Once an address has been
 * reserved, call KeepDestination() on the ReserveDestination object to make sure it is not
 * returned. Call ReturnDestination() to return the address so it can be reused (for
 * example, if the address was used in a new transaction
 * and that transaction was not completed and needed to be aborted).
 *
 * If an address is reserved and KeepDestination() is not called, then the address will be
 * returned when the ReserveDestination goes out of scope.
 */
class ReserveDestination
{
protected:
    //! The wallet to reserve from
    const CWallet* const pwallet;
    //! The ScriptPubKeyMan to reserve from. Based on type when GetReservedDestination is called
    ScriptPubKeyMan* m_spk_man{nullptr};
    OutputType const type;
    //! The index of the address's key in the keypool
    int64_t nIndex{-1};
    //! The destination
    CTxDestination address;
    //! Whether this is from the internal (change output) keypool
    bool fInternal{false};

public:
    //! Construct a ReserveDestination object. This does NOT reserve an address yet
    explicit ReserveDestination(CWallet* pwallet, OutputType type)
      : pwallet(pwallet)
      , type(type) { }

    ReserveDestination(const ReserveDestination&) = delete;
    ReserveDestination& operator=(const ReserveDestination&) = delete;

    //! Destructor. If a key has been reserved and not KeepKey'ed, it will be returned to the keypool
    ~ReserveDestination()
    {
        ReturnDestination();
    }

    //! Reserve an address
    util::Result<CTxDestination> GetReservedDestination(bool internal);
    //! Return reserved address
    void ReturnDestination();
    //! Keep the address. Do not return its key to the keypool when this object goes out of scope
    void KeepDestination();
};

/**
 * Address book data.
 */
struct CAddressBookData
{
    /**
     * Address label which is always nullopt for change addresses. For sending
     * and receiving addresses, it will be set to an arbitrary label string
     * provided by the user, or to "", which is the default label. The presence
     * or absence of a label is used to distinguish change addresses from
     * non-change addresses by wallet transaction listing and fee bumping code.
     */
    std::optional<std::string> label;

    /**
     * Address purpose which was originally recorded for payment protocol
     * support but now serves as a cached IsMine value. Wallet code should
     * not rely on this field being set.
     */
    std::optional<AddressPurpose> purpose;

    /**
     * Whether coins with this address have previously been spent. Set when the
     * the wallet avoid_reuse option is enabled and this is an IsMine address
     * that has already received funds and spent them. This is used during coin
     * selection to increase privacy by not creating different transactions
     * that spend from the same addresses.
     */
    bool previously_spent{false};

    /**
     * Map containing data about previously generated receive requests
     * requesting funds to be sent to this address. Only present for IsMine
     * addresses. Map keys are decimal numbers uniquely identifying each
     * request, and map values are serialized RecentRequestEntry objects
     * containing BIP21 URI information including message and amount.
     */
    std::map<std::string, std::string> receive_requests{};

    /** Accessor methods. */
    bool IsChange() const { return !label.has_value(); }
    std::string GetLabel() const { return label ? *label : std::string{}; }
    void SetLabel(std::string name) { label = std::move(name); }
};

inline std::string PurposeToString(AddressPurpose p)
{
    switch(p) {
    case AddressPurpose::RECEIVE: return "receive";
    case AddressPurpose::SEND: return "send";
    case AddressPurpose::REFUND: return "refund";
    } // no default case so the compiler will warn when a new enum as added
    assert(false);
}

inline std::optional<AddressPurpose> PurposeFromString(std::string_view s)
{
    if (s == "receive") return AddressPurpose::RECEIVE;
    else if (s == "send") return AddressPurpose::SEND;
    else if (s == "refund") return AddressPurpose::REFUND;
    return {};
}

struct CRecipient
{
    CTxDestination dest;
    CAmount nAmount;
    bool fSubtractFeeFromAmount;
};

class WalletRescanReserver; //forward declarations for ScanForWalletTransactions/RescanFromTime
/**
 * A CWallet maintains a set of transactions and balances, and provides the ability to create new transactions.
 */
class CWallet final : public WalletStorage, public interfaces::Chain::Notifications
{
private:
    CKeyingMaterial vMasterKey GUARDED_BY(cs_wallet);

    bool Unlock(const CKeyingMaterial& vMasterKeyIn);

    std::atomic<bool> fAbortRescan{false};
    std::atomic<bool> fScanningWallet{false}; // controlled by WalletRescanReserver
    std::atomic<bool> m_attaching_chain{false};
    std::atomic<bool> m_scanning_with_passphrase{false};
    std::atomic<SteadyClock::time_point> m_scanning_start{SteadyClock::time_point{}};
    std::atomic<double> m_scanning_progress{0};
    friend class WalletRescanReserver;

    //! the current wallet version: clients below this version are not able to load the wallet
    int nWalletVersion GUARDED_BY(cs_wallet){FEATURE_BASE};

    /** The next scheduled rebroadcast of wallet transactions. */
    NodeClock::time_point m_next_resend{GetDefaultNextResend()};
    /** Whether this wallet will submit newly created transactions to the node's mempool and
     * prompt rebroadcasts (see ResendWalletTransactions()). */
    bool fBroadcastTransactions = false;
    // Local time that the tip block was received. Used to schedule wallet rebroadcasts.
    std::atomic<int64_t> m_best_block_time {0};

    // First created key time. Used to skip blocks prior to this time.
    // 'std::numeric_limits<int64_t>::max()' if wallet is blank.
    std::atomic<int64_t> m_birth_time{std::numeric_limits<int64_t>::max()};

    /**
     * Used to keep track of spent outpoints, and
     * detect and report conflicts (double-spends or
     * mutated transactions where the mutant gets mined).
     */
    typedef std::unordered_multimap<COutPoint, uint256, SaltedOutpointHasher> TxSpends;
    TxSpends mapTxSpends GUARDED_BY(cs_wallet);
    void AddToSpends(const COutPoint& outpoint, const uint256& wtxid, WalletBatch* batch = nullptr) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    void AddToSpends(const CWalletTx& wtx, WalletBatch* batch = nullptr) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    /**
     * Add a transaction to the wallet, or update it.  confirm.block_* should
     * be set when the transaction was known to be included in a block.  When
     * block_hash.IsNull(), then wallet state is not updated in AddToWallet, but
     * notifications happen and cached balances are marked dirty.
     *
     * If fUpdate is true, existing transactions will be updated.
     * TODO: One exception to this is that the abandoned state is cleared under the
     * assumption that any further notification of a transaction that was considered
     * abandoned is an indication that it is not safe to be considered abandoned.
     * Abandoned state should probably be more carefully tracked via different
     * chain notifications or by checking mempool presence when necessary.
     *
     * Should be called with rescanning_old_block set to true, if the transaction is
     * not discovered in real time, but during a rescan of old blocks.
     */
    bool AddToWalletIfInvolvingMe(const CTransactionRef& tx, const SyncTxState& state, bool fUpdate, bool rescanning_old_block) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    /** Mark a transaction (and its in-wallet descendants) as conflicting with a particular block. */
    void MarkConflicted(const uint256& hashBlock, int conflicting_height, const uint256& hashTx);

    enum class TxUpdate { UNCHANGED, CHANGED, NOTIFY_CHANGED };

    using TryUpdatingStateFn = std::function<TxUpdate(CWalletTx& wtx)>;

    /** Mark a transaction (and its in-wallet descendants) as a particular tx state. */
    void RecursiveUpdateTxState(const uint256& tx_hash, const TryUpdatingStateFn& try_updating_state) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    void RecursiveUpdateTxState(WalletBatch* batch, const uint256& tx_hash, const TryUpdatingStateFn& try_updating_state) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    /** Mark a transaction's inputs dirty, thus forcing the outputs to be recomputed */
    void MarkInputsDirty(const CTransactionRef& tx) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    void SyncMetaData(std::pair<TxSpends::iterator, TxSpends::iterator>) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    void SyncTransaction(const CTransactionRef& tx, const SyncTxState& state, bool update_tx = true, bool rescanning_old_block = false) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    /** WalletFlags set on this wallet. */
    std::atomic<uint64_t> m_wallet_flags{0};

    bool SetAddressBookWithDB(WalletBatch& batch, const CTxDestination& address, const std::string& strName, const std::optional<AddressPurpose>& strPurpose);

    //! Unsets a wallet flag and saves it to disk
    void UnsetWalletFlagWithDB(WalletBatch& batch, uint64_t flag);

    //! Unset the blank wallet flag and saves it to disk
    void UnsetBlankWalletFlag(WalletBatch& batch) override;

    /** Interface for accessing chain state. */
    interfaces::Chain* m_chain;

    /** Wallet name: relative directory name or "" for default wallet. */
    std::string m_name;

    /** Internal database handle. */
    std::unique_ptr<WalletDatabase> m_database;

    /**
     * The following is used to keep track of how far behind the wallet is
     * from the chain sync, and to allow clients to block on us being caught up.
     *
     * Processed hash is a pointer on node's tip and doesn't imply that the wallet
     * has scanned sequentially all blocks up to this one.
     */
    uint256 m_last_block_processed GUARDED_BY(cs_wallet);

    /** Height of last block processed is used by wallet to know depth of transactions
     * without relying on Chain interface beyond asynchronous updates. For safety, we
     * initialize it to -1. Height is a pointer on node's tip and doesn't imply
     * that the wallet has scanned sequentially all blocks up to this one.
     */
    int m_last_block_processed_height GUARDED_BY(cs_wallet) = -1;

    std::map<OutputType, ScriptPubKeyMan*> m_external_spk_managers;
    std::map<OutputType, ScriptPubKeyMan*> m_internal_spk_managers;

    // Indexed by a unique identifier produced by each ScriptPubKeyMan using
    // ScriptPubKeyMan::GetID. In many cases it will be the hash of an internal structure
    std::map<uint256, std::unique_ptr<ScriptPubKeyMan>> m_spk_managers;

    // Appends spk managers into the main 'm_spk_managers'.
    // Must be the only method adding data to it.
    void AddScriptPubKeyMan(const uint256& id, std::unique_ptr<ScriptPubKeyMan> spkm_man);

    // Same as 'AddActiveScriptPubKeyMan' but designed for use within a batch transaction context
    void AddActiveScriptPubKeyManWithDb(WalletBatch& batch, uint256 id, OutputType type, bool internal);

    /** Store wallet flags */
    void SetWalletFlagWithDB(WalletBatch& batch, uint64_t flags);

    //! Cache of descriptor ScriptPubKeys used for IsMine. Maps ScriptPubKey to set of spkms
    std::unordered_map<CScript, std::vector<ScriptPubKeyMan*>, SaltedSipHasher> m_cached_spks;

    /**
     * Catch wallet up to current chain, scanning new blocks, updating the best
     * block locator and m_last_block_processed, and registering for
     * notifications about new blocks and transactions.
     */
    static bool AttachChain(const std::shared_ptr<CWallet>& wallet, interfaces::Chain& chain, const bool rescan_required, bilingual_str& error, std::vector<bilingual_str>& warnings);

    static NodeClock::time_point GetDefaultNextResend();

public:
    /**
     * Main wallet lock.
     * This lock protects all the fields added by CWallet.
     */
    mutable RecursiveMutex cs_wallet;

    WalletDatabase& GetDatabase() const override
    {
        assert(static_cast<bool>(m_database));
        return *m_database;
    }

    /** Get a name for this wallet for logging/debugging purposes.
     */
    const std::string& GetName() const { return m_name; }

    typedef std::map<unsigned int, CMasterKey> MasterKeyMap;
    MasterKeyMap mapMasterKeys;
    unsigned int nMasterKeyMaxID = 0;

    /** Construct wallet with specified name and database implementation. */
    CWallet(interfaces::Chain* chain, const std::string& name, std::unique_ptr<WalletDatabase> database)
        : m_chain(chain),
          m_name(name),
          m_database(std::move(database))
    {
    }

    ~CWallet()
    {
        // Should not have slots connected at this point.
        assert(NotifyUnload.empty());
    }

    bool IsCrypted() const;
    bool IsLocked() const override;
    bool Lock();

    /** Interface to assert chain access */
    bool HaveChain() const { return m_chain ? true : false; }

    /** Map from txid to CWalletTx for all transactions this wallet is
     * interested in, including received and sent transactions. */
    std::unordered_map<uint256, CWalletTx, SaltedTxidHasher> mapWallet GUARDED_BY(cs_wallet);

    typedef std::multimap<int64_t, CWalletTx*> TxItems;
    TxItems wtxOrdered;

    int64_t nOrderPosNext GUARDED_BY(cs_wallet) = 0;

    std::map<CTxDestination, CAddressBookData> m_address_book GUARDED_BY(cs_wallet);
    const CAddressBookData* FindAddressBookEntry(const CTxDestination&, bool allow_change = false) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    /** Set of Coins owned by this wallet that we won't try to spend from. A
     * Coin may be locked if it has already been used to fund a transaction
     * that hasn't confirmed yet. We wouldn't consider the Coin spent already,
     * but also shouldn't try to use it again. */
    std::set<COutPoint> setLockedCoins GUARDED_BY(cs_wallet);

    /** Registered interfaces::Chain::Notifications handler. */
    std::unique_ptr<interfaces::Handler> m_chain_notifications_handler;

    /** Interface for accessing chain state. */
    interfaces::Chain& chain() const { assert(m_chain); return *m_chain; }

    const CWalletTx* GetWalletTx(const uint256& hash) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    std::set<uint256> GetTxConflicts(const CWalletTx& wtx) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    /**
     * Return depth of transaction in blockchain:
     * <0  : conflicts with a transaction this deep in the blockchain
     *  0  : in memory pool, waiting to be included in a block
     * >=1 : this many blocks deep in the main chain
     *
     * Preconditions: it is only valid to call this function when the wallet is
     * online and the block index is loaded. So this cannot be called by
     * bitcoin-wallet tool code or by wallet migration code. If this is called
     * without the wallet being online, it won't be able able to determine the
     * the height of the last block processed, or the heights of blocks
     * referenced in transaction, and might cause assert failures.
     */
    int GetTxDepthInMainChain(const CWalletTx& wtx) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    /**
     * @return number of blocks to maturity for this transaction:
     *  0 : is not a coinbase transaction, or is a mature coinbase transaction
     * >0 : is a coinbase transaction which matures in this many blocks
     */
    int GetTxBlocksToMaturity(const CWalletTx& wtx) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool IsTxImmatureCoinBase(const CWalletTx& wtx) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    //! check whether we support the named feature
    bool CanSupportFeature(enum WalletFeature wf) const override EXCLUSIVE_LOCKS_REQUIRED(cs_wallet) { AssertLockHeld(cs_wallet); return IsFeatureSupported(nWalletVersion, wf); }

    bool IsSpent(const COutPoint& outpoint) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    // Whether this or any known scriptPubKey with the same single key has been spent.
    bool IsSpentKey(const CScript& scriptPubKey) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    void SetSpentKeyState(WalletBatch& batch, const uint256& hash, unsigned int n, bool used, std::set<CTxDestination>& tx_destinations) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    /** Display address on an external signer. */
    util::Result<void> DisplayAddress(const CTxDestination& dest) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    bool IsLockedCoin(const COutPoint& output) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool LockCoin(const COutPoint& output, WalletBatch* batch = nullptr) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool UnlockCoin(const COutPoint& output, WalletBatch* batch = nullptr) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool UnlockAllCoins() EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    void ListLockedCoins(std::vector<COutPoint>& vOutpts) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    /*
     * Rescan abort properties
     */
    void AbortRescan() { fAbortRescan = true; }
    bool IsAbortingRescan() const { return fAbortRescan; }
    bool IsScanning() const { return fScanningWallet; }
    bool IsScanningWithPassphrase() const { return m_scanning_with_passphrase; }
    SteadyClock::duration ScanningDuration() const { return fScanningWallet ? SteadyClock::now() - m_scanning_start.load() : SteadyClock::duration{}; }
    double ScanningProgress() const { return fScanningWallet ? (double) m_scanning_progress : 0; }

    //! Upgrade stored CKeyMetadata objects to store key origin info as KeyOriginInfo
    void UpgradeKeyMetadata() EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    //! Upgrade DescriptorCaches
    void UpgradeDescriptorCache() EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    bool LoadMinVersion(int nVersion) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet) { AssertLockHeld(cs_wallet); nWalletVersion = nVersion; return true; }

    //! Marks destination as previously spent.
    void LoadAddressPreviouslySpent(const CTxDestination& dest) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    //! Appends payment request to destination.
    void LoadAddressReceiveRequest(const CTxDestination& dest, const std::string& id, const std::string& request) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    //! Holds a timestamp at which point the wallet is scheduled (externally) to be relocked. Caller must arrange for actual relocking to occur via Lock().
    int64_t nRelockTime GUARDED_BY(cs_wallet){0};

    // Used to prevent concurrent calls to walletpassphrase RPC.
    Mutex m_unlock_mutex;
    // Used to prevent deleting the passphrase from memory when it is still in use.
    RecursiveMutex m_relock_mutex;

    bool Unlock(const SecureString& strWalletPassphrase);
    bool ChangeWalletPassphrase(const SecureString& strOldWalletPassphrase, const SecureString& strNewWalletPassphrase);
    bool EncryptWallet(const SecureString& strWalletPassphrase);

    void GetKeyBirthTimes(std::map<CKeyID, int64_t> &mapKeyBirth) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    unsigned int ComputeTimeSmart(const CWalletTx& wtx, bool rescanning_old_block) const;

    /**
     * Increment the next transaction order id
     * @return next transaction order id
     */
    int64_t IncOrderPosNext(WalletBatch *batch = nullptr) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    DBErrors ReorderTransactions();

    void MarkDirty();

    //! Callback for updating transaction metadata in mapWallet.
    //!
    //! @param wtx - reference to mapWallet transaction to update
    //! @param new_tx - true if wtx is newly inserted, false if it previously existed
    //!
    //! @return true if wtx is changed and needs to be saved to disk, otherwise false
    using UpdateWalletTxFn = std::function<bool(CWalletTx& wtx, bool new_tx)>;

    /**
     * Add the transaction to the wallet, wrapping it up inside a CWalletTx
     * @return the recently added wtx pointer or nullptr if there was a db write error.
     */
    CWalletTx* AddToWallet(CTransactionRef tx, const TxState& state, const UpdateWalletTxFn& update_wtx=nullptr, bool fFlushOnClose=true, bool rescanning_old_block = false);
    bool LoadToWallet(const uint256& hash, const UpdateWalletTxFn& fill_wtx) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    void transactionAddedToMempool(const CTransactionRef& tx) override;
    void blockConnected(ChainstateRole role, const interfaces::BlockInfo& block) override;
    void blockDisconnected(const interfaces::BlockInfo& block) override;
    void updatedBlockTip() override;
    int64_t RescanFromTime(int64_t startTime, const WalletRescanReserver& reserver, bool update);

    struct ScanResult {
        enum { SUCCESS, FAILURE, USER_ABORT } status = SUCCESS;

        //! Hash and height of most recent block that was successfully scanned.
        //! Unset if no blocks were scanned due to read errors or the chain
        //! being empty.
        uint256 last_scanned_block;
        std::optional<int> last_scanned_height;

        //! Height of the most recent block that could not be scanned due to
        //! read errors or pruning. Will be set if status is FAILURE, unset if
        //! status is SUCCESS, and may or may not be set if status is
        //! USER_ABORT.
        uint256 last_failed_block;
    };
    ScanResult ScanForWalletTransactions(const uint256& start_block, int start_height, std::optional<int> max_height, const WalletRescanReserver& reserver, bool fUpdate, const bool save_progress);
    void transactionRemovedFromMempool(const CTransactionRef& tx, MemPoolRemovalReason reason) override;
    /** Set the next time this wallet should resend transactions to 12-36 hours from now, ~1 day on average. */
    void SetNextResend() { m_next_resend = GetDefaultNextResend(); }
    /** Return true if all conditions for periodically resending transactions are met. */
    bool ShouldResend() const;
    void ResubmitWalletTransactions(bool relay, bool force);

    OutputType TransactionChangeType(const std::optional<OutputType>& change_type, const std::vector<CRecipient>& vecSend) const;

    /** Fetch the inputs and sign with SIGHASH_ALL. */
    bool SignTransaction(CMutableTransaction& tx) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    /** Sign the tx given the input coins and sighash. */
    bool SignTransaction(CMutableTransaction& tx, const std::map<COutPoint, Coin>& coins, int sighash, std::map<int, bilingual_str>& input_errors) const;
    SigningResult SignMessage(const std::string& message, const PKHash& pkhash, std::string& str_sig) const;

    /**
     * Fills out a PSBT with information from the wallet. Fills in UTXOs if we have
     * them. Tries to sign if sign=true. Sets `complete` if the PSBT is now complete
     * (i.e. has all required signatures or signature-parts, and is ready to
     * finalize.) Sets `error` and returns false if something goes wrong.
     *
     * @param[in]  psbtx PartiallySignedTransaction to fill in
     * @param[out] complete indicates whether the PSBT is now complete
     * @param[in]  sighash_type the sighash type to use when signing (if PSBT does not specify)
     * @param[in]  sign whether to sign or not
     * @param[in]  bip32derivs whether to fill in bip32 derivation information if available
     * @param[out] n_signed the number of inputs signed by this wallet
     * @param[in] finalize whether to create the final scriptSig or scriptWitness if possible
     * return error
     */
    std::optional<common::PSBTError> FillPSBT(PartiallySignedTransaction& psbtx,
                  bool& complete,
                  int sighash_type = SIGHASH_DEFAULT,
                  bool sign = true,
                  bool bip32derivs = true,
                  size_t* n_signed = nullptr,
                  bool finalize = true) const;

    /**
     * Submit the transaction to the node's mempool and then relay to peers.
     * Should be called after CreateTransaction unless you want to abort
     * broadcasting the transaction.
     *
     * @param[in] tx The transaction to be broadcast.
     * @param[in] mapValue key-values to be set on the transaction.
     * @param[in] orderForm BIP 70 / BIP 21 order form details to be set on the transaction.
     */
    void CommitTransaction(CTransactionRef tx, mapValue_t mapValue, std::vector<std::pair<std::string, std::string>> orderForm);

    /** Pass this transaction to node for mempool insertion and relay to peers if flag set to true */
    bool SubmitTxMemoryPoolAndRelay(CWalletTx& wtx, std::string& err_string, bool relay) const
        EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    bool ImportScripts(const std::set<CScript> scripts, int64_t timestamp) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool ImportPrivKeys(const std::map<CKeyID, CKey>& privkey_map, const int64_t timestamp) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool ImportPubKeys(const std::vector<CKeyID>& ordered_pubkeys, const std::map<CKeyID, CPubKey>& pubkey_map, const std::map<CKeyID, std::pair<CPubKey, KeyOriginInfo>>& key_origins, const bool add_keypool, const bool internal, const int64_t timestamp) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool ImportScriptPubKeys(const std::string& label, const std::set<CScript>& script_pub_keys, const bool have_solving_data, const bool apply_label, const int64_t timestamp) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    /** Updates wallet birth time if 'time' is below it */
    void MaybeUpdateBirthTime(int64_t time);

    CFeeRate m_pay_tx_fee{DEFAULT_PAY_TX_FEE};
    unsigned int m_confirm_target{DEFAULT_TX_CONFIRM_TARGET};
    /** Allow Coin Selection to pick unconfirmed UTXOs that were sent from our own wallet if it
     * cannot fund the transaction otherwise. */
    bool m_spend_zero_conf_change{DEFAULT_SPEND_ZEROCONF_CHANGE};
    bool m_signal_rbf{DEFAULT_WALLET_RBF};
    bool m_allow_fallback_fee{true}; //!< will be false if -fallbackfee=0
    CFeeRate m_min_fee{DEFAULT_TRANSACTION_MINFEE}; //!< Override with -mintxfee
    /**
     * If fee estimation does not have enough data to provide estimates, use this fee instead.
     * Has no effect if not using fee estimation
     * Override with -fallbackfee
     */
    CFeeRate m_fallback_fee{DEFAULT_FALLBACK_FEE};

     /** If the cost to spend a change output at this feerate is greater than the value of the
      * output itself, just drop it to fees. */
    CFeeRate m_discard_rate{DEFAULT_DISCARD_FEE};

    /** When the actual feerate is less than the consolidate feerate, we will tend to make transactions which
     * consolidate inputs. When the actual feerate is greater than the consolidate feerate, we will tend to make
     * transactions which have the lowest fees.
     */
    CFeeRate m_consolidate_feerate{DEFAULT_CONSOLIDATE_FEERATE};

    /** The maximum fee amount we're willing to pay to prioritize partial spend avoidance. */
    CAmount m_max_aps_fee{DEFAULT_MAX_AVOIDPARTIALSPEND_FEE}; //!< note: this is absolute fee, not fee rate
    OutputType m_default_address_type{DEFAULT_ADDRESS_TYPE};
    /**
     * Default output type for change outputs. When unset, automatically choose type
     * based on address type setting and the types other of non-change outputs
     * (see -changetype option documentation and implementation in
     * CWallet::TransactionChangeType for details).
     */
    std::optional<OutputType> m_default_change_type{};
    /** Absolute maximum transaction fee (in satoshis) used by default for the wallet */
    CAmount m_default_max_tx_fee{DEFAULT_TRANSACTION_MAXFEE};

    /** Number of pre-generated keys/scripts by each spkm (part of the look-ahead process, used to detect payments) */
    int64_t m_keypool_size{DEFAULT_KEYPOOL_SIZE};

    /** Notify external script when a wallet transaction comes in or is updated (handled by -walletnotify) */
    std::string m_notify_tx_changed_script;

    size_t KeypoolCountExternalKeys() const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool TopUpKeyPool(unsigned int kpSize = 0);

    std::optional<int64_t> GetOldestKeyPoolTime() const;

    // Filter struct for 'ListAddrBookAddresses'
    struct AddrBookFilter {
        // Fetch addresses with the provided label
        std::optional<std::string> m_op_label{std::nullopt};
        // Don't include change addresses by default
        bool ignore_change{true};
    };

    /**
     * Filter and retrieve destinations stored in the addressbook
     */
    std::vector<CTxDestination> ListAddrBookAddresses(const std::optional<AddrBookFilter>& filter) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    /**
     * Retrieve all the known labels in the address book
     */
    std::set<std::string> ListAddrBookLabels(const std::optional<AddressPurpose> purpose) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    /**
     * Walk-through the address book entries.
     * Stops when the provided 'ListAddrBookFunc' returns false.
     */
    using ListAddrBookFunc = std::function<void(const CTxDestination& dest, const std::string& label, bool is_change, const std::optional<AddressPurpose> purpose)>;
    void ForEachAddrBookEntry(const ListAddrBookFunc& func) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    /**
     * Marks all outputs in each one of the destinations dirty, so their cache is
     * reset and does not return outdated information.
     */
    void MarkDestinationsDirty(const std::set<CTxDestination>& destinations) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    util::Result<CTxDestination> GetNewDestination(const OutputType type, const std::string label);
    util::Result<CTxDestination> GetNewChangeDestination(const OutputType type);

    isminetype IsMine(const CTxDestination& dest) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    isminetype IsMine(const CScript& script) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    /**
     * Returns amount of debit if the input matches the
     * filter, otherwise returns 0
     */
    CAmount GetDebit(const CTxIn& txin, const isminefilter& filter) const;
    isminetype IsMine(const CTxOut& txout) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool IsMine(const CTransaction& tx) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    isminetype IsMine(const COutPoint& outpoint) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    /** should probably be renamed to IsRelevantToMe */
    bool IsFromMe(const CTransaction& tx) const;
    CAmount GetDebit(const CTransaction& tx, const isminefilter& filter) const;
    void chainStateFlushed(ChainstateRole role, const CBlockLocator& loc) override;

    DBErrors LoadWallet();

    /** Erases the provided transactions from the wallet. */
    util::Result<void> RemoveTxs(std::vector<uint256>& txs_to_remove) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    util::Result<void> RemoveTxs(WalletBatch& batch, std::vector<uint256>& txs_to_remove) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    bool SetAddressBook(const CTxDestination& address, const std::string& strName, const std::optional<AddressPurpose>& purpose);

    bool DelAddressBook(const CTxDestination& address);
    bool DelAddressBookWithDB(WalletBatch& batch, const CTxDestination& address);

    bool IsAddressPreviouslySpent(const CTxDestination& dest) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool SetAddressPreviouslySpent(WalletBatch& batch, const CTxDestination& dest, bool used) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    std::vector<std::string> GetAddressReceiveRequests() const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool SetAddressReceiveRequest(WalletBatch& batch, const CTxDestination& dest, const std::string& id, const std::string& value) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool EraseAddressReceiveRequest(WalletBatch& batch, const CTxDestination& dest, const std::string& id) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    unsigned int GetKeyPoolSize() const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    //! signify that a particular wallet feature is now used.
    void SetMinVersion(enum WalletFeature, WalletBatch* batch_in = nullptr) override;

    //! get the current wallet format (the oldest client version guaranteed to understand this wallet)
    int GetVersion() const { LOCK(cs_wallet); return nWalletVersion; }

    //! Get wallet transactions that conflict with given transaction (spend same outputs)
    std::set<uint256> GetConflicts(const uint256& txid) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    //! Check if a given transaction has any of its outputs spent by another transaction in the wallet
    bool HasWalletSpend(const CTransactionRef& tx) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    //! Flush wallet (bitdb flush)
    void Flush();

    //! Close wallet database
    void Close();

    /** Wallet is about to be unloaded */
    boost::signals2::signal<void ()> NotifyUnload;

    /**
     * Address book entry changed.
     * @note called without lock cs_wallet held.
     */
    boost::signals2::signal<void(const CTxDestination& address,
                                 const std::string& label, bool isMine,
                                 AddressPurpose purpose, ChangeType status)>
        NotifyAddressBookChanged;

    /**
     * Wallet transaction added, removed or updated.
     * @note called with lock cs_wallet held.
     */
    boost::signals2::signal<void(const uint256& hashTx, ChangeType status)> NotifyTransactionChanged;

    /** Show progress e.g. for rescan */
    boost::signals2::signal<void (const std::string &title, int nProgress)> ShowProgress;

    /** Watch-only address added */
    boost::signals2::signal<void (bool fHaveWatchOnly)> NotifyWatchonlyChanged;

    /** Keypool has new keys */
    boost::signals2::signal<void ()> NotifyCanGetAddressesChanged;

    /**
     * Wallet status (encrypted, locked) changed.
     * Note: Called without locks held.
     */
    boost::signals2::signal<void (CWallet* wallet)> NotifyStatusChanged;

    /** Inquire whether this wallet broadcasts transactions. */
    bool GetBroadcastTransactions() const { return fBroadcastTransactions; }
    /** Set whether this wallet broadcasts transactions. */
    void SetBroadcastTransactions(bool broadcast) { fBroadcastTransactions = broadcast; }

    /** Return whether transaction can be abandoned */
    bool TransactionCanBeAbandoned(const uint256& hashTx) const;

    /* Mark a transaction (and it in-wallet descendants) as abandoned so its inputs may be respent. */
    bool AbandonTransaction(const uint256& hashTx);

    /** Mark a transaction as replaced by another transaction. */
    bool MarkReplaced(const uint256& originalHash, const uint256& newHash);

    /* Initializes the wallet, returns a new CWallet instance or a null pointer in case of an error */
    static std::shared_ptr<CWallet> Create(WalletContext& context, const std::string& name, std::unique_ptr<WalletDatabase> database, uint64_t wallet_creation_flags, bilingual_str& error, std::vector<bilingual_str>& warnings);

    /**
     * Wallet post-init setup
     * Gives the wallet a chance to register repetitive tasks and complete post-init tasks
     */
    void postInitProcess();

    bool BackupWallet(const std::string& strDest) const;

    /* Returns true if HD is enabled */
    bool IsHDEnabled() const;

    /* Returns true if the wallet can give out new addresses. This means it has keys in the keypool or can generate new keys */
    bool CanGetAddresses(bool internal = false) const;

    /* Returns the time of the first created key or, in case of an import, it could be the time of the first received transaction */
    int64_t GetBirthTime() const { return m_birth_time; }

    /**
     * Blocks until the wallet state is up-to-date to /at least/ the current
     * chain at the time this function is entered
     * Obviously holding cs_main/cs_wallet when going into this call may cause
     * deadlock
     */
    void BlockUntilSyncedToCurrentChain() const LOCKS_EXCLUDED(::cs_main) EXCLUSIVE_LOCKS_REQUIRED(!cs_wallet);

    /** set a single wallet flag */
    void SetWalletFlag(uint64_t flags);

    /** Unsets a single wallet flag */
    void UnsetWalletFlag(uint64_t flag);

    /** check if a certain wallet flag is set */
    bool IsWalletFlagSet(uint64_t flag) const override;

    /** overwrite all flags by the given uint64_t
       flags must be uninitialised (or 0)
       only known flags may be present */
    void InitWalletFlags(uint64_t flags);
    /** Loads the flags into the wallet. (used by LoadWallet) */
    bool LoadWalletFlags(uint64_t flags);

    /** Determine if we are a legacy wallet */
    bool IsLegacy() const;

    /** Returns a bracketed wallet name for displaying in logs, will return [default wallet] if the wallet has no name */
    std::string GetDisplayName() const override
    {
        std::string wallet_name = GetName().length() == 0 ? "default wallet" : GetName();
        return strprintf("[%s]", wallet_name);
    };

    /** Prepends the wallet name in logging output to ease debugging in multi-wallet use cases */
    template <typename... Params>
    void WalletLogPrintf(const char* fmt, Params... parameters) const
    {
        LogPrintf(("%s " + std::string{fmt}).c_str(), GetDisplayName(), parameters...);
    };

    /** Upgrade the wallet */
    bool UpgradeWallet(int version, bilingual_str& error);

    //! Returns all unique ScriptPubKeyMans in m_internal_spk_managers and m_external_spk_managers
    std::set<ScriptPubKeyMan*> GetActiveScriptPubKeyMans() const;
    bool IsActiveScriptPubKeyMan(const ScriptPubKeyMan& spkm) const;

    //! Returns all unique ScriptPubKeyMans
    std::set<ScriptPubKeyMan*> GetAllScriptPubKeyMans() const;

    //! Get the ScriptPubKeyMan for the given OutputType and internal/external chain.
    ScriptPubKeyMan* GetScriptPubKeyMan(const OutputType& type, bool internal) const;

    //! Get all the ScriptPubKeyMans for a script
    std::set<ScriptPubKeyMan*> GetScriptPubKeyMans(const CScript& script) const;
    //! Get the ScriptPubKeyMan by id
    ScriptPubKeyMan* GetScriptPubKeyMan(const uint256& id) const;

    //! Get the SigningProvider for a script
    std::unique_ptr<SigningProvider> GetSolvingProvider(const CScript& script) const;
    std::unique_ptr<SigningProvider> GetSolvingProvider(const CScript& script, SignatureData& sigdata) const;

    //! Get the wallet descriptors for a script.
    std::vector<WalletDescriptor> GetWalletDescriptors(const CScript& script) const;

    //! Get the LegacyScriptPubKeyMan which is used for all types, internal, and external.
    LegacyScriptPubKeyMan* GetLegacyScriptPubKeyMan() const;
    LegacyScriptPubKeyMan* GetOrCreateLegacyScriptPubKeyMan();

    //! Make a LegacyScriptPubKeyMan and set it for all types, internal, and external.
    void SetupLegacyScriptPubKeyMan();

    bool WithEncryptionKey(std::function<bool (const CKeyingMaterial&)> cb) const override;

    bool HasEncryptionKeys() const override;

    /** Get last block processed height */
    int GetLastBlockHeight() const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet)
    {
        AssertLockHeld(cs_wallet);
        assert(m_last_block_processed_height >= 0);
        return m_last_block_processed_height;
    };
    uint256 GetLastBlockHash() const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet)
    {
        AssertLockHeld(cs_wallet);
        assert(m_last_block_processed_height >= 0);
        return m_last_block_processed;
    }
    /** Set last block processed height, currently only use in unit test */
    void SetLastBlockProcessed(int block_height, uint256 block_hash) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet)
    {
        AssertLockHeld(cs_wallet);
        m_last_block_processed_height = block_height;
        m_last_block_processed = block_hash;
    };

    //! Connect the signals from ScriptPubKeyMans to the signals in CWallet
    void ConnectScriptPubKeyManNotifiers();

    //! Instantiate a descriptor ScriptPubKeyMan from the WalletDescriptor and load it
    DescriptorScriptPubKeyMan& LoadDescriptorScriptPubKeyMan(uint256 id, WalletDescriptor& desc);

    //! Adds the active ScriptPubKeyMan for the specified type and internal. Writes it to the wallet file
    //! @param[in] id The unique id for the ScriptPubKeyMan
    //! @param[in] type The OutputType this ScriptPubKeyMan provides addresses for
    //! @param[in] internal Whether this ScriptPubKeyMan provides change addresses
    void AddActiveScriptPubKeyMan(uint256 id, OutputType type, bool internal);

    //! Loads an active ScriptPubKeyMan for the specified type and internal. (used by LoadWallet)
    //! @param[in] id The unique id for the ScriptPubKeyMan
    //! @param[in] type The OutputType this ScriptPubKeyMan provides addresses for
    //! @param[in] internal Whether this ScriptPubKeyMan provides change addresses
    void LoadActiveScriptPubKeyMan(uint256 id, OutputType type, bool internal);

    //! Remove specified ScriptPubKeyMan from set of active SPK managers. Writes the change to the wallet file.
    //! @param[in] id The unique id for the ScriptPubKeyMan
    //! @param[in] type The OutputType this ScriptPubKeyMan provides addresses for
    //! @param[in] internal Whether this ScriptPubKeyMan provides change addresses
    void DeactivateScriptPubKeyMan(uint256 id, OutputType type, bool internal);

    //! Create new DescriptorScriptPubKeyMan and add it to the wallet
    DescriptorScriptPubKeyMan& SetupDescriptorScriptPubKeyMan(WalletBatch& batch, const CExtKey& master_key, const OutputType& output_type, bool internal) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    //! Create new DescriptorScriptPubKeyMans and add them to the wallet
    void SetupDescriptorScriptPubKeyMans(WalletBatch& batch, const CExtKey& master_key) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    void SetupDescriptorScriptPubKeyMans() EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    //! Create new seed and default DescriptorScriptPubKeyMans for this wallet
    void SetupOwnDescriptorScriptPubKeyMans(WalletBatch& batch) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    //! Return the DescriptorScriptPubKeyMan for a WalletDescriptor if it is already in the wallet
    DescriptorScriptPubKeyMan* GetDescriptorScriptPubKeyMan(const WalletDescriptor& desc) const;

    //! Returns whether the provided ScriptPubKeyMan is internal
    //! @param[in] spk_man The ScriptPubKeyMan to test
    //! @return contains value only for active DescriptorScriptPubKeyMan, otherwise undefined
    std::optional<bool> IsInternalScriptPubKeyMan(ScriptPubKeyMan* spk_man) const;

    //! Add a descriptor to the wallet, return a ScriptPubKeyMan & associated output type
    ScriptPubKeyMan* AddWalletDescriptor(WalletDescriptor& desc, const FlatSigningProvider& signing_provider, const std::string& label, bool internal) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    /** Move all records from the BDB database to a new SQLite database for storage.
     * The original BDB file will be deleted and replaced with a new SQLite file.
     * A backup is not created.
     * May crash if something unexpected happens in the filesystem.
     */
    bool MigrateToSQLite(bilingual_str& error) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    //! Get all of the descriptors from a legacy wallet
    std::optional<MigrationData> GetDescriptorsForLegacy(bilingual_str& error) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    //! Adds the ScriptPubKeyMans given in MigrationData to this wallet, removes LegacyScriptPubKeyMan,
    //! and where needed, moves tx and address book entries to watchonly_wallet or solvable_wallet
    util::Result<void> ApplyMigrationData(WalletBatch& local_wallet_batch, MigrationData& data) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    //! Whether the (external) signer performs R-value signature grinding
    bool CanGrindR() const;

    //! Add scriptPubKeys for this ScriptPubKeyMan into the scriptPubKey cache
    void CacheNewScriptPubKeys(const std::set<CScript>& spks, ScriptPubKeyMan* spkm);

    void TopUpCallback(const std::set<CScript>& spks, ScriptPubKeyMan* spkm) override;

    //! Retrieve the xpubs in use by the active descriptors
    std::set<CExtPubKey> GetActiveHDPubKeys() const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    //! Find the private key for the given key id from the wallet's descriptors, if available
    //! Returns nullopt when no descriptor has the key or if the wallet is locked.
    std::optional<CKey> GetKey(const CKeyID& keyid) const;
};

/**
 * Called periodically by the schedule thread. Prompts individual wallets to resend
 * their transactions. Actual rebroadcast schedule is managed by the wallets themselves.
 */
void MaybeResendWalletTxs(WalletContext& context);

/** RAII object to check and reserve a wallet rescan */
class WalletRescanReserver
{
private:
    using Clock = std::chrono::steady_clock;
    using NowFn = std::function<Clock::time_point()>;
    CWallet& m_wallet;
    bool m_could_reserve{false};
    NowFn m_now;
public:
    explicit WalletRescanReserver(CWallet& w) : m_wallet(w) {}

    bool reserve(bool with_passphrase = false)
    {
        assert(!m_could_reserve);
        if (m_wallet.fScanningWallet.exchange(true)) {
            return false;
        }
        m_wallet.m_scanning_with_passphrase.exchange(with_passphrase);
        m_wallet.m_scanning_start = SteadyClock::now();
        m_wallet.m_scanning_progress = 0;
        m_could_reserve = true;
        return true;
    }

    bool isReserved() const
    {
        return (m_could_reserve && m_wallet.fScanningWallet);
    }

    Clock::time_point now() const { return m_now ? m_now() : Clock::now(); };

    void setNow(NowFn now) { m_now = std::move(now); }

    ~WalletRescanReserver()
    {
        if (m_could_reserve) {
            m_wallet.fScanningWallet = false;
            m_wallet.m_scanning_with_passphrase = false;
        }
    }
};

//! Add wallet name to persistent configuration so it will be loaded on startup.
bool AddWalletSetting(interfaces::Chain& chain, const std::string& wallet_name);

//! Remove wallet name from persistent configuration so it will not be loaded on startup.
bool RemoveWalletSetting(interfaces::Chain& chain, const std::string& wallet_name);

struct MigrationResult {
    std::string wallet_name;
    std::shared_ptr<CWallet> wallet;
    std::shared_ptr<CWallet> watchonly_wallet;
    std::shared_ptr<CWallet> solvables_wallet;
    fs::path backup_path;
};

//! Do all steps to migrate a legacy wallet to a descriptor wallet
[[nodiscard]] util::Result<MigrationResult> MigrateLegacyToDescriptor(const std::string& wallet_name, const SecureString& passphrase, WalletContext& context);
} // namespace wallet

#endif // BITCOIN_WALLET_WALLET_H
