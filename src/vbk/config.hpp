// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SRC_VBK_CONFIG_HPP
#define BITCOIN_SRC_VBK_CONFIG_HPP

#include <cstdint>
#include <string>
#include <vector>

#include <array>
#include <cstdint>
#include <uint256.h>
#include <veriblock/config.hpp>

namespace VeriBlock {

using KeystoneArray = std::array<uint256, 2>;


struct Config {
    // unique index to this chain; network id across chains
    uint32_t index = 0x3ae6ca;

    uint32_t btc_header_size = 80;
    uint32_t vbk_header_size = 64;
    uint32_t max_vtb_size = 100000;        // TODO: figure out number
    uint32_t min_vtb_size = 1;             // TODO: figure out number
    uint32_t max_atv_size = 100000;        // TODO: figure out numer
    uint32_t min_atv_size = 1;             // TODO: figure out number

    uint32_t max_future_block_time = 10 * 60; // 10 minutes

    /// The maximum allowed weight for the PoP transaction
//    uint32_t max_pop_tx_weight = 150000;

    /// The maximum allowed number of PoP transaction in a block
    uint32_t max_pop_tx_amount = 50;

    altintegration::Config popconfig;

    /////// Pop Rewards section start
    uint32_t POP_REWARD_PERCENTAGE = 40;
    int32_t POP_REWARD_COEFFICIENT = 20;
};

} // namespace VeriBlock

#endif //BITCOIN_SRC_VBK_CONFIG_HPP
