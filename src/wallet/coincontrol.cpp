// Copyright (c) 2011-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "coincontrol.h"

bool InputTypeFromString(const std::string& input_string, InputType& input_type) {
    static const std::map<std::string, InputType> input_types = {
        {"LEGACY", InputType::LEGACY},
        {"SEGWIT", InputType::SEGWIT},
        {"ALL", InputType::ALL},
    };
    auto it = input_types.find(input_string);
    if (it == input_types.end()) return false;
    input_type = it->second;
    return true;
}
