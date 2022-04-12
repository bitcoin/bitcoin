// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/walletdb.h>

#include <common/system.h>
#include <key_io.h>
#include <protocol.h>
#include <serialize.h>
#include <sync.h>
#include <util/bip32.h>
#include <util/fs.h>
#include <util/time.h>
#include <util/translation.h>
#ifdef USE_BDB
#include <wallet/bdb.h>
#endif
#ifdef USE_SQLITE
#include <wallet/sqlite.h>
#endif
#include <wallet/wallet.h>

#include <atomic>
#include <optional>
#include <string>

namespace wallet {
namespace DBKeys {
const std::string ACENTRY{"acentry"};
const std::string ACTIVEEXTERNALSPK{"activeexternalspk"};
const std::string ACTIVEINTERNALSPK{"activeinternalspk"};
const std::string BESTBLOCK_NOMERKLE{"bestblock_nomerkle"};
const std::string BESTBLOCK{"bestblock"};
const std::string CRYPTED_KEY{"ckey"};
const std::string CSCRIPT{"cscript"};
const std::string DEFAULTKEY{"defaultkey"};
const std::string DESTDATA{"destdata"};
const std::string FLAGS{"flags"};
const std::string HDCHAIN{"hdchain"};
const std::string KEYMETA{"keymeta"};
const std::string KEY{"key"};
const std::string LOCKED_UTXO{"lockedutxo"};
const std::string MASTER_KEY{"mkey"};
const std::string MINVERSION{"minversion"};
const std::string NAME{"name"};
const std::string OLD_KEY{"wkey"};
const std::string ORDERPOSNEXT{"orderposnext"};
const std::string POOL{"pool"};
const std::string PURPOSE{"purpose"};
const std::string SETTINGS{"settings"};
const std::string TX{"tx"};
const std::string VERSION{"version"};
const std::string WALLETDESCRIPTOR{"walletdescriptor"};
const std::string WALLETDESCRIPTORCACHE{"walletdescriptorcache"};
const std::string WALLETDESCRIPTORLHCACHE{"walletdescriptorlhcache"};
const std::string WALLETDESCRIPTORCKEY{"walletdescriptorckey"};
const std::string WALLETDESCRIPTORKEY{"walletdescriptorkey"};
const std::string WATCHMETA{"watchmeta"};
const std::string WATCHS{"watchs"};
const std::unordered_set<std::string> LEGACY_TYPES{CRYPTED_KEY, CSCRIPT, DEFAULTKEY, HDCHAIN, KEYMETA, KEY, OLD_KEY, POOL, WATCHMETA, WATCHS};
} // namespace DBKeys

//
// WalletBatch
//

bool WalletBatch::WriteName(const std::string& strAddress, const std::string& strName)
{
    return WriteIC(std::make_pair(DBKeys::NAME, strAddress), strName);
}

bool WalletBatch::EraseName(const std::string& strAddress)
{
    // This should only be used for sending addresses, never for receiving addresses,
    // receiving addresses must always have an address book entry if they're not change return.
    return EraseIC(std::make_pair(DBKeys::NAME, strAddress));
}

bool WalletBatch::WritePurpose(const std::string& strAddress, const std::string& strPurpose)
{
    return WriteIC(std::make_pair(DBKeys::PURPOSE, strAddress), strPurpose);
}

bool WalletBatch::ErasePurpose(const std::string& strAddress)
{
    return EraseIC(std::make_pair(DBKeys::PURPOSE, strAddress));
}

bool WalletBatch::WriteTx(const CWalletTx& wtx)
{
    return WriteIC(std::make_pair(DBKeys::TX, wtx.GetHash()), wtx);
}

bool WalletBatch::EraseTx(uint256 hash)
{
    return EraseIC(std::make_pair(DBKeys::TX, hash));
}

bool WalletBatch::WriteKeyMetadata(const CKeyMetadata& meta, const CPubKey& pubkey, const bool overwrite)
{
    return WriteIC(std::make_pair(DBKeys::KEYMETA, pubkey), meta, overwrite);
}

bool WalletBatch::WriteKey(const CPubKey& vchPubKey, const CPrivKey& vchPrivKey, const CKeyMetadata& keyMeta)
{
    if (!WriteKeyMetadata(keyMeta, vchPubKey, false)) {
        return false;
    }

    // hash pubkey/privkey to accelerate wallet load
    std::vector<unsigned char> vchKey;
    vchKey.reserve(vchPubKey.size() + vchPrivKey.size());
    vchKey.insert(vchKey.end(), vchPubKey.begin(), vchPubKey.end());
    vchKey.insert(vchKey.end(), vchPrivKey.begin(), vchPrivKey.end());

    return WriteIC(std::make_pair(DBKeys::KEY, vchPubKey), std::make_pair(vchPrivKey, Hash(vchKey)), false);
}

bool WalletBatch::WriteCryptedKey(const CPubKey& vchPubKey,
                                const std::vector<unsigned char>& vchCryptedSecret,
                                const CKeyMetadata &keyMeta)
{
    if (!WriteKeyMetadata(keyMeta, vchPubKey, true)) {
        return false;
    }

    // Compute a checksum of the encrypted key
    uint256 checksum = Hash(vchCryptedSecret);

    const auto key = std::make_pair(DBKeys::CRYPTED_KEY, vchPubKey);
    if (!WriteIC(key, std::make_pair(vchCryptedSecret, checksum), false)) {
        // It may already exist, so try writing just the checksum
        std::vector<unsigned char> val;
        if (!m_batch->Read(key, val)) {
            return false;
        }
        if (!WriteIC(key, std::make_pair(val, checksum), true)) {
            return false;
        }
    }
    EraseIC(std::make_pair(DBKeys::KEY, vchPubKey));
    return true;
}

bool WalletBatch::WriteMasterKey(unsigned int nID, const CMasterKey& kMasterKey)
{
    return WriteIC(std::make_pair(DBKeys::MASTER_KEY, nID), kMasterKey, true);
}

bool WalletBatch::WriteCScript(const uint160& hash, const CScript& redeemScript)
{
    return WriteIC(std::make_pair(DBKeys::CSCRIPT, hash), redeemScript, false);
}

bool WalletBatch::WriteWatchOnly(const CScript &dest, const CKeyMetadata& keyMeta)
{
    if (!WriteIC(std::make_pair(DBKeys::WATCHMETA, dest), keyMeta)) {
        return false;
    }
    return WriteIC(std::make_pair(DBKeys::WATCHS, dest), uint8_t{'1'});
}

bool WalletBatch::EraseWatchOnly(const CScript &dest)
{
    if (!EraseIC(std::make_pair(DBKeys::WATCHMETA, dest))) {
        return false;
    }
    return EraseIC(std::make_pair(DBKeys::WATCHS, dest));
}

bool WalletBatch::WriteBestBlock(const CBlockLocator& locator)
{
    WriteIC(DBKeys::BESTBLOCK, CBlockLocator()); // Write empty block locator so versions that require a merkle branch automatically rescan
    return WriteIC(DBKeys::BESTBLOCK_NOMERKLE, locator);
}

bool WalletBatch::ReadBestBlock(CBlockLocator& locator)
{
    if (m_batch->Read(DBKeys::BESTBLOCK, locator) && !locator.vHave.empty()) return true;
    return m_batch->Read(DBKeys::BESTBLOCK_NOMERKLE, locator);
}

bool WalletBatch::WriteOrderPosNext(int64_t nOrderPosNext)
{
    return WriteIC(DBKeys::ORDERPOSNEXT, nOrderPosNext);
}

bool WalletBatch::ReadPool(int64_t nPool, CKeyPool& keypool)
{
    return m_batch->Read(std::make_pair(DBKeys::POOL, nPool), keypool);
}

bool WalletBatch::WritePool(int64_t nPool, const CKeyPool& keypool)
{
    return WriteIC(std::make_pair(DBKeys::POOL, nPool), keypool);
}

bool WalletBatch::ErasePool(int64_t nPool)
{
    return EraseIC(std::make_pair(DBKeys::POOL, nPool));
}

bool WalletBatch::WriteMinVersion(int nVersion)
{
    return WriteIC(DBKeys::MINVERSION, nVersion);
}

bool WalletBatch::WriteActiveScriptPubKeyMan(uint8_t type, const uint256& id, bool internal)
{
    std::string key = internal ? DBKeys::ACTIVEINTERNALSPK : DBKeys::ACTIVEEXTERNALSPK;
    return WriteIC(make_pair(key, type), id);
}

bool WalletBatch::EraseActiveScriptPubKeyMan(uint8_t type, bool internal)
{
    const std::string key{internal ? DBKeys::ACTIVEINTERNALSPK : DBKeys::ACTIVEEXTERNALSPK};
    return EraseIC(make_pair(key, type));
}

bool WalletBatch::WriteDescriptorKey(const uint256& desc_id, const CPubKey& pubkey, const CPrivKey& privkey)
{
    // hash pubkey/privkey to accelerate wallet load
    std::vector<unsigned char> key;
    key.reserve(pubkey.size() + privkey.size());
    key.insert(key.end(), pubkey.begin(), pubkey.end());
    key.insert(key.end(), privkey.begin(), privkey.end());

    return WriteIC(std::make_pair(DBKeys::WALLETDESCRIPTORKEY, std::make_pair(desc_id, pubkey)), std::make_pair(privkey, Hash(key)), false);
}

bool WalletBatch::WriteCryptedDescriptorKey(const uint256& desc_id, const CPubKey& pubkey, const std::vector<unsigned char>& secret)
{
    if (!WriteIC(std::make_pair(DBKeys::WALLETDESCRIPTORCKEY, std::make_pair(desc_id, pubkey)), secret, false)) {
        return false;
    }
    EraseIC(std::make_pair(DBKeys::WALLETDESCRIPTORKEY, std::make_pair(desc_id, pubkey)));
    return true;
}

bool WalletBatch::WriteDescriptor(const uint256& desc_id, const WalletDescriptor& descriptor)
{
    return WriteIC(make_pair(DBKeys::WALLETDESCRIPTOR, desc_id), descriptor);
}

bool WalletBatch::WriteDescriptorDerivedCache(const CExtPubKey& xpub, const uint256& desc_id, uint32_t key_exp_index, uint32_t der_index)
{
    std::vector<unsigned char> ser_xpub(BIP32_EXTKEY_SIZE);
    xpub.Encode(ser_xpub.data());
    return WriteIC(std::make_pair(std::make_pair(DBKeys::WALLETDESCRIPTORCACHE, desc_id), std::make_pair(key_exp_index, der_index)), ser_xpub);
}

bool WalletBatch::WriteDescriptorParentCache(const CExtPubKey& xpub, const uint256& desc_id, uint32_t key_exp_index)
{
    std::vector<unsigned char> ser_xpub(BIP32_EXTKEY_SIZE);
    xpub.Encode(ser_xpub.data());
    return WriteIC(std::make_pair(std::make_pair(DBKeys::WALLETDESCRIPTORCACHE, desc_id), key_exp_index), ser_xpub);
}

bool WalletBatch::WriteDescriptorLastHardenedCache(const CExtPubKey& xpub, const uint256& desc_id, uint32_t key_exp_index)
{
    std::vector<unsigned char> ser_xpub(BIP32_EXTKEY_SIZE);
    xpub.Encode(ser_xpub.data());
    return WriteIC(std::make_pair(std::make_pair(DBKeys::WALLETDESCRIPTORLHCACHE, desc_id), key_exp_index), ser_xpub);
}

bool WalletBatch::WriteDescriptorCacheItems(const uint256& desc_id, const DescriptorCache& cache)
{
    for (const auto& parent_xpub_pair : cache.GetCachedParentExtPubKeys()) {
        if (!WriteDescriptorParentCache(parent_xpub_pair.second, desc_id, parent_xpub_pair.first)) {
            return false;
        }
    }
    for (const auto& derived_xpub_map_pair : cache.GetCachedDerivedExtPubKeys()) {
        for (const auto& derived_xpub_pair : derived_xpub_map_pair.second) {
            if (!WriteDescriptorDerivedCache(derived_xpub_pair.second, desc_id, derived_xpub_map_pair.first, derived_xpub_pair.first)) {
                return false;
            }
        }
    }
    for (const auto& lh_xpub_pair : cache.GetCachedLastHardenedExtPubKeys()) {
        if (!WriteDescriptorLastHardenedCache(lh_xpub_pair.second, desc_id, lh_xpub_pair.first)) {
            return false;
        }
    }
    return true;
}

bool WalletBatch::WriteLockedUTXO(const COutPoint& output)
{
    return WriteIC(std::make_pair(DBKeys::LOCKED_UTXO, std::make_pair(output.hash, output.n)), uint8_t{'1'});
}

bool WalletBatch::EraseLockedUTXO(const COutPoint& output)
{
    return EraseIC(std::make_pair(DBKeys::LOCKED_UTXO, std::make_pair(output.hash, output.n)));
}

class CWalletScanState {
public:
    unsigned int nKeys{0};
    unsigned int nCKeys{0};
    unsigned int nKeyMeta{0};
    unsigned int m_unknown_records{0};
    bool fAnyUnordered{false};
    std::vector<uint256> vWalletUpgrade;
    std::map<OutputType, uint256> m_active_external_spks;
    std::map<OutputType, uint256> m_active_internal_spks;
    std::map<uint256, DescriptorCache> m_descriptor_caches;
    std::map<std::pair<uint256, CKeyID>, CKey> m_descriptor_keys;
    std::map<std::pair<uint256, CKeyID>, std::pair<CPubKey, std::vector<unsigned char>>> m_descriptor_crypt_keys;
    bool tx_corrupt{false};
    bool descriptor_unknown{false};

