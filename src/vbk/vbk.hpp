// Copyright (c) 2019-2021 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SRC_VBK_VBK_HPP
#define BITCOIN_SRC_VBK_VBK_HPP

#include <uint256.h>

namespace VeriBlock {

const static int32_t POP_BLOCK_VERSION_BIT = 0x80000UL;

// We want to set a limit of ATVs per VBK block by setting 2 last bytes 
// of altchain ID with specific value.
// VBK block time is 30 sec, BTCSQ block time is 2 min. 
// BTCSQ maximum can contain 100 ATVs per block, so we set this limit 
// to be around 25. We set it to be 20.
// 0x26ff are bytes that control this behavior:
// 0x26 is 0b100110 in bits, which translates into 
// base = 0b10011 = 19
// exponent = 1
// so, max ATVs per VBK block would be pow(19+1, 1).
const static int64_t ALT_CHAIN_ID = 0x3ae6ca000026ff;

}  // namespace VeriBlock

#endif //BITCOIN_SRC_VBK_VBK_HPP
