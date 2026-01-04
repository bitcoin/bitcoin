// Copyright (c) 2014-2025 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ACTIVE_MASTERNODE_H
#define BITCOIN_ACTIVE_MASTERNODE_H

#include <bls/bls.h>

#include <netaddress.h>
#include <primitives/transaction.h>
#include <threadsafety.h>
#include <validationinterface.h>

class CConnman;
class CDeterministicMNManager;

class CActiveMasternodeManager
{
private:
    CConnman& m_connman;
    CDeterministicMNManager& m_dmnman;
    const CBLSPublicKey m_operator_pk;
    const CBLSSecretKey m_operator_sk;

private:
    enum class MasternodeState {
        WAITING_FOR_PROTX,
        POSE_BANNED,
        REMOVED,
        OPERATOR_KEY_CHANGED,
        PROTX_IP_CHANGED,
        READY,
        SOME_ERROR,
    };

    mutable SharedMutex cs;
    COutPoint m_outpoint GUARDED_BY(cs);
    CService m_service GUARDED_BY(cs);
    MasternodeState m_state GUARDED_BY(cs){MasternodeState::WAITING_FOR_PROTX};
    std::string m_error GUARDED_BY(cs);
    uint256 m_protx_hash GUARDED_BY(cs);

public:
    CActiveMasternodeManager() = delete;
    CActiveMasternodeManager(const CActiveMasternodeManager&) = delete;
    CActiveMasternodeManager& operator=(const CActiveMasternodeManager&) = delete;
    explicit CActiveMasternodeManager(CConnman& connman, CDeterministicMNManager& dmnman, const CBLSSecretKey& sk);
    ~CActiveMasternodeManager();

    void UpdatedBlockTip(const CBlockIndex* pindexNew, const CBlockIndex* pindexFork, bool fInitialDownload)
        EXCLUSIVE_LOCKS_REQUIRED(!cs);

    void Init(const CBlockIndex* pindex) EXCLUSIVE_LOCKS_REQUIRED(!cs) { LOCK(cs); InitInternal(pindex); };

    std::string GetStateString() const;
    std::string GetStatus() const;

    static bool IsValidNetAddr(const CService& addrIn);

    template <template <typename> class EncryptedObj, typename Obj>
    [[nodiscard]] bool Decrypt(const EncryptedObj<Obj>& obj, size_t idx, Obj& ret_obj, int version) const
        EXCLUSIVE_LOCKS_REQUIRED(!cs);
    [[nodiscard]] CBLSSignature Sign(const uint256& hash, const bool is_legacy) const EXCLUSIVE_LOCKS_REQUIRED(!cs);
    [[nodiscard]] std::vector<uint8_t> SignBasic(const uint256& hash) const EXCLUSIVE_LOCKS_REQUIRED(!cs);

    [[nodiscard]] COutPoint GetOutPoint() const { READ_LOCK(cs); return m_outpoint; }
    [[nodiscard]] uint256 GetProTxHash() const { READ_LOCK(cs); return m_protx_hash; }
    [[nodiscard]] CService GetService() const { READ_LOCK(cs); return m_service; }
    [[nodiscard]] CBLSPublicKey GetPubKey() const;

private:
    void InitInternal(const CBlockIndex* pindex) EXCLUSIVE_LOCKS_REQUIRED(cs);
    bool GetLocalAddress(CService& addrRet) EXCLUSIVE_LOCKS_REQUIRED(cs);
};

#endif // BITCOIN_ACTIVE_MASTERNODE_H
