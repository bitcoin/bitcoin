// Copyright (c) 2018-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_WALLET_H
#define BITCOIN_INTERFACES_WALLET_H

#include <consensus/amount.h>          // For CAmount
#include <fs.h>
#include <interfaces/chain.h>          // For ChainClient
#include <pubkey.h>                    // For CKeyID and CScriptID (definitions needed in CTxDestination instantiation)
#include <script/standard.h>           // For CTxDestination
#include <support/allocators/secure.h> // For SecureString
#include <util/message.h>
#include <util/result.h>
#include <util/ui_change_type.h>

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <psbt.h>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

class CFeeRate;
class CKey;
class CRPCCommand;
enum class FeeReason;
enum class TransactionError;
struct bilingual_str;
struct PartiallySignedTransaction;
namespace node {
struct NodeContext;
} // namespace node
namespace wallet {
class CCoinControl;
class CWallet;
enum isminetype : unsigned int;
struct CRecipient;
struct WalletContext;
using isminefilter = std::underlying_type<isminetype>::type;
} // namespace wallet

class UniValue;

template <typename T>
class Span;

namespace interfaces {

class Handler;
struct WalletAddress;
struct WalletBalances;
struct WalletTx;
struct WalletTxOut;
struct WalletTxStatus;
namespace CoinJoin {
class Loader;
}

using WalletOrderForm = std::vector<std::pair<std::string, std::string>>;
using WalletValueMap = std::map<std::string, std::string>;

//! Interface for accessing a wallet.
class Wallet
{
public:
    virtual ~Wallet() {}

    //! Mark wallet as dirty
    virtual void markDirty() = 0;

    //! Encrypt wallet.
    virtual bool encryptWallet(const SecureString& wallet_passphrase) = 0;

    //! Return whether wallet is encrypted.
    virtual bool isCrypted() = 0;

    //! Lock wallet.
    virtual bool lock(bool fAllowMixing = false) = 0;

    //! Unlock wallet.
    virtual bool unlock(const SecureString& wallet_passphrase, bool fAllowMixing = false) = 0;

    //! Return whether wallet is locked.
    virtual bool isLocked(bool fForMixing = false) = 0;

    //! Change wallet passphrase.
    virtual bool changeWalletPassphrase(const SecureString& old_wallet_passphrase,
        const SecureString& new_wallet_passphrase) = 0;

    //! Abort a rescan.
    virtual void abortRescan() = 0;

    //! Lock masternode collaterals
    virtual void autoLockMasternodeCollaterals() = 0;

    //! Back up wallet.
    virtual bool backupWallet(const std::string& filename) = 0;

    //! Automatically backup up wallet.
    virtual bool autoBackupWallet(const fs::path& wallet_path, bilingual_str& error_string, std::vector<bilingual_str>& warnings) = 0;

    //! Get the number of keys since the last auto backup
    virtual int64_t getKeysLeftSinceAutoBackup() = 0;

    //! Get wallet name.
    virtual std::string getWalletName() = 0;

    // Get a new address.
    virtual bool getNewDestination(const std::string label, CTxDestination& dest) = 0;

    //! Get public key.
    virtual bool getPubKey(const CScript& script, const CKeyID& address, CPubKey& pub_key) = 0;

    //! Sign message
    virtual SigningResult signMessage(const std::string& message, const PKHash& pkhash, std::string& str_sig) = 0;

    //! Return whether wallet has private key.
    virtual bool isSpendable(const CScript& script) = 0;
    virtual bool isSpendable(const CTxDestination& dest) = 0;

    //! Return whether wallet has watch only keys.
    virtual bool haveWatchOnly() = 0;

    //! Add or update address.
    virtual bool setAddressBook(const CTxDestination& dest, const std::string& name, const std::string& purpose) = 0;

    // Remove address.
    virtual bool delAddressBook(const CTxDestination& dest) = 0;

    //! Look up address in wallet, return whether exists.
    virtual bool getAddress(const CTxDestination& dest,
        std::string* name,
        wallet::isminetype* is_mine,
        std::string* purpose) = 0;

    //! Get wallet address list.
    virtual std::vector<WalletAddress> getAddresses() const = 0;

    //! Get receive requests.
    virtual std::vector<std::string> getAddressReceiveRequests() = 0;

