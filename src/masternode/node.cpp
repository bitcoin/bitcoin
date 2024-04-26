// Copyright (c) 2014-2023 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <masternode/node.h>

#include <bls/bls_ies.h>
#include <chainparams.h>
#include <deploymentstatus.h>
#include <evo/deterministicmns.h>
#include <net.h>
#include <netbase.h>
#include <protocol.h>
#include <util/check.h>
#include <validation.h>
#include <warnings.h>

CActiveMasternodeManager::CActiveMasternodeManager(const CBLSSecretKey& sk, CConnman& connman, const std::unique_ptr<CDeterministicMNManager>& dmnman) :
    m_info(sk, sk.GetPublicKey()),
    m_connman{connman},
    m_dmnman{dmnman}
{
    assert(sk.IsValid()); /* We can assume pk is valid if sk is valid */
    LogPrintf("MASTERNODE:\n  blsPubKeyOperator legacy: %s\n  blsPubKeyOperator basic: %s\n",
            m_info.blsPubKeyOperator.ToString(/*specificLegacyScheme=*/ true),
            m_info.blsPubKeyOperator.ToString(/*specificLegacyScheme=*/ false));
}

std::string CActiveMasternodeManager::GetStateString() const
{
    switch (WITH_READ_LOCK(cs, return m_state)) {
    case MASTERNODE_WAITING_FOR_PROTX:
        return "WAITING_FOR_PROTX";
    case MASTERNODE_POSE_BANNED:
        return "POSE_BANNED";
    case MASTERNODE_REMOVED:
        return "REMOVED";
    case MASTERNODE_OPERATOR_KEY_CHANGED:
        return "OPERATOR_KEY_CHANGED";
    case MASTERNODE_PROTX_IP_CHANGED:
        return "PROTX_IP_CHANGED";
    case MASTERNODE_READY:
        return "READY";
    case MASTERNODE_ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

std::string CActiveMasternodeManager::GetStatus() const
{
    READ_LOCK(cs);
    switch (m_state) {
    case MASTERNODE_WAITING_FOR_PROTX:
        return "Waiting for ProTx to appear on-chain";
    case MASTERNODE_POSE_BANNED:
        return "Masternode was PoSe banned";
    case MASTERNODE_REMOVED:
        return "Masternode removed from list";
    case MASTERNODE_OPERATOR_KEY_CHANGED:
        return "Operator key changed or revoked";
    case MASTERNODE_PROTX_IP_CHANGED:
        return "IP address specified in ProTx changed";
    case MASTERNODE_READY:
        return "Ready";
    case MASTERNODE_ERROR:
        return "Error. " + m_error;
    default:
        return "Unknown";
    }
}

void CActiveMasternodeManager::InitInternal(const CBlockIndex* pindex)
{
    AssertLockHeld(cs);

    if (!DeploymentDIP0003Enforced(pindex->nHeight, Params().GetConsensus())) return;

    // Check that our local network configuration is correct
    if (!fListen && Params().RequireRoutableExternalIP()) {
        // listen option is probably overwritten by something else, no good
        m_state = MASTERNODE_ERROR;
        m_error = "Masternode must accept connections from outside. Make sure listen configuration option is not overwritten by some another parameter.";
        LogPrintf("CActiveMasternodeManager::Init -- ERROR: %s\n", m_error);
        return;
    }

    if (!GetLocalAddress(m_info.service)) {
        m_state = MASTERNODE_ERROR;
        return;
    }

    CDeterministicMNList mnList = Assert(m_dmnman)->GetListForBlock(pindex);

    CDeterministicMNCPtr dmn = mnList.GetMNByOperatorKey(m_info.blsPubKeyOperator);
    if (!dmn) {
        // MN not appeared on the chain yet
        return;
    }

    if (!mnList.IsMNValid(dmn->proTxHash)) {
        if (mnList.IsMNPoSeBanned(dmn->proTxHash)) {
            m_state = MASTERNODE_POSE_BANNED;
        } else {
            m_state = MASTERNODE_REMOVED;
        }
        return;
    }

    LogPrintf("CActiveMasternodeManager::Init -- proTxHash=%s, proTx=%s\n", dmn->proTxHash.ToString(), dmn->ToString());

    if (m_info.service != dmn->pdmnState->addr) {
        m_state = MASTERNODE_ERROR;
        m_error = "Local address does not match the address from ProTx";
        LogPrintf("CActiveMasternodeManager::Init -- ERROR: %s\n", m_error);
        return;
    }

    // Check socket connectivity
    LogPrintf("CActiveMasternodeManager::Init -- Checking inbound connection to '%s'\n", m_info.service.ToString());
    std::unique_ptr<Sock> sock = CreateSock(m_info.service);
    if (!sock) {
        m_state = MASTERNODE_ERROR;
        m_error = "Could not create socket to connect to " + m_info.service.ToString();
        LogPrintf("CActiveMasternodeManager::Init -- ERROR: %s\n", m_error);
        return;
    }
    bool fConnected = ConnectSocketDirectly(m_info.service, *sock, nConnectTimeout, true) && IsSelectableSocket(sock->Get());
    sock->Reset();

    if (!fConnected && Params().RequireRoutableExternalIP()) {
        m_state = MASTERNODE_ERROR;
        m_error = "Could not connect to " + m_info.service.ToString();
        LogPrintf("CActiveMasternodeManager::Init -- ERROR: %s\n", m_error);
        return;
    }

    m_info.proTxHash = dmn->proTxHash;
    m_info.outpoint = dmn->collateralOutpoint;
    m_info.legacy = dmn->pdmnState->nVersion == CProRegTx::LEGACY_BLS_VERSION;
    m_state = MASTERNODE_READY;
}

void CActiveMasternodeManager::UpdatedBlockTip(const CBlockIndex* pindexNew, const CBlockIndex* pindexFork, bool fInitialDownload)
{
    if (!DeploymentDIP0003Enforced(pindexNew->nHeight, Params().GetConsensus())) return;

    const auto [cur_state, cur_protx_hash] = WITH_READ_LOCK(cs, return std::make_pair(m_state, m_info.proTxHash));
    if (cur_state == MASTERNODE_READY) {
        auto oldMNList = Assert(m_dmnman)->GetListForBlock(pindexNew->pprev);
        auto newMNList = m_dmnman->GetListForBlock(pindexNew);
        auto reset = [this, pindexNew] (masternode_state_t state) -> void {
            LOCK(cs);
            m_state = state;
            m_info.proTxHash = uint256();
            m_info.outpoint.SetNull();
            // MN might have reappeared in same block with a new ProTx
            InitInternal(pindexNew);
        };

        if (!newMNList.IsMNValid(cur_protx_hash)) {
            // MN disappeared from MN list
            return reset(MASTERNODE_REMOVED);
        }

        auto oldDmn = oldMNList.GetMN(cur_protx_hash);
        auto newDmn = newMNList.GetMN(cur_protx_hash);
        if (newDmn->pdmnState->pubKeyOperator != oldDmn->pdmnState->pubKeyOperator) {
            // MN operator key changed or revoked
            return reset(MASTERNODE_OPERATOR_KEY_CHANGED);
        }
        if (newDmn->pdmnState->addr != oldDmn->pdmnState->addr) {
            // MN IP changed
            return reset(MASTERNODE_PROTX_IP_CHANGED);
        }
    } else {
        // MN might have (re)appeared with a new ProTx or we've found some peers
        // and figured out our local address
        Init(pindexNew);
    }
}

bool CActiveMasternodeManager::GetLocalAddress(CService& addrRet)
{
    AssertLockHeld(cs);
    // First try to find whatever our own local address is known internally.
    // Addresses could be specified via externalip or bind option, discovered via UPnP
    // or added by TorController. Use some random dummy IPv4 peer to prefer the one
    // reachable via IPv4.
    CNetAddr addrDummyPeer;
    bool fFoundLocal{false};
    if (LookupHost("8.8.8.8", addrDummyPeer, false)) {
        fFoundLocal = GetLocal(addrRet, &addrDummyPeer) && IsValidNetAddr(addrRet);
    }
    if (!fFoundLocal && !Params().RequireRoutableExternalIP()) {
        if (Lookup("127.0.0.1", addrRet, GetListenPort(), false)) {
            fFoundLocal = true;
        }
    }
    if (!fFoundLocal) {
        bool empty = true;
        // If we have some peers, let's try to find our local address from one of them
        auto service = m_info.service;
        m_connman.ForEachNodeContinueIf(CConnman::AllNodes, [&](CNode* pnode) {
            empty = false;
            if (pnode->addr.IsIPv4())
                fFoundLocal = GetLocal(service, &pnode->addr) && IsValidNetAddr(service);
            return !fFoundLocal;
        });
        // nothing and no live connections, can't do anything for now
        if (empty) {
            m_error = "Can't detect valid external address. Please consider using the externalip configuration option if problem persists. Make sure to use IPv4 address only.";
            LogPrintf("CActiveMasternodeManager::GetLocalAddress -- ERROR: %s\n", m_error);
            return false;
        }
    }
    return true;
}

bool CActiveMasternodeManager::IsValidNetAddr(const CService& addrIn)
{
    // TODO: regtest is fine with any addresses for now,
    // should probably be a bit smarter if one day we start to implement tests for this
    return !Params().RequireRoutableExternalIP() ||
           (addrIn.IsIPv4() && IsReachable(addrIn) && addrIn.IsRoutable());
}

template <template <typename> class EncryptedObj, typename Obj>
[[nodiscard]] bool CActiveMasternodeManager::Decrypt(const EncryptedObj<Obj>& obj, size_t idx, Obj& ret_obj,
                                                     int version) const
{
    AssertLockNotHeld(cs);
    return WITH_READ_LOCK(cs, return obj.Decrypt(idx, m_info.blsKeyOperator, ret_obj, version));
}
template bool CActiveMasternodeManager::Decrypt(const CBLSIESEncryptedObject<CBLSSecretKey>& obj, size_t idx,
                                                CBLSSecretKey& ret_obj, int version) const;
template bool CActiveMasternodeManager::Decrypt(const CBLSIESMultiRecipientObjects<CBLSSecretKey>& obj, size_t idx,
                                                CBLSSecretKey& ret_obj, int version) const;

[[nodiscard]] CBLSSignature CActiveMasternodeManager::Sign(const uint256& hash) const
{
    AssertLockNotHeld(cs);
    return WITH_READ_LOCK(cs, return m_info.blsKeyOperator.Sign(hash));
}

[[nodiscard]] CBLSSignature CActiveMasternodeManager::Sign(const uint256& hash, const bool is_legacy) const
{
    AssertLockNotHeld(cs);
    return WITH_READ_LOCK(cs, return m_info.blsKeyOperator.Sign(hash, is_legacy));
}

// We need to pass a copy as opposed to a const ref because CBLSPublicKeyVersionWrapper
// does not accept a const ref in its construction args
[[nodiscard]] CBLSPublicKey CActiveMasternodeManager::GetPubKey() const
{
    READ_LOCK(cs);
    return m_info.blsPubKeyOperator;
}
