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

/**
 * bitcoind-res.rc includes this file, but it cannot cope with real c++ code.
 * WINDRES_PREPROC is defined to indicate that its pre-processor is running.
 * Anything other than a define should be guarded below.
 */

#if !defined(WINDRES_PREPROC)

#include <cstdint>
#include <string>
#include <vector>

static const int CLIENT_VERSION =
                             10000 * CLIENT_VERSION_MAJOR
                         +     100 * CLIENT_VERSION_MINOR
                         +       1 * CLIENT_VERSION_BUILD;

extern const std::string UA_NAME;


std::string FormatFullVersion();
std::string FormatSubVersion(const std::string& name, int nClientVersion, const std::vector<std::string>& comments, bool base_name_only = false);

std::string CopyrightHolders(const std::string& strPrefix);

/** Returns licensing information (for -version) */
std::string LicenseInfo();

static constexpr int64_t SECONDS_PER_WEEK = 604800;
static constexpr int64_t SECONDS_PER_YEAR = 31558060;

static constexpr int POSIX_EPOCH_YEAR = 1970;
static constexpr int64_t DEFAULT_SOFTWARE_EXPIRY_OFFSET = 26784000;  // Around Nov 7
static constexpr int64_t DEFAULT_SOFTWARE_EXPIRY = ((COPYRIGHT_YEAR - POSIX_EPOCH_YEAR) * SECONDS_PER_YEAR) + (SECONDS_PER_YEAR * 2) + DEFAULT_SOFTWARE_EXPIRY_OFFSET;
extern int64_t g_software_expiry;

static constexpr int64_t SOFTWARE_EXPIRY_WARN_PERIOD = SECONDS_PER_WEEK * 4;

bool IsThisSoftwareExpired(int64_t nTime);

#endif // WINDRES_PREPROC

#endif // BITCOIN_CLIENTVERSION_H