    CWalletScanState() = default;
};

bool LoadKey(CWallet* pwallet, DataStream& ssKey, DataStream& ssValue, std::string& strErr)
{
    LOCK(pwallet->cs_wallet);
    try {
        CPubKey vchPubKey;
        ssKey >> vchPubKey;
        if (!vchPubKey.IsValid())
        {
            strErr = "Error reading wallet database: CPubKey corrupt";
            return false;
        }
        CKey key;
        CPrivKey pkey;
        uint256 hash;

        ssValue >> pkey;

        // Old wallets store keys as DBKeys::KEY [pubkey] => [privkey]
        // ... which was slow for wallets with lots of keys, because the public key is re-derived from the private key
        // using EC operations as a checksum.
        // Newer wallets store keys as DBKeys::KEY [pubkey] => [privkey][hash(pubkey,privkey)], which is much faster while
        // remaining backwards-compatible.
        try
        {
            ssValue >> hash;
        }
        catch (const std::ios_base::failure&) {}

        bool fSkipCheck = false;

        if (!hash.IsNull())
        {
            // hash pubkey/privkey to accelerate wallet load
            std::vector<unsigned char> vchKey;
            vchKey.reserve(vchPubKey.size() + pkey.size());
            vchKey.insert(vchKey.end(), vchPubKey.begin(), vchPubKey.end());
            vchKey.insert(vchKey.end(), pkey.begin(), pkey.end());

            if (Hash(vchKey) != hash)
            {
                strErr = "Error reading wallet database: CPubKey/CPrivKey corrupt";
                return false;
            }

            fSkipCheck = true;
        }

        if (!key.Load(pkey, vchPubKey, fSkipCheck))
        {
            strErr = "Error reading wallet database: CPrivKey corrupt";
            return false;
        }
        if (!pwallet->GetOrCreateLegacyScriptPubKeyMan()->LoadKey(key, vchPubKey))
        {
            strErr = "Error reading wallet database: LegacyScriptPubKeyMan::LoadKey failed";
            return false;
        }
    } catch (const std::exception& e) {
        if (strErr.empty()) {
            strErr = e.what();
        }
        return false;
    }
    return true;
}

bool LoadCryptedKey(CWallet* pwallet, DataStream& ssKey, DataStream& ssValue, std::string& strErr)
{
    LOCK(pwallet->cs_wallet);
    try {
        CPubKey vchPubKey;
        ssKey >> vchPubKey;
        if (!vchPubKey.IsValid())
        {
            strErr = "Error reading wallet database: CPubKey corrupt";
            return false;
        }
        std::vector<unsigned char> vchPrivKey;
        ssValue >> vchPrivKey;

        // Get the checksum and check it
        bool checksum_valid = false;
        if (!ssValue.eof()) {
            uint256 checksum;
            ssValue >> checksum;
            if (!(checksum_valid = Hash(vchPrivKey) == checksum)) {
                strErr = "Error reading wallet database: Encrypted key corrupt";
                return false;
            }
        }

        if (!pwallet->GetOrCreateLegacyScriptPubKeyMan()->LoadCryptedKey(vchPubKey, vchPrivKey, checksum_valid))
        {
            strErr = "Error reading wallet database: LegacyScriptPubKeyMan::LoadCryptedKey failed";
            return false;
        }
    } catch (const std::exception& e) {
        if (strErr.empty()) {
            strErr = e.what();
        }
        return false;
    }
    return true;
}

bool LoadEncryptionKey(CWallet* pwallet, DataStream& ssKey, DataStream& ssValue, std::string& strErr)
{
    LOCK(pwallet->cs_wallet);
    try {
        // Master encryption key is loaded into only the wallet and not any of the ScriptPubKeyMans.
        unsigned int nID;
        ssKey >> nID;
        CMasterKey kMasterKey;
        ssValue >> kMasterKey;
        if(pwallet->mapMasterKeys.count(nID) != 0)
        {
            strErr = strprintf("Error reading wallet database: duplicate CMasterKey id %u", nID);
            return false;
        }
        pwallet->mapMasterKeys[nID] = kMasterKey;
        if (pwallet->nMasterKeyMaxID < nID)
            pwallet->nMasterKeyMaxID = nID;

    } catch (const std::exception& e) {
        if (strErr.empty()) {
            strErr = e.what();
        }
        return false;
    }
    return true;
}

bool LoadHDChain(CWallet* pwallet, DataStream& ssValue, std::string& strErr)
{
    LOCK(pwallet->cs_wallet);
    try {
        CHDChain chain;
        ssValue >> chain;
        pwallet->GetOrCreateLegacyScriptPubKeyMan()->LoadHDChain(chain);
    } catch (const std::exception& e) {
        if (strErr.empty()) {
            strErr = e.what();
        }
        return false;
    }
    return true;
}

static bool
ReadKeyValue(CWallet* pwallet, DataStream& ssKey, CDataStream& ssValue,
             CWalletScanState &wss, std::string& strType, std::string& strErr) EXCLUSIVE_LOCKS_REQUIRED(pwallet->cs_wallet)
{
    try {
        // Unserialize
        // Taking advantage of the fact that pair serialization
        // is just the two items serialized one after the other
        ssKey >> strType;
        if (pwallet->IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS) && DBKeys::LEGACY_TYPES.count(strType) > 0) {
            return true;
        }
        if (strType == DBKeys::NAME) {
            std::string strAddress;
            ssKey >> strAddress;
            std::string label;
            ssValue >> label;
            pwallet->m_address_book[DecodeDestination(strAddress)].SetLabel(label);
        } else if (strType == DBKeys::PURPOSE) {
            std::string strAddress;
            ssKey >> strAddress;
            std::string purpose_str;
            ssValue >> purpose_str;
            std::optional<AddressPurpose> purpose{PurposeFromString(purpose_str)};
            if (!purpose) {
                pwallet->WalletLogPrintf("Warning: nonstandard purpose string '%s' for address '%s'\n", purpose_str, strAddress);
            }
            pwallet->m_address_book[DecodeDestination(strAddress)].purpose = purpose;
        } else if (strType == DBKeys::TX) {
            uint256 hash;
            ssKey >> hash;
            // LoadToWallet call below creates a new CWalletTx that fill_wtx
            // callback fills with transaction metadata.
            auto fill_wtx = [&](CWalletTx& wtx, bool new_tx) {
                if(!new_tx) {
                    // There's some corruption here since the tx we just tried to load was already in the wallet.
                    // We don't consider this type of corruption critical, and can fix it by removing tx data and
                    // rescanning.
                    wss.tx_corrupt = true;
                    return false;
                }
                ssValue >> wtx;
                if (wtx.GetHash() != hash)
                    return false;

                // Undo serialize changes in 31600
                if (31404 <= wtx.fTimeReceivedIsTxTime && wtx.fTimeReceivedIsTxTime <= 31703)
                {
                    if (!ssValue.empty())
                    {
                        uint8_t fTmp;
                        uint8_t fUnused;
                        std::string unused_string;
                        ssValue >> fTmp >> fUnused >> unused_string;
                        strErr = strprintf("LoadWallet() upgrading tx ver=%d %d %s",
                                           wtx.fTimeReceivedIsTxTime, fTmp, hash.ToString());
                        wtx.fTimeReceivedIsTxTime = fTmp;
                    }
                    else
                    {
                        strErr = strprintf("LoadWallet() repairing tx ver=%d %s", wtx.fTimeReceivedIsTxTime, hash.ToString());
                        wtx.fTimeReceivedIsTxTime = 0;
                    }
                    wss.vWalletUpgrade.push_back(hash);
                }

                if (wtx.nOrderPos == -1)
                    wss.fAnyUnordered = true;

                return true;
            };
            if (!pwallet->LoadToWallet(hash, fill_wtx)) {
                return false;
            }
        } else if (strType == DBKeys::WATCHS) {
        } else if (strType == DBKeys::KEY) {
            wss.nKeys++;
        } else if (strType == DBKeys::MASTER_KEY) {
            if (!LoadEncryptionKey(pwallet, ssKey, ssValue, strErr)) return false;
        } else if (strType == DBKeys::CRYPTED_KEY) {
            wss.nCKeys++;
        } else if (strType == DBKeys::KEYMETA) {
            wss.nKeyMeta++;
        } else if (strType == DBKeys::WATCHMETA) {
            wss.nKeyMeta++;
        } else if (strType == DBKeys::DEFAULTKEY) {
            // We don't want or need the default key, but if there is one set,
            // we want to make sure that it is valid so that we can detect corruption
            CPubKey vchPubKey;
            ssValue >> vchPubKey;
            if (!vchPubKey.IsValid()) {
                strErr = "Error reading wallet database: Default Key corrupt";
                return false;
            }
        } else if (strType == DBKeys::POOL) {
        } else if (strType == DBKeys::CSCRIPT) {
        } else if (strType == DBKeys::ORDERPOSNEXT) {
            ssValue >> pwallet->nOrderPosNext;
        } else if (strType == DBKeys::DESTDATA) {
            std::string strAddress, strKey, strValue;
            ssKey >> strAddress;
            ssKey >> strKey;
            ssValue >> strValue;
            const CTxDestination& dest{DecodeDestination(strAddress)};
            if (strKey.compare("used") == 0) {
                // Load "used" key indicating if an IsMine address has
                // previously been spent from with avoid_reuse option enabled.
                // The strValue is not used for anything currently, but could
                // hold more information in the future. Current values are just
                // "1" or "p" for present (which was written prior to
                // f5ba424cd44619d9b9be88b8593d69a7ba96db26).
                pwallet->LoadAddressPreviouslySpent(dest);
            } else if (strKey.compare(0, 2, "rr") == 0) {
                // Load "rr##" keys where ## is a decimal number, and strValue
                // is a serialized RecentRequestEntry object.
                pwallet->LoadAddressReceiveRequest(dest, strKey.substr(2), strValue);
            }
        } else if (strType == DBKeys::HDCHAIN) {
        } else if (strType == DBKeys::OLD_KEY) {
            strErr = "Found unsupported 'wkey' record, try loading with version 0.18";
            return false;
        } else if (strType == DBKeys::ACTIVEEXTERNALSPK || strType == DBKeys::ACTIVEINTERNALSPK) {
            uint8_t type;
            ssKey >> type;
            uint256 id;
            ssValue >> id;

            bool internal = strType == DBKeys::ACTIVEINTERNALSPK;
            auto& spk_mans = internal ? wss.m_active_internal_spks : wss.m_active_external_spks;
            if (spk_mans.count(static_cast<OutputType>(type)) > 0) {
                strErr = "Multiple ScriptPubKeyMans specified for a single type";
                return false;
            }
            spk_mans[static_cast<OutputType>(type)] = id;
        } else if (strType == DBKeys::WALLETDESCRIPTOR) {
            uint256 id;
            ssKey >> id;
            WalletDescriptor desc;
            try {
                ssValue >> desc;
            } catch (const std::ios_base::failure& e) {
                strErr = e.what();
                wss.descriptor_unknown = true;
                return false;
            }
            if (wss.m_descriptor_caches.count(id) == 0) {
                wss.m_descriptor_caches[id] = DescriptorCache();
            }
            pwallet->LoadDescriptorScriptPubKeyMan(id, desc);
        } else if (strType == DBKeys::WALLETDESCRIPTORCACHE) {
            bool parent = true;
            uint256 desc_id;
            uint32_t key_exp_index;
            uint32_t der_index;
            ssKey >> desc_id;
            ssKey >> key_exp_index;

            // if the der_index exists, it's a derived xpub
            try
            {
                ssKey >> der_index;
                parent = false;
            }
            catch (...) {}

            std::vector<unsigned char> ser_xpub(BIP32_EXTKEY_SIZE);
            ssValue >> ser_xpub;
            CExtPubKey xpub;
            xpub.Decode(ser_xpub.data());
            if (parent) {
                wss.m_descriptor_caches[desc_id].CacheParentExtPubKey(key_exp_index, xpub);
            } else {
                wss.m_descriptor_caches[desc_id].CacheDerivedExtPubKey(key_exp_index, der_index, xpub);
            }
        } else if (strType == DBKeys::WALLETDESCRIPTORLHCACHE) {
            uint256 desc_id;
            uint32_t key_exp_index;
            ssKey >> desc_id;
            ssKey >> key_exp_index;

            std::vector<unsigned char> ser_xpub(BIP32_EXTKEY_SIZE);
            ssValue >> ser_xpub;
            CExtPubKey xpub;
            xpub.Decode(ser_xpub.data());
            wss.m_descriptor_caches[desc_id].CacheLastHardenedExtPubKey(key_exp_index, xpub);
        } else if (strType == DBKeys::WALLETDESCRIPTORKEY) {
            uint256 desc_id;
            CPubKey pubkey;
            ssKey >> desc_id;
            ssKey >> pubkey;
            if (!pubkey.IsValid())
            {
                strErr = "Error reading wallet database: CPubKey corrupt";
                return false;
            }
            CKey key;
            CPrivKey pkey;
            uint256 hash;

            wss.nKeys++;
            ssValue >> pkey;
            ssValue >> hash;

            // hash pubkey/privkey to accelerate wallet load
            std::vector<unsigned char> to_hash;
            to_hash.reserve(pubkey.size() + pkey.size());
            to_hash.insert(to_hash.end(), pubkey.begin(), pubkey.end());
            to_hash.insert(to_hash.end(), pkey.begin(), pkey.end());

            if (Hash(to_hash) != hash)
            {
                strErr = "Error reading wallet database: CPubKey/CPrivKey corrupt";
                return false;
            }

            if (!key.Load(pkey, pubkey, true))
            {
                strErr = "Error reading wallet database: CPrivKey corrupt";
                return false;
            }
            wss.m_descriptor_keys.insert(std::make_pair(std::make_pair(desc_id, pubkey.GetID()), key));
        } else if (strType == DBKeys::WALLETDESCRIPTORCKEY) {
            uint256 desc_id;
            CPubKey pubkey;
            ssKey >> desc_id;
            ssKey >> pubkey;
            if (!pubkey.IsValid())
            {
                strErr = "Error reading wallet database: CPubKey corrupt";
                return false;
            }
            std::vector<unsigned char> privkey;
            ssValue >> privkey;
            wss.nCKeys++;

            wss.m_descriptor_crypt_keys.insert(std::make_pair(std::make_pair(desc_id, pubkey.GetID()), std::make_pair(pubkey, privkey)));
        } else if (strType == DBKeys::LOCKED_UTXO) {
            uint256 hash;
            uint32_t n;
            ssKey >> hash;
            ssKey >> n;
            pwallet->LockCoin(COutPoint(hash, n));
        } else if (strType != DBKeys::BESTBLOCK && strType != DBKeys::BESTBLOCK_NOMERKLE &&
                   strType != DBKeys::MINVERSION && strType != DBKeys::ACENTRY &&
                   strType != DBKeys::VERSION && strType != DBKeys::SETTINGS &&
                   strType != DBKeys::FLAGS) {
            wss.m_unknown_records++;
        }
    } catch (const std::exception& e) {
        if (strErr.empty()) {
            strErr = e.what();
        }
        return false;
    } catch (...) {
        if (strErr.empty()) {
            strErr = "Caught unknown exception in ReadKeyValue";
        }
        return false;
    }
    return true;
}

