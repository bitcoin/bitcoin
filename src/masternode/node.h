// Copyright (c) 2014-2024 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MASTERNODE_NODE_H
#define BITCOIN_MASTERNODE_NODE_H

#include <bls/bls.h>
#include <netaddress.h>
#include <primitives/transaction.h>
#include <threadsafety.h>
#include <validationinterface.h>

class CDeterministicMNManager;

struct CActiveMasternodeInfo {
    // Keys for the active Masternode
    const CBLSSecretKey blsKeyOperator;
    const CBLSPublicKey blsPubKeyOperator;

    // Initialized while registering Masternode
    uint256 proTxHash;
    COutPoint outpoint;
    CService service;

    CActiveMasternodeInfo(const CBLSSecretKey& blsKeyOperator, const CBLSPublicKey& blsPubKeyOperator) :
        blsKeyOperator(blsKeyOperator), blsPubKeyOperator(blsPubKeyOperator) {};
};

class CActiveMasternodeManager
{
public:
    enum class MasternodeState {
        WAITING_FOR_PROTX,
        POSE_BANNED,
        REMOVED,
        OPERATOR_KEY_CHANGED,
        PROTX_IP_CHANGED,
        READY,
        SOME_ERROR,
    };

private:
    mutable SharedMutex cs;
    MasternodeState m_state GUARDED_BY(cs){MasternodeState::WAITING_FOR_PROTX};
    CActiveMasternodeInfo m_info GUARDED_BY(cs);
    std::string m_error GUARDED_BY(cs);

    CConnman& m_connman;
    const std::unique_ptr<CDeterministicMNManager>& m_dmnman;

public:
    explicit CActiveMasternodeManager(const CBLSSecretKey& sk, CConnman& connman, const std::unique_ptr<CDeterministicMNManager>& dmnman);

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

    /* TODO: Reconsider external locking */
    [[nodiscard]] COutPoint GetOutPoint() const { READ_LOCK(cs); return m_info.outpoint; }
    [[nodiscard]] uint256 GetProTxHash() const { READ_LOCK(cs); return m_info.proTxHash; }
    [[nodiscard]] CService GetService() const { READ_LOCK(cs); return m_info.service; }
    [[nodiscard]] CBLSPublicKey GetPubKey() const;

private:
    void InitInternal(const CBlockIndex* pindex) EXCLUSIVE_LOCKS_REQUIRED(cs);
    bool GetLocalAddress(CService& addrRet) EXCLUSIVE_LOCKS_REQUIRED(cs);
};

#endif // BITCOIN_MASTERNODE_NODE_H
