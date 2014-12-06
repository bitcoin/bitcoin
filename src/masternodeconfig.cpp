#include "masternodeconfig.h"
#include "util.h"

CMasternodeConfig masternodeConfig;

void CMasternodeConfig::add(std::string alias, std::string ip, std::string privKey, std::string txHash, std::string outputIndex) {
	CMasternodeEntry cme(alias, ip, privKey, txHash, outputIndex);
	entries.push_back(cme);
}

void CMasternodeConfig::read(boost::filesystem::path path) {
	boost::filesystem::ifstream streamConfig(GetMasternodeConfigFile());
	if (!streamConfig.good()) {
		return; // No masternode.conf file is OK
	}

	for(std::string line; std::getline(streamConfig, line); )
	{
		if(line.empty()) {
			continue;
		}
		std::istringstream iss(line);
        std::string alias, ip, privKey, txHash, outputIndex;
        if (!(iss >> alias >> ip >> privKey >> txHash >> outputIndex)) {
        	LogPrintf("CMasternodeConfig::read - Could not parse masternode.conf. Line: %s\n", line.c_str());
        	continue;
        }
        add(alias, ip, privKey, txHash, outputIndex);
	}

	streamConfig.close();
}