static DBErrors LoadMinVersion(CWallet* pwallet, DatabaseBatch& batch) EXCLUSIVE_LOCKS_REQUIRED(pwallet->cs_wallet)
{
    AssertLockHeld(pwallet->cs_wallet);
    int nMinVersion = 0;
    if (batch.Read(DBKeys::MINVERSION, nMinVersion)) {
        if (nMinVersion > FEATURE_LATEST)
            return DBErrors::TOO_NEW;
        pwallet->LoadMinVersion(nMinVersion);
    }
    return DBErrors::LOAD_OK;
}

static DBErrors LoadWalletFlags(CWallet* pwallet, DatabaseBatch& batch) EXCLUSIVE_LOCKS_REQUIRED(pwallet->cs_wallet)
{
    AssertLockHeld(pwallet->cs_wallet);
    uint64_t flags;
    if (batch.Read(DBKeys::FLAGS, flags)) {
        if (!pwallet->LoadWalletFlags(flags)) {
            pwallet->WalletLogPrintf("Error reading wallet database: Unknown non-tolerable wallet flags found\n");
            return DBErrors::TOO_NEW;
        }
    }
    return DBErrors::LOAD_OK;
}

struct LoadResult
{
    DBErrors m_result{DBErrors::LOAD_OK};
    int m_records{0};
};

