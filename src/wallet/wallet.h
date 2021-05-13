// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_WALLET_WALLET_H
#define SYSCOIN_WALLET_WALLET_H

#include <amount.h>
#include <interfaces/chain.h>
#include <interfaces/handler.h>
#include <outputtype.h>
#include <policy/feerate.h>
#include <psbt.h>
#include <tinyformat.h>
#include <util/message.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/system.h>
#include <util/ui_change_type.h>
#include <validationinterface.h>
#include <wallet/coinselection.h>
#include <wallet/crypter.h>
#include <wallet/scriptpubkeyman.h>
#include <external_signer.h>
#include <wallet/walletdb.h>
#include <wallet/walletutil.h>

#include <algorithm>
#include <atomic>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

#include <boost/signals2/signal.hpp>
// SYSCOIN
#include <governance/governanceobject.h>
class CAssetCoinInfo;
class CAuxFeeDetails;

using LoadWalletFn = std::function<void(std::unique_ptr<interfaces::Wallet> wallet)>;

struct bilingual_str;

//! Explicitly unload and delete the wallet.
//! Blocks the current thread after signaling the unload intent so that all
//! wallet clients release the wallet.
//! Note that, when blocking is not required, the wallet is implicitly unloaded
//! by the shared pointer deleter.
void UnloadWallet(std::shared_ptr<CWallet>&& wallet);

bool AddWallet(const std::shared_ptr<CWallet>& wallet);
bool RemoveWallet(const std::shared_ptr<CWallet>& wallet, std::optional<bool> load_on_start, std::vector<bilingual_str>& warnings);
bool RemoveWallet(const std::shared_ptr<CWallet>& wallet, std::optional<bool> load_on_start);
std::vector<std::shared_ptr<CWallet>> GetWallets();
std::shared_ptr<CWallet> GetWallet(const std::string& name);
std::shared_ptr<CWallet> LoadWallet(interfaces::Chain& chain, const std::string& name, std::optional<bool> load_on_start, const DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error, std::vector<bilingual_str>& warnings);
std::shared_ptr<CWallet> CreateWallet(interfaces::Chain& chain, const std::string& name, std::optional<bool> load_on_start, DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error, std::vector<bilingual_str>& warnings);
std::unique_ptr<interfaces::Handler> HandleLoadWallet(LoadWalletFn load_wallet);
std::unique_ptr<WalletDatabase> MakeWalletDatabase(const std::string& name, const DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error);

//! -paytxfee default
constexpr CAmount DEFAULT_PAY_TX_FEE = 0;
//! -fallbackfee default
static const CAmount DEFAULT_FALLBACK_FEE = 1000;
//! -discardfee default
static const CAmount DEFAULT_DISCARD_FEE = 10000;
//! -mintxfee default
static const CAmount DEFAULT_TRANSACTION_MINFEE = 1000;
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
//! minimum recommended increment for BIP 125 replacement txs
static const CAmount WALLET_INCREMENTAL_RELAY_FEE = 5000;
//! Default for -spendzeroconfchange
static const bool DEFAULT_SPEND_ZEROCONF_CHANGE = true;
//! Default for -walletrejectlongchains
static const bool DEFAULT_WALLET_REJECT_LONG_CHAINS = false;
//! -txconfirmtarget default
static const unsigned int DEFAULT_TX_CONFIRM_TARGET = 6;
//! -walletrbf default
static const bool DEFAULT_WALLET_RBF = false;
static const bool DEFAULT_WALLETBROADCAST = true;
static const bool DEFAULT_DISABLE_WALLET = false;
//! -maxtxfee default
constexpr CAmount DEFAULT_TRANSACTION_MAXFEE{COIN / 10};
//! Discourage users to set fees higher than this amount (in satoshis) per kB
constexpr CAmount HIGH_TX_FEE_PER_KB{COIN / 100};
//! -maxtxfee will warn if called with a higher fee than this amount (in satoshis)
constexpr CAmount HIGH_MAX_TX_FEE{100 * HIGH_TX_FEE_PER_KB};
//! Pre-calculated constants for input size estimation in *virtual size*
static constexpr size_t DUMMY_NESTED_P2WPKH_INPUT_SIZE = 91;

class CCoinControl;
class COutput;
class CScript;
class CWalletTx;
struct FeeCalculation;
enum class FeeEstimateMode;
class ReserveDestination;

//! Default for -addresstype
constexpr OutputType DEFAULT_ADDRESS_TYPE{OutputType::BECH32};

static constexpr uint64_t KNOWN_WALLET_FLAGS =
        WALLET_FLAG_AVOID_REUSE
    |   WALLET_FLAG_BLANK_WALLET
    |   WALLET_FLAG_KEY_ORIGIN_METADATA
    |   WALLET_FLAG_DISABLE_PRIVATE_KEYS
    |   WALLET_FLAG_DESCRIPTORS
    |   WALLET_FLAG_EXTERNAL_SIGNER;

static constexpr uint64_t MUTABLE_WALLET_FLAGS =
        WALLET_FLAG_AVOID_REUSE;

static const std::map<std::string,WalletFlags> WALLET_FLAG_MAP{
    {"avoid_reuse", WALLET_FLAG_AVOID_REUSE},
    {"blank", WALLET_FLAG_BLANK_WALLET},
    {"key_origin_metadata", WALLET_FLAG_KEY_ORIGIN_METADATA},
    {"disable_private_keys", WALLET_FLAG_DISABLE_PRIVATE_KEYS},
    {"descriptor_wallet", WALLET_FLAG_DESCRIPTORS},
    {"external_signer", WALLET_FLAG_EXTERNAL_SIGNER}
};

extern const std::map<uint64_t,std::string> WALLET_FLAG_CAVEATS;

/** A wrapper to reserve an address from a wallet
 *
 * ReserveDestination is used to reserve an address.
 * It is currently only used inside of CreateTransaction.
 *
 * Instantiating a ReserveDestination does not reserve an address. To do so,
 * GetReservedDestination() needs to be called on the object. Once an address has been
 * reserved, call KeepDestination() on the ReserveDestination object to make sure it is not
 * returned. Call ReturnDestination() to return the address so it can be re-used (for
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
    bool GetReservedDestination(CTxDestination& pubkey, bool internal);
    //! Return reserved address
    void ReturnDestination();
    //! Keep the address. Do not return it's key to the keypool when this object goes out of scope
    void KeepDestination();
};

/** Address book data */
class CAddressBookData
{
private:
    bool m_change{true};
    std::string m_label;
public:
    std::string purpose;

    CAddressBookData() : purpose("unknown") {}

    typedef std::map<std::string, std::string> StringMap;
    StringMap destdata;

