// Copyright (c) 2019-2020 Xenios SEZC
// https://www.veriblock.org
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef INTEGRATION_REFERENCE_BTC_VBTC_PARAMS_HPP
#define INTEGRATION_REFERENCE_BTC_VBTC_PARAMS_HPP

#include <primitives/block.h>
#include <veriblock/pop.hpp>


class ArgsManager;

namespace VeriBlock {

struct AltChainParamsVBTC : public altintegration::AltChainParams {
    ~AltChainParamsVBTC() override = default;
    AltChainParamsVBTC() = default;

    AltChainParamsVBTC(const CBlock& genesis)
    {
        bootstrap.hash = genesis.GetHash().asVector();
        // intentionally leave prevHash empty
        bootstrap.height = 0;
        bootstrap.timestamp = genesis.GetBlockTime();
    }

    altintegration::AltBlock getBootstrapBlock() const noexcept override
    {
        return bootstrap;
    }

    int64_t getIdentifier() const noexcept override
    {
        return VeriBlock::ALT_CHAIN_ID;
    }

    std::vector<uint8_t> getHash(const std::vector<uint8_t>& bytes) const noexcept override;

    // we should verify:
    // - check that 'bytes' can be deserialized to a CBlockHeader
    // - check that this CBlockHeader is valid (time, pow, version...)
    // - check that 'root' is equal to Merkle Root in CBlockHeader
    bool checkBlockHeader(
        const std::vector<uint8_t>& bytes,
        const std::vector<uint8_t>& root, altintegration::ValidationState& state) const noexcept override;

    altintegration::AltBlock bootstrap;
};

struct AltChainParamsVBTCRegTest : public AltChainParamsVBTC {
    ~AltChainParamsVBTCRegTest() = default;

    AltChainParamsVBTCRegTest(const CBlock& genesis) : AltChainParamsVBTC(genesis)
    {
        mMaxReorgDistance = 1000;
    }
};

struct AltChainParamsVBTCDetRegTest : public AltChainParamsVBTC {
    ~AltChainParamsVBTCDetRegTest() = default;

    AltChainParamsVBTCDetRegTest()
    {
        bootstrap.hash = uint256S("393e1fea789a3ac750921d6d9f6aa7e84df9e031ecd63fca22dc3adc0632025c").asVector();
        bootstrap.previousBlock = uint256S("42b16a400669ab1403410585e793cec350baa53ff3fddc2414be92f03f1b12f2").asVector();
        bootstrap.height = 1000;
        // intentionally leave timestamp empty
    }
};

void printConfig(const altintegration::Config& config);
void selectPopConfig(const std::string& network = "test");

} // namespace VeriBlock

#endif //INTEGRATION_REFERENCE_BTC_VBTC_PARAMS_HPP
