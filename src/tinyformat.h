// Copyright (c) 2014-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TINYFORMAT_H
#define BITCOIN_TINYFORMAT_H

#include <stdexcept>
#include <sstream>
#include <string>

// configure tinyformat prior to inclusion
#define TINYFORMAT_USE_VARIADIC_TEMPLATES
#define TINYFORMAT_ERROR(reasonString) throw tinyformat::format_error(reasonString)

namespace tinyformat {
// must define format_error early to enable expansion of TINYFORMAT_ERROR in tinyformat.h
class format_error : public std::runtime_error
{
public:
    format_error(const std::string& what) : std::runtime_error(what) {}
};
} // namespace tinyformat

#include <tinyformat/tinyformat.h>

namespace tinyformat {
// must define format wrapper template after wrapped format is defined in tinyformat.h
template<typename... Args>
std::string format(const std::string& fmt, const Args&... args)
{
    std::ostringstream oss;
    format(oss, fmt.c_str(), args...);
    return oss.str();
}
} // namespace tinyformat

#define strprintf tinyformat::format

#endif
