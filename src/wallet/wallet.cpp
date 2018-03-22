// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2014-2020 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/wallet.h>

#include <checkpoints.h>
#include <chain.h>
#include <wallet/coinselection.h>
#include <consensus/consensus.h>
#include <consensus/validation.h>
#include <fs.h>
#include <init.h>
#include <key.h>
#include <key_io.h>
#include <keystore.h>
#include <validation.h>
#include <net.h>
#include <policy/fees.h>
#include <policy/policy.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <script/sign.h>
#include <timedata.h>
#include <txmempool.h>
#include <utilmoneystr.h>
#include <wallet/fees.h>

#include <coinjoin/coinjoin-client.h>
#include <coinjoin/coinjoin-client-options.h>
#include <governance/governance.h>
#include <keepass.h>

#include <evo/providertx.h>

#include <llmq/quorums_instantsend.h>
#include <llmq/quorums_chainlocks.h>

#include <assert.h>
#include <future>

#include <boost/algorithm/string/replace.hpp>

static CCriticalSection cs_wallets;
static std::vector<std::shared_ptr<CWallet>> vpwallets GUARDED_BY(cs_wallets);

bool AddWallet(const std::shared_ptr<CWallet>& wallet)
{
    LOCK(cs_wallets);
    assert(wallet);
    std::vector<std::shared_ptr<CWallet>>::const_iterator i = std::find(vpwallets.begin(), vpwallets.end(), wallet);
    if (i != vpwallets.end()) return false;
    coinJoinClientManagers.emplace(std::make_pair(wallet->GetName(), std::make_shared<CCoinJoinClientManager>(*wallet)));
    vpwallets.push_back(wallet);
    return true;
}

bool RemoveWallet(const std::shared_ptr<CWallet>& wallet)
{
    LOCK(cs_wallets);
    assert(wallet);
    std::vector<std::shared_ptr<CWallet>>::iterator i = std::find(vpwallets.begin(), vpwallets.end(), wallet);
    if (i == vpwallets.end()) return false;
    vpwallets.erase(i);
    auto it = coinJoinClientManagers.find(wallet->GetName());
    coinJoinClientManagers.erase(it);
    return true;
}

bool HasWallets()
{
    LOCK(cs_wallets);
    return !vpwallets.empty();
}

std::vector<std::shared_ptr<CWallet>> GetWallets()
{
    LOCK(cs_wallets);
    return vpwallets;
}

std::shared_ptr<CWallet> GetWallet(const std::string& name)
{
    LOCK(cs_wallets);
    for (const std::shared_ptr<CWallet>& wallet : vpwallets) {
        if (wallet->GetName() == name) return wallet;
    }
    return nullptr;
}

/** Transaction fee set by the user */
CFeeRate payTxFee(DEFAULT_TRANSACTION_FEE);
unsigned int nTxConfirmTarget = DEFAULT_TX_CONFIRM_TARGET;
bool bSpendZeroConfChange = DEFAULT_SPEND_ZEROCONF_CHANGE;

/**
 * Fees smaller than this (in duffs) are considered zero fee (for transaction creation)
 * Override with -mintxfee
 */
CFeeRate CWallet::minTxFee = CFeeRate(DEFAULT_TRANSACTION_MINFEE);
/**
 * If fee estimation does not have enough data to provide estimates, use this fee instead.
 * Has no effect if not using fee estimation
 * Override with -fallbackfee
 */
CFeeRate CWallet::fallbackFee = CFeeRate(DEFAULT_FALLBACK_FEE);

CFeeRate CWallet::m_discard_rate = CFeeRate(DEFAULT_DISCARD_FEE);

// Custom deleter for shared_ptr<CWallet>.
static void ReleaseWallet(CWallet* wallet)
{
    LogPrintf("Releasing wallet %s\n", wallet->GetName());
    wallet->BlockUntilSyncedToCurrentChain();
    wallet->Flush();
    delete wallet;
}

const uint256 CMerkleTx::ABANDON_HASH(uint256S("0000000000000000000000000000000000000000000000000000000000000001"));

/** @defgroup mapWallet
 *
 * @{
 */

std::string COutput::ToString() const
{
    return strprintf("COutput(%s, %d, %d) [%s]", tx->GetHash().ToString(), i, nDepth, FormatMoney(tx->tx->vout[i].nValue));
}

class CAffectedKeysVisitor : public boost::static_visitor<void> {
private:
    const CKeyStore &keystore;
    std::vector<CKeyID> &vKeys;

public:
    CAffectedKeysVisitor(const CKeyStore &keystoreIn, std::vector<CKeyID> &vKeysIn) : keystore(keystoreIn), vKeys(vKeysIn) {}

    void Process(const CScript &script) {
        txnouttype type;
        std::vector<CTxDestination> vDest;
        int nRequired;
        if (ExtractDestinations(script, type, vDest, nRequired)) {
            for (const CTxDestination &dest : vDest)
                boost::apply_visitor(*this, dest);
        }
    }

    void operator()(const CKeyID &keyId) {
        if (keystore.HaveKey(keyId))
            vKeys.push_back(keyId);
    }

    void operator()(const CScriptID &scriptId) {
        CScript script;
        if (keystore.GetCScript(scriptId, script))
            Process(script);
    }

    void operator()(const CNoDestination &none) {}
};

int COutput::Priority() const
{
    for (const auto& d : CCoinJoin::GetStandardDenominations()) {
        // large denoms have lower value
        if(tx->tx->vout[i].nValue == d) return (float)COIN / d * 10000;
    }
    if(tx->tx->vout[i].nValue < 1*COIN) return 20000;

    //nondenom return largest first
    return -(tx->tx->vout[i].nValue/COIN);
}

const CWalletTx* CWallet::GetWalletTx(const uint256& hash) const
{
    LOCK(cs_wallet);
    std::map<uint256, CWalletTx>::const_iterator it = mapWallet.find(hash);
    if (it == mapWallet.end())
        return nullptr;
    return &(it->second);
}

CPubKey CWallet::GenerateNewKey(WalletBatch &batch, uint32_t nAccountIndex, bool fInternal)
{
    AssertLockHeld(cs_wallet); // mapKeyMetadata
    bool fCompressed = CanSupportFeature(FEATURE_COMPRPUBKEY); // default to compressed public keys if we want 0.6.0 wallets

    CKey secret;

    // Create new metadata
    int64_t nCreationTime = GetTime();
    CKeyMetadata metadata(nCreationTime);

    CPubKey pubkey;
    // use HD key derivation if HD was enabled during wallet creation
    if (IsHDEnabled()) {
        DeriveNewChildKey(batch, metadata, secret, nAccountIndex, fInternal);
        pubkey = secret.GetPubKey();
    } else {
        secret.MakeNewKey(fCompressed);

        // Compressed public keys were introduced in version 0.6.0
        if (fCompressed) {
            SetMinVersion(FEATURE_COMPRPUBKEY);
        }

        pubkey = secret.GetPubKey();
        assert(secret.VerifyPubKey(pubkey));

        // Create new metadata
        mapKeyMetadata[pubkey.GetID()] = metadata;
        UpdateTimeFirstKey(nCreationTime);

        if (!AddKeyPubKeyWithDB(batch, secret, pubkey)) {
            throw std::runtime_error(std::string(__func__) + ": AddKey failed");
        }
    }
    return pubkey;
}

void CWallet::DeriveNewChildKey(WalletBatch &batch, const CKeyMetadata& metadata, CKey& secretRet, uint32_t nAccountIndex, bool fInternal)
{
    CHDChain hdChainTmp;
    if (!GetHDChain(hdChainTmp)) {
        throw std::runtime_error(std::string(__func__) + ": GetHDChain failed");
    }

    if (!DecryptHDChain(hdChainTmp))
        throw std::runtime_error(std::string(__func__) + ": DecryptHDChain failed");
    // make sure seed matches this chain
    if (hdChainTmp.GetID() != hdChainTmp.GetSeedHash())
        throw std::runtime_error(std::string(__func__) + ": Wrong HD chain!");

    CHDAccount acc;
    if (!hdChainTmp.GetAccount(nAccountIndex, acc))
        throw std::runtime_error(std::string(__func__) + ": Wrong HD account!");

    // derive child key at next index, skip keys already known to the wallet
    CExtKey childKey;
    uint32_t nChildIndex = fInternal ? acc.nInternalChainCounter : acc.nExternalChainCounter;
    do {
        hdChainTmp.DeriveChildExtKey(nAccountIndex, fInternal, nChildIndex, childKey);
        // increment childkey index
        nChildIndex++;
    } while (HaveKey(childKey.key.GetPubKey().GetID()));
    secretRet = childKey.key;

    CPubKey pubkey = secretRet.GetPubKey();
    assert(secretRet.VerifyPubKey(pubkey));

    // store metadata
    mapKeyMetadata[pubkey.GetID()] = metadata;
    UpdateTimeFirstKey(metadata.nCreateTime);

    // update the chain model in the database
    CHDChain hdChainCurrent;
    GetHDChain(hdChainCurrent);

    if (fInternal) {
        acc.nInternalChainCounter = nChildIndex;
    }
    else {
        acc.nExternalChainCounter = nChildIndex;
    }

    if (!hdChainCurrent.SetAccount(nAccountIndex, acc))
        throw std::runtime_error(std::string(__func__) + ": SetAccount failed");

    if (IsCrypted()) {
        if (!SetCryptedHDChain(batch, hdChainCurrent, false))
            throw std::runtime_error(std::string(__func__) + ": SetCryptedHDChain failed");
    }
    else {
        if (!SetHDChain(batch, hdChainCurrent, false))
            throw std::runtime_error(std::string(__func__) + ": SetHDChain failed");
    }

    if (!AddHDPubKey(batch, childKey.Neuter(), fInternal))
        throw std::runtime_error(std::string(__func__) + ": AddHDPubKey failed");
}

bool CWallet::GetPubKey(const CKeyID &address, CPubKey& vchPubKeyOut) const
{
    LOCK(cs_wallet);
    std::map<CKeyID, CHDPubKey>::const_iterator mi = mapHdPubKeys.find(address);
    if (mi != mapHdPubKeys.end())
    {
        const CHDPubKey &hdPubKey = (*mi).second;
        vchPubKeyOut = hdPubKey.extPubKey.pubkey;
        return true;
    }
    else
        return CCryptoKeyStore::GetPubKey(address, vchPubKeyOut);
}

bool CWallet::GetKey(const CKeyID &address, CKey& keyOut) const
{
    LOCK(cs_wallet);
    std::map<CKeyID, CHDPubKey>::const_iterator mi = mapHdPubKeys.find(address);
    if (mi != mapHdPubKeys.end())
    {
        // if the key has been found in mapHdPubKeys, derive it on the fly
        const CHDPubKey &hdPubKey = (*mi).second;
        CHDChain hdChainCurrent;
        if (!GetHDChain(hdChainCurrent))
            throw std::runtime_error(std::string(__func__) + ": GetHDChain failed");
        if (!DecryptHDChain(hdChainCurrent))
            throw std::runtime_error(std::string(__func__) + ": DecryptHDChain failed");
        // make sure seed matches this chain
        if (hdChainCurrent.GetID() != hdChainCurrent.GetSeedHash())
            throw std::runtime_error(std::string(__func__) + ": Wrong HD chain!");

        CExtKey extkey;
        hdChainCurrent.DeriveChildExtKey(hdPubKey.nAccountIndex, hdPubKey.nChangeIndex != 0, hdPubKey.extPubKey.nChild, extkey);
        keyOut = extkey.key;

        return true;
    }
    else {
        return CCryptoKeyStore::GetKey(address, keyOut);
    }
}

bool CWallet::HaveKey(const CKeyID &address) const
{
    LOCK(cs_wallet);
    if (mapHdPubKeys.count(address) > 0)
        return true;
    return CCryptoKeyStore::HaveKey(address);
}

bool CWallet::LoadHDPubKey(const CHDPubKey &hdPubKey)
{
    AssertLockHeld(cs_wallet);

    mapHdPubKeys[hdPubKey.extPubKey.pubkey.GetID()] = hdPubKey;
    return true;
}

bool CWallet::AddHDPubKey(WalletBatch &batch, const CExtPubKey &extPubKey, bool fInternal)
{
    AssertLockHeld(cs_wallet);

    CHDChain hdChainCurrent;
    GetHDChain(hdChainCurrent);

    CHDPubKey hdPubKey;
    hdPubKey.extPubKey = extPubKey;
    hdPubKey.hdchainID = hdChainCurrent.GetID();
    hdPubKey.nChangeIndex = fInternal ? 1 : 0;
    mapHdPubKeys[extPubKey.pubkey.GetID()] = hdPubKey;

    // check if we need to remove from watch-only
    CScript script;
    script = GetScriptForDestination(extPubKey.pubkey.GetID());
    if (HaveWatchOnly(script))
        RemoveWatchOnly(script);
    script = GetScriptForRawPubKey(extPubKey.pubkey);
    if (HaveWatchOnly(script))
        RemoveWatchOnly(script);

    return batch.WriteHDPubKey(hdPubKey, mapKeyMetadata[extPubKey.pubkey.GetID()]);
}

bool CWallet::AddKeyPubKeyWithDB(WalletBatch &batch, const CKey& secret, const CPubKey &pubkey)
{
    AssertLockHeld(cs_wallet); // mapKeyMetadata

    // CCryptoKeyStore has no concept of wallet databases, but calls AddCryptedKey
    // which is overridden below.  To avoid flushes, the database handle is
    // tunneled through to it.
    bool needsDB = !encrypted_batch;
    if (needsDB) {
        encrypted_batch = &batch;
    }
    if (!CCryptoKeyStore::AddKeyPubKey(secret, pubkey)) {
        if (needsDB) encrypted_batch = nullptr;
        return false;
    }
    if (needsDB) encrypted_batch = nullptr;
    // check if we need to remove from watch-only
    CScript script;
    script = GetScriptForDestination(pubkey.GetID());
    if (HaveWatchOnly(script)) {
        RemoveWatchOnly(script);
    }
    script = GetScriptForRawPubKey(pubkey);
    if (HaveWatchOnly(script)) {
        RemoveWatchOnly(script);
    }

    if (!IsCrypted()) {
        return batch.WriteKey(pubkey,
                                 secret.GetPrivKey(),
                                 mapKeyMetadata[pubkey.GetID()]);
    }
    return true;
}


bool CWallet::AddKeyPubKey(const CKey& secret, const CPubKey &pubkey)
{
    WalletBatch batch(*database);

    return CWallet::AddKeyPubKeyWithDB(batch, secret, pubkey);
}

bool CWallet::AddCryptedKey(const CPubKey &vchPubKey,
                            const std::vector<unsigned char> &vchCryptedSecret)
{
    if (!CCryptoKeyStore::AddCryptedKey(vchPubKey, vchCryptedSecret))
        return false;
    {
        LOCK(cs_wallet);
        if (encrypted_batch)
            return encrypted_batch->WriteCryptedKey(vchPubKey,
                                                        vchCryptedSecret,
                                                        mapKeyMetadata[vchPubKey.GetID()]);
        else
            return WalletBatch(*database).WriteCryptedKey(vchPubKey,
                                                            vchCryptedSecret,
                                                            mapKeyMetadata[vchPubKey.GetID()]);
    }
}

bool CWallet::LoadKeyMetadata(const CKeyID& keyID, const CKeyMetadata &meta)
{
    AssertLockHeld(cs_wallet); // mapKeyMetadata
    UpdateTimeFirstKey(meta.nCreateTime);
    mapKeyMetadata[keyID] = meta;
    return true;
}

bool CWallet::LoadScriptMetadata(const CScriptID& script_id, const CKeyMetadata &meta)
{
    AssertLockHeld(cs_wallet); // m_script_metadata
    UpdateTimeFirstKey(meta.nCreateTime);
    m_script_metadata[script_id] = meta;
    return true;
}

bool CWallet::LoadCryptedKey(const CPubKey &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret)
{
    return CCryptoKeyStore::AddCryptedKey(vchPubKey, vchCryptedSecret);
}

/**
 * Update wallet first key creation time. This should be called whenever keys
 * are added to the wallet, with the oldest key creation time.
 */
void CWallet::UpdateTimeFirstKey(int64_t nCreateTime)
{
    AssertLockHeld(cs_wallet);
    if (nCreateTime <= 1) {
        // Cannot determine birthday information, so set the wallet birthday to
        // the beginning of time.
        nTimeFirstKey = 1;
    } else if (!nTimeFirstKey || nCreateTime < nTimeFirstKey) {
        nTimeFirstKey = nCreateTime;
    }
}

int64_t CWallet::GetTimeFirstKey() const
{
    AssertLockHeld(cs_wallet);
    return nTimeFirstKey;
}

bool CWallet::AddCScript(const CScript& redeemScript)
{
    if (!CCryptoKeyStore::AddCScript(redeemScript))
        return false;
    return WalletBatch(*database).WriteCScript(Hash160(redeemScript), redeemScript);
}

bool CWallet::LoadCScript(const CScript& redeemScript)
{
    /* A sanity check was added in pull #3843 to avoid adding redeemScripts
     * that never can be redeemed. However, old wallets may still contain
     * these. Do not add them to the wallet and warn. */
    if (redeemScript.size() > MAX_SCRIPT_ELEMENT_SIZE)
    {
        std::string strAddr = EncodeDestination(CScriptID(redeemScript));
        LogPrintf("%s: Warning: This wallet contains a redeemScript of size %i which exceeds maximum size %i thus can never be redeemed. Do not use address %s.\n",
            __func__, redeemScript.size(), MAX_SCRIPT_ELEMENT_SIZE, strAddr);
        return true;
    }

    return CCryptoKeyStore::AddCScript(redeemScript);
}

bool CWallet::AddWatchOnly(const CScript& dest)
{
    if (!CCryptoKeyStore::AddWatchOnly(dest))
        return false;
    const CKeyMetadata& meta = m_script_metadata[CScriptID(dest)];
    UpdateTimeFirstKey(meta.nCreateTime);
    NotifyWatchonlyChanged(true);
    return WalletBatch(*database).WriteWatchOnly(dest, meta);
}

bool CWallet::AddWatchOnly(const CScript& dest, int64_t nCreateTime)
{
    m_script_metadata[CScriptID(dest)].nCreateTime = nCreateTime;
    return AddWatchOnly(dest);
}

bool CWallet::RemoveWatchOnly(const CScript &dest)
{
    AssertLockHeld(cs_wallet);
    if (!CCryptoKeyStore::RemoveWatchOnly(dest))
        return false;
    if (!HaveWatchOnly())
        NotifyWatchonlyChanged(false);
    if (!WalletBatch(*database).EraseWatchOnly(dest))
        return false;

    return true;
}

bool CWallet::LoadWatchOnly(const CScript &dest)
{
    return CCryptoKeyStore::AddWatchOnly(dest);
}