    bool IsChange() const { return m_change; }
    const std::string& GetLabel() const { return m_label; }
    void SetLabel(const std::string& label) {
        m_change = false;
        m_label = label;
    }
};

struct CRecipient
{
    CScript scriptPubKey;
    CAmount nAmount;
    bool fSubtractFeeFromAmount;
};

typedef std::map<std::string, std::string> mapValue_t;


static inline void ReadOrderPos(int64_t& nOrderPos, mapValue_t& mapValue)
{
    if (!mapValue.count("n"))
    {
        nOrderPos = -1; // TODO: calculate elsewhere
        return;
    }
    nOrderPos = atoi64(mapValue["n"]);
}


static inline void WriteOrderPos(const int64_t& nOrderPos, mapValue_t& mapValue)
{
    if (nOrderPos == -1)
        return;
    mapValue["n"] = ToString(nOrderPos);
}

struct COutputEntry
{
    CTxDestination destination;
    CAmount amount;
    int vout;
    // SYSCOIN
    CAssetCoinInfo assetInfo;
};

/** Legacy class used for deserializing vtxPrev for backwards compatibility.
 * vtxPrev was removed in commit 93a18a3650292afbb441a47d1fa1b94aeb0164e3,
 * but old wallet.dat files may still contain vtxPrev vectors of CMerkleTxs.
 * These need to get deserialized for field alignment when deserializing
 * a CWalletTx, but the deserialized values are discarded.**/
class CMerkleTx
{
public:
    template<typename Stream>
    void Unserialize(Stream& s)
    {
        CTransactionRef tx;
        uint256 hashBlock;
        std::vector<uint256> vMerkleBranch;
        int nIndex;

        s >> tx >> hashBlock >> vMerkleBranch >> nIndex;
    }
};

//Get the marginal bytes of spending the specified output
int CalculateMaximumSignedInputSize(const CTxOut& txout, const CWallet* pwallet, bool use_max_sig = false);

/**
 * A transaction with a bunch of additional info that only the owner cares about.
 * It includes any unrecorded transactions needed to link it back to the block chain.
 */
class CWalletTx
{
private:
    const CWallet* const pwallet;

    /** Constant used in hashBlock to indicate tx has been abandoned, only used at
     * serialization/deserialization to avoid ambiguity with conflicted.
     */
    static constexpr const uint256& ABANDON_HASH = uint256::ONEV;

public:
    /**
     * Key/value map with information about the transaction.
     *
     * The following keys can be read and written through the map and are
     * serialized in the wallet database:
     *
     *     "comment", "to"   - comment strings provided to sendtoaddress,
     *                         and sendmany wallet RPCs
     *     "replaces_txid"   - txid (as HexStr) of transaction replaced by
     *                         bumpfee on transaction created by bumpfee
     *     "replaced_by_txid" - txid (as HexStr) of transaction created by
     *                         bumpfee on transaction replaced by bumpfee
     *     "from", "message" - obsolete fields that could be set in UI prior to
     *                         2011 (removed in commit 4d9b223)
     *
     * The following keys are serialized in the wallet database, but shouldn't
     * be read or written through the map (they will be temporarily added and
     * removed from the map during serialization):
     *
     *     "fromaccount"     - serialized strFromAccount value
     *     "n"               - serialized nOrderPos value
     *     "timesmart"       - serialized nTimeSmart value
     *     "spent"           - serialized vfSpent value that existed prior to
     *                         2014 (removed in commit 93a18a3)
     */
    mapValue_t mapValue;
    std::vector<std::pair<std::string, std::string> > vOrderForm;
    unsigned int fTimeReceivedIsTxTime;
    unsigned int nTimeReceived; //!< time received by this node
    /**
     * Stable timestamp that never changes, and reflects the order a transaction
     * was added to the wallet. Timestamp is based on the block time for a
     * transaction added as part of a block, or else the time when the
     * transaction was received if it wasn't part of a block, with the timestamp
     * adjusted in both cases so timestamp order matches the order transactions
     * were added to the wallet. More details can be found in
     * CWallet::ComputeTimeSmart().
     */
    unsigned int nTimeSmart;
    /**
     * From me flag is set to 1 for transactions that were created by the wallet
     * on this syscoin node, and set to 0 for transactions that were created
     * externally and came in through the network or sendrawtransaction RPC.
     */
    bool fFromMe;
    int64_t nOrderPos; //!< position in ordered transaction list
    std::multimap<int64_t, CWalletTx*>::const_iterator m_it_wtxOrdered;

    // memory only
    enum AmountType { DEBIT, CREDIT, IMMATURE_CREDIT, AVAILABLE_CREDIT, AMOUNTTYPE_ENUM_ELEMENTS };
    CAmount GetCachableAmount(AmountType type, const isminefilter& filter, bool recalculate = false) const;
    mutable CachableAmount m_amounts[AMOUNTTYPE_ENUM_ELEMENTS];
    /**
     * This flag is true if all m_amounts caches are empty. This is particularly
     * useful in places where MarkDirty is conditionally called and the
     * condition can be expensive and thus can be skipped if the flag is true.
     * See MarkDestinationsDirty.
     */
    mutable bool m_is_cache_empty{true};
    mutable bool fChangeCached;
    mutable bool fInMempool;
    mutable CAmount nChangeCached;

    CWalletTx(const CWallet* wallet, CTransactionRef arg)
        : pwallet(wallet),
          tx(std::move(arg))
    {
        Init();
    }

    void Init()
    {
        mapValue.clear();
        vOrderForm.clear();
        fTimeReceivedIsTxTime = false;
        nTimeReceived = 0;
        nTimeSmart = 0;
        fFromMe = false;
        fChangeCached = false;
        fInMempool = false;
        nChangeCached = 0;
        nOrderPos = -1;
        m_confirm = Confirmation{};
    }

    CTransactionRef tx;

    /** New transactions start as UNCONFIRMED. At BlockConnected,
     * they will transition to CONFIRMED. In case of reorg, at BlockDisconnected,
     * they roll back to UNCONFIRMED. If we detect a conflicting transaction at
     * block connection, we update conflicted tx and its dependencies as CONFLICTED.
     * If tx isn't confirmed and outside of mempool, the user may switch it to ABANDONED
     * by using the abandontransaction call. This last status may be override by a CONFLICTED
     * or CONFIRMED transition.
     */
    enum Status {
        UNCONFIRMED,
        CONFIRMED,
        CONFLICTED,
        ABANDONED
    };

    /** Confirmation includes tx status and a triplet of {block height/block hash/tx index in block}
     * at which tx has been confirmed. All three are set to 0 if tx is unconfirmed or abandoned.
     * Meaning of these fields changes with CONFLICTED state where they instead point to block hash
     * and block height of the deepest conflicting tx.
     */
    struct Confirmation {
        Status status;
        int block_height;
        uint256 hashBlock;
        int nIndex;
        Confirmation(Status s = UNCONFIRMED, int b = 0, uint256 h = uint256(), int i = 0) : status(s), block_height(b), hashBlock(h), nIndex(i) {}
    };

