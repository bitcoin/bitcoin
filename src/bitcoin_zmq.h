// Copyright (c) 2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ZMQ_H
#define BITCOIN_ZMQ_H

#include "main.h"
void BZmq_InitCtx();
void BZmq_InitSockets();
void BZmq_CtxSetOptions(std::string&);
void BZmq_PubSetOptions(std::string&);
void BZmq_PubBind(std::string&);
void BZmq_PubConnect(std::string&);
void BZmq_RepSetOptions(std::string&);
void BZmq_RepBind(std::string&);
void BZmq_RepConnect(std::string&);
void BZmq_ThreadReqRep(void *);
void BZmq_Shutdown();
void BZmq_Version();
void BZmq_SendTX(CTransaction& tx);
void BZmq_SendBlock(CBlockIndex*);
void BZmq_SendIPAddress(const char *);

#endif
