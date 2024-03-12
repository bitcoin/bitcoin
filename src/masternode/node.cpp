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
#include <validation.h>
#include <warnings.h>

// Keep track of the active Masternode
std::unique_ptr<CActiveMasternodeManager> activeMasternodeManager;

CActiveMasternodeManager::~CActiveMasternodeManager()
{
    // Make sure to clean up BLS keys before global destructors are called
    // (they have been allocated from the secure memory pool)
    {
        LOCK(cs);
        m_info.blsKeyOperator.reset();
        m_info.blsPubKeyOperator.reset();
    }
}

std::string CActiveMasternodeManager::GetStateString() const
{
    switch (state) {
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
    switch (state) {
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
        return "Error. " + strError;
    default:
        return "Unknown";
    }
}

void CActiveMasternodeManager::Init(const CBlockIndex* pindex)
{
    LOCK2(::cs_main, cs);

    if (!fMasternodeMode) return;

    if (!DeploymentDIP0003Enforced(pindex->nHeight, Params().GetConsensus())) return;

    // Check that our local network configuration is correct
    if (!fListen && Params().RequireRoutableExternalIP()) {
        // listen option is probably overwritten by something else, no good
        state = MASTERNODE_ERROR;
        strError = "Masternode must accept connections from outside. Make sure listen configuration option is not overwritten by some another parameter.";
        LogPrintf("CActiveMasternodeManager::Init -- ERROR: %s\n", strError);
        return;
    }

    if (!GetLocalAddress(m_info.service)) {
        state = MASTERNODE_ERROR;
        return;
    }

    CDeterministicMNList mnList = Assert(m_dmnman)->GetListForBlock(pindex);

    CDeterministicMNCPtr dmn = mnList.GetMNByOperatorKey(*m_info.blsPubKeyOperator);
    if (!dmn) {
        // MN not appeared on the chain yet
        return;
    }

    if (!mnList.IsMNValid(dmn->proTxHash)) {
        if (mnList.IsMNPoSeBanned(dmn->proTxHash)) {
            state = MASTERNODE_POSE_BANNED;
        } else {
            state = MASTERNODE_REMOVED;
        }
        return;
    }

    LogPrintf("CActiveMasternodeManager::Init -- proTxHash=%s, proTx=%s\n", dmn->proTxHash.ToString(), dmn->ToString());

    if (m_info.service != dmn->pdmnState->addr) {
        state = MASTERNODE_ERROR;
        strError = "Local address does not match the address from ProTx";
        LogPrintf("CActiveMasternodeManager::Init -- ERROR: %s\n", strError);
        return;
    }

    // Check socket connectivity
    LogPrintf("CActiveMasternodeManager::Init -- Checking inbound connection to '%s'\n", m_info.service.ToString());
    std::unique_ptr<Sock> sock = CreateSock(m_info.service);
    if (!sock) {
        state = MASTERNODE_ERROR;
        strError = "Could not create socket to connect to " + m_info.service.ToString();
        LogPrintf("CActiveMasternodeManager::Init -- ERROR: %s\n", strError);
        return;
    }
    bool fConnected = ConnectSocketDirectly(m_info.service, *sock, nConnectTimeout, true) && IsSelectableSocket(sock->Get());
    sock->Reset();

    if (!fConnected && Params().RequireRoutableExternalIP()) {
        state = MASTERNODE_ERROR;
        strError = "Could not connect to " + m_info.service.ToString();
        LogPrintf("CActiveMasternodeManager::Init -- ERROR: %s\n", strError);
        return;
    }

    m_info.proTxHash = dmn->proTxHash;
    m_info.outpoint = dmn->collateralOutpoint;
    m_info.legacy = dmn->pdmnState->nVersion == CProRegTx::LEGACY_BLS_VERSION;
    state = MASTERNODE_READY;
}

void CActiveMasternodeManager::InitKeys(const CBLSSecretKey& sk)
{
    AssertLockNotHeld(cs);

    LOCK(cs);
    assert(m_info.blsKeyOperator == nullptr);
    assert(m_info.blsPubKeyOperator == nullptr);
    m_info.blsKeyOperator = std::make_unique<CBLSSecretKey>(sk);
    m_info.blsPubKeyOperator = std::make_unique<CBLSPublicKey>(sk.GetPublicKey());
    // We don't know the actual scheme at this point, print both
    LogPrintf("MASTERNODE:\n  blsPubKeyOperator legacy: %s\n  blsPubKeyOperator basic: %s\n",
            m_info.blsPubKeyOperator->ToString(/*specificLegacyScheme=*/ true),
            m_info.blsPubKeyOperator->ToString(/*specificLegacyScheme=*/ false));
}

void CActiveMasternodeManager::UpdatedBlockTip(const CBlockIndex* pindexNew, const CBlockIndex* pindexFork, bool fInitialDownload)
{
    LOCK2(::cs_main, cs);

    if (!fMasternodeMode) return;

    if (!DeploymentDIP0003Enforced(pindexNew->nHeight, Params().GetConsensus())) return;

    if (state == MASTERNODE_READY) {
        auto oldMNList = Assert(m_dmnman)->GetListForBlock(pindexNew->pprev);
        auto newMNList = m_dmnman->GetListForBlock(pindexNew);
        if (!newMNList.IsMNValid(m_info.proTxHash)) {
            // MN disappeared from MN list
            state = MASTERNODE_REMOVED;
            m_info.proTxHash = uint256();
            m_info.outpoint.SetNull();
            // MN might have reappeared in same block with a new ProTx
            Init(pindexNew);
            return;
        }

        auto oldDmn = oldMNList.GetMN(m_info.proTxHash);
        auto newDmn = newMNList.GetMN(m_info.proTxHash);
        if (newDmn->pdmnState->pubKeyOperator != oldDmn->pdmnState->pubKeyOperator) {
            // MN operator key changed or revoked
            state = MASTERNODE_OPERATOR_KEY_CHANGED;
            m_info.proTxHash = uint256();
            m_info.outpoint.SetNull();
            // MN might have reappeared in same block with a new ProTx
            Init(pindexNew);
            return;
        }

        if (newDmn->pdmnState->addr != oldDmn->pdmnState->addr) {
            // MN IP changed
            state = MASTERNODE_PROTX_IP_CHANGED;
            m_info.proTxHash = uint256();
            m_info.outpoint.SetNull();
            Init(pindexNew);
            return;
        }
    } else {
        // MN might have (re)appeared with a new ProTx or we've found some peers
        // and figured out our local address
        Init(pindexNew);
    }
}

bool CActiveMasternodeManager::GetLocalAddress(CService& addrRet)
{
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
        auto service = WITH_LOCK(cs, return m_info.service);
        connman.ForEachNodeContinueIf(CConnman::AllNodes, [&](CNode* pnode) {
            empty = false;
            if (pnode->addr.IsIPv4())
                fFoundLocal = GetLocal(service, &pnode->addr) && IsValidNetAddr(service);
            return !fFoundLocal;
        });
        // nothing and no live connections, can't do anything for now
        if (empty) {
            strError = "Can't detect valid external address. Please consider using the externalip configuration option if problem persists. Make sure to use IPv4 address only.";
            LogPrintf("CActiveMasternodeManager::GetLocalAddress -- ERROR: %s\n", strError);
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
    return WITH_LOCK(cs, return obj.Decrypt(idx, *Assert(m_info.blsKeyOperator), ret_obj, version));
}
template bool CActiveMasternodeManager::Decrypt(const CBLSIESEncryptedObject<CBLSSecretKey>& obj, size_t idx,
                                                CBLSSecretKey& ret_obj, int version) const;
template bool CActiveMasternodeManager::Decrypt(const CBLSIESMultiRecipientObjects<CBLSSecretKey>& obj, size_t idx,
                                                CBLSSecretKey& ret_obj, int version) const;

[[nodiscard]] CBLSSignature CActiveMasternodeManager::Sign(const uint256& hash) const
{
    AssertLockNotHeld(cs);
    return WITH_LOCK(cs, return Assert(m_info.blsKeyOperator)->Sign(hash));
}

[[nodiscard]] CBLSSignature CActiveMasternodeManager::Sign(const uint256& hash, const bool is_legacy) const
{
    AssertLockNotHeld(cs);
    return WITH_LOCK(cs, return Assert(m_info.blsKeyOperator)->Sign(hash, is_legacy));
}
