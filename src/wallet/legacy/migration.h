#ifndef BITCOIN_WALLET_LEGACY_MIGRATION_H
#define BITCOIN_WALLET_LEGACY_MIGRATION_H

#include <wallet/wallet.h>

#include <wallet/legacy/spkm.h>

namespace wallet {
namespace legacy_migration {

class LegacyWallet : public CWallet
{
public:
    LegacyWallet(const std::string& name, std::unique_ptr<WalletDatabase> database) : CWallet::CWallet(/*chain=*/nullptr, name, std::move(database)) {}

    //! Get the LegacyDataSPKM used for all legacy output types and both internal and external chains.
    LegacyDataSPKM* GetLegacyDataSPKM() const;
    LegacyDataSPKM* GetOrCreateLegacyDataSPKM();

    //! Create a LegacyDataSPKM and set it for all legacy output types and both internal and external chains.
    void SetupLegacyDataSPKM();

    /** Move all records from the BDB database to a new SQLite database for storage.
     * The new database is built in a temporary directory and then moved into place, replacing the original one.
     * A backup is not created.
     * Returns false, with the original database untouched, if the new one can't be built. Once the original
     * database may have been erased or replaced, failures throw instead, so the caller restores the backup.
     */
    bool MigrateToSQLite(const std::unordered_set<std::string>& skip_types, bilingual_str& error, bool& had_legacy_records) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    //! Get all of the descriptors from a legacy wallet
    std::optional<MigrationData> GetDescriptorsForLegacy(bilingual_str& error) const EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);

    //! Adds the ScriptPubKeyMans from MigrationData to this wallet, removes the LegacyDataSPKM,
    //! and moves transaction and address book entries to watchonly_wallet or solvable_wallet as needed.
    util::Result<void> ApplyMigrationData(WalletBatch& local_wallet_batch, MigrationData& data) EXCLUSIVE_LOCKS_REQUIRED(cs_wallet);
};

struct MigrationResult {
    std::string wallet_name;
    std::optional<std::string> watchonly_wallet_name;
    std::optional<std::string> solvables_wallet_name;
    std::shared_ptr<CWallet> wallet;
    std::shared_ptr<CWallet> watchonly_wallet;
    std::shared_ptr<CWallet> solvables_wallet;
    fs::path backup_path;
};

//! Do all steps to migrate a legacy wallet to a descriptor wallet
[[nodiscard]] util::Result<MigrationResult> MigrateLegacyToDescriptor(const std::string& wallet_name, const SecureString& passphrase, WalletContext& context, bool load_wallet = true);
//! Requirement: The wallet provided to this function must be isolated, with no attachment to the node's context.
[[nodiscard]] util::Result<MigrationResult> MigrateLegacyToDescriptor(std::shared_ptr<CWallet> local_wallet, const SecureString& passphrase, WalletContext& context, bool load_wallet = true);

} // namespace legacy_migration
} // namespace wallet

#endif // BITCOIN_WALLET_LEGACY_MIGRATION_H
