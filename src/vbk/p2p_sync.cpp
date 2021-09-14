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
bool processGetPopData(CNode* node, CConnman* connman, CDataStream& vRecv, altintegration::MemPool& pop_mempool)
{
    std::vector<std::vector<uint8_t>> requested_data;
    vRecv >> requested_data;

    if (requested_data.size() > MAX_POP_DATA_SENDING_AMOUNT) {
        LOCK(cs_main);
        LogPrint(BCLog::NET, "peer %d send oversized message getdata size() = %u \n", node->GetId(), requested_data.size());
        Misbehaving(node->GetId(), 20, strprintf("message getdata size() = %u", requested_data.size()));
        return false;
    }
    
    LOCK(cs_main);
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
bool processOfferPopData(CNode* node, CConnman* connman, CDataStream& vRecv, altintegration::MemPool& pop_mempool)
{
    LogPrint(BCLog::NET, "received offered pop data: %s, bytes size: %d\n", pop_t::name(), vRecv.size());

    std::vector<std::vector<uint8_t>> offered_data;
    vRecv >> offered_data;

    if (offered_data.size() > MAX_POP_DATA_SENDING_AMOUNT) {
        LOCK(cs_main);
        LogPrint(BCLog::NET, "peer %d sent oversized message getdata size() = %u \n", node->GetId(), offered_data.size());
        Misbehaving(node->GetId(), 20, strprintf("message getdata size() = %u", offered_data.size()));
        return false;
    }

    LOCK(cs_main);
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
bool processPopData(CNode* node, CDataStream& vRecv, altintegration::MemPool& pop_mempool)
{
    LogPrint(BCLog::NET, "received pop data: %s, bytes size: %d\n", pop_t::name(), vRecv.size());
    pop_t data;
    vRecv >> data;

    altintegration::ValidationState state;
    if (!VeriBlock::GetPop().check(data, state)) {
        LogPrint(BCLog::NET, "peer %d sent statelessly invalid pop data: %s\n", node->GetId(), state.toString());
        LOCK(cs_main);
        Misbehaving(node->GetId(), 20, strprintf("statelessly invalid pop data getdata, reason: %s", state.toString()));
        return false;
    }

    LOCK(cs_main);
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
        return processPopData<altintegration::ATV>(pfrom, vRecv, pop_mempool);
    }

    if (strCommand == altintegration::VTB::name()) {
        return processPopData<altintegration::VTB>(pfrom, vRecv, pop_mempool);
    }

    if (strCommand == altintegration::VbkBlock::name()) {
        return processPopData<altintegration::VbkBlock>(pfrom, vRecv, pop_mempool);
    }
    //----------------------

    // offer Pop Data
    static std::string ofATV = offer_prefix + altintegration::ATV::name();
    if (strCommand == ofATV) {
        return processOfferPopData<altintegration::ATV>(pfrom, connman, vRecv, pop_mempool);
    }

    static std::string ofVTB = offer_prefix + altintegration::VTB::name();
    if (strCommand == ofVTB) {
        return processOfferPopData<altintegration::VTB>(pfrom, connman, vRecv, pop_mempool);
    }

    static std::string ofVBK = offer_prefix + altintegration::VbkBlock::name();
    if (strCommand == ofVBK) {
        return processOfferPopData<altintegration::VbkBlock>(pfrom, connman, vRecv, pop_mempool);
    }
    //-----------------

    // get Pop Data
    static std::string getATV = get_prefix + altintegration::ATV::name();
    if (strCommand == getATV) {
        return processGetPopData<altintegration::ATV>(pfrom, connman, vRecv, pop_mempool);
    }

    static std::string getVTB = get_prefix + altintegration::VTB::name();
    if (strCommand == getVTB) {
        return processGetPopData<altintegration::VTB>(pfrom, connman, vRecv, pop_mempool);
    }

    static std::string getVBK = get_prefix + altintegration::VbkBlock::name();
    if (strCommand == getVBK) {
        return processGetPopData<altintegration::VbkBlock>(pfrom, connman, vRecv, pop_mempool);
    }

    return -1;
}


} // namespace p2p

} // namespace VeriBlock
