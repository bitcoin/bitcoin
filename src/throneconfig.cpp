
#include "net.h"
#include "throneconfig.h"
#include "util.h"
#include <base58.h>

CThroneConfig throneConfig;

void CThroneConfig::add(std::string alias, std::string ip, std::string privKey, std::string txHash, std::string outputIndex) {
    CThroneEntry cme(alias, ip, privKey, txHash, outputIndex);
    entries.push_back(cme);
}

bool CThroneConfig::read(boost::filesystem::path path) {
    boost::filesystem::ifstream streamConfig(GetThroneConfigFile());
    if (!streamConfig.good()) {
        return true; // No throne.conf file is OK
    }

    for(std::string line; std::getline(streamConfig, line); )
    {
        if(line.empty()) {
            continue;
        }
        std::istringstream iss(line);
        std::string alias, ip, privKey, txHash, outputIndex;
        if (!(iss >> alias >> ip >> privKey >> txHash >> outputIndex)) {
            LogPrintf( "Could not parse throne.conf line: %s\n", line.c_str());
            streamConfig.close();
            return false;
        }


        add(alias, ip, privKey, txHash, outputIndex);
    }

    streamConfig.close();
    return true;
}
