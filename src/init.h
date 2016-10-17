// Copyright (c) 2009-2010 Satoshi Nakamoto
<<<<<<< HEAD
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
=======
// Copyright (c) 2009-2013 The Crowncoin developers
// Distributed under the MIT/X11 software license, see the accompanying
>>>>>>> origin/dirty-merge-dash-0.11.0
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CROWNCOIN_INIT_H
#define CROWNCOIN_INIT_H

#include <string>

class CWallet;

namespace boost
{
class thread_group;
} // namespace boost

extern CWallet* pwalletMain;

void StartShutdown();
bool ShutdownRequested();
void Shutdown();
void PrepareShutdown();
bool AppInit2(boost::thread_group& threadGroup);

<<<<<<< HEAD
/** The help message mode determines what help message to show */
enum HelpMessageMode {
    HMM_BITCOIND,
    HMM_BITCOIN_QT
=======
/* The help message mode determines what help message to show */
enum HelpMessageMode
{
    HMM_CROWNCOIND,
    HMM_CROWNCOIN_QT
>>>>>>> origin/dirty-merge-dash-0.11.0
};

/** Help for options shared between UI and daemon (for -help) */
std::string HelpMessage(HelpMessageMode mode);
/** Returns licensing information (for -version) */
std::string LicenseInfo();

#endif // BITCOIN_INIT_H