bool CWallet::Unlock(const SecureString& strWalletPassphrase, bool fForMixingOnly)
{
    SecureString strWalletPassphraseFinal;

    if (!IsLocked()) // was already fully unlocked, not only for mixing
        return true;

    // Verify KeePassIntegration
    if (strWalletPassphrase == "keepass" && gArgs.GetBoolArg("-keepass", false)) {
        try {
            strWalletPassphraseFinal = keePassInt.retrievePassphrase();
        } catch (std::exception& e) {
            LogPrintf("CWallet::Unlock could not retrieve passphrase from KeePass: Error: %s\n", e.what());
            return false;
        }
    } else {
        strWalletPassphraseFinal = strWalletPassphrase;
    }

    CCrypter crypter;
    CKeyingMaterial _vMasterKey;

    {
        LOCK(cs_wallet);
        for (const MasterKeyMap::value_type& pMasterKey : mapMasterKeys)
        {
            if (!crypter.SetKeyFromPassphrase(strWalletPassphraseFinal, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                return false;
            if (!crypter.Decrypt(pMasterKey.second.vchCryptedKey, _vMasterKey))
                continue; // try another master key
            if (CCryptoKeyStore::Unlock(_vMasterKey, fForMixingOnly)) {
                if(nWalletBackups == -2) {
                    TopUpKeyPool();
                    LogPrintf("Keypool replenished, re-initializing automatic backups.\n");
                    nWalletBackups = gArgs.GetArg("-createwalletbackups", 10);
                }
                return true;
            }
        }
    }
    return false;
}

bool CWallet::ChangeWalletPassphrase(const SecureString& strOldWalletPassphrase, const SecureString& strNewWalletPassphrase)
{
    bool fWasLocked = IsLocked(true);
    bool bUseKeePass = false;

    SecureString strOldWalletPassphraseFinal;

    // Verify KeePassIntegration
    if(strOldWalletPassphrase == "keepass" && gArgs.GetBoolArg("-keepass", false)) {
        bUseKeePass = true;
        try {
            strOldWalletPassphraseFinal = keePassInt.retrievePassphrase();
        } catch (std::exception& e) {
            LogPrintf("CWallet::ChangeWalletPassphrase -- could not retrieve passphrase from KeePass: Error: %s\n", e.what());
            return false;
        }
    } else {
        strOldWalletPassphraseFinal = strOldWalletPassphrase;
    }

    {
        LOCK(cs_wallet);
        Lock();

        CCrypter crypter;
        CKeyingMaterial _vMasterKey;
        for (MasterKeyMap::value_type& pMasterKey : mapMasterKeys)
        {
            if(!crypter.SetKeyFromPassphrase(strOldWalletPassphraseFinal, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                return false;
            if (!crypter.Decrypt(pMasterKey.second.vchCryptedKey, _vMasterKey))
                return false;
            if (CCryptoKeyStore::Unlock(_vMasterKey))
            {
                int64_t nStartTime = GetTimeMillis();
                crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod);
                pMasterKey.second.nDeriveIterations = static_cast<unsigned int>(pMasterKey.second.nDeriveIterations * (100 / ((double)(GetTimeMillis() - nStartTime))));

                nStartTime = GetTimeMillis();
                crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod);
                pMasterKey.second.nDeriveIterations = (pMasterKey.second.nDeriveIterations + static_cast<unsigned int>(pMasterKey.second.nDeriveIterations * 100 / ((double)(GetTimeMillis() - nStartTime)))) / 2;

                if (pMasterKey.second.nDeriveIterations < 25000)
                    pMasterKey.second.nDeriveIterations = 25000;

                LogPrintf("Wallet passphrase changed to an nDeriveIterations of %i\n", pMasterKey.second.nDeriveIterations);

                if (!crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                    return false;
                if (!crypter.Encrypt(_vMasterKey, pMasterKey.second.vchCryptedKey))
                    return false;
                WalletBatch(*database).WriteMasterKey(pMasterKey.first, pMasterKey.second);
                if (fWasLocked)
                    Lock();

                // Update KeePass if necessary
                if(bUseKeePass) {
                    LogPrintf("CWallet::ChangeWalletPassphrase -- Updating KeePass with new passphrase\n");
                    try {
                        keePassInt.updatePassphrase(strNewWalletPassphrase);
                    } catch (std::exception& e) {
                        LogPrintf("CWallet::ChangeWalletPassphrase -- could not update passphrase in KeePass: Error: %s\n", e.what());
                        return false;
                    }
                }

                return true;
            }
        }
    }

    return false;
}

void CWallet::SetBestChain(const CBlockLocator& loc)
{
    WalletBatch batch(*database);
    batch.WriteBestBlock(loc);
}

bool CWallet::SetMinVersion(enum WalletFeature nVersion, WalletBatch* batch_in, bool fExplicit)
{
    LOCK(cs_wallet); // nWalletVersion
    if (nWalletVersion >= nVersion)
        return true;

    // when doing an explicit upgrade, if we pass the max version permitted, upgrade all the way
    if (fExplicit && nVersion > nWalletMaxVersion)
            nVersion = FEATURE_LATEST;

    nWalletVersion = nVersion;

    if (nVersion > nWalletMaxVersion)
        nWalletMaxVersion = nVersion;

    {
        WalletBatch* batch = batch_in ? batch_in : new WalletBatch(*database);
        if (nWalletVersion > 40000)
            batch->WriteMinVersion(nWalletVersion);
        if (!batch_in)
            delete batch;
    }

    return true;
}

bool CWallet::SetMaxVersion(int nVersion)
{
    LOCK(cs_wallet); // nWalletVersion, nWalletMaxVersion
    // cannot downgrade below current version
    if (nWalletVersion > nVersion)
        return false;

    nWalletMaxVersion = nVersion;

    return true;
}

std::set<uint256> CWallet::GetConflicts(const uint256& txid) const
{
    std::set<uint256> result;
    AssertLockHeld(cs_wallet);

    std::map<uint256, CWalletTx>::const_iterator it = mapWallet.find(txid);
    if (it == mapWallet.end())
        return result;
    const CWalletTx& wtx = it->second;

    std::pair<TxSpends::const_iterator, TxSpends::const_iterator> range;

    for (const CTxIn& txin : wtx.tx->vin)
    {
        if (mapTxSpends.count(txin.prevout) <= 1)
            continue;  // No conflict if zero or one spends
        range = mapTxSpends.equal_range(txin.prevout);
        for (TxSpends::const_iterator _it = range.first; _it != range.second; ++_it)
            result.insert(_it->second);
    }
    return result;
}

void CWallet::Flush(bool shutdown)
{
    database->Flush(shutdown);
}

void CWallet::SyncMetaData(std::pair<TxSpends::iterator, TxSpends::iterator> range)
{
    // We want all the wallet transactions in range to have the same metadata as
    // the oldest (smallest nOrderPos).
    // So: find smallest nOrderPos:

    int nMinOrderPos = std::numeric_limits<int>::max();
    const CWalletTx* copyFrom = nullptr;
    for (TxSpends::iterator it = range.first; it != range.second; ++it) {
        const CWalletTx* wtx = &mapWallet.at(it->second);
        if (wtx->nOrderPos < nMinOrderPos) {
            nMinOrderPos = wtx->nOrderPos;
            copyFrom = wtx;
        }
    }

    if (!copyFrom) {
        return;
    }

    // Now copy data from copyFrom to rest:
    for (TxSpends::iterator it = range.first; it != range.second; ++it)
    {
        const uint256& hash = it->second;
        CWalletTx* copyTo = &mapWallet.at(hash);
        if (copyFrom == copyTo) continue;
        assert(copyFrom && "Oldest wallet transaction in range assumed to have been found.");
        if (!copyFrom->IsEquivalentTo(*copyTo)) continue;
        copyTo->mapValue = copyFrom->mapValue;
        copyTo->vOrderForm = copyFrom->vOrderForm;
        // fTimeReceivedIsTxTime not copied on purpose
        // nTimeReceived not copied on purpose
        copyTo->nTimeSmart = copyFrom->nTimeSmart;
        copyTo->fFromMe = copyFrom->fFromMe;
        copyTo->strFromAccount = copyFrom->strFromAccount;
        // nOrderPos not copied on purpose
        // cached members not copied on purpose
    }
}

/**
 * Outpoint is spent if any non-conflicted transaction
 * spends it:
 */
bool CWallet::IsSpent(const uint256& hash, unsigned int n) const
{
    const COutPoint outpoint(hash, n);
    std::pair<TxSpends::const_iterator, TxSpends::const_iterator> range;
    range = mapTxSpends.equal_range(outpoint);

    for (TxSpends::const_iterator it = range.first; it != range.second; ++it)
    {
        const uint256& wtxid = it->second;
        std::map<uint256, CWalletTx>::const_iterator mit = mapWallet.find(wtxid);
        if (mit != mapWallet.end()) {
            int depth = mit->second.GetDepthInMainChain();
            if (depth > 0  || (depth == 0 && !mit->second.isAbandoned()))
                return true; // Spent
        }
    }
    return false;
}

void CWallet::AddToSpends(const COutPoint& outpoint, const uint256& wtxid)
{
    mapTxSpends.insert(std::make_pair(outpoint, wtxid));
    setWalletUTXO.erase(outpoint);

    setLockedCoins.erase(outpoint);

    std::pair<TxSpends::iterator, TxSpends::iterator> range;
    range = mapTxSpends.equal_range(outpoint);
    SyncMetaData(range);
}


void CWallet::AddToSpends(const uint256& wtxid)
{
    auto it = mapWallet.find(wtxid);
    assert(it != mapWallet.end());
    CWalletTx& thisTx = it->second;
    if (thisTx.IsCoinBase()) // Coinbases don't spend anything!
        return;

    for (const CTxIn& txin : thisTx.tx->vin)
        AddToSpends(txin.prevout, wtxid);
}

bool CWallet::EncryptWallet(const SecureString& strWalletPassphrase)
{
    if (IsCrypted())
        return false;

    CKeyingMaterial _vMasterKey;

    _vMasterKey.resize(WALLET_CRYPTO_KEY_SIZE);
    GetStrongRandBytes(&_vMasterKey[0], WALLET_CRYPTO_KEY_SIZE);

    CMasterKey kMasterKey;

    kMasterKey.vchSalt.resize(WALLET_CRYPTO_SALT_SIZE);
    GetStrongRandBytes(&kMasterKey.vchSalt[0], WALLET_CRYPTO_SALT_SIZE);

    CCrypter crypter;
    int64_t nStartTime = GetTimeMillis();
    crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, 25000, kMasterKey.nDerivationMethod);
    kMasterKey.nDeriveIterations = static_cast<unsigned int>(2500000 / ((double)(GetTimeMillis() - nStartTime)));

    nStartTime = GetTimeMillis();
    crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, kMasterKey.nDeriveIterations, kMasterKey.nDerivationMethod);
    kMasterKey.nDeriveIterations = (kMasterKey.nDeriveIterations + static_cast<unsigned int>(kMasterKey.nDeriveIterations * 100 / ((double)(GetTimeMillis() - nStartTime)))) / 2;

    if (kMasterKey.nDeriveIterations < 25000)
        kMasterKey.nDeriveIterations = 25000;

    LogPrintf("Encrypting Wallet with an nDeriveIterations of %i\n", kMasterKey.nDeriveIterations);

    if (!crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt, kMasterKey.nDeriveIterations, kMasterKey.nDerivationMethod))
        return false;
    if (!crypter.Encrypt(_vMasterKey, kMasterKey.vchCryptedKey))
        return false;

    {
        LOCK(cs_wallet);
        mapMasterKeys[++nMasterKeyMaxID] = kMasterKey;
        assert(!encrypted_batch);
        encrypted_batch = new WalletBatch(*database);
        if (!encrypted_batch->TxnBegin()) {
            delete encrypted_batch;
            encrypted_batch = nullptr;
            return false;
        }
        encrypted_batch->WriteMasterKey(nMasterKeyMaxID, kMasterKey);

        // must get current HD chain before EncryptKeys
        CHDChain hdChainCurrent;
        GetHDChain(hdChainCurrent);

        if (!EncryptKeys(_vMasterKey))
        {
            encrypted_batch->TxnAbort();
            delete encrypted_batch;
            // We now probably have half of our keys encrypted in memory, and half not...
            // die and let the user reload the unencrypted wallet.
            assert(false);
        }

        if (!hdChainCurrent.IsNull()) {
            assert(EncryptHDChain(_vMasterKey));

            CHDChain hdChainCrypted;
            assert(GetHDChain(hdChainCrypted));

            DBG(
                printf("EncryptWallet -- current seed: '%s'\n", HexStr(hdChainCurrent.GetSeed()).c_str());
                printf("EncryptWallet -- crypted seed: '%s'\n", HexStr(hdChainCrypted.GetSeed()).c_str());
            );

            // ids should match, seed hashes should not
            assert(hdChainCurrent.GetID() == hdChainCrypted.GetID());
            assert(hdChainCurrent.GetSeedHash() != hdChainCrypted.GetSeedHash());

            assert(SetCryptedHDChain(*encrypted_batch, hdChainCrypted, false));
        }

        // Encryption was introduced in version 0.4.0
        SetMinVersion(FEATURE_WALLETCRYPT, encrypted_batch, true);

        if (!encrypted_batch->TxnCommit()) {
            delete encrypted_batch;
            // We now have keys encrypted in memory, but not on disk...
            // die to avoid confusion and let the user reload the unencrypted wallet.
            assert(false);
        }

        delete encrypted_batch;
        encrypted_batch = nullptr;

        Lock();
        Unlock(strWalletPassphrase);

        // if we are not using HD, generate new keypool
        if(IsHDEnabled()) {
            TopUpKeyPool();
        }
        else {
            NewKeyPool();
        }

        Lock();

        // Need to completely rewrite the wallet file; if we don't, bdb might keep
        // bits of the unencrypted private key in slack space in the database file.
        database->Rewrite();

        // Update KeePass if necessary
        if(gArgs.GetBoolArg("-keepass", false)) {
            LogPrintf("CWallet::EncryptWallet -- Updating KeePass with new passphrase\n");
            try {
                keePassInt.updatePassphrase(strWalletPassphrase);
            } catch (std::exception& e) {
                LogPrintf("CWallet::EncryptWallet -- could not update passphrase in KeePass: Error: %s\n", e.what());
            }
        }

        // BDB seems to have a bad habit of writing old data into
        // slack space in .dat files; that is bad if the old data is
        // unencrypted private keys. So:
        database->ReloadDbEnv();

    }
    NotifyStatusChanged(this);

    return true;
}

DBErrors CWallet::ReorderTransactions()
{
    LOCK(cs_wallet);
    WalletBatch batch(*database);

    // Old wallets didn't have any defined order for transactions
    // Probably a bad idea to change the output of this

    // First: get all CWalletTx and CAccountingEntry into a sorted-by-time multimap.
    typedef std::pair<CWalletTx*, CAccountingEntry*> TxPair;
    typedef std::multimap<int64_t, TxPair > TxItems;
    TxItems txByTime;

    for (auto& entry : mapWallet)
    {
        CWalletTx* wtx = &entry.second;
        txByTime.insert(std::make_pair(wtx->nTimeReceived, TxPair(wtx, nullptr)));
    }
    std::list<CAccountingEntry> acentries;
    batch.ListAccountCreditDebit("", acentries);
    for (CAccountingEntry& entry : acentries)
    {
        txByTime.insert(std::make_pair(entry.nTime, TxPair(nullptr, &entry)));
    }

    nOrderPosNext = 0;
    std::vector<int64_t> nOrderPosOffsets;
    for (TxItems::iterator it = txByTime.begin(); it != txByTime.end(); ++it)
    {
        CWalletTx *const pwtx = (*it).second.first;
        CAccountingEntry *const pacentry = (*it).second.second;
        int64_t& nOrderPos = (pwtx != nullptr) ? pwtx->nOrderPos : pacentry->nOrderPos;

        if (nOrderPos == -1)
        {
            nOrderPos = nOrderPosNext++;
            nOrderPosOffsets.push_back(nOrderPos);

            if (pwtx)
            {
                if (!batch.WriteTx(*pwtx))
                    return DBErrors::LOAD_FAIL;
            }
            else
                if (!batch.WriteAccountingEntry(pacentry->nEntryNo, *pacentry))
                    return DBErrors::LOAD_FAIL;
        }
        else
        {
            int64_t nOrderPosOff = 0;
            for (const int64_t& nOffsetStart : nOrderPosOffsets)
            {
                if (nOrderPos >= nOffsetStart)
                    ++nOrderPosOff;
            }
            nOrderPos += nOrderPosOff;
            nOrderPosNext = std::max(nOrderPosNext, nOrderPos + 1);

            if (!nOrderPosOff)
                continue;

            // Since we're changing the order, write it back
            if (pwtx)
            {
                if (!batch.WriteTx(*pwtx))
                    return DBErrors::LOAD_FAIL;
            }
            else
                if (!batch.WriteAccountingEntry(pacentry->nEntryNo, *pacentry))
                    return DBErrors::LOAD_FAIL;
        }
    }
    batch.WriteOrderPosNext(nOrderPosNext);

    return DBErrors::LOAD_OK;
}

int64_t CWallet::IncOrderPosNext(WalletBatch *batch)
{
    AssertLockHeld(cs_wallet); // nOrderPosNext
    int64_t nRet = nOrderPosNext++;
    if (batch) {
        batch->WriteOrderPosNext(nOrderPosNext);
    } else {
        WalletBatch(*database).WriteOrderPosNext(nOrderPosNext);
    }
    return nRet;
}

bool CWallet::AccountMove(std::string strFrom, std::string strTo, CAmount nAmount, std::string strComment)
{
    WalletBatch batch(*database);
    if (!batch.TxnBegin())
        return false;

    int64_t nNow = GetAdjustedTime();

    // Debit
    CAccountingEntry debit;
    debit.nOrderPos = IncOrderPosNext(&batch);
    debit.strAccount = strFrom;
    debit.nCreditDebit = -nAmount;
    debit.nTime = nNow;
    debit.strOtherAccount = strTo;
    debit.strComment = strComment;
    AddAccountingEntry(debit, &batch);

    // Credit
    CAccountingEntry credit;
    credit.nOrderPos = IncOrderPosNext(&batch);
    credit.strAccount = strTo;
    credit.nCreditDebit = nAmount;
    credit.nTime = nNow;
    credit.strOtherAccount = strFrom;
    credit.strComment = strComment;
    AddAccountingEntry(credit, &batch);

    if (!batch.TxnCommit())
        return false;

    return true;
}

bool CWallet::GetLabelDestination(CTxDestination &dest, const std::string& label, bool bForceNew)
{
    WalletBatch batch(*database);

    CAccount account;
    batch.ReadAccount(label, account);

    if (!bForceNew) {
        if (!account.vchPubKey.IsValid())
            bForceNew = true;
        else {
            // Check if the current key has been used
            CScript scriptPubKey = GetScriptForDestination(account.vchPubKey.GetID());
            for (std::map<uint256, CWalletTx>::iterator it = mapWallet.begin();
                 it != mapWallet.end() && account.vchPubKey.IsValid();
                 ++it)
                for (const CTxOut& txout : (*it).second.tx->vout)
                    if (txout.scriptPubKey == scriptPubKey) {
                        bForceNew = true;
                        break;
                    }
        }
    }

    // Generate a new key
    if (bForceNew) {
        if (!GetKeyFromPool(account.vchPubKey, false))
            return false;

        dest = account.vchPubKey.GetID();
        SetAddressBook(dest, label, "receive");
        batch.WriteAccount(label, account);
    } else {
        dest = account.vchPubKey.GetID();
    }

    return true;
}

void CWallet::MarkDirty()
{
    {
        LOCK(cs_wallet);
        for (std::pair<const uint256, CWalletTx>& item : mapWallet)
            item.second.MarkDirty();
    }

    fAnonymizableTallyCached = false;
    fAnonymizableTallyCachedNonDenom = false;
}

bool CWallet::AddToWallet(const CWalletTx& wtxIn, bool fFlushOnClose)
{
    AssertLockHeld(cs_main); // detect potential deadlocks which might be caused by GetListAtChainTip and IsSpent below
    LOCK(cs_wallet);

    WalletBatch batch(*database, "r+", fFlushOnClose);

    uint256 hash = wtxIn.GetHash();

    // Inserts only if not already there, returns tx inserted or tx found
    std::pair<std::map<uint256, CWalletTx>::iterator, bool> ret = mapWallet.insert(std::make_pair(hash, wtxIn));
    CWalletTx& wtx = (*ret.first).second;
    wtx.BindWallet(this);
    bool fInsertedNew = ret.second;
    if (fInsertedNew) {
        wtx.nTimeReceived = GetAdjustedTime();
        wtx.nOrderPos = IncOrderPosNext(&batch);
        wtx.m_it_wtxOrdered = wtxOrdered.insert(std::make_pair(wtx.nOrderPos, TxPair(&wtx, nullptr)));
        wtx.nTimeSmart = ComputeTimeSmart(wtx);
        AddToSpends(hash);

        auto mnList = deterministicMNManager->GetListAtChainTip();
        for(unsigned int i = 0; i < wtx.tx->vout.size(); ++i) {
            if (IsMine(wtx.tx->vout[i]) && !IsSpent(hash, i)) {
                setWalletUTXO.insert(COutPoint(hash, i));
                if (deterministicMNManager->IsProTxWithCollateral(wtx.tx, i) || mnList.HasMNByCollateral(COutPoint(hash, i))) {
                    LockCoin(COutPoint(hash, i));
                }
            }
        }
    }

    bool fUpdated = false;
    if (!fInsertedNew)
    {
        // Merge
        if (!wtxIn.hashUnset() && wtxIn.hashBlock != wtx.hashBlock)
        {
            wtx.hashBlock = wtxIn.hashBlock;
            fUpdated = true;
        }
        // If no longer abandoned, update
        if (wtxIn.hashBlock.IsNull() && wtx.isAbandoned())
        {
            wtx.hashBlock = wtxIn.hashBlock;
            fUpdated = true;
        }
        if (wtxIn.nIndex != -1 && (wtxIn.nIndex != wtx.nIndex))
        {
            wtx.nIndex = wtxIn.nIndex;
            fUpdated = true;
        }
        if (wtxIn.fFromMe && wtxIn.fFromMe != wtx.fFromMe)
        {
            wtx.fFromMe = wtxIn.fFromMe;
            fUpdated = true;
        }

        auto mnList = deterministicMNManager->GetListAtChainTip();
        for (unsigned int i = 0; i < wtx.tx->vout.size(); ++i) {
            if (IsMine(wtx.tx->vout[i]) && !IsSpent(hash, i)) {
                bool new_utxo = setWalletUTXO.insert(COutPoint(hash, i)).second;
                if (new_utxo && (deterministicMNManager->IsProTxWithCollateral(wtx.tx, i) || mnList.HasMNByCollateral(COutPoint(hash, i)))) {
                    LockCoin(COutPoint(hash, i));
                }
                fUpdated |= new_utxo;
            }
        }
    }

    //// debug print
    LogPrintf("AddToWallet %s  %s%s\n", wtxIn.GetHash().ToString(), (fInsertedNew ? "new" : ""), (fUpdated ? "update" : ""));

    // Write to disk
    if (fInsertedNew || fUpdated)
        if (!batch.WriteTx(wtx))
            return false;

    // Break debit/credit balance caches:
    wtx.MarkDirty();

    // Notify UI of new or updated transaction
    NotifyTransactionChanged(this, hash, fInsertedNew ? CT_NEW : CT_UPDATED);

    // notify an external script when a wallet transaction comes in or is updated
    std::string strCmd = gArgs.GetArg("-walletnotify", "");

    if (!strCmd.empty())
    {
        boost::replace_all(strCmd, "%s", wtxIn.GetHash().GetHex());
        std::thread t(runCommand, strCmd);
        t.detach(); // thread runs free
    }

    fAnonymizableTallyCached = false;
    fAnonymizableTallyCachedNonDenom = false;

    return true;
}

bool CWallet::LoadToWallet(const CWalletTx& wtxIn)
{
    uint256 hash = wtxIn.GetHash();
    const auto& ins = mapWallet.emplace(hash, wtxIn);
    CWalletTx& wtx = ins.first->second;
    wtx.BindWallet(this);
    if (/* insertion took place */ ins.second) {
        wtx.m_it_wtxOrdered = wtxOrdered.insert(std::make_pair(wtx.nOrderPos, TxPair(&wtx, nullptr)));
    }
    AddToSpends(hash);
    for (const CTxIn& txin : wtx.tx->vin) {
        auto it = mapWallet.find(txin.prevout.hash);
        if (it != mapWallet.end()) {
            CWalletTx& prevtx = it->second;
            if (prevtx.nIndex == -1 && !prevtx.hashUnset()) {
                MarkConflicted(prevtx.hashBlock, wtx.GetHash());
            }
        }
    }

    return true;
}

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
bool CWallet::AddToWalletIfInvolvingMe(const CTransactionRef& ptx, const CBlockIndex* pIndex, int posInBlock, bool fUpdate)
{
    const CTransaction& tx = *ptx;
    {
        AssertLockHeld(cs_main); // because of AddToWallet
        AssertLockHeld(cs_wallet);

        if (pIndex != nullptr) {
            for (const CTxIn& txin : tx.vin) {
                std::pair<TxSpends::const_iterator, TxSpends::const_iterator> range = mapTxSpends.equal_range(txin.prevout);
                while (range.first != range.second) {
                    if (range.first->second != tx.GetHash()) {
                        LogPrintf("Transaction %s (in block %s) conflicts with wallet transaction %s (both spend %s:%i)\n", tx.GetHash().ToString(), pIndex->GetBlockHash().ToString(), range.first->second.ToString(), range.first->first.hash.ToString(), range.first->first.n);
                        MarkConflicted(pIndex->GetBlockHash(), range.first->second);
                    }
                    range.first++;
                }
            }
        }

        bool fExisted = mapWallet.count(tx.GetHash()) != 0;
        if (fExisted && !fUpdate) return false;
        if (fExisted || IsMine(tx) || IsFromMe(tx))
        {
            /* Check if any keys in the wallet keypool that were supposed to be unused
             * have appeared in a new transaction. If so, remove those keys from the keypool.
             * This can happen when restoring an old wallet backup that does not contain
             * the mostly recently created transactions from newer versions of the wallet.
             */

            WalletBatch batch(*database);
            // loop though all outputs
            for (const CTxOut& txout: tx.vout) {
                // extract addresses, check if they match with an unused keypool key, update metadata if needed
                std::vector<CKeyID> vAffected;
                CAffectedKeysVisitor(*this, vAffected).Process(txout.scriptPubKey);
                for (const CKeyID &keyid : vAffected) {
                    std::map<CKeyID, int64_t>::const_iterator mi = m_pool_key_to_index.find(keyid);
                    if (mi != m_pool_key_to_index.end()) {
                        LogPrintf("%s: Detected a used keypool key, mark all keypool key up to this key as used\n", __func__);
                        MarkReserveKeysAsUsed(mi->second);

                        if (!TopUpKeyPool()) {
                            LogPrintf("%s: Topping up keypool failed (locked wallet)\n", __func__);
                        }
                    }
                    if (pIndex != nullptr && mapKeyMetadata[keyid].nCreateTime > pIndex->nTime) {
                        LogPrintf("%s: Found a key which appears to be used earlier than we expected, updating metadata\n", __func__);
                        CPubKey vchPubKey;
                        bool res = GetPubKey(keyid, vchPubKey);
                        assert(res); // this should never fail
                        mapKeyMetadata[keyid].nCreateTime = pIndex->nTime;
                        batch.WriteKeyMeta(vchPubKey, mapKeyMetadata[keyid]);
                        UpdateTimeFirstKey(pIndex->nTime);
                    }
                }
            }

            CWalletTx wtx(this, ptx);

            // Get merkle branch if transaction was found in a block
            if (pIndex != nullptr)
                wtx.SetMerkleBranch(pIndex, posInBlock);

            return AddToWallet(wtx, false);
        }
    }
    return false;
}

bool CWallet::TransactionCanBeAbandoned(const uint256& hashTx) const
{
    LOCK2(cs_main, cs_wallet);
    const CWalletTx* wtx = GetWalletTx(hashTx);
    return wtx && !wtx->isAbandoned() && wtx->GetDepthInMainChain() == 0 && !wtx->InMempool();
}

bool CWallet::AbandonTransaction(const uint256& hashTx)
{
    LOCK2(cs_main, cs_wallet);

    WalletBatch batch(*database, "r+");

    std::set<uint256> todo;
    std::set<uint256> done;

    // Can't mark abandoned if confirmed or in mempool
    auto it = mapWallet.find(hashTx);
    assert(it != mapWallet.end());
    CWalletTx& origtx = it->second;
    if (origtx.GetDepthInMainChain() != 0 || origtx.InMempool() || origtx.IsLockedByInstantSend()) {
        return false;
    }

    todo.insert(hashTx);

    while (!todo.empty()) {
        uint256 now = *todo.begin();
        todo.erase(now);
        done.insert(now);
        auto it = mapWallet.find(now);
        assert(it != mapWallet.end());
        CWalletTx& wtx = it->second;
        int currentconfirm = wtx.GetDepthInMainChain();
        // If the orig tx was not in block, none of its spends can be
        assert(currentconfirm <= 0);
        // if (currentconfirm < 0) {Tx and spends are already conflicted, no need to abandon}
        if (currentconfirm == 0 && !wtx.isAbandoned()) {
            // If the orig tx was not in block/mempool, none of its spends can be in mempool
            assert(!wtx.InMempool());
            wtx.nIndex = -1;
            wtx.setAbandoned();
            wtx.MarkDirty();
            batch.WriteTx(wtx);
            NotifyTransactionChanged(this, wtx.GetHash(), CT_UPDATED);
            // Iterate over all its outputs, and mark transactions in the wallet that spend them abandoned too
            TxSpends::const_iterator iter = mapTxSpends.lower_bound(COutPoint(now, 0));
            while (iter != mapTxSpends.end() && iter->first.hash == now) {
                if (!done.count(iter->second)) {
                    todo.insert(iter->second);
                }
                iter++;
            }
            // If a transaction changes 'conflicted' state, that changes the balance
            // available of the outputs it spends. So force those to be recomputed
            for (const CTxIn& txin : wtx.tx->vin)
            {
                auto it = mapWallet.find(txin.prevout.hash);
                if (it != mapWallet.end()) {
                    it->second.MarkDirty();
                }
            }
        }
    }

    fAnonymizableTallyCached = false;
    fAnonymizableTallyCachedNonDenom = false;

    return true;
}

void CWallet::MarkConflicted(const uint256& hashBlock, const uint256& hashTx)
{
    LOCK2(cs_main, cs_wallet); // check "LOCK2(cs_main, pwallet->cs_wallet);" in WalletBatch::LoadWallet()

    int conflictconfirms = 0;
    CBlockIndex* pindex = LookupBlockIndex(hashBlock);
    if (pindex && chainActive.Contains(pindex)) {
        conflictconfirms = -(chainActive.Height() - pindex->nHeight + 1);
    }
    // If number of conflict confirms cannot be determined, this means
    // that the block is still unknown or not yet part of the main chain,
    // for example when loading the wallet during a reindex. Do nothing in that
    // case.
    if (conflictconfirms >= 0)
        return;

    // Do not flush the wallet here for performance reasons
    WalletBatch batch(*database, "r+", false);

    std::set<uint256> todo;
    std::set<uint256> done;

    todo.insert(hashTx);

    while (!todo.empty()) {
        uint256 now = *todo.begin();
        todo.erase(now);
        done.insert(now);
        auto it = mapWallet.find(now);
        assert(it != mapWallet.end());
        CWalletTx& wtx = it->second;
        int currentconfirm = wtx.GetDepthInMainChain();
        if (conflictconfirms < currentconfirm) {
            // Block is 'more conflicted' than current confirm; update.
            // Mark transaction as conflicted with this block.
            wtx.nIndex = -1;
            wtx.hashBlock = hashBlock;
            wtx.MarkDirty();
            batch.WriteTx(wtx);
            // Iterate over all its outputs, and mark transactions in the wallet that spend them conflicted too
            TxSpends::const_iterator iter = mapTxSpends.lower_bound(COutPoint(now, 0));
            while (iter != mapTxSpends.end() && iter->first.hash == now) {
                 if (!done.count(iter->second)) {
                     todo.insert(iter->second);
                 }
                 iter++;
            }
            // If a transaction changes 'conflicted' state, that changes the balance
            // available of the outputs it spends. So force those to be recomputed
            for (const CTxIn& txin : wtx.tx->vin) {
                auto it = mapWallet.find(txin.prevout.hash);
                if (it != mapWallet.end()) {
                    it->second.MarkDirty();
                }
            }
        }
    }

    fAnonymizableTallyCached = false;
    fAnonymizableTallyCachedNonDenom = false;
}

