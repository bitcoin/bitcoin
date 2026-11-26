// Copyright (c) 2019-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/scriptpubkeyman.h>

#include <coins.h>
#include <hash.h>
#include <key_io.h>
#include <node/types.h>
#include <outputtype.h>
#include <script/descriptor.h>
#include <script/script.h>
#include <script/sign.h>
#include <script/solver.h>
#include <util/bip32.h>
#include <util/check.h>
#include <util/log.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/time.h>
#include <util/translation.h>

#include <limits>
#include <optional>

using common::PSBTError;

namespace wallet {

std::unique_ptr<DescriptorScriptPubKeyMan> DescriptorScriptPubKeyMan::CreateFromImport(WalletStorage& storage, WalletDescriptor& descriptor, int64_t keypool_size, const FlatSigningProvider& provider)
{
    auto spkm = std::unique_ptr<DescriptorScriptPubKeyMan>(new DescriptorScriptPubKeyMan(storage, descriptor, keypool_size));
    LOCK(spkm->cs_desc_man);
    WalletBatch batch(storage.GetDatabase());
    spkm->UpdateWithSigningProvider(batch, provider);
    return spkm;
}

std::unique_ptr<DescriptorScriptPubKeyMan> DescriptorScriptPubKeyMan::CreateFromMigration(WalletStorage& storage, WalletBatch& batch, WalletDescriptor& descriptor, int64_t keypool_size, const FlatSigningProvider& provider)
{
    auto spkm = std::unique_ptr<DescriptorScriptPubKeyMan>(new DescriptorScriptPubKeyMan(storage, descriptor, keypool_size));
    LOCK(spkm->cs_desc_man);
    spkm->UpdateWithSigningProvider(batch, provider);
    return spkm;
}

DescriptorScriptPubKeyMan::DescriptorScriptPubKeyMan(WalletStorage& storage, const uint256& id, WalletDescriptor& descriptor, int64_t keypool_size, const KeyMap& keys, const CryptedKeyMap& ckeys)
    : ScriptPubKeyMan(storage),
    m_map_keys(keys),
    m_map_crypted_keys(ckeys),
    m_keypool_size(keypool_size),
    m_id(id),
    m_wallet_descriptor(descriptor)
{
    if (!keys.empty() && !ckeys.empty()) {
        throw std::runtime_error("Wallet contains both unencrypted and encrypted keys");
    }
    Load();
}

std::unique_ptr<DescriptorScriptPubKeyMan> DescriptorScriptPubKeyMan::LoadFromStorage(WalletStorage& storage, const uint256& id, WalletDescriptor& descriptor, int64_t keypool_size, const KeyMap& keys, const CryptedKeyMap& ckeys)
{
    return std::unique_ptr<DescriptorScriptPubKeyMan>(new DescriptorScriptPubKeyMan(storage, id, descriptor, keypool_size, keys, ckeys));
}

std::unique_ptr<DescriptorScriptPubKeyMan> DescriptorScriptPubKeyMan::GenerateNewSingleSig(WalletStorage& storage, WalletBatch& batch, int64_t keypool_size, const CExtKey& master_key, OutputType addr_type, bool internal)
{
    WalletDescriptor desc = GenerateWalletDescriptor(master_key.Neuter(), addr_type, internal);

    auto spkm = std::unique_ptr<DescriptorScriptPubKeyMan>(new DescriptorScriptPubKeyMan(storage, desc, keypool_size));

    LOCK(spkm->cs_desc_man);
    Assert(spkm->m_storage.IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS));

    // Store the master private key, and descriptor
    if (!spkm->AddDescriptorKeyWithDB(batch, master_key.key, master_key.key.GetPubKey())) {
        throw std::runtime_error(std::string(__func__) + ": writing descriptor master private key failed");
    }
    if (!batch.WriteDescriptor(spkm->GetID(), spkm->m_wallet_descriptor)) {
        throw std::runtime_error(std::string(__func__) + ": writing descriptor failed");
    }

    // Set m_decryption_thoroughly_checked for encrypted wallets
    if (spkm->m_storage.HasEncryptionKeys()) {
        spkm->m_decryption_thoroughly_checked = true;
    }

    // TopUp
    spkm->TopUpWithDB(batch);

    spkm->m_storage.UnsetBlankWalletFlag(batch);
    return spkm;
}

void DescriptorScriptPubKeyMan::IncIndex()
{
    AssertLockHeld(cs_desc_man);

    const auto old_can = CanGetAddresses();
    m_wallet_descriptor.IncNext();
    const auto new_can = CanGetAddresses();
    if (old_can != new_can) {
        NotifyCanGetAddressesChanged();
    }
}

