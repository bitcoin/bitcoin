// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2016-2017 The Bitcoin Unlimited developers

#include <atomic>

#include "connmgr.h"


NodeId CConnMgr::nextNodeId()
{
    static std::atomic<NodeId> next;

    // Pre-increment; do not use zero
    return ++next;
}

std::unique_ptr<CConnMgr> connmgr(new CConnMgr);
