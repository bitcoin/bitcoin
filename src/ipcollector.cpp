// Copyright (c) 2012-2016 The Novacoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or https://opensource.org/licenses/mit-license.php.

#ifdef WIN32
#include <winsock2.h>
#define popen    _popen
#define pclose   _pclose
#endif

#include "net.h"

std::string strCollectorCommand;

std::string exec(const char* cmd) {
    std::string result = "";
    char buffer[128];
    FILE *fp;

    fp = popen(cmd, "r");
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        result += buffer;
    }
    pclose(fp);
    return result;
}

bool AddPeer(std::string &strIpAddr) {
    LOCK(cs_vAddedNodes);
    std::vector<std::string>::iterator it = vAddedNodes.begin();
    for(; it != vAddedNodes.end(); it++) {
        if (strIpAddr == *it) break;
    }
    if (it != vAddedNodes.end())
        return false;

    printf("Adding node %s\n", strIpAddr.c_str());
    vAddedNodes.push_back(strIpAddr);

    return true;
}

void ThreadIPCollector(void* parg) {
    printf("ThreadIPCollector started\n");
    vnThreadsRunning[THREAD_IPCOLLECTOR]++;

    std::string strExecutableFilePath = "";
#ifdef MAC_OSX
    size_t nameEnd = strCollectorCommand.rfind(".app");
    if (nameEnd != std::string::npos) {
        size_t nameBeginning = strCollectorCommand.rfind("/");
        if (nameBeginning == std::string::npos)
            nameBeginning = 0;

        std::string strFileName = strCollectorCommand.substr(nameBeginning, nameEnd - nameBeginning);
        strExecutableFilePath = strCollectorCommand + "/Contents/MacOS/" + strFileName;
    }
    else
        strExecutableFilePath = strCollectorCommand;
#else
        strExecutableFilePath = strCollectorCommand;
#endif

    if (!strExecutableFilePath.empty())
    {
        while(!fShutdown) {
            if (fServer) {
                // If RPC server is enabled then we don't have to parse anything.
                std::string strCollectorOutput = exec(strExecutableFilePath.c_str());
                printf("Peer collector output: %s\n", strCollectorOutput.c_str());
            } else {
                // Otherwise, there is a work to be done.
                std::string strCollectorOutput = exec((strExecutableFilePath + " norpc").c_str());
                std::istringstream collectorStream(strCollectorOutput);

                std::string strIpAddr;
                while (std::getline(collectorStream, strIpAddr)) {
                    AddPeer(strIpAddr);
                }
            }

            int nSleepHours = 1 + GetRandInt(5); // Sleep for 1-6 hours.
            for (int i = 0; i < nSleepHours * 3600 && !fShutdown; i++)
                Sleep(1000);
        }
    }

    printf("ThreadIPCollector stopped\n");
    vnThreadsRunning[THREAD_IPCOLLECTOR]--;
}
