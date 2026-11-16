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
#ifdef WIN32
    char buf[1024];
    const char *s = nullptr;

    if (strerror_s(buf, sizeof(buf), err) == 0) s = buf;

    if (s != nullptr) {
        return strprintf("%s (%d)", s, err);
    } else {
        return strprintf("Unknown error (%d)", err);
    }
#else
    return strprintf("%s (%d)", std::system_category().message(err), err);
#endif
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
