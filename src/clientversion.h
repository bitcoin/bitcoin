// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2014-2018 The Syscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SYSCOIN_CLIENTVERSION_H
#define SYSCOIN_CLIENTVERSION_H

#if defined(HAVE_CONFIG_H)
#include "config/syscoin-config.h"
#endif //HAVE_CONFIG_H

// Check that required client information is defined
#if !defined(CLIENT_VERSION_MAJOR) || !defined(CLIENT_VERSION_MINOR) || !defined(CLIENT_VERSION_REVISION) || !defined(CLIENT_VERSION_BUILD) || !defined(CLIENT_VERSION_IS_RELEASE) || !defined(COPYRIGHT_YEAR)
#error Client version information missing: version is not defined by syscoin-config.h or in any other way
#endif

#if !defined(DASH_VERSION_MAJOR) || !defined(DASH_VERSION_MINOR) || !defined(DASH_VERSION_REVISION) 
#error Dash version information missing: version is not defined by syscoin-config.h or in any other way
#endif

#define BUILD_VERSION(maj, min, rev, build) \
    DO_STRINGIZE(maj) "." DO_STRINGIZE(min) "." DO_STRINGIZE(rev) "." DO_STRINGIZE(build)
#define BUILD_DASH_VERSION(maj, min, rev) \
    DO_STRINGIZE(maj) "." DO_STRINGIZE(min) "." DO_STRINGIZE(rev)


#define SYSCOIN_VERSION BUILD_VERSION(CLIENT_VERSION_MAJOR, CLIENT_VERSION_MINOR, CLIENT_VERSION_REVISION, CLIENT_VERSION_BUILD)
#define DASH_VERSION BUILD_DASH_VERSION(DASH_VERSION_MAJOR, DASH_VERSION_MINOR, DASH_VERSION_REVISION)

/**
 * Converts the parameter X to a string after macro replacement on X has been performed.
 * Don't merge these into one macro!
 */
#define STRINGIZE(X) DO_STRINGIZE(X)
#define DO_STRINGIZE(X) #X

//! Copyright string used in Windows .rc files
#define COPYRIGHT_STR "2009-" STRINGIZE(COPYRIGHT_YEAR) " The Bitcoin Core Developers, " \
	"2014-" STRINGIZE(COPYRIGHT_YEAR) " The Dash Core Developers, " \
        "2014-" STRINGIZE(COPYRIGHT_YEAR) " The Syscoin Core Developers"

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
static const int CLIENT_VERSION =
                           1000000 * CLIENT_VERSION_MAJOR
                         +   10000 * CLIENT_VERSION_MINOR
                         +     100 * CLIENT_VERSION_REVISION
                         +       1 * CLIENT_VERSION_BUILD;

extern const std::string CLIENT_NAME;
extern const std::string CLIENT_BUILD;
extern const std::string CLIENT_DATE;


std::string FormatFullVersion();
std::string FormatDashVersion();
std::string FormatSubVersion(const std::string& name, int nClientVersion, const std::vector<std::string>& comments);

#endif // WINDRES_PREPROC

#endif // SYSCOIN_CLIENTVERSION_H