using LoadFunc = std::function<DBErrors(CWallet* pwallet, DataStream& key, CDataStream& value, std::string& err)>;
static LoadResult LoadRecords(CWallet* pwallet, DatabaseBatch& batch, const std::string& key, LoadFunc load_func)
{
    LoadResult result;
    DataStream ssKey;
    CDataStream ssValue(SER_DISK, CLIENT_VERSION);

    DataStream prefix;
    prefix << key;
    std::unique_ptr<DatabaseCursor> cursor = batch.GetNewPrefixCursor(prefix);
    if (!cursor) {
        pwallet->WalletLogPrintf("Error getting database cursor for '%s' records\n", key);
        result.m_result = DBErrors::CORRUPT;
        return result;
    }

    while (true) {
        DatabaseCursor::Status status = cursor->Next(ssKey, ssValue);
        if (status == DatabaseCursor::Status::DONE) {
            break;
        } else if (status == DatabaseCursor::Status::FAIL) {
            pwallet->WalletLogPrintf("Error reading next '%s' record for wallet database\n", key);
            result.m_result = DBErrors::CORRUPT;
            return result;
        }
        std::string type;
        ssKey >> type;
        assert(type == key);
        std::string error;
        DBErrors record_res = load_func(pwallet, ssKey, ssValue, error);
        if (record_res != DBErrors::LOAD_OK) {
            pwallet->WalletLogPrintf("%s\n", error);
        }
        result.m_result = std::max(result.m_result, record_res);
        ++result.m_records;
    }
    return result;
}

