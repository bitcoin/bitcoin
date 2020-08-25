// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SRC_VBK_GENESIS_HPP
#define BITCOIN_SRC_VBK_GENESIS_HPP

#include <primitives/block.h>
#include <vbk/genesis_common.hpp>

namespace VeriBlock {

CBlock MineGenesisBlock(
    uint32_t nTime,
    const std::string& pszTimestamp,
    const std::string& initialPubkeyHex,
    uint32_t nBits,
    uint32_t nVersion = 1,
    uint32_t nNonce = 0, // starting nonce
    uint64_t genesisReward = 50 * COIN);

} // namespace VeriBlock

#endif //BITCOIN_SRC_VBK_GENESIS_HPP
