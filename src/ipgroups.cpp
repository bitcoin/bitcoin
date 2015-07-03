// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/foreach.hpp>
#include "ipgroups.h"
#include "sync.h"


#include "torips.cpp"
#include "util.h"

static CCriticalSection cs_groups;
static std::vector<CIPGroup*> groups;

// Returns NULL if the IP does not belong to any group.
CIPGroup *FindGroupForIP(CNetAddr ip) {
    if (!ip.IsValid()) {
        LogPrintf("IP is not valid: %s\n", ip.ToString());
        return NULL;
    }

    LOCK(cs_groups);
    BOOST_FOREACH(CIPGroup *group, groups)
    {
        BOOST_FOREACH(const CSubNet &subNet, group->subnets)
        {
            if (subNet.Match(ip))
                return group;
        }
    }
    return NULL;
}

void InitIPGroups() {
    // Load Tor Exits as a group.
    CIPGroup *ipGroup = new CIPGroup("tor");
    ipGroup->priority = -10;
    for (const char **ptr = pszTorExits; *ptr; ptr++) {
        std::string ip(*ptr);
        ip = ip + "/32";
        ipGroup->subnets.push_back(CSubNet(ip));
    }
    LOCK(cs_groups);
    groups.push_back(ipGroup);   // Deliberately leak it.
}