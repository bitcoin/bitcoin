#include "vbk/p2p_sync.hpp"

#include "veriblock/entities/atv.hpp"
#include "veriblock/entities/vbkblock.hpp"
#include "veriblock/entities/vtb.hpp"

namespace VeriBlock {
namespace p2p {

std::map<NodeId, std::shared_ptr<PopDataNodeState>> mapPopDataNodeState;

template <typename PopDataType>
bool processGetPopData(CNode* node, CConnman* connman, CDataStream& vRecv, altintegration::MemPool& pop_mempool) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    std::vector<std::vector<uint8_t>> requested_data;
    vRecv >> requested_data;

    if (requested_data.size() > MAX_POP_DATA_SENDING_AMOUNT) {
        Misbehaving(node->GetId(), 20, strprintf("message getdata size() = %u", requested_data.size()));
        return false;
    }

    auto& known_set = getPopDataNodeState(node->GetId()).getSet<PopDataType>();

    const CNetMsgMaker msgMaker(PROTOCOL_VERSION);
    for (const auto& data_hash : requested_data) {
        const PopDataType* data = pop_mempool.get<PopDataType>(data_hash);
        known_set.insert(data_hash);
        if (data != nullptr) {
            connman->PushMessage(node, msgMaker.Make(PopDataType::name(), *data));
        }
    }

    return true;
}

template <typename PopDataType>
bool processOfferPopData(CNode* node, CConnman* connman, CDataStream& vRecv, altintegration::MemPool& pop_mempool) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    LogPrint(BCLog::NET, "received offered pop data: %s, bytes size: %d\n", PopDataType::name(), vRecv.size());
    std::vector<std::vector<uint8_t>> offered_data;
    vRecv >> offered_data;

    if (offered_data.size() > MAX_POP_DATA_SENDING_AMOUNT) {
        Misbehaving(node->GetId(), 20, strprintf("message getdata size() = %u", offered_data.size()));
        return false;
    }

    auto& known_set = getPopDataNodeState(node->GetId()).getSet<PopDataType>();

    std::vector<std::vector<uint8_t>> requested_data;
    const CNetMsgMaker msgMaker(PROTOCOL_VERSION);
    for (const auto& data_hash : offered_data) {
        if (!pop_mempool.get<PopDataType>(data_hash)) {
            requested_data.push_back(data_hash);
            known_set.insert(data_hash);
        }
    }

    if (!requested_data.empty()) {
        connman->PushMessage(node, msgMaker.Make(get_prefix + PopDataType::name(), requested_data));
    }

    return true;
}

template <typename PopDataType>
bool processPopData(CNode* node, CDataStream& vRecv, altintegration::MemPool& pop_mempool) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);
    LogPrint(BCLog::NET, "received pop data: %s, bytes size: %d", PopDataType::name(), vRecv.size());
    PopDataType data;
    vRecv >> data;

    altintegration::ValidationState state;
    if (!pop_mempool.submit(data, state)) {
        LogPrint(BCLog::NET, "peer %d sent statelessly invalid pop data: %s", node->GetId(), state.GetPath());
        Misbehaving(node->GetId(), 20, strprintf("invalid pop data getdata, reason: %s", state.GetPath()));
        return false;
    }

    return true;
}

bool processPopData(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, CConnman* connman)
{
    auto& pop_mempool = VeriBlock::getService<VeriBlock::PopService>().getMemPool();
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

    return true;
}


} // namespace p2p

} // namespace VeriBlock