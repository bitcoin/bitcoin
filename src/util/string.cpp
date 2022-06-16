// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/string.h>

#include <boost/algorithm/string/replace.hpp>

void ReplaceAll(std::string& in_out, std::string_view search, std::string_view substitute)
{
    boost::replace_all(in_out, search, substitute);
}
