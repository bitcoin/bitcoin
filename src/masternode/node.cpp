// Copyright (c) 2014-2024 The Dash Core developers
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

namespace {
bool GetLocal(CService& addr, const CNetAddr* paddrPeer)
{
    if (!fListen)
        return false;

    int nBestScore = -1;
    {
        LOCK(g_maplocalhost_mutex);
        int nBestReachability = -1;
        for (const auto& entry : mapLocalHost)
        {
            // For privacy reasons, don't advertise our privacy-network address
            // to other networks and don't advertise our other-network address
            // to privacy networks.
            const Network our_net{entry.first.GetNetwork()};
            const Network peers_net{paddrPeer->GetNetwork()};
            if (our_net != peers_net &&
                (our_net == NET_ONION || our_net == NET_I2P ||
                 peers_net == NET_ONION || peers_net == NET_I2P)) {
                continue;
            }
            int nScore = entry.second.nScore;
            int nReachability = entry.first.GetReachabilityFrom(*paddrPeer);
            if (nReachability > nBestReachability || (nReachability == nBestReachability && nScore > nBestScore))
            {
                addr = CService(entry.first, entry.second.nPort);
                nBestReachability = nReachability;
                nBestScore = nScore;
            }
        }
    }
    return nBestScore >= 0;
}
} // anonymous namespace

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
    case MasternodeState::WAITING_FOR_PROTX:
        return "WAITING_FOR_PROTX";
    case MasternodeState::POSE_BANNED:
        return "POSE_BANNED";
    case MasternodeState::REMOVED:
        return "REMOVED";
    case MasternodeState::OPERATOR_KEY_CHANGED:
        return "OPERATOR_KEY_CHANGED";
    case MasternodeState::PROTX_IP_CHANGED:
        return "PROTX_IP_CHANGED";
    case MasternodeState::READY:
        return "READY";
    case MasternodeState::SOME_ERROR:
        return "ERROR";
    default:
        return "UNKNOWN";
    }
}

std::string CActiveMasternodeManager::GetStatus() const
{
    READ_LOCK(cs);
    switch (m_state) {
    case MasternodeState::WAITING_FOR_PROTX:
        return "Waiting for ProTx to appear on-chain";
    case MasternodeState::POSE_BANNED:
        return "Masternode was PoSe banned";
    case MasternodeState::REMOVED:
        return "Masternode removed from list";
    case MasternodeState::OPERATOR_KEY_CHANGED:
        return "Operator key changed or revoked";
    case MasternodeState::PROTX_IP_CHANGED:
        return "IP address specified in ProTx changed";
    case MasternodeState::READY:
        return "Ready";
    case MasternodeState::SOME_ERROR:
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
        m_state = MasternodeState::SOME_ERROR;
        m_error = "Masternode must accept connections from outside. Make sure listen configuration option is not overwritten by some another parameter.";
        LogPrintf("CActiveMasternodeManager::Init -- ERROR: %s\n", m_error);
        return;
    }

    if (!GetLocalAddress(m_info.service)) {
        m_state = MasternodeState::SOME_ERROR;
        return;
    }

    CDeterministicMNList mnList = Assert(m_dmnman)->GetListForBlock(pindex);

    auto dmn = mnList.GetMNByOperatorKey(m_info.blsPubKeyOperator);
    if (!dmn) {
        // MN not appeared on the chain yet
        return;
    }

    if (!mnList.IsMNValid(dmn->proTxHash)) {
        if (mnList.IsMNPoSeBanned(dmn->proTxHash)) {
            m_state = MasternodeState::POSE_BANNED;
        } else {
            m_state = MasternodeState::REMOVED;
        }
        return;
    }

    LogPrintf("CActiveMasternodeManager::Init -- proTxHash=%s, proTx=%s\n", dmn->proTxHash.ToString(), dmn->ToString());

    if (m_info.service != dmn->pdmnState->netInfo->GetPrimary()) {
        m_state = MasternodeState::SOME_ERROR;
        m_error = "Local address does not match the address from ProTx";
        LogPrintf("CActiveMasternodeManager::Init -- ERROR: %s\n", m_error);
        return;
    }

    // Check socket connectivity
    LogPrintf("CActiveMasternodeManager::Init -- Checking inbound connection to '%s'\n", m_info.service.ToStringAddrPort());
    std::unique_ptr<Sock> sock{ConnectDirectly(m_info.service, /*manual_connection=*/true)};
    bool fConnected{sock && sock->IsSelectable(/*is_select=*/::g_socket_events_mode == SocketEventsMode::Select)};
    sock = std::make_unique<Sock>(INVALID_SOCKET);
    if (!fConnected && Params().RequireRoutableExternalIP()) {
        m_state = MasternodeState::SOME_ERROR;
        m_error = "Could not connect to " + m_info.service.ToStringAddrPort();
        LogPrintf("CActiveMasternodeManager::Init -- ERROR: %s\n", m_error);
        return;
    }

    m_info.proTxHash = dmn->proTxHash;
    m_info.outpoint = dmn->collateralOutpoint;
    m_state = MasternodeState::READY;
}