void CWallet::SyncTransaction(const CTransactionRef& ptx, const CBlockIndex *pindex, int posInBlock) {
    const CTransaction& tx = *ptx;

    if (!AddToWalletIfInvolvingMe(ptx, pindex, posInBlock, true))
        return; // Not one of ours

    // If a transaction changes 'conflicted' state, that changes the balance
    // available of the outputs it spends. So force those to be
    // recomputed, also:
    for (const CTxIn& txin : tx.vin) {
        auto it = mapWallet.find(txin.prevout.hash);
        if (it != mapWallet.end()) {
            it->second.MarkDirty();
        }
    }

    fAnonymizableTallyCached = false;
    fAnonymizableTallyCachedNonDenom = false;
}

void CWallet::TransactionAddedToMempool(const CTransactionRef& ptx, int64_t nAcceptTime) {
    LOCK2(cs_main, cs_wallet);
    SyncTransaction(ptx);

    auto it = mapWallet.find(ptx->GetHash());
    if (it != mapWallet.end()) {
        it->second.fInMempool = true;
    }
}

void CWallet::TransactionRemovedFromMempool(const CTransactionRef &ptx) {
    LOCK(cs_wallet);
    auto it = mapWallet.find(ptx->GetHash());
    if (it != mapWallet.end()) {
        it->second.fInMempool = false;
    }
}

void CWallet::BlockConnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex *pindex, const std::vector<CTransactionRef>& vtxConflicted) {
    LOCK2(cs_main, cs_wallet);
    // TODO: Temporarily ensure that mempool removals are notified before
    // connected transactions.  This shouldn't matter, but the abandoned
    // state of transactions in our wallet is currently cleared when we
    // receive another notification and there is a race condition where
    // notification of a connected conflict might cause an outside process
    // to abandon a transaction and then have it inadvertently cleared by
    // the notification that the conflicted transaction was evicted.

    for (const CTransactionRef& ptx : vtxConflicted) {
        SyncTransaction(ptx);
        TransactionRemovedFromMempool(ptx);
    }
    for (size_t i = 0; i < pblock->vtx.size(); i++) {
        SyncTransaction(pblock->vtx[i], pindex, i);
        TransactionRemovedFromMempool(pblock->vtx[i]);
    }

    m_last_block_processed = pindex;

    // The GUI expects a NotifyTransactionChanged when a coinbase tx
    // which is in our wallet moves from in-the-best-block to
    // 2-confirmations (as it only displays them at that time).
    // We do that here.
    if (hashPrevBestCoinbase.IsNull()) {
        // Immediately after restart we have no idea what the coinbase
        // transaction from the previous block is.
        // For correctness we scan over the entire wallet, looking for
        // the previous block's coinbase, just in case it is ours, so
        // that we can notify the UI that it should now be displayed.
        if (pindex->pprev) {
            for (const std::pair<const uint256, CWalletTx>& p : mapWallet) {
                if (p.second.IsCoinBase() && p.second.hashBlock == pindex->pprev->GetBlockHash()) {
                    NotifyTransactionChanged(this, p.first, CT_UPDATED);
                    break;
                }
            }
        }
    } else {
        std::map<uint256, CWalletTx>::const_iterator mi = mapWallet.find(hashPrevBestCoinbase);
        if (mi != mapWallet.end()) {
            NotifyTransactionChanged(this, hashPrevBestCoinbase, CT_UPDATED);
        }
    }

    hashPrevBestCoinbase = pblock->vtx[0]->GetHash();

    // reset cache to make sure no longer immature coins are included
    fAnonymizableTallyCached = false;
    fAnonymizableTallyCachedNonDenom = false;
}

void CWallet::BlockDisconnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindexDisconnected) {
    LOCK2(cs_main, cs_wallet);

    for (const CTransactionRef& ptx : pblock->vtx) {
        // NOTE: do NOT pass pindex here
        SyncTransaction(ptx);
    }

    // reset cache to make sure no longer mature coins are excluded
    fAnonymizableTallyCached = false;
    fAnonymizableTallyCachedNonDenom = false;
}



void CWallet::BlockUntilSyncedToCurrentChain() {
    AssertLockNotHeld(cs_main);
    AssertLockNotHeld(cs_wallet);

    {
        // Skip the queue-draining stuff if we know we're caught up with
        // chainActive.Tip()...
        // We could also take cs_wallet here, and call m_last_block_processed
        // protected by cs_wallet instead of cs_main, but as long as we need
        // cs_main here anyway, it's easier to just call it cs_main-protected.
        LOCK(cs_main);
        const CBlockIndex* initialChainTip = chainActive.Tip();

        if (m_last_block_processed && m_last_block_processed->GetAncestor(initialChainTip->nHeight) == initialChainTip) {
            return;
        }
    }

    // ...otherwise put a callback in the validation interface queue and wait
    // for the queue to drain enough to execute it (indicating we are caught up
    // at least with the time we entered this function).
    SyncWithValidationInterfaceQueue();
}


isminetype CWallet::IsMine(const CTxIn &txin) const
{
    {
        LOCK(cs_wallet);
        std::map<uint256, CWalletTx>::const_iterator mi = mapWallet.find(txin.prevout.hash);
        if (mi != mapWallet.end())
        {
            const CWalletTx& prev = (*mi).second;
            if (txin.prevout.n < prev.tx->vout.size())
                return IsMine(prev.tx->vout[txin.prevout.n]);
        }
    }
    return ISMINE_NO;
}

// Note that this function doesn't distinguish between a 0-valued input,
// and a not-"is mine" (according to the filter) input.
CAmount CWallet::GetDebit(const CTxIn &txin, const isminefilter& filter) const
{
    {
        LOCK(cs_wallet);
        std::map<uint256, CWalletTx>::const_iterator mi = mapWallet.find(txin.prevout.hash);
        if (mi != mapWallet.end())
        {
            const CWalletTx& prev = (*mi).second;
            if (txin.prevout.n < prev.tx->vout.size())
                if (IsMine(prev.tx->vout[txin.prevout.n]) & filter)
                    return prev.tx->vout[txin.prevout.n].nValue;
        }
    }
    return 0;
}

// Recursively determine the rounds of a given input (How deep is the CoinJoin chain for a given input)
int CWallet::GetRealOutpointCoinJoinRounds(const COutPoint& outpoint, int nRounds) const
{
    LOCK(cs_wallet);

    const int nRoundsMax = MAX_COINJOIN_ROUNDS + CCoinJoinClientOptions::GetRandomRounds();

    if (nRounds >= nRoundsMax) {
        // there can only be nRoundsMax rounds max
        return nRoundsMax - 1;
    }

    auto pair = mapOutpointRoundsCache.emplace(outpoint, -10);
    auto nRoundsRef = &pair.first->second;
    if (!pair.second) {
        // we already processed it, just return what we have
        return *nRoundsRef;
    }

    // TODO wtx should refer to a CWalletTx object, not a pointer, based on surrounding code
    const CWalletTx* wtx = GetWalletTx(outpoint.hash);

    if (wtx == nullptr || wtx->tx == nullptr) {
        // no such tx in this wallet
        *nRoundsRef = -1;
        LogPrint(BCLog::COINJOIN, "%s FAILED    %-70s %3d\n", __func__, outpoint.ToStringShort(), -1);
        return *nRoundsRef;
    }

    // bounds check
    if (outpoint.n >= wtx->tx->vout.size()) {
        // should never actually hit this
        *nRoundsRef = -4;
        LogPrint(BCLog::COINJOIN, "%s FAILED    %-70s %3d\n", __func__, outpoint.ToStringShort(), -4);
        return *nRoundsRef;
    }

    auto txOutRef = &wtx->tx->vout[outpoint.n];

    if (CCoinJoin::IsCollateralAmount(txOutRef->nValue)) {
        *nRoundsRef = -3;
        LogPrint(BCLog::COINJOIN, "%s UPDATED   %-70s %3d\n", __func__, outpoint.ToStringShort(), *nRoundsRef);
        return *nRoundsRef;
    }

    // make sure the final output is non-denominate
    if (!CCoinJoin::IsDenominatedAmount(txOutRef->nValue)) { //NOT DENOM
        *nRoundsRef = -2;
        LogPrint(BCLog::COINJOIN, "%s UPDATED   %-70s %3d\n", __func__, outpoint.ToStringShort(), *nRoundsRef);
        return *nRoundsRef;
    }

    for (const auto& out : wtx->tx->vout) {
        if (!CCoinJoin::IsDenominatedAmount(out.nValue)) {
            // this one is denominated but there is another non-denominated output found in the same tx
            *nRoundsRef = 0;
            LogPrint(BCLog::COINJOIN, "%s UPDATED   %-70s %3d\n", __func__, outpoint.ToStringShort(), *nRoundsRef);
            return *nRoundsRef;
        }
    }

    int nShortest = -10; // an initial value, should be no way to get this by calculations
    bool fDenomFound = false;
    // only denoms here so let's look up
    for (const auto& txinNext : wtx->tx->vin) {
        if (IsMine(txinNext)) {
            int n = GetRealOutpointCoinJoinRounds(txinNext.prevout, nRounds + 1);
            // denom found, find the shortest chain or initially assign nShortest with the first found value
            if(n >= 0 && (n < nShortest || nShortest == -10)) {
                nShortest = n;
                fDenomFound = true;
            }
        }
    }
    *nRoundsRef = fDenomFound
            ? (nShortest >= nRoundsMax - 1 ? nRoundsMax : nShortest + 1) // good, we a +1 to the shortest one but only nRoundsMax rounds max allowed
            : 0;            // too bad, we are the fist one in that chain
    LogPrint(BCLog::COINJOIN, "%s UPDATED   %-70s %3d\n", __func__, outpoint.ToStringShort(), *nRoundsRef);
    return *nRoundsRef;
}

// respect current settings
int CWallet::GetCappedOutpointCoinJoinRounds(const COutPoint& outpoint) const
{
    LOCK(cs_wallet);
    int realCoinJoinRounds = GetRealOutpointCoinJoinRounds(outpoint);
    return realCoinJoinRounds > CCoinJoinClientOptions::GetRounds() ? CCoinJoinClientOptions::GetRounds() : realCoinJoinRounds;
}

bool CWallet::IsDenominated(const COutPoint& outpoint) const
{
    LOCK(cs_wallet);

    const auto it = mapWallet.find(outpoint.hash);
    if (it == mapWallet.end()) {
        return false;
    }

    if (outpoint.n >= it->second.tx->vout.size()) {
        return false;
    }

    return CCoinJoin::IsDenominatedAmount(it->second.tx->vout[outpoint.n].nValue);
}

bool CWallet::IsFullyMixed(const COutPoint& outpoint) const
{
    int nRounds = GetRealOutpointCoinJoinRounds(outpoint);
    // Mix again if we don't have N rounds yet
    if (nRounds < CCoinJoinClientOptions::GetRounds()) return false;

    // Try to mix a "random" number of rounds more than minimum.
    // If we have already mixed N + MaxOffset rounds, don't mix again.
    // Otherwise, we should mix again 50% of the time, this results in an exponential decay
    // N rounds 50% N+1 25% N+2 12.5%... until we reach N + GetRandomRounds() rounds where we stop.
    if (nRounds < CCoinJoinClientOptions::GetRounds() + CCoinJoinClientOptions::GetRandomRounds()) {
        CDataStream ss(SER_GETHASH, PROTOCOL_VERSION);
        ss << outpoint << nCoinJoinSalt;
        uint256 nHash;
        CSHA256().Write((const unsigned char*)ss.data(), ss.size()).Finalize(nHash.begin());
        if (nHash.GetCheapHash() % 2 == 0) {
            return false;
        }
    }

    return true;
}

isminetype CWallet::IsMine(const CTxOut& txout) const
{
    return ::IsMine(*this, txout.scriptPubKey);
}

CAmount CWallet::GetCredit(const CTxOut& txout, const isminefilter& filter) const
{
    if (!MoneyRange(txout.nValue))
        throw std::runtime_error(std::string(__func__) + ": value out of range");
    return ((IsMine(txout) & filter) ? txout.nValue : 0);
}

bool CWallet::IsChange(const CTxOut& txout) const
{
    // TODO: fix handling of 'change' outputs. The assumption is that any
    // payment to a script that is ours, but is not in the address book
    // is change. That assumption is likely to break when we implement multisignature
    // wallets that return change back into a multi-signature-protected address;
    // a better way of identifying which outputs are 'the send' and which are
    // 'the change' will need to be implemented (maybe extend CWalletTx to remember
    // which output, if any, was change).
    if (::IsMine(*this, txout.scriptPubKey))
    {
        CTxDestination address;
        if (!ExtractDestination(txout.scriptPubKey, address))
            return true;

        LOCK(cs_wallet);
        if (!mapAddressBook.count(address))
            return true;
    }
    return false;
}

CAmount CWallet::GetChange(const CTxOut& txout) const
{
    if (!MoneyRange(txout.nValue))
        throw std::runtime_error(std::string(__func__) + ": value out of range");
    return (IsChange(txout) ? txout.nValue : 0);
}

void CWallet::GenerateNewHDChain(const SecureString& secureMnemonic, const SecureString& secureMnemonicPassphrase)
{
    CHDChain newHdChain;

    // NOTE: an empty mnemonic means "generate a new one for me"
    // NOTE: default mnemonic passphrase is an empty string
    if (!newHdChain.SetMnemonic(secureMnemonic, secureMnemonicPassphrase, true)) {
        throw std::runtime_error(std::string(__func__) + ": SetMnemonic failed");
    }

    newHdChain.Debug(__func__);

    if (!SetHDChainSingle(newHdChain, false)) {
        throw std::runtime_error(std::string(__func__) + ": SetHDChainSingle failed");
    }

    if (!NewKeyPool()) {
        throw std::runtime_error(std::string(__func__) + ": NewKeyPool failed");
    }
}

bool CWallet::GenerateNewHDChainEncrypted(const SecureString& secureMnemonic, const SecureString& secureMnemonicPassphrase, const SecureString& secureWalletPassphrase)
{
    LOCK(cs_wallet);

    if (!IsCrypted()) {
        return false;
    }

    CCrypter crypter;
    CKeyingMaterial vMasterKey;
    CHDChain hdChainTmp;

    // NOTE: an empty mnemonic means "generate a new one for me"
    // NOTE: default mnemonic passphrase is an empty string
    if (!hdChainTmp.SetMnemonic(secureMnemonic, secureMnemonicPassphrase, true)) {
        throw std::runtime_error(std::string(__func__) + ": SetMnemonic failed");
    }

    hdChainTmp.Debug(__func__);

    for (const MasterKeyMap::value_type& pMasterKey : mapMasterKeys) {
        if (!crypter.SetKeyFromPassphrase(secureWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod)) {
            return false;
        }
        // get vMasterKey to encrypt new hdChain
        if (!crypter.Decrypt(pMasterKey.second.vchCryptedKey, vMasterKey)) {
            continue; // try another master key
        }

        bool res = EncryptHDChain(vMasterKey, hdChainTmp);
        assert(res);

        CHDChain hdChainCrypted;
        res = GetHDChain(hdChainCrypted);
        assert(res);

        DBG(
            printf("GenerateNewHDChainEncrypted -- current seed: '%s'\n", HexStr(hdChainTmp.GetSeed()).c_str());
            printf("GenerateNewHDChainEncrypted -- crypted seed: '%s'\n", HexStr(hdChainCrypted.GetSeed()).c_str());
        );

        // ids should match, seed hashes should not
        assert(hdChainTmp.GetID() == hdChainCrypted.GetID());
        assert(hdChainTmp.GetSeedHash() != hdChainCrypted.GetSeedHash());

        hdChainCrypted.Debug(__func__);

        if (SetCryptedHDChainSingle(hdChainCrypted, false)) {
            Lock();
            if (!Unlock(secureWalletPassphrase)) {
                // this should never happen
                throw std::runtime_error(std::string(__func__) + ": Unlock failed");
            }
            if (!NewKeyPool()) {
                throw std::runtime_error(std::string(__func__) + ": NewKeyPool failed");
            }
            Lock();
            return true;
        }
    }

    return false;
}

bool CWallet::SetHDChain(WalletBatch &batch, const CHDChain& chain, bool memonly)
{
    LOCK(cs_wallet);

    if (!CCryptoKeyStore::SetHDChain(chain))
        return false;

    if (!memonly && !batch.WriteHDChain(chain))
        throw std::runtime_error(std::string(__func__) + ": WriteHDChain failed");

    return true;
}

bool CWallet::SetCryptedHDChain(WalletBatch &batch, const CHDChain& chain, bool memonly)
{
    LOCK(cs_wallet);

    if (!CCryptoKeyStore::SetCryptedHDChain(chain))
        return false;

    if (!memonly) {
        if (encrypted_batch) {
            if (!encrypted_batch->WriteCryptedHDChain(chain))
                throw std::runtime_error(std::string(__func__) + ": WriteCryptedHDChain failed");
        } else {
            if (!batch.WriteCryptedHDChain(chain))
                throw std::runtime_error(std::string(__func__) + ": WriteCryptedHDChain failed");
        }
    }

    return true;
}

bool CWallet::SetHDChainSingle(const CHDChain& chain, bool memonly)
{
    WalletBatch batch(*database);
    return SetHDChain(batch, chain, memonly);
}

bool CWallet::SetCryptedHDChainSingle(const CHDChain& chain, bool memonly)
{
    WalletBatch batch(*database);
    return SetCryptedHDChain(batch, chain, memonly);
}

bool CWallet::GetDecryptedHDChain(CHDChain& hdChainRet)
{
    LOCK(cs_wallet);

    CHDChain hdChainTmp;
    if (!GetHDChain(hdChainTmp)) {
        return false;
    }

    if (!DecryptHDChain(hdChainTmp))
        return false;

    // make sure seed matches this chain
    if (hdChainTmp.GetID() != hdChainTmp.GetSeedHash())
        return false;

    hdChainRet = hdChainTmp;

    return true;
}

bool CWallet::IsHDEnabled() const
{
    CHDChain hdChainCurrent;
    return GetHDChain(hdChainCurrent);
}

bool CWallet::IsMine(const CTransaction& tx) const
{
    for (const CTxOut& txout : tx.vout)
        if (IsMine(txout))
            return true;
    return false;
}

bool CWallet::IsFromMe(const CTransaction& tx) const
{
    return (GetDebit(tx, ISMINE_ALL) > 0);
}

CAmount CWallet::GetDebit(const CTransaction& tx, const isminefilter& filter) const
{
    CAmount nDebit = 0;
    for (const CTxIn& txin : tx.vin)
    {
        nDebit += GetDebit(txin, filter);
        if (!MoneyRange(nDebit))
            throw std::runtime_error(std::string(__func__) + ": value out of range");
    }
    return nDebit;
}

bool CWallet::IsAllFromMe(const CTransaction& tx, const isminefilter& filter) const
{
    LOCK(cs_wallet);

    for (const CTxIn& txin : tx.vin)
    {
        auto mi = mapWallet.find(txin.prevout.hash);
        if (mi == mapWallet.end())
            return false; // any unknown inputs can't be from us

        const CWalletTx& prev = (*mi).second;

        if (txin.prevout.n >= prev.tx->vout.size())
            return false; // invalid input!

        if (!(IsMine(prev.tx->vout[txin.prevout.n]) & filter))
            return false;
    }
    return true;
}

CAmount CWallet::GetCredit(const CTransaction& tx, const isminefilter& filter) const
{
    CAmount nCredit = 0;
    for (const CTxOut& txout : tx.vout)
    {
        nCredit += GetCredit(txout, filter);
        if (!MoneyRange(nCredit))
            throw std::runtime_error(std::string(__func__) + ": value out of range");
    }
    return nCredit;
}

CAmount CWallet::GetChange(const CTransaction& tx) const
{
    CAmount nChange = 0;
    for (const CTxOut& txout : tx.vout)
    {
        nChange += GetChange(txout);
        if (!MoneyRange(nChange))
            throw std::runtime_error(std::string(__func__) + ": value out of range");
    }
    return nChange;
}

int64_t CWalletTx::GetTxTime() const
{
    int64_t n = nTimeSmart;
    return n ? n : nTimeReceived;
}

// Helper for producing a max-sized low-S signature (eg 72 bytes)
bool CWallet::DummySignInput(CTxIn &tx_in, const CTxOut &txout) const
{
    // Fill in dummy signatures for fee calculation.
    const CScript& scriptPubKey = txout.scriptPubKey;
    SignatureData sigdata;

    if (!ProduceSignature(DummySignatureCreator(this), scriptPubKey, sigdata))
    {
        return false;
    } else {
        UpdateInput(tx_in, sigdata);
    }
    return true;
}

// Helper for producing a bunch of max-sized low-S signatures (eg 72 bytes)
bool CWallet::DummySignTx(CMutableTransaction &txNew, const std::vector<CTxOut> &txouts) const
{
    // Fill in dummy signatures for fee calculation.
    int nIn = 0;
    for (const auto& txout : txouts)
    {
        if (!DummySignInput(txNew.vin[nIn], txout)) {
            return false;
        }

        nIn++;
    }
    return true;
}

int64_t CalculateMaximumSignedTxSize(const CTransaction &tx, const CWallet *wallet)
{
    std::vector<CTxOut> txouts;
    // Look up the inputs.  We should have already checked that this transaction
    // IsAllFromMe(ISMINE_SPENDABLE), so every input should already be in our
    // wallet, with a valid index into the vout array, and the ability to sign.
    for (auto& input : tx.vin) {
        const auto mi = wallet->mapWallet.find(input.prevout.hash);
        if (mi == wallet->mapWallet.end()) {
            return -1;
        }
        assert(input.prevout.n < mi->second.tx->vout.size());
        txouts.emplace_back(mi->second.tx->vout[input.prevout.n]);
    }
    return CalculateMaximumSignedTxSize(tx, wallet, txouts);
}

// txouts needs to be in the order of tx.vin
int64_t CalculateMaximumSignedTxSize(const CTransaction &tx, const CWallet *wallet, const std::vector<CTxOut>& txouts)
{
    CMutableTransaction txNew(tx);
    if (!wallet->DummySignTx(txNew, txouts)) {
        // This should never happen, because IsAllFromMe(ISMINE_SPENDABLE)
        // implies that we can sign for every input.
        return -1;
    }
    return ::GetSerializeSize(txNew, SER_NETWORK, PROTOCOL_VERSION);
}

int CalculateMaximumSignedInputSize(const CTxOut& txout, const CWallet* wallet)
{
    CMutableTransaction txn;
    txn.vin.push_back(CTxIn(COutPoint()));
    if (!wallet->DummySignInput(txn.vin[0], txout)) {
        // This should never happen, because IsAllFromMe(ISMINE_SPENDABLE)
        // implies that we can sign for every input.
        return -1;
    }
    return ::GetSerializeSize(txn.vin[0], SER_NETWORK, PROTOCOL_VERSION);
}

void CWalletTx::GetAmounts(std::list<COutputEntry>& listReceived,
                           std::list<COutputEntry>& listSent, CAmount& nFee, std::string& strSentAccount, const isminefilter& filter) const
{
    nFee = 0;
    listReceived.clear();
    listSent.clear();
    strSentAccount = strFromAccount;

    // Compute fee:
    CAmount nDebit = GetDebit(filter);
    if (nDebit > 0) // debit>0 means we signed/sent this transaction
    {
        CAmount nValueOut = tx->GetValueOut();
        nFee = nDebit - nValueOut;
    }

    // Sent/received.
    for (unsigned int i = 0; i < tx->vout.size(); ++i)
    {
        const CTxOut& txout = tx->vout[i];
        isminetype fIsMine = pwallet->IsMine(txout);
        // Only need to handle txouts if AT LEAST one of these is true:
        //   1) they debit from us (sent)
        //   2) the output is to us (received)
        if (nDebit > 0)
        {
            // Don't report 'change' txouts
            if (pwallet->IsChange(txout))
                continue;
        }
        else if (!(fIsMine & filter))
            continue;

        // In either case, we need to get the destination address
        CTxDestination address;

        if (!ExtractDestination(txout.scriptPubKey, address) && !txout.scriptPubKey.IsUnspendable())
        {
            LogPrintf("CWalletTx::GetAmounts: Unknown transaction type found, txid %s\n",
                     this->GetHash().ToString());
            address = CNoDestination();
        }

        COutputEntry output = {address, txout.nValue, (int)i};

        // If we are debited by the transaction, add the output as a "sent" entry
        if (nDebit > 0)
            listSent.push_back(output);

        // If we are receiving the output, add it as a "received" entry
        if (fIsMine & filter)
            listReceived.push_back(output);
    }

}

/**
 * Scan active chain for relevant transactions after importing keys. This should
 * be called whenever new keys are added to the wallet, with the oldest key
 * creation time.
 *
 * @return Earliest timestamp that could be successfully scanned from. Timestamp
 * returned will be higher than startTime if relevant blocks could not be read.
 */
