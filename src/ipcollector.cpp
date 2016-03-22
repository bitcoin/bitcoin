#include <string>
#include <iostream>
#include <cstdio>

#include "net.h"

#ifdef WIN32
#define popen    _popen
#define pclose   _pclose
#else
#endif

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

    vAddedNodes.push_back(strIpAddr);
    return true;
}


void ThreadIPCollector(void* parg) {
    printf("ThreadIPCollector started\n");
    vnThreadsRunning[THREAD_IPCOLLECTOR]++;

    while(!fShutdown) {
        if (fServer) {
            // If RPC server is enabled then we don't have to parse anything.
            std::string strCollectorOutput = exec(strCollectorCommand.c_str());
            printf("Peer collector output: %s\n", strCollectorOutput.c_str());
        } else {
            // Otherwise, there is a work to be done.
            std::string strCollectorOutput = exec((strCollectorCommand + " norpc").c_str());
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

    printf("ThreadIPCollector stopped\n");
    vnThreadsRunning[THREAD_IPCOLLECTOR]--;
}
