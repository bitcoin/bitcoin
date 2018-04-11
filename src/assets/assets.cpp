// Copyright (c) 2017 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "assets.h"

#include <amount.h>

#include <cmath>
#include <regex>
#include <string>

static const std::regex ASSET_NAME_REGEX("[A-Za-z_]{3,}");

// Does static checking (length, characters, etc.)
bool IsAssetNameValid(const std::string& name) {
    return std::regex_match(name, ASSET_NAME_REGEX);
}

// 1, 10, 100 ... COIN
// (0.00000001, 0.0000001, ... 1)
bool IsAssetUnitsValid(const CAmount& units) {
    for (int i = 1; i <= COIN; i *= 10) {
        if (units == i) return true;
    }
    return false;
}