    Confirmation m_confirm;

    template<typename Stream>
    void Serialize(Stream& s) const
    {
        mapValue_t mapValueCopy = mapValue;

        mapValueCopy["fromaccount"] = "";
        WriteOrderPos(nOrderPos, mapValueCopy);
        if (nTimeSmart) {
            mapValueCopy["timesmart"] = strprintf("%u", nTimeSmart);
        }

        std::vector<char> dummy_vector1; //!< Used to be vMerkleBranch
        std::vector<char> dummy_vector2; //!< Used to be vtxPrev
        bool dummy_bool = false; //!< Used to be fSpent
        uint256 serializedHash = isAbandoned() ? ABANDON_HASH : m_confirm.hashBlock;
        int serializedIndex = isAbandoned() || isConflicted() ? -1 : m_confirm.nIndex;
        s << tx << serializedHash << dummy_vector1 << serializedIndex << dummy_vector2 << mapValueCopy << vOrderForm << fTimeReceivedIsTxTime << nTimeReceived << fFromMe << dummy_bool;
    }

    template<typename Stream>
    void Unserialize(Stream& s)
    {
        Init();

        std::vector<uint256> dummy_vector1; //!< Used to be vMerkleBranch
        std::vector<CMerkleTx> dummy_vector2; //!< Used to be vtxPrev
        bool dummy_bool; //! Used to be fSpent
        int serializedIndex;
        s >> tx >> m_confirm.hashBlock >> dummy_vector1 >> serializedIndex >> dummy_vector2 >> mapValue >> vOrderForm >> fTimeReceivedIsTxTime >> nTimeReceived >> fFromMe >> dummy_bool;
        /* At serialization/deserialization, an nIndex == -1 means that hashBlock refers to
         * the earliest block in the chain we know this or any in-wallet ancestor conflicts
         * with. If nIndex == -1 and hashBlock is ABANDON_HASH, it means transaction is abandoned.
         * In same context, an nIndex >= 0 refers to a confirmed transaction (if hashBlock set) or
         * unconfirmed one. Older clients interpret nIndex == -1 as unconfirmed for backward
         * compatibility (pre-commit 9ac63d6).
         */
        if (serializedIndex == -1 && m_confirm.hashBlock == ABANDON_HASH) {
            setAbandoned();
        } else if (serializedIndex == -1) {
            setConflicted();
        } else if (!m_confirm.hashBlock.IsNull()) {
            m_confirm.nIndex = serializedIndex;
            setConfirmed();
        }

        ReadOrderPos(nOrderPos, mapValue);
        nTimeSmart = mapValue.count("timesmart") ? (unsigned int)atoi64(mapValue["timesmart"]) : 0;

        mapValue.erase("fromaccount");
        mapValue.erase("spent");
        mapValue.erase("n");
        mapValue.erase("timesmart");
    }

    void SetTx(CTransactionRef arg)
    {
        tx = std::move(arg);
    }

    //! make sure balances are recalculated
    void MarkDirty()
    {
        m_amounts[DEBIT].Reset();
        m_amounts[CREDIT].Reset();
        m_amounts[IMMATURE_CREDIT].Reset();
        m_amounts[AVAILABLE_CREDIT].Reset();
        fChangeCached = false;
        m_is_cache_empty = true;
    }

    //! filter decides which addresses will count towards the debit
    CAmount GetDebit(const isminefilter& filter) const;
    CAmount GetCredit(const isminefilter& filter) const;
    CAmount GetImmatureCredit(bool fUseCache = true) const;
    // TODO: Remove "NO_THREAD_SAFETY_ANALYSIS" and replace it with the correct
    // annotation "EXCLUSIVE_LOCKS_REQUIRED(pwallet->cs_wallet)". The
    // annotation "NO_THREAD_SAFETY_ANALYSIS" was temporarily added to avoid
    // having to resolve the issue of member access into incomplete type CWallet.
    CAmount GetAvailableCredit(bool fUseCache = true, const isminefilter& filter = ISMINE_SPENDABLE) const NO_THREAD_SAFETY_ANALYSIS;
    CAmount GetImmatureWatchOnlyCredit(const bool fUseCache = true) const;
    CAmount GetChange() const;

    /** Get the marginal bytes if spending the specified output from this transaction */
    int GetSpendSize(unsigned int out, bool use_max_sig = false) const
    {
        return CalculateMaximumSignedInputSize(tx->vout[out], pwallet, use_max_sig);
    }

    void GetAmounts(std::list<COutputEntry>& listReceived,
                    std::list<COutputEntry>& listSent, CAmount& nFee, const isminefilter& filter) const;

    bool IsFromMe(const isminefilter& filter) const
    {
        return (GetDebit(filter) > 0);
    }

    /** True if only scriptSigs are different */
    bool IsEquivalentTo(const CWalletTx& tx) const;

    bool InMempool() const;
    bool IsTrusted() const;

    int64_t GetTxTime() const;

    /** Pass this transaction to node for mempool insertion and relay to peers if flag set to true */
    bool SubmitMemoryPoolAndRelay(std::string& err_string, bool relay);

    // TODO: Remove "NO_THREAD_SAFETY_ANALYSIS" and replace it with the correct
    // annotation "EXCLUSIVE_LOCKS_REQUIRED(pwallet->cs_wallet)". The annotation
    // "NO_THREAD_SAFETY_ANALYSIS" was temporarily added to avoid having to
    // resolve the issue of member access into incomplete type CWallet. Note
    // that we still have the runtime check "AssertLockHeld(pwallet->cs_wallet)"
    // in place.
    std::set<uint256> GetConflicts() const NO_THREAD_SAFETY_ANALYSIS;

    /**
     * Return depth of transaction in blockchain:
     * <0  : conflicts with a transaction this deep in the blockchain
     *  0  : in memory pool, waiting to be included in a block
     * >=1 : this many blocks deep in the main chain
     */
    // TODO: Remove "NO_THREAD_SAFETY_ANALYSIS" and replace it with the correct
    // annotation "EXCLUSIVE_LOCKS_REQUIRED(pwallet->cs_wallet)". The annotation
    // "NO_THREAD_SAFETY_ANALYSIS" was temporarily added to avoid having to
    // resolve the issue of member access into incomplete type CWallet. Note
    // that we still have the runtime check "AssertLockHeld(pwallet->cs_wallet)"
    // in place.
    int GetDepthInMainChain() const NO_THREAD_SAFETY_ANALYSIS;
    bool IsInMainChain() const { return GetDepthInMainChain() > 0; }

