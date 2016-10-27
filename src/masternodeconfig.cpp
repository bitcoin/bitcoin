
#include "net.h"
#include "masternodeconfig.h"
#include "util.h"
#include "ui_interface.h"
#include <base58.h>

CThroneConfig masternodeConfig;

void CThroneConfig::add(std::string alias, std::string ip, std::string privKey, std::string txHash, std::string outputIndex) {
    CThroneEntry cme(alias, ip, privKey, txHash, outputIndex);
    entries.push_back(cme);
}

bool CThroneConfig::read(std::string& strErr) {
    int linenumber = 1;
    boost::filesystem::path pathThroneConfigFile = GetThroneConfigFile();
    boost::filesystem::ifstream streamConfig(pathThroneConfigFile);

    if (!streamConfig.good()) {
        FILE* configFile = fopen(pathThroneConfigFile.string().c_str(), "a");
        if (configFile != NULL) {
            std::string strHeader = "# Throne config file\n"
                          "# Format: alias IP:port masternodeprivkey collateral_output_txid collateral_output_index\n"
                          "# Example: mn1 127.0.0.2:19340 93HaYBVUCYjEMeeH1Y4sBGLALQZE1Yc1K64xiqgX37tGBDQL8Xg 2bcd3c84c84f87eaa86e4e56834c92927a07f9e18718810b92e0d0324456a67c 0\n";
            fwrite(strHeader.c_str(), std::strlen(strHeader.c_str()), 1, configFile);
            fclose(configFile);
        }
        return true; // Nothing to read, so just return
    }

    for(std::string line; std::getline(streamConfig, line); linenumber++)
    {
        if(line.empty()) continue;

        std::istringstream iss(line);
        std::string comment, alias, ip, privKey, txHash, outputIndex;

        if (iss >> comment) {
            if(comment.at(0) == '#') continue;
            iss.str(line);
            iss.clear();
        }

        if (!(iss >> alias >> ip >> privKey >> txHash >> outputIndex)) {
            iss.str(line);
            iss.clear();
            if (!(iss >> alias >> ip >> privKey >> txHash >> outputIndex)) {
                strErr = _("Could not parse masternode.conf") + "\n" +
                        strprintf(_("Line: %d"), linenumber) + "\n\"" + line + "\"";
                streamConfig.close();
                return false;
            }
        }

        if(Params().NetworkID() == CBaseChainParams::MAIN) {
            if(CService(ip).GetPort() != 9340) {
                strErr = _("Invalid port detected in masternode.conf") + "\n" +
                        strprintf(_("Line: %d"), linenumber) + "\n\"" + line + "\"" + "\n" +
                        _("(must be 9340 for mainnet)");
                streamConfig.close();
                return false;
            }
        } else if(CService(ip).GetPort() == 9340) {
            strErr = _("Invalid port detected in masternode.conf") + "\n" +
                    strprintf(_("Line: %d"), linenumber) + "\n\"" + line + "\"" + "\n" +
                    _("(9340 could be used only on mainnet)");
            streamConfig.close();
            return false;
        }


        add(alias, ip, privKey, txHash, outputIndex);
    }

    streamConfig.close();
    return true;
}
