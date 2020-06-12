#include "vbk/p2p_sync.hpp"

#include <vbk/pop_service.hpp>

#include "veriblock/entities/atv.hpp"
#include "veriblock/entities/vbkblock.hpp"
#include "veriblock/entities/vtb.hpp"

#include "veriblock/mempool.hpp"

namespace VeriBlock {
namespace p2p {

bool processPopData(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, int64_t nTimeReceived, const CChainParams& chainparams, CConnman* connman, BanMan* banman, const std::atomic<bool>& interruptMsgProc)
{
    auto& pop_mempool = VeriBlock::getService<VeriBlock::PopService>().getMemPool();
    if (strCommand == altintegration::ATV::name()) {
        LOCK(cs_main);
        LogPrint(BCLog::NET, "received: ATV\n");
        altintegration::ATV atv;
        vRecv >> atv;

        altintegration::ValidationState state;
        if (!pop_mempool.submitATV({atv}, state)) {
            LogPrint(BCLog::NET, "VeriBlock-PoP: %s ", state.GetPath());
            return false;
        }

        return true;
    }

    if (strCommand == altintegration::VTB::name()) {
        LOCK(cs_main);
        LogPrint(BCLog::NET, "received: VTB\n");
        altintegration::VTB vtb;
        vRecv >> vtb;

        altintegration::ValidationState state;
        if (!pop_mempool.submitVTB({vtb}, state)) {
            LogPrint(BCLog::NET, "VeriBlock-PoP: %s ", state.GetPath());
            return false;
        }

        return true;
    }

    if (strCommand == altintegration::VbkBlock::name()) {
        LOCK(cs_main);
        LogPrint(BCLog::NET, "received: VbkBlock\n");
        altintegration::VbkBlock block;
        vRecv >> block;


        return true;
    }

    return true;
}


} // namespace p2p

} // namespace VeriBlock