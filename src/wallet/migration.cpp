#include <key_io.h>

#include <wallet/context.h>
#include <wallet/migration.h>
#include <wallet/wallet.h>
#include <wallet/walletutil.h>

namespace wallet {

LegacyDataSPKM* CWallet::GetLegacyDataSPKM() const
{
    if (IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS)) {
        return nullptr;
    }
    auto it = m_internal_spk_managers.find(OutputType::LEGACY);
    if (it == m_internal_spk_managers.end()) return nullptr;
    return dynamic_cast<LegacyDataSPKM*>(it->second);
}

bool CWallet::MigrateToSQLite(bilingual_str& error)
{
    AssertLockHeld(cs_wallet);

    WalletLogPrintf("Migrating wallet storage database from BerkeleyDB to SQLite.\n");

    if (m_database->Format() == "sqlite") {
        error = _("Error: This wallet already uses SQLite");
        return false;
    }

    // Get all of the records for DB type migration
    std::unique_ptr<DatabaseBatch> batch = m_database->MakeBatch();
    std::unique_ptr<DatabaseCursor> cursor = batch->GetNewCursor();
    std::vector<std::pair<SerializeData, SerializeData>> records;
    if (!cursor) {
        error = _("Error: Unable to begin reading all records in the database");
        return false;
    }
    DatabaseCursor::Status status = DatabaseCursor::Status::FAIL;
    while (true) {
        DataStream ss_key{};
        DataStream ss_value{};
        status = cursor->Next(ss_key, ss_value);
        if (status != DatabaseCursor::Status::MORE) {
            break;
        }
        SerializeData key(ss_key.begin(), ss_key.end());
        SerializeData value(ss_value.begin(), ss_value.end());
        records.emplace_back(key, value);
    }
    cursor.reset();
    batch.reset();
    if (status != DatabaseCursor::Status::DONE) {
        error = _("Error: Unable to read all records in the database");
        return false;
    }

    // Close this database and delete the file
    fs::path db_path = fs::PathFromString(m_database->Filename());
    m_database->Close();
    fs::remove(db_path);

    // Generate the path for the location of the migrated wallet
    // Wallets that are plain files rather than wallet directories will be migrated to be wallet directories.
    const fs::path wallet_path = fsbridge::AbsPathJoin(GetWalletDir(), fs::PathFromString(m_name));

    // Make new DB
    DatabaseOptions opts;
    opts.require_create = true;
    opts.require_format = DatabaseFormat::SQLITE;
    DatabaseStatus db_status;
    std::unique_ptr<WalletDatabase> new_db = MakeDatabase(wallet_path, opts, db_status, error);
    assert(new_db); // This is to prevent doing anything further with this wallet. The original file was deleted, but a backup exists.
    m_database.reset();
    m_database = std::move(new_db);

    // Write existing records into the new DB
    batch = m_database->MakeBatch();
    bool began = batch->TxnBegin();
    assert(began); // This is a critical error, the new db could not be written to. The original db exists as a backup, but we should not continue execution.
    for (const auto& [key, value] : records) {
        if (!batch->Write(std::span{key}, std::span{value})) {
            batch->TxnAbort();
            m_database->Close();
            fs::remove(m_database->Filename());
            assert(false); // This is a critical error, the new db could not be written to. The original db exists as a backup, but we should not continue execution.
        }
    }
    bool committed = batch->TxnCommit();
    assert(committed); // This is a critical error, the new db could not be written to. The original db exists as a backup, but we should not continue execution.
    return true;
}

std::optional<MigrationData> CWallet::GetDescriptorsForLegacy(bilingual_str& error) const
{
    AssertLockHeld(cs_wallet);

    LegacyDataSPKM* legacy_spkm = GetLegacyDataSPKM();
    if (!Assume(legacy_spkm)) {
        // This shouldn't happen
        error = Untranslated(STR_INTERNAL_BUG("Error: Legacy wallet data missing"));
        return std::nullopt;
    }

    std::optional<MigrationData> res = legacy_spkm->MigrateToDescriptor();
    if (res == std::nullopt) {
        error = _("Error: Unable to produce descriptors for this legacy wallet. Make sure to provide the wallet's passphrase if it is encrypted.");
        return std::nullopt;
    }
    return res;
}