int64_t CWallet::RescanFromTime(int64_t startTime, const WalletRescanReserver& reserver, bool update)
{
    // Find starting block. May be null if nCreateTime is greater than the
    // highest blockchain timestamp, in which case there is nothing that needs
    // to be scanned.
    CBlockIndex* startBlock = nullptr;
    {
        LOCK(cs_main);
        startBlock = chainActive.FindEarliestAtLeast(startTime - TIMESTAMP_WINDOW);
        LogPrintf("%s: Rescanning last %i blocks\n", __func__, startBlock ? chainActive.Height() - startBlock->nHeight + 1 : 0);
    }

    if (startBlock) {
        const CBlockIndex* const failedBlock = ScanForWalletTransactions(startBlock, nullptr, reserver, update);
        if (failedBlock) {
            return failedBlock->GetBlockTimeMax() + TIMESTAMP_WINDOW + 1;
        }
    }
    return startTime;
}

/**
 * Scan the block chain (starting in pindexStart) for transactions
 * from or to us. If fUpdate is true, found transactions that already
 * exist in the wallet will be updated.
 *
 * Returns null if scan was successful. Otherwise, if a complete rescan was not
 * possible (due to pruning or corruption), returns pointer to the most recent
 * block that could not be scanned.
 *
 * If pindexStop is not a nullptr, the scan will stop at the block-index
 * defined by pindexStop
 *
 * Caller needs to make sure pindexStop (and the optional pindexStart) are on
 * the main chain after to the addition of any new keys you want to detect
 * transactions for.
 */
CBlockIndex* CWallet::ScanForWalletTransactions(CBlockIndex* pindexStart, CBlockIndex* pindexStop, const WalletRescanReserver &reserver, bool fUpdate)
{
    int64_t nNow = GetTime();
    const CChainParams& chainParams = Params();

    assert(reserver.isReserved());
    if (pindexStop) {
        assert(pindexStop->nHeight >= pindexStart->nHeight);
    }

    CBlockIndex* pindex = pindexStart;
    CBlockIndex* ret = nullptr;

    if (pindex) LogPrintf("Rescan started from block %d...\n", pindex->nHeight);

    {
        fAbortRescan = false;
        ShowProgress(_("Rescanning..."), 0); // show rescan progress in GUI as dialog or on splashscreen, if -rescan on startup
        CBlockIndex* tip = nullptr;
        double progress_begin;
        double progress_end;
        {
            LOCK(cs_main);
            progress_begin = GuessVerificationProgress(chainParams.TxData(), pindex);
            if (pindexStop == nullptr) {
                tip = chainActive.Tip();
                progress_end = GuessVerificationProgress(chainParams.TxData(), tip);
            } else {
                progress_end = GuessVerificationProgress(chainParams.TxData(), pindexStop);
            }
        }
        double progress_current = progress_begin;
        while (pindex && !fAbortRescan && !ShutdownRequested())
        {
            m_scanning_progress = (progress_current - progress_begin) / (progress_end - progress_begin);
            if (pindex->nHeight % 100 == 0 && progress_end - progress_begin > 0.0) {
                ShowProgress(_("Rescanning..."), std::max(1, std::min(99, (int)(m_scanning_progress * 100))));
            }
            if (GetTime() >= nNow + 60) {
                nNow = GetTime();
                LogPrintf("Still rescanning. At block %d. Progress=%f\n", pindex->nHeight, progress_current);
            }

            CBlock block;
            if (ReadBlockFromDisk(block, pindex, Params().GetConsensus())) {
                LOCK2(cs_main, cs_wallet);
                if (pindex && !chainActive.Contains(pindex)) {
                    // Abort scan if current block is no longer active, to prevent
                    // marking transactions as coming from the wrong block.
                    ret = pindex;
                    break;
                }
                for (size_t posInBlock = 0; posInBlock < block.vtx.size(); ++posInBlock) {
                    AddToWalletIfInvolvingMe(block.vtx[posInBlock], pindex, posInBlock, fUpdate);
                }
            } else {
                ret = pindex;
            }
            if (pindex == pindexStop) {
                break;
            }
            {
                LOCK(cs_main);
                pindex = chainActive.Next(pindex);
                progress_current = GuessVerificationProgress(chainParams.TxData(), pindex);
                if (pindexStop == nullptr && tip != chainActive.Tip()) {
                    tip = chainActive.Tip();
                    // in case the tip has changed, update progress max
                    progress_end = GuessVerificationProgress(chainParams.TxData(), tip);
                }
            }
        }
        if (pindex && fAbortRescan) {
            LogPrintf("Rescan aborted at block %d. Progress=%f\n", pindex->nHeight, progress_current);
        } else if (pindex && ShutdownRequested()) {
            LogPrintf("Rescan interrupted by shutdown request at block %d. Progress=%f\n", pindex->nHeight, progress_current);
        }
        ShowProgress(_("Rescanning..."), 100); // hide progress dialog in GUI
    }
    return ret;
}

void CWallet::ReacceptWalletTransactions()
{
    // If transactions aren't being broadcasted, don't let them into local mempool either
    if (!fBroadcastTransactions)
        return;
    LOCK2(cs_main, mempool.cs);
    LOCK(cs_wallet);
    std::map<int64_t, CWalletTx*> mapSorted;

    // Sort pending wallet transactions based on their initial wallet insertion order
    for (std::pair<const uint256, CWalletTx>& item : mapWallet)
    {
        const uint256& wtxid = item.first;
        CWalletTx& wtx = item.second;
        assert(wtx.GetHash() == wtxid);

        int nDepth = wtx.GetDepthInMainChain();

        if (!wtx.IsCoinBase() && (nDepth == 0 && !wtx.IsLockedByInstantSend() && !wtx.isAbandoned())) {
            mapSorted.insert(std::make_pair(wtx.nOrderPos, &wtx));
        }
    }

    // Try to add wallet transactions to memory pool
    for (std::pair<const int64_t, CWalletTx*>& item : mapSorted) {
        CWalletTx& wtx = *(item.second);
        CValidationState state;
        wtx.AcceptToMemoryPool(maxTxFee, state);
    }
}

bool CWalletTx::RelayWalletTransaction(CConnman* connman)
{
    assert(pwallet->GetBroadcastTransactions());
    if (!IsCoinBase() && !isAbandoned() && GetDepthInMainChain() == 0)
    {
        CValidationState state;
        /* GetDepthInMainChain already catches known conflicts. */
        if (InMempool() || AcceptToMemoryPool(maxTxFee, state)) {
            uint256 hash = GetHash();
            LogPrintf("Relaying wtx %s\n", hash.ToString());

            if (connman) {
                connman->RelayTransaction(*tx);
                return true;
            }
        }
    }
    return false;
}

std::set<uint256> CWalletTx::GetConflicts() const
{
    std::set<uint256> result;
    if (pwallet != nullptr)
    {
        uint256 myHash = GetHash();
        result = pwallet->GetConflicts(myHash);
        result.erase(myHash);
    }
    return result;
}

CAmount CWalletTx::GetDebit(const isminefilter& filter) const
{
    if (tx->vin.empty())
        return 0;

    CAmount debit = 0;
    if(filter & ISMINE_SPENDABLE)
    {
        if (fDebitCached)
            debit += nDebitCached;
        else
        {
            nDebitCached = pwallet->GetDebit(*tx, ISMINE_SPENDABLE);
            fDebitCached = true;
            debit += nDebitCached;
        }
    }
    if(filter & ISMINE_WATCH_ONLY)
    {
        if(fWatchDebitCached)
            debit += nWatchDebitCached;
        else
        {
            nWatchDebitCached = pwallet->GetDebit(*tx, ISMINE_WATCH_ONLY);
            fWatchDebitCached = true;
            debit += nWatchDebitCached;
        }
    }
    return debit;
}

CAmount CWalletTx::GetCredit(const isminefilter& filter) const
{
    // Must wait until coinbase is safely deep enough in the chain before valuing it
    if (IsCoinBase() && GetBlocksToMaturity() > 0)
        return 0;

    CAmount credit = 0;
    if (filter & ISMINE_SPENDABLE)
    {
        // GetBalance can assume transactions in mapWallet won't change
        if (fCreditCached)
            credit += nCreditCached;
        else
        {
            nCreditCached = pwallet->GetCredit(*tx, ISMINE_SPENDABLE);
            fCreditCached = true;
            credit += nCreditCached;
        }
    }
    if (filter & ISMINE_WATCH_ONLY)
    {
        if (fWatchCreditCached)
            credit += nWatchCreditCached;
        else
        {
            nWatchCreditCached = pwallet->GetCredit(*tx, ISMINE_WATCH_ONLY);
            fWatchCreditCached = true;
            credit += nWatchCreditCached;
        }
    }
    return credit;
}

CAmount CWalletTx::GetImmatureCredit(bool fUseCache) const
{
    if (IsCoinBase() && GetBlocksToMaturity() > 0 && IsInMainChain())
    {
        if (fUseCache && fImmatureCreditCached)
            return nImmatureCreditCached;
        nImmatureCreditCached = pwallet->GetCredit(*tx, ISMINE_SPENDABLE);
        fImmatureCreditCached = true;
        return nImmatureCreditCached;
    }

    return 0;
}

CAmount CWalletTx::GetAvailableCredit(bool fUseCache, const isminefilter& filter) const
{
    if (pwallet == nullptr)
        return 0;

    // Must wait until coinbase is safely deep enough in the chain before valuing it
    if (IsCoinBase() && GetBlocksToMaturity() > 0)
        return 0;

    CAmount* cache = nullptr;
    bool* cache_used = nullptr;

    if (filter == ISMINE_SPENDABLE) {
        cache = &nAvailableCreditCached;
        cache_used = &fAvailableCreditCached;
    } else if (filter == ISMINE_WATCH_ONLY) {
        cache = &nAvailableWatchCreditCached;
        cache_used = &fAvailableWatchCreditCached;
    }

    if (fUseCache && cache_used && *cache_used) {
        return *cache;
    }

    CAmount nCredit = 0;
    uint256 hashTx = GetHash();
    for (unsigned int i = 0; i < tx->vout.size(); i++)
    {
        if (!pwallet->IsSpent(hashTx, i))
        {
            const CTxOut &txout = tx->vout[i];
            nCredit += pwallet->GetCredit(txout, filter);
            if (!MoneyRange(nCredit))
                throw std::runtime_error(std::string(__func__) + ": value out of range");
        }
    }

    if (cache) {
        *cache = nCredit;
        assert(cache_used);
        *cache_used = true;
    }
    return nCredit;
}

CAmount CWalletTx::GetImmatureWatchOnlyCredit(const bool fUseCache) const
{
    if (IsCoinBase() && GetBlocksToMaturity() > 0 && IsInMainChain())
    {
        if (fUseCache && fImmatureWatchCreditCached)
            return nImmatureWatchCreditCached;
        nImmatureWatchCreditCached = pwallet->GetCredit(*tx, ISMINE_WATCH_ONLY);
        fImmatureWatchCreditCached = true;
        return nImmatureWatchCreditCached;
    }

    return 0;
}

CAmount CWalletTx::GetAnonymizedCredit(const CCoinControl* coinControl) const
{
    if (!pwallet)
        return 0;

    // Exclude coinbase and conflicted txes
    if (IsCoinBase() || GetDepthInMainChain() < 0)
        return 0;

    if (coinControl == nullptr && fAnonymizedCreditCached)
        return nAnonymizedCreditCached;

    CAmount nCredit = 0;
    uint256 hashTx = GetHash();
    for (unsigned int i = 0; i < tx->vout.size(); i++)
    {
        const CTxOut &txout = tx->vout[i];
        const COutPoint outpoint = COutPoint(hashTx, i);

        if (coinControl != nullptr && coinControl->HasSelected() && !coinControl->IsSelected(outpoint)) {
            continue;
        }

        if (pwallet->IsSpent(hashTx, i) || !CCoinJoin::IsDenominatedAmount(txout.nValue)) continue;

        if (pwallet->IsFullyMixed(outpoint)) {
            nCredit += pwallet->GetCredit(txout, ISMINE_SPENDABLE);
            if (!MoneyRange(nCredit))
                throw std::runtime_error(std::string(__func__) + ": value out of range");
        }
    }

    if (coinControl == nullptr) {
        nAnonymizedCreditCached = nCredit;
        fAnonymizedCreditCached = true;
    }

    return nCredit;
}

CAmount CWalletTx::GetDenominatedCredit(bool unconfirmed, bool fUseCache) const
{
    if (pwallet == nullptr)
        return 0;

    // Must wait until coinbase is safely deep enough in the chain before valuing it
    if (IsCoinBase() && GetBlocksToMaturity() > 0)
        return 0;

    int nDepth = GetDepthInMainChain();
    if (nDepth < 0) return 0;

    bool isUnconfirmed = IsTrusted() && nDepth == 0;
    if (unconfirmed != isUnconfirmed) return 0;

    if (fUseCache) {
        if(unconfirmed && fDenomUnconfCreditCached)
            return nDenomUnconfCreditCached;
        else if (!unconfirmed && fDenomConfCreditCached)
            return nDenomConfCreditCached;
    }

    CAmount nCredit = 0;
    uint256 hashTx = GetHash();
    for (unsigned int i = 0; i < tx->vout.size(); i++)
    {
        const CTxOut &txout = tx->vout[i];

        if (pwallet->IsSpent(hashTx, i) || !CCoinJoin::IsDenominatedAmount(txout.nValue)) continue;

        nCredit += pwallet->GetCredit(txout, ISMINE_SPENDABLE);
        if (!MoneyRange(nCredit))
            throw std::runtime_error(std::string(__func__) + ": value out of range");
    }

    if (unconfirmed) {
        nDenomUnconfCreditCached = nCredit;
        fDenomUnconfCreditCached = true;
    } else {
        nDenomConfCreditCached = nCredit;
        fDenomConfCreditCached = true;
    }
    return nCredit;
}

CAmount CWalletTx::GetChange() const
{
    if (fChangeCached)
        return nChangeCached;
    nChangeCached = pwallet->GetChange(*tx);
    fChangeCached = true;
    return nChangeCached;
}

bool CWalletTx::InMempool() const
{
    return fInMempool;
}

bool CWalletTx::IsTrusted() const
{
    // Quick answer in most cases
    if (!CheckFinalTx(*tx))
        return false;
    int nDepth = GetDepthInMainChain();
    if (nDepth >= 1)
        return true;
    if (nDepth < 0)
        return false;
    if (IsLockedByInstantSend())
        return true;
    if (!bSpendZeroConfChange || !IsFromMe(ISMINE_ALL)) // using wtx's cached debit
        return false;

    // Don't trust unconfirmed transactions from us unless they are in the mempool.
    if (!InMempool())
        return false;

    // Trusted if all inputs are from us and are in the mempool:
    for (const CTxIn& txin : tx->vin)
    {
        // Transactions not sent by us: not trusted
        const CWalletTx* parent = pwallet->GetWalletTx(txin.prevout.hash);
        if (parent == nullptr)
            return false;
        const CTxOut& parentOut = parent->tx->vout[txin.prevout.n];
        if (pwallet->IsMine(parentOut) != ISMINE_SPENDABLE)
            return false;
    }
    return true;
}

bool CWalletTx::IsEquivalentTo(const CWalletTx& _tx) const
{
        CMutableTransaction tx1 = *this->tx;
        CMutableTransaction tx2 = *_tx.tx;
        for (auto& txin : tx1.vin) txin.scriptSig = CScript();
        for (auto& txin : tx2.vin) txin.scriptSig = CScript();
        return CTransaction(tx1) == CTransaction(tx2);
}

std::vector<uint256> CWallet::ResendWalletTransactionsBefore(int64_t nTime, CConnman* connman)
{
    std::vector<uint256> result;

    LOCK2(mempool.cs, cs_wallet);

    // Sort them in chronological order
    std::multimap<unsigned int, CWalletTx*> mapSorted;
    for (std::pair<const uint256, CWalletTx>& item : mapWallet)
    {
        CWalletTx& wtx = item.second;
        // Don't rebroadcast if newer than nTime:
        if (wtx.nTimeReceived > nTime)
            continue;
        mapSorted.insert(std::make_pair(wtx.nTimeReceived, &wtx));
    }
    for (std::pair<const unsigned int, CWalletTx*>& item : mapSorted)
    {
        CWalletTx& wtx = *item.second;
        if (wtx.RelayWalletTransaction(connman))
            result.push_back(wtx.GetHash());
    }
    return result;
}

void CWallet::ResendWalletTransactions(int64_t nBestBlockTime, CConnman* connman)
{
    // Do this infrequently and randomly to avoid giving away
    // that these are our transactions.
    if (GetTime() < nNextResend || !fBroadcastTransactions)
        return;
    bool fFirst = (nNextResend == 0);
    nNextResend = GetTime() + GetRand(30 * 60);
    if (fFirst)
        return;

    // Only do it if there's been a new block since last time
    if (nBestBlockTime < nLastResend)
        return;
    nLastResend = GetTime();

    // Rebroadcast unconfirmed txes older than 5 minutes before the last
    // block was found:
    std::vector<uint256> relayed = ResendWalletTransactionsBefore(nBestBlockTime-5*60, connman);
    if (!relayed.empty())
        LogPrintf("%s: rebroadcast %u unconfirmed transactions\n", __func__, relayed.size());
}

/** @} */ // end of mapWallet




/** @defgroup Actions
 *
 * @{
 */


std::unordered_set<const CWalletTx*, WalletTxHasher> CWallet::GetSpendableTXs() const
{
    AssertLockHeld(cs_wallet);

    std::unordered_set<const CWalletTx*, WalletTxHasher> ret;
    for (auto it = setWalletUTXO.begin(); it != setWalletUTXO.end(); ) {
        const auto& outpoint = *it;
        const auto jt = mapWallet.find(outpoint.hash);
        if (jt != mapWallet.end()) {
            ret.emplace(&jt->second);
        }

        // setWalletUTXO is sorted by COutPoint, which means that all UTXOs for the same TX are neighbors
        // skip entries until we encounter a new TX
        while (it != setWalletUTXO.end() && it->hash == outpoint.hash) {
            ++it;
        }
    }
    return ret;
}

CAmount CWallet::GetBalance(const isminefilter& filter, const int min_depth, const bool fAddLocked) const
{
    CAmount nTotal = 0;
    {
        LOCK2(cs_main, cs_wallet);
        for (auto pcoin : GetSpendableTXs()) {
            if (pcoin->IsTrusted() && ((pcoin->GetDepthInMainChain() >= min_depth) || (fAddLocked && pcoin->IsLockedByInstantSend()))) {
                nTotal += pcoin->GetAvailableCredit(true, filter);
            }
        }
    }

    return nTotal;
}

CAmount CWallet::GetAnonymizableBalance(bool fSkipDenominated, bool fSkipUnconfirmed) const
{
    if (!CCoinJoinClientOptions::IsEnabled()) return 0;

    std::vector<CompactTallyItem> vecTally;
    if(!SelectCoinsGroupedByAddresses(vecTally, fSkipDenominated, true, fSkipUnconfirmed)) return 0;

    CAmount nTotal = 0;

    const CAmount nSmallestDenom = CCoinJoin::GetSmallestDenomination();
    const CAmount nMixingCollateral = CCoinJoin::GetCollateralAmount();
    for (const auto& item : vecTally) {
        bool fIsDenominated = CCoinJoin::IsDenominatedAmount(item.nAmount);
        if(fSkipDenominated && fIsDenominated) continue;
        // assume that the fee to create denoms should be mixing collateral at max
        if(item.nAmount >= nSmallestDenom + (fIsDenominated ? 0 : nMixingCollateral))
            nTotal += item.nAmount;
    }

    return nTotal;
}

CAmount CWallet::GetAnonymizedBalance(const CCoinControl* coinControl) const
{
    if (!CCoinJoinClientOptions::IsEnabled()) return 0;

    CAmount nTotal = 0;

    LOCK2(cs_main, cs_wallet);

    for (auto pcoin : GetSpendableTXs()) {
        nTotal += pcoin->GetAnonymizedCredit(coinControl);
    }

    return nTotal;
}

// Note: calculated including unconfirmed,
// that's ok as long as we use it for informational purposes only
float CWallet::GetAverageAnonymizedRounds() const
{
    if (!CCoinJoinClientOptions::IsEnabled()) return 0;

    int nTotal = 0;
    int nCount = 0;

    LOCK2(cs_main, cs_wallet);
    for (const auto& outpoint : setWalletUTXO) {
        if(!IsDenominated(outpoint)) continue;

        nTotal += GetCappedOutpointCoinJoinRounds(outpoint);
        nCount++;
    }

    if(nCount == 0) return 0;

    return (float)nTotal/nCount;
}

// Note: calculated including unconfirmed,
// that's ok as long as we use it for informational purposes only
CAmount CWallet::GetNormalizedAnonymizedBalance() const
{
    if (!CCoinJoinClientOptions::IsEnabled()) return 0;

    CAmount nTotal = 0;

    LOCK2(cs_main, cs_wallet);
    for (const auto& outpoint : setWalletUTXO) {
        const auto it = mapWallet.find(outpoint.hash);
        if (it == mapWallet.end()) continue;

        CAmount nValue = it->second.tx->vout[outpoint.n].nValue;
        if (!CCoinJoin::IsDenominatedAmount(nValue)) continue;
        if (it->second.GetDepthInMainChain() < 0) continue;

        int nRounds = GetCappedOutpointCoinJoinRounds(outpoint);
        nTotal += nValue * nRounds / CCoinJoinClientOptions::GetRounds();
    }

    return nTotal;
}

CAmount CWallet::GetDenominatedBalance(bool unconfirmed) const
{
    if (!CCoinJoinClientOptions::IsEnabled()) return 0;

    CAmount nTotal = 0;

    LOCK2(cs_main, cs_wallet);

    for (auto pcoin : GetSpendableTXs()) {
        nTotal += pcoin->GetDenominatedCredit(unconfirmed);
    }

    return nTotal;
}

CAmount CWallet::GetUnconfirmedBalance() const
{
    CAmount nTotal = 0;
    {
        LOCK2(cs_main, cs_wallet);
        for (auto pcoin : GetSpendableTXs()) {
            if (!pcoin->IsTrusted() && pcoin->GetDepthInMainChain() == 0 && !pcoin->IsLockedByInstantSend() && pcoin->InMempool())
                nTotal += pcoin->GetAvailableCredit();
        }
    }
    return nTotal;
}

CAmount CWallet::GetImmatureBalance() const
{
    CAmount nTotal = 0;
    {
        LOCK2(cs_main, cs_wallet);
        for (auto pcoin : GetSpendableTXs()) {
            nTotal += pcoin->GetImmatureCredit();
        }
    }
    return nTotal;
}

CAmount CWallet::GetUnconfirmedWatchOnlyBalance() const
{
    CAmount nTotal = 0;
    {
        LOCK2(cs_main, cs_wallet);
        for (auto pcoin : GetSpendableTXs()) {
            if (!pcoin->IsTrusted() && pcoin->GetDepthInMainChain() == 0 && !pcoin->IsLockedByInstantSend() && pcoin->InMempool())
                nTotal += pcoin->GetAvailableCredit(true, ISMINE_WATCH_ONLY);
        }
    }
    return nTotal;
}

CAmount CWallet::GetImmatureWatchOnlyBalance() const
{
    CAmount nTotal = 0;
    {
        LOCK2(cs_main, cs_wallet);
        for (auto pcoin : GetSpendableTXs()) {
            nTotal += pcoin->GetImmatureWatchOnlyCredit();
        }
    }
    return nTotal;
}

// Calculate total balance in a different way from GetBalance. The biggest
// difference is that GetBalance sums up all unspent TxOuts paying to the
// wallet, while this sums up both spent and unspent TxOuts paying to the
// wallet, and then subtracts the values of TxIns spending from the wallet. This
// also has fewer restrictions on which unconfirmed transactions are considered
// trusted.
CAmount CWallet::GetLegacyBalance(const isminefilter& filter, int minDepth, const std::string* account, const bool fAddLocked) const
{
    LOCK2(cs_main, cs_wallet);

    CAmount balance = 0;
    for (const auto& entry : mapWallet) {
        const CWalletTx& wtx = entry.second;
        const int depth = wtx.GetDepthInMainChain();
        if (depth < 0 || !CheckFinalTx(*wtx.tx) || wtx.GetBlocksToMaturity() > 0) {
            continue;
        }

        // Loop through tx outputs and add incoming payments. For outgoing txs,
        // treat change outputs specially, as part of the amount debited.
        CAmount debit = wtx.GetDebit(filter);
        const bool outgoing = debit > 0;
        for (const CTxOut& out : wtx.tx->vout) {
            if (outgoing && IsChange(out)) {
                debit -= out.nValue;
            } else if (IsMine(out) & filter && (depth >= minDepth || (fAddLocked && wtx.IsLockedByInstantSend())) && (!account || *account == GetLabelName(out.scriptPubKey))) {
                balance += out.nValue;
            }
        }

        // For outgoing txs, subtract amount debited.
        if (outgoing && (!account || *account == wtx.strFromAccount)) {
            balance -= debit;
        }
    }

    if (account) {
        balance += WalletBatch(*database).GetAccountCreditDebit(*account);
    }

    return balance;
}

CAmount CWallet::GetAvailableBalance(const CCoinControl* coinControl) const
{
    LOCK2(cs_main, cs_wallet);

    CAmount balance = 0;
    std::vector<COutput> vCoins;
    AvailableCoins(vCoins, true, coinControl);
    for (const COutput& out : vCoins) {
        if (out.fSpendable) {
            balance += out.tx->tx->vout[out.i].nValue;
        }
    }
    return balance;
}

