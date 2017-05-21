// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2016-2017 The Bitcoin Unlimited developers

#ifndef BITCOIN_CONNMGR_H
#define BITCOIN_CONNMGR_H

#include "net.h"

class CConnMgr
{
public:
    static NodeId nextNodeId();
};

extern std::unique_ptr<CConnMgr> connmgr;

#endif
