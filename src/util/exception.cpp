// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/exception.h>

#include <tinyformat.h>
#include <util/log.h>
#include <util/threadnames.h>

#include <exception>
#include <iostream>
#include <string>
#include <typeinfo>

void PrintExceptionContinue(
    const std::exception* pex,
    std::string_view context,
    std::source_location caught_at)
{
    std::string message = strprintf(
        "***** EXCEPTION thread: %s, caught at: %s:%u ",
        util::ThreadGetInternalName(), caught_at.file_name(), caught_at.line());
    if (!context.empty()) {
        message += strprintf("context: %s ", context);
    }
    if (pex) {
        message += strprintf("type: %s what: \"%s\"", typeid(*pex).name(), pex->what());
    } else {
        message += "unknown exception";
    }
    LogWarning("%s", message);
    tfm::format(std::cerr, "%s\n", message);
}