    //! Save or remove receive request.
    virtual bool setAddressReceiveRequest(const CTxDestination& dest, const std::string& id, const std::string& value) = 0;

    //! Lock coin.
    virtual bool lockCoin(const COutPoint& output, const bool write_to_db) = 0;

    //! Unlock coin.
    virtual bool unlockCoin(const COutPoint& output) = 0;

    //! Return whether coin is locked.
    virtual bool isLockedCoin(const COutPoint& output) = 0;

    //! List locked coins.
    virtual std::vector<COutPoint> listLockedCoins() = 0;

    //! List protx coins.
    virtual std::vector<COutPoint> listProTxCoins() = 0;

    //! Create transaction.
    virtual CTransactionRef createTransaction(const std::vector<wallet::CRecipient>& recipients,
        const wallet::CCoinControl& coin_control,
        bool sign,
        int& change_pos,
        CAmount& fee,
        bilingual_str& fail_reason) = 0;

    //! Commit transaction.
    virtual void commitTransaction(CTransactionRef tx,
        WalletValueMap value_map,
        WalletOrderForm order_form) = 0;

    //! Return whether transaction can be abandoned.
    virtual bool transactionCanBeAbandoned(const uint256& txid) = 0;

    //! Return whether transaction can be resent.
    virtual bool transactionCanBeResent(const uint256& txid) = 0;

    //! Abandon transaction.
    virtual bool abandonTransaction(const uint256& txid) = 0;

    //! Resend transaction.
    virtual bool resendTransaction(const uint256& txid) = 0;

    //! Get a transaction.
    virtual CTransactionRef getTx(const uint256& txid) = 0;

    //! Get transaction information.
    virtual WalletTx getWalletTx(const uint256& txid) = 0;

    //! Get list of all wallet transactions.
    virtual std::vector<WalletTx> getWalletTxs() = 0;

    //! Try to get updated status for a particular transaction, if possible without blocking.
    virtual bool tryGetTxStatus(const uint256& txid,
        WalletTxStatus& tx_status,
        int& num_blocks,
        int64_t& block_time) = 0;

    //! Get transaction details.
    virtual WalletTx getWalletTxDetails(const uint256& txid,
        WalletTxStatus& tx_status,
        WalletOrderForm& order_form,
        bool& in_mempool,
        int& num_blocks) = 0;

    // Get the number of coinjoin rounds an output went through
    virtual int getRealOutpointCoinJoinRounds(const COutPoint& outpoint) = 0;

    // Check if an outpoint is fully mixed
    virtual bool isFullyMixed(const COutPoint& outpoint) = 0;

    //! Fill PSBT.
    virtual TransactionError fillPSBT(int sighash_type,
        bool sign,
        bool bip32derivs,
        size_t* n_signed,
        PartiallySignedTransaction& psbtx,
        bool& complete) = 0;

    //! Get balances.
    virtual WalletBalances getBalances() = 0;

    //! Get balances if possible without blocking.
    virtual bool tryGetBalances(WalletBalances& balances, uint256& block_hash) = 0;

    //! Get balance.
    virtual CAmount getBalance() = 0;

    //! Get anonymizable balance.
    virtual CAmount getAnonymizableBalance(bool fSkipDenominated, bool fSkipUnconfirmed) = 0;

    //! Get normalized anonymized balance.
    virtual CAmount getNormalizedAnonymizedBalance() = 0;

    //! Get average anonymized rounds.
    virtual CAmount getAverageAnonymizedRounds() = 0;

    //! Get available balance.
    virtual CAmount getAvailableBalance(const wallet::CCoinControl& coin_control) = 0;

    //! Return whether transaction input belongs to wallet.
    virtual wallet::isminetype txinIsMine(const CTxIn& txin) = 0;

    //! Return whether transaction output belongs to wallet.
    virtual wallet::isminetype txoutIsMine(const CTxOut& txout) = 0;

    //! Return debit amount if transaction input belongs to wallet.
    virtual CAmount getDebit(const CTxIn& txin, wallet::isminefilter filter) = 0;

    //! Return credit amount if transaction input belongs to wallet.
    virtual CAmount getCredit(const CTxOut& txout, wallet::isminefilter filter) = 0;