static DBErrors LoadLegacyWalletRecords(CWallet* pwallet, DatabaseBatch& batch, int last_client) EXCLUSIVE_LOCKS_REQUIRED(pwallet->cs_wallet)
{
    AssertLockHeld(pwallet->cs_wallet);
    DBErrors result = DBErrors::LOAD_OK;

    // Make sure descriptor wallets don't have any legacy records
    if (pwallet->IsWalletFlagSet(WALLET_FLAG_DESCRIPTORS)) {
        for (const auto& type : DBKeys::LEGACY_TYPES) {
            DataStream key;
            CDataStream value(SER_DISK, CLIENT_VERSION);

            DataStream prefix;
            prefix << type;
            std::unique_ptr<DatabaseCursor> cursor = batch.GetNewPrefixCursor(prefix);
            if (!cursor) {
                pwallet->WalletLogPrintf("Error getting database cursor for '%s' records\n", type);
                return DBErrors::CORRUPT;
            }

            DatabaseCursor::Status status = cursor->Next(key, value);
            if (status != DatabaseCursor::Status::DONE) {
                pwallet->WalletLogPrintf("Error: Unexpected legacy entry found in descriptor wallet %s. The wallet might have been tampered with or created with malicious intent.\n", pwallet->GetName());
                return DBErrors::UNEXPECTED_LEGACY_ENTRY;
            }
        }

        return DBErrors::LOAD_OK;
    }

    // Load HD Chain
    // Note: There should only be one HDCHAIN record with no data following the type
    LoadResult hd_chain_res = LoadRecords(pwallet, batch, DBKeys::HDCHAIN,
        [] (CWallet* pwallet, DataStream& key, CDataStream& value, std::string& err) {
        return LoadHDChain(pwallet, value, err) ? DBErrors:: LOAD_OK : DBErrors::CORRUPT;
    });
    result = std::max(result, hd_chain_res.m_result);

    // Load unencrypted keys
    LoadResult key_res = LoadRecords(pwallet, batch, DBKeys::KEY,
        [] (CWallet* pwallet, DataStream& key, CDataStream& value, std::string& err) {
        return LoadKey(pwallet, key, value, err) ? DBErrors::LOAD_OK : DBErrors::CORRUPT;
    });
    result = std::max(result, key_res.m_result);

    // Load encrypted keys
    LoadResult ckey_res = LoadRecords(pwallet, batch, DBKeys::CRYPTED_KEY,
        [] (CWallet* pwallet, DataStream& key, CDataStream& value, std::string& err) {
        return LoadCryptedKey(pwallet, key, value, err) ? DBErrors::LOAD_OK : DBErrors::CORRUPT;
    });
    result = std::max(result, ckey_res.m_result);

    // Load scripts
    LoadResult script_res = LoadRecords(pwallet, batch, DBKeys::CSCRIPT,
        [] (CWallet* pwallet, DataStream& key, CDataStream& value, std::string& strErr) {
        uint160 hash;
        key >> hash;
        CScript script;
        value >> script;
        if (!pwallet->GetOrCreateLegacyScriptPubKeyMan()->LoadCScript(script))
        {
            strErr = "Error reading wallet database: LegacyScriptPubKeyMan::LoadCScript failed";
            return DBErrors::NONCRITICAL_ERROR;
        }
        return DBErrors::LOAD_OK;
    });
    result = std::max(result, script_res.m_result);

    // Check whether rewrite is needed
    if (ckey_res.m_records > 0) {
        // Rewrite encrypted wallets of versions 0.4.0 and 0.5.0rc:
        if (last_client == 40000 || last_client == 50000) result = std::max(result, DBErrors::NEED_REWRITE);
    }

    // Load keymeta
    std::map<uint160, CHDChain> hd_chains;
    LoadResult keymeta_res = LoadRecords(pwallet, batch, DBKeys::KEYMETA,
        [&hd_chains] (CWallet* pwallet, DataStream& key, CDataStream& value, std::string& strErr) {
        CPubKey vchPubKey;
        key >> vchPubKey;
        CKeyMetadata keyMeta;
        value >> keyMeta;
        pwallet->GetOrCreateLegacyScriptPubKeyMan()->LoadKeyMetadata(vchPubKey.GetID(), keyMeta);

        // Extract some CHDChain info from this metadata if it has any
        if (keyMeta.nVersion >= CKeyMetadata::VERSION_WITH_HDDATA && !keyMeta.hd_seed_id.IsNull() && keyMeta.hdKeypath.size() > 0) {
            // Get the path from the key origin or from the path string
            // Not applicable when path is "s" or "m" as those indicate a seed
            // See https://github.com/bitcoin/bitcoin/pull/12924
            bool internal = false;
            uint32_t index = 0;
            if (keyMeta.hdKeypath != "s" && keyMeta.hdKeypath != "m") {
                std::vector<uint32_t> path;
                if (keyMeta.has_key_origin) {
                    // We have a key origin, so pull it from its path vector
                    path = keyMeta.key_origin.path;
                } else {
                    // No key origin, have to parse the string
                    if (!ParseHDKeypath(keyMeta.hdKeypath, path)) {
                        strErr = "Error reading wallet database: keymeta with invalid HD keypath";
                        return DBErrors::NONCRITICAL_ERROR;
                    }
                }

                // Extract the index and internal from the path
                // Path string is m/0'/k'/i'
                // Path vector is [0', k', i'] (but as ints OR'd with the hardened bit
                // k == 0 for external, 1 for internal. i is the index
                if (path.size() != 3) {
                    strErr = "Error reading wallet database: keymeta found with unexpected path";
                    return DBErrors::NONCRITICAL_ERROR;
                }
                if (path[0] != 0x80000000) {
                    strErr = strprintf("Unexpected path index of 0x%08x (expected 0x80000000) for the element at index 0", path[0]);
                    return DBErrors::NONCRITICAL_ERROR;
                }
                if (path[1] != 0x80000000 && path[1] != (1 | 0x80000000)) {
                    strErr = strprintf("Unexpected path index of 0x%08x (expected 0x80000000 or 0x80000001) for the element at index 1", path[1]);
                    return DBErrors::NONCRITICAL_ERROR;
                }
                if ((path[2] & 0x80000000) == 0) {
                    strErr = strprintf("Unexpected path index of 0x%08x (expected to be greater than or equal to 0x80000000)", path[2]);
                    return DBErrors::NONCRITICAL_ERROR;
                }
                internal = path[1] == (1 | 0x80000000);
                index = path[2] & ~0x80000000;
            }

            // Insert a new CHDChain, or get the one that already exists
            auto [ins, inserted] = hd_chains.emplace(keyMeta.hd_seed_id, CHDChain());
            CHDChain& chain = ins->second;
            if (inserted) {
                // For new chains, we want to default to VERSION_HD_BASE until we see an internal
                chain.nVersion = CHDChain::VERSION_HD_BASE;
                chain.seed_id = keyMeta.hd_seed_id;
            }
            if (internal) {
                chain.nVersion = CHDChain::VERSION_HD_CHAIN_SPLIT;
                chain.nInternalChainCounter = std::max(chain.nInternalChainCounter, index + 1);
            } else {
                chain.nExternalChainCounter = std::max(chain.nExternalChainCounter, index + 1);
            }
        }
        return DBErrors::LOAD_OK;
    });
    result = std::max(result, keymeta_res.m_result);

    // Set inactive chains
    if (!hd_chains.empty()) {
        LegacyScriptPubKeyMan* legacy_spkm = pwallet->GetLegacyScriptPubKeyMan();
        if (legacy_spkm) {
            for (const auto& [hd_seed_id, chain] : hd_chains) {
                if (hd_seed_id != legacy_spkm->GetHDChain().seed_id) {
                    legacy_spkm->AddInactiveHDChain(chain);
                }
            }
        } else {
            pwallet->WalletLogPrintf("Inactive HD Chains found but no Legacy ScriptPubKeyMan\n");
            result = DBErrors::CORRUPT;
        }
    }

    // Load watchonly scripts
    LoadResult watch_script_res = LoadRecords(pwallet, batch, DBKeys::WATCHS,
        [] (CWallet* pwallet, DataStream& key, CDataStream& value, std::string& err) {
        CScript script;
        key >> script;
        uint8_t fYes;
        value >> fYes;
        if (fYes == '1') {
            pwallet->GetOrCreateLegacyScriptPubKeyMan()->LoadWatchOnly(script);
        }
        return DBErrors::LOAD_OK;
    });
    result = std::max(result, watch_script_res.m_result);

    // Load watchonly meta
    LoadResult watch_meta_res = LoadRecords(pwallet, batch, DBKeys::WATCHMETA,
        [] (CWallet* pwallet, DataStream& key, CDataStream& value, std::string& err) {
        CScript script;
        key >> script;
        CKeyMetadata keyMeta;
        value >> keyMeta;
        pwallet->GetOrCreateLegacyScriptPubKeyMan()->LoadScriptMetadata(CScriptID(script), keyMeta);
        return DBErrors::LOAD_OK;
    });
    result = std::max(result, watch_meta_res.m_result);

    // Load keypool
    LoadResult pool_res = LoadRecords(pwallet, batch, DBKeys::POOL,
        [] (CWallet* pwallet, DataStream& key, CDataStream& value, std::string& err) {
        int64_t nIndex;
        key >> nIndex;
        CKeyPool keypool;
        value >> keypool;
        pwallet->GetOrCreateLegacyScriptPubKeyMan()->LoadKeyPool(nIndex, keypool);
        return DBErrors::LOAD_OK;
    });
    result = std::max(result, pool_res.m_result);

    if (result <= DBErrors::NONCRITICAL_ERROR) {
        // Only do logging and time first key update if there were no critical errors
        pwallet->WalletLogPrintf("Legacy Wallet Keys: %u plaintext, %u encrypted, %u w/ metadata, %u total.\n",
               key_res.m_records, ckey_res.m_records, keymeta_res.m_records, key_res.m_records + ckey_res.m_records);

        // nTimeFirstKey is only reliable if all keys have metadata
        if (pwallet->IsLegacy() && (key_res.m_records + ckey_res.m_records + watch_script_res.m_records) != (keymeta_res.m_records + watch_meta_res.m_records)) {
            auto spk_man = pwallet->GetOrCreateLegacyScriptPubKeyMan();
            if (spk_man) {
                LOCK(spk_man->cs_KeyStore);
                spk_man->UpdateTimeFirstKey(1);
            }
        }
    }

    return result;
}

