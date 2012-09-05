// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef __BITCOIN_TIMER_H__
#define __BITCOIN_TIMER_H__


#include "util.h"


class CTimer {
public:
    void *privdata;
    int64 expire_time;

    void (*actor)(CTimer *t, void *p);
};


class CTimerList {
private:
    std::map<int64, CTimer> mapTimers;

public:
    unsigned int runTimers();
    bool addTimer(CTimer t);
    int64 nextExpire() {
        std::map<int64, CTimer>::iterator mi = mapTimers.begin();
        return (*mi).first;
    }

    unsigned int size() { return mapTimers.size(); }
    void clear() { mapTimers.clear(); }
};


#endif // __BITCOIN_TIMER_H__