void DescriptorScriptPubKeyMan::DecIndex()
{
    AssertLockHeld(cs_desc_man);

    const auto old_can = CanGetAddresses();
    m_wallet_descriptor.DecNext();
    const auto new_can = CanGetAddresses();
    if (old_can != new_can) {
        NotifyCanGetAddressesChanged();
    }
}

void DescriptorScriptPubKeyMan::SetRangeEnd(int32_t end)
{
    AssertLockHeld(cs_desc_man);

    const auto old_can = CanGetAddresses();
    m_wallet_descriptor.SetEnd(end);
    const auto new_can = CanGetAddresses();
    if (old_can != new_can) {
        NotifyCanGetAddressesChanged();
    }
}

util::Result<CTxDestination> DescriptorScriptPubKeyMan::GetNewDestination(const OutputType type)
{
    // Returns true if this descriptor supports getting new addresses. Conditions where we may be unable to fetch them (e.g. locked) are caught later
    if (!CanGetAddresses()) {
        return util::Error{_("No addresses available")};
    }
    {
        LOCK(cs_desc_man);
        assert(m_wallet_descriptor.descriptor->IsSingleType()); // This is a combo descriptor which should not be an active descriptor
        std::optional<OutputType> desc_addr_type = m_wallet_descriptor.descriptor->GetOutputType();
        assert(desc_addr_type);
        if (type != *desc_addr_type) {
            throw std::runtime_error(std::string(__func__) + ": Types are inconsistent. Stored type does not match type of newly generated address");
        }

        TopUp();

        // Get the scriptPubKey from the descriptor
        FlatSigningProvider out_keys;
        std::vector<CScript> scripts_temp;
        if (m_wallet_descriptor.GetEnd() <= m_max_cached_index && !TopUp(1)) {
            // We can't generate anymore keys
            return util::Error{_("Error: Keypool ran out, please call keypoolrefill first")};
        }
        if (!m_wallet_descriptor.descriptor->ExpandFromCache(m_wallet_descriptor.GetNext(), m_wallet_descriptor.cache, scripts_temp, out_keys)) {
            // We can't generate anymore keys
            return util::Error{_("Error: Keypool ran out, please call keypoolrefill first")};
        }

        CTxDestination dest;
        if (!ExtractDestination(scripts_temp[0], dest)) {
            return util::Error{_("Error: Cannot extract destination from the generated scriptpubkey")}; // shouldn't happen
        }
        IncIndex();
        WalletBatch(m_storage.GetDatabase()).WriteDescriptor(GetID(), m_wallet_descriptor);
        return dest;
    }
}

bool DescriptorScriptPubKeyMan::IsMine(const CScript& script) const
{
    LOCK(cs_desc_man);
    return m_map_script_pub_keys.contains(script);
}

bool DescriptorScriptPubKeyMan::CheckDecryptionKey(const CKeyingMaterial& master_key)
{
    LOCK(cs_desc_man);
    if (!m_map_keys.empty()) {
        return false;
    }

    bool keyPass = m_map_crypted_keys.empty(); // Always pass when there are no encrypted keys
    bool keyFail = false;
    for (const auto& mi : m_map_crypted_keys) {
        const CPubKey &pubkey = mi.second.first;
        const std::vector<unsigned char> &crypted_secret = mi.second.second;
        CKey key;
        if (!DecryptKey(master_key, crypted_secret, pubkey, key)) {
            keyFail = true;
            break;
        }
        keyPass = true;
        if (m_decryption_thoroughly_checked)
            break;
    }
    if (keyPass && keyFail) {
        LogWarning("The wallet is probably corrupted: Some keys decrypt but not all.");
        throw std::runtime_error("Error unlocking wallet: some keys decrypt but not all. Your wallet file may be corrupt.");
    }
    if (keyFail || !keyPass) {
        return false;
    }
    m_decryption_thoroughly_checked = true;
    return true;
}

