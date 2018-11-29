// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2014-2018 The Crown developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "net.h"
#include "nodeconfig.h"
#include "util.h"
#include "ui_interface.h"
#include <base58.h>

void CNodeConfig::add(std::string alias, std::string ip, std::string privKey, std::string txHash, std::string outputIndex) {
    CNodeEntry cme(alias, ip, privKey, txHash, outputIndex);
    entries.push_back(cme);
}

void CNodeConfig::add(CNodeEntry cne) {
    entries.push_back(cne);
}

bool CNodeConfig::read(std::string& strErr) {
    int linenumber = 1;
    boost::filesystem::path pathNodeConfigFile = getNodeConfigFile();
    boost::filesystem::ifstream streamConfig(pathNodeConfigFile);

    if (!streamConfig.good()) {
        FILE* configFile = fopen(pathNodeConfigFile.string().c_str(), "a");
        if (configFile != NULL) {
            fwrite(getHeader().c_str(), std::strlen(getHeader().c_str()), 1, configFile);
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
                strErr = _("Could not parse ") + getFileName() + "\n" +
                        strprintf(_("Line: %d"), linenumber) + "\n\"" + line + "\"";
                streamConfig.close();
                return false;
            }
        }

        if(Params().NetworkID() == CBaseChainParams::MAIN) {
            if(CService(ip).GetPort() != 9340) {
                strErr = _("Invalid port detected in ") + getFileName() + "\n" +
                        strprintf(_("Line: %d"), linenumber) + "\n\"" + line + "\"" + "\n" +
                        _("(must be 9340 for mainnet)");
                streamConfig.close();
                return false;
            }
        } else if(CService(ip).GetPort() == 9340) {
            strErr = _("Invalid port detected in ") + getFileName() + "\n" +
                    strprintf(_("Line: %d"), linenumber) + "\n\"" + line + "\"" + "\n" +
                    _("(9340 could be used only on mainnet)");
            streamConfig.close();
            return false;
        }
        if (Params().NetworkID() != CBaseChainParams::DEVNET && !(CService(ip).IsIPv4() && CService(ip).IsRoutable())) {
            strErr = _("Invalid Address detected in ") + getFileName() + "\n" +
                    strprintf(_("Line: %d"), linenumber) + "\n\"" + line + "\"" + "\n" +
                    _("(IPV4 ONLY)");
            streamConfig.close();
            return false;
        }


        add(alias, ip, privKey, txHash, outputIndex);
    }

    streamConfig.close();
    return true;
}

bool CNodeConfig::aliasExists(const std::string& alias)
{
    BOOST_FOREACH(CNodeEntry mne, getEntries()) {
        if (mne.getAlias() == alias)
        {
            return true;
        }
    }
    return false;
}

bool CNodeConfig::write()
{
    boost::filesystem::path pathNodeConfigFile = getNodeConfigFile();
    boost::filesystem::ofstream streamConfig(pathNodeConfigFile, std::ofstream::out);
    streamConfig << getHeader() << "\n";
    BOOST_FOREACH(CNodeEntry sne, getEntries()) {
        streamConfig << sne.getAlias() << " " << sne.getIp() << " " << sne.getPrivKey() << " " << sne.getTxHash() << " " << sne.getOutputIndex() << "\n";
    }
    streamConfig.close();
    return true;
}
