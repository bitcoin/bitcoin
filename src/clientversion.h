// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CLIENTVERSION_H
#define BITCOIN_CLIENTVERSION_H

#include <util/macros.h>

#include <bitcoin-build-config.h> // IWYU pragma: keep

// Check that required client information is defined
#if !defined(CLIENT_VERSION_MAJOR) || !defined(CLIENT_VERSION_MINOR) || !defined(CLIENT_VERSION_BUILD) || !defined(CLIENT_VERSION_IS_RELEASE) || !defined(COPYRIGHT_YEAR)
#error Client version information missing: version is not defined by bitcoin-build-config.h or in any other way
#endif

//! Copyright string used in Windows .rc files
#define COPYRIGHT_STR "2009-" STRINGIZE(COPYRIGHT_YEAR) " " COPYRIGHT_HOLDERS_FINAL

// Windows .rc files include this header, but they cannot cope with real C++ code.
#if !defined(RC_INVOKED)

#include <string>
#include <vector>

static const int CLIENT_VERSION =
                             10000 * CLIENT_VERSION_MAJOR
                         +     100 * CLIENT_VERSION_MINOR
                         +       1 * CLIENT_VERSION_BUILD;

extern const std::string UA_NAME;


std::string FormatFullVersion();
std::string FormatSubVersion(const std::string& name, int nClientVersion, const std::vector<std::string>& comments);

std::string CopyrightHolders(const std::string& strPrefix);

/** Returns licensing information (for -version) */
std::string LicenseInfo();

#endif // RC_INVOKED

#endif // BITCOIN_CLIENTVERSION_H
