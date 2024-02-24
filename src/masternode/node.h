// Copyright (c) 2014-2023 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MASTERNODE_NODE_H
#define BITCOIN_MASTERNODE_NODE_H

#include <netaddress.h>
#include <primitives/transaction.h>
#include <validationinterface.h>

class CBLSPublicKey;
class CBLSSecretKey;

struct CActiveMasternodeInfo;
class CActiveMasternodeManager;

extern CActiveMasternodeInfo activeMasternodeInfo;
extern RecursiveMutex activeMasternodeInfoCs;
extern std::unique_ptr<CActiveMasternodeManager> activeMasternodeManager;

struct CActiveMasternodeInfo {
    // Keys for the active Masternode
    std::unique_ptr<CBLSPublicKey> blsPubKeyOperator;
    std::unique_ptr<CBLSSecretKey> blsKeyOperator;

    // Initialized while registering Masternode
    uint256 proTxHash;
    COutPoint outpoint;
    CService service;
    bool legacy{true};
};


class CActiveMasternodeManager final : public CValidationInterface
{
public:
    enum masternode_state_t {
        MASTERNODE_WAITING_FOR_PROTX,
        MASTERNODE_POSE_BANNED,
        MASTERNODE_REMOVED,
        MASTERNODE_OPERATOR_KEY_CHANGED,
        MASTERNODE_PROTX_IP_CHANGED,
        MASTERNODE_READY,
        MASTERNODE_ERROR,
    };

private:
    masternode_state_t state{MASTERNODE_WAITING_FOR_PROTX};
    std::string strError;
    CConnman& connman;

public:
    explicit CActiveMasternodeManager(CConnman& _connman) : connman(_connman) {};
    ~CActiveMasternodeManager() = default;

    void UpdatedBlockTip(const CBlockIndex* pindexNew, const CBlockIndex* pindexFork, bool fInitialDownload) override;

    void Init(const CBlockIndex* pindex);

    std::string GetStateString() const;
    std::string GetStatus() const;

    static bool IsValidNetAddr(CService addrIn);

private:
    bool GetLocalAddress(CService& addrRet);
};

#endif // BITCOIN_MASTERNODE_NODE_H