util::Result<void> CWallet::ApplyMigrationData(WalletBatch& local_wallet_batch, MigrationData& data)
{
    AssertLockHeld(cs_wallet);

    LegacyDataSPKM* legacy_spkm = GetLegacyDataSPKM();
    if (!Assume(legacy_spkm)) {
        // This shouldn't happen
        return util::Error{Untranslated(STR_INTERNAL_BUG("Error: Legacy wallet data missing"))};
    }

    // Note: when the legacy wallet has no spendable scripts, it must be empty at the end of the process.
    bool has_spendable_material = !data.desc_spkms.empty() || data.master_key.key.IsValid();

    // Get all invalid or non-watched scripts that will not be migrated
    std::set<CTxDestination> not_migrated_dests;
    for (const auto& script : legacy_spkm->GetNotMineScriptPubKeys()) {
        CTxDestination dest;
        if (ExtractDestination(script, dest)) not_migrated_dests.emplace(dest);
    }

    // When the legacy wallet has no spendable scripts, the main wallet will be empty, leaving its script cache empty as well.
    // The watch-only and/or solvable wallet(s) will contain the scripts in their respective caches.
    if (!data.desc_spkms.empty()) Assume(!m_cached_spks.empty());
    if (!data.watch_descs.empty()) Assume(!data.watchonly_wallet->m_cached_spks.empty());
    if (!data.solvable_descs.empty()) Assume(!data.solvable_wallet->m_cached_spks.empty());

    for (auto& desc_spkm : data.desc_spkms) {
        if (m_spk_managers.contains(desc_spkm->GetID())) {
            return util::Error{_("Error: Duplicate descriptors created during migration. Your wallet may be corrupted.")};
        }
        uint256 id = desc_spkm->GetID();
        AddScriptPubKeyMan(id, std::move(desc_spkm));
    }

    // Remove the LegacyScriptPubKeyMan from disk
    if (!legacy_spkm->DeleteRecordsWithDB(local_wallet_batch)) {
        return util::Error{_("Error: cannot remove legacy wallet records")};
    }

    // Remove the LegacyScriptPubKeyMan from memory
    m_spk_managers.erase(legacy_spkm->GetID());
    m_external_spk_managers.clear();
    m_internal_spk_managers.clear();

    // Setup new descriptors (only if we are migrating any key material)
    SetWalletFlagWithDB(local_wallet_batch, WALLET_FLAG_DESCRIPTORS | WALLET_FLAG_LAST_HARDENED_XPUB_CACHED);
    if (has_spendable_material && !IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS)) {
        // Use the existing master key if we have it
        if (data.master_key.key.IsValid()) {
            SetupDescriptorScriptPubKeyMans(local_wallet_batch, data.master_key);
        } else {
            // Setup with a new seed if we don't.
            SetupOwnDescriptorScriptPubKeyMans(local_wallet_batch);
        }
    }

    // Get best block locator so that we can copy it to the watchonly and solvables
    CBlockLocator best_block_locator;
    if (!local_wallet_batch.ReadBestBlock(best_block_locator)) {
        return util::Error{_("Error: Unable to read wallet's best block locator record")};
    }

    // Update m_txos to match the descriptors remaining in this wallet
    m_txos.clear();
    RefreshAllTXOs();

    // Check if the transactions in the wallet are still ours. Either they belong here, or they belong in the watchonly wallet.
    // We need to go through these in the tx insertion order so that lookups to spends works.
    std::vector<Txid> txids_to_delete;
    std::unique_ptr<WalletBatch> watchonly_batch;
    if (data.watchonly_wallet) {
        watchonly_batch = std::make_unique<WalletBatch>(data.watchonly_wallet->GetDatabase());
        if (!watchonly_batch->TxnBegin()) return util::Error{strprintf(_("Error: database transaction cannot be executed for wallet %s"), data.watchonly_wallet->GetName())};
        // Copy the next tx order pos to the watchonly wallet
        LOCK(data.watchonly_wallet->cs_wallet);
        data.watchonly_wallet->nOrderPosNext = nOrderPosNext;
        watchonly_batch->WriteOrderPosNext(data.watchonly_wallet->nOrderPosNext);
        // Write the best block locator to avoid rescanning on reload
        if (!watchonly_batch->WriteBestBlock(best_block_locator)) {
            return util::Error{_("Error: Unable to write watchonly wallet best block locator record")};
        }
    }
    std::unique_ptr<WalletBatch> solvables_batch;
    if (data.solvable_wallet) {
        solvables_batch = std::make_unique<WalletBatch>(data.solvable_wallet->GetDatabase());
        if (!solvables_batch->TxnBegin()) return util::Error{strprintf(_("Error: database transaction cannot be executed for wallet %s"), data.solvable_wallet->GetName())};
        // Write the best block locator to avoid rescanning on reload
        if (!solvables_batch->WriteBestBlock(best_block_locator)) {
            return util::Error{_("Error: Unable to write solvable wallet best block locator record")};
        }
    }
    for (const auto& [_pos, wtx] : wtxOrdered) {
        // Check it is the watchonly wallet's
        // solvable_wallet doesn't need to be checked because transactions for those scripts weren't being watched for
        bool is_mine = IsMine(*wtx->tx) || IsFromMe(*wtx->tx);
        if (data.watchonly_wallet) {
            LOCK(data.watchonly_wallet->cs_wallet);
            if (data.watchonly_wallet->IsMine(*wtx->tx) || data.watchonly_wallet->IsFromMe(*wtx->tx)) {
                // Add to watchonly wallet
                const Txid& hash = wtx->GetHash();
                const CWalletTx& to_copy_wtx = *wtx;
                if (!data.watchonly_wallet->LoadToWallet(hash, [&](CWalletTx& ins_wtx, bool new_tx) EXCLUSIVE_LOCKS_REQUIRED(data.watchonly_wallet->cs_wallet) {
                    if (!new_tx) return false;
                    ins_wtx.SetTx(to_copy_wtx.tx);
                    ins_wtx.CopyFrom(to_copy_wtx);
                    return true;
                })) {
                    return util::Error{strprintf(_("Error: Could not add watchonly tx %s to watchonly wallet"), wtx->GetHash().GetHex())};
                }
                watchonly_batch->WriteTx(data.watchonly_wallet->mapWallet.at(hash));
                // Mark as to remove from the migrated wallet only if it does not also belong to it
                if (!is_mine) {
                    txids_to_delete.push_back(hash);
                    continue;
                }
            }
        }
        if (!is_mine) {
            // Both not ours and not in the watchonly wallet
            return util::Error{strprintf(_("Error: Transaction %s in wallet cannot be identified to belong to migrated wallets"), wtx->GetHash().GetHex())};
        }
        // Rewrite the transaction so that anything that may have changed about it in memory also persists to disk
        local_wallet_batch.WriteTx(*wtx);
    }

    // Do the removes
    if (txids_to_delete.size() > 0) {
        if (auto res = RemoveTxs(local_wallet_batch, txids_to_delete); !res) {
            return util::Error{_("Error: Could not delete watchonly transactions. ") + util::ErrorString(res)};
        }
    }

    // Pair external wallets with their corresponding db handler
    std::vector<std::pair<std::shared_ptr<CWallet>, std::unique_ptr<WalletBatch>>> wallets_vec;
    if (data.watchonly_wallet) wallets_vec.emplace_back(data.watchonly_wallet, std::move(watchonly_batch));
    if (data.solvable_wallet) wallets_vec.emplace_back(data.solvable_wallet, std::move(solvables_batch));

    // Write address book entry to disk
    auto func_store_addr = [](WalletBatch& batch, const CTxDestination& dest, const CAddressBookData& entry) {
        auto address{EncodeDestination(dest)};
        if (entry.purpose) batch.WritePurpose(address, PurposeToString(*entry.purpose));
        if (entry.label) batch.WriteName(address, *entry.label);
        for (const auto& [id, request] : entry.receive_requests) {
            batch.WriteAddressReceiveRequest(dest, id, request);
        }
        if (entry.previously_spent) batch.WriteAddressPreviouslySpent(dest, true);
    };

    // Check the address book data in the same way we did for transactions
    std::vector<CTxDestination> dests_to_delete;
    for (const auto& [dest, record] : m_address_book) {
        // Ensure "receive" entries that are no longer part of the original wallet are transferred to another wallet
        // Entries for everything else ("send") will be cloned to all wallets.
        bool require_transfer = record.purpose == AddressPurpose::RECEIVE && !IsMine(dest);
        bool copied = false;
        for (auto& [wallet, batch] : wallets_vec) {
            LOCK(wallet->cs_wallet);
            if (require_transfer && !wallet->IsMine(dest)) continue;

            // Copy the entire address book entry
            wallet->m_address_book[dest] = record;
            func_store_addr(*batch, dest, record);

            copied = true;
            // Only delete 'receive' records that are no longer part of the original wallet
            if (require_transfer) {
                dests_to_delete.push_back(dest);
                break;
            }
        }

        // Fail immediately if we ever found an entry that was ours and cannot be transferred
        // to any of the created wallets (watch-only, solvable).
        // Means that no inferred descriptor maps to the stored entry. Which mustn't happen.
        if (require_transfer && !copied) {

            // Skip invalid/non-watched scripts that will not be migrated
            if (not_migrated_dests.contains(dest)) {
                dests_to_delete.push_back(dest);
                continue;
            }

            return util::Error{_("Error: Address book data in wallet cannot be identified to belong to migrated wallets")};
        }
    }

    // Persist external wallets address book entries
    for (auto& [wallet, batch] : wallets_vec) {
        if (!batch->TxnCommit()) {
            return util::Error{strprintf(_("Error: Unable to write data to disk for wallet %s"), wallet->GetName())};
        }
    }

    // Remove the things to delete in this wallet
    if (dests_to_delete.size() > 0) {
        for (const auto& dest : dests_to_delete) {
            if (!DelAddressBookWithDB(local_wallet_batch, dest)) {
                return util::Error{_("Error: Unable to remove watchonly address book data")};
            }
        }
    }

    // If there was no key material in the main wallet, there should be no records on it anymore.
    // This wallet will be discarded at the end of the process. Only wallets that contain the
    // migrated records will be presented to the user.
    if (!has_spendable_material) {
        if (!m_address_book.empty()) return util::Error{_("Error: Not all address book records were migrated")};
        if (!mapWallet.empty()) return util::Error{_("Error: Not all transaction records were migrated")};
    }

    return {}; // all good
}

