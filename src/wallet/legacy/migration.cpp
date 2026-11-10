#include <key_io.h>

#include <wallet/context.h>
#include <wallet/legacy/migration.h>
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

LegacyDataSPKM* CWallet::GetOrCreateLegacyDataSPKM()
{
    SetupLegacyDataSPKM();
    return GetLegacyDataSPKM();
}

void CWallet::SetupLegacyDataSPKM()
{
    if (!m_internal_spk_managers.empty() || !m_external_spk_managers.empty() || !m_spk_managers.empty() || IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS)) {
        return;
    }

    Assert(m_database->Format() == "bdb_ro" || m_database->Format() == "sqlite-mock");
    std::unique_ptr<ScriptPubKeyMan> spk_manager = std::make_unique<LegacyDataSPKM>(*this);

    for (const auto& type : LEGACY_OUTPUT_TYPES) {
        m_internal_spk_managers[type] = spk_manager.get();
        m_external_spk_managers[type] = spk_manager.get();
    }
    uint256 id = spk_manager->GetID();
    AddScriptPubKeyMan(id, std::move(spk_manager));
}

// Returns wallet prefix for migration.
// Used to name the backup file and newly created wallets.
// E.g. a watch-only wallet is named "<prefix>_watchonly".
static std::string MigrationPrefixName(CWallet& wallet)
{
    const std::string& name{wallet.GetName()};
    return name.empty() ? "default_wallet" : name;
}

// Returns prefix for files created in the wallets directory during migration:
// the backup file and the temporary database. Unlike MigrationPrefixName(),
// this is a single path component, because the wallet name can be a path.
static std::string MigrationFilePrefix(CWallet& wallet)
{
    const std::string& name{wallet.GetName()};
    if (name.empty()) return MigrationPrefixName(wallet);
    // fs::weakly_canonical resolves relative specifiers and removes trailing slashes.
    const fs::path legacy_wallet_path{fs::weakly_canonical(GetWalletDir() / fs::PathFromString(name))};
    return fs::PathToString(legacy_wallet_path.filename());
}

