// Copyright (c) 2014-2021 The Dash Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_STACKTRACES_H
#define BITCOIN_STACKTRACES_H

#include <string>
#include <sstream>
#include <exception>

#include <cxxabi.h>

#include <tinyformat.h>

std::string DemangleSymbol(const std::string& name);

std::string GetPrettyExceptionStr(const std::exception_ptr& e);
std::string GetCrashInfoStrFromSerializedStr(const std::string& ciStr);

template<typename T>
std::string GetExceptionWhat(const T& e);

template<>
inline std::string GetExceptionWhat(const std::exception& e)
{
    return e.what();
}

// Default implementation
template<typename T>
inline std::string GetExceptionWhat(const T& e)
{
    std::ostringstream s;
    s << e;
    return s.str();
}

void RegisterPrettyTerminateHander();
void RegisterPrettySignalHandlers();

#endif//BITCOIN_STACKTRACES_H
