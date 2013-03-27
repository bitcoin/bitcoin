// Copyright (c) 2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ZMQ_H
#define BITCOIN_ZMQ_H

#include "main.h"

void bz_InitCtx();
void bz_InitSockets();
void bz_CtxSetOptions(std::string&);
void bz_PubSetOptions(std::string&);
void bz_PubBind(std::string&);
void bz_PubConnect(std::string&);
void bz_Shutdown();
void bz_Version();
void bz_Send_TX(CTransaction& tx);
void bz_Send_Block(CBlockIndex*);
void bz_Send_IpAddress(const char *);

#endif
