// Copyright (c) 2019-2021 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SRC_VBK_P2P_SYNC_HPP
#define BITCOIN_SRC_VBK_P2P_SYNC_HPP

#include <chainparams.h>
#include <map>
#include <net_processing.h>
#include <netmessagemaker.h>
#include <node/context.h>
#include <rpc/blockchain.h>
#include <vbk/pop_common.hpp>
#include <vbk/util.hpp>
#include <veriblock/pop.hpp>

namespace VeriBlock {

namespace p2p {

void SendPopPayload(
    CNode* pto,
    CConnman* connman,
    int typeIn,
    CRollingBloomFilter& filterInventoryKnown,
    std::set<uint256>& toSend,
    std::vector<CInv>& vInv) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

void ProcessGetPopPayloads(
    std::deque<CInv>::iterator& it,
    CNode* pfrom,
    CConnman* connman,
    const std::atomic<bool>& interruptMsgProc,
    std::vector<CInv>& vNotFound

    ) EXCLUSIVE_LOCKS_REQUIRED(cs_main);

// clang-format off
template <typename T> static int GetType();
template <> inline int GetType<altintegration::ATV>(){ return MSG_POP_ATV; }
template <> inline int GetType<altintegration::VTB>(){ return MSG_POP_VTB; }
template <> inline int GetType<altintegration::VbkBlock>(){ return MSG_POP_VBK; }
// clang-format on

template <typename T>
CInv PayloadToInv(const typename T::id_t& id) {
    return CInv(GetType<T>(), IdToUint256<T>(id));
}

template <typename T>
void RelayPopPayload(
    CConnman* connman,
    const T& t)
{
    auto inv = PayloadToInv<T>(t.getId());
    connman->ForEachNode([&inv](CNode* pto) {
        pto->PushInventory(inv);
    });
}

template <typename T, typename F>
bool ProcessPopPayload(CNode* pfrom, CConnman* connman, CDataStream& vRecv, F onInv) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    if ((!g_relay_txes && !pfrom->HasPermission(PF_RELAY)) || (pfrom->m_tx_relay == nullptr)) {
        LogPrint(BCLog::NET, "%s sent in violation of protocol peer=%d\n", T::name(), pfrom->GetId());
        pfrom->fDisconnect = true;
        return true;
    }

    T data;
    vRecv >> data;

    LogPrint(BCLog::NET, "received %s from peer %d\n", data.toShortPrettyString(), pfrom->GetId());

    uint256 id = IdToUint256<T>(data.getId());
    CInv inv(GetType<T>(), id);
    pfrom->AddInventoryKnown(inv);

    // CNodeState is defined inside net_processing.cpp.
    // we use that structure in this function onInv().
    onInv(inv);

    auto& mp = VeriBlock::GetPop().getMemPool();
    altintegration::ValidationState state;
    auto result = mp.submit(data, state);
    if (result.isAccepted()) {
        // relay this POP payload to other peers
        RelayPopPayload(connman, data);
    } else {
        assert(result.isFailedStateless());
        // peer sent us statelessly invalid payload.
        Misbehaving(pfrom->GetId(), 1000, strprintf("peer %d sent us statelessly invalid %s, reason: %s", pfrom->GetId(), T::name(), state.toString()));
        return false;
    }

    return true;
}

template <typename T>
void RelayPopMempool(CNode* pto) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    auto& mp = VeriBlock::GetPop().getMemPool();

    size_t counter = 0;
    for (const auto& p : mp.getMap<T>()) {
        T& t = *p.second;
        auto inv = PayloadToInv<T>(t.getId());
        pto->PushInventory(inv);
        counter++;
    }

    for (const auto& p : mp.template getInFlightMap<T>()) {
        T& t = *p.second;
        auto inv = PayloadToInv<T>(t.getId());
        pto->PushInventory(inv);
        counter++;
    }

    LogPrint(BCLog::NET, "relay %s=%u from POP mempool to peer=%d\n", T::name(), counter, pto->GetId());
}

} // namespace p2p
} // namespace VeriBlock


#endif