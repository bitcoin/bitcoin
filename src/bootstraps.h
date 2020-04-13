#ifndef __BOOTSTRAPS_BTC_VBK
#define __BOOTSTRAPS_BTC_VBK

#include <string>
#include <vector>

#include <veriblock/config.hpp>

extern int testnetVBKstartHeight;
extern std::vector<std::string> testnetVBKblocks;

extern int testnetBTCstartHeight;
extern std::vector<std::string> testnetBTCblocks;

std::shared_ptr<altintegration::Config> makeConfig(std::string btcnet, std::string vbknet);

#endif