bool CWallet::MigrateToSQLite(const std::unordered_set<std::string>& skip_types, bilingual_str& error, bool& had_legacy_records)
{
    AssertLockHeld(cs_wallet);

    WalletLogPrintf("Migrating wallet storage database from BerkeleyDB to SQLite.");

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

        std::string type;
        ss_key >> type;
        if (skip_types.contains(type)) {
            had_legacy_records = true;
            continue;
        }

        records.emplace_back(key, value);
    }
    cursor.reset();
    batch.reset();
    if (status != DatabaseCursor::Status::DONE) {
        error = _("Error: Unable to read all records in the database");
        return false;
    }

    // Close this database. It stays untouched until the new one is fully built
    const fs::path db_file = fsbridge::AbsPathJoin(GetWalletDir(), fs::PathFromString(m_database->Filename()));
    m_database->Close();

    // Create a temporary database next to the original db file
    const std::string new_db_name = strprintf("%s_sqlite_%d.tmp", MigrationFilePrefix(*this), GetTime());
    const fs::path tmp_wallet_path = fs::path{db_file.parent_path()} / fs::PathFromString(new_db_name);
    // Refuse any existing path, not just an existing database (all that require_create checks), so cleanup only
    // deletes files this function created.
    if (std::error_code ec; fs::exists(fs::symlink_status(tmp_wallet_path, ec))) {
        error = strprintf(_("Error: Unable to create the new database for wallet '%s'. Temporary path '%s' already exists."), GetName(), fs::PathToString(tmp_wallet_path));
        return false;
    }

    // Generate the path for the location of the migrated wallet
    // Wallets that are plain files rather than wallet directories will be migrated to be wallet directories.
    const fs::path dst_wallet_dir = fsbridge::AbsPathJoin(GetWalletDir(), fs::PathFromString(m_name));

    // Make new DB
    DatabaseOptions opts;
    opts.require_create = true;
    opts.require_format = DatabaseFormat::SQLITE;
    DatabaseStatus db_status;
    std::unique_ptr<WalletDatabase> new_db = MakeDatabase(tmp_wallet_path, opts, db_status, error);

    // Set the error, then delete the temporary database and its directory. The directory did not exist before, so it
    // only holds files of the new db, including any left by a failed creation.
    const auto remove_tmp_and_fail = [&](bilingual_str err) {
        error = std::move(err);
        new_db.reset(); // Close the db before deleting its files
        std::error_code ec;
        for (auto it{fs::directory_iterator(tmp_wallet_path, ec)}; !ec && it != fs::directory_iterator(); it.increment(ec)) {
            fs::remove(it->path(), ec);
        }
        fs::remove(tmp_wallet_path, ec);
        if (ec) {
            error += Untranslated(" ") + strprintf(_("The temporary database at %s could not be removed."), fs::PathToString(tmp_wallet_path));
        }
        return false;
    };

    if (!new_db) {
        return remove_tmp_and_fail(strprintf(_("Error: Unable to create the new database for wallet '%s'."), GetName()) + Untranslated(" ") + error);
    }

    // Write existing records into the new DB
    const bool written = RunWithinTxn(*new_db, "migration: write records to new db", [&records](DatabaseBatch& in_batch) {
        for (const auto& [key, value] : records) {
            if (!in_batch.Write(std::span{key}, std::span{value})) return false;
        }
        return true;
    });
    if (!written) {
        return remove_tmp_and_fail(strprintf(_("Error: Unable to write records into the new database for wallet '%s'"), GetName()));
    }

    // ######################################################################
    // At this point, the new database has all records.                     #
    // Move it into the wallet location, replacing the original db file.    #
    // ######################################################################

    // A plain wallet file (its db file is the wallet path itself) becomes a wallet directory. As a file cannot be
    // atomically replaced by a directory, the original db is erased first in that case. The original db is a single
    // file, so if Erase() fails nothing was removed, and returning false is still correct.
    if (db_file == dst_wallet_dir) {
        Assume(m_database->Files().size() == 1);
        if (auto res{m_database->Erase()}; !res) return remove_tmp_and_fail(util::ErrorString(res));
    }

    // From here on, the original db may have been erased or replaced. Never return false after this point, as the
    // caller would skip the restore. Throw instead, so it deletes the wallet files and restores the backup.
    if (auto res{MoveDatabase(new_db, dst_wallet_dir, opts)}; !res) {
        remove_tmp_and_fail(util::ErrorString(res));
        throw std::runtime_error(error.original);
    }
    // The caller's rollback deletes m_database's files, so point it at the new db as soon as it replaces the original
    m_database = std::move(new_db);
    // The temporary dir only held the new db files, which were just moved out. If it can't be removed, something
    // unexpected happened, so stop and let the caller restore the backup.
    if (std::error_code ec; !fs::remove(tmp_wallet_path, ec)) {
        throw std::runtime_error(strprintf("Unable to remove temporary directory %s after moving the new database: %s", fs::PathToString(tmp_wallet_path), ec ? ec.message() : "it no longer exists"));
    }
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

    // Remove the LegacyDataSPKM from memory
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

    // Set the last opened version and features
    if (!local_wallet_batch.WriteLastOpenedVersion()) {
        return util::Error{_("Error: Unable to write last opened version")};
    }
    if (!local_wallet_batch.WriteLastOpenedFeatures()) {
        return util::Error{_("Error: Unable to write last opened features")};
    }
    if (HasEncryptionKeys()) {
        if (!local_wallet_batch.WriteLastDecryptedFeatures()) {
            return util::Error{_("Error: Unable to write last decrypted features")};
        }
    }

    // Get best block locator so that we can copy it to the watchonly and solvables
    // Note: The best block locator was introduced in #152 so ancient wallets do not have it
    CBlockLocator best_block_locator;
    (void)local_wallet_batch.ReadBestBlock(best_block_locator);

    // Update m_txos to match the descriptors remaining in this wallet
    m_txos.clear();
    RefreshAllTXOs();

    // Check if the transactions in the wallet are still ours. Either they belong here, or they belong in the watchonly wallet.
    // We need to go through these in the tx insertion order so that lookups to spends works.
    std::unique_ptr<WalletBatch> watchonly_batch;
    if (data.watchonly_wallet) {
        watchonly_batch = std::make_unique<WalletBatch>(data.watchonly_wallet->GetDatabase());
        if (!watchonly_batch->TxnBegin()) return util::Error{strprintf(_("Error: database transaction cannot be executed for wallet %s"), data.watchonly_wallet->GetName())};
        // Copy the next tx order pos to the watchonly wallet
        LOCK(data.watchonly_wallet->cs_wallet);
        data.watchonly_wallet->nOrderPosNext = nOrderPosNext;
        watchonly_batch->WriteOrderPosNext(data.watchonly_wallet->nOrderPosNext);
        // Write the locator record. An empty locator is valid and triggers rescan on load.
        if (!watchonly_batch->WriteBestBlock(best_block_locator)) {
            return util::Error{_("Error: Unable to write watchonly wallet best block locator record")};
        }
    }
    std::unique_ptr<WalletBatch> solvables_batch;
    if (data.solvable_wallet) {
        solvables_batch = std::make_unique<WalletBatch>(data.solvable_wallet->GetDatabase());
        if (!solvables_batch->TxnBegin()) return util::Error{strprintf(_("Error: database transaction cannot be executed for wallet %s"), data.solvable_wallet->GetName())};
        // Write the locator record. An empty locator is valid and triggers rescan on load.
        if (!solvables_batch->WriteBestBlock(best_block_locator)) {
            return util::Error{_("Error: Unable to write solvable wallet best block locator record")};
        }
    }
    for (const auto& [_pos, wtx] : wtxOrdered) {
        // Check it is the watchonly wallet's
        // solvable_wallet doesn't need to be checked because transactions for those scripts weren't being watched for
        bool is_mine = IsMine(*wtx->GetTx()) || IsFromMe(*wtx->GetTx());
        if (data.watchonly_wallet) {
            LOCK(data.watchonly_wallet->cs_wallet);
            if (data.watchonly_wallet->IsMine(*wtx->GetTx()) || data.watchonly_wallet->IsFromMe(*wtx->GetTx())) {
                // Add to watchonly wallet
                const Txid& hash = wtx->GetHash();
                DataStream wtx_ser;
                wtx_ser << *wtx;
                CWalletTx copy_wtx(deserialize, wtx_ser, wtx->GetTxs());
                if (!data.watchonly_wallet->LoadToWallet(std::move(copy_wtx))) {
                    return util::Error{strprintf(_("Error: Could not add watchonly tx %s to watchonly wallet"), wtx->GetHash().GetHex())};
                }
                watchonly_batch->WriteFullTx(data.watchonly_wallet->mapWallet.at(hash));
                if (!is_mine) {
                    continue;
                }
            }
        }
        if (!is_mine) {
            // Both not ours and not in the watchonly wallet
            return util::Error{strprintf(_("Error: Transaction %s in wallet cannot be identified to belong to migrated wallets"), wtx->GetHash().GetHex())};
        }
        // Rewrite the transaction so that anything that may have changed about it in memory also persists to disk
        local_wallet_batch.WriteTxMetadata(*wtx);
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

namespace legacy_migration {

bool DoMigration(CWallet& wallet, WalletContext& context, bilingual_str& error, MigrationResult& res, const bool load_on_startup = true) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet)
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

        const auto fn_create_wallet = [&context, &options, &empty_context, load_on_startup](const std::string& wallet_name,
                                                                           const std::vector<std::pair<std::string, int64_t>>& descs_to_import,
                                                                           std::shared_ptr<CWallet>& out_wallet, bilingual_str& error) {
            try {
                DatabaseStatus status;
                std::vector<bilingual_str> warnings;
                std::unique_ptr<WalletDatabase> database = MakeWalletDatabase(wallet_name, options, status, error);
                if (!database) {
                    error = strprintf(_("Wallet file creation failed: %s"), error);
                    return false;
                }

                out_wallet = CWallet::CreateNew(empty_context, wallet_name, std::move(database), options.create_flags, /*born_encrypted=*/false, error, warnings);
                if (!out_wallet) {
                    error = strprintf(_("Error: Failed to create new wallet '%s'"), wallet_name);
                    return false;
                }
                LOCK(out_wallet->cs_wallet);

                // Parse the descriptors and add them to the new wallet
                for (const auto& [desc_str, creation_time] : descs_to_import) {
                    // Parse the descriptor
                    FlatSigningProvider keys;
                    std::string parse_err;
                    std::vector<std::unique_ptr<Descriptor>> descs = Parse(desc_str, keys, parse_err, /*require_checksum=*/ true);
                    // LegacyDataSPKM should not produce invalid, multipath, or ranged watch-only descriptors.
                    assert(descs.size() == 1);
                    assert(!descs.at(0)->IsRange());

                    // Add to the wallet
                    WalletDescriptor w_desc(std::move(descs.at(0)), creation_time, 0, 0, 0);
                    if (auto spkm_res = out_wallet->AddWalletDescriptor(w_desc, keys, "", false); !spkm_res) {
                        throw std::runtime_error(util::ErrorString(spkm_res).original);
                    }
                }

                // Add the wallet to settings
                UpdateWalletSetting(*context.chain, wallet_name, load_on_startup, warnings);
                return true;
            } catch (std::exception& e) {
                error = strprintf(_("Failed to create new wallet '%s'. Error: %s"), wallet_name, e.what());
                return false;
            }
        };

        if (data->watch_descs.size() > 0) {
            wallet.WalletLogPrintf("Making a new watchonly wallet containing the watched scripts");
            bool ret = fn_create_wallet(/*wallet_name=*/MigrationPrefixName(wallet) + "_watchonly", data->watch_descs, /*out_wallet=*/data->watchonly_wallet, error);
            res.watchonly_wallet = data->watchonly_wallet; // always set just so it can be cleaned up later
            if (!ret) return false;
        }
        if (data->solvable_descs.size() > 0) {
            wallet.WalletLogPrintf("Making a new watchonly wallet containing the unwatched solvable scripts");
            bool ret = fn_create_wallet(/*wallet_name=*/MigrationPrefixName(wallet) + "_solvables", data->solvable_descs, /*out_wallet=*/data->solvable_wallet, error);
            res.solvables_wallet = data->solvable_wallet;  // always set just so it can be cleaned up later
            if (!ret) return false;
        }
    }

    // Add the descriptors to the wallet, remove the LegacyDataSPKM, and clean up transactions and address book data
    return RunWithinTxn(wallet.GetDatabase(), /*process_desc=*/"apply migration process", [&](WalletBatch& batch) EXCLUSIVE_LOCKS_REQUIRED(wallet.cs_wallet){
        if (auto res_migration = wallet.ApplyMigrationData(batch, *data); !res_migration) {
            error = util::ErrorString(res_migration);
            return false;
        }
        wallet.WalletLogPrintf("Wallet migration complete.");
        return true;
    });
}

