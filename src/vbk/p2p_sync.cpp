// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "vbk/p2p_sync.hpp"
#include "validation.h"
#include <veriblock/pop.hpp>


namespace VeriBlock {
namespace p2p {

static std::map<NodeId, std::shared_ptr<PopDataNodeState>> mapPopDataNodeState;

template <>
std::map<altintegration::ATV::id_t, PopP2PState>& PopDataNodeState::getMap<altintegration::ATV>()
{
    return atv_state;
}

template <>
std::map<altintegration::VTB::id_t, PopP2PState>& PopDataNodeState::getMap<altintegration::VTB>()
{
    return vtb_state;
}

template <>
std::map<altintegration::VbkBlock::id_t, PopP2PState>& PopDataNodeState::getMap<altintegration::VbkBlock>()
{
    return vbk_blocks_state;
}

PopDataNodeState& getPopDataNodeState(const NodeId& id) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    std::shared_ptr<PopDataNodeState>& val = mapPopDataNodeState[id];
    if (val == nullptr) {
        mapPopDataNodeState[id] = std::make_shared<PopDataNodeState>();
        val = mapPopDataNodeState[id];
    }
    return *val;
}

void erasePopDataNodeState(const NodeId& id) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    mapPopDataNodeState.erase(id);
}

template <typename pop_t>
bool processGetPopData(CNode* node, CConnman* connman, CDataStream& vRecv, altintegration::MemPool& pop_mempool) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    std::vector<std::vector<uint8_t>> requested_data;
    vRecv >> requested_data;

    if (requested_data.size() > MAX_POP_DATA_SENDING_AMOUNT) {
        LogPrint(BCLog::NET, "peer %d send oversized message getdata size() = %u \n", node->GetId(), requested_data.size());
        Misbehaving(node->GetId(), 20, strprintf("message getdata size() = %u", requested_data.size()));
        return false;
    }

    auto& pop_state_map = getPopDataNodeState(node->GetId()).getMap<pop_t>();

    const CNetMsgMaker msgMaker(PROTOCOL_VERSION);
    for (const auto& data_hash : requested_data) {
        PopP2PState& pop_state = pop_state_map[data_hash];
        uint32_t ddosPreventionCounter = pop_state.known_pop_data++;

        if (ddosPreventionCounter > MAX_POP_MESSAGE_SENDING_COUNT) {
            LogPrint(BCLog::NET, "peer %d is spamming pop data %s \n", node->GetId(), pop_t::name());
            Misbehaving(node->GetId(), 20, strprintf("peer %d is spamming pop data %s", node->GetId(), pop_t::name()));
            return false;
        }

        const auto* data = pop_mempool.get<pop_t>(data_hash);
        if (data != nullptr) {
            connman->PushMessage(node, msgMaker.Make(pop_t::name(), *data));
        }
    }

    return true;
}

template <typename pop_t>
bool processOfferPopData(CNode* node, CConnman* connman, CDataStream& vRecv, altintegration::MemPool& pop_mempool) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    LogPrint(BCLog::NET, "received offered pop data: %s, bytes size: %d\n", pop_t::name(), vRecv.size());

    // do not process 'offers' during initial block download
    if (::ChainstateActive().IsInitialBlockDownload()) {
        // TODO: may want to keep a list of offered payloads, then filter all existing (on-chain) payloadsm and 'GET' others
        return true;
    }

    std::vector<std::vector<uint8_t>> offered_data;
    vRecv >> offered_data;

    if (offered_data.size() > MAX_POP_DATA_SENDING_AMOUNT) {
        LogPrint(BCLog::NET, "peer %d sent oversized message getdata size() = %u \n", node->GetId(), offered_data.size());
        Misbehaving(node->GetId(), 20, strprintf("message getdata size() = %u", offered_data.size()));
        return false;
    }

    auto& pop_state_map = getPopDataNodeState(node->GetId()).getMap<pop_t>();

    std::vector<std::vector<uint8_t>> requested_data;
    const CNetMsgMaker msgMaker(PROTOCOL_VERSION);
    for (const auto& data_hash : offered_data) {
        PopP2PState& pop_state = pop_state_map[data_hash];
        uint32_t ddosPreventionCounter = pop_state.requested_pop_data++;

        if (!pop_mempool.get<pop_t>(data_hash)) {
            requested_data.push_back(data_hash);
        } else if (ddosPreventionCounter > MAX_POP_MESSAGE_SENDING_COUNT) {
            LogPrint(BCLog::NET, "peer %d is spamming pop data %s \n", node->GetId(), pop_t::name());
            Misbehaving(node->GetId(), 20, strprintf("peer %d is spamming pop data %s", node->GetId(), pop_t::name()));
            return false;
        }
    }

    if (!requested_data.empty()) {
        connman->PushMessage(node, msgMaker.Make(get_prefix + pop_t::name(), requested_data));
    }

    return true;
}

