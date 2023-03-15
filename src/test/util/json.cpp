// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/json.h>

#include <string>
#include <util/check.h>

#include <univalue.h>

UniValue read_json(const std::string& jsondata)
{
    UniValue v;
    Assert(v.read(jsondata) && v.isArray());
    return v.get_array();
}