    /**
     * @return number of blocks to maturity for this transaction:
     *  0 : is not a coinbase transaction, or is a mature coinbase transaction
     * >0 : is a coinbase transaction which matures in this many blocks
     */
    int GetBlocksToMaturity() const;
    bool isAbandoned() const { return m_confirm.status == CWalletTx::ABANDONED; }
    void setAbandoned()
    {
        m_confirm.status = CWalletTx::ABANDONED;
        m_confirm.hashBlock = uint256();
        m_confirm.block_height = 0;
        m_confirm.nIndex = 0;
    }
    bool isConflicted() const { return m_confirm.status == CWalletTx::CONFLICTED; }
    void setConflicted() { m_confirm.status = CWalletTx::CONFLICTED; }
    bool isUnconfirmed() const { return m_confirm.status == CWalletTx::UNCONFIRMED; }
    void setUnconfirmed() { m_confirm.status = CWalletTx::UNCONFIRMED; }
    bool isConfirmed() const { return m_confirm.status == CWalletTx::CONFIRMED; }
    void setConfirmed() { m_confirm.status = CWalletTx::CONFIRMED; }
    const uint256& GetHash() const { return tx->GetHash(); }
    bool IsCoinBase() const { return tx->IsCoinBase(); }
    bool IsImmatureCoinBase() const;

    // Disable copying of CWalletTx objects to prevent bugs where instances get
    // copied in and out of the mapWallet map, and fields are updated in the
    // wrong copy.
    CWalletTx(CWalletTx const &) = delete;
    void operator=(CWalletTx const &x) = delete;
};

class COutput
{
public:
    const CWalletTx *tx;

    /** Index in tx->vout. */
    int i;

    /**
     * Depth in block chain.
     * If > 0: the tx is on chain and has this many confirmations.
     * If = 0: the tx is waiting confirmation.
     * If < 0: a conflicting tx is on chain and has this many confirmations. */
    int nDepth;

    /** Pre-computed estimated size of this output as a fully-signed input in a transaction. Can be -1 if it could not be calculated */
    int nInputBytes;

    /** Whether we have the private keys to spend this output */
    bool fSpendable;

    /** Whether we know how to spend this output, ignoring the lack of keys */
    bool fSolvable;

    /** Whether to use the maximum sized, 72 byte signature when calculating the size of the input spend. This should only be set when watch-only outputs are allowed */
    bool use_max_sig;

    /**
     * Whether this output is considered safe to spend. Unconfirmed transactions
     * from outside keys and unconfirmed replacement transactions are considered
     * unsafe and will not be used to fund new spending transactions.
     */
    bool fSafe;

    COutput(const CWalletTx *txIn, int iIn, int nDepthIn, bool fSpendableIn, bool fSolvableIn, bool fSafeIn, bool use_max_sig_in = false)
    {
        tx = txIn; i = iIn; nDepth = nDepthIn; fSpendable = fSpendableIn; fSolvable = fSolvableIn; fSafe = fSafeIn; nInputBytes = -1; use_max_sig = use_max_sig_in;
        // If known and signable by the given wallet, compute nInputBytes
        // Failure will keep this value -1
        if (fSpendable && tx) {
            nInputBytes = tx->GetSpendSize(i, use_max_sig);
        }
    }

    std::string ToString() const;

    inline CInputCoin GetInputCoin() const
    {
        return CInputCoin(tx->tx, i, nInputBytes);
    }
};

/** Parameters for one iteration of Coin Selection. */
struct CoinSelectionParams
{
    /** Toggles use of Branch and Bound instead of Knapsack solver. */
    bool use_bnb = true;
    /** Size of a change output in bytes, determined by the output type. */
    size_t change_output_size = 0;
    /** Size of the input to spend a change output in virtual bytes. */
    size_t change_spend_size = 0;
    /** The targeted feerate of the transaction being built. */
    CFeeRate m_effective_feerate;
    /** The feerate estimate used to estimate an upper bound on what should be sufficient to spend
     * the change output sometime in the future. */
    CFeeRate m_long_term_feerate;
    /** If the cost to spend a change output at the discard feerate exceeds its value, drop it to fees. */
    CFeeRate m_discard_feerate;
    /** Size of the transaction before coin selection, consisting of the header and recipient
     * output(s), excluding the inputs and change output(s). */
    size_t tx_noinputs_size = 0;
    /** Indicate that we are subtracting the fee from outputs */
    bool m_subtract_fee_outputs = false;
    /** When true, always spend all (up to OUTPUT_GROUP_MAX_ENTRIES) or none of the outputs
     * associated with the same address. This helps reduce privacy leaks resulting from address
     * reuse. Dust outputs are not eligible to be added to output groups and thus not considered. */
    bool m_avoid_partial_spends = false;

    CoinSelectionParams(bool use_bnb, size_t change_output_size, size_t change_spend_size, CFeeRate effective_feerate,
                        CFeeRate long_term_feerate, CFeeRate discard_feerate, size_t tx_noinputs_size, bool avoid_partial) :
        use_bnb(use_bnb),
        change_output_size(change_output_size),
        change_spend_size(change_spend_size),
        m_effective_feerate(effective_feerate),
        m_long_term_feerate(long_term_feerate),
        m_discard_feerate(discard_feerate),
        tx_noinputs_size(tx_noinputs_size),
        m_avoid_partial_spends(avoid_partial)
    {}
    CoinSelectionParams() {}
};
class WalletRescanReserver; //forward declarations for ScanForWalletTransactions/RescanFromTime
/**
 * A CWallet maintains a set of transactions and balances, and provides the ability to create new transactions.
 */
class CWallet final : public WalletStorage, public interfaces::Chain::Notifications
{
private:
    CKeyingMaterial vMasterKey GUARDED_BY(cs_wallet);


    bool Unlock(const CKeyingMaterial& vMasterKeyIn, bool accept_no_keys = false);

    std::atomic<bool> fAbortRescan{false};
    std::atomic<bool> fScanningWallet{false}; // controlled by WalletRescanReserver
    std::atomic<int64_t> m_scanning_start{0};
    std::atomic<double> m_scanning_progress{0};
    friend class WalletRescanReserver;

    //! the current wallet version: clients below this version are not able to load the wallet
    int nWalletVersion GUARDED_BY(cs_wallet){FEATURE_BASE};

    /** The next scheduled rebroadcast of wallet transactions. */
    int64_t nNextResend = 0;
    /** Whether this wallet will submit newly created transactions to the node's mempool and
     * prompt rebroadcasts (see ResendWalletTransactions()). */
    bool fBroadcastTransactions = false;
    // Local time that the tip block was received. Used to schedule wallet rebroadcasts.
    std::atomic<int64_t> m_best_block_time {0};

