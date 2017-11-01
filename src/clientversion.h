// Copyright (c) 2009-2015 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_CLIENTVERSION_H
#define SYSCOIN_CLIENTVERSION_H

#if defined(HAVE_CONFIG_H)
#include "config/syscoin-config.h"
#else

/**
 * client versioning and copyright year
 */

//! These need to be macros, as clientversion.cpp's and syscoin*-res.rc's voodoo requires it
#define CLIENT_VERSION_MAJOR 2
#define CLIENT_VERSION_MINOR 1
#define CLIENT_VERSION_REVISION 6
#define CLIENT_VERSION_BUILD 0

#define BITCOIN_VERSION_MAJOR 0
#define BITCOIN_VERSION_MINOR 13
#define BITCOIN_VERSION_REVISION 2
#define BITCOIN_VERSION_BUILD 0

//! Set to true for release, false for prerelease or test build
#define CLIENT_VERSION_IS_RELEASE true

/**
 * Copyright year (2009-this)
 * Todo: update this when changing our copyright comments in the source
 */
#define COPYRIGHT_YEAR 2016

#endif //HAVE_CONFIG_H

#define BUILD_VERSION(maj, min, rev, build) \
    DO_STRINGIZE(maj) "." DO_STRINGIZE(min) "." DO_STRINGIZE(rev) "." DO_STRINGIZE(build)

#define SYSCOIN_VERSION BUILD_VERSION(CLIENT_VERSION_MAJOR, CLIENT_VERSION_MINOR, CLIENT_VERSION_REVISION, CLIENT_VERSION_BUILD)
#define BITCOIN_VERSION BUILD_VERSION(BITCOIN_VERSION_MAJOR, BITCOIN_VERSION_MINOR, BITCOIN_VERSION_REVISION, BITCOIN_VERSION_BUILD)


/**
 * Converts the parameter X to a string after macro replacement on X has been performed.
 * Don't merge these into one macro!
 */
#define STRINGIZE(X) DO_STRINGIZE(X)
#define DO_STRINGIZE(X) #X

//! Copyright string used in Windows .rc files
#define COPYRIGHT_STR "2009-" STRINGIZE(COPYRIGHT_YEAR) " " COPYRIGHT_HOLDERS_FINAL

/**
 * syscoind-res.rc includes this file, but it cannot cope with real c++ code.
 * WINDRES_PREPROC is defined to indicate that its pre-processor is running.
 * Anything other than a define should be guarded below.
 */

#if !defined(WINDRES_PREPROC)

#include <string>
#include <vector>
// SYSCOIN
static const std::string SYSCOIN_CLIENT_VERSION = SYSCOIN_VERSION;
static const std::string BITCOIN_CLIENT_VERSION = BITCOIN_VERSION;
static const int CLIENT_VERSION =
                           1000000 * CLIENT_VERSION_MAJOR
                         +   10000 * CLIENT_VERSION_MINOR
                         +     100 * CLIENT_VERSION_REVISION
                         +       1 * CLIENT_VERSION_BUILD;

extern const std::string CLIENT_NAME;
extern const std::string CLIENT_BUILD;


std::string FormatFullVersion();
std::string FormatBitcoinVersion();
std::string FormatSubVersion(const std::string& name, int nClientVersion, const std::vector<std::string>& comments);

#endif // WINDRES_PREPROC

#endif // SYSCOIN_CLIENTVERSION_H
