// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/check.h>

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

#include <clientversion.h>
#include <tinyformat.h>

#include <cstdio>
#include <cstdlib>
#include <string>


NonFatalCheckError::NonFatalCheckError(const char* msg, const char* file, int line, const char* func)
    : std::runtime_error{
          strprintf("Internal bug detected: \"%s\"\n%s:%d (%s)\n"
                    "%s %s\n"
                    "Please report this issue here: %s\n",
                    msg, file, line, func, PACKAGE_NAME, FormatFullVersion(), PACKAGE_BUGREPORT)}
{
}

void assertion_fail(const char* file, int line, const char* func, const char* assertion)
{
    auto str = strprintf("%s:%s %s: Assertion `%s' failed.\n", file, line, func, assertion);
    fwrite(str.data(), 1, str.size(), stderr);
    std::abort();
}
