// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/json.h>

#include <univalue.h>
#include <util/check.h>

#include <string_view>

UniValue read_json(std::string_view jsondata)
{
    UniValue v;
    Assert(v.read(jsondata) && v.isArray());
    return v.get_array();
}