DBErrors WalletBatch::LoadWallet(CWallet* pwallet)
{
    CWalletScanState wss;
    bool fNoncriticalErrors = false;
    bool rescan_required = false;
    DBErrors result = DBErrors::LOAD_OK;

    LOCK(pwallet->cs_wallet);

    // Last client version to open this wallet
    int last_client = CLIENT_VERSION;
    bool has_last_client = m_batch->Read(DBKeys::VERSION, last_client);
    pwallet->WalletLogPrintf("Wallet file version = %d, last client version = %d\n", pwallet->GetVersion(), last_client);

    try {
        if ((result = LoadMinVersion(pwallet, *m_batch)) != DBErrors::LOAD_OK) return result;

        // Load wallet flags, so they are known when processing other records.
        // The FLAGS key is absent during wallet creation.
        if ((result = LoadWalletFlags(pwallet, *m_batch)) != DBErrors::LOAD_OK) return result;

#ifndef ENABLE_EXTERNAL_SIGNER
        if (pwallet->IsWalletFlagSet(WALLET_FLAG_EXTERNAL_SIGNER)) {
            pwallet->WalletLogPrintf("Error: External signer wallet being loaded without external signer support compiled\n");
            return DBErrors::EXTERNAL_SIGNER_SUPPORT_REQUIRED;
        }
#endif

        // Load legacy wallet keys
        result = std::max(LoadLegacyWalletRecords(pwallet, *m_batch, last_client), result);

        // Get cursor
        std::unique_ptr<DatabaseCursor> cursor = m_batch->GetNewCursor();
        if (!cursor)
        {
            pwallet->WalletLogPrintf("Error getting wallet database cursor\n");
            return DBErrors::CORRUPT;
        }

        while (true)
        {
            // Read next record
            DataStream ssKey{};
            CDataStream ssValue(SER_DISK, CLIENT_VERSION);
            DatabaseCursor::Status status = cursor->Next(ssKey, ssValue);
            if (status == DatabaseCursor::Status::DONE) {
                break;
            } else if (status == DatabaseCursor::Status::FAIL) {
                cursor.reset();
                pwallet->WalletLogPrintf("Error reading next record from wallet database\n");
                return DBErrors::CORRUPT;
            }

            // Try to be tolerant of single corrupt records:
            std::string strType, strErr;
            if (!ReadKeyValue(pwallet, ssKey, ssValue, wss, strType, strErr))
            {
                // losing keys is considered a catastrophic error, anything else
                // we assume the user can live with:
                if (strType == DBKeys::MASTER_KEY ||
                    strType == DBKeys::DEFAULTKEY) {
                    result = DBErrors::CORRUPT;
                } else if (wss.tx_corrupt) {
                    pwallet->WalletLogPrintf("Error: Corrupt transaction found. This can be fixed by removing transactions from wallet and rescanning.\n");
                    // Set tx_corrupt back to false so that the error is only printed once (per corrupt tx)
                    wss.tx_corrupt = false;
                    result = DBErrors::CORRUPT;
                } else if (wss.descriptor_unknown) {
                    strErr = strprintf("Error: Unrecognized descriptor found in wallet %s. ", pwallet->GetName());
                    strErr += (last_client > CLIENT_VERSION) ? "The wallet might had been created on a newer version. " :
                            "The database might be corrupted or the software version is not compatible with one of your wallet descriptors. ";
                    strErr += "Please try running the latest software version";
                    pwallet->WalletLogPrintf("%s\n", strErr);
                    return DBErrors::UNKNOWN_DESCRIPTOR;
                } else {
                    // Leave other errors alone, if we try to fix them we might make things worse.
                    fNoncriticalErrors = true; // ... but do warn the user there is something wrong.
                    if (strType == DBKeys::TX)
                        // Rescan if there is a bad transaction record:
                        rescan_required = true;
                }
            }
            if (!strErr.empty())
                pwallet->WalletLogPrintf("%s\n", strErr);
        }
    } catch (...) {
        // Exceptions that can be ignored or treated as non-critical are handled by the individual loading functions.
        // Any uncaught exceptions will be caught here and treated as critical.
        result = DBErrors::CORRUPT;
    }

    // Set the active ScriptPubKeyMans
    for (auto spk_man_pair : wss.m_active_external_spks) {
        pwallet->LoadActiveScriptPubKeyMan(spk_man_pair.second, spk_man_pair.first, /*internal=*/false);
    }
    for (auto spk_man_pair : wss.m_active_internal_spks) {
        pwallet->LoadActiveScriptPubKeyMan(spk_man_pair.second, spk_man_pair.first, /*internal=*/true);
    }

    // Set the descriptor caches
    for (const auto& desc_cache_pair : wss.m_descriptor_caches) {
        auto spk_man = pwallet->GetScriptPubKeyMan(desc_cache_pair.first);
        assert(spk_man);
        ((DescriptorScriptPubKeyMan*)spk_man)->SetCache(desc_cache_pair.second);
    }

    // Set the descriptor keys
    for (const auto& desc_key_pair : wss.m_descriptor_keys) {
        auto spk_man = pwallet->GetScriptPubKeyMan(desc_key_pair.first.first);
        ((DescriptorScriptPubKeyMan*)spk_man)->AddKey(desc_key_pair.first.second, desc_key_pair.second);
    }
    for (const auto& desc_key_pair : wss.m_descriptor_crypt_keys) {
        auto spk_man = pwallet->GetScriptPubKeyMan(desc_key_pair.first.first);
        ((DescriptorScriptPubKeyMan*)spk_man)->AddCryptedKey(desc_key_pair.first.second, desc_key_pair.second.first, desc_key_pair.second.second);
    }

    if (rescan_required && result == DBErrors::LOAD_OK) {
        result = DBErrors::NEED_RESCAN;
    } else if (fNoncriticalErrors && result == DBErrors::LOAD_OK) {
        result = DBErrors::NONCRITICAL_ERROR;
    }

    // Any wallet corruption at all: skip any rewriting or
    // upgrading, we don't want to make it worse.
    if (result != DBErrors::LOAD_OK)
        return result;

    pwallet->WalletLogPrintf("Keys: %u plaintext, %u encrypted, %u w/ metadata, %u total. Unknown wallet records: %u\n",
           wss.nKeys, wss.nCKeys, wss.nKeyMeta, wss.nKeys + wss.nCKeys, wss.m_unknown_records);

    for (const uint256& hash : wss.vWalletUpgrade)
        WriteTx(pwallet->mapWallet.at(hash));

    if (!has_last_client || last_client != CLIENT_VERSION) // Update
        m_batch->Write(DBKeys::VERSION, CLIENT_VERSION);

    if (wss.fAnyUnordered)
        result = pwallet->ReorderTransactions();

    // Upgrade all of the wallet keymetadata to have the hd master key id
    // This operation is not atomic, but if it fails, updated entries are still backwards compatible with older software
    try {
        pwallet->UpgradeKeyMetadata();
    } catch (...) {
        result = DBErrors::CORRUPT;
    }

    // Upgrade all of the descriptor caches to cache the last hardened xpub
    // This operation is not atomic, but if it fails, only new entries are added so it is backwards compatible
    try {
        pwallet->UpgradeDescriptorCache();
    } catch (...) {
        result = DBErrors::CORRUPT;
    }

    return result;
}