    /**
     * Used to keep track of spent outpoints, and
     * detect and report conflicts (double-spends or
     * mutated transactions where the mutant gets mined).
     */
    typedef std::multimap<COutPoint, uint256> TxSpends;
    TxSpends mapTxSpends GUARDED_BY(cs_wallet);
    void AddToSpends(const COutPoint& outpoint, const uint256& wtxid) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    void AddToSpends(const uint256& wtxid) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    /**
     * Add a transaction to the wallet, or update it.  pIndex and posInBlock should
     * be set when the transaction was known to be included in a block.  When
     * pIndex == nullptr, then wallet state is not updated in AddToWallet, but
     * notifications happen and cached balances are marked dirty.
     *
     * If fUpdate is true, existing transactions will be updated.
     * TODO: One exception to this is that the abandoned state is cleared under the
     * assumption that any further notification of a transaction that was considered
     * abandoned is an indication that it is not safe to be considered abandoned.
     * Abandoned state should probably be more carefully tracked via different
     * posInBlock signals or by checking mempool presence when necessary.
     */
    bool AddToWalletIfInvolvingMe(const CTransactionRef& tx, CWalletTx::Confirmation confirm, bool fUpdate) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    /** Mark a transaction (and its in-wallet descendants) as conflicting with a particular block. */
    void MarkConflicted(const uint256& hashBlock, int conflicting_height, const uint256& hashTx);

    /** Mark a transaction's inputs dirty, thus forcing the outputs to be recomputed */
    void MarkInputsDirty(const CTransactionRef& tx) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    void SyncMetaData(std::pair<TxSpends::iterator, TxSpends::iterator>) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    /* Used by TransactionAddedToMemorypool/BlockConnected/Disconnected/ScanForWalletTransactions.
     * Should be called with non-zero block_hash and posInBlock if this is for a transaction that is included in a block. */
    void SyncTransaction(const CTransactionRef& tx, CWalletTx::Confirmation confirm, bool update_tx = true) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    /** WalletFlags set on this wallet. */
    std::atomic<uint64_t> m_wallet_flags{0};

    bool SetAddressBookWithDB(WalletBatch& batch, const CTxDestination& address, const std::string& strName, const std::string& strPurpose);

    //! Unsets a wallet flag and saves it to disk
    void UnsetWalletFlagWithDB(WalletBatch& batch, uint64_t flag);

    //! Unset the blank wallet flag and saves it to disk
    void UnsetBlankWalletFlag(WalletBatch& batch) override;

    /** Interface for accessing chain state. */
    interfaces::Chain* m_chain;

    /** Wallet name: relative directory name or "" for default wallet. */
    std::string m_name;

    /** Internal database handle. */
    std::unique_ptr<WalletDatabase> const m_database;

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
    bool CreateTransactionInternal(const std::vector<CRecipient>& vecSend, CTransactionRef& tx, CAmount& nFeeRet, int& nChangePosInOut, bilingual_str& error, const CCoinControl &coin_control, FeeCalculation& fee_calc_out, bool sign, const int& nVersion=2);

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