void CActiveMasternodeManager::UpdatedBlockTip(const CBlockIndex* pindexNew, const CBlockIndex* pindexFork, bool fInitialDownload)
{
    if (!DeploymentDIP0003Enforced(pindexNew->nHeight, Params().GetConsensus())) return;

    const auto [cur_state, cur_protx_hash] = WITH_READ_LOCK(cs, return std::make_pair(m_state, m_info.proTxHash));
    if (cur_state == MasternodeState::READY) {
        auto oldMNList = Assert(m_dmnman)->GetListForBlock(pindexNew->pprev);
        auto newMNList = m_dmnman->GetListForBlock(pindexNew);
        auto reset = [this, pindexNew](MasternodeState state) -> void {
            LOCK(cs);
            m_state = state;
            m_info.proTxHash = uint256();
            m_info.outpoint.SetNull();
            // MN might have reappeared in same block with a new ProTx
            InitInternal(pindexNew);
        };

        if (!newMNList.IsMNValid(cur_protx_hash)) {
            // MN disappeared from MN list
            return reset(MasternodeState::REMOVED);
        }

        auto oldDmn = oldMNList.GetMN(cur_protx_hash);
        auto newDmn = newMNList.GetMN(cur_protx_hash);
        if (!oldDmn || !newDmn) {
            return reset(MasternodeState::SOME_ERROR);
        }
        if (newDmn->pdmnState->pubKeyOperator != oldDmn->pdmnState->pubKeyOperator) {
            // MN operator key changed or revoked
            return reset(MasternodeState::OPERATOR_KEY_CHANGED);
        }
        if (newDmn->pdmnState->netInfo->GetPrimary() != oldDmn->pdmnState->netInfo->GetPrimary()) {
            // MN IP changed
            return reset(MasternodeState::PROTX_IP_CHANGED);
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
    bool fFoundLocal{false};
    if (auto peerAddr = LookupHost("8.8.8.8", false); peerAddr.has_value()) {
        fFoundLocal = GetLocal(addrRet, &peerAddr.value()) && IsValidNetAddr(addrRet);
    }
    if (!fFoundLocal && !Params().RequireRoutableExternalIP()) {
        if (auto addr = Lookup("127.0.0.1", GetListenPort(), false); addr.has_value()) {
            addrRet = addr.value();
            fFoundLocal = true;
        }
    }
    if (!fFoundLocal) {
        bool empty = true;
        // If we have some peers, let's try to find our local address from one of them
        m_connman.ForEachNodeContinueIf(CConnman::AllNodes, [&](CNode* pnode) {
            empty = false;
            if (pnode->addr.IsIPv4()) {
                if (auto addr = ::GetLocalAddress(*pnode); IsValidNetAddr(addr)) {
                    addrRet = addr;
                    fFoundLocal = true;
                }
            }
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
    if (!addrIn.IsValid() || !addrIn.IsIPv4()) return false;
    // TODO: regtest is fine with any addresses for now,
    // should probably be a bit smarter if one day we start to implement tests for this
    return !Params().RequireRoutableExternalIP() || (g_reachable_nets.Contains(addrIn) && addrIn.IsRoutable());
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