bool DescriptorScriptPubKeyMan::Encrypt(const CKeyingMaterial& master_key, WalletBatch* batch)
{
    LOCK(cs_desc_man);
    if (!m_map_crypted_keys.empty()) {
        return false;
    }

    CryptedKeyMap crypted_keys;
    for (const KeyMap::value_type& key_in : m_map_keys)
    {
        const CKey &key = key_in.second;
        CPubKey pubkey = key.GetPubKey();
        CKeyingMaterial secret{UCharCast(key.begin()), UCharCast(key.end())};
        std::vector<unsigned char> crypted_secret;
        if (!EncryptSecret(master_key, secret, pubkey.GetHash(), crypted_secret)) {
            return false;
        }
        if (!batch->WriteCryptedDescriptorKey(GetID(), pubkey, crypted_secret)) {
            return false;
        }
        crypted_keys[pubkey.GetID()] = make_pair(pubkey, std::move(crypted_secret));
    }

    batch->RegisterTxnListener({
        .on_commit = [this, keys = std::move(crypted_keys)]() mutable {
            LOCK(cs_desc_man);
            m_map_crypted_keys = std::move(keys);
            m_map_keys.clear();
        },
        .on_abort = [] {}});
    return true;
}

util::Result<CTxDestination> DescriptorScriptPubKeyMan::GetReservedDestination(const OutputType type, bool internal, int64_t& index)
{
    LOCK(cs_desc_man);
    auto op_dest = GetNewDestination(type);
    index = m_wallet_descriptor.GetNext() - 1;
    return op_dest;
}

void DescriptorScriptPubKeyMan::ReturnDestination(int64_t index, bool internal, const CTxDestination& addr)
{
    LOCK(cs_desc_man);
    // Only return when the index was the most recent
    if (m_wallet_descriptor.GetNext() - 1 == index) {
        DecIndex();
    }
    WalletBatch(m_storage.GetDatabase()).WriteDescriptor(GetID(), m_wallet_descriptor);
}

std::map<CKeyID, CKey> DescriptorScriptPubKeyMan::GetKeys() const
{
    AssertLockHeld(cs_desc_man);
    if (m_storage.HasEncryptionKeys() && !m_storage.IsLocked()) {
        KeyMap keys;
        for (const auto& key_pair : m_map_crypted_keys) {
            const CPubKey& pubkey = key_pair.second.first;
            const std::vector<unsigned char>& crypted_secret = key_pair.second.second;
            CKey key;
            m_storage.WithEncryptionKey([&](const CKeyingMaterial& encryption_key) {
                return DecryptKey(encryption_key, crypted_secret, pubkey, key);
            });
            keys[pubkey.GetID()] = key;
        }
        return keys;
    }
    return m_map_keys;
}

bool DescriptorScriptPubKeyMan::HasPrivKey(const CKeyID& keyid) const
{
    AssertLockHeld(cs_desc_man);
    return m_map_keys.contains(keyid) || m_map_crypted_keys.contains(keyid);
}

