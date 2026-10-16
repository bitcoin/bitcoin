// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/syserror.h>

#include <tinyformat.h>

#include <string>
#include <system_error>

#if defined(WIN32)
#include <windows.h>
#endif

std::string SysErrorString(int err)
{
    return strprintf("%s (%d)", std::system_category().message(err), err);
}

#if defined(WIN32)
std::string Win32ErrorString(int err)
{
    char buf[256];
    buf[0] = 0;
    if (FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK,
                       nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       buf, ARRAYSIZE(buf), nullptr)) {
        return strprintf("%s (%d)", buf, err);
    } else {
        return strprintf("Unknown error (%d)", err);
    }
}
#endif