// Returns wallet prefix for migration.
// Used to name the backup file and newly created wallets.
// E.g. a watch-only wallet is named "<prefix>_watchonly".
static std::string MigrationPrefixName(CWallet& wallet)
{
    const std::string& name{wallet.GetName()};
    return name.empty() ? "default_wallet" : name;
}

bool DoMigration(CWallet& wallet, WalletContext& context, bilingual_str& error, MigrationResult& res) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet)
{
    AssertLockHeld(wallet.cs_wallet);

    // Get all of the descriptors from the legacy wallet
    std::optional<MigrationData> data = wallet.GetDescriptorsForLegacy(error);
    if (data == std::nullopt) return false;

    // Create the watchonly and solvable wallets if necessary
    if (data->watch_descs.size() > 0 || data->solvable_descs.size() > 0) {
        DatabaseOptions options;
        options.require_existing = false;
        options.require_create = true;
        options.require_format = DatabaseFormat::SQLITE;

        WalletContext empty_context;
        empty_context.args = context.args;

        // Make the wallets
        options.create_flags = WALLET_FLAG_DISABLE_PRIVATE_KEYS | WALLET_FLAG_BLANK_WALLET | WALLET_FLAG_DESCRIPTORS;
        if (wallet.IsWalletFlagSet(WALLET_FLAG_AVOID_REUSE)) {
            options.create_flags |= WALLET_FLAG_AVOID_REUSE;
        }
        if (wallet.IsWalletFlagSet(WALLET_FLAG_KEY_ORIGIN_METADATA)) {
            options.create_flags |= WALLET_FLAG_KEY_ORIGIN_METADATA;
        }
        if (data->watch_descs.size() > 0) {
            wallet.WalletLogPrintf("Making a new watchonly wallet containing the watched scripts\n");

            DatabaseStatus status;
            std::vector<bilingual_str> warnings;
            std::string wallet_name = MigrationPrefixName(wallet) + "_watchonly";
            std::unique_ptr<WalletDatabase> database = MakeWalletDatabase(wallet_name, options, status, error);
            if (!database) {
                error = strprintf(_("Wallet file creation failed: %s"), error);
                return false;
            }

            data->watchonly_wallet = CWallet::CreateNew(empty_context, wallet_name, std::move(database), options.create_flags, error, warnings);
            if (!data->watchonly_wallet) {
                error = _("Error: Failed to create new watchonly wallet");
                return false;
            }
            res.watchonly_wallet = data->watchonly_wallet;
            LOCK(data->watchonly_wallet->cs_wallet);

            // Parse the descriptors and add them to the new wallet
            for (const auto& [desc_str, creation_time] : data->watch_descs) {
                // Parse the descriptor
                FlatSigningProvider keys;
                std::string parse_err;
                std::vector<std::unique_ptr<Descriptor>> descs = Parse(desc_str, keys, parse_err, /*require_checksum=*/ true);
                assert(descs.size() == 1); // It shouldn't be possible to have the LegacyScriptPubKeyMan make an invalid descriptor or a multipath descriptors
                assert(!descs.at(0)->IsRange()); // It shouldn't be possible to have LegacyScriptPubKeyMan make a ranged watchonly descriptor

                // Add to the wallet
                WalletDescriptor w_desc(std::move(descs.at(0)), creation_time, 0, 0, 0);
                if (auto spkm_res = data->watchonly_wallet->AddWalletDescriptor(w_desc, keys, "", false); !spkm_res) {
                    throw std::runtime_error(util::ErrorString(spkm_res).original);
                }
            }

            // Add the wallet to settings
            UpdateWalletSetting(*context.chain, wallet_name, /*load_on_startup=*/true, warnings);
        }
        if (data->solvable_descs.size() > 0) {
            wallet.WalletLogPrintf("Making a new watchonly wallet containing the unwatched solvable scripts\n");

            DatabaseStatus status;
            std::vector<bilingual_str> warnings;
            std::string wallet_name = MigrationPrefixName(wallet) + "_solvables";
            std::unique_ptr<WalletDatabase> database = MakeWalletDatabase(wallet_name, options, status, error);
            if (!database) {
                error = strprintf(_("Wallet file creation failed: %s"), error);
                return false;
            }

            data->solvable_wallet = CWallet::CreateNew(empty_context, wallet_name, std::move(database), options.create_flags, error, warnings);
            if (!data->solvable_wallet) {
                error = _("Error: Failed to create new watchonly wallet");
                return false;
            }
            res.solvables_wallet = data->solvable_wallet;
            LOCK(data->solvable_wallet->cs_wallet);

            // Parse the descriptors and add them to the new wallet
            for (const auto& [desc_str, creation_time] : data->solvable_descs) {
                // Parse the descriptor
                FlatSigningProvider keys;
                std::string parse_err;
                std::vector<std::unique_ptr<Descriptor>> descs = Parse(desc_str, keys, parse_err, /*require_checksum=*/ true);
                assert(descs.size() == 1); // It shouldn't be possible to have the LegacyScriptPubKeyMan make an invalid descriptor or a multipath descriptors
                assert(!descs.at(0)->IsRange()); // It shouldn't be possible to have LegacyScriptPubKeyMan make a ranged watchonly descriptor

                // Add to the wallet
                WalletDescriptor w_desc(std::move(descs.at(0)), creation_time, 0, 0, 0);
                if (auto spkm_res = data->solvable_wallet->AddWalletDescriptor(w_desc, keys, "", false); !spkm_res) {
                    throw std::runtime_error(util::ErrorString(spkm_res).original);
                }
            }

            // Add the wallet to settings
            UpdateWalletSetting(*context.chain, wallet_name, /*load_on_startup=*/true, warnings);
        }
    }

    // Add the descriptors to wallet, remove LegacyScriptPubKeyMan, and cleanup txs and address book data
    return RunWithinTxn(wallet.GetDatabase(), /*process_desc=*/"apply migration process", [&](WalletBatch& batch) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet){
        if (auto res_migration = wallet.ApplyMigrationData(batch, *data); !res_migration) {
            error = util::ErrorString(res_migration);
            return false;
        }
        wallet.WalletLogPrintf("Wallet migration complete.\n");
        return true;
    });
}