    //! Return AvailableCoins + LockedCoins grouped by wallet address.
    //! (put change in one group with wallet address)
    using CoinsList = std::map<CTxDestination, std::vector<std::tuple<COutPoint, WalletTxOut>>>;
    virtual CoinsList listCoins() = 0;

    //! Return wallet transaction output information.
    virtual std::vector<WalletTxOut> getCoins(const std::vector<COutPoint>& outputs) = 0;

    //! Get required fee.
    virtual CAmount getRequiredFee(unsigned int tx_bytes) = 0;

    //! Get minimum fee.
    virtual CAmount getMinimumFee(unsigned int tx_bytes,
        const wallet::CCoinControl& coin_control,
        int* returned_target,
        FeeReason* reason) = 0;

    //! Get tx confirm target.
    virtual unsigned int getConfirmTarget() = 0;

    // Return whether HD enabled.
    virtual bool hdEnabled() = 0;

    // Return whether the wallet is blank.
    virtual bool canGetAddresses() = 0;

    // Return whether private keys enabled.
    virtual bool privateKeysDisabled() = 0;

    //! Get max tx fee.
    virtual CAmount getDefaultMaxTxFee() = 0;

    // Remove wallet.
    virtual void remove() = 0;

    //! Return whether is a legacy wallet
    virtual bool isLegacy() = 0;

    //! Register handler for unload message.
    using UnloadFn = std::function<void()>;
    virtual std::unique_ptr<Handler> handleUnload(UnloadFn fn) = 0;

    //! Register handler for show progress messages.
    using ShowProgressFn = std::function<void(const std::string& title, int progress)>;
    virtual std::unique_ptr<Handler> handleShowProgress(ShowProgressFn fn) = 0;

    //! Register handler for status changed messages.
    using StatusChangedFn = std::function<void()>;
    virtual std::unique_ptr<Handler> handleStatusChanged(StatusChangedFn fn) = 0;

    //! Register handler for address book changed messages.
    using AddressBookChangedFn = std::function<void(const CTxDestination& address,
        const std::string& label,
        bool is_mine,
        const std::string& purpose,
        ChangeType status)>;
    virtual std::unique_ptr<Handler> handleAddressBookChanged(AddressBookChangedFn fn) = 0;

    //! Register handler for transaction changed messages.
    using TransactionChangedFn = std::function<void(const uint256& txid, ChangeType status)>;
    virtual std::unique_ptr<Handler> handleTransactionChanged(TransactionChangedFn fn) = 0;

    //! Register handler for instantlock messages.
    using InstantLockReceivedFn = std::function<void()>;
    virtual std::unique_ptr<Handler> handleInstantLockReceived(InstantLockReceivedFn fn) = 0;

    //! Register handler for chainlock messages.
    using ChainLockReceivedFn = std::function<void(int chainLockHeight)>;
    virtual std::unique_ptr<Handler> handleChainLockReceived(ChainLockReceivedFn fn) = 0;

    //! Register handler for watchonly changed messages.
    using WatchOnlyChangedFn = std::function<void(bool have_watch_only)>;
    virtual std::unique_ptr<Handler> handleWatchOnlyChanged(WatchOnlyChangedFn fn) = 0;

    //! Register handler for keypool changed messages.
    using CanGetAddressesChangedFn = std::function<void()>;
    virtual std::unique_ptr<Handler> handleCanGetAddressesChanged(CanGetAddressesChangedFn fn) = 0;

    //! Return pointer to internal wallet class, useful for testing.
    virtual wallet::CWallet* wallet() { return nullptr; }
};

//! Wallet chain client that in addition to having chain client methods for
//! starting up, shutting down, and registering RPCs, also has additional
//! methods (called by the GUI) to load and create wallets.
class WalletLoader : public ChainClient
{
public:
   //! Register non-core wallet RPCs
   virtual void registerOtherRpcs(const Span<const CRPCCommand>& commands) = 0;

   //! Create new wallet.
   virtual std::unique_ptr<Wallet> createWallet(const std::string& name, const SecureString& passphrase, uint64_t wallet_creation_flags, bilingual_str& error, std::vector<bilingual_str>& warnings) = 0;

   //! Load existing wallet.
   virtual std::unique_ptr<Wallet> loadWallet(const std::string& name, bilingual_str& error, std::vector<bilingual_str>& warnings) = 0;

