// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/syserror.h>

#include <tinyformat.h>

#include <string>
#include <system_error>

std::string SysErrorString(int err)
{
    return strprintf("%s (%d)", std::system_category().message(err), err);
}
