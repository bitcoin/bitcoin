// Copyright (c) 2019-2021 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "vbk/p2p_sync.hpp"
#include "validation.h"
#include <net_processing.h>
#include <protocol.h>
#include <vbk/util.hpp>
#include <veriblock/pop.hpp>


namespace VeriBlock {
namespace p2p {

template <typename T>
static void DoSubmitPopPayload(
    CNode* pfrom,
    CConnman* connman,
    const CNetMsgMaker& msgMaker,
    altintegration::MemPool& mp,
    const CInv& inv,
    std::vector<CInv>& vNotFound)
{
    const auto id = VeriBlock::Uint256ToId<T>(inv.hash);
    auto* atv = mp.get<T>(id);
    if (atv == nullptr) {
        vNotFound.push_back(inv);
    } else {
        LogPrint(BCLog::NET, "sending %s to peer %d\n", atv->toShortPrettyString(), pfrom->GetId());
        connman->PushMessage(pfrom, msgMaker.Make(T::name(), *atv));
    }
}

void ProcessGetPopPayloads(
    std::deque<CInv>::iterator& it,
    CNode* pfrom,
    CConnman* connman,
    const std::atomic<bool>& interruptMsgProc,
    std::vector<CInv>& vNotFound) EXCLUSIVE_LOCKS_REQUIRED(cs_main) // for pop mempool
{
    AssertLockHeld(cs_main);
    const CNetMsgMaker msgMaker(pfrom->GetSendVersion());
    auto& mp = VeriBlock::GetPop().getMemPool();
    while (it != pfrom->vRecvGetData.end() && (it->type == MSG_POP_ATV || it->type == MSG_POP_VTB || it->type == MSG_POP_VBK)) {
        if (interruptMsgProc) {
            return;
        }

        // Don't bother if send buffer is too full to respond anyway
        if (pfrom->fPauseSend) {
            break;
        }

        const CInv& inv = *it;
        ++it;

        if (inv.type == MSG_POP_ATV) {
            DoSubmitPopPayload<altintegration::ATV>(pfrom, connman, msgMaker, mp, inv, vNotFound);
        } else if (inv.type == MSG_POP_VTB) {
            DoSubmitPopPayload<altintegration::VTB>(pfrom, connman, msgMaker, mp, inv, vNotFound);
        } else if (inv.type == MSG_POP_VBK) {
            DoSubmitPopPayload<altintegration::VbkBlock>(pfrom, connman, msgMaker, mp, inv, vNotFound);
        }
    }
}

void SendPopPayload(
    CNode* pto,
    CConnman* connman,
    int typeIn,
    CRollingBloomFilter& filterInventoryKnown,
    std::set<uint256>& toSend,
    std::vector<CInv>& vInv) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    const CNetMsgMaker msgMaker(pto->GetSendVersion());

    for (const auto& hash : toSend) {
        CInv inv(typeIn, hash);
        if (filterInventoryKnown.contains(hash)) {
            LogPrint(BCLog::NET, "inv %s is known by peer %d (bloom filtered)\n", inv.ToString(), pto->GetId());
            continue;
        }

        vInv.push_back(inv);
        filterInventoryKnown.insert(hash);

        if (vInv.size() == MAX_INV_SZ) {
            connman->PushMessage(pto, msgMaker.Make(NetMsgType::INV, vInv));
            vInv.clear();
        }
    }

    toSend.clear();
}


} // namespace p2p

} // namespace VeriBlock