DBErrors WalletBatch::FindWalletTxHashes(std::vector<uint256>& tx_hashes)
{
    DBErrors result = DBErrors::LOAD_OK;

    try {
        int nMinVersion = 0;
        if (m_batch->Read(DBKeys::MINVERSION, nMinVersion)) {
            if (nMinVersion > FEATURE_LATEST)
                return DBErrors::TOO_NEW;
        }

        // Get cursor
        std::unique_ptr<DatabaseCursor> cursor = m_batch->GetNewCursor();
        if (!cursor)
        {
            LogPrintf("Error getting wallet database cursor\n");
            return DBErrors::CORRUPT;
        }

        while (true)
        {
            // Read next record
            DataStream ssKey{};
            DataStream ssValue{};
            DatabaseCursor::Status status = cursor->Next(ssKey, ssValue);
            if (status == DatabaseCursor::Status::DONE) {
                break;
            } else if (status == DatabaseCursor::Status::FAIL) {
                LogPrintf("Error reading next record from wallet database\n");
                return DBErrors::CORRUPT;
            }

            std::string strType;
            ssKey >> strType;
            if (strType == DBKeys::TX) {
                uint256 hash;
                ssKey >> hash;
                tx_hashes.push_back(hash);
            }
        }
    } catch (...) {
        result = DBErrors::CORRUPT;
    }

    return result;
}

DBErrors WalletBatch::ZapSelectTx(std::vector<uint256>& vTxHashIn, std::vector<uint256>& vTxHashOut)
{
    // build list of wallet TX hashes
    std::vector<uint256> vTxHash;
    DBErrors err = FindWalletTxHashes(vTxHash);
    if (err != DBErrors::LOAD_OK) {
        return err;
    }

    std::sort(vTxHash.begin(), vTxHash.end());
    std::sort(vTxHashIn.begin(), vTxHashIn.end());

    // erase each matching wallet TX
    bool delerror = false;
    std::vector<uint256>::iterator it = vTxHashIn.begin();
    for (const uint256& hash : vTxHash) {
        while (it < vTxHashIn.end() && (*it) < hash) {
            it++;
        }
        if (it == vTxHashIn.end()) {
            break;
        }
        else if ((*it) == hash) {
            if(!EraseTx(hash)) {
                LogPrint(BCLog::WALLETDB, "Transaction was found for deletion but returned database error: %s\n", hash.GetHex());
                delerror = true;
            }
            vTxHashOut.push_back(hash);
        }
    }

    if (delerror) {
        return DBErrors::CORRUPT;
    }
    return DBErrors::LOAD_OK;
}