template <typename pop_t>
bool processPopData(CNode* node, CDataStream& vRecv, altintegration::MemPool& pop_mempool) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    LogPrint(BCLog::NET, "received pop data: %s, bytes size: %d\n", pop_t::name(), vRecv.size());
    pop_t data;
    vRecv >> data;

    auto& pop_state_map = getPopDataNodeState(node->GetId()).getMap<pop_t>();
    PopP2PState& pop_state = pop_state_map[data.getId()];

    if (pop_state.requested_pop_data == 0) {
        LogPrint(BCLog::NET, "peer %d send pop data %s that has not been requested \n", node->GetId(), pop_t::name());
        Misbehaving(node->GetId(), 20, strprintf("peer %d send pop data %s that has not been requested", node->GetId(), pop_t::name()));
        return false;
    }

    uint32_t ddosPreventionCounter = pop_state.requested_pop_data++;

    if (ddosPreventionCounter > MAX_POP_MESSAGE_SENDING_COUNT) {
        LogPrint(BCLog::NET, "peer %d is spaming pop data %s\n", node->GetId(), pop_t::name());
        Misbehaving(node->GetId(), 20, strprintf("peer %d is spamming pop data %s", node->GetId(), pop_t::name()));
        return false;
    }

    altintegration::ValidationState state;
    auto result = pop_mempool.submit(data, state);
    if (!result && result.status == altintegration::MemPool::FAILED_STATELESS) {
        LogPrint(BCLog::NET, "peer %d sent statelessly invalid pop data: %s\n", node->GetId(), state.toString());
        Misbehaving(node->GetId(), 20, strprintf("statelessly invalid pop data getdata, reason: %s", state.toString()));
        return false;
    }

    return true;
}

int processPopData(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman* connman)
{
    auto& pop_mempool = VeriBlock::GetPop().getMemPool();

    // process Pop Data
    if (strCommand == altintegration::ATV::name()) {
        LOCK(cs_main);
        return processPopData<altintegration::ATV>(pfrom, vRecv, pop_mempool);
    }

    if (strCommand == altintegration::VTB::name()) {
        LOCK(cs_main);
        return processPopData<altintegration::VTB>(pfrom, vRecv, pop_mempool);
    }

    if (strCommand == altintegration::VbkBlock::name()) {
        LOCK(cs_main);
        return processPopData<altintegration::VbkBlock>(pfrom, vRecv, pop_mempool);
    }
    //----------------------

    // offer Pop Data
    if (strCommand == offer_prefix + altintegration::ATV::name()) {
        LOCK(cs_main);
        return processOfferPopData<altintegration::ATV>(pfrom, connman, vRecv, pop_mempool);
    }

    if (strCommand == offer_prefix + altintegration::VTB::name()) {
        LOCK(cs_main);
        return processOfferPopData<altintegration::VTB>(pfrom, connman, vRecv, pop_mempool);
    }

    if (strCommand == offer_prefix + altintegration::VbkBlock::name()) {
        LOCK(cs_main);
        return processOfferPopData<altintegration::VbkBlock>(pfrom, connman, vRecv, pop_mempool);
    }
    //-----------------

    // get Pop Data
    if (strCommand == get_prefix + altintegration::ATV::name()) {
        LOCK(cs_main);
        return processGetPopData<altintegration::ATV>(pfrom, connman, vRecv, pop_mempool);
    }

    if (strCommand == get_prefix + altintegration::VTB::name()) {
        LOCK(cs_main);
        return processGetPopData<altintegration::VTB>(pfrom, connman, vRecv, pop_mempool);
    }

    if (strCommand == get_prefix + altintegration::VbkBlock::name()) {
        LOCK(cs_main);
        return processGetPopData<altintegration::VbkBlock>(pfrom, connman, vRecv, pop_mempool);
    }

    return -1;
}


} // namespace p2p

} // namespace VeriBlock