void CWallet::AvailableCoins(std::vector<COutput> &vCoins, bool fOnlySafe, const CCoinControl *coinControl, const CAmount &nMinimumAmount, const CAmount &nMaximumAmount, const CAmount &nMinimumSumAmount, const uint64_t nMaximumCount, const int nMinDepth, const int nMaxDepth) const
{
    AssertLockHeld(cs_main);
    AssertLockHeld(cs_wallet);

    vCoins.clear();
    CoinType nCoinType = coinControl ? coinControl->nCoinType : CoinType::ALL_COINS;

    CAmount nTotal = 0;

    for (auto pcoin : GetSpendableTXs()) {
        const uint256& wtxid = pcoin->GetHash();

        if (!CheckFinalTx(*pcoin->tx))
            continue;

        if (pcoin->IsCoinBase() && pcoin->GetBlocksToMaturity() > 0)
            continue;

        int nDepth = pcoin->GetDepthInMainChain();

        // We should not consider coins which aren't at least in our mempool
        // It's possible for these to be conflicted via ancestors which we may never be able to detect
        if (nDepth == 0 && !pcoin->InMempool())
            continue;

        bool safeTx = pcoin->IsTrusted();

        if (fOnlySafe && !safeTx) {
            continue;
        }

        if (nDepth < nMinDepth || nDepth > nMaxDepth)
            continue;

        for (unsigned int i = 0; i < pcoin->tx->vout.size(); i++) {
            bool found = false;
            if (nCoinType == CoinType::ONLY_FULLY_MIXED) {
                if (!CCoinJoin::IsDenominatedAmount(pcoin->tx->vout[i].nValue)) continue;
                found = IsFullyMixed(COutPoint(wtxid, i));
            } else if(nCoinType == CoinType::ONLY_READY_TO_MIX) {
                if (!CCoinJoin::IsDenominatedAmount(pcoin->tx->vout[i].nValue)) continue;
                found = !IsFullyMixed(COutPoint(wtxid, i));
            } else if(nCoinType == CoinType::ONLY_NONDENOMINATED) {
                if (CCoinJoin::IsCollateralAmount(pcoin->tx->vout[i].nValue)) continue; // do not use collateral amounts
                found = !CCoinJoin::IsDenominatedAmount(pcoin->tx->vout[i].nValue);
            } else if(nCoinType == CoinType::ONLY_MASTERNODE_COLLATERAL) {
                found = pcoin->tx->vout[i].nValue == 1000*COIN;
            } else if(nCoinType == CoinType::ONLY_COINJOIN_COLLATERAL) {
                found = CCoinJoin::IsCollateralAmount(pcoin->tx->vout[i].nValue);
            } else {
                found = true;
            }
            if(!found) continue;

            if (pcoin->tx->vout[i].nValue < nMinimumAmount || pcoin->tx->vout[i].nValue > nMaximumAmount)
                continue;

            if (coinControl && coinControl->HasSelected() && !coinControl->fAllowOtherInputs && !coinControl->IsSelected(COutPoint(wtxid, i)))
                continue;

            if (IsLockedCoin(wtxid, i) && nCoinType != CoinType::ONLY_MASTERNODE_COLLATERAL)
                continue;

            if (IsSpent(wtxid, i))
                continue;

            isminetype mine = IsMine(pcoin->tx->vout[i]);

            if (mine == ISMINE_NO) {
                continue;
            }

            bool fSpendableIn = ((mine & ISMINE_SPENDABLE) != ISMINE_NO) || (coinControl && coinControl->fAllowWatchOnly && (mine & ISMINE_WATCH_SOLVABLE) != ISMINE_NO);
            bool fSolvableIn = (mine & (ISMINE_SPENDABLE | ISMINE_WATCH_SOLVABLE)) != ISMINE_NO;

            vCoins.push_back(COutput(pcoin, i, nDepth, fSpendableIn, fSolvableIn, safeTx));

            // Checks the sum amount of all UTXO's.
            if (nMinimumSumAmount != MAX_MONEY) {
                nTotal += pcoin->tx->vout[i].nValue;

                if (nTotal >= nMinimumSumAmount) {
                    return;
                }
            }

            // Checks the maximum number of UTXO's.
            if (nMaximumCount > 0 && vCoins.size() >= nMaximumCount) {
                return;
            }
        }
    }
}

std::map<CTxDestination, std::vector<COutput>> CWallet::ListCoins() const
{
    // TODO: Add AssertLockHeld(cs_wallet) here.
    //
    // Because the return value from this function contains pointers to
    // CWalletTx objects, callers to this function really should acquire the
    // cs_wallet lock before calling it. However, the current caller doesn't
    // acquire this lock yet. There was an attempt to add the missing lock in
    // https://github.com/bitcoin/bitcoin/pull/10340, but that change has been
    // postponed until after https://github.com/bitcoin/bitcoin/pull/10244 to
    // avoid adding some extra complexity to the Qt code.

    std::map<CTxDestination, std::vector<COutput>> result;
    std::vector<COutput> availableCoins;

    LOCK2(cs_main, cs_wallet);
    AvailableCoins(availableCoins);

    for (auto& coin : availableCoins) {
        CTxDestination address;
        if (coin.fSpendable &&
            ExtractDestination(FindNonChangeParentOutput(*coin.tx->tx, coin.i).scriptPubKey, address)) {
            result[address].emplace_back(std::move(coin));
        }
    }

    std::vector<COutPoint> lockedCoins;
    ListLockedCoins(lockedCoins);
    for (const auto& output : lockedCoins) {
        auto it = mapWallet.find(output.hash);
        if (it != mapWallet.end()) {
            int depth = it->second.GetDepthInMainChain();
            if (depth >= 0 && output.n < it->second.tx->vout.size() &&
                IsMine(it->second.tx->vout[output.n]) == ISMINE_SPENDABLE) {
                CTxDestination address;
                if (ExtractDestination(FindNonChangeParentOutput(*it->second.tx, output.n).scriptPubKey, address)) {
                    result[address].emplace_back(
                        &it->second, output.n, depth, true /* spendable */, true /* solvable */, false /* safe */);
                }
            }
        }
    }

    return result;
}

const CTxOut& CWallet::FindNonChangeParentOutput(const CTransaction& tx, int output) const
{
    const CTransaction* ptx = &tx;
    int n = output;
    while (IsChange(ptx->vout[n]) && ptx->vin.size() > 0) {
        const COutPoint& prevout = ptx->vin[0].prevout;
        auto it = mapWallet.find(prevout.hash);
        if (it == mapWallet.end() || it->second.tx->vout.size() <= prevout.n ||
            !IsMine(it->second.tx->vout[prevout.n])) {
            break;
        }
        ptx = it->second.tx.get();
        n = prevout.n;
    }
    return ptx->vout[n];
}

void CWallet::InitCoinJoinSalt()
{
    // Avoid fetching it multiple times
    assert(nCoinJoinSalt.IsNull());

    WalletBatch batch(*database);
    if (!batch.ReadCoinJoinSalt(nCoinJoinSalt) && batch.ReadCoinJoinSalt(nCoinJoinSalt, true)) {
        batch.WriteCoinJoinSalt(nCoinJoinSalt);
    }

    while (nCoinJoinSalt.IsNull()) {
        // We never generated/saved it
        nCoinJoinSalt = GetRandHash();
        batch.WriteCoinJoinSalt(nCoinJoinSalt);
    }
}

struct CompareByPriority
{
    bool operator()(const COutput& t1,
                    const COutput& t2) const
    {
        return t1.Priority() > t2.Priority();
    }
};

bool CWallet::OutputEligibleForSpending(const COutput& output, const CoinEligibilityFilter& eligibility_filter) const
{
    if (!output.fSpendable)
        return false;

    bool fLockedByIS = output.tx->IsLockedByInstantSend();

    if ((output.nDepth < (output.tx->IsFromMe(ISMINE_ALL) ? eligibility_filter.conf_mine : eligibility_filter.conf_theirs)) && !fLockedByIS)
        return false;

    size_t ancestors, descendants;
    mempool.GetTransactionAncestry(output.tx->GetHash(), ancestors, descendants);
    if (ancestors > eligibility_filter.max_ancestors || descendants > eligibility_filter.max_descendants) {
        return false;
    }

    return true;
}

bool CWallet::SelectCoinsMinConf(const CAmount& nTargetValue, const CoinEligibilityFilter& eligibility_filter, std::vector<COutput> vCoins,
                                 std::set<CInputCoin>& setCoinsRet, CAmount& nValueRet, const CoinSelectionParams& coin_selection_params, bool& bnb_used, CoinType nCoinType) const
{
    setCoinsRet.clear();
    nValueRet = 0;

    std::vector<CInputCoin> utxo_pool;
    if (coin_selection_params.use_bnb) {

        // Get long term estimate
        FeeCalculation feeCalc;
        CCoinControl temp;
        temp.m_confirm_target = 1008;
        CFeeRate long_term_feerate = GetMinimumFeeRate(temp, ::mempool, ::feeEstimator, &feeCalc);

        // Calculate cost of change
        CAmount cost_of_change = GetDiscardRate(::feeEstimator).GetFee(coin_selection_params.change_spend_size) + coin_selection_params.effective_fee.GetFee(coin_selection_params.change_output_size);

        // Filter by the min conf specs and add to utxo_pool and calculate effective value
        for (const COutput &output : vCoins)
        {
            if (!OutputEligibleForSpending(output, eligibility_filter))
                continue;

            CInputCoin coin(output.tx->tx, output.i);
            coin.effective_value = coin.txout.nValue - (output.nInputBytes < 0 ? 0 : coin_selection_params.effective_fee.GetFee(output.nInputBytes));
            // Only include outputs that are positive effective value (i.e. not dust)
            if (coin.effective_value > 0) {
                coin.fee = output.nInputBytes < 0 ? 0 : coin_selection_params.effective_fee.GetFee(output.nInputBytes);
                coin.long_term_fee = output.nInputBytes < 0 ? 0 : long_term_feerate.GetFee(output.nInputBytes);
                utxo_pool.push_back(coin);
            }
        }
        // Calculate the fees for things that aren't inputs
        CAmount not_input_fees = coin_selection_params.effective_fee.GetFee(coin_selection_params.tx_noinputs_size);
        bnb_used = true;
        return SelectCoinsBnB(utxo_pool, nTargetValue, cost_of_change, setCoinsRet, nValueRet, not_input_fees);
    } else {
        // Filter by the min conf specs and add to utxo_pool
        for (const COutput &output : vCoins)
        {
            if (!OutputEligibleForSpending(output, eligibility_filter))
                continue;

            CInputCoin coin = CInputCoin(output.tx->tx, output.i);
            utxo_pool.push_back(coin);
        }
        bnb_used = false;
        return KnapsackSolver(nTargetValue, utxo_pool, setCoinsRet, nValueRet, nCoinType == CoinType::ONLY_FULLY_MIXED, maxTxFee);
    }
}

bool CWallet::SelectCoins(const std::vector<COutput>& vAvailableCoins, const CAmount& nTargetValue, std::set<CInputCoin>& setCoinsRet, CAmount& nValueRet, const CCoinControl& coin_control, CoinSelectionParams& coin_selection_params, bool& bnb_used) const
{
    // Note: this function should never be used for "always free" tx types like dstx

    std::vector<COutput> vCoins(vAvailableCoins);
    CoinType nCoinType = coin_control.nCoinType;

    // coin control -> return all selected outputs (we want all selected to go into the transaction for sure)
    if (coin_control.HasSelected() && !coin_control.fAllowOtherInputs)
    {
        // We didn't use BnB here, so set it to false.
        bnb_used = false;

        for (const COutput& out : vCoins)
        {
            if(!out.fSpendable)
                continue;

            nValueRet += out.tx->tx->vout[out.i].nValue;
            setCoinsRet.insert(CInputCoin(out.tx->tx, out.i));

            if (!coin_control.fRequireAllInputs && nValueRet >= nTargetValue) {
                // stop when we added at least one input and enough inputs to have at least nTargetValue funds
                return true;
            }
        }

        return (nValueRet >= nTargetValue);
    }

    // calculate value from preset inputs and store them
    std::set<CInputCoin> setPresetCoins;
    CAmount nValueFromPresetInputs = 0;

    std::vector<COutPoint> vPresetInputs;
    coin_control.ListSelected(vPresetInputs);
    for (const COutPoint& outpoint : vPresetInputs)
    {
        // For now, don't use BnB if preset inputs are selected. TODO: Enable this later
        bnb_used = false;
        coin_selection_params.use_bnb = false;

        std::map<uint256, CWalletTx>::const_iterator it = mapWallet.find(outpoint.hash);
        if (it != mapWallet.end())
        {
            const CWalletTx* pcoin = &it->second;
            // Clearly invalid input, fail
            if (pcoin->tx->vout.size() <= outpoint.n)
                return false;
            if (nCoinType == CoinType::ONLY_FULLY_MIXED) {
                // Make sure to include mixed preset inputs only,
                // even if some non-mixed inputs were manually selected via CoinControl
                if (!IsFullyMixed(outpoint)) continue;
            }
            // Just to calculate the marginal byte size
            nValueFromPresetInputs += pcoin->tx->vout[outpoint.n].nValue;
            setPresetCoins.insert(CInputCoin(pcoin->tx, outpoint.n));
        } else
            return false; // TODO: Allow non-wallet inputs
    }

    // remove preset inputs from vCoins
    for (std::vector<COutput>::iterator it = vCoins.begin(); it != vCoins.end() && coin_control.HasSelected();)
    {
        if (setPresetCoins.count(CInputCoin(it->tx->tx, it->i)))
            it = vCoins.erase(it);
        else
            ++it;
    }

    size_t max_ancestors = (size_t)std::max<int64_t>(1, gArgs.GetArg("-limitancestorcount", DEFAULT_ANCESTOR_LIMIT));
    size_t max_descendants = (size_t)std::max<int64_t>(1, gArgs.GetArg("-limitdescendantcount", DEFAULT_DESCENDANT_LIMIT));
    bool fRejectLongChains = gArgs.GetBoolArg("-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS);

    bool res = nTargetValue <= nValueFromPresetInputs ||
        SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, CoinEligibilityFilter(1, 6, 0), vCoins, setCoinsRet, nValueRet, coin_selection_params, bnb_used, nCoinType) ||
        SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, CoinEligibilityFilter(1, 1, 0), vCoins, setCoinsRet, nValueRet, coin_selection_params, bnb_used, nCoinType) ||
        (bSpendZeroConfChange && SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, CoinEligibilityFilter(0, 1, 2), vCoins, setCoinsRet, nValueRet, coin_selection_params, bnb_used, nCoinType)) ||
        (bSpendZeroConfChange && SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, CoinEligibilityFilter(0, 1, std::min((size_t)4, max_ancestors/3), std::min((size_t)4, max_descendants/3)), vCoins, setCoinsRet, nValueRet, coin_selection_params, bnb_used, nCoinType)) ||
        (bSpendZeroConfChange && SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, CoinEligibilityFilter(0, 1, max_ancestors/2, max_descendants/2), vCoins, setCoinsRet, nValueRet, coin_selection_params, bnb_used, nCoinType)) ||
        (bSpendZeroConfChange && SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, CoinEligibilityFilter(0, 1, max_ancestors-1, max_descendants-1), vCoins, setCoinsRet, nValueRet, coin_selection_params, bnb_used, nCoinType)) ||
        (bSpendZeroConfChange && !fRejectLongChains && SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, CoinEligibilityFilter(0, 1, std::numeric_limits<uint64_t>::max()), vCoins, setCoinsRet, nValueRet, coin_selection_params, bnb_used, nCoinType));

    // because SelectCoinsMinConf clears the setCoinsRet, we now add the possible inputs to the coinset
    setCoinsRet.insert(setPresetCoins.begin(), setPresetCoins.end());

    // add preset inputs to the total value selected
    nValueRet += nValueFromPresetInputs;

    return res;
}

bool CWallet::FundTransaction(CMutableTransaction& tx, CAmount& nFeeRet, int& nChangePosInOut, std::string& strFailReason, bool lockUnspents, const std::set<int>& setSubtractFeeFromOutputs, CCoinControl coinControl)
{
    std::vector<CRecipient> vecSend;

    // Turn the txout set into a CRecipient vector.
    for (size_t idx = 0; idx < tx.vout.size(); idx++) {
        const CTxOut& txOut = tx.vout[idx];
        CRecipient recipient = {txOut.scriptPubKey, txOut.nValue, setSubtractFeeFromOutputs.count(idx) == 1};
        vecSend.push_back(recipient);
    }

    coinControl.fAllowOtherInputs = true;

    for (const CTxIn& txin : tx.vin) {
        coinControl.Select(txin.prevout);
    }

    // Acquire the locks to prevent races to the new locked unspents between the
    // CreateTransaction call and LockCoin calls (when lockUnspents is true).
    LOCK2(cs_main, mempool.cs);
    LOCK(cs_wallet);

    int nExtraPayloadSize = 0;
    if (tx.nVersion == 3 && tx.nType != TRANSACTION_NORMAL)
        nExtraPayloadSize = (int)tx.vExtraPayload.size();

    CReserveKey reservekey(this);
    CTransactionRef tx_new;
    if (!CreateTransaction(vecSend, tx_new, reservekey, nFeeRet, nChangePosInOut, strFailReason, coinControl, false, nExtraPayloadSize)) {
        return false;
    }

    if (nChangePosInOut != -1) {
        tx.vout.insert(tx.vout.begin() + nChangePosInOut, tx_new->vout[nChangePosInOut]);
        // We don't have the normal Create/Commit cycle, and don't want to risk
        // reusing change, so just remove the key from the keypool here.
        reservekey.KeepKey();
    }

    // Copy output sizes from new transaction; they may have had the fee
    // subtracted from them.
    for (unsigned int idx = 0; idx < tx.vout.size(); idx++) {
        tx.vout[idx].nValue = tx_new->vout[idx].nValue;
    }

    // Add new txins while keeping original txin scriptSig/order.
    for (const CTxIn& txin : tx_new->vin) {
        if (!coinControl.IsSelected(txin.prevout)) {
            tx.vin.push_back(txin);

            if (lockUnspents) {
                LockCoin(txin.prevout);
            }
        }
    }

    return true;
}

bool CWallet::SelectTxDSInsByDenomination(int nDenom, CAmount nValueMax, std::vector<CTxDSIn>& vecTxDSInRet)
{
    LOCK2(cs_main, cs_wallet);

    CAmount nValueTotal{0};

    std::set<uint256> setRecentTxIds;
    std::vector<COutput> vCoins;

    vecTxDSInRet.clear();

    if (!CCoinJoin::IsValidDenomination(nDenom)) {
        return false;
    }
    CAmount nDenomAmount = CCoinJoin::DenominationToAmount(nDenom);

    CCoinControl coin_control;
    coin_control.nCoinType = CoinType::ONLY_READY_TO_MIX;
    AvailableCoins(vCoins, true, &coin_control);
    LogPrint(BCLog::COINJOIN, "CWallet::%s -- vCoins.size(): %d\n", __func__, vCoins.size());

    Shuffle(vCoins.rbegin(), vCoins.rend(), FastRandomContext());

    for (const auto& out : vCoins) {
        uint256 txHash = out.tx->GetHash();
        CAmount nValue = out.tx->tx->vout[out.i].nValue;
        if (setRecentTxIds.find(txHash) != setRecentTxIds.end()) continue; // no duplicate txids
        if (nValueTotal + nValue > nValueMax) continue;
        if (nValue != nDenomAmount) continue;

        CTxIn txin = CTxIn(txHash, out.i);
        CScript scriptPubKey = out.tx->tx->vout[out.i].scriptPubKey;
        int nRounds = GetRealOutpointCoinJoinRounds(txin.prevout);

        nValueTotal += nValue;
        vecTxDSInRet.emplace_back(CTxDSIn(txin, scriptPubKey, nRounds));
        setRecentTxIds.emplace(txHash);
        LogPrint(BCLog::COINJOIN, "CWallet::%s -- hash: %s, nValue: %d.%08d\n",
                        __func__, txHash.ToString(), nValue / COIN, nValue % COIN);
    }

    LogPrint(BCLog::COINJOIN, "CWallet::%s -- setRecentTxIds.size(): %d\n", __func__, setRecentTxIds.size());

    return nValueTotal > 0;
}

bool CWallet::SelectCoinsGroupedByAddresses(std::vector<CompactTallyItem>& vecTallyRet, bool fSkipDenominated, bool fAnonymizable, bool fSkipUnconfirmed, int nMaxOupointsPerAddress) const
{
    LOCK2(cs_main, cs_wallet);

    isminefilter filter = ISMINE_SPENDABLE;

    // Try using the cache for already confirmed mixable inputs.
    // This should only be used if nMaxOupointsPerAddress was NOT specified.
    if(nMaxOupointsPerAddress == -1 && fAnonymizable && fSkipUnconfirmed) {
        if(fSkipDenominated && fAnonymizableTallyCachedNonDenom) {
            vecTallyRet = vecAnonymizableTallyCachedNonDenom;
            LogPrint(BCLog::SELECTCOINS, "SelectCoinsGroupedByAddresses - using cache for non-denom inputs %d\n", vecTallyRet.size());
            return vecTallyRet.size() > 0;
        }
        if(!fSkipDenominated && fAnonymizableTallyCached) {
            vecTallyRet = vecAnonymizableTallyCached;
            LogPrint(BCLog::SELECTCOINS, "SelectCoinsGroupedByAddresses - using cache for all inputs %d\n", vecTallyRet.size());
            return vecTallyRet.size() > 0;
        }
    }

    CAmount nSmallestDenom = CCoinJoin::GetSmallestDenomination();

    // Tally
    std::map<CTxDestination, CompactTallyItem> mapTally;
    std::set<uint256> setWalletTxesCounted;
    for (const auto& outpoint : setWalletUTXO) {

        if (!setWalletTxesCounted.emplace(outpoint.hash).second) continue;

        std::map<uint256, CWalletTx>::const_iterator it = mapWallet.find(outpoint.hash);
        if (it == mapWallet.end()) continue;

        const CWalletTx& wtx = (*it).second;

        if(wtx.IsCoinBase() && wtx.GetBlocksToMaturity() > 0) continue;
        if(fSkipUnconfirmed && !wtx.IsTrusted()) continue;
        if (wtx.GetDepthInMainChain() < 0) continue;

        for (unsigned int i = 0; i < wtx.tx->vout.size(); i++) {
            CTxDestination txdest;
            if (!ExtractDestination(wtx.tx->vout[i].scriptPubKey, txdest)) continue;

            isminefilter mine = ::IsMine(*this, txdest);
            if(!(mine & filter)) continue;

            auto itTallyItem = mapTally.find(txdest);
            if (nMaxOupointsPerAddress != -1 && itTallyItem != mapTally.end() && itTallyItem->second.vecInputCoins.size() >= nMaxOupointsPerAddress) continue;

            if(IsSpent(outpoint.hash, i) || IsLockedCoin(outpoint.hash, i)) continue;

            if(fSkipDenominated && CCoinJoin::IsDenominatedAmount(wtx.tx->vout[i].nValue)) continue;

            if(fAnonymizable) {
                // ignore collaterals
                if(CCoinJoin::IsCollateralAmount(wtx.tx->vout[i].nValue)) continue;
                if(fMasternodeMode && wtx.tx->vout[i].nValue == 1000*COIN) continue;
                // ignore outputs that are 10 times smaller then the smallest denomination
                // otherwise they will just lead to higher fee / lower priority
                if(wtx.tx->vout[i].nValue <= nSmallestDenom/10) continue;
                // ignore mixed
                if (IsFullyMixed(COutPoint(outpoint.hash, i))) continue;
            }

            if (itTallyItem == mapTally.end()) {
                itTallyItem = mapTally.emplace(txdest, CompactTallyItem()).first;
                itTallyItem->second.txdest = txdest;
            }
            itTallyItem->second.nAmount += wtx.tx->vout[i].nValue;
            itTallyItem->second.vecInputCoins.emplace_back(wtx.tx, i);
        }
    }

    // construct resulting vector
    // NOTE: vecTallyRet is "sorted" by txdest (i.e. address), just like mapTally
    vecTallyRet.clear();
    for (const auto& item : mapTally) {
        if(fAnonymizable && item.second.nAmount < nSmallestDenom) continue;
        vecTallyRet.push_back(item.second);
    }

    // Cache already confirmed mixable entries for later use.
    // This should only be used if nMaxOupointsPerAddress was NOT specified.
    if(nMaxOupointsPerAddress == -1 && fAnonymizable && fSkipUnconfirmed) {
        if(fSkipDenominated) {
            vecAnonymizableTallyCachedNonDenom = vecTallyRet;
            fAnonymizableTallyCachedNonDenom = true;
        } else {
            vecAnonymizableTallyCached = vecTallyRet;
            fAnonymizableTallyCached = true;
        }
    }

    // debug
    if (LogAcceptCategory(BCLog::SELECTCOINS)) {
        std::string strMessage = "SelectCoinsGroupedByAddresses - vecTallyRet:\n";
        for (const auto& item : vecTallyRet)
            strMessage += strprintf("  %s %f\n", EncodeDestination(item.txdest).c_str(), float(item.nAmount)/COIN);
        LogPrint(BCLog::SELECTCOINS, "%s", strMessage); /* Continued */
    }

    return vecTallyRet.size() > 0;
}

