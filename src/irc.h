// Copyright (c) 2009-2010 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_IRC_H
#define BITCOIN_IRC_H

bool RecvLine(SOCKET hSocket, std::string& strLine);
void ThreadIRCSeed(void* parg);

extern int nGotIRCAddresses;
extern bool fGotExternalIP;

#endif
