#ifndef BITCOIN_SRC_VBK_CONFIG_HPP
#define BITCOIN_SRC_VBK_CONFIG_HPP

#include <cstdint>
#include <string>

#include "vbk/typed_wrapper.hpp"
#include <array>
#include <cstdint>
#include <uint256.h>

#ifndef VBK_NUM_KEYSTONES
/// number of keystones stored in ContextInfoContainer
// after change of this number, you need to re-generate genesis blocks (main, test, regtest)
#define VBK_NUM_KEYSTONES 2u
#endif

namespace VeriBlock {

using KeystoneArray = std::array<uint256, VBK_NUM_KEYSTONES>;

using AltchainId = TypedWrapper<int64_t, struct AltchainIdTag>;

struct Config {
    // unique index to this chain; network id across chains
    AltchainId index = AltchainId(0x3ae6ca);

    uint32_t max_pop_script_size = 15000; // TODO: figure out number
    uint32_t max_vtb_size = 13000; // TODO: figure out number
    uint32_t min_vtb_size = 1; // TODO: figure out number
    uint32_t max_atv_size = 13000; // TODO: figure out numer
    uint32_t min_atv_size = 1; // TODO: figure out number

    uint32_t max_future_block_time = 10 * 60; // 10 minutes

    uint32_t keystone_interval = 5;
    uint32_t keystone_finality_delay = 50;
    uint32_t amnesty_period = 20;

    /// The maximum allowed weight for the PoP transaction
    uint32_t max_pop_tx_weight = 50000;

    /// The maximun allowed number of PoP transaction in a block
    uint32_t max_pop_tx_amount = 50;

    /////// Pop Rewards section start
    uint32_t POP_REWARD_PERCENTAGE = 40;
    int32_t POP_REWARD_SETTLEMENT_INTERVAL = 400;
    int32_t POP_REWARD_PAYMENT_DELAY = 500;
    uint32_t POP_DIFFICULTY_AVERAGING_INTERVAL = 50;
    int64_t ROUND_1_CONSTANT = 9700000000;
    int64_t ROUND_2_CONSTANT = 10300000000;
    int64_t ROUND_3_CONSTANT = 10700000000;
    int64_t ROUND_4_CONSTANT = 30000000000;
    int64_t ROUND_1_SLOPE = -194000;
    int64_t ROUND_2_SLOPE = -206000;
    int64_t ROUND_3_SLOPE = -214000;
    int64_t ROUND_4_SLOPE = -639750;
    int64_t MAX_NORMAL_THRESHOLD = 2;
    int64_t MAX_KEYSTONE_THRESHOLD = 3;
    int64_t START_POINT_OF_KEYSTONE_DECREASING_LINE = 100;
    int64_t START_POINT_OF_NORMAL_DECREASING_LINE = 100;
    int64_t START_POINT_OF_KEYSTONE_DECREASING_LINE_RATIO = 1; // START_POINT_OF_KEYSTONE_DECREASING_LINE / 100
    int64_t START_POINT_OF_NORMAL_DECREASING_LINE_RATIO = 1;   // START_POINT_OF_NORMAL_DECREASING_LINE / 100
    int64_t MULTIPLIER_FOR_BLOCK_REWARD = 744;
    /////// Pop Rewards section end

    // GRPC config
    std::string service_port = "19012";
    std::string service_ip = "127.0.0.1";

};

} // namespace VeriBlock

#endif //BITCOIN_SRC_VBK_CONFIG_HPP