bool CWallet::SelectDenominatedAmounts(CAmount nValueMax, std::set<CAmount>& setAmountsRet) const
{
    LOCK2(cs_main, cs_wallet);

    CAmount nValueTotal{0};
    setAmountsRet.clear();

    std::vector<COutput> vCoins;
    CCoinControl coin_control;
    coin_control.nCoinType = CoinType::ONLY_READY_TO_MIX;
    AvailableCoins(vCoins, true, &coin_control);
    // larger denoms first
    std::sort(vCoins.rbegin(), vCoins.rend(), CompareByPriority());

    for (const auto& out : vCoins) {
        CAmount nValue = out.tx->tx->vout[out.i].nValue;
        if (nValueTotal + nValue <= nValueMax) {
            nValueTotal += nValue;
            setAmountsRet.emplace(nValue);
        }
    }

    return nValueTotal >= CCoinJoin::GetSmallestDenomination();
}

int CWallet::CountInputsWithAmount(CAmount nInputAmount) const
{
    CAmount nTotal = 0;

    LOCK2(cs_main, cs_wallet);

    for (const auto& outpoint : setWalletUTXO) {
        const auto it = mapWallet.find(outpoint.hash);
        if (it == mapWallet.end()) continue;
        if (it->second.tx->vout[outpoint.n].nValue != nInputAmount) continue;
        if (it->second.GetDepthInMainChain() < 0) continue;

        nTotal++;
    }

    return nTotal;
}

bool CWallet::HasCollateralInputs(bool fOnlyConfirmed) const
{
    LOCK2(cs_main, cs_wallet);

    std::vector<COutput> vCoins;
    CCoinControl coin_control;
    coin_control.nCoinType = CoinType::ONLY_COINJOIN_COLLATERAL;
    AvailableCoins(vCoins, fOnlyConfirmed, &coin_control);

    return !vCoins.empty();
}

bool CWallet::GetBudgetSystemCollateralTX(CTransactionRef& tx, uint256 hash, CAmount amount, const COutPoint& outpoint)
{
    // make our change address
    CReserveKey reservekey(this);

    CScript scriptChange;
    scriptChange << OP_RETURN << ToByteVector(hash);

    CAmount nFeeRet = 0;
    int nChangePosRet = -1;
    std::string strFail = "";
    std::vector< CRecipient > vecSend;
    vecSend.push_back((CRecipient){scriptChange, amount, false});

    CCoinControl coinControl;
    if (!outpoint.IsNull()) {
        coinControl.Select(outpoint);
    }
    bool success = CreateTransaction(vecSend, tx, reservekey, nFeeRet, nChangePosRet, strFail, coinControl);
    if(!success){
        LogPrintf("CWallet::GetBudgetSystemCollateralTX -- Error: %s\n", strFail);
        return false;
    }

    return true;
}

