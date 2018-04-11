// Copyright (c) 2017 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef RAVENCOIN_ASSET_PROTOCOL_H
#define RAVENCOIN_ASSET_PROTOCOL_H

#include <amount.h>

#include <string>

bool IsAssetNameValid(const std::string& name);

bool IsAssetUnitsValid(const CAmount& units);

#endif //RAVENCOIN_ASSET_PROTOCOL_H