std::optional<CKey> DescriptorScriptPubKeyMan::GetKey(const CKeyID& keyid) const
{
    AssertLockHeld(cs_desc_man);
    if (m_storage.HasEncryptionKeys() && !m_storage.IsLocked()) {
        const auto& it = m_map_crypted_keys.find(keyid);
        if (it == m_map_crypted_keys.end()) {
            return std::nullopt;
        }
        const std::vector<unsigned char>& crypted_secret = it->second.second;
        CKey key;
        if (!Assume(m_storage.WithEncryptionKey([&](const CKeyingMaterial& encryption_key) {
            return DecryptKey(encryption_key, crypted_secret, it->second.first, key);
        }))) {
            return std::nullopt;
        }
        return key;
    }
    const auto& it = m_map_keys.find(keyid);
    if (it == m_map_keys.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool DescriptorScriptPubKeyMan::TopUp(unsigned int size)
{
    WalletBatch batch(m_storage.GetDatabase());
    if (!batch.TxnBegin()) return false;
    bool res = TopUpWithDB(batch, size);
    if (!batch.TxnCommit()) throw std::runtime_error(strprintf("Error during descriptors keypool top up. Cannot commit changes for wallet [%s]", m_storage.LogName()));
    return res;
}

bool DescriptorScriptPubKeyMan::TopUpWithDB(WalletBatch& batch, unsigned int size)
{
    LOCK(cs_desc_man);
    std::set<CScript> new_spks;
    unsigned int target_size;
    if (size > 0) {
        target_size = size;
    } else {
        target_size = m_keypool_size;
    }

    // Calculate the new range_end. next_index can be as high as 2^31-1 on an imported
    // descriptor and target_size is user controlled, so compute the end in a wider type.
    // The range is left unchanged if that end does not fit in the int32_t it is stored in.
    const int64_t target_end{int64_t{m_wallet_descriptor.GetNext()} + target_size};
    int32_t new_range_end = m_wallet_descriptor.GetEnd();
    if (target_end <= std::numeric_limits<int32_t>::max()) {
        new_range_end = std::max(static_cast<int32_t>(target_end), m_wallet_descriptor.GetEnd());
    }

    // If the descriptor is not ranged, we actually just want to fill the first cache item
    if (!m_wallet_descriptor.descriptor->IsRange()) {
        new_range_end = 1;
    }

    FlatSigningProvider provider;
    provider.keys = GetKeys();

    uint256 id = GetID();
    for (int32_t i = m_max_cached_index + 1; i < new_range_end; ++i) {
        FlatSigningProvider out_keys;
        std::vector<CScript> scripts_temp;
        DescriptorCache temp_cache;
        // Maybe we have a cached xpub and we can expand from the cache first
        if (!m_wallet_descriptor.descriptor->ExpandFromCache(i, m_wallet_descriptor.cache, scripts_temp, out_keys)) {
            if (!m_wallet_descriptor.descriptor->Expand(i, provider, scripts_temp, out_keys, &temp_cache)) return false;
        }
        // Add all of the scriptPubKeys to the scriptPubKey set
        new_spks.insert(scripts_temp.begin(), scripts_temp.end());
        for (const CScript& script : scripts_temp) {
            m_map_script_pub_keys[script] = i;
        }
        for (const auto& pk_pair : out_keys.pubkeys) {
            const CPubKey& pubkey = pk_pair.second;
            if (m_map_pubkeys.contains(pubkey)) {
                // We don't need to give an error here.
                // It doesn't matter which of many valid indexes the pubkey has, we just need an index where we can derive it and its private key
                continue;
            }
            m_map_pubkeys[pubkey] = i;
        }
        // Merge and write the cache
        DescriptorCache new_items = m_wallet_descriptor.cache.MergeAndDiff(temp_cache);
        if (!batch.WriteDescriptorCacheItems(id, new_items)) {
            throw std::runtime_error(std::string(__func__) + ": writing cache items failed");
        }
        m_max_cached_index++;
    }
    SetRangeEnd(new_range_end);
    batch.WriteDescriptor(GetID(), m_wallet_descriptor);

    // By this point, the cache size should be the size of the entire range
    assert(m_wallet_descriptor.GetEnd() - 1 == m_max_cached_index);

    m_storage.TopUpCallback(new_spks, this);
    return true;
}

std::vector<WalletDestination> DescriptorScriptPubKeyMan::MarkUnusedAddresses(const CScript& script)
{
    LOCK(cs_desc_man);
    std::vector<WalletDestination> result;
    if (IsMine(script)) {
        int32_t index = m_map_script_pub_keys[script];
        if (index >= m_wallet_descriptor.GetNext()) {
            WalletLogPrintf("%s: Detected a used keypool item at index %d, mark all keypool items up to this item as used", __func__, index);
            auto out_keys = std::make_unique<FlatSigningProvider>();
            std::vector<CScript> scripts_temp;
            while (index >= m_wallet_descriptor.GetNext()) {
                if (!m_wallet_descriptor.descriptor->ExpandFromCache(m_wallet_descriptor.GetNext(), m_wallet_descriptor.cache, scripts_temp, *out_keys)) {
                    throw std::runtime_error(std::string(__func__) + ": Unable to expand descriptor from cache");
                }
                CTxDestination dest;
                ExtractDestination(scripts_temp[0], dest);
                result.push_back({dest, std::nullopt});
                IncIndex();
            }
        }
        if (!TopUp()) {
            WalletLogPrintf("%s: Topping up keypool failed (locked wallet)", __func__);
        }
    }

    return result;
}

bool DescriptorScriptPubKeyMan::AddDescriptorKeyWithDB(WalletBatch& batch, const CKey& key, const CPubKey &pubkey)
{
    AssertLockHeld(cs_desc_man);
    assert(!m_storage.IsWalletFlagSet(WALLET_FLAG_DISABLE_PRIVATE_KEYS));

    // Check if provided key already exists
    if (m_map_keys.contains(pubkey.GetID()) ||
        m_map_crypted_keys.contains(pubkey.GetID())) {
        return true;
    }

    if (m_storage.HasEncryptionKeys()) {
        if (m_storage.IsLocked()) {
            return false;
        }

        std::vector<unsigned char> crypted_secret;
        CKeyingMaterial secret{UCharCast(key.begin()), UCharCast(key.end())};
        if (!m_storage.WithEncryptionKey([&](const CKeyingMaterial& encryption_key) {
                return EncryptSecret(encryption_key, secret, pubkey.GetHash(), crypted_secret);
            })) {
            return false;
        }

        if (!batch.WriteCryptedDescriptorKey(GetID(), pubkey, crypted_secret)) {
            return false;
        }
        m_map_crypted_keys[pubkey.GetID()] = make_pair(pubkey, std::move(crypted_secret));
    } else {
        if (!batch.WriteDescriptorKey(GetID(), pubkey, key.GetPrivKey())) {
            return false;
        }
        m_map_keys[pubkey.GetID()] = key;
    }
    return true;
}

bool DescriptorScriptPubKeyMan::IsHDEnabled() const
{
    LOCK(cs_desc_man);
    return m_wallet_descriptor.descriptor->IsRange();
}

bool DescriptorScriptPubKeyMan::CanGetAddresses(bool internal) const
{
    // We can only give out addresses from descriptors that are single type (not combo), ranged,
    // and either have cached keys or can generate more keys (ignoring encryption)
    LOCK(cs_desc_man);
    return m_wallet_descriptor.descriptor->IsSingleType() &&
           m_wallet_descriptor.descriptor->IsRange() &&
           (HavePrivateKeys() || m_wallet_descriptor.GetNext() < m_wallet_descriptor.GetEnd() || m_wallet_descriptor.descriptor->CanSelfExpand());
}

bool DescriptorScriptPubKeyMan::HavePrivateKeys() const
{
    LOCK(cs_desc_man);
    return m_map_keys.size() > 0 || m_map_crypted_keys.size() > 0;
}

bool DescriptorScriptPubKeyMan::HaveCryptedKeys() const
{
    LOCK(cs_desc_man);
    return !m_map_crypted_keys.empty();
}

unsigned int DescriptorScriptPubKeyMan::GetKeyPoolSize() const
{
    LOCK(cs_desc_man);
    return m_wallet_descriptor.GetEnd() - m_wallet_descriptor.GetNext();
}

int64_t DescriptorScriptPubKeyMan::GetTimeFirstKey() const
{
    LOCK(cs_desc_man);
    return m_wallet_descriptor.creation_time;
}

std::unique_ptr<FlatSigningProvider> DescriptorScriptPubKeyMan::GetSigningProvider(const CScript& script, bool include_private) const
{
    LOCK(cs_desc_man);

    // Find the index of the script
    auto it = m_map_script_pub_keys.find(script);
    if (it == m_map_script_pub_keys.end()) {
        return nullptr;
    }
    int32_t index = it->second;

    return GetSigningProvider(index, include_private);
}

std::unique_ptr<FlatSigningProvider> DescriptorScriptPubKeyMan::GetSigningProvider(const CPubKey& pubkey) const
{
    LOCK(cs_desc_man);

    // Find index of the pubkey
    auto it = m_map_pubkeys.find(pubkey);
    if (it == m_map_pubkeys.end()) {
        return nullptr;
    }
    int32_t index = it->second;

    // Always try to get the signing provider with private keys. This function should only be called during signing anyways
    std::unique_ptr<FlatSigningProvider> out = GetSigningProvider(index, true);
    if (!out->HaveKey(pubkey.GetID())) {
        return nullptr;
    }
    return out;
}

std::unique_ptr<FlatSigningProvider> DescriptorScriptPubKeyMan::GetSigningProvider(int32_t index, bool include_private) const
{
    AssertLockHeld(cs_desc_man);

    std::unique_ptr<FlatSigningProvider> out_keys = std::make_unique<FlatSigningProvider>();

    // Fetch SigningProvider from cache to avoid re-deriving
    auto it = m_map_signing_providers.find(index);
    if (it != m_map_signing_providers.end()) {
        out_keys->Merge(FlatSigningProvider{it->second});
    } else {
        // Get the scripts, keys, and key origins for this script
        std::vector<CScript> scripts_temp;
        if (!m_wallet_descriptor.descriptor->ExpandFromCache(index, m_wallet_descriptor.cache, scripts_temp, *out_keys)) return nullptr;

        // Cache SigningProvider so we don't need to re-derive if we need this SigningProvider again
        m_map_signing_providers[index] = *out_keys;
    }

    if (HavePrivateKeys() && include_private) {
        FlatSigningProvider master_provider;
        master_provider.keys = GetKeys();
        m_wallet_descriptor.descriptor->ExpandPrivate(index, master_provider, *out_keys);

        // Always include musig_secnonces as this descriptor may have a participant private key
        // but not a musig() descriptor
        out_keys->musig2_secnonces = &m_musig2_secnonces;
    }

    return out_keys;
}

std::unique_ptr<SigningProvider> DescriptorScriptPubKeyMan::GetSolvingProvider(const CScript& script) const
{
    return GetSigningProvider(script, false);
}

bool DescriptorScriptPubKeyMan::CanProvide(const CScript& script, SignatureData& sigdata)
{
    return IsMine(script);
}

bool DescriptorScriptPubKeyMan::SignTransaction(CMutableTransaction& tx, const std::map<COutPoint, Coin>& coins, int sighash, std::map<int, bilingual_str>& input_errors) const
{
    std::unique_ptr<FlatSigningProvider> keys = std::make_unique<FlatSigningProvider>();
    for (const auto& coin_pair : coins) {
        std::unique_ptr<FlatSigningProvider> coin_keys = GetSigningProvider(coin_pair.second.out.scriptPubKey, true);
        if (!coin_keys) {
            continue;
        }
        keys->Merge(std::move(*coin_keys));
    }

    return ::SignTransaction(tx, keys.get(), coins, {.sighash_type = sighash}, input_errors);
}

SigningResult DescriptorScriptPubKeyMan::SignMessage(const std::string& message, const PKHash& pkhash, std::string& str_sig) const
{
    std::unique_ptr<FlatSigningProvider> keys = GetSigningProvider(GetScriptForDestination(pkhash), true);
    if (!keys) {
        return SigningResult::PRIVATE_KEY_NOT_AVAILABLE;
    }

    CKey key;
    if (!keys->GetKey(ToKeyID(pkhash), key)) {
        return SigningResult::PRIVATE_KEY_NOT_AVAILABLE;
    }

    if (!MessageSign(key, message, str_sig)) {
        return SigningResult::SIGNING_FAILED;
    }
    return SigningResult::OK;
}

std::optional<PSBTError> DescriptorScriptPubKeyMan::FillPSBT(PartiallySignedTransaction& psbtx, const PrecomputedTransactionData& txdata, const common::PSBTFillOptions& options, int* n_signed) const
{
    if (n_signed) {
        *n_signed = 0;
    }
    for (unsigned int i = 0; i < psbtx.inputs.size(); ++i) {
        PSBTInput& input = psbtx.inputs.at(i);

        if (PSBTInputSigned(input)) {
            continue;
        }

        // Get the scriptPubKey to know which SigningProvider to use
        CScript script;
        if (!input.witness_utxo.IsNull()) {
            script = input.witness_utxo.scriptPubKey;
        } else if (input.non_witness_utxo) {
            if (input.prev_out >= input.non_witness_utxo->vout.size()) {
                return PSBTError::MISSING_INPUTS;
            }
            script = input.non_witness_utxo->vout[input.prev_out].scriptPubKey;
        } else {
            // There's no UTXO so we can just skip this now
            continue;
        }

        std::unique_ptr<FlatSigningProvider> keys = std::make_unique<FlatSigningProvider>();
        std::unique_ptr<FlatSigningProvider> script_keys = GetSigningProvider(script, /*include_private=*/options.sign);
        if (script_keys) {
            keys->Merge(std::move(*script_keys));
        } else {
            // Maybe there are pubkeys listed that we can sign for
            std::vector<CPubKey> pubkeys;
            pubkeys.reserve(input.hd_keypaths.size() + 2);

            // ECDSA Pubkeys
            for (const auto& [pk, _] : input.hd_keypaths) {
                pubkeys.push_back(pk);
            }

            // Taproot output pubkey
            std::vector<std::vector<unsigned char>> sols;
            if (Solver(script, sols) == TxoutType::WITNESS_V1_TAPROOT) {
                sols[0].insert(sols[0].begin(), 0x02);
                pubkeys.emplace_back(sols[0]);
                sols[0][0] = 0x03;
                pubkeys.emplace_back(sols[0]);
            }

            // Taproot pubkeys
            for (const auto& pk_pair : input.m_tap_bip32_paths) {
                const XOnlyPubKey& pubkey = pk_pair.first;
                for (unsigned char prefix : {0x02, 0x03}) {
                    unsigned char b[33] = {prefix};
                    std::copy(pubkey.begin(), pubkey.end(), b + 1);
                    CPubKey fullpubkey;
                    fullpubkey.Set(b, b + 33);
                    pubkeys.push_back(fullpubkey);
                }
            }

            for (const auto& pubkey : pubkeys) {
                std::unique_ptr<FlatSigningProvider> pk_keys = GetSigningProvider(pubkey);
                if (pk_keys) {
                    keys->Merge(std::move(*pk_keys));
                }
            }
        }

        const auto sign_result = SignPSBTInput(HidingSigningProvider(keys.get(), /*hide_secret=*/!options.sign, /*hide_origin=*/!options.bip32_derivs), psbtx, i, &txdata, options, /*out_sigdata=*/nullptr);
        if (!sign_result.has_value() && sign_result.error() != PSBTError::INCOMPLETE) {
            return sign_result.error();
        }

        bool signed_one = PSBTInputSigned(input);
        if (n_signed && (signed_one || !options.sign)) {
            // If sign is false, we assume that we _could_ sign if we get here. This
            // will never have false negatives; it is hard to tell under what i
            // circumstances it could have false positives.
            (*n_signed)++;
        }
    }

    // Fill in the bip32 keypaths and redeemscripts for the outputs so that hardware wallets can identify change
    for (unsigned int i = 0; i < psbtx.outputs.size(); ++i) {
        std::unique_ptr<SigningProvider> keys = GetSolvingProvider(psbtx.outputs.at(i).script);
        if (!keys) {
            continue;
        }
        UpdatePSBTOutput(HidingSigningProvider(keys.get(), /*hide_secret=*/true, /*hide_origin=*/!options.bip32_derivs), psbtx, i);
    }

    return {};
}

std::unique_ptr<CKeyMetadata> DescriptorScriptPubKeyMan::GetMetadata(const CTxDestination& dest) const
{
    std::unique_ptr<SigningProvider> provider = GetSigningProvider(GetScriptForDestination(dest));
    if (provider) {
        KeyOriginInfo orig;
        CKeyID key_id = GetKeyForDestination(*provider, dest);
        if (provider->GetKeyOrigin(key_id, orig)) {
            LOCK(cs_desc_man);
            std::unique_ptr<CKeyMetadata> meta = std::make_unique<CKeyMetadata>();
            meta->key_origin = orig;
            meta->has_key_origin = true;
            meta->nCreateTime = m_wallet_descriptor.creation_time;
            return meta;
        }
    }
    return nullptr;
}

uint256 DescriptorScriptPubKeyMan::GetID() const
{
    return m_id;
}

void DescriptorScriptPubKeyMan::Load()
{
    LOCK(cs_desc_man);
    std::set<CScript> new_spks;
    for (int32_t i = m_wallet_descriptor.GetStart(); i < m_wallet_descriptor.GetEnd(); ++i) {
        FlatSigningProvider out_keys;
        std::vector<CScript> scripts_temp;
        if (!m_wallet_descriptor.descriptor->ExpandFromCache(i, m_wallet_descriptor.cache, scripts_temp, out_keys)) {
            throw std::runtime_error("Error: Unable to expand wallet descriptor from cache");
        }
        // Add all of the scriptPubKeys to the scriptPubKey set
        new_spks.insert(scripts_temp.begin(), scripts_temp.end());
        for (const CScript& script : scripts_temp) {
            if (m_map_script_pub_keys.contains(script)) {
                throw std::runtime_error(strprintf("Error: Already loaded script at index %d as being at index %d", i, m_map_script_pub_keys[script]));
            }
            m_map_script_pub_keys[script] = i;
        }
        for (const auto& pk_pair : out_keys.pubkeys) {
            const CPubKey& pubkey = pk_pair.second;
            if (m_map_pubkeys.contains(pubkey)) {
                // We don't need to give an error here.
                // It doesn't matter which of many valid indexes the pubkey has, we just need an index where we can derive it and its private key
                continue;
            }
            m_map_pubkeys[pubkey] = i;
        }
        m_max_cached_index++;
    }
    // Make sure the wallet knows about our new spks
    m_storage.TopUpCallback(new_spks, this);
}

bool DescriptorScriptPubKeyMan::HasWalletDescriptor(const WalletDescriptor& desc) const
{
    LOCK(cs_desc_man);
    return m_wallet_descriptor.IsCanonicallyEquivalent(desc);
}

void DescriptorScriptPubKeyMan::WriteDescriptor()
{
    LOCK(cs_desc_man);
    WalletBatch batch(m_storage.GetDatabase());
    if (!batch.WriteDescriptor(GetID(), m_wallet_descriptor)) {
        throw std::runtime_error(std::string(__func__) + ": writing descriptor failed");
    }
}

WalletDescriptor DescriptorScriptPubKeyMan::GetWalletDescriptor() const
{
    return m_wallet_descriptor;
}

std::unordered_set<CScript, SaltedSipHasher> DescriptorScriptPubKeyMan::GetScriptPubKeys() const
{
    return GetScriptPubKeys(0);
}

std::unordered_set<CScript, SaltedSipHasher> DescriptorScriptPubKeyMan::GetScriptPubKeys(int32_t minimum_index) const
{
    LOCK(cs_desc_man);
    std::unordered_set<CScript, SaltedSipHasher> script_pub_keys;
    script_pub_keys.reserve(m_map_script_pub_keys.size());

    for (auto const& [script_pub_key, index] : m_map_script_pub_keys) {
        if (index >= minimum_index) script_pub_keys.insert(script_pub_key);
    }
    return script_pub_keys;
}

int32_t DescriptorScriptPubKeyMan::GetEndRange() const
{
    return m_max_cached_index + 1;
}

bool DescriptorScriptPubKeyMan::GetDescriptorString(std::string& out, const bool priv) const
{
    LOCK(cs_desc_man);

    FlatSigningProvider provider;
    provider.keys = GetKeys();

    if (priv) {
        // For the private version, always return the master key to avoid
        // exposing child private keys. The risk implications of exposing child
        // private keys together with the parent xpub may be non-obvious for users.
        return m_wallet_descriptor.descriptor->ToPrivateString(provider, out);
    }

    return m_wallet_descriptor.descriptor->ToNormalizedString(provider, out, &m_wallet_descriptor.cache);
}

void DescriptorScriptPubKeyMan::UpgradeDescriptorCache()
{
    LOCK(cs_desc_man);
    if (m_storage.IsLocked() || m_storage.IsWalletFlagSet(WALLET_FLAG_LAST_HARDENED_XPUB_CACHED)) {
        return;
    }

    // Skip if we have the last hardened xpub cache
    if (m_wallet_descriptor.cache.GetCachedLastHardenedExtPubKeys().size() > 0) {
        return;
    }

    // Expand the descriptor
    FlatSigningProvider provider;
    provider.keys = GetKeys();
    FlatSigningProvider out_keys;
    std::vector<CScript> scripts_temp;
    DescriptorCache temp_cache;
    if (!m_wallet_descriptor.descriptor->Expand(0, provider, scripts_temp, out_keys, &temp_cache)){
        throw std::runtime_error("Unable to expand descriptor");
    }

    // Cache the last hardened xpubs
    DescriptorCache diff = m_wallet_descriptor.cache.MergeAndDiff(temp_cache);
    if (!WalletBatch(m_storage.GetDatabase()).WriteDescriptorCacheItems(GetID(), diff)) {
        throw std::runtime_error(std::string(__func__) + ": writing cache items failed");
    }
}

util::Result<void> DescriptorScriptPubKeyMan::UpdateWalletDescriptor(WalletDescriptor& descriptor, const FlatSigningProvider& provider)
{
    LOCK(cs_desc_man);
    std::string error;
    if (!CanUpdateToWalletDescriptor(descriptor, error)) {
        return util::Error{Untranslated(std::move(error))};
    }

    m_map_pubkeys.clear();
    m_map_script_pub_keys.clear();
    m_max_cached_index = -1;
    m_wallet_descriptor.UpdateFrom(descriptor);

    WalletBatch batch(m_storage.GetDatabase());
    UpdateWithSigningProvider(batch, provider);
    NotifyFirstKeyTimeChanged(this, m_wallet_descriptor.creation_time);
    return {};
}

void DescriptorScriptPubKeyMan::UpdateWithSigningProvider(WalletBatch& batch, const FlatSigningProvider& signing_provider)
{
    AssertLockHeld(cs_desc_man);
    // Add the private keys to the descriptor
    for (const auto& entry : signing_provider.keys) {
        const CKey& key = entry.second;
        if (!AddDescriptorKeyWithDB(batch, key, key.GetPubKey())) {
            throw std::runtime_error(std::string(__func__) + ": writing descriptor private key failed");
        }
    }

    // Top up key pool, to generate scriptPubKeys
    if (!TopUpWithDB(batch)) {
        throw std::runtime_error("Could not top up scriptPubKeys");
    }
}

bool DescriptorScriptPubKeyMan::CanUpdateToWalletDescriptor(const WalletDescriptor& descriptor, std::string& error)
{
    LOCK(cs_desc_man);
    if (!HasWalletDescriptor(descriptor)) {
        error = "can only update matching descriptor";
        return false;
    }

    if (!descriptor.descriptor->IsRange()) {
        // Skip range check for non-range descriptors
        return true;
    }

    if (descriptor.GetStart() > m_wallet_descriptor.GetStart() ||
        descriptor.GetEnd() < m_wallet_descriptor.GetEnd()) {
        // Use inclusive range for error
        error = strprintf("new range must include current range = [%d,%d]",
                          m_wallet_descriptor.GetStart(),
                          m_wallet_descriptor.GetEnd() - 1);
        return false;
    }

    return true;
}
} // namespace wallet
