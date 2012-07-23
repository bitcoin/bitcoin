// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_IRC_H
#define BITCOIN_IRC_H

void ThreadIRCSeed(void* parg);

extern int nGotIRCAddresses;
extern bool fGotExternalIP;

#endif