void MaybeCompactWalletDB(WalletContext& context)
{
    static std::atomic<bool> fOneThread(false);
    if (fOneThread.exchange(true)) {
        return;
    }

    for (const std::shared_ptr<CWallet>& pwallet : GetWallets(context)) {
        WalletDatabase& dbh = pwallet->GetDatabase();

        unsigned int nUpdateCounter = dbh.nUpdateCounter;

        if (dbh.nLastSeen != nUpdateCounter) {
            dbh.nLastSeen = nUpdateCounter;
            dbh.nLastWalletUpdate = GetTime();
        }

        if (dbh.nLastFlushed != nUpdateCounter && GetTime() - dbh.nLastWalletUpdate >= 2) {
            if (dbh.PeriodicFlush()) {
                dbh.nLastFlushed = nUpdateCounter;
            }
        }
    }

    fOneThread = false;
}

bool WalletBatch::WriteAddressPreviouslySpent(const CTxDestination& dest, bool previously_spent)
{
    auto key{std::make_pair(DBKeys::DESTDATA, std::make_pair(EncodeDestination(dest), std::string("used")))};
    return previously_spent ? WriteIC(key, std::string("1")) : EraseIC(key);
}

bool WalletBatch::WriteAddressReceiveRequest(const CTxDestination& dest, const std::string& id, const std::string& receive_request)
{
    return WriteIC(std::make_pair(DBKeys::DESTDATA, std::make_pair(EncodeDestination(dest), "rr" + id)), receive_request);
}

bool WalletBatch::EraseAddressReceiveRequest(const CTxDestination& dest, const std::string& id)
{
    return EraseIC(std::make_pair(DBKeys::DESTDATA, std::make_pair(EncodeDestination(dest), "rr" + id)));
}

bool WalletBatch::EraseAddressData(const CTxDestination& dest)
{
    DataStream prefix;
    prefix << DBKeys::DESTDATA << EncodeDestination(dest);
    return m_batch->ErasePrefix(prefix);
}

bool WalletBatch::WriteHDChain(const CHDChain& chain)
{
    return WriteIC(DBKeys::HDCHAIN, chain);
}

bool WalletBatch::WriteWalletFlags(const uint64_t flags)
{
    return WriteIC(DBKeys::FLAGS, flags);
}

bool WalletBatch::EraseRecords(const std::unordered_set<std::string>& types)
{
    // Begin db txn
    if (!m_batch->TxnBegin()) return false;

    // Get cursor
    std::unique_ptr<DatabaseCursor> cursor = m_batch->GetNewCursor();
    if (!cursor)
    {
        return false;
    }

    // Iterate the DB and look for any records that have the type prefixes
    while (true) {
        // Read next record
        DataStream key{};
        DataStream value{};
        DatabaseCursor::Status status = cursor->Next(key, value);
        if (status == DatabaseCursor::Status::DONE) {
            break;
        } else if (status == DatabaseCursor::Status::FAIL) {
            cursor.reset(nullptr);
            m_batch->TxnAbort(); // abort db txn
            return false;
        }

        // Make a copy of key to avoid data being deleted by the following read of the type
        Span<const unsigned char> key_data = MakeUCharSpan(key);

        std::string type;
        key >> type;

        if (types.count(type) > 0) {
            if (!m_batch->Erase(key_data)) {
                cursor.reset(nullptr);
                m_batch->TxnAbort();
                return false; // erase failed
            }
        }
    }
    // Finish db txn
    cursor.reset(nullptr);
    return m_batch->TxnCommit();
}

bool WalletBatch::TxnBegin()
{
    return m_batch->TxnBegin();
}

bool WalletBatch::TxnCommit()
{
    return m_batch->TxnCommit();
}

bool WalletBatch::TxnAbort()
{
    return m_batch->TxnAbort();
}

std::unique_ptr<WalletDatabase> MakeDatabase(const fs::path& path, const DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error)
{
    bool exists;
    try {
        exists = fs::symlink_status(path).type() != fs::file_type::not_found;
    } catch (const fs::filesystem_error& e) {
        error = Untranslated(strprintf("Failed to access database path '%s': %s", fs::PathToString(path), fsbridge::get_filesystem_error_message(e)));
        status = DatabaseStatus::FAILED_BAD_PATH;
        return nullptr;
    }

    std::optional<DatabaseFormat> format;
    if (exists) {
        if (IsBDBFile(BDBDataFile(path))) {
            format = DatabaseFormat::BERKELEY;
        }
        if (IsSQLiteFile(SQLiteDataFile(path))) {
            if (format) {
                error = Untranslated(strprintf("Failed to load database path '%s'. Data is in ambiguous format.", fs::PathToString(path)));
                status = DatabaseStatus::FAILED_BAD_FORMAT;
                return nullptr;
            }
            format = DatabaseFormat::SQLITE;
        }
    } else if (options.require_existing) {
        error = Untranslated(strprintf("Failed to load database path '%s'. Path does not exist.", fs::PathToString(path)));
        status = DatabaseStatus::FAILED_NOT_FOUND;
        return nullptr;
    }

    if (!format && options.require_existing) {
        error = Untranslated(strprintf("Failed to load database path '%s'. Data is not in recognized format.", fs::PathToString(path)));
        status = DatabaseStatus::FAILED_BAD_FORMAT;
        return nullptr;
    }

    if (format && options.require_create) {
        error = Untranslated(strprintf("Failed to create database path '%s'. Database already exists.", fs::PathToString(path)));
        status = DatabaseStatus::FAILED_ALREADY_EXISTS;
        return nullptr;
    }

    // A db already exists so format is set, but options also specifies the format, so make sure they agree
    if (format && options.require_format && format != options.require_format) {
        error = Untranslated(strprintf("Failed to load database path '%s'. Data is not in required format.", fs::PathToString(path)));
        status = DatabaseStatus::FAILED_BAD_FORMAT;
        return nullptr;
    }

    // Format is not set when a db doesn't already exist, so use the format specified by the options if it is set.
    if (!format && options.require_format) format = options.require_format;

    // If the format is not specified or detected, choose the default format based on what is available. We prefer BDB over SQLite for now.
    if (!format) {
#ifdef USE_SQLITE
        format = DatabaseFormat::SQLITE;
#endif
#ifdef USE_BDB
        format = DatabaseFormat::BERKELEY;
#endif
    }

    if (format == DatabaseFormat::SQLITE) {
#ifdef USE_SQLITE
        return MakeSQLiteDatabase(path, options, status, error);
#endif
        error = Untranslated(strprintf("Failed to open database path '%s'. Build does not support SQLite database format.", fs::PathToString(path)));
        status = DatabaseStatus::FAILED_BAD_FORMAT;
        return nullptr;
    }

#ifdef USE_BDB
    return MakeBerkeleyDatabase(path, options, status, error);
#endif
    error = Untranslated(strprintf("Failed to open database path '%s'. Build does not support Berkeley DB database format.", fs::PathToString(path)));
    status = DatabaseStatus::FAILED_BAD_FORMAT;
    return nullptr;
}
} // namespace wallet