util::Result<MigrationResult> MigrateLegacyToDescriptor(const std::string& wallet_name, const SecureString& passphrase, WalletContext& context)
{
    std::vector<bilingual_str> warnings;
    bilingual_str error;

    // The only kind of wallets that could be loaded are descriptor ones, which don't need to be migrated.
    if (auto wallet = GetWallet(context, wallet_name)) {
        assert(wallet->IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS));
        return util::Error{_("Error: This wallet is already a descriptor wallet")};
    } else {
        // Check if the wallet is BDB
        const auto& wallet_path = GetWalletPath(wallet_name);
        if (!wallet_path) {
            return util::Error{util::ErrorString(wallet_path)};
        }
        if (!fs::exists(*wallet_path)) {
            return util::Error{_("Error: Wallet does not exist")};
        }
        if (!IsBDBFile(BDBDataFile(*wallet_path))) {
            return util::Error{_("Error: This wallet is already a descriptor wallet")};
        }
    }

    // Load the wallet but only in the context of this function.
    // No signals should be connected nor should anything else be aware of this wallet
    WalletContext empty_context;
    empty_context.args = context.args;
    DatabaseOptions options;
    options.require_existing = true;
    options.require_format = DatabaseFormat::BERKELEY_RO;
    DatabaseStatus status;
    std::unique_ptr<WalletDatabase> database = MakeWalletDatabase(wallet_name, options, status, error);
    if (!database) {
        return util::Error{Untranslated("Wallet file verification failed.") + Untranslated(" ") + error};
    }

    // Make the local wallet
    std::shared_ptr<CWallet> local_wallet = CWallet::LoadExisting(empty_context, wallet_name, std::move(database), error, warnings);
    if (!local_wallet) {
        return util::Error{Untranslated("Wallet loading failed.") + Untranslated(" ") + error};
    }

    return MigrateLegacyToDescriptor(std::move(local_wallet), passphrase, context);
}

