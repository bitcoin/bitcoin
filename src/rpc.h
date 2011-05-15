// Copyright (c) 2010 Satoshi Nakamoto
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.

#ifndef RPC_H
#define RPC_H

void ThreadRPCServer(void* parg);
int CommandLineRPC(int argc, char *argv[]);

#endif // !RPC_H
