// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "timer.h"

using namespace std;


bool CTimerList::addTimer(CTimer t)
{
    if (fShutdown)
        return false;

    if (mapTimers.count(t.expire_time))
        return false;

    mapTimers[t.expire_time] = t;
    return true;
}

unsigned int CTimerList::runTimers()
{
    int64 nNow = GetTime();
    unsigned int nRun = 0;

    map<int64, CTimer>::iterator miLast = mapTimers.end();
    vector<CTimer> vAdd;

    // execute any expired timers
    for (map<int64, CTimer>::iterator mi = mapTimers.begin();
         mi != mapTimers.end(); ++mi) {
        CTimer& t = (*mi).second;
        if (t.expire_time > nNow)
            break;

        nRun++;
        miLast = mi;

        t.actor(&t, t.privdata);
        if (t.expire_time > nNow)
            vAdd.push_back(t);
    }

    // delete executed timers
    if (miLast != mapTimers.end()) {
        miLast++;
        mapTimers.erase(mapTimers.begin(), miLast);
    }

    // re-add timers with new expired times
    for (unsigned int i = 0; i < vAdd.size(); i++)
        addTimer(vAdd[i]);

    return nRun;
}