util::Result<MigrationResult> MigrateLegacyToDescriptor(const std::string& wallet_name, const SecureString& passphrase, WalletContext& context, bool load_wallet)
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

    return MigrateLegacyToDescriptor(std::move(local_wallet), passphrase, context, load_wallet);
}

util::Result<MigrationResult> MigrateLegacyToDescriptor(std::shared_ptr<CWallet> local_wallet, const SecureString& passphrase, WalletContext& context, bool load_wallet)
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
    const std::string backup_prefix = MigrationFilePrefix(*local_wallet);

    fs::path backup_filename = fs::PathFromString(strprintf("%s_%d.legacy.bak", backup_prefix, GetTime()));
    fs::path backup_path = fsbridge::AbsPathJoin(GetWalletDir(), backup_filename);
    if (!local_wallet->BackupWallet(fs::PathToString(backup_path))) {
        return util::Error{_("Error: Unable to make a backup of your wallet")};
    }
    res.backup_path = backup_path;

    bool success = false;

    // Unlock the wallet if needed
    if (local_wallet->IsLocked()) {
        if (auto unlocked{local_wallet->Unlock(passphrase)}; !unlocked) return util::Error{unlocked.error().message};
    }

    // Indicates whether the current wallet is empty after migration.
    // Notes:
    // When non-empty: the local wallet becomes the main spendable wallet.
    // When empty: The local wallet is excluded from the result, as the
    //             user does not expect an empty spendable wallet after
    //             migrating only watch-only scripts.
    bool empty_local_wallet = false;

    try {
        LOCK(local_wallet->cs_wallet);
        // First change to using SQLite
        std::unordered_set<std::string> skip_types = DBKeys::LEGACY_TYPES;
        skip_types.insert(DBKeys::TX);
        bool had_legacy_records = false;
        if (!local_wallet->MigrateToSQLite(skip_types, error, had_legacy_records)) return util::Error{error};

        // Do the migration of keys and scripts for non-empty wallets, and cleanup if it fails
        if (had_legacy_records) {
            success = DoMigration(*local_wallet, context, error, res, load_wallet);
            // No scripts mean empty wallet after migration
            empty_local_wallet = local_wallet->GetAllScriptPubKeyMans().empty();
        } else {
            // Make sure that descriptors flag is actually set
            local_wallet->SetWalletFlag(WALLET_FLAG_DESCRIPTORS);
            success = true;
        }
    } catch (const std::exception& e) {
        error = Untranslated(strprintf("Migration failed: %s", e.what()));
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

        if (load_wallet) {
            LogInfo("Loading new wallets after migration...\n");
            /** We only override the load_on_startup setting in case the user explicitly said
             * that he does not want to load the wallet, otherwise keep the old wallet configuration */
        } else {
            UpdateWalletSetting(*context.chain, wallet_name, /*load_on_startup=*/false, warnings);
        }
        // Migration successful, if load_wallet is set load all the migrated wallets.
        bool main_wallet_set{false};
        for (std::shared_ptr<CWallet>* wallet_ptr : {&local_wallet, &res.watchonly_wallet, &res.solvables_wallet}) {
            if (success && *wallet_ptr) {
                std::shared_ptr<CWallet>& wallet = *wallet_ptr;
                // Track db path
                track_for_cleanup(*wallet);
                assert(wallet.use_count() == 1);
                std::string wallet_name = wallet->GetName();
                wallet.reset();
                if (load_wallet) {
                    wallet = LoadWallet(context, wallet_name, /*load_on_start=*/std::nullopt, options, status, error, warnings);
                    if (!wallet) {
                        LogError("Failed to load wallet '%s' after migration. Rolling back migration to preserve consistency. "
                                 "Error cause: %s\n", wallet_name, error.original);
                        success = false;
                        break;
                    }
                }
                // Set the first wallet as the main one.
                // The loop order is intentional and must always start with the local wallet.
                if (!main_wallet_set) {
                    res.wallet_name = wallet_name;
                    if (load_wallet) res.wallet = std::move(wallet);
                    main_wallet_set = true;
                }
                if (wallet_ptr == &res.watchonly_wallet) {
                    res.watchonly_wallet_name = wallet_name;
                } else if (wallet_ptr == &res.solvables_wallet) {
                    res.solvables_wallet_name = wallet_name;
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
} // namespace legacy_migration
} // namespace wallet
