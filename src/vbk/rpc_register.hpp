// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SRC_VBK_RPC_REGISTER_HPP
#define BITCOIN_SRC_VBK_RPC_REGISTER_HPP

class CRPCTable;

namespace VeriBlock {

void RegisterPOPMiningRPCCommands(CRPCTable& t);

} // namespace VeriBlock


#endif //BITCOIN_SRC_VBK_RPC_REGISTER_HPP
