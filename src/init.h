// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INIT_H
#define BITCOIN_INIT_H

#include <string>
#include <stdint.h>

class Credits_CDBEnv;
class Credits_CWallet;
class Bitcoin_CDBEnv;
class Bitcoin_CWallet;

namespace boost {
    class thread_group;
};

//extern uint64_t bitcoin_nAccountingEntryNumber;
//extern Bitcoin_CDBEnv bitcoin_bitdb;
extern Bitcoin_CWallet* bitcoin_pwalletMain;

extern uint64_t bitcredit_nAccountingEntryNumber;
extern Credits_CDBEnv bitcredit_bitdb;
extern Credits_CWallet* bitcredit_pwalletMain;

extern uint64_t deposit_nAccountingEntryNumber;
extern Credits_CDBEnv deposit_bitdb;
extern Credits_CWallet* deposit_pwalletMain;


void StartShutdown();
bool ShutdownRequested();
void Shutdown();
bool AppInit2(boost::thread_group& threadGroup);

/* The help message mode determines what help message to show */
enum HelpMessageMode
{
    HMM_BITCOIND,
    HMM_BITCOIN_QT
};

std::string HelpMessage(HelpMessageMode mode);

#endif
