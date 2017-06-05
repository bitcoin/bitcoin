// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2015-2017 The Bitcoin Unlimited developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BANDB_H
#define BITCOIN_BANDB_H

#include "banentry.h" // for banmap_t

#include <boost/filesystem.hpp>

/** Access to the banlist database (banlist.dat) */
class CBanDB
{
private:
    boost::filesystem::path pathBanlist;

public:
    CBanDB();
    bool Write(const banmap_t &banSet);
    bool Read(banmap_t &banSet);

    // NOTE: Added for use in unit testing
    boost::filesystem::path GetDatabasePath() const { return pathBanlist; }
};

#endif // BITCOIN_BANDB_H