bool CWallet::CreateTransaction(const std::vector<CRecipient>& vecSend, CTransactionRef& tx, CReserveKey& reservekey, CAmount& nFeeRet,
                                int& nChangePosInOut, std::string& strFailReason, const CCoinControl& coin_control, bool sign, int nExtraPayloadSize)
{
    CAmount nValue = 0;
    int nChangePosRequest = nChangePosInOut;
    unsigned int nSubtractFeeFromAmount = 0;
    for (const auto& recipient : vecSend)
    {
        if (nValue < 0 || recipient.nAmount < 0)
        {
            strFailReason = _("Transaction amounts must not be negative");
            return false;
        }
        nValue += recipient.nAmount;

        if (recipient.fSubtractFeeFromAmount)
            nSubtractFeeFromAmount++;
    }
    if (vecSend.empty())
    {
        strFailReason = _("Transaction must have at least one recipient");
        return false;
    }

    CMutableTransaction txNew;

    // Discourage fee sniping.
    //
    // For a large miner the value of the transactions in the best block and
    // the mempool can exceed the cost of deliberately attempting to mine two
    // blocks to orphan the current best block. By setting nLockTime such that
    // only the next block can include the transaction, we discourage this
    // practice as the height restricted and limited blocksize gives miners
    // considering fee sniping fewer options for pulling off this attack.
    //
    // A simple way to think about this is from the wallet's point of view we
    // always want the blockchain to move forward. By setting nLockTime this
    // way we're basically making the statement that we only want this
    // transaction to appear in the next block; we don't want to potentially
    // encourage reorgs by allowing transactions to appear at lower heights
    // than the next block in forks of the best chain.
    //
    // Of course, the subsidy is high enough, and transaction volume low
    // enough, that fee sniping isn't a problem yet, but by implementing a fix
    // now we ensure code won't be written that makes assumptions about
    // nLockTime that preclude a fix later.

    txNew.nLockTime = chainActive.Height();

    // Secondly occasionally randomly pick a nLockTime even further back, so
    // that transactions that are delayed after signing for whatever reason,
    // e.g. high-latency mix networks and some CoinJoin implementations, have
    // better privacy.
    if (GetRandInt(10) == 0)
        txNew.nLockTime = std::max(0, (int)txNew.nLockTime - GetRandInt(100));

    assert(txNew.nLockTime <= (unsigned int)chainActive.Height());
    assert(txNew.nLockTime < LOCKTIME_THRESHOLD);
    FeeCalculation feeCalc;
    CFeeRate discard_rate = coin_control.m_discard_feerate ? *coin_control.m_discard_feerate : GetDiscardRate(::feeEstimator);
    unsigned int nBytes;
    {
        std::vector<CInputCoin> vecCoins;
        LOCK2(cs_main, mempool.cs);
        LOCK(cs_wallet);
        {
            CAmount nAmountAvailable{0};
            std::vector<COutput> vAvailableCoins;
            AvailableCoins(vAvailableCoins, true, &coin_control);
            CoinSelectionParams coin_selection_params; // Parameters for coin selection, init with dummy
            coin_selection_params.use_bnb = false; // never use BnB

            for (auto out : vAvailableCoins) {
                if (out.fSpendable) {
                    nAmountAvailable += out.tx->tx->vout[out.i].nValue;
                }
            }

            // Create change script that will be used if we need change
            // TODO: pass in scriptChange instead of reservekey so
            // change transaction isn't always pay-to-bitcoin-address
            CScript scriptChange;

            // coin control: send change to custom address
            if (!boost::get<CNoDestination>(&coin_control.destChange)) {
                scriptChange = GetScriptForDestination(coin_control.destChange);
            } else { // no coin control: send change to newly generated address
                // Note: We use a new key here to keep it from being obvious which side is the change.
                //  The drawback is that by not reusing a previous key, the change may be lost if a
                //  backup is restored, if the backup doesn't have the new private key for the change.
                //  If we reused the old key, it would be possible to add code to look for and
                //  rediscover unknown transactions that were written with keys of ours to recover
                //  post-backup change.

                // Reserve a new key pair from key pool
                CPubKey vchPubKey;
                bool ret;
                ret = reservekey.GetReservedKey(vchPubKey, true);
                if (!ret)
                {
                    strFailReason = _("Keypool ran out, please call keypoolrefill first");
                    return false;
                }

                scriptChange = GetScriptForDestination(vchPubKey.GetID());
            }

            nFeeRet = 0;
            bool pick_new_inputs = true;
            CAmount nValueIn = 0;
            CAmount nAmountToSelectAdditional{0};
            // Start with nAmountToSelectAdditional=0 and loop until there is enough to cover the request + fees, try it 500 times.
            int nMaxTries = 500;
            while (--nMaxTries > 0)
            {
                nChangePosInOut = std::numeric_limits<int>::max();
                txNew.vin.clear();
                txNew.vout.clear();
                bool fFirst = true;

                CAmount nValueToSelect = nValue;
                if (nSubtractFeeFromAmount == 0) {
                    assert(nAmountToSelectAdditional >= 0);
                    nValueToSelect += nAmountToSelectAdditional;
                }
                // vouts to the payees
                for (const auto& recipient : vecSend)
                {
                    CTxOut txout(recipient.nAmount, recipient.scriptPubKey);

                    if (recipient.fSubtractFeeFromAmount)
                    {
                        assert(nSubtractFeeFromAmount != 0);
                        txout.nValue -= nFeeRet / nSubtractFeeFromAmount; // Subtract fee equally from each selected recipient

                        if (fFirst) // first receiver pays the remainder not divisible by output count
                        {
                            fFirst = false;
                            txout.nValue -= nFeeRet % nSubtractFeeFromAmount;
                        }
                    }

                    if (IsDust(txout, ::dustRelayFee))
                    {
                        if (recipient.fSubtractFeeFromAmount && nFeeRet > 0)
                        {
                            if (txout.nValue < 0)
                                strFailReason = _("The transaction amount is too small to pay the fee");
                            else
                                strFailReason = _("The transaction amount is too small to send after the fee has been deducted");
                        }
                        else
                            strFailReason = _("Transaction amount too small");
                        return false;
                    }
                    txNew.vout.push_back(txout);
                }

                // Choose coins to use
                bool bnb_used;
                if (pick_new_inputs) {
                    nValueIn = 0;
                    std::set<CInputCoin> setCoinsTmp;
                    if (!SelectCoins(vAvailableCoins, nValueToSelect, setCoinsTmp, nValueIn, coin_control, coin_selection_params, bnb_used)) {
                        if (coin_control.nCoinType == CoinType::ONLY_NONDENOMINATED) {
                            strFailReason = _("Unable to locate enough non-denominated funds for this transaction.");
                        } else if (coin_control.nCoinType == CoinType::ONLY_FULLY_MIXED) {
                            strFailReason = _("Unable to locate enough mixed funds for this transaction.");
                            strFailReason += " " + strprintf(_("%s uses exact denominated amounts to send funds, you might simply need to mix some more coins."), "CoinJoin");
                        } else if (nValueIn < nValueToSelect) {
                            strFailReason = _("Insufficient funds.");
                        }
                        return false;
                    }
                    vecCoins.assign(setCoinsTmp.begin(), setCoinsTmp.end());
                }

                // Fill vin
                //
                // Note how the sequence number is set to max()-1 so that the
                // nLockTime set above actually works.
                txNew.vin.clear();
                for (const auto& coin : vecCoins) {
                    txNew.vin.emplace_back(coin.outpoint, CScript(), CTxIn::SEQUENCE_FINAL - 1);
                }

                auto calculateFee = [&](CAmount& nFee) -> bool {
                    // Fill in dummy signatures for fee calculation.
                    int nIn = 0;
                    for (const auto& coin : vecCoins) {
                        const CScript& scriptPubKey = coin.txout.scriptPubKey;
                        SignatureData sigdata;
                        if (!ProduceSignature(DummySignatureCreator(this), scriptPubKey, sigdata)) {
                            strFailReason = _("Signing transaction failed");
                            return false;
                        } else {
                            UpdateTransaction(txNew, nIn, sigdata);
                        }

                        nIn++;
                    }

                    nBytes = ::GetSerializeSize(txNew, SER_NETWORK, PROTOCOL_VERSION);

                    if (nExtraPayloadSize != 0) {
                        // account for extra payload in fee calculation
                        nBytes += GetSizeOfCompactSize(nExtraPayloadSize) + nExtraPayloadSize;
                    }

                    if (nBytes > MAX_STANDARD_TX_SIZE) {
                        // Do not create oversized transactions (bad-txns-oversize).
                        strFailReason = _("Transaction too large");
                        return false;
                    }

                    // Remove scriptSigs to eliminate the fee calculation dummy signatures
                    for (auto& txin : txNew.vin) {
                        txin.scriptSig = CScript();
                    }

                    nFee = GetMinimumFee(nBytes, coin_control, ::mempool, ::feeEstimator, &feeCalc);

                    // If we made it here and we aren't even able to meet the relay fee on the next pass, give up
                    // because we must be at the maximum allowed fee.
                    if (nFee < ::minRelayTxFee.GetFee(nBytes)) {
                        strFailReason = _("Transaction too large for fee policy");
                        return false;
                    }

                    return true;
                };

                if (!calculateFee(nFeeRet)) {
                    return false;
                }

                CTxOut newTxOut;
                const CAmount nAmountLeft = nValueIn - nValue;
                auto getChange = [&]() {
                    if (nSubtractFeeFromAmount > 0) {
                        return nAmountLeft;
                    } else {
                        return nAmountLeft - nFeeRet;
                    }
                };

                if (getChange() > 0)
                {
                    //over pay for denominated transactions
                    if (coin_control.nCoinType == CoinType::ONLY_FULLY_MIXED) {
                        nChangePosInOut = -1;
                        nFeeRet += getChange();
                    } else {
                        // Fill a vout to ourself with zero amount until we know the correct change
                        newTxOut = CTxOut(0, scriptChange);
                        txNew.vout.push_back(newTxOut);

                        // Calculate the fee with the change output added, store the
                        // current fee to reset it in case the remainder is dust and we
                        // don't need to fee with change output added.
                        CAmount nFeePrev = nFeeRet;
                        if (!calculateFee(nFeeRet)) {
                            return false;
                        }

                        // Remove the change output again, it will be added later again if required
                        txNew.vout.pop_back();

                        // Set the change amount properly
                        newTxOut.nValue = getChange();

                        // Never create dust outputs; if we would, just
                        // add the dust to the fee.
                        if (IsDust(newTxOut, discard_rate))
                        {
                            nFeeRet = nFeePrev;
                            nChangePosInOut = -1;
                            nFeeRet += getChange();
                        }
                        else
                        {
                            if (nChangePosRequest == -1)
                            {
                                // Insert change txn at random position:
                                nChangePosInOut = GetRandInt(txNew.vout.size()+1);
                            }
                            else if ((unsigned int)nChangePosRequest > txNew.vout.size())
                            {
                                strFailReason = _("Change index out of range");
                                return false;
                            } else {
                                nChangePosInOut = nChangePosRequest;
                            }

                            std::vector<CTxOut>::iterator position = txNew.vout.begin()+nChangePosInOut;
                            txNew.vout.insert(position, newTxOut);
                        }
                    }
                } else {
                    nChangePosInOut = -1;
                }

                if (getChange() < 0) {
                    if (nSubtractFeeFromAmount == 0) {
                        // nValueIn is not enough to cover nValue + nFeeRet. Add the missing amount abs(nChange) to the fee
                        // and try to select other inputs in the next loop step to cover the full required amount.
                        nAmountToSelectAdditional += abs(getChange());
                    } else if (nAmountToSelectAdditional > 0 && nValueToSelect == nAmountAvailable) {
                        // We tried selecting more and failed. We have no extra funds left,
                        // so just add 1 duff to fail in the next loop step with a correct reason
                        nAmountToSelectAdditional += 1;
                    }
                    continue;
                }

                // If no specific change position was requested, apply BIP69
                if (nChangePosRequest == -1) {
                    std::sort(vecCoins.begin(), vecCoins.end(), CompareInputCoinBIP69());
                    std::sort(txNew.vin.begin(), txNew.vin.end(), CompareInputBIP69());
                    std::sort(txNew.vout.begin(), txNew.vout.end(), CompareOutputBIP69());

                    // If there was a change output added before, we must update its position now
                    if (nChangePosInOut != -1) {
                        int i = 0;
                        for (const CTxOut& txOut : txNew.vout)
                        {
                            if (txOut == newTxOut)
                            {
                                nChangePosInOut = i;
                                break;
                            }
                            i++;
                        }
                    }
                }

                if (nAmountLeft == nFeeRet) {
                    // We either added the change amount to nFeeRet because the change amount was considered
                    // to be dust or the input exactly matches output + fee.
                    // Either way, we used the total amount of the inputs we picked and the transaction is ready.
                    break;
                }

                // We have a change output and we don't need to subtruct fees, which means the transaction is ready.
                if (nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
                    break;
                }

                // If subtracting fee from recipients, we now know what fee we
                // need to subtract, we have no reason to reselect inputs
                if (nSubtractFeeFromAmount > 0) {
                    // If we are in here the second time it means we already subtracted the fee from the
                    // output(s) and there weren't any issues while doing that. So the transaction is ready now
                    // and we can break.
                    if (!pick_new_inputs) {
                        break;
                    }
                    pick_new_inputs = false;
                }
            }

            if (nMaxTries == 0) {
                strFailReason = _("Exceeded max tries.");
                return false;
            }
        }

        // Make sure change position was updated one way or another
        assert(nChangePosInOut != std::numeric_limits<int>::max());

        if (nChangePosInOut == -1) reservekey.ReturnKey(); // Return any reserved key if we don't have change

        if (sign)
        {
            CTransaction txNewConst(txNew);
            int nIn = 0;
            for(const auto& coin : vecCoins)
            {
                const CScript& scriptPubKey = coin.txout.scriptPubKey;
                SignatureData sigdata;

                if (!ProduceSignature(TransactionSignatureCreator(this, &txNewConst, nIn, SIGHASH_ALL), scriptPubKey, sigdata))
                {
                    strFailReason = _("Signing transaction failed");
                    return false;
                } else {
                    UpdateTransaction(txNew, nIn, sigdata);
                }

                nIn++;
            }
        }

        // Return the constructed transaction data.
        tx = MakeTransactionRef(std::move(txNew));
    }

    if (gArgs.GetBoolArg("-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS)) {
        // Lastly, ensure this tx will pass the mempool's chain limits
        LockPoints lp;
        CTxMemPoolEntry entry(tx, 0, 0, 0, false, 0, lp);
        CTxMemPool::setEntries setAncestors;
        size_t nLimitAncestors = gArgs.GetArg("-limitancestorcount", DEFAULT_ANCESTOR_LIMIT);
        size_t nLimitAncestorSize = gArgs.GetArg("-limitancestorsize", DEFAULT_ANCESTOR_SIZE_LIMIT)*1000;
        size_t nLimitDescendants = gArgs.GetArg("-limitdescendantcount", DEFAULT_DESCENDANT_LIMIT);
        size_t nLimitDescendantSize = gArgs.GetArg("-limitdescendantsize", DEFAULT_DESCENDANT_SIZE_LIMIT)*1000;
        std::string errString;
        LOCK(::mempool.cs);
        if (!::mempool.CalculateMemPoolAncestors(entry, setAncestors, nLimitAncestors, nLimitAncestorSize, nLimitDescendants, nLimitDescendantSize, errString)) {
            strFailReason = _("Transaction has too long of a mempool chain");
            return false;
        }
    }

    LogPrintf("Fee Calculation: Fee:%d Bytes:%u Tgt:%d (requested %d) Reason:\"%s\" Decay %.5f: Estimation: (%g - %g) %.2f%% %.1f/(%.1f %d mem %.1f out) Fail: (%g - %g) %.2f%% %.1f/(%.1f %d mem %.1f out)\n",
              nFeeRet, nBytes, feeCalc.returnedTarget, feeCalc.desiredTarget, StringForFeeReason(feeCalc.reason), feeCalc.est.decay,
              feeCalc.est.pass.start, feeCalc.est.pass.end,
              100 * feeCalc.est.pass.withinTarget / (feeCalc.est.pass.totalConfirmed + feeCalc.est.pass.inMempool + feeCalc.est.pass.leftMempool),
              feeCalc.est.pass.withinTarget, feeCalc.est.pass.totalConfirmed, feeCalc.est.pass.inMempool, feeCalc.est.pass.leftMempool,
              feeCalc.est.fail.start, feeCalc.est.fail.end,
              100 * feeCalc.est.fail.withinTarget / (feeCalc.est.fail.totalConfirmed + feeCalc.est.fail.inMempool + feeCalc.est.fail.leftMempool),
              feeCalc.est.fail.withinTarget, feeCalc.est.fail.totalConfirmed, feeCalc.est.fail.inMempool, feeCalc.est.fail.leftMempool);
    return true;
}

/**
 * Call after CreateTransaction unless you want to abort
 */
bool CWallet::CommitTransaction(CTransactionRef tx, mapValue_t mapValue, std::vector<std::pair<std::string, std::string>> orderForm, std::string fromAccount, CReserveKey& reservekey, CConnman* connman, CValidationState& state)
{
    {
        LOCK2(cs_main, mempool.cs);
        LOCK(cs_wallet);

        CWalletTx wtxNew(this, std::move(tx));
        wtxNew.mapValue = std::move(mapValue);
        wtxNew.vOrderForm = std::move(orderForm);
        wtxNew.strFromAccount = std::move(fromAccount);
        wtxNew.fTimeReceivedIsTxTime = true;
        wtxNew.fFromMe = true;

        LogPrintf("CommitTransaction:\n%s", wtxNew.tx->ToString()); /* Continued */
        {
            // Take key pair from key pool so it won't be used again
            reservekey.KeepKey();

            // Add tx to wallet, because if it has change it's also ours,
            // otherwise just for transaction history.
            AddToWallet(wtxNew);

            // Notify that old coins are spent
            std::set<uint256> updated_hahes;
            for (const CTxIn& txin : wtxNew.tx->vin)
            {
                // notify only once
                if(updated_hahes.find(txin.prevout.hash) != updated_hahes.end()) continue;

                CWalletTx &coin = mapWallet.at(txin.prevout.hash);
                coin.BindWallet(this);
                NotifyTransactionChanged(this, txin.prevout.hash, CT_UPDATED);
                updated_hahes.insert(txin.prevout.hash);
            }
        }

        // Get the inserted-CWalletTx from mapWallet so that the
        // fInMempool flag is cached properly
        CWalletTx& wtx = mapWallet.at(wtxNew.GetHash());

        if (fBroadcastTransactions)
        {
            // Broadcast
            if (!wtx.AcceptToMemoryPool(maxTxFee, state)) {
                LogPrintf("CommitTransaction(): Transaction cannot be broadcast immediately, %s\n", FormatStateMessage(state));
                // TODO: if we expect the failure to be long term or permanent, instead delete wtx from the wallet and return failure.
            } else {
                wtx.RelayWalletTransaction(connman);
            }
        }
    }
    return true;
}

void CWallet::ListAccountCreditDebit(const std::string& strAccount, std::list<CAccountingEntry>& entries) {
    WalletBatch batch(*database);
    return batch.ListAccountCreditDebit(strAccount, entries);
}

bool CWallet::AddAccountingEntry(const CAccountingEntry& acentry)
{
    WalletBatch batch(*database);

    return AddAccountingEntry(acentry, &batch);
}

bool CWallet::AddAccountingEntry(const CAccountingEntry& acentry, WalletBatch *batch)
{
    if (!batch->WriteAccountingEntry(++nAccountingEntryNumber, acentry)) {
        return false;
    }

    laccentries.push_back(acentry);
    CAccountingEntry & entry = laccentries.back();
    wtxOrdered.insert(std::make_pair(entry.nOrderPos, TxPair(nullptr, &entry)));

    return true;
}

DBErrors CWallet::LoadWallet(bool& fFirstRunRet)
{
    LOCK2(cs_main, cs_wallet);

    fFirstRunRet = false;
    DBErrors nLoadWalletRet = WalletBatch(*database,"cr+").LoadWallet(this);
    if (nLoadWalletRet == DBErrors::NEED_REWRITE)
    {
        if (database->Rewrite("\x04pool"))
        {
            setInternalKeyPool.clear();
            setExternalKeyPool.clear();
            nKeysLeftSinceAutoBackup = 0;
            m_pool_key_to_index.clear();
            // Note: can't top-up keypool here, because wallet is locked.
            // User will be prompted to unlock wallet the next operation
            // that requires a new key.
        }
    }

    {
        LOCK(cs_KeyStore);
        // This wallet is in its first run if all of these are empty
        fFirstRunRet = mapKeys.empty() && mapHdPubKeys.empty() && mapCryptedKeys.empty() && mapWatchKeys.empty() && setWatchOnly.empty() && mapScripts.empty();
    }

    {
        LOCK2(cs_main, cs_wallet);
        for (auto& pair : mapWallet) {
            for(unsigned int i = 0; i < pair.second.tx->vout.size(); ++i) {
                if (IsMine(pair.second.tx->vout[i]) && !IsSpent(pair.first, i)) {
                    setWalletUTXO.insert(COutPoint(pair.first, i));
                }
            }
        }
    }

    InitCoinJoinSalt();

    if (nLoadWalletRet != DBErrors::LOAD_OK)
        return nLoadWalletRet;

    return DBErrors::LOAD_OK;
}

// Goes through all wallet transactions and checks if they are masternode collaterals, in which case these are locked
// This avoids accidential spending of collaterals. They can still be unlocked manually if a spend is really intended.
void CWallet::AutoLockMasternodeCollaterals()
{
    auto mnList = deterministicMNManager->GetListAtChainTip();

    LOCK2(cs_main, cs_wallet);
    for (const auto& pair : mapWallet) {
        for (unsigned int i = 0; i < pair.second.tx->vout.size(); ++i) {
            if (IsMine(pair.second.tx->vout[i]) && !IsSpent(pair.first, i)) {
                if (deterministicMNManager->IsProTxWithCollateral(pair.second.tx, i) || mnList.HasMNByCollateral(COutPoint(pair.first, i))) {
                    LockCoin(COutPoint(pair.first, i));
                }
            }
        }
    }
}

DBErrors CWallet::ZapSelectTx(std::vector<uint256>& vHashIn, std::vector<uint256>& vHashOut)
{
    AssertLockHeld(cs_wallet); // mapWallet
    DBErrors nZapSelectTxRet = WalletBatch(*database,"cr+").ZapSelectTx(vHashIn, vHashOut);
    for (uint256 hash : vHashOut) {
        const auto& it = mapWallet.find(hash);
        wtxOrdered.erase(it->second.m_it_wtxOrdered);
        mapWallet.erase(it);
    }

    if (nZapSelectTxRet == DBErrors::NEED_REWRITE)
    {
        if (database->Rewrite("\x04pool"))
        {
            setInternalKeyPool.clear();
            setExternalKeyPool.clear();
            m_pool_key_to_index.clear();
            // Note: can't top-up keypool here, because wallet is locked.
            // User will be prompted to unlock wallet the next operation
            // that requires a new key.
        }
    }

    if (nZapSelectTxRet != DBErrors::LOAD_OK)
        return nZapSelectTxRet;

    MarkDirty();

    return DBErrors::LOAD_OK;

}

DBErrors CWallet::ZapWalletTx(std::vector<CWalletTx>& vWtx)
{
    DBErrors nZapWalletTxRet = WalletBatch(*database,"cr+").ZapWalletTx(vWtx);
    if (nZapWalletTxRet == DBErrors::NEED_REWRITE)
    {
        if (database->Rewrite("\x04pool"))
        {
            LOCK(cs_wallet);
            setInternalKeyPool.clear();
            setExternalKeyPool.clear();
            nKeysLeftSinceAutoBackup = 0;
            m_pool_key_to_index.clear();
            // Note: can't top-up keypool here, because wallet is locked.
            // User will be prompted to unlock wallet the next operation
            // that requires a new key.
        }
    }

    if (nZapWalletTxRet != DBErrors::LOAD_OK)
        return nZapWalletTxRet;

    return DBErrors::LOAD_OK;
}


bool CWallet::SetAddressBook(const CTxDestination& address, const std::string& strName, const std::string& strPurpose)
{
    bool fUpdated = false;
    {
        LOCK(cs_wallet); // mapAddressBook
        std::map<CTxDestination, CAddressBookData>::iterator mi = mapAddressBook.find(address);
        fUpdated = mi != mapAddressBook.end();
        mapAddressBook[address].name = strName;
        if (!strPurpose.empty()) /* update purpose only if requested */
            mapAddressBook[address].purpose = strPurpose;
    }
    NotifyAddressBookChanged(this, address, strName, ::IsMine(*this, address) != ISMINE_NO,
                             strPurpose, (fUpdated ? CT_UPDATED : CT_NEW) );
    if (!strPurpose.empty() && !WalletBatch(*database).WritePurpose(EncodeDestination(address), strPurpose))
        return false;
    return WalletBatch(*database).WriteName(EncodeDestination(address), strName);
}

bool CWallet::DelAddressBook(const CTxDestination& address)
{
    {
        LOCK(cs_wallet); // mapAddressBook

        // Delete destdata tuples associated with address
        std::string strAddress = EncodeDestination(address);
        for (const std::pair<const std::string, std::string> &item : mapAddressBook[address].destdata)
        {
            WalletBatch(*database).EraseDestData(strAddress, item.first);
        }
        mapAddressBook.erase(address);
    }

    NotifyAddressBookChanged(this, address, "", ::IsMine(*this, address) != ISMINE_NO, "", CT_DELETED);

    WalletBatch(*database).ErasePurpose(EncodeDestination(address));
    return WalletBatch(*database).EraseName(EncodeDestination(address));
}

const std::string& CWallet::GetLabelName(const CScript& scriptPubKey) const
{
    CTxDestination address;
    if (ExtractDestination(scriptPubKey, address) && !scriptPubKey.IsUnspendable()) {
        auto mi = mapAddressBook.find(address);
        if (mi != mapAddressBook.end()) {
            return mi->second.name;
        }
    }
    // A scriptPubKey that doesn't have an entry in the address book is
    // associated with the default label ("").
    const static std::string DEFAULT_LABEL_NAME;
    return DEFAULT_LABEL_NAME;
}

/**
 * Mark old keypool keys as used,
 * and generate all new keys
 */
bool CWallet::NewKeyPool()
{
    {
        LOCK(cs_wallet);
        WalletBatch batch(*database);
        for (int64_t nIndex : setInternalKeyPool) {
            batch.ErasePool(nIndex);
        }
        setInternalKeyPool.clear();
        for (int64_t nIndex : setExternalKeyPool) {
            batch.ErasePool(nIndex);
        }
        setExternalKeyPool.clear();
        coinJoinClientManagers.at(GetName())->StopMixing();
        nKeysLeftSinceAutoBackup = 0;

        m_pool_key_to_index.clear();

        if (!TopUpKeyPool())
            return false;

        LogPrintf("CWallet::NewKeyPool rewrote keypool\n");
    }
    return true;
}

size_t CWallet::KeypoolCountExternalKeys()
{
    AssertLockHeld(cs_wallet); // setExternalKeyPool
    return setExternalKeyPool.size();
}

void CWallet::LoadKeyPool(int64_t nIndex, const CKeyPool &keypool)
{
    AssertLockHeld(cs_wallet);
    if (keypool.fInternal) {
        setInternalKeyPool.insert(nIndex);
    } else {
        setExternalKeyPool.insert(nIndex);
    }
    m_max_keypool_index = std::max(m_max_keypool_index, nIndex);
    m_pool_key_to_index[keypool.vchPubKey.GetID()] = nIndex;

    // If no metadata exists yet, create a default with the pool key's
    // creation time. Note that this may be overwritten by actually
    // stored metadata for that key later, which is fine.
    CKeyID keyid = keypool.vchPubKey.GetID();
    if (mapKeyMetadata.count(keyid) == 0)
        mapKeyMetadata[keyid] = CKeyMetadata(keypool.nTime);
}

size_t CWallet::KeypoolCountInternalKeys()
{
    AssertLockHeld(cs_wallet); // setInternalKeyPool
    return setInternalKeyPool.size();
}

bool CWallet::TopUpKeyPool(unsigned int kpSize)
{
    {
        LOCK(cs_wallet);

        if (IsLocked(true))
            return false;

        // Top up key pool
        unsigned int nTargetSize;
        if (kpSize > 0)
            nTargetSize = kpSize;
        else
            nTargetSize = std::max(gArgs.GetArg("-keypool", DEFAULT_KEYPOOL_SIZE), (int64_t) 0);

        // count amount of available keys (internal, external)
        // make sure the keypool of external and internal keys fits the user selected target (-keypool)
        int64_t amountExternal = setExternalKeyPool.size();
        int64_t amountInternal = setInternalKeyPool.size();
        int64_t missingExternal = std::max(std::max((int64_t) nTargetSize, (int64_t) 1) - amountExternal, (int64_t) 0);
        int64_t missingInternal = std::max(std::max((int64_t) nTargetSize, (int64_t) 1) - amountInternal, (int64_t) 0);

        if (!IsHDEnabled())
        {
            // don't create extra internal keys
            missingInternal = 0;
        } else {
            nTargetSize *= 2;
        }
        bool fInternal = false;
        WalletBatch batch(*database);
        for (int64_t i = missingInternal + missingExternal; i--;)
        {
            if (i < missingInternal) {
                fInternal = true;
            }

            assert(m_max_keypool_index < std::numeric_limits<int64_t>::max()); // How in the hell did you use so many keys?
            int64_t index = ++m_max_keypool_index;

            // TODO: implement keypools for all accounts?
            CPubKey pubkey(GenerateNewKey(batch, 0, fInternal));
            if (!batch.WritePool(index, CKeyPool(pubkey, fInternal))) {
                throw std::runtime_error(std::string(__func__) + ": writing generated key failed");
            }

            if (fInternal) {
                setInternalKeyPool.insert(index);
            } else {
                setExternalKeyPool.insert(index);
            }

            m_pool_key_to_index[pubkey.GetID()] = index;
            if (missingInternal + missingExternal > 0) {
                LogPrintf("keypool added %d keys (%d internal), size=%u (%u internal)\n",
                          missingInternal + missingExternal, missingInternal,
                          setInternalKeyPool.size() + setExternalKeyPool.size(), setInternalKeyPool.size());
            }

            double dProgress = 100.f * index / (nTargetSize + 1);
            std::string strMsg = strprintf(_("Loading wallet... (%3.2f %%)"), dProgress);
            uiInterface.InitMessage(strMsg);
        }
    }
    return true;
}

void CWallet::ReserveKeyFromKeyPool(int64_t& nIndex, CKeyPool& keypool, bool fInternal)
{
    nIndex = -1;
    keypool.vchPubKey = CPubKey();
    {
        LOCK(cs_wallet);

        if (!IsLocked(true))
            TopUpKeyPool();

        fInternal = fInternal && IsHDEnabled();
        std::set<int64_t>& setKeyPool = fInternal ? setInternalKeyPool : setExternalKeyPool;

        // Get the oldest key
        if(setKeyPool.empty())
            return;

        WalletBatch batch(*database);

        nIndex = *setKeyPool.begin();
        setKeyPool.erase(nIndex);
        if (!batch.ReadPool(nIndex, keypool)) {
            throw std::runtime_error(std::string(__func__) + ": read failed");
        }
        if (!HaveKey(keypool.vchPubKey.GetID())) {
            throw std::runtime_error(std::string(__func__) + ": unknown key in key pool");
        }
        if (keypool.fInternal != fInternal) {
            throw std::runtime_error(std::string(__func__) + ": keypool entry misclassified");
        }

        assert(keypool.vchPubKey.IsValid());
        m_pool_key_to_index.erase(keypool.vchPubKey.GetID());
        LogPrintf("keypool reserve %d\n", nIndex);
    }
}

void CWallet::KeepKey(int64_t nIndex)
{
    // Remove from key pool
    WalletBatch batch(*database);
    if (batch.ErasePool(nIndex))
        --nKeysLeftSinceAutoBackup;
    if (!nWalletBackups)
        nKeysLeftSinceAutoBackup = 0;
    LogPrintf("keypool keep %d\n", nIndex);
}

void CWallet::ReturnKey(int64_t nIndex, bool fInternal, const CPubKey& pubkey)
{
    // Return to key pool
    {
        LOCK(cs_wallet);
        if (fInternal) {
            setInternalKeyPool.insert(nIndex);
        } else {
            setExternalKeyPool.insert(nIndex);
        }
        m_pool_key_to_index[pubkey.GetID()] = nIndex;
    }
    LogPrintf("keypool return %d\n", nIndex);
}

bool CWallet::GetKeyFromPool(CPubKey& result, bool internal)
{
    CKeyPool keypool;
    {
        LOCK(cs_wallet);
        int64_t nIndex = 0;
        ReserveKeyFromKeyPool(nIndex, keypool, internal);
        if (nIndex == -1)
        {
            if (IsLocked(true)) return false;
            // TODO: implement keypool for all accouts?

            WalletBatch batch(*database);
            result = GenerateNewKey(batch, 0, internal);
            return true;
        }
        KeepKey(nIndex);
        result = keypool.vchPubKey;
    }
    return true;
}

static int64_t GetOldestKeyInPool(const std::set<int64_t>& setKeyPool, WalletBatch& batch) {
    CKeyPool keypool;
    int64_t nIndex = *(setKeyPool.begin());
    if (!batch.ReadPool(nIndex, keypool)) {
        throw std::runtime_error(std::string(__func__) + ": read oldest key in keypool failed");
    }
    assert(keypool.vchPubKey.IsValid());
    return keypool.nTime;
}

int64_t CWallet::GetOldestKeyPoolTime()
{
    LOCK(cs_wallet);

    // if the keypool is empty, return <NOW>
    if (setExternalKeyPool.empty() && setInternalKeyPool.empty())
        return GetTime();

    WalletBatch batch(*database);
    int64_t oldestKey = -1;

    // load oldest key from keypool, get time and return
    if (!setInternalKeyPool.empty()) {
        oldestKey = std::max(GetOldestKeyInPool(setInternalKeyPool, batch), oldestKey);
    }
    if (!setExternalKeyPool.empty()) {
        oldestKey = std::max(GetOldestKeyInPool(setExternalKeyPool, batch), oldestKey);
    }
    return oldestKey;
}

std::map<CTxDestination, CAmount> CWallet::GetAddressBalances()
{
    std::map<CTxDestination, CAmount> balances;

    {
        LOCK(cs_wallet);
        for (const auto& walletEntry : mapWallet)
        {
            const CWalletTx *pcoin = &walletEntry.second;

            if (!pcoin->IsTrusted())
                continue;

            if (pcoin->IsCoinBase() && pcoin->GetBlocksToMaturity() > 0)
                continue;

            int nDepth = pcoin->GetDepthInMainChain();
            if ((nDepth < (pcoin->IsFromMe(ISMINE_ALL) ? 0 : 1)) && !pcoin->IsLockedByInstantSend())
                continue;

            for (unsigned int i = 0; i < pcoin->tx->vout.size(); i++)
            {
                CTxDestination addr;
                if (!IsMine(pcoin->tx->vout[i]))
                    continue;
                if(!ExtractDestination(pcoin->tx->vout[i].scriptPubKey, addr))
                    continue;

                CAmount n = IsSpent(walletEntry.first, i) ? 0 : pcoin->tx->vout[i].nValue;

                if (!balances.count(addr))
                    balances[addr] = 0;
                balances[addr] += n;
            }
        }
    }

    return balances;
}

std::set< std::set<CTxDestination> > CWallet::GetAddressGroupings()
{
    AssertLockHeld(cs_wallet); // mapWallet
    std::set< std::set<CTxDestination> > groupings;
    std::set<CTxDestination> grouping;

    for (const auto& walletEntry : mapWallet)
    {
        const CWalletTx *pcoin = &walletEntry.second;

        if (pcoin->tx->vin.size() > 0)
        {
            bool any_mine = false;
            // group all input addresses with each other
            for (CTxIn txin : pcoin->tx->vin)
            {
                CTxDestination address;
                if(!IsMine(txin)) /* If this input isn't mine, ignore it */
                    continue;
                if(!ExtractDestination(mapWallet.at(txin.prevout.hash).tx->vout[txin.prevout.n].scriptPubKey, address))
                    continue;
                grouping.insert(address);
                any_mine = true;
            }

            // group change with input addresses
            if (any_mine)
            {
               for (CTxOut txout : pcoin->tx->vout)
                   if (IsChange(txout))
                   {
                       CTxDestination txoutAddr;
                       if(!ExtractDestination(txout.scriptPubKey, txoutAddr))
                           continue;
                       grouping.insert(txoutAddr);
                   }
            }
            if (grouping.size() > 0)
            {
                groupings.insert(grouping);
                grouping.clear();
            }
        }

        // group lone addrs by themselves
        for (const auto& txout : pcoin->tx->vout)
            if (IsMine(txout))
            {
                CTxDestination address;
                if(!ExtractDestination(txout.scriptPubKey, address))
                    continue;
                grouping.insert(address);
                groupings.insert(grouping);
                grouping.clear();
            }
    }

    std::set< std::set<CTxDestination>* > uniqueGroupings; // a set of pointers to groups of addresses
    std::map< CTxDestination, std::set<CTxDestination>* > setmap;  // map addresses to the unique group containing it
    for (std::set<CTxDestination> _grouping : groupings)
    {
        // make a set of all the groups hit by this new group
        std::set< std::set<CTxDestination>* > hits;
        std::map< CTxDestination, std::set<CTxDestination>* >::iterator it;
        for (CTxDestination address : _grouping)
            if ((it = setmap.find(address)) != setmap.end())
                hits.insert((*it).second);

        // merge all hit groups into a new single group and delete old groups
        std::set<CTxDestination>* merged = new std::set<CTxDestination>(_grouping);
        for (std::set<CTxDestination>* hit : hits)
        {
            merged->insert(hit->begin(), hit->end());
            uniqueGroupings.erase(hit);
            delete hit;
        }
        uniqueGroupings.insert(merged);

        // update setmap
        for (CTxDestination element : *merged)
            setmap[element] = merged;
    }

    std::set< std::set<CTxDestination> > ret;
    for (std::set<CTxDestination>* uniqueGrouping : uniqueGroupings)
    {
        ret.insert(*uniqueGrouping);
        delete uniqueGrouping;
    }

    return ret;
}

std::set<CTxDestination> CWallet::GetLabelAddresses(const std::string& label) const
{
    LOCK(cs_wallet);
    std::set<CTxDestination> result;
    for (const std::pair<const CTxDestination, CAddressBookData>& item : mapAddressBook)
    {
        const CTxDestination& address = item.first;
        const std::string& strName = item.second.name;
        if (strName == label)
            result.insert(address);
    }
    return result;
}

void CWallet::DeleteLabel(const std::string& label)
{
    WalletBatch batch(*database);
    batch.EraseAccount(label);
}

bool CReserveKey::GetReservedKey(CPubKey& pubkey, bool fInternalIn)
{
    if (nIndex == -1)
    {
        CKeyPool keypool;
        pwallet->ReserveKeyFromKeyPool(nIndex, keypool, fInternalIn);
        if (nIndex != -1) {
            vchPubKey = keypool.vchPubKey;
        }
        else {
            return false;
        }
        fInternal = keypool.fInternal;
    }
    assert(vchPubKey.IsValid());
    pubkey = vchPubKey;
    return true;
}

void CReserveKey::KeepKey()
{
    if (nIndex != -1) {
        pwallet->KeepKey(nIndex);
    }
    nIndex = -1;
    vchPubKey = CPubKey();
}

void CReserveKey::ReturnKey()
{
    if (nIndex != -1) {
        pwallet->ReturnKey(nIndex, fInternal, vchPubKey);
    }
    nIndex = -1;
    vchPubKey = CPubKey();
}

void CWallet::MarkReserveKeysAsUsed(int64_t keypool_id)
{
    AssertLockHeld(cs_wallet);
    bool internal = setInternalKeyPool.count(keypool_id);
    if (!internal) assert(setExternalKeyPool.count(keypool_id));
    std::set<int64_t> *setKeyPool = internal ? &setInternalKeyPool : &setExternalKeyPool;
    auto it = setKeyPool->begin();

    WalletBatch batch(*database);
    while (it != std::end(*setKeyPool)) {
        const int64_t& index = *(it);
        if (index > keypool_id) break; // set*KeyPool is ordered

        CKeyPool keypool;
        if (batch.ReadPool(index, keypool)) { //TODO: This should be unnecessary
            m_pool_key_to_index.erase(keypool.vchPubKey.GetID());
        }
        batch.ErasePool(index);
        LogPrintf("keypool index %d removed\n", index);
        it = setKeyPool->erase(it);
    }
}

void CWallet::GetScriptForMining(std::shared_ptr<CReserveScript> &script)
{
    std::shared_ptr<CReserveKey> rKey = std::make_shared<CReserveKey>(this);
    CPubKey pubkey;
    if (!rKey->GetReservedKey(pubkey, false))
        return;

    script = rKey;
    script->reserveScript = CScript() << ToByteVector(pubkey) << OP_CHECKSIG;
}

void CWallet::LockCoin(const COutPoint& output)
{
    AssertLockHeld(cs_wallet); // setLockedCoins
    setLockedCoins.insert(output);
    std::map<uint256, CWalletTx>::iterator it = mapWallet.find(output.hash);
    if (it != mapWallet.end()) it->second.MarkDirty(); // recalculate all credits for this tx

    fAnonymizableTallyCached = false;
    fAnonymizableTallyCachedNonDenom = false;
}

void CWallet::UnlockCoin(const COutPoint& output)
{
    AssertLockHeld(cs_wallet); // setLockedCoins
    setLockedCoins.erase(output);
    std::map<uint256, CWalletTx>::iterator it = mapWallet.find(output.hash);
    if (it != mapWallet.end()) it->second.MarkDirty(); // recalculate all credits for this tx

    fAnonymizableTallyCached = false;
    fAnonymizableTallyCachedNonDenom = false;
}

void CWallet::UnlockAllCoins()
{
    AssertLockHeld(cs_wallet); // setLockedCoins
    setLockedCoins.clear();
}

bool CWallet::IsLockedCoin(uint256 hash, unsigned int n) const
{
    AssertLockHeld(cs_wallet); // setLockedCoins
    COutPoint outpt(hash, n);

    return (setLockedCoins.count(outpt) > 0);
}

void CWallet::ListLockedCoins(std::vector<COutPoint>& vOutpts) const
{
    AssertLockHeld(cs_wallet); // setLockedCoins
    for (std::set<COutPoint>::iterator it = setLockedCoins.begin();
         it != setLockedCoins.end(); it++) {
        COutPoint outpt = (*it);
        vOutpts.push_back(outpt);
    }
}

void CWallet::ListProTxCoins(std::vector<COutPoint>& vOutpts) const
{
    auto mnList = deterministicMNManager->GetListAtChainTip();

    AssertLockHeld(cs_wallet);
    for (const auto &o : setWalletUTXO) {
        auto it = mapWallet.find(o.hash);
        if (it != mapWallet.end()) {
            const auto &p = it->second;
            if (deterministicMNManager->IsProTxWithCollateral(p.tx, o.n) || mnList.HasMNByCollateral(o)) {
                vOutpts.emplace_back(o);
            }
        }
    }
}

/** @} */ // end of Actions

void CWallet::GetKeyBirthTimes(std::map<CTxDestination, int64_t> &mapKeyBirth) const {
    AssertLockHeld(cs_wallet); // mapKeyMetadata
    mapKeyBirth.clear();

    // get birth times for keys with metadata
    for (const auto& entry : mapKeyMetadata) {
        if (entry.second.nCreateTime) {
            mapKeyBirth[entry.first] = entry.second.nCreateTime;
        }
    }

    // map in which we'll infer heights of other keys
    CBlockIndex *pindexMax = chainActive[std::max(0, chainActive.Height() - 144)]; // the tip can be reorganized; use a 144-block safety margin
    std::map<CKeyID, CBlockIndex*> mapKeyFirstBlock;
    for (const CKeyID &keyid : GetKeys()) {
        if (mapKeyBirth.count(keyid) == 0)
            mapKeyFirstBlock[keyid] = pindexMax;
    }

    // if there are no such keys, we're done
    if (mapKeyFirstBlock.empty())
        return;

    // find first block that affects those keys, if there are any left
    std::vector<CKeyID> vAffected;
    for (const auto& entry : mapWallet) {
        // iterate over all wallet transactions...
        const CWalletTx &wtx = entry.second;
        CBlockIndex* pindex = LookupBlockIndex(wtx.hashBlock);
        if (pindex && chainActive.Contains(pindex)) {
            // ... which are already in a block
            int nHeight = pindex->nHeight;
            for (const CTxOut &txout : wtx.tx->vout) {
                // iterate over all their outputs
                CAffectedKeysVisitor(*this, vAffected).Process(txout.scriptPubKey);
                for (const CKeyID &keyid : vAffected) {
                    // ... and all their affected keys
                    std::map<CKeyID, CBlockIndex*>::iterator rit = mapKeyFirstBlock.find(keyid);
                    if (rit != mapKeyFirstBlock.end() && nHeight < rit->second->nHeight)
                        rit->second = pindex;
                }
                vAffected.clear();
            }
        }
    }

    // Extract block timestamps for those keys
    for (const auto& entry : mapKeyFirstBlock)
        mapKeyBirth[entry.first] = entry.second->GetBlockTime() - TIMESTAMP_WINDOW; // block times can be 2h off
}

/**
 * Compute smart timestamp for a transaction being added to the wallet.
 *
 * Logic:
 * - If sending a transaction, assign its timestamp to the current time.
 * - If receiving a transaction outside a block, assign its timestamp to the
 *   current time.
 * - If receiving a block with a future timestamp, assign all its (not already
 *   known) transactions' timestamps to the current time.
 * - If receiving a block with a past timestamp, before the most recent known
 *   transaction (that we care about), assign all its (not already known)
 *   transactions' timestamps to the same timestamp as that most-recent-known
 *   transaction.
 * - If receiving a block with a past timestamp, but after the most recent known
 *   transaction, assign all its (not already known) transactions' timestamps to
 *   the block time.
 *
 * For more information see CWalletTx::nTimeSmart,
 * https://bitcointalk.org/?topic=54527, or
 * https://github.com/bitcoin/bitcoin/pull/1393.
 */
unsigned int CWallet::ComputeTimeSmart(const CWalletTx& wtx) const
{
    unsigned int nTimeSmart = wtx.nTimeReceived;
    if (!wtx.hashUnset()) {
        if (const CBlockIndex* pindex = LookupBlockIndex(wtx.hashBlock)) {
            int64_t latestNow = wtx.nTimeReceived;
            int64_t latestEntry = 0;

            // Tolerate times up to the last timestamp in the wallet not more than 5 minutes into the future
            int64_t latestTolerated = latestNow + 300;
            const TxItems& txOrdered = wtxOrdered;
            for (auto it = txOrdered.rbegin(); it != txOrdered.rend(); ++it) {
                CWalletTx* const pwtx = it->second.first;
                if (pwtx == &wtx) {
                    continue;
                }
                CAccountingEntry* const pacentry = it->second.second;
                int64_t nSmartTime;
                if (pwtx) {
                    nSmartTime = pwtx->nTimeSmart;
                    if (!nSmartTime) {
                        nSmartTime = pwtx->nTimeReceived;
                    }
                } else {
                    nSmartTime = pacentry->nTime;
                }
                if (nSmartTime <= latestTolerated) {
                    latestEntry = nSmartTime;
                    if (nSmartTime > latestNow) {
                        latestNow = nSmartTime;
                    }
                    break;
                }
            }

            int64_t blocktime = pindex->GetBlockTime();
            nTimeSmart = std::max(latestEntry, std::min(blocktime, latestNow));
        } else {
            LogPrintf("%s: found %s in block %s not in index\n", __func__, wtx.GetHash().ToString(), wtx.hashBlock.ToString());
        }
    }
    return nTimeSmart;
}

bool CWallet::AddDestData(const CTxDestination &dest, const std::string &key, const std::string &value)
{
    if (boost::get<CNoDestination>(&dest))
        return false;

    mapAddressBook[dest].destdata.insert(std::make_pair(key, value));
    return WalletBatch(*database).WriteDestData(EncodeDestination(dest), key, value);
}

bool CWallet::EraseDestData(const CTxDestination &dest, const std::string &key)
{
    if (!mapAddressBook[dest].destdata.erase(key))
        return false;
    return WalletBatch(*database).EraseDestData(EncodeDestination(dest), key);
}

bool CWallet::LoadDestData(const CTxDestination &dest, const std::string &key, const std::string &value)
{
    mapAddressBook[dest].destdata.insert(std::make_pair(key, value));
    return true;
}

bool CWallet::GetDestData(const CTxDestination &dest, const std::string &key, std::string *value) const
{
    std::map<CTxDestination, CAddressBookData>::const_iterator i = mapAddressBook.find(dest);
    if(i != mapAddressBook.end())
    {
        CAddressBookData::StringMap::const_iterator j = i->second.destdata.find(key);
        if(j != i->second.destdata.end())
        {
            if(value)
                *value = j->second;
            return true;
        }
    }
    return false;
}

std::vector<std::string> CWallet::GetDestValues(const std::string& prefix) const
{
    LOCK(cs_wallet);
    std::vector<std::string> values;
    for (const auto& address : mapAddressBook) {
        for (const auto& data : address.second.destdata) {
            if (!data.first.compare(0, prefix.size(), prefix)) {
                values.emplace_back(data.second);
            }
        }
    }
    return values;
}

bool CWallet::Verify(const WalletLocation& location, bool salvage_wallet, std::string& error_string, std::string& warning_string)
{
    // Do some checking on wallet path. It should be either a:
    //
    // 1. Path where a directory can be created.
    // 2. Path to an existing directory.
    // 3. Path to a symlink to a directory.
    // 4. For backwards compatibility, the name of a data file in -walletdir.
    LOCK(cs_wallets);
    const fs::path& wallet_path = location.GetPath();
    fs::file_type path_type = fs::symlink_status(wallet_path).type();
    if (!(path_type == fs::file_not_found || path_type == fs::directory_file ||
          (path_type == fs::symlink_file && fs::is_directory(wallet_path)) ||
          (path_type == fs::regular_file && fs::path(location.GetName()).filename() == location.GetName()))) {
        error_string =strprintf(
                "Invalid -wallet path '%s'. -wallet path should point to a directory where wallet.dat and "
                  "database/log.?????????? files can be stored, a location where such a directory could be created, "
                  "or (for backwards compatibility) the name of an existing data file in -walletdir (%s)",
                location.GetName(), GetWalletDir());
        return false;
    }

    // Make sure that the wallet path doesn't clash with an existing wallet path
    if (IsWalletLoaded(wallet_path)) {
        error_string = strprintf("Error loading wallet %s. Duplicate -wallet filename specified.", location.GetName());
        return false;
    }

    try {
        if (!WalletBatch::VerifyEnvironment(wallet_path, error_string)) {
            return false;
        }
    } catch (const fs::filesystem_error& e) {
        error_string = strprintf("Error loading wallet %s. %s", location.GetName(), e.what());
        return false;
    }

    std::unique_ptr<CWallet> tempWallet = MakeUnique<CWallet>(location, WalletDatabase::Create(wallet_path));
    if (!tempWallet->AutoBackupWallet(wallet_path, warning_string, error_string) && !error_string.empty()) {
        return false;
    }

    if (salvage_wallet) {
        // Recover readable keypairs:
        CWallet dummyWallet(WalletLocation(), WalletDatabase::CreateDummy());
        std::string backup_filename;
        if (!WalletBatch::Recover(wallet_path, (void *)&dummyWallet, WalletBatch::RecoverKeysOnlyFilter, backup_filename)) {
            return false;
        }
    }

    return WalletBatch::VerifyDatabaseFile(wallet_path, warning_string, error_string);
}

std::shared_ptr<CWallet> CWallet::CreateWalletFromFile(const WalletLocation& location)
{
    const std::string& walletFile = location.GetName();

    // needed to restore wallet transaction meta data after -zapwallettxes
    std::vector<CWalletTx> vWtx;

    if (gArgs.GetBoolArg("-zapwallettxes", false)) {
        uiInterface.InitMessage(_("Zapping all transactions from wallet..."));

        std::unique_ptr<CWallet> tempWallet = MakeUnique<CWallet>(location, WalletDatabase::Create(location.GetPath()));
        DBErrors nZapWalletRet = tempWallet->ZapWalletTx(vWtx);
        if (nZapWalletRet != DBErrors::LOAD_OK) {
            InitError(strprintf(_("Error loading %s: Wallet corrupted"), walletFile));
            return nullptr;
        }
    }

    uiInterface.InitMessage(_("Loading wallet..."));

    int64_t nStart = GetTimeMillis();
    bool fFirstRun = true;
    // TODO: Can't use std::make_shared because we need a custom deleter but
    // should be possible to use std::allocate_shared.
    std::shared_ptr<CWallet> walletInstance(new CWallet(location, WalletDatabase::Create(location.GetPath())), ReleaseWallet);
    AddWallet(walletInstance);
    auto error = [&](const std::string& strError) {
        RemoveWallet(walletInstance);
        InitError(strError);
        return nullptr;
    };
    DBErrors nLoadWalletRet;
    try {
        nLoadWalletRet = walletInstance->LoadWallet(fFirstRun);
    } catch (const std::exception& e) {
        RemoveWallet(walletInstance);
        throw;
    }
    if (nLoadWalletRet != DBErrors::LOAD_OK)
    {
        if (nLoadWalletRet == DBErrors::CORRUPT) {
            return error(strprintf(_("Error loading %s: Wallet corrupted"), walletFile));
        }
        else if (nLoadWalletRet == DBErrors::NONCRITICAL_ERROR)
        {
            InitWarning(strprintf(_("Error reading %s! All keys read correctly, but transaction data"
                                         " or address book entries might be missing or incorrect."),
                walletFile));
        }
        else if (nLoadWalletRet == DBErrors::TOO_NEW) {
            return error(strprintf(_("Error loading %s: Wallet requires newer version of %s"), walletFile, _(PACKAGE_NAME)));
        }
        else if (nLoadWalletRet == DBErrors::NEED_REWRITE)
        {
            return error(strprintf(_("Wallet needed to be rewritten: restart %s to complete"), _(PACKAGE_NAME)));
        }
        else {
            return error(strprintf(_("Error loading %s"), walletFile));
        }
    }

    if (gArgs.GetBoolArg("-upgradewallet", fFirstRun))
    {
        int nMaxVersion = gArgs.GetArg("-upgradewallet", 0);
        if (nMaxVersion == 0) // the -upgradewallet without argument case
        {
            LogPrintf("Performing wallet upgrade to %i\n", FEATURE_LATEST);
            nMaxVersion = FEATURE_LATEST;
            walletInstance->SetMinVersion(FEATURE_LATEST); // permanently upgrade the wallet immediately
        }
        else
            LogPrintf("Allowing wallet upgrade up to %i\n", nMaxVersion);
        if (nMaxVersion < walletInstance->GetVersion())
        {
            return error(_("Cannot downgrade wallet"));
        }
        walletInstance->SetMaxVersion(nMaxVersion);
    }

    if (fFirstRun)
    {
        // Create new keyUser and set as default key
        if (gArgs.GetBoolArg("-usehd", DEFAULT_USE_HD_WALLET) && !walletInstance->IsHDEnabled()) {
            std::string strSeed = gArgs.GetArg("-hdseed", "not hex");

            if (gArgs.IsArgSet("-hdseed") && IsHex(strSeed)) {
                CHDChain newHdChain;
                std::vector<unsigned char> vchSeed = ParseHex(strSeed);
                if (!newHdChain.SetSeed(SecureVector(vchSeed.begin(), vchSeed.end()), true)) {
                    return error(_("SetSeed failed"));
                }
                if (!walletInstance->SetHDChainSingle(newHdChain, false)) {
                    return error(_("SetHDChainSingle failed"));
                }
                newHdChain.Debug(__func__);
            } else {
                if (gArgs.IsArgSet("-hdseed") && !IsHex(strSeed)) {
                    LogPrintf("%s -- Incorrect seed, generating a random mnemonic instead\n", __func__);
                }
                SecureString secureMnemonic = gArgs.GetArg("-mnemonic", "").c_str();
                SecureString secureMnemonicPassphrase = gArgs.GetArg("-mnemonicpassphrase", "").c_str();
                walletInstance->GenerateNewHDChain(secureMnemonic, secureMnemonicPassphrase);
            }

            // ensure this wallet.dat can only be opened by clients supporting HD
            LogPrintf("Upgrading wallet to HD\n");
            walletInstance->SetMinVersion(FEATURE_HD);

            // clean up
            gArgs.ForceRemoveArg("-hdseed");
            gArgs.ForceRemoveArg("-mnemonic");
            gArgs.ForceRemoveArg("-mnemonicpassphrase");
        }

        // Top up the keypool
        if (!walletInstance->TopUpKeyPool()) {
            return error(_("Unable to generate initial keys"));
        }

        walletInstance->SetBestChain(chainActive.GetLocator());

        // Try to create wallet backup right after new wallet was created
        std::string strBackupWarning;
        std::string strBackupError;
        if(!walletInstance->AutoBackupWallet("", strBackupWarning, strBackupError)) {
            if (!strBackupWarning.empty()) {
                InitWarning(strBackupWarning);
            }
            if (!strBackupError.empty()) {
                return error(strBackupError);
            }
        }

    }
    else if (gArgs.IsArgSet("-usehd")) {
        bool useHD = gArgs.GetBoolArg("-usehd", DEFAULT_USE_HD_WALLET);
        if (walletInstance->IsHDEnabled() && !useHD) {
            return error(strprintf(_("Error loading %s: You can't disable HD on an already existing HD wallet"),
                                walletInstance->GetName()));
        }
        if (!walletInstance->IsHDEnabled() && useHD) {
            return error(strprintf(_("Error loading %s: You can't enable HD on an already existing non-HD wallet"),
                                walletInstance->GetName()));
        }
    }

    // Warn user every time he starts non-encrypted HD wallet
    if (gArgs.GetBoolArg("-usehd", DEFAULT_USE_HD_WALLET) && !walletInstance->IsLocked()) {
        InitWarning(_("Make sure to encrypt your wallet and delete all non-encrypted backups after you have verified that the wallet works!"));
    }

    LogPrintf(" wallet      %15dms\n", GetTimeMillis() - nStart);

    // Try to top up keypool. No-op if the wallet is locked.
    walletInstance->TopUpKeyPool();

    LOCK(cs_main);

    CBlockIndex *pindexRescan = chainActive.Genesis();
    if (!gArgs.GetBoolArg("-rescan", false))
    {
        WalletBatch batch(*walletInstance->database);
        CBlockLocator locator;
        if (batch.ReadBestBlock(locator))
            pindexRescan = FindForkInGlobalIndex(chainActive, locator);
    }

    walletInstance->m_last_block_processed = chainActive.Tip();

    if (chainActive.Tip() && chainActive.Tip() != pindexRescan)
    {
        //We can't rescan beyond non-pruned blocks, stop and throw an error
        //this might happen if a user uses an old wallet within a pruned node
        // or if he ran -disablewallet for a longer time, then decided to re-enable
        if (fPruneMode)
        {
            CBlockIndex *block = chainActive.Tip();
            while (block && block->pprev && (block->pprev->nStatus & BLOCK_HAVE_DATA) && block->pprev->nTx > 0 && pindexRescan != block)
                block = block->pprev;

            if (pindexRescan != block) {
                return error(_("Prune: last wallet synchronisation goes beyond pruned data. You need to -reindex (download the whole blockchain again in case of pruned node)"));
            }
        }

        uiInterface.InitMessage(_("Rescanning..."));
        LogPrintf("Rescanning last %i blocks (from block %i)...\n", chainActive.Height() - pindexRescan->nHeight, pindexRescan->nHeight);

        // No need to read and scan block if block was created before
        // our wallet birthday (as adjusted for block time variability)
        // unless a full rescan was requested
        if (gArgs.GetArg("-rescan", 0) != 2) {
            while (pindexRescan && walletInstance->nTimeFirstKey && (pindexRescan->GetBlockTime() < (walletInstance->nTimeFirstKey - TIMESTAMP_WINDOW))) {
                pindexRescan = chainActive.Next(pindexRescan);
            }
        }

        nStart = GetTimeMillis();
        {
            WalletRescanReserver reserver(walletInstance.get());
            if (!reserver.reserve()) {
                return error(_("Failed to rescan the wallet during initialization"));
            }
            walletInstance->ScanForWalletTransactions(pindexRescan, nullptr, reserver, true);
        }
        LogPrintf(" rescan      %15dms\n", GetTimeMillis() - nStart);
        walletInstance->SetBestChain(chainActive.GetLocator());
        walletInstance->database->IncrementUpdateCounter();

        // Restore wallet transaction metadata after -zapwallettxes=1
        if (gArgs.GetBoolArg("-zapwallettxes", false) && gArgs.GetArg("-zapwallettxes", "1") != "2")
        {
            WalletBatch batch(*walletInstance->database);

            for (const CWalletTx& wtxOld : vWtx)
            {
                uint256 hash = wtxOld.GetHash();
                std::map<uint256, CWalletTx>::iterator mi = walletInstance->mapWallet.find(hash);
                if (mi != walletInstance->mapWallet.end())
                {
                    const CWalletTx* copyFrom = &wtxOld;
                    CWalletTx* copyTo = &mi->second;
                    copyTo->mapValue = copyFrom->mapValue;
                    copyTo->vOrderForm = copyFrom->vOrderForm;
                    copyTo->nTimeReceived = copyFrom->nTimeReceived;
                    copyTo->nTimeSmart = copyFrom->nTimeSmart;
                    copyTo->fFromMe = copyFrom->fFromMe;
                    copyTo->strFromAccount = copyFrom->strFromAccount;
                    copyTo->nOrderPos = copyFrom->nOrderPos;
                    batch.WriteTx(*copyTo);
                }
            }
        }
    }

    uiInterface.LoadWallet(walletInstance);

    // Register with the validation interface. It's ok to do this after rescan since we're still holding cs_main.
    RegisterValidationInterface(walletInstance.get());

    walletInstance->SetBroadcastTransactions(gArgs.GetBoolArg("-walletbroadcast", DEFAULT_WALLETBROADCAST));

    {
        LOCK(walletInstance->cs_wallet);
        LogPrintf("setExternalKeyPool.size() = %u\n",   walletInstance->KeypoolCountExternalKeys());
        LogPrintf("setInternalKeyPool.size() = %u\n",   walletInstance->KeypoolCountInternalKeys());
        LogPrintf("mapWallet.size() = %u\n",            walletInstance->mapWallet.size());
        LogPrintf("mapAddressBook.size() = %u\n",       walletInstance->mapAddressBook.size());
        LogPrintf("nTimeFirstKey = %u\n",               walletInstance->nTimeFirstKey);
    }

    return walletInstance;
}

void CWallet::postInitProcess()
{
    // Add wallet transactions that aren't already in a block to mempool
    // Do this here as mempool requires genesis block to be loaded
    ReacceptWalletTransactions();
}

bool CWallet::InitAutoBackup()
{
    if (gArgs.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET))
        return true;

    nWalletBackups = gArgs.GetArg("-createwalletbackups", 10);
    nWalletBackups = std::max(0, std::min(10, nWalletBackups));

    return true;
}

