// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SRC_VBK_P2P_SYNC_HPP
#define BITCOIN_SRC_VBK_P2P_SYNC_HPP

#include <chainparams.h>
#include <net_processing.h>
#include <netmessagemaker.h>

namespace VeriBlock {

namespace p2p {

template <typename PopDataType>
void sendPopData(CConnman* connman, const CNetMsgMaker& msgMaker, const std::vector<PopDataType>& data)
{
    AssertLockHeld(cs_main);
    LogPrint(BCLog::NET, "send PopData: count %d\n", data.size());
    LogPrintf("send PopData: count %d\n", data.size());
    connman->ForEachNode([&connman, &msgMaker, &data](CNode* pnode) {
        for (const auto& el : data) {
            connman->PushMessage(pnode, msgMaker.Make(PopDataType::name(), el));
        }
    });
}

bool processPopData(CNode* pfrom, const std::string& strCommand, CDataStream& vRecv, int64_t nTimeReceived, const CChainParams& chainparams, CConnman* connman, BanMan* banman, const std::atomic<bool>& interruptMsgProc);

} // namespace p2p
} // namespace VeriBlock


#endif