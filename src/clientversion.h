// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CLIENTVERSION_H
#define BITCOIN_CLIENTVERSION_H

#if defined(HAVE_CONFIG_H)
<<<<<<< HEAD
#include "config/dash-config.h"
=======
#include "crowncoin-config.h"
>>>>>>> origin/dirty-merge-dash-0.11.0
#else

<<<<<<< HEAD
/**
 * client versioning and copyright year
 */

//! These need to be macros, as clientversion.cpp's and dash*-res.rc's voodoo requires it
#define CLIENT_VERSION_MAJOR 0
#define CLIENT_VERSION_MINOR 12
#define CLIENT_VERSION_REVISION 0
#define CLIENT_VERSION_BUILD 60
=======
// These need to be macros, as version.cpp's and crowncoin-qt.rc's voodoo requires it
#define CLIENT_VERSION_MAJOR       0
#define CLIENT_VERSION_MINOR       9
#define CLIENT_VERSION_REVISION    4
#define CLIENT_VERSION_BUILD       0
>>>>>>> origin/dirty-merge-dash-0.11.0

//! Set to true for release, false for prerelease or test build
#define CLIENT_VERSION_IS_RELEASE true

<<<<<<< HEAD
/**
 * Copyright year (2009-this)
 * Todo: update this when changing our copyright comments in the source
 */
=======
// Copyright year (2009-this)
// Todo: update this when changing our copyright comments in the source
>>>>>>> origin/dirty-merge-dash-0.11.0
#define COPYRIGHT_YEAR 2016

#endif //HAVE_CONFIG_H

/**
 * Converts the parameter X to a string after macro replacement on X has been performed.
 * Don't merge these into one macro!
 */
#define STRINGIZE(X) DO_STRINGIZE(X)
#define DO_STRINGIZE(X) #X

//! Copyright string used in Windows .rc files
#define COPYRIGHT_STR "2009-" STRINGIZE(COPYRIGHT_YEAR) " The Bitcoin Core Developers, 2014-" STRINGIZE(COPYRIGHT_YEAR) " The Dash Core Developers"

/**
 * dashd-res.rc includes this file, but it cannot cope with real c++ code.
 * WINDRES_PREPROC is defined to indicate that its pre-processor is running.
 * Anything other than a define should be guarded below.
 */

#if !defined(WINDRES_PREPROC)

#include <string>
#include <vector>

static const int CLIENT_VERSION =
                           1000000 * CLIENT_VERSION_MAJOR
                         +   10000 * CLIENT_VERSION_MINOR
                         +     100 * CLIENT_VERSION_REVISION
                         +       1 * CLIENT_VERSION_BUILD;

extern const std::string CLIENT_NAME;
extern const std::string CLIENT_BUILD;
extern const std::string CLIENT_DATE;


std::string FormatFullVersion();
std::string FormatSubVersion(const std::string& name, int nClientVersion, const std::vector<std::string>& comments);

#endif // WINDRES_PREPROC

#endif // BITCOIN_CLIENTVERSION_H