bool CWallet::BackupWallet(const std::string& strDest)
{
    return database->Backup(strDest);
}

// This should be called carefully:
// either supply the actual wallet_path to make a raw copy of wallet.dat or "" to backup current instance via BackupWallet()
bool CWallet::AutoBackupWallet(const fs::path& wallet_path, std::string& strBackupWarningRet, std::string& strBackupErrorRet)
{
    strBackupWarningRet = strBackupErrorRet = "";
    std::string strWalletName = GetName();
    if (strWalletName.empty()) {
        strWalletName = "wallet.dat";
    }

    if (nWalletBackups <= 0) {
        LogPrintf("Automatic wallet backups are disabled!\n");
        return false;
    }

    fs::path backupsDir = GetBackupsDir();
    backupsDir.make_preferred();

    if (!fs::exists(backupsDir))
    {
        // Always create backup folder to not confuse the operating system's file browser
        LogPrintf("Creating backup folder %s\n", backupsDir.string());
        if(!fs::create_directories(backupsDir)) {
            // something is wrong, we shouldn't continue until it's resolved
            strBackupErrorRet = strprintf(_("Wasn't able to create wallet backup folder %s!"), backupsDir.string());
            LogPrintf("%s\n", strBackupErrorRet);
            nWalletBackups = -1;
            return false;
        }
    } else if (!fs::is_directory(backupsDir)) {
        // something is wrong, we shouldn't continue until it's resolved
        strBackupErrorRet = strprintf(_("%s is not a valid backup folder!"), backupsDir.string());
        LogPrintf("%s\n", strBackupErrorRet);
        nWalletBackups = -1;
        return false;
    }

    // Create backup of the ...
    std::string dateTimeStr = DateTimeStrFormat(".%Y-%m-%d-%H-%M", GetTime());
    if (wallet_path.empty()) {
        // ... opened wallet
        LOCK2(cs_main, cs_wallet);
        fs::path backupFile = backupsDir / (strWalletName + dateTimeStr);
        backupFile.make_preferred();
        if (!BackupWallet(backupFile.string())) {
            strBackupWarningRet = strprintf(_("Failed to create backup %s!"), backupFile.string());
            LogPrintf("%s\n", strBackupWarningRet);
            nWalletBackups = -1;
            return false;
        }

        // Update nKeysLeftSinceAutoBackup using current external keypool size
        nKeysLeftSinceAutoBackup = KeypoolCountExternalKeys();
        LogPrintf("nKeysLeftSinceAutoBackup: %d\n", nKeysLeftSinceAutoBackup);
        if (IsLocked(true)) {
            strBackupWarningRet = _("Wallet is locked, can't replenish keypool! Automatic backups and mixing are disabled, please unlock your wallet to replenish keypool.");
            LogPrintf("%s\n", strBackupWarningRet);
            nWalletBackups = -2;
            return false;
        }
    } else {
        // ... strWalletName file
        std::string strSourceFile;
        BerkeleyEnvironment* env = GetWalletEnv(wallet_path, strSourceFile);
        fs::path sourceFile = env->Directory() / strSourceFile;
        fs::path backupFile = backupsDir / (strWalletName + dateTimeStr);
        sourceFile.make_preferred();
        backupFile.make_preferred();
        if (fs::exists(backupFile))
        {
            strBackupWarningRet = _("Failed to create backup, file already exists! This could happen if you restarted wallet in less than 60 seconds. You can continue if you are ok with this.");
            LogPrintf("%s\n", strBackupWarningRet);
            return false;
        }
        if(fs::exists(sourceFile)) {
            try {
                fs::copy_file(sourceFile, backupFile);
                LogPrintf("Creating backup of %s -> %s\n", sourceFile.string(), backupFile.string());
            } catch(fs::filesystem_error &error) {
                strBackupWarningRet = strprintf(_("Failed to create backup, error: %s"), error.what());
                LogPrintf("%s\n", strBackupWarningRet);
                nWalletBackups = -1;
                return false;
            }
        }
    }

    // Keep only the last 10 backups, including the new one of course
    typedef std::multimap<std::time_t, fs::path> folder_set_t;
    folder_set_t folder_set;
    fs::directory_iterator end_iter;
    // Build map of backup files for current(!) wallet sorted by last write time
    fs::path currentFile;
    for (fs::directory_iterator dir_iter(backupsDir); dir_iter != end_iter; ++dir_iter)
    {
        // Only check regular files
        if ( fs::is_regular_file(dir_iter->status()))
        {
            currentFile = dir_iter->path().filename();
            // Only add the backups for the current wallet, e.g. wallet.dat.*
            if (dir_iter->path().stem().string() == strWalletName) {
                folder_set.insert(folder_set_t::value_type(fs::last_write_time(dir_iter->path()), *dir_iter));
            }
        }
    }

    // Loop backward through backup files and keep the N newest ones (1 <= N <= 10)
    int counter = 0;
    for(auto it = folder_set.rbegin(); it != folder_set.rend(); ++it) {
        std::pair<const std::time_t, fs::path> file = *it;
        counter++;
        if (counter > nWalletBackups)
        {
            // More than nWalletBackups backups: delete oldest one(s)
            try {
                fs::remove(file.second);
                LogPrintf("Old backup deleted: %s\n", file.second);
            } catch(fs::filesystem_error &error) {
                strBackupWarningRet = strprintf(_("Failed to delete backup, error: %s"), error.what());
                LogPrintf("%s\n", strBackupWarningRet);
                return false;
            }
        }
    }

    return true;
}

void CWallet::NotifyTransactionLock(const CTransactionRef &tx, const std::shared_ptr<const llmq::CInstantSendLock>& islock)
{
    LOCK(cs_wallet);
    // Only notify UI if this transaction is in this wallet
    uint256 txHash = tx->GetHash();
    std::map<uint256, CWalletTx>::const_iterator mi = mapWallet.find(txHash);
    if (mi != mapWallet.end()){
        NotifyTransactionChanged(this, txHash, CT_UPDATED);
        NotifyISLockReceived();
        // notify an external script
        std::string strCmd = gArgs.GetArg("-instantsendnotify", "");
        if (!strCmd.empty()) {
            boost::replace_all(strCmd, "%s", txHash.GetHex());
            std::thread t(runCommand, strCmd);
            t.detach(); // thread runs free
        }
    }
}

void CWallet::NotifyChainLock(const CBlockIndex* pindexChainLock, const std::shared_ptr<const llmq::CChainLockSig>& clsig)
{
    NotifyChainLockReceived(pindexChainLock->nHeight);
}

bool CWallet::LoadGovernanceObject(const CGovernanceObject& obj)
{
    AssertLockHeld(cs_wallet);
    return m_gobjects.emplace(obj.GetHash(), obj).second;
}

bool CWallet::WriteGovernanceObject(const CGovernanceObject& obj)
{
    AssertLockHeld(cs_wallet);
    WalletBatch batch(*database);
    return batch.WriteGovernanceObject(obj) && LoadGovernanceObject(obj);
}

std::vector<const CGovernanceObject*> CWallet::GetGovernanceObjects()
{
    AssertLockHeld(cs_wallet);
    std::vector<const CGovernanceObject*> vecObjects;
    vecObjects.reserve(m_gobjects.size());
    for (auto& obj : m_gobjects) {
        vecObjects.push_back(&obj.second);
    }
    return vecObjects;
}

CKeyPool::CKeyPool()
{
    nTime = GetTime();
    fInternal = false;
}

CKeyPool::CKeyPool(const CPubKey& vchPubKeyIn, bool fInternalIn)
{
    nTime = GetTime();
    vchPubKey = vchPubKeyIn;
    fInternal = fInternalIn;
}

CWalletKey::CWalletKey(int64_t nExpires)
{
    nTimeCreated = (nExpires ? GetTime() : 0);
    nTimeExpires = nExpires;
}

void CMerkleTx::SetMerkleBranch(const CBlockIndex* pindex, int posInBlock)
{
    // Update the tx's hashBlock
    hashBlock = pindex->GetBlockHash();

    // set the position of the transaction in the block
    nIndex = posInBlock;
}

int CMerkleTx::GetDepthInMainChain() const
{
    int nResult;

    if (hashUnset())
        return 0;

    AssertLockHeld(cs_main);

    // Find the block it claims to be in
    CBlockIndex* pindex = LookupBlockIndex(hashBlock);
    if (!pindex || !chainActive.Contains(pindex))
        return 0;

    return ((nIndex == -1) ? (-1) : 1) * (chainActive.Height() - pindex->nHeight + 1);
}

bool CMerkleTx::IsLockedByInstantSend() const
{
    if (fIsChainlocked) {
        fIsInstantSendLocked = false;
    } else if (!fIsInstantSendLocked) {
        fIsInstantSendLocked = llmq::quorumInstantSendManager->IsLocked(GetHash());
    }
    return fIsInstantSendLocked;
}

bool CMerkleTx::IsChainLocked() const
{
    if (!fIsChainlocked) {
        AssertLockHeld(cs_main);
        CBlockIndex* pIndex = LookupBlockIndex(hashBlock);
        if (pIndex != nullptr) {
            fIsChainlocked = llmq::chainLocksHandler->HasChainLock(pIndex->nHeight, hashBlock);
        }
    }
    return fIsChainlocked;
}

int CMerkleTx::GetBlocksToMaturity() const
{
    if (!IsCoinBase())
        return 0;
    int chain_depth = GetDepthInMainChain();
    assert(chain_depth >= 0); // coinbase tx should not be conflicted
    return std::max(0, (COINBASE_MATURITY+1) - chain_depth);
}


bool CWalletTx::AcceptToMemoryPool(const CAmount& nAbsurdFee, CValidationState& state)
{
    // We must set fInMempool here - while it will be re-set to true by the
    // entered-mempool callback, if we did not there would be a race where a
    // user could call sendmoney in a loop and hit spurious out of funds errors
    // because we think that this newly generated transaction's change is
    // unavailable as we're not yet aware that it is in the mempool.
    bool ret = ::AcceptToMemoryPool(mempool, state, tx, nullptr /* pfMissingInputs */,
                                false /* bypass_limits */, nAbsurdFee);
    fInMempool |= ret;
    return ret;
}