    /**
     * Select a set of coins such that nValueRet >= nTargetValue and at least
     * all coins from coin_control are selected; never select unconfirmed coins if they are not ours
     * param@[out]  setCoinsRet         Populated with inputs including pre-selected inputs from
     *                                  coin_control and Coin Selection if successful.
     * param@[out]  nValueRet           Total value of selected coins including pre-selected ones
     *                                  from coin_control and Coin Selection if successful.
     */
    // SYSCOIN
    bool SelectCoins(const std::vector<COutput>& vAvailableCoins, const CAmount& nTargetValue, std::set<CInputCoin>& setCoinsRet, CAmount& nValueRet,
                    const CCoinControl& coin_control, CoinSelectionParams& coin_selection_params, bool& bnb_used) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    bool SelectCoins(const std::vector<COutput>& vAvailableCoins, const CAmount& nTargetValue, const CAssetCoinInfo& nTargetValueAsset, std::set<CInputCoin>& setCoinsRet, CAmount& nValueRet, CAmount& nValueRetAsset,
                    const CCoinControl& coin_control, CoinSelectionParams& coin_selection_params, bool& bnb_used, const int& nVersion=0) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);


    /** Get a name for this wallet for logging/debugging purposes.
     */
    const std::string& GetName() const { return m_name; }

    typedef std::map<unsigned int, CMasterKey> MasterKeyMap;
    MasterKeyMap mapMasterKeys;
    unsigned int nMasterKeyMaxID = 0;

    // SYSCOIN Map from governance object hash to governance object, they are added by gobject_prepare.
    std::map<uint256, CGovernanceObject> m_gobjects;

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
    std::map<uint256, CWalletTx> mapWallet GUARDED_BY(cs_wallet);

    typedef std::multimap<int64_t, CWalletTx*> TxItems;
    TxItems wtxOrdered;

    int64_t nOrderPosNext GUARDED_BY(cs_wallet) = 0;
    uint64_t nAccountingEntryNumber = 0;

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
    bool IsTrusted(const CWalletTx& wtx, std::set<uint256>& trusted_parents) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    //! check whether we support the named feature
    bool CanSupportFeature(enum WalletFeature wf) const override EXCLUSIVE_LOCKS_REQUIRED(cs_wallet) { AssertLockHeld(cs_wallet); return IsFeatureSupported(nWalletVersion, wf); }

    /**
     * populate vCoins with vector of available COutputs.
     */
    // SYSCOIN
    void AvailableCoins(std::vector<COutput>& vCoins, const CCoinControl* coinControl = nullptr, const CAmount& nMinimumAmount = 1, const CAmount& nMaximumAmount = MAX_MONEY, const CAmount& nMinimumSumAmount = MAX_MONEY, const CAmount& nMinimumAmountAsset = 0, const CAmount& nMaximumAmountAsset = MAX_ASSET, const CAmount& nMinimumSumAmountAsset = MAX_ASSET, const uint64_t nMaximumCount = 0, const bool bIncludeLocked = false, const CAssetCoinInfo& assetInfo=CAssetCoinInfo(), const int &txVersion=0) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    /**
     * Return list of available coins and locked coins grouped by non-change output address.
     */
    std::map<CTxDestination, std::vector<COutput>> ListCoins() const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    /**
     * Find non-change parent output.
     */
    const CTxOut& FindNonChangeParentOutput(const CTransaction& tx, int output) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    /**
     * Shuffle and select coins until nTargetValue+nTargetValueAsset is reached while avoiding
     * small change; This method is stochastic for some inputs and upon
     * completion the coin set and corresponding actual target value is
     * assembled
     * param@[in]   coins           Set of UTXOs to consider. These will be categorized into
     *                              OutputGroups and filtered using eligibility_filter before
     *                              selecting coins.
     * param@[out]  setCoinsRet     Populated with the coins selected if successful.
     * param@[out]  nValueRet       Used to return the total value of selected coins.
     */
    bool SelectCoinsMinConf(const CAmount& nTargetValue, const CoinEligibilityFilter& eligibility_filter, std::vector<COutput> coins,
        std::set<CInputCoin>& setCoinsRet, CAmount& nValueRet, const CoinSelectionParams& coin_selection_params, bool& bnb_used) const;
    // SYSCOIN
    bool SelectCoinsMinConf(const CAmount& nTargetValue, CAssetCoinInfo nTargetValueAsset, const CoinEligibilityFilter& eligibility_filter, std::vector<COutput> coins,
        std::set<CInputCoin>& setCoinsRet, CAmount& nValueRet, CAmount& nValueRetAsset, const CoinSelectionParams& coin_selection_params, bool& bnb_used, const int& nVersion=0) const;

    bool IsSpent(const uint256& hash, unsigned int n) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    // Whether this or any known UTXO with the same single key has been spent.
    bool IsSpentKey(const uint256& hash, unsigned int n) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    void SetSpentKeyState(WalletBatch& batch, const uint256& hash, unsigned int n, bool used, std::set<CTxDestination>& tx_destinations) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    // SYSCOIN
    void GroupOutputs(const std::vector<COutput>& outputs, bool separate_coins, const CFeeRate& effective_feerate, const CFeeRate& long_term_feerate, const CoinEligibilityFilter& filter, bool positive_only, std::vector<OutputGroup> &coins_out, const CAssetCoinInfo& nTargetValueAsset=CAssetCoinInfo(), std::vector<OutputGroup> *coinsasset_out = nullptr) const;

    /** Display address on an external signer. Returns false if external signer support is not compiled */
    bool DisplayAddress(const CTxDestination& dest) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    bool IsLockedCoin(uint256 hash, unsigned int n) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    void LockCoin(const COutPoint& output) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    void UnlockCoin(const COutPoint& output) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    void UnlockAllCoins() EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    void ListLockedCoins(std::vector<COutPoint>& vOutpts) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    // SYSCOIN
    void ListProTxCoins(std::vector<COutPoint>& vOutpts) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    /*
     * Rescan abort properties
     */
    void AbortRescan() { fAbortRescan = true; }
    bool IsAbortingRescan() const { return fAbortRescan; }
    bool IsScanning() const { return fScanningWallet; }
    int64_t ScanningDuration() const { return fScanningWallet ? GetTimeMillis() - m_scanning_start : 0; }
    double ScanningProgress() const { return fScanningWallet ? (double) m_scanning_progress : 0; }

    //! Upgrade stored CKeyMetadata objects to store key origin info as KeyOriginInfo
    void UpgradeKeyMetadata() EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    bool LoadMinVersion(int nVersion) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet) { AssertLockHeld(cs_wallet); nWalletVersion = nVersion; return true; }

    /**
     * Adds a destination data tuple to the store, and saves it to disk
     * When adding new fields, take care to consider how DelAddressBook should handle it!
     */
    bool AddDestData(WalletBatch& batch, const CTxDestination& dest, const std::string& key, const std::string& value) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    //! Erases a destination data tuple in the store and on disk
    bool EraseDestData(WalletBatch& batch, const CTxDestination& dest, const std::string& key) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    //! Adds a destination data tuple to the store, without saving it to disk
    void LoadDestData(const CTxDestination& dest, const std::string& key, const std::string& value) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    //! Look up a destination data tuple in the store, return true if found false otherwise
    bool GetDestData(const CTxDestination& dest, const std::string& key, std::string* value) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    //! Get all destination values matching a prefix.
    std::vector<std::string> GetDestValues(const std::string& prefix) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    //! Holds a timestamp at which point the wallet is scheduled (externally) to be relocked. Caller must arrange for actual relocking to occur via Lock().
    int64_t nRelockTime GUARDED_BY(cs_wallet){0};

    // Used to prevent concurrent calls to walletpassphrase RPC.
    Mutex m_unlock_mutex;
    bool Unlock(const SecureString& strWalletPassphrase, bool accept_no_keys = false);
    bool ChangeWalletPassphrase(const SecureString& strOldWalletPassphrase, const SecureString& strNewWalletPassphrase);
    bool EncryptWallet(const SecureString& strWalletPassphrase);

    void GetKeyBirthTimes(std::map<CKeyID, int64_t> &mapKeyBirth) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    unsigned int ComputeTimeSmart(const CWalletTx& wtx) const;

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

    CWalletTx* AddToWallet(CTransactionRef tx, const CWalletTx::Confirmation& confirm, const UpdateWalletTxFn& update_wtx=nullptr, bool fFlushOnClose=true);
    bool LoadToWallet(const uint256& hash, const UpdateWalletTxFn& fill_wtx) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    void transactionAddedToMempool(const CTransactionRef& tx, uint64_t mempool_sequence) override;
    void blockConnected(const CBlock& block, int height) override;
    void blockDisconnected(const CBlock& block, int height) override;
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
    ScanResult ScanForWalletTransactions(const uint256& start_block, int start_height, std::optional<int> max_height, const WalletRescanReserver& reserver, bool fUpdate);
    void transactionRemovedFromMempool(const CTransactionRef& tx, MemPoolRemovalReason reason, uint64_t mempool_sequence) override;
    void ReacceptWalletTransactions() EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    void ResendWalletTransactions();
    struct Balance {
        CAmount m_mine_trusted{0};           //!< Trusted, at depth=GetBalance.min_depth or more
        CAmount m_mine_untrusted_pending{0}; //!< Untrusted, but in mempool (pending)
        CAmount m_mine_immature{0};          //!< Immature coinbases in the main chain
        CAmount m_watchonly_trusted{0};
        CAmount m_watchonly_untrusted_pending{0};
        CAmount m_watchonly_immature{0};
    };
    Balance GetBalance(int min_depth = 0, bool avoid_reuse = true) const;
    CAmount GetAvailableBalance(const CCoinControl* coinControl = nullptr) const;

    OutputType TransactionChangeType(const std::optional<OutputType>& change_type, const std::vector<CRecipient>& vecSend) const;

    /**
     * Insert additional inputs into the transaction by
     * calling CreateTransaction();
     */
     // SYSCOIN
    bool FundTransaction(CMutableTransaction& tx, CAmount& nFeeRet, int& nChangePosInOut, bilingual_str& error, bool lockUnspents, const std::set<int>& setSubtractFeeFromOutputs, CCoinControl&);
    // Fetch the inputs and sign with SIGHASH_ALL.
    bool SignTransaction(CMutableTransaction& tx) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    /** Sign the tx given the input coins and sighash. */
    bool SignTransaction(CMutableTransaction& tx, const std::map<COutPoint, Coin>& coins, int sighash, std::map<int, std::string>& input_errors) const;
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
     * return error
     */
    TransactionError FillPSBT(PartiallySignedTransaction& psbtx,
                  bool& complete,
                  int sighash_type = 1 /* SIGHASH_ALL */,
                  bool sign = true,
                  bool bip32derivs = true,
                  size_t* n_signed = nullptr) const;

    /**
     * Create a new transaction paying the recipients with a set of coins
     * selected by SelectCoins(); Also create the change output, when needed
     * @note passing nChangePosInOut as -1 will result in setting a random position
     */
    // SYSCOIN
    bool CreateTransaction(const std::vector<CRecipient>& vecSend, CTransactionRef& tx, CAmount& nFeeRet, int& nChangePosInOut, bilingual_str& error, const CCoinControl& coin_control, FeeCalculation& fee_calc_out, bool sign = true, const int& nVersion = 2);
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

    bool DummySignTx(CMutableTransaction &txNew, const std::set<CTxOut> &txouts, bool use_max_sig = false) const
    {
        std::vector<CTxOut> v_txouts(txouts.size());
        std::copy(txouts.begin(), txouts.end(), v_txouts.begin());
        return DummySignTx(txNew, v_txouts, use_max_sig);
    }
    bool DummySignTx(CMutableTransaction &txNew, const std::vector<CTxOut> &txouts, bool use_max_sig = false) const;
    bool DummySignInput(CTxIn &tx_in, const CTxOut &txout, bool use_max_sig = false) const;

    bool ImportScripts(const std::set<CScript> scripts, int64_t timestamp) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool ImportPrivKeys(const std::map<CKeyID, CKey>& privkey_map, const int64_t timestamp) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool ImportPubKeys(const std::vector<CKeyID>& ordered_pubkeys, const std::map<CKeyID, CPubKey>& pubkey_map, const std::map<CKeyID, std::pair<CPubKey, KeyOriginInfo>>& key_origins, const bool add_keypool, const bool internal, const int64_t timestamp) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool ImportScriptPubKeys(const std::string& label, const std::set<CScript>& script_pub_keys, const bool have_solving_data, const bool apply_label, const int64_t timestamp) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

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

    size_t KeypoolCountExternalKeys() const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool TopUpKeyPool(unsigned int kpSize = 0);

    int64_t GetOldestKeyPoolTime() const;

    std::set<std::set<CTxDestination>> GetAddressGroupings() const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    std::map<CTxDestination, CAmount> GetAddressBalances() const;

    std::set<CTxDestination> GetLabelAddresses(const std::string& label) const;

    /**
     * Marks all outputs in each one of the destinations dirty, so their cache is
     * reset and does not return outdated information.
     */
    void MarkDestinationsDirty(const std::set<CTxDestination>& destinations) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    bool GetNewDestination(const OutputType type, const std::string label, CTxDestination& dest, std::string& error);
    bool GetNewChangeDestination(const OutputType type, CTxDestination& dest, std::string& error);

    isminetype IsMine(const CTxDestination& dest) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    isminetype IsMine(const CScript& script) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    isminetype IsMine(const CTxIn& txin) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    /**
     * Returns amount of debit if the input matches the
     * filter, otherwise returns 0
     */
    CAmount GetDebit(const CTxIn& txin, const isminefilter& filter) const;
    isminetype IsMine(const CTxOut& txout) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    CAmount GetCredit(const CTxOut& txout, const isminefilter& filter) const;
    bool IsChange(const CTxOut& txout) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool IsChange(const CScript& script) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    CAmount GetChange(const CTxOut& txout) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    bool IsMine(const CTransaction& tx) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    /** should probably be renamed to IsRelevantToMe */
    bool IsFromMe(const CTransaction& tx) const;
    CAmount GetDebit(const CTransaction& tx, const isminefilter& filter) const;
    /** Returns whether all of the inputs match the filter */
    bool IsAllFromMe(const CTransaction& tx, const isminefilter& filter) const;
    CAmount GetCredit(const CTransaction& tx, const isminefilter& filter) const;
    CAmount GetChange(const CTransaction& tx) const;
    void chainStateFlushed(const CBlockLocator& loc) override;

    DBErrors LoadWallet(bool& fFirstRunRet);
    // SYSCOIN
    void AutoLockMasternodeCollaterals();
    DBErrors ZapSelectTx(std::vector<uint256>& vHashIn, std::vector<uint256>& vHashOut) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    bool SetAddressBook(const CTxDestination& address, const std::string& strName, const std::string& purpose);

    bool DelAddressBook(const CTxDestination& address);

    unsigned int GetKeyPoolSize() const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    //! signify that a particular wallet feature is now used.
    void SetMinVersion(enum WalletFeature, WalletBatch* batch_in = nullptr) override;

    //! get the current wallet format (the oldest client version guaranteed to understand this wallet)
    int GetVersion() const { LOCK(cs_wallet); return nWalletVersion; }

    //! Get wallet transactions that conflict with given transaction (spend same outputs)
    std::set<uint256> GetConflicts(const uint256& txid) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    //! Check if a given transaction has any of its outputs spent by another transaction in the wallet
    bool HasWalletSpend(const uint256& txid) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    //! Flush wallet (bitdb flush)
    void Flush();

    //! Close wallet database
    void Close();

    /** Wallet is about to be unloaded */
    boost::signals2::signal<void ()> NotifyUnload;

    /**
     * Address book entry changed.
     * @note called with lock cs_wallet held.
     */
    boost::signals2::signal<void (CWallet *wallet, const CTxDestination
            &address, const std::string &label, bool isMine,
            const std::string &purpose,
            ChangeType status)> NotifyAddressBookChanged;

    /**
     * Wallet transaction added, removed or updated.
     * @note called with lock cs_wallet held.
     */
    boost::signals2::signal<void (CWallet *wallet, const uint256 &hashTx,
            ChangeType status)> NotifyTransactionChanged;

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

    /** Mark a transaction as replaced by another transaction (e.g., BIP 125). */
    bool MarkReplaced(const uint256& originalHash, const uint256& newHash);

    /* Initializes the wallet, returns a new CWallet instance or a null pointer in case of an error */
    static std::shared_ptr<CWallet> Create(interfaces::Chain& chain, const std::string& name, std::unique_ptr<WalletDatabase> database, uint64_t wallet_creation_flags, bilingual_str& error, std::vector<bilingual_str>& warnings);

    /**
     * Wallet post-init setup
     * Gives the wallet a chance to register repetitive tasks and complete post-init tasks
     */
    void postInitProcess();
    // SYSCOIN
    bool GetBudgetSystemCollateralTX(CTransactionRef &tx, uint256 hash, CAmount amount, const COutPoint& outpoint=COutPoint()/*defaults null*/);
    bool BackupWallet(const std::string& strDest) const;

    /* Returns true if HD is enabled */
    bool IsHDEnabled() const;

    /* Returns true if the wallet can give out new addresses. This means it has keys in the keypool or can generate new keys */
    bool CanGetAddresses(bool internal = false) const;
    // SYSCOIN
    /** Load a CGovernanceObject into m_gobjects. */
    bool LoadGovernanceObject(const CGovernanceObject& obj) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    /** Store a CGovernanceObject in the wallet database. This should only be used by governance objects that are created by this wallet via `gobject prepare`. */
    bool WriteGovernanceObject(const CGovernanceObject& obj) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
    /** Returns a vector containing pointers to the governance objects in m_gobjects */
    std::vector<const CGovernanceObject*> GetGovernanceObjects() EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

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
       returns false if unknown, non-tolerable flags are present */
    bool AddWalletFlags(uint64_t flags);
    /** Loads the flags into the wallet. (used by LoadWallet) */
    bool LoadWalletFlags(uint64_t flags);

    /** Determine if we are a legacy wallet */
    bool IsLegacy() const;

    /** Returns a bracketed wallet name for displaying in logs, will return [default wallet] if the wallet has no name */
    const std::string GetDisplayName() const override {
        std::string wallet_name = GetName().length() == 0 ? "default wallet" : GetName();
        return strprintf("[%s]", wallet_name);
    };

    /** Prepends the wallet name in logging output to ease debugging in multi-wallet use cases */
    template<typename... Params>
    void WalletLogPrintf(std::string fmt, Params... parameters) const {
        LogPrintf(("%s " + fmt).c_str(), GetDisplayName(), parameters...);
    };

    /** Upgrade the wallet */
    bool UpgradeWallet(int version, bilingual_str& error);

    //! Returns all unique ScriptPubKeyMans in m_internal_spk_managers and m_external_spk_managers
    std::set<ScriptPubKeyMan*> GetActiveScriptPubKeyMans() const;

    //! Returns all unique ScriptPubKeyMans
    std::set<ScriptPubKeyMan*> GetAllScriptPubKeyMans() const;

    //! Get the ScriptPubKeyMan for the given OutputType and internal/external chain.
    ScriptPubKeyMan* GetScriptPubKeyMan(const OutputType& type, bool internal) const;

    //! Get the ScriptPubKeyMan for a script
    ScriptPubKeyMan* GetScriptPubKeyMan(const CScript& script) const;
    //! Get the ScriptPubKeyMan by id
    ScriptPubKeyMan* GetScriptPubKeyMan(const uint256& id) const;

    //! Get all of the ScriptPubKeyMans for a script given additional information in sigdata (populated by e.g. a psbt)
    std::set<ScriptPubKeyMan*> GetScriptPubKeyMans(const CScript& script, SignatureData& sigdata) const;

    //! Get the SigningProvider for a script
    std::unique_ptr<SigningProvider> GetSolvingProvider(const CScript& script) const;
    std::unique_ptr<SigningProvider> GetSolvingProvider(const CScript& script, SignatureData& sigdata) const;

    //! Get the LegacyScriptPubKeyMan which is used for all types, internal, and external.
    LegacyScriptPubKeyMan* GetLegacyScriptPubKeyMan() const;
    LegacyScriptPubKeyMan* GetOrCreateLegacyScriptPubKeyMan();

    //! Make a LegacyScriptPubKeyMan and set it for all types, internal, and external.
    void SetupLegacyScriptPubKeyMan();

    const CKeyingMaterial& GetEncryptionKey() const override;
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
    void LoadDescriptorScriptPubKeyMan(uint256 id, WalletDescriptor& desc);

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

    //! Create new DescriptorScriptPubKeyMans and add them to the wallet
    void SetupDescriptorScriptPubKeyMans() EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    //! Return the DescriptorScriptPubKeyMan for a WalletDescriptor if it is already in the wallet
    DescriptorScriptPubKeyMan* GetDescriptorScriptPubKeyMan(const WalletDescriptor& desc) const;

    //! Add a descriptor to the wallet, return a ScriptPubKeyMan & associated output type
    ScriptPubKeyMan* AddWalletDescriptor(WalletDescriptor& desc, const FlatSigningProvider& signing_provider, const std::string& label, bool internal);
};

