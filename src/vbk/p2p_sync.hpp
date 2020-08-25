// Copyright (c) 2019-2020 Xenios SEZC
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
#include <veriblock/mempool.hpp>

namespace VeriBlock {

namespace p2p {

struct PopP2PState {
    uint32_t known_pop_data{0};
    uint32_t offered_pop_data{0};
    uint32_t requested_pop_data{0};
};

// The state of the Node that stores already known Pop Data
struct PopDataNodeState {
    // we use map to store DDoS prevention counter as a value in the map
    std::map<altintegration::ATV::id_t, PopP2PState> atv_state{};
    std::map<altintegration::VTB::id_t, PopP2PState> vtb_state{};
    std::map<altintegration::VbkBlock::id_t, PopP2PState> vbk_blocks_state{};

    template <typename T>
    std::map<typename T::id_t, PopP2PState>& getMap();
};

PopDataNodeState& getPopDataNodeState(const NodeId& id);

void erasePopDataNodeState(const NodeId& id);

} // namespace p2p

} // namespace VeriBlock


namespace VeriBlock {

namespace p2p {

const static std::string get_prefix = "g";
const static std::string offer_prefix = "of";

const static uint32_t MAX_POP_DATA_SENDING_AMOUNT = MAX_INV_SZ;
const static uint32_t MAX_POP_MESSAGE_SENDING_COUNT = 30;

template <typename pop_t>
void offerPopDataToAllNodes(const pop_t& p)
{
    std::vector<std::vector<uint8_t>> p_id = {p.getId().asVector()};
    CConnman* connman = g_rpc_node->connman.get();
    const CNetMsgMaker msgMaker(PROTOCOL_VERSION);

    connman->ForEachNode([&connman, &msgMaker, &p_id](CNode* node) {
        LOCK(cs_main);

        auto& pop_state_map = getPopDataNodeState(node->GetId()).getMap<pop_t>();
        PopP2PState& pop_state = pop_state_map[p_id[0]];
        if (pop_state.offered_pop_data == 0) {
            ++pop_state.offered_pop_data;
            connman->PushMessage(node, msgMaker.Make(offer_prefix + pop_t::name(), p_id));
        }
    });
}


template <typename PopDataType>
void offerPopData(CNode* node, CConnman* connman, const CNetMsgMaker& msgMaker) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    auto& pop_mempool = *VeriBlock::GetPop().mempool;
    const auto& data = pop_mempool.getMap<PopDataType>();

    auto& pop_state_map = getPopDataNodeState(node->GetId()).getMap<PopDataType>();

    std::vector<std::vector<uint8_t>>
        hashes;
    for (const auto& el : data) {
        PopP2PState& pop_state = pop_state_map[el.first];
        if (pop_state.offered_pop_data == 0 && pop_state.known_pop_data == 0) {
            ++pop_state.offered_pop_data;
            hashes.push_back(el.first.asVector());
        }

        if (hashes.size() == MAX_POP_DATA_SENDING_AMOUNT) {
            connman->PushMessage(node, msgMaker.Make(offer_prefix + PopDataType::name(), hashes));
            hashes.clear();
        }
    }

    if (!hashes.empty()) {
        connman->PushMessage(node, msgMaker.Make(offer_prefix + PopDataType::name(), hashes));
    }
}

int processPopData(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman* connman);

} // namespace p2p
} // namespace VeriBlock


#endif