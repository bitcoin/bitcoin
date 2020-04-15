#ifndef __BOOTSTRAPS_BTC_VBK
#define __BOOTSTRAPS_BTC_VBK

#include <string>
#include <vector>

#include "chainparams.h"
#include <primitives/block.h>
#include <util/system.h> // for gArgs
#include <veriblock/config.hpp>

extern int testnetVBKstartHeight;
extern std::vector<std::string> testnetVBKblocks;

extern int testnetBTCstartHeight;
extern std::vector<std::string> testnetBTCblocks;

struct AltChainParamsVBTC : public altintegration::AltChainParams {
    ~AltChainParamsVBTC() override = default;

    AltChainParamsVBTC(const CBlock& genesis)
    {
        auto hash = genesis.GetHash();
        bootstrap.hash = std::vector<uint8_t>{hash.begin(), hash.end()};
        bootstrap.height = 0; // pop is enabled starting at genesis
        bootstrap.timestamp = genesis.GetBlockTime();
    }

    altintegration::AltBlock getBootstrapBlock() const noexcept override
    {
        return bootstrap;
    }

    uint32_t getIdentifier() const noexcept override
    {
        return 0x3ae6ca;
    }

    altintegration::AltBlock bootstrap;
};

void printConfig(const altintegration::Config& config);
void selectPopConfig(const ArgsManager& mgr);
void SelectPopConfig(
    std::string btcnet,
    std::string vbknet,
    bool popautoconfig = true,
    int btcstart = 0,
    std::string btcblocks = {},
    int vbkstart = 0,
    std::string vbkblocks = {});


#endif