util::Result<MigrationResult> MigrateLegacyToDescriptor(std::shared_ptr<CWallet> local_wallet, const SecureString& passphrase, WalletContext& context)
{
    MigrationResult res;
    bilingual_str error;
    std::vector<bilingual_str> warnings;

    DatabaseOptions options;
    options.require_existing = true;
    DatabaseStatus status;

    const std::string wallet_name = local_wallet->GetName();

    // Before anything else, check if there is something to migrate.
    if (local_wallet->IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS)) {
        return util::Error{_("Error: This wallet is already a descriptor wallet")};
    }

    // Make a backup of the DB in the wallet's directory with a unique filename
    // using the wallet name and current timestamp. The backup filename is based
    // on the name of the parent directory containing the wallet data in most
    // cases, but in the case where the wallet name is a path to a data file,
    // the name of the data file is used, and in the case where the wallet name
    // is blank, "default_wallet" is used.
    const std::string backup_prefix = wallet_name.empty() ? MigrationPrefixName(*local_wallet) : [&] {
        // fs::weakly_canonical resolves relative specifiers and remove trailing slashes.
        const auto legacy_wallet_path = fs::weakly_canonical(GetWalletDir() / fs::PathFromString(wallet_name));
        return fs::PathToString(legacy_wallet_path.filename());
    }();

    fs::path backup_filename = fs::PathFromString(strprintf("%s_%d.legacy.bak", backup_prefix, GetTime()));
    fs::path backup_path = fsbridge::AbsPathJoin(GetWalletDir(), backup_filename);
    if (!local_wallet->BackupWallet(fs::PathToString(backup_path))) {
        return util::Error{_("Error: Unable to make a backup of your wallet")};
    }
    res.backup_path = backup_path;

    bool success = false;

    // Unlock the wallet if needed
    if (local_wallet->IsLocked() && !local_wallet->Unlock(passphrase)) {
        if (passphrase.find('\0') == std::string::npos) {
            return util::Error{Untranslated("Error: Wallet decryption failed, the wallet passphrase was not provided or was incorrect.")};
        } else {
            return util::Error{Untranslated("Error: Wallet decryption failed, the wallet passphrase entered was incorrect. "
                                            "The passphrase contains a null character (ie - a zero byte). "
                                            "If this passphrase was set with a version of this software prior to 25.0, "
                                            "please try again with only the characters up to — but not including — "
                                            "the first null character.")};
        }
    }

    // Indicates whether the current wallet is empty after migration.
    // Notes:
    // When non-empty: the local wallet becomes the main spendable wallet.
    // When empty: The local wallet is excluded from the result, as the
    //             user does not expect an empty spendable wallet after
    //             migrating only watch-only scripts.
    bool empty_local_wallet = false;

    {
        LOCK(local_wallet->cs_wallet);
        // First change to using SQLite
        if (!local_wallet->MigrateToSQLite(error)) return util::Error{error};

        // Do the migration of keys and scripts for non-empty wallets, and cleanup if it fails
        if (HasLegacyRecords(*local_wallet)) {
            success = DoMigration(*local_wallet, context, error, res);
            // No scripts mean empty wallet after migration
            empty_local_wallet = local_wallet->GetAllScriptPubKeyMans().empty();
        } else {
            // Make sure that descriptors flag is actually set
            local_wallet->SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
            success = true;
        }
    }

    // In case of loading failure, we need to remember the wallet files we have created to remove.
    // A `set` is used as it may be populated with the same wallet directory paths multiple times,
    // both before and after loading. This ensures the set is complete even if one of the wallets
    // fails to load.
    std::set<fs::path> wallet_files_to_remove;
    std::set<fs::path> wallet_empty_dirs_to_remove;

    // Helper to track wallet files and directories for cleanup on failure.
    // Only directories of wallets created during migration (not the main wallet) are tracked.
    auto track_for_cleanup = [&](const CWallet& wallet) {
        const auto files = wallet.GetDatabase().Files();
        wallet_files_to_remove.insert(files.begin(), files.end());
        if (wallet.GetName() != wallet_name) {
            // If this isn’t the main wallet, mark its directory for removal.
            // This applies to the watch-only and solvable wallets.
            // Wallets stored directly as files in the top-level directory
            // (e.g. default unnamed wallets) don’t have a removable parent directory.
            wallet_empty_dirs_to_remove.insert(fs::PathFromString(wallet.GetDatabase().Filename()).parent_path());
        }
    };


    if (success) {
        Assume(!res.wallet); // We will set it here.
        // Check if the local wallet is empty after migration
        if (empty_local_wallet) {
            // This wallet has no records. We can safely remove it.
            std::vector<fs::path> paths_to_remove = local_wallet->GetDatabase().Files();
            local_wallet.reset();
            for (const auto& path_to_remove : paths_to_remove) fs::remove(path_to_remove);
        }

        LogInfo("Loading new wallets after migration...\n");
        // Migration successful, load all the migrated wallets.
        for (std::shared_ptr<CWallet>* wallet_ptr : {&local_wallet, &res.watchonly_wallet, &res.solvables_wallet}) {
            if (success && *wallet_ptr) {
                std::shared_ptr<CWallet>& wallet = *wallet_ptr;
                // Track db path and load wallet
                track_for_cleanup(*wallet);
                assert(wallet.use_count() == 1);
                std::string wallet_name = wallet->GetName();
                wallet.reset();
                wallet = LoadWallet(context, wallet_name, /*load_on_start=*/std::nullopt, options, status, error, warnings);
                if (!wallet) {
                    LogError("Failed to load wallet '%s' after migration. Rolling back migration to preserve consistency. "
                             "Error cause: %s\n", wallet_name, error.original);
                    success = false;
                    break;
                }

                // Set the first successfully loaded wallet as the main one.
                // The loop order is intentional and must always start with the local wallet.
                if (!res.wallet) {
                    res.wallet_name = wallet->GetName();
                    res.wallet = std::move(wallet);
                }
            }
        }
    }
    if (!success) {
        // Make list of wallets to cleanup
        std::vector<std::shared_ptr<CWallet>> created_wallets;
        if (local_wallet) created_wallets.push_back(std::move(local_wallet));
        if (res.watchonly_wallet) created_wallets.push_back(std::move(res.watchonly_wallet));
        if (res.solvables_wallet) created_wallets.push_back(std::move(res.solvables_wallet));

        // Get the directories to remove after unloading
        for (std::shared_ptr<CWallet>& wallet : created_wallets) {
            track_for_cleanup(*wallet);
        }

        // Unload the wallets
        for (std::shared_ptr<CWallet>& w : created_wallets) {
            if (w->HaveChain()) {
                // Unloading for wallets that were loaded for normal use
                if (!RemoveWallet(context, w, /*load_on_start=*/false)) {
                    error += _("\nUnable to cleanup failed migration");
                    return util::Error{error};
                }
                WaitForDeleteWallet(std::move(w));
            } else {
                // Unloading for wallets in local context
                assert(w.use_count() == 1);
                w.reset();
            }
        }

        // First, delete the db files we have created throughout this process and nothing else
        for (const fs::path& file : wallet_files_to_remove) {
            fs::remove(file);
        }

        // Second, delete the created wallet directories and nothing else. They must be empty at this point.
        for (const fs::path& dir : wallet_empty_dirs_to_remove) {
            Assume(fs::is_empty(dir));
            fs::remove(dir);
        }

        // Restore the backup
        // Convert the backup file to the wallet db file by renaming it and moving it into the wallet's directory.
        bilingual_str restore_error;
        const auto& ptr_wallet = RestoreWallet(context, backup_path, wallet_name, /*load_on_start=*/std::nullopt, status, restore_error, warnings, /*load_after_restore=*/false, /*allow_unnamed=*/true);
        if (!restore_error.empty()) {
            error += restore_error + _("\nUnable to restore backup of wallet.");
            return util::Error{error};
        }
        // Verify that the legacy wallet is not loaded after restoring from the backup.
        assert(!ptr_wallet);

        return util::Error{error};
    }
    return res;
}

} // namespace wallet
