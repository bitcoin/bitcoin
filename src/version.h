// Copyright (c) 2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_VERSION_H
#define BITCOIN_VERSION_H

#include "clientversion.h"
#include <string>

//
// client versioning
//

static const int CLIENT_VERSION =
                           1000000 * CLIENT_VERSION_MAJOR
                         +   10000 * CLIENT_VERSION_MINOR
                         +     100 * CLIENT_VERSION_REVISION
                         +       1 * CLIENT_VERSION_BUILD;

extern const std::string CLIENT_NAME;
extern const std::string CLIENT_BUILD;
extern const std::string CLIENT_DATE;

//
// database format versioning
//
static const int DATABASE_VERSION = 70507;

//
// network protocol versioning
//

static const int PROTOCOL_VERSION = 60019;

// earlier versions not supported and disconnected
static const int MIN_PROTO_VERSION = 209;

// nTime field added to CAddress, starting with this version;
// if possible, avoid requesting addresses nodes older than this
static const int CADDR_TIME_VERSION = 31402;

// only request blocks from nodes outside this range of versions
static const int NOBLKS_VERSION_START = 60002;
static const int NOBLKS_VERSION_END = 60006;

#define DISPLAY_VERSION_MAJOR       0
#define DISPLAY_VERSION_MINOR       0
#define DISPLAY_VERSION_REVISION    9

#endif
