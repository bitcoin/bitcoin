// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2014 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef ZMQPORTS_H
#define ZMQPORTS_H

#include <string>

class CBlock;
class CTransaction;

// Global state
extern bool fZMQPub;

void ZMQInitialize(const std::string &endp);
void ZMQPublishBlock(const CBlock &blk);
void ZMQPublishTransaction(const CTransaction &tx);
void ZMQShutdown();

#endif
