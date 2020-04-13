#ifndef __BOOTSTRAPS_BTC_VBK
#define __BOOTSTRAPS_BTC_VBK

#include <string>
#include <vector>

#include <veriblock/config.hpp>
#include <util/system.h> // for gArgs

extern int testnetVBKstartHeight;
extern std::vector<std::string> testnetVBKblocks;

extern int testnetBTCstartHeight;
extern std::vector<std::string> testnetBTCblocks;

void printConfig(const altintegration::Config& config);
std::shared_ptr<altintegration::Config> makeConfig(const ArgsManager& mgr);

#endif