// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2015-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_DOSMANAGER_H
#define BITCOIN_DOSMANAGER_H

class CDoSManager
{
};

// actual definition should be in globals.cpp for ordered construction/destruction
extern CDoSManager dosMan;

#endif // BITCOIN_DOSMANAGER_H