/**
 * Called periodically by the schedule thread. Prompts individual wallets to resend
 * their transactions. Actual rebroadcast schedule is managed by the wallets themselves.
 */
void MaybeResendWalletTxs();

/** RAII object to check and reserve a wallet rescan */
class WalletRescanReserver
{
private:
    CWallet& m_wallet;
    bool m_could_reserve;
public:
    explicit WalletRescanReserver(CWallet& w) : m_wallet(w), m_could_reserve(false) {}

    bool reserve()
    {
        assert(!m_could_reserve);
        if (m_wallet.fScanningWallet.exchange(true)) {
            return false;
        }
        m_wallet.m_scanning_start = GetTimeMillis();
        m_wallet.m_scanning_progress = 0;
        m_could_reserve = true;
        return true;
    }

    bool isReserved() const
    {
        return (m_could_reserve && m_wallet.fScanningWallet);
    }

    ~WalletRescanReserver()
    {
        if (m_could_reserve) {
            m_wallet.fScanningWallet = false;
        }
    }
};

/** Calculate the size of the transaction assuming all signatures are max size
* Use DummySignatureCreator, which inserts 71 byte signatures everywhere.
* NOTE: this requires that all inputs must be in mapWallet (eg the tx should
* be IsAllFromMe). */
std::pair<int64_t, int64_t> CalculateMaximumSignedTxSize(const CTransaction &tx, const CWallet *wallet, bool use_max_sig = false) EXCLUSIVE_LOCKS_REQUIRED(wallet->cs_wallet);
std::pair<int64_t, int64_t> CalculateMaximumSignedTxSize(const CTransaction &tx, const CWallet *wallet, const std::vector<CTxOut>& txouts, bool use_max_sig = false);

//! Add wallet name to persistent configuration so it will be loaded on startup.
bool AddWalletSetting(interfaces::Chain& chain, const std::string& wallet_name);

//! Remove wallet name from persistent configuration so it will not be loaded on startup.
bool RemoveWalletSetting(interfaces::Chain& chain, const std::string& wallet_name);
// SYSCOIN
void getAuxFee(const CAuxFeeDetails &auxFeeDetails, const CAmount& nAmount, CAmount& nValue);
bool AddAssetCommitment(CMutableTransaction& mtx, const CAssetCoinInfo &assetInfo, const uint32_t &nChangePosIn);
#endif // SYSCOIN_WALLET_WALLET_H