   //! Return default wallet directory.
   virtual std::string getWalletDir() = 0;

   //! Restore backup wallet
   virtual BResult<std::unique_ptr<Wallet>> restoreWallet(const fs::path& backup_file, const std::string& wallet_name, std::vector<bilingual_str>& warnings) = 0;

   //! Return available wallets in wallet directory.
   virtual std::vector<std::string> listWalletDir() = 0;

   //! Return interfaces for accessing wallets (if any).
   virtual std::vector<std::unique_ptr<Wallet>> getWallets() = 0;

   //! Register handler for load wallet messages. This callback is triggered by
   //! createWallet and loadWallet above, and also triggered when wallets are
   //! loaded at startup or by RPC.
   using LoadWalletFn = std::function<void(std::unique_ptr<Wallet> wallet)>;
   virtual std::unique_ptr<Handler> handleLoadWallet(LoadWalletFn fn) = 0;

   //! Return pointer to internal context, useful for testing.
   virtual wallet::WalletContext* context() { return nullptr; }
};

//! Information about one wallet address.
struct WalletAddress
{
    CTxDestination dest;
    wallet::isminetype is_mine;
    std::string name;
    std::string purpose;

    WalletAddress(CTxDestination dest, wallet::isminetype is_mine, std::string name, std::string purpose)
        : dest(std::move(dest)), is_mine(is_mine), name(std::move(name)), purpose(std::move(purpose))
    {
    }
};

//! Collection of wallet balances.
struct WalletBalances
{
    CAmount balance = 0;
    CAmount unconfirmed_balance = 0;
    CAmount immature_balance = 0;
    CAmount anonymized_balance = 0;
    bool have_watch_only = false;
    CAmount watch_only_balance = 0;
    CAmount unconfirmed_watch_only_balance = 0;
    CAmount immature_watch_only_balance = 0;
    CAmount denominated_untrusted_pending = 0;
    CAmount denominated_trusted = 0;

    bool balanceChanged(const WalletBalances& prev) const
    {
        return balance != prev.balance || unconfirmed_balance != prev.unconfirmed_balance ||  anonymized_balance != prev.anonymized_balance ||
               immature_balance != prev.immature_balance || watch_only_balance != prev.watch_only_balance ||
               unconfirmed_watch_only_balance != prev.unconfirmed_watch_only_balance ||
               immature_watch_only_balance != prev.immature_watch_only_balance;
    }
};

// Wallet transaction information.
struct WalletTx
{
    CTransactionRef tx;
    std::vector<wallet::isminetype> txin_is_mine;
    std::vector<wallet::isminetype> txout_is_mine;
    std::vector<CTxDestination> txout_address;
    std::vector<wallet::isminetype> txout_address_is_mine;
    CAmount credit;
    CAmount debit;
    CAmount change;
    int64_t time;
    std::map<std::string, std::string> value_map;
    bool is_coinbase;
    bool is_platform_transfer{false};
    bool is_denominate;
};

//! Updated transaction status.
struct WalletTxStatus
{
    int block_height;
    int blocks_to_maturity;
    int depth_in_main_chain;
    unsigned int time_received;
    uint32_t lock_time;
    bool is_trusted;
    bool is_abandoned;
    bool is_coinbase;
    bool is_in_main_chain;
    bool is_chainlocked;
    bool is_islocked;
};

//! Wallet transaction output.
struct WalletTxOut
{
    CTxOut txout;
    int64_t time;
    int depth_in_main_chain = -1;
    bool is_spent = false;
};

//! Return implementation of Wallet interface. This function is defined in
//! dummywallet.cpp and throws if the wallet component is not compiled.
std::unique_ptr<Wallet> MakeWallet(wallet::WalletContext& context, const std::shared_ptr<wallet::CWallet>& wallet);

//! Return implementation of ChainClient interface for a wallet loader. This
//! function will be undefined in builds where ENABLE_WALLET is false.
std::unique_ptr<WalletLoader> MakeWalletLoader(Chain& chain, ArgsManager& args, node::NodeContext& node_context,
                                               CoinJoin::Loader& coinjoin_loader);

} // namespace interfaces

#endif // BITCOIN_INTERFACES_WALLET_H
