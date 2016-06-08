
#include "net.h"
#include "throneconfig.h"
#include "util.h"
#include <base58.h>

CThroneConfig throneConfig;

void CThroneConfig::add(std::string alias, std::string ip, std::string privKey, std::string txHash, std::string outputIndex, std::string donationAddress, std::string donationPercent) {
    CThroneEntry cme(alias, ip, privKey, txHash, outputIndex, donationAddress, donationPercent);
    entries.push_back(cme);
}

bool CThroneConfig::read(std::string& strErr) {
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
        std::string alias, ip, privKey, txHash, outputIndex, donation, donationAddress, donationPercent;
        if (!(iss >> alias >> ip >> privKey >> txHash >> outputIndex >> donation)) {
            donationAddress = "";
            donationPercent = "";
            iss.str(line);
            iss.clear();
            if (!(iss >> alias >> ip >> privKey >> txHash >> outputIndex)) {
                strErr = "Could not parse throne.conf line: " + line;
                streamConfig.close();
                return false;
            }
        } else {
            size_t pos = donation.find_first_of(":");
            if(pos == string::npos) { // no ":" found
                donationPercent = "100";
                donationAddress = donation;
            } else {
                donationPercent = donation.substr(pos + 1);
                donationAddress = donation.substr(0, pos);
            }
            CCrowncoinAddress address(donationAddress);
            if (!address.IsValid()) {
                strErr = "Invalid Dash address in throne.conf line: " + line;
                streamConfig.close();
                return false;
            }
        }

        if(Params().NetworkID() == CChainParams::MAIN){
            if(CService(ip).GetPort() != 9999) {
                strErr = "Invalid port detected in throne.conf: " + line + " (must be 9999 for mainnet)";
                streamConfig.close();
                return false;
            }
        } else if(CService(ip).GetPort() == 9999) {
            strErr = "Invalid port detected in throne.conf: " + line + " (9999 must be only on mainnet)";
            streamConfig.close();
            return false;
        }


        add(alias, ip, privKey, txHash, outputIndex, donationAddress, donationPercent);
    }

    streamConfig.close();
    return true;
}
